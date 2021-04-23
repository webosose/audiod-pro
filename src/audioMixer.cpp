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

#include "audioMixer.h"

AudioMixer* AudioMixer::getAudioMixerInstance()
{
    static AudioMixer mAudioMixerObj;
    return &mAudioMixerObj;
}

AudioMixer::AudioMixer():mObjUmiAudioMixer(nullptr), mObjPulseAudioMixer(nullptr), \
                         mUmiMixerStatus(false), mPulseMixerStatus(false)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                "AudioMixer: constructor");
    mActiveStreams.clear();
    mUmiStreams.clear();
    mPulseStreams.clear();
    if (!mObjUmiAudioMixer)
        mObjUmiAudioMixer = new (std::nothrow)umiaudiomixer(this);
    if (!mObjPulseAudioMixer)
        mObjPulseAudioMixer = new (std::nothrow)PulseAudioMixer(this);
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
}

AudioMixer::~AudioMixer()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: destructor");
}

//Audio mixer calls start//
bool AudioMixer::getUmiMixerReadyStatus()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: getUmiMixerStatus:%d", mUmiMixerStatus);
    return mUmiMixerStatus;
}

bool AudioMixer::getPulseMixerReadyStatus()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: getPulseMixerReadyStatus:%d", mPulseMixerStatus);
    return mPulseMixerStatus;
}

bool AudioMixer::readyToProgram()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: readyToProgram");
    return  mUmiMixerStatus && mPulseMixerStatus;
}

utils::vectorVirtualSink AudioMixer::getActiveStreams()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: getActiveStreams");
    return mActiveStreams;
}

bool AudioMixer::isStreamActive(EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: isStreamActive sinkId:%d", (int)sink);
    if (IsValidVirtualSink(sink))
    {
        utils::itVirtualSink it;
        it = std::find(mActiveStreams.begin(), mActiveStreams.end(), sink);
        if (it != mActiveStreams.end())
            return true;
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "%s Invalid virtual Sink", __FUNCTION__);
    return false;
}


utils::vectorVirtualSource AudioMixer::getActiveSources()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: getActiveSources");
    return mActiveSources;
}

bool AudioMixer::isSourceActive(EVirtualSource source)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: isSourceActive Source:%d", (int)source);
    if (IsValidVirtualSource(source))
    {
        utils::itVirtualSource it;
        it = std::find(mActiveSources.begin(), mActiveSources.end(), source);
        if (it != mActiveSources.end())
            return true;
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "%s Invalid virtual Source", __FUNCTION__);
    return false;
}

//Audio mixer calls end//

//AudioMixer util functions start//
void AudioMixer::removeAudioSink(EVirtualAudioSink audioSink, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_DEBUG("AudioMixer:: removeAudioSink");
    utils::itVirtualSink it;
    it = std::find(mActiveStreams.begin(), mActiveStreams.end(), audioSink);
    if(it != mActiveStreams.end())
        mActiveStreams.erase(it);

    if (utils::ePulseMixer == mixerType)
    {
        it = std::find(mPulseStreams.begin(), mPulseStreams.end(), audioSink);
        if(it != mPulseStreams.end())
            mPulseStreams.erase(it);
    }
    else if (utils::eUmiMixer == mixerType)
    {
        it = std::find(mUmiStreams.begin(), mUmiStreams.end(), audioSink);
        if(it != mUmiStreams.end())
            mUmiStreams.erase(it);
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
           "AudioMixer:: removeAudioSink invalid mixer type");
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: removeAudioSink active streams:%d pulse streams:%d umi streams:%d",\
        mActiveStreams.size(), mPulseStreams.size(), mUmiStreams.size());
}

void AudioMixer::removeAudioSource(EVirtualSource audioSource, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: removeAudioSource");
    utils::itVirtualSource it;
    it = std::find(mActiveSources.begin(), mActiveSources.end(), audioSource);
    if (it != mActiveSources.end())
        mActiveSources.erase(it);

    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: removeAudioSource active streams:%d", (int)mActiveStreams.size());
}

void AudioMixer::addAudioSink(EVirtualAudioSink audioSink, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_DEBUG("AudioMixer:: addAudioSink");
    mActiveStreams.push_back(audioSink);
    if (utils::ePulseMixer == mixerType)
        mPulseStreams.push_back(audioSink);
    else if (utils::eUmiMixer == mixerType)
        mUmiStreams.push_back(audioSink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
           "AudioMixer:: addAudioSink invalid mixer type");
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: addAudioSink active streams:%d pulse streams:%d umi streams:%d",\
        mActiveStreams.size(), mPulseStreams.size(), mUmiStreams.size());
}

void AudioMixer::addAudioSource(EVirtualSource audioSource, utils::EMIXER_TYPE mixerType)
{
    mActiveSources.push_back(audioSource);

    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: addAudioSource active sources:%d", (int)mActiveSources.size());
}

void AudioMixer::resetStreamInfo(utils::EMIXER_TYPE mixerType)
{
    PM_LOG_DEBUG("AudioMixer:: resetStreamInfo");
    utils::itVirtualSink it;
    if (utils::eUmiMixer == mixerType)
    {
        for (const auto& elements : mUmiStreams)
        {
            it = std::find(mActiveStreams.begin(), mActiveStreams.end(), elements);
            if(it != mActiveStreams.end())
                mActiveStreams.erase(it);
        }
        mUmiStreams.clear();
    }
    else if (utils::ePulseMixer == mixerType)
    {
        for (const auto& elements : mPulseStreams)
        {
            it = std::find(mActiveStreams.begin(), mActiveStreams.end(), elements);
            if(it != mActiveStreams.end())
                mActiveStreams.erase(it);
        }
        mPulseStreams.clear();
    }
    else
       PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
           "AudioMixer:: resetStreamInfo invalid mixer type");
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer:: resetStreamInfo active streams:%d pulse streams:%d umi streams:%d",\
        mActiveStreams.size(), mPulseStreams.size(), mUmiStreams.size());
}

//AudioMixer util functions end//

//Mixer calbacks start//
void AudioMixer::callBackMixerStatus(const bool& mixerStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer::callBackMixerStatus status:%d mixerType:%d", mixerStatus, mixerType);
    if (utils::ePulseMixer == mixerType)
        mPulseMixerStatus = mixerStatus;
    else if (utils::eUmiMixer == mixerType)
        mUmiMixerStatus = mixerStatus;
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "%s Invalid mixer type", __FUNCTION__);
    if (!mixerStatus)
        resetStreamInfo(mixerType);
    if (mObjModuleManager)
    {
        events::EVENT_MIXER_STATUS_T eventMixerStatus;
        eventMixerStatus.eventName = utils::eEventMixerStatus;
        eventMixerStatus.mixerStatus = mixerStatus ;
        eventMixerStatus.mixerType = mixerType;
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventMixerStatus);
    }
}

void AudioMixer::callBackSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
              utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                "callBackSinkStatus::source:%s sink:%s sinkId:%d sinkStatus:%d mixerType:%d",\
                source.c_str(), sink.c_str(), (int)audioSink, (int)sinkStatus, (int)mixerType);
    if (IsValidVirtualSink(audioSink))
    {
        if (utils::eSinkOpened == sinkStatus)
            addAudioSink(audioSink, mixerType);
        else if (utils::eSinkClosed == sinkStatus)
            removeAudioSink(audioSink, mixerType);
        if (mObjModuleManager)
        {
            events::EVENT_SINK_STATUS_T eventSinkStatus;
            eventSinkStatus.eventName = utils::eEventSinkStatus;
            eventSinkStatus.source = source;
            eventSinkStatus.sink = sink;
            eventSinkStatus.audioSink = audioSink;
            eventSinkStatus.sinkStatus = sinkStatus;
            eventSinkStatus.mixerType = mixerType;
            mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventSinkStatus);
        }
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "%s Invalid virtual Sink", __FUNCTION__);
}


void AudioMixer::callBackSourceStatus(const std::string& source, const std::string& sink, EVirtualSource audioSource, \
            utils::ESINK_STATUS sourceStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "callBackSourceStatus::source:%s sink:%s sourceId:%d sinkStatus:%d mixerType:%d",\
        source.c_str(), sink.c_str(), (int)audioSource, (int)sourceStatus, (int)mixerType);
    if (IsValidVirtualSource(audioSource))
    {
        if (utils::eSinkOpened == sourceStatus)
        {
            addAudioSource(audioSource, mixerType);
        }
        else if (utils::eSinkClosed == sourceStatus)
        {
            removeAudioSource(audioSource, mixerType);
        }
        if (mObjModuleManager)
        {
            events::EVENT_SOURCE_STATUS_T eventSourceStatus;
            eventSourceStatus.eventName = utils::eEventSourceStatus;
            eventSourceStatus.source = source;
            eventSourceStatus.sink = sink;
            eventSourceStatus.audioSource = audioSource;
            eventSourceStatus.sourceStatus = sourceStatus;
            eventSourceStatus.mixerType = mixerType;
            mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventSourceStatus);
        }
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "%s Invalid virtual Sink", __FUNCTION__);
}

void AudioMixer::callBackDeviceConnectionStatus(const std::string &deviceName, utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "callBackDeviceConnectionStatus::deviceName:%s deviceStatus:%d mixerType:%d",\
        deviceName.c_str(), (int)deviceStatus, (int)mixerType);
    if (deviceName == "pcm_headphone")
    {
        PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "pcm_headphone loaded, routing is from udev");
        return;
    }
    if (mObjModuleManager)
    {
        events::EVENT_DEVICE_CONNECTION_STATUS_T  stEventDeviceConnectionStatus;
        stEventDeviceConnectionStatus.eventName = utils::eEventDeviceConnectionStatus;
        stEventDeviceConnectionStatus.devicename = deviceName;
        stEventDeviceConnectionStatus.deviceStatus = deviceStatus;
        stEventDeviceConnectionStatus.mixerType = mixerType;
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stEventDeviceConnectionStatus);
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "callBackDeviceConnectionStatus mObjModuleManager is null");
}

void AudioMixer::callBackMasterVolumeStatus()
{
    PM_LOG_DEBUG("callBackMasterVolumeStatus");
    if (mObjModuleManager)
    {
        events::EVENT_MASTER_VOLUME_STATUS_T eventMasterVolumeStatus;
        eventMasterVolumeStatus.eventName = utils::eEventMasterVolumeStatus;
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventMasterVolumeStatus);
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "callBackMasterVolumeStatus: mObjModuleManager is null");
}
//Mixer calbacks end//

//UMI Mixer Calls Start//
bool AudioMixer::connectAudio(const std::string &strSourceName, const std::string &strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: connectAudio");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->connectAudio(strSourceName, strPhysicalSinkName, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "connectAudio: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::disconnectAudio(const std::string &strSourceName, const std::string &strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: disconnectAudio");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->disconnectAudio(strSourceName, strPhysicalSinkName, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "disconnectAudio: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setSoundOut(const std::string &strOutputMode, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: setSoundOut");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->setSoundOut(strOutputMode, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setSoundOut: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setMasterVolume(const std::string &strSoundOutPut, const int &iVolume, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: setMasterVolume");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->setMasterVolume(strSoundOutPut, iVolume, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setMasterVolume: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::getMasterVolume(LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: getMasterVolume");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->getMasterVolume(cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "getMasterVolume: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::masterVolumeUp(const std::string &strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: masterVolumeUp");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->masterVolumeUp(strSoundOutPut, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "masterVolumeUp: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::masterVolumeDown(const std::string &strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: masterVolumeDown");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->masterVolumeDown(strSoundOutPut, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "masterVolumeDown: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::masterVolumeMute(const std::string &strSoundOutPut, const bool &bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: masterVolumeMute");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->masterVolumeMute(strSoundOutPut, bIsMute, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "masterVolumeMute: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::inputVolumeMute(const std::string &strPhysicalSink, const std::string &strSource, const bool &bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: inputVolumeMute");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->inputVolumeMute(strPhysicalSink, strSource, bIsMute, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputVolumeMute: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::getConnectionStatus(LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_DEBUG("AudioMixer: getConnectionStatus");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->getConnectionStatus(cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "getConnectionStatus: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::onSinkChangedReply(const std::string& source, const std::string& sink, EVirtualAudioSink eVirtualSink,\
               utils::ESINK_STATUS eSinkStatus, utils::EMIXER_TYPE eMixerType)
{
    PM_LOG_DEBUG("AudioMixer: onSinkChangedReply");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->onSinkChangedReply(source, sink, eVirtualSink, eSinkStatus, eMixerType);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                     "onSinkChangedReply: mObjUmiAudioMixer is null");
        return false;
    }
}
//UMI Mixer Calls End//

//Pulse Mixer Calls Start//
bool AudioMixer::programVolume(EVirtualAudioSink sink, int volume, bool ramp)
{
    PM_LOG_DEBUG("AudioMixer: programVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programVolume(sink, volume, ramp);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programVolume(EVirtualSource source, int volume, bool ramp)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: programVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programVolume(source, volume, ramp);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "programVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programCallVoiceOrMICVolume(char cmd, int volume)
{
    PM_LOG_DEBUG("AudioMixer: programCallVoiceOrMICVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programCallVoiceOrMICVolume(cmd, volume);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programCallVoiceOrMICVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programMute(EVirtualSource source, int mute)
{
    PM_LOG_DEBUG("AudioMixer: programMute");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programMute(source, mute);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programMute: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::rampVolume(EVirtualAudioSink sink, int endVolume)
{
    PM_LOG_DEBUG("AudioMixer: rampVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programVolume(sink, endVolume, true);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "rampVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::moveOutputDeviceRouting(EVirtualAudioSink sink, const char* deviceName)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: moveOutputDeviceRouting");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->moveOutputDeviceRouting(sink, deviceName);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "moveOutputDeviceRouting: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::moveInputDeviceRouting(EVirtualSource source, const char* deviceName)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: moveInputDeviceRouting");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->moveInputDeviceRouting(source, deviceName);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "moveInputDeviceRouting: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setSoundOutputOnRange(EVirtualAudioSink startSink, EVirtualAudioSink endSink, const char* deviceName)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setSoundOutputOnRange");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setSoundOutputOnRange(startSink, endSink, deviceName);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setSoundoutputOnRange: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setSoundInputOnRange(EVirtualSource startSource,\
            EVirtualSource endSource, const char* deviceName)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setSoundInputOnRange");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setSoundInputOnRange(startSource, endSource, deviceName);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setSoundInputOnRange: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setDefaultSinkRouting(EVirtualAudioSink startSink, EVirtualAudioSink endSink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setDefaultSinkRouting");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setDefaultSinkRouting(startSink, endSink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setDefaultSinkRouting: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setDefaultSourceRouting(EVirtualSource startSource, EVirtualSource endSource)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setDefaultSourceRouting");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setDefaultSourceRouting(startSource, endSource);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setDefaultSourceRouting: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setSinkOutputDevice(const std::string& soundOutput,const int& sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setSinkOutputDevice got soundoutput = %s sinkID = %d",soundOutput.c_str(),sink);
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setSinkOutputDevice(soundOutput, sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setSinkOutputDevice: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setSourceInputDevice(EVirtualSource source, const char* deviceName)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setSourceInputDevice got deviceName:%s sourceId:%d", deviceName, (int)source);
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setSourceInputDevice(source, deviceName);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setSourceInputDevice: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programFilter(int filterTable)
{
    PM_LOG_DEBUG("AudioMixer: programFilter");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programFilter(filterTable);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programFilter: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::muteAll()
{
    PM_LOG_DEBUG("AudioMixer: muteAll");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->muteAll();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "muteAll: mObjPulseAudioMixer is null");
        return false;
    }
}
bool AudioMixer::suspendAll()
{
    PM_LOG_DEBUG("AudioMixer: suspendAll");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->suspendAll();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "suspendAll: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::updateRate(int rate)
{
    PM_LOG_DEBUG("AudioMixer: updateRate");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->updateRate(rate);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "updateRate: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setMute(const char* deviceName, const int& mutestatus)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setMute");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setMute(deviceName, mutestatus);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setMute: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::muteSink(const int& sink, const int& mutestatus)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: muteSink");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->muteSink(sink, mutestatus);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "\
        muteSink: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setPhysicalSourceMute(const char* source, const int& mutestatus)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setPhysicalSourceMute %s mute = %d", source, mutestatus);
    if (mObjPulseAudioMixer)
    {
        return mObjPulseAudioMixer->setPhysicalSourceMute(source, mutestatus);
    }
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setPhysicalSourceMute: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setVirtualSourceMute(int source, int mutestatus)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setVirtualSourceMute mute = %d", mutestatus);
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setVirtualSourceMute(source, mutestatus);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setVirtualSourceMute: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setVolume(const char* deviceName, const int& volume)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: setVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setVolume(deviceName, volume);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "setVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::playSystemSound(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_DEBUG("AudioMixer: playSystemSound");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->playSystemSound(snd, sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playSystemSound: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::playSound(const char *snd, EVirtualAudioSink sink, \
               const char *format, int rate, int channels)
{
    PM_LOG_DEBUG("AudioMixer: playSound");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->playSound(snd, sink, format, rate, channels);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "playSound: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programHeadsetRoute(EHeadsetState route)
{
    PM_LOG_DEBUG("AudioMixer: programHeadsetRoute");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programHeadsetRoute(route);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programHeadsetRoute: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::externalSoundcardPathCheck(std::string filename,  int status)
{
    PM_LOG_DEBUG("AudioMixer: externalSoundcardPathCheck");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->externalSoundcardPathCheck(filename, status);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "externalSoundcardPathCheck: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::loadInternalSoundCard(char cmd, int cardno, int deviceno, int status, bool isLineOut, const char* deviceName)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
        "AudioMixer: loadInternalSoundCard");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->loadInternalSoundCard(cmd, cardno, deviceno, status, isLineOut, deviceName);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "loadInternalSoundCard: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::loadUSBSinkSource(char cmd, int cardno, int deviceno, int status)
{
    PM_LOG_DEBUG("AudioMixer: loadUSBSinkSource");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->loadUSBSinkSource(cmd, cardno, deviceno, status);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "loadUSBSinkSource: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::_connectSocket()
{
    PM_LOG_DEBUG("AudioMixer: _connectSocket");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->_connectSocket();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_connectSocket: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::suspendSink(int sink)
{
    PM_LOG_DEBUG("AudioMixer: suspendSink");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->suspendSink(sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "suspendSink: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programLoadBluetooth(const char * address , const char *profile, const int displayID)
{
    PM_LOG_DEBUG("AudioMixer: programLoadBluetooth");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programLoadBluetooth(address, profile, displayID);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programLoadBluetooth: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programUnloadBluetooth(const char *profile, const int displayID)
{
    PM_LOG_DEBUG("AudioMixer: programUnloadBluetooth");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programUnloadBluetooth(profile, displayID);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programUnloadBluetooth: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programA2dpSource(const bool & a2dpSource)
{
    PM_LOG_DEBUG("AudioMixer: programA2dpSource");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programA2dpSource(a2dpSource);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
            "programA2dpSource: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setRouting(const ConstString & scenario)
{
    PM_LOG_DEBUG("AudioMixer: setRouting");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setRouting(scenario);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setRouting: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programSource(char cmd, int sink, int value)
{
    PM_LOG_DEBUG("AudioMixer: programSource");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programSource(cmd, sink, value);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programSource: mObjPulseAudioMixer is null");
        return false;
    }
}

void AudioMixer::outputStreamOpened(EVirtualAudioSink sink)
{
    PM_LOG_DEBUG("AudioMixer: outputStreamOpened");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->outputStreamOpened(sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "outputStreamOpened: mObjPulseAudioMixer is null");
}

void AudioMixer::outputStreamClosed(EVirtualAudioSink sink)
{
    PM_LOG_DEBUG("AudioMixer: outputStreamClosed");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->outputStreamClosed(sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "outputStreamClosed: mObjPulseAudioMixer is null");
}

void AudioMixer::inputStreamOpened(EVirtualSource source)
{
    PM_LOG_DEBUG("AudioMixer: inputStreamOpened");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->inputStreamOpened(source);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputStreamOpened: mObjPulseAudioMixer is null");
}

void AudioMixer::inputStreamClosed(EVirtualSource source)
{
    PM_LOG_DEBUG("AudioMixer: inputStreamClosed");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->inputStreamClosed(source);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputStreamClosed: mObjPulseAudioMixer is null");
}

void AudioMixer::preloadSystemSound(const char * snd)
{
    PM_LOG_DEBUG("AudioMixer: preloadSystemSound");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->preloadSystemSound(snd);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "preloadSystemSound: mObjPulseAudioMixer is null");
}

void AudioMixer::playOneshotDtmf(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_DEBUG("AudioMixer: playOneshotDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playOneshotDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playOneshotDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::playOneshotDtmf(const char *snd, const char* sink)
{
    PM_LOG_DEBUG("AudioMixer: playOneshotDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playOneshotDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playOneshotDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::playDtmf(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_DEBUG("AudioMixer: playDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::playDtmf(const char *snd, const char* sink)
{
    PM_LOG_DEBUG("AudioMixer: playDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::stopDtmf()
{
    PM_LOG_DEBUG("AudioMixer: stopDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->stopDtmf();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "stopDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::_pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data)
{
    PM_LOG_DEBUG("AudioMixer: _pulseStatus");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->_pulseStatus(ch, condition, user_data);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_pulseStatus: mObjPulseAudioMixer is null");
}

void AudioMixer::_timer()
{
    PM_LOG_DEBUG("AudioMixer: _timer");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->_timer();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_timer: mObjPulseAudioMixer is null");
}

void AudioMixer::setNREC(bool value)
{
    PM_LOG_DEBUG("AudioMixer: setNREC");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->setNREC(value);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setNREC: mObjPulseAudioMixer is null");
}

void AudioMixer::openCloseSink(EVirtualAudioSink sink, bool openNotClose)
{
    PM_LOG_DEBUG("AudioMixer: openCloseSink");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->openCloseSink(sink, openNotClose);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "openCloseSink: mObjPulseAudioMixer is null");
}

int AudioMixer::loopback_set_parameters(const char * value)
{
    PM_LOG_DEBUG("AudioMixer: loopback_set_parameters");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->loopback_set_parameters(value);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "loopback_set_parameters: mObjPulseAudioMixer is null");
        return -1;
    }
}
//Pulse Mixer Calls End//
