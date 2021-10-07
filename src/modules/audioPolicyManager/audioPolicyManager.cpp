/* @@@LICENSE
*
*      Copyright (c) 2020-2022 LG Electronics Company.
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

#include "audioPolicyManager.h"
#define MAX_PRIORITY 100
#define MAX_VOLUME 100
#define INIT_VOLUME 0
#define DISPLAY_ONE 0
#define DISPLAY_TWO 1
#define DEFAULT_ONE "default1"
#define DEFAULT_TWO "default2"

#define AUDIOD_API_SET_INPUT_VOLUME    "/setInputVolume"
#define AUDIOD_API_GET_SOURCE_INPUT_VOLUME    "/getSourceInputVolume"
#define AUDIOD_API_GET_INPUT_VOLUME    "/getInputVolume"
#define AUDIOD_API_GET_STREAM_STATUS   "/getStreamStatus"
#define AUDIOD_API_GET_SOURCE_STATUS   "/getSourceStatus"
#define AUDIOD_API_SET_MUTE_SINK       "/muteSink"
#define AUDIOD_API_SET_APP_VOLUME      "/setAppVolume"

bool AudioPolicyManager::mIsObjRegistered = AudioPolicyManager::RegisterObject();

//Event handling starts
void AudioPolicyManager::eventSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType,const int& sinkIndex, const std::string& appName)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventSinkStatus source:%s sink:%s sinkId:%d sinkStatus:%d mixerType:%d sinkIndex:%d, app : %s",\
        source.c_str(), sink.c_str(), (int)audioSink, (int)sinkStatus, (int)mixerType,\
        sinkIndex, appName.c_str());
    if (IsValidVirtualSink(audioSink))
    {
        std::string streamType = getStreamType(audioSink);
        std::string payload = "";
        int priority = 0;
        int currentVolume = 100;
        bool ramp = false;
        for (auto &elements : mVolumePolicyInfo)
        {
            if (elements.streamType == streamType)
            {
                elements.source = source;
                elements.sink = sink;
                elements.mixerType = mixerType;
                priority = elements.priority;
                currentVolume = elements.currentVolume;
                ramp = elements.ramp;
                elements.isStreamActive = (sinkStatus == utils::eSinkOpened) ? true : false;
                break;
            }
        }
        if (utils::eSinkOpened == sinkStatus)
        {
            addSinkInput(appName, sinkIndex, getStreamType(audioSink));
            muteSink(audioSink, getCurrentSinkMuteStatus(streamType), mixerType);
            if (setVolume(audioSink, currentVolume, mixerType, ramp))
                notifyGetVolumeSubscribers(streamType, currentVolume);
            payload = getStreamStatus(true);
            notifyGetStreamStatusSubscribers(payload);
            applyVolumePolicy(audioSink, streamType, priority);
        }
        else if (utils::eSinkClosed == sinkStatus)
        {
            if (setVolume(audioSink, INIT_VOLUME, mixerType, ramp))
                notifyGetVolumeSubscribers(streamType, INIT_VOLUME);
            payload = getStreamStatus(streamType, true);
            notifyGetStreamStatusSubscribers(payload);
            removeVolumePolicy(audioSink, streamType, priority);
        }
        else
            PM_LOG_WARNING(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager::invalid sink status");
    }
    else
        PM_LOG_WARNING(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager::invalid sink");
}


void AudioPolicyManager::eventSourceStatus(const std::string& source, const std::string& sink, EVirtualSource audioSource, \
            utils::ESINK_STATUS sourceStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventSourceStatus source:%s sink:%s sourceId:%d sourceStatus:%d mixerType:%d",\
        source.c_str(), sink.c_str(), (int)audioSource, (int)sourceStatus, (int)mixerType);
    if (IsValidVirtualSource(audioSource))
    {
        std::string streamType = getStreamType(audioSource);
        std::string payload = "";
        int priority = 0;
        int currentVolume = 100;
        bool ramp = false;
        for (auto &elements : mSourceVolumePolicyInfo)
        {
            if (elements.streamType == streamType)
            {
                elements.source = source;
                elements.sink = sink;
                elements.mixerType = mixerType;
                priority = elements.priority;
                currentVolume = elements.currentVolume;
                ramp = elements.ramp;
                elements.isStreamActive = (sourceStatus == utils::eSinkOpened) ? true : false;
                break;
            }
        }
        if (utils::eSinkOpened == sourceStatus)
        {
            if (mObjAudioMixer)
                mObjAudioMixer->setVirtualSourceMute((int)audioSource, getCurrentSourceMuteStatus(streamType));
            if (setVolume(audioSource, currentVolume, mixerType, ramp))
                notifyGetSourceVolumeSubscribers(streamType, currentVolume);
            payload = getSourceStatus(true);
            notifyGetSourceStatusSubscribers(payload);
            applyVolumePolicy(audioSource, streamType, priority);
        }
        else if (utils::eSinkClosed == sourceStatus)
        {
            if (setVolume(audioSource, INIT_VOLUME, mixerType, ramp))
                notifyGetSourceVolumeSubscribers(streamType, INIT_VOLUME);
            payload = getSourceStatus(true);
            notifyGetSourceStatusSubscribers(payload);
            removeVolumePolicy(audioSource, streamType, priority);
        }
        else
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager::invalid source status");
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager::invalid source");
}

void AudioPolicyManager::eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventMixerStatus mixerStatus:%d mixerType:%d", (int)mixerStatus, (int)mixerType);
    if (utils::ePulseMixer == mixerType)
        initStreamVolume();
}

void AudioPolicyManager::eventCurrentInputVolume(EVirtualAudioSink audioSink, const int& volume)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventCurrentInputVolume audioSink:%d volume:%d", (int)audioSink, volume);
    std::string streamType = getStreamType(audioSink);
    updateCurrentVolume(streamType, volume);
}
//Event handling ends

//Utility functions start

void AudioPolicyManager::addSinkInput(const std::string &appName, const int &sinkIndex, const std::string& sink)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::addSinkInput : appName : %s, sink-index : %d, sink : %s", appName.c_str(), sinkIndex, sink.c_str());
    auto it = mAppVolumeInfo.find(appName);
    if (it != mAppVolumeInfo.end())
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "appid found");
        for(auto elements:it->second)
        {
            if (elements.audioSink == getSinkType(sink))
            {
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "sink found");
                elements.sinkInputIndex = sinkIndex;
            }
        }
    }
    else
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "APPID NOT FOUND");
        utils::APP_VOLUME_INFO_T stAppVolumeInfo;
        stAppVolumeInfo.audioSink = getSinkType(sink);
        stAppVolumeInfo.volume = getCurrentVolume(sink);
        stAppVolumeInfo.sinkInputIndex = sinkIndex;
        mAppVolumeInfo[appName].push_back(stAppVolumeInfo);
    }
}
//TODO: fixme
void AudioPolicyManager::removeSinkInput(const std::string &appName, const int &sinkIndex, const std::string& sink)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::removeSinkInput : appName : %s, sink-index : %d, sink : %s", appName.c_str(), sinkIndex, sink.c_str());
    auto it = mAppVolumeInfo.find(appName);
    if (it != mAppVolumeInfo.end())
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "appid found");
        for(auto elements = it->second.begin();elements < it->second.end();elements++)
        {
            if (elements->audioSink == getSinkType(sink) && elements->sinkInputIndex == sinkIndex)
            {
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "sink found and REMOVED");
                it->second.erase(elements);
            }
        }
    }
    else
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "APPID NOT FOUND");
    }
}

void AudioPolicyManager::readPolicyInfo()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::readPolicyInfo");
    mObjPolicyInfoParser = new (std::nothrow) VolumePolicyInfoParser();
    if (mObjPolicyInfoParser)
    {
        if (mObjPolicyInfoParser->loadVolumePolicyJsonConfig())
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "loadVolumePolicyJsonConfig success");
            pbnjson::JValue policyInfo = mObjPolicyInfoParser->getVolumePolicyInfo(true);
            pbnjson::JValue policyInfoSources = mObjPolicyInfoParser->getVolumePolicyInfo(false);
            if (mObjModuleManager)
            {
                events::EVENT_SINK_POLICY_INFO_T eventSinkPolicyInfo;
                eventSinkPolicyInfo.eventName = utils::eEventSinkPolicyInfo;
                eventSinkPolicyInfo.policyInfo =  policyInfo;
                mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventSinkPolicyInfo);

                events::EVENT_SOURCE_POLICY_INFO_T eventSourcePolicyInfo;
                eventSourcePolicyInfo.eventName = utils::eEventSourcePolicyInfo;
                eventSourcePolicyInfo.sourcePolicyInfo = policyInfoSources;
                mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventSourcePolicyInfo);
            }
            else
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "readPolicyInfo: mObjModuleManager is null");
            if (initializePolicyInfo(policyInfo, true))
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializePolicyInfo success");
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializePolicyInfo failed");
            createSinkStreamMap(policyInfo);
            if (initializePolicyInfo(policyInfoSources, false))
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializePolicyInfo success");
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializePolicyInfo failed");
            createSourceStreamMap(policyInfoSources);
        }
        else
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Unable to load VolumePolicyJsonConfig");
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mObjPolicyInfoParser is null");
}

bool AudioPolicyManager::initializePolicyInfo(const pbnjson::JValue& policyInfo, bool isSink)
{
    PM_LOG_DEBUG("AudioPolicyManager::initializePolicyInfo");
    if (!policyInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "policyVolumeInfo is not an array");
        return false;
    }
    else
    {
        for (const pbnjson::JValue& elements : policyInfo.items())
        {
            utils::VOLUME_POLICY_INFO_T stPolicyInfo;
            std::string streamType;
            std::string sink;
            std::string source;
            std::string category;
            bool volumeAdjustable = true;
            bool muteStatus = false;
            bool ramp = false;

            if (elements["streamType"].asString(streamType) == CONV_OK)
                stPolicyInfo.streamType = streamType;
            stPolicyInfo.policyVolume = elements["policyVolume"].asNumber<int>();
            stPolicyInfo.priority = elements["priority"].asNumber<int>();
            stPolicyInfo.groupId = elements["group"].asNumber<int>();
            stPolicyInfo.defaultVolume = elements["defaultVolume"].asNumber<int>();
            stPolicyInfo.maxVolume = elements["maxVolume"].asNumber<int>();
            stPolicyInfo.minVolume = elements["minVolume"].asNumber<int>();
            if (elements["volumeAdjustable"].asBool(volumeAdjustable) == CONV_OK)
                stPolicyInfo.volumeAdjustable = volumeAdjustable;
            stPolicyInfo.currentVolume = elements["currentVolume"].asNumber<int>();
            if (elements["muteStatus"].asBool(muteStatus) == CONV_OK)
                stPolicyInfo.muteStatus = muteStatus;
            if (elements["sink"].asString(sink) == CONV_OK)
                stPolicyInfo.sink = sink;
            if (elements["source"].asString(source) == CONV_OK)
                stPolicyInfo.source = source;
            if (elements["ramp"].asBool(ramp) == CONV_OK)
                stPolicyInfo.ramp = ramp;
            if (elements["category"].asString(category) == CONV_OK)
                stPolicyInfo.category = category;
            if (isSink)
                mVolumePolicyInfo.push_back(stPolicyInfo);
            else
                mSourceVolumePolicyInfo.push_back(stPolicyInfo);
        }
    }
    printPolicyInfo();
    return true;
}

void AudioPolicyManager::initStreamVolume()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::initStreamVolume");
    EVirtualAudioSink sink = eVirtualSink_None;
    for (const auto &elements : mVolumePolicyInfo)
    {
        sink = getSinkType(elements.streamType);
        if (!setVolume(sink, INIT_VOLUME, utils::ePulseMixer))
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "AudioPolicyManager::initStreamVolume volume could not be set for stream:%s",\
                elements.streamType.c_str());
        if (mObjAudioMixer)
            mObjAudioMixer->muteSink(sink, false);
    }
    for (const auto &elements : mSourceVolumePolicyInfo)
    {
        EVirtualSource source = getSourceType(elements.streamType);
        if (!setVolume(source, INIT_VOLUME, utils::ePulseMixer))
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "AudioPolicyManager::initStreamVolume volume could not be set for stream:%s",\
                elements.streamType.c_str());
        if (mObjAudioMixer)
            mObjAudioMixer->setVirtualSourceMute(source, false);
    }
}

void AudioPolicyManager::printPolicyInfo()
{
    PM_LOG_DEBUG("AudioPolicyManager::printPolicyInfo");
    for (const auto& elements : mVolumePolicyInfo)
    {
        PM_LOG_DEBUG("*************%s*************", elements.streamType.c_str());
        PM_LOG_DEBUG("policyVolume:%d", elements.policyVolume);
        PM_LOG_DEBUG("priority:%d groupId:%d defaultVolume:%d",\
            elements.priority, elements.groupId, elements.defaultVolume);
        PM_LOG_DEBUG("maxVolume:%d minVolume:%d volumeAdjustable:%d",\
             elements.maxVolume, elements.minVolume, (int)elements.volumeAdjustable);
        PM_LOG_DEBUG("currentVolume:%d muteStatus:%d source:%s sinkId:%s mixerType:%d ramp:%d",\
            elements.currentVolume, (int)elements.muteStatus, elements.source.c_str(),\
            elements.sink.c_str(), (int)elements.mixerType, (int)elements.ramp);
        PM_LOG_DEBUG("isPolicyInProgress:%d activeStatus:%d category:%s",\
            (int)elements.isPolicyInProgress, (int)elements.isStreamActive, elements.category.c_str());
    }
    PM_LOG_DEBUG("AudioPolicyManager::Sounces policy:\n");
    for (const auto& elements : mSourceVolumePolicyInfo)
    {
        PM_LOG_DEBUG("*************%s*************", elements.streamType.c_str());
        PM_LOG_DEBUG("policyVolume:%d", elements.policyVolume);
        PM_LOG_DEBUG("priority:%d groupId:%d defaultVolume:%d",\
            elements.priority, elements.groupId, elements.defaultVolume);
        PM_LOG_DEBUG("maxVolume:%d minVolume:%d volumeAdjustable:%d",\
             elements.maxVolume, elements.minVolume, elements.volumeAdjustable);
        PM_LOG_DEBUG("currentVolume:%d muteStatus:%d source:%s sinkId:%s mixerType:%d ramp:%d",\
            elements.currentVolume, elements.muteStatus, elements.source.c_str(),\
            elements.sink.c_str(), elements.mixerType, (int)elements.ramp);
        PM_LOG_DEBUG("isPolicyInProgress:%d activeStatus:%d category:%s",\
            elements.isPolicyInProgress, elements.isStreamActive, elements.category.c_str());
    }
}

void AudioPolicyManager::printActivePolicyInfo()
{
    PM_LOG_DEBUG("AudioPolicyManager::printActivePolicyInfo:");
    for (const auto& elements : mVolumePolicyInfo)
    {
        if (elements.isPolicyInProgress)
        {
            PM_LOG_DEBUG("*************%s*************", elements.streamType.c_str());
            PM_LOG_DEBUG("policyVolume:%d", elements.policyVolume);
            PM_LOG_DEBUG("priority:%d groupId:%d defaultVolume:%d",\
                elements.priority, elements.groupId, elements.defaultVolume);
            PM_LOG_DEBUG("maxVolume:%d minVolume:%d volumeAdjustable:%d",\
                 elements.maxVolume, elements.minVolume, (int)elements.volumeAdjustable);
            PM_LOG_DEBUG("currentVolume:%d muteStatus:%d source:%s sinkId:%s mixerType:%d ramp:%d",\
                elements.currentVolume, (int)elements.muteStatus, elements.source.c_str(),\
                elements.sink.c_str(), (int)elements.mixerType, (int)elements.ramp);
            PM_LOG_DEBUG("isPolicyInProgress:%d activeStatus:%d category:%s",\
                (int)elements.isPolicyInProgress, (int)elements.isStreamActive, elements.category.c_str());
        }
    }
}

std::string AudioPolicyManager::getStreamType(EVirtualAudioSink audioSink)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: getStreamType:%d", (int)audioSink);
     utils::itMapSinkToStream it = mSinkToStream.find(audioSink);
     if (it != mSinkToStream.end())
         return it->second;
     else
         return "";
}

std::string AudioPolicyManager::getStreamType(EVirtualSource audioSource)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: getStreamType:%d", (int)audioSource);
     utils::itMapSourceToStream it = mSourceToStream.find(audioSource);
     if (it != mSourceToStream.end())
         return it->second;
     else
         return "";
}

bool AudioPolicyManager::getSourceActiveStatus(const std::string& streamType)
{
    for (const auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            return elements.isStreamActive;
        }
    }
    return false;
}


bool AudioPolicyManager::getStreamActiveStatus(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: getStreamActiveStatus :%s", streamType.c_str());

    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            return elements.isStreamActive;
        }
    }
    return false;
}

EVirtualAudioSink AudioPolicyManager::getSinkType(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: getSinkType:%s", streamType.c_str());
     utils::itMapStreamToSink it = mStreamToSink.find(streamType);
     if (it != mStreamToSink.end())
         return it->second;
     else
         return eVirtualSink_None;
}


EVirtualSource AudioPolicyManager::getSourceType(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: getSourceType:%s", streamType.c_str());
     utils::itMapStreamToSource it = mStreamToSource.find(streamType);
     if (it != mStreamToSource.end())
         return it->second;
     else
         return eVirtualSource_None;
}

void AudioPolicyManager::createSinkStreamMap(const pbnjson::JValue& policyInfo)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: createSinkStreamMap");

    if (!policyInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "policyInfo is not an array");
        return;
    }
    else
    {
        std::string streamType;
        for (const pbnjson::JValue& elements : policyInfo.items())
        {
            if (elements["streamType"].asString(streamType) == CONV_OK)
            {
                PM_LOG_DEBUG("AudioPolicyManager: createSinkStreamMap Inserting the elements into map for stream %s", streamType.c_str());
                mSinkToStream[getSinkByName(streamType.c_str())] = streamType;
                mStreamToSink[streamType] = getSinkByName(streamType.c_str());
            }
        }
    }
}

void AudioPolicyManager::createSourceStreamMap(const pbnjson::JValue& policyInfo)
{
    //Source related
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: createSourceStreamMap");

    if (!policyInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "policyInfo is not an array");
        return;
    }
    else
    {
        std::string streamType;
        for (const pbnjson::JValue& elements : policyInfo.items())
        {
            if (elements["streamType"].asString(streamType) == CONV_OK)
            {
                PM_LOG_DEBUG("AudioPolicyManager: createSinkStreamMap Inserting the elements into map for stream %s", streamType.c_str());
                mSourceToStream[getSourceByName(streamType.c_str())] = streamType;
                mStreamToSource[streamType] = getSourceByName(streamType.c_str());
            }
        }
    }
}

void AudioPolicyManager::applyVolumePolicy(EVirtualAudioSink audioSink, const std::string& streamType, const int& priority)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:applyVolumePolicy sink:%d streamType:%s priority:%d",\
        (int)audioSink, streamType.c_str(), priority);
    if (mObjAudioMixer)
    {
        int policyPriority = MAX_PRIORITY;
        bool isPolicyInProgress = false;
        std::string policyStreamType;
        int currentPolicyStreamVolume = MAX_VOLUME;
        int currentVolume = MAX_VOLUME;
        utils::vectorVirtualSink activeStreams = mObjAudioMixer->getActiveStreams();
        std::string incomingStreamCategory = getCategory(streamType);
        std::string activeStreamCategory;
        for (const auto &it : activeStreams)
        {
            policyStreamType = getStreamType(it);
            policyPriority  = getPriority(policyStreamType);
            activeStreamCategory  = getCategory(policyStreamType);
            if (incomingStreamCategory == activeStreamCategory)
            {
                if (priority < policyPriority)
                {
                    isPolicyInProgress = getPolicyStatus(policyStreamType);
                    currentPolicyStreamVolume = getPolicyVolume(policyStreamType);
                    currentVolume  = getCurrentVolume(policyStreamType);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicy Incoming stream:%s is high priority than active stream:%s with priority:%d policyStatus:%d",\
                        streamType.c_str(), policyStreamType.c_str(), policyPriority, isPolicyInProgress);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:currentPolicyStreamVolume:%d currentVolume:%d",\
                        currentPolicyStreamVolume, currentVolume);
                    if (!isPolicyInProgress && currentVolume > currentPolicyStreamVolume)
                    {
                        if (setVolume(getSinkType(policyStreamType), currentPolicyStreamVolume,\
                            getMixerType(policyStreamType), isRampPolicyActive(policyStreamType)))
                            updatePolicyStatus(policyStreamType, true);
                    }
                    else
                        PM_LOG_WARNING(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicy current volume is less than policy volume");
                }
                else if (priority > policyPriority)
                {
                    isPolicyInProgress = getPolicyStatus(streamType);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicy Incoming stream is low priority than active stream:%s with priority:%d policyStatus:%d",\
                        policyStreamType.c_str(), policyPriority, isPolicyInProgress);
                    if (!isPolicyInProgress)
                    {
                        if (setVolume(audioSink, getPolicyVolume(streamType), getMixerType(streamType), isRampPolicyActive(streamType)))
                            updatePolicyStatus(streamType, true);
                    }
                }
                else
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicy Incoming stream:%s and active stream:%s priority is same",\
                        streamType.c_str(), policyStreamType.c_str());
            }
            else
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:incoming stream category:%s active stream category:%s are not same",\
                    incomingStreamCategory.c_str(), activeStreamCategory.c_str());
        }
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager:applyVolumePolicy mObjAudioMixer is null");
}


void AudioPolicyManager::applyVolumePolicy(EVirtualSource audioSource, const std::string& streamType, const int& priority)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:applyVolumePolicyForSource source:%d streamType:%s priority:%d",\
        (int)audioSource, streamType.c_str(), priority);
    if (mObjAudioMixer)
    {
        int policyPriority = MAX_PRIORITY;
        bool isPolicyInProgress = false;
        std::string policyStreamType;
        int currentPolicyStreamVolume = MAX_VOLUME;
        int currentVolume = MAX_VOLUME;
        utils::vectorVirtualSource activeStreams = mObjAudioMixer->getActiveSources();
        std::string incomingStreamCategory = getCategoryOfSource(streamType);
        std::string activeStreamCategory;
        for (const auto &it : activeStreams)
        {
            policyStreamType = getStreamType(it);
            policyPriority  = getPriorityOfSource(policyStreamType);
            activeStreamCategory  = getCategoryOfSource(policyStreamType);
            if (incomingStreamCategory == activeStreamCategory)
            {
                if (priority < policyPriority)
                {
                    isPolicyInProgress = getPolicyStatusOfSource(policyStreamType);
                    currentPolicyStreamVolume = getPolicyVolumeofSource(policyStreamType);
                    currentVolume  = getCurrentVolumeOfSource(policyStreamType);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicyForSource Incoming stream:%s is high priority than active stream:%s with priority:%d policyStatus:%d",\
                        streamType.c_str(), policyStreamType.c_str(), policyPriority, isPolicyInProgress);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:currentPolicyStreamVolume:%d currentVolume:%d",\
                        currentPolicyStreamVolume, currentVolume);
                    if (!isPolicyInProgress && currentVolume > currentPolicyStreamVolume)
                    {
                        if (setVolume(getSourceType(policyStreamType), currentPolicyStreamVolume,\
                            getMixerTypeofSource(policyStreamType), isRampPolicyActiveForSource(policyStreamType)))
                            updatePolicyStatusForSource(policyStreamType, true);
                    }
                    else
                        PM_LOG_WARNING(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicyForSource current volume is less than policy volume");
                }
                else if (priority > policyPriority)
                {
                    isPolicyInProgress = getPolicyStatusOfSource(streamType);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicyForSource Incoming stream is low priority than active stream:%s with priority:%d policyStatus:%d",\
                        policyStreamType.c_str(), policyPriority, isPolicyInProgress);
                    if (!isPolicyInProgress)
                    {
                        if (setVolume(audioSource, getPolicyVolumeofSource(streamType), getMixerTypeofSource(streamType), isRampPolicyActiveForSource(streamType)))
                            updatePolicyStatusForSource(streamType, true);
                    }
                }
                else
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:applyVolumePolicyForSource Incoming stream:%s and active stream:%s priority is same",\
                        streamType.c_str(), policyStreamType.c_str());
            }
            else
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:incoming stream category:%s active stream category:%s are not same",\
                    incomingStreamCategory.c_str(), activeStreamCategory.c_str());
        }
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager:applyVolumePolicyForSource mObjAudioMixer is null");
}

void AudioPolicyManager::removeVolumePolicy(EVirtualAudioSink audioSink, const std::string& streamType, const int& priority)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:removeVolumePolicy sink:%d streamType:%s priority:%d",\
        (int)audioSink, streamType.c_str(), priority);
    if (mObjAudioMixer)
    {
        int policyPriority = MAX_PRIORITY;
        bool isPolicyInProgress = false;
        std::string policyStreamType;
        utils::vectorVirtualSink activeStreams = mObjAudioMixer->getActiveStreams();
        std::string incomingStreamCategory = getCategory(streamType);
        std::string activeStreamCategory;
        for (const auto &it : activeStreams)
        {
            policyStreamType = getStreamType(it);
            policyPriority  = getPriority(policyStreamType);
            activeStreamCategory = getCategory(policyStreamType);
            if (incomingStreamCategory == activeStreamCategory)
            {
                if (priority < policyPriority)
                {
                    isPolicyInProgress = getPolicyStatus(policyStreamType);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:removeVolumePolicy Incoming stream:%s is high priority than active stream:%s with priority:%d policyStatus:%d",\
                        streamType.c_str(), policyStreamType.c_str(), policyPriority, isPolicyInProgress);
                    if (isPolicyInProgress && !isHighPriorityStreamActive(policyPriority, activeStreams, incomingStreamCategory))
                    {
                        if (setVolume(getSinkType(policyStreamType), getCurrentVolume(policyStreamType),\
                            getMixerType(policyStreamType), isRampPolicyActive(policyStreamType)))
                            updatePolicyStatus(policyStreamType, false);
                    }
                    else
                        PM_LOG_WARNING(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:removeVolumePolicy policy is not in progress or still other high priority streams are active");
                }
                else
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:removeVolumePolicy Incoming stream:%s priority is same or less than active stream:%s priority",\
                        streamType.c_str(), policyStreamType.c_str());
            }
            else
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:removeVolumePolicy incoming stream category:%s active stream category:%s are not same",\
                    incomingStreamCategory.c_str(), activeStreamCategory.c_str());
        }
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager:removeVolumePolicy mObjAudioMixer is null");
    updatePolicyStatus(streamType, false);
}

void AudioPolicyManager::removeVolumePolicy(EVirtualSource audioSource, const std::string& streamType, const int& priority)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:removeVolumePolicy sink:%d streamType:%s priority:%d",\
        (int)audioSource, streamType.c_str(), priority);
    if (mObjAudioMixer)
    {
        int policyPriority = MAX_PRIORITY;
        bool isPolicyInProgress = false;
        std::string policyStreamType;
        utils::vectorVirtualSource activeStreams = mObjAudioMixer->getActiveSources();
        std::string incomingStreamCategory = getCategoryOfSource(streamType);
        std::string activeStreamCategory;
        for (const auto &it : activeStreams)
        {
            policyStreamType = getStreamType(it);
            policyPriority  = getPriorityOfSource(policyStreamType);
            activeStreamCategory = getCategoryOfSource(policyStreamType);
            if (incomingStreamCategory == activeStreamCategory)
            {
                if (priority < policyPriority)
                {
                    isPolicyInProgress = getPolicyStatusOfSource(policyStreamType);
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:removeVolumePolicy Incoming stream:%s is high priority than active stream:%s with priority:%d policyStatus:%d",\
                        streamType.c_str(), policyStreamType.c_str(), policyPriority, isPolicyInProgress);
                    if (isPolicyInProgress && !isHighPrioritySourceActive(policyPriority, activeStreams))
                    {
                        if (setVolume(getSourceType(policyStreamType), getCurrentVolumeOfSource(policyStreamType),\
                            getMixerTypeofSource(policyStreamType), isRampPolicyActiveForSource(policyStreamType)))
                            updatePolicyStatusForSource(policyStreamType, false);
                    }
                    else
                        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:removeVolumePolicy policy is not in progress or still other high priority streams are active");
                }
                else
                    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                        "AudioPolicyManager:removeVolumePolicy Incoming stream:%s priority is same or less than active stream:%s priority",\
                        streamType.c_str(), policyStreamType.c_str());
            }
            else
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:removeVolumePolicy incoming stream category:%s active stream category:%s are not same",\
                    incomingStreamCategory.c_str(), activeStreamCategory.c_str());
        }
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager:removeVolumePolicy mObjAudioMixer is null");
    updatePolicyStatusForSource(streamType, false);
}

bool AudioPolicyManager::setVolume(EVirtualSource audioSource, const int& volume, \
    utils::EMIXER_TYPE mixerType, bool ramp)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:setVolume:%d for audioSource:%d, ramp = %d", volume, (int)audioSource, (int)ramp);
    bool returnStatus = false;
    if (mObjAudioMixer)
    {
        if (utils::ePulseMixer == mixerType)
        {
            if (mObjAudioMixer->programVolume(audioSource, volume, ramp))
                returnStatus = true;
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:programVolume failed");
        }
    }
    return returnStatus;
}

bool AudioPolicyManager::storeAppVolume(const std::string &mediaId, const int &volume, EVirtualAudioSink audioSink)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "AudioPolicyManager:storeAppVolume volume%d for mediaId:%s, sink = %d", volume, mediaId.c_str(), (int)audioSink);
    bool status = false;
    auto it = mAppVolumeInfo.find(mediaId);
    if (it != mAppVolumeInfo.end())
    {
        for (auto &elements : it->second)
        {
            if (elements.audioSink == audioSink)
            {
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "storeAppVolume Updating volume for existing mediaId and sink");
                elements.volume = volume;
                status = true;
                break;
            }
        }
        if (!status)
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "storeAppVolume adding volume for new sink with existing mediaId");
            utils::APP_VOLUME_INFO_T stAppVolumeInfo;
            stAppVolumeInfo.audioSink = audioSink;
            stAppVolumeInfo.volume = volume;
            it->second.push_back(stAppVolumeInfo);
        }
    }
    else
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "storeAppVolume Adding mediaId for the first time");
        utils::APP_VOLUME_INFO_T stAppVolumeInfo;
        stAppVolumeInfo.audioSink = audioSink;
        stAppVolumeInfo.volume = volume;
        stAppVolumeInfo.sinkInputIndex = -1;    //default initialization
        mVectAppVolumeInfo.push_back(stAppVolumeInfo);
        mAppVolumeInfo[mediaId] = mVectAppVolumeInfo;
    }
    printAppVolumeInfo();
    return true;
}

void AudioPolicyManager::printAppVolumeInfo()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager::printAppVolumeInfo");
    for (const auto it : mAppVolumeInfo)
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mAppVolumeInfo: mediaId:%s", it.first.c_str());
        for (const auto& volumeInfo : it.second)
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "audioSink:%d volume:%d", (int)volumeInfo.audioSink, volumeInfo.volume);
        }
    }
}

bool AudioPolicyManager::setAppVolume(const std::string& mediaId, const int &volume, EVirtualAudioSink audioSink, \
    utils::EMIXER_TYPE mixerType, bool ramp)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:setAppVolume:%d for sink:%d, mixerType = %d, ramp = %d", volume, (int)audioSink, (int)mixerType, (int)ramp);
    bool returnStatus = false;
    if (mObjAudioMixer)
    {
        if (utils::ePulseMixer == mixerType)
        {
            auto it = mAppVolumeInfo.find(mediaId);
            if (it != mAppVolumeInfo.end())
            {
                for (auto &elements : it->second)
                {
                    if ((elements.audioSink == audioSink) && (elements.sinkInputIndex != -1))
                    {
                        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager calling programVolume");
                        if (mObjAudioMixer->programAppVolume(audioSink, elements.sinkInputIndex, volume, ramp))
                            returnStatus = true;
                        else
                            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager:programAppVolume failed");
                    }
                }
            }
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: mediaId is not present");
        }
        else if (utils::eUmiMixer == mixerType)
        {
            //Will be uncommented when umi mixer is enabled
        }
        else
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "AudioPolicyManager:Invalid mixer type");
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager:applyVolumePolicy mObjAudioMixer is null");
    return returnStatus;
}

bool AudioPolicyManager::setVolume(EVirtualAudioSink audioSink, const int& volume, \
    utils::EMIXER_TYPE mixerType, bool ramp)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:setVolume:%d for sink:%d, mixerType = %d, ramp = %d", volume, (int)audioSink, (int)mixerType, (int)ramp);
    bool returnStatus = false;
    if (mObjAudioMixer)
    {
        if (utils::ePulseMixer == mixerType)
        {
            if (mObjAudioMixer->programVolume(audioSink, volume, ramp))
                returnStatus = true;
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:programVolume failed");
        }
        else if (utils::eUmiMixer == mixerType)
        {
            //Will be uncommented when umi mixer is enabled
            /*if (mObjAudioMixer->setInputVolume(source, sink, volume))
                returnStatus = true;
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "AudioPolicyManager:setInputVolume failed");*/
        }
        else
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "AudioPolicyManager:Invalid mixer type");
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "AudioPolicyManager:applyVolumePolicy mObjAudioMixer is null");
    return returnStatus;
}

std::string AudioPolicyManager::getCategory(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getCategory streamType:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.category;
    }
    return "";
}

std::string AudioPolicyManager::getCategoryOfSource(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getCategoryOfSource streamType:%s", streamType.c_str());
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.category;
    }
    return "";
}

bool AudioPolicyManager::isHighPriorityStreamActive(const int& policyPriority, utils::vectorVirtualSink activeStreams, const std::string& category)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:isHighPriorityStreamActive priority:%d", policyPriority);
    int priority  = MAX_PRIORITY;
    for (const auto &it : activeStreams)
    {
        if (category == getCategory(getStreamType(it)))
        {
            priority  = getPriority(getStreamType(it));
            if (policyPriority > priority)
                return true;
        }
    }
    return false;
}

bool AudioPolicyManager::isHighPrioritySourceActive(const int& policyPriority, utils::vectorVirtualSource activeStreams)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:isHighPrioritySourceActive priority:%d", policyPriority);
    int priority  = MAX_PRIORITY;
    for (const auto &it : activeStreams)
    {
        priority  = getPriorityOfSource(getStreamType(it));
        if (policyPriority > priority)
            return true;
    }
    return false;
}

int AudioPolicyManager::getCurrentVolume(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getCurrentVolume streamType:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.currentVolume;
    }
    return MAX_VOLUME;
}

int AudioPolicyManager::getCurrentVolumeOfSource(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getCurrentVolumeOfSource streamType:%s", streamType.c_str());
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.currentVolume;
    }
    return MAX_VOLUME;
}

void AudioPolicyManager::updatePolicyStatus(const std::string& streamType, const bool& status)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:updatePolicyStatus streamType:%s status:%d", streamType.c_str(), status);
    for (auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            elements.isPolicyInProgress = status;
            break;
        }
    }
    std::string payload = getStreamStatus(streamType, true);
    notifyGetStreamStatusSubscribers(payload);
}

void AudioPolicyManager::updatePolicyStatusForSource(const std::string& streamType, const bool& status)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:updatePolicyStatusForSource streamType:%s status:%d", streamType.c_str(), (int)status);
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            elements.isPolicyInProgress = status;
            break;
        }
    }
    std::string payload = getSourceStatus(streamType, true);
    notifyGetSourceStatusSubscribers(payload);
}

bool AudioPolicyManager::getPolicyStatus(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getPolicyStatus streamType:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.isPolicyInProgress;
    }
    return false;
}

bool AudioPolicyManager::getPolicyStatusOfSource(const std::string& streamType)
{
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.isPolicyInProgress;
    }
    return true;
}

int AudioPolicyManager::getPriority(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getPriority streamType:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.priority;
    }
    return MAX_PRIORITY;
}

int AudioPolicyManager::getPriorityOfSource(const std::string& streamType)
{
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.priority;
    }
    return MAX_PRIORITY;
}

int AudioPolicyManager::getPolicyVolume(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getPolicyVolume streamType:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.policyVolume;
    }
    return MAX_VOLUME;
}

int AudioPolicyManager::getPolicyVolumeofSource(const std::string& streamType)
{
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.policyVolume;
    }
    return MAX_VOLUME;
}

utils::EMIXER_TYPE AudioPolicyManager::getMixerType(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getMixerType streamType:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.mixerType;
    }
    return utils::eMixerNone;
}

utils::EMIXER_TYPE AudioPolicyManager::getMixerTypeofSource(const std::string& streamType)
{
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.mixerType;
    }
    return utils::eMixerNone;
}

void AudioPolicyManager::updateCurrentVolume(const std::string& streamType, const int &volume)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:updateCurrentVolume streamType:%s, Volume = %d", streamType.c_str(), volume);
    for (auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            elements.currentVolume = volume;
            break;
        }
    }
}

void AudioPolicyManager::updateCurrentVolumeForSource(const std::string& streamType, const int &volume)
{
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            elements.currentVolume = volume;
            return;
        }
    }
}

void AudioPolicyManager::updateMuteStatus(const std::string& streamType, const bool &mute)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:updateMuteStatus streamType:%s, MuteStatus = %d", streamType.c_str(), (int)mute);
    for (auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            elements.muteStatus = mute;
            return;
        }
    }
}

void AudioPolicyManager::updateMuteStatusForSource(const std::string& streamType, const bool &mute)
{
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            elements.currentVolume = mute;
            return;
        }
    }
}

int AudioPolicyManager::getCurrentSinkMuteStatus(const std::string &streamType)
{
    bool muteStatus = false;
    for (auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            muteStatus = elements.muteStatus;
            break;
        }
    }
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "getCurrentSinkMuteStatus %s : %d", streamType.c_str(), (int)muteStatus);
    return (int)muteStatus;
}

int AudioPolicyManager::getCurrentSourceMuteStatus(const std::string &streamType)
{
    bool muteStatus = false;
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            muteStatus = elements.muteStatus;
            break;
        }
    }
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "getCurrentSourceMuteStatus %s : %d", streamType.c_str(), (int)muteStatus);
    return (int)muteStatus;
}

bool AudioPolicyManager::isRampPolicyActive(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:isRampPolicyActive:%s", streamType.c_str());
    for (const auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.ramp;
    }
    return false;
}

bool AudioPolicyManager::isRampPolicyActiveForSource(const std::string& streamType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:getMixerType isRampPolicyActive:%s", streamType.c_str());
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
            return elements.ramp;
    }
    return false;
}

bool AudioPolicyManager::muteSink(EVirtualAudioSink audioSink, const int& muteStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager:muteSink:%d for sink:%d, \
     mixerType = %d", muteStatus, (int)audioSink, (int)mixerType);
    bool returnStatus = false;
    if (mObjAudioMixer)
    {
        if (utils::ePulseMixer == mixerType)
        {
            if (mObjAudioMixer->muteSink(audioSink, muteStatus))
                returnStatus = true;
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager:muteSink failed");
        }
        else if (utils::eUmiMixer == mixerType)
        {
            //Will be uncommented when umi mixer is enabled
        }
        else
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,"AudioPolicyManager:Invalid mixer type");
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,"AudioPolicyManager:muteSink mObjAudioMixer is null");
    return returnStatus;
}
//Utility functions ends

//API functions start

bool AudioPolicyManager::_muteSink(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: _muteSink");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(streamType, string), PROP(mute, boolean))
                                                          REQUIRED_2(streamType, mute)));
    std::string reply;
    if (!msg.parse(__FUNCTION__, lshandle))
       return true;
    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (audioPolicyManagerInstance)
    {
        bool status = false;
        bool isValidStream = false;
        bool isStreamActive = false;
        std::string streamType;
        bool mute = false;
        msg.get ("streamType", streamType);
        msg.get("mute", mute);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "got sink = %s , mute = %d ",\
         streamType.c_str(), (int)mute);
        EVirtualAudioSink sink = audioPolicyManagerInstance->getSinkType(streamType);
        if (sink != eVirtualSink_None)
        {
            isValidStream = true;
        }
        if (audioPolicyManagerInstance->getStreamActiveStatus(streamType))
        {
            isStreamActive = true;
        }
        if (isValidStream)
        {
            if (isStreamActive)
            {
                if (!audioPolicyManagerInstance->muteSink(sink, mute, audioPolicyManagerInstance->getMixerType(streamType)))
                {
                    PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, "_muteSink: failed mixer call");
                }
                else
                {
                    audioPolicyManagerInstance->updateMuteStatus(streamType, mute);
                    status = true;
                    PM_LOG_INFO (MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Mute status updated successfully");
                }
            }
        }
        if (status)
        {
            pbnjson::JValue muteSinkResponse = pbnjson::Object();
            muteSinkResponse.put("returnValue", status);
            muteSinkResponse.put("mute", mute);
            muteSinkResponse.put("streamType", streamType);
            reply = muteSinkResponse.stringify();
        }
        else
        {
            if (!isValidStream)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Audiod Unknown Stream");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
            else
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Audiod internal error");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager instance Null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioPolicyManager::_setInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: _setInputVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(streamType, string), PROP(volume, integer),
                                                           PROP(ramp, boolean)) REQUIRED_2(streamType, volume)));

    std::string reply;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;

    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (audioPolicyManagerInstance)
    {
        bool status = false;
        bool isValidVolume = false;
        bool isValidStream = false;
        bool isStreamActive = false;
        int volume = 0;
        std::string streamType;
        bool ramp = false;

        msg.get ("streamType", streamType);
        msg.get ("volume", volume);
        if (!msg.get ("ramp", ramp))
        {
            ramp = false;
        }
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
            "got sink = %s , vol = %d , ramp = %d", streamType.c_str(), volume, ramp);

        EVirtualAudioSink sink = audioPolicyManagerInstance->getSinkType(streamType);
        if (sink != eVirtualSink_None)
        {
            isValidStream = true;
        }

        if ((volume >= INIT_VOLUME) && (volume <= MAX_VOLUME))
        {
            isValidVolume = true;
        }

        if (audioPolicyManagerInstance->getStreamActiveStatus(streamType))
        {
            isStreamActive = true;
        }

        if (isValidStream && isValidVolume)
        {
            if (isStreamActive)
            {
                if (!audioPolicyManagerInstance->setVolume(sink, volume, \
                    audioPolicyManagerInstance->getMixerType(streamType), ramp))
                    PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                        "_setInputVolume: failed mixer call");
            }
            status = true;
            audioPolicyManagerInstance->updateCurrentVolume(streamType, volume);
            audioPolicyManagerInstance->notifyInputVolume(sink, volume, ramp);
            PM_LOG_INFO (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "Volume updated successfully");
        }

        if (status)
        {
            pbnjson::JValue setInputVolumeResponse = pbnjson::Object();
            setInputVolumeResponse.put("returnValue", true);
            setInputVolumeResponse.put("volume", volume);
            setInputVolumeResponse.put("streamType", streamType);
            reply = setInputVolumeResponse.stringify();
            audioPolicyManagerInstance->notifyGetVolumeSubscribers(streamType, volume);
        }
        else
        {
            if (!isValidStream)
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod Unknown Stream");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
            else if (!isValidVolume)
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Volume Not in Range");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "Volume Not in Range");
            }
            else
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod internal error");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "AudioPolicyManager instance Null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioPolicyManager::_setAppVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: _setAppVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(streamType, string), PROP(volume, integer),
                                                           PROP(mediaId, string)) REQUIRED_3(streamType, volume, mediaId)));

    std::string reply;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;

    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (audioPolicyManagerInstance)
    {
        bool status = false;
        bool isValidVolume = false;
        bool isValidStream = false;
        bool isStreamActive = false;
        int volume = 0;
        std::string streamType;
        std::string mediaId;

        msg.get("streamType", streamType);
        msg.get("volume", volume);
        msg.get("mediaId", mediaId);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
            "got sink = %s , vol = %d , mediaId = %s", streamType.c_str(), volume, mediaId.c_str());

        EVirtualAudioSink sink = audioPolicyManagerInstance->getSinkType(streamType);
        if (sink != eVirtualSink_None)
        {
            isValidStream = true;
        }

        if ((volume >= INIT_VOLUME) && (volume <= MAX_VOLUME))
        {
            isValidVolume = true;
        }

        if (audioPolicyManagerInstance->getStreamActiveStatus(streamType))
        {
            isStreamActive = true;
        }

        if (isValidStream && isValidVolume)
        {
            if (audioPolicyManagerInstance->storeAppVolume(mediaId, volume, sink))
            {
                if (!audioPolicyManagerInstance->setAppVolume(mediaId, volume, sink, audioPolicyManagerInstance->getMixerType(streamType), true))
                    PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "setAppVolume: failed mixer call");
                status = true;
            }
            //status = true;
            //audioPolicyManagerInstance->updateCurrentVolume(streamType, volume);
            //audioPolicyManagerInstance->notifyInputVolume(sink, volume, ramp);
            //PM_LOG_INFO (MSGID_POLICY_MANAGER, INIT_KVCOUNT, "setAppVolume updated successfully");
        }

        if (status)
        {
            pbnjson::JValue setAppVolumeResponse = pbnjson::Object();
            setAppVolumeResponse.put("returnValue", true);
            setAppVolumeResponse.put("mediaId", mediaId);
            setAppVolumeResponse.put("volume", volume);
            setAppVolumeResponse.put("streamType", streamType);
            reply = setAppVolumeResponse.stringify();
        }
        else
        {
            if (!isValidStream)
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Audiod Unknown Stream");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
            else if (!isValidVolume)
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Volume Not in Range");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "Volume Not in Range");
            }
            else
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Audiod internal error");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager instance Null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioPolicyManager::_setSourceInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: _setInputVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(sourceType, string), PROP(volume, integer),
                                                           PROP(ramp, boolean)) REQUIRED_2(sourceType, volume)));

    std::string reply;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;

    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (audioPolicyManagerInstance)
    {
        bool status = false;
        bool isValidVolume = false;
        bool isValidStream = false;
        bool isStreamActive = false;
        int volume = 0;
        std::string streamType;
        bool ramp = false;
        msg.get ("sourceType", streamType);
        msg.get ("volume", volume);
        if (!msg.get ("ramp", ramp))
        {
            ramp = false;
        }
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
            "got sink = %s , vol = %d", streamType.c_str(), volume);

        EVirtualSource source = audioPolicyManagerInstance->getSourceType(streamType);
        if (source != eVirtualSource_None)
        {
            isValidStream = true;
        }

        if ((volume >= INIT_VOLUME) && (volume <= MAX_VOLUME))
        {
            isValidVolume = true;
        }

        if (audioPolicyManagerInstance->getSourceActiveStatus(streamType))
        {
            isStreamActive = true;
        }

        if (isValidStream && isValidVolume)
        {
            if (isStreamActive)
            {
                if (!audioPolicyManagerInstance->setVolume(source, volume, \
                    audioPolicyManagerInstance->getMixerTypeofSource(streamType), ramp))
                    PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                        "_setSourceInputVolume: failed mixer call");
            }
            status = true;
            audioPolicyManagerInstance->updateCurrentVolumeForSource(streamType, volume);
            PM_LOG_INFO (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "Volume updated successfully");
        }
        if (status)
        {
            pbnjson::JValue setInputVolumeResponse = pbnjson::Object();
            setInputVolumeResponse.put("returnValue", true);
            setInputVolumeResponse.put("volume", volume);
            setInputVolumeResponse.put("sourceType", streamType);
            reply = setInputVolumeResponse.stringify();
            audioPolicyManagerInstance->notifyGetSourceVolumeSubscribers(streamType, volume);
        }
        else
        {
            if (!isValidStream)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod Unknown Stream");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
            else if (!isValidVolume)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Volume Not in Range");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "Volume Not in Range");
            }
            else
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod internal error");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "AudioPolicyManager instance Null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioPolicyManager::_getInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(streamType, string), PROP(subscribe, boolean)) REQUIRED_1(streamType)));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg.parse failed");
        return true;
    }
    bool subscribed = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    std::string streamType;
    CLSError lserror;
    int volume = 0;
    msg.get("subscribe", subscribed);
    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (msg.get("streamType", streamType))
    {
        if (audioPolicyManagerInstance)
        {
            EVirtualAudioSink audioSink = audioPolicyManagerInstance->getSinkType(streamType);
            if (IsValidVirtualSink(audioSink))
            {
                if (LSMessageIsSubscription (message))
                {
                    if(!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
                    {
                        lserror.Print(__FUNCTION__, __LINE__);
                        PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
                        return true;
                    }
                }
                pbnjson::JValue returnPayload = pbnjson::Object();
                volume = audioPolicyManagerInstance->getCurrentVolume(streamType);
                returnPayload.put("subscribed", subscribed);
                returnPayload.put("streamType", streamType);
                returnPayload.put("volume", volume);
                returnPayload.put("returnValue", true);
                reply = returnPayload.stringify();
            }
            else
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: Unknown stream type");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,"AudioPolicyManager: audioPolicyManagerInstance is null");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager:Missing stream type");
        reply =  MISSING_PARAMETER_ERROR(streamType, string);
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}


bool AudioPolicyManager::_getSourceInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(sourceType, string), PROP(subscribe, boolean)) REQUIRED_1(sourceType)));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg.parse failed");
        return true;
    }
    bool subscribed = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    std::string streamType;
    CLSError lserror;
    int volume = 0;
    msg.get("subscribe", subscribed);
    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (msg.get("sourceType", streamType))
    {
        if (audioPolicyManagerInstance)
        {
            EVirtualSource audioSource = audioPolicyManagerInstance->getSourceType(streamType);
            if (IsValidVirtualSource(audioSource))
            {
                if (LSMessageIsSubscription (message))
                {
                    if(!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
                    {
                        lserror.Print(__FUNCTION__, __LINE__);
                        PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
                        return true;
                    }
                }
                pbnjson::JValue returnPayload = pbnjson::Object();
                volume = audioPolicyManagerInstance->getCurrentVolumeOfSource(streamType);
                returnPayload.put("subscribed", subscribed);
                returnPayload.put("sourceType", streamType);
                returnPayload.put("volume", volume);
                returnPayload.put("returnValue", true);
                reply = returnPayload.stringify();
            }
            else
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: Unknown stream type");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,"AudioPolicyManager: audioPolicyManagerInstance is null");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager:Missing stream type");
        reply =  MISSING_PARAMETER_ERROR(streamType, string);
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

void AudioPolicyManager::notifyGetVolumeSubscribers(const std::string& streamType, const int& volume)
{
    PM_LOG_DEBUG("AudioPolicyManager notifyGetVolumeSubscribers with streamType:%s volume:%d", streamType.c_str(), volume);
    pbnjson::JValue returnPayload = pbnjson::Object();
    returnPayload.put("subscribed", true);
    returnPayload.put("streamType", streamType);
    returnPayload.put("volume", volume);
    returnPayload.put("returnValue", true);
    CLSError lserror;
    LSHandle *lsHandle = GetPalmService ();
    if (! LSSubscriptionReply(lsHandle, AUDIOD_API_GET_INPUT_VOLUME, returnPayload.stringify().c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Notify error");
    }
}

void AudioPolicyManager::notifyGetSourceVolumeSubscribers(const std::string& streamType, const int& volume)
{
    PM_LOG_DEBUG("AudioPolicyManager notifyGetSourceVolumeSubscribers with streamType:%s volume:%d", streamType.c_str(), volume);
    pbnjson::JValue returnPayload = pbnjson::Object();
    returnPayload.put("subscribed", true);
    returnPayload.put("sourceType", streamType);
    returnPayload.put("volume", volume);
    returnPayload.put("returnValue", true);
    CLSError lserror;
    LSHandle *lsHandle = GetPalmService ();
    if (! LSSubscriptionReply(lsHandle, AUDIOD_API_GET_SOURCE_INPUT_VOLUME, returnPayload.stringify().c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Notify error");
    }
}

std::string AudioPolicyManager::getStreamStatus(const std::string& streamType, bool subscribed)
{
    PM_LOG_DEBUG("getStreamStatus streamType %s subscribed %d", streamType.c_str(), (int)subscribed);
    pbnjson::JValue streamObjectArray = pbnjson::Array();
    pbnjson::JObject streamObject = pbnjson::JObject();
    pbnjson::JObject finalString = pbnjson::JObject();
    for (auto &elements : mVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            streamObject = pbnjson::JObject();
            streamObject.put("streamType", elements.streamType);
            streamObject.put("muteStatus", elements.muteStatus);
            streamObject.put("inputVolume", elements.currentVolume);
            streamObject.put("sink", elements.sink);
            streamObject.put("source", elements.source);
            streamObject.put("policyStatus", elements.isPolicyInProgress);
            streamObject.put("activeStatus", elements.isStreamActive);
            streamObjectArray.append(streamObject);
        }
    }
    finalString = pbnjson::JObject{{"streamObject", streamObjectArray}};
    finalString.put("returnValue", true);
    finalString.put("subscribed", subscribed);
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "getStreamStatus returning payload = %s", finalString.stringify().c_str());
    return finalString.stringify();
}

std::string AudioPolicyManager::getSourceStatus(const std::string& streamType, bool subscribed)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "getStreamStatus streamType %s subscribed %d", streamType.c_str(), (int)subscribed);
    pbnjson::JValue streamObjectArray = pbnjson::Array();
    pbnjson::JObject streamObject = pbnjson::JObject();
    pbnjson::JObject finalString = pbnjson::JObject();
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (elements.streamType == streamType)
        {
            streamObject = pbnjson::JObject();
            streamObject.put("sourceType", elements.streamType);
            streamObject.put("muteStatus", elements.muteStatus);
            streamObject.put("inputVolume", elements.currentVolume);
            streamObject.put("sink", elements.sink);
            streamObject.put("source", elements.source);
            streamObject.put("policyStatus", elements.isPolicyInProgress);
            streamObject.put("activeStatus", elements.isStreamActive);
            streamObjectArray.append(streamObject);
        }
    }
    finalString = pbnjson::JObject{{"sourceObject", streamObjectArray}};
    finalString.put("returnValue", true);
    finalString.put("subscribed", subscribed);
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "getSourceStatus returning payload = %s", finalString.stringify().c_str());
    return finalString.stringify();
}

std::string AudioPolicyManager::getSourceStatus(bool subscribed)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "getSourceStatus subscribed %d", subscribed);
    pbnjson::JValue streamObjectArray = pbnjson::Array();
    pbnjson::JObject streamObject = pbnjson::JObject();
    pbnjson::JObject finalString = pbnjson::JObject();
    for (auto &elements : mSourceVolumePolicyInfo)
    {
        if (true == elements.isStreamActive)
        {
            streamObject = pbnjson::JObject();
            streamObject.put("sourceType", elements.streamType);
            streamObject.put("muteStatus", elements.muteStatus);
            streamObject.put("inputVolume", elements.currentVolume);
            streamObject.put("sink", elements.sink);
            streamObject.put("source", elements.source);
            streamObject.put("policyStatus", elements.isPolicyInProgress);
            streamObject.put("activeStatus", elements.isStreamActive);
            streamObjectArray.append(streamObject);
        }
    }
    finalString = pbnjson::JObject{{"sourceObject", streamObjectArray}};
    finalString.put("returnValue", true);
    finalString.put("subscribed", subscribed);
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "getSourceStatus returning payload = %s", finalString.stringify().c_str());
    return finalString.stringify();
}

bool AudioPolicyManager::_getSourceStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(sourceType, string), PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg parse failed");
        return true;
    }
    bool subscribed = false;
    bool addSubscribers = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    std::string streamType;
    CLSError lserror;
    msg.get("subscribe", subscribed);

    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if(msg.get("sourceType", streamType))
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "sourceType is present %s", streamType.c_str());
        if (audioPolicyManagerInstance)
        {
            EVirtualSource audioSource = audioPolicyManagerInstance->getSourceType(streamType);
            if (IsValidVirtualSource(audioSource))
            {
                addSubscribers = true;
                reply = audioPolicyManagerInstance->getSourceStatus(streamType, subscribed);
            }
            else
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: Unknown stream type");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,"AudioPolicyManager: audioPolicyManagerInstance is null");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
        }
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "reply : %s", reply.c_str());
    }
    else
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "_getSourceStatus request received for all active streams");
        addSubscribers = true;
        reply = audioPolicyManagerInstance->getSourceStatus(subscribed);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "reply: %s", reply.c_str());
    }
    if (addSubscribers)
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "_getSourceStatus request - adding the subscribers");
        if (LSMessageIsSubscription (message))
        {
            if(!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
                PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
                return true;
            }
        }
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

std::string AudioPolicyManager::getStreamStatus(bool subscribed)
{
    PM_LOG_DEBUG("getStreamStatus subscribed %d", (int)subscribed);
    pbnjson::JValue streamObjectArray = pbnjson::Array();
    pbnjson::JObject streamObject = pbnjson::JObject();
    pbnjson::JObject finalString = pbnjson::JObject();
    for (auto &elements : mVolumePolicyInfo)
    {
        if (true == elements.isStreamActive)
        {
            streamObject = pbnjson::JObject();
            streamObject.put("streamType", elements.streamType);
            streamObject.put("muteStatus", elements.muteStatus);
            streamObject.put("inputVolume", elements.currentVolume);
            streamObject.put("sink", elements.sink);
            streamObject.put("source", elements.source);
            streamObject.put("policyStatus", elements.isPolicyInProgress);
            streamObject.put("activeStatus", elements.isStreamActive);
            streamObjectArray.append(streamObject);
        }
    }
    finalString = pbnjson::JObject{{"streamObject", streamObjectArray}};
    finalString.put("returnValue", true);
    finalString.put("subscribed", subscribed);
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                "getStreamStatus returning payload = %s", finalString.stringify().c_str());
    return finalString.stringify();
}

bool AudioPolicyManager::_getStreamStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(streamType, string), PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle))
    {
        PM_LOG_CRITICAL(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT, "msg parse failed");
        return true;
    }
    bool subscribed = false;
    bool addSubscribers = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    std::string streamType;
    CLSError lserror;
    msg.get("subscribe", subscribed);

    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if(msg.get("streamType", streamType))
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "streamType is present %s", streamType.c_str());
        if (audioPolicyManagerInstance)
        {
            EVirtualAudioSink audioSink = audioPolicyManagerInstance->getSinkType(streamType);
            if (IsValidVirtualSink(audioSink))
            {
                addSubscribers = true;
                reply = audioPolicyManagerInstance->getStreamStatus(streamType, subscribed);
            }
            else
            {
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: Unknown stream type");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,"AudioPolicyManager: audioPolicyManagerInstance is null");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod Internal Error");
        }
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "reply : %s", reply.c_str());
    }
    else
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "getStreamStatus request received for all active streams");
        addSubscribers = true;
        reply = audioPolicyManagerInstance->getStreamStatus(subscribed);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "reply: %s", reply.c_str());
    }
    if (addSubscribers)
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "getStreamStatus request - adding the subscribers");
        if (LSMessageIsSubscription (message))
        {
            if(!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
                PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
                return true;
            }
        }
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

//TODO: is this required?
std::vector<std::string> physicalSources = {
    "pcm_input"
};

bool AudioPolicyManager::_muteSource(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    //To be changed to virtual source
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: _muteSource");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(sourceType, string), PROP(mute, boolean)) REQUIRED_2(sourceType,mute)));
    std::string reply ;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;
    std::string streamType;
    bool mute = false;
    bool isValidStream = false;
    bool isStreamActive = false;
    bool retVal = false;
    AudioPolicyManager *AudioPolicyManagerObj = AudioPolicyManager::getAudioPolicyManagerInstance();

    msg.get ("sourceType", streamType);
    msg.get ("mute", mute);

    auto it = std::find (physicalSources.begin(), physicalSources.end(), streamType);
    if (it != physicalSources.end())
    {
        retVal = AudioPolicyManagerObj->mObjAudioMixer->setPhysicalSourceMute(streamType.c_str(), mute);
        if (retVal)
        {
            pbnjson::JValue responseObj = pbnjson::Object();
            responseObj.put("returnValue", true);
            responseObj.put("sourceType", streamType);
            responseObj.put("mute", mute);
            reply = responseObj.stringify();
        }
        else
        {
            PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod internal error");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
        }
    }
    else
    {
        EVirtualSource sourceId = AudioPolicyManagerObj->getSourceType(streamType);
        if (sourceId != eVirtualSource_None)
        {
            isValidStream = true;
        }
        if (isValidStream && AudioPolicyManagerObj->getSourceActiveStatus(streamType))
        {
            retVal = AudioPolicyManagerObj->mObjAudioMixer->setVirtualSourceMute(sourceId, mute);
        }
        if (retVal)
        {
            AudioPolicyManagerObj->updateMuteStatusForSource(streamType, mute);
            //Mute status change is notified via getStatus subscription
            std::string payload = AudioPolicyManagerObj->getSourceStatus(streamType, true);
            AudioPolicyManagerObj->notifyGetSourceStatusSubscribers(payload);

            pbnjson::JValue setInputVolumeResponse = pbnjson::Object();
            setInputVolumeResponse.put("returnValue", true);
            setInputVolumeResponse.put("mute", mute);
            setInputVolumeResponse.put("sourceType", streamType);
            reply = setInputVolumeResponse.stringify();
        }
        else
        {
            if (!isValidStream)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                        "Audiod Unknown Stream");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
            else
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod internal error");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

void AudioPolicyManager::notifyGetStreamStatusSubscribers(const std::string& payload) const
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
               "AudioPolicyManager notifyGetStreamStatusSubscribers with payload = %s", payload.c_str());
    CLSError lserror;
    LSHandle *lsHandle = GetPalmService();
    if (!LSSubscriptionReply(lsHandle, AUDIOD_API_GET_STREAM_STATUS, payload.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Notify error");
    }
}

void AudioPolicyManager::notifyGetSourceStatusSubscribers(const std::string& payload) const
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
               "AudioPolicyManager notifyGetSourceStatusSubscribers with payload = %s", payload.c_str());
    CLSError lserror;
    LSHandle *lsHandle = GetPalmService();
    if (!LSSubscriptionReply(lsHandle, AUDIOD_API_GET_SOURCE_STATUS, payload.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_CRITICAL(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Notify error");
    }
}

void AudioPolicyManager::notifyInputVolume(EVirtualAudioSink audioSink, const int& volume, const bool& ramp)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
               "AudioPolicyManager notifyInputVolume audioSink = %d volume = %d", (int)audioSink, volume);
    events::EVENT_INPUT_VOLUME_T eventInputVolume;
    eventInputVolume.eventName = utils::eEventInputVolume;
    eventInputVolume.audioSink = audioSink ;
    eventInputVolume.volume = volume;
    eventInputVolume.ramp = ramp;
    if (mObjModuleManager)
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventInputVolume);
}

bool AudioPolicyManager::_setMediaInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: _setMediaInputVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(volume, integer), PROP(sessionId, integer)) REQUIRED_1(volume)));
    std::string reply ;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;

    AudioPolicyManager *audioPolicyManagerInstance = AudioPolicyManager::getAudioPolicyManagerInstance();
    if (audioPolicyManagerInstance)
    {
        bool status = false;
        bool isValidVolume = false;
        bool isValidStream = false;
        bool isStreamActive = false;
        bool isValidSessionId = false;
        int volume = 0;
        bool ramp = false;
        std::string streamType = " ";
        int sessionId = 0;

        msg.get ("volume", volume);
        if (!msg.get ("sessionId", sessionId))
            sessionId = 0;

        if (DISPLAY_ONE == sessionId)
        {
            streamType = DEFAULT_ONE;
        }
        else if (DISPLAY_TWO == sessionId)
        {
            streamType = DEFAULT_TWO;
        }
        else
            streamType = DEFAULT_ONE;

        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
            "got sink = %s , vol = %d", streamType.c_str(), volume);

        EVirtualAudioSink sink = audioPolicyManagerInstance->getSinkType(streamType);
        if (sink != eVirtualSink_None)
        {
            isValidStream = true;
        }

        if ((volume >= INIT_VOLUME) && (volume <= MAX_VOLUME))
        {
            isValidVolume = true;
        }

        if (audioPolicyManagerInstance->getStreamActiveStatus(streamType))
        {
            isStreamActive = true;
        }
        if ((DISPLAY_ONE == sessionId) || (DISPLAY_TWO == sessionId))
        {
            isValidSessionId = true;
        }

        if (isValidStream && isValidVolume && isValidSessionId)
        {
            if (isStreamActive)
            {
                if (!audioPolicyManagerInstance->setVolume(sink, volume, \
                    audioPolicyManagerInstance->getMixerType(streamType), ramp))
                    PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                        "_setMediaInputVolume: failed mixer call");
            }
            status = true;
            audioPolicyManagerInstance->updateCurrentVolume(streamType, volume);
            audioPolicyManagerInstance->notifyInputVolume(sink, volume, ramp);

            if (DISPLAY_ONE == sessionId)
            {
                EVirtualAudioSink mediaSink = audioPolicyManagerInstance->getSinkType("pmedia");
                EVirtualAudioSink defaultappSink = audioPolicyManagerInstance->getSinkType("pdefaultapp");
                if (audioPolicyManagerInstance->getStreamActiveStatus("pmedia"))
                {
                    if (!audioPolicyManagerInstance->setVolume(mediaSink, volume, audioPolicyManagerInstance->getMixerType("pmedia"), ramp))
                        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                            "AudioPolicyManager::_setMediaInputVolume volume could not be set for stream pmedia");
                }
                if (audioPolicyManagerInstance->getStreamActiveStatus("pdefaultapp"))
                {
                    if (!audioPolicyManagerInstance->setVolume(defaultappSink, volume, audioPolicyManagerInstance->getMixerType("pdefaultapp"), ramp))
                        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                            "AudioPolicyManager::_setMediaInputVolume volume could not be set for stream pdefaultapp");

                }

                audioPolicyManagerInstance->updateCurrentVolume("pmedia", volume);
                audioPolicyManagerInstance->updateCurrentVolume("pdefaultapp", volume);
                audioPolicyManagerInstance->notifyInputVolume(defaultappSink, volume, ramp);
                audioPolicyManagerInstance->notifyInputVolume(mediaSink, volume, ramp);
            }
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Volume updated successfully");
        }

        if (status)
        {
            reply = STANDARD_JSON_SUCCESS;
            audioPolicyManagerInstance->notifyGetVolumeSubscribers(streamType, volume);
        }
        else
        {
            if (!isValidStream)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod Unknown Stream");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM, "Audiod Unknown Stream");
            }
            else if (!isValidVolume)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Volume Not in Range");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "Volume Not in Range");
            }
            else if (!isValidSessionId)
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
                reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
            }
            else
            {
                PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "Audiod internal error");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
                    "AudioPolicyManager instance Null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

static LSMethod InputVolumeMethods[] = {
    {"setInputVolume", AudioPolicyManager::_setInputVolume},
    {"getInputVolume", AudioPolicyManager::_getInputVolume},
    {"getStreamStatus", AudioPolicyManager::_getStreamStatus},
    {"getSourceStatus", AudioPolicyManager::_getSourceStatus},
    {"muteSource", AudioPolicyManager::_muteSource},
    {"muteSink", AudioPolicyManager::_muteSink},
    {"setSourceInputVolume", AudioPolicyManager::_setSourceInputVolume},
    {"getSourceInputVolume", AudioPolicyManager::_getSourceInputVolume},
    {"setAppVolume", AudioPolicyManager::_setAppVolume},
    { },
};

static LSMethod MediaInputVolumeMethods[] = {
    {"setVolume", AudioPolicyManager::_setMediaInputVolume},
    { },
};
//API functions end

//Class init start
AudioPolicyManager* AudioPolicyManager::mAudioPolicyManager = nullptr;

AudioPolicyManager* AudioPolicyManager::getAudioPolicyManagerInstance()
{
    return mAudioPolicyManager;
}

AudioPolicyManager::AudioPolicyManager(ModuleConfig* const pConfObj):mObjModuleManager(nullptr),\
                                                                     mObjPolicyInfoParser(nullptr),\
                                                                     mObjAudioMixer(nullptr)
{
    PM_LOG_DEBUG("AudioPolicyManager: constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventSinkStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventSourceStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventMixerStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventCurrentInputVolume);
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mObjModuleManager is null");
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    readPolicyInfo();
}

AudioPolicyManager::~AudioPolicyManager()
{
    PM_LOG_DEBUG("AudioPolicyManager: destructor");
    if (mObjPolicyInfoParser)
    {
        delete mObjPolicyInfoParser;
        mObjPolicyInfoParser = nullptr;
    }
}

void AudioPolicyManager::initialize()
{
    if (mAudioPolicyManager)
    {
        CLSError lserror;
        bool bRetVal;
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager module initialize is successful");
        bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/", InputVolumeMethods, nullptr, &lserror);
        if (!bRetVal || !LSCategorySetData(GetPalmService(), "/", mAudioPolicyManager, &lserror))
        {
           PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
            "%s: Registering Service for '%s' category failed", __FUNCTION__, "/");
           lserror.Print(__FUNCTION__, __LINE__);
        }

        bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/media", MediaInputVolumeMethods, nullptr, &lserror);
        if (!bRetVal || !LSCategorySetData(GetPalmService(), "/media", mAudioPolicyManager, &lserror))
        {
           PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, \
            "%s: Registering Service for '%s' category failed", __FUNCTION__, "/media");
           lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mAudioPolicyManager is nullptr");
}

void AudioPolicyManager::deInitialize()
{
    PM_LOG_DEBUG("AudioPolicyManager deInitialize()");
    if (mAudioPolicyManager)
    {
        delete mAudioPolicyManager;
        mAudioPolicyManager = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mAudioPolicyManager is nullptr");
}

void AudioPolicyManager::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {
        case utils::eEventSinkStatus:
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventSinkStatus");
            events::EVENT_SINK_STATUS_T *sinkStatusEvent = (events::EVENT_SINK_STATUS_T*)event;
            eventSinkStatus(sinkStatusEvent->source, sinkStatusEvent->sink, sinkStatusEvent->audioSink, sinkStatusEvent->sinkStatus, sinkStatusEvent->mixerType,sinkStatusEvent->sinkIndex, \
            sinkStatusEvent->appName);
        }
        break;
        case utils::eEventSourceStatus:
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventSourceStatus");
            events::EVENT_SOURCE_STATUS_T *sourceStatusEvent = (events::EVENT_SOURCE_STATUS_T*)event;
            eventSourceStatus(sourceStatusEvent->source, sourceStatusEvent->sink, sourceStatusEvent->audioSource, sourceStatusEvent->sourceStatus, sourceStatusEvent->mixerType);
        }
        break;
        case utils::eEventMixerStatus:
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "handleEvent:: eEventMixerStatus");
            events::EVENT_MIXER_STATUS_T *mixerStatusEvent = (events::EVENT_MIXER_STATUS_T*)event;
            eventMixerStatus(mixerStatusEvent->mixerStatus, mixerStatusEvent->mixerType);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}
