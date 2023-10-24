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
#include "solo/ec_openssl.h"

#include "setops/psi/psi.h"
#include "setops/util/defines.h"

namespace petace {
namespace setops {

/**
 * @brief Implementation of PSI protocol based on Elliptic-Curve Diffie-Hellman (ECDH-PSI).
 *
 * @par Example
 * Refer to example/ecdh_psi_example.cpp.
 */
class EcdhPSI : public PSI {
public:
    EcdhPSI() {
    }

    ~EcdhPSI() = default;

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
     *         "psi_scheme": "ecdh"
     *     },
     *     "data": {
     *         "input_file": "/data/receiver_input_file.csv",
     *         "has_header": false,
     *         "output_file": "/data/receiver_output_file.csv"
     *     },
     *     "ecdh_params": {
     *         "curve_id": 415,
     *         "obtain_result": true
     *     }
     * }
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] params The PSI parameters configuration.
     */
    void init(const std::shared_ptr<network::Network>& net, const json& params) override;

    /**
     * @brief Preprocess data and stores results in preprocessed_keys.
     *
     * Actually, we do nothing here since the ecdh-psi will internally hash the keys to ECC points.
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
     * In details, the workflow of ecdh-psi:
     *   1. Shuffles and encrypts keys of every row on both parties' side. Exchanges keys with the other party.
     *   2. Doublely encrypts the exchanged keys.
     *   3. Sends back keys to the other party if the other party can obtain result.
     *   4. Computes intersection on the exchanged keys and saves intersection corresponding to input keys.
     *
     * @param[in] net The network interface (e.g., PETAce-Network interface).
     * @param[in] input_keys The input keys  to perform intersection, such as phone numbers and emails.
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
    EcdhPSI(const EcdhPSI& copy) = delete;

    EcdhPSI& operator=(const EcdhPSI& assign) = delete;

    EcdhPSI(EcdhPSI&& source) = delete;

    EcdhPSI& operator=(EcdhPSI&& assign) = delete;

private:
    // Checks the validity and consistency of JSON params of both parties.
    void check_params(const std::shared_ptr<network::Network>& net) override;

    // Encrypts inpute keys with its ECC secret key.
    // Stores results in encrypted_keys.
    void encrypt_keys(const std::vector<std::string>& input_keys, std::vector<ByteVector>& encrypted_keys) const;

    // Doublely encrypts exchanged encryted keys with its ECC secret key.
    // Restore results in exchanged_encrypted_keys.
    void doublely_encrypt_keys(std::vector<ByteVector>& exchanged_encrypted_keys) const;

    // Exchanges encrypted keys or doublely encrypted keys with the other party.
    void exchange_encrypted_keys(std::shared_ptr<network::Network> net, const std::vector<ByteVector>& encrypted_keys,
            std::vector<ByteVector>& received_keys, std::size_t point_len) const;

    // Computes intersection between remote doublely encrypted keys and self doublely encrypted keys.
    // Stores the intersection corresponding to input keys in output keys.
    void calculate_intersection(const std::vector<ByteVector>& remote_doublely_encrypted_keys,
            const std::vector<ByteVector>& self_doublely_encrypted_keys, const std::vector<std::string>& input_keys,
            std::vector<std::string>& output_keys) const;

    // Computes intersection between remote doublely encrypted keys and self doublely encrypted keys.
    // Retures the cardinality of intersection.
    std::size_t calculate_cardinality_only(const std::vector<ByteVector>& remote_doublely_encrypted_keys,
            const std::vector<ByteVector>& self_doublely_encrypted_keys) const;

    bool is_sender_ = false;
    bool obtain_result_ = false;
    bool remote_obtain_result_ = false;

    json params_ = "";
    bool verbose_ = false;

    std::unique_ptr<petace::solo::ECOpenSSL> ecc_cipher_ = nullptr;
    petace::solo::ECOpenSSL::SecretKey sk_{};
    std::size_t num_threads_ = 0;
};

}  // namespace setops
}  // namespace petace
