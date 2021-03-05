// Copyright (c) 2021 LG Electronics, Inc.
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

#include "OSEMasterVolumeManager.h"

bool OSEMasterVolumeManager::mIsObjRegistered = OSEMasterVolumeManager::CreateInstance();

OSEMasterVolumeManager* OSEMasterVolumeManager::getInstance()
{
    static OSEMasterVolumeManager objOSEMasterVolumeManager;
    return &objOSEMasterVolumeManager;
}

OSEMasterVolumeManager::OSEMasterVolumeManager()
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager constructor");
}

OSEMasterVolumeManager::~OSEMasterVolumeManager()
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager destructor");
}

void OSEMasterVolumeManager::setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: setVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(volume, integer), PROP(sessionId, integer)) REQUIRED_2(soundOutput, volume)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    bool status = false;
    std::string soundOutput;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int displayId = -1;
    int volume = MIN_VOLUME;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);
    msg.get("sessionId", display);

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;
    else
    {
        PM_LOG_ERROR ("OSEMasterVolumeManager", INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "SetMasterVolume with soundout: %s volume: %d display: %d", \
                soundOutput.c_str(), volume, displayId);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    if (DISPLAY_TWO == display)
    {
        if (soundOutput != "alsa")
        {
            PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
        else if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            displayTwoVolume = volume;
            std::string callerId = LSMessageGetSenderServiceName(message);
            notifyVolumeSubscriber(displayId, callerId);
            pbnjson::JValue setVolumeResponse = pbnjson::Object();
            setVolumeResponse.put("returnValue", true);
            setVolumeResponse.put("volume", volume);
            setVolumeResponse.put("soundOutput", soundOutput);
            reply = setVolumeResponse.stringify();
        }
        else
        {
            PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
        }

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else if (DISPLAY_ONE == display)
    {
        if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            displayOneVolume = volume;
            std::string callerId = LSMessageGetSenderServiceName(message);
            notifyVolumeSubscriber(displayId, callerId);
            status = true;
        }
        else
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
        envelopeRef *envelope = new (std::nothrow)envelopeRef;
        if (nullptr != envelope)
        {
            envelope->message = message;
            envelope->context = (OSEMasterVolumeManager*)ctx;
            OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)ctx;

            if ((nullptr != audioMixerObj) && (isValidVolume))
            {
                if(audioMixerObj->setMasterVolume(soundOutput, volume, _setVolumeCallBack, envelope))
                {
                    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: SetMasterVolume umimixer call successfull");
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    status = false;
                    PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: SetMasterVolume umimixer call failed");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
                }
            }
            else
            {
                status = false;
                PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
            }
        }
        else
        {
            status = false;
            PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: SetMasterVolume envelope is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        }
        if (false == status)
        {
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
            if (nullptr != envelope)
            {
                delete envelope;
                envelope = nullptr;
            }
        }
    }
    return;
}

bool OSEMasterVolumeManager::_setVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: setVolumeCallBack");
    std::string payload = LSMessageGetPayload(reply);
    JsonMessageParser ret(payload.c_str(), NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean)) REQUIRED_1(returnValue)));
    bool returnValue = false;
    if (ret.parse(__FUNCTION__))
    {
        ret.get("returnValue", returnValue);
    }
    std::string soundOutput;
    int iVolume = 0;
    if (returnValue)
    {
        JsonMessageParser data(payload.c_str(), NORMAL_SCHEMA(PROPS_2(PROP(volume, integer),\
            PROP(soundOutput, string)) REQUIRED_2(soundOutput, volume)));
        if (data.parse(__FUNCTION__))
        {
            data.get("soundOutput", soundOutput);
            data.get("volume", iVolume);
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume::Successfully Set the speaker volume for sound out %s with volume %d", \
                        soundOutput.c_str(), iVolume);
        }
        else
        {
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: Could not SetMasterVolume");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;
        if (true == returnValue)
        {
            if (nullptr != OSEMasterVolumeManagerObj)
            {
                OSEMasterVolumeManagerObj->setCurrentVolume(iVolume);
            }
        }
        if (nullptr != message)
        {
            CLSError lserror;
            if (!LSMessageRespond(message, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
            LSMessageUnref(message);
        }
        else
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: getVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(subscribe, boolean), PROP(sessionId, integer))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("subscribe", subscribed);
    msg.get("sessionId", display);

    if (LSMessageIsSubscription(message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return;
        }
    }

    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)ctx;
    if (nullptr != OSEMasterVolumeManagerObj)
    {
        int displayId = DEFAULT_ONE_DISPLAY_ID;
        std::string callerId = LSMessageGetSenderServiceName(message);
        if (DISPLAY_TWO == display)
            displayId = DEFAULT_TWO_DISPLAY_ID;
        else if (DISPLAY_ONE == display)
            displayId = DEFAULT_ONE_DISPLAY_ID;
        else
        {
            PM_LOG_ERROR ("OSEMasterVolumeManager", INIT_KVCOUNT, \
                        "sessionId Not in Range");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        }

        if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
            reply = OSEMasterVolumeManagerObj->getVolumeInfo(displayId, callerId);
    }
    else
    {
        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeManagerObj is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
    }

    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "%s : Reply:%s", reply.c_str(), __FUNCTION__);
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return;
}

void OSEMasterVolumeManager::muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: muteVolume");
}

void OSEMasterVolumeManager::volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: volumeUp");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundOutput;
    bool status = false;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int volume = MIN_VOLUME;
    int displayId = -1;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = 2;
    else if (DISPLAY_ONE == display)
        displayId = 1;
    else
    {
        PM_LOG_ERROR ("OSEMasterVolumeManager", INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: volumeUp with soundout: %s", soundOutput.c_str());
    std::string callerId = LSMessageGetSenderServiceName(message);
    AudioMixer *audioMixerInstance = AudioMixer::getAudioMixerInstance();
    if (audioMixerInstance)
    {
        if (DISPLAY_TWO == display)
        {
            if ((displayTwoVolume+1) <= MAX_VOLUME)
            {
                isValidVolume = true;
                volume = displayTwoVolume+1;
            }
            else
                PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "Volume up value not in range");
            if ((isValidVolume) && (audioMixerInstance->setVolume(displayId, volume)))
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                ++(displayTwoVolume);
                notifyVolumeSubscriber(displayId, callerId);
                pbnjson::JValue setVolumeResponse = pbnjson::Object();
                setVolumeResponse.put("returnValue", true);
                setVolumeResponse.put("volume", volume);
                setVolumeResponse.put("soundOutput", soundOutput);
                reply = setVolumeResponse.stringify();
            }
            else
            {
                PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
        else if (DISPLAY_ONE == display)
        {
            if ((displayOneVolume+1) <= MAX_VOLUME)
            {
                isValidVolume = true;
                volume = displayOneVolume+1;
            }
            else
            {
                PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "Volume up value not in range");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }
            if (isValidVolume && (audioMixerInstance->setVolume(displayId, volume)))
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                ++(displayOneVolume);
                notifyVolumeSubscriber(displayId, callerId);
            }
            envelopeRef *envelope = new (std::nothrow)envelopeRef;
            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = (OSEMasterVolumeManager*)ctx;
                if (isValidVolume)
                {
                    if (audioMixerInstance->masterVolumeUp(soundOutput, _volumeUpCallBack, envelope))
                    {
                        PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeUp umimixer call successfull");
                        LSMessageRef(message);
                        status = true;
                    }
                    else
                    {
                        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeUp umimixer call failed");
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
                    }
                }
                else
                {
                    PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_PARAMETER_BE_EMPTY, "Internal error");
                }
            }
            else
            {
                PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeUp envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
            if (false == status)
            {
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                {
                    lserror.Print(__FUNCTION__, __LINE__);
                }
                if (nullptr != envelope)
                {
                    delete envelope;
                    envelope = nullptr;
                }
            }
        }
    }
    else
    {
        PM_LOG_ERROR ("OSEMasterVolumeManager", INIT_KVCOUNT, \
            "audiomixer instance is nulptr");
    }
    return;
}

bool OSEMasterVolumeManager::_volumeUpCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: volumeUpCallBack");
    std::string payload = LSMessageGetPayload(reply);
    JsonMessageParser ret(payload.c_str(), NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean)) REQUIRED_1(returnValue)));
    bool returnValue = false;
    if (ret.parse(__FUNCTION__))
    {
        ret.get("returnValue", returnValue);
    }
    std::string soundOutput;
    int iVolume = 0;
    if (returnValue)
    {
        JsonMessageParser data(payload.c_str(), NORMAL_SCHEMA(PROPS_2(PROP(volume, integer),\
            PROP(soundOutput, string)) REQUIRED_2(soundOutput, volume)));
        if (data.parse(__FUNCTION__))
        {
            data.get("soundOutput", soundOutput);
            data.get("volume", iVolume);
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, \
                       "MasterVolume::Successfully increased the speaker volume for sound out %s with volume %d", \
                       soundOutput.c_str(), iVolume);
        }
        else
        {
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: Could not volume up");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;
        if (true == returnValue)
        {
            if (nullptr != OSEMasterVolumeManagerObj)
            {
                OSEMasterVolumeManagerObj->setCurrentVolume(iVolume);
            }
        }
        if (nullptr != message)
        {
            CLSError lserror;
            if (!LSMessageRespond(message, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
            LSMessageUnref(message);
        }
        else
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: volumeDown");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundOutput;
    bool status = false;
    int displayId = -1;
    bool isValidVolume = false;
    int volume = MIN_VOLUME;
    int display = DISPLAY_ONE;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = 2;
    else if (DISPLAY_ONE == display)
        displayId = 1;
    else
    {
        PM_LOG_ERROR ("OSEMasterVolumeManager", INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: volumeDown with soundout: %s", soundOutput.c_str());
    std::string callerId = LSMessageGetSenderServiceName(message);
    AudioMixer *audioMixerInstance = AudioMixer::getAudioMixerInstance();
    if (audioMixerInstance)
    {
        if (DISPLAY_TWO == display)
        {
            if ((displayTwoVolume-1) >= MIN_VOLUME)
            {
                isValidVolume = true;
                volume = displayTwoVolume-1;
            }
            else
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "Volume down value not in range");
            if ((isValidVolume) && (audioMixerInstance->setVolume(displayId, volume)))
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                --(displayTwoVolume);
                notifyVolumeSubscriber(displayId, callerId);
                pbnjson::JValue setVolumeResponse = pbnjson::Object();
                setVolumeResponse.put("returnValue", true);
                setVolumeResponse.put("volume", volume);
                setVolumeResponse.put("soundOutput", soundOutput);
                reply = setVolumeResponse.stringify();
            }
            else
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
        else if (DISPLAY_ONE == display)
        {
            if ((displayOneVolume-1) >= MIN_VOLUME)
            {
                isValidVolume = true;
                volume = displayOneVolume-1;
            }
            else
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "Volume down value not in range");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }
            if ((isValidVolume) && (audioMixerInstance->setVolume(displayId, volume)))
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                --(displayOneVolume);
                notifyVolumeSubscriber(displayId, callerId);
            }
            envelopeRef *envelope = new (std::nothrow)envelopeRef;
            if (nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = (OSEMasterVolumeManager*)ctx;
                if (isValidVolume)
                {
                    if (audioMixerInstance->masterVolumeDown(soundOutput, _volumeDownCallBack, envelope))
                    {
                        PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeDown umimixer call successfull");
                        LSMessageRef(message);
                        status = true;
                    }
                    else
                    {
                        PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeDown umimixer call failed");
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
                    }
                }
                else
                {
                    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
                }
            }
            else
            {
                PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: masterVolumeDown envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
            if (false == status)
            {
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                {
                    lserror.Print(__FUNCTION__, __LINE__);
                }
                if (nullptr != envelope)
                {
                    delete envelope;
                    envelope = nullptr;
                }
            }
        }
    }
    else
    {
        PM_LOG_ERROR ("OSEMasterVolumeManager", INIT_KVCOUNT, \
            "audiomixer instance is nulptr");
    }
    return;
}

bool OSEMasterVolumeManager::_volumeDownCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: volumeDownCallBack");
    std::string payload = LSMessageGetPayload(reply);
    JsonMessageParser ret(payload.c_str(), NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean)) REQUIRED_1(returnValue)));
    bool returnValue = false;
    if (ret.parse(__FUNCTION__))
    {
        ret.get("returnValue", returnValue);
    }
    std::string soundOutput;
    int iVolume = 0;
    if (returnValue)
    {
        JsonMessageParser data(payload.c_str(), NORMAL_SCHEMA(PROPS_2(PROP(volume, integer),\
            PROP(soundOutput, string)) REQUIRED_2(soundOutput, volume)));
        if (data.parse(__FUNCTION__))
        {
            data.get("soundOutput", soundOutput);
            data.get("volume", iVolume);
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, \
                        "MasterVolume::Successfully decreased the speaker volume for sound out %s with volume %d", \
                        soundOutput.c_str(), iVolume);
        }
        else
        {
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: Could not volume up");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;
        if(true == returnValue)
        {
            if (nullptr != OSEMasterVolumeManagerObj)
            {
                OSEMasterVolumeManagerObj->setCurrentVolume(iVolume);
            }
        }
        if (nullptr != message)
        {
            CLSError lserror;
            if (!LSMessageRespond(message, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
            LSMessageUnref(message);
        }
        else
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::notifyVolumeSubscriber(const int &displayId, const std::string &callerId)
{
    CLSError lserror;
    std::string reply = getVolumeInfo(displayId, callerId);
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR("OSEMasterVolumeManager", INIT_KVCOUNT, "Notify error");
    }
}

void OSEMasterVolumeManager::setCurrentVolume(int iVolume)
{
    mVolume = iVolume;
    PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "MasterVolume::updated volume: %d ", mVolume);
}

std::string OSEMasterVolumeManager::getVolumeInfo(const int &displayId, const std::string &callerId)
{
    pbnjson::JValue soundOutInfo = pbnjson::Object();
    pbnjson::JValue volumeStatus = pbnjson::Object();
    int volume = MIN_VOLUME;
    bool muteStatus = false;
    int display = DISPLAY_ONE;
    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        volume = displayOneVolume;
        muteStatus = displayOneMuteStatus;
        display = DISPLAY_ONE;
    }
    else
    {
        volume = displayTwoVolume;
        muteStatus = displayTwoMuteStatus;
        display = DISPLAY_TWO;
    }

    volumeStatus = {{"muted", muteStatus},
                    {"volume", volume},
                    {"soundOutput", "alsa"},
                    {"sessionId", display}};

    soundOutInfo.put("volumeStatus", volumeStatus);
    soundOutInfo.put("returnValue", true);
    soundOutInfo.put("callerId", callerId);

    return soundOutInfo.stringify();
}