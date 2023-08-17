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

enum class PJCScheme : std::uint32_t { DPCA_PSI = 0, Circuit_PSI = 1, VOLE_PSI = 2 };

/**
 * @brief Abstract class for various pjc(private join and compute) protocols' implementation.
 */
class PJC {
public:
    PJC() {
    }

    virtual ~PJC() = default;

    /**
     * @brief Initializes parameters and variables according to parameters' JSON configuration.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] params The PJC parameters configuration.
     */
    virtual void init(std::shared_ptr<network::Network> net, const json& params) = 0;

    /**
     * @brief Preprocess keys and features.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The raw input keys to perform intersection, such as phone numbers and emails.
     * @param[in] input_featuress The related features appended to input keys.
     * @param[out] preprocessed_keys The preprocessed keys via hashing.
     * @param[out] output_features The preprocessed features.
     */
    virtual void preprocess_data(std::shared_ptr<network::Network> net,
            const std::vector<std::vector<std::string>>& input_keys,
            const std::vector<std::vector<std::uint64_t>>& input_features,
            std::vector<std::vector<std::string>>& preprocessed_keys,
            std::vector<std::vector<std::uint64_t>>& output_features) const = 0;

    /**
     * @brief Performs intersection and stores secret shares in output shares for both parties.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys  to perform intersection, such as phone numbers and emails.
     * @param[in] input_features The related features appended to input keys.
     * @param[out] output_shares The secret shares of input features corresponding to intersection keys.
     */
    virtual void process(std::shared_ptr<network::Network> net, const std::vector<std::vector<std::string>>& input_keys,
            const std::vector<std::vector<std::uint64_t>>& input_features,
            std::vector<std::vector<std::uint64_t>>& output_shares) const = 0;

protected:
    PJC(const PJC& copy) = delete;

    PJC& operator=(const PJC& assign) = delete;

    PJC(PJC&& source) = delete;

    PJC& operator=(PJC&& assign) = delete;

    // Checks the validity and consistency of JSON params of both parties.
    virtual void check_params() = 0;
};

template <PJCScheme scheme>
std::unique_ptr<PJC> CreatePJC();

}  // namespace setops
}  // namespace petace
