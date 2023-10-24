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

#####################################################
# Project SETOPS includes the following components: #
#   1. SETOPS C++ library                           #
#   2. SETOPS C++ test                              #
#   3. SETOPS C++ example                           #
#####################################################

# [OPTION] CMAKE_BUILD_TYPE (DEFAULT: "Release")
# Select from Release, Debug, MiniSizeRel, or RelWithDebInfo.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

project(SETOPS VERSION 0.2.0 LANGUAGES CXX C)

########################
# Global configuration #
########################

# CMake modules
include(CMakeDependentOption)
include(CMakePushCheckState)
include(CheckIncludeFiles)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)

# Custom modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(SetOpsCustomMacros)

# In Debug mode, define SETOPS_DEBUG.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SETOPS_DEBUG ON)
else()
    set(SETOPS_DEBUG OFF)
endif()
message(STATUS "SETOPS debug mode: ${SETOPS_DEBUG}")

# In Debug mode, enable extra compiler flags.
include(EnableDebugFlags)

# Always build position-independent-code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# [OPTION] SETOPS_USE_CXX17 (default: OFF)
# Use C++17, use C++14 otherwise.
set(SETOPS_USE_CXX17_OPTION_STR "Use C++17")
option(SETOPS_USE_CXX17 ${SETOPS_USE_CXX17_OPTION_STR} OFF)
message(STATUS "SETOPS_USE_CXX17: ${SETOPS_USE_CXX17}")
# Enable features from C++17 if available, disable features if set to OFF.
include(EnableCXX17)

# Add default files and directories.
include(GNUInstallDirs)

# Runtime path
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Source Tree
set(SETOPS_INCLUDES_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(SETOPS_CONFIG_IN_FILENAME ${CMAKE_CURRENT_LIST_DIR}/cmake/PETAce-SetOpsConfig.cmake.in)
set(SETOPS_CONFIG_H_IN_FILENAME ${SETOPS_INCLUDES_DIR}/setops/util/config.h.in)

# Build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(SETOPS_CONFIG_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/PETAce-SetOpsConfig.cmake)
set(SETOPS_TARGETS_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/PETAce-SetOpsTargets.cmake)
set(SETOPS_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/PETAce-SetOpsConfigVersion.cmake)
set(SETOPS_CONFIG_H_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/src/setops/util/config.h)
set(SETOPS_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)

# Installation tree
set(SETOPS_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/PETAce-SetOps-${SETOPS_VERSION_MAJOR}.${SETOPS_VERSION_MINOR})
set(SETOPS_INCLUDES_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/PETAce-${SETOPS_VERSION_MAJOR}.${SETOPS_VERSION_MINOR})
set(SETOPS_THIRDPARTY_INCLUDES_INSTALL_DIR ${SETOPS_INCLUDES_INSTALL_DIR}/thirdparty)

# Make the install target depend on the all target.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY OFF)

# Supported target operating systems are Linux and macOS.
if (NOT DEFINED LINUX)
    if (UNIX AND NOT APPLE AND NOT CYGWIN AND NOT MINGW)
        set(LINUX ON)
    endif()
endif()
if (UNIX AND APPLE)
    set(MACOS ON)
endif()
if (NOT LINUX AND NOT MACOS)
    message(FATAL_ERROR "Supported target operating systems are Linux and macOS")
endif()

# Only support x86_64 and arm64
set(CMAKE_REQUIRED_QUIET_OLD ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ON)
check_cxx_source_runs("
    #if defined(__aarch64__)
        int main() {
            return 0;
        }
    #else
        #error
    #endif
    "
    SETOPS_ARM64
)
check_cxx_source_runs("
    #if defined(__amd64)
        int main() {
            return 0;
        }
    #else
        #error
    #endif
    "
    SETOPS_AMD64
)
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})
if (NOT SETOPS_AMD64 AND NOT SETOPS_ARM64)
    message(FATAL_ERROR "Supported target architectures are x86_64 and arm64")
endif()

add_compile_options(-msse4.2 -Wno-ignored-attributes -mavx)

# Enable test coverage
set(SETOPS_ENABLE_GCOV_STR "Enable gcov")
option(SETOPS_ENABLE_GCOV ${SETOPS_ENABLE_GCOV_STR} OFF)
message(STATUS "SETOPS_ENABLE_GCOV: ${SETOPS_ENABLE_GCOV_STR}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND SETOPS_ENABLE_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -fprofile-arcs -ftest-coverage -lgcov")
endif()

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")

#########################
# External dependencies #
#########################

# [OPTION] SETOPS_BUILD_DEPS (DEFAULT: ON)
# Download and build dependencies if set to ON.
# Look for dependencies using find_package, otherwise.
set(SETOPS_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
option(SETOPS_BUILD_DEPS ${SETOPS_BUILD_DEPS_OPTION_STR} ON)
message(STATUS "SETOPS_BUILD_DEPS: ${SETOPS_BUILD_DEPS}")

if(SETOPS_BUILD_DEPS)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
endif()

# [OPTION] SETOPS_BUILD_SHARED_LIBS (DEFAULT: OFF)
# Build a shared library if set to ON.
set(SETOPS_BUILD_SHARED_LIBS_STR "Build shared library")
option(SETOPS_BUILD_SHARED_LIBS ${SETOPS_BUILD_SHARED_LIBS_STR} OFF)
message(STATUS "SETOPS_BUILD_SHARED_LIBS: ${SETOPS_BUILD_SHARED_LIBS}")

# Require Threads::Threads
if(NOT TARGET Threads::Threads)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    if(NOT Threads_FOUND)
        message(FATAL_ERROR "Threads: not found, please download and install manually")
    else()
        message(STATUS "Threads: found")
    endif()
endif()

# OpenMP::OpenMP_CXX
if(NOT TARGET OpenMP::OpenMP_CXX)
    find_package(OpenMP)
    if(NOT OpenMP_FOUND)
        message(FATAL_ERROR "OpenMP: not found, please download and install manually")
    else()
        message(STATUS "OpenMP: found")
    endif()
endif()

# PETAce-Network::network
find_package(PETAce-Network 0.2 QUIET CONFIG)
if(PETAce-Network_FOUND)
    message(STATUS "PETAce-Network: found")
    if(PETAce-Network_STATIC_FOUND)
        set(network "PETAce-Network::network")
    else()
        set(network "PETAce-Network::network_shared")
    endif()
else()
    if(SETOPS_BUILD_DEPS)
        message(STATUS "PETAce-Network: download ...")
        setops_fetch_thirdparty_content(PETAce-Network)
        if(TARGET network)
            set(network "network")
        else()
            set(network "network_shared")
        endif()
        set(SETOPS_BUILD_NETWORK TRUE CACHE BOOL "" FORCE)
    else()
        message(FATAL_ERROR "PETAce-Network: not found, please download and install manually")
    endif()
endif()

# PETAce-Solo::solo
find_package(PETAce-Solo 0.2 QUIET CONFIG)
if(PETAce-Solo_FOUND)
    message(STATUS "PETAce-Solo: found")
    if(PETAce-Solo_STATIC_FOUND)
        set(solo "PETAce-Solo::solo")
    else()
        set(solo "PETAce-Solo::solo_shared")
    endif()
else()
    if(SETOPS_BUILD_DEPS)
        message(STATUS "PETAce-Solo: download ...")
        setops_fetch_thirdparty_content(PETAce-Solo)
        if(TARGET solo)
            set(solo "solo")
        else()
            set(solo "solo_shared")
        endif()
        set(SETOPS_BUILD_SOLO TRUE CACHE BOOL "" FORCE)
    else()
        message(FATAL_ERROR "PETAce-Solo: not found, please download and install manually")
    endif()
endif()

# PETAce-Verse::verse
find_package(PETAce-Verse 0.2 QUIET CONFIG)
if(PETAce-Verse_FOUND)
    message(STATUS "PETAce-Verse: found")
    if(PETAce-Verse_STATIC_FOUND)
        set(verse "PETAce-Verse::verse")
    else()
        set(verse "PETAce-Verse::verse_shared")
    endif()
else()
    if(SETOPS_BUILD_DEPS)
        message(STATUS "PETAce-Verse: download ...")
        setops_fetch_thirdparty_content(PETAce-Verse)
        if(TARGET verse)
            set(verse "verse")
        else()
            set(verse "verse_shared")
        endif()
        set(SETOPS_BUILD_VERSE TRUE CACHE BOOL "" FORCE)
    else()
        message(FATAL_ERROR "PETAce-Verse: not found, please download and install manually")
    endif()
endif()

# PETAce-Duet::duet
find_package(PETAce-Duet 0.2 QUIET CONFIG)
if(PETAce-Duet_FOUND)
    message(STATUS "PETAce-Duet: found")
    if(PETAce-Duet_STATIC_FOUND)
        set(duet "PETAce-Duet::duet")
    else()
        set(duet "PETAce-Duet::duet_shared")
    endif()
else()
    if(SETOPS_BUILD_DEPS)
        message(STATUS "PETAce-Duet: download ...")
        setops_fetch_thirdparty_content(PETAce-Duet)
        if(TARGET duet)
            set(duet "duet")
        else()
            set(duet "duet_shared")
        endif()
        set(SETOPS_BUILD_DUET TRUE CACHE BOOL "" FORCE)
    else()
        message(FATAL_ERROR "PETAce-Duet: not found, please download and install manually")
    endif()
endif()

# glog::glog
find_package(glog 0.4.0 QUIET CONFIG)
if(glog_FOUND)
    message(STATUS "glog: found")
else()
    if(SETOPS_BUILD_DEPS)
        message(STATUS "glog: download ...")
        setops_fetch_thirdparty_content(ExternalGlog)
        set(SETOPS_BUILD_GLOG TRUE CACHE BOOL "" FORCE)
    else()
        message(FATAL_ERROR "glog: not found, please download and install manually")
    endif()
endif()

# nlohmann_json::nlohmann_json
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json 3 QUIET REQUIRED)
    if(nlohmann_json_FOUND)
        message(STATUS "nlohmann_json: found")
    else()
        message(STATUS "nlohmann_json: not found, please download and install manually")
    endif()
endif()

######################
# SETOPS C++ library #
######################

# Add source files to library and header files to install
set(SETOPS_SOURCE_FILES "")
add_subdirectory(src/setops)

# Create the config file
configure_file(${SETOPS_CONFIG_H_IN_FILENAME} ${SETOPS_CONFIG_H_FILENAME})
install(
    FILES ${SETOPS_CONFIG_H_FILENAME}
    DESTINATION ${SETOPS_INCLUDES_INSTALL_DIR}/setops/util)

# Build only a static library
if(NOT SETOPS_BUILD_SHARED_LIBS)
    add_library(setops STATIC ${SETOPS_SOURCE_FILES})
    if(SETOPS_USE_CXX17)
        target_compile_features(setops PUBLIC cxx_std_17)
    else()
        target_compile_features(setops PUBLIC cxx_std_14)
    endif()
    target_include_directories(setops PUBLIC
        $<BUILD_INTERFACE:${SETOPS_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    target_include_directories(setops PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(setops PROPERTIES VERSION ${SETOPS_VERSION})
    set_target_properties(setops PROPERTIES OUTPUT_NAME petace_setops-${SETOPS_VERSION_MAJOR}.${SETOPS_VERSION_MINOR})

    if(SETOPS_BUILD_GLOG)
        add_dependencies(setops glog::glog)
        target_include_directories(setops PRIVATE
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:glog::glog,INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        setops_combine_archives(setops glog::glog)
        set(SETOPS_CARRY_GLOG TRUE)
    else()
        target_link_libraries(setops PRIVATE glog::glog)
        set(SETOPS_CARRY_GLOG FALSE)
    endif()

    if(SETOPS_BUILD_NETWORK)
        add_dependencies(setops ${network})
        target_include_directories(setops PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${network},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
        setops_combine_archives(setops ${network})
        set(SETOPS_CARRY_NETWORK TRUE)
    else()
        target_link_libraries(setops PUBLIC ${network})
        set(SETOPS_CARRY_NETWORK FALSE)
    endif()

    if(SETOPS_BUILD_SOLO)
        add_dependencies(setops ${solo})
        target_include_directories(setops PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${solo},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops PUBLIC ${solo})
    set(SETOPS_CARRY_SOLO FALSE)

    if(SETOPS_BUILD_VERSE)
        add_dependencies(setops ${verse})
        target_include_directories(setops PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${verse},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops PUBLIC ${verse})
    set(SETOPS_CARRY_VERSE FALSE)

    if(SETOPS_BUILD_DUET)
        add_dependencies(setops ${duet})
        target_include_directories(setops PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${duet},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops PUBLIC ${duet} m)
    set(SETOPS_CARRY_DUET FALSE)

    target_link_libraries(setops PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(setops PUBLIC Threads::Threads OpenMP::OpenMP_CXX)

    install(TARGETS setops EXPORT PETAce-SetOpsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Build only a shared library
else()
    add_library(setops_shared SHARED ${SETOPS_SOURCE_FILES})
    if(SETOPS_USE_CXX17)
        target_compile_features(setops_shared PUBLIC cxx_std_17)
    else()
        target_compile_features(setops_shared PUBLIC cxx_std_14)
    endif()
    target_include_directories(setops_shared PUBLIC
        $<BUILD_INTERFACE:${SETOPS_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    target_include_directories(setops_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(setops_shared PROPERTIES VERSION ${SETOPS_VERSION})
    set_target_properties(setops_shared PROPERTIES OUTPUT_NAME petace_setops)
    set_target_properties(setops_shared PROPERTIES SOVERSION ${SETOPS_VERSION_MAJOR}.${SETOPS_VERSION_MINOR})

    if(SETOPS_BUILD_GLOG)
        target_include_directories(setops_shared PRIVATE
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:glog::glog,INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        set(SETOPS_CARRY_GLOG TRUE)
    else()
        set(SETOPS_CARRY_GLOG FALSE)
    endif()
    target_link_libraries(setops_shared PRIVATE glog::glog)

    if(SETOPS_BUILD_NETWORK)
        target_include_directories(setops_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${network},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops_shared PUBLIC ${network})
    set(SETOPS_CARRY_NETWORK FALSE)

    if(SETOPS_BUILD_SOLO)
        target_include_directories(setops_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${solo},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops_shared PUBLIC ${solo})
    set(SETOPS_CARRY_SOLO FALSE)

    if(SETOPS_BUILD_VERSE)
        add_dependencies(setops_shared ${verse})
        target_include_directories(setops_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${verse},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops_shared PUBLIC ${verse})
    set(SETOPS_CARRY_VERSE FALSE)

    if(SETOPS_BUILD_DUET)
        add_dependencies(setops_shared ${duet})
        target_include_directories(setops_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${duet},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${SETOPS_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(setops_shared PUBLIC ${duet} m)
    set(SETOPS_CARRY_DUET FALSE)

    target_link_libraries(setops_shared PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(setops_shared PUBLIC Threads::Threads OpenMP::OpenMP_CXX)

    install(TARGETS setops_shared EXPORT PETAce-SetOpsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Add standard alias targets for PETAce-SetOps::setops and PETAce-SetOps::setops_shared
if(TARGET setops)
    add_library(PETAce-SetOps::setops ALIAS setops)
endif()
if(TARGET setops_shared)
    add_library(PETAce-SetOps::setops_shared ALIAS setops_shared)
endif()

#################################
# Installation and CMake config #
#################################

# Create the CMake config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${SETOPS_CONFIG_IN_FILENAME} ${SETOPS_CONFIG_FILENAME}
    INSTALL_DESTINATION ${SETOPS_CONFIG_INSTALL_DIR})

# Install the export
install(
    EXPORT PETAce-SetOpsTargets
    NAMESPACE PETAce-SetOps::
    DESTINATION ${SETOPS_CONFIG_INSTALL_DIR})

# Version file; we require exact version match for downstream
write_basic_package_version_file(
    ${SETOPS_CONFIG_VERSION_FILENAME}
    VERSION ${SETOPS_VERSION}
    COMPATIBILITY SameMinorVersion)

# Install config and module files
install(
    FILES
        ${SETOPS_CONFIG_FILENAME}
        ${SETOPS_CONFIG_VERSION_FILENAME}
    DESTINATION ${SETOPS_CONFIG_INSTALL_DIR})

# We export PETAce-SetOpsTargets from the build tree so it can be used by other projects
# without requiring an install.
export(
    EXPORT PETAce-SetOpsTargets
    NAMESPACE PETAce-SetOps::
    FILE ${SETOPS_TARGETS_FILENAME})

# Install header files of dependencies if SETOPS_BUILD_DEPS is ON
if(SETOPS_BUILD_DEPS)
    # Insert dependencies here
    if(SETOPS_BUILD_GLOG)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${glog_BINARY_DIR} -t install)")
    endif()
    if(SETOPS_BUILD_SOLO)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${solo_BINARY_DIR} -t install)")
    endif()
    if(SETOPS_BUILD_VERSE)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${verse_BINARY_DIR} -t install)")
    endif()
    if(SETOPS_BUILD_DUET)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${duet_BINARY_DIR} -t install)")
    endif()
    if(SETOPS_BUILD_NETWORK)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${network_BINARY_DIR} -t install)")
    endif()
endif()

######################
# SETOPS C++ example #
######################

# [option] SETOPS_BUILD_EXAMPLE
set(SETOPS_BUILD_EXAMPLE_OPTION_STR "Build C++ example for SETOPS")
option(SETOPS_BUILD_EXAMPLE ${SETOPS_BUILD_EXAMPLE_OPTION_STR} ON)
message(STATUS "SETOPS_BUILD_EXAMPLE: ${SETOPS_BUILD_EXAMPLE}")

if(SETOPS_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()

###################
# SETOPS C++ test #
###################

# [option] SETOPS_BUILD_TEST
set(SETOPS_BUILD_TEST_OPTION_STR "Build C++ test for SETOPS")
option(SETOPS_BUILD_TEST ${SETOPS_BUILD_TEST_OPTION_STR} ON)
message(STATUS "SETOPS_BUILD_TEST: ${SETOPS_BUILD_TEST}")

if(SETOPS_BUILD_TEST)
    add_subdirectory(test)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND SETOPS_ENABLE_GCOV)
        add_custom_target(test_coverage
            COMMAND gcovr -r ${CMAKE_CURRENT_LIST_DIR} -f \"src\" -e \".+\(test\\.cpp\)\" --xml-pretty -o "${CMAKE_CURRENT_BINARY_DIR}/report/coverage.xml"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    endif()
endif()

##################
# SETOPS python #
##################

# [OPTION] SETOPS_BUILD_PYTHON (DEFAULT: OFF)
# Build a shared library if set to ON.
set(SETOPS_BUILD_PYTHON_STR "Build python shared library")
option(SETOPS_BUILD_PYTHON ${SETOPS_BUILD_PYTHON_STR} OFF)
message(STATUS "SETOPS_BUILD_PYTHON: ${SETOPS_BUILD_PYTHON}")

if(SETOPS_BUILD_PYTHON)
    add_subdirectory(python)
endif()
