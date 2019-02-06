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

#include "module.h"
#include "scenario.h"
#include "utils.h"
#include "messageUtils.h"
#include "state.h"
#include "phone.h"
#include "media.h"
#include "voicecommand.h"
#include "log.h"
#include "system.h"

static bool
_systemmanagerLockedSubscription(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__))
        return true;

    bool locked;
    if (msg.get("locked", locked))
        gState.setPhoneLocked (locked);

    return true;
}

static bool
_systemmanagerLockModeSubscription(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__))
        return true;

    std::string lockMode;
    if (!msg.get("lockMode", lockMode)) return true;
    std::string policyState;
    if (msg.get("policyState", policyState)) return true;

    gState.setPhoneSecureLockActive(!(lockMode == "none" &&
                                      policyState != "pending"));

    return true;
}

static bool
_systemmanagerServerStatus(LSHandle *sh,
                           const char *serviceName,
                           bool connected,
                           void *ctx)
{
    bool result;
    CLSError lserror;

    if (connected)
    {
        // we have a winner!
        g_message ("%s: connecting to '%s' server", __FUNCTION__, serviceName);
        result = LSCall(sh, "palm://com.palm.systemmanager/getLockStatus",
                            "{\"subscribe\":true}",
                            _systemmanagerLockedSubscription,
                            ctx, NULL, &lserror);
        if (!result)
            lserror.Print(__FUNCTION__, __LINE__);

        result = LSCall(sh, "palm://com.palm.systemmanager/getDeviceLockMode",
                            "{\"subscribe\":true}",
                            _systemmanagerLockModeSubscription,
                             ctx, NULL, &lserror);
        if (!result)
            lserror.Print(__FUNCTION__, __LINE__);
    }
    return true;
}

static bool
_systemserviceSubscription(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__))
        return true;

    bool systemSounds;
    if (msg.get("systemSounds", systemSounds) && VERIFY(getSystemModule()))
        getSystemModule()->setMuted(!systemSounds);

    // Voice Command active if pref exists & is set, otherwise, disable
    bool voiceCommand;
    if (msg.get("enableVoiceCommand",
                voiceCommand) && VERIFY(getVoiceCommandModule()))
        getVoiceCommandModule()->setIsEnabled(voiceCommand);

    return true;
}

static bool
_systemserviceServerStatus(LSHandle *sh,
                           const char *serviceName,
                           bool connected,
                           void *ctx)
{
    bool result;
    CLSError lserror;

    if (connected)
    {
        // we have a winner!
        g_message ("%s: connecting to '%s' server", __FUNCTION__, serviceName);
        result = LSCall(sh, "palm://com.palm.systemservice/getPreferences",
                            "{\"keys\":[\"systemSounds\",\"enableVoiceCommand\"],  \
                             \"subscribe\": true}",
                            _systemserviceSubscription, ctx, NULL, &lserror);
        if (!result)
            lserror.Print(__FUNCTION__, __LINE__);
    }
    return true;
}

int
SystemmanagerInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    /* luna service interface */
    bool result;
    CLSError lserror;

    // check if the service is up
    result = LSRegisterServerStatusEx(handle, "com.palm.systemservice",
                 _systemserviceServerStatus, loop, NULL, &lserror);
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);

    return 0;
}

SERVICE_START_FUNC (SystemmanagerInterfaceInit);

