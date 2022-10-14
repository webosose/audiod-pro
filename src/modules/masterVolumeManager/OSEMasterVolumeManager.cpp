// Copyright (c) 2021-2022 LG Electronics, Inc.
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

bool OSEMasterVolumeManager::mIsObjRegistered = OSEMasterVolumeManager::RegisterObject();
OSEMasterVolumeManager::OSEMasterVolumeManager():mVolume(0), mMuteStatus(false), \
                                                 displayOneVolume(100), displayTwoVolume(100),displayOneMicMuteStatus(false), displayTwoMicMuteStatus(false),\
                                                 displayOneMicVolume(100), displayTwoMicVolume(100), displayOneMuteStatus(0), displayTwoMuteStatus(0)
{
    PM_LOG_DEBUG("OSEMasterVolumeManager constructor");
}

OSEMasterVolumeManager::~OSEMasterVolumeManager()
{
    PM_LOG_DEBUG("OSEMasterVolumeManager destructor");
}

void OSEMasterVolumeManager::setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: setVolume");
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
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                      "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMasterVolume with soundout: %s volume: %d display: %d", \
                soundOutput.c_str(), volume, displayId);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMasterVolume envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    if (DISPLAY_TWO == display)
    {
        std::string activeDevice = getDisplaySoundOutput(DISPLAY_TWO);
        PM_LOG_DEBUG("active soundoutput = %s", activeDevice.c_str());
        if (soundOutput != "alsa")
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
        else if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _setVolumeCallBackPA)))
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            LSMessageRef(message);
            status = true;
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
        }
    }
    else if (DISPLAY_ONE == display)
    {
        std::string activeDevice = getDisplaySoundOutput(DISPLAY_ONE);
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display 1 = %s", activeDevice.c_str());
        if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _setVolumeCallBackPA)))
        {
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            LSMessageRef(message);
            status = true;
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
        }
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
    return;
}

void OSEMasterVolumeManager::setMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: setMicVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(volume, integer), PROP(displayId, integer)) REQUIRED_1(volume)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    bool status = false;
    std::string soundInput;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int displayId = -1;
    int volume = MIN_VOLUME;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("volume", volume);
    msg.get("displayId", display);

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "displayId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMicVolume with soundin: %s volume: %d display: %d", \
                soundInput.c_str(), volume, displayId);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if (DISPLAY_ONE == display)
    {
        std::string activeDevice = getDisplaySoundInput(DISPLAY_ONE);
        if (activeDevice == "")
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundInput");
            if (isValidVolume)
            {
                displayOneMicVolume = volume;
                std::string callerId = LSMessageGetSenderServiceName(message);
                pbnjson::JValue setVolumeResponse = pbnjson::Object();
                setVolumeResponse.put("returnValue", true);
                setVolumeResponse.put("volume", volume);
                setVolumeResponse.put("soundInput", "pcm_input");
                notifyMicVolumeSubscriber(displayId, true);
                reply = setVolumeResponse.stringify();
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mic volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundInput volume is not in range");
            }
        }
        else
        {
            PM_LOG_DEBUG("active soundInput  = %s is available", activeDevice.c_str());

            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = this;

                soundInput = activeDevice;
                if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setMicVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _setMicVolumeCallBackPA)))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set Mic volume %d for display: %d", volume, displayId);
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mic volume %d for display: %d", volume, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundInput volume is not in range");
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setMicVolume envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
        }
    }
    else if((DISPLAY_TWO == display))
    {
        std::string activeDevice = getDisplaySoundInput(DISPLAY_TWO);
        if (activeDevice == "")
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundInput");
            if (isValidVolume)
            {
                displayTwoMicVolume = volume;
                pbnjson::JValue setVolumeResponse = pbnjson::Object();
                setVolumeResponse.put("returnValue", true);
                setVolumeResponse.put("volume", volume);
                setVolumeResponse.put("soundInput", "pcm_input");
                notifyMicVolumeSubscriber(displayId, true);
                reply = setVolumeResponse.stringify();
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mic volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundInput volume is not in range");
            }
        }
        else {
            PM_LOG_DEBUG("active soundInput  = %s is available", activeDevice.c_str());

            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = this;

                soundInput = activeDevice;
                if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setMicVolume(activeDevice.c_str(), volume, lshandle, message, (void*)envelope, _setMicVolumeCallBackPA)))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set Mic volume %d for display: %d", volume, displayId);
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mic volume %d for display: %d", volume, displayId);
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundInput volume is not in range");
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setMicVolume envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: SetMicVolume displayID is not valid");
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

    return;
}

bool OSEMasterVolumeManager::_setMicVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "_setMicVolumeCallBackPA");

    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_2(PROP(volume, integer), PROP(displayId, integer)) REQUIRED_1(volume)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    int display = DISPLAY_ONE;
    int volume = MIN_VOLUME;
    int displayId = -1;

    msg.get("volume", volume);
    msg.get("displayId", display);

    envelopeRef *envelope = (envelopeRef*)ctx;
    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string activeDevice = OSEMasterVolumeManagerObj->getDisplaySoundInput(display);

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", status);
    responseObj.put("volume", volume);
    responseObj.put("soundInput", activeDevice);
    std::string payload = responseObj.stringify();

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA::Successfully set the mic volume");
        if (DISPLAY_ONE == display)
            OSEMasterVolumeManagerObj->displayOneMicVolume = volume;
        else
            OSEMasterVolumeManagerObj->displayTwoMicVolume = volume;
        OSEMasterVolumeManagerObj->notifyMicVolumeSubscriber(displayId, true);
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: Could not set mic volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }
    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: context is null");
    }
    return true;
}

bool OSEMasterVolumeManager::_setVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: _setVolumeCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(volume, integer), PROP(sessionId, integer)) REQUIRED_2(soundOutput, volume)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    std::string soundOutput;
    int volume = MIN_VOLUME;
    int displayId = -1;
    int display = DISPLAY_ONE;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else
        displayId = DEFAULT_ONE_DISPLAY_ID;

    envelopeRef *envelope = (envelopeRef*)ctx;
    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("volume", volume);
    setVolumeResponse.put("soundOutput", soundOutput);
    std::string payload = setVolumeResponse.stringify();

    std::string callerId = LSMessageGetSenderServiceName(reply);

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA::Successfully set the volume");
        if (DISPLAY_TWO == display)
        {
            OSEMasterVolumeManagerObj->displayTwoVolume = volume;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(displayId, callerId);
        }
        else
        {
            OSEMasterVolumeManagerObj->displayOneVolume = volume;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(displayId, callerId);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: Could not set the volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: context is null");
    }

    return true;
}

void OSEMasterVolumeManager::getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: getVolume");
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
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return;
        }
    }

    int displayId = DEFAULT_ONE_DISPLAY_ID;
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
    }

    if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
            reply = getVolumeInfo(displayId, callerId);

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", __FUNCTION__, reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return;
}

void OSEMasterVolumeManager::getMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: getMicVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(subscribe, boolean), PROP(displayId, integer))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;

    if (!msg.get("displayId", display))
    {
        display = 0;
    }
    msg.get("subscribe", subscribed);

    if (LSMessageIsSubscription(message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return;
        }
    }

    int displayId = DEFAULT_ONE_DISPLAY_ID;
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else if (DISPLAY_ONE == display)
        displayId = DEFAULT_ONE_DISPLAY_ID;
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "displayId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");
    }

    if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
            reply = getMicVolumeInfo(displayId, subscribed);

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", __FUNCTION__, reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return;
}

void OSEMasterVolumeManager::muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: muteVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(mute, boolean), PROP(sessionId, integer)) REQUIRED_2(soundOutput, mute)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundOutput;
    bool mute = false;
    bool status = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("mute", mute);
    if (!msg.get("sessionId", display))
    {
        display = DISPLAY_ONE;
        displayId = 3;
    }
    else
    {
        if (DISPLAY_ONE == display)
            displayId = 1;
        else if (DISPLAY_TWO == display)
            displayId = 2;
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteVolume with soundout: %s mute status: %d", \
                soundOutput.c_str(),(int)mute);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string callerId = LSMessageGetSenderServiceName(message);

    envelopeRef *envelope = new (std::nothrow)envelopeRef;

    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    if (DISPLAY_TWO == display || displayId == 3 )
    {
        std::string activeDevice = getDisplaySoundOutput(DISPLAY_TWO);
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display 2 = %s", activeDevice.c_str());
        if (audioMixerObj && audioMixerObj->setMute(activeDevice.c_str(), mute, lshandle, message, envelope, _muteVolumeCallBackPA))
        {
            LSMessageRef(message);
            status = true;
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", \
                        mute, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    if (DISPLAY_ONE == display)
    {
        std::string activeDevice = getDisplaySoundOutput(DISPLAY_ONE);
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display 1 = %s", activeDevice.c_str());
        if (audioMixerObj && audioMixerObj->setMute(activeDevice.c_str(), mute, lshandle, message, envelope, _muteVolumeCallBackPA))
        {
            LSMessageRef(message);
            status = true;
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", \
                        mute, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
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
    return;
}

void OSEMasterVolumeManager::muteMic(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: muteMic");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(mute, boolean), PROP(displayId, integer)) REQUIRED_1(mute)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundInput;
    bool mute = false;
    bool status = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("mute", mute);
    if (!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
        displayId = 1;
    }
    else
    {
        if (DISPLAY_ONE == display)
            displayId = 1;
        else if (DISPLAY_TWO == display)
            displayId = 2;
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "displayId Not in Range");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "displayId Not in Range");

            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
            return;
        }
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteMic with soundin: %s mute status: %d", \
                soundInput.c_str(),(int)mute);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();

    std::string activeDevice = getDisplaySoundInput(display);

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundinput for display %d = %s", display, activeDevice.c_str());

    if (audioMixerObj && audioMixerObj->setPhysicalSourceMute(activeDevice.c_str(), mute, lshandle, message, envelope, _muteMicCallBackPA))
    {
        LSMessageRef(message);
        status = true;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mute for display: %d", displayId);
    }
    return;
}

bool OSEMasterVolumeManager::_muteMicCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: _muteMicCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_2(PROP(mute, boolean), PROP(displayId, integer)) REQUIRED_1(mute)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    bool mute = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string soundInput;

    msg.get("mute", mute);
    if (!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
        displayId = 1;
    }
    else
    {
        if (DISPLAY_ONE == display)
            displayId = 1;
        else
            displayId = 2;
    }
    envelopeRef *envelope = (envelopeRef*)ctx;
    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string callerId = LSMessageGetSenderServiceName(message);
    std::string activeDevice = OSEMasterVolumeManagerObj->getDisplaySoundInput(display);

    if (activeDevice.empty())
    {
        soundInput = "pcm_input";
    }
    else
    {
        soundInput = activeDevice;
    }

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("mute", mute);
    setVolumeResponse.put("soundInput", soundInput);
    std::string payload = setVolumeResponse.stringify();

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA::Successfully set the mute for mic");
        if (display == 0)
        {
            OSEMasterVolumeManagerObj->displayOneMicMuteStatus = mute;
        }
        else
        {
            OSEMasterVolumeManagerObj->displayTwoMicMuteStatus = mute;
        }
        OSEMasterVolumeManagerObj->notifyMicVolumeSubscriber(displayId, true);
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: Could not set the mute for mic");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: context is null");
    }
    return true;
}

bool OSEMasterVolumeManager::_muteVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "_muteVolumeCallBackPA");

    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(mute, boolean), PROP(sessionId, integer)) REQUIRED_2(soundOutput, mute)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    std::string soundOutput;
    bool mute = false;
    int displayId = DISPLAY_ONE;
    int display = -1;

    msg.get("soundOutput", soundOutput);
    msg.get("mute", mute);
    if (!msg.get("sessionId", display))
    {
        display = DISPLAY_ONE;
        displayId = 3;
    }
    else
    {
        if (DISPLAY_ONE == display)
            displayId = 1;
        else if (DISPLAY_TWO == display)
            displayId = 2;
    }

    envelopeRef *envelope = (envelopeRef*)ctx;
    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string callerId = LSMessageGetSenderServiceName(message);

    pbnjson::JValue muteVolumeResponse = pbnjson::Object();
    muteVolumeResponse.put("returnValue", status);
    muteVolumeResponse.put("mute", mute);
    muteVolumeResponse.put("soundOutput", soundOutput);
    std::string payload = muteVolumeResponse.stringify();

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA::Successfully set the mic mute");
        if (DEFAULT_ONE_DISPLAY_ID == displayId)
        {
            OSEMasterVolumeManagerObj->displayOneMuteStatus = mute;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(DEFAULT_ONE_DISPLAY_ID, callerId);
        }
        else if (DEFAULT_TWO_DISPLAY_ID == displayId)
        {
            OSEMasterVolumeManagerObj->displayTwoMuteStatus = mute;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(DEFAULT_TWO_DISPLAY_ID, callerId);
        }
        else
        {
            OSEMasterVolumeManagerObj->displayOneMuteStatus = mute;
            OSEMasterVolumeManagerObj->displayTwoMuteStatus = mute;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(DEFAULT_ONE_DISPLAY_ID, callerId);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(DEFAULT_TWO_DISPLAY_ID, callerId);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: Could not set mic mute");
    }
    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: volumeUp");
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
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp with soundout: %s", soundOutput.c_str());
    std::string callerId = LSMessageGetSenderServiceName(message);
    AudioMixer *audioMixerInstance = AudioMixer::getAudioMixerInstance();

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if(nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeUp envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

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
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }
            std::string activeDevice = getDisplaySoundOutput(DISPLAY_TWO);
            if ((isValidVolume) && (audioMixerInstance->setVolume(displayTwoSoundoutput.c_str(), volume, lshandle, message, envelope, _volumeUpCallBackPA)))
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
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
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }
            std::string activeDevice = getDisplaySoundOutput(DISPLAY_ONE);
            if (isValidVolume && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeUpCallBackPA)))
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                      "audiomixer instance is nulptr");
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
    return;
}

bool OSEMasterVolumeManager::_volumeUpCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: _volumeUpCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
        if (!msg.parse(__FUNCTION__,sh))
            return true;

    std::string soundOutput;
    int display = DISPLAY_ONE;
    int displayId = -1;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    envelopeRef *envelope = (envelopeRef*)ctx;
    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string activeDevice = OSEMasterVolumeManagerObj->getDisplaySoundInput(display);

    std::string callerId = LSMessageGetSenderServiceName(message);

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("soundOutput", soundOutput);
    std::string payload;

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA::Successfully set the volume");
        if (DISPLAY_TWO == display)
        {
            ++(OSEMasterVolumeManagerObj->displayTwoVolume);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(displayId, callerId);
            setVolumeResponse.put("volume", OSEMasterVolumeManagerObj->displayTwoVolume);
            payload = setVolumeResponse.stringify();
        }
        else
        {
            ++(OSEMasterVolumeManagerObj->displayOneVolume);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(displayId, callerId);
            setVolumeResponse.put("volume", OSEMasterVolumeManagerObj->displayOneVolume);
            payload = setVolumeResponse.stringify();
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: Could not set volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: volumeDown");
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
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = this;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "volumeDown envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown with soundout: %s", soundOutput.c_str());
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
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");

            std::string activeDevice = getDisplaySoundOutput(DISPLAY_TWO);
            if ((isValidVolume) && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeDownCallBackPA)))
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
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
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                CLSError lserror;
                if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }

            std::string activeDevice = getDisplaySoundOutput(DISPLAY_ONE);
            if ((isValidVolume) && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeDownCallBackPA)))
            {
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
            "audiomixer instance is nulptr");
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
    return;
}

bool OSEMasterVolumeManager::_volumeDownCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: _volumeDownCallBackPA");
        LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
        if (!msg.parse(__FUNCTION__,sh))
            return true;

    std::string soundOutput;
    int display = DISPLAY_ONE;
    int displayId = -1;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    envelopeRef *envelope = (envelopeRef*)ctx;
    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string callerId = LSMessageGetSenderServiceName(reply);

    pbnjson::JValue setVolumeResponse = pbnjson::Object();
    setVolumeResponse.put("returnValue", status);
    setVolumeResponse.put("soundOutput", soundOutput);
    std::string payload;

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA::Successfully set the volume");
        if (DISPLAY_TWO == display)
        {
            --(OSEMasterVolumeManagerObj->displayTwoVolume);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(displayId, callerId);
            setVolumeResponse.put("volume", OSEMasterVolumeManagerObj->displayTwoVolume);
            payload = setVolumeResponse.stringify();
        }
        else
        {
            --(OSEMasterVolumeManagerObj->displayOneVolume);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(displayId, callerId);
            setVolumeResponse.put("volume", OSEMasterVolumeManagerObj->displayOneVolume);
            payload = setVolumeResponse.stringify();
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: Could not set volume");
        payload = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (nullptr != ctx)
    {
        if (nullptr != reply)
        {
            CLSError lserror;
            if (!LSMessageRespond(reply, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: message is null");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: context is null");
    }
    return true;
}

void OSEMasterVolumeManager::notifyVolumeSubscriber(const int &displayId, const std::string &callerId)
{
    CLSError lserror;
    std::string reply = getVolumeInfo(displayId, callerId);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Notify volume subscriber error");
    }
}

void OSEMasterVolumeManager::notifyMicVolumeSubscriber(const int &displayId, bool subscribed)
{
    CLSError lserror;
    std::string reply = getMicVolumeInfo(displayId, subscribed);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_MIC_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Notify mic volume subscriber error");
    }
}

std::string OSEMasterVolumeManager::getDisplaySoundOutput(const int& display)
{
    std::string soundOutput;
    if (display == DISPLAY_ONE)
    {
        soundOutput = displayOneSoundoutput;
    }
    else if (display == DISPLAY_TWO)
    {
        soundOutput = displayTwoSoundoutput;
    }
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,
        "getDisplaySoundOutput display = %d, sound output = %s", display, soundOutput.c_str());
    return soundOutput;
}

std::string OSEMasterVolumeManager::getDisplaySoundInput(const int& display)
{
    std::string soundInput;
    if (display == DISPLAY_ONE)
    {
        soundInput = displayOneSoundinput;
    }
    else if (display == DISPLAY_TWO)
    {
        soundInput = displayTwoSoundinput;
    }
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,
        "getDisplaySoundInput display = %d, sound input = %s", display, soundInput.c_str());
    return soundInput;
}

void OSEMasterVolumeManager::setCurrentVolume(int iVolume)
{
    mVolume = iVolume;
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume::updated volume: %d ", mVolume);
}

std::string OSEMasterVolumeManager::getVolumeInfo(const int &displayId, const std::string &callerId)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getVolumeInfo");
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

std::string OSEMasterVolumeManager::getMicVolumeInfo(const int &displayId, bool subscribed)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getMicVolumeInfo");
    pbnjson::JValue soundInInfo = pbnjson::Object();
    int volume = MIN_VOLUME;
    bool muteStatus = false;
    std::string soundInput;
    std::string activeDevice;
    int display = DISPLAY_ONE;

    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        activeDevice = getDisplaySoundInput(DISPLAY_ONE);
        if (activeDevice.empty())
        {
            soundInput="pcm_input";
        }
        else
        {
            soundInput = activeDevice;
        }
        volume = displayOneMicVolume;
        muteStatus = displayOneMicMuteStatus;

        soundInInfo.put("returnValue", true);
        soundInInfo.put("subscribed", subscribed);
        soundInInfo.put("soundInput", soundInput);
        soundInInfo.put("volume", volume);
        soundInInfo.put("muteStatus", muteStatus);
    }
    else
    {
        activeDevice = getDisplaySoundInput(DISPLAY_TWO);
        if (activeDevice.empty())
        {
            soundInput="pcm_input";
        }
        else
        {
            soundInput = activeDevice;
        }
        volume = displayTwoMicVolume;
        muteStatus = displayTwoMicMuteStatus;
        soundInInfo.put("returnValue", true);
        soundInInfo.put("subscribed", subscribed);
        soundInInfo.put("soundInput", soundInput);
        soundInInfo.put("volume", volume);
        soundInInfo.put("muteStatus", muteStatus);
    }
    return soundInInfo.stringify();
}

void OSEMasterVolumeManager::setCurrentMuteStatus(bool bMuteStatus)
{
    mMuteStatus = bMuteStatus;
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,  "MasterVolume::updated mute status: %d ", (int)mMuteStatus);
}

void OSEMasterVolumeManager::setVolume(const int &displayId)
{
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    int volume = 0;
    std::string soundOutput;
    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        volume = displayOneVolume;
        soundOutput = getDisplaySoundOutput(DISPLAY_ONE);
    }
    else if (DEFAULT_TWO_DISPLAY_ID == displayId)
    {
        volume = displayTwoVolume;
        soundOutput = displayTwoSoundoutput;
    }
    if (audioMixerObj && audioMixerObj->setVolume(soundOutput.c_str(), volume, nullptr, nullptr, nullptr, nullptr))
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
    else
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
}

void OSEMasterVolumeManager::setMicVolume(const int &displayId, LSHandle *lshandle, LSMessage *message, void *ctx)
{
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    int volume = 0;
    std::string soundInput;
    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        volume = displayOneVolume;
        soundInput = getDisplaySoundInput(DISPLAY_ONE);
    }
    else if (DEFAULT_TWO_DISPLAY_ID == displayId)
    {
        volume = displayTwoVolume;
        soundInput = displayTwoSoundinput;
    }
    if (audioMixerObj && audioMixerObj->setMicVolume(soundInput.c_str(), volume, lshandle, message, ctx, _setMicVolumeCallBackPA))
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
    else
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
}

void OSEMasterVolumeManager::setDisplaySoundOutput(const std::string& display, const std::string& soundOutput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setDisplaySoundOutput %s %s", display.c_str(),soundOutput.c_str());

    if (display == DISPLAY_ONE_NAME)
    {
        displayOneSoundoutput = soundOutput;
    }
    else if (display == DISPLAY_TWO_NAME)
    {
        displayTwoSoundoutput = soundOutput;
    }
}

void OSEMasterVolumeManager::setDisplaySoundInput(const std::string& display, const std::string& soundInput)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setDisplaySoundInput %s %s", display.c_str(),soundInput.c_str());

    if (display == DISPLAY_ONE_NAME)
    {
        displayOneSoundinput = soundInput;
    }
    else if (display == DISPLAY_TWO_NAME)
    {
        displayTwoSoundinput = soundInput;
    }
}

void OSEMasterVolumeManager::setMuteStatus(const int &displayId)
{
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    if (DEFAULT_ONE_DISPLAY_ID == displayId && audioMixerObj)
    {
        audioMixerObj->setMute(displayOneSoundoutput.c_str(), displayOneMuteStatus, nullptr, nullptr, nullptr, nullptr);
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set mute status %d for display: %d", displayOneMuteStatus, displayId);
    }
    else if (DEFAULT_TWO_DISPLAY_ID == displayId && audioMixerObj)
    {
        audioMixerObj->setMute(displayTwoSoundoutput.c_str(), displayTwoMuteStatus, nullptr, nullptr, nullptr, nullptr);
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set mute status %d for display: %d", displayTwoMuteStatus, displayId);
    }
    else
    {
        if (audioMixerObj)
        {
            audioMixerObj->setMute(displayOneSoundoutput.c_str(), displayOneMuteStatus, nullptr, nullptr, nullptr, nullptr);
            audioMixerObj->setMute(displayTwoSoundoutput.c_str(), displayTwoMuteStatus, nullptr, nullptr, nullptr, nullptr);
            PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set mute status is %d for display one and mute status is %d for display two", \
                        displayOneMuteStatus, displayTwoMuteStatus);
        }
    }
}
