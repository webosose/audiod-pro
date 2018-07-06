// Copyright (c) 2018 LG Electronics, Inc.
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

#include <cstring>
#include <unistd.h>

#include "update.h"
#include "module.h"
#include "genericScenarioModule.h"
#include "AudioDevice.h"
#include "phone.h"
#include "media.h"
#include "ringtone.h"
#include "voicecommand.h"
#include "system.h"
#include "state.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "notification.h"
#include "alarm.h"
#include "timer.h"
#include "alert.h"

GenericScenarioModule * GenericScenarioModule::sCurrentModule = 0;

//We've retained the old code changes while separating from Scenario to Generic Architecture
Volume cNoVolume(ConstString("<Invalid Volume>"), -1);

GenericScenarioModule::GenericScenarioModule(const ConstString & category) :
mCategory(category), mCurrentScenario(0), mStoreTimerID(0),
mMuted(false), mVolumeOverride(0)
{
}

void
GenericScenarioModule::_setCurrentScenarioByPriority ()
{
    GenericScenario * previouslyCurrent = mCurrentScenario;
    for (ScenarioModule::ScenarioMap::iterator iter = mScenarioTable.begin();
        iter != mScenarioTable.end(); ++iter)
    {
        GenericScenario *s = iter->second;
        if (true == s->mEnabled)
        {
            if (nullptr == mCurrentScenario)
            mCurrentScenario = s;
            else if (mCurrentScenario->mPriority < s->mPriority)
            mCurrentScenario = s;
        }
    }
    if (VERIFY (mCurrentScenario != 0) && mCurrentScenario != previouslyCurrent)
    g_message("Scenario '%s' selected by priority", mCurrentScenario->getName());
}

bool
GenericScenarioModule::addScenario(GenericScenario *s)
{
    g_debug ("%s: adding scenario '%s'", __FUNCTION__, s->getName());
    mScenarioTable[s->getName()] = s;

    // To do?: notify of scenario being added
    if (true == s->mEnabled)
    {
        // check if the new scenario should become current
        if (nullptr == mCurrentScenario)
            mCurrentScenario = s;
        else if (mCurrentScenario->mPriority < s->mPriority)
            mCurrentScenario = s;
    }
    return true;
}

bool
GenericScenarioModule::enableScenario (const char * name)
{
    GenericScenario* s = getScenario(name);
    if (nullptr != s)
    {
        g_message("Scenario '%s' enabled", s->getName());
        LogIndent indentLogs("| ");
        if (false == s->mEnabled)
        {
            s->mEnabled = true;
            CHECK(sendEnabledUpdate (name, UPDATE_ENABLED_SCENARIO));
        }
        //commented for BT routing : this check is not required
        //if (!isCurrentModule())
        setCurrentScenarioByPriority();
        return true;
    }
    return false;
}

bool
GenericScenarioModule::setCurrentScenario (const char * name)
{
    GenericScenario* s = getScenario(name);
    if (nullptr != s && true == s->mEnabled)
    {
        if (s != mCurrentScenario)
        {
            g_message("Scenario '%s' selected", s->getName());
            int flags = UPDATE_CHANGED_SCENARIO;
            int volume = -1;
            int micgain = -1;

            if (mCurrentScenario)
            {
                volume = mCurrentScenario->getVolume();
                if (mCurrentScenario->hasMicGain())
                micgain = mCurrentScenario->getMicGain();
            }

            mCurrentScenario = s;

            if (mCurrentScenario && mCurrentScenario->getVolume() != volume)
                flags |= UPDATE_CHANGED_VOLUME;
            if (mCurrentScenario &&
                mCurrentScenario->hasMicGain() &&
                mCurrentScenario->getMicGain() != micgain)
                flags |= UPDATE_CHANGED_MICGAIN;

            CHECK(sendChangedUpdate(flags));
            _updateHardwareSettings();
        }
        return true;
    }
    return false;
}

bool
GenericScenarioModule::setCurrentScenarioByPriority()
{
    GenericScenario * s = mCurrentScenario;

    if (nullptr == s || (true == s->mEnabled))
    {
        int flags = 0;
        int volume = -1;
        int micgain = -1;

        if (mCurrentScenario)
        {
            volume = mCurrentScenario->getVolume();
            if (mCurrentScenario->hasMicGain())
                micgain = mCurrentScenario->getMicGain();
        }

        _setCurrentScenarioByPriority();

        if (mCurrentScenario != s)
            flags |= UPDATE_CHANGED_SCENARIO;
        if (mCurrentScenario && mCurrentScenario->getVolume() != volume)
            flags |= UPDATE_CHANGED_VOLUME;
        if (mCurrentScenario &&
            mCurrentScenario->hasMicGain() &&
            mCurrentScenario->getMicGain() != micgain)
            flags |= UPDATE_CHANGED_MICGAIN;

        if (flags)
        {
            CHECK(sendChangedUpdate(flags));
            _updateHardwareSettings();
        }
        return true;
    }
    return false;
}

bool
GenericScenarioModule::unsetCurrentScenario (const char * name)
{
    GenericScenario * s = mCurrentScenario;
    if (nullptr != s && true == s->mEnabled && s->mName == name)
    {
        int flags = 0;
        int volume = -1;
        int micgain = -1;

        if (mCurrentScenario)
        {
            volume = mCurrentScenario->getVolume();
            if (mCurrentScenario->hasMicGain())
            micgain = mCurrentScenario->getMicGain();
        }

        EScenarioPriority priority = s->mPriority;
        s->mPriority = eScenarioPriority_Special_Lowest;
        _setCurrentScenarioByPriority();
        s->mPriority = priority;

        if (mCurrentScenario != s)
            flags |= UPDATE_CHANGED_SCENARIO;
        if (mCurrentScenario && mCurrentScenario->getVolume() != volume)
            flags |= UPDATE_CHANGED_VOLUME;
        if (mCurrentScenario &&
            mCurrentScenario->hasMicGain() &&
            mCurrentScenario->getMicGain() != micgain)
            flags |= UPDATE_CHANGED_MICGAIN;

        CHECK(sendChangedUpdate(flags));
        _updateHardwareSettings();

        return true;
    }
    return false;
}

bool
GenericScenarioModule::disableScenario (const char * name)
{
    GenericScenario* s = getScenario(name);
    if (nullptr != s)
    {
        // if the scenario is already disabled
        // don't bother to do anything, just return
        if (!s->mEnabled)
            return true;

        // if the scenario cannot be disabled
        // do not allow it to be disabled
        if (s->mHardwired)
            return false;

        g_message("Scenario '%s' disabled", s->getName());
        LogIndent    indentLogs("| ");

        s->mEnabled = false;
        if (s == mCurrentScenario)
        {
            int flags = UPDATE_CHANGED_SCENARIO;
            int volume = -1;
            int micgain = -1;

            if (mCurrentScenario)
            {
                volume = mCurrentScenario->getVolume();
                if (mCurrentScenario->hasMicGain())
                    micgain = mCurrentScenario->getMicGain();
            }

            mCurrentScenario = nullptr;
            _setCurrentScenarioByPriority();

            if (mCurrentScenario && mCurrentScenario->getVolume() != volume)
                flags |= UPDATE_CHANGED_VOLUME;
            if (mCurrentScenario &&
                mCurrentScenario->hasMicGain() &&
                mCurrentScenario->getMicGain() != micgain)
                flags |= UPDATE_CHANGED_MICGAIN;

            CHECK(sendChangedUpdate(flags));

            // mutes media sink if scenario is changed to back speaker
            if (mCurrentScenario)
            {
                bool    pauseAllMedia = mCurrentScenario->mName == cMedia_BackSpeaker;
                _updateHardwareSettings (pauseAllMedia);// mute media right
                //away to avoid bleed
                if (pauseAllMedia)
                gState.pauseAllMedia();
            }
        }

        CHECK(sendEnabledUpdate(name, UPDATE_DISABLED_SCENARIO));

        return true;
    }
    return false;
}

bool
GenericScenarioModule::setScenarioVolumeOrMicGain (const char * name,
int volume,
bool volumeNotMicGain)
{
    GenericScenario * s = mCurrentScenario;
    if (name)
    s = getScenario(name);

    if (nullptr == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
        (name ? name : "mCurrentScenario"));
        return false;
    }

    if (s->getVolumeOrMicGain(volumeNotMicGain) != volume)
    {
        g_debug("%s: %s%s %s set to %i", __FUNCTION__, s->getName(),
        (name == 0 ? " (current scenario)" : ""),
        (volumeNotMicGain ? "volume" : "mic_gain"), volume);
        s->setVolumeOrMicGain(volume, volumeNotMicGain);
        if (s == mCurrentScenario)
        {
            CHECK(sendChangedUpdate (volumeNotMicGain ?
            UPDATE_CHANGED_VOLUME :
            UPDATE_CHANGED_MICGAIN));

            programControlVolume();
            programHardwareVolume();
        }
        // for now, only persist volume
        if (volumeNotMicGain)
            scheduleStorePreferences();
    }

    if (volumeNotMicGain)
    {
        if (cVolumePercent_Max == volume)
            broadcastEvent ("maxVolume");
        if (cVolumePercent_Min == volume)
            broadcastEvent ("minVolume");
    }

    return true;
}

bool
GenericScenarioModule::getScenarioVolumeOrMicGain (const char * name,
int & volume,
bool volumeNotMicGain)
{
    GenericScenario * s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (nullptr == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
        (name ? name : "mCurrentScenario"));
        return false;
    }

    volume = s->getVolumeOrMicGain(volumeNotMicGain);

    return true;
}

GenericScenario *
GenericScenarioModule::getScenario (const char * name)
{
    GenericScenarioModule::ScenarioMap::iterator iter = mScenarioTable.find(name);
    return (iter != mScenarioTable.end()) ? iter->second : 0;
}

int
GenericScenarioModule::adjustSystemVolume(int volume)
{
    return getMediaModule()->adjustToHeadsetVolume(volume);
}

GenericScenario::GenericScenario(const ConstString & name,
bool enabled,
EScenarioPriority priority,
Volume & volume,
Volume & micGain) :
mName(name),
mEnabled(enabled),
mHardwired(enabled),
mPriority(priority),
mVolume(volume), mMicGain(micGain)
{
    // some HW need to know about every scenario
    gAudioDevice.registerScenario(name);
}

