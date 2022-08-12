// Copyright (c) 2021-2022 LG Electronics, Inc.
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

#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include <cstring>
#include "utils.h"
#include "log.h"
#include "audioMixer.h"
#include "moduleFactory.h"
#include "deviceManagerInterface.h"
#include "deviceConfigReader.h"

struct cardDetail
{
    std::string name;
    int cardNum;
    int deviceNum;
    std::string cardId;
    cardDetail()
    {
        cardNum = -1;
        deviceNum = -1;
    }
};

class DeviceManager : public ModuleInterface
{
    private:
        DeviceManager(const DeviceManager&) = delete;
        DeviceManager& operator=(const DeviceManager&) = delete;
        DeviceManager(ModuleConfig* const pConfObj);
        AudioMixer *mObjAudioMixer;
        ModuleManager *mObjModuleManager;
        static bool mIsObjRegistered;
        std::vector<Device> mDeviceAddedQueue;
        std::vector<Device> mDeviceRemovedQueue;
        std::list<std::string> mCardNames;
        std::map<std::string, cardDetail> mSinkInfo;
        std::map<std::string, cardDetail> mSourceInfo;
        bool alsaConfRead;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_device_manager", &DeviceManager::CreateObject));
        }

    public:
        ~DeviceManager();
        static LSMethod deviceManagerMethods[];
        static DeviceManager* getDeviceManagerInstance();
        static DeviceManager* mObjDeviceManager;
        static DeviceManagerInterface* mClientDeviceManagerInstance;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the DeviceManager handler");
                mObjDeviceManager = new(std::nothrow) DeviceManager(pConfObj);
                if (mObjDeviceManager)
                    return mObjDeviceManager;
            }
            return nullptr;
        }
        void initialize();
        void deInitialize();
        void eventMixerStatus (bool mixerStatus, utils::EMIXER_TYPE mixerType);
        void handleEvent(events::EVENTS_T* ev);
        static bool _event(LSHandle *lshandle, LSMessage *message, void *ctx);

        void addEventToQueue(bool isAdd, const Device& device);
        bool setDeviceJsonDetails();
        bool readAlsaCardFile();
};
#endif // _DEVICE_MANAGER_H_
