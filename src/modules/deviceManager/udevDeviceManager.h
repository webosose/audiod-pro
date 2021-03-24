// Copyright (c) 2020-2021 LG Electronics, Inc.
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

#ifndef _UDEV_DEVICE_MANAGER_H_
#define _UDEV_DEVICE_MANAGER_H_

#include <cstring>
#include "utils.h"
#include "log.h"
#include "audioMixer.h"
#include "deviceManagerInterface.h"
#include "deviceManager.h"

#define MSGID_UDEV_MANAGER                           "UDEV_EVENT_MANAGER"              //For UDEV Device event manager

class UdevDeviceManager : public DeviceManagerInterface
{
    private:
        UdevDeviceManager(const UdevDeviceManager&) = delete;
        UdevDeviceManager& operator=(const UdevDeviceManager&) = delete;
        AudioMixer *mObjAudioMixer;
        static bool mIsObjRegistered;
        static bool CreateInstance()
        {
            PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT, "UdevDeviceManager: CreateInstance");
            DeviceManager::setInstance(getInstance());
            return true;
        }

    public:
         UdevDeviceManager();
        ~UdevDeviceManager();
        static UdevDeviceManager* getInstance();
        bool event(LSHandle *lshandle, LSMessage *message, void *ctx);
};
#endif //_UDEV_DEVICE_MANAGER_H_