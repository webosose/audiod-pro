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


#include <dlfcn.h>
#include "AudioDevice.h"

#include "messageUtils.h"
#include "main.h"
#include "state.h"

AudioDevice &gAudioDevice = AudioDevice::get();

bool AudioDevice::setRouting(const ConstString & scenario,
                                   ERouting routing,
                                   int micGain)
{
    mCurrentScenario = scenario;
    mRouting = routing;
    mMicGain = micGain;
    return true;
}

bool AudioDevice::setMicGain(const ConstString & scenario, int gain)
{
    if (!VERIFY(scenario == mCurrentScenario))
        return false;

    mMicGain = gain;
    return true;
}

int AudioDevice::getDefaultMicGain(const ConstString & scenario)
{
    return 50;
}

bool AudioDevice::museSet (bool enable)
{
    std::string param = string_printf("{\"enable\":%s}", (enable) ? "true" : "false");

    CLSError lserror;
    LSHandle * sh = GetPalmService();

    if (!LSCall(sh, "luna://com.palm.telephony/museSet", param.c_str(),
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

gboolean AudioDevice::delayCall_timercallback(void* data) {
    Callback* callback = (Callback*)data;
    callback->callback(callback);
    return FALSE;

}

void AudioDevice::delayCall(Callback* callback, int millisecond) {
    if (!callback) return;
    if (millisecond<=0) {
        callback->callback(callback);
    } else {
        g_timeout_add(millisecond, &delayCall_timercallback, callback);
    }
}

void AudioDevice::playThroughPulse(const char* soundname,
                                   int count,
                                   int interval,
                                   Callback* callback) {
    if (soundname==NULL || count==0) return;
    PulsePlay*  p = new PulsePlay();
    p->soundname = soundname;
    p->interval = interval;
    p->count = count;
    p->callback = callback;
    playThroughPulseCallback(p);
}

gboolean
AudioDevice::playThroughPulseCallback(gpointer parameter)
{
    // use char * to be able to use pointer arithmetics to count
    PulsePlay*  p = (PulsePlay*) parameter;
    if (p->count==0) {
        if (p->callback) delayCall(p->callback, 0);
        delete p;
    } else {
        gAudioMixer.playSystemSound (p->soundname, eeffects);
        --p->count;
        g_timeout_add (p->interval, AudioDevice::playThroughPulseCallback, p);
    }

    return FALSE;
}

void AudioDevice::playThroughPulseIncomingTone(const char* soundname,
                                                int count,
                                                int interval,
                                                Callback* callback) {
    if (soundname==NULL || count==0) return;
    PulsePlay*  p = new PulsePlay();
    p->soundname = soundname;
    p->interval = interval;
    p->count = count;
    p->callback = callback;
    setIncomingCallRinging(true);
    playThroughPulseCallbackIncomingTone(p);
}

gboolean
AudioDevice::playThroughPulseCallbackIncomingTone(gpointer parameter)
{
    // use char * to be able to use pointer arithmetics to count
    PulsePlay*  p = (PulsePlay*) parameter;
    g_debug("gState.getCallMode() %d, p->count %d, mIncomingCallRinging %d",\
          gState.getCallMode(), p->count, gAudioDevice.getIncomingCallRinging());
    if (p->count==0 || gAudioDevice.getIncomingCallRinging() == false) {
        if (p->callback) delayCall(p->callback, 0);
        delete p;
    } else {
        gAudioMixer.playSystemSound (p->soundname, eeffects);
        --p->count;
        g_timeout_add (p->interval, AudioDevice::playThroughPulseCallbackIncomingTone, p);
    }

    return FALSE;
}

void AudioDevice::setIncomingCallRinging(bool ringing)
{
    mIncomingCallRinging = ringing;
}

bool AudioDevice::getIncomingCallRinging()
{
    return mIncomingCallRinging;
}

