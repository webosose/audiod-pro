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

#include <cstring>
#include <unistd.h>

#include "update.h"
#include "module.h"
#include "scenario.h"
#include "genericScenarioModule.h"
#include "AudioDevice.h"
#include "AudioMixer.h"
#include "media.h"
#include "state.h"
#include "volume.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"

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


void
ScenarioModule::programHardwareState ()
{

    if (nullptr == mCurrentScenario)
    {
        g_warning ("%s: no current scenario found", __FUNCTION__);
        return;
    }
    /*if (!gAudioDevice.setRouting(mCurrentScenario->mName,
         eRouting_base,
         mCurrentScenario->getMicGainTics()))
        return;*/
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

bool ScenarioModule::makeCurrent ()
{
    if (!VERIFY(this))
        return false;

    if (sCurrentModule == this)
        return true;

   // g_debug("ScenarioModule::makeCurrent: %s", getCategory());
    LogIndent    indentLogs("| ");

    GenericScenarioModule * previous = sCurrentModule;

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


void ScenarioModule::_updateHardwareSettings(bool muteMediaSink)
{
    if (isCurrentModule())
    {
        //g_debug("ScenarioModule::
        //_updateHardwareSettings: %s", this->getCategory());
        LogIndent    indentLogs("| ");
        gAudioMixer.muteAll ();
        //We've retained the old code changes while separating from Scenario to Generic Architecture
        if (this == getMediaModule())
            usleep (2);  //changes reduced from 200ms to 2 microsecond
        programHardwareState ();
        programSoftwareMixer (true, muteMediaSink);
    }
}

void ScenarioModule::programSoftwareMixer (bool ramp, bool muteMediaSink)
{
    if (VERIFY(isCurrentModule()))
    {
        LogIndent    indentLogs("| ");

        if (!gAudioMixer.readyToProgram())
        {
            g_warning ("%s: audio mixer not ready for programming", __FUNCTION__);
            return;
        }

        if ((!mCurrentScenario) && (!(dynamic_cast <Scenario *> (mCurrentScenario))))
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

        dynamic_cast <Scenario *> (mCurrentScenario)->logRoutes();

        gAudioMixer.programFilter(dynamic_cast <Scenario *> (mCurrentScenario)->mFilter);

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
        // Update routing
        for (EVirtualAudiodSink sink = eVirtualSink_First; sink <= eVirtualSink_Last;
               sink = EVirtualAudiodSink(sink + 1))
        {
            if (!strcmp(mCurrentScenario->getName(),"phone_bluetooth_sco") &&
                 (sink == emedia || sink == edefaultapp))
                 g_debug ("dont move media/defaultapp streams to MainSink");
             else {
                 EPhysicalSink    destination = dynamic_cast <Scenario *> (mCurrentScenario)->getDestination(sink);

                 gAudioMixer.programDestination (sink, destination);
            }
        }
        for (EVirtualSource source = eVirtualSource_First; source <=
               eVirtualSource_Last; source = EVirtualSource(source + 1))
        {
            EPhysicalSource destination = dynamic_cast <Scenario *> (mCurrentScenario)->getDestination(source);

            gAudioMixer.programDestination (source, destination);
        }

//for balance;
         g_message("The volume balance applying for the BT case = %d\n",gState.getSoundBalance());
         gAudioMixer.programBalance(gState.getSoundBalance());

    }
}

bool ScenarioModule::programVolume (EVirtualAudiodSink sink, int volume, bool ramp)
{
    bool routed = true;
    if ((!sCurrentModule) && (!(dynamic_cast <ScenarioModule *> (sCurrentModule))))
    {
        g_warning ("%s: no current module exiting", __FUNCTION__);
        return false;
    }

    ScenarioModule * module = dynamic_cast <ScenarioModule *> (sCurrentModule);
    if ((!module->mCurrentScenario)&& (!(dynamic_cast <Scenario*> (module->mCurrentScenario))))
    {
        g_warning ("%s: no current scenario exiting", __FUNCTION__);
        return false;
    }

    Scenario * scenario = dynamic_cast <Scenario*> (module->mCurrentScenario);
    // Check if the sink is enabled (routed) in the current scenario table
    if (volume && VERIFY(module && module->mCurrentScenario) && (!scenario->isRouted(sink)))
    {
        volume = 0;
        routed = false;
    }

    // always program the volume, even if it's not routed
    return gAudioMixer.programVolume (sink, volume, ramp) && routed;
}

bool ScenarioModule::programMute (EVirtualSource source, int mute)
{
    bool    routed = true;
    if ((!sCurrentModule) && (!(dynamic_cast <ScenarioModule *> (sCurrentModule))))
    {
        g_warning ("%s: no current module exiting", __FUNCTION__);
        return false;
    }

    ScenarioModule * module = dynamic_cast <ScenarioModule *> (sCurrentModule);
    if ((!module->mCurrentScenario)&& (!(dynamic_cast <Scenario*> (module->mCurrentScenario))))
    {
        g_warning ("%s: no current scenario exiting", __FUNCTION__);
        return false;
    }

    Scenario * scenario = dynamic_cast <Scenario*> (module->mCurrentScenario);
    // Check if the sink is enabled (routed) in the current scenario table
    if (mute && VERIFY(module && module->mCurrentScenario) &&
                      !scenario->isRouted(source))
    {
        mute = 1;
        routed = false;
    }

    // always program the volume, even if it's not routed
    return gAudioMixer.programMute (source, mute) && routed;
}

bool ScenarioModule::setScenarioLatency (const char * name, int latency)
{
    if ((!mCurrentScenario) && (!(dynamic_cast <Scenario *> (mCurrentScenario))))
    {
        g_warning ("%s: no current scenario or not a pulseaudio scenario", __FUNCTION__);
        return false;
    }

    GenericScenario * s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (NULL == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
                                           (name ? name : "mCurrentScenario"));
        return false;
    }

    dynamic_cast <Scenario *> (s)->mLatency = latency;
    if (s == mCurrentScenario)
    {
        if (mCurrentScenario->mName == cMedia_A2DP)
            gAudioMixer.programLatency(dynamic_cast <Scenario *>(mCurrentScenario)->mLatency);
        else
            gAudioMixer.programLatency(SCENARIO_DEFAULT_LATENCY);
    }

    return true;
}

bool ScenarioModule::getScenarioLatency (const char * name, int & latency)
{
    if ((!mCurrentScenario) && (!(dynamic_cast <Scenario *> (mCurrentScenario))))
    {
        g_warning ("%s: no current scenario or not a pulseaudio scenario", __FUNCTION__);
        return false;
    }

    GenericScenario * s = mCurrentScenario;
    if (name)
        s = getScenario(name);

    if (NULL == s)
    {
        g_warning ("%s: scenario '%s' not found", __FUNCTION__,
                                           (name ? name : "mCurrentScenario"));
        return false;
    }
    latency = dynamic_cast <Scenario *>(s)->mLatency;
    return true;
}

ScenarioRoute * Scenario::getScenarioRoute(EVirtualAudiodSink sink, bool ringerOn)
{
    if (VERIFY(sink >= 0 && sink < eVirtualSink_Count))
        return ringerOn ? mRoutesRingerOn + sink : mRoutesRingerOff + sink;
    return 0;
}

ScenarioRoute * Scenario::getScenarioRoute(EVirtualSource source, bool ringerOn)
{
    if (VERIFY(source >= 0 && source < eVirtualSource_Count))
        return mRoutesSource + source;
    return 0;
}

void Scenario::configureRoute (EVirtualAudiodSink sink,
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

void Scenario::configureRoute (EVirtualSource source,
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

bool Scenario::isRouted(EVirtualAudiodSink sink)
{
    ScenarioRoute * route = getScenarioRoute(sink, gState.getRingerOn());
    if (VERIFY(route))
        return route->mRouted;
    return false;
}

bool Scenario::isRouted(EVirtualSource source)
{
    ScenarioRoute * route = getScenarioRoute(source, gState.getRingerOn());
    if (VERIFY(route))
        return route->mRouted;
    return false;
}

EPhysicalSink Scenario::getDestination(EVirtualAudiodSink sink)
{
    ScenarioRoute * route = getScenarioRoute(sink, gState.getRingerOn());
    if (VERIFY(route))
        return (EPhysicalSink)route->mDestination;
    return eMainSink;
}

EPhysicalSource Scenario::getDestination(EVirtualSource source)
{
    ScenarioRoute * route = getScenarioRoute(source, gState.getRingerOn());
    if (VERIFY(route))
        return (EPhysicalSource)route->mDestination;
    return eMainSource;
}

void Scenario::logRoutes() const
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
            routeList += virtualSinkName(EVirtualAudiodSink(sink));
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

bool Scenario::setFilter(int filter)
{
    mFilter = filter;

    return true;
}

Scenario::Scenario(const ConstString & name,
                   bool enabled,
                   EScenarioPriority priority,
                   Volume & volume,
                   Volume & micGain) :
    GenericScenario(name, enabled, priority, volume, micGain),
    mFilter(0),
    mLatency(SCENARIO_DEFAULT_LATENCY)
{

}

