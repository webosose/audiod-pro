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


#ifndef AUDIOMIXER_H_
#define AUDIOMIXER_H_

#include <lunaservice.h>

#include <pulse/module-palm-policy.h>
//#include "AudioDevice.h"
#include "ConstString.h"

enum EPhoneEvent
{
    ePhoneEvent_CallStarted,
    ePhoneEvent_CallEnded,
    ePhoneEvent_EmergencyCall,
    ePhoneEvent_BusyTone,
    ePhoneEvent_DTMFToneEnd,
   // parameter is the DTMF key as a character, '0' to '9', '*' or '#'
    ePhoneEvent_DTMFTone,
    // parameter is the DTMF key as a character, '0' to '9', '*' or '#'
    ePhoneEvent_OneShotDTMFTone,
    ePhoneEvent_IncomingCallTone,
    ePhoneEvent_Incoming,
    ePhoneEvent_Disconnected,
    ePhoneEvent_voLTEcallsupported,
    ePhoneEvent_voLTEcallNOTsupported
};

enum EBTDeviceType
{
    eBTDevice_NarrowBand =1,
    eBTDevice_Wideband =2
};

/// Simple set class to hold a set of sinks & test it
class VirtualSinkSet
{
public:
    VirtualSinkSet() : mSet(0) {}

    void    clear()                    { mSet = 0; }

    void    add(EVirtualSink sink)     { mSet |= mask(sink); }
    void    remove(EVirtualSink sink)  { mSet &= ~mask(sink); }

    /// does the set contain this sink?
    bool    contain(EVirtualSink sink) const
                { return (mSet & mask(sink)) != 0; }

    /// does the set contain any of these two sinks?
    bool    containAnyOf(EVirtualSink sink1, EVirtualSink sink2) const
                { return (mSet & (mask(sink1) | mask(sink2))) != 0; }

    /// does the set contain any of these three sinks?
    bool    containAnyOf( EVirtualSink sink1,
                          EVirtualSink sink2,
                          EVirtualSink sink3) const
                { return (mSet & ( mask(sink1) |
                                   mask(sink2) |
                                   mask(sink3))) != 0; }

    /// does the set contain any of these four sinks?
    bool    containAnyOf( EVirtualSink sink1,
                          EVirtualSink sink2,
                          EVirtualSink sink3,
                          EVirtualSink sink4) const
                { return (mSet & ( mask(sink1) |
                                   mask(sink2) |
                                   mask(sink3) |
                                    mask(sink4))) != 0; }

    bool    containAnyOf( EVirtualSink sink1,
                          EVirtualSink sink2,
                          EVirtualSink sink3,
                          EVirtualSink sink4,
                          EVirtualSink sink5) const
                { return (mSet & ( mask(sink1) |
                                   mask(sink2) |
                                   mask(sink3) |
                                   mask(sink4) |
                                   mask(sink5))) != 0; }


    bool    operator==(const VirtualSinkSet & rhs) const { return mSet == rhs.mSet; }
    bool    operator!=(const VirtualSinkSet & rhs) const { return mSet != rhs.mSet; }

private:
    int        mask(EVirtualSink sink) const         { return 1 << sink; }

    int        mSet;
};

inline bool IsValidVirtualSink(EVirtualSink sink)
   { return sink >= eVirtualSink_First && sink <= eVirtualSink_Last; }

inline bool IsValidVirtualSource(EVirtualSource source)
    { return source >= eVirtualSource_First && source <= eVirtualSource_Last; }

// Low latency sinks are never muted. Their volume should always be set properly...
inline bool isNeverMutedSink(EVirtualSink sink)
{
    return sink == eDTMF || sink == efeedback || sink == eeffects || sink == ecallertone;
}

enum EControlEvent
{
    eControlEvent_None                    = 0,
    eControlEvent_FirstStreamOpened       = 1,
    eControlEvent_LastStreamClosed        = 2
};

/*
TODO : Currently there are 2 mixers (UMI mixer and Pulse audio Mixer). 
We'll be working on common interface layer for mixers and 
Take care of generation of them using config files and respective design pattern 
*/
enum ESinkType
{
    eNone,
    ePulseAudio,
    eUmi
};

/// Interface to notify audiod that AudioMixier events happened
class AudiodCallbacksInterface
{
public:
    /// Audio Mixer connection with its subsystem established:
    // set all audio mixer parameters as needed
    virtual void        onAudioMixerConnected() = 0;

    /// An audio sink was opened or closed: adjust volumes as necessary
    virtual void        onSinkChanged(EVirtualSink sink, EControlEvent event,ESinkType p_eSinkType) = 0;

    /// Notification that a first input stream was opened,
    // or that the last input stream was closed
    virtual void        onInputStreamActiveChanged(bool active) = 0;
};

const char * controlEventName(EControlEvent event);
const char * virtualSinkName(EVirtualSink sink, bool prettyNameNotInternal = true);
const char * virtualSourceName(EVirtualSource source,
                               bool prettyNameNotInternal = true);
EVirtualSink getSinkByName(const char * name);

/*
 * AudioMixer abstracts Audiod's view of the subsystem that implements audio mixing.
 * It lets audiod control the mixer's parameters that audiod is responsible for, such as:
 *  - volume setting for each sink
 *  - destination of each sink
 *  - audio filter to be applied
 *  - the audio latency
 * It allows audiod to access information about the current streams playing, such as:
 *  - a mask telling which sinks are active right now
 *  - count of steams active per sink
 *  - count of input & output streams (all sinks combined)
 *  - whether a sink is currently routed & possible audible (volume set)
 * It provides a way for audiod to play system sounds handled directly
 * by the mixer for low latency.
 * It also allows audiod to setup synchronous callbacks so that
 * audiod can know about key events immediately
 */

class AudioMixer
{
public:
    /// Initialize mixer & possibly, register services
    virtual void            init(GMainLoop * loop, LSHandle * handle,
                                  AudiodCallbacksInterface * interface) = 0;

    /// We might not be ready for programming the mixer
    virtual bool            readyToProgram () = 0;

    /// Program volume of a sink. Will ignore volume
    // of high latency sinks not playing and mute them.
    virtual bool            programVolume(EVirtualSink sink, int volume,
                                           bool ramp = false) = 0;

    /// Program volume of a source.
    virtual bool            programMute(EVirtualSource source, int mute) = 0;

    /// Same as program volume, but ramped.
    virtual bool            rampVolume(EVirtualSink sink, int endVolume) = 0;

    /// Program destination of a sink
    virtual bool            programDestination(EVirtualSink sink,
                                               EPhysicalSink destination) = 0;
    /// Program destination of a source
    virtual bool            programDestination(EVirtualSource source,
                                               EPhysicalSource destination) = 0;

    /// Program a filter
    virtual bool            programFilter(int filterTable) = 0;
    virtual bool            programLatency(int latency) = 0;
    virtual	bool           programBalance(int balance) = 0;
    virtual bool            muteAll() = 0;

    /// Offset a volume by a number of dB. Calculation only.
    virtual int                adjustVolume(int volume, int dB) = 0;

    /// Get active streams set to test which sinks are active.
    virtual VirtualSinkSet    getActiveStreams() = 0;

    /// Get how many streams are active for a particular sink.
    virtual int                getStreamCount(EVirtualSink sink) = 0;

    /// Count how many output streams are opened
    virtual int                getOutputStreamOpenedCount () = 0;

    /// Count how many input streams are opened
    virtual int                getInputStreamOpenedCount () = 0;

    /// Find out if a particular sink is audible.
    // Something must be playing & the volume must be non-null
    virtual bool            isSinkAudible(EVirtualSink sink) = 0;

    /// Suspend all streams (when entering power saving mode).
    virtual bool            suspendAll() = 0;

    /// Suspend sampling rate on sinks/sources
    virtual bool            updateRate(int rate) = 0;

    /// Play a low latency system sound using a particular sink
    virtual bool            playSystemSound(const char *snd, EVirtualSink sink) = 0;

    /// For faster first play, on-demand sounds can be pre-loaded
    virtual void            preloadSystemSound(const char * snd) = 0;

    /// mute/unmute the Physical sink
    virtual bool            setMute(int sink, int mutestatus) = 0;

    /// set volume on a particular diaplyID
    virtual bool            setVolume(int display, int volume) = 0;

    virtual void            playDtmf(const char *snd, EVirtualSink sink) = 0;

    virtual void            playOneshotDtmf(const char *snd, EVirtualSink sink) = 0 ;

    virtual void            stopDtmf()= 0;

    virtual bool            programLoadRTP(const char *type, const char *ip, int port) = 0;
    virtual bool            programHeadsetRoute(int route) = 0;
    virtual bool            programUnloadRTP() = 0;
    virtual bool            loadUSBSinkSource(char cmd, int cardno, int deviceno, int status) = 0;

#ifdef HAVE_BT_SERVICE_V1
    virtual void setBTvolumeSupport(bool value) = 0;
    virtual void sendBTDeviceType(bool type, bool hfpStatus) = 0;
#endif
    virtual bool suspendSink(int sink) = 0;
    virtual void setNREC(bool value) = 0;
    virtual bool programLoadBluetooth (const char *address, const char *profile) = 0;
    virtual bool programUnloadBluetooth (const char *profile) = 0;
    virtual bool setRouting(const ConstString & scenario) = 0;
    virtual bool phoneEvent(EPhoneEvent event, int parameter) = 0;
    virtual bool programCallVoiceOrMICVolume (char cmd, int volume) = 0;
    virtual bool getBTVolumeSupport() = 0;
    virtual void setBTDeviceType(int type) = 0;
    virtual bool setPhoneMuted(const ConstString & scenario, bool muted) = 0;
    virtual bool setPhoneVolume(const ConstString & scenario, int volume) =0;
    virtual int loopback_set_parameters(const char * value) =0;
    virtual inline bool inHfpAgRole(void) = 0;
    virtual inline void setHfpAgRole(bool HfpAgRole) = 0;
};

extern AudioMixer & gAudioMixer;

#define SCENARIO_DEFAULT_LATENCY 65536

#endif /* AUDIOMIXER_H_ */
