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

#include "module.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "audioMixer.h"

#define SYSFS_HEADSET_MIC "/sys/devices/virtual/switch/h2w/state"

static bool setMicOrHeadset (EHeadsetState state, int cardno, int deviceno, int status);

#if defined(AUDIOD_PALM_LEGACY)
static bool
_event(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    bool retVal = false;
    LSMessageJsonParser    msg(message, SCHEMA_3(REQUIRED(event, string),OPTIONAL(soundcard_no, integer),OPTIONAL(device_no, integer)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar * reply = STANDARD_JSON_SUCCESS;

    // read optional parameters with appropriate default values
    int soundcard_no = 0;
    int device_no = 0;
    if (!msg.get("soundcard_no", soundcard_no))
        soundcard_no = 0;
    if (!msg.get("device_no", device_no))
        device_no = 0;

    std::string event;
    if (!msg.get("event", event))
    {
        reply = MISSING_PARAMETER_ERROR(event, string);
    }
    else
    {
        if (event == "headset-removed")
        {
            //same way like setMicOrHeadset needs to be implemented
            //gState.setHeadsetState (eHeadsetState_None);
        }
        else if (event == "headset-inserted")
        {
            //same way like setMicOrHeadset needs to be implemented
            //gState.setHeadsetState (eHeadsetState_Headset);
        }
        else if (event == "headset-mic-inserted")
        {
            //same way like setMicOrHeadset needs to be implemented
            //gState.setHeadsetState (eHeadsetState_HeadsetMic);
        }
        else if (event == "usb-mic-inserted")
        {
            retVal = setMicOrHeadset (eHeadsetState_UsbMic_Connected , soundcard_no, device_no, 1);
            if (false == retVal)
              reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
        }
        else if (event == "usb-mic-removed")
        {
            retVal = setMicOrHeadset (eHeadsetState_UsbMic_DisConnected , soundcard_no, device_no, 0);
            if (false == retVal)
              reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
        }
        else if (event == "usb-headset-inserted")
        {
            retVal = setMicOrHeadset (eHeadsetState_UsbHeadset_Connected , soundcard_no, device_no, 1);
            if (false == retVal)
              reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
        }
        else if (event == "usb-headset-removed")
        {
            retVal = setMicOrHeadset (eHeadsetState_UsbHeadset_DisConnected , soundcard_no, device_no, 0);
            if (false == retVal)
              reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
        }
        else
            reply = INVALID_PARAMETER_ERROR(event, string);
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool setMicOrHeadset (EHeadsetState state, int cardno, int deviceno, int status)
{
    bool ret = false;
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    if (state == eHeadsetState_UsbMic_Connected || state == eHeadsetState_UsbMic_DisConnected)
        if (audioMixerObj)
            ret = audioMixerObj->loadUSBSinkSource('j',cardno,deviceno,status);
    else if (state == eHeadsetState_UsbHeadset_Connected || state == eHeadsetState_UsbHeadset_DisConnected)
        if (audioMixerObj)
            ret = audioMixerObj->loadUSBSinkSource('z',cardno,deviceno,status);
    else
        return false;

    if (false == ret) {
        g_debug("Failed execution of loadUSBSinkSource");
        return false;
    }
    return true;
}

static LSMethod udevMethods[] = {

    { "event", _event},
    {},
};
#endif

static int UdevInit()
{
    /**
     * lets us detect if we want to use udev or com.palm.keys for
     * headset events and let's be smart about.
     */
    if (TRUE == g_file_test ("/etc/udev/rules.d/86-audiod.rules", (GFileTest)
        (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)))
    {
        g_message ("Using udev headset events");
        //will be implemented as per DAP design
    }

    return 0;
}

static void readInitialUdevHeadsetState()
{
    //will be implemented as per DAP design
}

int
UdevInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    //will be implemented as per DAP design
#if defined(AUDIOD_PALM_LEGACY)
    /* luna service interface */
    CLSError lserror;
    bool result;
    result = ServiceRegisterCategory ("/udev", udevMethods, NULL, NULL);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/udev");
        return (-1);
    }
#endif

    return 0;
}

INIT_FUNC (UdevInit);
SERVICE_START_FUNC (UdevInterfaceInit);
