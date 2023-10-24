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

#include "setops/psi/kkrt_psi.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "glog/logging.h"

#include "solo/prng.h"

#include "setops/util/parameter_check.h"
#include "setops/util/permutation.h"
#include "setops/util/serialize.h"

namespace petace {
namespace setops {

void KkrtPSI::init(const std::shared_ptr<network::Network>& net, const json& params) {
    // set parameter
    verbose_ = params["common"]["verbose"];
    is_sender_ = params["common"]["is_sender"];
    epsilon_ = params["kkrt_psi_params"]["epsilon"];
    num_of_fun_ = params["kkrt_psi_params"]["fun_num"];
    sender_obtain_result_ = params["kkrt_psi_params"]["sender_obtain_result"];

    check_params(net);

    LOG_IF(INFO, verbose_) << "\nKKRT PSI parameters: \n" << params.dump(4);

    // prng
    solo::PRNGFactory prng_factory(solo::PRNGScheme::AES_ECB_CTR);
    prng_ = prng_factory.create();

    // common prng
    block sender_data;
    block receiver_data;
    prng_->generate(sizeof(block), reinterpret_cast<Byte*>(&sender_data));
    net->send_data(reinterpret_cast<std::uint8_t*>(&sender_data), sizeof(block));
    net->recv_data(reinterpret_cast<std::uint8_t*>(&receiver_data), sizeof(block));
    sender_data ^= receiver_data;

    std::vector<Byte> seed(kRandSeedBytesLen);
    std::memcpy(seed.data(), reinterpret_cast<Byte*>(const_cast<block*>(&sender_data)), kRandSeedBytesLen);
    common_prng_ = prng_factory.create(seed);

    // ot
    verse::VerseParams verse_params;
    verse_params.base_ot_sizes = 512;

    if (is_sender_) {
        base_ot_receiver_ = verse::VerseFactory<petace::verse::BaseOtReceiver>::get_instance().build(
                verse::OTScheme::NaorPinkasReceiver, verse_params);
        nco_ot_ext_sender_ = verse::VerseFactory<petace::verse::NcoOtExtSender>::get_instance().build(
                verse::OTScheme::KkrtSender, verse_params);
    } else {
        base_ot_sender_ = verse::VerseFactory<petace::verse::BaseOtSender>::get_instance().build(
                verse::OTScheme::NaorPinkasSender, verse_params);
        nco_ot_ext_recver_ = verse::VerseFactory<petace::verse::NcoOtExtReceiver>::get_instance().build(
                verse::OTScheme::KkrtReceiver, verse_params);
    }

    if (is_sender_) {
        std::vector<block> base_recv_ots;
        std::vector<block> rand_choice(4);
        prng_->generate(sizeof(block) * 4, reinterpret_cast<Byte*>(&rand_choice[0]));
        base_ot_receiver_->receive(net, rand_choice, base_recv_ots);
        nco_ot_ext_sender_->set_base_ots(rand_choice, base_recv_ots);
    } else {
        std::vector<std::array<block, 2>> base_send_ots;
        base_ot_sender_->send(net, base_send_ots);
        nco_ot_ext_recver_->set_base_ots(base_send_ots);
    }
}

void KkrtPSI::preprocess_data(const std::shared_ptr<network::Network>& /*net*/,
        const std::vector<std::string>& /*input_keys*/, std::vector<std::string>& /*preprocessed_keys*/) const {
    LOG_IF(INFO, verbose_) << "preprocess input keys done.";
}

void KkrtPSI::process(const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys,
        std::vector<std::string>& output_keys) const {
    std::size_t sender_data_size;
    std::size_t receiver_data_size;
    if (is_sender_) {
        sender_data_size = input_keys.size();
        net->recv_data(&receiver_data_size, sizeof(receiver_data_size));
        net->send_data(&sender_data_size, sizeof(sender_data_size));
    } else {
        receiver_data_size = input_keys.size();
        net->send_data(&receiver_data_size, sizeof(receiver_data_size));
        net->recv_data(&sender_data_size, sizeof(sender_data_size));
    }

    std::size_t num_of_bins = static_cast<std::size_t>(std::ceil(static_cast<double>(receiver_data_size) * epsilon_));

    std::vector<Item> keys(input_keys.size());
    auto hash = solo::Hash::create(solo::HashScheme::SHA_256);
    for (std::size_t i = 0; i < input_keys.size(); i++) {
        hash->compute(reinterpret_cast<const Byte*>(input_keys[i].data()), input_keys[i].size(),
                reinterpret_cast<Byte*>(&keys[i]), sizeof(Item));
    }

    solo::PRNGFactory prng_factory(solo::PRNGScheme::AES_ECB_CTR);
    if (is_sender_) {
        std::vector<Byte> simple_table_seed(kRandSeedBytesLen);
        common_prng_->generate(kRandSeedBytesLen, simple_table_seed.data());
        auto simple_table = std::make_shared<solo::SimpleHashing<kItemBytesLen>>(num_of_bins, simple_table_seed);

        // Hashing Phase
        simple_table->set_num_of_hash_functions(num_of_fun_);
        simple_table->insert(keys);
        simple_table->map_elements();

        std::size_t stash_size;
        net->recv_data(&stash_size, sizeof(std::size_t));
        if (stash_size > 0u) {
            LOG_IF(INFO, verbose_) << "stash of size is not zero.";
            throw std::invalid_argument("stash of size is not zero.");
        }

        auto simple_table_values = simple_table->obtain_bin_entry_values();
        auto simple_table_source_values = simple_table->obtain_bin_entry_function_ids();

        LOG_IF(INFO, verbose_) << "simple hash done.";

        // OPRF
        std::vector<std::vector<block>> masks(num_of_bins);
        std::vector<std::vector<block>> simple_table_values_block(num_of_bins);
        for (std::size_t i = 0; i < simple_table_values.size(); i++) {
            for (std::size_t j = 0; j < simple_table_values[i].size(); j++) {
                simple_table_values_block[i].emplace_back(
                        *(reinterpret_cast<block*>(simple_table_values[i][j].data())));
            }
        }
        nco_ot_ext_sender_->send(net, num_of_bins);
        std::vector<std::vector<block>> sender_enc_data(num_of_fun_);
        for (std::size_t i = 0; i < num_of_bins; i++) {
            masks[i].resize(simple_table_values_block[i].size());
            for (std::size_t j = 0; j < simple_table_values_block[i].size(); j++) {
                nco_ot_ext_sender_->encode(i, simple_table_values_block[i][j], masks[i][j]);
                sender_enc_data[simple_table_source_values[i][j]].push_back(masks[i][j]);
            }
        }

        LOG_IF(INFO, verbose_) << "oprf done.";

        std::vector<std::size_t> permutation;
        std::vector<std::vector<block>> shuffled_sender_enc_data(num_of_fun_);
        for (std::size_t i = 0; i < num_of_fun_; i++) {
            generate_permutation(prng_, sender_data_size, permutation);
            shuffled_sender_enc_data[i].assign(sender_enc_data[i].begin(), sender_enc_data[i].end());
            permute_and_undo(permutation, true, shuffled_sender_enc_data[i]);
        }

        ByteVector reduced_shuffled_sender_enc_data;
        reduced_shuffled_sender_enc_data.reserve(num_of_fun_ * sender_data_size * kReduceStatisticsLen);
        for (std::size_t i = 0; i < num_of_fun_; i++) {
            for (std::size_t j = 0; j < sender_data_size; j++) {
                reduced_shuffled_sender_enc_data.insert(reduced_shuffled_sender_enc_data.end(),
                        reinterpret_cast<Byte*>(&shuffled_sender_enc_data[i][j]),
                        reinterpret_cast<Byte*>(&shuffled_sender_enc_data[i][j]) + kReduceStatisticsLen);
            }
        }
        net->send_data(reduced_shuffled_sender_enc_data.data(), reduced_shuffled_sender_enc_data.size());

        if (sender_obtain_result_) {
            LOG_IF(INFO, verbose_) << "sender can obtain result.";
            std::size_t count;
            net->recv_data(&count, sizeof(std::size_t));
            std::vector<char> serialized_key(count);
            net->recv_data(serialized_key.data(), count);
            output_keys.clear();
            deserialize_string_from_char(serialized_key, output_keys);
            LOG_IF(INFO, verbose_) << "sender receives intersection done.";
        } else {
            LOG_IF(INFO, verbose_) << "sender can not obtain result.";
        }

    } else {
        std::vector<Byte> cuckoo_table_seed(kRandSeedBytesLen);
        common_prng_->generate(kRandSeedBytesLen, cuckoo_table_seed.data());
        auto cuckoo_table = std::make_shared<solo::CuckooHashing<kItemBytesLen>>(num_of_bins, cuckoo_table_seed);

        // Hashing Phase
        cuckoo_table->set_num_of_hash_functions(num_of_fun_);
        cuckoo_table->insert(keys);
        cuckoo_table->map_elements();
        auto stash_size = cuckoo_table->get_stash_size();
        net->send_data(&stash_size, sizeof(std::size_t));
        if (stash_size > 0u) {
            LOG_IF(INFO, verbose_) << "stash of size is not zero.";
            throw std::invalid_argument("stash of size is not zero.");
        }
        auto cuckoo_table_values = cuckoo_table->obtain_entry_values();
        auto cuckoo_table_source_values = cuckoo_table->obtain_entry_ids();
        auto cuckoo_table_function_ids = cuckoo_table->obtain_entry_function_ids();

        LOG_IF(INFO, verbose_) << "cuckoo hash done.";

        // OPRF
        std::vector<block> masks_with_dummies_block;
        std::vector<block> masks_with_dummies;
        for (std::size_t i = 0; i < cuckoo_table_values.size(); i++) {
            masks_with_dummies_block.emplace_back(*(reinterpret_cast<block*>(cuckoo_table_values[i].data())));
        }
        nco_ot_ext_recver_->receive(net, masks_with_dummies_block, masks_with_dummies);

        LOG_IF(INFO, verbose_) << "oprf done.";

        ByteVector reduced_receiver_enc_data;
        reduced_receiver_enc_data.resize(num_of_fun_ * sender_data_size * kReduceStatisticsLen);
        net->recv_data(reduced_receiver_enc_data.data(), reduced_receiver_enc_data.size());

        std::vector<ByteVector> unpacked_reduced_receiver_enc_data;
        unpacked_reduced_receiver_enc_data.reserve(num_of_fun_ * sender_data_size);
        for (std::size_t item_idx = 0; item_idx < sender_data_size * num_of_fun_; ++item_idx) {
            unpacked_reduced_receiver_enc_data.emplace_back(
                    reduced_receiver_enc_data.begin() + item_idx * kReduceStatisticsLen,
                    reduced_receiver_enc_data.begin() + (item_idx + 1) * kReduceStatisticsLen);
        }

        std::vector<bool> intersection_indices(num_of_bins, false);
        std::size_t count = 0;
        for (std::size_t item_idx = 0; item_idx < num_of_bins; ++item_idx) {
            auto index = cuckoo_table_function_ids[item_idx];
            if (index >= num_of_fun_) {
                continue;
            }
            ByteVector search_data(reinterpret_cast<Byte*>(&masks_with_dummies[item_idx]),
                    reinterpret_cast<Byte*>(&masks_with_dummies[item_idx]) + kReduceStatisticsLen);
            if (std::find(unpacked_reduced_receiver_enc_data.begin() + index * sender_data_size,
                        unpacked_reduced_receiver_enc_data.begin() + (index + 1) * sender_data_size,
                        search_data) != (unpacked_reduced_receiver_enc_data.begin() + (index + 1) * sender_data_size)) {
                intersection_indices[cuckoo_table_source_values[item_idx]] = true;
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

        LOG_IF(INFO, verbose_) << "receiver calculate intersection done.";

        if (sender_obtain_result_) {
            LOG_IF(INFO, verbose_) << "sender can obtain result.";
            std::vector<char> serialized_key;
            serialize_string_to_char(output_keys, serialized_key);
            std::size_t count_serialize = serialized_key.size();
            net->send_data(&count_serialize, sizeof(std::size_t));
            net->send_data(serialized_key.data(), serialized_key.size());
            LOG_IF(INFO, verbose_) << "receiver sends intersection to sender.";
        } else {
            LOG_IF(INFO, verbose_) << "sender can not obtain result.";
        }
    }
}

std::size_t KkrtPSI::process_cardinality_only(
        const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys) const {
    std::size_t sender_data_size;
    std::size_t receiver_data_size;
    if (is_sender_) {
        sender_data_size = input_keys.size();
        net->recv_data(&receiver_data_size, sizeof(receiver_data_size));
        net->send_data(&sender_data_size, sizeof(sender_data_size));
    } else {
        receiver_data_size = input_keys.size();
        net->send_data(&receiver_data_size, sizeof(receiver_data_size));
        net->recv_data(&sender_data_size, sizeof(sender_data_size));
    }

    std::size_t num_of_bins = static_cast<std::size_t>(std::ceil(static_cast<double>(receiver_data_size) * epsilon_));

    std::vector<Item> keys(input_keys.size());
    auto hash = solo::Hash::create(solo::HashScheme::SHA_256);
    for (std::size_t i = 0; i < input_keys.size(); i++) {
        hash->compute(reinterpret_cast<const Byte*>(input_keys[i].data()), input_keys[i].size(),
                reinterpret_cast<Byte*>(&keys[i]), sizeof(Item));
    }

    solo::PRNGFactory prng_factory(solo::PRNGScheme::AES_ECB_CTR);
    if (is_sender_) {
        std::vector<Byte> simple_table_seed(kRandSeedBytesLen);
        common_prng_->generate(kRandSeedBytesLen, simple_table_seed.data());
        auto simple_table = std::make_shared<solo::SimpleHashing<kItemBytesLen>>(num_of_bins, simple_table_seed);

        // Hashing Phase
        simple_table->set_num_of_hash_functions(num_of_fun_);
        simple_table->insert(keys);
        simple_table->map_elements();

        std::size_t stash_size;
        net->recv_data(&stash_size, sizeof(std::size_t));
        if (stash_size > 0u) {
            LOG_IF(INFO, verbose_) << "stash of size is not zero.";
            throw std::invalid_argument("stash of size is not zero.");
        }

        auto simple_table_values = simple_table->obtain_bin_entry_values();
        auto simple_table_source_values = simple_table->obtain_bin_entry_function_ids();

        LOG_IF(INFO, verbose_) << "simple hash done.";

        // OPRF
        std::vector<std::vector<block>> masks(num_of_bins);
        std::vector<std::vector<block>> simple_table_values_block(num_of_bins);
        for (std::size_t i = 0; i < simple_table_values.size(); i++) {
            for (std::size_t j = 0; j < simple_table_values[i].size(); j++) {
                simple_table_values_block[i].emplace_back(
                        *(reinterpret_cast<block*>(simple_table_values[i][j].data())));
            }
        }
        nco_ot_ext_sender_->send(net, num_of_bins);
        std::vector<std::vector<block>> sender_enc_data(num_of_fun_);
        for (std::size_t i = 0; i < num_of_bins; i++) {
            masks[i].resize(simple_table_values_block[i].size());
            for (std::size_t j = 0; j < simple_table_values_block[i].size(); j++) {
                nco_ot_ext_sender_->encode(i, simple_table_values_block[i][j], masks[i][j]);
                sender_enc_data[simple_table_source_values[i][j]].push_back(masks[i][j]);
            }
        }

        LOG_IF(INFO, verbose_) << "oprf done.";

        std::vector<std::size_t> permutation;
        std::vector<std::vector<block>> shuffled_sender_enc_data(num_of_fun_);
        for (std::size_t i = 0; i < num_of_fun_; i++) {
            generate_permutation(prng_, sender_data_size, permutation);
            shuffled_sender_enc_data[i].assign(sender_enc_data[i].begin(), sender_enc_data[i].end());
            permute_and_undo(permutation, true, shuffled_sender_enc_data[i]);
        }

        ByteVector reduced_shuffled_sender_enc_data;
        reduced_shuffled_sender_enc_data.reserve(num_of_fun_ * sender_data_size * kReduceStatisticsLen);
        for (std::size_t i = 0; i < num_of_fun_; i++) {
            for (std::size_t j = 0; j < sender_data_size; j++) {
                reduced_shuffled_sender_enc_data.insert(reduced_shuffled_sender_enc_data.end(),
                        reinterpret_cast<Byte*>(&shuffled_sender_enc_data[i][j]),
                        reinterpret_cast<Byte*>(&shuffled_sender_enc_data[i][j]) + kReduceStatisticsLen);
            }
        }
        net->send_data(reduced_shuffled_sender_enc_data.data(), reduced_shuffled_sender_enc_data.size());

        std::size_t count = 0;
        if (sender_obtain_result_) {
            LOG_IF(INFO, verbose_) << "sender can obtain result.";
            net->recv_data(&count, sizeof(std::size_t));
            LOG_IF(INFO, verbose_) << "sender receives cardinality done.";
        } else {
            LOG_IF(INFO, verbose_) << "sender can not obtain result.";
        }
        return count;
    } else {
        std::vector<Byte> cuckoo_table_seed(kRandSeedBytesLen);
        common_prng_->generate(kRandSeedBytesLen, cuckoo_table_seed.data());
        auto cuckoo_table = std::make_shared<solo::CuckooHashing<kItemBytesLen>>(num_of_bins, cuckoo_table_seed);

        // Hashing Phase
        cuckoo_table->set_num_of_hash_functions(num_of_fun_);
        cuckoo_table->insert(keys);
        cuckoo_table->map_elements();
        auto stash_size = cuckoo_table->get_stash_size();
        net->send_data(&stash_size, sizeof(std::size_t));
        if (stash_size > 0u) {
            LOG_IF(INFO, verbose_) << "stash of size is not zero.";
            throw std::invalid_argument("stash of size is not zero.");
        }

        auto cuckoo_table_values = cuckoo_table->obtain_entry_values();
        auto cuckoo_table_source_values = cuckoo_table->obtain_entry_ids();
        auto cuckoo_table_function_ids = cuckoo_table->obtain_entry_function_ids();

        LOG_IF(INFO, verbose_) << "cuckoo hash done.";

        // OPRF
        std::vector<block> masks_with_dummies_block;
        std::vector<block> masks_with_dummies;
        for (std::size_t i = 0; i < cuckoo_table_values.size(); i++) {
            masks_with_dummies_block.emplace_back(*(reinterpret_cast<block*>(cuckoo_table_values[i].data())));
        }
        nco_ot_ext_recver_->receive(net, masks_with_dummies_block, masks_with_dummies);

        LOG_IF(INFO, verbose_) << "oprf done.";

        ByteVector reduced_receiver_enc_data;
        reduced_receiver_enc_data.resize(num_of_fun_ * sender_data_size * kReduceStatisticsLen);
        net->recv_data(reduced_receiver_enc_data.data(), reduced_receiver_enc_data.size());

        std::vector<ByteVector> unpacked_reduced_receiver_enc_data;
        unpacked_reduced_receiver_enc_data.reserve(num_of_fun_ * sender_data_size);
        for (std::size_t item_idx = 0; item_idx < sender_data_size * num_of_fun_; ++item_idx) {
            unpacked_reduced_receiver_enc_data.emplace_back(
                    reduced_receiver_enc_data.begin() + item_idx * kReduceStatisticsLen,
                    reduced_receiver_enc_data.begin() + (item_idx + 1) * kReduceStatisticsLen);
        }

        std::vector<bool> intersection_indices(num_of_bins, false);
        std::size_t count = 0;
        for (std::size_t item_idx = 0; item_idx < num_of_bins; ++item_idx) {
            auto index = cuckoo_table_function_ids[item_idx];
            if (index >= num_of_fun_) {
                continue;
            }
            ByteVector search_data(reinterpret_cast<Byte*>(&masks_with_dummies[item_idx]),
                    reinterpret_cast<Byte*>(&masks_with_dummies[item_idx]) + kReduceStatisticsLen);
            if (std::find(unpacked_reduced_receiver_enc_data.begin() + index * sender_data_size,
                        unpacked_reduced_receiver_enc_data.begin() + (index + 1) * sender_data_size,
                        search_data) != (unpacked_reduced_receiver_enc_data.begin() + (index + 1) * sender_data_size)) {
                intersection_indices[cuckoo_table_source_values[item_idx]] = true;
                ++count;
            }
        }

        LOG_IF(INFO, verbose_) << "receiver calculate cardinality done.";

        if (sender_obtain_result_) {
            LOG_IF(INFO, verbose_) << "sender can obtain result.";
            net->send_data(&count, sizeof(std::size_t));
            LOG_IF(INFO, verbose_) << "receiver sends cardinality to sender.";
        } else {
            LOG_IF(INFO, verbose_) << "sender can not obtain result.";
        }
        return count;
    }
}

void KkrtPSI::check_params(const std::shared_ptr<network::Network>& net) {
    check_consistency(is_sender_, net, "epsilon", epsilon_);
    check_consistency(is_sender_, net, "number of function", num_of_fun_);
}

template <>
std::unique_ptr<PSI> CreatePSI<PSIScheme::KKRT_PSI>() {
    return std::make_unique<KkrtPSI>();
}

}  // namespace setops
}  // namespace petace
