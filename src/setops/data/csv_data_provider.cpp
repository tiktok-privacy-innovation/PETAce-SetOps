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

#include "setops/data/csv_data_provider.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace petace {
namespace setops {

petace::setops::CsvDataProvider::CsvDataProvider(
        const std::string& file_path, bool has_header, std::size_t items_columns_num)
        : has_header_(has_header), items_columns_num_(items_columns_num) {
    if (file_path.empty()) {
        throw std::invalid_argument("file path is empty.");
    }
    file_stream_in_.open(file_path);
    if (!(file_stream_in_.eof() || file_stream_in_.good())) {
        throw std::runtime_error("file " + file_path + "read failed.");
    }

    auto count = count_rows_and_coulmns(file_stream_in_, has_header_);
    rows_num_ = count.first;
    columns_num_ = count.second;
    std::string header_line;
    if (has_header_) {
        std::getline(file_stream_in_, header_line);
    }
}

void CsvDataProvider::get_next_batch(std::size_t batch_size, std::vector<std::string>& items) {
    if (batch_size == 0) {
        return;
    }
    std::size_t read_lines_count = std::min(batch_size, rows_num_ - cur_line_idx_);
    items.reserve(read_lines_count);
    std::size_t count = 0;
    std::string cur_line;
    while (count++ < read_lines_count) {
        std::getline(file_stream_in_, cur_line);
        if (!cur_line.empty()) {
            items.emplace_back(cur_line);
        }
    }
    cur_line_idx_ += read_lines_count;
}

void CsvDataProvider::get_next_batch_2d(std::size_t batch_size, std::vector<std::vector<std::string>>& items) {
    if (batch_size == 0) {
        return;
    }
    std::size_t read_lines_count = std::min(batch_size, rows_num_ - cur_line_idx_);
    items.resize(items_columns_num_);
    std::size_t count = 0;
    std::string cur_line;
    while (count++ < read_lines_count) {
        std::getline(file_stream_in_, cur_line);
        if (!cur_line.empty()) {
            std::string column_str;
            std::stringstream ss(cur_line);
            for (std::size_t idx = 0; idx < items_columns_num_; ++idx) {
                std::getline(ss, column_str, ',');
                items[idx].emplace_back(column_str);
            }
        }
    }
    cur_line_idx_ += read_lines_count;
}

void CsvDataProvider::get_next_batch_with_payloads(
        std::size_t batch_size, std::vector<std::string>& items, std::vector<std::uint64_t>& payloads) {
    if (batch_size == 0) {
        return;
    }
    std::size_t read_lines_count = std::min(batch_size, rows_num_ - cur_line_idx_);
    items.reserve(read_lines_count);
    payloads.reserve(read_lines_count);
    std::size_t count = 0;
    std::string cur_line;
    while (count++ < read_lines_count) {
        std::getline(file_stream_in_, cur_line);
        if (!cur_line.empty()) {
            std::string column_str;
            std::stringstream ss(cur_line);
            std::getline(ss, column_str, ',');
            items.emplace_back(column_str);
            std::getline(ss, column_str, ',');
            payloads.emplace_back(std::stoull(column_str));
        }
    }
    cur_line_idx_ += read_lines_count;
}

void CsvDataProvider::get_next_batch_with_payloads_2d(std::size_t batch_size,
        std::vector<std::vector<std::string>>& items, std::vector<std::vector<std::uint64_t>>& payloads) {
    if (batch_size == 0) {
        return;
    }
    std::size_t read_lines_count = std::min(batch_size, rows_num_ - cur_line_idx_);
    items.resize(items_columns_num_);
    payloads.resize(columns_num_ - items_columns_num_);
    std::size_t count = 0;
    std::string cur_line;
    while (count++ < read_lines_count) {
        std::getline(file_stream_in_, cur_line);
        if (!cur_line.empty()) {
            std::string column_str;
            std::stringstream ss(cur_line);
            for (std::size_t idx = 0; idx < items_columns_num_; ++idx) {
                std::getline(ss, column_str, ',');
                items[idx].emplace_back(column_str);
            }
            for (std::size_t idx = 0; idx < columns_num_ - items_columns_num_; ++idx) {
                std::getline(ss, column_str, ',');
                payloads[idx].emplace_back(std::stoull(column_str));
            }
        }
    }
    cur_line_idx_ += read_lines_count;
}

void CsvDataProvider::write_data_to_file(const std::vector<std::vector<std::string>>& items,
        const std::vector<std::vector<std::uint64_t>>& payloads, const std::string& file_path, bool has_header,
        const std::vector<std::string>& header) {
    if (items.empty()) {
        throw std::invalid_argument("items can not be empty.");
    }
    std::ofstream out(file_path);
    bool has_payloads = !payloads.empty();
    if (has_header) {
        for (std::size_t i = 0; i < header.size(); ++i) {
            if (i != header.size() - 1) {
                out << header[i] << ',';
            } else {
                out << header[i] << "\n";
            }
        }
    }

    for (std::size_t j = 0; j < items[0].size(); ++j) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i != items.size() - 1) {
                out << items[i][j] << ',';
            } else if (has_payloads) {
                out << items[i][j] << ',';
            } else {
                out << items[i][j] << "\n";
            }
        }
        for (std::size_t i = 0; i < payloads.size(); ++i) {
            if (i != payloads.size() - 1) {
                out << payloads[i][j] << ',';
            } else {
                out << payloads[i][j] << "\n";
            }
        }
    }
    out.close();
}

void CsvDataProvider::seek_begin() {
    file_stream_in_.clear();
    file_stream_in_.seekg(0, file_stream_in_.beg);
    std::string header_line;
    if (has_header_) {
        std::getline(file_stream_in_, header_line);
    }
    cur_line_idx_ = 0;
}

std::pair<std::size_t, std::size_t> CsvDataProvider::count_rows_and_coulmns(std::ifstream& in, bool has_header) {
    in.clear();
    in.seekg(0, in.beg);

    std::size_t raws = 0;
    std::size_t cols = 0;

    std::string line;
    std::getline(in, line);
    std::string column_str;
    std::stringstream ss(line);
    while (std::getline(ss, column_str, ',')) {
        if (!column_str.empty())
            ++cols;
    }
    if (!has_header) {
        ++raws;
    }
    while (getline(in, line)) {
        if (!line.empty()) {
            ++raws;
        }
    }

    in.clear();
    in.seekg(0, in.beg);
    return std::make_pair(raws, cols);
}

}  // namespace setops
}  // namespace petace
