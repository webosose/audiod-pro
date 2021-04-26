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

#define MSGID_UDEV_MANAGER                           "UDEV_EVENT_MANAGER"              //For UDEV Device event manager
#define USB_MIC0           0
#define USB_SPEAKER0       0
#define USB_DEVICE_COUNT   1
#define USB_MIC_ZERO_NAME      "pcm_input"
#define USB_SPEAKER_ZERO_NAME  "display_usb1"

typedef std::vector<std::pair<bool, int>> vectorUdevDeviceInfo;

class UdevDeviceManager : public DeviceManagerInterface
{
    private:
        UdevDeviceManager(const UdevDeviceManager&) = delete;
        UdevDeviceManager& operator=(const UdevDeviceManager&) = delete;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (DeviceManagerInterface::Register(&UdevDeviceManager::CreateObject));
        }
        AudioMixer *mObjAudioMixer;
        vectorUdevDeviceInfo mMicInfo;
        vectorUdevDeviceInfo mSpeakerInfo;
        void printMicInfo();
        void setUSBMicStatus(const std::string& deviceName, const int& cardNumber, const bool& status);
        void setUSBSpeakerStatus(const std::string& deviceName, const int& cardNumber, const bool& status);
        bool getUSBMicStatus(const int &cardNumber, const bool& isLoaded);
        bool getUSBSpeakerStatus(const int &cardNumber, const bool& isLoaded);
        std::string getUSBMicName(const int &cardNumber, const bool& isLoaded);
        std::string getUSBSpeakerName(const int &cardNumber, const bool& isLoaded);

    public:
         UdevDeviceManager();
        ~UdevDeviceManager();
        static DeviceManagerInterface* CreateObject()
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the UdevDeviceManager handler");
                return new(std::nothrow) UdevDeviceManager();
            }
            return nullptr;
        }
        std::string onDeviceAdded(Device *device);
        std::string onDeviceRemoved(Device *device);
        bool setHeadphoneSoundOut(const bool& connected);
};
#endif //_UDEV_DEVICE_MANAGER_H_