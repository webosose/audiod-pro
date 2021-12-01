/* @@@LICENSE
*
*      Copyright (c) 2020-2021 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */
#include "udevDeviceManager.h"

#define SYSFS_HEADSET_MIC "/sys/devices/virtual/switch/h2w/state"

bool UdevDeviceManager::mIsObjRegistered = UdevDeviceManager::RegisterObject();

std::string UdevDeviceManager::onDeviceAdded(Device *device)
{
    std::string reply = STANDARD_JSON_SUCCESS;
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    bool connected = true;
    if (audioMixerObj)
    {
        std::string deviceName;
        if (device->event == "headset-inserted") {
            if (!setHeadphoneSoundOut(true))
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
        }
        else if (device->event == "usb-mic-inserted") {
            printMicInfo();
            if (getUSBMicStatus(device->soundCardNumber, true))
            {
                PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                    "onDeviceAdded: Mic is already loaded");
            }
            else
            {
                deviceName = getUSBMicName(device->soundCardNumber, true);
                if (!deviceName.empty())
                {
                    if (!audioMixerObj->loadUSBSinkSource('j', device->soundCardNumber, device->deviceNumber, connected, deviceName.c_str()))
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                    else
                        setUSBMicStatus(deviceName, device->soundCardNumber, true);
                }
            }
        }
        else if (device->event == "usb-headset-inserted") {
            if (getUSBSpeakerStatus(device->soundCardNumber, true))
            {
                PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                    "onDeviceAdded: Speaker is already loaded");
            }
            else
            {
                deviceName = getUSBSpeakerName(device->soundCardNumber, true);
                if (!deviceName.empty())
                {
                    if (!audioMixerObj->loadUSBSinkSource('z', device->soundCardNumber, device->deviceNumber, 1, deviceName.c_str()))
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                    else
                        setUSBSpeakerStatus(deviceName, device->soundCardNumber, true);
                }
            }
        }
        else
            reply = INVALID_PARAMETER_ERROR(device->event, string);
    }
    else
    {
        PM_LOG_ERROR(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
            "onDeviceAdded: audioMixerObj is null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    return reply;
}

std::string UdevDeviceManager::onDeviceRemoved(Device *device)
{
    std::string reply = STANDARD_JSON_SUCCESS;
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    bool connected = false;
    if (audioMixerObj)
    {
        std::string deviceName;
        if (device->event == "headset-removed") {
            if (!setHeadphoneSoundOut(false))
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
        }
        else if (device->event == "usb-mic-removed") {
            printMicInfo();
            if (!getUSBMicStatus(device->soundCardNumber, false))
            {
                PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                    "onDeviceRemoved: Mic is already unloaded");
            }
            else
            {
                deviceName = getUSBMicName(device->soundCardNumber, false);
                if (!deviceName.empty())
                {
                    if (!audioMixerObj->loadUSBSinkSource('j', device->soundCardNumber, device->deviceNumber, 0, deviceName.c_str()))
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                    else
                        setUSBMicStatus(deviceName, device->soundCardNumber, false);
                }
            }
        }
        else if (device->event == "usb-headset-removed") {
            if (!getUSBSpeakerStatus(device->soundCardNumber, false))
            {
                PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                    "onDeviceRemoved: Speaker is already unloaded");
            }
            else
            {
                deviceName = getUSBSpeakerName(device->soundCardNumber, false);
                if (!deviceName.empty())
                {
                    if (!audioMixerObj->loadUSBSinkSource('z', device->soundCardNumber, device->deviceNumber, 0, deviceName.c_str()))
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                    else
                        setUSBSpeakerStatus(deviceName, device->soundCardNumber, false);
                }
            }
        }
        else
            reply = INVALID_PARAMETER_ERROR(device->event, string);
    }
    else
    {
        PM_LOG_ERROR(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
            "onDeviceRemoved: audioMixerObj is null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    return reply;
}

bool UdevDeviceManager::setHeadphoneSoundOut(const bool& connected)
{
    ModuleManager* mObjModuleManager = ModuleManager::getModuleManagerInstance();
    PM_LOG_DEBUG("UdevDeviceManager setHeadphoneSoundOut");
    events::EVENT_DEVICE_CONNECTION_STATUS_T  stEventDeviceConnectionStatus;
    stEventDeviceConnectionStatus.eventName = utils::eEventDeviceConnectionStatus;
    stEventDeviceConnectionStatus.devicename = "pcm_headphone";
    if (connected)
    {
        stEventDeviceConnectionStatus.deviceStatus = utils::eDeviceConnected;
    }
    else
    {
        stEventDeviceConnectionStatus.deviceStatus = utils::eDeviceDisconnected;

    }
    stEventDeviceConnectionStatus.mixerType = utils::ePulseMixer;
    mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stEventDeviceConnectionStatus);
    return true;
}

void UdevDeviceManager::printMicInfo()
{
    PM_LOG_DEBUG("printMicInfo:");
    for (const auto& element : mMicInfo)
        PM_LOG_DEBUG("status:%d cardnumber:%d", element.first, element.second);
}

void UdevDeviceManager::setUSBMicStatus(const std::string& deviceName, const int& cardNumber, const bool& status)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "setUSBMicStatus for deviceName:%s, card:%d, status:%d", deviceName.c_str(), cardNumber, (int)status);
    if (status)
    {
        if (deviceName == USB_MIC_ZERO_NAME)
            mMicInfo[USB_MIC0] = {true, cardNumber};
    }
    else
    {
        if (deviceName == USB_MIC_ZERO_NAME)
            mMicInfo[USB_MIC0] = {false, -1};
    }
    printMicInfo();
}

void UdevDeviceManager::setUSBSpeakerStatus(const std::string& deviceName, const int& cardNumber, const bool& status)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "setUSBSpeakerStatus for deviceName:%s, card:%d, status:%d", deviceName.c_str(), cardNumber, (int)status);
    if (status)
    {
        if (deviceName == USB_SPEAKER_ZERO_NAME)
            mSpeakerInfo[USB_SPEAKER0] = {true, cardNumber};
    }
    else
    {
        if (deviceName == USB_SPEAKER_ZERO_NAME)
            mSpeakerInfo[USB_SPEAKER0] = {false, -1};
    }
}

bool UdevDeviceManager::getUSBMicStatus(const int &cardNumber, const bool& isLoaded)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "getUSBMicStatus for card:%d, isLoaded:%d", cardNumber, (int)isLoaded);
    if (isLoaded)
    {
        if ((mMicInfo[USB_MIC0].first) && (cardNumber == mMicInfo[USB_MIC0].second))
            return true;
    }
    else
    {
        if (cardNumber == mMicInfo[USB_MIC0].second)
            return true;
    }
    return false;
}

bool UdevDeviceManager::getUSBSpeakerStatus(const int &cardNumber, const bool& isLoaded)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "getUSBSpeakerStatus for card:%d, isLoaded:%d", cardNumber, (int)isLoaded);
    if (isLoaded)
    {
        if ((mSpeakerInfo[USB_SPEAKER0].first) && (cardNumber == mSpeakerInfo[USB_SPEAKER0].second))
            return true;
    }
    else
    {
        if (cardNumber == mSpeakerInfo[USB_SPEAKER0].second)
            return true;
    }
    return false;
}

std::string UdevDeviceManager::getUSBMicName(const int &cardNumber, const bool& isLoaded)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "getUSBMicName for card:%d, isLoaded:%d", cardNumber, (int)isLoaded);
    std::string deviceName;
    if (isLoaded)
    {
        if ((!mMicInfo[USB_MIC0].first))
            deviceName = USB_MIC_ZERO_NAME;
        else
            PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                "getUSBMicName: Max mic count reached");
    }
    else
    {
        if (mMicInfo[USB_MIC0].first &&\
            mMicInfo[USB_MIC0].second == cardNumber)
            deviceName = USB_MIC_ZERO_NAME;
        else
            PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                "getUSBMicName: This mic is not loaded");
    }
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "getUSBMicName = %s", deviceName.c_str());
    return deviceName;
}

std::string UdevDeviceManager::getUSBSpeakerName(const int &cardNumber, const bool& isLoaded)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT,\
        "getUSBSpeakerName for card:%d, isLoaded:%d", cardNumber, (int)isLoaded);
    std::string deviceName;
    if (isLoaded)
    {
        if ((!mSpeakerInfo[USB_SPEAKER0].first))
            deviceName = USB_SPEAKER_ZERO_NAME;
        else
            PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                "getUSBSpeakerName: Max usb speaker  count reached");
    }
    else
    {
        if (mSpeakerInfo[USB_SPEAKER0].first &&\
            mSpeakerInfo[USB_SPEAKER0].second == cardNumber)
            deviceName = USB_SPEAKER_ZERO_NAME;
        else
            PM_LOG_WARNING(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                "getUSBSpeakerName: This speaker is not loaded");
    }
    return deviceName;
}

UdevDeviceManager::UdevDeviceManager():mMicInfo(USB_DEVICE_COUNT, {false, -1}),\
                                       mSpeakerInfo(USB_DEVICE_COUNT, {false, -1})
{
    PM_LOG_DEBUG("UdevDeviceManager constructor");
}

UdevDeviceManager::~UdevDeviceManager()
{
    PM_LOG_DEBUG("UdevDeviceManager destructor");
}