# Copyright (c) 2009-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

message(STATUS "BUILDING audio_ipc")

# ----
#  local component configuration
#set(BIN_NAME "audio_utils")
set(LIB_NAME "audio-ipc")


# ---
#
set(CMAKE_C_FLAGS "-Wall -Werror")
set(CMAKE_CXX_FLAGS "-Wall -Werror")


#Build library
set(TARGET_NAME audio-ipc)
set(TARGET_SRCS "ipc.cpp")
add_library(${TARGET_NAME} SHARED ${TARGET_SRCS})
target_link_libraries(${TARGET_NAME})


##---
# install
#  INSTALL_INCLUDE_DIR comes from top-level cmake file.
#

INSTALL(FILES ${HEADER_FILES}
        DESTINATION ${INSTALL_INCLUDE_DIR}
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

INSTALL(TARGETS ${LIB_NAME} DESTINATION ${INSTALL_LIB_DIR})
