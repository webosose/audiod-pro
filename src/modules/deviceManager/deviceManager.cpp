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

#define GET_ATTACHED_NOSTORAGE_DEVICES_URI "luna://com.webos.service.pdm/getAttachedNonStorageDeviceList"
#define GET_ATTACHED_NOSTORAGE_DEVICES_PAYLOAD "{\"subscribe\":true,\"category\":\"Audio\",\"groupSubDevices\":true}"
#define SYSNAME_BASE_PATH "/sys"

bool DeviceManager::mIsObjRegistered = DeviceManager::RegisterObject();
DeviceManager* DeviceManager::mObjDeviceManager = nullptr;
DeviceManagerInterface* DeviceManager::mClientDeviceManagerInstance = nullptr;
DeviceManagerInterface* DeviceManagerInterface::mClientInstance = nullptr;
pFuncCreateClient DeviceManagerInterface::mClientFuncPointer = nullptr;

void DeviceManager::eventMixerStatus (bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
     PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "eventMixerStatus");
     if (mixerType == utils::ePulseMixer)
     {
        if (mixerStatus)
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "eventMixerStatus : pulsemixer is connected");
            mObjAudioMixer->sendInternalDeviceInfo(true, internalSinkCount);
            mObjAudioMixer->sendInternalDeviceInfo(false, internalSourceCount);
            for (auto it : mPhyInternalInfo)
            {
                for(auto &devices:it.second)
                {
                    if (devices.cardNumber != -1)
                        loadInternalCard(devices);
                }
            }
            for (auto &it : mPhyExternalInfo)
            {
                for(auto &devices:it.second)
                {
                    loadUnloadExternalCard(devices,true);
                }
            }
        }
     }
}

bool DeviceManager::loadInternalCard (utils::CARD_INFO_T& cardInfo)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "loadInternalCard cardInfo.isConnected = %d",cardInfo.isConnected);
    bool ret = false;
    if (cardInfo.isConnected == false)
    {
        PM_LOG_DEBUG("calling load internal card with parameters cardno :%d,deviceno:%d,status:%d,isoutput:%d,name:%s",\
            cardInfo.cardNumber, cardInfo.deviceID,1,cardInfo.isOutput,cardInfo.name.c_str());
        ret = mObjAudioMixer->loadInternalSoundCard('i',cardInfo.cardNumber, cardInfo.deviceID,1,cardInfo.isOutput,cardInfo.name.c_str());
        cardInfo.isConnected = true; // TODO: move to callback once socket comm initiative completed
    }
    else
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, " Internal card already loaded");
    }
    return ret;
}

bool DeviceManager::loadUnloadExternalCard (utils::CARD_INFO_T& cardInfo, bool isLoad)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "loadUnloadExternalCard %d,%d",isLoad,cardInfo.isConnected);
    bool ret = false;
    char cmd = cardInfo.isOutput?'z':'j';
    if (isLoad && cardInfo.isConnected == false)
    {
        PM_LOG_DEBUG("calling load External card with parameters cmd : %c,cardno :%d,deviceno:%d,status:%d,isoutput:%d",\
                cmd, cardInfo.cardNumber, cardInfo.deviceID,isLoad,cardInfo.isOutput);
        ret = mObjAudioMixer->loadUSBSinkSource(cmd, cardInfo.cardNumber,cardInfo.deviceID, isLoad);
        cardInfo.isConnected = true; // TODO: move to callback once socket comm initiative completed
    }
    else if( !isLoad && cardInfo.isConnected)
    {
        PM_LOG_DEBUG("calling Unload External card with parameters cmd : %c,cardno :%d,deviceno:%d,status:%d,isoutput:%d",\
                cmd, cardInfo.cardNumber, cardInfo.deviceID,isLoad,cardInfo.isOutput);
        ret = mObjAudioMixer->loadUSBSinkSource(cmd, cardInfo.cardNumber,cardInfo.deviceID, isLoad);
        cardInfo.isConnected = false; // TODO: move to callback once socket comm initiative completed
    }
    return ret;
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
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "deviceInfo is not an array");
            return false;
        }
        pbnjson::JValue internalListInfo;
        pbnjson::JValue externalListInfo;
        for (const auto& it : deviceInfo.items())
        {
            if (it.hasKey("internalList"))
                internalListInfo = it["internalList"];
            else if (it.hasKey("externalList"))
                externalListInfo = it["externalList"];
        }
        if(!setInternalCardDetails(internalListInfo) || !setExternalCardDetails(externalListInfo))
            return false;
    }
    return true;
}

bool DeviceManager:: setInternalCardDetails(const pbnjson::JValue& internalListInfo)
{
    PM_LOG_DEBUG("DeviceManager:: setInternalCardDetails");
    if (!internalListInfo.isArray())
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "internalListInfo is not an array");
        return false;
    }
    for (pbnjson::JValue arrItem: internalListInfo.items())
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s",arrItem.stringify().c_str());
        std::string cardName;
        if(arrItem["cardName"].asString(cardName)!= CONV_OK)
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find device card name");
            return false;
        }
        if (arrItem["sink"].isArray() && arrItem["sink"].arraySize() > 0)
        {
            utils::CARD_INFO_T newCard;
            std::string sinkName;
            std::string sinkType;
            int deviceID;
            int mmap;
            int tsched;
            int fragmentSize;
            internalSinkCount++;
            pbnjson::JValue sinkDetails =  arrItem["sink"];
            for (pbnjson::JValue sinkArrItem : sinkDetails.items())
            {
                if(sinkArrItem["Name"].asString(sinkName) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink Name");
                }
                if(sinkArrItem["type"].asString(sinkType) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink type,setting default to pcm");
                    sinkType="pcm";
                }
                if(sinkArrItem["deviceID"].asNumber(deviceID) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink deviceID");
                }
                if (sinkArrItem["mmap"].asNumber(mmap) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink mmap,setting default to 0");
                    mmap=0;
                }
                if (sinkArrItem["tsched"].asNumber(tsched) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink tsched,setting default to 0");
                    tsched=0;
                }
                if (sinkArrItem["fragmentSize"].asNumber(fragmentSize) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink fragmentSize,,setting default to 4096");
                    fragmentSize=4096;
                }
                if (sinkArrItem["preCondition"].isArray() && sinkArrItem["preCondition"].arraySize() > 0)
                {
                    pbnjson::JValue preConditionDetails =  sinkArrItem["preCondition"];
                    for (auto const& item : preConditionDetails.items())
                        newCard.preConditionList.push_back(item.asString());
                }
                else
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "precondition array not found");
                }
                newCard.cardName=cardName;
                newCard.name=sinkName;
                newCard.type=sinkType;
                newCard.deviceID=deviceID;
                newCard.mmap=mmap;
                newCard.cardNumber=-1;
                newCard.tsched=tsched;
                newCard.fragmentSize=fragmentSize;
                newCard.isOutput=true;
                newCard.deviceType="playback";
                auto it = internalDevices.find(cardName);
                if( it != internalDevices.end())
                    it->second.push_back(newCard);
                else
                {
                    std::vector<utils::CARD_INFO_T> tmpCardInfo;
                    tmpCardInfo.push_back(newCard);
                    internalDevices[cardName] = tmpCardInfo;
                }
            }
        }
        else
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "sink array not found");
        }
        if (arrItem["source"].isArray() && arrItem["source"].arraySize() > 0)
        {
            utils::CARD_INFO_T newCard;
            std::string sourceName;
            std::string sourceType;
            int deviceID;
            int mmap = 0;
            int tsched = 0;
            int fragmentSize;
            internalSourceCount++;
            pbnjson::JValue sourceDetails =  arrItem["source"];
            for (pbnjson::JValue sourceArrItem : sourceDetails.items())
            {
                if(sourceArrItem["Name"].asString(sourceName) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source Name");
                }
                if(sourceArrItem["type"].asString(sourceType) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source type,,setting default to pcm");
                    sourceType = "pcm";
                }
                if(sourceArrItem["deviceID"].asNumber(deviceID) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source deviceID");
                }
                if (sourceArrItem["mmap"].asNumber(mmap) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source mmap,,setting default to 0");
                    mmap=0;
                }
                if (sourceArrItem["tsched"].asNumber(tsched) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source tsched,,setting default to 0");
                    tsched=0;
                }
                if (sourceArrItem["fragmentSize"].asNumber(fragmentSize) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source fragmentSize,,setting default to 4096");
                    fragmentSize=4096;
                }
                if (sourceArrItem["preCondition"].isArray() && sourceArrItem["preCondition"].arraySize() > 0)
                {
                    pbnjson::JValue preConditionDetails =  sourceArrItem["preCondition"];
                    for (auto const& item : preConditionDetails.items())
                        newCard.preConditionList.push_back(item.asString());
                }
                else
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "precondition array not found");
                }
                newCard.cardName=cardName;
                newCard.name=sourceName;
                newCard.type=sourceType;
                newCard.deviceID=deviceID;
                newCard.mmap=mmap;
                newCard.cardNumber=-1;
                newCard.tsched=tsched;
                newCard.fragmentSize=fragmentSize;
                newCard.isOutput = false;
                newCard.deviceType="capture";
                auto it = internalDevices.find(cardName);
                if( it != internalDevices.end())
                    it->second.push_back(newCard);
                else
                {
                    std::vector<utils::CARD_INFO_T> tmpCardInfo;
                    tmpCardInfo.push_back(newCard);
                    internalDevices[cardName] = tmpCardInfo;
                }
            }
        }
        else
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "source array not found");
        }
    }
    return true;
}

bool DeviceManager:: setExternalCardDetails(const pbnjson::JValue& externalListInfo)
{
    PM_LOG_DEBUG("DeviceManager:: setExternalCardDetails");
    if (!externalListInfo.isArray())
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "externalListInfo is not an array");
        return false;
    }
    for (pbnjson::JValue arrItem: externalListInfo.items())
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s",arrItem.stringify().c_str());
        std::string cardName;
        if(arrItem["cardName"].asString(cardName)!= CONV_OK)
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find device card name");
            return false;
        }
        if (arrItem["sink"].isArray() && arrItem["sink"].arraySize() > 0)
        {
            std::string sinkName;
            std::string sinkType;
            int deviceID;
            int mmap;
            int tsched;
            int fragmentSize;
            pbnjson::JValue sinkDetails =  arrItem["sink"];
            for (pbnjson::JValue sinkArrItem : sinkDetails.items())
            {
                if(sinkArrItem["Name"].asString(sinkName) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink Name");
                }
                if(sinkArrItem["type"].asString(sinkType) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink type,setting default to pcm");
                    sinkType="pcm";
                }
                if(sinkArrItem["deviceID"].asNumber(deviceID) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink deviceID");
                }
                if (sinkArrItem["mmap"].asNumber(mmap) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink mmap,setting default to 0");
                    mmap=0;
                }
                if (sinkArrItem["tsched"].asNumber(tsched) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink tsched,,setting default to 0");
                    tsched=0;
                }
                if (sinkArrItem["fragmentSize"].asNumber(fragmentSize) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find sink fragmentSize,,setting default to 4096");
                    fragmentSize=4096;
                }
                if (sinkArrItem["preCondition"].isArray() && sinkArrItem["preCondition"].arraySize() > 0)
                {
                    pbnjson::JValue preConditionDetails =  sinkArrItem["preCondition"];
                    for (auto const& item : preConditionDetails.items())
                        outputDevices.preConditionList.push_back(item.asString());
                }
                else
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "precondition array not found");
                }
                outputDevices.name = sinkName;
                outputDevices.fragmentSize = fragmentSize;
                outputDevices.mmap = mmap;
                outputDevices.tsched = tsched;
                outputDevices.type = sinkType;
            }
        }
        else
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "sink array not found");
        }
        if (arrItem["source"].isArray() && arrItem["source"].arraySize() > 0)
        {
            std::string sourceName;
            std::string sourceType;
            int deviceID;
            int mmap = 0;
            int tsched = 0;
            int fragmentSize;
            pbnjson::JValue sourceDetails =  arrItem["source"];
            for (pbnjson::JValue sourceArrItem : sourceDetails.items())
            {
                if(sourceArrItem["Name"].asString(sourceName) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source Name");
                }
                if(sourceArrItem["type"].asString(sourceType) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source type,setting default to pcm");
                    sourceType="pcm";
                }
                if(sourceArrItem["deviceID"].asNumber(deviceID) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source deviceID");
                }
                if (sourceArrItem["mmap"].asNumber(mmap) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source mmap");
                }
                if (sourceArrItem["tsched"].asNumber(tsched) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source tsched");
                }
                if (sourceArrItem["fragmentSize"].asNumber(fragmentSize) != CONV_OK)
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find source fragmentSize");
                }
                if (sourceArrItem["preCondition"].isArray() && sourceArrItem["preCondition"].arraySize() > 0)
                {
                    pbnjson::JValue preConditionDetails =  sourceArrItem["preCondition"];
                    for (auto const& item : preConditionDetails.items())
                        inputDevices.preConditionList.push_back(item.asString());
                }
                else
                {
                    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "precondition not found");
                }
                inputDevices.name = sourceName;
                inputDevices.fragmentSize = fragmentSize;
                inputDevices.mmap = mmap;
                inputDevices.tsched = tsched;
                inputDevices.type = sourceType;
            }
        }
        else
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "source array not found");
        }
    }
    return true;
}

void DeviceManager:: printIntCardInfo()
{
    PM_LOG_DEBUG("DeviceManager::printIntCardInfo");
    PM_LOG_DEBUG("****************************************");
    for (const auto it : mPhyInternalInfo)
    {
        PM_LOG_DEBUG("mPhyInternalInfo: CardName:%s", it.first.c_str());
        for (const auto& physicalInfo : it.second)
        {
            PM_LOG_DEBUG("----------------------------------------");
            PM_LOG_DEBUG("cardNumber: %d ",physicalInfo.cardNumber);
            PM_LOG_DEBUG("cardId: %s ",physicalInfo.cardId.c_str());
            PM_LOG_DEBUG("cardName: %s ",physicalInfo.cardName.c_str());
            PM_LOG_DEBUG("name: %s ",physicalInfo.name.c_str());
            PM_LOG_DEBUG("type: %s ",physicalInfo.type.c_str());
            PM_LOG_DEBUG("deviceType: %s ",physicalInfo.deviceType.c_str());
            PM_LOG_DEBUG("devPath: %s ",physicalInfo.devPath.c_str());
            PM_LOG_DEBUG("deviceID: %d ",physicalInfo.deviceID);
            PM_LOG_DEBUG("mmap: %d ",physicalInfo.mmap);
            PM_LOG_DEBUG("tsched: %d ",physicalInfo.tsched);
            PM_LOG_DEBUG("fragmentSize: %d ",physicalInfo.fragmentSize);
            PM_LOG_DEBUG("isOutput: %d ",physicalInfo.isOutput);
            PM_LOG_DEBUG("isConnected: %d ",physicalInfo.isConnected);
            PM_LOG_DEBUG("----------------------------------------");
        }
    }
    PM_LOG_DEBUG("****************************************");
}

void DeviceManager::printExtCardInfo()
{
    PM_LOG_DEBUG("DeviceManager::printExtCardInfo");
    PM_LOG_DEBUG("****************************************");
    for (const auto it : mPhyExternalInfo)
    {
        PM_LOG_DEBUG("mPhyExternalInfo: CardName:%s", it.first.c_str());
        for (const auto& physicalInfo : it.second)
        {
            PM_LOG_DEBUG("----------------------------------------");
            PM_LOG_DEBUG("cardNumber: %d ",physicalInfo.cardNumber);
            PM_LOG_DEBUG("cardId: %s ",physicalInfo.cardId.c_str());
            PM_LOG_DEBUG("cardName: %s ",physicalInfo.cardName.c_str());
            PM_LOG_DEBUG("name: %s ",physicalInfo.name.c_str());
            PM_LOG_DEBUG("type: %s ",physicalInfo.type.c_str());
            PM_LOG_DEBUG("deviceType: %s ",physicalInfo.deviceType.c_str());
            PM_LOG_DEBUG("devPath: %s ",physicalInfo.devPath.c_str());
            PM_LOG_DEBUG("deviceID: %d ",physicalInfo.deviceID);
            PM_LOG_DEBUG("mmap: %d ",physicalInfo.mmap);
            PM_LOG_DEBUG("tsched: %d ",physicalInfo.tsched);
            PM_LOG_DEBUG("fragmentSize: %d ",physicalInfo.fragmentSize);
            PM_LOG_DEBUG("isOutput: %d ",physicalInfo.isOutput);
            PM_LOG_DEBUG("isConnected: %d ",physicalInfo.isConnected);
            PM_LOG_DEBUG("----------------------------------------");
        }
    }
    PM_LOG_DEBUG("****************************************");
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
            if ("headset-inserted" == event)
            {
                mClientDeviceManagerInstance->onDeviceAdded(&device);
            }
            else if ("headset-removed" == event)
            {
                mClientDeviceManagerInstance->onDeviceRemoved(&device);
            }
            else if ("usb-mic-inserted" == event)
            {
                mObjDeviceManager->addExternalCard(soundcard_no,\
                mObjDeviceManager->getCardId(soundcard_no,true),\
                mObjDeviceManager->getCardName(soundcard_no,true),"capture",\
                mObjDeviceManager->getdevPath(soundcard_no,true));
                mObjDeviceManager->printExtCardInfo();
            }
            else if ("usb-headset-inserted" == event)
            {
                mObjDeviceManager->addExternalCard(soundcard_no,\
                mObjDeviceManager->getCardId(soundcard_no,true),\
                mObjDeviceManager->getCardName(soundcard_no,true),"playback",\
                mObjDeviceManager->getdevPath(soundcard_no,true));
                mObjDeviceManager->printExtCardInfo();
            }
            else if ("usb-mic-removed" == event)
            {
                mObjDeviceManager->removeExternalCard(soundcard_no,\
                mObjDeviceManager->getCardId(soundcard_no,true),"capture");
                mObjDeviceManager->printExtCardInfo();
            }
            else if ("usb-headset-removed" == event)
            {
                mObjDeviceManager->removeExternalCard(soundcard_no,\
                mObjDeviceManager->getCardId(soundcard_no,true),"playback");
                mObjDeviceManager->printExtCardInfo();
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

std::string DeviceManager:: getCardId(int cardNumber,bool isExternal)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s: cardNumber: %d ", __FUNCTION__,(int)cardNumber);
    utils::mapPhysicalInfo mPhysicalInfo = isExternal?mPhyExternalInfo:mPhyInternalInfo;
    for (const auto &it : mPhysicalInfo)
    {
        for (const auto &physicalInfo : it.second)
        {
            if(physicalInfo.cardNumber==cardNumber)
                return physicalInfo.cardId;
        }
    }
    PM_LOG_DEBUG("Invalid cardId");
    return "";
}

std::string DeviceManager:: getCardName(int cardNumber,bool isExternal)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s: cardNumber: %d ", __FUNCTION__,(int)cardNumber);
    utils::mapPhysicalInfo mPhysicalInfo = isExternal?mPhyExternalInfo:mPhyInternalInfo;
    for (const auto &it : mPhysicalInfo)
    {
        for (const auto &physicalInfo : it.second)
        {
            if(physicalInfo.cardNumber==cardNumber)
                return physicalInfo.cardName;
        }
    }
    PM_LOG_DEBUG("Invalid cardId");
    return "";
}

std::string DeviceManager:: getdevPath(int cardNumber,bool isExternal)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s: cardNumber: %d ", __FUNCTION__,(int)cardNumber);
    utils::mapPhysicalInfo mPhysicalInfo = isExternal?mPhyExternalInfo:mPhyInternalInfo;
    for (const auto &it : mPhysicalInfo)
    {
        for (const auto &physicalInfo : it.second)
        {
            if(physicalInfo.cardNumber==cardNumber)
                return physicalInfo.devPath;
        }
    }
    PM_LOG_DEBUG("Invalid cardId");
    return "";
}

bool DeviceManager::supportPlaybackCapture(int cardNumber,std::string deviceType)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s", __FUNCTION__);
    for( auto it = mPhyExternalInfo.begin();it != mPhyExternalInfo.end();it++)
    {
        for(auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
        {
            if(it2->cardNumber == cardNumber && it2->deviceType == deviceType)
            {
                PM_LOG_DEBUG("%s supported by cardNo:%d",deviceType.c_str(),cardNumber);
                return true;
            }
        }
    }
    PM_LOG_DEBUG("CardNo:%d doesn't support %s",cardNumber,deviceType.c_str());
    return false;
}

bool DeviceManager::removeExternalCard(int cardNumber, std::string cardId, std::string deviceType)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s", __FUNCTION__);
    if(cardId=="Device")
        cardId = "USB";
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
     "cardNumber :%d cardId: %s deviceType: %s",cardNumber, cardId.c_str(),deviceType.c_str());
    bool isOutput = (deviceType == "playback")?true:false;
    bool found = false;
    for( auto it = mPhyExternalInfo.begin();it != mPhyExternalInfo.end();it++)
    {
        for(auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
        {
            if(it2->cardNumber == cardNumber && it2->isOutput == isOutput)
            {
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"match found");
                loadUnloadExternalCard(*it2, false);
                it->second.erase(it2);
                found = true;
                break;
            }
        }
        if(it->second.empty())
        {
            it = mPhyExternalInfo.erase(it);
        }
        if (found)
            break;
    }
    return true;
}

bool DeviceManager::addExternalCard(int cardNumber, std::string cardId, std::string cardName, const std::string deviceType, const std::string devPath)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s", __FUNCTION__);
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
     "cardNumber :%d cardId: %s cardName: %s",cardNumber, cardId.c_str(),cardName.c_str());
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
     "deviceType: %s devPath: %s",deviceType.c_str(), devPath.c_str());
    utils::CARD_INFO_T physicalInfo;
    USB_DETAILS data;
    bool isOutput = (deviceType == "playback")?true:false;
    bool found = false;
    if(cardId=="Device")
        cardId = "USB";
    auto it = mPhyExternalInfo.find(cardId);
    if (it != mPhyExternalInfo.end())
    {
        for(auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
        {
            if(it2->cardNumber == cardNumber && it2->isOutput == isOutput)
            {
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
                    "External device already loaded, will not load again");
                found = true;
                break;
            }
        }
    }
    if(!found)
    {
        if( deviceType == "playback")
        {
            data = outputDevices;
        }
        else if (deviceType == "capture")
        {
            data = inputDevices;
        }
        physicalInfo.cardId=cardId;
        physicalInfo.cardName=cardName;
        physicalInfo.cardNumber = cardNumber;
        physicalInfo.devPath = devPath;
        physicalInfo.name = data.name;
        physicalInfo.type = data.type;
        physicalInfo.mmap = data.mmap;
        physicalInfo.fragmentSize = data.fragmentSize;
        physicalInfo.tsched = data.tsched;
        physicalInfo.deviceID = data.deviceID;
        physicalInfo.isOutput = data.isOutput;
        physicalInfo.deviceType = data.deviceType;
        if (mObjAudioMixer->getPulseMixerReadyStatus() == true)
        {
            loadUnloadExternalCard(physicalInfo, true);
        }
        it = mPhyExternalInfo.find(cardId);
        if( it != mPhyExternalInfo.end())
            it->second.push_back(physicalInfo);
        else
        {
            std::vector<utils::CARD_INFO_T> tmpDeviceInfo;
            tmpDeviceInfo.push_back(physicalInfo);
            mPhyExternalInfo[cardId] = tmpDeviceInfo;
        }
    }
    return true;
}

bool DeviceManager::addInternalCard(int cardNumber, std::string cardId, std::string cardName, const std::string deviceType, const std::string devPath)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s", __FUNCTION__);
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
     "cardNumber :%d cardId: %s cardName: %s ",cardNumber,cardId.c_str(),cardName.c_str());
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
     "deviceType: %s devPath: %s",deviceType.c_str(), devPath.c_str());
    utils::CARD_INFO_T physicalInfo;
    bool isOutput = (deviceType == "playback")?true:false;
    bool found = false;
    auto it = mPhyInternalInfo.find(cardId);
    if (it != mPhyInternalInfo.end())
    {
        for(auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
        {
            if(it2->cardNumber == cardNumber && it2->isOutput == isOutput)
            {
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
                    "Internal device already loaded, will not load again");
                found = true;
            }
        }
    }
    if(!found)
    {
        bool deviceFoundInlist = false;
        it = internalDevices.find(cardId);
        if (it != internalDevices.end())
        {
            for (auto& deviceInfo : it->second)
            {
                if (deviceInfo.isOutput == isOutput)
                {
                    physicalInfo.name = deviceInfo.name;
                    physicalInfo.type = deviceInfo.type;
                    physicalInfo.mmap = deviceInfo.mmap;
                    physicalInfo.fragmentSize = deviceInfo.fragmentSize;
                    physicalInfo.tsched = deviceInfo.tsched;
                    physicalInfo.isOutput = deviceInfo.isOutput;
                    physicalInfo.deviceType = deviceInfo.deviceType;
                    physicalInfo.deviceID = deviceInfo.deviceID;
                    deviceFoundInlist = true;
                    break;
                }
            }
        }
        physicalInfo.cardId=cardId;
        physicalInfo.cardName=cardName;
        physicalInfo.cardNumber = cardNumber;
        physicalInfo.devPath = devPath;
        physicalInfo.isConnected = false;
        if (deviceFoundInlist && mObjAudioMixer->getPulseMixerReadyStatus() == true)
        {
            loadInternalCard(physicalInfo);
        }
        it = mPhyInternalInfo.find(cardId);
        if( it != mPhyInternalInfo.end())
            it->second.push_back(physicalInfo);
        else
        {
            std::vector<utils::CARD_INFO_T> tmpDeviceInfo;
            tmpDeviceInfo.push_back(physicalInfo);
            mPhyInternalInfo[cardId] = tmpDeviceInfo;
        }
    }
    return true;
}

void DeviceManager::getAttachedNonStorageDeviceList(LSMessage *message)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s", __FUNCTION__);

    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__)){
        PM_LOG_DEBUG("PDM ls response parsing error");
        return;
    }
    bool returnValue = false;
    msg.get("returnValue", returnValue);
    if (!returnValue)
    {
        PM_LOG_DEBUG("PDM ls call returned subcription fail");
        return;
    }
    pbnjson::JValue deviceListInfo = msg.get();
    pbnjson::JValue audioDeviceList = deviceListInfo["audioDeviceList"];

    if (!audioDeviceList.isArray()){
        PM_LOG_DEBUG("audioDeviceList is not an array");
        return;
    }
    int newDeviceList = audioDeviceList.arraySize();
    PM_LOG_DEBUG("%s: newDeviceList size: %zu ",__FUNCTION__,newDeviceList);
    PM_LOG_DEBUG("%s: mDeviceList size: %d",__FUNCTION__,mDeviceList);

    if(newDeviceList > mDeviceList)
    {
        for (int j = 0; j < newDeviceList; j++)
        {
            pbnjson::JValue audioDeviceObject = audioDeviceList[j];

            int cardNumber;
            std::string cardName;
            std::string cardId;
            bool builtin;
            int usbPortNum;

            if (audioDeviceObject["cardNumber"].asNumber(cardNumber) != CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "cardNumber not found in Object");
            }
            if (audioDeviceObject["cardId"].asString(cardId) != CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "cardId not found in Object");
            }
            mConnectedDevices.insert(std::pair<int,std::string>(cardNumber, cardId));
            if (audioDeviceObject["cardName"].asString(cardName) != CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "cardName not found in Object");
            }
            if (audioDeviceObject["usbPortNum"].asNumber(usbPortNum) != CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find usbPortNum,setting as internal");
            }
            if(!audioDeviceObject["builtin"].asBool(builtin)==CONV_OK)
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find builtin parameter");
            }
            if(!audioDeviceObject["subDeviceList"].isArray()){
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"subDeviceList of card :%s is not an array",cardName);
                return;
            }
            pbnjson::JValue subDeviceList = audioDeviceObject["subDeviceList"];
            for (int k = 0; k < subDeviceList.arraySize(); k++)
            {
                std::string devPath;
                std::string deviceType;
                pbnjson::JValue subDeviceListObject = subDeviceList[k];
                if (subDeviceListObject["devPath"].asString(devPath) != CONV_OK)
                {
                    PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find devPath,test with default");
                }
                if (subDeviceListObject["deviceType"].asString(deviceType) != CONV_OK)
                {
                    PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Unable to find deviceType,test with default");
                }
                if(deviceType=="playback" || deviceType=="capture"){
                    if(builtin)
                        addInternalCard(cardNumber,cardId,cardName,deviceType,devPath);
                    else
                        addExternalCard(cardNumber,cardId,cardName,deviceType,devPath);
                }
            }
        }
    }
    else if(newDeviceList < mDeviceList)
    {
        std::vector<int> connectedDeviceList;
        for (int j = 0; j < newDeviceList; j++)
        {
            pbnjson::JValue audioDeviceObject = audioDeviceList[j];
            int cardNumber=audioDeviceObject["cardNumber"].asNumber<int>();
            connectedDeviceList.push_back(cardNumber);
        }
        for (auto it = mConnectedDevices.begin(); it != mConnectedDevices.end(); it++)
        {
            if (std::find(connectedDeviceList.begin(), connectedDeviceList.end(), it->first) == connectedDeviceList.end())
            {
                if(supportPlaybackCapture(it->first,"playback"))
                    removeExternalCard(it->first,it->second,"playback");
                if(supportPlaybackCapture(it->first,"capture"))
                    removeExternalCard(it->first,it->second,"capture");
                mConnectedDevices.erase(it->first);
                break;
            }
        }
    }
    else
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s : %s", __FUNCTION__,"No change in deviceList");
    mDeviceList = newDeviceList;
    printIntCardInfo();
    printExtCardInfo();
}

void DeviceManager::eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected)
{
    if (serviceName == ePdmService)
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
        "Got PDM server status event %d : %d", serviceName, connected);
        if (connected)
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "%s: connecting to '%d' server", __FUNCTION__, serviceName);
        }
    }
}

void DeviceManager::eventKeyInfo (LUNA_KEY_TYPE_E type, LSMessage *message)
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
        "%s LUNA_KEY_TYPE_E = %d", __FUNCTION__, (int)type);
    switch (type)
    {
        case eEventPdmDeviceStatus:
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
                        "DeviceManager::eventKeyInfo : eEventPdmDeviceStatus");
            getAttachedNonStorageDeviceList(message);
        }
        break;
        default:
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
                "DeviceManager::eventKeyInfo unknown event");
        }
        break;
    }
}

DeviceManager::DeviceManager(ModuleConfig* const pConfObj) : internalSinkCount(0),internalSourceCount(0),mDeviceList(0)
{
    PM_LOG_DEBUG("DeviceManager constructor");
    mClientDeviceManagerInstance = DeviceManagerInterface::getClientInstance();
    if (!mClientDeviceManagerInstance)
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "mClientDeviceManagerInstance is nullptr");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if(setDeviceJsonDetails())
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"Successfully loaded Device config json");
    }
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventMixerStatus);
    }
    printIntCardInfo();
    printExtCardInfo();
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
        PM_LOG_DEBUG("Subscribing to PDM service");
        mObjDeviceManager->mObjModuleManager->subscribeServerStatusInfo(mObjDeviceManager, ePdmService);
        mObjDeviceManager->mObjModuleManager->subscribeKeyInfo(mObjDeviceManager, eEventPdmDeviceStatus, ePdmService,\
                                                                         GET_ATTACHED_NOSTORAGE_DEVICES_URI,\
                                                                         GET_ATTACHED_NOSTORAGE_DEVICES_PAYLOAD);
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
        case utils::eEventServerStatusSubscription:
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventServerStatusSubscription");
            events::EVENT_SERVER_STATUS_INFO_T *serverStatusInfoEvent = (events::EVENT_SERVER_STATUS_INFO_T*)ev;
            eventServerStatusInfo(serverStatusInfoEvent->serviceName, serverStatusInfoEvent->connectionStatus);
        }
        break;
        case eEventPdmDeviceStatus:
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,\
                "handleEvent:: eEventPdmDeviceStatus");
            events::EVENT_KEY_INFO_T *keySubscribeInfoEvent = (events::EVENT_KEY_INFO_T*)ev;
            eventKeyInfo(keySubscribeInfoEvent->type, keySubscribeInfoEvent->message);
        }
        break;
        default:
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"unknown event");
        }
        break;
    }
}