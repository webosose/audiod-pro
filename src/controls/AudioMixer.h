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
#include "ConstString.h"

enum EVirtualUMISink
{
    eUmiLivetv = eVirtualSink_Count+1,
    eUmiMediaMusic,
    eUmiMedia,
    eUmiMusic,
    eUmiGallery,
    eUmiOverlay,
    eVirtualUMISink_Count,
    eVirtualUMISink_First = eUmiLivetv,
    eVirtualUMISink_Last = eUmiOverlay,
    eVirtualUMISink_All = eVirtualUMISink_Count,     //Needed if modules want to subscribe all UMI sinks
    eAllSink = eVirtualUMISink_Count                 //Needed if modules want to subscribe all pulse and UMI sinks
};

template <typename EnumT, typename BaseEnumT>
class InheritEnum
{
    public:
        InheritEnum()
        {
        }

        InheritEnum(EnumT e):enum_(e)
        {
        }

        InheritEnum(BaseEnumT e):enum_(static_cast<EnumT>(e))
        {
        }

        explicit InheritEnum( int val):enum_(static_cast<EnumT>(val))
        {
        }

        operator EnumT() const
        {
            return enum_;
        }

        private:
            EnumT enum_;
};

typedef InheritEnum<EVirtualUMISink, EVirtualSink> EVirtualAudiodSink;

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

    void    add(EVirtualAudiodSink sink)     { mSet |= mask(sink); }
    void    remove(EVirtualAudiodSink sink)  { mSet &= ~mask(sink); }

    /// does the set contain this sink?
    bool    contain(EVirtualAudiodSink sink) const
                { return (mSet & mask(sink)) != 0; }

    /// does the set contain any of these two sinks?
    bool    containAnyOf(EVirtualAudiodSink sink1, EVirtualAudiodSink sink2) const
                { return (mSet & (mask(sink1) | mask(sink2))) != 0; }

    /// does the set contain any of these three sinks?
    bool    containAnyOf( EVirtualAudiodSink sink1,
                          EVirtualAudiodSink sink2,
                          EVirtualAudiodSink sink3) const
                { return (mSet & ( mask(sink1) |
                                   mask(sink2) |
                                   mask(sink3))) != 0; }

    /// does the set contain any of these four sinks?
    bool    containAnyOf( EVirtualAudiodSink sink1,
                          EVirtualAudiodSink sink2,
                          EVirtualAudiodSink sink3,
                          EVirtualAudiodSink sink4) const
                { return (mSet & ( mask(sink1) |
                                   mask(sink2) |
                                   mask(sink3) |
                                    mask(sink4))) != 0; }

    bool    containAnyOf( EVirtualAudiodSink sink1,
                          EVirtualAudiodSink sink2,
                          EVirtualAudiodSink sink3,
                          EVirtualAudiodSink sink4,
                          EVirtualAudiodSink sink5) const
                { return (mSet & ( mask(sink1) |
                                   mask(sink2) |
                                   mask(sink3) |
                                   mask(sink4) |
                                   mask(sink5))) != 0; }


    bool    operator==(const VirtualSinkSet & rhs) const { return mSet == rhs.mSet; }
    bool    operator!=(const VirtualSinkSet & rhs) const { return mSet != rhs.mSet; }

private:
    int        mask(EVirtualAudiodSink sink) const         { return 1 << sink; }

    int        mSet;
};

inline bool IsValidVirtualSink(EVirtualAudiodSink sink)
   { return sink >= eVirtualSink_First && sink <= eVirtualSink_Last; }

inline bool IsValidVirtualSource(EVirtualSource source)
    { return source >= eVirtualSource_First && source <= eVirtualSource_Last; }

// Low latency sinks are never muted. Their volume should always be set properly...
inline bool isNeverMutedSink(EVirtualAudiodSink sink)
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
    virtual void        onSinkChanged(EVirtualAudiodSink sink, EControlEvent event,ESinkType p_eSinkType) = 0;

    /// Notification that a first input stream was opened,
    // or that the last input stream was closed
    virtual void        onInputStreamActiveChanged(bool active) = 0;
};

const char * controlEventName(EControlEvent event);
const char * virtualSinkName(EVirtualAudiodSink sink, bool prettyNameNotInternal = true);
const char * virtualSourceName(EVirtualSource source,
                               bool prettyNameNotInternal = true);
EVirtualAudiodSink getSinkByName(const char * name);

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
    virtual bool            programVolume(EVirtualAudiodSink sink, int volume,
                                           bool ramp = false) = 0;

    /// Program volume of a source.
    virtual bool            programMute(EVirtualSource source, int mute) = 0;

    /// Same as program volume, but ramped.
    virtual bool            rampVolume(EVirtualAudiodSink sink, int endVolume) = 0;

    /// Program destination of a sink
    virtual bool            programDestination(EVirtualAudiodSink sink,
                                               EPhysicalSink destination) = 0;
    /// Program destination of a source
    virtual bool            programDestination(EVirtualSource source,
                                               EPhysicalSource destination) = 0;

    /// Program a filter
    virtual bool            programFilter(int filterTable) = 0;
    virtual	bool           programBalance(int balance) = 0;
    virtual bool            muteAll() = 0;

    /// Get active streams set to test which sinks are active.
    virtual VirtualSinkSet    getActiveStreams() = 0;

    /// Suspend all streams (when entering power saving mode).
    virtual bool            suspendAll() = 0;

    /// Suspend sampling rate on sinks/sources
    virtual bool            updateRate(int rate) = 0;

    /// Play a low latency system sound using a particular sink
    virtual bool            playSystemSound(const char *snd, EVirtualAudiodSink sink) = 0;

    /// For faster first play, on-demand sounds can be pre-loaded
    virtual void            preloadSystemSound(const char * snd) = 0;

    /// mute/unmute the Physical sink
    virtual bool            setMute(int sink, int mutestatus) = 0;

    /// set volume on a particular diaplyID
    virtual bool            setVolume(int display, int volume) = 0;

    virtual void            playDtmf(const char *snd, EVirtualAudiodSink sink) = 0;

    virtual void            playOneshotDtmf(const char *snd, EVirtualAudiodSink sink) = 0 ;

    virtual void            stopDtmf()= 0;

    virtual bool            programHeadsetRoute(int route) = 0;
    virtual bool            loadUSBSinkSource(char cmd, int cardno, int deviceno, int status) = 0;
    virtual bool suspendSink(int sink) = 0;
    virtual void setNREC(bool value) = 0;
    virtual bool programLoadBluetooth (const char *address, const char *profile) = 0;
    virtual bool programUnloadBluetooth (const char *profile) = 0;
    virtual bool setRouting(const ConstString & scenario) = 0;
    virtual bool programCallVoiceOrMICVolume (char cmd, int volume) = 0;
    virtual int loopback_set_parameters(const char * value) =0;
};

extern AudioMixer & gAudioMixer;

#define SCENARIO_DEFAULT_LATENCY 65536

#endif /* AUDIOMIXER_H_ */
