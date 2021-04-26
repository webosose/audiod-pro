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


#include "audioRouter.h"

bool AudioRouter::mIsObjRegistered = AudioRouter::RegisterObject();

void  AudioRouter::eventSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType)
{
    //TBD
}

void AudioRouter::eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "eventMixerStatus with mixerStatus:%d mixerType:%d",\
        (int)mixerStatus, (int)mixerType);
    if (!mixerStatus && (utils::ePulseMixer == mixerType))
    {
        for (auto& it : mSoundOutputInfo)
        {
            for (auto& deviceInfo : it.second)
            {
                deviceInfo.isConnected = false;
                deviceInfo.activeStatus = false;
            }
        }
        for (auto& it : mSoundInputInfo)
        {
            for (auto& deviceInfo : it.second)
            {
                deviceInfo.isConnected = false;
                deviceInfo.activeStatus = false;
            }
        }
    }
}

void AudioRouter::eventDeviceConnectionStatus(const std::string &deviceName,\
    utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "eventDeviceConnectionStatus with deviceName:%s deviceStatus:%d mixerType:%d",\
        deviceName.c_str(), (int)deviceStatus, (int)mixerType);
    std::string actualDeviceName = deviceName;
    if (deviceName.find(BLUETOOTH_SINK_IDENTIFIER) != std::string::npos)
    {
        for (auto const& items : mMapBTDeviceInfo)
        {
            if (actualDeviceName.find(items.first) != std::string::npos)
            {
                if (BT_DISPLAY_ONE == items.second)
                    actualDeviceName = BLUETOOTH_ONE;
                else if (BT_DISPLAY_TWO == items.second)
                    actualDeviceName = BLUETOOTH_TWO;
            }
            else
                PM_LOG_WARNING(MSGID_AUDIOROUTER, INIT_KVCOUNT, "deviceName not found in:%s", items.first.c_str());
        }
        PM_LOG_DEBUG("remapped soundoutput:%s", actualDeviceName.c_str());
    }
    else
        PM_LOG_DEBUG("device other than BT is connected");

    for (const auto& items : mSoundOutputInfo)
    {
        for (const auto &deviceInfo : items.second)
        {
            if (actualDeviceName == deviceInfo.deviceName)
            {
                if (utils::eDeviceConnected == deviceStatus)
                    setOutputDeviceRouting(actualDeviceName,\
                        deviceInfo.priority, items.first, mixerType);
                else if(utils::eDeviceDisconnected == deviceStatus)
                    resetOutputDeviceRouting(actualDeviceName,\
                        deviceInfo.priority,  items.first, mixerType);
                return;
            }
        }
    }

    for (const auto& items : mSoundInputInfo)
    {
        for (const auto &deviceInfo : items.second)
        {
            if (actualDeviceName == deviceInfo.deviceName)
            {
                if (utils::eDeviceConnected == deviceStatus)
                    setInputDeviceRouting(actualDeviceName,\
                        deviceInfo.priority, items.first, mixerType);
                else if(utils::eDeviceDisconnected == deviceStatus)
                    resetInputDeviceRouting(actualDeviceName,\
                        deviceInfo.priority, items.first, mixerType);
                return;

            }
        }
    }
}

void AudioRouter::eventBTDeviceDisplayInfo(const bool &connectionStatus, const std::string &deviceAddress, const int &displayId)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "eventBTDeviceDisplayInfo with connectionStatus:%d deviceAddress:%s displayId:%d",\
        (int)connectionStatus, deviceAddress.c_str(), displayId);
    std::string btDeviceAddress = deviceAddress;
    std::transform(btDeviceAddress.begin(), btDeviceAddress.end(), btDeviceAddress.begin(), ::toupper);
    std::replace(btDeviceAddress.begin(), btDeviceAddress.end(), ':', '_');
    if (connectionStatus)
    {
        utils::itMapBTDeviceInfo it = mMapBTDeviceInfo.find(btDeviceAddress);
        if (it != mMapBTDeviceInfo.end())
            it->second = displayId;
        else
            mMapBTDeviceInfo[btDeviceAddress] = displayId;
    }
    else
    {
        utils::itMapBTDeviceInfo it = mMapBTDeviceInfo.find(btDeviceAddress);
        if (it != mMapBTDeviceInfo.end())
            mMapBTDeviceInfo.erase(it);
    }
}

void AudioRouter::eventSinkPolicyInfo(const pbnjson::JValue& sinkPolicyInfo)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "eventSinkPolicyInfo");
    if (!sinkPolicyInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "sinkPolicyInfo is not an array");
    }
    else
    {
        PM_LOG_DEBUG("sinkPolicyInfo is an array");
        std::list<EVirtualAudioSink> sinkList;
        utils::SINK_ROUTING_INFO_T outputRoutingInfo;
        mMapSinkRoutingInfo["display1"] = outputRoutingInfo;
        utils::itMapSinkRoutingInfo it;
        for (const pbnjson::JValue& elements : sinkPolicyInfo.items())
        {
            std::string streamType;
            std::string category;
            if ((elements["category"].asString(category) == CONV_OK) && \
                (elements["streamType"].asString(streamType) == CONV_OK))
            {
                it = mMapSinkRoutingInfo.find("display1");
                if (it != mMapSinkRoutingInfo.end())
                    it->second.sinkList.push_back(getSinkByName(streamType.c_str()));
            }
        }
        it = mMapSinkRoutingInfo.find("display1");      ////only one category, since only 1 set of displays are supported
        if (it != mMapSinkRoutingInfo.end())
        {
            it->second.startSink = it->second.sinkList.front();
            it->second.endSink = it->second.sinkList.back();
        }
    }
    for (auto const& items: mMapSinkRoutingInfo)
    {
        PM_LOG_DEBUG("SinkPolicyInfo: display:%s", items.first.c_str());
        PM_LOG_DEBUG("size:%d", (int)items.second.sinkList.size());
        PM_LOG_DEBUG("start sink id:%d name:%s  end sink id:%d name:%s ",\
            (int)items.second.startSink, virtualSinkName(items.second.startSink),\
            (int)items.second.endSink, virtualSinkName(items.second.endSink));
    }
}

void AudioRouter::eventSourcePolicyInfo(const pbnjson::JValue& sourcePolicyInfo)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "eventSourcePolicyInfo");
    if (!sourcePolicyInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "sourcePolicyInfo is not an array");
    }
    else
    {
        PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "sourcePolicyInfo is an array");
        std::list<EVirtualSource> sourceList;
        utils::SOURCE_ROUTING_INFO_T inputRoutingInfo;
        mMapSourceRoutingInfo["display1"] = inputRoutingInfo;
        utils::itMapSourceRoutingInfo it;
        for (const pbnjson::JValue& elements : sourcePolicyInfo.items())
        {
            std::string streamType;
            std::string category;
            if ((elements["category"].asString(category) == CONV_OK) && \
                (elements["streamType"].asString(streamType) == CONV_OK))
            {
                it = mMapSourceRoutingInfo.find("display1");            //only one category, since only 1 set of displays are supported
                if (it != mMapSourceRoutingInfo.end())
                    it->second.sourceList.push_back(getSourceByName(streamType.c_str()));
            }
        }
        it = mMapSourceRoutingInfo.find("display1");
        if (it != mMapSourceRoutingInfo.end())
        {
            it->second.startSource = it->second.sourceList.front();
            it->second.endSource = it->second.sourceList.back();
        }

    }
    for (auto const& items: mMapSourceRoutingInfo)
    {
        PM_LOG_DEBUG("SourcePolicyInfo: display:%s", items.first.c_str());
        PM_LOG_DEBUG("size:%d", (int)items.second.sourceList.size());
        PM_LOG_DEBUG("start source id:%d name:%s  end sourceId id:%d name:%s ",\
            (int)items.second.startSource, virtualSourceName(items.second.startSource),\
            (int)items.second.endSource, virtualSourceName(items.second.endSource));
    }
}

void AudioRouter::setOutputDeviceRouting(const std::string &deviceName, const int &priority,\
    const std::string &display, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "setOutputDeviceRouting deviceName:%s priority:%d display:%s mixerType:%d",\
        deviceName.c_str(), priority, display.c_str(), (int)mixerType);
    std::string activeDevice = getActiveDevice(display, true);
    bool setStatus = false;
    if (priority < getPriority(display, activeDevice, true))
    {
        if (nullptr != mObjAudioMixer)
        {
            utils::SINK_ROUTING_INFO_T sinkInfo = getSinkRoutingInfo(display);
            std::string outputDevice = getActualOutputDevice(deviceName);
            if (mObjAudioMixer->setSoundOutputOnRange(sinkInfo.startSink, sinkInfo.endSink, outputDevice.c_str()))
            {
                updateDeviceStatus(display, deviceName, true, true, true);
                updateDeviceStatus(display, activeDevice, true, false, true);
                setStatus = true;
            }
        }
    }
    if (!setStatus)
        updateDeviceStatus(display, deviceName, true, false, true);
}

void AudioRouter::resetOutputDeviceRouting(const std::string &deviceName, const int &priority,\
    const std::string &display, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "resetOutputDeviceRouting deviceName:%s mixerType:%d",\
        deviceName.c_str(), (int)mixerType);
    updateDeviceStatus(display, deviceName, false, false, true);
    if (nullptr != mObjAudioMixer)
    {
        std::string activeDevice = getPriorityDevice(display, true);
        utils::SINK_ROUTING_INFO_T sinkInfo = getSinkRoutingInfo(display);
        if (!activeDevice.empty())
        {
            std::string outputDevice = getActualOutputDevice(activeDevice);
            if (mObjAudioMixer->setSoundOutputOnRange(sinkInfo.startSink, sinkInfo.endSink, outputDevice.c_str()))
                updateDeviceStatus(display, activeDevice, true, true, true);
        }
        else
            mObjAudioMixer->setDefaultSinkRouting(sinkInfo.startSink, sinkInfo.endSink);
    }
}


void AudioRouter::setInputDeviceRouting(const std::string &deviceName, const int &priority,\
    const std::string &display, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "setInputDeviceRouting deviceName:%s priority:%d display:%s mixerType:%d",\
        deviceName.c_str(), priority, display.c_str(), (int)mixerType);
    std::string activeDevice = getActiveDevice(display, false);
    bool setStatus = false;
    if (priority < getPriority(display, activeDevice, false))
    {
        if (nullptr != mObjAudioMixer)
        {
            utils::SOURCE_ROUTING_INFO_T sourceInfo = getSourceRoutingInfo(display);
            if (mObjAudioMixer->setSoundInputOnRange(sourceInfo.startSource, sourceInfo.endSource, deviceName.c_str()))
            {
                updateDeviceStatus(display, deviceName, true, true, false);
                updateDeviceStatus(display, activeDevice, true, false, false);
                setStatus = true;
            }
        }
    }
    if (!setStatus)
        updateDeviceStatus(display, deviceName, true, false, false);
}


void AudioRouter::resetInputDeviceRouting(const std::string &deviceName, const int &priority,\
    const std::string &display, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "resetInputDeviceRouting deviceName:%s priority:%d display:%s mixerType:%d",\
        deviceName.c_str(), priority, display.c_str(), (int)mixerType);
    updateDeviceStatus(display, deviceName, false, false, false);
    if (nullptr != mObjAudioMixer)
    {
        std::string activeDevice = getPriorityDevice(display, false);
        utils::SOURCE_ROUTING_INFO_T sourceInfo = getSourceRoutingInfo(display);
        if (!activeDevice.empty())
        {
            if (mObjAudioMixer->setSoundInputOnRange(sourceInfo.startSource, sourceInfo.endSource, activeDevice.c_str()))
                updateDeviceStatus(display, activeDevice, true, true, false);
        }
        else
        {
            mObjAudioMixer->setDefaultSourceRouting(sourceInfo.startSource, sourceInfo.endSource);
            notifyGetSoundInput("", display);
        }
    }
}


std::string AudioRouter::getActualOutputDevice(const std::string &deviceName)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "getActualOutputDevice deviceName:%s", deviceName.c_str());
    int deviceId = -1;
    bool isMapped = false;
    std::string actualDeviceName = deviceName;
    if (BLUETOOTH_ONE == actualDeviceName)
        deviceId = BT_DISPLAY_ONE;
    else if(BLUETOOTH_TWO == actualDeviceName)
        deviceId = BT_DISPLAY_TWO;
    for (auto const& items : mMapBTDeviceInfo)
    {
        if (deviceId == items.second)
        {
            actualDeviceName = items.first;
            isMapped = true;
        }
        else if (deviceId == items.second)
        {
            actualDeviceName = items.first;
            isMapped = true;
        }
    }
    if (isMapped)
    {
        std::string mappedName = BLUETOOTH_SINK_IDENTIFIER;
        mappedName.append(actualDeviceName);
        return mappedName;
    }
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "actual devcie is:%s", actualDeviceName.c_str());
    return actualDeviceName;
}


utils::SINK_ROUTING_INFO_T AudioRouter::getSinkRoutingInfo(const std::string &display)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "getSinkRoutingInfo display:%s", display.c_str());
    auto it  = mMapSinkRoutingInfo.find(display);
    utils::SINK_ROUTING_INFO_T sinkInfo;
    if (it != mMapSinkRoutingInfo.end())
        sinkInfo = it->second;
    return sinkInfo;
}

utils::SOURCE_ROUTING_INFO_T AudioRouter::getSourceRoutingInfo(const std::string &display)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "getSourceRoutingInfo display:%s", display.c_str());
    utils::SOURCE_ROUTING_INFO_T sourceInfo;
    auto it  = mMapSourceRoutingInfo.find(display);
    if (it != mMapSourceRoutingInfo.end())
        sourceInfo = it->second;
    return sourceInfo;
}

int AudioRouter::getPriority(const std::string& display, const std::string &deviceName, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "getPriority display:%s deviceName:%s",\
        display.c_str(), deviceName.c_str());
    int priority = INT_MAX;
    if (isOutput)
    {
        auto it = mSoundOutputInfo.find(display);
        if (it != mSoundOutputInfo.end())
        {
            for (const auto &deviceInfo : it->second)
            {
                if (deviceName == deviceInfo.deviceName)
                    priority = deviceInfo.priority;
            }
        }
    }
    else
    {
        auto it = mSoundInputInfo.find(display);
        if (it != mSoundInputInfo.end())
        {
            for (const auto &deviceInfo : it->second)
            {
                if (deviceName == deviceInfo.deviceName)
                    priority = deviceInfo.priority;
            }
        }
    }
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "priority is:%d", priority);
    return priority;
}

std::string AudioRouter::getPriorityDevice(const std::string& display, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "getPriorityDevice display:%s isOutput:%d", display.c_str(), (int)isOutput);
    std::string deviceName;
    int priority = INT_MAX;
    if (isOutput)
    {
        auto it = mSoundOutputInfo.find(display);
        if (it != mSoundOutputInfo.end())
        {
            for (const auto &deviceInfo : it->second)
            {
                if (deviceInfo.isConnected)
                {
                    if (deviceInfo.priority < priority)
                    {
                        deviceName = deviceInfo.deviceName;
                        priority = deviceInfo.priority;
                    }
                }
            }
        }
    }
    else
    {
        auto it = mSoundInputInfo.find(display);
        if (it != mSoundInputInfo.end())
        {
            for (const auto &deviceInfo : it->second)
            {
                if (deviceInfo.isConnected)
                {
                    if (deviceInfo.priority < priority)
                    {
                        deviceName = deviceInfo.deviceName;
                        priority = deviceInfo.priority;
                    }
                }
            }
        }
    }
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "priority device is:%s", deviceName.c_str());
    return deviceName;
}

std::string AudioRouter::getActiveDevice(const std::string& display, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "getActiveDevice display:%s", display.c_str());
    if (isOutput)
    {
        auto it = mSoundOutputInfo.find(display);
        if (it != mSoundOutputInfo.end())
        {
            for (const auto &deviceInfo : it->second)
            {
                if (deviceInfo.activeStatus)
                {
                    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                        "active device is:%s", deviceInfo.deviceName.c_str());
                    return deviceInfo.deviceName;
                }
            }
        }
    }
    else
    {
        auto it = mSoundInputInfo.find(display);
        if (it != mSoundInputInfo.end())
        {
            for (const auto &deviceInfo : it->second)
            {
                if (deviceInfo.activeStatus)
                {
                    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                        "active device is:%s", deviceInfo.deviceName.c_str());
                    return deviceInfo.deviceName;
                }
            }
        }
    }
    return "";
}


void AudioRouter::updateDeviceStatus(const std::string& display, const std::string& deviceName,
    const bool& isConnected, bool const& isActive, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "updateDeviceStatus display:%s deviceName:%s isConnected:%d isActive:%d",\
        display.c_str(), deviceName.c_str(), (int)isConnected, (int)isActive);
    bool isUpdated = false;
    if (isOutput)
    {
        auto it = mSoundOutputInfo.find(display);
        if (it != mSoundOutputInfo.end())
        {
            for (auto &deviceInfo : it->second)
            {
                if (deviceName == deviceInfo.deviceName)
                {
                    deviceInfo.isConnected = isConnected;
                    deviceInfo.activeStatus = isActive;
                    isUpdated = true;
                    break;
                }
            }
        }
        if (isActive)
        {
            notifyGetSoundoutput(deviceName, display);
        }
        if (mObjModuleManager && isUpdated)
        {
            events::EVENT_ACTIVE_DEVICE_INFO_T stEventActiveDeviceInfo;
            stEventActiveDeviceInfo.eventName = utils::eEventActiveDeviceInfo;
            stEventActiveDeviceInfo.display = display;
            stEventActiveDeviceInfo.deviceName = getActualOutputDevice(deviceName);
            stEventActiveDeviceInfo.isConnected = isConnected;
            stEventActiveDeviceInfo.isOutput = isOutput;
            mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stEventActiveDeviceInfo);
        }
            //mObjModuleManager->notifyActiveDeviceInfo(getActualOutputDevice(deviceName), display, isConnected, isOutput);
        printDeviceInfo(true);
    }
    else
    {
        auto it = mSoundInputInfo.find(display);
        if (it != mSoundInputInfo.end())
        {
            for (auto &deviceInfo : it->second)
            {
                if (deviceName == deviceInfo.deviceName)
                {
                    deviceInfo.isConnected = isConnected;
                    deviceInfo.activeStatus = isActive;
                    isUpdated = true;
                    break;
                }
            }
        }
        if (mObjModuleManager && isUpdated)
        {
            events::EVENT_ACTIVE_DEVICE_INFO_T stEventActiveDeviceInfo;
            stEventActiveDeviceInfo.eventName = utils::eEventActiveDeviceInfo;
            stEventActiveDeviceInfo.display = display;
            stEventActiveDeviceInfo.deviceName = getActualOutputDevice(deviceName);
            stEventActiveDeviceInfo.isConnected = isConnected;
            stEventActiveDeviceInfo.isOutput = isOutput;
            mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stEventActiveDeviceInfo);
        }
        if (isActive)
        {
            notifyGetSoundInput(deviceName, display);
        }
            //mObjModuleManager->notifyActiveDeviceInfo(getActualOutputDevice(deviceName), display, isConnected, isOutput);
        printDeviceInfo(false);
    }
}

void AudioRouter::notifyGetSoundInput(const std::string& soundInput, const std::string& display)
{
    CLSError lserror;
    std::string reply;

    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "notifyGetSoundInput");
    pbnjson::JValue responseObj = pbnjson::JObject();
    responseObj.put("returnValue",true);
    responseObj.put("subscribed",true);
    responseObj.put("soundInput",soundInput);
    responseObj.put("displayId",getNotificationSessionId(display));
    reply = responseObj.stringify();
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_SOUNDINPUT, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "Notify error");
    }
}

void AudioRouter::notifyGetSoundoutput(const std::string& soundoutput, const std::string& display)
{
    CLSError lserror;
    std::string reply;

    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "notifyGetSoundoutput");
    pbnjson::JValue responseObj = pbnjson::JObject();
    responseObj.put("returnValue",true);
    responseObj.put("subscribed",true);
    responseObj.put("soundOutput",soundoutput);
    responseObj.put("displayId",getNotificationSessionId(display));
    reply = responseObj.stringify();
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_SOUNDOUT, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "Notify error");
    }
}

void AudioRouter::printDeviceInfo(const bool& isOutput)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "printDeviceInfo");
    if (isOutput)
    {
        for (const auto it : mSoundOutputInfo)
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "mSoundOutputInfo: display:%s", it.first.c_str());
            for (const auto& deviceInfo : it.second)
            {
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "deviceName:%s priority:%d",\
                    deviceInfo.deviceName.c_str(), deviceInfo.priority);
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "activeStatus:%d isConnected:%d",\
                    (int)deviceInfo.activeStatus, (int)deviceInfo.isConnected);
            }
        }
    }
    else
    {
        for (const auto it : mSoundInputInfo)
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "mSoundInputInfo: display:%s", it.first.c_str());
            for (const auto& deviceInfo : it.second)
            {
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "deviceName:%s priority:%d",\
                    deviceInfo.deviceName.c_str(), deviceInfo.priority);
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "activeStatus:%d isConnected:%d",\
                    (int)deviceInfo.activeStatus, (int)deviceInfo.isConnected);
            }
        }
    }
}


void AudioRouter::readDeviceRoutingInfo()
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
        "readDeviceRoutingInfo");
    if (mObjDeviceRoutingParser)
    {
        if (mObjDeviceRoutingParser->loadDeviceRoutingJsonConfig())
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "loadDeviceRoutingJsonConfig success");
            pbnjson::JValue routingInfo = mObjDeviceRoutingParser->getDeviceRoutingInfo();
            if (!routingInfo.isArray())
            {
                PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "routingInfo is not an array");
                return;
            }
            setDeviceRoutingInfo(routingInfo);
        }
        else
            PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "Unable to load DeviceRoutingJsonConfig");
    }
    else
        PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "mObjDeviceRoutingParser is null");
}

void AudioRouter::setDeviceRoutingInfo(const pbnjson::JValue& deviceRoutingInfo)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter::eventSoundOutputInfo");
    if (!deviceRoutingInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter::deviceRoutingInfo is not an array");
        return;
    }
    pbnjson::JValue soundOutputListInfo;
    pbnjson::JValue soundInputListInfo;
    for (const auto& it : deviceRoutingInfo.items())
    {
        if (it.hasKey("soundOutputList"))
            soundOutputListInfo = it["soundOutputList"];
        else if (it.hasKey("soundInputList"))
            soundInputListInfo = it["soundInputList"];
    }

    if (!soundOutputListInfo.isArray())
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter::soundOutputList is not an array");
    else
    {
        utils::SINK_ROUTING_INFO_T outputRoutingInfo;
        for (pbnjson::JValue arrItem: soundOutputListInfo.items())
        {
            utils::DEVICE_INFO_T deviceInfo;
            std::string soundoutput;
            std::string display;
            bool adjustVolume = true;
            bool activeStatus = false;
            bool muteStatus = false;
            if (arrItem["soundOutput"].asString(soundoutput) == CONV_OK)
            {
                deviceInfo.volume = arrItem["volume"].asNumber<int>();
                deviceInfo.maxVolume = arrItem["maxVolume"].asNumber<int>();
                if (arrItem["muteStatus"].asBool(muteStatus) == CONV_OK)
                    deviceInfo.muteStatus = muteStatus;
                else
                    deviceInfo.muteStatus = false;
                if (arrItem["activeStatus"].asBool(activeStatus) == CONV_OK)
                    deviceInfo.activeStatus = activeStatus;
                else
                    deviceInfo.activeStatus = false;
                if (arrItem["adjustVolume"].asBool(adjustVolume) == CONV_OK)
                    deviceInfo.adjustVolume = adjustVolume;
                else
                    deviceInfo.adjustVolume = true;
                deviceInfo.priority = arrItem["priority"].asNumber<int>();
                deviceInfo.deviceName = soundoutput;
                if (arrItem["display"].asString(display) == CONV_OK)
                {
                    auto it = mSoundOutputInfo.find(display);
                    if (it != mSoundOutputInfo.end())
                        it->second.push_back(deviceInfo);
                    else
                    {
                        std::vector<utils::DEVICE_INFO_T> tmpDeviceInfo;
                        tmpDeviceInfo.push_back(deviceInfo);
                        mSoundOutputInfo[display] = tmpDeviceInfo;
                    }
                }
                else
                    PM_LOG_WARNING(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                        "eventSoundOutputInfo::Unable to read display parameter");
            }
        }
        for (const auto it : mSoundOutputInfo)
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "mSoundOutputInfo: display:%s", it.first.c_str());
            for (const auto& deviceInfo : it.second)
            {
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "deviceName:%s priority:%d",\
                    deviceInfo.deviceName.c_str(), deviceInfo.priority);
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "activeStatus:%d isConnected:%d",\
                    (int)deviceInfo.activeStatus, (int)deviceInfo.isConnected);
            }
        }
    }

    if (!soundInputListInfo.isArray())
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter::soundInputListInfo is not an array");
    else
    {
        utils::SINK_ROUTING_INFO_T inputRoutingInfo;
        for (pbnjson::JValue arrItem: soundInputListInfo.items())
        {
            utils::DEVICE_INFO_T deviceInfo;
            std::string soundinput;
            std::string display;
            bool adjustVolume = true;
            bool activeStatus = false;
            bool muteStatus = false;
            if (arrItem["soundInput"].asString(soundinput) == CONV_OK)
            {
                deviceInfo.volume = arrItem["volume"].asNumber<int>();
                deviceInfo.maxVolume = arrItem["maxVolume"].asNumber<int>();
                if (arrItem["muteStatus"].asBool(muteStatus) == CONV_OK)
                    deviceInfo.muteStatus = muteStatus;
                else
                    deviceInfo.muteStatus = false;
                if (arrItem["activeStatus"].asBool(activeStatus) == CONV_OK)
                    deviceInfo.activeStatus = activeStatus;
                else
                    deviceInfo.activeStatus = false;
                if (arrItem["adjustVolume"].asBool(adjustVolume) == CONV_OK)
                    deviceInfo.adjustVolume = adjustVolume;
                else
                    deviceInfo.adjustVolume = true;
                deviceInfo.priority = arrItem["priority"].asNumber<int>();
                deviceInfo.deviceName = soundinput;
                if (arrItem["display"].asString(display) == CONV_OK)
                {
                    auto it = mSoundInputInfo.find(display);
                    if (it != mSoundInputInfo.end())
                        it->second.push_back(deviceInfo);
                    else
                    {
                        std::vector<utils::DEVICE_INFO_T> tmpDeviceInfo;
                        tmpDeviceInfo.push_back(deviceInfo);
                        mSoundInputInfo[display] = tmpDeviceInfo;
                    }
                }
                else
                    PM_LOG_WARNING(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                        "eventSoundOutputInfo::Unable to read display parameter");
            }
        }
        for (const auto it : mSoundInputInfo)
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "mSoundInputInfo: display:%s", it.first.c_str());
            for (const auto& deviceInfo : it.second)
            {
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "deviceName:%s priority:%d",\
                    deviceInfo.deviceName.c_str(), deviceInfo.priority);
                PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "activeStatus:%d isConnected:%d",\
                    (int)deviceInfo.activeStatus, (int)deviceInfo.isConnected);
            }
        }
    }
}


bool AudioRouter::setSoundOutput(const std::string& soundOutput, const int &displayId)
{
    std::string activeDevice;
    bool returnStatus = false;
    std::string display=getDisplayName(displayId);
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "setSoundOutput: display:%s soundOutput:%s",\
     display.c_str(), soundOutput.c_str());
    auto it = mSoundOutputInfo.find(display);
    if (it != mSoundOutputInfo.end())
    {
        for (auto &deviceInfo : it->second)
        {
            if (soundOutput == deviceInfo.deviceName)
            {
                if (nullptr != mObjAudioMixer)
                {
                    utils::SINK_ROUTING_INFO_T sinkInfo = getSinkRoutingInfo(display);
                    activeDevice = getActiveDevice(display, true);
                    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "setSoundOutput: sinkid:%d",\
                     (int)sinkInfo.startSink);
                    if (mObjAudioMixer->setSoundOutputOnRange(sinkInfo.startSink, sinkInfo.startSink,
                        soundOutput.c_str()))
                    {
                        updateDeviceStatus(display, soundOutput, true, true, true);
                        updateDeviceStatus(display, activeDevice, true, false, true);
                        returnStatus = true;
                    }
                }
            }
        }
    }
    if (!returnStatus)
    {
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT,"AudioRouter:setSoundOutput failed");
        updateDeviceStatus(display, soundOutput, true, false, true);
    }
    return returnStatus;
}



bool AudioRouter::setSoundInput(const std::string& soundInput, const int &displayId)
{
    std::string activeDevice;
    bool returnStatus = false;
    std::string display=getDisplayName(displayId);
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "setSoundInput: display:%s , soundInput:%s",\
     display.c_str(), soundInput.c_str());
    auto it = mSoundInputInfo.find(display);
    if (it != mSoundInputInfo.end())
    {
        for (auto &deviceInfo : it->second)
        {
            if (soundInput == deviceInfo.deviceName)
            {
                if (nullptr != mObjAudioMixer)
                {
                    activeDevice = getActiveDevice(display, false);
                    utils::SOURCE_ROUTING_INFO_T sourceInfo = getSourceRoutingInfo(display);
                    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "setSoundInput: sourceid:%d",
                        (int)sourceInfo.startSource);
                    if (mObjAudioMixer->setSoundInputOnRange(sourceInfo.startSource,
                        sourceInfo.startSource, soundInput.c_str()))
                    {
                        updateDeviceStatus(display, soundInput, true, true, false);
                        updateDeviceStatus(display, activeDevice, true, false, false);
                        returnStatus = true;
                    }
                }
            }
        }
    }
    if (!returnStatus)
    {
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT,"AudioRouter:setSoundInput failed");
        updateDeviceStatus(display, soundInput, true, false, false);
    }
    return returnStatus;
}

//API functions start//
bool AudioRouter::_getSoundOutput(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    bool status = false;
    bool subscribed = false;
    std::string reply;
    int sessionId = -1;
    CLSError lserror;
    std::string displayId;

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(displayId,integer),PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg.parse failed");
        return true;
    }
    msg.get("subscribe", subscribed);
    if (LSMessageIsSubscription (message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return true;
        }
    }
    AudioRouter *audioRouterInstance = AudioRouter::getAudioRouterInstance();


    if(!msg.get("displayId",sessionId))
    {
        sessionId = 0;
    }
    displayId = audioRouterInstance->getDisplayName(sessionId);

    std::string soundOutput  = audioRouterInstance->getActiveDevice (displayId, true);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);
    responseObj.put("displayId", audioRouterInstance->getNotificationSessionId(displayId));
    responseObj.put("soundOutput", soundOutput);
    responseObj.put("subscribed", subscribed);
    reply = responseObj.stringify();

    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "payload = %s", reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

std::string AudioRouter::getDisplayName(const int &displayId)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "getDisplayName:%d", displayId);
    std::string displayName = "";
    if (displayId == 0)
        displayName = "display1";
    else if (displayId == 1)
        displayName = "display2";

    return displayName;

}
int AudioRouter::getNotificationSessionId(const std::string &displayId)
{
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "getNotificationSessionId:%s", displayId.c_str());
    int sessionId = -1;

    if ("display1" == displayId)
        sessionId = 0;
    else if ("display2" == displayId)
        sessionId = 1;

    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "returning sessionId:%d", sessionId);
    return sessionId;
}

bool AudioRouter::_setSoundOutput(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(soundOutput, string)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg.parse failed");
        return true;
    }
    bool status = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    std::string soundOutput;
    int displayId = 0;      //only display 0 is supporting soundoutput
    CLSError lserror;
    AudioRouter *audioRouterInstance = AudioRouter::getAudioRouterInstance();

    if (msg.get("soundOutput", soundOutput))
    {
        if (audioRouterInstance)
        {

            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "got soundOutput = %s", soundOutput.c_str());

            if (!audioRouterInstance->setSoundOutput(soundOutput, displayId))
            {
                PM_LOG_ERROR (MSGID_AUDIOROUTER, INIT_KVCOUNT,"_setSoundOutput: failed setting soundOtput");
            }
            else
            {
                status = true;
                PM_LOG_INFO (MSGID_AUDIOROUTER, INIT_KVCOUNT,"_setSoundOutput successfull");
            }
            if(status)
            {
                pbnjson::JValue returnPayload = pbnjson::Object();
                returnPayload.put("returnValue", status);
                returnPayload.put("soundOutput", soundOutput);
                reply = returnPayload.stringify();
            }
            else
            {
                PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter: Unknown soundOutput");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_SOUNDOUTPUT_NOT_SUPPORT, "Audiod Unknown soundOutput");
            }

        }
        else
        {
            PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter: audioRouterInstance is null");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter:Missing soundOutput parameter");
        reply =  MISSING_PARAMETER_ERROR(soundOutput, string);
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioRouter::_setSoundInput(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(soundInput, string)) REQUIRED_1(soundInput)));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg.parse failed");
        return true;
    }
    bool status = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    std::string soundInput;
    int displayId = 0;      //assumed only display 0 supports soundoutputs
    CLSError lserror;
    AudioRouter *audioRouterInstance = AudioRouter::getAudioRouterInstance();
    if ((msg.get("soundInput", soundInput)))
    {
        if (audioRouterInstance)
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "got soundInput = %s", soundInput.c_str());

            if (!audioRouterInstance->setSoundInput(soundInput, displayId))
            {
                PM_LOG_ERROR (MSGID_AUDIOROUTER, INIT_KVCOUNT,"_setSoundInput: failed setting soundInput");
            }
            else
            {
                status = true;
                PM_LOG_INFO (MSGID_AUDIOROUTER, INIT_KVCOUNT,"setSoundInput successfull");
            }
            if(status)
            {
                pbnjson::JValue returnPayload = pbnjson::Object();
                returnPayload.put("returnValue", status);
                returnPayload.put("soundInput", soundInput);
                reply = returnPayload.stringify();
            }
            else
            {
                PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter: Unknown soundInput");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_SOUNDINPUT_NOT_SUPPORT, "Audiod Unknown soundInput");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter: audioRouterInstance is null");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter:Missing soundInput parameter");
        reply =  MISSING_PARAMETER_ERROR(soundInput, string);
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioRouter::_getSoundInput(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(subscribe, boolean),PROP(displayId,integer))));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg.parse failed");
        return true;
    }
    bool status = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool subscribed;
    int displayId;
    std::string activeSoundInput;
    CLSError lserror;
    AudioRouter *audioRouterInstance = AudioRouter::getAudioRouterInstance();
    if(!msg.get("subscribe", subscribed))
        subscribed=false;
    PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "_getSoundInput subscribed = %d", subscribed);

    if (audioRouterInstance)
    {
        if(!msg.get("displayId",displayId))
        {
            displayId = 0;
        }
        if (LSMessageIsSubscription (message))
        {
            if(!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
                PM_LOG_CRITICAL(MSGID_AUDIOROUTER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
                return true;
            }
        }
        std::string display = audioRouterInstance->getDisplayName(displayId);
        activeSoundInput = audioRouterInstance->getActiveDevice(display, false);
        PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "_getSoundInput activeSoundInput = %s",\
         activeSoundInput.c_str());
        pbnjson::JValue returnPayload = pbnjson::Object();
        returnPayload.put("subscribed", subscribed);
        returnPayload.put("soundInput", activeSoundInput);
        returnPayload.put("displayId", displayId);
        returnPayload.put("returnValue", true);
        reply = returnPayload.stringify();
    }
    else
    {
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "AudioRouter: audioRouterInstance is null");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}
//Soundoutput API start


bool AudioRouter::_SetSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(soundOut, string)) REQUIRED_1(soundOut)));

    if (!msg.parse(__FUNCTION__, lshandle))
    {
        return true;
    }
    std::string soundOut = "";
    msg.get("soundOut", soundOut);
    envelopeRef *envelopeObj = new (std::nothrow)envelopeRef;
    bool status = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    if (nullptr != envelopeObj)
    {
        envelopeObj->message = message;
        envelopeObj->context = (AudioRouter*)ctx;
        AudioRouter *soundOutputManagerObj = (AudioRouter*)ctx;
        if (nullptr != soundOutputManagerObj->mObjAudioMixer)
        {
            if (soundOutputManagerObj->mObjAudioMixer->setSoundOut(soundOut,_updateSoundOutStatus,envelopeObj))
            {
                PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "SoundSettings: SetSoundOut Successful");
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Sound Settings: SetSoundOut call failed");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Sound Settings :mObjAudioMixer is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (status == false)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelopeObj)
        {
            delete envelopeObj;
            envelopeObj = nullptr;
        }
    }
    return true;
}

bool AudioRouter::_updateSoundOutStatus(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_DEBUG("_updateSoundOutStatus Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                                                 REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;

    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "%s:%d no update. Could not updateSoundOutStatus", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData(reply, STRICT_SCHEMA(PROPS_2(PROP(returnValue, boolean),
        PROP(soundOut, string)) REQUIRED_2(returnValue, soundOut)));
        std::string l_strSoundOut;
        msgData.get("soundOut", l_strSoundOut);
        PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Soundoutmode set Successfully for sound out %s",l_strSoundOut.c_str());
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelopeObj = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelopeObj->message;
        if (nullptr != message)
        {
            CLSError lserror;
            std::string payload = LSMessageGetPayload(reply);
            if (!LSMessageRespond(message, payload.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
            LSMessageUnref(message);
        }

        if (nullptr != envelopeObj)
        {
            delete envelopeObj;
            envelopeObj = nullptr;
        }
    }
    PM_LOG_DEBUG("_updateSoundOutStatus Done");
    return returnValue;
}

//Soundoutput API end

//Class init start
AudioRouter* AudioRouter::mAudioRouter = nullptr;

AudioRouter* AudioRouter::getAudioRouterInstance()
{
    return mAudioRouter;
}

AudioRouter::AudioRouter(ModuleConfig* const pConfObj)
{
    PM_LOG_DEBUG("AudioRouter constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
    {
        //To make sure the default policy is already applied on tts and default
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventSinkStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventMixerStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventSinkPolicyInfo);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventSourcePolicyInfo);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventDeviceConnectionStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventBTDeviceDisplayInfo);
    }
    else
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
            "AutoManager:mObjModuleManager is null");
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    mObjDeviceRoutingParser = new (std::nothrow)DeviceRoutingConfigParser();
    if (mObjDeviceRoutingParser)
    {
        PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
            "AutoManager::constructor:mObjDeviceRoutingParser created successfully");
    }
    readDeviceRoutingInfo();
}

AudioRouter::~AudioRouter()
{
    PM_LOG_DEBUG("AudioRouter destructor");
    if (mObjDeviceRoutingParser)
    {
        delete mObjDeviceRoutingParser;
        mObjDeviceRoutingParser = nullptr;
    }
}

static LSMethod SoundOutputMethods[] = {
    {"setSoundOutput", AudioRouter::_setSoundOutput},
    {"setSoundInput", AudioRouter::_setSoundInput},
    {"getSoundInput", AudioRouter::_getSoundInput},
    {"getSoundOutput", AudioRouter::_getSoundOutput},

    { },
};


static LSMethod SoundOutputManagerMethods[] = {
    {"setSoundOut", AudioRouter::_SetSoundOut},
    { },
};

void AudioRouter::initialize()
{
    CLSError lserror;
    if (mAudioRouter)
    {
        CLSError lserror;
        bool bRetVal;
        PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, "load module AudioRouter successful");
        bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/", SoundOutputMethods, nullptr, &lserror);
        if (!bRetVal || !LSCategorySetData(GetPalmService(), "/", mAudioRouter, &lserror))
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT, \
            "%s: Registering Service for '%s' category failed", __FUNCTION__, "/");
            lserror.Print(__FUNCTION__, __LINE__);
        }
        bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/soundSettings", SoundOutputManagerMethods, nullptr, &lserror);
        if (!bRetVal || !LSCategorySetData(GetPalmService(), "/soundSettings", mAudioRouter, &lserror))
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                "%s: Registering Service for '%s' category failed", __FUNCTION__, "/soundSettings");
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
        PM_LOG_ERROR(MSGID_AUDIOROUTER, INIT_KVCOUNT, "mAudioRouter is nullptr");

}

void AudioRouter::deInitialize()
{
    PM_LOG_DEBUG("AudioRouter deInitialize()");
    if (mAudioRouter)
    {
        delete mAudioRouter;
        mAudioRouter = nullptr;
    }
}

void AudioRouter::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {
        case utils::eEventSinkStatus:
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent : eEventSinkStatus");
            events::EVENT_SINK_STATUS_T *stEventSinkStatus = (events::EVENT_SINK_STATUS_T*)event;
            eventSinkStatus(stEventSinkStatus->source, stEventSinkStatus->sink, stEventSinkStatus->audioSink,  \
                stEventSinkStatus->sinkStatus, stEventSinkStatus->mixerType);
        }
        break;
        case utils::eEventMixerStatus:
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent : eEventMixerStatus");
            events::EVENT_MIXER_STATUS_T *stEventMixerStatus = (events::EVENT_MIXER_STATUS_T*)event;
            eventMixerStatus( stEventMixerStatus->mixerStatus, stEventMixerStatus->mixerType);
        }
        break;
        case  utils::eEventDeviceConnectionStatus:
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent : eEventDeviceConnectionStatus");
            events::EVENT_DEVICE_CONNECTION_STATUS_T * stDeviceConnectionStatus = (events::EVENT_DEVICE_CONNECTION_STATUS_T*)event;
            eventDeviceConnectionStatus(stDeviceConnectionStatus->devicename, stDeviceConnectionStatus->deviceStatus, stDeviceConnectionStatus->mixerType);
        }
        break;
        case utils::eEventSinkPolicyInfo:
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent : eEventSinkPolicyInfo");
            events::EVENT_SINK_POLICY_INFO_T *stSinkPolicyInfo = (events::EVENT_SINK_POLICY_INFO_T*)event;
            eventSinkPolicyInfo(stSinkPolicyInfo->policyInfo);
        }
        break;
        case utils::eEventSourcePolicyInfo:
        {
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent : eEventSourcePolicyInfo");
            events::EVENT_SOURCE_POLICY_INFO_T *stSourcePolicyInfo = (events::EVENT_SOURCE_POLICY_INFO_T*)event;
            eventSourcePolicyInfo(stSourcePolicyInfo->sourcePolicyInfo);

        }
        break;
        case utils::eEventBTDeviceDisplayInfo:
        {
            events::EVENT_BT_DEVICE_DISPAY_INFO_T * stEventBtDeviceDisplayInfo = (events::EVENT_BT_DEVICE_DISPAY_INFO_T *)event;
            PM_LOG_INFO(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent : eEventBTDeviceDisplayInfo");
            eventBTDeviceDisplayInfo(stEventBtDeviceDisplayInfo->state, stEventBtDeviceDisplayInfo->address, stEventBtDeviceDisplayInfo->displayId);

        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_AUDIOROUTER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}