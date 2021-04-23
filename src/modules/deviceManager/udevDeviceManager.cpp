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
        if (device->event == "headset-inserted") {
            if (!setHeadphoneSoundOut(true))
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
        }
        else if (device->event == "usb-mic-inserted") {
            if (!audioMixerObj->loadUSBSinkSource('j', device->soundCardNumber, device->deviceNumber, connected))
                reply = INVALID_PARAMETER_ERROR(device->soundCardNumber, integer);
        }
        else if (device->event == "usb-headset-inserted") {
            if (!audioMixerObj->loadUSBSinkSource('z', device->soundCardNumber, device->deviceNumber, connected))
                reply = INVALID_PARAMETER_ERROR(device->soundCardNumber, integer);
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
        if (device->event == "headset-removed") {
            if (!setHeadphoneSoundOut(false))
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
        }
        else if (device->event == "usb-mic-removed") {
            if (!audioMixerObj->loadUSBSinkSource('j', device->soundCardNumber, device->deviceNumber, connected))
                reply = INVALID_PARAMETER_ERROR(device->soundCardNumber, integer);
        }
        else if (device->event == "usb-headset-removed") {
            if (!audioMixerObj->loadUSBSinkSource('z', device->soundCardNumber, device->deviceNumber, connected))
                reply = INVALID_PARAMETER_ERROR(device->soundCardNumber, integer);
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

UdevDeviceManager::UdevDeviceManager()
{
    PM_LOG_DEBUG("UdevDeviceManager constructor");
}

UdevDeviceManager::~UdevDeviceManager()
{
    PM_LOG_DEBUG("UdevDeviceManager destructor");
}