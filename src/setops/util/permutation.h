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
#include <utility>
#include <vector>

#include "solo/prng.h"
#include "solo/sampling.h"

namespace petace {
namespace setops {

/**
 * @brief Generates random swap permutation.
 *
 * @param[in] prng A pseudorandom number generator to generate permutation.
 * @param[in] size The size of permutation.
 * @param[out] permutation The generated random swap permutation.
 */
inline void generate_permutation(
        std::shared_ptr<petace::solo::PRNG> prng, std::size_t n, std::vector<std::size_t>& permutation) {
    permutation.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        permutation[i] = i;
    }
    std::shuffle(permutation.begin(), permutation.end(), petace::solo::PRNGStandard(prng));
}

/**
 * @brief Applies or un-applies permutation given data, permutation and is_permute flag.
 *
 * @param[in] permutation The permutation to permute data.
 * @param[in] is A bool indicates applies or un-applies permutation.
 * @param[in] data The data need to be permuted.
 * @param[out] data The permuted data.
 */
template <typename T>
inline void permute_and_undo(const std::vector<std::size_t>& permutation, bool is_permute, std::vector<T>& data) {
    std::vector<T> output;
    output.resize(data.size());
    if (is_permute) {
        for (std::size_t i = 0; i < permutation.size(); ++i) {
            output[i] = data[permutation[i]];
        }
    } else {
        for (std::size_t i = 0; i < permutation.size(); ++i) {
            output[permutation[i]] = data[i];
        }
    }
    data.clear();
    std::swap(output, data);
}

}  // namespace setops
}  // namespace petace
