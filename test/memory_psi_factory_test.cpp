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

#include "setops/memory_psi_factory.h"

#include "gtest/gtest.h"

namespace petace {
namespace setops {

class MemoryPSIFactoryTest : public ::testing::Test {};

TEST_F(MemoryPSIFactoryTest, psi) {
    MemoryPSIFactory<MemoryPSIScheme::PSI>::get_instance().build(PSIScheme::ECDH_PSI);
}

TEST_F(MemoryPSIFactoryTest, psi_not_registered) {
    auto not_registered_test = []() {
        MemoryPSIFactory<MemoryPSIScheme::PSI>::get_instance().build(PSIScheme::KKRT_PSI);
    };
    EXPECT_THROW(not_registered_test(), std::invalid_argument);
}

TEST_F(MemoryPSIFactoryTest, pjc_not_registered) {
    auto not_registered_test = []() {
        MemoryPSIFactory<MemoryPSIScheme::PJC>::get_instance().build(PJCScheme::DPCA_PSI);
    };
    EXPECT_THROW(not_registered_test(), std::invalid_argument);
}

}  // namespace setops
}  // namespace petace
