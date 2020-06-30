// Copyright (c) 2012-2020 LG Electronics, Inc.
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

#include "AudioMixer.h"
#include "PulseAudioLink.h"

/*
 * Implementation of AudioMixer using Pulse as backend
 */

class PulseAudioMixer : public AudioMixer
{

public:
    /// Constructor that only does initialization of member variables
    PulseAudioMixer();
    ~PulseAudioMixer();

    /// Initialize mixer & possibly, register services
    void init(GMainLoop * loop,
                             LSHandle * handle,
                             AudiodCallbacksInterface * interface);

    /// We might not be ready for programming the mixer
    bool readyToProgram()    { return mChannel != 0; }

    /// Program volume of a sink.
    //Will ignore volume of high latency sinks not playing and mute them.
    bool programVolume(EVirtualAudiodSink sink, int volume, bool ramp = false);

    bool programCallVoiceOrMICVolume(char cmd, int volume);
    bool programMute(EVirtualSource source, int mute);

    /// Same as program volume, but ramped.
    bool rampVolume(EVirtualAudiodSink sink, int endVolume)
                       { return programVolume(sink, endVolume, true); }

    /// Program destination of a sink
    bool programDestination(EVirtualAudiodSink sink, EPhysicalSink destination);
    /// Program destination of a source
    bool programDestination(EVirtualSource source, EPhysicalSource destination);

    /// Program a filter
    bool programFilter(int filterTable);
    bool programLatency(int latency);
    bool programBalance(int balance);
    bool muteAll();

    /// Offset a volume by a number of dB. Calculation only.
    int  adjustVolume(int volume, int dB);

    /// Get active streams set to test which sinks are active.
    VirtualSinkSet getActiveStreams() { return mActiveStreams; }

    /// Get how many streams are active for a particular sink.
    int getStreamCount(EVirtualAudiodSink sink);

    /// Count how many output streams are opened
    void outputStreamOpened (EVirtualAudiodSink sink);
    void outputStreamClosed (EVirtualAudiodSink sink);
    int  getOutputStreamOpenedCount ()
                { return mOutputStreamsCurrentlyOpenedCount; }

    /// Count how many input streams are opened
    void inputStreamOpened (EVirtualSource source);
    void inputStreamClosed (EVirtualSource source);
    int  getInputStreamOpenedCount () { return mInputStreamsCurrentlyOpenedCount; }

    /// Find out if a particular sink is audible.
    // Something must be playing & the volume must be non-null
    bool isSinkAudible(EVirtualAudiodSink sink);

    /// Suspend all streams (when entering power saving mode).
    bool suspendAll();

    /// Suspend sampling rate on sinks/sources
    bool updateRate(int rate);


    /// Play a system sound using Pulse's API
    bool playSystemSound(const char *snd, EVirtualAudiodSink sink);

    /// Pre-load system sound in Pulse, if necessary
    void preloadSystemSound(const char * snd) { mPulseLink.preload(snd); }
    /// mute/unmute the Physical sink
    bool setMute(int sink, int mutestatus);

    /// set volume on a particular display
    bool setVolume(int display, int volume);

    void playOneshotDtmf(const char *snd, EVirtualAudiodSink sink) ;

    void playOneshotDtmf(const char *snd, const char* sink) ;

    void playDtmf(const char *snd, EVirtualAudiodSink sink) ;

    void playDtmf(const char *snd, const char* sink) ;

    void stopDtmf();

    bool programLoadRTP(const char *type, const char *ip, int port);
    bool programHeadsetRoute (int route);
    bool programUnloadRTP();

    bool externalSoundcardPathCheck (std::string filename,  int status);
    bool loadUSBSinkSource(char cmd,int cardno, int deviceno, int status);

    /// These should really be private, but they're needed for global callbacks...
    bool _connectSocket();
    void _pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data);
    void _timer();
    bool _sinkStatus(LSHandle *lshandle, LSMessage *message);
    bool _setFilter(LSHandle * lshandle, LSMessage * message);
    bool _suspend(LSHandle * lshandle, LSMessage * message);

    GIOChannel* mChannel;

#ifdef HAVE_BT_SERVICE_V1
    void sendBTDeviceType(bool type, bool hfpStatus);
    void setBTvolumeSupport(bool value);
#endif
    bool suspendSink(int sink);
    void setNREC(bool value);
    void sendNREC(bool value);
    bool programLoadBluetooth (const char * address , const char *profile);
    bool programUnloadBluetooth (const char *profile);
    bool setRouting(const ConstString & scenario);
    bool phoneEvent(EPhoneEvent event, int parameter);
    bool getBTVolumeSupport();
    void setBTDeviceType(int type);
    bool setPhoneVolume(const ConstString & scenario, int volume);
    bool setPhoneMuted(const ConstString & scenario, bool muted);
    int loopback_set_parameters(const char * value);


    inline void setHfpAgRole(bool HfpAgRole);
    inline bool inHfpAgRole(void);

private:
    bool programSource(char cmd, int sink, int value);
    void openCloseSink(EVirtualAudiodSink sink, bool openNotClose);
    int  getCurrentPulseVolume(EVirtualAudiodSink sink);// get Pulse volume

    // Direct socket connection to Pulse
    int mTimeout;
    unsigned int mSourceID;
    int mConnectAttempt;

    // Connection to Pulse via official Pulse APIs
    PulseAudioLink mPulseLink;
    PulseDtmfGenerator* mCurrentDtmf;

    VirtualSinkSet mActiveStreams;
    int mPulseStateVolume[eVirtualSink_Count];
    int mPulseStateVolumeHeadset[eVirtualSink_Count];
    int mPulseStateRoute[eVirtualSink_Count];
    int mPulseStateSourceRoute[eVirtualSource_Count];
    int mPulseStateActiveStreamCount[eVirtualSink_Count];
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
    bool mIsHfpAgRole;

#ifdef HAVE_BT_SERVICE_V1
    bool voiceRxMuted;
#endif
    AudiodCallbacksInterface* mCallbacks;
};

extern PulseAudioMixer gPulseAudioMixer;


#endif /* PULSEAUDIOMIXER_H_ */
