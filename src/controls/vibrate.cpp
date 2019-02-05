// Copyright (c) 2012-2019 LG Electronics, Inc.
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

#include "vibrate.h"
#include "messageUtils.h"
#include "main.h"
#include "vibrate.h"
#include "AudioDevice.h"
#include "state.h"
#include "utils.h"
#include "messageUtils.h"

#define DO_DEBUG_VIBRATE 0

#if DO_DEBUG_VIBRATE
    #define DEBUG_VIBRATE g_debug
#else
    #define DEBUG_VIBRATE(...)
#endif

static VibrateDevice * sVibrateDevice = 0;

VibrateDevice * getVibrateDevice()
{
    return sVibrateDevice;
}

void cancelVibrate ()
{
    if (sVibrateDevice)
        sVibrateDevice->cancelVibrate();
}

void
VibrateDevice::cancelVibrate()
{
    if (isVibrating)
    {
        DEBUG_VIBRATE("%s: cancel vibrate", __FUNCTION__);
        CLSError lserror;
        g_message("Invoking LSCallCancel for vibrate");
        LSHandle * sh = GetPalmService();
        bool result = LSCall(sh, "luna://com.webos.vibrate/cancel",
                                     "{}", NULL, NULL, NULL, &lserror);
        if(result)
            isVibrating = false;
        else
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else{
        DEBUG_VIBRATE("%s: not vibrating...", __FUNCTION__);
        g_message("%s: not vibrating...", __FUNCTION__);
        }
    mVibrateToken = 0;
    mFakeBuzzRunning = false;
}

static bool _stubcb(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return true;
}

void
VibrateDevice::startFakeBuzz()
{
    if (!mFakeBuzzRunning)
    {
        mFakeBuzzRunning = true;
        fakeBuzz();
    }
}

static gboolean
_fakeBuzz(gpointer data)
{
    DEBUG_VIBRATE("_fakeBuzz");
    sVibrateDevice->fakeBuzz(*((int*)(data)));

    return FALSE;
}

void
VibrateDevice::fakeBuzz(int runningCount)
{
    if (mFakeBuzzRunning)
    {
        if (++runningCount < 3)
        {
               gAudioMixer.playSystemSound ("ringtone_buzz_short", eeffects);
            g_timeout_add (1000, _fakeBuzz, gpointer(runningCount));
        }
        else
        {
            gAudioMixer.playSystemSound ("ringtone_buzz", eeffects);
            g_timeout_add (2000, _fakeBuzz, 0);
        }
    }
}

void VibrateDevice::setVibrateAmplitude(int amplitude) {
	if (mAmplitude != amplitude)
           mAmplitude = amplitude;
}

void
VibrateDevice::startVibrate(bool fakeVibrateIfCantVibrate)
{
    // check that we do not have a current request
    char message[100];
    snprintf (message, sizeof(message), "{\"period\": 400, \"duration\": 0, \"amplitude\": %d}", mAmplitude);
    g_debug ("message Vibrate service for ringtone = %s",message);
    if (0 == mVibrateToken)
    {
        if (gState.getOnThePuck() || gState.getOnActiveCall())
        {
            if (fakeVibrateIfCantVibrate && !mFakeBuzzRunning)
            {
                DEBUG_VIBRATE("Ringtone::startVibrate: ringtone fake vibrate");
                startFakeBuzz();
            }
        }
        else
        {
            DEBUG_VIBRATE("Ringtonee::startVibrate: ringtone vibrate");
            CLSError lserror;

            // we need to start the ringtone buzz
            LSHandle * sh = GetPalmService();
            if(mAmplitude > 0) {
                if (!LSCall(sh, "luna://com.webos.vibrate/vibrate",message, NULL, NULL, NULL, &lserror)) {
                    lserror.Print(__FUNCTION__, __LINE__);
                }
                else
                    isVibrating = true;

                g_message("RingtoneScenarioModule::startVibrate: '%s' was sent",
                              "luna://com.webos.vibrate/vibrate");
            }
        }
    }
}

void
VibrateDevice::realVibrate(bool alertNotNotification)
{
    char message[100];
    snprintf (message, sizeof(message), "{\"period\": 200, \"duration\": 600, \"amplitude\": %d}", getVibrateDevice()->mAmplitude);
    g_debug ("message to Vibrate service for alert = %s",message);
    CLSError lserror;

    LSHandle * sh = GetPalmService();
    if (getVibrateDevice()->mAmplitude > 0){
        if (!LSCall(sh, "luna://com.webos.vibrate/vibrate",message, NULL, NULL, NULL, &lserror)) {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        g_debug("realVibrate: '%s' was sent","luna://com.webos.vibrate/vibrate");
    }
}

void
VibrateDevice::realVibrate(const char * vibrateSpec)
{
    CLSError lserror;

    LSHandle * sh = GetPalmService();

    bool result = LSCall(sh, "luna://com.webos.vibrate/vibrateNamedEffect",
                                     vibrateSpec, NULL, NULL, NULL, &lserror);
    g_debug("realVibrate: '%s' was sent '%s'",
                              "luna://com.webos.vibrate/vibrateNamedEffect", vibrateSpec);

    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);
    else
	isVibrating = true;
}


/*
 * This service is now obsolete, because made redundant by
 * the generic state/setPreference & state/getPreference
 * We keep it for now, until the pref app is updated to use the new API
 */

#if defined(AUDIOD_TEST_API)
static bool
_setProperty(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_2(OPTIONAL(VibrateWhenRingerOn,
                                                         boolean),
                                   OPTIONAL(VibrateWhenRingerOff, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    bool value;

    if (msg.get(cPref_VibrateWhenRingerOn, value))
        gState.setPreference(cPref_VibrateWhenRingerOn, value);

    if (msg.get(cPref_VibrateWhenRingerOff, value))
        gState.setPreference(cPref_VibrateWhenRingerOff, value);

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, STANDARD_JSON_SUCCESS, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static bool
_getProperty(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_0);
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    pbnjson::JValue reply = createJsonReply(true);

    bool    pref;
    if (gState.getPreference(cPref_VibrateWhenRingerOn, pref))
        reply.put(cPref_VibrateWhenRingerOn, pref);
    if (gState.getPreference(cPref_VibrateWhenRingerOff, pref))
        reply.put(cPref_VibrateWhenRingerOff, pref);

    std::string replyString = jsonToString(reply);

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, replyString.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static LSMethod vibrateMethods[] = {
    { "set", _setProperty},
    { "get", _getProperty},
    { },
};

#endif
int
VibrateInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    /* luna service interface */
    CLSError lserror;
#if VIBRATE_SUPPORTED
#if defined (AUDIOD_TEST_API)
    bool result;
    result = ServiceRegisterCategory ("/vibrate", vibrateMethods, NULL, NULL);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/vibrate");
        return (-1);
    }
#endif
#endif

    sVibrateDevice = new VibrateDevice();

    return 0;
}

CONTROL_START_FUNC (VibrateInterfaceInit);
