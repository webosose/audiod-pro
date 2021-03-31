// Copyright (c) 2012-2021 LG Electronics, Inc.
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


#ifndef PULSEAUDIOMIXER_H_
#define PULSEAUDIOMIXER_H_

#include "PulseAudioLink.h"
#include "utils.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>
#include "messageUtils.h"
#include "log.h"
#include "main.h"
#include <audiodTracer.h>
#include <pulse/module-palm-policy-tables.h>
#include "mixerInterface.h"

//Implementation of PulseMixer using Pulse as backend
class PulseAudioMixer
{
public:
    /// Constructor that only does initialization of member variables
    PulseAudioMixer(MixerInterface* mixerCallBack);
    ~PulseAudioMixer();

    //Will ignore volume of high latency sinks not playing and mute them.
    bool programVolume(EVirtualAudioSink sink, int volume, bool ramp = false);
    bool programCallVoiceOrMICVolume(char cmd, int volume);
    bool programMute(EVirtualSource source, int mute);
    /// Same as program volume, but ramped.
    bool rampVolume(EVirtualAudioSink sink, int endVolume)
        {return programVolume(sink, endVolume, true);}
    /// Program destination of a sink
    bool programDestination(EVirtualAudioSink sink, EPhysicalSink destination);
    /// Program destination of a source
    bool programDestination(EVirtualSource source, EPhysicalSource destination);
    /// Program a filter
    bool programFilter(int filterTable);
    bool programBalance(int balance);
    bool muteAll();
    /// Get active streams set to test which sinks are active.
    VirtualSinkSet getActiveStreams()
        {return mActiveStreams;}
    /// Count how many output streams are opened
    void outputStreamOpened (EVirtualAudioSink sink);
    void outputStreamClosed (EVirtualAudioSink sink);
    /// Count how many input streams are opened
    void inputStreamOpened (EVirtualSource source);
    void inputStreamClosed (EVirtualSource source);
    /// Suspend all streams (when entering power saving mode).
    bool suspendAll();
    /// Suspend sampling rate on sinks/sources
    bool updateRate(int rate);
    /// Play a system sound using Pulse's API
    bool playSystemSound(const char *snd, EVirtualAudioSink sink);
    bool playSound(const char *snd, EVirtualAudioSink sink, \
        const char *format, int rate, int channels);
    /// Pre-load system sound in Pulse, if necessary
    void preloadSystemSound(const char * snd);
    /// mute/unmute the Physical sink
    bool setMute(int sink, int mutestatus);
    /// set volume on a particular display
    bool setVolume(int display, int volume);
    void playOneshotDtmf(const char *snd, EVirtualAudioSink sink) ;
    void playOneshotDtmf(const char *snd, const char* sink) ;
    void playDtmf(const char *snd, EVirtualAudioSink sink) ;
    void playDtmf(const char *snd, const char* sink) ;
    void stopDtmf();
    bool programHeadsetRoute (int route);
    bool externalSoundcardPathCheck (std::string filename,  int status);
    bool loadUSBSinkSource(char cmd,int cardno, int deviceno, int status);
    /// These should really be private, but they're needed for global callbacks...
    bool _connectSocket();
    void _pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data);
    void _timer();

    GIOChannel* mChannel;
    bool suspendSink(int sink);
    bool programSource(char cmd, int sink, int value);
    void openCloseSink(EVirtualAudioSink sink, bool openNotClose);
    void setNREC(bool value);
    bool programLoadBluetooth (const char * address , const char *profile, const int displayID);
    bool programUnloadBluetooth (const char *profile, const int displayID);
    bool programA2dpSource (const bool& a2dpSource);
    bool setRouting(const ConstString & scenario);
    int loopback_set_parameters(const char * value);
    void init(GMainLoop * loop, LSHandle * handle);

private:
    PulseAudioMixer() = delete;
    PulseAudioMixer(const PulseAudioMixer &) = delete;
    PulseAudioMixer& operator=(const PulseAudioMixer &) = delete;
    MixerInterface *mObjMixerCallBack;
    // Direct socket connection to Pulse
    int mTimeout;
    unsigned int mSourceID;
    int mConnectAttempt;

    // Connection to Pulse via official Pulse APIs
    PulseAudioLink mPulseLink;
    PulseDtmfGenerator* mCurrentDtmf;
    VirtualSinkSet mActiveStreams;
    int mPulseStateVolume[eVirtualSink_Count+1];
    int mPulseStateVolumeHeadset[eVirtualSink_Count+1];
    int mPulseStateRoute[eVirtualSink_Count+1];
    int mPulseStateSourceRoute[eVirtualSource_Count+1];
    int mPulseStateActiveStreamCount[eVirtualSink_Count+1];
    bool mPulseFilterEnabled;
    int mPulseStateFilter;
    int mPulseStateLatency;
    int mInputStreamsCurrentlyOpenedCount;
    int mOutputStreamsCurrentlyOpenedCount;
    bool voLTE;
    int BTDeviceType;
    int mPreviousVolume;
    bool NRECvalue;
    bool BTvolumeSupport;

    //To start the pulse socket connect timer
    void createPulseSocketCommunication();
};

#endif //PULSEAUDIOMIXER_H_
