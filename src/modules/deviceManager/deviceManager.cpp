/* @@@LICENSE
*
*      Copyright (c) 2021-2022 LG Electronics Company.
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

void DeviceManager::eventMixerStatus (bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    if (mixerStatus && (mixerType == utils::ePulseMixer))
    {
        mObjAudioMixer->sendInternalDeviceInfo(true, mSinkInfo.size());
        mObjAudioMixer->sendInternalDeviceInfo(false, mSourceInfo.size());
        if (!alsaConfRead) {
            readAlsaCardFile();
        }
        for (auto items:mSourceInfo)
        {
            if (items.second.cardNum != -1)
            {
                mObjAudioMixer->loadInternalSoundCard('i',items.second.cardNum, items.second.deviceNum, 1, false, items.second.name.c_str());
            }
        }
        for (auto items:mSinkInfo)
        {
            if (items.second.cardNum != -1)
            {
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"eventmixerstatus cardno : %d", items.second.cardNum);
                mObjAudioMixer->loadInternalSoundCard('i',items.second.cardNum, items.second.deviceNum, 1, true, items.second.name.c_str());
            }
        }
        for (auto items = mDeviceAddedQueue.begin(); items!=mDeviceAddedQueue.end(); items++)
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"Found unread udev event for device add");
            mClientDeviceManagerInstance->onDeviceAdded(&(*items));
        }
        mDeviceAddedQueue.clear();
        for (auto items = mDeviceRemovedQueue.begin(); items!=mDeviceRemovedQueue.end(); items++)
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"Found unread udev event for device removed");
            mClientDeviceManagerInstance->onDeviceRemoved(&(*items));
        }
        mDeviceRemovedQueue.clear();
    }
}

bool DeviceManager::setDeviceJsonDetails()
{
    PM_LOG_DEBUG("DeviceManager: setDeviceJsonDetails");
    std::unique_ptr<deviceConfigReader> deviceConfigReaderObj ( new (std::nothrow) deviceConfigReader());
    pbnjson::JValue deviceInfo;
    if (deviceConfigReaderObj->loadDeviceInfoJson())
    {
        deviceInfo = deviceConfigReaderObj->getDeviceInfo();
        if (!deviceInfo.isArray())
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "deviceInfo not an array");
            return false;
        }

        for (pbnjson::JValue arrItem: deviceInfo.items())
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s",arrItem.stringify().c_str());
            std::string deviceShortName;
            std::string sinkName;
            std::string type;
            int deviceNum;

            if(arrItem["shortName"].asString(deviceShortName)!= CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find device short name");
                return false;
            }
            mCardNames.push_back(deviceShortName);

            if(arrItem["Type"].asString(type)!= CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find type");
                return false;
            }
            if (arrItem["sink"].isArray() && arrItem["sink"].arraySize() > 0)
            {
                cardDetail newCard;
                std::string sinkName;
                int deviceNum;

                pbnjson::JValue sinkDetails =  arrItem["sink"];
                for (pbnjson::JValue sinkArrItem : sinkDetails.items())
                {
                    if(sinkArrItem["Name"].asString(sinkName) != CONV_OK)
                    {
                        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find device sinkName");
                    }
                    if(sinkArrItem["deviceId"].asNumber(deviceNum) != CONV_OK)
                    {
                        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find deviceId");
                    }
                    newCard.cardId=deviceShortName;
                    newCard.cardNum=-1;
                    newCard.deviceNum=deviceNum;
                    newCard.name=sinkName;
                    mSinkInfo[deviceShortName] = newCard;
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "sink array not found");
            }

            if (arrItem["source"].isArray() && arrItem["source"].arraySize() > 0)
            {
                cardDetail newCard;
                std::string sourceName;
                int deviceNum;

                pbnjson::JValue sourceDetails =  arrItem["source"];

                for (pbnjson::JValue sourceArrItem : sourceDetails.items())
                {

                    if(sourceArrItem["Name"].asString(sourceName) != CONV_OK)
                    {
                        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find device sourceName");
                    }
                    if(sourceArrItem["deviceId"].asNumber(deviceNum) != CONV_OK)
                    {
                        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find deviceId");
                    }
                    newCard.cardId=deviceShortName;
                    newCard.cardNum=-1;
                    newCard.deviceNum=deviceNum;
                    newCard.name=sourceName;
                    mSourceInfo[deviceShortName] = newCard;
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "source array not found");
            }
        }
        for (const auto &it : mSinkInfo)
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"details:");
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"cardname:%s",it.first.c_str());
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"second  : %s",it.second.name.c_str());
        }
    }
    return true;
}

bool DeviceManager::readAlsaCardFile()
{
    PM_LOG_DEBUG("DeviceManager: readAlsaCardFile");

    std::ifstream ifs("/proc/asound/cards");

    if(!ifs)
    {
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"/proc/asound/card file not exist");
        return false;
    }

    int cardnum;
    std::string details;
    while(getline(ifs, details))
    {
        for (const auto it : mCardNames)
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"name:%s",it.c_str());
            if (details.find(it) != std::string::npos)
            {
                std::stringstream ss(details);
                int cardno;
                ss>>cardno;
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "cardno : %s:%d",details.c_str(),cardno);

                if (mSinkInfo.find(it) != mSinkInfo.end())
                {
                    mSinkInfo[it].cardNum=cardno;
                }
                if (mSourceInfo.find(it)!=mSourceInfo.end())
                {
                    mSourceInfo[it].cardNum=cardno;
                }
                ss.str("");
                ss.clear();
            }
        }
    }
    ifs.close();
    return true;
}

void DeviceManager::addEventToQueue(bool isAdd, const Device& device)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"DeviceManager: addEventToQueue - isAdd : %d",(int)isAdd);
    if(isAdd)
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"mDeviceAddedQueue");
        mDeviceAddedQueue.push_back(device);
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"mDeviceAddedQueue %d",mDeviceAddedQueue.size());
    }
    else
    {
        mDeviceRemovedQueue.push_back(device);
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"mDeviceRemovedQueue %d",mDeviceRemovedQueue.size());
    }
}

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
                if (!mObjDeviceManager->mObjAudioMixer->getPulseMixerReadyStatus())
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Pulsemixer not ready, adding to queue");
                    mObjDeviceManager->addEventToQueue(true, device);
                }
                else
                {
                    reply = mClientDeviceManagerInstance->onDeviceAdded(&device);
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManager: device added event call to device manager client object is success");
                }
            }
            else if ("headset-removed" == event || "usb-mic-removed" == event || "usb-headset-removed" == event)
            {
                if (!mObjDeviceManager->mObjAudioMixer->getPulseMixerReadyStatus())
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Pulsemixer not ready, adding to queue");
                    mObjDeviceManager->addEventToQueue(false, device);
                }
                else
                {
                    reply = mClientDeviceManagerInstance->onDeviceRemoved(&device);
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManager: device removed event call to device manager client object is success");
                }
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
    alsaConfRead = false;
    mClientDeviceManagerInstance = DeviceManagerInterface::getClientInstance();
    if (!mClientDeviceManagerInstance)
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "mClientDeviceManagerInstance is nullptr");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();

    if (setDeviceJsonDetails())
    {
        alsaConfRead = readAlsaCardFile();
    }

    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventMixerStatus);
    }
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
    switch(ev->eventName)
    {
        case utils::eEventMixerStatus:
        {
            events::EVENT_MIXER_STATUS_T *stMixerStatus = (events::EVENT_MIXER_STATUS_T*)ev;
            eventMixerStatus(stMixerStatus->mixerStatus, stMixerStatus->mixerType);
        }
        break;
        default:
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"unknown event");
        }
        break;
    }
}
