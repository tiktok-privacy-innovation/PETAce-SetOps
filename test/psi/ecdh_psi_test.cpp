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

#include "setops/psi/ecdh_psi.h"

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include "network/net_factory.h"

#include "setops/util/dummy_data_util.h"

namespace petace {
namespace setops {

using json = nlohmann::json;

class ECDHPSITest : public ::testing::Test {
public:
    void SetUp() {
        sender_params_ = R"({
            "network": {
                "address": "127.0.0.1",
                "remote_port": 30330,
                "local_port": 30331,
                "timeout": 90,
                "scheme": 0
            },
            "common": {
                "ids_num": 1,
                "is_sender": true,
                "verbose": true,
                "memory_psi_scheme": "psi",
                "psi_scheme": "ecdh"
            },
            "data": {
                "input_file": "data/receiver_input_file.csv",
                "has_header": false,
                "output_file": "data/receiver_output_file.csv"
            },
            "ecdh_params": {
                "curve_id": 415,
                "obtain_result": true
            }
        })"_json;

        auto receiver_params = R"({
            "network": {
                "address": "127.0.0.1",
                "remote_port": 30331,
                "local_port": 30330
            },
            "common": {
                "is_sender": false
            },
            "data": {
                "input_file": "data/receiver_input_file.csv",
                "output_file": "data/receiver_output_file.csv"
            }
        })"_json;
        receiver_params_ = sender_params_;
        receiver_params_.merge_patch(receiver_params);
        sender_without_obtain_result_params_ = sender_params_;
        receiver_without_obtain_result_params_ = receiver_params_;
        sender_without_obtain_result_params_["ecdh_params"]["obtain_result"] = false;
        receiver_without_obtain_result_params_["ecdh_params"]["obtain_result"] = false;
    }

    void ecdh_psi_default(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        EcdhPSI psi;
        psi.init(net, params);
        if (is_sender) {
            psi.preprocess_data(net, default_sender_keys_, default_sender_keys_);
            psi.process(net, default_sender_keys_, output_keys_0_);
        } else {
            psi.preprocess_data(net, default_receiver_keys_, default_receiver_keys_);
            psi.process(net, default_receiver_keys_, output_keys_1_);
        }
    }

    std::size_t ecdh_psi_cardinality_default(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        EcdhPSI psi;
        std::size_t cardinality = 0;
        psi.init(net, params);
        if (is_sender) {
            psi.preprocess_data(net, default_sender_keys_, default_sender_keys_);
            cardinality = psi.process_cardinality_only(net, default_sender_keys_);
        } else {
            psi.preprocess_data(net, default_receiver_keys_, default_receiver_keys_);
            cardinality = psi.process_cardinality_only(net, default_receiver_keys_);
        }
        return cardinality;
    }

    std::size_t ecdh_psi_cardinality_random(const json& params, std::size_t intersection_size) {
        std::size_t data_size = 10 * intersection_size;
        auto prng_factory = petace::solo::PRNGFactory(petace::solo::PRNGScheme::SHAKE_128);
        std::vector<Byte> seed(16, Byte(0));

        auto common_prng = prng_factory.create(seed);
        auto unique_prng = prng_factory.create();

        std::vector<std::string> common_keys;
        std::vector<std::string> unique_keys;
        generate_random_keys(*common_prng, intersection_size, "0", common_keys);
        generate_random_keys(*unique_prng, data_size - intersection_size, "0", unique_keys);
        unique_keys.insert(unique_keys.begin(), common_keys.begin(), common_keys.end());

        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        EcdhPSI psi;
        psi.init(net, params);
        std::size_t cardinality = psi.process_cardinality_only(net, unique_keys);
        return cardinality;
    }

public:
    json sender_params_;
    json receiver_params_;
    json sender_without_obtain_result_params_;
    json receiver_without_obtain_result_params_;
    std::thread t_[2];

    std::vector<std::string> output_keys_0_;
    std::vector<std::string> output_keys_1_;

    std::vector<std::string> default_sender_keys_ = {"c", "h", "e", "g", "y", "z"};
    std::vector<std::string> default_receiver_keys_ = {{"b", "c", "e", "g"}};
    std::size_t default_expected_cardinality_ = 3;
    std::vector<std::string> default_expected_results_ = {"c", "e", "g"};
};

TEST_F(ECDHPSITest, default_test) {
    t_[0] = std::thread([this]() { ecdh_psi_default(sender_params_); });
    t_[1] = std::thread([this]() { ecdh_psi_default(receiver_params_); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(output_keys_0_.size(), output_keys_1_.size());
    EXPECT_EQ(output_keys_0_.size(), default_expected_cardinality_);
    EXPECT_EQ(output_keys_0_, default_expected_results_);
}

TEST_F(ECDHPSITest, default_cardinality_test) {
    std::size_t sender_cardinality = 0;
    std::size_t receiver_cardinality = 0;
    t_[0] = std::thread(
            [this, &sender_cardinality]() { sender_cardinality = ecdh_psi_cardinality_default(sender_params_); });
    t_[1] = std::thread(
            [this, &receiver_cardinality]() { receiver_cardinality = ecdh_psi_cardinality_default(receiver_params_); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(sender_cardinality, receiver_cardinality);
    EXPECT_EQ(sender_cardinality, default_expected_cardinality_);
}

TEST_F(ECDHPSITest, default_sender_without_obtain_result) {
    t_[0] = std::thread([this]() { ecdh_psi_default(sender_without_obtain_result_params_); });
    t_[1] = std::thread([this]() { ecdh_psi_default(receiver_params_); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(output_keys_0_.size(), 0);
    EXPECT_EQ(output_keys_1_.size(), default_expected_cardinality_);
    EXPECT_EQ(output_keys_1_, default_expected_results_);
}

TEST_F(ECDHPSITest, default_receiver_without_obtain_result) {
    t_[0] = std::thread([this]() { ecdh_psi_default(sender_params_); });
    t_[1] = std::thread([this]() { ecdh_psi_default(receiver_without_obtain_result_params_); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(output_keys_1_.size(), 0);
    EXPECT_EQ(output_keys_0_.size(), default_expected_cardinality_);
    EXPECT_EQ(output_keys_0_, default_expected_results_);
}

TEST_F(ECDHPSITest, random_test) {
    std::size_t sender_cardinality = 0;
    std::size_t receiver_cardinality = 0;
    t_[0] = std::thread(
            [this, &sender_cardinality]() { sender_cardinality = ecdh_psi_cardinality_random(sender_params_, 5); });
    t_[1] = std::thread([this, &receiver_cardinality]() {
        receiver_cardinality = ecdh_psi_cardinality_random(receiver_params_, 5);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(sender_cardinality, receiver_cardinality);
    EXPECT_EQ(sender_cardinality, 5);
}

TEST_F(ECDHPSITest, random_sender_without_obtain_result) {
    std::size_t sender_cardinality = 0;
    std::size_t receiver_cardinality = 0;
    t_[0] = std::thread([this, &sender_cardinality]() {
        sender_cardinality = ecdh_psi_cardinality_random(sender_without_obtain_result_params_, 5);
    });
    t_[1] = std::thread([this, &receiver_cardinality]() {
        receiver_cardinality = ecdh_psi_cardinality_random(receiver_params_, 5);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(sender_cardinality, 0);
    EXPECT_EQ(receiver_cardinality, 5);
}

TEST_F(ECDHPSITest, random_receiver_without_obtain_result) {
    std::size_t sender_cardinality = 0;
    std::size_t receiver_cardinality = 0;
    t_[0] = std::thread(
            [this, &sender_cardinality]() { sender_cardinality = ecdh_psi_cardinality_random(sender_params_, 5); });
    t_[1] = std::thread([this, &receiver_cardinality]() {
        receiver_cardinality = ecdh_psi_cardinality_random(receiver_without_obtain_result_params_, 5);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(sender_cardinality, 5);
    EXPECT_EQ(receiver_cardinality, 0);
}

TEST_F(ECDHPSITest, inconsistent_curve_id) {
    json receiver_invalid_params = receiver_params_;
    receiver_invalid_params["ecdh_params"]["curve_id"] = 414;

    t_[0] = std::thread([this]() { EXPECT_THROW(ecdh_psi_default(sender_params_), std::invalid_argument); });
    t_[1] = std::thread([this, &receiver_invalid_params]() {
        EXPECT_THROW(ecdh_psi_default(receiver_invalid_params), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(ECDHPSITest, unexpected_ecurve_id) {
    json receiver_invalid_params = receiver_params_;
    json sender_invalid_params = sender_params_;
    receiver_invalid_params["ecdh_params"]["curve_id"] = 414;
    sender_invalid_params["ecdh_params"]["curve_id"] = 416;

    t_[0] = std::thread([this, &sender_invalid_params]() {
        EXPECT_THROW(ecdh_psi_default(sender_invalid_params), std::invalid_argument);
    });
    t_[1] = std::thread([this, &receiver_invalid_params]() {
        EXPECT_THROW(ecdh_psi_default(receiver_invalid_params), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

}  // namespace setops
}  // namespace petace
