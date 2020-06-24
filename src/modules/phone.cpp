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
#include <unistd.h>

#include <lunaservice.h>

#include "phone.h"
#include "module.h"
#include "state.h"
#include "volume.h"
#include "AudioDevice.h"
#include "utils.h"
#include "log.h"
#include "update.h"
#include "AudiodCallbacks.h"

static PhoneScenarioModule * sPhoneModule = 0;

PhoneScenarioModule * getPhoneModule()
{
    return sPhoneModule;
}

void
PhoneScenarioModule::programHardwareVolume ()
{
    if (isCurrentModule() && VERIFY(mCurrentScenario))
    {
        //changes from getVolumeTics() to getVolume()
        gAudioMixer.setPhoneVolume(mCurrentScenario->mName,
                                                   mCurrentScenario->getVolume());
        gAudioMixer.programVolume (eeffects, MAX(mCurrentScenario->getVolume(),
                                                  25));
        // put something here to set voip volume if applicable on sink voip sink
        gAudioMixer.programVolume(evoip, mCurrentScenario->getVolume(), false);
    }
}

void
PhoneScenarioModule::programControlVolume()
{
    programCallertoneVolume(false);
}

void
PhoneScenarioModule::programCallertoneVolume(bool ramp)
{
    programVolume(ecallertone, mCurrentScenario->getVolume(), ramp);
}

void
PhoneScenarioModule::onActivating ()
{
    //gAudioDevice.phoneEvent(ePhoneEvent_CallStarted);
}

void
PhoneScenarioModule::onDeactivated ()
{
    //gAudioDevice.phoneEvent(ePhoneEvent_CallEnded);
    /* Explicitly unmuting source,
     need to fix source ouput muting in pulse later */
    gAudioMixer.programMute(evoipsource, false);
    g_message("Muting Voice Call Recording Source");
    gAudioMixer.programMute(evoicecallsource, true);
}

void
PhoneScenarioModule::programMuted ()
{
    ECallMode mode = gState.getCallMode();

    if (isCurrentModule() && VERIFY(mCurrentScenario))
    {
        if (mode == eCallMode_Carrier) {
            g_debug("%s: %d: Muting voip, setting carrier mute to %d!",
                                              __FUNCTION__, __LINE__, mMuted);
            gAudioMixer.setPhoneMuted(mCurrentScenario->mName, mMuted);
            gAudioMixer.programMute(evoipsource, true);
        }
        else if (mode == eCallMode_Voip) {
            g_debug("%s: %d: Muting carrier, setting voip mute to %d!",
                                               __FUNCTION__, __LINE__, mMuted);
            gAudioMixer.setPhoneMuted(mCurrentScenario->mName, true);
            gAudioMixer.programMute(evoipsource, mMuted);
        } else {
            g_debug("%s: %d: setting voip and carrier mute to %d!",
                                               __FUNCTION__, __LINE__, mMuted);
            gAudioMixer.setPhoneMuted(mCurrentScenario->mName, mMuted);
            gAudioMixer.programMute(evoipsource, mMuted);
        }
    }
}

void
PhoneScenarioModule::programMuse (bool enable)
{
    g_debug("setting muse to %d, to disable or enable phone audio eq", enable);
    gAudioDevice.museSet(enable);
}

void
PhoneScenarioModule::programHac (bool enable)
{
    g_debug("setting hac to %d", enable);
    gState.hacSet(enable);
    CHECK(sendChangedUpdate (UPDATE_CHANGED_HAC));
}

int
PhoneScenarioModule::adjustAlertVolume(int volume, bool alertStarting)
{    // ignore alert volume and derive from phone's volume.
     // Map 0-100% phone volume to 20-50% alert volume
    const int minVolume = 20;
    const int maxVolume = 50;
    return (maxVolume - minVolume) * mCurrentScenario->getVolume() / 100 + minVolume;
}

void
PhoneScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
    g_debug("Phone::onSinkChanged: sink %i-%s %s %d", sink,
                               virtualSinkName(sink), controlEventName(event),(int)p_eSinkType);
    if(p_eSinkType==ePulseAudio)
      {
        g_debug("PhoneScenarioModule: pulse audio onsink changed");                           

        if (evoip == sink)
        {
            if (eControlEvent_FirstStreamOpened == event)
            {
                gAudioMixer.programVolume(evoip, mCurrentScenario->getVolume(), false);
            }
            else if (eControlEvent_LastStreamClosed == event)
            {
                gAudioMixer.programVolume(evoip, 0, false);
            }
        }
      }
      else
      {
          g_debug("PhoneScenarioModule: UMI onsink changed");
      }
}


static LSMethod phoneMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_EnableScenario, _enableScenario},
    { cModuleMethod_DisableScenario, _disableScenario},
    { cModuleMethod_SetCurrentScenario, _setCurrentScenario},
    { cModuleMethod_GetCurrentScenario, _getCurrentScenario},
    { cModuleMethod_ListScenarios, _listScenarios},
    { cModuleMethod_Status, _status},
    { cModuleMethod_SetMuted, _setMuted},
    { cModuleMethod_BroadcastEvent, _broadcastEvent},
    { cModuleMethod_LockVolumeKeys, _lockVolumeKeys},
#if defined(AUDIOD_PALM_LEGACY)
    { cModuleMethod_MuseSet, _museSet},
    { cModuleMethod_HacSet, _hacSet},
    { cModuleMethod_HacGet, _hacGet},
    { cModuleMethod_CallStatusUpdate, _callStatusUpdate},
    { cModuleMethod_BluetoothAudioPropertiesSet, _bluetoothAudioPropertiesSet},
#endif
    { },
};

PhoneScenarioModule::PhoneScenarioModule() :
    ScenarioModule(ConstString("/phone")),
    mFrontSpeakerVolume(cPhone_FrontSpeaker, 65),
    mBackSpeakerVolume(cPhone_BackSpeaker, 65),
    mHeadsetVolume(cPhone_Headset, 50),
    mBluetoothVolume(cPhone_BluetoothSCO, 65),
    mTTYFullVolume(cPhone_TTY_Full, 50),
    mTTYHCOVolume(cPhone_TTY_HCO, 50),
    mTTYVCOVolume(cPhone_TTY_VCO, 50)
{
    gAudiodCallbacks.registerModuleCallback(this, evoip);
    Scenario *s;


    s = new Scenario (cPhone_BackSpeaker,
                      true,
                      eScenarioPriority_Phone_BackSpeaker,
                      mBackSpeakerVolume);
    s->mHardwired = false;
    //                 class              destination    switch enabled
    s->configureRoute (ealerts,           eMainSink,     true,      true);
    s->configureRoute (ealerts,           eMainSink,     false,     true);
    s->configureRoute (eeffects,      eMainSink,  true,   true);
    s->configureRoute (eeffects,      eMainSink,  false,  true);
    s->configureRoute (enotifications,    eMainSink,     true,      true);
    s->configureRoute (enotifications,    eMainSink,     false,     true);
    s->configureRoute (ecallertone,       eMainSink,     true,      true);
    s->configureRoute (ecallertone,       eMainSink,     false,     true);
    s->configureRoute (ecalendar,         eMainSink,     true,      true);
    s->configureRoute (ecalendar,         eMainSink,     false,     true);
    s->configureRoute (eDTMF,             eMainSink,     true,      true);
    s->configureRoute (etts,              eMainSink,     true,      true);
    s->configureRoute (etts,              eMainSink,     false,     true);
    s->configureRoute (etts1,              eMainSink,     true,      true);
    s->configureRoute (etts1,              eMainSink,     false,     true);
    s->configureRoute (efeedback,          eMainSink,     true,     true);
    s->configureRoute (ealarm,             eMainSink,     true,     true);
    s->configureRoute (ealarm,             eMainSink,     false,    true);
    s->configureRoute (etimer,             eMainSink,     true,     true);
    s->configureRoute (etimer,             eMainSink,     false,    true);
    s->configureRoute (evoicedial,      eMainSink,  true,   true);
    s->configureRoute (evoicedial,      eMainSink,  false,  true);
    s->configureRoute (evoip,           eMainSink,  false,  true);
    s->configureRoute (evoip,           eMainSink,  true,   true);
    s->configureRoute (erecord,            eMainSource,   false,  true);
    s->configureRoute (erecord,            eMainSource,   true,  true);
    s->configureRoute (evoicedialsource,   eMainSource,   false,  true);
    s->configureRoute (evoicedialsource,   eMainSource,   true,  true);
    s->configureRoute (evoipsource,        eMainSource,   false,  true);
    s->configureRoute (evoipsource,        eMainSource,   true,  true);
    addScenario (s);

#if PHONE_SUPPORTED
    registerMe (phoneMethods);
#endif

    restorePreferences();
}


int
PhoneInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sPhoneModule = new PhoneScenarioModule();
    g_debug("PhoneInterfaceInit- %s",__FUNCTION__);
    if(NULL == sPhoneModule) {
        g_debug("Error : Cannot allocate memory.- %s",__FUNCTION__);
        return (-1);
    }

   return 0;
}

MODULE_START_FUNC (PhoneInterfaceInit);
