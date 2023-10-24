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
#include <vector>

namespace petace {
namespace setops {

/**
 * @brief Serialize the vector<string>.
 *
 * @param[in] input The input of vector<string>.
 * @param[out] output The serialized result.
 */
inline void serialize_string_to_char(const std::vector<std::string>& input, std::vector<char>& output) {
    for (std::size_t i = 0; i < input.size(); i++) {
        std::string str = input[i];
        for (std::size_t j = 0; j < str.size(); j++) {
            output.push_back(str[j]);
        }
        output.push_back(0);
    }
    return;
}

/**
 * @brief Deserialize the vector<char>.
 *
 * @param[in] input The input of vector<char>.
 * @param[out] output The deserialized result.
 */
inline void deserialize_string_from_char(const std::vector<char>& input, std::vector<std::string>& output) {
    for (std::size_t i = 0; i < input.size();) {
        const char* begin = &input[i];
        int size = 0;
        while (input[i++]) {
            size += 1;
        }
        output.push_back(std::string(begin, size));
    }
}

}  // namespace setops
}  // namespace petace
