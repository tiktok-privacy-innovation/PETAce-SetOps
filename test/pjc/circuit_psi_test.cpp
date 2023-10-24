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

#include "setops/pjc/circuit_psi.h"

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

class CircuitPSITest : public ::testing::Test {
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
                "memory_pjc_scheme": "pjc",
                "pjc_scheme": "circuit"
            },
            "data": {
                "input_file": "data/receiver_input_file.csv",
                "has_header": false,
                "output_file": "data/receiver_output_file.csv"
            },
            "circuit_psi_params": {
                "epsilon": 1.27,
                "fun_epsilon": 1.27,
                "fun_num": 3,
                "hint_fun_num": 3
            }
        })"_json;
        sender_params_stash_zero_ = R"({
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
                "memory_pjc_scheme": "pjc",
                "pjc_scheme": "circuit"
            },
            "data": {
                "input_file": "data/receiver_input_file.csv",
                "has_header": false,
                "output_file": "data/receiver_output_file.csv"
            },
            "circuit_psi_params": {
                "epsilon": 0.27,
                "fun_epsilon": 0.27,
                "fun_num": 3,
                "hint_fun_num": 3
            }
        })"_json;
        sender_params_hint_stash_zero_ = R"({
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
                "memory_pjc_scheme": "pjc",
                "pjc_scheme": "circuit"
            },
            "data": {
                "input_file": "data/receiver_input_file.csv",
                "has_header": false,
                "output_file": "data/receiver_output_file.csv"
            },
            "circuit_psi_params": {
                "epsilon": 1.27,
                "fun_epsilon": 0.27,
                "fun_num": 3,
                "hint_fun_num": 3
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
        receiver_params_stash_zero_ = sender_params_stash_zero_;
        receiver_params_stash_zero_.merge_patch(receiver_params);
        receiver_params_hint_stash_zero_ = sender_params_hint_stash_zero_;
        receiver_params_hint_stash_zero_.merge_patch(receiver_params);
    }

    void circuit_psi_balanced(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        CircuitPSI pjc;
        pjc.init(net, params);
        if (is_sender) {
            pjc.process(net, balanced_sender_keys_, balanced_sender_values_, balanced_sender_output_);
        } else {
            pjc.process(net, balanced_receiver_keys_, balanced_receiver_values_, balanced_receiver_output_);
        }
    }

    void circuit_psi_balanced_null_feature(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        CircuitPSI pjc;
        pjc.init(net, params);
        if (is_sender) {
            pjc.process(net, balanced_sender_keys_, balanced_sender_null_values_, balanced_sender_output_);
        } else {
            pjc.process(net, balanced_receiver_keys_, balanced_receiver_null_values_, balanced_receiver_output_);
        }
    }

    void circuit_psi_unbalanced(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        CircuitPSI pjc;
        pjc.init(net, params);
        if (is_sender) {
            pjc.process(net, unbalanced_sender_keys_, unbalanced_sender_values_, unbalanced_sender_output_);
        } else {
            pjc.process(net, unbalanced_receiver_keys_, unbalanced_receiver_values_, unbalanced_receiver_output_);
        }
    }

    void circuit_psi_unbalanced_null_feature(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        CircuitPSI pjc;
        pjc.init(net, params);
        if (is_sender) {
            pjc.process(net, unbalanced_sender_keys_, unbalanced_sender_null_values_, unbalanced_sender_output_);
        } else {
            pjc.process(net, unbalanced_receiver_keys_, unbalanced_receiver_null_values_, unbalanced_receiver_output_);
        }
    }

    void circuit_psi_stash_not_zero(const json& params) {
        network::NetParams net_params;
        net_params.remote_addr = params["network"]["address"];
        net_params.remote_port = params["network"]["remote_port"];
        net_params.local_port = params["network"]["local_port"];
        auto net = network::NetFactory::get_instance().build(network::NetScheme::SOCKET, net_params);

        bool is_sender = params["common"]["is_sender"];

        CircuitPSI pjc;
        pjc.init(net, params);
        if (is_sender) {
            pjc.process(net, balanced_sender_keys_, balanced_sender_values_, balanced_sender_output_);
        } else {
            pjc.process(net, balanced_receiver_keys_, balanced_receiver_values_, balanced_receiver_output_);
        }
    }

public:
    json sender_params_;
    json receiver_params_;
    json sender_params_stash_zero_;
    json receiver_params_stash_zero_;
    json sender_params_hint_stash_zero_;
    json receiver_params_hint_stash_zero_;
    std::thread t_[2];

    std::vector<std::string> balanced_sender_keys_ = {"c", "h", "e", "g", "y", "z"};
    std::vector<std::string> balanced_receiver_keys_ = {"b", "c", "e", "g", "u", "v"};
    std::vector<std::vector<std::uint64_t>> balanced_sender_values_{{0, 1, 2, 3, 4, 5}, {6, 7, 8, 9, 10, 11}};
    std::vector<std::vector<std::uint64_t>> balanced_receiver_values_{
            {20, 21, 22, 23, 24, 25}, {26, 27, 28, 29, 30, 31}};
    std::vector<std::vector<std::uint64_t>> balanced_sender_null_values_{};
    std::vector<std::vector<std::uint64_t>> balanced_receiver_null_values_{};
    std::vector<std::vector<std::uint64_t>> balanced_sender_output_;
    std::vector<std::vector<std::uint64_t>> balanced_receiver_output_;
    std::vector<std::vector<std::uint64_t>> balanced_output_;
    std::vector<std::string> unbalanced_sender_keys_ = {"c", "h", "e", "g"};
    std::vector<std::string> unbalanced_receiver_keys_ = {"b", "c", "e", "g", "u", "v"};
    std::vector<std::vector<std::uint64_t>> unbalanced_sender_values_{{0, 1, 2, 3}, {6, 7, 8, 9}};
    std::vector<std::vector<std::uint64_t>> unbalanced_receiver_values_{
            {20, 21, 22, 23, 24, 25}, {26, 27, 28, 29, 30, 31}};
    std::vector<std::vector<std::uint64_t>> unbalanced_sender_null_values_{};
    std::vector<std::vector<std::uint64_t>> unbalanced_receiver_null_values_{};
    std::vector<std::vector<std::uint64_t>> unbalanced_sender_output_;
    std::vector<std::vector<std::uint64_t>> unbalanced_receiver_output_;
    std::vector<std::vector<std::uint64_t>> unbalanced_output_;
    std::vector<std::uint64_t> expected_results_{3, 5, 23, 66, 84};
    std::vector<std::uint64_t> actual_results_;
};

TEST_F(CircuitPSITest, balanced_test) {
    t_[0] = std::thread([this]() {
        try {
            circuit_psi_balanced(sender_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });
    t_[1] = std::thread([this]() {
        try {
            circuit_psi_balanced(receiver_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });

    t_[0].join();
    t_[1].join();

    balanced_output_.resize(balanced_sender_output_.size());
    for (std::size_t i = 0; i < balanced_sender_output_.size(); i++) {
        balanced_output_[i].resize(balanced_sender_output_[i].size());
        for (std::size_t j = 0; j < balanced_sender_output_[i].size(); j++) {
            if (i == 0) {
                balanced_output_[i][j] = balanced_sender_output_[i][j] ^ balanced_receiver_output_[i][j];
            } else {
                balanced_output_[i][j] = balanced_sender_output_[i][j] + balanced_receiver_output_[i][j];
            }
        }
    }
    actual_results_.resize(balanced_output_.size());
    for (std::size_t i = 0; i < balanced_output_.size(); i++) {
        actual_results_[i] = 0;
        for (std::size_t j = 0; j < balanced_output_[i].size(); j++) {
            if (i == 0) {
                actual_results_[i] += balanced_output_[i][j];
            } else {
                actual_results_[i] += balanced_output_[0][j] * balanced_output_[i][j];
            }
        }
    }

    for (std::size_t i = 0; i < actual_results_.size(); i++) {
        EXPECT_EQ(expected_results_[i], actual_results_[i]);
    }
}

TEST_F(CircuitPSITest, balanced_null_feature_test) {
    t_[0] = std::thread([this]() {
        try {
            circuit_psi_balanced_null_feature(sender_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });
    t_[1] = std::thread([this]() {
        try {
            circuit_psi_balanced_null_feature(receiver_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });

    t_[0].join();
    t_[1].join();

    balanced_output_.resize(balanced_sender_output_.size());
    for (std::size_t i = 0; i < balanced_sender_output_.size(); i++) {
        balanced_output_[i].resize(balanced_sender_output_[i].size());
        for (std::size_t j = 0; j < balanced_sender_output_[i].size(); j++) {
            if (i == 0) {
                balanced_output_[i][j] = balanced_sender_output_[i][j] ^ balanced_receiver_output_[i][j];
            } else {
                balanced_output_[i][j] = balanced_sender_output_[i][j] + balanced_receiver_output_[i][j];
            }
        }
    }
    actual_results_.resize(balanced_output_.size());
    for (std::size_t i = 0; i < balanced_output_.size(); i++) {
        actual_results_[i] = 0;
        for (std::size_t j = 0; j < balanced_output_[i].size(); j++) {
            if (i == 0) {
                actual_results_[i] += balanced_output_[i][j];
            } else {
                actual_results_[i] += balanced_output_[0][j] * balanced_output_[i][j];
            }
        }
    }

    for (std::size_t i = 0; i < actual_results_.size(); i++) {
        EXPECT_EQ(expected_results_[i], actual_results_[i]);
    }
}

TEST_F(CircuitPSITest, unbalanced_test) {
    t_[0] = std::thread([this]() {
        try {
            circuit_psi_unbalanced(sender_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });
    t_[1] = std::thread([this]() {
        try {
            circuit_psi_unbalanced(receiver_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });

    t_[0].join();
    t_[1].join();

    unbalanced_output_.resize(unbalanced_sender_output_.size());
    for (std::size_t i = 0; i < unbalanced_sender_output_.size(); i++) {
        unbalanced_output_[i].resize(unbalanced_sender_output_[i].size());
        for (std::size_t j = 0; j < unbalanced_sender_output_[i].size(); j++) {
            if (i == 0) {
                unbalanced_output_[i][j] = unbalanced_sender_output_[i][j] ^ unbalanced_receiver_output_[i][j];
            } else {
                unbalanced_output_[i][j] = unbalanced_sender_output_[i][j] + unbalanced_receiver_output_[i][j];
            }
        }
    }
    actual_results_.resize(unbalanced_output_.size());
    for (std::size_t i = 0; i < unbalanced_output_.size(); i++) {
        actual_results_[i] = 0;
        for (std::size_t j = 0; j < unbalanced_output_[i].size(); j++) {
            if (i == 0) {
                actual_results_[i] += unbalanced_output_[i][j];
            } else {
                actual_results_[i] += unbalanced_output_[0][j] * unbalanced_output_[i][j];
            }
        }
    }

    for (std::size_t i = 0; i < actual_results_.size(); i++) {
        EXPECT_EQ(expected_results_[i], actual_results_[i]);
    }
}

TEST_F(CircuitPSITest, unbalanced_null_feature_test) {
    t_[0] = std::thread([this]() {
        try {
            circuit_psi_unbalanced_null_feature(sender_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });
    t_[1] = std::thread([this]() {
        try {
            circuit_psi_unbalanced_null_feature(receiver_params_);
        } catch (const std::invalid_argument& exception) {
            EXPECT_EQ("stash of size is not zero.", std::string(exception.what()));
        }
    });

    t_[0].join();
    t_[1].join();

    unbalanced_output_.resize(unbalanced_sender_output_.size());
    for (std::size_t i = 0; i < unbalanced_sender_output_.size(); i++) {
        unbalanced_output_[i].resize(unbalanced_sender_output_[i].size());
        for (std::size_t j = 0; j < unbalanced_sender_output_[i].size(); j++) {
            if (i == 0) {
                unbalanced_output_[i][j] = unbalanced_sender_output_[i][j] ^ unbalanced_receiver_output_[i][j];
            } else {
                unbalanced_output_[i][j] = unbalanced_sender_output_[i][j] + unbalanced_receiver_output_[i][j];
            }
        }
    }
    actual_results_.resize(unbalanced_output_.size());
    for (std::size_t i = 0; i < unbalanced_output_.size(); i++) {
        actual_results_[i] = 0;
        for (std::size_t j = 0; j < unbalanced_output_[i].size(); j++) {
            if (i == 0) {
                actual_results_[i] += unbalanced_output_[i][j];
            } else {
                actual_results_[i] += unbalanced_output_[0][j] * unbalanced_output_[i][j];
            }
        }
    }

    EXPECT_EQ(actual_results_.size(), 1);
    for (std::size_t i = 0; i < actual_results_.size(); i++) {
        EXPECT_EQ(expected_results_[i], actual_results_[i]);
    }
}

TEST_F(CircuitPSITest, circuit_psi_stash_not_zero) {
    t_[0] = std::thread(
            [this]() { EXPECT_THROW(circuit_psi_stash_not_zero(sender_params_stash_zero_), std::invalid_argument); });
    t_[1] = std::thread(
            [this]() { EXPECT_THROW(circuit_psi_stash_not_zero(receiver_params_stash_zero_), std::invalid_argument); });

    t_[0].join();
    t_[1].join();
}

TEST_F(CircuitPSITest, circuit_psi_hint_stash_not_zero) {
    t_[0] = std::thread([this]() {
        EXPECT_THROW(circuit_psi_stash_not_zero(sender_params_hint_stash_zero_), std::invalid_argument);
    });
    t_[1] = std::thread([this]() {
        EXPECT_THROW(circuit_psi_stash_not_zero(receiver_params_hint_stash_zero_), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

}  // namespace setops
}  // namespace petace
