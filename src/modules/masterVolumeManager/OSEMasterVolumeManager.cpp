// Copyright (c) 2021-2023 LG Electronics, Inc.
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

#include "OSEMasterVolumeManager.h"

#define GETSETTINGS "luna://com.webos.service.settings/getSystemSettings"
#define SETSETTINGS "luna://com.webos.service.settings/setSystemSettings"

bool OSEMasterVolumeManager::mIsObjRegistered = OSEMasterVolumeManager::RegisterObject();
OSEMasterVolumeManager::OSEMasterVolumeManager(): mCacheRead(false)
{
    PM_LOG_DEBUG("OSEMasterVolumeManager constructor");
}

bool OSEMasterVolumeManager::readInitialVolume(pbnjson::JValue settingsObj)
{
    PM_LOG_DEBUG("OSEMasterVolumeManager readInitialVolume");

    std::map<int,deviceInfo> OutputDeviceList,InputDeviceList;
    if (settingsObj.isValid() && settingsObj.isObject())
    {
        pbnjson::JValue outputInfo = settingsObj["soundOutputList"];
        if (outputInfo.isValid() && outputInfo.isArray())
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT," got soundoutput list");
            for( auto devices : outputInfo.items())
            {
                int deviceNum,volume;
                std::string deviceName, deviceNameDetail;
                if (devices["soundOutput"].asString(deviceName) == CONV_OK)
                {
                    PM_LOG_DEBUG("soundOutput read");
                }
                if (devices["deviceNameDetails"].asString(deviceNameDetail)== CONV_OK)
                {
                    PM_LOG_DEBUG("deviceNameDetails read");
                }
                devices["volume"].asNumber<int>(volume);
                devices["deviceCounter"].asNumber<int>(deviceNum);
                OutputDeviceList[deviceNum] = deviceInfo(deviceName,deviceNameDetail,volume);
            }
        }
        pbnjson::JValue inputInfo = settingsObj["soundInputList"];
        if (inputInfo.isValid() && inputInfo.isArray())
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT," got soundInputList list");
            for( auto devices : inputInfo.items())
            {
                int deviceNum,volume;
                std::string deviceName, deviceNameDetail;
                if (devices["soundInput"].asString(deviceName) == CONV_OK)
                {
                    PM_LOG_DEBUG("soundInput read");
                }
                if (devices["deviceNameDetails"].asString(deviceNameDetail)== CONV_OK)
                {
                    PM_LOG_DEBUG("deviceNameDetails read");
                }
                devices["volume"].asNumber<int>(volume);
                devices["deviceCounter"].asNumber<int>(deviceNum);
                InputDeviceList[deviceNum] = deviceInfo(deviceName,deviceNameDetail,volume);
            }
        }
    }
    //fill internal DB structs according to the device counter
    for(auto it:OutputDeviceList)
    {
        if (isInternalDevice(it.second.deviceName, true))
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"INternal device, push in intenal list");
            mIntOutputDeviceListDB.push_back(it.second);
        }
        else
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"external device, push in external list");

            mExtOutputDeviceListDB.push_back(it.second);
        }
    }
    for(auto it:InputDeviceList)
    {
        if (isInternalDevice(it.second.deviceName, false))
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"INternal device, push in intenal list");
            mIntInputDeviceListDB.push_back(it.second);
        }
        else
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"external device, push in external list");
            mExtInputDeviceListDB.push_back(it.second);
        }
    }
    mCacheRead = true;
    printDb();
    //update mastervolume of already connected devices from DB
    if(mConnectedDevicesList.size()>0)
    {
        for (auto it:mConnectedDevicesList)
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"connected afterwards it.deviceName : %s, it.deviceNameDetail : %s, it.isOutput :%d", \
                it.deviceName, it.deviceNameDetail, it.isOutput);
            deviceConnectOp(it.deviceName, it.deviceNameDetail, it.isOutput);
        }
    }
    printDb();
    return true;
}

void OSEMasterVolumeManager::sendDataToDB()
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"sendDataToDB");
    int count = 1;
    pbnjson::JObject devicedata = pbnjson::JObject();
    pbnjson::JObject finalString = pbnjson::JObject();
    pbnjson::JValue outputlist = pbnjson::Array();
    pbnjson::JValue inputlist = pbnjson::Array();
    for(auto it:mIntOutputDeviceListDB)
    {
        pbnjson::JObject data = pbnjson::JObject();
        data.put("soundOutput",it.deviceName);
        data.put("deviceNameDetails",it.deviceNameDetail);
        data.put("volume",it.volume);
        data.put("deviceCounter",count++);
        outputlist.append(data);
    }
    for(auto it:mExtOutputDeviceListDB)
    {
        pbnjson::JObject data = pbnjson::JObject();
        data.put("soundOutput",it.deviceName);
        data.put("deviceNameDetails",it.deviceNameDetail);
        data.put("volume",it.volume);
        data.put("deviceCounter",count++);
        outputlist.append(data);
    }
    devicedata = pbnjson::JObject {{"soundOutputList", outputlist}};
    finalString.put("settings",devicedata);
    finalString.put("category","sound");

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"DB output %s", finalString.stringify().c_str());
    //call set settings
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    result = LSCall(sh, SETSETTINGS, finalString.stringify().c_str(),nullptr,nullptr,nullptr, &lserror);

    finalString = pbnjson::JObject();
    count = 1;
    for(auto it:mIntInputDeviceListDB)
    {
        pbnjson::JObject data = pbnjson::JObject();
        data.put("soundInput",it.deviceName);
        data.put("deviceNameDetails",it.deviceNameDetail);
        data.put("volume",it.volume);
        data.put("deviceCounter",count++);
        inputlist.append(data);
    }
    for(auto it:mExtInputDeviceListDB)
    {
        pbnjson::JObject data = pbnjson::JObject();
        data.put("soundInput",it.deviceName);
        data.put("deviceNameDetails",it.deviceNameDetail);
        data.put("volume",it.volume);
        data.put("deviceCounter",count++);
        inputlist.append(data);
    }
    devicedata = pbnjson::JObject {{"soundInputList", inputlist}};
    finalString.put("settings",devicedata);
    finalString.put("category","sound");
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"DB input %s", finalString.stringify().c_str());
    result = LSCall(sh, SETSETTINGS, finalString.stringify().c_str(),nullptr,nullptr,nullptr, &lserror);
}

void OSEMasterVolumeManager::printDb()
{
    for (auto items: mIntOutputDeviceListDB)
    {
        PM_LOG_DEBUG("============================");
        PM_LOG_DEBUG( "mIntOutputDeviceListDB");
        PM_LOG_DEBUG( "device name %s",items.deviceName.c_str());
        PM_LOG_DEBUG( "device name detail %s",items.deviceNameDetail.c_str());
        PM_LOG_DEBUG( "device volume %d",items.volume);
        PM_LOG_DEBUG( "device status %d",items.connected);
        PM_LOG_DEBUG("============================");
    }
    for (auto items: mExtOutputDeviceListDB)
    {
        PM_LOG_DEBUG("============================");
        PM_LOG_DEBUG( "mExtOutputDeviceListDB");
        PM_LOG_DEBUG( "device name %s",items.deviceName.c_str());
        PM_LOG_DEBUG( "device name detail %s",items.deviceNameDetail.c_str());
        PM_LOG_DEBUG( "device volume %d",items.volume);
        PM_LOG_DEBUG( "device status %d",items.connected);
        PM_LOG_DEBUG("============================");
    }
    for (auto items: mIntInputDeviceListDB)
    {
        PM_LOG_DEBUG("============================");
        PM_LOG_DEBUG("mIntInputDeviceListDB");
        PM_LOG_DEBUG( "device name %s",items.deviceName.c_str());
        PM_LOG_DEBUG( "device name detail %s",items.deviceNameDetail.c_str());
        PM_LOG_DEBUG( "device volume %d",items.volume);
        PM_LOG_DEBUG( "device status %d",items.connected);
        PM_LOG_DEBUG("============================");
    }
    for (auto items: mExtInputDeviceListDB)
    {
        PM_LOG_DEBUG("============================");
        PM_LOG_DEBUG("mExtInputDeviceListDB");
        PM_LOG_DEBUG( "device name %s",items.deviceName.c_str());
        PM_LOG_DEBUG( "device name detail %s",items.deviceNameDetail.c_str());
        PM_LOG_DEBUG( "device volume %d",items.volume);
        PM_LOG_DEBUG( "device status %d",items.connected);
        PM_LOG_DEBUG("============================");
    }
}

OSEMasterVolumeManager::~OSEMasterVolumeManager()
{
    PM_LOG_DEBUG("OSEMasterVolumeManager destructor");
}


void OSEMasterVolumeManager::setActiveStatus(std::string deviceName, int display, bool isOutput, bool isActive)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, isActive:%d",deviceName.c_str(), display, isOutput, isActive);

    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"make %s as actvestatus : %d", it->first.c_str(), isActive);
        it->second.isActive = isActive;
    }
}

std::string OSEMasterVolumeManager::getActiveDevice(int display, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "display:%d, isOutput:%d", display, isOutput);
    std::string retVal;
    for(auto &it : deviceDetailMap)
    {
        //make the current active device of display as inactive.
        if(it.second.display == display && it.second.isOutput == isOutput && it.second.isActive == true)
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"retrurning %s as avtive", it.first.c_str());
            retVal = it.first;
        }
    }
    return retVal;
}

void OSEMasterVolumeManager::setConnStatus(std::string deviceName, int display, bool isOutput, bool connStatus)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, connstatus : %d"\
        ,deviceName.c_str(), display, isOutput,connStatus);
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"make %s as connected : %d", it->first.c_str(), connStatus);
        it->second.isConnected = connStatus;
    }
}

bool OSEMasterVolumeManager::getConnStatus(std::string deviceName, int display, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, "\
        ,deviceName.c_str(), display, isOutput);
    bool status = false;
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        status = it->second.isConnected;
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s as connected : %d", it->first.c_str(), status);
    }
    return status;
}

void OSEMasterVolumeManager::setDeviceVolume(std::string deviceName, int display, bool isOutput, int volume)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, volume : %d"\
        ,deviceName.c_str(), display, isOutput, volume);
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"make %s as volume : %d", it->first.c_str(), volume);
        it->second.volume = volume;
    }
}

int OSEMasterVolumeManager::getDeviceVolume(std::string deviceName, int display, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, "\
        ,deviceName.c_str(), display, isOutput);
    int volume = DEFAULT_INITIAL_VOLUME;
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        volume = it->second.volume;
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s as volume : %d", it->first.c_str(), volume);
    }
    return volume;
}

void OSEMasterVolumeManager::setDeviceMute(std::string deviceName, int display, bool isOutput, bool mute)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, mute : %d",\
        deviceName.c_str(), display, isOutput, mute);
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"make %s as volume : %d", it->first.c_str(), mute);
        it->second.muteStatus = mute;
    }
}

bool OSEMasterVolumeManager::getDeviceMute(std::string deviceName, int display, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, "\
        ,deviceName.c_str(), display, isOutput);

    bool status = false;
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        status = it->second.muteStatus;
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s as muteStatus : %d", it->first.c_str(), status);
    }
    return status;
}

int OSEMasterVolumeManager::getDeviceDisplay(std::string deviceName, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, "\
        ,deviceName.c_str());

    int display = 0;
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        display = it->second.display;
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s as display : %d", it->first.c_str(), display);
    }
    return display;
}

void OSEMasterVolumeManager::setDeviceNameDetail(std::string deviceName, int display, bool isOutput, std::string deviceNameDetail)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, deviceNameDetail %s, "\
        ,deviceName.c_str(), display, isOutput, deviceNameDetail.c_str());
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"make %s as %s", it->first.c_str(), deviceNameDetail.c_str());
        it->second.deviceNameDetail = deviceNameDetail;
    }
}

std::string OSEMasterVolumeManager::getDeviceNameDetail(std::string deviceName, int display, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, display:%d, isOutput:%d, "\
        ,deviceName.c_str(), display, isOutput);
    std::string status;
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        status = it->second.deviceNameDetail;
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s as deviceNameDetail : %s", it->first.c_str(), status.c_str());
    }
    return status;
}

bool OSEMasterVolumeManager::isValidSoundDevice(std::string deviceName, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s",__FUNCTION__);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "deviceName: %s, isOutput:%d, "\
        ,deviceName.c_str(),isOutput);
    bool status = false;
    auto it = deviceDetailMap.find(deviceName);
    if (it != deviceDetailMap.end())
    {
        status = true;
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s as valid? %d", deviceName.c_str(),status);
    }
    return status;
}

std::string OSEMasterVolumeManager::getMappedName (std::string deviceName)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s :%s",__FUNCTION__, deviceName.c_str());
    if(deviceName.find("bluez")!=deviceName.npos)
    {
        //FIXME:
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return bt speaker0");
        return "bluetooth_speaker0";
    }
    else
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"return %s",deviceName.c_str());
        return deviceName;
    }
}

std::string OSEMasterVolumeManager::getActualDeviceName (std::string mappedName)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: %s :%s",__FUNCTION__, mappedName.c_str());
    if(mappedName.find("bluetooth")!=mappedName.npos)
    {
        //FIXME:
        return mBluetoothName;
    }
    else
    {
        return mappedName;
    }
}

void OSEMasterVolumeManager::setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: setVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(volume, integer), PROP(sessionId, integer)) \
        REQUIRED_2(soundOutput, volume)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    bool status = false;
    std::string soundOutput;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int displayId = -1;
    int volume = MIN_VOLUME;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundOutputfound =false;
    int deviceDisplay=DISPLAY_ONE;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);

    if (isValidSoundDevice(soundOutput, true))
    {
        isSoundOutputfound = true;
        deviceDisplay = getDeviceDisplay(soundOutput, true);
    }

    if(!msg.get("sessionId", display))
    {
        display = deviceDisplay;
    }
    if (DISPLAY_ONE == display){
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else if (DISPLAY_TWO == display){
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMasterVolume with soundout: %s volume: %d display: %d", \
                soundOutput.c_str(), volume, displayId);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();


    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMasterVolume envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if (soundOutput != "alsa")
    {
        if(isSoundOutputfound && getConnStatus(soundOutput, display, true))
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Invalid displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Invalid displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getActiveDevice(display, true);

                soundOutput = getActualDeviceName(soundOutput);   //FIXME: to handle BT speaker

                PM_LOG_DEBUG("setting volume for soundoutput = %s", soundOutput.c_str());
                PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());

                if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(soundOutput.c_str(), volume, lshandle, message, envelope, _setVolumeCallBackPA)))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                }
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("setVolume for active device");
        if (DISPLAY_TWO == display)
        {
            std::string activeDevice = getActiveDevice(DISPLAY_TWO, true);
            activeDevice = getActualDeviceName(activeDevice);   //FIXME:
            PM_LOG_DEBUG("active soundoutput = %s", activeDevice.c_str());

            if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _setVolumeCallBackPA)))
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
        }
        else if (DISPLAY_ONE == display)
        {
            std::string activeDevice = getActiveDevice(DISPLAY_ONE, true);
            activeDevice = getActualDeviceName(activeDevice);   //FIXME:
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display 1 = %s", activeDevice.c_str());

            if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _setVolumeCallBackPA)))
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
        }
    }

    if (false == status)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    return;
}

void OSEMasterVolumeManager::setMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: setMicVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(volume, integer), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_1(volume)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    bool status = false;
    std::string soundInput;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int displayId = -1;
    int volume = MIN_VOLUME;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundInputfound =false;
    int deviceDisplay=DISPLAY_ONE;
    bool noDeviceConnected = false;

    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
    }
    else if (DISPLAY_TWO == display)
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                "displayId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if (!msg.get("soundInput", soundInput))
    {
        soundInput = getActiveDevice(display,false);
        if (soundInput.empty())
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: setMicVolume No device connected or active,");
            noDeviceConnected = true;
            soundInput = "pcm_input";
        }
    }
    msg.get("volume", volume);
    if (isValidSoundDevice(soundInput, false))
    {
        isSoundInputfound=true;
        deviceDisplay = getDeviceDisplay(soundInput, false);
    }

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mic volume %d for display: %d", volume, displayId);
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundInput volume is not in range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        return;
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMicVolume with soundin: %s volume: %d display: %d", \
        soundInput.c_str(), volume, displayId);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    envelopeRef *envelope = new (std::nothrow)envelopeRef;

    if(isSoundInputfound && getConnStatus(soundInput, display, false))
    {
        PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
        if(deviceDisplay!=display)
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Invalid displayId for the soundInput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Invalid displayId for the soundInput");
        }
        else
        {
            std::string activeDevice = getActiveDevice(display, false);
            soundInput = getActualDeviceName(soundInput);   //FIXME:
            PM_LOG_DEBUG("setting mic volume for soundInput = %s", soundInput.c_str());
            PM_LOG_DEBUG("active soundInput now = %s", activeDevice.c_str());
            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = this;

                if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setMicVolume(soundInput.c_str(), volume, lshandle, message, (void*)envelope, _setMicVolumeCallBackPA)))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set Mic volume %d for display:%d soundInput: %s", volume, displayId,soundInput.c_str());
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mic volume %d for display: %d", volume, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundInput volume is not in range");
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setMicVolume envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundInput");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
    }

    if (false == status)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    return;
}

bool OSEMasterVolumeManager::updateMasterVolumeInMap(std::string deviceName, int displayId,int volume, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: updateMasterVolumeInMap");
    std::list<deviceInfo> dbList;
    std::string modifiedDevice;
    if (isInternalDevice(deviceName,isOutput))
    {
        std::list<deviceInfo> &deviceList = getDBListRef(true, isOutput);
        for(auto &it: deviceList)
        {
            if(it.deviceName == deviceName)
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "updateMasterVolumeInMap updating volume of %s to %d", deviceName, volume);
                it.volume = volume;
                break;
            }
        }
    }
    else
    {
        std::list<deviceInfo> &deviceList = getDBListRef(false, isOutput);
        std::string modDevice = getDeviceNameDetail(deviceName,displayId, isOutput);
        for(auto &it: deviceList)
        {
            if(it.deviceNameDetail == modDevice)
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "updateMasterVolumeInMap updating volume of %s to %d", modDevice, volume);
                it.volume = volume;
                break;
            }
        }
    }
    deviceVolumeMap[deviceName] = volume;

    sendDataToDB();
    return true;
}

bool OSEMasterVolumeManager::_setMicVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "_setMicVolumeCallBackPA");

    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(volume, integer), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_1(volume)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    int display = DISPLAY_ONE;
    int volume = MIN_VOLUME;
    int displayId = -1;
    std::string soundInput;
    envelopeRef *envelope = nullptr;

    msg.get("soundInput", soundInput);
    msg.get("volume", volume);
    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;
    if (soundInput.empty())
    {
        soundInput = OSEMasterVolumeManagerObj->getActiveDevice(display, false);
    }
    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", status);
    responseObj.put("volume", volume);
    responseObj.put("soundInput", soundInput);
    std::string payload = responseObj.stringify();

    if (status)
    {
        if (OSEMasterVolumeManagerObj->updateMasterVolumeInMap(soundInput, display, volume, false))
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA::updated db");
        }
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA::Successfully set the mic volume");
        /*if (DISPLAY_ONE == display)
            OSEMasterVolumeManagerObj->displayOneMicVolume = volume;
        else
            OSEMasterVolumeManagerObj->displayTwoMicVolume = volume;*/
        OSEMasterVolumeManagerObj->setDeviceVolume(soundInput, display, false, volume);
        OSEMasterVolumeManagerObj->notifyMicVolumeSubscriber(soundInput, displayId, true);
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: Could not set mic volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }
    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: context is null");
    }
    return true;
}

bool OSEMasterVolumeManager::_setVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: _setVolumeCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(volume, integer), PROP(sessionId, integer)) REQUIRED_2(soundOutput, volume)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    std::string soundOutput;
    int volume = MIN_VOLUME;
    int displayId = -1;
    int display = DISPLAY_ONE;
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else
        displayId = DEFAULT_ONE_DISPLAY_ID;

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;


    if (soundOutput == "alsa")
        soundOutput = OSEMasterVolumeManagerObj->getActiveDevice(display, true);

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("volume", volume);
    setVolumeResponse.put("soundOutput", soundOutput);
    std::string payload = setVolumeResponse.stringify();

    std::string callerId = LSMessageGetSenderServiceName(reply);

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA::Successfully set the volume");
        if (OSEMasterVolumeManagerObj->updateMasterVolumeInMap(soundOutput,display,volume,true))
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA::updated db");
        }
        OSEMasterVolumeManagerObj->setDeviceVolume(soundOutput, display, true, volume);
        if (DISPLAY_TWO == display)
        {
            //OSEMasterVolumeManagerObj->displayTwoVolume = volume;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
        }
        else
        {
            //OSEMasterVolumeManagerObj->displayOneVolume = volume;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: Could not set the volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: context is null");
    }

    return true;
}

void OSEMasterVolumeManager::getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: getVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    std::string soundOutput;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundOutputfound =false;

    if(msg.get("soundOutput", soundOutput))
    {
        if (isValidSoundDevice(soundOutput, true))
        {
            isSoundOutputfound=true;
            display = getDeviceDisplay(soundOutput, true);
        }
    }

    msg.get("subscribe", subscribed);

    if (LSMessageIsSubscription(message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return;
        }
    }

    int displayId = DEFAULT_ONE_DISPLAY_ID;
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if (!soundOutput.empty())
    {
        if(isSoundOutputfound && getConnStatus(soundOutput,display,true))
        {
            PM_LOG_DEBUG("Get volume info for soundoutput = %s", soundOutput.c_str());
            if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
                reply = getVolumeInfo(soundOutput, displayId, callerId);
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("Get volume info for active device");
        if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
                reply = getVolumeInfo(soundOutput, displayId, callerId);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", __FUNCTION__, reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return;
}

void OSEMasterVolumeManager::getMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: getMicVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundInput, string), PROP(subscribe, boolean),PROP(displayId,integer))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    std::string soundInput;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundInputfound = false;

    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display || DISPLAY_TWO == display)
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                "displayId Valid");
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                "displayId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    msg.get("soundInput", soundInput);

    if(isValidSoundDevice(soundInput,false))
    {
        isSoundInputfound=true;
        display = getDeviceDisplay(soundInput, false);
    }

    msg.get("subscribe", subscribed);

    if (LSMessageIsSubscription(message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return;
        }
    }

    int displayId = DEFAULT_ONE_DISPLAY_ID;
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "displayId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");
    }

    if (!soundInput.empty())
    {
        if(isSoundInputfound)
        {
            PM_LOG_DEBUG("Get volume info for soundInput = %s", soundInput.c_str());
            if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
            reply = getMicVolumeInfo(soundInput, displayId, subscribed);
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("Get mic-volume info for active device");
        soundInput = getActiveDevice(display, false);
        if (!soundInput.empty())
        {

            if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
                reply = getMicVolumeInfo(soundInput, displayId, subscribed);
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", __FUNCTION__, reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return;
}

void OSEMasterVolumeManager::muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: muteVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(mute, boolean), PROP(sessionId, integer)) REQUIRED_2(soundOutput, mute)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundOutput;
    bool mute = false;
    bool status = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string reply = STANDARD_JSON_SUCCESS;
    envelopeRef *envelope = nullptr;
    bool isSoundOutputfound =false;
    int deviceDisplay=DISPLAY_ONE;

    msg.get("soundOutput", soundOutput);
    msg.get("mute", mute);

    if (isValidSoundDevice(soundOutput, true))
    {
        isSoundOutputfound=true;
        deviceDisplay=getDeviceDisplay(soundOutput, true);
    }

    if(!msg.get("sessionId", display))
    {
        display = deviceDisplay;
        displayId=3;
    }
    else
    {
        if (DISPLAY_ONE == display)
        {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
        }
        else if (DISPLAY_TWO == display)
        {
            displayId = DEFAULT_TWO_DISPLAY_ID;
            display = DISPLAY_TWO;
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");

            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
            return;
        }
    }

    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string callerId = LSMessageGetSenderServiceName(message);
    envelope = new (std::nothrow)envelopeRef;
    bool found = false;

    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }
    if (soundOutput != "alsa")
    {
        PM_LOG_DEBUG("soundOutput :%s other than alsa found",soundOutput.c_str());
        if(isSoundOutputfound)
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getActiveDevice(display,true);
                soundOutput = getActualDeviceName(soundOutput);     //FIXME:
                PM_LOG_DEBUG("setvolume for soundoutput = %s", soundOutput.c_str());
                PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());

                if (audioMixerObj && audioMixerObj->setMute(soundOutput.c_str(), mute, lshandle, message, envelope, _muteVolumeCallBackPA))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteVolume with soundout: %s mute status: %d", \
                        soundOutput.c_str(),(int)mute);
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", \
                                mute, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
                }
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("muteVolume for active device");
        if (DISPLAY_TWO == display || displayId == 3 )
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getActiveDevice(DISPLAY_TWO, true);
                activeDevice = getActualDeviceName(activeDevice);     //FIXME:
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display %d = %s", displayId, activeDevice.c_str());
                if (audioMixerObj && audioMixerObj->setMute(activeDevice.c_str(), mute, lshandle, message, envelope, _muteVolumeCallBackPA))
                {
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", \
                                mute, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
                }
            }
        }
        if (DISPLAY_ONE == display)
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getActiveDevice(DISPLAY_ONE,true);
                activeDevice = getActualDeviceName(activeDevice);     //FIXME:
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display %d = %s", displayId, activeDevice.c_str());
                if (audioMixerObj && audioMixerObj->setMute(activeDevice.c_str(), mute, lshandle, message, envelope, _muteVolumeCallBackPA))
                {
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", \
                                mute, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
                }
            }
        }
    }

    if (false == status)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    return;
}

void OSEMasterVolumeManager::muteMic(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: muteMic");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(mute, boolean), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_1(mute)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundInput;
    bool mute = false;
    bool status = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundInputfound =false;
    int deviceDisplay=DISPLAY_ONE;
    bool noDeviceConnected = false;


    msg.get("mute", mute);

    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else if (DISPLAY_TWO == display)
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                "displayId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if (!msg.get("soundInput", soundInput))
    {
        soundInput = getActiveDevice(display,false);
        if (soundInput.empty())
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: setMicVolume No device connected or active,");
            noDeviceConnected = true;
            soundInput = "pcm_input";
        }
    }

    if (isValidSoundDevice(soundInput, false))
    {
        isSoundInputfound = true;
        deviceDisplay = getDeviceDisplay(soundInput, false);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteMic with soundin: %s mute status: %d", \
                soundInput.c_str(),(int)mute);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    envelopeRef *envelope = new (std::nothrow)envelopeRef;

    if(isSoundInputfound && getConnStatus(soundInput, display, false))
    {
        PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
        if(deviceDisplay!=display)
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Invalid displayId for the soundInput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Invalid displayId for the soundInput");
        }
        else
        {
            std::string activeDevice = getActiveDevice(display, false);
            soundInput = getActualDeviceName(soundInput);     //FIXME:
            PM_LOG_DEBUG("setting mute for soundInput = %s", soundInput.c_str());
            PM_LOG_DEBUG("active soundInput now = %s", activeDevice.c_str());
            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = this;
                if (audioMixerObj && audioMixerObj->setPhysicalSourceMute(soundInput.c_str(), mute, lshandle, message, envelope, _muteMicCallBackPA))
                {
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mute for display: %d", displayId);
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteMic envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundInput");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
    }
    if (false == status)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    return;
}

bool OSEMasterVolumeManager::_muteMicCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: _muteMicCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(mute, boolean), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_1(mute)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    bool mute = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string soundInput;
    envelopeRef *envelope = nullptr;

    msg.get("soundInput", soundInput);
    msg.get("mute", mute);
    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;
    if (soundInput.empty())
    {
        soundInput = OSEMasterVolumeManagerObj->getActiveDevice(display, false);
    }
    std::string callerId = LSMessageGetSenderServiceName(message);
    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("mute", mute);
    setVolumeResponse.put("soundInput", soundInput);
    std::string payload = setVolumeResponse.stringify();

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA::Successfully set the mute for mic");
        OSEMasterVolumeManagerObj->setDeviceMute(soundInput, display, false, mute);
        OSEMasterVolumeManagerObj->notifyMicVolumeSubscriber(soundInput, displayId, true);
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: Could not set the mute for mic");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: context is null");
    }
    return true;
}

bool OSEMasterVolumeManager::_muteVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "_muteVolumeCallBackPA");

    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(mute, boolean), PROP(sessionId, integer)) REQUIRED_2(soundOutput, mute)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    std::string soundOutput;
    bool mute = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("mute", mute);
    if (!msg.get("sessionId", display))
    {
        display = DISPLAY_ONE;
        displayId = 3;
    }
    else
    {
        if (DISPLAY_ONE == display)
            displayId = 1;
        else if (DISPLAY_TWO == display)
            displayId = 2;
    }

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string callerId = LSMessageGetSenderServiceName(message);
    if(soundOutput == "alsa")
    {
        soundOutput = OSEMasterVolumeManagerObj->getActiveDevice(display,true);
    }

    pbnjson::JValue muteVolumeResponse = pbnjson::Object();
    muteVolumeResponse.put("returnValue", status);
    muteVolumeResponse.put("mute", mute);
    muteVolumeResponse.put("soundOutput", soundOutput);
    std::string payload = muteVolumeResponse.stringify();

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA::Successfully set the mic mute");

        OSEMasterVolumeManagerObj->setDeviceMute(soundOutput, display, true, mute);
        if (DEFAULT_ONE_DISPLAY_ID == displayId)
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_ONE_DISPLAY_ID, callerId);
        }
        else if (DEFAULT_TWO_DISPLAY_ID == displayId)
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_TWO_DISPLAY_ID, callerId);
        }
        else
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_ONE_DISPLAY_ID, callerId);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_TWO_DISPLAY_ID, callerId);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: Could not set mic mute");
    }
    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: volumeUp");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundOutput;
    bool status = false;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int volume = MIN_VOLUME;
    int displayVol = MIN_VOLUME;
    int displayId = -1;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundOutputfound = false;
    int deviceDisplay = DISPLAY_ONE;
    msg.get("soundOutput", soundOutput);

    if (isValidSoundDevice(soundOutput, true))
    {
        isSoundOutputfound = true;
        deviceDisplay = getDeviceDisplay(soundOutput, true);

    }
    if(!msg.get("sessionId", display))
        display = deviceDisplay;

    if (DISPLAY_TWO == display)
    {
        displayId = 2;
    }
    else if (DISPLAY_ONE == display)
    {
        displayId = 1;
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp with soundout: %s", soundOutput.c_str());
    std::string callerId = LSMessageGetSenderServiceName(message);
    AudioMixer *audioMixerInstance = AudioMixer::getAudioMixerInstance();
    bool found=false;

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if(nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeUp envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    if (audioMixerInstance)
    {
        if (soundOutput != "alsa")
        {
            PM_LOG_DEBUG("soundOutput :%s other than alsa found",soundOutput.c_str());
            if(isSoundOutputfound && getConnStatus(soundOutput, display, true))
            {
                PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
                if(deviceDisplay!=display)
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
                }
                else
                {
                    displayVol = getDeviceVolume(soundOutput, display, true);

                    if ((displayVol+1) <= MAX_VOLUME)
                    {
                        isValidVolume = true;
                        volume = displayVol+1;
                        std::string activeDevice = getActiveDevice(display,true);
                        soundOutput = getActualDeviceName(soundOutput);
                        PM_LOG_DEBUG("volume up for soundoutput = %s", soundOutput.c_str());
                        PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());
                        if ((isValidVolume) && (audioMixerInstance->setVolume(soundOutput.c_str(), volume, lshandle, message, envelope, _volumeUpCallBackPA)))
                        {
                            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                            LSMessageRef(message);
                            status = true;
                        }
                        else
                        {
                            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                        }
                    }
                    else
                    {
                        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    }
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
            }
        }
        else
        {
            PM_LOG_DEBUG("volumeUp for active device");
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getActiveDevice(display, true);

                displayVol = getDeviceVolume(activeDevice, display, true);
                if ((displayVol+1) <= MAX_VOLUME)
                {
                    isValidVolume = true;
                    volume = displayVol+1;
                    activeDevice = getActualDeviceName(activeDevice);
                    if ((isValidVolume) && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeUpCallBackPA)))
                    {
                        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                        LSMessageRef(message);
                        status = true;
                    }
                    else
                    {
                        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    }
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                }
            }

        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                      "audiomixer instance is nulptr");
    }

    if (false == status)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    return;
}

bool OSEMasterVolumeManager::_volumeUpCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: _volumeUpCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
        if (!msg.parse(__FUNCTION__,sh))
            return true;

    std::string soundOutput;
    int display = DISPLAY_ONE;
    int displayId = -1;
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string activeDevice = OSEMasterVolumeManagerObj->getActiveDevice(display,true);

    std::string callerId = LSMessageGetSenderServiceName(message);

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    if(soundOutput == "alsa")
            soundOutput = OSEMasterVolumeManagerObj->getActiveDevice(display,true);
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("soundOutput", soundOutput);
    std::string payload;

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA::Successfully set the volume");

        int volume = OSEMasterVolumeManagerObj->getDeviceVolume(soundOutput, display, true);
        volume++;
        OSEMasterVolumeManagerObj->setDeviceVolume(soundOutput, display, true, volume);

        OSEMasterVolumeManagerObj->updateMasterVolumeInMap(soundOutput,displayId, volume, true);

        if (DISPLAY_TWO == display)
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
            setVolumeResponse.put("volume", volume);
            payload = setVolumeResponse.stringify();
        }
        else
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
            setVolumeResponse.put("volume", volume);
            payload = setVolumeResponse.stringify();
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: Could not set volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: volumeDown");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundOutput;
    bool status = false;
    int displayId = -1;
    bool isValidVolume = false;
    int volume = MIN_VOLUME;
    int displayVol = MIN_VOLUME;
    int display = DISPLAY_ONE;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool found = false;

    bool isSoundOutputfound = false;
    int deviceDisplay = DISPLAY_ONE;
    msg.get("soundOutput", soundOutput);

    if (isValidSoundDevice(soundOutput, true))
    {
        isSoundOutputfound = true;
        deviceDisplay = getDeviceDisplay(soundOutput, true);
    }
    if(!msg.get("sessionId", display))
        display = deviceDisplay;

    if (DISPLAY_TWO == display)
    {
        displayId = 2;

    }
    else if (DISPLAY_ONE == display)
    {
        displayId = 1;
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }


    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    std::string callerId = LSMessageGetSenderServiceName(message);
    AudioMixer *audioMixerInstance = AudioMixer::getAudioMixerInstance();

    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "volumeDown envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown with soundout: %s", soundOutput.c_str());
    if (audioMixerInstance)
    {
        if (soundOutput != "alsa")
        {
            PM_LOG_DEBUG("soundOutput :%s other than alsa found",soundOutput.c_str());
            if(isSoundOutputfound && getConnStatus(soundOutput, display, true))
            {
                displayVol = getDeviceVolume(soundOutput, display, true);
                PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
                if(deviceDisplay!=display)
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
                }
                else
                {
                    if ((displayVol-1) >= MIN_VOLUME)
                    {
                        isValidVolume = true;
                        volume = displayVol-1;
                        std::string activeDevice = getActiveDevice(display,true);
                        soundOutput = getActualDeviceName(soundOutput);     //FIXME:
                        PM_LOG_DEBUG("volume down for soundoutput = %s", soundOutput.c_str());
                        PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());
                        if ((isValidVolume) && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeDownCallBackPA)))
                        {
                            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                            LSMessageRef(message);
                            status = true;
                        }
                        else
                        {
                            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                        }
                    }
                    else
                    {
                        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    }
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
            }
        }
        else
        {
            PM_LOG_DEBUG("volumeDown for active device");
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getActiveDevice(display,true);
                displayVol = getDeviceVolume(activeDevice,display, true);

                if ((displayVol-1) >= MIN_VOLUME)
                {
                    isValidVolume = true;
                    volume = displayVol-1;

                    activeDevice = getActualDeviceName(activeDevice);     //FIXME:

                    if ((isValidVolume) && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeDownCallBackPA)))
                    {
                        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                        LSMessageRef(message);
                        status = true;
                    }
                    else
                    {
                        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    }
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    CLSError lserror;
                    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                        lserror.Print(__FUNCTION__, __LINE__);
                }
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"audiomixer instance is nulptr");
    }
    if (false == status)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
         if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    return;
}

bool OSEMasterVolumeManager::_volumeDownCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: _volumeDownCallBackPA");
        LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
        if (!msg.parse(__FUNCTION__,sh))
            return true;

    std::string soundOutput;
    int display = DISPLAY_ONE;
    int displayId = -1;
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string callerId = LSMessageGetSenderServiceName(reply);

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    if (soundOutput == "alsa")
        soundOutput = OSEMasterVolumeManagerObj->getActiveDevice(display, true);
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("soundOutput", soundOutput);
    std::string payload;

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA::Successfully set the volume");

        int volume = OSEMasterVolumeManagerObj->getDeviceVolume(soundOutput, display, true);
        volume--;
        OSEMasterVolumeManagerObj->setDeviceVolume(soundOutput, display, true, volume);

        OSEMasterVolumeManagerObj->updateMasterVolumeInMap(soundOutput, display, volume, true);
        if (DISPLAY_TWO == display)
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
            setVolumeResponse.put("volume", volume);
            payload = setVolumeResponse.stringify();
        }
        else
        {
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
            setVolumeResponse.put("volume", volume);
            payload = setVolumeResponse.stringify();
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: Could not set volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::notifyVolumeSubscriber(const std::string &soundOutput, const int &displayId, const std::string &callerId)
{
    CLSError lserror;
    std::string reply = getVolumeInfo(soundOutput, displayId, callerId);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Notify volume subscriber error");
    }
}

void OSEMasterVolumeManager::notifyMicVolumeSubscriber(const std::string &soundInput, const int &displayId, bool subscribed)
{
    CLSError lserror;
    std::string reply = getMicVolumeInfo(soundInput, displayId, subscribed);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_MIC_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Notify mic volume subscriber error");
    }
}

std::string OSEMasterVolumeManager::getVolumeInfo(const std::string &soundOutput, const int &displayId, const std::string &callerId)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getVolumeInfo");
    pbnjson::JValue soundOutInfo = pbnjson::Object();
    pbnjson::JValue volumeStatus = pbnjson::Object();
    int volume = MIN_VOLUME;
    bool muteStatus = false;
    int display = DISPLAY_ONE;
    std::string soundDevice;

    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        display = DISPLAY_ONE;
    }
    else
    {
        display = DISPLAY_TWO;
    }
    if (soundOutput == "alsa" || soundOutput.empty())
    {
        soundDevice = getActiveDevice(display,true);
    }
    else
    {
        soundDevice = soundOutput;
    }

    volume  = getDeviceVolume(soundDevice,display,true);
    PM_LOG_DEBUG("getVolumeInfo:: soundOutput = %s ,display: %d", soundDevice.c_str(), display);

    volumeStatus = {{"muted", muteStatus},
                    {"volume", volume},
                    {"soundOutput", soundDevice},
                    {"sessionId", display}};

    soundOutInfo.put("volumeStatus", volumeStatus);
    soundOutInfo.put("returnValue", true);
    soundOutInfo.put("callerId", callerId);
    return soundOutInfo.stringify();
}

std::string OSEMasterVolumeManager::getMicVolumeInfo(const std::string &soundInput, const int &displayId, bool subscribed)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getMicVolumeInfo");
    pbnjson::JValue soundInInfo = pbnjson::Object();
    int volume = MIN_VOLUME;
    bool muteStatus = false;
    std::string soundDevice;
    int display = DISPLAY_ONE;

    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        display=DISPLAY_ONE;
    }
    else
    {
        display=DISPLAY_TWO;
    }
    if (soundInput.empty()){
        soundDevice=getActiveDevice(display,false);
        if(soundDevice.empty()){
            PM_LOG_DEBUG("Setting default to pcm_input");
            soundDevice = "pcm_input";
        }
        else
        {
            volume = getDeviceVolume(soundDevice,display,false);
            muteStatus = getDeviceMute(soundDevice, display, false);
        }
    }
    else
    {
        soundDevice = soundInput;
        volume = getDeviceVolume(soundDevice,display,false);
        muteStatus = getDeviceMute(soundDevice, display, false);
    }

    PM_LOG_DEBUG("getVolumeInfo:: soundInput = %s ,display: %d", soundDevice.c_str(), display);

    soundInInfo.put("returnValue", true);
    soundInInfo.put("subscribed", subscribed);
    soundInInfo.put("soundInput", soundDevice);
    soundInInfo.put("volume", volume);
    soundInInfo.put("muteStatus", muteStatus);
    return soundInInfo.stringify();
}

void OSEMasterVolumeManager::setSoundOutputInfo(utils::mapSoundDevicesInfo soundOutputInfo)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager::setSoundOutputInfo");
    for (auto it:soundOutputInfo)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s",it.first.c_str());
        int displayId = getDisplayId(it.first);
        for (auto it2: it.second)
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s",it2.c_str());
            deviceDetail newdevice(true);
            newdevice.display = getDisplayId(it.first);
            deviceDetailMap.insert(std::pair<std::string,deviceDetail>{it2,newdevice});
        }
    }
}

void OSEMasterVolumeManager::setSoundInputInfo(utils::mapSoundDevicesInfo soundInputInfo)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager::setSoundInputInfo");
    for (auto it:soundInputInfo)
    {
        int displayId = getDisplayId(it.first);
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s",it.first.c_str());
        for (auto it2: it.second)
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s",it2.c_str());
            deviceDetail newdevice(false);
            newdevice.display = getDisplayId(it.first);
            deviceDetailMap.insert(std::pair<std::string,deviceDetail>{it2,newdevice});
        }
    }
}

int OSEMasterVolumeManager::getDisplayId(const std::string &displayName)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getDisplayId:%s", displayName.c_str());
    int displayId = -1;

    if ("display1" == displayName)
        displayId = 0;
    else if ("display2" == displayName)
        displayId = 1;

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "returning displayId:%d", displayId);
    return displayId;
}


void OSEMasterVolumeManager::eventMasterVolumeStatus()
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "eventMasterVolumeStatus: setMuteStatus and setVolume");
}

void OSEMasterVolumeManager::eventActiveDeviceInfo(const std::string deviceName,\
    const std::string& display, const bool& isConnected, const bool& isOutput, const bool& isActive)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "eventActiveDeviceInfo deviceName:%s display:%s isConnected:%d isOutput:%d", \
        deviceName.c_str(), display.c_str(), (int)isConnected, (int)isOutput);
    std::string device = getMappedName(deviceName);     //FIXME:
    setActiveStatus(device, getDisplayId(display), isOutput, isActive);
}

void OSEMasterVolumeManager::eventResponseSoundOutputDeviceInfo(utils::mapSoundDevicesInfo soundOutputInfo)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventResponseSoundOutputDeviceInfo");

    setSoundOutputInfo(soundOutputInfo);

}

void OSEMasterVolumeManager::requestInternalDevices()
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"requestInternalDevices");
    events::EVENT_REQUEST_INTERNAL_DEVICES_INFO_T stEventRequestInternalDevices;
    stEventRequestInternalDevices.eventName = utils::eEventRequestInternalDevices;
    stEventRequestInternalDevices.func = [&](std::list<std::string>a, std::list<std::string>b)->void
                                            {
                                                mInternalInputDeviceList = a;
                                                mInternalOutputDeviceList = b;
                                            };
    ModuleManager::getModuleManagerInstance()->publishModuleEvent((events::EVENTS_T*)&stEventRequestInternalDevices);
}

void OSEMasterVolumeManager::requestSoundOutputDeviceInfo()
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"requestSoundOutputDeviceInfo");
    events::EVENT_REQUEST_SOUNDOUTPUT_INFO_T stEventRequestSoundOutputDeviceInfo;
    stEventRequestSoundOutputDeviceInfo.eventName = utils::eEventRequestSoundOutputDeviceInfo;
    ModuleManager::getModuleManagerInstance()->publishModuleEvent((events::EVENTS_T*)&stEventRequestSoundOutputDeviceInfo);

}

void OSEMasterVolumeManager::requestSoundInputDeviceInfo()
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"requestSoundInputDeviceInfo");

    events::EVENT_REQUEST_SOUNDOUTPUT_INFO_T stEventRequestSoundInputDeviceInfo;
    stEventRequestSoundInputDeviceInfo.eventName = utils::eEventRequestSoundInputDeviceInfo;
    ModuleManager::getModuleManagerInstance()->publishModuleEvent((events::EVENTS_T*)&stEventRequestSoundInputDeviceInfo);
}

void OSEMasterVolumeManager::eventResponseSoundInputDeviceInfo(utils::mapSoundDevicesInfo soundInputInfo)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventResponseSoundInputDeviceInfo");
    setSoundInputInfo(soundInputInfo);
}

void OSEMasterVolumeManager::eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "OSEMasterVolumeManager::eventMixerStatus mixerStatus:%d mixerType:%d", (int)mixerStatus, (int)mixerType);
    if (utils::ePulseMixer == mixerType && mixerStatus == true)
    {
        requestInternalDevices();
        requestSoundInputDeviceInfo();
        requestSoundOutputDeviceInfo();
    }
}

int OSEMasterVolumeManager::getVolumeFromDB(std::string deviceName, bool isOutput, bool isInternal, bool &isFound)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "getVolumeFromDB deviceName :%s, isOutput : %d, isInternal %d",deviceName.c_str(), isOutput, isInternal);
    int volume = 100;
    isFound = false;

    std::list<deviceInfo> &deviceList = getDBListRef(isInternal, isOutput);
    if (isInternal)
    {
        for (const auto it: deviceList)
        {
            if(it.deviceName == deviceName)
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Found deivce in DB");
                volume = it.volume;
                isFound = true;
                break;
            }
        }
    }
    else
    {
        for (const auto it: deviceList)
        {
            if(it.deviceNameDetail == deviceName)
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Found deivce in DB");
                volume = it.volume;
                isFound = true;
                break;
            }
        }
    }
    return volume;
}

std::list<deviceInfo>& OSEMasterVolumeManager::getDBListRef(bool isInternal, bool isOutput)
{

    if (isInternal)
    {
        if(isOutput)
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getDBListRef return internal output list");
            return mIntOutputDeviceListDB;
        }
        else
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getDBListRef return internal input list");
            return mIntInputDeviceListDB;
        }
    }
    else
    {
        if(isOutput)
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getDBListRef return external output list");
            return mExtOutputDeviceListDB;
        }
        else
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getDBListRef return external input list");
            return mExtInputDeviceListDB;
        }
    }
}

void OSEMasterVolumeManager::updateConnStatusAndReorder(std::string deviceName, bool isOutput, bool isInternal, bool isConnected)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "deviceName :%s, isOutput : %d, isInternal %d",deviceName.c_str(), isOutput, isInternal);

    if (isConnected)
    {
        if(isInternal)
        {
            std::list<deviceInfo> &deviceInfoList = getDBListRef(isInternal,isOutput);

            for (auto &it:deviceInfoList)
            {
                if (deviceName == it.deviceName)
                {
                    it.connected = true;
                    break;
                }
            }
        }
        else
        {
            std::list<deviceInfo> &deviceInfoList = getDBListRef(isInternal,isOutput);
            //The device is present in list. we need to bring the connected device to the front of the list
            //and update connection status
            for(auto it = deviceInfoList.begin();it != deviceInfoList.end(); it++)
            {
                if(it->deviceNameDetail == deviceName)
                {
                    it->connected = true;
                    //move to front of the list.
                    deviceInfoList.splice(deviceInfoList.begin(), deviceInfoList,it);
                }
            }
        }
    }
    else
    {
        //disconnected.
        if(isInternal)
        {
            std::list<deviceInfo> &deviceInfoList = getDBListRef(isInternal,isOutput);
            for (auto &it:deviceInfoList)
            {
                if (deviceName == it.deviceName)
                {
                    it.connected = false;
                    break;
                }
            }
        }
        else
        {
            std::list<deviceInfo> &deviceInfoList = getDBListRef(isInternal,isOutput);
            auto iter1=deviceInfoList.begin(),iter2=deviceInfoList.begin();
            //find the iterator for the currently disconnected device
            for(auto it = deviceInfoList.begin();it != deviceInfoList.end(); it++)
            {
                if(it->deviceNameDetail == deviceName)
                {
                    iter1 = it;
                    break;
                }
            }
            //find the last of connected devices
            iter2 = std::find_if(deviceInfoList.begin(), deviceInfoList.end(), [](deviceInfo& i){
                return i.connected == false;
            });
            if(iter2 == deviceInfoList.end())
            {
                //no other connected devices. make the connected field as false
                iter1->connected = false;
                deviceInfoList.splice(iter2, deviceInfoList,iter1);
            }
            else
            {
                //move to above the first disconnctected device present in list
                iter1->connected = false;
                deviceInfoList.splice(iter2, deviceInfoList,iter1);
            }
        }
    }
}

void OSEMasterVolumeManager::addNewItemToDB(std::string deviceName, std::string deviceNameDetail, int volume, bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "%s", __FUNCTION__);
    //this is only applicable for external devices
    //because internal devices details should be always present
    deviceInfo newItem;
    newItem.connected = true;
    newItem.deviceName = deviceName;
    newItem.deviceNameDetail = deviceNameDetail;
    newItem.volume = volume;
    std::list<deviceInfo>deviceInfoList;
    if (isOutput)
    {
        if (mExtOutputDeviceListDB.size() == STORED_VOLUME_SIZE)
        {
            //remove the last entry
            mExtOutputDeviceListDB.pop_back();
        }
        mExtOutputDeviceListDB.push_front(newItem);
    }
    else
    {
        if (mExtInputDeviceListDB.size() == STORED_VOLUME_SIZE)
        {
            //remove the last entry
            mExtInputDeviceListDB.pop_back();
        }
        mExtInputDeviceListDB.push_front(newItem);
    }
    sendDataToDB();
}

bool OSEMasterVolumeManager::DBSetVoulumeCallbackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "DBSetVoulumeCallbackPA");
    if (ctx)
    {
        masterVolumeCallbackDetails *envelope = (masterVolumeCallbackDetails*)ctx;
        OSEMasterVolumeManager *obj = (OSEMasterVolumeManager*)envelope->context;
        int volume = envelope->volume;
        std::string device(envelope->deviceName);
        if (envelope->isOutput)
        {
            obj->notifyVolumeSubscriber(device, (device=="pcm_output1")?1:0, "internal");
        }
        else
        {
            obj->notifyMicVolumeSubscriber(device, 0, true);
        }
    }
    return true;
}

void OSEMasterVolumeManager::deviceConnectOp(std::string deviceName, std::string deviceNameDetail,bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "deviceConnectOp");
    bool found = false;
    if (isInternalDevice(deviceName, true))
    {
        int volume = getVolumeFromDB(deviceName, isOutput, true, found);
        if (found) //initial volume is found;
        {
            masterVolumeCallbackDetails *envelope =  new (std::nothrow)masterVolumeCallbackDetails;
            if (!envelope)
            {
                //fail!!!
            }
            else
            {
                //call mobjaudiomixer setvolume, with callback to notify master get volume
                envelope->volume = volume;
                strncpy(envelope->deviceName, deviceName.c_str(), 50);
                envelope->context = this;
                envelope->isOutput = 1;
                deviceName = getActualDeviceName(deviceName);
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"call mobjaudiomixer setvolume, with callback to notify master get volume,%s:%d",\
                    deviceName.c_str(),volume);
                if (isOutput)
                    AudioMixer::getAudioMixerInstance()->setVolume(deviceName.c_str(), volume, nullptr, nullptr, envelope, DBSetVoulumeCallbackPA);
                else
                    AudioMixer::getAudioMixerInstance()->setMicVolume(deviceName.c_str(), volume, nullptr, nullptr, envelope, DBSetVoulumeCallbackPA);
            }
        }
        setDeviceVolume(deviceName,0,isOutput,volume);
    }
    else
    {
        //external devices will be dynamic list basded on device name details
        int volume = getVolumeFromDB(deviceNameDetail, isOutput, false, found);
        if (found)
        {
            //update the connected status of deviceNameDetail to true
            //reorder the list to move the connected device to front.
            updateConnStatusAndReorder(deviceNameDetail,isOutput,false,true);
            sendDataToDB();

            //call mobjaudiomixer setvolume, with callback to notify master get volume
            //update DB with new device order
        }
        else
        {
            //New device connected, add to list.
            volume = DEFAULT_INITIAL_VOLUME;
            addNewItemToDB(deviceName, deviceNameDetail, volume, isOutput);
            //update DB with new device order
        }
        //call mobjaudiomixer setvolume, with callback to notify master get volume
        masterVolumeCallbackDetails *envelope =  new (std::nothrow)masterVolumeCallbackDetails;
        if (!envelope)
        {
            //fail!!!
        }
        else
        {
            //call mobjaudiomixer setvolume, with callback to notify master get volume
            envelope->volume = volume;
            strncpy(envelope->deviceName, deviceName.c_str(), 50);
            envelope->context = this;
            envelope->isOutput = isOutput;
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"//TODO : call mobjaudiomixer setvolume, with callback to notify master get volume");
            if (isOutput)
                AudioMixer::getAudioMixerInstance()->setVolume(deviceName.c_str(), volume, nullptr, nullptr, envelope, DBSetVoulumeCallbackPA);
            else
                AudioMixer::getAudioMixerInstance()->setMicVolume(deviceName.c_str(), volume, nullptr, nullptr, envelope, DBSetVoulumeCallbackPA);
        }
        setDeviceVolume(deviceName, 0, isOutput, volume);
    }
    printDb();
}

void OSEMasterVolumeManager::deviceDisconnectOp(std::string deviceName, std::string deviceNameDetail,bool isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "deviceDisconnectOp");
    bool found = false;
    if (isInternalDevice(deviceName, true))
    {
        //Nothing to do
    }
    else
    {
        //external devices will be dynamic list based on device name details
        //we will not get devicename detail in disconnect event, hence using the stored value.
        if (deviceNameDetail == "")
            deviceNameDetail = getDeviceNameDetail(deviceName, 0, isOutput);
            //deviceNameDetail = deviceNameMap[deviceName];
        //To check if the entry exist in DB struct. ideally should be present
        int volume = getVolumeFromDB(deviceNameDetail, isOutput, false, found);
        if (found)
        {
            //reorder the list to move the disconnected device to end of connected list.
            //update DB with new device order
            updateConnStatusAndReorder(deviceNameDetail,isOutput,false,false);
            sendDataToDB();
        }
        else
        {
            // ERRORR!!!!! it should be found.
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"error!!!");
        }
    }
    printDb();
}

void OSEMasterVolumeManager::eventDeviceConnectionStatus(const std::string &deviceName , const std::string &deviceNameDetail, const std::string &deviceIcon, \
    utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "eventDeviceConnectionStatus with deviceName:%s deviceStatus:%d mixerType:%d",\
        deviceName.c_str(), (int)deviceStatus, (int)mixerType);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "eventDeviceConnectionStatus deviceNameDetail : %s, deviceIcon: %s isOutput : %d",deviceNameDetail.c_str(), deviceIcon.c_str(), (int)isOutput);

    if (deviceStatus == utils::eDeviceConnected)
    {
        bool found;
        std::string modDeviceName;
        if (deviceName.find("bluez") != deviceName.npos)
        {
            mBluetoothName = deviceName;
        }
        //to set the device as connected
        modDeviceName = getMappedName(deviceName);
        setDeviceNameDetail(modDeviceName, 0, isOutput, deviceNameDetail);
        setConnStatus(modDeviceName, 0, isOutput, true);
        if (mCacheRead)
        {
            deviceConnectOp(modDeviceName, deviceNameDetail, isOutput);
        }
        else
        {
            connectedDevices newDev;
            newDev.deviceName =modDeviceName;
            newDev.deviceNameDetail = deviceNameDetail;
            newDev.isOutput = isOutput;
            mConnectedDevicesList.push_back(newDev);
        }
    }
    else if ((deviceStatus == utils::eDeviceDisconnected))
    {

        bool found = false;
        std::string modDeviceName;
        modDeviceName = getMappedName(deviceName);
        if (mCacheRead)
        {
            deviceDisconnectOp(modDeviceName, deviceNameDetail, isOutput);
        }
        else
        {
            //ideally shouldnt happen
            if (mConnectedDevicesList.size()>0)
            {
                for (auto it=mConnectedDevicesList.begin(); it != mConnectedDevicesList.end();it++)
                {
                    if(it->deviceName == modDeviceName)
                    {
                        it = mConnectedDevicesList.erase(it);
                        break;
                    }
                }
            }
        }
        //to set the device as disconnected
        setDeviceNameDetail(modDeviceName, 0, isOutput, deviceNameDetail);
        setConnStatus(modDeviceName, 0, isOutput, false);

    }
}

bool OSEMasterVolumeManager::isInternalDevice(std::string device, bool isOutput)
{
    bool retVal = false;
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"isInternalDevice");
    if (isOutput)
    {
        if (std::find(mInternalOutputDeviceList.begin(),mInternalOutputDeviceList.end(),device) != \
            mInternalOutputDeviceList.end())
        {
             retVal = true;
        }
    }
    else
    {
        if (std::find(mInternalInputDeviceList.begin(),mInternalInputDeviceList.end(),device) != \
            mInternalInputDeviceList.end())
        {
             retVal = true;
        }
    }
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"isInternalDevice returns : %d", retVal);
    return retVal;
}

void OSEMasterVolumeManager::eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "Got OSEMasterVolumeManager server status event %d : %d", serviceName, connected);
    if (connected && serviceName == eSettingsService2)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
            "Settingsservice service is connected");
        std::string payload = "{\"category\":\"sound\",\"keys\":[\"soundOutputList\",\"soundInputList\"]}";
        bool result = false;
        CLSError lserror;
        LSHandle *sh = GetPalmService();
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
            "payload = %s",payload.c_str());
        result = LSCallOneReply (sh, GETSETTINGS, payload.c_str(), VolumeFromSettingService, this, nullptr, &lserror);
    }
}

bool OSEMasterVolumeManager::VolumeFromSettingService(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "got VolumeFromSettingService");
    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                                                 REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;
    OSEMasterVolumeManager *obj = (OSEMasterVolumeManager*) ctx;
    pbnjson::JValue settingsObj;
    settingsObj = msg.get()["settings"];
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "settings = %s", settingsObj.stringify().c_str());
    obj->readInitialVolume(settingsObj);
    return true;
}

void OSEMasterVolumeManager::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {
        case utils::eEventServerStatusSubscription:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventServerStatusSubscription");
            events::EVENT_SERVER_STATUS_INFO_T *serverStatusInfoEvent = (events::EVENT_SERVER_STATUS_INFO_T*)event;
            eventServerStatusInfo(serverStatusInfoEvent->serviceName, serverStatusInfoEvent->connectionStatus);
        }
        break;
        case utils::eEventMasterVolumeStatus:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventMasterVolumeStatus");
            events::EVENT_MASTER_VOLUME_STATUS_T *masterVolumeStatusEvent = (events::EVENT_MASTER_VOLUME_STATUS_T*)event;
            eventMasterVolumeStatus();
        }
        break;
        case utils::eEventActiveDeviceInfo:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventActiveDeviceInfo");
            events::EVENT_ACTIVE_DEVICE_INFO_T *stActiveDeviceInfo = (events::EVENT_ACTIVE_DEVICE_INFO_T*)event;
            eventActiveDeviceInfo(stActiveDeviceInfo->deviceName, stActiveDeviceInfo->display, stActiveDeviceInfo->isConnected,
                stActiveDeviceInfo->isOutput, stActiveDeviceInfo->isActive);
        }
        break;
        case utils::eEventResponseSoundOutputDeviceInfo:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventResponseSoundOutputDeviceInfo");
            events::EVENT_RESPONSE_SOUNDOUTPUT_INFO_T *stSoundOutputDeviceInfo = (events::EVENT_RESPONSE_SOUNDOUTPUT_INFO_T*)event;
            eventResponseSoundOutputDeviceInfo(stSoundOutputDeviceInfo->soundOutputInfo);
        }
        break;
        case utils::eEventResponseSoundInputDeviceInfo:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventResponseSoundInputDeviceInfo");
            events::EVENT_RESPONSE_SOUNDINPUT_INFO_T *stSoundInputDeviceInfo = (events::EVENT_RESPONSE_SOUNDINPUT_INFO_T*)event;
            eventResponseSoundInputDeviceInfo(stSoundInputDeviceInfo->soundInputInfo);
        }
        break;
        case utils::eEventMixerStatus:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                "handleEvent : eEventMixerStatus");
            events::EVENT_MIXER_STATUS_T *stEventMixerStatus = (events::EVENT_MIXER_STATUS_T*)event;
            eventMixerStatus( stEventMixerStatus->mixerStatus, stEventMixerStatus->mixerType);
        }
        break;
        case  utils::eEventDeviceConnectionStatus:
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                "handleEvent : eEventDeviceConnectionStatus");
            events::EVENT_DEVICE_CONNECTION_STATUS_T * stDeviceConnectionStatus = (events::EVENT_DEVICE_CONNECTION_STATUS_T*)event;
            eventDeviceConnectionStatus(stDeviceConnectionStatus->devicename, stDeviceConnectionStatus->deviceNameDetail, stDeviceConnectionStatus->deviceIcon, stDeviceConnectionStatus->deviceStatus, stDeviceConnectionStatus->mixerType, stDeviceConnectionStatus->isOutput);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}