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

#include "notification.h"
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


static NotificationScenarioModule * sNotificationModule = 0;

static const ConstString    cNotification_Default("notification" SCENARIO_DEFAULT);

NotificationScenarioModule * getNotificationModule()
{
    return sNotificationModule;
}

void
NotificationScenarioModule::programControlVolume ()
{
    programNotificationVolumes(false);
}

void
NotificationScenarioModule::programNotificationVolumes(bool ramp)
{
    int volume = mNotificationVolume.get ();
    bool ringerOn = gState.getRingerOn();
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();
    programVolume(enotifications, (ringerOn && activeStreams.contain(enotifications)) ?
                                                             volume : 0, ramp);
    programVolume(ecalendar, (ringerOn && activeStreams.contain(ecalendar)) ?
                               volume : 0, ramp);
}

void
NotificationScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event)
{
    g_debug("NotificationScenarioModule::onSinkChanged: sink %i-%s %s", sink,
                                                         virtualSinkName(sink),
                                                         controlEventName(event));

        const int cKillFakeVibrateThreshold = 5;

        if (eControlEvent_FirstStreamOpened == event)
        {
            bool shouldFakeVibrate = false;
            bool fakeVibrateIfCantVibrate = true;
            bool needsSCOBeepAlert = false;

            bool ringerOn = gState.getRingerOn();
            if (gState.getOnActiveCall() || !ringerOn  || gState.getRecordStatus())
            {
                programVolume(sink, 0); // mute calendar/notification sound
                shouldFakeVibrate = true; // fake vibrate,
                                          //even if user has disabled vibrations!
            }
            else
            {
                int volume = mNotificationVolume.get();
                if (programVolume(sink, volume) &&
                    volume >= cKillFakeVibrateThreshold)
                    fakeVibrateIfCantVibrate = false; // if alert is audible enough, no need for a fake vibration

                if (gAudioDevice.needsSCOBeepAlert())
                    needsSCOBeepAlert = true;
            }

            // Determine if we should vibrate?
            if (gState.shouldVibrate()) // take into account user settings
            {
                if (gState.getOnActiveCall() || gState.getOnThePuck() || gState.getRecordStatus())
                {
                    shouldFakeVibrate = fakeVibrateIfCantVibrate;
                }
                else
                {
                    getVibrateDevice()->realVibrate("{\"name\":\"notification\"}");
                    shouldFakeVibrate = false; // if vibrated, we no need to fake vibrate anymore...
                }

                if (!ringerOn && gState.getHeadsetState() != eHeadsetState_None)
                    shouldFakeVibrate = true;
            }

            if (shouldFakeVibrate && gState.getOnActiveCall())
            {    // fake vibrate during calls
                gAudioMixer.playSystemSound ("notification_buzz", eeffects);
                if (gAudioDevice.needsSCOBeepAlert())
                    needsSCOBeepAlert = true;
            }
            if (needsSCOBeepAlert)
                gAudioDevice.generateSCOBeepAlert();
        }
}

static LSMethod notificationMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_Status, _status},
    { },
};

NotificationScenarioModule::NotificationScenarioModule() :
    ScenarioModule(ConstString("/notification")),
    mNotificationVolume(cNotification_Default, 50)
{
        gAudiodCallbacks.registerModuleCallback(this, enotifications, true);
        gAudiodCallbacks.registerModuleCallback(this, ecalendar, true);

    Scenario *s = new Scenario (cNotification_Default,
                                true,
                                eScenarioPriority_System_Default,
                                mNotificationVolume);
    addScenario (s);

    registerMe (notificationMethods);

    restorePreferences();

    mMuted = false;
}

int
NotificationInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sNotificationModule = new NotificationScenarioModule();

    return 0;
}

MODULE_START_FUNC (NotificationInterfaceInit);

