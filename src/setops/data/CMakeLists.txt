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

# Source files in this directory
set(SETOPS_SOURCE_FILES ${SETOPS_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/csv_data_provider.cpp
)

# Add header files for installation
install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/csv_data_provider.h
        ${CMAKE_CURRENT_LIST_DIR}/data_provider.h
    DESTINATION
        ${SETOPS_INCLUDES_INSTALL_DIR}/setops/data
)

set(SETOPS_SOURCE_FILES ${SETOPS_SOURCE_FILES} PARENT_SCOPE)
