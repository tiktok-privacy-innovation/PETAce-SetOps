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

project(SetopsPython VERSION 0.2.0 LANGUAGES CXX)

# If not called from root CMakeLists.txt
if(NOT DEFINED SETOPS_BUILD_PYTHON)
    set(SETOPS_BUILD_PYTHON ON)

    find_package(PETAce-SetOps 0.2.0 EXACT REQUIRED)

    add_compile_options(-msse4.2 -Wno-ignored-attributes -mavx)

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

# if SETOPS_BUILD_PYTHON is ON, find pybind11
if(SETOPS_BUILD_PYTHON)
    find_package(pybind11 QUIET CONFIG)
    if(pybind11_FOUND)
        message(STATUS "pybind11: found")
    else()
        if(SETOPS_BUILD_DEPS)
            message(STATUS "pybind11: download ...")
            setops_fetch_thirdparty_content(ExternalPybind11)
        else()
            message(FATAL_ERROR "pybind11: not found, please download and install manually")
        endif()
    endif()

    pybind11_add_module(pysetops setops_py_warpper.cpp setops_py_psi.cpp)

    if(TARGET PETAce-SetOps::setops)
        target_link_libraries(pysetops PRIVATE PETAce-SetOps::setops)
    elseif(TARGET PETAce-SetOps::setops_shared)
        target_link_libraries(pysetops PRIVATE PETAce-SetOps::setops_shared)
    else()
        message(FATAL_ERROR "Cannot find target PETAce-SetOps::setops or PETAce-SetOps::setops_shared")
    endif()

    find_package(Python3 COMPONENTS Interpreter)
    if(NOT Python3_Interpreter_FOUND )
        message(FATAL_ERROR "Python3 not found")
    else()
        message(STATUS "Python3: found")
    endif()

    set(WHEEL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/wheel)
    file(MAKE_DIRECTORY ${WHEEL_OUTPUT_DIR})

    set(PY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(PY_BUILD_DIR ${CMAKE_BINARY_DIR}/python/buildwheel)
    file(MAKE_DIRECTORY ${PY_BUILD_DIR})
    message(STATUS "${PY_BUILD_DIR}")

    add_custom_target(copy_python
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PY_SRC_DIR}  ${PY_BUILD_DIR}
    )

    file(MAKE_DIRECTORY ${WHEEL_OUTPUT_DIR})
    add_custom_target(wheel
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pysetops> ${PY_BUILD_DIR}/petace/setops
        COMMAND ${Python3_EXECUTABLE} setup.py bdist_wheel --dist-dir ${WHEEL_OUTPUT_DIR}
        WORKING_DIRECTORY ${PY_BUILD_DIR}
        DEPENDS copy_python
    )
endif()
