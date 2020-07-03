// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#include <lunaservice.h>
#include "utils.h"
#include "messageUtils.h"
#include "module.h"
#include "umiDispatcher.h"
#include "main.h"

Dispatcher * Dispatcher :: getDispatcher()
{
    static Dispatcher dispatcherinstance;
    return &dispatcherinstance;
}

void Dispatcher :: notifyGetStatusSubscribers()
{
    g_debug ("Dispatcher notifyGetStatusSubscribers");
    CLSError lserror;
    LSHandle *lsHandle = GetPalmService ();
    if (!getAudioOutputStatus (lsHandle, nullptr, nullptr))
        lserror.Print(__FUNCTION__, __LINE__);
}

Dispatcher :: ~Dispatcher()
{
    g_debug ("Dispatcher destructor");
}

bool Dispatcher :: _mute(LSHandle *lshandle, LSMessage *message, void *ctx) {

   /*TBD: Implementation of input volume mute/unmute
     Temporarily, sending success message on receiving this API */

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_4(PROP(source, string),
    PROP(sourcePort, integer), PROP(sink, string), PROP(mute, boolean))
    REQUIRED_4(source, sourcePort, sink, mute)));

   std::string reply = STANDARD_JSON_SUCCESS;

   if(!msg.parse(__FUNCTION__,lshandle))
       return true;

   CLSError lserror;
   if (!LSMessageRespond(message,reply.c_str(), &lserror))
   {
       lserror.Print(__FUNCTION__, __LINE__);
   }

   return true;
}

bool Dispatcher :: _connectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return true;
}

bool Dispatcher :: _disconnectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return true;
}

bool Dispatcher :: getAudioOutputStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return true;
}

bool Dispatcher :: _getStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    g_debug("Dispatcher ::_getStatus:: Audio get status request");
    LSMessageJsonParser msg (message, NORMAL_SCHEMA(PROPS_1(PROP(subscribe, boolean))));

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;

    bool subscribed = false;
    CLSError lserror;
    if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        return false;
    }

    if (nullptr != ctx)
        static_cast <Dispatcher *> (ctx)-> getAudioOutputStatus(lshandle, message, ctx);
    return true;
}

bool Dispatcher ::_getAudioOutputStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx)
{
    g_debug("_getAudioOutputStatusCallback Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                            REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;
    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        g_debug("%s:%d Could not get status", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData (reply, STRICT_SCHEMA(PROPS_2(PROP(returnValue, boolean),
        PROP(audio, array)) REQUIRED_2(returnValue, audio)));
    }
    std::string payload = LSMessageGetPayload(reply);
    if (nullptr != ctx)
    {
        envelopeRef *handle = (envelopeRef *)ctx;
        LSMessage *messageptr = (LSMessage*)handle->message;

        if (nullptr != messageptr)
        {
            CLSError lserror;
            if (!LSMessageRespond(messageptr, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
                g_message("Inside LSMessageReply if");
            }
            LSMessageUnref(messageptr);
        }

        delete handle;
        handle = nullptr;
    }
    else
    {
        g_debug("context is null");

        /* Update Notifiers */
        LSHandle *lsHandle = GetPalmService();
        CLSError lserror;
        if (! LSSubscriptionReply(lsHandle, GET_STATUS_CATEGORY_AND_KEY, payload.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            g_message("Notify error");
        }
    }
    return true;
}

LSMethod Dispatcher :: DispatcherMethods[] =
{
    { "connect", Dispatcher::_connectAudioOut},
    { "disconnect", Dispatcher::_disconnectAudioOut},
    { "getStatus", Dispatcher::_getStatus},
    { "mute", Dispatcher::_mute},
    { },
};

int dispatcherptrInterfaceInit(void)
{
    CLSError lSError;
    LSHandle *sh = nullptr;
    bool result = false;

    LSHandle *handle = GetPalmService();
    result = LSRegisterCategoryAppend(handle, UMI_CATEGORY_NAME, Dispatcher :: DispatcherMethods, nullptr, &lSError);

    if (!result || !LSCategorySetData(handle, UMI_CATEGORY_NAME, (Dispatcher :: getDispatcher()), &lSError))
    {
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, UMI_CATEGORY_NAME);
        LSErrorPrint(&lSError, stderr);
        return (-1);
    }
    return 0;
}

INIT_FUNC (dispatcherptrInterfaceInit);
