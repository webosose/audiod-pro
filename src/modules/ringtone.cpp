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

#include "ringtone.h"
#include "module.h"
#include "state.h"
#include "phone.h"
#include "media.h"
#include "system.h"
#include "volume.h"
#include "utils.h"
#include "messageUtils.h"
#include "AudioDevice.h"
#include "AudioMixer.h"
#include "update.h"
#include "log.h"
#include "main.h"
#include "AudiodCallbacks.h"
#include "vibrate.h"
#include "IPC_SharedAudiodProperties.h"

#define DO_DEBUG_VIBRATE 0

#if DO_DEBUG_VIBRATE
    #define DEBUG_VIBRATE g_debug
#else
    #define DEBUG_VIBRATE(...)
#endif

static RingtoneScenarioModule * sRingtoneModule = 0;

static const ConstString    cRingtone_Default("ringtone" SCENARIO_DEFAULT);

RingtoneScenarioModule * getRingtoneModule()
{
    return sRingtoneModule;
}

bool
RingtoneScenarioModule::muteIfRinging()
{
    if (gAudioMixer.getActiveStreams().contain(eringtones))
    {
        setMuted(true);
        return true;
    }
    return false;
}

void
RingtoneScenarioModule::silenceRingtone()
{
    programVolume(eringtones, 0, false);
    cancelVibrate();
}

bool
RingtoneScenarioModule::setMuted (bool mute)
{
    if ( mute &&
        !mMuted &&
        !gAudioMixer.getActiveStreams().contain(eringtones))
    {
        cancelVibrate();
    }
    else
    {
        if (mute)
        {
            mRingtoneMuted = true;// we could be 'mMuted'but
                                                //playing a ringtone
            if (mMuted)
                programMuted();        // module's muting is going
                                    //to be optimized out! we don't want that...
            else
                cancelVibrate();    // we need to make sure we
                                    //are canceling vibrations
        }
        else
            mRingtoneMuted = false; //for next call

        return ScenarioModule::setMuted(mute);
    }
    return true;
}

static gboolean
_SCOBeepAlarm(gpointer data)
{
    if (sRingtoneModule)
        sRingtoneModule->SCOBeepAlarm(*((int*)(data)));

    return FALSE;
}

void
RingtoneScenarioModule::SCOBeepAlarm(int runningCount)
{
    if (mSCOBeepAlarmRunning)
    {
        if (++runningCount < 3)
            g_timeout_add (1000, _SCOBeepAlarm, gpointer(runningCount));
        else
            g_timeout_add (2000, _SCOBeepAlarm, 0);
        gAudioDevice.generateSCOBeepAlert();
    }
}

void
RingtoneScenarioModule::programControlVolume ()
{
    if (!mMuted)
        programRingToneVolumes(false);
}

void
RingtoneScenarioModule::programMuted ()
{
    if (mMuted)
        cancelVibrate();
    programRingToneVolumes(false);

}

void
RingtoneScenarioModule::programRingToneVolumes(bool ramp)
{
    int volume = mRingtoneVolume.get();
    int mediaVolume = 0;
    bool dndOn = false;

    gState.getPreference(cPref_DndOn, dndOn);
    getMediaModule()->getScenarioVolumeOrMicGain(0, mediaVolume, true);

    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();

    programVolume(eringtones, (!mRingtoneMuted &&
                               activeStreams.contain(eringtones)) ?
                               volume : 0, ramp);

    if (activeStreams.containAnyOf(etts, etts1) && !dndOn) {
        if (!gState.getRingerOn())
        {
            programVolume(etts, mediaVolume, ramp);
           programVolume(etts1, mediaVolume, ramp);
        }
        else
        {
            programVolume(etts, volume, ramp);
            programVolume(etts1, volume, ramp);
        }
    }
    else
    {
        programVolume(etts, 0);
        programVolume(etts1, 0);
    }
    programVolume(evoicerecognition, (gState.getRingerOn() &&
                                      activeStreams.contain(evoicerecognition)) ?
                                      volume : 0, ramp);
}

int
RingtoneScenarioModule::getAdjustedRingtoneVolume(bool alertStarting) const
{
    int volume = mRingtoneVolume.get();
    if (VERIFY(sCurrentModule))
        volume = sCurrentModule->adjustAlertVolume(volume, alertStarting);
    return volume;
}

bool RingtoneScenarioModule::getRingtoneVibration()
{
    bool ringtoneWithVibration = false;
    gState.getPreference(cPref_RingtoneWithVibration, ringtoneWithVibration);
    return ringtoneWithVibration;
}

void RingtoneScenarioModule::setRingtoneVibration(bool enable)
{
    gAudiodProperties->mRingtoneWithVibration.set(enable);
    gState.setPreference(cPref_RingtoneWithVibration, enable);
}

/* if the shouldVibrate() return  false means, it is either silent or sound profile, then check for
ringer state(sound profile), if ringtone with vibration is enabled then vibrate */
bool RingtoneScenarioModule::checkRingtoneVibration()
{
    bool ringtoneShouldVibrate = false;
    ringtoneShouldVibrate = gState.shouldVibrate();
    if (!ringtoneShouldVibrate) {
        if (getRingtoneVibration() && gState.getRingerOn())
            ringtoneShouldVibrate = true;
    }

    return ringtoneShouldVibrate;
}

static bool _getRingtoneWithVibration(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, SCHEMA_0);
    pbnjson::JValue reply = pbnjson::Object();
    const std::string name = "ringtonewithvibration";

    if (!msg.parse(__FUNCTION__, lshandle)) {
        g_message("_getRingtoneWithVibration : parse message error");
        return true;
    }

    bool enable = getRingtoneModule()->getRingtoneVibration();
    reply.put(name.c_str(), enable);

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, jsonToString(reply).c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

   return true;
}

static bool _setRingtoneWithVibration(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    bool ringtonewithvibration;
    const char *reply = STANDARD_JSON_SUCCESS;
    ScenarioModule *module = (ScenarioModule*)ctx;
    CLSError lserror;
    bool subscribed = false;
    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(ringtonewithvibration, boolean)));

    if (!msg.parse(__FUNCTION__, lshandle)) {
        reply = STANDARD_JSON_ERROR(3, "missing parameter ringtonewithvibration for the ringtone with vibration when profile is sound");
        goto error;
    }

    if (!msg.get("ringtonewithvibration", ringtonewithvibration)) {
        reply = MISSING_PARAMETER_ERROR(ringtonewithvibration, bool);
        goto error;
    } else {
        if (getRingtoneModule()->getRingtoneVibration() != ringtonewithvibration)
            getRingtoneModule()->setRingtoneVibration(ringtonewithvibration);
    }

    if (LSMessageIsSubscription(message)) {
        if (!LSSubscriptionProcess (lshandle, message, &subscribed, &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    CHECK(module->sendChangedUpdate(UPDATE_RINGTONE_WITH_VIBRATION));

error:
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

void
RingtoneScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
    g_debug("RingtoneScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
                                                    virtualSinkName(sink),
                                                    controlEventName(event),(int)p_eSinkType);
    if(p_eSinkType==ePulseAudio)
    {
      g_debug("RingtoneScenarioModule: Pulse Audio onsink changed");
      const int cKillFakeVibrateThreshold = 5;
      int mediaVolume = 0;
      bool dndOn = false;
      ScenarioModule *media = getMediaModule();
      VirtualSinkSet      activeStreams = gAudioMixer.getActiveStreams();
      if (eringtones == sink || etts == sink || etts1 == sink)
      {
          if (eControlEvent_FirstStreamOpened == event)
          {
              bool    needsSCOBeepAlarm = false;
              if (mMuted){
                  g_message("Ringtone::onSinkChanged: Ringtone module muted when opening %s!!!!",\
                                                        virtualSinkName(sink));
              }
              // We unmute a newly opened ringtone
              //That's the only way to set these flags,
              // because until the user mutes them, they should always go through!
              //not required  as we are doing it per call now and not per sink open-close
              /*if (eringtones == sink)
                  mRingtoneMuted = false; */
              if (etts == sink || etts1 == sink) {
                  getMediaModule()->getScenarioVolumeOrMicGain(0, mediaVolume, true);
                  gState.getPreference(cPref_DndOn, dndOn);
              }

              bool    fakeVibrateIfCantVibrate = true;
              if (VERIFY(mCurrentScenario) &&
                   gState.getRingerOn () &&
                    !mRingtoneMuted )//added to continue in muted mode if ringtone is muted using vol keys
              {
                  int volume = !gState.getOnActiveCall()?getAdjustedRingtoneVolume(true):mRingtoneVolume.get();
                  if((etts == sink || etts1 == sink) && gState.getRingerOn ()){
                      programVolume(sink, mediaVolume);
                  }
                  else if ( programVolume(sink, volume) && volume >=
                      cKillFakeVibrateThreshold){
                      fakeVibrateIfCantVibrate = false;// if alert is audible
                                                       // enough, no need for
                                                       // a fake vibration
                  }

                  if (gAudioDevice.needsSCOBeepAlert()){
                      needsSCOBeepAlarm = true;
                  }

              }else{
                  if(!gState.getRingerOn ())
                  {
                      if (activeStreams.containAnyOf(etts, etts1) && !dndOn)
                          programVolume(sink, mediaVolume);
                      else
                          programVolume(sink, 0);

                  }else{
                      programVolume(sink, 0);
                  }
              }

              if (!gState.getOnActiveCall() && checkRingtoneVibration() && eringtones == sink)
              {    // repeated vibration
                  //getVibrateDevice()->startVibrate(fakeVibrateIfCantVibrate);
                  getVibrateDevice()->realVibrate("{\"name\":\"ringtone\"}");
                  if (gState.getHeadsetState() != eHeadsetState_None &&
                                                  fakeVibrateIfCantVibrate)
                  {
                      getVibrateDevice()->startFakeBuzz();
                      fakeVibrateIfCantVibrate = false;
                  }
                  if (gAudioDevice.needsSCOBeepAlert())
                      needsSCOBeepAlarm = true;
              }
              if (needsSCOBeepAlarm && !mSCOBeepAlarmRunning)
              {
                  mSCOBeepAlarmRunning = true;
                  SCOBeepAlarm();    // we can't play the ringtone
                                  //alarm in the user's ear,
                                  // let's beep! (Castle bluetooth...)
              }
          }
          else if (eControlEvent_LastStreamClosed == event)
          {
              if (!activeStreams.contain(eringtones))
              {    // last alarm/ringtone: stop vibrating & unmute for the next time
                  cancelVibrate();
                  setMuted(false); //not required - in case the ringtone is muted
              }
              programVolume(sink, 0);
          }
      }
    else if (evoicerecognition == sink) {
           bool muteVoicerecognition = false;
           muteVoicerecognition = (event == eControlEvent_FirstStreamOpened) ? false : true;

           int volume = 0;
           if(muteVoicerecognition)
               programVolume(evoicerecognition, volume);
           else {
               getScenarioVolume(cRingtone_Default, volume);
               programVolume(evoicerecognition, (gState.getRingerOn() \
                   && activeStreams.contain(evoicerecognition)) ? volume : 0);
           }
      }
  }
  else
  {
    g_debug("RingtoneScenarioModule: UMI onsink changed");
  }
}

static LSMethod ringtoneMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_SetMuted, _setMuted},
    { cModuleMethod_Status, _status},
    { cModuleMethod_LockVolumeKeys, _lockVolumeKeys},
    { },
};

static LSMethod ringtoneAdditionalMethods[] = {
    { cModuleMethod_SetRingtoneWithVibration, _setRingtoneWithVibration},
    { cModuleMethod_GetRingtoneWithVibration, _getRingtoneWithVibration},
    { },
};

RingtoneScenarioModule::RingtoneScenarioModule() :
    ScenarioModule(ConstString("/ringtone")),
    mSCOBeepAlarmRunning(false),
    mRingtoneVolume(cRingtone_Default, 50),
    mRingtoneMuted(false),
    mIsRingtoneCombined(false)
{
    gAudiodCallbacks.registerModuleCallback(this, eringtones, true);
    gAudiodCallbacks.registerModuleCallback(this, etts, true);
    gAudiodCallbacks.registerModuleCallback(this, etts1, true);
    gAudiodCallbacks.registerModuleCallback(this, evoicerecognition, true);

    Scenario *s = new Scenario(cRingtone_Default,
                                true,
                                eScenarioPriority_Ringtone_Default,
                                mRingtoneVolume);
    if(NULL == s) {
         g_error("Unable to allocate memory for RingtoneScenarioModule \n") ;
         return ;
    }

    addScenario(s);

    registerMe(ringtoneMethods);
    registerMe(ringtoneAdditionalMethods);

    restorePreferences();
}

RingtoneScenarioModule::RingtoneScenarioModule(int default_volume) :
    ScenarioModule(ConstString("/ringtone")),
    mSCOBeepAlarmRunning(false),
    mRingtoneVolume(cRingtone_Default, default_volume),
    mRingtoneMuted(true),
    mIsRingtoneCombined(false)
{
    g_debug("%s: RingtoneScenarioModule with arg '%d' volume", __FUNCTION__, default_volume);
}

void RingtoneScenarioModule::RingtoneInterfaceExit() {
    delete sRingtoneModule;
    sRingtoneModule = NULL;
}


int
RingtoneInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sRingtoneModule = new RingtoneScenarioModule();
    g_debug("RingtoneInterfaceInit- %s",__FUNCTION__);

    if(NULL == sRingtoneModule) {
        g_error("Error : Cannot allocate memory.- %s",__FUNCTION__);
        return (-1);
    }

   return 0;
}

MODULE_START_FUNC (RingtoneInterfaceInit);
