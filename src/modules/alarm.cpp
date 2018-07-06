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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>

#include "alarm.h"
#include "system.h"
#include "module.h"
#include "state.h"
#include "phone.h"
#include "media.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"
#include "AudioMixer.h"
#include "ringtone.h"
#include "AudioDevice.h"
#include "log.h"
#include "AudiodCallbacks.h"
#include "vibrate.h"
#include "messageUtils.h"
#include "IPC_SharedAudiodProperties.h"


static AlarmScenarioModule * sAlarmModule = 0;

static const ConstString    cAlarm_Default("alarm" SCENARIO_DEFAULT);

AlarmScenarioModule * getAlarmModule()
{
    return sAlarmModule;
}

void
AlarmScenarioModule::programControlVolume ()
{
    programAlarmVolumes(false);
}

bool
AlarmScenarioModule::setMuted (bool mute)
{
    if(mute &&
        !mMuted &&
        !gAudioMixer.getActiveStreams().contain(ealarm))
        cancelVibrate();
     else
     {
         if(mute)
         {
             mAlarmMuted = true;

             if(mMuted)
                 programMuted();
             else
                 cancelVibrate();
         }
         else
             mAlarmMuted = false;
         return ScenarioModule::setMuted(mute);
     }

    return true;
}

void
AlarmScenarioModule::programMuted ()
{
    if(mMuted)
        cancelVibrate();
    programAlarmVolumes(false);
}

void
AlarmScenarioModule::programAlarmVolumes(bool ramp)
{
    int volume = mAlarmVolume.get ();
    bool ringerOn = gState.getRingerOn();
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();
    programVolume(ealarm, (((ringerOn || getAlarmOn()) && !mAlarmMuted) && activeStreams.contain(ealarm)) ?
                                                             volume : 0, ramp);
}

bool AlarmScenarioModule::getAlarmOn () {
    return gAudiodProperties->mAlarmOn.get();
}

void AlarmScenarioModule::setAlarmOn (bool AlarmOn)
{
    if (getAlarmOn() != AlarmOn)
    {
        g_message ("setting alarm");
        gAudiodProperties->mAlarmOn.set(AlarmOn);
        gState.setPreference(cPref_OverrideRingerForAlaram, AlarmOn);
        if (ScenarioModule * module = dynamic_cast <ScenarioModule *> (ScenarioModule::getCurrent()))
        { 
            module->programSoftwareMixer(true);
        }
    }
}

static bool
_setAlarmOn(LSHandle *lshandle, LSMessage *message, void *ctx)
{
     bool alarmOn;
     const char * reply = STANDARD_JSON_SUCCESS;

     LSMessageJsonParser    msg(message,
                                        SCHEMA_1(REQUIRED(alarmOn, boolean)));

    if (!msg.parse(__FUNCTION__, lshandle)){
       reply = STANDARD_JSON_ERROR(3, "alarm setting failed: missing parameters ?");
       goto error;
    }
    if (!msg.get("alarmOn", alarmOn)) {
       reply = STANDARD_JSON_ERROR(3, "alarm setting failed: missing parameters ?");
       goto error;
    } else {
       getAlarmModule()->setAlarmOn(alarmOn);
    }

    error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static bool
_getAlarmOn(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message,SCHEMA_0);
    pbnjson::JValue    reply = pbnjson::Object();
    reply.put("returnValue", true);
    const std::string name = "AlarmOn";
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;
    bool alarmOn = getAlarmModule()->getAlarmOn();

    reply.put(name.c_str(), alarmOn);
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, jsonToString(reply).c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

gboolean alarmTimeoutForVibrate (gpointer data)
{
    g_debug("Timer expired ... Closing alarm stream");

    if (data) {
        AlarmScenarioModule *alarmModule = (AlarmScenarioModule *)data;
        alarmModule->onSinkChanged(ealarm, eControlEvent_LastStreamClosed,ePulseAudio);
    }

    return false;
}

void
AlarmScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
      g_debug("AlarmScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
                                                         virtualSinkName(sink),
                                                         controlEventName(event),(int)p_eSinkType);
      if(p_eSinkType==ePulseAudio)
      {
        g_debug("AlarmScenarioModule: pulse audio onsink changed");
        if (eControlEvent_FirstStreamOpened == event)
        {
            bool shouldFakeVibrate = false;
            bool fakeVibrateIfCantVibrate = true;
            bool needsSCOBeepAlert = false;

            bool ringerOn = gState.getRingerOn();
            int volume = mAlarmVolume.get();
            if (gState.getOnActiveCall() &&!ringerOn)
            {
                programVolume(sink, 0); // mute alert/alarm sound
                shouldFakeVibrate = true; // fake vibrate,
                                          //even if user has disabled vibrations!
            }else if ((!ringerOn && !getAlarmOn()) || mAlarmMuted)
                programVolume(sink, 0);
            else
                programVolume(sink, volume);

            // Determine if we should vibrate?
            if (gState.shouldVibrate()) // take into account user settings
            {
                if (gState.getOnActiveCall() || gState.getOnThePuck())
                {
                    shouldFakeVibrate = fakeVibrateIfCantVibrate;
                }
                else
                {
                    if (alarmTimeout != 0) {
                        if (g_source_remove(alarmTimeout)) {
                            g_debug("AlarmScenarioModule:Removed timer since new alarm stream is created");
                            alarmTimeout = 0;
                        }
                    } else {
                         if (gAudioMixer.getActiveStreams().containAnyOf(enotifications, ealerts, eeffects, ecalendar))
                             cancelVibrate();
                         getVibrateDevice()->realVibrate("{\"name\":\"alarm\"}");
                    }
                    shouldFakeVibrate = false; // if vibrated, we no need to
                                               // fake vibrate anymore...
                }
                if (!ringerOn && gState.getHeadsetState() != eHeadsetState_None)
                    shouldFakeVibrate = true;
            }

            if (shouldFakeVibrate && (!gState.getOnThePuck() || gState.getOnActiveCall()))
            {    // don't fake vibrate on the puck,
                gAudioMixer.playSystemSound ("alarm_buzz", eeffects);
                if (gAudioDevice.needsSCOBeepAlert())
                    needsSCOBeepAlert = true;
            }
            if (needsSCOBeepAlert)
                gAudioDevice.generateSCOBeepAlert();
            }else if (eControlEvent_LastStreamClosed == event && sink == ealarm){
                if (!alarmTimeout) {
                    g_debug("AlarmScenarioModule: start Timer for 300ms");
                    alarmTimeout = g_timeout_add (300, alarmTimeoutForVibrate, this);
                } else {
                     if (!gAudioMixer.getActiveStreams().contain(ealarm)) {
                         cancelVibrate();
                         setMuted(false);
                         alarmTimeout = 0;
                     }
               }
            }
     }
     else
     {
       g_debug("AlarmScenarioModule: UMI onsink changed");
     }
}

static LSMethod alarmMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_Status, _status},
    { cModuleMethod_SetMuted, _setMuted},
    { cModuleMethod_SetAlarmOn, _setAlarmOn},
    { cModuleMethod_GetAlarmOn, _getAlarmOn},
    { },
};

AlarmScenarioModule::AlarmScenarioModule() :
    ScenarioModule(ConstString("/alarm")),
    mAlarmVolume(cAlarm_Default, 50),
    mAlarmMuted(false),
    alarmTimeout(0)
{
        gAudiodCallbacks.registerModuleCallback(this, ealarm, true);

    Scenario *s = new Scenario (cAlarm_Default,
                                true,
                                eScenarioPriority_System_Default,
                                mAlarmVolume);
    addScenario (s);

    registerMe (alarmMethods);

    restorePreferences();

    mMuted = false;
}

int
AlarmInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sAlarmModule = new AlarmScenarioModule();

    return 0;
}

MODULE_START_FUNC (AlarmInterfaceInit);
