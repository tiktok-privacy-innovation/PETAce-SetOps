
// Copyright 2023 TikTok Pte. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>

#include "example.h"
#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "network/net_factory.h"
#include "solo/prng.h"

#include "setops/pjc/circuit_psi.h"
#include "setops/util/dummy_data_util.h"
#include "setops/util/time.h"

void circuit_psi_example(const std::string& config_path, const std::string& log_path, bool use_random_data,
        std::size_t intersection_size, std::size_t intersection_ratio) {
    auto start = petace::setops::clock_start();
    // 1. Read json config.
    std::ifstream in(config_path);
    nlohmann::json params = nlohmann::json::parse(in, nullptr, true);
    in.close();

    bool is_sender = params["common"]["is_sender"];
    FLAGS_alsologtostderr = 1;
    FLAGS_log_dir = log_path;
    std::string log_file_name;
    if (use_random_data) {
        log_file_name = std::string("ecdh_psi_") + (is_sender ? "sender_" : "receiver_") + "intersection_size_" +
                        std::to_string(intersection_size);
    } else {
        log_file_name = std::string("ecdh_psi_") + (is_sender ? "sender_" : "receiver_") + "from_file";
    }
    google::InitGoogleLogging(log_file_name.c_str());

    // 2. Connect net io.
    petace::network::NetParams net_params;
    net_params.remote_addr = params["network"]["address"];
    net_params.remote_port = params["network"]["remote_port"];
    net_params.local_port = params["network"]["local_port"];

    auto net = petace::network::NetFactory::get_instance().build(petace::network::NetScheme::SOCKET, net_params);

    // 3. Read keys and features from file or use randomly generated data.
    std::vector<std::string> keys;
    std::vector<std::vector<uint64_t>> features;

    if (use_random_data) {
        std::vector<std::string> common_keys;
        std::size_t data_size = intersection_ratio * intersection_size;

        auto prng_factory = petace::solo::PRNGFactory(petace::solo::PRNGScheme::SHAKE_128);
        std::vector<petace::setops::Byte> commom_seed(16, petace::setops::Byte(0));
        auto common_prng = prng_factory.create(commom_seed);
        auto unique_prng = prng_factory.create();

        petace::setops::generate_random_keys(*common_prng, intersection_size, "0", common_keys);
        petace::setops::generate_random_keys(*unique_prng, data_size - intersection_size, "0", keys);
        keys.insert(keys.begin(), common_keys.begin(), common_keys.end());

        std::vector<std::uint64_t> col_features;
        petace::setops::generate_random_features(*unique_prng, data_size, false, col_features);
        features.push_back(col_features);

        LOG(INFO) << "key: features: " << std::endl;
        for (std::size_t i = 0; i < features.size(); i++) {
            for (std::size_t j = 0; j < features[i].size(); j++) {
                LOG(INFO) << keys[j] << ": " << features[i][j] << " ";
            }
            LOG(INFO) << std::endl;
        }
    } else {
        LOG(INFO) << "Read from csv not supported.";
    }

    // 4. run circuit-psi.
    std::vector<std::vector<uint64_t>> output_shares;
    petace::setops::CircuitPSI psi;
    psi.init(net, params);
    psi.process(net, keys, features, output_shares);

    // 5. calculate circuit-psi results
    std::vector<std::vector<uint64_t>> recv_shares(
            output_shares.size(), std::vector<uint64_t>(output_shares[0].size()));
    if (is_sender) {
        for (std::size_t i = 0; i < output_shares.size(); i++) {
            net->send_data(output_shares[i].data(), output_shares[i].size() * sizeof(uint64_t));
        }
        for (std::size_t i = 0; i < output_shares.size(); i++) {
            net->recv_data(recv_shares[i].data(), output_shares[i].size() * sizeof(uint64_t));
        }
    } else {
        for (std::size_t i = 0; i < output_shares.size(); i++) {
            net->send_data(output_shares[i].data(), output_shares[i].size() * sizeof(uint64_t));
        }
        for (std::size_t i = 0; i < output_shares.size(); i++) {
            net->recv_data(recv_shares[i].data(), output_shares[i].size() * sizeof(uint64_t));
        }
    }

    for (std::size_t i = 0; i < output_shares.size(); i++) {
        for (std::size_t j = 0; j < output_shares[i].size(); j++) {
            if (i == 0) {
                recv_shares[i][j] ^= output_shares[i][j];
            } else {
                recv_shares[i][j] += output_shares[i][j];
            }
        }
    }

    auto&& log = COMPACT_GOOGLE_LOG_INFO;
    LOG(INFO) << "results: " << std::endl;
    for (std::size_t i = 0; i < recv_shares[0].size(); i++) {
        log.stream() << "\n";
        for (std::size_t j = 0; j < recv_shares.size(); j++) {
            log.stream() << recv_shares[j][i] << " ";
        }
    }

    // 6. calculate statistics information.
    std::size_t communication = net->get_bytes_sent();
    auto duration = static_cast<double>(petace::setops::time_from(start)) * 1.0 / 1000000.0;
    std::size_t remote_communication = 0;
    if (is_sender) {
        net->send_data(&communication, sizeof(communication));
        net->recv_data(&remote_communication, sizeof(remote_communication));
    } else {
        net->recv_data(&remote_communication, sizeof(remote_communication));
        net->send_data(&communication, sizeof(communication));
    }

    double self_comm = static_cast<double>(communication) * 1.0 / (1024 * 1024);
    double remote_comm = static_cast<double>(remote_communication) * 1.0 / (1024 * 1024);
    double total_comm = static_cast<double>(communication + remote_communication) * 1.0 / (1024 * 1024);

    LOG(INFO) << "-------------------------------";
    LOG(INFO) << (is_sender ? "Sender" : "Receiver");
    LOG(INFO) << (use_random_data ? "Use random data." : "Use input file.");
    LOG(INFO) << "Cardinality is " << output_shares.size() << std::endl;
    LOG(INFO) << "Total Communication is " << total_comm << "(" << self_comm << " + " << remote_comm << ")"
              << "MB." << std::endl;
    LOG(INFO) << "Total time is " << duration << " s.";

    google::ShutdownGoogleLogging();
}
