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
#include "mock.h"
#include "main.h"
#include "umiaudiomixer.h"
#include "umiScenarioModule.h"
#include "umiDispatcher.h"

#define MOCK_MEDIA_KEY "umimedia"

static const ConstString    cMockMedia_Default(MOCK_MEDIA_KEY SCENARIO_DEFAULT);

bool MockScenarioModule :: connectAudioOut(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, "{}");
    if(!msg.parse(__FUNCTION__, lshandle))
        return true;

    std::string sourceName;
    int sourcePort = 0;

    msg.get("source", sourceName);
    msg.get("sink", sourceSinkInfo.sinkName);
    msg.get("sourcePort", sourcePort);

    std::string source = sourceName;
    if ("AMIXER" != sourceName)
        source = sourceName + std::to_string(sourcePort);

    g_debug("%s::_connectAudioOut Audio connect request for source %s, sink %s\
            source port %d source %s", __FILE__, sourceName.c_str(),
            sourceSinkInfo.sinkName.c_str(), sourcePort, source.c_str());

    sourceSinkInfo.sourceName = source;

    envelopeRef *handle = new (std::nothrow) envelopeRef();
    std::string reply = STANDARD_JSON_SUCCESS;
    bool returnvalue = false;
    if(nullptr != handle)
    {
        handle->message = message;
        handle->context = this;
        if(nullptr != mixerHandle)
        {
            if (mixerHandle -> connectAudio(source,
                sourceSinkInfo.sinkName,
                _connectStatusCallback, handle))
            {
                LSMessageRef(message);
                returnvalue = true;
            }
            else
            {
                g_debug("connect umimixer call failed");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
            }
        }
        else
        {
            g_debug("Audio connect :mixerHdl is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE, "Internal error");
    }
    if (false == returnvalue)
    {
        CLSError lserror;
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

bool MockScenarioModule ::_connectStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx)
{
    g_debug("_updateConnectStatus Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                                                 REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;

    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        g_debug("%s:%d Could not Connect", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData (reply, STRICT_SCHEMA(PROPS_3(PROP(returnValue, boolean),
        PROP(source, string), PROP(sink, string)) REQUIRED_3(returnValue, source, sink)));
        std::string sourceString;
        std::string sinkString;
        msgData.get("source", sourceString);
        msgData.get("sink", sinkString);
        g_debug("Connected successfully for source %s Physicalsink %s",
                sourceString.c_str(), sinkString.c_str());
    }

    if (nullptr != ctx)
    {
        envelopeRef *handle = (envelopeRef *)ctx;
        LSMessage *messageptr = (LSMessage*)handle->message;
        MockScenarioModule *object = (MockScenarioModule *)handle->context;

        if (nullptr != messageptr)
        {
            CLSError lserror;
            std::string payload = LSMessageGetPayload(reply);
            if (!LSMessageRespond(messageptr, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
                g_message("Inside LSMessageReply if");
            }
            LSMessageUnref(messageptr);
        }
        if (nullptr != object)
        {
            if(nullptr != object -> mixerHandle)
            {
                object -> mixerHandle -> onSinkChangedReply(eumiMedia,eConnect,eUmi);
            }
            else
            {
                g_debug("Audio connect return :mixerHdl is NULL");
            }
        }

        delete handle;
        handle = nullptr;
    }
    else
    {
        g_debug("context is null");
    }
    /* Reply for Subscribers of get status */
    if (Dispatcher * pDispatcher = Dispatcher :: getDispatcher())
        pDispatcher -> notifyGetStatusSubscribers ();
    return true;
}

bool MockScenarioModule :: disconnectAudioOut(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, "{}");
    if(!msg.parse(__FUNCTION__, lshandle))
        return true;
    std::string sourceName;
    int sourcePort = 0;

    msg.get("source", sourceName);
    msg.get("sink", sourceSinkInfo.sinkName);
    msg.get("sourcePort",sourcePort);

    std::string source = sourceName;
    if ("AMIXER" != sourceName)
        source = sourceName + std::to_string(sourcePort);

    g_debug("%s::_disconnectAudioOut Audio disconnect request for source %s, sink %s\
            source port %d source %s", __FILE__, sourceName.c_str(),
            sourceSinkInfo.sinkName.c_str(), sourcePort, source.c_str());

    sourceSinkInfo.sourceName = source;

    envelopeRef *handle = new (std::nothrow) envelopeRef();
    bool returnvalue = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    if(nullptr != handle)
    {
        handle->message = message;
        handle->context = this;
        if(nullptr != mixerHandle)
        {
            if (mixerHandle -> disconnectAudio(source,
                sourceSinkInfo.sinkName,
                _disConnectStatusCallback, handle))
            {
                LSMessageRef(message);
                returnvalue = true;
            }
            else
            {
                g_debug("disconnect umimixer call failed");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
            }
        }
        else
        {
            g_debug("Audio disconnect :mixerHdl is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }
    if (false == returnvalue)
    {
        CLSError lserror;
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

bool MockScenarioModule :: _disConnectStatusCallback (LSHandle *sh, LSMessage *reply, void *ctx)
{
    g_debug("_updateDisConnectStatus Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                                                 REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;

    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        g_debug("%s:%d no update. Could not DisConnect", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData (reply, STRICT_SCHEMA(PROPS_3(PROP(returnValue, boolean),
        PROP(source, string), PROP(sink, string)) REQUIRED_3(returnValue, source, sink)));
        std::string sourceString;
        std::string sinkString;
        msgData.get("source", sourceString);
        msgData.get("sink", sinkString);
        g_debug("Disconnected successfully for source %s Physicalsink %s ",\
                sourceString.c_str(), sinkString.c_str());
    }
    if (nullptr != ctx)
    {
        envelopeRef *handle = (envelopeRef *)ctx;
        LSMessage *messageptr = (LSMessage*)handle->message;
        MockScenarioModule *object = (MockScenarioModule *)handle->context;

        if (nullptr != messageptr)
        {
            CLSError lserror;
            std::string payload = LSMessageGetPayload(reply);
            if (!LSMessageRespond(messageptr, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
                g_message("Inside LSMessageReply if");
            }
            LSMessageUnref(messageptr);
        }
        if (nullptr != object)
        {
            if(nullptr != object -> mixerHandle)
            {
                object -> mixerHandle -> onSinkChangedReply(eumiMedia,eDisConnect,eUmi);
            }
            else
            {
                g_debug("Audio connect return :mixerHdl is NULL");
            }
        }

        delete handle;
        handle = nullptr;
    }
    else
    {
        g_debug("context is null");
    }
    /* Reply for Subscribers of get status */
    if (Dispatcher * pDispatcher = Dispatcher :: getDispatcher())
        pDispatcher -> notifyGetStatusSubscribers ();
    return true;
}

void MockScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
      g_debug("MockScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
              virtualSinkName(sink), controlEventName(event),(int)p_eSinkType);
      if(p_eSinkType == ePulseAudio)
      {
          g_debug("MockScenarioModule: pulse audio onsink changed");
      }
      else
      {
          g_debug("MockScenarioModule: UMI onsink changed");
      }
}


MockScenarioModule :: MockScenarioModule():UMIScenarioModule(ConstString("/MockScenarioModule")),
               mMockScenarioModuleVolume(cMockMedia_Default, 80), mMockScenarioModuleMuted (false)
{
    gAudiodCallbacks.registerModuleCallback(this,eumiAll,true);
    gAudiodCallbacks.registerModuleCallback(this, eVirtualSink_All);

    GenericScenario *mockMediaScenario = new (std::nothrow) GenericScenario (cMockMedia_Default,
                                                                            true,
                                                                            eScenarioPriority_TV_MasterVolume,
                                                                            mMockScenarioModuleVolume);
    if(nullptr != mockMediaScenario)
        addScenario (mockMediaScenario);
    else
        g_debug("Unable to add Mock scenario");
    restorePreferences();
    setMuted(false);
}

MockScenarioModule :: ~MockScenarioModule()
{
    g_debug ("MockScenarioModule destructor");
}

MockScenarioModule * MockScenarioModule :: getMockScenarioModule()
{
    static MockScenarioModule mockUMISMInstance;
    return &mockUMISMInstance;
}

int MockScenarioModuleInit(GMainLoop *loop, LSHandle *handle)
{
    Dispatcher * pDispatcher = Dispatcher :: getDispatcher();
    bool returnValue = false;
    g_debug ("registering Sample UMI");
    returnValue = pDispatcher -> registerModule (MOCK_MEDIA_KEY, MockScenarioModule :: getMockScenarioModule());
    return 0;
}

MODULE_START_FUNC (MockScenarioModuleInit);
