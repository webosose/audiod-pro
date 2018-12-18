# Copyright (c) 2012-2018 LG Electronics, Inc.
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

project(audiod CXX C)
cmake_minimum_required(VERSION 2.8.7)

set(CMAKE_MODULE_PATH ${audiod_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC4)
webos_component(1 0 0)

webos_add_compiler_flags(ALL "-DAUDIOD_IPC_SERVER -DBOOST_DISABLE_THREADS -D_GNU_SOURCE")
webos_add_compiler_flags(ALL "-Wall")

webos_machine_dep()

# These variables can change the default behaviour and need to be set before calling find_package
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS regex system filesystem)
include_directories(Boost_INCLUDE_DIRS)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(LIBPBNJSON REQUIRED pbnjson_cpp)
include_directories(${LIBPBNJSON_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LIBPBNJSON_CFLAGS})

pkg_check_modules(LUNASERVICE REQUIRED luna-service2)
include_directories(${LUNASERVICE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE_CFLAGS})

pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
include_directories(${PMLOGLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOGLIB_CFLAGS_OTHER})

pkg_check_modules(LUNASERVICE2 REQUIRED luna-service2)
include_directories(${LUNASERVICE2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE2_CFLAGS_OTHER})

pkg_check_modules(LUNAPREFS REQUIRED luna-prefs)
include_directories(${LUNAPREFS_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNAPREFS_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_C REQUIRED pbnjson_c)
include_directories(${PBNJSON_C_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_C_CFLAGS_OTHER})

pkg_check_modules(PULSE REQUIRED libpulse)
include_directories(${PULSE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PULSE_CFLAGS_OTHER})

pkg_check_modules(PULSE_SIMPLE REQUIRED libpulse-simple)
include_directories(${PULSE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PULSE_CFLAGS_OTHER})

pkg_check_modules(LTTNG_UST lttng-ust)
include_directories(${LTTNG_UST_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LTTNG_UST_CFLAGS_OTHER})

pkg_check_modules(URCU_BP liburcu-bp)
include_directories(${URCU_BP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${URCU_BP_CFLAGS_OTHER})

include_directories()

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost REQUIRED COMPONENTS regex)
include_directories(${Boost_INCLUDE_DIRS})

# global installation directories for all components

include_directories(src)
include_directories(src/umi)
include_directories(src/umi/modules)
include_directories(src/umi/soundSettings)
include_directories(src/controls)
include_directories(src/controls/hardware)
include_directories(src/controls/pulse)
include_directories(src/modules)
include_directories(include/public/)

#Please check these header files again
include_directories(include)

aux_source_directory(src src_files)
aux_source_directory(src/utils utils_files)
aux_source_directory(src/controls controls_files)
aux_source_directory(src/controls/pulse controls_pulse_files)
aux_source_directory(utils utils_1_files)
aux_source_directory(ipc ipc_files)
aux_source_directory(pmtrace pmtrace_files)


#add_definitions(-DHAVE_BT_SERVICE_V1)
#add_definitions(-DAUDIOD_PALM_LEGACY)
#add_definitions(-DAUDIOD_TEST_API)

if (AUDIOD_PALM_LEGACY)
    add_definitions(-DAUDIOD_PALM_LEGACY)
endif(AUDIOD_PALM_LEGACY)

if (AUDIOD_TEST_API)
    add_definitions(-DAUDIOD_TEST_API)
endif(AUDIOD_TEST_API)

SET (services_files
        src/services/udev.cpp
        src/services/settingsservice.cpp
        src/services/systemmanager.cpp
        src/services/bluetooth.cpp

)

SET (modules_files
        src/modules/alarm.cpp
        src/modules/alert.cpp
        src/modules/media.cpp
        src/modules/nav.cpp
        src/modules/notification.cpp
        src/modules/phone.cpp
        src/modules/ringtone.cpp
        src/modules/system.cpp
        src/modules/timer.cpp
        src/modules/vvm.cpp
        src/umi/soundSettings/soundSettings.cpp
        src/modules/voicecommand.cpp
        src/umi/umiDispatcher.cpp
        src/controls/umiaudiomixer.cpp
        src/MixerInit.cpp
        src/umi/soundSettings/volumeSettings.cpp
        src/umi/modules/mock.cpp
    )

    LIST (APPEND controls_hardware_files src/controls/hardware/unknowndevice.cpp)
    add_definitions(-DSYSTEMSOUNDS_PATH="/media/internal/systemsounds/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/files/share/sounds/systemsounds" DESTINATION ${WEBOS_INSTALL_MEDIADIR}/internal FILES_MATCHING PATTERN "*.pcm")

if (WEBOS_LTTNG_ENABLED)
add_executable(audiod     ${utils_1_files}
            ${utils_files}
            ${src_files}
            ${controls_files}
            ${controls_pulse_files}
            ${services_files}
            ${modules_files}
            ${controls_hardware_files}
            ${pmtrace_files})

target_link_libraries(audiod     ${GLIB2_LDFLAGS}
                ${LUNASERVICE_LDFLAGS}
                ${PBNJSON_C_LDFLAGS}
                ${LUNAPREFS_LDFLAGS}
                ${PMLOGLIB_LDFLAGS}
                ${LIBPBNJSON_LDFLAGS}
                ${PULSE_LDFLAGS}
                ${LTTNG_UST_LDFLAGS}
                ${URCU_BP_LDFLAGS}
                ${PULSE_SIMPLE_LDFLAGS}
                pthread
                rt
                dl
                )

add_definitions(-DHAS_LTTNG)

else ()
add_executable(audiod     ${utils_1_files}
            ${utils_files}
            ${src_files}
            ${controls_files}
            ${controls_pulse_files}
            ${controls_hardware_files}
            ${services_files}
            ${modules_files} )

target_link_libraries(audiod     ${GLIB2_LDFLAGS}
                ${LUNASERVICE_LDFLAGS}
                ${PBNJSON_C_LDFLAGS}
                ${LUNAPREFS_LDFLAGS}
                ${PMLOGLIB_LDFLAGS}
                ${LIBPBNJSON_LDFLAGS}
                ${PULSE_LDFLAGS}
                ${PULSE_SIMPLE_LDFLAGS}
                pthread
                rt
                dl
                )
endif (WEBOS_LTTNG_ENABLED)
add_definitions(-DENABLE_POWEROFF_REBOOT_SIGNAL)
add_definitions(-DENABLE_WAKELOCK_FOR_SLEEP_STATE)

webos_build_system_bus_files()
webos_build_daemon()

install(FILES include/public/mixerconfig.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod)

#-- install udev rule for headset detection
install(FILES etc/udev/rules.d/86-audiod.rules DESTINATION ${WEBOS_INSTALL_WEBOS}/etc/udev/rules.d/)
install(FILES etc/udev/scripts/headset.sh DESTINATION ${WEBOS_INSTALL_WEBOS}/etc/udev/scripts/ PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
install(FILES etc/udev/scripts/usbsoundcard.sh DESTINATION ${WEBOS_INSTALL_WEBOS}/etc/udev/scripts/ PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)

