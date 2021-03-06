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

#include "audioMixer.h"
#include "moduleFactory.h"

class SystemSoundsManager : public ModuleInterface
{
    private:
        SystemSoundsManager(const SystemSoundsManager&) = delete;
        SystemSoundsManager& operator=(const SystemSoundsManager&) = delete;
        SystemSoundsManager(ModuleConfig* const pConfObj);
        AudioMixer *mObjAudioMixer;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_system_sounds_manager", &SystemSoundsManager::CreateObject));
        }

    public:
        ~SystemSoundsManager();
        static LSMethod systemsoundsMethods[];
        static SystemSoundsManager* getSystemSoundsManagerInstance();
        static SystemSoundsManager* mSystemSoundsManager;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the SystemSoundsManager handler");
                mSystemSoundsManager = new(std::nothrow) SystemSoundsManager(pConfObj);
                if (mSystemSoundsManager)
                    return mSystemSoundsManager;
            }
            return nullptr;
        }
        void initialize();
        void deInitialize();
        void handleEvent(events::EVENTS_T* ev);
        static bool _playFeedback(LSHandle *lshandle, LSMessage *message, void *ctx);
};
