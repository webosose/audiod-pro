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
#include <chrono>
#include <thread>
#include "utils.h"
#include "log.h"
#include "audioMixer.h"
#include "moduleFactory.h"
#include "deviceManagerInterface.h"
#include "deviceConfigReader.h"

typedef std::map<int, std::string> connectedDeviceInfo;
struct USB_DETAILS
{
    std::string name;
    std::string type;
    int mmap;
    int tsched;
    int fragmentSize;
    int deviceID;
    std::list<std::string> preConditionList;
    bool isOutput;
    std::string deviceType;
    std::string devPath;
    USB_DETAILS( )
    {
        mmap=0;
        tsched = 0;
        fragmentSize= 4096;
        deviceID = 0;
        isOutput = false;
    }
    USB_DETAILS(bool isOutput)
    {
        mmap=0;
        tsched = 0;
        fragmentSize= 4096;
        deviceID = 0;
        this->isOutput = isOutput;
        this->deviceType=isOutput?"playback":"capture";
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
        utils::mapPhysicalInfo internalDevices;
        utils::mapPhysicalInfo mPhyInternalInfo;
        utils::mapPhysicalInfo mPhyExternalInfo;
        int internalSinkCount;
        int internalSourceCount;
        int mDeviceList;
        connectedDeviceInfo mConnectedDevices;
        USB_DETAILS inputDevices = USB_DETAILS(false);
        USB_DETAILS outputDevices = USB_DETAILS(true);
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
        void eventKeyInfo (LUNA_KEY_TYPE_E type, LSMessage *message);
        void eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected);
        void handleEvent(events::EVENTS_T* ev);
        static bool _event(LSHandle *lshandle, LSMessage *message, void *ctx);
        bool addInternalCard(int cardNumber, std::string cardId, std::string cardName,const std::string deviceType, const std::string devPath);
        bool addExternalCard(int cardNumber, std::string cardId, std::string cardName,const std::string deviceType, const std::string devPath);
        bool removeExternalCard(int cardNumber, std::string cardId, std::string deviceType);
        bool loadInternalCard (utils::CARD_INFO_T& cardInfo);
        bool loadUnloadExternalCard (utils::CARD_INFO_T& cardInfo, bool isLoad);
        void addEventToQueue(bool isAdd, const Device& device);
        void printIntCardInfo();
        void printExtCardInfo();
        bool setDeviceJsonDetails();
        bool setInternalCardDetails(const pbnjson::JValue& internalListInfo);
        bool setExternalCardDetails(const pbnjson::JValue& externalListInfo);
        void getAttachedNonStorageDeviceList(LSMessage *message);
        std::string getdevPath(int cardId, bool isExternal);
        std::string getCardId(int cardId, bool isExternal);
        std::string getCardName(int cardId, bool isExternal);
        bool supportPlaybackCapture(int cardNumber,std::string deviceType);
};
#endif // _DEVICE_MANAGER_H_