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

AudioMixer::AudioMixer():mObjUmiAudioMixer(nullptr), mObjPulseAudioMixer(nullptr)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                "AudioMixer: constructor");
    if (!mObjUmiAudioMixer)
        mObjUmiAudioMixer = new (std::nothrow)umiaudiomixer();
    if (!mObjPulseAudioMixer)
        mObjPulseAudioMixer = new (std::nothrow)PulseAudioMixer();
}

AudioMixer::~AudioMixer()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: destructor");
}

bool AudioMixer::readyToProgram()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: readyToProgram");
    bool umiMixerStatus = false;
    bool pulseMixerStatus = false;
    if (mObjUmiAudioMixer)
        umiMixerStatus = mObjUmiAudioMixer->getUmiMixerReadyStatus();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "readyToProgram: mObjUmiAudioMixer is null");
    if (mObjPulseAudioMixer)
        pulseMixerStatus = mObjPulseAudioMixer->getPulseMixerReadyStatus();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "readyToProgram: mObjPulseAudioMixer is null");
    return  umiMixerStatus && pulseMixerStatus;
}

bool AudioMixer::connectAudio(const std::string &strSourceName, const std::string &strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: connectAudio");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: disconnectAudio");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setSoundOut");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setMasterVolume");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: getMasterVolume");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: masterVolumeUp");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: masterVolumeDown");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: masterVolumeMute");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: inputVolumeMute");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: getConnectionStatus");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->getConnectionStatus(cb, message);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "getConnectionStatus: mObjUmiAudioMixer is null");
        return false;
    }
}

bool AudioMixer::isStreamActive(EVirtualAudioSink eVirtualSink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: isStreamActive");
    if (mObjUmiAudioMixer)
        return mObjUmiAudioMixer->isStreamActive(eVirtualSink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "isStreamActive: mObjUmiAudioMixer is null");
        return false;
    }
}

//pulseaudiomixer calls
bool AudioMixer::programVolume(EVirtualAudioSink sink, int volume, bool ramp)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programVolume(sink, volume, ramp);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programCallVoiceOrMICVolume(char cmd, int volume)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programCallVoiceOrMICVolume");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programMute");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: rampVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programVolume(sink, endVolume, true);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "rampVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programDestination(EVirtualAudioSink sink, EPhysicalSink destination)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programDestination");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programDestination(sink, destination);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programDestination: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programDestination(EVirtualSource source, EPhysicalSource destination)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programDestination");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programDestination(source, destination);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programDestination: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programFilter(int filterTable)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programFilter");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programFilter(filterTable);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programFilter: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programBalance(int balance)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programBalance");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programBalance(balance);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programBalance: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::muteAll()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: muteAll");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: suspendAll");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: updateRate");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->updateRate(rate);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "updateRate: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setMute(int sink, int mutestatus)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setMute");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setMute(sink, mutestatus);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setMute: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setVolume(int display, int volume)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setVolume");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->setVolume(display, volume);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setVolume: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::playSystemSound(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playSystemSound");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->playSystemSound(snd, sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playSystemSound: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programHeadsetRoute(int route)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programHeadsetRoute");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: externalSoundcardPathCheck");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->externalSoundcardPathCheck(filename, status);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "externalSoundcardPathCheck: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::loadUSBSinkSource(char cmd, int cardno, int deviceno, int status)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: loadUSBSinkSource");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: _connectSocket");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: suspendSink");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->suspendSink(sink);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "suspendSink: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programLoadBluetooth(const char * address , const char *profile)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programLoadBluetooth");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programLoadBluetooth(address, profile);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programLoadBluetooth: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::programUnloadBluetooth(const char *profile)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programUnloadBluetooth");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->programUnloadBluetooth(profile);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "programUnloadBluetooth: mObjPulseAudioMixer is null");
        return false;
    }
}

bool AudioMixer::setRouting(const ConstString & scenario)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setRouting");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: programSource");
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
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: outputStreamOpened");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->outputStreamOpened(sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "outputStreamOpened: mObjPulseAudioMixer is null");
}

void AudioMixer::outputStreamClosed(EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: outputStreamClosed");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->outputStreamClosed(sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "outputStreamClosed: mObjPulseAudioMixer is null");
}

void AudioMixer::inputStreamOpened(EVirtualSource source)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: inputStreamOpened");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->inputStreamOpened(source);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputStreamOpened: mObjPulseAudioMixer is null");
}

void AudioMixer::inputStreamClosed(EVirtualSource source)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: inputStreamClosed");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->inputStreamClosed(source);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "inputStreamClosed: mObjPulseAudioMixer is null");
}

void AudioMixer::preloadSystemSound(const char * snd)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: preloadSystemSound");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->preloadSystemSound(snd);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "preloadSystemSound: mObjPulseAudioMixer is null");
}

void AudioMixer::playOneshotDtmf(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playOneshotDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playOneshotDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playOneshotDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::playOneshotDtmf(const char *snd, const char* sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playOneshotDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playOneshotDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playOneshotDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::playDtmf(const char *snd, EVirtualAudioSink sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::playDtmf(const char *snd, const char* sink)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: playDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->playDtmf(snd, sink);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "playDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::stopDtmf()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: stopDtmf");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->stopDtmf();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "stopDtmf: mObjPulseAudioMixer is null");
}

void AudioMixer::_pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: _pulseStatus");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->_pulseStatus(ch, condition, user_data);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_pulseStatus: mObjPulseAudioMixer is null");
}

void AudioMixer::_timer()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: _timer");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->_timer();
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "_timer: mObjPulseAudioMixer is null");
}

void AudioMixer::setNREC(bool value)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: setNREC");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->setNREC(value);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "setNREC: mObjPulseAudioMixer is null");
}

void AudioMixer::openCloseSink(EVirtualAudioSink sink, bool openNotClose)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: openCloseSink");
    if (mObjPulseAudioMixer)
        mObjPulseAudioMixer->openCloseSink(sink, openNotClose);
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "openCloseSink: mObjPulseAudioMixer is null");
}

utils::vectorVirtualSink AudioMixer::getActiveStreams()
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: getActiveStreams");
    return mActiveStreams;
}

int AudioMixer::loopback_set_parameters(const char * value)
{
    PM_LOG_INFO(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "AudioMixer: loopback_set_parameters");
    if (mObjPulseAudioMixer)
        return mObjPulseAudioMixer->loopback_set_parameters(value);
    else
    {
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT, "loopback_set_parameters: mObjPulseAudioMixer is null");
        return -1;
    }
}
