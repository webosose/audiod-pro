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

#include "alert.h"
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

static AlertScenarioModule * sAlertModule = 0;

static const ConstString    cAlert_Default("alert" SCENARIO_DEFAULT);

AlertScenarioModule * getAlertModule()
{
    return sAlertModule;
}

void
AlertScenarioModule::programControlVolume ()
{
    programAlertVolumes(false);
}

bool
AlertScenarioModule::setMuted (bool mute)
{
    if(mute &&
        !mMuted &&
        !gAudioMixer.getActiveStreams().contain(ealerts))
        cancelVibrate();
     else
     {
         if(mute)
         {
             mAlertMuted = true;

             if(mMuted)
                 programMuted();
             else
                 cancelVibrate();
         }
         else
             mAlertMuted = false;
         return ScenarioModule::setMuted(mute);
     }

    return true;
}
void
AlertScenarioModule::programMuted ()
{
    if(mMuted)
        cancelVibrate();
    programAlertVolumes(false);
}
void
AlertScenarioModule::programAlertVolumes(bool ramp)
{
    int volume = mAlertVolume.get();
    bool ringerOn = gState.getRingerOn();
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();
    programVolume(ealerts, ((ringerOn  && !mAlertMuted) && activeStreams.contain(ealerts)) ?
                                                                 volume : 0, ramp);
    programVolume(eeffects, ((ringerOn  && !mAlertMuted) && activeStreams.contain(eeffects)) ?
                                                                 volume : 0, ramp);
}

int
AlertScenarioModule::getAdjustedAlertVolume(bool alertStarting) const
{
    int volume = mAlertVolume.get();
    if (VERIFY(sCurrentModule))
        volume = sCurrentModule->adjustAlertVolume(volume, alertStarting);
    return volume;
}
void
AlertScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
    g_debug("AlertScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
                                                     virtualSinkName(sink),
                                                     controlEventName(event),p_eSinkType);
      if(p_eSinkType==ePulseAudio)
      {
        g_debug("AlertScenarioModule: pulse audio onsink changed");                                                

        if (eControlEvent_FirstStreamOpened == event)
        {
            bool shouldFakeVibrate = false;
            bool fakeVibrateIfCantVibrate = true;
            bool needsSCOBeepAlert = false;

            bool ringerOn = gState.getRingerOn();
            //int  volume = (ringerOn || !mAlertMuted)? getAdjustedAlertVolume(true) : 0;
            int volume = mAlertVolume.get();
            if (gState.getOnActiveCall() && !ringerOn)
            {
                programVolume(sink, 0); // mute alert/notification sound
                shouldFakeVibrate = true; // fake vibrate,
                                              //even if user has disabled vibrations!
            } else if (!ringerOn || mAlertMuted )
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
                    //getVibrateDevice()->startVibrate(fakeVibrateIfCantVibrate);
                    getVibrateDevice()->realVibrate("{\"name\":\"system_notification\"}");
                    shouldFakeVibrate = false; // if vibrated, we no need to
                                               // fake vibrate anymore...
                }
                if (!ringerOn && gState.getHeadsetState() != eHeadsetState_None)
                    shouldFakeVibrate = true;
            }

            if (shouldFakeVibrate && (!gState.getOnThePuck() ||
                                        gState.getOnActiveCall()))
            {    // don't fake vibrate on the puck,
                gAudioMixer.playSystemSound ("notification_buzz", eeffects);
                if (gAudioDevice.needsSCOBeepAlert())
                    needsSCOBeepAlert = true;
            }
            if (needsSCOBeepAlert)
                gAudioDevice.generateSCOBeepAlert();
        }else if (eControlEvent_LastStreamClosed == event)
        {
            if(!gAudioMixer.getActiveStreams().contain(ealarm)){
                cancelVibrate();
                setMuted(false);
            }
        }
     }
    else
    {
       g_debug("AlertScenarioModule: UMI onsink changed");
    }
}

static LSMethod alertMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_SetMuted, _setMuted},
    { cModuleMethod_Status, _status},
    { },
};

AlertScenarioModule::AlertScenarioModule() :
    ScenarioModule(ConstString("/alert")),
    mAlertVolume(cAlert_Default, 80),
    mAlertMuted(false)
{
    gAudiodCallbacks.registerModuleCallback(this, ealerts, true);
    gAudiodCallbacks.registerModuleCallback(this, eeffects, true);
    Scenario *s = new Scenario (cAlert_Default,
                                true,
                                eScenarioPriority_Alert_Default,
                                mAlertVolume);
    addScenario (s);

    registerMe (alertMethods);

    restorePreferences();

    mMuted = false;
}

int
AlertInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sAlertModule = new AlertScenarioModule();

    return 0;
}

MODULE_START_FUNC (AlertInterfaceInit);
