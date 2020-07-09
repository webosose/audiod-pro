/* @@@LICENSE
*
*      Copyright (c) 2020 LG Electronics Company.
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

//Event handling starts
void AudioPolicyManager::eventSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventSinkStatus source:%s sink:%s sinkId:%d sinkStatus:%d mixerType:%d",\
        source.c_str(), sink.c_str(), audioSink, sinkStatus, mixerType);
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
            // Will un-comment the same in next patch, after implementing these functions.
            #if 0
            if (setVolume(audioSink, currentVolume, mixerType, ramp))
                notifyGetVolumeSubscribers(streamType, currentVolume);
            payload = getStreamStatus(true);
            notifyGetStreamStatusSubscribers(payload)
            setVolume(audioSink, currentVolume, mixerType, ramp);
            #endif
            applyVolumePolicy(audioSink, streamType, priority);
        }
        else if (utils::eSinkClosed == sinkStatus)
        {
            // Will un-comment the same in next patch, after implementing these functions.
            #if 0
            if (setVolume(audioSink, INIT_VOLUME, mixerType, ramp))
                notifyGetVolumeSubscribers(streamType, INIT_VOLUME);
            payload = getStreamStatus(streamType, true);
            notifyGetStreamStatusSubscribers(payload);
            #endif
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

void AudioPolicyManager::eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventMixerStatus mixerStatus:%d mixerType:%d", mixerStatus, mixerType);
    if (utils::ePulseMixer == mixerType)
        initStreamVolume();
}

void AudioPolicyManager::eventCurrentInputVolume(EVirtualAudioSink audioSink, const int& volume)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::eventCurrentInputVolume audioSink:%d volume:%d", audioSink, volume);
    std::string streamType = getStreamType(audioSink);
    updateCurrentVolume(streamType, volume);
}
//Event handling ends

//Utility functions start
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
            pbnjson::JValue policyInfo = mObjPolicyInfoParser->getVolumePolicyInfo();
            if (initializePolicyInfo(policyInfo))
                PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializePolicyInfo success");
            else
                PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializePolicyInfo failed");
        }
        else
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Unable to load VolumePolicyJsonConfig");
    }
    else
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mObjPolicyInfoParser is null");
}

bool AudioPolicyManager::initializePolicyInfo(const pbnjson::JValue& policyInfo)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::initializePolicyInfo");
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
            mVolumePolicyInfo.push_back(stPolicyInfo);
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
    }
}

void AudioPolicyManager::printPolicyInfo()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::printPolicyInfo:\n");
    for (const auto& elements : mVolumePolicyInfo)
    {
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "*************%s*************", elements.streamType.c_str());
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "policyVolume:%d", elements.policyVolume);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "priority:%d groupId:%d defaultVolume:%d",\
            elements.priority, elements.groupId, elements.defaultVolume);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "maxVolume:%d minVolume:%d volumeAdjustable:%d",\
             elements.maxVolume, elements.minVolume, elements.volumeAdjustable);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "currentVolume:%d muteStatus:%d source:%s sinkId:%s mixerType:%d ramp:%d",\
            elements.currentVolume, elements.muteStatus, elements.source.c_str(),\
            elements.sink.c_str(), elements.mixerType, (int)elements.ramp);
        PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
            "isPolicyInProgress:%d activeStatus:%d category:%s",\
            elements.isPolicyInProgress, elements.isStreamActive, elements.category.c_str());
    }
}

void AudioPolicyManager::printActivePolicyInfo()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager::printActivePolicyInfo:\n");
    for (const auto& elements : mVolumePolicyInfo)
    {
        if (elements.isPolicyInProgress)
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "*************%s*************", elements.streamType.c_str());
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "policyVolume:%d", elements.policyVolume);
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "priority:%d groupId:%d defaultVolume:%d",\
                elements.priority, elements.groupId, elements.defaultVolume);
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "maxVolume:%d minVolume:%d volumeAdjustable:%d",\
                 elements.maxVolume, elements.minVolume, elements.volumeAdjustable);
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "currentVolume:%d muteStatus:%d source:%s sinkId:%s mixerType:%d ramp:%d",\
                elements.currentVolume, elements.muteStatus, elements.source.c_str(),\
                elements.sink.c_str(), elements.mixerType, (int)elements.ramp);
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
                "isPolicyInProgress:%d activeStatus:%d category:%s",\
                elements.isPolicyInProgress, elements.isStreamActive, elements.category.c_str());
        }
    }
}

std::string AudioPolicyManager::getStreamType(EVirtualAudioSink audioSink)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: getStreamType:%d", audioSink);
     utils::itMapSinkToStream it = mSinkToStream.find(audioSink);
     if (it != mSinkToStream.end())
         return it->second;
     else
         return "";
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

void AudioPolicyManager::createMapSinkToStream()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: createMapSinkToStream");
    mSinkToStream[ealerts] = "palerts";
    mSinkToStream[efeedback] = "pfeedback";
    mSinkToStream[eringtones] = "pringtones";
    mSinkToStream[emedia] = "pmedia";
    mSinkToStream[edefaultapp] = "pdefaultapp";
    mSinkToStream[eeffects] = "peffects";
    mSinkToStream[evoicerecognition] = "pvoicerecognition";
    mSinkToStream[etts] = "ptts";
    mSinkToStream[edefault1] = "default1";
    mSinkToStream[etts1] = "tts1";
    mSinkToStream[edefault2] = "default2";
    mSinkToStream[etts2] = "tts2";
}

void AudioPolicyManager::createMapStreamToSink()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager: createMapStreamToSink");
    mStreamToSink["palerts"] = ealerts;
    mStreamToSink["pfeedback"] = efeedback;
    mStreamToSink["pringtones"] = eringtones;
    mStreamToSink["pmedia"] = emedia;
    mStreamToSink["pdefaultapp"] = edefaultapp;
    mStreamToSink["peffects"] = eeffects;
    mStreamToSink["pvoicerecognition"] = evoicerecognition;
    mStreamToSink["ptts"] = etts;
    mStreamToSink["default1"] = edefault1;
    mStreamToSink["tts1"] = etts1;
    mStreamToSink["default2"] = edefault2;
    mStreamToSink["tts2"] = etts2;
}

void AudioPolicyManager::applyVolumePolicy(EVirtualAudioSink audioSink, const std::string& streamType, const int& priority)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:applyVolumePolicy sink:%d streamType:%s priority:%d",\
        audioSink, streamType.c_str(), priority);
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

void AudioPolicyManager::removeVolumePolicy(EVirtualAudioSink audioSink, const std::string& streamType, const int& priority)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:removeVolumePolicy sink:%d streamType:%s priority:%d",\
        audioSink, streamType.c_str(), priority);
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
                    if (isPolicyInProgress && !isHighPriorityStreamActive(policyPriority, activeStreams))
                    {
                        if (setVolume(getSinkType(policyStreamType), getCurrentVolume(policyStreamType),\
                            getMixerType(policyStreamType), isRampPolicyActive(policyStreamType)))
                            updatePolicyStatus(policyStreamType, false);
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
    updatePolicyStatus(streamType, false);
}

bool AudioPolicyManager::setVolume(EVirtualAudioSink audioSink, const int& volume, \
    utils::EMIXER_TYPE mixerType, bool ramp)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:setVolume:%d for sink:%d, ramp = %d", volume, audioSink, mixerType, ramp);
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
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
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

bool AudioPolicyManager::isHighPriorityStreamActive(const int& policyPriority, utils::vectorVirtualSink activeStreams)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT,\
        "AudioPolicyManager:isHighPriorityStreamActive priority:%d", policyPriority);
    int priority  = MAX_PRIORITY;
    for (const auto &it : activeStreams)
    {
        priority  = getPriority(getStreamType(it));
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
    // Will un-comment the same in next patch, after implementing these functions.
    //std::string payload = getStreamStatus(streamType, true);
    //notifyGetStreamStatusSubscribers(payload);
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
//Utility functions ends

//Class init start
AudioPolicyManager* AudioPolicyManager::mAudioPolicyManager = nullptr;

AudioPolicyManager* AudioPolicyManager::getAudioPolicyManagerInstance()
{
    return mAudioPolicyManager;
}

AudioPolicyManager::AudioPolicyManager():mObjModuleManager(nullptr),\
                                         mObjPolicyInfoParser(nullptr),\
                                         mObjAudioMixer(nullptr)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, true, utils::eEventSinkStatus);
        mObjModuleManager->subscribeModuleEvent(this, true, utils::eEventMixerStatus);
        mObjModuleManager->subscribeModuleEvent(this, true, utils::eEventCurrentInputVolume);
    }
    else
        PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "mObjModuleManager is null");
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    readPolicyInfo();
    createMapSinkToStream();
    createMapStreamToSink();
}

AudioPolicyManager::~AudioPolicyManager()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "AudioPolicyManager: destructor");
    if (!mObjPolicyInfoParser)
    {
        delete mObjPolicyInfoParser;
        mObjPolicyInfoParser = nullptr;
    }
}

void AudioPolicyManager::loadModuleAudioPolicyManager()
{
    if (!mAudioPolicyManager)
    {
        mAudioPolicyManager = new (std::nothrow)AudioPolicyManager();
        if (mAudioPolicyManager)
        {
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "load module AudioPolicyManager successful");
        }
        else
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "Could not load module AudioPolicyManager");
    }
}
//Class init ends

//Load/Unload module starts
int load_audio_policy_manager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "loading audio_policy_manager");
    AudioPolicyManager::loadModuleAudioPolicyManager();
    return 0;
}

void unload_audio_policy_manager()
{
    PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "unloading audio_policy_manager");
    if (AudioPolicyManager::mAudioPolicyManager)
    {
        delete AudioPolicyManager::mAudioPolicyManager;
        AudioPolicyManager::mAudioPolicyManager = nullptr;
    }
}
//Load/Unload module ends
