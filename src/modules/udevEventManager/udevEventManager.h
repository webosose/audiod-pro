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

#ifndef _UDEV_EVENT_MANAGER_H_
#define _UDEV_EVENT_MANAGER_H_

#include <cstring>
#include "utils.h"
#include "log.h"
#include "audioMixer.h"
#include "moduleFactory.h"

class UdevEventManager : public ModuleInterface
{
    private:
        UdevEventManager(const UdevEventManager&) = delete;
        UdevEventManager& operator=(const UdevEventManager&) = delete;
        UdevEventManager(ModuleConfig* const pConfObj);
        AudioMixer *mObjAudioMixer;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_udev_event_manager", &UdevEventManager::CreateObject));
        }

    public:
        ~UdevEventManager();
        static LSMethod udevMethods[];
        static UdevEventManager* getUdevEventManagerInstance();
        static UdevEventManager* mObjUdevEventManager;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the UdevEventManager handler");
                mObjUdevEventManager = new(std::nothrow) UdevEventManager(pConfObj);
                if (mObjUdevEventManager)
                    return mObjUdevEventManager;
            }
            return nullptr;
        }
        void initialize();
        static bool _event(LSHandle *lshandle, LSMessage *message, void *ctx);
};
#endif
