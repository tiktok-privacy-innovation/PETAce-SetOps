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

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "setops/pjc/pjc.h"
#include "setops/psi/psi.h"

namespace petace {
namespace setops {

enum class MemoryPSIScheme : std::uint32_t { PSI = 0, PJC = 1, PIR = 2 };

using PSICreator = std::function<std::unique_ptr<PSI>()>;
using PJCCreator = std::function<std::unique_ptr<PJC>()>;

template <MemoryPSIScheme scheme>
class MemoryPSIFactory;

/**
 * @brief Provides memory PSI objects.
 */
template <>
class MemoryPSIFactory<MemoryPSIScheme::PSI> {
public:
    /**
     * @brief Gets the memory PSI factory singleton.
     */
    static MemoryPSIFactory& get_instance() {
        static MemoryPSIFactory factory;
        return factory;
    }

    /**
     * @brief Builds a shared pointer of a PSI object.
     *
     * @param[in] scheme A PSI scheme.
     * @return Returns a shared pointer to the constructed object.
     */
    std::unique_ptr<PSI> build(const PSIScheme& scheme) {
        auto where = creator_map_.find(scheme);
        if (where == creator_map_.end()) {
            throw std::invalid_argument("PSI creator not registered.");
        }
        return where->second();
    }

protected:
    MemoryPSIFactory() {
        register_psi(PSIScheme::ECDH_PSI, CreatePSI<PSIScheme::ECDH_PSI>);
    }
    ~MemoryPSIFactory() {
    }
    MemoryPSIFactory(const MemoryPSIFactory&) = delete;
    MemoryPSIFactory& operator=(const MemoryPSIFactory&) = delete;
    MemoryPSIFactory(MemoryPSIFactory&&) = delete;
    MemoryPSIFactory& operator=(MemoryPSIFactory&&) = delete;

private:
    void register_psi(const PSIScheme& scheme, PSICreator creator) {
        creator_map_.insert(std::make_pair(scheme, creator));
    }
    std::map<PSIScheme, PSICreator> creator_map_;
};

/**
 * @brief Provides memory PJC objects.
 */
template <>
class MemoryPSIFactory<MemoryPSIScheme::PJC> {
public:
    /**
     * @brief Gets the memory PJC factory singleton.
     */
    static MemoryPSIFactory& get_instance() {
        static MemoryPSIFactory factory;
        return factory;
    }

    /**
     * @brief Builds a shared pointer of a PJC object.
     *
     * @param[in] scheme A PJC scheme.
     * @return Returns a shared pointer to the constructed object.
     */
    std::unique_ptr<PJC> build(const PJCScheme& scheme) {
        auto where = creator_map_.find(scheme);
        if (where == creator_map_.end()) {
            throw std::invalid_argument("PSI creator not registered.");
        }
        return where->second();
    }

protected:
    MemoryPSIFactory() {
    }
    ~MemoryPSIFactory() {
    }
    MemoryPSIFactory(const MemoryPSIFactory&) = delete;
    MemoryPSIFactory& operator=(const MemoryPSIFactory&) = delete;
    MemoryPSIFactory(MemoryPSIFactory&&) = delete;
    MemoryPSIFactory& operator=(MemoryPSIFactory&&) = delete;

private:
    void register_pjc(const PJCScheme& scheme, PJCCreator creator) {
        creator_map_.insert(std::make_pair(scheme, creator));
    }
    std::map<PJCScheme, PJCCreator> creator_map_;
};

}  // namespace setops
}  // namespace petace
