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

#include <string>
#include <utility>
#include <vector>

#include "solo/sampling.h"

namespace petace {
namespace setops {

const std::size_t kIdentifierLen = 32;

/**
 * @brief Generates ramdom keys using prng.
 *
 * The identifier is filled alternately with numbers and alphabets.
 *
 * @param[in] prng A pseudorandom number generator .
 * @param[in] n The number of keys need to be generated.
 * @param[in] suffix The suffix that needs to be added to the end of key.
 * @param[in] result The generated ramdom keys.
 */
inline void generate_random_keys(
        petace::solo::PRNG& prng, std::size_t n, const std::string& suffix, std::vector<std::string>& result) {
    const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const std::string number = "0123456789";
    result.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        std::string identifier;
        for (std::size_t idx = 0; idx < kIdentifierLen; ++idx) {
            if (idx & 1) {
                identifier += alphabet[(static_cast<std::uint8_t>(sample_uniform_byte(prng))) % alphabet.size()];
            } else {
                identifier += number[(static_cast<std::uint8_t>(sample_uniform_byte(prng))) % number.size()];
            }
        }
        identifier += suffix;
        result.emplace_back(identifier);
    }
}

/**
 * @brief Generates ramdom features using prng.
 *
 * @param[in] prng A pseudorandom number generator .
 * @param[in] n The number of features need to be generated.
 * @param[in] is_zero A bool indiciates whether the random feature should be set to zero.
 * @param[in] result The generated ramdom features.
 */
inline void generate_random_features(
        petace::solo::PRNG& prng, std::size_t n, bool is_zero, std::vector<std::uint64_t>& result) {
    result.resize(n);
    if (!is_zero) {
        for (std::size_t i = 0; i < n; ++i) {
            result[i] = solo::sample_uniform_uint64(prng);
        }
    }
}

}  // namespace setops
}  // namespace petace
