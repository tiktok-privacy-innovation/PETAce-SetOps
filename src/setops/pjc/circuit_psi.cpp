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
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "glog/logging.h"

#include "solo/prng.h"

#include "setops/util/parameter_check.h"

namespace petace {
namespace setops {

void CircuitPSI::init(const std::shared_ptr<network::Network>& net, const json& params) {
    // set parameter
    verbose_ = params["common"]["verbose"];
    is_sender_ = params["common"]["is_sender"];
    epsilon_ = params["circuit_psi_params"]["epsilon"];
    epsilon_hint_ = params["circuit_psi_params"]["fun_epsilon"];
    num_of_fun_ = params["circuit_psi_params"]["fun_num"];
    num_of_fun_hint_ = params["circuit_psi_params"]["hint_fun_num"];

    check_params(net);

    LOG_IF(INFO, verbose_) << "\nCircuit PSI parameters: \n" << params.dump(4);

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

    //  mpc
    mpc_op_ = std::make_shared<duet::Duet>(net, is_sender_ == true ? 0 : 1);
}

void CircuitPSI::process(const std::shared_ptr<network::Network>& net, const std::vector<std::string>& input_keys,
        const std::vector<std::vector<std::uint64_t>>& input_features,
        std::vector<std::vector<std::uint64_t>>& output_shares) const {
    std::size_t sender_data_size;
    std::size_t sender_feature_size;
    std::size_t receiver_data_size;
    std::size_t receiver_feature_size;
    if (is_sender_) {
        sender_data_size = input_keys.size();
        sender_feature_size = input_features.size();
        net->recv_data(&receiver_data_size, sizeof(receiver_data_size));
        net->recv_data(&receiver_feature_size, sizeof(receiver_feature_size));
        net->send_data(&sender_data_size, sizeof(sender_data_size));
        net->send_data(&sender_feature_size, sizeof(sender_feature_size));
    } else {
        receiver_data_size = input_keys.size();
        receiver_feature_size = input_features.size();
        net->send_data(&receiver_data_size, sizeof(receiver_data_size));
        net->send_data(&receiver_feature_size, sizeof(receiver_feature_size));
        net->recv_data(&sender_data_size, sizeof(sender_data_size));
        net->recv_data(&sender_feature_size, sizeof(sender_feature_size));
    }

    std::size_t num_of_bins = static_cast<std::size_t>(std::ceil(static_cast<double>(receiver_data_size) * epsilon_));
    std::size_t num_of_bins_hint =
            static_cast<std::size_t>(std::ceil(epsilon_hint_ * static_cast<double>(sender_data_size * num_of_fun_)));
    if (sender_data_size * num_of_fun_ < num_of_bins) {
        num_of_bins_hint = static_cast<std::size_t>(std::ceil(epsilon_hint_ * static_cast<double>(num_of_bins)));
    }

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
        for (std::size_t i = 0; i < num_of_bins; i++) {
            masks[i].resize(simple_table_values_block[i].size());
            for (std::size_t j = 0; j < simple_table_values_block[i].size(); j++) {
                nco_ot_ext_sender_->encode(i, simple_table_values_block[i][j], masks[i][j]);
            }
        }

        LOG_IF(INFO, verbose_) << "oprf done.";

        // Hint Computation
        std::vector<std::uint64_t> content_of_bins;
        for (std::size_t i = 0; i < num_of_bins; i++) {
            std::uint64_t content;
            prng_->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&content));
            content_of_bins.push_back(content);
        }

        std::unordered_map<std::string, HashLocMap> table_loc;
        std::vector<Item> filter_inputs;
        for (std::size_t i = 0; i < num_of_bins; i++) {
            std::size_t bin_size = simple_table_values[i].size();
            for (std::size_t j = 0; j < bin_size; j++) {
                table_loc[std::string(reinterpret_cast<char*>(simple_table_values[i][j].data()), sizeof(Item))].bin =
                        static_cast<int>(i);
                table_loc[std::string(reinterpret_cast<char*>(simple_table_values[i][j].data()), sizeof(Item))].index =
                        static_cast<int>(j);
                filter_inputs.push_back(simple_table_values[i][j]);
            }
        }

        std::vector<Byte> local_cuckoo_table_seed(kRandSeedBytesLen);
        common_prng_->generate(kRandSeedBytesLen, local_cuckoo_table_seed.data());
        auto local_cuckoo_table =
                std::make_shared<solo::CuckooHashing<kItemBytesLen>>(num_of_bins_hint, local_cuckoo_table_seed);
        local_cuckoo_table->set_num_of_hash_functions(num_of_fun_hint_);
        local_cuckoo_table->insert(filter_inputs);
        local_cuckoo_table->map_elements();

        stash_size = local_cuckoo_table->get_stash_size();
        net->send_data(&stash_size, sizeof(std::size_t));
        if (stash_size > 0u) {
            LOG_IF(INFO, verbose_) << "stash of size is not zero.";
            throw std::invalid_argument("stash of size is not zero.");
        }

        std::vector<std::uint64_t> garbled_cuckoo_filter(num_of_bins_hint);
        auto local_cuckoo_bin_occupancy = local_cuckoo_table->obtain_bin_occupancy();
        auto local_cuckoo_table_source_values = local_cuckoo_table->obtain_entry_source_values();
        auto local_cuckoo_table_functions = local_cuckoo_table->obtain_entry_function_ids();
        for (std::size_t i = 0; i < num_of_bins_hint; i++) {
            if (local_cuckoo_bin_occupancy[i]) {
                auto element = local_cuckoo_table_source_values[i];
                auto function_id = local_cuckoo_table_functions[i];
                HashLocMap location = table_loc[std::string(reinterpret_cast<char*>(element.data()), sizeof(Item))];
                std::vector<Byte> seed(kRandSeedBytesLen);
                std::memcpy(seed.data(),
                        reinterpret_cast<Byte*>(const_cast<block*>(&masks[location.bin][location.index])),
                        kRandSeedBytesLen);
                auto local_prng = prng_factory.create(seed);
                std::uint64_t pad = 0;
                for (std::size_t j = 0; j <= function_id; j++) {
                    local_prng->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&pad));
                }
                garbled_cuckoo_filter[i] = content_of_bins[location.bin] ^ pad;
            } else {
                prng_->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&garbled_cuckoo_filter[i]));
            }
        }

        net->send_data(garbled_cuckoo_filter.data(), num_of_bins_hint * sizeof(std::uint64_t));
        std::vector<duet::ArithMatrix> feature_shares(sender_feature_size);
        if (sender_feature_size != 0) {
            for (std::size_t i = 0; i < sender_feature_size; i++) {
                feature_shares[i].resize(num_of_bins, num_of_fun_hint_);
            }
            std::unordered_map<std::string, std::vector<std::uint64_t>> table_features_loc;
            for (std::size_t i = 0; i < sender_data_size; i++) {
                std::vector<std::uint64_t> feature;
                for (std::size_t j = 0; j < sender_feature_size; j++) {
                    feature.emplace_back(input_features[j][i]);
                }
                for (std::size_t j = 0; j < num_of_fun_; j++) {
                    Item keys_xor_fun = keys[i];
                    keys_xor_fun[0] ^= Byte(j);
                    table_features_loc.emplace(
                            std::string(reinterpret_cast<char*>(keys_xor_fun.data()), sizeof(Item)), feature);
                }
            }

            for (std::size_t fid = 0; fid < sender_feature_size; fid++) {
                std::vector<std::uint64_t> content_of_bins_features;
                for (std::size_t i = 0; i < num_of_bins; i++) {
                    std::uint64_t content;
                    prng_->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&content));
                    content_of_bins_features.push_back(content);
                    for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                        feature_shares[fid].shares()(i, j) = content_of_bins_features[i];
                    }
                }
                std::vector<std::uint64_t> garbled_cuckoo_filter_features(num_of_bins_hint);
                for (std::size_t i = 0; i < num_of_bins_hint; i++) {
                    if (local_cuckoo_bin_occupancy[i]) {
                        auto element = local_cuckoo_table_source_values[i];
                        auto function_id = local_cuckoo_table_functions[i];
                        HashLocMap location =
                                table_loc[std::string(reinterpret_cast<char*>(element.data()), sizeof(Item))];

                        std::vector<Byte> seed(kRandSeedBytesLen);
                        auto seed_block = masks[location.bin][location.index] ^ _mm_set_epi64x(0, fid);
                        std::memcpy(seed.data(), reinterpret_cast<Byte*>(const_cast<block*>(&seed_block)),
                                kRandSeedBytesLen);
                        auto local_prng = prng_factory.create(seed);
                        std::uint64_t pad = 0;
                        for (std::size_t j = 0; j <= function_id; j++) {
                            local_prng->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&pad));
                        }
                        garbled_cuckoo_filter_features[i] =
                                (table_features_loc[std::string(reinterpret_cast<char*>(element.data()), sizeof(Item))]
                                                   [fid] -
                                        content_of_bins_features[location.bin]) ^
                                pad;
                    } else {
                        prng_->generate(
                                sizeof(std::uint64_t), reinterpret_cast<Byte*>(&garbled_cuckoo_filter_features[i]));
                    }
                }
                net->send_data(garbled_cuckoo_filter_features.data(), num_of_bins_hint * sizeof(std::uint64_t));
            }
        }

        LOG_IF(INFO, verbose_) << "opprf computation done.";

        duet::ArithMatrix receiver_data(num_of_bins, num_of_fun_hint_);
        duet::ArithMatrix sender_data(num_of_bins, num_of_fun_hint_);

        receiver_data.shares().setZero();
        for (std::size_t i = 0; i < num_of_bins; i++) {
            for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                sender_data.shares()(i, j) = content_of_bins[i] & kReduceBitsLen;
            }
        }

        duet::BoolMatrix result(num_of_bins, num_of_fun_hint_);
        mpc_op_->equal(net, sender_data, receiver_data, result);

        output_shares.resize(sender_feature_size + receiver_feature_size + 1);
        for (std::size_t i = 0; i < output_shares.size(); i++) {
            output_shares[i].resize(num_of_bins);
            output_shares[i].assign(num_of_bins, 0);
        }
        for (std::size_t i = 0; i < num_of_bins; i++) {
            for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                output_shares[0][i] ^= result.shares()(i, j);
            }
        }

        if ((sender_feature_size != 0) || (receiver_feature_size != 0)) {
            std::vector<duet::ArithMatrix> feature_result(sender_feature_size);
            for (std::size_t i = 0; i < sender_feature_size; i++) {
                feature_result[i].resize(num_of_bins, num_of_fun_hint_);
            }
            for (std::size_t i = 0; i < sender_feature_size; i++) {
                mpc_op_->multiplexer(net, result, feature_shares[i], feature_result[i]);
            }
            for (std::size_t i = 0; i < num_of_bins; i++) {
                for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                    for (std::size_t k = 0; k < sender_feature_size; k++) {
                        output_shares[k + 1][i] += feature_result[k].shares()(i, j);
                    }
                    for (std::size_t k = 0; k < receiver_feature_size; k++) {
                        output_shares[sender_feature_size + k + 1][i] += 0;
                    }
                }
            }
        }
        LOG_IF(INFO, verbose_) << "secret shares computation done.";
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

        LOG_IF(INFO, verbose_) << "cuckoo hash done.";

        // OPRF
        std::vector<block> masks_with_dummies_block;
        std::vector<block> masks_with_dummies;
        for (std::size_t i = 0; i < cuckoo_table_values.size(); i++) {
            masks_with_dummies_block.emplace_back(*(reinterpret_cast<block*>(cuckoo_table_values[i].data())));
        }

        nco_ot_ext_recver_->receive(net, masks_with_dummies_block, masks_with_dummies);

        LOG_IF(INFO, verbose_) << "oprf done.";

        net->recv_data(&stash_size, sizeof(std::size_t));
        if (stash_size > 0u) {
            LOG_IF(INFO, verbose_) << "stash of size is not zero.";
            throw std::invalid_argument("stash of size is not zero.");
        }

        // HInt
        std::vector<std::uint64_t> garbled_cuckoo_filter(num_of_bins_hint);
        net->recv_data(garbled_cuckoo_filter.data(), num_of_bins_hint * sizeof(std::uint64_t));

        std::vector<Byte> local_cuckoo_table_seed(kRandSeedBytesLen);
        common_prng_->generate(kRandSeedBytesLen, local_cuckoo_table_seed.data());
        auto garbled_cuckoo_table =
                std::make_shared<solo::CuckooHashing<kItemBytesLen>>(num_of_bins_hint, local_cuckoo_table_seed);
        garbled_cuckoo_table->set_num_of_hash_functions(num_of_fun_hint_);
        garbled_cuckoo_table->insert(cuckoo_table_values);
        auto addresses = garbled_cuckoo_table->get_element_addresses();

        std::vector<std::uint64_t> content_of_bins(num_of_bins * num_of_fun_hint_);
        for (std::size_t i = 0; i < num_of_bins; i++) {
            std::vector<Byte> seed(kRandSeedBytesLen);
            std::memcpy(seed.data(), reinterpret_cast<Byte*>(const_cast<block*>(&masks_with_dummies[i])),
                    kRandSeedBytesLen);
            auto local_prng = prng_factory.create(seed);
            for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                std::uint64_t pad;
                local_prng->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&pad));
                content_of_bins[i * num_of_fun_hint_ + j] =
                        garbled_cuckoo_filter[addresses[i * num_of_fun_hint_ + j]] ^ pad;
            }
        }

        std::vector<std::vector<std::uint64_t>> content_of_bins_features(
                sender_feature_size, std::vector<std::uint64_t>(num_of_bins * num_of_fun_hint_));
        std::unordered_map<std::string, std::vector<std::uint64_t>> table_features_loc;
        if ((sender_feature_size != 0) || (receiver_feature_size != 0)) {
            for (std::size_t i = 0; i < receiver_data_size; i++) {
                std::vector<std::uint64_t> feature;
                for (std::size_t j = 0; j < receiver_feature_size; j++) {
                    feature.emplace_back(input_features[j][i]);
                }
                table_features_loc.emplace(std::string(reinterpret_cast<char*>(keys[i].data()), sizeof(Item)), feature);
            }

            for (std::size_t fid = 0; fid < sender_feature_size; fid++) {
                std::vector<std::uint64_t> garbled_cuckoo_filter_features(num_of_bins_hint);
                net->recv_data(garbled_cuckoo_filter_features.data(), num_of_bins_hint * sizeof(std::uint64_t));

                for (std::size_t i = 0; i < num_of_bins; i++) {
                    std::vector<Byte> seed(kRandSeedBytesLen);
                    auto seed_block = masks_with_dummies[i] ^ _mm_set_epi64x(0, fid);
                    std::memcpy(
                            seed.data(), reinterpret_cast<Byte*>(const_cast<block*>(&seed_block)), kRandSeedBytesLen);
                    auto local_prng = prng_factory.create(seed);
                    for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                        std::uint64_t pad;
                        local_prng->generate(sizeof(std::uint64_t), reinterpret_cast<Byte*>(&pad));
                        content_of_bins_features[fid][i * num_of_fun_hint_ + j] =
                                garbled_cuckoo_filter_features[addresses[i * num_of_fun_hint_ + j]] ^ pad;
                    }
                }
            }
        }

        LOG_IF(INFO, verbose_) << "opprf computation done.";

        duet::ArithMatrix receiver_data(num_of_bins, num_of_fun_hint_);
        duet::ArithMatrix sender_data(num_of_bins, num_of_fun_hint_);

        sender_data.shares().setZero();
        for (std::size_t i = 0; i < num_of_bins; i++) {
            for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                receiver_data.shares()(i, j) = content_of_bins[i * num_of_fun_hint_ + j] & kReduceBitsLen;
            }
        }

        duet::BoolMatrix result(num_of_bins, num_of_fun_hint_);
        mpc_op_->equal(net, sender_data, receiver_data, result);

        output_shares.resize(sender_feature_size + receiver_feature_size + 1);
        for (std::size_t i = 0; i < output_shares.size(); i++) {
            output_shares[i].resize(num_of_bins);
            output_shares[i].assign(num_of_bins, 0);
        }
        for (std::size_t i = 0; i < num_of_bins; i++) {
            for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                output_shares[0][i] ^= result.shares()(i, j);
            }
        }

        if ((sender_feature_size != 0) || (receiver_feature_size != 0)) {
            auto cuckoo_bin_occupancy = cuckoo_table->obtain_bin_occupancy();
            auto cuckoo_table_source_values = cuckoo_table->obtain_entry_source_values();
            std::vector<duet::ArithMatrix> feature_shares(sender_feature_size);
            std::vector<duet::ArithMatrix> feature_result(sender_feature_size);
            for (std::size_t i = 0; i < sender_feature_size; i++) {
                feature_shares[i].resize(num_of_bins, num_of_fun_hint_);
                feature_result[i].resize(num_of_bins, num_of_fun_hint_);
                for (std::size_t j = 0; j < num_of_bins; j++) {
                    for (std::size_t k = 0; k < num_of_fun_hint_; k++) {
                        feature_shares[i].shares()(j, k) = content_of_bins_features[i][j * num_of_fun_hint_ + k];
                    }
                }
                mpc_op_->multiplexer(net, result, feature_shares[i], feature_result[i]);
            }
            for (std::size_t i = 0; i < num_of_bins; i++) {
                for (std::size_t j = 0; j < num_of_fun_hint_; j++) {
                    for (std::size_t k = 0; k < sender_feature_size; k++) {
                        output_shares[k + 1][i] += feature_result[k].shares()(i, j);
                    }
                }

                if (cuckoo_bin_occupancy[i]) {
                    for (std::size_t k = 0; k < receiver_feature_size; k++) {
                        output_shares[sender_feature_size + k + 1][i] += table_features_loc[std::string(
                                reinterpret_cast<char*>(cuckoo_table_source_values[i].data()), sizeof(Item))][k];
                    }
                }
            }
        }
        LOG_IF(INFO, verbose_) << "secret shares computation done.";
    }
}

void CircuitPSI::check_params(const std::shared_ptr<network::Network>& net) {
    check_consistency(is_sender_, net, "epsilon", epsilon_);
    check_consistency(is_sender_, net, "epsilon_hint", epsilon_hint_);
    check_consistency(is_sender_, net, "number of function", num_of_fun_);
    check_consistency(is_sender_, net, "number of hint function", num_of_fun_hint_);
}

template <>
std::unique_ptr<PJC> CreatePJC<PJCScheme::CIRCUIT_PSI>() {
    return std::make_unique<CircuitPSI>();
}

}  // namespace setops
}  // namespace petace
