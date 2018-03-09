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

#include <cstring>
#include <unistd.h>

#include "update.h"
#include "module.h"
#include "scenario.h"
#include "AudioDevice.h"
#include "AudioMixer.h"
#include "phone.h"
#include "media.h"
#include "ringtone.h"
#include "voicecommand.h"
#include "system.h"
#include "state.h"
#include "volume.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "notification.h"
#include "alarm.h"
#include "timer.h"
#include "alert.h"

#define VOICE_COMMAND_SAMPLING_RATE 8000
#define PHONE_SAMPLING_RATE 8000
#define MEDIA_SAMPLING_RATE 44100

const ConstString    cMedia_Default(MEDIA_SCENARIO_DEFAULT);
const ConstString    cMedia_FrontSpeaker(MEDIA_SCENARIO_FRONT_SPEAKER);
const ConstString    cMedia_BackSpeaker(MEDIA_SCENARIO_BACK_SPEAKER);
const ConstString    cMedia_Headset(MEDIA_SCENARIO_HEADSET);
const ConstString    cMedia_HeadsetMic(MEDIA_SCENARIO_HEADSET_MIC);
const ConstString    cMedia_A2DP(MEDIA_SCENARIO_A2DP);
const ConstString    cMedia_Wireless(MEDIA_SCENARIO_WIRELESS);

const ConstString    cMedia_Mic_Front(MEDIA_MIC_FRONT);
const ConstString    cMedia_Mic_HeadsetMic(MEDIA_MIC_HEADSET_MIC);

const ConstString    cPhone_FrontSpeaker(PHONE_SCENARIO_FRONT_SPEAKER);
const ConstString    cPhone_BackSpeaker(PHONE_SCENARIO_BACK_SPEAKER);
const ConstString    cPhone_Headset(PHONE_SCENARIO_HEADSET);
const ConstString    cPhone_HeadsetMic(PHONE_SCENARIO_HEADSET_MIC);
const ConstString    cPhone_BluetoothSCO(PHONE_SCENARIO_BLUETOOTH_SCO);
const ConstString    cPhone_TTY_Full(PHONE_SCENARIO_TTY_FULL);
const ConstString    cPhone_TTY_HCO(PHONE_SCENARIO_TTY_HCO);
const ConstString    cPhone_TTY_VCO(PHONE_SCENARIO_TTY_VCO);

const ConstString    cVoiceCommand_BackSpeaker(VOICE_COMMAND_SCENARIO_BACK_SPEAKER);
const ConstString    cVoiceCommand_Headset(VOICE_COMMAND_SCENARIO_HEADSET);
const ConstString    cVoiceCommand_HeadsetMic(VOICE_COMMAND_SCENARIO_HEADSET_MIC);
const ConstString    cVoiceCommand_BluetoothSCO(VOICE_COMMAND_SCENARIO_BLUETOOTH_SCO);

const ConstString    cVoiceCommand_Mic_Front(VOICE_COMMAND_ );
const ConstString    cVoiceCommand_Mic_HeadsetMic(VOICE_COMMAND_MIC_HEADSET_MIC);
const ConstString    cVoiceCommand_Mic_BluetoothSCO(VOICE_COMMAND_MIC_BLUETOOTH_SCO);

const ConstString    cVvm_BackSpeaker(VVM_SCENARIO_BACK_SPEAKER);
const ConstString    cVvm_FrontSpeaker(VVM_SCENARIO_FRONT_SPEAKER);
const ConstString    cVvm_Headset(VVM_SCENARIO_HEADSET);
const ConstString    cVvm_HeadsetMic(VVM_SCENARIO_HEADSET_MIC);
const ConstString    cVvm_BluetoothSCO(VVM_SCENARIO_BLUETOOTH_SCO);

const ConstString    cVvm_Mic_Front(VVM_);
const ConstString    cVvm_Mic_HeadsetMic(VVM_MIC_HEADSET_MIC);
const ConstString    cVvm_Mic_BluetoothSCO(VVM_MIC_BLUETOOTH_SCO);


Volume    cNoVolume(ConstString("<Invalid Volume>"), -1);

ScenarioModule * ScenarioModule::sCurrentModule = 0;

ScenarioModule::ScenarioModule(const ConstString & category) :
    mCategory(category), mCurrentScenario(0), mStoreTimerID(0),
    mMuted(false), mVolumeOverride(0)
{
}

bool
ScenarioModule::makeCurrent ()
{
    if (!VERIFY(this))
        return false;

    if (sCurrentModule == this)
        return true;

    LogIndent    indentLogs("| ");

    ScenarioModule * previous = sCurrentModule;

    if (previous)
        previous->onDeactivated();

    sCurrentModule = this;

    onActivating();

    if(!strcmp(sCurrentModule->getCategory(),"media")){
        ConstString scenario = (ConstString)sCurrentModule->getCurrentScenarioName();
        g_message("Restoring hardware volume %s",scenario.c_str());
        gAudioDevice.restoreMediaVolume(scenario,60);
    }

    gAudioMixer.muteAll ();
    programHardwareState ();
    programSoftwareMixer(true);

    programMuted ();

    CHECK(sendChangedUpdate (UPDATE_CHANGED_ACTIVE));

    if (previous)
        CHECK(previous->sendChangedUpdate (UPDATE_CHANGED_ACTIVE));

    return true;
}

void
ScenarioModule::_setCurrentScenarioByPriority ()
{
    Scenario *    previouslyCurrent = mCurrentScenario;
    for (ScenarioModule::ScenarioMap::iterator iter = mScenarioTable.begin();
                                          iter != mScenarioTable.end(); ++iter)
    {
        Scenario *s = iter->second;
        if (true == s->mEnabled)
        {
            if (NULL == mCurrentScenario)
                mCurrentScenario = s;
            else if (mCurrentScenario->mPriority < s->mPriority)
                mCurrentScenario = s;
        }
    }
    if (VERIFY(mCurrentScenario != 0) && mCurrentScenario != previouslyCurrent)
        g_message("Scenario '%s' selected by priority", mCurrentScenario->getName());
}

void
ScenarioModule::_updateHardwareSettings(bool muteMediaSink)
{
    if (isCurrentModule())
    {
        LogIndent    indentLogs("| ");
        gAudioMixer.muteAll ();
        if (this == getMediaModule())
            usleep (2);  //changes reduced from 200ms to 2 microsecond
        programHardwareState ();
        programSoftwareMixer (true, muteMediaSink);
    }
}

bool
ScenarioModule::addScenario(Scenario *s)
{
    g_debug ("%s: adding scenario '%s'", __FUNCTION__, s->getName());
    mScenarioTable[s->getName()] = s;

    // To do?: notify of scenario being added
    if (true == s->mEnabled)
    {
        // check if the new scenario should become current
        if (NULL == mCurrentScenario)
            mCurrentScenario = s;
        else if (mCurrentScenario->mPriority < s->mPriority)
            mCurrentScenario = s;
    }
    return true;
}

bool
ScenarioModule::enableScenario (const char * name)
{
    Scenario* s = getScenario(name);
    if (NULL != s)
    {
        g_message("Scenario '%s' enabled", s->getName());
        LogIndent    indentLogs("| ");
        if (false == s->mEnabled)
        {
            s->mEnabled = true;
            CHECK(sendEnabledUpdate (name, UPDATE_ENABLED_SCENARIO));
        }
        //commented for BT routing : this check is not required
        setCurrentScenarioByPriority();
        return true;
    }
    return false;
}

void
ScenarioModule::programHardwareState ()
{
    if (NULL == mCurrentScenario)
    {
        g_warning ("%s: no current scenario found", __FUNCTION__);
        return;
    }
    if(!gAudioMixer.inHfpAgRole()) {
        if (!gAudioMixer.setRouting(mCurrentScenario->mName))
            return;
    }

    programState();

    if (mCurrentScenario->mName == "phone_bluetooth_sco" && gAudioMixer.getBTVolumeSupport()) {
        g_debug ("programHardwareState: BT has volume support, set MAX volume on DSP");
        gAudioMixer.programCallVoiceOrMICVolume ( 'a', 100);
    }

    programHardwareVolume();
    if (mMuted)
        programMuted();
}

void
ScenarioModule::programSoftwareMixer (bool ramp, bool muteMediaSink)
{
    if (VERIFY(isCurrentModule()))
    {
        LogIndent    indentLogs("| ");

        if (!gAudioMixer.readyToProgram())
        {
            g_warning ("%s: audio mixer not ready for programming", __FUNCTION__);
            return;
        }

        if (!mCurrentScenario)
        {
            g_warning ("%s: no current scenario", __FUNCTION__);
            return;
        }


        ConstString tail;

        if (mCurrentScenario->mName.hasPrefix(VOICE_COMMAND_, tail)) {
            gAudioMixer.updateRate(VOICE_COMMAND_SAMPLING_RATE);
        } else if (mCurrentScenario->mName.hasPrefix(PHONE_, tail)) {
            gAudioMixer.updateRate(PHONE_SAMPLING_RATE);
        } else {
            gAudioMixer.updateRate(MEDIA_SAMPLING_RATE);
        }

        mCurrentScenario->logRoutes();

        gAudioMixer.programFilter (mCurrentScenario->mFilter);

        /* effects' volume is only ever set here, when we switch module
        in particular, which is the only possible cause for a change*/
        if (gState.getRingerOn()) {
            gAudioMixer.programVolume (eeffects, gState.getOnActiveCall () ?
                                                     25 : 50);
        } else
            //change to mute alerts when ringer switch of OFF/ON
            gAudioMixer.programVolume (eeffects, gState.getOnActiveCall () ?
                                                     25 : 0);

        // emedia, eflash, edefaultapp, enavigation
        //getMediaModule()->programMediaVolumes(ramp, ramp, muteMediaSink);
        //changed to implement policy of restoring volume level of sinks after headset is removed
        getMediaModule()->programMediaVolumes(ramp, ramp, FALSE);
        // eringtones
        getRingtoneModule()->programRingToneVolumes(ramp);
        // eDTMF, efeedback
        getSystemModule()->programSystemVolumes(ramp);
        // evoicedial
        getVoiceCommandModule()->programVoiceCommandVolume(ramp);
        // enotifications, ecalendar
        getNotificationModule()->programNotificationVolumes(ramp);
        // ealarm
        getAlarmModule()->programAlarmVolumes(ramp);
       // etimer
        getTimerModule()->programTimerVolumes(ramp);
       //ealerts,effects
        getAlertModule()->programAlertVolumes(ramp);
        //ecallertone
        getPhoneModule()->programCallertoneVolume(ramp);
        // Update routing
        for (EVirtualSink sink = eVirtualSink_First; sink <= eVirtualSink_Last;
               sink = EVirtualSink(sink + 1))
        {
            if (!strcmp(mCurrentScenario->getName(),"phone_bluetooth_sco") &&
                 (sink == emedia || sink == edefaultapp))
                 g_debug ("dont move media/defaultapp streams to MainSink");
             else {
                 EPhysicalSink    destination = mCurrentScenario->getDestination(sink);

                 gAudioMixer.programDestination (sink, destination);
            }
        }
        for (EVirtualSource source = eVirtualSource_First; source <=
               eVirtualSource_Last; source = EVirtualSource(source + 1))
        {
            EPhysicalSource destination = mCurrentScenario->getDestination(source);

            gAudioMixer.programDestination (source, destination);
        }

//for balance;
         g_message("The volume balance applying for the BT case = %d\n",gState.getSoundBalance());
         gAudioMixer.programBalance(gState.getSoundBalance());

    }
}

bool
ScenarioModule::programVolume (EVirtualSink sink, int volume, bool ramp)
{
    bool    routed = true;
    // Check if the sink is enabled (routed) in the current scenario table
    if (volume && VERIFY(sCurrentModule && sCurrentModule->mCurrentScenario) &&
                          !sCurrentModule->mCurrentScenario->isRouted(sink))
    {
        volume = 0;
        routed = false;
    }

    // always program the volume, even if it's not routed
    return gAudioMixer.programVolume (sink, volume, ramp) && routed;
}

bool
ScenarioModule::programMute (EVirtualSource source, int mute)
{
    bool    routed = true;
    // Check if the sink is enabled (routed) in the current scenario table
    if (mute && VERIFY(sCurrentModule && sCurrentModule->mCurrentScenario) &&
                      !sCurrentModule->mCurrentScenario->isRouted(source))
    {
        mute = 1;
        routed = false;
    }

    // always program the volume, even if it's not routed
    return gAudioMixer.programMute (source, mute) && routed;
}

bool
ScenarioModule::setCurrentScenario (const char * name)
{
    Scenario* s = getScenario(name);
    if (NULL != s && true == s->mEnabled)
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
ScenarioModule::setCurrentScenarioByPriority()
{
    Scenario* s = mCurrentScenario;

    if (NULL == s || (true == s->mEnabled))
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
ScenarioModule::unsetCurrentScenario (const char * name)
{
    Scenario* s = mCurrentScenario;
    if (NULL != s && true == s->mEnabled && s->mName == name)
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
ScenarioModule::disableScenario (const char * name)
{
    Scenario* s = getScenario(name);
    if (NULL != s)
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

            mCurrentScenario = NULL;
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
ScenarioModule::setScenarioVolumeOrMicGain (const char * name,
                                            int volume,
                                            bool volumeNotMicGain)
{
    Scenario* s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (NULL == s)
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
ScenarioModule::getScenarioVolumeOrMicGain (const char * name,
                                             int & volume,
                                             bool volumeNotMicGain)
{
    Scenario* s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (NULL == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
                                           (name ? name : "mCurrentScenario"));
        return false;
    }

    volume = s->getVolumeOrMicGain(volumeNotMicGain);

    return true;
}

bool
ScenarioModule::setScenarioLatency (const char * name, int latency)
{
    Scenario* s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (NULL == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
                                           (name ? name : "mCurrentScenario"));
        return false;
    }

    s->mLatency = latency;
    if (s == mCurrentScenario)
    {
        if (mCurrentScenario->mName == cMedia_A2DP)
            gAudioMixer.programLatency(mCurrentScenario->mLatency);
        else
            gAudioMixer.programLatency(SCENARIO_DEFAULT_LATENCY);
    }

    return true;
}

bool
ScenarioModule::getScenarioLatency (const char * name, int & latency)
{
    Scenario* s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (NULL == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
                                           (name ? name : "mCurrentScenario"));
        return false;
    }

    latency = s->mLatency;

    return true;
}

Scenario *
ScenarioModule::getScenario (const char * name)
{
    ScenarioModule::ScenarioMap::iterator iter = mScenarioTable.find(name);
    return (iter != mScenarioTable.end()) ? iter->second : 0;
}

int
ScenarioModule::adjustSystemVolume(int volume)
{
    return getMediaModule()->adjustToHeadsetVolume(volume);
}

ScenarioRoute *
Scenario::getScenarioRoute(EVirtualSink sink, bool ringerOn)
{
    if (VERIFY(sink >= 0 && sink < eVirtualSink_Count))
        return ringerOn ? mRoutesRingerOn + sink : mRoutesRingerOff + sink;
    return 0;
}

ScenarioRoute *
Scenario::getScenarioRoute(EVirtualSource source, bool ringerOn)
{
    if (VERIFY(source >= 0 && source < eVirtualSource_Count))
        return mRoutesSource + source;
    return 0;
}

void
Scenario::configureRoute (EVirtualSink sink,
                          EPhysicalSink destination,
                          bool ringerSwitchOn,
                          bool enabled)
{
    ScenarioRoute * route = getScenarioRoute(sink, ringerSwitchOn);
    if (VERIFY(route))
    {
        route->mDestination = destination;
        route->mRouted = enabled;
    }
}

void
Scenario::configureRoute (EVirtualSource source,
                          EPhysicalSource destination,
                          bool ringerSwitchOn,
                          bool enabled)
{
    ScenarioRoute * route = getScenarioRoute(source, ringerSwitchOn);
    if (VERIFY(route))
    {
        route->mDestination = destination;
        route->mRouted = enabled;
    }
}

bool
Scenario::isRouted(EVirtualSink sink)
{
    ScenarioRoute * route = getScenarioRoute(sink, gState.getRingerOn());
    if (VERIFY(route))
        return route->mRouted;
    return false;
}

bool
Scenario::isRouted(EVirtualSource source)
{
    ScenarioRoute * route = getScenarioRoute(source, gState.getRingerOn());
    if (VERIFY(route))
        return route->mRouted;
    return false;
}

EPhysicalSink
Scenario::getDestination(EVirtualSink sink)
{
    ScenarioRoute * route = getScenarioRoute(sink, gState.getRingerOn());
    if (VERIFY(route))
        return (EPhysicalSink)route->mDestination;
    return eMainSink;
}

EPhysicalSource
Scenario::getDestination(EVirtualSource source)
{
    ScenarioRoute * route = getScenarioRoute(source, gState.getRingerOn());
    if (VERIFY(route))
        return (EPhysicalSource)route->mDestination;
    return eMainSource;
}

void
Scenario::logRoutes() const
{
    std::string    routeList;
    routeList.reserve(200);
    bool    ringerOn = gState.getRingerOn();
    const ScenarioRoute * routes = (ringerOn ? mRoutesRingerOn : mRoutesRingerOff);
    for (int sink = eVirtualSink_First; sink <= eVirtualSink_Last; ++sink)
    {
        if (routes[sink].mRouted)
        {
            if (routeList.size() > 0)
                routeList += ", ";
            routeList += virtualSinkName(EVirtualSink(sink));
        }
    }
    g_message("Routes for %s ringer %s: %s.", this->getName(),
                                   ringerOn ? "on" : "off", routeList.c_str());
    const ScenarioRoute * sourceroutes = mRoutesSource;
    for (int source = eVirtualSource_First; source <= eVirtualSource_Last; ++source)
    {
        if (sourceroutes[source].mRouted)
        {
            if (routeList.size() > 0)
                routeList += ", ";
            routeList += virtualSourceName(EVirtualSource(source));
        }
    }
    g_message("SourceRoutes for %s ringer %s: %s.", this->getName(),
                                     ringerOn ? "on" : "off", routeList.c_str());
}

bool
Scenario::setFilter(int filter)
{
    mFilter = filter;

    return true;
}

Scenario::Scenario(const ConstString & name,
                   bool enabled,
                   EScenarioPriority priority,
                   Volume & volume,
                   Volume & micGain) :
    mName(name),
    mEnabled(enabled),
    mHardwired(enabled),
    mPriority(priority),
    mFilter(0),
    mLatency(SCENARIO_DEFAULT_LATENCY),
    mVolume(volume), mMicGain(micGain)
{
    // some HW need to know about every scenario
    gAudioDevice.registerScenario(name);
}

