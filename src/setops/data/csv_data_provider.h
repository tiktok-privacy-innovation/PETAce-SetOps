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

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "setops/data/data_provider.h"

namespace petace {
namespace setops {

/**
 * @brief Implementation for providing data from csv files.
 */
class CsvDataProvider : public DataProvider {
public:
    /**
     * @brief Constructor of CsvDataProvider.
     *
     * @param[in] file_path The path of input file.
     * @param[in] has_header A bool indicates whether the input file has header.
     * @param[in] items_columns_num The number of columns of items.
     */
    CsvDataProvider(const std::string& file_path, bool has_header, std::size_t items_columns_num);

    ~CsvDataProvider() = default;

    /**
     * @brief Reads items.
     *
     * Reads batch_size items and stores result in items.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of data read form csv.
     */
    void get_next_batch(std::size_t batch_size, std::vector<std::string>& items) override;

    /**
     * @brief Reads two-dimensional items.
     *
     * Reads batch_size two-dimensional items and stores result in items.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of two-dimensional data read form csv.
     */
    void get_next_batch_2d(std::size_t batch_size, std::vector<std::vector<std::string>>& items) override;

    /**
     * @brief Reads items and payloads.
     *
     * Reads batch_size items and batch_size payloads and stores results in items and payloads respectively.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of items read form the csv.
     * @param[out] payloads A batch of payloads corresponding to the items read form csv.
     */
    void get_next_batch_with_payloads(
            std::size_t batch_size, std::vector<std::string>& items, std::vector<std::uint64_t>& payloads) override;

    /**
     * @brief Reads two-dimensional items and two-dimensional payloads.
     *
     * Reads batch_size two-dimensional items and batch_size two-dimensional payloads and stores results in items and
     * payloads respectively.
     *
     * @param[in] batch_size The size of a batch of data.
     * @param[out] items A batch of two-dimensional items read form csv.
     * @param[out] payloads A batch of two-dimensional payloads corresponding to the items read form csv.
     */
    void get_next_batch_with_payloads_2d(std::size_t batch_size, std::vector<std::vector<std::string>>& items,
            std::vector<std::vector<std::uint64_t>>& payloads) override;

    /**
     * @brief Writes items and payloads to csv file.
     *
     * @param[in] items A batch of two-dimensional items that need to be wrote to csv.
     * @param[in] payloads A batch of two-dimensional payloads that need to be wrote to csv.
     * @param[in] file_path The path of output file.
     * @param[in] has_header If true, place the header in the first raw of file.
     * @param[in] header The header information about the items and payloads.
     */
    static void write_data_to_file(const std::vector<std::vector<std::string>>& items,
            const std::vector<std::vector<std::uint64_t>>& payloads, const std::string& file_path, bool has_header,
            const std::vector<std::string>& header);

    /**
     * @brief Seeks to the begin of csv ifstream.
     */
    void seek_begin();

private:
    static std::pair<std::size_t, std::size_t> count_rows_and_coulmns(std::ifstream& in, bool has_header);

    bool has_header_ = false;
    std::ifstream file_stream_in_;

    std::size_t cur_line_idx_ = 0;
    std::size_t rows_num_ = 0;
    std::size_t columns_num_ = 0;
    std::size_t items_columns_num_ = 0;
};

}  // namespace setops
}  // namespace petace
