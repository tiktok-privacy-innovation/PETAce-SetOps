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

#include "duet/duet.h"
#include "network/network.h"
#include "solo/cuckoo_hashing.h"
#include "solo/simple_hashing.h"
#include "verse/verse_factory.h"

#include "setops/pjc/pjc.h"
#include "setops/util/defines.h"

namespace petace {
namespace setops {

/**
 * @brief Implementation of PJC protocol based on Circuit-PSI protocol (Ref: Circuit-PSI With Linear Complexity via
 * Relaxed Batch OPPRF).
 *
 * @par Example
 * Refer to example/circuit_psi_example.cpp.
 */
class CircuitPSI : public PJC {
public:
    CircuitPSI() {
    }

    ~CircuitPSI() = default;

    /**
     * @brief Initializes parameters and variables according to parameters' json configuration.
     *
     * Params of json format is structured as follows:
     * {
     *     "network": {
     *         "address": "127.0.0.1",
     *         "remote_port": 30330,
     *         "local_port": 30331,
     *         "timeout": 90,
     *         "scheme": 0
     *     },
     *     "common": {
     *         "ids_num": 1,
     *         "is_sender": true,
     *         "verbose": true,
     *         "memory_pjc_scheme": "pjc",
     *         "pjc_scheme": "circuit"
     *     },
     *     "data": {
     *         "input_file": "/data/receiver_input_file.csv",
     *         "has_header": false,
     *         "output_file": "/data/receiver_output_file.csv"
     *     },
     *     "circuit_psi_params": {
     *         "epsilon": 1.27,
     *         "fun_epsilon": 1.27,
     *         "fun_num": 3,
     *         "hint_fun_num": 3
     *     }
     * }
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] params The PJC parameters configuration.
     */
    void init(const std::shared_ptr<network::Network>& net, const json& params) override;

    /**
     * @brief Performs intersection and stores intersection results in output_keys.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys  to perform intersection, such as phone numbers and emails. Only one ID type
     * can be supported at the same time。
     * @param[out] output_keys The intersection corresponding to input keys.
     */
    void process(const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys,
            const std::vector<std::vector<std::uint64_t>>& input_features,
            std::vector<std::vector<std::uint64_t>>& output_shares) const override;

protected:
    CircuitPSI(const CircuitPSI& copy) = delete;

    CircuitPSI& operator=(const CircuitPSI& assign) = delete;

    CircuitPSI(CircuitPSI&& source) = delete;

    CircuitPSI& operator=(CircuitPSI&& assign) = delete;

private:
    // Checks the validity and consistency of json params of both parties.
    void check_params(const std::shared_ptr<network::Network>& net) override;

    bool is_sender_ = false;

    bool verbose_ = false;

    std::shared_ptr<solo::PRNG> prng_ = nullptr;

    std::shared_ptr<solo::PRNG> common_prng_ = nullptr;

    std::shared_ptr<duet::Duet> mpc_op_ = nullptr;

    std::shared_ptr<verse::BaseOtSender> base_ot_sender_ = nullptr;

    std::shared_ptr<verse::BaseOtReceiver> base_ot_receiver_ = nullptr;

    std::shared_ptr<verse::NcoOtExtSender> nco_ot_ext_sender_ = nullptr;

    std::shared_ptr<verse::NcoOtExtReceiver> nco_ot_ext_recver_ = nullptr;

    double epsilon_ = 0.0;

    double epsilon_hint_ = 0.0;

    std::size_t num_of_fun_ = 0;

    std::size_t num_of_fun_hint_ = 0;
};

}  // namespace setops
}  // namespace petace
