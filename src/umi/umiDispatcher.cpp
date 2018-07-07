// Copyright (c) 2018 LG Electronics, Inc.
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
#include "system.h"
#include "utils.h"
#include "messageUtils.h"
#include "module.h"
#include "AudiodCallbacks.h"
#include "umiScenarioModule.h"
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

UMIScenarioModule * Dispatcher :: getModule (std::string &name)
{
    g_debug ("Get %s UMI module object", name.c_str());
    DispatchstreamMap::iterator iter = dispatcher_map.find(name.c_str());
    return (iter != dispatcher_map.end()) ? iter->second : nullptr;
}

bool Dispatcher :: registerModule (std::string name, UMIScenarioModule *obj)
{
    dispatcher_map[name.c_str()] = obj;
    g_debug ("%s UMI module has been registered", name.c_str());
    return true;
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
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_6(PROP(source, string),
    PROP(sourcePort, integer), PROP(sink, string), PROP(audioType,string),
    PROP(outputMode,string), PROP(context,string))
    REQUIRED_5(source, sourcePort, sink, outputMode, audioType)));

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;
    std::string audioType;
    msg.get("audioType",audioType);
    g_debug("Dispatcher::_connectAudioOut:: Audio connect request");
    UMIScenarioModule *module = nullptr;
    if (nullptr != ctx)
        module = static_cast <Dispatcher *> (ctx) -> getModule (audioType);
    if (!VERIFY (module))
    {
        CLSError lserror;
        std::string reply = STANDARD_JSON_SUCCESS;
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_AUDIO_TYPE, "Internal error");
        if (!LSMessageRespond(message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        g_debug ("Calling connect for audiotype : %s", audioType.c_str());
        module->connectAudioOut (lshandle, message, ctx);
    }
    return true;
}

bool Dispatcher :: _disconnectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_5(PROP(source, string),
    PROP(sourcePort, integer), PROP(sink, string), PROP(audioType,string), PROP(context,string))
    REQUIRED_4(source, sourcePort, sink, audioType)));

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;

    std::string audioType;
    msg.get("audioType",audioType);
    g_debug("Dispatcher::_disconnectAudioOut:: Audio disconnect request");
    UMIScenarioModule *module = nullptr;
    if (nullptr != ctx)
        module = static_cast <Dispatcher *> (ctx)-> getModule (audioType);
    if (!VERIFY (module))
    {
        CLSError lserror;
        std::string reply = STANDARD_JSON_SUCCESS;
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_AUDIO_TYPE, "Internal error");
        if (!LSMessageRespond(message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        g_debug ("Calling disconnect for audiotype : %s", audioType.c_str());
        module->disconnectAudioOut (lshandle, message, ctx);
    }
    return true;
}

bool Dispatcher :: getAudioOutputStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    envelopeRef *handle = nullptr;

    if (nullptr != message)
    {
        handle = new envelopeRef();
        if(nullptr != handle)
        {
            handle->message = message;
        }
        LSMessageRef(message);
    }
    umiaudiomixer * mixerHandle = umiaudiomixer::getUmiMixerInstance();
    std::string reply = STANDARD_JSON_SUCCESS;
    bool returnvalue = false;
    if(nullptr != mixerHandle)
    {
        if (mixerHandle -> getConnectionStatus(_getAudioOutputStatusCallback, handle))
        {
            returnvalue = true;
        }
        else
        {
            g_debug("get connect status umimixer call failed");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
        }
    }
    else
    {
        g_debug("Audio get connect status :mixerHdl is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
    }
    if (false == returnvalue)
    {
        CLSError lserror;
        if (nullptr != message)
            if (!LSMessageRespond(message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        if (nullptr != handle)
        {
            delete handle;
            handle = nullptr;
        }
    }
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
