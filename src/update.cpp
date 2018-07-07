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

#include "module.h"
#include "state.h"
#include "update.h"
#include "messageUtils.h"
#include "main.h"
#include "AudioDevice.h"
#include "genericScenarioModule.h"

bool GenericScenarioModule::subscriptionPost(LSHandle * palmService, std::string replyString,ESendUpdate update)
{
    bool result = true;
    CLSError lserror;
    std::string key;
    if (mCategory == "/")
        key = mCategory;
    else
        key = mCategory + "/";
    g_message("subscriptionPost");
    switch(update)
    {
       case eUpdate_Status:
           key += cModuleMethod_Status;
       break;
       case eUpdate_GetVolume:
           key += cModuleMethod_GetVolume;
       break;
       case eUpdate_GetSoundOut:
           key += cModuleMethod_GetSoundOut;
       break;
       default :
           g_message("SubscriptionPost failed:No updates");
           return result;
    }

    result = LSSubscriptionReply(palmService, key.c_str(), replyString.c_str(), &lserror);

    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        return false;
    }

    return result;
}

bool
GenericScenarioModule::sendChangedUpdate(int changedFlags, const gchar * broadCastEvent)
{
    g_message("sendChangedUpdate");
    pbnjson::JValue reply = pbnjson::Object();
    bool ringtoneWithVibration = false;

    gState.getPreference(cPref_RingtoneWithVibration, ringtoneWithVibration);

    reply.put("returnValue", true);

    if (changedFlags)
    {
        if (changedFlags & UPDATE_DISABLED)
        {
            reply.put("action", "disabled");
        }
        else
        {
            reply.put("action", "changed");
        }

        pbnjson::JValue array = pbnjson::Array();

        if (changedFlags & UPDATE_CHANGED_SCENARIO)
            array << pbnjson::JValue("scenario");

        if (changedFlags & UPDATE_CHANGED_VOLUME)
            array << pbnjson::JValue("volume");

        if (changedFlags & UPDATE_CHANGED_MICGAIN)
            array << pbnjson::JValue("mic_gain");

        if (changedFlags & UPDATE_CHANGED_ACTIVE)
            array << pbnjson::JValue("active");

        if (changedFlags & UPDATE_CHANGED_RINGER)
            array << pbnjson::JValue("ringer switch");

        if (changedFlags & UPDATE_CHANGED_SLIDER)
            array << pbnjson::JValue("slider");

        if (changedFlags & UPDATE_CHANGED_MUTED)
            array << pbnjson::JValue("muted");

        if (changedFlags & UPDATE_BROADCAST_EVENT)
            array << pbnjson::JValue("event");

        if (changedFlags & UPDATE_CHANGED_HAC)
            array << pbnjson::JValue("hac");

        if (changedFlags & UPDATE_RINGTONE_WITH_VIBRATION)
            array << pbnjson::JValue("ringtonewithvibration");

        if (changedFlags & NOTIFY_SOUNDOUT)
        {
           array << pbnjson::JValue("notify");
        }
        reply.put("changed", array);
    }

    if (mCurrentScenario)
    {
        reply.put("scenario", mCurrentScenario->getName());
        reply.put("volume", mCurrentScenario->getVolume());
        if (mCurrentScenario->hasMicGain())
            reply.put("mic_gain", mCurrentScenario->getMicGain());
        if (changedFlags & CAUSE_VOLUME_UP)
        {
            reply.put("cause", "volumeUp");
        }
        else if (changedFlags & CAUSE_VOLUME_DOWN)
        {
            reply.put("cause", "volumeDown");
        }
        else if (changedFlags & CAUSE_SET_VOLUME)
        {
            reply.put("cause", "setVolume");
        }
    }
    else
    {
        reply.put("scenario", "none");
        reply.put("volume", "undefined");
        reply.put("mic_gain", "undefined");
    }

    reply.put("active", isCurrentModule());
    reply.put("ringer switch", gState.getRingerOn());
    reply.put("muted", mMuted);
    reply.put("slider", (bool)(gState.getSliderState() == eSlider_Open));
    reply.put("hac", gState.hacGet());
    reply.put("ringtonewithvibration", ringtoneWithVibration);

    if (changedFlags & UPDATE_BROADCAST_EVENT && broadCastEvent)
    {
        reply.put("event", broadCastEvent);
    }

    std::string        replyString = jsonToString(reply);
    g_debug("ScenarioModule::sendChangedUpdate: %s", replyString.c_str());

    CLSError lserror;
    bool result = true;
    ESendUpdate update ;

   
    update = eUpdate_Status;
    result = subscriptionPost(GetPalmService(),replyString,update);
    if (!result)
    {
        return false;
    }

    return result;
}

bool
GenericScenarioModule::sendRequestedUpdate(LSHandle *sh, LSMessage *message, bool subscribed)
{
    g_message("sendRequestedUpdate") ;
    pbnjson::JValue reply = pbnjson::Object();
    bool ringtoneWithVibration = false;

    gState.getPreference(cPref_RingtoneWithVibration, ringtoneWithVibration);

    reply.put("returnValue", true);

    reply.put("action", "requested");

    if (mCurrentScenario)
    {
        reply.put("scenario", mCurrentScenario->getName());
        reply.put("volume", mCurrentScenario->getVolume());
        if (mCurrentScenario->hasMicGain())
            reply.put("mic_gain", mCurrentScenario->getMicGain());
    }
    else
    {
        reply.put("scenario", "none");
        reply.put("volume", "undefined");
        reply.put("mic_gain", "undefined");
    }

    reply.put("active", isCurrentModule());
    reply.put("ringer switch", gState.getRingerOn());
    reply.put("muted", mMuted);
    reply.put("slider", (bool)(gState.getSliderState() == eSlider_Open));
    reply.put("hac", gState.hacGet());
    reply.put("ringtonewithvibration", ringtoneWithVibration);

    reply.put("subscribed", subscribed);

    std::string        replyString = jsonToString(reply);
    g_debug("ScenarioModule::sendRequestedUpdate: %s", replyString.c_str());

    CLSError lserror;
    bool result = LSMessageReply(sh, message, replyString.c_str(), &lserror);
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);

    return result;
}

bool
GenericScenarioModule::sendEnabledUpdate(const char *scenario, int enabledFlags)
{
    g_message("sendEnabledUpdate");
    pbnjson::JValue reply = pbnjson::Object();

    reply.put("returnValue", true);

    const gchar * action = "undefined";
    if (enabledFlags == UPDATE_ENABLED_SCENARIO)
        action = "enabled";
    else if (enabledFlags == UPDATE_DISABLED_SCENARIO)
        action ="disabled";
    reply.put("action", action);

    reply.put("scenario", scenario);

    std::string        replyString = jsonToString(reply);
    g_debug("ScenarioModule::sendEnabledUpdate: %s", replyString.c_str());

    bool result = true;
    CLSError lserror;
    ESendUpdate update ;

    update = eUpdate_Status;
    result = subscriptionPost(GetPalmService(),replyString,update);
    if (!result)
    {
        return false;
    }

    return result;
}
