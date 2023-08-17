# Copyright 2023 TikTok Pte. Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.14)

project(SETOPSExamples VERSION 0.1.0 LANGUAGES CXX)

# If not called from root CMakeLists.txt
if(NOT DEFINED SETOPS_BUILD_EXAMPLE)
    set(SETOPS_BUILD_EXAMPLE ON)

    # Import PETAce SETOPS
    find_package(PETAce-SetOps  0.1.0 EXACT REQUIRED)

    # Must define these variables and include macros
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
    set(SETOPS_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
    include(SetOpsCustomMacros)
else()
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../thirdparty)
endif()

if(NOT DEFINED SETOPS_BUILD_DEPS)
    # [option] SETOPS_BUILD_DEPS (default: ON)
    # Download and build missing dependencies, throw error if disabled.
    set(SETOPS_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
    option(SETOPS_BUILD_DEPS ${SETOPS_BUILD_DEPS_OPTION_STR} ON)
endif()

if(SETOPS_BUILD_EXAMPLE)
    # gflags::gflags
    set (GFLAGS_USE_TARGET_NAMESPACE TRUE)
    find_package(gflags 2.2.2 QUIET CONFIG)
    if(gflags_FOUND)
        message(STATUS "gflags: found")
    else()
        if(SETOPS_BUILD_DEPS)
            message(STATUS "gflags: downloading ...")
            setops_fetch_thirdparty_content(ExternalGflags)
        else()
            message(FATAL_ERROR "gflags: not found, please download and install manually")
        endif()
    endif()

    # glog::glog
    if(NOT TARGET glog::glog)
        find_package(glog 0.4.0 QUIET CONFIG)
        if(glog_FOUND)
            message(STATUS "glog: found")
        else()
            if(SETOPS_BUILD_DEPS)
                message(STATUS "glog: download ...")
                setops_fetch_thirdparty_content(ExternalGlog)
            else()
                message(FATAL_ERROR "glog: not found, please download and install manually")
            endif()
        endif()
    endif()

    # Add source files to example
    set(SETOPS_EXAMPLE_FILES "")
    # Test files in this directory
    set(SETOPS_EXAMPLE_FILES ${SETOPS_EXAMPLE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/ecdh_psi_example.cpp
        ${CMAKE_CURRENT_LIST_DIR}/example.cpp
    )

    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
    add_executable(setops_example ${SETOPS_EXAMPLE_FILES})

    if(TARGET PETAce-SetOps::setops)
        target_link_libraries(setops_example PRIVATE PETAce-SetOps::setops gflags::gflags glog::glog)
    elseif(TARGET PETAce-SetOps::setops_shared)
        target_link_libraries(setops_example PRIVATE PETAce-SetOps::setops_shared gflags::gflags glog::glog)
    else()
        message(FATAL_ERROR "Cannot find target PETAce-SetOps::setops or PETAce-SetOps::setops_shared")
    endif()

    set(SETOPS_EXAMPLE_SCRIPTS_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/scripts)
    set(BIN_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(JSON_DIR ${CMAKE_CURRENT_LIST_DIR}/json)
    set(LOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/log)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/ecdh_psi_receiver_example.sh.in ${SETOPS_EXAMPLE_SCRIPTS_INSTALL_DIR}/ecdh_psi_receiver_example.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/ecdh_psi_sender_example.sh.in ${SETOPS_EXAMPLE_SCRIPTS_INSTALL_DIR}/ecdh_psi_sender_example.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/ecdh_psi_receiver_use_file_data.sh.in ${SETOPS_EXAMPLE_SCRIPTS_INSTALL_DIR}/ecdh_psi_receiver_use_file_data.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/ecdh_psi_sender_use_file_data.sh.in ${SETOPS_EXAMPLE_SCRIPTS_INSTALL_DIR}/ecdh_psi_sender_use_file_data.sh @ONLY)
endif()
