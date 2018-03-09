// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "messageUtils.h"
#include "state.h"
#include "utils.h"
#include "vibrate.h"

#define GET_SYSTEM_SETTINGS "luna://com.webos.settingsservice/getSystemSettings"
#define DND_PARAMS "{\"category\":\"DND\",\"subscribe\":true}"
#define MEDIA_PARAMS "{\"category\":\"Media\",\"subscribe\":true}"

bool __settingCallback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__func__))
        return true;

    bool touchEnable;
    bool dndEnable;
    int amplitude = 0;
    pbnjson::JValue json_status = msg.get();
    pbnjson::JValue object = json_status["settings"];

    if (!object.isObject())
        return true;

    if (object["touchSoundEnable"].asBool(touchEnable) == CONV_OK)
        gState.setTouchSound(touchEnable);
    else
        g_debug("__settingCallback() touchSoundEnable field not found");

    if (object["dndEnable"].asBool(dndEnable) == CONV_OK)
        gState.setDndMode(dndEnable);
    else
        g_debug("__settingCallback(): dndEnable field not found");

    if (object["amplitude"].asNumber(amplitude) == CONV_OK)
        getVibrateDevice()->setVibrateAmplitude(amplitude);
    else
        g_debug("_settingCallback() amplitude field not found");

    return true;
}

static bool
_settingsServiceServerStatus( LSHandle *sh,
                      const char *serviceName,
                      bool connected,
                      void *ctx)
{
    CLSError lserror;

    if (connected)
    {
        // we have a winner!
        g_message ("%s: connecting to '%s' server", __FUNCTION__, serviceName);

        if (!LSCall(sh, GET_SYSTEM_SETTINGS, MEDIA_PARAMS, __settingCallback, NULL, NULL, &lserror))
            lserror.Print(__func__, __LINE__);

        if (!LSCall(sh, GET_SYSTEM_SETTINGS, DND_PARAMS, __settingCallback, NULL, NULL, &lserror))
            lserror.Print(__func__, __LINE__);
    }

    return true;
}

int
SettingsServiceInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    /* luna service interface */
    CLSError lserror;

    // check if the service is up
    bool result = LSRegisterServerStatus(handle, "com.webos.settingsservice",
                                         _settingsServiceServerStatus, loop, &lserror);
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);

    return 0;
}

SERVICE_START_FUNC(SettingsServiceInterfaceInit);

