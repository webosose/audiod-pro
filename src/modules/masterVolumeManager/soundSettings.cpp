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

#include "soundSettings.h"

bool soundSettings::_SetSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(soundOut, string)) REQUIRED_1(soundOut)));

    if (!msg.parse(__FUNCTION__, lshandle))
    {
        return true;
    }
    std::string soundOut;
    msg.get("soundOut", soundOut);
    envelopeRef *envelopeObj = new (std::nothrow)envelopeRef;
    bool status = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    if(nullptr != envelopeObj)
    {
        envelopeObj->message = message;
        envelopeObj->context = (soundSettings*)ctx;
        soundSettings *soundSettingsObj = (soundSettings*)ctx;
        if(nullptr!=soundSettingsObj->mObjAudioMixer)
        {
            if(soundSettingsObj->mObjAudioMixer->setSoundOut(soundOut,_updateSoundOutStatus,envelopeObj))
            {
                g_debug("SoundSettings: SetSoundOut Successful");
                LSMessageRef(message);
                status = true;
            }
            else
            {
                g_debug("Sound Settings: SetSoundOut call failed");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
            }
        }
        else
        {
            g_debug("Sound Settings :mObjAudioMixer is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if(status == false)
    {
        CLSError lserror;
        if(!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if(nullptr != envelopeObj)
        {
            delete envelopeObj;
            envelopeObj = nullptr;
        }
    }

    return true;
}

bool soundSettings::_updateSoundOutStatus(LSHandle *sh, LSMessage *reply, void *ctx)
{
    g_debug("_updateSoundOutStatus Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                                                 REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;

    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        g_debug("%s:%d no update. Could not updateSoundOutStatus", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData(reply, STRICT_SCHEMA(PROPS_2(PROP(returnValue, boolean),
        PROP(soundOut, string)) REQUIRED_2(returnValue, soundOut)));
        std::string l_strSoundOut;
        msgData.get("soundOut", l_strSoundOut);
        g_debug("Soutoutmode set Successfully for sound out %s",l_strSoundOut.c_str());
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelopeObj = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelopeObj->message;
        soundSettings* soundSettingsObj = (soundSettings*)envelopeObj->context;

        if(nullptr != message)
        {
            CLSError lserror;
            std::string payload = LSMessageGetPayload(reply);
            if (!LSMessageRespond(message, payload.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
            LSMessageUnref(message);
        }

        if(nullptr != envelopeObj)
        {
            delete envelopeObj;
            envelopeObj = nullptr;
        }
    }
    g_debug("_updateSoundOutStatus Done");
    return returnValue;
}

static LSMethod soundSettingsMethods[] = {
    {"setSoundOut", soundSettings::_SetSoundOut},
    { },
};

soundSettings::soundSettings():mObjAudioMixer(AudioMixer::getAudioMixerInstance()),soundMode("")
{
   g_debug("soundSettings: constructor");
}
soundSettings::~soundSettings()
{
  g_debug("soundSettings: destructor");
}

soundSettings* soundSettings::getSoundSettingsInstance()
{
    static soundSettings soundSettingsInstance;
    return &soundSettingsInstance;
}

int SettingsInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    CLSError lserror;
    bool result = false;
    LSErrorInit(&lserror);
    soundSettings * soundSettingsInstance = soundSettings::getSoundSettingsInstance();

    if(soundSettingsInstance)
    {
        result = LSRegisterCategoryAppend(handle, "/soundSettings", soundSettingsMethods, nullptr, &lserror);
        if (!result || !LSCategorySetData(handle, "/soundSettings", soundSettingsInstance, &lserror))
        {
            g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/soundSettings");
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            return (-1);
        }
    }
    else
    {
        g_debug("soundSettings Init Failed");
        return (-1);
    }

    g_debug("soundSettings Done");
    return 0;
}

MODULE_START_FUNC (SettingsInterfaceInit);
