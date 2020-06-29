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

#include "scenario.h"
#include "module.h"
#include "update.h"
#include "utils.h"
#include "messageUtils.h"
#include "volume.h"
#include "media.h"
#include "state.h"
#include "main.h"
#include "log.h"
#include "VolumeControlChangesMonitor.h"
#include "AudioDevice.h"
#include "PulseAudioMixer.h"
#include "main.h"
#include "genericScenarioModule.h"
#include "default2.h"

#define SUBSCRIPTION_KEY "lockVolumeKeysSubscription"

#define VOLUME_GAIN(x) ((x) ? "volume" : "mic_gain")

#define ZERO_IF_EMPTY(x) (x.empty() ? 0 : x.c_str())

#define DISPLAY_ONE 0
#define DISPLAY_TWO 1

static bool
_setVolumeOrMicGain(LSHandle *lshandle,
                    LSMessage *message,
                    void *ctx,
                    bool volumeNotMicGain)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    int display = 0;

    const char * schema = volumeNotMicGain ?
                          SCHEMA_3(REQUIRED(volume, integer),
                                              OPTIONAL(scenario, string),
                                              OPTIONAL(sessionId, integer)) :
                          SCHEMA_2(REQUIRED(mic_gain, integer),
                                               OPTIONAL(scenario, string));

    LSMessageJsonParser    msg(message, schema);
    if (!msg.parse(volumeNotMicGain ? "setVolume" :
                                      "setMicGain",
                                      lshandle,
                                      eLogOption_LogMessageWithCategory))
        return true;

    const char * reply = STANDARD_JSON_SUCCESS;
    const char * parameter = VOLUME_GAIN(volumeNotMicGain);
    std::string scenario;
    int volume = -1;
    ScenarioModule * module = (ScenarioModule*) ctx;

    if (!msg.get(parameter, volume))
    {
        reply = volumeNotMicGain ?
                MISSING_PARAMETER_ERROR(volume, integer) :
                MISSING_PARAMETER_ERROR(mic_gain, integer);
        goto error;
    }


    if (volume < cVolumePercent_Min || volume > cVolumePercent_Max)
    {
        reply = volumeNotMicGain ?
                INVALID_PARAMETER_ERROR(volume, integer) :
                INVALID_PARAMETER_ERROR(mic_gain, integer);
        goto error;
    }

    // scenario is optional. If not present,
    // we will pass 0 to mean "current scenario"
    if (!msg.get("scenario", scenario))
         scenario.clear();

    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
    {
        Default2ScenarioModule * s = getDefault2Module();
        s->setVolume(volume);
    }
    else
    {
        if (!module->setScenarioVolumeOrMicGain(ZERO_IF_EMPTY(scenario),
                                                     volume, volumeNotMicGain))
        {
            reply = STANDARD_JSON_ERROR(3, "failed to set parameter (invalid scenario name?)");
            goto error;
        }
    }

error:

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return _setVolumeOrMicGain(lshandle, message, ctx, true);
}

#if defined(AUDIOD_PALM_LEGACY)
bool
_setMicGain(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return _setVolumeOrMicGain(lshandle, message, ctx, false);
}

#endif
static bool
_getVolumeOrMicGain(LSHandle *lshandle,
                    LSMessage *message,
                    void *ctx,
                    bool volumeNotMicGain)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    const char * schema = SCHEMA_1(OPTIONAL(scenario, string));


    LSMessageJsonParser msg(message, schema);
    if (!msg.parse(volumeNotMicGain ? "getVolume" :
                                      "getMicGain",
                                      lshandle,
                                      eLogOption_LogMessageWithCategory))
        return true;

    const char * parameter = VOLUME_GAIN(volumeNotMicGain);
    ScenarioModule * module = (ScenarioModule*) ctx;

    // scenario is optional. If not present,
    // we will pass 0 to mean "current scenario"
    std::string scenario;
    if (!msg.get("scenario", scenario))
        scenario.clear();

    std::string reply;
    int volume = -1;
    if (module->getScenarioVolumeOrMicGain(ZERO_IF_EMPTY(scenario),
                                            volume, volumeNotMicGain))
    {
        reply = string_printf("{\"returnValue\":true,\"scenario\":\"%s\",\"%s\":%i}",
                             (scenario.empty() ?
                              module->getCurrentScenarioName() :
                              scenario.c_str()), parameter, volume);

    }
    else
        reply = STANDARD_JSON_ERROR(3,
                           "failed to get parameter (invalid scenario name?)");

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool _getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    g_message("[getVolume Triggered]");
    g_message("[getVolumeOrMicGain Triggered] --%s",__FUNCTION__);
    return _getVolumeOrMicGain(lshandle, message, ctx, true);
}

#if defined(AUDIOD_PALM_LEGACY)
bool
_getMicGain(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return _getVolumeOrMicGain(lshandle, message, ctx, false);
}
#endif
static bool
_offsetVolumeOrMicGain(LSHandle *lshandle,
                       LSMessage *message,
                       void *ctx,
                       bool volumeNotMicGain)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message,
                            SCHEMA_3(REQUIRED(offset, integer),
                            OPTIONAL(scenario, string),
                            OPTIONAL(unit, string)));
    if (!msg.parse(volumeNotMicGain ? "offsetVolume" :
                                      "offsetMicGain",
                                       lshandle,
                                       eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*)ctx;

    const char *    reply = STANDARD_JSON_SUCCESS;
    std::string        scenario;
    std::string        unit;
    int                volume = -1;
    int                offset = -1;

    if (!msg.get("offset", offset))
    {
        reply = MISSING_PARAMETER_ERROR(offset, integer);
        goto error;
    }

    // scenario parameter is optional
    if (!msg.get("scenario", scenario))
        scenario.clear();

    if (!module->getScenarioVolumeOrMicGain(ZERO_IF_EMPTY(scenario),
                                            volume, volumeNotMicGain))
    {
           reply = STANDARD_JSON_ERROR(3,
                            "failed to set parameter (invalid scenario name?)");
        goto error;
    }

    // unit parameter is optional
    if (!msg.get("unit", unit))
        unit.clear();

    if (unit.empty() || unit == "step")
    {
        if (offset < -20 || offset > 20)
        {
            reply = INVALID_PARAMETER_ERROR(offset, integer);
            goto error;
        }
        while (offset > 0)
        {
            volume = SCVolumeUp(volume);
            --offset;
        }
        while (offset < 0)
        {
            volume = SCVolumeDown(volume);
            ++offset;
        }
    }
    else if (unit == "percent")
    {
        if (offset < -cVolumePercent_Max || offset > cVolumePercent_Max)
        {
            reply = INVALID_PARAMETER_ERROR(offset, integer);
            goto error;
        }
        volume += offset;
        if (volume < cVolumePercent_Min)
            volume = cVolumePercent_Min;
        else if (volume > cVolumePercent_Max)
            volume = cVolumePercent_Max;
    }
    else
    {
        reply = INVALID_PARAMETER_ERROR(unit, string);
        goto error;
    }

    if (!module->setScenarioVolumeOrMicGain(ZERO_IF_EMPTY(scenario),
                                            volume, volumeNotMicGain))
    {
           reply = STANDARD_JSON_ERROR(3,
                           "failed to set parameter (invalid scenario name?)");
        goto error;
    }

error:

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_offsetVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return _offsetVolumeOrMicGain(lshandle, message, ctx, true);
}

#if defined(AUDIOD_PALM_LEGACY)
bool
_offsetMicGain(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return _offsetVolumeOrMicGain(lshandle, message, ctx, false);
}

bool
_setLatency(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message,
                            SCHEMA_2(REQUIRED(latency, integer),
                            OPTIONAL(scenario, string)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*) ctx;
    std::string    scenario;
    int latency;
    const char * reply = STANDARD_JSON_SUCCESS;

    if (!msg.get("latency", latency))
    {
        reply = MISSING_PARAMETER_ERROR(latency, integer);
        goto error;
    }

    if (latency <= 0)
    {
        reply = INVALID_PARAMETER_ERROR(latency, integer);
        goto error;
    }

    // scenario parameter is optional
    if (!msg.get("scenario", scenario))
        scenario.clear();

    if (!module->setScenarioLatency(ZERO_IF_EMPTY(scenario), latency))
    {
        reply = STANDARD_JSON_ERROR(3,
                             "failed to set latency (invalid scenario name?)");
        goto error;
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_getLatency(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(OPTIONAL(scenario, string)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    std::string    scenario, reply;
    ScenarioModule * module = (ScenarioModule*) ctx;
    // scenario parameter is optional
    if (!msg.get("scenario", scenario))
        scenario.clear();

    int latency = SCENARIO_DEFAULT_LATENCY;
    if (module->getScenarioLatency(ZERO_IF_EMPTY(scenario), latency))
        reply = string_printf ("{\"returnValue\":true,\"scenario\":\"%s\",\"latency\":%i}",
                              (scenario.empty() ?
                               module->getCurrentScenarioName() :
                                scenario.c_str()), latency);
    else
        reply = STANDARD_JSON_ERROR(3,
                             "failed to get latency (invalid scenario name?)");

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}
#endif

bool
_enableScenario(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(scenario, string)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*)ctx;
    const gchar * reply = STANDARD_JSON_SUCCESS;
    std::string scenario;
    GenericScenario * s = nullptr;
    // scenario parameter is NOT optional
    if (!msg.get("scenario", scenario))
    {
        reply = MISSING_PARAMETER_ERROR(scenario, string);
        goto error;
    }

    s = module->getScenario(scenario.c_str());
    if (s == nullptr  || g_str_has_suffix(scenario.c_str(),"NULL"))
    {
        reply = INVALID_PARAMETER_ERROR(scenario, string);
        goto error;
    }

    if (!s->mEnabled && !module->enableScenario(scenario.c_str()))
    {
        reply = STANDARD_JSON_ERROR(3, "Could not enable requested scenario.");
        goto error;
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_disableScenario(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(scenario, string)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*)ctx;
    const gchar * reply = STANDARD_JSON_SUCCESS;
    std::string scenario;
    GenericScenario * s = nullptr;
    // scenario parameter is NOT optional
    if (!msg.get("scenario", scenario))
    {
        reply = MISSING_PARAMETER_ERROR(scenario, string);
        goto error;
    }

    s = module->getScenario(scenario.c_str());
    if (s == nullptr)
    {
        reply = INVALID_PARAMETER_ERROR(scenario, string);
        goto error;
    }

    if (s->mEnabled && !module->disableScenario(scenario.c_str()))
    {
        reply = STANDARD_JSON_ERROR(3, "Could not disable requested scenario.");
        goto error;
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_setCurrentScenario(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(scenario, string)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*)ctx;
    const gchar * reply = STANDARD_JSON_SUCCESS;
    std::string scenario;

    // scenario parameter is NOT optional
    if (!msg.get("scenario", scenario))
    {
        reply = MISSING_PARAMETER_ERROR(scenario, string);
        goto error;
    }

    if (!module->setCurrentScenario(scenario.c_str()))
    {
        reply = STANDARD_JSON_ERROR(3, "Could not set current scenario.");
        goto error;
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_getCurrentScenario(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_0);
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*)ctx;

    std::string reply = string_printf ("{\"returnValue\":true,\"scenario\":\"%s\"}",
                                             module->getCurrentScenarioName());

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_listScenarios(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message,
                            SCHEMA_2(OPTIONAL(enabled, boolean),
                                     OPTIONAL(disabled, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    ScenarioModule * module = (ScenarioModule*)ctx;

    // read optional parameters with appropriate default values
    bool    enabled, disabled;
    if (!msg.get("enabled", enabled))
        enabled = true;
    if (!msg.get("disabled", disabled))
        disabled = false;

    // build reply
    pbnjson::JValue reply = createJsonReply(true);
    pbnjson::JValue array = pbnjson::Array();
    const ScenarioModule::ScenarioMap &    scenarios = module->getScenarios();
    for (ScenarioModule::ScenarioMap::const_iterator iter = scenarios.begin();
                                              iter != scenarios.end(); ++iter)
    {
        GenericScenario *s = iter->second;
//        g_debug("%s: looking at '%s' scenario", __FUNCTION__, s->getName());
        if (true == s->mEnabled)
        {
            if (enabled)
                array << pbnjson::JValue(s->getName());
        }
        else
        {
            if (disabled)
                array << pbnjson::JValue(s->getName());
        }
    }
    reply.put("scenarios", array);

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, jsonToString(reply).c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_status(LSHandle *lshandle, LSMessage *message, void *ctx)
{

        LSMessageJsonParser msg(message, SCHEMA_1(OPTIONAL(subscribe, boolean)));
        if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
            return true;

    if (!VERIFY(ctx != 0 && message != 0))
        return true;
    ScenarioModule * module = (ScenarioModule*)ctx;

    CLSError lserror;
    bool subscribed = false;

    if (LSMessageIsSubscription (message))
    {
        if (!LSSubscriptionProcess (lshandle, message, &subscribed, &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    CHECK(module->sendRequestedUpdate (lshandle, message, subscribed));

    return true;
}

bool
GenericScenarioModule::broadcastEvent (const char *event)
{
    return CHECK(sendChangedUpdate(UPDATE_BROADCAST_EVENT, event));
}

bool
_broadcastEvent(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(event, string)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    const char * reply = STANDARD_JSON_SUCCESS;
    ScenarioModule *module = (ScenarioModule*)ctx;

    std::string    event;
    if (!msg.get("event", event))
    {
        reply = MISSING_PARAMETER_ERROR(event, string);
    }
    else if (!module->broadcastEvent(event.c_str()))
    {
        reply = STANDARD_JSON_ERROR(3, "Could not broadcast event.");
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
GenericScenarioModule::setMuted (bool muted)
{
    if (mMuted != muted)
    {
        mMuted = muted;

        programMuted();
        CHECK(sendChangedUpdate (UPDATE_CHANGED_MUTED));
    }

    return true;
}

bool
_setMuted(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(muted, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    const char * reply = STANDARD_JSON_SUCCESS;
    ScenarioModule * module = (ScenarioModule*)ctx;

    bool muted;
    bool success = true;

    if (!msg.get("muted", muted))
    {
        reply = MISSING_PARAMETER_ERROR(muted, boolean);
        success = false;
    }


    if(success)
    {
        module->setMuted(muted);
        gAudioDevice.setIncomingCallRinging(!muted);
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_lockVolumeKeys(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_2(OPTIONAL(subscribe, boolean),
                                              OPTIONAL(foregroundApp, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    CLSError lserror;
    const char * reply = STANDARD_JSON_SUCCESS;

    VolumeControlChangesMonitor    monitor;

    ScenarioModule *module = (ScenarioModule*)ctx;

    g_debug("%s: locking volume keys for module %s",
                                         __FUNCTION__, module->getCategory());
    LogIndent    indentLogs("| ");

    bool foregroundApp;
    if (msg.get("foregroundApp", foregroundApp) && foregroundApp)
    {
        // this a foreground app thingy
        module->setVolumeOverride(true);
    }
    else
    {
        foregroundApp = false;
    }

    if (!foregroundApp)
    {
        if (!gState.setLockedVolumeModule(module))
        {
            g_warning ("%s: could not lock volume keys.", __FUNCTION__);
            reply = STANDARD_JSON_ERROR(3, "Could not lock volume keys.");
            goto send;
        }
    }

    if (!LSSubscriptionAdd(lshandle, SUBSCRIPTION_KEY, message, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);

        reply = STANDARD_JSON_ERROR(3, "Failed to subscribe to lockVolumeKeys.");

        if (foregroundApp)
            module->setVolumeOverride (false);
        else
            gState.setLockedVolumeModule (NULL);
    }

send:

    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
ScenarioModule::museSet (bool enable)
{
    programMuse(enable);

    return true;
}
#if defined(AUDIOD_PALM_LEGACY)
bool
_museSet(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(enable, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle, eLogOption_LogMessageWithCategory))
        return true;

    const char * reply = STANDARD_JSON_SUCCESS;
    ScenarioModule * module = (ScenarioModule*)ctx;

    bool enable;
    if (!msg.get("enable", enable))
    {
        reply = MISSING_PARAMETER_ERROR(enable, boolean);
    }
    else
    {
        module->museSet (enable);
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_hacSet(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(enable, boolean)));
    if (!msg.parse(__FUNCTION__))
        return true;


    const char * reply = STANDARD_JSON_SUCCESS;
    ScenarioModule * module = (ScenarioModule*)ctx;

    bool enable;
    if (!msg.get("enable", enable))
        reply = MISSING_PARAMETER_ERROR(enable, boolean);
    else
        module->programHac (enable);

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_hacGet(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    CLSError lserror;
    bool subscribed = false;

    if (LSMessageIsSubscription (message))
    {
        if (!LSSubscriptionProcess (lshandle, message, &subscribed, &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    pbnjson::JValue reply = pbnjson::Object();

    reply.put("returnValue", true);
    reply.put("enabled", gState.hacGet());
    reply.put("subscribed", subscribed);

    std::string replyString = jsonToString(reply);
    g_debug("Hac enabled: %s", replyString.c_str());

    bool result = LSMessageReply(lshandle, message, replyString.c_str(), &lserror);
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);

    CHECK(result);

    return true;
}

bool
_callStatusUpdate(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser    msg(message, SCHEMA_1(REQUIRED(lines, array)));
    if (!msg.parse(__FUNCTION__))
        return true;

    const char * reply = STANDARD_JSON_SUCCESS;
    pbnjson::JValue    lines = msg.get()["lines"];
    if (!lines.isArray())
        return true;

    int carrier = 0, voip = 0;
    int length = lines.arraySize();
    std::string state[length];
    std::string transport[length];
    pbnjson::JValue calls;

    ECallStatus status = eCallStatus_NoCall;

    for (int i = 0; i < lines.arraySize(); i++) {
        if (lines[i]["state"].asString(state[i]) == CONV_OK)
            g_debug("state is %s", state[i].c_str());
        else
            g_warning("could not get state");

        calls = lines[i]["calls"];
        if (!calls.isArray()) {
            g_warning("calls is not an array!!!");
            return true;
        }

        if (calls[0]["transport"].asString(transport[i]) == CONV_OK)
            g_debug("transport is %s", transport[i].c_str());
        else
            g_warning("could not get transport");

        if (!strcmp(transport[i].c_str(), "com.palm.telephony")) {
            carrier++;
        } else {
            voip++;
        }

        if (voip) {
            if (!strcmp(state[i].c_str(), "active"))
                status = eCallStatus_Active;
            else if (!strcmp(state[i].c_str(), "incoming"))
                status = eCallStatus_Incoming;
            else if (!strcmp(state[i].c_str(), "connecting"))
                status = eCallStatus_Connecting;
            else if (!strcmp(state[i].c_str(), "dialing"))
                status = eCallStatus_Dialing;
            else if (!strcmp(state[i].c_str(), "disconnected"))
                status = eCallStatus_Disconnected;
            else if (!strcmp(state[i].c_str(), "onHold"))
                status = eCallStatus_OnHold;

            gState.setCallMode(eCallMode_Voip, status);

            bool outgoingvideo = false;
            bool incomingvideo = false;
            if (lines[i]["outgoingVideo"].asBool(outgoingvideo) == CONV_OK) {
                g_debug("outgoingvideo is %d", outgoingvideo);
            } else
                g_debug("could not get outgoingvideo");

            if (lines[i]["incomingVideo"].asBool(incomingvideo) == CONV_OK) {
                g_debug("incomingvideo is %d", incomingvideo);
            } else
                g_debug("could not get incomingvideo");

            gState.setCallWithVideo(outgoingvideo || incomingvideo);
        } else {
            if (!strcmp(state[i].c_str(), "active"))
                status = eCallStatus_Active;
            else if (!strcmp(state[i].c_str(), "incoming"))
                status = eCallStatus_Incoming;
            else if (!strcmp(state[i].c_str(), "connecting"))
                status = eCallStatus_Connecting;
            else if (!strcmp(state[i].c_str(), "dialing"))
                status = eCallStatus_Dialing;
            else if (!strcmp(state[i].c_str(), "disconnected"))
                status = eCallStatus_Disconnected;
            else if (!strcmp(state[i].c_str(), "onHold"))
                status = eCallStatus_OnHold;

            gState.setCallMode(eCallMode_Carrier, status);
        }
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

bool
_bluetoothAudioPropertiesSet(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    if (!VERIFY(ctx != 0 && message != 0))
        return true;

    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__))
        return true;

    const char * reply = STANDARD_JSON_SUCCESS;

    bool enable;
    bool carkit;

    if (!msg.get("carkit", carkit)) {
        reply = MISSING_PARAMETER_ERROR(carkit, boolean);
        goto error;
    }

    if (!msg.get("echocancellation", enable)) {
        reply = MISSING_PARAMETER_ERROR(enable, boolean);
        goto error;
    }

    g_debug("bluetooth carkit %d echo cancellation enable %d", carkit, enable);
    gAudioDevice.setBTSupportEC(carkit, enable);

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}
#endif

bool
GenericScenarioModule::setVolumeOverride (bool override)
{
    if (override)
        mVolumeOverride++;
    else
        mVolumeOverride--;

    g_debug("ScenarioModule::setVolumeOverride: changing volume override of %s by %d -> %d",
        getCategory(), (override ? 1 : -1), mVolumeOverride);

    if (mVolumeOverride < 0)
    {
        g_warning ("%s: volume override mismatch detected for %s. Setting to 0.",
                                                 __FUNCTION__, getCategory());
        mVolumeOverride = 0;
    }

    return true;
}

bool
GenericScenarioModule::registerMe (LSMethod *methods)
{
    bool result;
    CLSError lserror;

    result = ServiceRegisterCategory (mCategory, methods, nullptr, this);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, mCategory.c_str());
        return false;
    }

    return true;
}

bool
_volumeUp(LSHandle *sh, LSMessage *message, void *ctx)
{
    const char *reply = STANDARD_JSON_SUCCESS;
    CLSError lserror;
    pbnjson::JValue msgFirst;
    int volume = -1;

    LSMessageJsonParser msgParse(message, SCHEMA_ANY);
    ScenarioModule *module = (ScenarioModule *) ctx;

    // add LSMessageIsSubscription for not support subscribe parameter for volumeUp, jinyo 2015-05-28
    if ((!msgParse.parse(cModuleMethod_VolumeUp)) && (LSMessageIsSubscription(message)))
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_PARAMETER_BE_EMPTY, "Parameters should be empty");
        goto error;
    }

    if (!module->getScenarioVolumeOrMicGain(module->getCurrentScenarioName(),
                                            volume, true))
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_SCENARIO_NOT_FOUND, "Scenario not found");
        goto error;
    }


    module->setMuted(false);
    module->setVolumeUpDown(true);

error:

    if (!LSMessageReply(sh, message, reply, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
    }

    return true;
}

bool
_volumeDown(LSHandle *sh, LSMessage *message, void *ctx)
{
    const char *reply = STANDARD_JSON_SUCCESS;
    CLSError lserror;
    pbnjson::JValue msgFirst;
    int volume = -1;

    LSMessageJsonParser msgParse(message, SCHEMA_ANY);
    ScenarioModule *module = (ScenarioModule *) ctx;

    // add LSMessageIsSubscription for not support subscribe parameter for volumeDown, jinyo 2015-05-28
    if ((!msgParse.parse(cModuleMethod_VolumeDown)) || (LSMessageIsSubscription(message)))
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_PARAMETER_BE_EMPTY, "Parameters should be empty");
        goto error;
    }

    if (!module->getScenarioVolumeOrMicGain(module->getCurrentScenarioName(),
                                            volume, true))
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_SCENARIO_NOT_FOUND, "Scenario not found");
        goto error;
    }


    module->setMuted(false);
    module->setVolumeUpDown(false);

error:

    if (!LSMessageReply(sh, message, reply, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
    }

    return true;
}

bool _getSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    CLSError lserror;
    bool subscribed = false;

    if (!VERIFY(ctx != 0 && message != 0))
    {
        return false;
    }

    ScenarioModule *module = (ScenarioModule *)ctx;

    if (LSMessageIsSubscription(message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            return false;
        }
        else
        {
            CHECK(module->sendChangedUpdate(NOTIFY_SOUNDOUT));
        }
    }
    else
    {
        return _getVolumeOrMicGain(lshandle, message, ctx, true);
    }

    return true;
}

