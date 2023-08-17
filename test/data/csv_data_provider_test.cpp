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

#include "gtest/gtest.h"

namespace petace {
namespace setops {

class CsvDataProviderTest : public ::testing::Test {
public:
    std::string tmp_data_path = "/tmp/tmp_data.csv";
    static void SetUpTestCase() {
    }
    static void TearDownTestCase() {
        std::remove("/tmp/tmp_data.csv");
    }
};

TEST_F(CsvDataProviderTest, get_next_batch) {
    std::vector<std::vector<std::string>> items = {{"id1", "id2", "id3"}};
    std::vector<std::vector<std::uint64_t>> payloads;
    std::vector<std::string> header = {"ID"};
    CsvDataProvider::write_data_to_file(items, payloads, tmp_data_path, true, header);
    CsvDataProvider csv_data_provider(tmp_data_path, true, 1);
    std::vector<std::string> read_items;
    std::vector<std::uint64_t> read_payloads;
    csv_data_provider.get_next_batch(3, read_items);
    ASSERT_EQ(items[0], read_items);

    csv_data_provider.seek_begin();
    read_items.clear();
    std::vector<std::string> expected_items_1 = {"id1", "id2"};
    csv_data_provider.get_next_batch(2, read_items);
    ASSERT_EQ(read_items, expected_items_1);

    std::vector<std::string> expected_items_2 = {{"id3"}};
    read_items.clear();
    csv_data_provider.get_next_batch(1, read_items);
    ASSERT_EQ(read_items, expected_items_2);

    read_items.clear();
    csv_data_provider.get_next_batch(0, read_items);
    ASSERT_TRUE(read_items.empty());
}

TEST_F(CsvDataProviderTest, get_next_batch_without_header) {
    std::vector<std::vector<std::string>> items = {{"id1", "id2", "id3"}};
    std::vector<std::vector<std::uint64_t>> payloads;
    std::vector<std::string> header = {"ID"};
    CsvDataProvider::write_data_to_file(items, payloads, tmp_data_path, false, header);
    CsvDataProvider csv_data_provider(tmp_data_path, false, 1);
    std::vector<std::string> read_items;
    std::vector<std::uint64_t> read_payloads;
    csv_data_provider.get_next_batch(3, read_items);
    ASSERT_EQ(items[0], read_items);

    csv_data_provider.seek_begin();
    read_items.clear();
    std::vector<std::string> expected_items_1 = {"id1", "id2"};
    csv_data_provider.get_next_batch(2, read_items);
    ASSERT_EQ(read_items, expected_items_1);

    std::vector<std::string> expected_items_2 = {{"id3"}};
    read_items.clear();
    csv_data_provider.get_next_batch(1, read_items);
    ASSERT_EQ(read_items, expected_items_2);

    read_items.clear();
    csv_data_provider.get_next_batch(0, read_items);
    ASSERT_TRUE(read_items.empty());
}

TEST_F(CsvDataProviderTest, get_next_batch_with_payload) {
    std::vector<std::vector<std::string>> items = {{"id1", "id2", "id3"}};
    std::vector<std::vector<std::uint64_t>> payloads = {{1, 2, 3}};
    std::vector<std::string> header = {"ID", "Payload"};
    CsvDataProvider::write_data_to_file(items, payloads, tmp_data_path, true, header);
    CsvDataProvider csv_data_provider(tmp_data_path, true, 1);
    std::vector<std::string> read_items;
    std::vector<std::uint64_t> read_payloads;
    csv_data_provider.get_next_batch_with_payloads(3, read_items, read_payloads);
    ASSERT_EQ(items[0], read_items);
    ASSERT_EQ(payloads[0], read_payloads);

    csv_data_provider.seek_begin();
    std::vector<std::string> expected_items_1 = {"id1", "id2"};
    std::vector<std::uint64_t> expected_payloads_1 = {1, 2};
    read_items.clear();
    read_payloads.clear();
    csv_data_provider.get_next_batch_with_payloads(2, read_items, read_payloads);
    ASSERT_EQ(read_items, expected_items_1);
    ASSERT_EQ(read_payloads, expected_payloads_1);

    std::vector<std::string> expected_items_2 = {{"id3"}};
    std::vector<std::uint64_t> expected_payloads_2 = {3};
    read_items.clear();
    read_payloads.clear();
    csv_data_provider.get_next_batch_with_payloads(1, read_items, read_payloads);
    ASSERT_EQ(read_items, expected_items_2);
    ASSERT_EQ(read_payloads, expected_payloads_2);

    read_items.clear();
    read_payloads.clear();
    csv_data_provider.get_next_batch_with_payloads(0, read_items, read_payloads);
    ASSERT_TRUE(read_items.empty());
    ASSERT_TRUE(read_payloads.empty());
}

TEST_F(CsvDataProviderTest, get_next_batch_2d) {
    std::vector<std::vector<std::string>> items = {{"id1", "id2", "id3"}, {"ip1", "ip2", "ip3"}};
    std::vector<std::vector<std::uint64_t>> payloads;
    std::vector<std::string> header = {"ID", "IP"};
    CsvDataProvider::write_data_to_file(items, payloads, tmp_data_path, true, header);
    CsvDataProvider csv_data_provider(tmp_data_path, true, 2);
    std::vector<std::vector<std::string>> read_items;

    csv_data_provider.get_next_batch_2d(3, read_items);
    ASSERT_EQ(items, read_items);

    csv_data_provider.seek_begin();
    read_items.clear();
    std::vector<std::vector<std::string>> expected_items_1 = {{"id1", "id2"}, {"ip1", "ip2"}};
    csv_data_provider.get_next_batch_2d(2, read_items);
    ASSERT_EQ(read_items, expected_items_1);

    std::vector<std::vector<std::string>> expected_items_2 = {{"id3"}, {"ip3"}};
    read_items.clear();
    csv_data_provider.get_next_batch_2d(1, read_items);
    ASSERT_EQ(read_items, expected_items_2);

    read_items.clear();
    csv_data_provider.get_next_batch_2d(0, read_items);
    ASSERT_TRUE(read_items.empty());
}

TEST_F(CsvDataProviderTest, get_next_batch_with_payload_2d) {
    std::vector<std::vector<std::string>> items = {{"id1", "id2", "id3"}, {"ip1", "ip2", "ip3"}};
    std::vector<std::vector<std::uint64_t>> payloads = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::vector<std::string> header = {"ID", "IP", "Payload1", "Payload2", "Payload3"};
    CsvDataProvider::write_data_to_file(items, payloads, tmp_data_path, true, header);
    CsvDataProvider csv_data_provider(tmp_data_path, true, 2);
    std::vector<std::vector<std::string>> read_items;
    std::vector<std::vector<std::uint64_t>> read_payloads;
    csv_data_provider.get_next_batch_with_payloads_2d(3, read_items, read_payloads);
    ASSERT_EQ(items, read_items);
    ASSERT_EQ(payloads, read_payloads);

    csv_data_provider.seek_begin();
    std::vector<std::vector<std::string>> expected_items_1 = {{"id1", "id2"}, {"ip1", "ip2"}};
    std::vector<std::vector<std::uint64_t>> expected_payloads_1 = {{1, 2}, {4, 5}, {7, 8}};
    read_items.clear();
    read_payloads.clear();
    csv_data_provider.get_next_batch_with_payloads_2d(2, read_items, read_payloads);
    ASSERT_EQ(read_items, expected_items_1);
    ASSERT_EQ(read_payloads, expected_payloads_1);

    std::vector<std::vector<std::string>> expected_items_2 = {{"id3"}, {"ip3"}};
    std::vector<std::vector<std::uint64_t>> expected_payloads_2 = {{3}, {6}, {9}};
    read_items.clear();
    read_payloads.clear();
    csv_data_provider.get_next_batch_with_payloads_2d(1, read_items, read_payloads);
    ASSERT_EQ(read_items, expected_items_2);
    ASSERT_EQ(read_payloads, expected_payloads_2);

    read_items.clear();
    read_payloads.clear();
    csv_data_provider.get_next_batch_with_payloads_2d(0, read_items, read_payloads);
    ASSERT_TRUE(read_items.empty());
    ASSERT_TRUE(read_payloads.empty());
}

TEST_F(CsvDataProviderTest, file_path_empty) {
    auto expection_test = []() { CsvDataProvider csv_data_provider("", true, 2); };
    EXPECT_THROW(expection_test(), std::invalid_argument);
}

}  // namespace setops
}  // namespace petace
