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

#include "audioMixer.h"

AudioMixer* AudioMixer::getAudioMixerInstance()
{
    static AudioMixer mAudioMixerObj;
    return &mAudioMixerObj;
}

AudioMixer::AudioMixer():mObjUmiMixer(nullptr), mObjPulseMixer(nullptr)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                "AudioMixer: constructor");
    if (!mObjUmiMixer)
        mObjUmiMixer = new (std::nothrow)umiaudiomixer();
    if (!mObjPulseMixer)
        mObjPulseMixer = new (std::nothrow)PulseAudioMixer();
}

AudioMixer::~AudioMixer()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: destructor");
}

bool AudioMixer::connectAudio(const std::string &strSourceName, const std::string &strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: connectAudio");
    if (mObjUmiMixer)
        return mObjUmiMixer->connectAudio(strSourceName, strPhysicalSinkName, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "connectAudio: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::disconnectAudio(const std::string &strSourceName, const std::string &strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: disconnectAudio");
    if (mObjUmiMixer)
        return mObjUmiMixer->disconnectAudio(strSourceName, strPhysicalSinkName, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "disconnectAudio: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::setSoundOut(const std::string &strOutputMode, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setSoundOut");
    if (mObjUmiMixer)
        return mObjUmiMixer->setSoundOut(strOutputMode, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setSoundOut: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::setMasterVolume(const std::string &strSoundOutPut, const int &iVolume, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setMasterVolume");
    if (mObjUmiMixer)
        return mObjUmiMixer->setMasterVolume(strSoundOutPut, iVolume, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setMasterVolume: mObjUmiMixer is null");
        return false;
    }
}
bool AudioMixer::getMasterVolume(LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: getMasterVolume");
    if (mObjUmiMixer)
        return mObjUmiMixer->getMasterVolume(cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "getMasterVolume: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::masterVolumeUp(const std::string &strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: masterVolumeUp");
    if (mObjUmiMixer)
        return mObjUmiMixer->masterVolumeUp(strSoundOutPut, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "masterVolumeUp: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::masterVolumeDown(const std::string &strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: masterVolumeDown");
    if (mObjUmiMixer)
        return mObjUmiMixer->masterVolumeDown(strSoundOutPut, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "masterVolumeDown: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::masterVolumeMute(const std::string &strSoundOutPut, const bool &bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: masterVolumeMute");
    if (mObjUmiMixer)
        return mObjUmiMixer->masterVolumeMute(strSoundOutPut, bIsMute, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "masterVolumeMute: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::inputVolumeMute(const std::string &strPhysicalSink, const std::string &strSource, const bool &bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: inputVolumeMute");
    if (mObjUmiMixer)
        return mObjUmiMixer->inputVolumeMute(strPhysicalSink, strSource, bIsMute, cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputVolumeMute: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::getConnectionStatus(LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: getConnectionStatus");
    if (mObjUmiMixer)
        return mObjUmiMixer->getConnectionStatus(cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "getConnectionStatus: mObjUmiMixer is null");
        return false;
    }
}

bool AudioMixer::isStreamActive(EVirtualAudioSink eVirtualSink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: isStreamActive");
    if (mObjUmiMixer)
        return mObjUmiMixer->isStreamActive(eVirtualSink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "isStreamActive: mObjUmiMixer is null");
        return false;
    }
}

//pulseaudiomixer calls
bool AudioMixer::programVolume(EVirtualAudioSink sink, int volume, bool ramp)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->programVolume(sink, volume, ramp);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programVolume: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programCallVoiceOrMICVolume(char cmd, int volume)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programCallVoiceOrMICVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->programCallVoiceOrMICVolume(cmd, volume);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programCallVoiceOrMICVolume: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programMute(EVirtualSource source, int mute)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programMute");
    if (mObjPulseMixer)
        return mObjPulseMixer->programMute(source, mute);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programMute: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::rampVolume(EVirtualAudioSink sink, int endVolume)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: rampVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->programVolume(sink, endVolume, true);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "rampVolume: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programDestination(EVirtualAudioSink sink, EPhysicalSink destination)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programDestination");
    if (mObjPulseMixer)
        return mObjPulseMixer->programDestination(sink, destination);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programDestination: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programDestination(EVirtualSource source, EPhysicalSource destination)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programDestination");
    if (mObjPulseMixer)
        return mObjPulseMixer->programDestination(source, destination);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programDestination: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programFilter(int filterTable)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programFilter");
    if (mObjPulseMixer)
        return mObjPulseMixer->programFilter(filterTable);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programFilter: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programBalance(int balance)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programBalance");
    if (mObjPulseMixer)
        return mObjPulseMixer->programBalance(balance);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programBalance: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::muteAll()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: muteAll");
    if (mObjPulseMixer)
        return mObjPulseMixer->muteAll();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "muteAll: mObjPulseMixer is null");
        return false;
    }
}
bool AudioMixer::suspendAll()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: suspendAll");
    if (mObjPulseMixer)
        return mObjPulseMixer->suspendAll();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "suspendAll: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::updateRate(int rate)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: updateRate");
    if (mObjPulseMixer)
        return mObjPulseMixer->updateRate(rate);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "updateRate: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::setMute(int sink, int mutestatus)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setMute");
    if (mObjPulseMixer)
        return mObjPulseMixer->setMute(sink, mutestatus);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setMute: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::setVolume(int display, int volume)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->setVolume(display, volume);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setVolume: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::playSystemSound(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playSystemSound");
    if (mObjPulseMixer)
        return mObjPulseMixer->playSystemSound(snd, sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playSystemSound: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programHeadsetRoute(int route)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programHeadsetRoute");
    if (mObjPulseMixer)
        return mObjPulseMixer->programHeadsetRoute(route);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programHeadsetRoute: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::externalSoundcardPathCheck(std::string filename,  int status)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: externalSoundcardPathCheck");
    if (mObjPulseMixer)
        return mObjPulseMixer->externalSoundcardPathCheck(filename, status);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "externalSoundcardPathCheck: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::loadUSBSinkSource(char cmd, int cardno, int deviceno, int status)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: loadUSBSinkSource");
    if (mObjPulseMixer)
        return mObjPulseMixer->loadUSBSinkSource(cmd, cardno, deviceno, status);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "loadUSBSinkSource: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::_connectSocket()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: _connectSocket");
    if (mObjPulseMixer)
        return mObjPulseMixer->_connectSocket();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_connectSocket: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::suspendSink(int sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: suspendSink");
    if (mObjPulseMixer)
        return mObjPulseMixer->suspendSink(sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "suspendSink: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programLoadBluetooth(const char * address , const char *profile)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programLoadBluetooth");
    if (mObjPulseMixer)
        return mObjPulseMixer->programLoadBluetooth(address, profile);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programLoadBluetooth: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programUnloadBluetooth(const char *profile)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programUnloadBluetooth");
    if (mObjPulseMixer)
        return mObjPulseMixer->programUnloadBluetooth(profile);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programUnloadBluetooth: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::setRouting(const ConstString & scenario)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setRouting");
    if (mObjPulseMixer)
        return mObjPulseMixer->setRouting(scenario);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setRouting: mObjPulseMixer is null");
        return false;
    }
}

bool AudioMixer::programSource(char cmd, int sink, int value)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programSource");
    if (mObjPulseMixer)
        return mObjPulseMixer->programSource(cmd, sink, value);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programSource: mObjPulseMixer is null");
        return false;
    }
}

void AudioMixer::outputStreamOpened(EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: outputStreamOpened");
    if (mObjPulseMixer)
        mObjPulseMixer->outputStreamOpened(sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "outputStreamOpened: mObjPulseMixer is null");
}

void AudioMixer::outputStreamClosed(EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: outputStreamClosed");
    if (mObjPulseMixer)
        mObjPulseMixer->outputStreamClosed(sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "outputStreamClosed: mObjPulseMixer is null");
}

void AudioMixer::inputStreamOpened(EVirtualSource source)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: inputStreamOpened");
    if (mObjPulseMixer)
        mObjPulseMixer->inputStreamOpened(source);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputStreamOpened: mObjPulseMixer is null");
}

void AudioMixer::inputStreamClosed(EVirtualSource source)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: inputStreamClosed");
    if (mObjPulseMixer)
        mObjPulseMixer->inputStreamClosed(source);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputStreamClosed: mObjPulseMixer is null");
}

void AudioMixer::preloadSystemSound(const char * snd)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: preloadSystemSound");
    if (mObjPulseMixer)
        mObjPulseMixer->preloadSystemSound(snd);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "preloadSystemSound: mObjPulseMixer is null");
}

void AudioMixer::playOneshotDtmf(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playOneshotDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playOneshotDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playOneshotDtmf: mObjPulseMixer is null");
}

void AudioMixer::playOneshotDtmf(const char *snd, const char* sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playOneshotDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playOneshotDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playOneshotDtmf: mObjPulseMixer is null");
}

void AudioMixer::playDtmf(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playDtmf: mObjPulseMixer is null");
}

void AudioMixer::playDtmf(const char *snd, const char* sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playDtmf: mObjPulseMixer is null");
}

void AudioMixer::stopDtmf()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: stopDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->stopDtmf();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "stopDtmf: mObjPulseMixer is null");
}

void AudioMixer::_pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: _pulseStatus");
    if (mObjPulseMixer)
        mObjPulseMixer->_pulseStatus(ch, condition, user_data);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_pulseStatus: mObjPulseMixer is null");
}

void AudioMixer::_timer()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: _timer");
    if (mObjPulseMixer)
        mObjPulseMixer->_timer();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_timer: mObjPulseMixer is null");
}

void AudioMixer::setNREC(bool value)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setNREC");
    if (mObjPulseMixer)
        mObjPulseMixer->setNREC(value);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setNREC: mObjPulseMixer is null");
}

void AudioMixer::openCloseSink(EVirtualAudioSink sink, bool openNotClose)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: openCloseSink");
    if (mObjPulseMixer)
        mObjPulseMixer->openCloseSink(sink, openNotClose);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "openCloseSink: mObjPulseMixer is null");
}

VirtualSinkSet AudioMixer::getActiveStreams()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: getActiveStreams");
    VirtualSinkSet mActiveStreams;
    if (mObjPulseMixer)
        return mObjPulseMixer->getActiveStreams();
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "getActiveStreams: mObjPulseMixer is null");
        return mActiveStreams;
    }
}

int AudioMixer::loopback_set_parameters(const char * value)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: loopback_set_parameters");
    if (mObjPulseMixer)
        return mObjPulseMixer->loopback_set_parameters(value);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "loopback_set_parameters: mObjPulseMixer is null");
        return -1;
    }
}
