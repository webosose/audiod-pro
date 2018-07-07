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

static SystemScenarioModule * sSystemModule = 0;

static const ConstString    cSystem_Default("system" SCENARIO_DEFAULT);

SystemScenarioModule * getSystemModule()
{
    return sSystemModule;
}

void
SystemScenarioModule::programControlVolume ()
{
    programSystemVolumes(false);
}

void
SystemScenarioModule::programMuted ()
{
    if (mMuted)
        cancelVibrate();
    programSystemVolumes(false);
}

void
SystemScenarioModule::programSystemVolumes(bool ramp)
{
    int volume = mSystemVolume.get();
    g_debug("SystemScenarioModule::programSystemVolumes : ramp = %d", ramp);
    programVolume(efeedback, gState.getOnActiveCall()? 0:volume, ramp);
    programVolume(eDTMF, gState.getRingerOn()? volume:0, ramp);
}

int
SystemScenarioModule::getAdjustedSystemVolume(bool alertStarting) const
{
    int volume = mSystemVolume.get();
    if (VERIFY(sCurrentModule))
        volume = sCurrentModule->adjustAlertVolume(volume, alertStarting);
    return volume;
}

void
SystemScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event,ESinkType p_eSinkType)
{
    g_debug("SystemScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
                                                     virtualSinkName(sink),
                                                     controlEventName(event),(int)p_eSinkType);
    if(p_eSinkType==ePulseAudio)                                                     
    {
      const int cKillFakeVibrateThreshold = 5;

      if (eControlEvent_FirstStreamOpened == event)
      {
          bool shouldFakeVibrate = false;
          bool fakeVibrateIfCantVibrate = true;
          bool needsSCOBeepAlert = false;

          bool ringerOn = gState.getRingerOn();
          if (gState.getOnActiveCall() && !ringerOn)
          {
              programVolume(sink, 0); // mute alert/notification sound
              shouldFakeVibrate = true; // fake vibrate,
                                            //even if user has disabled vibrations!
          }
          else
          {
              int     volume = (ringerOn || !mMuted)? getAdjustedSystemVolume(true) : 0;
              if (programVolume(sink, volume) &&
                  volume >= cKillFakeVibrateThreshold)
                  fakeVibrateIfCantVibrate = false; // if alert is audible
                                                    //enough, no need for
                                                    //a fake vibration
              if (gAudioDevice.needsSCOBeepAlert())
                  needsSCOBeepAlert = true;
          }

              // Determine if we should vibrate?
          if (gState.shouldVibrate()) // take into account user settings
          {
              if (gState.getOnActiveCall() || gState.getOnThePuck())
              {
                  shouldFakeVibrate = fakeVibrateIfCantVibrate;
              }
              else
              {
                  if (sink == ealarm)
                      getVibrateDevice()->startVibrate(fakeVibrateIfCantVibrate);
                  else
                      getVibrateDevice()->realVibrate(enotifications != sink);
                  shouldFakeVibrate = false; // if vibrated, we no need to
                                             // fake vibrate anymore...
              }
              if (!ringerOn && gState.getHeadsetState() != eHeadsetState_None)
                  shouldFakeVibrate = true;
          }

          if (shouldFakeVibrate && (!gState.getOnThePuck() || gState.getOnActiveCall()))
          {    // don't fake vibrate on the puck,
              gAudioMixer.playSystemSound ("notification_buzz", eeffects);
              if (gAudioDevice.needsSCOBeepAlert())
                  needsSCOBeepAlert = true;
          }
          if (needsSCOBeepAlert)
              gAudioDevice.generateSCOBeepAlert();
      }else if (eControlEvent_LastStreamClosed == event && sink == ealarm)
      {
          cancelVibrate();
      }
  }
  else
  {
    g_debug("SystemScenarioModule: UMI onsink changed");
  }
}

unsigned long
SystemScenarioModule::getNewSessionId()
{
    return ++mSessionId;
}

static LSMethod systemMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
#if defined(AUDIOD_PALM_LEGACY)
    { cModuleMethod_SetMuted, _setMuted},
#endif
    { cModuleMethod_Status, _status},
    { },
};

SystemScenarioModule::SystemScenarioModule() :
    ScenarioModule(ConstString("/system")), mSessionId(0),
    mSystemVolume(cSystem_Default, 80)
{
    Scenario *s = new Scenario (cSystem_Default,
                                true,
                                eScenarioPriority_System_Default,
                                mSystemVolume);
    addScenario (s);

    registerMe (systemMethods);

    restorePreferences();

    mMuted = false;
}

int
SystemInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sSystemModule = new SystemScenarioModule();
    g_debug("SystemInterfaceInit- %s",__FUNCTION__);
    if(NULL == sSystemModule) {
        g_debug("Error : Cannot allocate memory.- %s",__FUNCTION__);
        return (-1);
    }

    return 0;
}

MODULE_START_FUNC (SystemInterfaceInit);
