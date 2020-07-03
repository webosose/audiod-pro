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
#include "state.h"
#include "log.h"

#define SYSFS_HEADSET_MIC "/sys/devices/virtual/switch/h2w/state"

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

    if (gState.getUseUdevForHeadsetEvents()) {
        std::string event;

        if (!msg.get("event", event)) {
            reply = MISSING_PARAMETER_ERROR(event, string);
        } else {
              if (event == "headset-removed")
                  gState.setHeadsetState (eHeadsetState_None);

              else if (event == "headset-inserted")
                  gState.setHeadsetState (eHeadsetState_Headset);

              else if (event == "headset-mic-inserted")
                  gState.setHeadsetState (eHeadsetState_HeadsetMic);

              else if (event == "usb-mic-inserted") {
                  retVal = gState.setMicOrHeadset (eHeadsetState_UsbMic_Connected , soundcard_no, device_no, 1);
                  if (false == retVal)
                    reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
              }
              else if (event == "usb-mic-removed") {
                  retVal = gState.setMicOrHeadset (eHeadsetState_UsbMic_DisConnected , soundcard_no, device_no, 0);
                  if (false == retVal)
                    reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
              }
              else if (event == "usb-headset-inserted") {
                  retVal = gState.setMicOrHeadset (eHeadsetState_UsbHeadset_Connected , soundcard_no, device_no, 1);
                  if (false == retVal)
                    reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
              }
              else if (event == "usb-headset-removed") {
                  retVal = gState.setMicOrHeadset (eHeadsetState_UsbHeadset_DisConnected , soundcard_no, device_no, 0);
                  if (false == retVal)
                    reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
              }
              else
                  reply = INVALID_PARAMETER_ERROR(event, string);
        }
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

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
    gState.setUseUdevForHeadsetEvents(true);
    }

    return 0;
}

static void readInitialUdevHeadsetState()
{
    FILE * hsfile  = 0;
    int headsetMic = 0;

    hsfile = fopen(SYSFS_HEADSET_MIC, "r");
    if (VERIFY(hsfile))    {
        fscanf(hsfile, "%i\n", &headsetMic);
    fclose(hsfile);

        if (1 == headsetMic)
        gState.setHeadsetState(eHeadsetState_Headset);

    else if(2 == headsetMic)
            gState.setHeadsetState(eHeadsetState_HeadsetMic);

    else
            gState.setHeadsetState(eHeadsetState_None);
    }
}

int
UdevInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    if (gState.getUseUdevForHeadsetEvents())
        readInitialUdevHeadsetState();

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
