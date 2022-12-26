// Copyright (c) 2012-2023 LG Electronics, Inc.
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

    //To add audio header details to be sent to pulseaudio
    paudiodMsgHdr addAudioMsgHeader(uint8_t msgType, uint8_t msgID);

    //Will ignore volume of high latency sinks not playing and mute them.
    bool programTrackVolume(EVirtualAudioSink sink, int sinkIndex, int volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb, bool ramp = false);
    bool programVolume(EVirtualSource source, int volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb, bool ramp = false);
    bool setSoundOutputOnRange(EVirtualAudioSink startSink,\
        EVirtualAudioSink endSink, const char* deviceName);
    bool setSoundInputOnRange(EVirtualSource startSource,\
        EVirtualSource endSource, const char* deviceName);
    bool setDefaultSinkRouting(EVirtualAudioSink startSink, EVirtualAudioSink endSink);
    bool setDefaultSourceRouting(EVirtualSource startSource, EVirtualSource endSource);

    /// Get active streams set to test which sinks are active.
    VirtualSinkSet getActiveStreams()
        {return mActiveStreams;}
    /// Count how many output streams are opened
    void outputStreamOpened (EVirtualAudioSink sink, int sinkIndex, std::string trackId);
    void outputStreamClosed (EVirtualAudioSink sink, int sinkIndex, std::string trackId);
    /// Count how many input streams are opened
    void inputStreamOpened (EVirtualSource source);
    void inputStreamClosed (EVirtualSource source);
    bool setPhysicalSourceMute(const char* source, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb);
    /// Play a system sound using Pulse's API
    bool playSystemSound(const char *snd, EVirtualAudioSink sink);
    bool playSound(const char *snd, EVirtualAudioSink sink, \
        const char *format, int rate, int channels);
    /// Pre-load system sound in Pulse, if necessary
    void preloadSystemSound(const char * snd);
    bool muteSink(const int& sink, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb);  //TODO : remove
    bool setMute(const char* deviceName, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb);
    //mute virtual source
    bool setVirtualSourceMute(int sink, int mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb);
    /// mute/unmute the Physical sink
    bool setMute(int sink, int mutestatus);
    /// set volume on a particular display
    bool setVolume(int display, int volume);    //TODO: remove
    bool setVolume(const char* deviceName, const int& volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb);
    bool setMicVolume(const char* deviceName, const int& volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb);
    void playOneshotDtmf(const char *snd, EVirtualAudioSink sink) ;
    void playOneshotDtmf(const char *snd, const char* sink) ;
    void playDtmf(const char *snd, EVirtualAudioSink sink) ;
    void playDtmf(const char *snd, const char* sink) ;
    void stopDtmf();
    bool externalSoundcardPathCheck (std::string filename,  int status);
    bool loadUSBSinkSource(char cmd,int cardno, int deviceno, int status, PulseCallBackFunc cb);
    bool sendUsbMultipleDeviceInfo(int isOutput, int maxDeviceCount, const std::string &deviceBaseName);
    bool sendInternalDeviceInfo(int isOutput, int maxDeviceCount);
    bool loadInternalSoundCard(char cmd, int cardno, int deviceno, int status, bool isOutput, const char* deviceName, PulseCallBackFunc cb);
    /// These should really be private, but they're needed for global callbacks...
    bool _connectSocket();
    void _pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data);
    void _timer();

    GIOChannel* mChannel;
    void openCloseSink(EVirtualAudioSink sink, bool openNotClose, int sinkIndex, std::string trackId);
    void openCloseSource(EVirtualSource source, bool openNotClose);
    bool programLoadBluetooth (const char * address , const char *profile, const int displayID);
    bool programUnloadBluetooth (const char *profile, const int displayID);
    bool programA2dpSource (const bool& a2dpSource);
    void init(GMainLoop * loop, LSHandle * handle);
    void deviceConnectionStatus (const std::string &deviceName, const std::string &deviceNameDetail, const std::string &deviceIcon, const bool &connectionStatus, const bool& isOutput);
    bool loadCombinedSink(const char* sinkname, const char* device1, const char *device2, EVirtualAudioSink startSink,
            EVirtualAudioSink endSink, int display);
    bool closeClient(int sinkIndex);

    bool setAudioEffect(int effectId, bool enabled);
    bool checkAudioEffectStatus(int effectId);

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
    VirtualSourceSet mActiveSources;
    int mPulseStateVolume[eVirtualSink_Count+1];
    int mPulseStateVolumeHeadset[eVirtualSink_Count+1];
    int mPulseStateRoute[eVirtualSink_Count+1];
    int mPulseStateSourceRoute[eVirtualSource_Count+1];
    int mPulseStateActiveStreamCount[eVirtualSink_Count+1];
    int mPulseStateActiveSourceCount[eVirtualSource_Count + 1];
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

    bool mEffectSpeechEnhancementEnabled;
    bool mEffectGainControlEnabled;

    //To start the pulse socket connect timer
    void createPulseSocketCommunication();

    //Structure to store pulseauio callback information
    struct pulseCallBackInfo
    {
        LSHandle *lshandle;
        LSMessage *message;
        void *ctx;
        PulseCallBackFunc cb;
    };

    //Map to store pulseaudio callback information along with operation id
    std::map<int, pulseCallBackInfo> mPulseCallBackInfo;

    // Function to send message to pulseaudio
    bool sendHeaderToPA(char *data, paudiodMsgHdr audioMsgHdr);
    template<typename T>bool sendDataToPule (uint32_t msgType, uint32_t msgID, T subObj);
};

#endif //PULSEAUDIOMIXER_H_
