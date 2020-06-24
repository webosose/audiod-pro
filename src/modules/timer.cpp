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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>

#include "timer.h"
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
#include "messageUtils.h"
#include "IPC_SharedAudiodProperties.h"

static TimerScenarioModule * sTimerModule = 0;

static const ConstString    cTimer_Default("timer" SCENARIO_DEFAULT);

TimerScenarioModule * getTimerModule()
{
    return sTimerModule;
}

void
TimerScenarioModule::programControlVolume ()
{
    programTimerVolumes(false);
}

bool
TimerScenarioModule::setMuted (bool mute)
{
    if(mute &&
        !mMuted &&
        !gAudioMixer.getActiveStreams().contain(etimer))
    {
        //will be removed if alarm module is removed as part of DAP
        //cancelVibrate();
    }
    else
    {
        if(mute)
        {
           mTimerMuted = true;
           if(mMuted)
               programMuted();
           else
           {
               //will be removed if alarm module is removed as part of DAP
               //cancelVibrate();
           }
        }
        else
           mTimerMuted = false;
        return ScenarioModule::setMuted(mute);
    }

    return true;
}
void
TimerScenarioModule::programMuted ()
{
    if(mMuted)
    {
        //will be removed if alarm module is removed as part of DAP
        //cancelVibrate();
    }
    programTimerVolumes(false);
}

void
TimerScenarioModule::programTimerVolumes(bool ramp)
{
    int volume = mTimerVolume.get();
    bool ringerOn = gState.getRingerOn();
    bool dndOn = false;

    gState.getPreference(cPref_DndOn, dndOn);
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();
    programVolume(etimer, (!dndOn && ((ringerOn || getTimerOn()) && !mTimerMuted) && activeStreams.contain(etimer)) ?
                                                                 volume : 0, ramp);
}

bool TimerScenarioModule::getTimerOn () {
    return gAudiodProperties->mTimerOn.get();
}
void TimerScenarioModule::setTimerOn (bool TimerOn)
{
    if (getTimerOn() != TimerOn)
    {
        g_message ("setting timer");
        gAudiodProperties->mTimerOn.set(TimerOn);
        gState.setPreference(cPref_OverrideRingerForTimer, TimerOn);
        if (ScenarioModule * module = dynamic_cast <ScenarioModule *> (ScenarioModule::getCurrent()))
        {
            module->programSoftwareMixer(true);
        }
    }
}

static bool
_setTimerOn(LSHandle *lshandle, LSMessage *message, void *ctx)
{
     bool timerOn;
     const char * reply = STANDARD_JSON_SUCCESS;

     LSMessageJsonParser    msg(message,
                                        SCHEMA_1(REQUIRED(timerOn, boolean)));

    if (!msg.parse(__FUNCTION__, lshandle)){
       reply = STANDARD_JSON_ERROR(3, "timer setting failed: missing parameters ?");
       goto error;
    }
    if (!msg.get("timerOn", timerOn)) {
       reply = STANDARD_JSON_ERROR(3, "timer setting failed: missing parameters ?");
       goto error;
    } else {
         getTimerModule()->setTimerOn(timerOn);
    }

    error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static bool
_getTimerOn(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message,SCHEMA_0);
    pbnjson::JValue    reply = pbnjson::Object();
    const std::string name = "TimerOn";
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;
    bool timerOn = getTimerModule()->getTimerOn();
    reply.put(name.c_str(), timerOn);
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, jsonToString(reply).c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

gboolean timerTimeoutForVibrate (gpointer data)
{
    g_debug("Timer expired ... Closing timer stream");

    if (data) {
        TimerScenarioModule *timerModule = (TimerScenarioModule *)data;
        timerModule->onSinkChanged(etimer, eControlEvent_LastStreamClosed,ePulseAudio);
    }

    return false;
}

void
TimerScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event,ESinkType p_eSinkType)
{
    g_debug("TimerScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
                                                     virtualSinkName(sink),
                                                     controlEventName(event),(int)p_eSinkType);
      if(p_eSinkType==ePulseAudio)
      {
            g_debug("TimerScenarioModule: pulse audio onsink changed");                                               

          if (eControlEvent_FirstStreamOpened == event)
          {
              bool shouldFakeVibrate = false;
              bool fakeVibrateIfCantVibrate = true;
              bool needsSCOBeepAlert = false;
              bool dndOn = false;

              bool ringerOn = gState.getRingerOn();
              int volume = mTimerVolume.get();
              gState.getPreference(cPref_DndOn, dndOn);

              if (gState.getOnActiveCall() && !ringerOn)
              {
                  programVolume(sink, 0);
                  shouldFakeVibrate = true; // fake vibrate,
                                                //even if user has disabled vibrations!
              } else if (dndOn || (!ringerOn && !getTimerOn()) || mTimerMuted)
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
                      if (timerTimeout != 0) {
                          if (g_source_remove(timerTimeout)) {
                              g_debug("TimerScenarioModule:Removed timer since new timer stream is created");
                              timerTimeout = 0;
                          }
                      } else {
                          if (gAudioMixer.getActiveStreams().containAnyOf(enotifications, ealerts, eeffects, ecalendar))
                          {
                              //will be removed if alarm module is removed as part of DAP
                              //cancelVibrate();
                          }
                          //will be removed if alarm module is removed as part of DAP
                          //getVibrateDevice()->realVibrate("{\"name\":\"alarm\"}");
                      }
                      shouldFakeVibrate = false; // if vibrated, we no need to
                                                 // fake vibrate anymore...
                  }
                  if (!ringerOn && gState.getHeadsetState() != eHeadsetState_None)
                      shouldFakeVibrate = true;
              }

              if (shouldFakeVibrate && (!gState.getOnThePuck() ||
                                          etimer == sink ||
                                          gState.getOnActiveCall()))
              {    // don't fake vibrate on the puck,
                  gAudioMixer.playSystemSound ("alarm_buzz", eeffects);
                  if (gAudioDevice.needsSCOBeepAlert())
                      needsSCOBeepAlert = true;
              }
              if (needsSCOBeepAlert)
                  gAudioDevice.generateSCOBeepAlert();
          }else if (eControlEvent_LastStreamClosed == event && sink == etimer){
              if (!timerTimeout) {
                  g_debug("TimerScenarioModule:start Timer for 300ms");
                  timerTimeout = g_timeout_add (300, timerTimeoutForVibrate, this);
              } else {
                  if (!gAudioMixer.getActiveStreams().contain(ealarm))
                  {
                      //will be removed if alarm module is removed as part of DAP
                      //cancelVibrate();
                      setMuted(false);
                      timerTimeout = 0;
                  }
              }
          }
     }
     else
     {
        g_debug("TimerScenarioModule: UMI onsink changed");
     }
}


static LSMethod timerMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_SetMuted, _setMuted},
    { cModuleMethod_Status, _status},
    {cModuleMethod_SetTimerOn, _setTimerOn},
    {cModuleMethod_GetTimerOn, _getTimerOn},
    { },
};

TimerScenarioModule::TimerScenarioModule() :
    ScenarioModule(ConstString("/timer")),
    mTimerVolume(cTimer_Default, 50),
    mTimerMuted(false),
    timerTimeout(0)
{
    gAudiodCallbacks.registerModuleCallback(this, etimer, true);
    Scenario *s = new Scenario (cTimer_Default,
                                true,
                                eScenarioPriority_Timer_Default,
                                mTimerVolume);
    addScenario (s);

#if TIMER_SUPPORTED
    registerMe (timerMethods);
#endif

    restorePreferences();

    mMuted = false;
}

int
TimerInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sTimerModule = new TimerScenarioModule();

    return 0;
}

MODULE_START_FUNC (TimerInterfaceInit);
