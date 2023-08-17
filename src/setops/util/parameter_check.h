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
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "network/network.h"

namespace petace {
namespace setops {

/**
 * @brief Checks the input parmmeter's value less than a given threshold.
 *
 * @param[in] param_name The parmmeter's name.
 * @param[in] value The parmmeter's value.
 * @param[in] threshold A given threshold.
 * @throws std::invalid_argument if value >= threshold.
 */
template <typename T>
void check_less_than(const std::string& param_name, T value, T threshold) {
    if (value >= threshold) {
        auto error_message = "Check less than failed." + param_name + "(" + std::to_string(value) + ")" +
                             " is greater than threshold(" + std::to_string(threshold) + ").";
        throw std::invalid_argument(error_message);
    }
}

/**
 * @brief Checks the input parmmeter's value greater than a given threshold.
 *
 * @param[in] param_name The parmmeter's name.
 * @param[in] value The parmmeter's value.
 * @param[in] threshold A given threshold.
 * @throws std::invalid_argument if value <= threshold.
 */
template <typename T>
void check_greater_than(const std::string& param_name, T value, T threshold) {
    if (value <= threshold) {
        auto error_message = "Check greater than failed." + param_name + "(" + std::to_string(value) + ")" +
                             " is less than threshold(" + std::to_string(threshold) + ").";
        throw std::invalid_argument(error_message);
    }
}

/**
 * @brief Checks the input parmmeter's value equals to a given value.
 *
 * @param[in] param_name The parmmeter's name.
 * @param[in] value The parmmeter's value.
 * @param[in] expected_value The expected value.
 * @throws std::invalid_argument if value != threshold.
 */
template <typename T>
void check_equal(const std::string& param_name, T value, T expected_value) {
    if (value != expected_value) {
        auto error_message = "Check equal failed." + param_name + "(" + std::to_string(value) + ")" +
                             " is not equal to expected value (" + std::to_string(expected_value) + ").";
        throw std::invalid_argument(error_message);
    }
}

/**
 * @brief Checks the input parmmeter's value in a list of given values.
 *
 * @param[in] param_name The parmmeter's name.
 * @param[in] value The parmmeter's value.
 * @param[in] expected_values A vector contains the expected values.
 * @throws std::invalid_argument if value is not equal to any element of the expected values.
 */
template <typename T>
void check_equal(const std::string& param_name, T value, const std::vector<T>& expected_values) {
    std::set<T> values_set(expected_values.begin(), expected_values.end());
    if (values_set.find(value) == values_set.end()) {
        std::string expected_values_str;
        for (std::size_t i = 0; i < expected_values.size(); ++i) {
            if (i != expected_values.size() - 1) {
                expected_values_str += std::to_string(expected_values[i]) + ", ";
            } else {
                expected_values_str += std::to_string(expected_values[i]);
            }
        }
        auto error_message = "Check equal failed." + param_name + "(" + std::to_string(value) + ")" +
                             " is not equal to expected values (" + expected_values_str + ").";
        throw std::invalid_argument(error_message);
    }
}

/**
 * @brief Checks the input parmmeter's value in a given range.
 *
 * @param[in] param_name The parmmeter's name.
 * @param[in] value The parmmeter's value.
 * @param[in] low The lower bound of the given range.
 * @param[in] high The upper bound of the given range.
 * @throws std::invalid_argument if value > high or value < low.
 */
template <typename T>
void check_in_range(const std::string& param_name, T value, T low, T high) {
    if (value > high || value < low) {
        auto error_message = "Check in range failed." + param_name + "(" + std::to_string(value) + ")" +
                             " is not in range [" + std::to_string(low) + "," + std::to_string(high) + "].";
        throw std::invalid_argument(error_message);
    }
}

/**
 * @brief Checks the value of sender's parmmeter is same with the value of receiver' parmmeter.
 *
 * @param[in] is_sender A bool indicates whether sender or receiver.
 * @param[in] net The network interface (e.g., PETAce-Network interface).
 * @param[in] param_name The parmmeter's name.
 * @param[in] value The parmmeter's value.
 * @throws std::invalid_argument if sender's value is not equal to receiver's value.
 */
template <typename T>
void check_consistency(
        bool is_sender, std::shared_ptr<petace::network::Network> net, const std::string& param_name, T value) {
    T remote_value;
    if (is_sender) {
        net->send_data(&value, sizeof(value));
        net->recv_data(&remote_value, sizeof(T));
    } else {
        net->recv_data(&remote_value, sizeof(T));
        net->send_data(&value, sizeof(value));
    }
    if (value != remote_value) {
        auto error_message = "Disagreement on parmeter " + param_name + ", " + std::to_string(value) + " vs " +
                             std::to_string(remote_value) + ".";
        throw std::invalid_argument(error_message);
    }
}

}  // namespace setops
}  // namespace petace
