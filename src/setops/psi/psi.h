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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "network/network.h"

namespace petace {
namespace setops {

using json = nlohmann::json;

enum class PSIScheme : std::uint32_t { ECDH_PSI = 0, KKRT_PSI = 1, VOLE_PSI = 2 };

/**
 * @brief Abstract class for various psi protocols' implementation.
 *
 * Currently, we only support ecdh-psi. We will support more PSI protocls in the future.
 */
class PSI {
public:
    PSI() {
    }

    virtual ~PSI() = default;

    /**
     * @brief Initializes parameters and variables according to parameters' JSON configuration.
     *
     * Params of JSON format is structured as follows:
     *
     * {
     *     "network": {
     *         "address": "127.0.0.1",
     *         "remote_port": 30330,
     *         "local_port": 30331,
     *         "timeout": 90,
     *         "scheme": "socket"/"grpc"/
     *     },
     *     "common": {
     *         "ids_num": 1,
     *         "is_sender": true,
     *         "verbose": true,
     *         "memory_psi_scheme": "psi" / "pjc"/ "pir",
     *         "psi_scheme": "circuit"/"vole"/"ecdh"
     *     },
     *     "data": {
     *         "input_file": "/data/receiver_input_file.csv",
     *         "has_header": false,
     *         "output_file": "/data/receiver_output_file.csv"
     *     },
     *     "ecdh_params": {
     *
     *     }
     *     "circuit_params": {
     *
     *     }
     *     "vole_params": {
     *
     *     }
     * }
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] params The PSI parameters configuration.
     */
    virtual void init(std::shared_ptr<network::Network> net, const json& params) = 0;

    /**
     * @brief Preprocess data and stores results in preprocessed_keys.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The raw input keys to perform intersection, such as phone numbers and emails.
     * @param[out] preprocessed_keys The preprocessed keys via hashing.
     */
    virtual void preprocess_data(std::shared_ptr<network::Network> net, const std::vector<std::string>& input_keys,
            std::vector<std::string>& preprocessed_keys) const = 0;

    /**
     * @brief Performs intersection and stores intersection results in output_keys.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys  to perform intersection, such as phone numbers and emails.
     * @param[out] output_keys The intersection corresponding to input keys.
     */
    virtual void process(std::shared_ptr<network::Network> net, const std::vector<std::string>& input_keys,
            std::vector<std::string>& output_keys) const = 0;

    /**
     * @brief Performs intersection and returns cardinality.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys  to perform intersection, such as phone numbers and emails.
     * @return A std::size_t number indicates the cardinality.
     */
    virtual std::size_t process_cardinality_only(
            std::shared_ptr<network::Network> net, const std::vector<std::string>& input_keys) const = 0;

protected:
    PSI(const PSI& copy) = delete;

    PSI& operator=(const PSI& assign) = delete;

    PSI(PSI&& source) = delete;

    PSI& operator=(PSI&& assign) = delete;

    // Checks the validity and consistency of JSON params of both parties.
    virtual void check_params(std::shared_ptr<network::Network> net) = 0;
};

template <PSIScheme scheme>
std::unique_ptr<PSI> CreatePSI();

}  // namespace setops
}  // namespace petace
