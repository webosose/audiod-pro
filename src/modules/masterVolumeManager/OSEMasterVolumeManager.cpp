// Copyright (c) 2021-2023 LG Electronics, Inc.
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
    requestSoundInputDeviceInfo();
    requestSoundOutputDeviceInfo();
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
    bool isSoundOutputfound =false;
    int deviceDisplay=DISPLAY_ONE;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);
    for(auto it:msoundOutputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundOutput)
            {
                isSoundOutputfound=true;
                deviceDisplay=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundOutputfound)
            break;
    }
    if(!msg.get("sessionId", display))
    {
        display = deviceDisplay;
    }
    if (DISPLAY_ONE == display){
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else if (DISPLAY_TWO == display){
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;

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
        return;
    }

    if (soundOutput != "alsa")
    {
        if(isSoundOutputfound)
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Invalid displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Invalid displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getDisplaySoundOutput(display);
                PM_LOG_DEBUG("setting volume for soundoutput = %s", soundOutput.c_str());
                PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());

                if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(soundOutput.c_str(), volume, lshandle, message, envelope, _setVolumeCallBackPA)))
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
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("setVolume for active device");
        if (DISPLAY_TWO == display)
        {
            std::string activeDevice = getDisplaySoundOutput(DISPLAY_TWO);
            PM_LOG_DEBUG("active soundoutput = %s", activeDevice.c_str());

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
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(volume, integer), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_2(soundInput,volume)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    bool status = false;
    std::string soundInput;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int displayId = -1;
    int volume = MIN_VOLUME;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundInputfound =false;
    int deviceDisplay=DISPLAY_ONE;

    msg.get("soundInput", soundInput);
    msg.get("volume", volume);
    for(auto it:msoundInputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundInput)
            {
                isSoundInputfound=true;
                deviceDisplay=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundInputfound)
            break;
    }
    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else if (DISPLAY_TWO == display)
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }
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

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMicVolume with soundin: %s volume: %d display: %d", \
        soundInput.c_str(), volume, displayId);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    envelopeRef *envelope = new (std::nothrow)envelopeRef;

    if(isSoundInputfound)
    {
        PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
        if(deviceDisplay!=display)
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Invalid displayId for the soundInput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Invalid displayId for the soundInput");
        }
        else
        {
            std::string activeDevice = getDisplaySoundInput(display);
            PM_LOG_DEBUG("setting mic volume for soundInput = %s", soundInput.c_str());
            PM_LOG_DEBUG("active soundInput now = %s", activeDevice.c_str());
            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = this;

                if ((isValidVolume) && (audioMixerObj) && (audioMixerObj->setMicVolume(activeDevice.c_str(), volume, lshandle, message, (void*)envelope, _setMicVolumeCallBackPA)))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set Mic volume %d for display:%d soundInput: %s", volume, displayId,soundInput.c_str());
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
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundInput");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
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

    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(volume, integer), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_2(soundInput,volume)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    int display = DISPLAY_ONE;
    int volume = MIN_VOLUME;
    int displayId = -1;
    std::string soundInput;
    envelopeRef *envelope = nullptr;

    msg.get("soundInput", soundInput);
    msg.get("volume", volume);
    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", status);
    responseObj.put("volume", volume);
    responseObj.put("soundInput", soundInput);
    std::string payload = responseObj.stringify();

    if (status)
    {
        PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setMicVolumeCallBackPA::Successfully set the mic volume");
        if (DISPLAY_ONE == display)
            OSEMasterVolumeManagerObj->displayOneMicVolume = volume;
        else
            OSEMasterVolumeManagerObj->displayTwoMicVolume = volume;
        OSEMasterVolumeManagerObj->notifyMicVolumeSubscriber(soundInput, displayId, true);
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
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else
        displayId = DEFAULT_ONE_DISPLAY_ID;

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_setVolumeCallBackPA: context is null");
       return true;
    }

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
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
        }
        else
        {
            OSEMasterVolumeManagerObj->displayOneVolume = volume;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
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
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    std::string soundOutput;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundOutputfound =false;

    if(msg.get("soundOutput", soundOutput))
    {
        for(auto it:msoundOutputDeviceInfo)
        {
            for(auto item:it.second)
            {
                if(item==soundOutput)
                {
                    isSoundOutputfound=true;
                    display=getDisplayId(it.first);
                    break;
                }
            }
            if(isSoundOutputfound)
                break;
        }

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
                    "sessionId Not in Range");
        reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return;
    }

    if (!soundOutput.empty())
    {
        if(isSoundOutputfound)
        {
            PM_LOG_DEBUG("Get volume info for soundoutput = %s", soundOutput.c_str());
            if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
                reply = getVolumeInfo(soundOutput, displayId, callerId);
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("Get volume info for active device");
        if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
                reply = getVolumeInfo(soundOutput, displayId, callerId);
    }

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", __FUNCTION__, reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return;
}

void OSEMasterVolumeManager::getMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: getMicVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundInput, string), PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    std::string soundInput;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundInputfound = false;

    msg.get("soundInput", soundInput);
    for(auto it:msoundInputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundInput)
            {
                isSoundInputfound=true;
                display=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundInputfound)
            break;
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

    if (!soundInput.empty())
    {
        if(isSoundInputfound)
        {
            PM_LOG_DEBUG("Get volume info for soundInput = %s", soundInput.c_str());
            if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
            reply = getMicVolumeInfo(soundInput, displayId, subscribed);
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("Get mic-volume info for active device");
        if ((DISPLAY_TWO == display) || (DISPLAY_ONE == display))
            reply = getMicVolumeInfo(soundInput, displayId, subscribed);
    }

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
    envelopeRef *envelope = nullptr;
    bool isSoundOutputfound =false;
    int deviceDisplay=DISPLAY_ONE;

    msg.get("soundOutput", soundOutput);
    msg.get("mute", mute);
    for(auto it:msoundOutputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundOutput)
            {
                isSoundOutputfound=true;
                deviceDisplay=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundOutputfound)
            break;
    }
    if(!msg.get("sessionId", display))
    {
        display = deviceDisplay;
        displayId=3;
    }
    else
    {
        if (DISPLAY_ONE == display)
        {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
        }
        else if (DISPLAY_TWO == display)
        {
            displayId = DEFAULT_TWO_DISPLAY_ID;
            display = DISPLAY_TWO;
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
                    "sessionId Not in Range");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "sessionId Not in Range");

            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
            return;
        }
    }

    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string callerId = LSMessageGetSenderServiceName(message);
    envelope = new (std::nothrow)envelopeRef;
    bool found = false;

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
        return;
    }
    if (soundOutput != "alsa")
    {
        PM_LOG_DEBUG("soundOutput :%s other than alsa found",soundOutput.c_str());
        if(isSoundOutputfound)
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getDisplaySoundOutput(display);
                PM_LOG_DEBUG("setvolume for soundoutput = %s", soundOutput.c_str());
                PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());

                if (audioMixerObj && audioMixerObj->setMute(soundOutput.c_str(), mute, lshandle, message, envelope, _muteVolumeCallBackPA))
                {
                    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteVolume with soundout: %s mute status: %d", \
                soundOutput.c_str(),(int)mute);
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
        }
        else
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
    }
    else
    {
        PM_LOG_DEBUG("muteVolume for active device");
        if (DISPLAY_TWO == display || displayId == 3 )
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getDisplaySoundOutput(DISPLAY_TWO);
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display %d = %s", displayId, activeDevice.c_str());
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
        }
        if (DISPLAY_ONE == display)
        {
            PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                std::string activeDevice = getDisplaySoundOutput(DISPLAY_ONE);
                PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "active soundoutput for display %d = %s", displayId, activeDevice.c_str());
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
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(mute, boolean), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_2(soundInput,mute)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return;
    std::string soundInput;
    bool mute = false;
    bool status = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundInputfound =false;
    int deviceDisplay=DISPLAY_ONE;

    msg.get("soundInput", soundInput);
    msg.get("mute", mute);

    for(auto it:msoundInputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundInput)
            {
                isSoundInputfound=true;
                deviceDisplay=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundInputfound)
            break;
    }

    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else if (DISPLAY_TWO == display)
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }
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


    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteMic with soundin: %s mute status: %d", \
                soundInput.c_str(),(int)mute);
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    envelopeRef *envelope = new (std::nothrow)envelopeRef;

    if(isSoundInputfound)
    {
        PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
        if(deviceDisplay!=display)
        {
            PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Invalid displayId for the soundInput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Invalid displayId for the soundInput");
        }
        else
        {
            std::string activeDevice = getDisplaySoundInput(display);
            PM_LOG_DEBUG("setting mute for soundInput = %s", soundInput.c_str());
            PM_LOG_DEBUG("active soundInput now = %s", activeDevice.c_str());
            if(nullptr != envelope)
            {
                envelope->message = message;
                envelope->context = this;
                if (audioMixerObj && audioMixerObj->setPhysicalSourceMute(soundInput.c_str(), mute, lshandle, message, envelope, _muteMicCallBackPA))
                {
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set mute for display: %d", displayId);
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteMic envelope is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundInput");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
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

bool OSEMasterVolumeManager::_muteMicCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager: _muteMicCallBackPA");
    LSMessageJsonParser msg(reply, STRICT_SCHEMA(PROPS_3(PROP(mute, boolean), PROP(displayId, integer),
    PROP(soundInput, string)) REQUIRED_2(soundInput,mute)));
    if (!msg.parse(__FUNCTION__,sh))
        return true;

    bool mute = false;
    int displayId = DISPLAY_ONE;
    int display = -1;
    std::string soundInput;
    envelopeRef *envelope = nullptr;

    msg.get("soundInput", soundInput);
    msg.get("mute", mute);
    if(!msg.get("displayId", display))
    {
        display = DISPLAY_ONE;
    }

    if (DISPLAY_ONE == display)
    {
        displayId = DEFAULT_ONE_DISPLAY_ID;
        display = DISPLAY_ONE;
    }
    else
    {
        displayId = DEFAULT_TWO_DISPLAY_ID;
        display = DISPLAY_TWO;
    }

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteMicCallBackPA: context is null");
       return true;
    }

    LSMessage *message = (LSMessage*)envelope->message;
    OSEMasterVolumeManager* OSEMasterVolumeManagerObj = (OSEMasterVolumeManager*)envelope->context;

    std::string callerId = LSMessageGetSenderServiceName(message);
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
        OSEMasterVolumeManagerObj->notifyMicVolumeSubscriber(soundInput, displayId, true);
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
    envelopeRef *envelope = nullptr;

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

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_muteVolumeCallBackPA: context is null");
       return true;
    }

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
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_ONE_DISPLAY_ID, callerId);
        }
        else if (DEFAULT_TWO_DISPLAY_ID == displayId)
        {
            OSEMasterVolumeManagerObj->displayTwoMuteStatus = mute;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_TWO_DISPLAY_ID, callerId);
        }
        else
        {
            OSEMasterVolumeManagerObj->displayOneMuteStatus = mute;
            OSEMasterVolumeManagerObj->displayTwoMuteStatus = mute;
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_ONE_DISPLAY_ID, callerId);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, DEFAULT_TWO_DISPLAY_ID, callerId);
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
    int displayVol = MIN_VOLUME;
    int displayId = -1;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool isSoundOutputfound = false;
    int deviceDisplay = DISPLAY_ONE;
    msg.get("soundOutput", soundOutput);
    for(auto it:msoundOutputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundOutput)
            {
                isSoundOutputfound=true;
                deviceDisplay=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundOutputfound)
            break;
    }
    if(!msg.get("sessionId", display))
        display = deviceDisplay;

    if (DISPLAY_TWO == display)
    {
        displayId = 2;
        display=DISPLAY_TWO;
        displayVol=displayTwoVolume;
    }
    else if (DISPLAY_ONE == display)
    {
        displayId = 1;
        display=DISPLAY_ONE;
        displayVol=displayOneVolume;
    }
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
    bool found=false;

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
        if (soundOutput != "alsa")
        {
            PM_LOG_DEBUG("soundOutput :%s other than alsa found",soundOutput.c_str());
            if(isSoundOutputfound)
            {
                PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
                if(deviceDisplay!=display)
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
                }
                else
                {
                    if ((displayVol+1) <= MAX_VOLUME)
                    {
                        isValidVolume = true;
                        volume = displayVol+1;
                        std::string activeDevice = getDisplaySoundOutput(display);
                        PM_LOG_DEBUG("volume up for soundoutput = %s", soundOutput.c_str());
                        PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());
                        if ((isValidVolume) && (audioMixerInstance->setVolume(soundOutput.c_str(), volume, lshandle, message, envelope, _volumeUpCallBackPA)))
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
                    else
                    {
                        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    }
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
            }
        }
        else
        {
            PM_LOG_DEBUG("volumeUp for active device");
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                if ((displayVol+1) <= MAX_VOLUME)
                {
                    isValidVolume = true;
                    volume = displayVol+1;
                    std::string activeDevice = getDisplaySoundOutput(display);

                    if ((isValidVolume) && (audioMixerInstance->setVolume(activeDevice.c_str(), volume, lshandle, message, envelope, _volumeUpCallBackPA)))
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
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                }
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
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeUpCallBackPA: context is null");
       return true;
    }

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
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
            setVolumeResponse.put("volume", OSEMasterVolumeManagerObj->displayTwoVolume);
            payload = setVolumeResponse.stringify();
        }
        else
        {
            ++(OSEMasterVolumeManagerObj->displayOneVolume);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
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
    int displayVol = MIN_VOLUME;
    int display = DISPLAY_ONE;
    std::string reply = STANDARD_JSON_SUCCESS;
    bool found = false;

    bool isSoundOutputfound = false;
    int deviceDisplay = DISPLAY_ONE;
    msg.get("soundOutput", soundOutput);
    for(auto it:msoundOutputDeviceInfo)
    {
        for(auto item:it.second)
        {
            if(item==soundOutput)
            {
                isSoundOutputfound=true;
                deviceDisplay=getDisplayId(it.first);
                break;
            }
        }
        if(isSoundOutputfound)
            break;
    }
    if(!msg.get("sessionId", display))
        display = deviceDisplay;

    if (DISPLAY_TWO == display)
    {
        displayId = 2;
        display=DISPLAY_TWO;
        displayVol=displayTwoVolume;
    }
    else if (DISPLAY_ONE == display)
    {
        displayId = 1;
        display=DISPLAY_ONE;
        displayVol=displayOneVolume;
    }
    else
    {
        displayId = 1;
    }

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    std::string callerId = LSMessageGetSenderServiceName(message);
    AudioMixer *audioMixerInstance = AudioMixer::getAudioMixerInstance();

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
        return;
    }
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown with soundout: %s", soundOutput.c_str());
    if (audioMixerInstance)
    {
        if (soundOutput != "alsa")
        {
            PM_LOG_DEBUG("soundOutput :%s other than alsa found",soundOutput.c_str());
            if(isSoundOutputfound)
            {
                PM_LOG_DEBUG("deviceDisplay found = %d  display got = %d", deviceDisplay,display);
                if(deviceDisplay!=display)
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
                }
                else
                {
                    if ((displayVol-1) >= MIN_VOLUME)
                    {
                        isValidVolume = true;
                        volume = displayVol-1;
                        std::string activeDevice = getDisplaySoundOutput(display);
                        PM_LOG_DEBUG("volume down for soundoutput = %s", soundOutput.c_str());
                        PM_LOG_DEBUG("active soundoutput now = %s", activeDevice.c_str());
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
                    else
                    {
                        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    }
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
            }
        }
        else
        {
            PM_LOG_DEBUG("volumeDown for active device");
            if(deviceDisplay!=display)
            {
                PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Unsupported displayId for the soundOutput");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SESSIONID, "Unsupported displayId for the soundOutput");
            }
            else
            {
                if ((displayVol-1) >= MIN_VOLUME)
                {
                    isValidVolume = true;
                    volume = displayVol-1;
                    std::string activeDevice = getDisplaySoundOutput(display);
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
                else
                {
                    PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
                    CLSError lserror;
                    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                        lserror.Print(__FUNCTION__, __LINE__);
                }
            }
        }
    }
    else
    {
        PM_LOG_ERROR (MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"audiomixer instance is nulptr");
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
    envelopeRef *envelope = nullptr;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);
    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    if (nullptr != ctx)
    {
        envelope = (envelopeRef*)ctx;
    }
    else
    {
       PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "_volumeDownCallBackPA: context is null");
       return true;
    }

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
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
            setVolumeResponse.put("volume", OSEMasterVolumeManagerObj->displayTwoVolume);
            payload = setVolumeResponse.stringify();
        }
        else
        {
            --(OSEMasterVolumeManagerObj->displayOneVolume);
            OSEMasterVolumeManagerObj->notifyVolumeSubscriber(soundOutput, displayId, callerId);
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

void OSEMasterVolumeManager::notifyVolumeSubscriber(const std::string &soundOutput, const int &displayId, const std::string &callerId)
{
    CLSError lserror;
    std::string reply = getVolumeInfo(soundOutput, displayId, callerId);
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Notify volume subscriber error");
    }
}

void OSEMasterVolumeManager::notifyMicVolumeSubscriber(const std::string &soundInput, const int &displayId, bool subscribed)
{
    CLSError lserror;
    std::string reply = getMicVolumeInfo(soundInput, displayId, subscribed);
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

std::string OSEMasterVolumeManager::getVolumeInfo(const std::string &soundOutput, const int &displayId, const std::string &callerId)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getVolumeInfo");
    pbnjson::JValue soundOutInfo = pbnjson::Object();
    pbnjson::JValue volumeStatus = pbnjson::Object();
    int volume = MIN_VOLUME;
    bool muteStatus = false;
    int display = DISPLAY_ONE;
    std::string soundDevice;
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
    if((soundOutput.empty())|| (soundOutput =="alsa"))
    {
        soundDevice = getDisplaySoundOutput(display);
        if(!mapActiveDevicesInfo[soundDevice]){
            PM_LOG_DEBUG("Setting default to pcm_output");
            soundDevice = "pcm_output";
        }
    }
    else
        soundDevice = soundOutput;
    PM_LOG_DEBUG("getVolumeInfo:: soundOutput = %s ,display: %d", soundDevice.c_str(), display);

    volumeStatus = {{"muted", muteStatus},
                    {"volume", volume},
                    {"soundOutput", soundDevice},
                    {"sessionId", display}};

    soundOutInfo.put("volumeStatus", volumeStatus);
    soundOutInfo.put("returnValue", true);
    soundOutInfo.put("callerId", callerId);
    return soundOutInfo.stringify();
}

std::string OSEMasterVolumeManager::getMicVolumeInfo(const std::string &soundInput, const int &displayId, bool subscribed)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getMicVolumeInfo");
    pbnjson::JValue soundInInfo = pbnjson::Object();
    int volume = MIN_VOLUME;
    bool muteStatus = false;
    std::string soundDevice;
    int display = DISPLAY_ONE;

    if (DEFAULT_ONE_DISPLAY_ID == displayId)
    {
        volume = displayOneMicVolume;
        muteStatus = displayOneMicMuteStatus;
        display=DISPLAY_ONE;
    }
    else
    {
        volume = displayTwoMicVolume;
        muteStatus = displayTwoMicMuteStatus;
        display=DISPLAY_TWO;
    }
    if (soundInput.empty()){
        soundDevice=getDisplaySoundInput(display);
        if(!mapActiveDevicesInfo[soundDevice]){
            PM_LOG_DEBUG("Setting default to pcm_input");
            soundDevice = "pcm_input";
        }
    }
    else
        soundDevice = soundInput;

    PM_LOG_DEBUG("getVolumeInfo:: soundInput = %s ,display: %d", soundDevice.c_str(), display);

    soundInInfo.put("returnValue", true);
    soundInInfo.put("subscribed", subscribed);
    soundInInfo.put("soundInput", soundDevice);
    soundInInfo.put("volume", volume);
    soundInInfo.put("muteStatus", muteStatus);
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

void OSEMasterVolumeManager::setDisplaySoundOutput(const std::string& display, const std::string& soundOutput, const bool& isConnected)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setDisplaySoundOutput %s %s", display.c_str(),soundOutput.c_str());
    bool isActive=false;
    if(isConnected)
    {
        isActive = true;
        if (display == DISPLAY_ONE_NAME)
        {
            displayOneSoundoutput = soundOutput;
        }
        else if (display == DISPLAY_TWO_NAME)
        {
            displayTwoSoundoutput = soundOutput;
        }
    }
    mapActiveDevicesInfo[soundOutput]=isActive;
}

void OSEMasterVolumeManager::setDisplaySoundInput(const std::string& display, const std::string& soundInput, const bool& isConnected)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "setDisplaySoundInput %s %s", display.c_str(),soundInput.c_str());
    bool isActive=false;
    if(isConnected)
    {
        isActive=true;
        if (display == DISPLAY_ONE_NAME)
        {
            displayOneSoundinput = soundInput;
        }
        else if (display == DISPLAY_TWO_NAME)
        {
            displayTwoSoundinput = soundInput;
        }
    }
    mapActiveDevicesInfo[soundInput]=isActive;
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

void OSEMasterVolumeManager::setSoundOutputInfo(utils::mapSoundDevicesInfo soundOutputInfo)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager::setSoundOutputInfo");
    msoundOutputDeviceInfo = soundOutputInfo;
}

void OSEMasterVolumeManager::setSoundInputInfo(utils::mapSoundDevicesInfo soundInputInfo)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "OSEMasterVolumeManager::setSoundInputInfo");
    msoundInputDeviceInfo = soundInputInfo;
}

int OSEMasterVolumeManager::getDisplayId(const std::string &displayName)
{
    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "getDisplayId:%s", displayName.c_str());
    int displayId = -1;

    if ("display1" == displayName)
        displayId = 0;
    else if ("display2" == displayName)
        displayId = 1;

    PM_LOG_INFO(MSGID_CLIENT_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "returning displayId:%d", displayId);
    return displayId;
}


void OSEMasterVolumeManager::eventMasterVolumeStatus()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "eventMasterVolumeStatus: setMuteStatus and setVolume");

    setMuteStatus(DEFAULT_ONE_DISPLAY_ID);
    setVolume(DEFAULT_ONE_DISPLAY_ID);
    setMuteStatus(DEFAULT_TWO_DISPLAY_ID);
    setVolume(DEFAULT_TWO_DISPLAY_ID);
}

void OSEMasterVolumeManager::eventActiveDeviceInfo(const std::string deviceName,\
    const std::string& display, const bool& isConnected, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "eventActiveDeviceInfo deviceName:%s display:%s isConnected:%d isOutput:%d", \
        deviceName.c_str(), display.c_str(), (int)isConnected, (int)isOutput);
    std::string device;
    if (isOutput)
    {
        setDisplaySoundOutput(display, deviceName,isConnected);
        if (isConnected)
        {
            setMuteStatus(DEFAULT_ONE_DISPLAY_ID);
            setVolume(DEFAULT_ONE_DISPLAY_ID);
            setMuteStatus(DEFAULT_TWO_DISPLAY_ID);
            setVolume(DEFAULT_TWO_DISPLAY_ID);
        }
    }
    else
    {
        setDisplaySoundInput(display, deviceName, isConnected);
    }
}

void OSEMasterVolumeManager::eventResponseSoundOutputDeviceInfo(utils::mapSoundDevicesInfo soundOutputInfo)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventResponseSoundOutputDeviceInfo");

    setSoundOutputInfo(soundOutputInfo);

}

void OSEMasterVolumeManager::requestSoundOutputDeviceInfo()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"requestSoundOutputDeviceInfo");

    events::EVENT_REQUEST_SOUNDOUTPUT_INFO_T stEventRequestSoundOutputDeviceInfo;
    stEventRequestSoundOutputDeviceInfo.eventName = utils::eEventRequestSoundOutputDeviceInfo;
    ModuleManager::getModuleManagerInstance()->publishModuleEvent((events::EVENTS_T*)&stEventRequestSoundOutputDeviceInfo);

}

void OSEMasterVolumeManager::requestSoundInputDeviceInfo()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"requestSoundInputDeviceInfo");

    events::EVENT_REQUEST_SOUNDOUTPUT_INFO_T stEventRequestSoundInputDeviceInfo;
    stEventRequestSoundInputDeviceInfo.eventName = utils::eEventRequestSoundInputDeviceInfo;
    ModuleManager::getModuleManagerInstance()->publishModuleEvent((events::EVENTS_T*)&stEventRequestSoundInputDeviceInfo);
}

void OSEMasterVolumeManager::eventResponseSoundInputDeviceInfo(utils::mapSoundDevicesInfo soundInputInfo)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventResponseSoundInputDeviceInfo");
    setSoundInputInfo(soundInputInfo);
}


void OSEMasterVolumeManager::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {
        case utils::eEventMasterVolumeStatus:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventMasterVolumeStatus");
            events::EVENT_MASTER_VOLUME_STATUS_T *masterVolumeStatusEvent = (events::EVENT_MASTER_VOLUME_STATUS_T*)event;
            eventMasterVolumeStatus();
        }
        break;
        case utils::eEventActiveDeviceInfo:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventActiveDeviceInfo");
            events::EVENT_ACTIVE_DEVICE_INFO_T *stActiveDeviceInfo = (events::EVENT_ACTIVE_DEVICE_INFO_T*)event;
            eventActiveDeviceInfo(stActiveDeviceInfo->deviceName, stActiveDeviceInfo->display, stActiveDeviceInfo->isConnected,
                stActiveDeviceInfo->isOutput);
        }
        break;
        case utils::eEventResponseSoundOutputDeviceInfo:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventResponseSoundOutputDeviceInfo");
            events::EVENT_RESPONSE_SOUNDOUTPUT_INFO_T *stSoundOutputDeviceInfo = (events::EVENT_RESPONSE_SOUNDOUTPUT_INFO_T*)event;
            eventResponseSoundOutputDeviceInfo(stSoundOutputDeviceInfo->soundOutputInfo);
        }
        break;
        case utils::eEventResponseSoundInputDeviceInfo:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventResponseSoundInputDeviceInfo");
            events::EVENT_RESPONSE_SOUNDINPUT_INFO_T *stSoundInputDeviceInfo = (events::EVENT_RESPONSE_SOUNDINPUT_INFO_T*)event;
            eventResponseSoundInputDeviceInfo(stSoundInputDeviceInfo->soundInputInfo);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}