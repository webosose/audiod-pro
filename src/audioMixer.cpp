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

Mixer* Mixer::getMixerInstance()
{
    static Mixer mMixerObj;
    return &mMixerObj;
}

Mixer::Mixer():mObjUmiMixer(nullptr), mObjPulseMixer(nullptr)
{
    g_debug("Mixer: constructor");
    if (!mObjUmiMixer)
        mObjUmiMixer = new (std::nothrow)umiaudiomixer();
    if (!mObjPulseMixer)
        mObjPulseMixer = new (std::nothrow)PulseAudioMixer();
}

Mixer::~Mixer()
{
    g_debug("Mixer: destructor");
}

bool Mixer::connectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: connectAudio");
    if (mObjUmiMixer)
        return mObjUmiMixer->connectAudio(strSourceName, strPhysicalSinkName, cb, message);
    else
    {
        g_debug("connectAudio: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::disconnectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: disconnectAudio");
    if (mObjUmiMixer)
        return mObjUmiMixer->disconnectAudio(strSourceName, strPhysicalSinkName, cb, message);
    else
    {
        g_debug("disconnectAudio: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::setSoundOut(std::string strOutputMode, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: setSoundOut");
    if (mObjUmiMixer)
        return mObjUmiMixer->setSoundOut(strOutputMode, cb, message);
    else
    {
        g_debug("setSoundOut: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::setMasterVolume(std::string strSoundOutPut, int iVolume, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: setMasterVolume");
    if (mObjUmiMixer)
        return mObjUmiMixer->setMasterVolume(strSoundOutPut, iVolume, cb, message);
    else
    {
        g_debug("setMasterVolume: mObjUmiMixer is null");
        return false;
    }
}
bool Mixer::getMasterVolume(LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: getMasterVolume");
    if (mObjUmiMixer)
        return mObjUmiMixer->getMasterVolume(cb, message);
    else
    {
        g_debug("getMasterVolume: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::masterVolumeUp(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: masterVolumeUp");
    if (mObjUmiMixer)
        return mObjUmiMixer->masterVolumeUp(strSoundOutPut, cb, message);
    else
    {
        g_debug("masterVolumeUp: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::masterVolumeDown(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: masterVolumeDown");
    if (mObjUmiMixer)
        return mObjUmiMixer->masterVolumeDown(strSoundOutPut, cb, message);
    else
    {
        g_debug("masterVolumeDown: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::masterVolumeMute(std::string strSoundOutPut, bool bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: masterVolumeMute");
    if (mObjUmiMixer)
        return mObjUmiMixer->masterVolumeMute(strSoundOutPut, bIsMute, cb, message);
    else
    {
        g_debug("masterVolumeMute: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::inputVolumeMute(std::string strPhysicalSink, std::string strSource, bool bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: inputVolumeMute");
    if (mObjUmiMixer)
        return mObjUmiMixer->inputVolumeMute(strPhysicalSink, strSource, bIsMute, cb, message);
    else
    {
        g_debug("inputVolumeMute: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::getConnectionStatus(LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Mixer: getConnectionStatus");
    if (mObjUmiMixer)
        return mObjUmiMixer->getConnectionStatus(cb, message);
    else
    {
        g_debug("getConnectionStatus: mObjUmiMixer is null");
        return false;
    }
}

bool Mixer::readyToProgram()
{
    g_debug("Mixer: readyToProgram");
    if (mObjUmiMixer)
        return mObjUmiMixer->readyToProgram();
    else
    {
        g_debug("readyToProgram: mObjUmiMixer is null");
        return false;
    }
    //need to enable for pulsemixer as well
    //return mChannel != 0;
    //return mObjPulseMixer->readyToProgram();
}

bool Mixer::isStreamActive(EVirtualAudiodSink eVirtualSink)
{
    g_debug("Mixer: isStreamActive");
    if (mObjUmiMixer)
        return mObjUmiMixer->isStreamActive(eVirtualSink);
    else
    {
        g_debug("isStreamActive: mObjUmiMixer is null");
        return false;
    }
}

void Mixer::onSinkChangedReply(EVirtualAudiodSink eVirtualSink, E_CONNSTATUS eConnStatus, ESinkType eSinkType)
{
    g_debug("Mixer: onSinkChangedReply");
    if (mObjUmiMixer)
        mObjUmiMixer->onSinkChangedReply(eVirtualSink, eConnStatus, eSinkType);
    else
        g_debug("onSinkChangedReply: mObjUmiMixer is null");
}

void Mixer::updateStreamStatus(EVirtualAudiodSink eVirtualSink, E_CONNSTATUS eConnStatus)
{
    g_debug("Mixer: updateStreamStatus");
    if (mObjUmiMixer)
        mObjUmiMixer->updateStreamStatus(eVirtualSink, eConnStatus);
    else
        g_debug("updateStreamStatus: mObjUmiMixer is null");
}

void Mixer::setMixerReadyStatus(bool eStatus)
{
    g_debug("Mixer: setMixerReadyStatus");
    if (mObjUmiMixer)
        mObjUmiMixer->setMixerReadyStatus(eStatus);
    else
        g_debug("setMixerReadyStatus: mObjUmiMixer is null");
}

//pulseaudiomixer calls
bool Mixer::programVolume(EVirtualAudiodSink sink, int volume, bool ramp)
{
    g_debug("Mixer: programVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->programVolume(sink, volume, ramp);
    else
    {
        g_debug("programVolume: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programCallVoiceOrMICVolume(char cmd, int volume)
{
    g_debug("Mixer: programCallVoiceOrMICVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->programCallVoiceOrMICVolume(cmd, volume);
    else
    {
        g_debug("programCallVoiceOrMICVolume: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programMute(EVirtualSource source, int mute)
{
    g_debug("Mixer: programMute");
    if (mObjPulseMixer)
        return mObjPulseMixer->programMute(source, mute);
    else
    {
        g_debug("programMute: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::rampVolume(EVirtualAudiodSink sink, int endVolume)
{
    g_debug("Mixer: rampVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->programVolume(sink, endVolume, true);
    else
    {
        g_debug("rampVolume: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programDestination(EVirtualAudiodSink sink, EPhysicalSink destination)
{
    g_debug("Mixer: programDestination");
    if (mObjPulseMixer)
        return mObjPulseMixer->programDestination(sink, destination);
    else
    {
        g_debug("programDestination: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programDestination(EVirtualSource source, EPhysicalSource destination)
{
    g_debug("Mixer: programDestination");
    if (mObjPulseMixer)
        return mObjPulseMixer->programDestination(source, destination);
    else
    {
        g_debug("programDestination: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programFilter(int filterTable)
{
    g_debug("Mixer: programFilter");
    if (mObjPulseMixer)
        return mObjPulseMixer->programFilter(filterTable);
    else
    {
        g_debug("programFilter: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programBalance(int balance)
{
    g_debug("Mixer: programBalance");
    if (mObjPulseMixer)
        return mObjPulseMixer->programBalance(balance);
    else
    {
        g_debug("programBalance: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::muteAll()
{
    g_debug("Mixer: muteAll");
    if (mObjPulseMixer)
        return mObjPulseMixer->muteAll();
    else
    {
        g_debug("muteAll: mObjPulseMixer is null");
        return false;
    }
}
bool Mixer::suspendAll()
{
    g_debug("Mixer: suspendAll");
    if (mObjPulseMixer)
        return mObjPulseMixer->suspendAll();
    else
    {
        g_debug("suspendAll: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::updateRate(int rate)
{
    g_debug("Mixer: updateRate");
    if (mObjPulseMixer)
        return mObjPulseMixer->updateRate(rate);
    else
    {
        g_debug("updateRate: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::setMute(int sink, int mutestatus)
{
    g_debug("Mixer: setMute");
    if (mObjPulseMixer)
        return mObjPulseMixer->setMute(sink, mutestatus);
    else
    {
        g_debug("setMute: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::setVolume(int display, int volume)
{
    g_debug("Mixer: setVolume");
    if (mObjPulseMixer)
        return mObjPulseMixer->setVolume(display, volume);
    else
    {
        g_debug("setVolume: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::playSystemSound(const char *snd, EVirtualAudiodSink sink)
{
    g_debug("Mixer: playSystemSound");
    if (mObjPulseMixer)
        return mObjPulseMixer->playSystemSound(snd, sink);
    else
    {
        g_debug("playSystemSound: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programHeadsetRoute(int route)
{
    g_debug("Mixer: programHeadsetRoute");
    if (mObjPulseMixer)
        return mObjPulseMixer->programHeadsetRoute(route);
    else
    {
        g_debug("programHeadsetRoute: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::externalSoundcardPathCheck(std::string filename,  int status)
{
    g_debug("Mixer: externalSoundcardPathCheck");
    if (mObjPulseMixer)
        return mObjPulseMixer->externalSoundcardPathCheck(filename, status);
    else
    {
        g_debug("externalSoundcardPathCheck: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::loadUSBSinkSource(char cmd, int cardno, int deviceno, int status)
{
    g_debug("Mixer: loadUSBSinkSource");
    if (mObjPulseMixer)
        return mObjPulseMixer->loadUSBSinkSource(cmd, cardno, deviceno, status);
    else
    {
        g_debug("loadUSBSinkSource: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::_connectSocket()
{
    g_debug("Mixer: _connectSocket");
    if (mObjPulseMixer)
        return mObjPulseMixer->_connectSocket();
    else
    {
        g_debug("_connectSocket: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::suspendSink(int sink)
{
    g_debug("Mixer: suspendSink");
    if (mObjPulseMixer)
        return mObjPulseMixer->suspendSink(sink);
    else
    {
        g_debug("suspendSink: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programLoadBluetooth(const char * address , const char *profile)
{
    g_debug("Mixer: programLoadBluetooth");
    if (mObjPulseMixer)
        return mObjPulseMixer->programLoadBluetooth(address, profile);
    else
    {
        g_debug("programLoadBluetooth: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programUnloadBluetooth(const char *profile)
{
    g_debug("Mixer: programUnloadBluetooth");
    if (mObjPulseMixer)
        return mObjPulseMixer->programUnloadBluetooth(profile);
    else
    {
        g_debug("programUnloadBluetooth: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::setRouting(const ConstString & scenario)
{
    g_debug("Mixer: setRouting");
    if (mObjPulseMixer)
        return mObjPulseMixer->setRouting(scenario);
    else
    {
        g_debug("setRouting: mObjPulseMixer is null");
        return false;
    }
}

bool Mixer::programSource(char cmd, int sink, int value)
{
    g_debug("Mixer: programSource");
    if (mObjPulseMixer)
        return mObjPulseMixer->programSource(cmd, sink, value);
    else
    {
        g_debug("programSource: mObjPulseMixer is null");
        return false;
    }
}

void Mixer::outputStreamOpened(EVirtualAudiodSink sink)
{
    g_debug("Mixer: outputStreamOpened");
    if (mObjPulseMixer)
        mObjPulseMixer->outputStreamOpened(sink);
    else
        g_debug("outputStreamOpened: mObjPulseMixer is null");
}

void Mixer::outputStreamClosed(EVirtualAudiodSink sink)
{
    g_debug("Mixer: outputStreamClosed");
    if (mObjPulseMixer)
        mObjPulseMixer->outputStreamClosed(sink);
    else
        g_debug("outputStreamClosed: mObjPulseMixer is null");
}

void Mixer::inputStreamOpened(EVirtualSource source)
{
    g_debug("Mixer: inputStreamOpened");
    if (mObjPulseMixer)
        mObjPulseMixer->inputStreamOpened(source);
    else
        g_debug("inputStreamOpened: mObjPulseMixer is null");
}

void Mixer::inputStreamClosed(EVirtualSource source)
{
    g_debug("Mixer: inputStreamClosed");
    if (mObjPulseMixer)
        mObjPulseMixer->inputStreamClosed(source);
    else
        g_debug("inputStreamClosed: mObjPulseMixer is null");
}

void Mixer::preloadSystemSound(const char * snd)
{
    g_debug("Mixer: preloadSystemSound");
    if (mObjPulseMixer)
        mObjPulseMixer->preloadSystemSound(snd);
    else
        g_debug("preloadSystemSound: mObjPulseMixer is null");
}

void Mixer::playOneshotDtmf(const char *snd, EVirtualAudiodSink sink)
{
    g_debug("Mixer: playOneshotDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playOneshotDtmf(snd, sink);
    else
        g_debug("playOneshotDtmf: mObjPulseMixer is null");
}

void Mixer::playOneshotDtmf(const char *snd, const char* sink)
{
    g_debug("Mixer: playOneshotDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playOneshotDtmf(snd, sink);
    else
        g_debug("playOneshotDtmf: mObjPulseMixer is null");
}

void Mixer::playDtmf(const char *snd, EVirtualAudiodSink sink)
{
    g_debug("Mixer: playDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playDtmf(snd, sink);
    else
        g_debug("playDtmf: mObjPulseMixer is null");
}

void Mixer::playDtmf(const char *snd, const char* sink)
{
    g_debug("Mixer: playDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->playDtmf(snd, sink);
    else
        g_debug("playDtmf: mObjPulseMixer is null");
}

void Mixer::stopDtmf()
{
    g_debug("Mixer: stopDtmf");
    if (mObjPulseMixer)
        mObjPulseMixer->stopDtmf();
    else
        g_debug("stopDtmf: mObjPulseMixer is null");
}

void Mixer::_pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data)
{
    g_debug("Mixer: _pulseStatus");
    if (mObjPulseMixer)
        mObjPulseMixer->_pulseStatus(ch, condition, user_data);
    else
        g_debug("_pulseStatus: mObjPulseMixer is null");
}

void Mixer::_timer()
{
    g_debug("Mixer: _timer");
    if (mObjPulseMixer)
        mObjPulseMixer->_timer();
    else
        g_debug("_timer: mObjPulseMixer is null");
}

void Mixer::setNREC(bool value)
{
    g_debug("Mixer: setNREC");
    if (mObjPulseMixer)
        mObjPulseMixer->setNREC(value);
    else
        g_debug("setNREC: mObjPulseMixer is null");
}

void Mixer::openCloseSink(EVirtualAudiodSink sink, bool openNotClose)
{
    g_debug("Mixer: openCloseSink");
    if (mObjPulseMixer)
        mObjPulseMixer->openCloseSink(sink, openNotClose);
    else
        g_debug("openCloseSink: mObjPulseMixer is null");
}

VirtualSinkSet Mixer::getActiveStreams()
{
    g_debug("Mixer: getActiveStreams");
    VirtualSinkSet mActiveStreams;
    if (mObjPulseMixer)
        return mObjPulseMixer->getActiveStreams();
    else
    {
        g_debug("getActiveStreams: mObjPulseMixer is null");
        return mActiveStreams;
    }
}

int Mixer::loopback_set_parameters(const char * value)
{
    g_debug("Mixer: loopback_set_parameters");
    if (mObjPulseMixer)
        return mObjPulseMixer->loopback_set_parameters(value);
    else
    {
        g_debug("loopback_set_parameters: mObjPulseMixer is null");
        return -1;
    }
}
