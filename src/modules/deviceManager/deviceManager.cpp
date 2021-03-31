/* @@@LICENSE
*
*      Copyright (c) 2021 LG Electronics Company.
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
#include "deviceManager.h"

bool DeviceManager::mIsObjRegistered = DeviceManager::RegisterObject();

DeviceManager* DeviceManager::mObjDeviceManager = nullptr;
DeviceManagerInterface* DeviceManager::mClientDeviceManagerInstance = nullptr;
DeviceManagerInterface* DeviceManagerInterface::mClientInstance = nullptr;
pFuncCreateClient DeviceManagerInterface::mClientFuncPointer = nullptr;

bool DeviceManager::_event(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("DeviceManager: event");
    std::string reply = STANDARD_JSON_SUCCESS;
    LSMessageJsonParser    msg(message, SCHEMA_3(REQUIRED(event, string),\
        OPTIONAL(soundcard_no, integer),OPTIONAL(device_no, integer)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;
    //read optional parameters with appropriate default values
    int soundcard_no = -1;
    int device_no = -1;
    if (!msg.get("soundcard_no", soundcard_no))
        soundcard_no = 0;
    if (!msg.get("device_no", device_no))
        device_no = 0;
    std::string event;
    if (!msg.get("event", event))
    {
        reply = MISSING_PARAMETER_ERROR(event, string);
    }
    else
    {
        if (mClientDeviceManagerInstance)
        {
            Device device;
            device.event = event;
            device.soundCardNumber = soundcard_no;
            device.deviceNumber = device_no;
            if ("headset-inserted" == event || "usb-mic-inserted" == event || "usb-headset-inserted" == event)
            {
                reply = mClientDeviceManagerInstance->onDeviceAdded(&device);
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManager: device added event call to device manager client object is success");
            }
            else if ("headset-removed" == event || "usb-mic-removed" == event || "usb-headset-removed" == event)
            {
                reply = mClientDeviceManagerInstance->onDeviceRemoved(&device);
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManager: device removed event call to device manager client object is success");
            }
            else
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Invalid device event received");
                reply = INVALID_PARAMETER_ERROR(event, string);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Client DeviceManagerInstance is nullptr");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "DeviceManager Instance is nullptr");
        }
    }
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return true;
}

DeviceManager* DeviceManager::getDeviceManagerInstance()
{
    return mObjDeviceManager;
}

DeviceManager::DeviceManager(ModuleConfig* const pConfObj)
{
    PM_LOG_DEBUG("DeviceManager constructor");
    mClientDeviceManagerInstance = DeviceManagerInterface::getClientInstance();
    if (!mClientDeviceManagerInstance)
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "mClientDeviceManagerInstance is nullptr");
}

DeviceManager::~DeviceManager()
{
    PM_LOG_DEBUG("DeviceManager destructor");
    if (mClientDeviceManagerInstance)
    {
        delete mClientDeviceManagerInstance;
        mClientDeviceManagerInstance = nullptr;
    }
}

LSMethod DeviceManager::deviceManagerMethods[] = {
    { "event", _event},
    {},
};

void DeviceManager::initialize()
{
    if (mObjDeviceManager)
    {
        bool result = false;
        CLSError lserror;

        result = LSRegisterCategoryAppend(GetPalmService(), "/udev", DeviceManager::deviceManagerMethods, nullptr, &lserror);
        if (!result || !LSCategorySetData(GetPalmService(), "/udev", mObjDeviceManager, &lserror))
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
                "%s: Registering Service for '%s' category failed", __FUNCTION__, "/udev");
        }
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
            "Successfully initialized DeviceManager");
    }
    else
    {
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
            "mObjDeviceManager is nullptr");
    }
}

void DeviceManager::deInitialize()
{
    PM_LOG_DEBUG("DeviceManager deInitialize()");
    if (mObjDeviceManager)
    {
        delete mObjDeviceManager;
        mObjDeviceManager = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "mObjDeviceManager is nullptr");
}

void DeviceManager::handleEvent(events::EVENTS_T* ev)
{
}
