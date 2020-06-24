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


#ifndef _AUDIODEVICE_H_
#define _AUDIODEVICE_H_

#include <lunaservice.h>

#include "ConstString.h"
#include "log.h"
#include "state.h"

// Will have to make a conditional include to fetch
// it from the shared location for the lib, or local for audiod
#include "../include/IPC_SharedAudiodDefinitions.h"
#include "AudioMixer.h"

enum ERouting
{
    eRouting_base,        ///< base routing for specified scenario
    eRouting_default,    ///< minimum commands to revert
                          // routing to base scenario changed to cases below
    eRouting_alerts        ///< alternate routing during alerts
};



typedef enum audio_loopback_option{
    AUDIO_LOOPBACK_OFF=0,
    AUDIO_NORMAL_LOOPBACK=1,
    AUDIO_TESTMODE_LOOPBACK=2,
    AUDIO_DELAY_LOOPBACK=3,
    AUDIO_DELAY_LOOPBACK_BT=4,
    AUDIO_LOOPBACK_OPTION_CNT,
    AUDIO_LOOPBACK_OPTION_MAX = AUDIO_LOOPBACK_OPTION_CNT - 1,
} audio_loopback_option_t;

struct audio_device {
    bool mIsLoopbackMode;
    bool mKillLoopbackThread;
    pthread_mutex_t loopback_lock;
    pthread_t loopback_thread;
    audio_loopback_option_t loopback_option;
};


class Callback {
public:
    Callback(void (*cb)(void* parameter), void* p)
    :callback(cb),parameter(p){}
    void (*callback)(void* parameter);
    void* parameter;
};

class volumeSettings;
class AudioDevice
{
protected:
    AudioDevice();
    virtual ~AudioDevice();

public:

    /// Give access to the appropriate platform specific HW object. There can be only one, and that's how audiod gets to access it.
    static AudioDevice &get();

    /*
     * The following methods are requests made by audiod that the HW needs to implement.
     */

    /// Program routing for a specific scenario, using specified routing, with a particular microphone gain. Overload calling base implementation first.
    bool setRouting(const ConstString &scenario, ERouting routing,
                                   int micGain);

    /// Perform the task expected by HW when a particular phone event occurs
    bool phoneEvent(EPhoneEvent event, int parameter = -1);

    // Change volume & mute state
    bool setVolume(int volume);
    bool setMuted(bool muted);
    bool volumeUpDown(bool volumeup);
    bool restoreMediaVolume(const ConstString & scenario, int volume){return true;}

    void disableHW(){}
    void prepareHWForPlayback(){}
    void setPhoneRecording(bool recording){}

    /// Some platforms (Castle...) need to program the modem to handle routing
    bool registerScenario(const char *name)
    {
        return true;
    }

    /// Let HW know if any audio mixer output is active, so that HW can mute/unmute speakers. Overload calling base implementation first.
    void setActiveOutput(bool active)
    {
        mActiveOutput = active;
    }
    bool isActiveOutput() const
    {
        return mActiveOutput;
    }

    /// Let HW know if any audio input is being recorded, so that HW can mute/unmute the appropriate microphone. Overload calling base implementation first.
    void setRecording(bool recording)
    {
        mRecording = recording;
    }
    bool isRecording() const
    {
        return mRecording;
    }

    void hacSet(bool hac)
    {
        mHAC = hac;
    }
    bool hacGet()
    {
        return mHAC;
    }

    /// Set microphone gain
    bool setMicGain(const ConstString &scenario, int gain);

    // get default mic gain
    int getDefaultMicGain(const ConstString &scenario);

    /// Request to check for headset presence
    bool checkHeadsetMic(bool poll)
    {
        return false;
    }

    /// Request various actions from HW
    void generateAlertSound(int sound, Callback *callback);

    /// Some phones (Castle...) can't play alerts in SCO BT. We need to generate beeps instead. These abstract this limitation.
    bool needsSCOBeepAlert()
    {
        return false;
    }
    void generateSCOBeepAlert()
    {
        VERIFY(false);    // must be implemented if needsBTBeepAlert() can return true
    }

    // play a systemsound, looping "count" times,  wait "intervalms" milliseconds between loop,  invoke "callback" when done
    void playThroughPulse(const char *soundname, int count, int intervalms,
                                         Callback *callback);

    // play a systemsound, looping "count" times,  wait "intervalms" milliseconds between loop,  invoke "callback" when done
    // added logic to play only while call mode is not carrier
    void playThroughPulseIncomingTone(const char *soundname, int count,
                                                     int intervalms, Callback *callback);

    void setIncomingCallRinging(bool ringing);
    bool getIncomingCallRinging();

    /// Power management notifications
    void suspend()
    {
        mSuspended = TRUE;
    }
    void unsuspend()
    {
        mSuspended = FALSE;
    }

    bool isSuspended()
    {
        return mSuspended;
    }

    /// Playback prepare
    void prepareForPlayback() {}

    /// a2dp start/stop notifications
    void a2dpStartStreaming() {}
    void a2dpStopStreaming() {}

    /// specify backspeaker filter for pulse
    int getBackSpeakerFilter();


    /// enable or disable audio eq for phone metrico testing
    bool museSet(bool enable);

    /// notify if slider is opened or closed to update routings if necessary
    void sliderChanged() {}

    /// used when a device is connected to phone, hands free relay over bt. for ex. tablet connected to phone.
    void setBTHF(bool enable) {}

    /// Indicates if headset does dsp/echo cancellation or phone does it.
    void setBTSupportEC(bool carkit, bool enableEC) {}

    /*
     * The following methods are called to allow for initialization, customization and get notification of relevant configuration changes.
     * No particular result is expected by audiod.
     */

    /// some HW need configuration files, which location may be specified via command line arguments. This is where these are located.
    void setConfigRegisterSetDirectory(const char *path) {}

    // earliest init point, post static initialization, command line option, logging, but prior to scenario setup or any service
    bool pre_init()
    {
        return true;
    }

    // last point initialization, just prior to running the event loop, after everything else has been setup
    bool post_init()
    {
        return true;
    }

    bool teardown()
    {
        return true;
    }

    EHeadsetState getHeadsetState()
    {
        return mHeadsetState;
    }
    void setHeadsetState(EHeadsetState state)
    {
        mHeadsetState = state;
    }

    const char *getName();

    void setBeatsOnForHeadphones(bool beats)
    {
        mBeatsOnForHeadphones = beats;
    }


    void updateCallMode() {}
    bool isVoiceappRunning() {return false;}

protected:
    EHeadsetState       mHeadsetState;
    ConstString         mCurrentScenario;
    ERouting            mRouting;
    int                 mMicGain;
    bool                mActiveOutput;
    bool                mRecording;
    bool                mHAC;
    bool                mSuspended;
    bool                mBeatsOnForHeadphones;
    bool                mBTSupportsEC;
    bool                mCarkit;
    bool                mIncomingCallRinging;
    ECallMode           mAudioDeviceCallMode;
    static gboolean delayCall_timercallback(void *data);
    static void delayCall(Callback *callback, int millisecond);
    struct PulsePlay
    {
        int count;
        const char *soundname;
        int interval;
        Callback *callback;
    };
    static gboolean playThroughPulseCallback(gpointer callback);
    static gboolean playThroughPulseCallbackIncomingTone(gpointer callback);

};

extern AudioDevice & gAudioDevice;

#endif // _AUDIODEVICE_H_
