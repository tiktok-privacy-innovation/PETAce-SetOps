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

#include "network/network.h"
#include "solo/cuckoo_hashing.h"
#include "solo/simple_hashing.h"
#include "verse/verse_factory.h"

#include "setops/psi/psi.h"
#include "setops/util/defines.h"

namespace petace {
namespace setops {

/**
 * @brief Implementation of KKRT-PSI protocol based on OPRF (Ref: Efficient Batched Oblivious PRF with Applications to
 * Private Set Intersection).
 *
 * @par Example
 * Refer to example/kkrt_psi_example.cpp.
 */
class KkrtPSI : public PSI {
public:
    KkrtPSI() {
    }

    ~KkrtPSI() = default;

    /**
     * @brief Initializes parameters and variables according to parameters' JSON configuration.
     *
     * Params of JSON format is structured as follows:
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
     *         "memory_psi_scheme": "psi",
     *         "psi_scheme": "kkrt"
     *     },
     *     "data": {
     *         "input_file": "/data/receiver_input_file.csv",
     *         "has_header": false,
     *         "output_file": "/data/receiver_output_file.csv"
     *     },
     *      "kkrt_psi_params": {
     *          "epsilon": 1.27,
     *          "fun_num": 3,
     *          "sender_obtain_result": true
     *      }
     * }
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] params The PSI parameters configuration.
     */
    void init(const std::shared_ptr<network::Network>& net, const json& params) override;

    /**
     * @brief Preprocess data and stores results in preprocessed_keys.
     *
     * Actually, we do nothing here since the kkrt-psi will internally hash the keys to 128 bits integers.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The raw input keys to perform intersection, such as phone numbers and emails.
     * @param[out] preprocessed_keys The preprocessed keys via hashing.
     */
    void preprocess_data(const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys,
            std::vector<std::string>& preprocessed_keys) const override;

    /**
     * @brief Performs intersection and stores intersection results in output_keys.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys to perform intersection, such as phone numbers and emails.
     * @param[out] output_keys The intersection corresponding to input keys.
     */
    void process(const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys,
            std::vector<std::string>& output_keys) const override;

    /**
     * @brief Performs intersection and returns cardinality.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys  to perform intersection, such as phone numbers and emails.
     * @return A std::size_t number indicates the cardinality.
     */
    std::size_t process_cardinality_only(
            const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys) const override;

protected:
    KkrtPSI(const KkrtPSI& copy) = delete;

    KkrtPSI& operator=(const KkrtPSI& assign) = delete;

    KkrtPSI(KkrtPSI&& source) = delete;

    KkrtPSI& operator=(KkrtPSI&& assign) = delete;

private:
    // Checks the validity and consistency of JSON params of both parties.
    void check_params(const std::shared_ptr<network::Network>& net) override;

    bool is_sender_ = false;

    bool sender_obtain_result_ = false;

    bool verbose_ = false;

    std::shared_ptr<solo::PRNG> prng_ = nullptr;

    std::shared_ptr<solo::PRNG> common_prng_ = nullptr;

    std::shared_ptr<verse::BaseOtSender> base_ot_sender_ = nullptr;

    std::shared_ptr<verse::BaseOtReceiver> base_ot_receiver_ = nullptr;

    std::shared_ptr<verse::NcoOtExtSender> nco_ot_ext_sender_ = nullptr;

    std::shared_ptr<verse::NcoOtExtReceiver> nco_ot_ext_recver_ = nullptr;

    double epsilon_ = 0.0;

    std::size_t num_of_fun_ = 0;
};

}  // namespace setops
}  // namespace petace
