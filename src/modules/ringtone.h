// Copyright (c) 2012-2018 LG Electronics, Inc.
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

#ifndef _RINGTONE_H_
#define _RINGTONE_H_

#include <lunaservice.h>

#include "scenario.h"

class RingtoneScenarioModule : public ScenarioModule
{
public:
    RingtoneScenarioModule();

    RingtoneScenarioModule(int default_volume);

    ~RingtoneScenarioModule() {};

    static void RingtoneInterfaceExit();

    virtual bool    setMuted (bool muted);

    virtual void    onSinkChanged(EVirtualSink sink, EControlEvent event,ESinkType p_eSinkType);

    virtual void    programControlVolume();
    virtual void    programMuted();

    //! eringtones, ealarm, ealerts, enotifications, ecalendar
    virtual void    programRingToneVolumes(bool ramp);

    //! for private use only. Public because global callbacks need them... :-(
    void    SCOBeepAlarm(int runningCount = 0);

    //! Returns true if a ringtone or an alarm was muted
    virtual bool            muteIfRinging();

    //! Silence ringtone & vibration without muting module
    void    silenceRingtone();

    //! Calculate volume adjusted for current situation
    int    getAdjustedRingtoneVolume(bool alertStarting = false) const;
    // get/set the ringtone with vibration
    bool    getRingtoneVibration();

    void    setRingtoneVibration(bool enable);

    bool    checkRingtoneVibration();
protected :
    Volume    mRingtoneVolume;
    bool    mRingtoneMuted;
    bool    mSCOBeepAlarmRunning;
private:
    bool    mIsRingtoneCombined;
};


RingtoneScenarioModule * getRingtoneModule();

void RingtoneCancelVibrate ();

#endif // _RINGTONE_H_
