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

#include <vector>

#include "solo/util/defines.h"
#include "verse/util/defines.h"

#include "setops/util/config.h"

namespace petace {
namespace setops {

const std::size_t kEccPointLen = 33;
const std::size_t kECCCompareBytesLen = 12;
const std::size_t kRandSeedBytesLen = 16;
const std::size_t kItemBytesLen = 16;
const std::size_t kReduceStatisticsLen = 12;
const std::int64_t kReduceBitsLen = 0x3fffffffffffffff;
using Byte = petace::solo::Byte;
using block = petace::verse::block;
using ByteVector = std::vector<Byte>;
using Item = std::array<Byte, kItemBytesLen>;
struct HashLocMap {
    int bin;
    int index;
};
}  // namespace setops
}  // namespace petace
