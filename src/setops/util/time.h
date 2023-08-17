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

#include <chrono>

namespace petace {
namespace setops {

/**
 * @brief Gets a std::chrono::time_point representing the current value of the clock.
 */
inline std::chrono::time_point<std::chrono::high_resolution_clock> clock_start() {
    return std::chrono::high_resolution_clock::now();
}

/**
 * @brief Gets the count of ticks in microseconds form clock start to now.
 */
inline std::int64_t time_from(const std::chrono::time_point<std::chrono::high_resolution_clock>& start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start)
            .count();
}

}  // namespace setops
}  // namespace petace
