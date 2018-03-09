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


#include "AudioDevice.h"

/*
 * This is default implementation of a null-device that will be used during bring-up of a device
 * and/or when a new build target is created. This will help speed things up by getting audiod out of the way
 * until the audio team can come up with a proper implementation.
 */

class UnknownDevice : public AudioDevice
{
public:

    /// Perform the task expected by HW when a particular phone event occurs
    virtual bool        phoneEvent(EPhoneEvent event, int parameter = -1)
    {
        return false;
    }
    /// During phone calls, change volume & mute state
    virtual bool        setVolume(const ConstString &scenario, int volume)
    {
        return false;
    }
    virtual bool        setMuted(const ConstString &scenario, bool muted)
    {
        return false;
    }

    /// Request various actions from HW
    virtual void        generateAlertSound(int sound, Callback *callback) {}

    /// specify backspeaker filter for pulse
    virtual int         getBackSpeakerFilter()
    {
        return 0;
    }
    const char        *getName()
    {
        return "Unknown Device";
    }

    virtual bool post_init() 
    {  
        gState.setRingerOn(true);
        return true;
    }

    virtual bool        pre_init()
    {
        g_critical("*************************************************************************************************");
        g_critical("* This target is NOT supported by audiod! Please contact the audiod team to get support for it! *");
        g_critical("* Until this work is complete, do not expect any audio to be routed or play...                  *");
        g_critical("*************************************************************************************************");
        return true;
    }
};

AudioDevice &AudioDevice::get()
{
    static UnknownDevice sUnknownDevice;
    return sUnknownDevice;
}
