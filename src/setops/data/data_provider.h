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
 * @brief Abstract class for providing data from different data sources.
 */
class DataProvider {
public:
    DataProvider() {
    }

    virtual ~DataProvider() {
    }

    /**
     * @brief Reads items.
     *
     * Reads batch_size items and stores result in items.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of data read form the data source.
     */
    virtual void get_next_batch(std::size_t batch_size, std::vector<std::string>& items) = 0;

    /**
     * @brief Reads two-dimensional items.
     *
     * Reads batch_size two-dimensional items and stores result in items.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of two-dimensional data read form the data source.
     */
    virtual void get_next_batch_2d(std::size_t batch_size, std::vector<std::vector<std::string>>& items) = 0;

    /**
     * @brief Reads items and payloads.
     *
     * Reads batch_size items and batch_size payloads and stores results in items and payloads respectively.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of items read form the data source.
     * @param[out] payloads A batch of payloads corresponding to the items read form the data source.
     */
    virtual void get_next_batch_with_payloads(
            std::size_t batch_size, std::vector<std::string>& items, std::vector<std::uint64_t>& payloads) = 0;

    /**
     * @brief Reads two-dimensional items and two-dimensional payloads.
     *
     * Reads batch_size two-dimensional items and batch_size two-dimensional payloads and stores results in items and
     * payloads respectively.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of two-dimensional items read form the data source.
     * @param[out] payloads A batch of two-dimensional payloads corresponding to the items read form the data source.
     */
    virtual void get_next_batch_with_payloads_2d(std::size_t batch_size, std::vector<std::vector<std::string>>& items,
            std::vector<std::vector<std::uint64_t>>& payloads) = 0;
};

}  // namespace setops
}  // namespace petace
