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

#ifndef _MEDIA_H_
#define _MEDIA_H_

#include "scenario.h"

class MediaScenarioModule : public ScenarioModule
{
public:
    enum EA2DP {
        eA2DP_Paused,
        eA2DP_Playing
    };
    MediaScenarioModule();
    ~MediaScenarioModule() {};
    MediaScenarioModule(int defaultvolume);
    static void MediaInterfaceExit();
    virtual void    onSinkChanged(EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType);
    virtual void    programControlVolume();
    virtual void    programState();
    virtual void    resumeA2DP();
    virtual void    pauseA2DP();

    virtual void    onActivating();

    // adjust alert when playing media, or when on headset
    virtual int        adjustAlertVolume(int volume, bool alertStarting = false);
    virtual int        adjustSystemVolume(int volume);

    // emedia, eflash, edefaultapp, enavigation
    virtual void programMediaVolumes(bool rampVolumes,
                         bool rampMedia = false,
                         bool muteMedia = false);

    // Ramp down media, wait long enough (if necessary), then mute media module
    virtual void rampDownAndMute();
    virtual bool someMediaIsPlaying();
    virtual bool someMediaIsAudible();
    virtual bool someAlertIsPlaying();
    bool            enableRingtoneRouting(bool enable);
    int                adjustToHeadsetVolume(int volume);
    EVirtualSink    mPreviousSink;

    virtual void sendAckToPowerd(bool isWakeUp);
    void setCurrentState(int value);
    bool getCurrentState(void);
    void setRequestedState(int value);
    bool getRequestedState(void);
    void setA2DPAddress(std::string address);

    bool            _isWirelessStreamingScenario();
    EA2DP mA2DPRequestedState;
    EA2DP mA2DPCurrentState;
    virtual void _updateA2DP(bool play, bool immediate = true);

private:
    int                _applyMinVolume(int adjustedVolume, int originalVolume);
    bool            PriorityToRingtone;
    Volume            mFrontSpeakerVolume;
    Volume            mHeadsetVolume;
    Volume            mA2DPVolume;
    Volume            mWirelessVolume;
    Volume            mHeadsetMicGain;
    guint             alarmTimeoutSource;
    guint             timerTimeoutSource;
    guint             mRingtoneTimeoutSource;
    std::string       mA2DPAddress;

protected:
    const int cAlertDuck = -12;
    Volume mFrontMicGain;
    Volume mBackSpeakerVolume;
    bool mPriorityToAlerts;
    bool mAlertsRoutingActive;
    guint mA2DPUpdateTimerID;
    virtual void _updateRouting();
    virtual void _startSinkPlayback(EVirtualSink sink);
    virtual void _endSinkPlayback(EVirtualSink sink);
    virtual bool _isWirelessStreamActive();
    static gboolean _A2DPDelayedUpdate(gpointer data);
};

MediaScenarioModule * getMediaModule();

inline bool isMediaSink(EVirtualSink sink)
{
    return sink == emedia ||
           sink == eflash ||
           sink == edefaultapp ||
           sink == edefault1 ||
           sink == enavigation;
}

inline bool isWirelessStreamedSink(EVirtualSink sink)
{
    return sink == emedia || sink == eflash || sink == edefault1 || sink == edefault2 || sink == enavigation;
}




#endif // _MEDIA_H_
