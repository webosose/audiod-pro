# Copyright (c) 2012-2024 LG Electronics, Inc.
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
include_directories(src/modules/audioPolicyManager)
include_directories(src/modules/bluetoothManager)
include_directories(src/modules/connectionManager)
include_directories(src/modules/masterVolumeManager)
include_directories(src/modules/settingsServiceManager)
include_directories(src/modules/systemSoundsManager)
include_directories(src/modules/deviceManager)
include_directories(src/modules/lunaEventSubscriber)
include_directories(src/modules/playbackManager)
include_directories(src/modules/trackManager)
include_directories(src/modules/audioEffectManager)
include_directories(include/public/)

#Please check these header files again
include_directories(include)

aux_source_directory(src src_files)
aux_source_directory(utils utils_1_files)
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

SET (modules_files
        src/modules/lunaEventSubscriber/lunaEventSubscriber.cpp
        src/modules/masterVolumeManager/OSEMasterVolumeManager.cpp
        src/modules/deviceManager/udevDeviceManager.cpp
        src/modules/audioPolicyManager/audioPolicyManager.cpp
        src/modules/audioPolicyManager/volumePolicyInfoParser.cpp
        src/modules/bluetoothManager/bluetoothManager.cpp
        src/modules/connectionManager/connectionManager.cpp
        src/modules/masterVolumeManager/masterVolumeManager.cpp
        src/modules/settingsServiceManager/settingsServiceManager.cpp
        src/modules/systemSoundsManager/systemSoundsManager.cpp
        src/modules/deviceManager/deviceManager.cpp
        src/modules/playbackManager/playbackManager.cpp
        src/modules/audioRouter/deviceRoutingConfigParser.cpp
        src/modules/audioRouter/audioRouter.cpp
        src/modules/trackManager/trackManager.cpp
        src/modules/deviceManager/deviceConfigReader.cpp
        src/modules/audioEffectManager/audioEffectManager.cpp
    )

    add_definitions(-DDEVICE_NAME="Unknown")
    add_definitions(-DDEVICE_ENUM=eDevice_unknown)
    add_definitions(-DSYSTEMSOUNDS_PATH="${WEBOS_INSTALL_DATADIR}/systemsounds/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/files/share/sounds/systemsounds" DESTINATION ${WEBOS_INSTALL_DATADIR} FILES_MATCHING PATTERN "*.pcm")
    install(FILES files/config/audiod_sink_volume_policy_config.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod)
    install(FILES files/config/audiod_source_volume_policy_config.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod)
    install(FILES files/config/audiod_module_config.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod)
    install(FILES files/config/audiod_device_routing_config.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod)
    install(FILES files/config/bluetooth_configuration.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod)
IF (${WEBOS_TARGET_MACHINE_IMPL} STREQUAL "emulator")
    install(FILES files/config/audiod_internal_device_loading_qemux86-64.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod RENAME audiod_internal_device_loading.json)
ELSE ()
    install(FILES files/config/audiod_internal_device_loading_open.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/audiod RENAME audiod_internal_device_loading.json)
ENDIF()


if (WEBOS_LTTNG_ENABLED)
add_executable(audiod     ${utils_1_files}
            ${src_files}
            ${modules_files}
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
            ${src_files}
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
IF (${WEBOS_TARGET_MACHINE_IMPL} STREQUAL "emulator")
add_definitions(-DWEBOS_SOC_TYPE="x86")
ELSE()
add_definitions(-DWEBOS_SOC_TYPE="RPI4")
ENDIF()


webos_build_system_bus_files()
webos_build_daemon()

#-- install udev rule for headset detection
install(FILES etc/udev/rules.d/86-audiod.rules DESTINATION ${WEBOS_INSTALL_WEBOS}/etc/udev/rules.d/)
install(FILES etc/udev/scripts/headset.sh DESTINATION ${WEBOS_INSTALL_WEBOS}/etc/udev/scripts/ PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
install(FILES etc/udev/scripts/usbsoundcard.sh DESTINATION ${WEBOS_INSTALL_WEBOS}/etc/udev/scripts/ PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)

