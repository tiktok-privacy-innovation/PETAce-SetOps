
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

#include "setops/data/csv_data_provider.h"
#include "setops/psi/kkrt_psi.h"
#include "setops/util/dummy_data_util.h"
#include "setops/util/time.h"

const std::size_t kBatchSize = 1 << 20;

void kkrt_psi_example(const std::string& config_path, const std::string& log_path, bool use_random_data,
        std::size_t intersection_size, std::size_t intersection_ratio) {
    auto start = petace::setops::clock_start();
    // 1. Read JSON config.
    std::ifstream in(config_path);
    nlohmann::json params = nlohmann::json::parse(in, nullptr, true);
    in.close();

    bool is_sender = params["common"]["is_sender"];
    FLAGS_alsologtostderr = 1;
    FLAGS_log_dir = log_path;
    std::string log_file_name;
    if (use_random_data) {
        log_file_name = std::string("kkrt_psi_") + (is_sender ? "sender_" : "receiver_") + "intersection_size_" +
                        std::to_string(intersection_size);
    } else {
        log_file_name = std::string("kkrt_psi_") + (is_sender ? "sender_" : "receiver_") + "from_file";
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
    } else {
        LOG(INFO) << "Read data from csv.";
        std::string input_path = params["data"]["input_file"];
        bool has_header = params["data"]["has_header"];
        std::size_t ids_num = params["common"]["ids_num"];
        petace::setops::CsvDataProvider csv(input_path, has_header, ids_num);
        csv.get_next_batch(kBatchSize, keys);
    }

    // 4. run kkrt-psi.
    std::vector<std::string> output_keys;
    petace::setops::KkrtPSI psi;
    psi.init(net, params);
    psi.preprocess_data(net, keys, keys);
    psi.process(net, keys, output_keys);

    if (!use_random_data) {
        bool sender_obtain_result = params["kkrt_params"]["sender_obtain_result"];
        if (sender_obtain_result) {
            std::string output_path = params["data"]["output_file"];
            std::vector<std::vector<std::string>> output_keys_2d;
            output_keys_2d.push_back(output_keys);
            petace::setops::CsvDataProvider::write_data_to_file(output_keys_2d, {}, output_path, false, {});
            LOG(INFO) << "write result to output file.";
        }
    }

    // 5. calculate runtime and  network communication.
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
    LOG(INFO) << "Cardinality is " << output_keys.size() << std::endl;
    LOG(INFO) << "Total Communication is " << total_comm << "(" << self_comm << " + " << remote_comm << ")"
              << "MB." << std::endl;
    LOG(INFO) << "Total time is " << duration << " s.";

    google::ShutdownGoogleLogging();
}
