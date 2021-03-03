// Copyright (c) 2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "OSEMasterVolumeManager.h"

bool OSEMasterVolumeManager::mIsObjRegistered = OSEMasterVolumeManager::CreateInstance();

OSEMasterVolumeManager* OSEMasterVolumeManager::getInstance()
{
    static OSEMasterVolumeManager objOSEMasterVolumeManager;
    return &objOSEMasterVolumeManager;
}

OSEMasterVolumeManager::OSEMasterVolumeManager()
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager constructor");
}

OSEMasterVolumeManager::~OSEMasterVolumeManager()
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager destructor");
}

void OSEMasterVolumeManager::setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: setVolume");
}

void OSEMasterVolumeManager::getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: getVolume");
}

void OSEMasterVolumeManager::muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: muteVolume");
}

void OSEMasterVolumeManager::volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: volumeUp");
}

void OSEMasterVolumeManager::volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: volumeDown");
}