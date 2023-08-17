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

#include <omp.h>

#include <algorithm>
#include <vector>

#include "glog/logging.h"

#include "solo/prng.h"

#include "setops/util/parameter_check.h"
#include "setops/util/permutation.h"

namespace petace {
namespace setops {

void EcdhPSI::init(std::shared_ptr<network::Network> net, const json& params) {
    auto defalut_config = R"({
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
            "input_file": "/data/receiver_input_file.csv",
            "has_header": false,
            "output_file": "/data/receiver_output_file.csv"
        },
        "ecdh_params": {
            "curve_id": 415,
            "obtain_result": true
        }
    })"_json;

    defalut_config.merge_patch(params);
    params_ = defalut_config;

    verbose_ = params_["common"]["verbose"];
    is_sender_ = params_["common"]["is_sender"];

    check_params(net);

    LOG_IF(INFO, verbose_) << "\nECDH PSI parameters: \n" << params_.dump(4);

    obtain_result_ = params_["ecdh_params"]["obtain_result"];
    if (is_sender_) {
        net->send_data(&obtain_result_, sizeof(obtain_result_));
        net->recv_data(&remote_obtain_result_, sizeof(remote_obtain_result_));
    } else {
        net->recv_data(&remote_obtain_result_, sizeof(remote_obtain_result_));
        net->send_data(&obtain_result_, sizeof(obtain_result_));
    }

    int curve_id = params_["ecdh_params"]["curve_id"];
    ecc_cipher_ = std::make_unique<petace::solo::ECOpenSSL>(curve_id, petace::solo::HashScheme::SHA3_256);
    LOG_IF(INFO, verbose_) << "ecc curve id is " << curve_id;

    auto prng_factory = petace::solo::PRNGFactory(petace::solo::PRNGScheme::SHAKE_128);
    auto prng = prng_factory.create();
    ecc_cipher_->create_secret_key(prng, sk_);

    num_threads_ = omp_get_max_threads();
}

void EcdhPSI::preprocess_data(std::shared_ptr<network::Network> net, const std::vector<std::string>& input_keys,
        std::vector<std::string>& preprocessed_keys) const {
    (void)net;
    (void)input_keys;
    (void)preprocessed_keys;
    LOG_IF(INFO, verbose_) << "preprocess input keys done.";
}

void EcdhPSI::process(std::shared_ptr<network::Network> net, const std::vector<std::string>& input_keys,
        std::vector<std::string>& output_keys) const {
    auto prng_factory = petace::solo::PRNGFactory(petace::solo::PRNGScheme::SHAKE_128);
    auto prng = prng_factory.create();
    std::vector<std::size_t> permutation;
    generate_permutation(prng, input_keys.size(), permutation);
    output_keys.assign(input_keys.begin(), input_keys.end());
    permute_and_undo(permutation, true, output_keys);
    LOG_IF(INFO, verbose_) << "shuffle input keys done.";

    std::size_t self_data_size = input_keys.size();
    std::vector<ByteVector> encrypted_keys(self_data_size, ByteVector(kEccPointLen));

    encrypt_keys(output_keys, encrypted_keys);
    LOG_IF(INFO, verbose_) << "encrypt keys done.";

    std::vector<ByteVector> exchanged_encrypted_keys;
    exchange_encrypted_keys(net, encrypted_keys, exchanged_encrypted_keys, kEccPointLen);
    encrypted_keys.clear();
    LOG_IF(INFO, verbose_) << "send and receive encryptd keys done.";

    doublely_encrypt_keys(exchanged_encrypted_keys);
    LOG_IF(INFO, verbose_) << "doublely encrypt keys done.";

    std::vector<ByteVector> self_doublely_encrypt_keys;
    if (remote_obtain_result_) {
        exchange_encrypted_keys(net, exchanged_encrypted_keys, self_doublely_encrypt_keys, kECCCompareBytesLen);
    } else {
        exchange_encrypted_keys(net, std::vector<ByteVector>(), self_doublely_encrypt_keys, kECCCompareBytesLen);
    }
    LOG_IF(INFO, verbose_) << "send and receive doublely encrypt keys done.";

    if (obtain_result_) {
        LOG_IF(INFO, verbose_) << "self can obtain result.";
        permute_and_undo(permutation, false, self_doublely_encrypt_keys);
        LOG_IF(INFO, verbose_) << "remove doublely encrypt keys' shuffle done.";

        std::sort(exchanged_encrypted_keys.begin(), exchanged_encrypted_keys.end());
        calculate_intersection(exchanged_encrypted_keys, self_doublely_encrypt_keys, input_keys, output_keys);
        LOG_IF(INFO, verbose_) << "calculate intersection done.";
    } else {
        LOG_IF(INFO, verbose_) << "self can not obtain result.";
        output_keys.clear();
    }
    exchanged_encrypted_keys.clear();
    self_doublely_encrypt_keys.clear();
}

std::size_t EcdhPSI::process_cardinality_only(
        std::shared_ptr<network::Network> net, const std::vector<std::string>& input_keys) const {
    auto prng_factory = petace::solo::PRNGFactory(petace::solo::PRNGScheme::SHAKE_128);
    auto prng = prng_factory.create();
    std::vector<std::size_t> permutation;
    generate_permutation(prng, input_keys.size(), permutation);

    std::vector<std::string> shuffled_keys;
    shuffled_keys.assign(input_keys.begin(), input_keys.end());
    permute_and_undo(permutation, true, shuffled_keys);
    LOG_IF(INFO, verbose_) << "shuffle input keys done.";

    std::size_t self_data_size = input_keys.size();
    std::vector<ByteVector> encrypted_keys(self_data_size, ByteVector(kEccPointLen));

    encrypt_keys(shuffled_keys, encrypted_keys);
    shuffled_keys.clear();
    LOG_IF(INFO, verbose_) << "encrypt keys done.";

    std::vector<ByteVector> exchanged_encrypted_keys;
    exchange_encrypted_keys(net, encrypted_keys, exchanged_encrypted_keys, kEccPointLen);
    encrypted_keys.clear();
    LOG_IF(INFO, verbose_) << "send and receive encryptd keys done.";

    doublely_encrypt_keys(exchanged_encrypted_keys);
    LOG_IF(INFO, verbose_) << "doublely encrypt keys done.";

    std::vector<ByteVector> self_doublely_encrypted_keys;
    if (remote_obtain_result_) {
        exchange_encrypted_keys(net, exchanged_encrypted_keys, self_doublely_encrypted_keys, kECCCompareBytesLen);
    } else {
        exchange_encrypted_keys(net, std::vector<ByteVector>(), self_doublely_encrypted_keys, kECCCompareBytesLen);
    }
    LOG_IF(INFO, verbose_) << "send and receive doublely encrypt keys done.";

    std::size_t cardinality = 0;
    if (obtain_result_) {
        LOG_IF(INFO, verbose_) << "self can obtain result.";
        std::sort(exchanged_encrypted_keys.begin(), exchanged_encrypted_keys.end());
        cardinality = calculate_cardinality_only(exchanged_encrypted_keys, self_doublely_encrypted_keys);
        LOG_IF(INFO, verbose_) << "calculate cardinality done.";
    } else {
        LOG_IF(INFO, verbose_) << "self can not obtain result.";
    }
    exchanged_encrypted_keys.clear();
    self_doublely_encrypted_keys.clear();
    return cardinality;
}

void EcdhPSI::check_params(std::shared_ptr<network::Network> net) {
    int curve_id = params_["ecdh_params"]["curve_id"];
    check_consistency(is_sender_, net, "ecc_curve_id", curve_id);
    check_equal<int>("curve_id", curve_id, 415);
}

void EcdhPSI::encrypt_keys(const std::vector<std::string>& input_keys, std::vector<ByteVector>& encrypted_keys) const {
#pragma omp parallel for num_threads(num_threads_)
    for (std::size_t item_idx = 0; item_idx < input_keys.size(); ++item_idx) {
        petace::solo::ECOpenSSL::Point point(*ecc_cipher_);
        ecc_cipher_->hash_to_curve(
                reinterpret_cast<const Byte*>(input_keys[item_idx].data()), input_keys[item_idx].size(), point);
        ecc_cipher_->encrypt(point, sk_, point);
        ecc_cipher_->point_to_bytes(point, kEccPointLen, encrypted_keys[item_idx].data());
    }
}

void EcdhPSI::doublely_encrypt_keys(std::vector<ByteVector>& exchanged_encrypted_keys) const {
#pragma omp parallel for num_threads(num_threads_)
    for (std::size_t item_idx = 0; item_idx < exchanged_encrypted_keys.size(); ++item_idx) {
        petace::solo::ECOpenSSL::Point point(*ecc_cipher_);
        ecc_cipher_->point_from_bytes(
                reinterpret_cast<const Byte*>(exchanged_encrypted_keys[item_idx].data()), kEccPointLen, point);
        ecc_cipher_->encrypt(point, sk_, point);
        ByteVector point_bytes_buffer(kEccPointLen);
        ecc_cipher_->point_to_bytes(point, kEccPointLen, point_bytes_buffer.data());
        exchanged_encrypted_keys[item_idx].resize(kECCCompareBytesLen);
        std::copy_n(point_bytes_buffer.rbegin(), kECCCompareBytesLen, exchanged_encrypted_keys[item_idx].rbegin());
    }
}

void EcdhPSI::calculate_intersection(const std::vector<ByteVector>& remote_doublely_encrypted_keys,
        const std::vector<ByteVector>& self_doublely_encrypted_keys, const std::vector<std::string>& input_keys,
        std::vector<std::string>& output_keys) const {
    if (remote_doublely_encrypted_keys.empty() || self_doublely_encrypted_keys.empty() || input_keys.empty()) {
        return;
    }
    std::vector<bool> intersection_indices(input_keys.size(), false);
    std::size_t count = 0;
    for (std::size_t item_idx = 0; item_idx < self_doublely_encrypted_keys.size(); ++item_idx) {
        if (std::binary_search(remote_doublely_encrypted_keys.begin(), remote_doublely_encrypted_keys.end(),
                    self_doublely_encrypted_keys[item_idx])) {
            intersection_indices[item_idx] = true;
            ++count;
        }
    }
    output_keys.resize(count);
    std::size_t result_idx = 0;
    for (std::size_t item_idx = 0; item_idx < input_keys.size(); ++item_idx) {
        if (intersection_indices[item_idx]) {
            output_keys[result_idx++] = input_keys[item_idx];
        }
    }
}

std::size_t EcdhPSI::calculate_cardinality_only(const std::vector<ByteVector>& remote_doublely_encrypted_keys,
        const std::vector<ByteVector>& self_doublely_encrypted_keys) const {
    if (remote_doublely_encrypted_keys.empty() || self_doublely_encrypted_keys.empty()) {
        return 0;
    }
    std::size_t count = 0;
    for (std::size_t item_idx = 0; item_idx < self_doublely_encrypted_keys.size(); ++item_idx) {
        if (std::binary_search(remote_doublely_encrypted_keys.begin(), remote_doublely_encrypted_keys.end(),
                    self_doublely_encrypted_keys[item_idx])) {
            ++count;
        }
    }
    return count;
}

void EcdhPSI::exchange_encrypted_keys(std::shared_ptr<network::Network> net,
        const std::vector<ByteVector>& encrypted_keys, std::vector<ByteVector>& received_keys,
        std::size_t point_byte_count) const {
    std::size_t self_data_size = encrypted_keys.size();
    if (net == nullptr) {
        throw std::invalid_argument("net is null.");
    }
    if (point_byte_count == 0) {
        throw std::invalid_argument("Length of an Ecc point is 0.");
    }

    if (is_sender_) {
        ByteVector encrypted_keys_buffer;
        encrypted_keys_buffer.reserve(self_data_size * point_byte_count);
        for (const auto& key : encrypted_keys) {
            encrypted_keys_buffer.insert(encrypted_keys_buffer.end(), key.begin(), key.end());
        }

        net->send_data(&self_data_size, sizeof(self_data_size));
        if (self_data_size != 0) {
            net->send_data(
                    reinterpret_cast<const unsigned char*>(encrypted_keys_buffer.data()), encrypted_keys_buffer.size());
        }
        encrypted_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "sender sent encryptd keys.";

        std::size_t received_data_size = 0;
        net->recv_data(&received_data_size, sizeof(received_data_size));

        ByteVector received_keys_buffer;
        received_keys_buffer.resize(received_data_size * point_byte_count);
        if (received_data_size != 0) {
            net->recv_data(reinterpret_cast<unsigned char*>(received_keys_buffer.data()),
                    received_data_size * point_byte_count);
        }

        received_keys.reserve(received_data_size);
        for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
            received_keys.emplace_back(received_keys_buffer.begin() + item_idx * point_byte_count,
                    received_keys_buffer.begin() + (item_idx + 1) * point_byte_count);
        }
        received_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "receiver received encryptd keys.";
    } else {
        std::size_t received_data_size = 0;
        net->recv_data(&received_data_size, sizeof(received_data_size));

        ByteVector received_keys_buffer;
        received_keys_buffer.resize(received_data_size * point_byte_count);
        if (received_data_size != 0) {
            net->recv_data(reinterpret_cast<unsigned char*>(received_keys_buffer.data()),
                    received_data_size * point_byte_count);
        }

        received_keys.reserve(received_data_size);
        for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
            received_keys.emplace_back(received_keys_buffer.begin() + item_idx * point_byte_count,
                    received_keys_buffer.begin() + (item_idx + 1) * point_byte_count);
        }
        received_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "receiver received encryptd keys.";

        ByteVector encrypted_keys_buffer;
        encrypted_keys_buffer.reserve(self_data_size * point_byte_count);
        for (const auto& key : encrypted_keys) {
            encrypted_keys_buffer.insert(encrypted_keys_buffer.end(), key.begin(), key.end());
        }

        net->send_data(&self_data_size, sizeof(self_data_size));
        if (self_data_size != 0) {
            net->send_data(
                    reinterpret_cast<const unsigned char*>(encrypted_keys_buffer.data()), encrypted_keys_buffer.size());
        }
        encrypted_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "receiver sent encryptd keys.";
    }
}

template <>
std::unique_ptr<PSI> CreatePSI<PSIScheme::ECDH_PSI>() {
    return std::make_unique<EcdhPSI>();
}

}  // namespace setops
}  // namespace petace
