// Copyright (c) 2020 LG Electronics, Inc.
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

#include "masterVolumeManager.h"

MasterVolumeManager* MasterVolumeManager::mMasterVolumeManager = nullptr;
SoundOutputManager* MasterVolumeManager::mObjSoundOutputManager = nullptr;

MasterVolumeManager* MasterVolumeManager::getMasterVolumeManagerInstance()
{
    return mMasterVolumeManager;
}

bool MasterVolumeManager::_setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(volume, integer), PROP(sessionId, integer)) REQUIRED_2(soundOutput, volume)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return true;

    bool status = false;
    std::string soundOutput;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int displayId = DISPLAY_ONE;
    int volume = MIN_VOLUME;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("volume", volume);
    msg.get("sessionId", display);

    if ((volume >= MIN_VOLUME) && (volume <= MAX_VOLUME))
        isValidVolume = true;

    if (DISPLAY_TWO == display)
        displayId = DEFAULT_TWO_DISPLAY_ID;
    else
        displayId = DEFAULT_ONE_DISPLAY_ID;

    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "SetMasterVolume with soundout: %s volume: %d display: %d", \
                soundOutput.c_str(), volume, displayId);
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    if (DISPLAY_TWO == display)
    {
        if (soundOutput != "alsa")
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Not a valid soundOutput");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_SOUNDOUT, "Volume control is not supported");
        }
        else if ((masterVolumeInstance) && (isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            masterVolumeInstance->displayTwoVolume = volume;
            std::string callerId = LSMessageGetSenderServiceName(message);
            masterVolumeInstance->notifyVolumeSubscriber(displayId, callerId);
            pbnjson::JValue setVolumeResponse = pbnjson::Object();
            setVolumeResponse.put("returnValue", true);
            setVolumeResponse.put("volume", volume);
            setVolumeResponse.put("soundOutput", soundOutput);
            reply = setVolumeResponse.stringify();
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
        }

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else
    {
        if ((masterVolumeInstance) && (isValidVolume) && (audioMixerObj) && (audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            masterVolumeInstance->displayOneVolume = volume;
            std::string callerId = LSMessageGetSenderServiceName(message);
            masterVolumeInstance->notifyVolumeSubscriber(displayId, callerId);
            status = true;
        }
        else
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
        envelopeRef *envelope = new (std::nothrow)envelopeRef;
        if (nullptr != envelope)
        {
            envelope->message = message;
            envelope->context = (MasterVolumeManager*)ctx;
            MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)ctx;

            if ((nullptr != masterVolumeManagerObj->mObjAudioMixer) && (isValidVolume))
            {
                if(masterVolumeManagerObj->mObjAudioMixer->setMasterVolume(soundOutput, volume, _setVolumeCallBack, envelope))
                {
                    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: SetMasterVolume umimixer call successfull");
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    status = false;
                    PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: SetMasterVolume umimixer call failed");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
                }
            }
            else
            {
                status = false;
                PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
            }
        }
        else
        {
            status = false;
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: SetMasterVolume envelope is NULL");
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
    return true;
}

bool MasterVolumeManager::_setVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setVolumeCallBack");
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume::Successfully Set the speaker volume for sound out %s with volume %d", \
                        soundOutput.c_str(), iVolume);
        }
        else
        {
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: Could not SetMasterVolume");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)envelope->context;
        if (true == returnValue)
        {
            if (nullptr != masterVolumeManagerObj)
            {
                masterVolumeManagerObj->setCurrentVolume(iVolume);
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

std::string MasterVolumeManager::getVolumeInfo(const int &displayId, const std::string &callerId)
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

void MasterVolumeManager::notifyVolumeSubscriber(const int &displayId, const std::string &callerId)
{
    CLSError lserror;
    std::string reply = getVolumeInfo(displayId, callerId);
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_VOLUME, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Notify error");
    }
}

void MasterVolumeManager::setMuteStatus(const int &displayId)
{
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    if (DEFAULT_ONE_DISPLAY_ID == displayId && audioMixerObj)
    {
        audioMixerObj->setMute(displayId, displayOneMuteStatus);
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set mute status %d for display: %d", displayOneMuteStatus, displayId);
    }
    else if (DEFAULT_TWO_DISPLAY_ID == displayId && audioMixerObj)
    {
        audioMixerObj->setMute(displayId, displayTwoMuteStatus);
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set mute status %d for display: %d", displayTwoMuteStatus, displayId);
    }
    else
    {
        if (audioMixerObj)
        {
            audioMixerObj->setMute(displayId, displayOneMuteStatus);
            audioMixerObj->setMute(displayId, displayTwoMuteStatus);
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set mute status is %d for display one and mute status is %d for display two", \
                        displayOneMuteStatus, displayTwoMuteStatus);
        }
    }
}

void MasterVolumeManager::setVolume(const int &displayId)
{
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    int volume = 0;
    if (DEFAULT_ONE_DISPLAY_ID == displayId)
        volume = displayOneVolume;
    else if (DEFAULT_TWO_DISPLAY_ID == displayId)
        volume = displayTwoVolume;
    if (audioMixerObj && audioMixerObj->setVolume(displayId, volume))
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
}

bool MasterVolumeManager::_getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(subscribe, boolean), PROP(sessionId, integer))));
    if (!msg.parse(__FUNCTION__,lshandle))
        return true;

    int display = DISPLAY_ONE;
    bool subscribed = false;
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("subscribe", subscribed);
    msg.get("sessionId", display);

    if (LSMessageIsSubscription (message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return true;
        }
    }

    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    MasterVolumeManager* masterVolumeManagerInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    if (nullptr != envelope)
    {
        envelope->message = message;
        envelope->context = (MasterVolumeManager*)ctx;
        MasterVolumeManager *masterVolumeManagerObj = (MasterVolumeManager*)ctx;
        if (nullptr != masterVolumeManagerObj)
        {
            int displayId = DEFAULT_ONE_DISPLAY_ID;
            std::string callerId = LSMessageGetSenderServiceName(message);
            if (DISPLAY_TWO == display)
                displayId = DEFAULT_TWO_DISPLAY_ID;

            reply = masterVolumeManagerInstance->getVolumeInfo(displayId, callerId);
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeManagerObj is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getVolume envelope is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", reply.c_str(), __FUNCTION__);
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return true;
}
bool MasterVolumeManager::_getVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getVolumeCallBack");
    std::string payload = LSMessageGetPayload(reply);
    JsonMessageParser ret(payload.c_str(), NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean)) REQUIRED_1(returnValue)));
    bool returnValue = false;
    if (ret.parse(__FUNCTION__))
    {
        ret.get("returnValue", returnValue);
    }
    if (returnValue)
    {
        JsonMessageParser data(payload.c_str(), NORMAL_SCHEMA(PROPS_1(PROP(volumeStatus, array)) REQUIRED_1(volumeStatus)));
        if (data.parse(__FUNCTION__))
        {
            pbnjson::JSchemaFragment inputSchema("{}");
            pbnjson::JDomParser parser(NULL);
            parser.parse(payload, inputSchema, NULL);
            pbnjson::JValue parsed = parser.getDom();
            pbnjson::JValue volumeStatus = parsed["volumeStatus"];
            if (!parsed["volumeStatus"].isArray())
            {
                PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: invalid volumeStatus array");
            }
            else
            {
                returnValue = true;
                for (int i = 0; i < volumeStatus.arraySize(); ++i)
                {
                    bool bMuteSstatus = volumeStatus[i]["muted"].asBool();
                    std::string strSoundOutput = volumeStatus[i]["SoundOutput"].asString();
                    int iVolume = volumeStatus[i]["volume"].asNumber<int>();
                    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: MuteStatus: %d Soundout: %s volume: %d \n", \
                                (int)bMuteSstatus, strSoundOutput.c_str(), iVolume);
                }
            }
        }
        else
        {
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: Could not GetMasterVolume");
    }
    if (nullptr != ctx)
    {
         envelopeRef *envelope = (envelopeRef*)ctx;
         LSMessage *message = (LSMessage*)envelope->message;
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
             PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: internal mixer call");
         }
         if (nullptr != envelope)
         {
             delete envelope;
             envelope = nullptr;
         }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

bool MasterVolumeManager::_muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_3(PROP(soundOutput, string), PROP(mute, boolean), PROP(sessionId, integer)) REQUIRED_2(soundOutput, mute)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return true;
    std::string soundOutput;
    bool mute = false;
    bool status = false;
    int displayId = DISPLAY_ONE;
    int display;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("mute", mute);
    msg.get("sessionId", display);

    if (DISPLAY_ONE == display)
        displayId = 1;
    else if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 3;

    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "muteVolume with soundout: %s mute status: %d", \
                soundOutput.c_str(),(int)mute);
    envelopeRef *envelope = new (std::nothrow)envelopeRef;
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
    {
        if (audioMixerObj && audioMixerObj->setMute(displayId, mute))
        {
            masterVolumeInstance->displayTwoMuteStatus = mute;
            masterVolumeInstance->notifyVolumeSubscriber(displayId, callerId);
            pbnjson::JValue muteVolumeResponse = pbnjson::Object();
            muteVolumeResponse.put("returnValue", true);
            muteVolumeResponse.put("mute", mute);
            muteVolumeResponse.put("soundOutput", soundOutput);
            reply = muteVolumeResponse.stringify();
        }
        else
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", \
                        mute, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }

        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else
    {
        if (audioMixerObj && audioMixerObj->setMute(displayId, mute))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Successfully set mute volume %d for display: %d", \
                        mute, displayId);
            if (DEFAULT_ONE_DISPLAY_ID == displayId)
                masterVolumeInstance->displayOneMuteStatus = mute;
            else
            {
                masterVolumeInstance->displayOneMuteStatus = mute;
                masterVolumeInstance->displayTwoMuteStatus = mute;
            }
            masterVolumeInstance->notifyVolumeSubscriber(displayId, callerId);
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to mute volume %d for display: %d", mute, displayId);
        }
        if(nullptr != envelope)
        {
            envelope->message = message;
            envelope->context = (MasterVolumeManager*)ctx;
            MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)ctx;
            if (nullptr != masterVolumeManagerObj->mObjAudioMixer)
            {
                if(masterVolumeManagerObj->mObjAudioMixer->masterVolumeMute(soundOutput, mute, _muteVolumeCallBack, envelope))
                {
                    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeMute umimixer call successfull");
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeMute umimixer call failed");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume envelope is NULL");
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
    return true;
}

bool MasterVolumeManager::_muteVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolumeCallBack");
    std::string payload = LSMessageGetPayload(reply);
    JsonMessageParser ret(payload.c_str(), NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean)) REQUIRED_1(returnValue)));
    bool returnValue = false;
    if (ret.parse(__FUNCTION__))
    {
        ret.get("returnValue", returnValue);
    }
    std::string soundOutput;
    bool bMute = false;
    if (returnValue)
    {
        JsonMessageParser data(payload.c_str(), NORMAL_SCHEMA(PROPS_2(PROP(mute, boolean),\
            PROP(soundOutput, string)) REQUIRED_2(soundOutput, mute)));
        if (data.parse(__FUNCTION__))
        {
           data.get("mute", bMute);
           data.get("soundOutput", soundOutput);
           PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume:Successfully muted/unmuted the soundout %s and mute status %d ", \
                       soundOutput.c_str(), bMute);
        }
        else
        {
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: Could not mute MasterVolume");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)envelope->context;
        if(true == returnValue)
        {
            if (nullptr != masterVolumeManagerObj)
            {
                masterVolumeManagerObj->setCurrentMuteStatus(bMute);
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

bool MasterVolumeManager::_volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return true;
    std::string soundOutput;
    bool status = false;
    int display = DISPLAY_ONE;
    bool isValidVolume = false;
    int volume = MIN_VOLUME;
    int displayId = DISPLAY_ONE;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp with soundout: %s", soundOutput.c_str());
    MasterVolumeManager* masterVolumeManagerInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
    {
        if ((masterVolumeManagerInstance->displayTwoVolume+1) <= MAX_VOLUME)
        {
            isValidVolume = true;
            volume = masterVolumeManagerInstance->displayTwoVolume+1;
        }
        else
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
        if ((isValidVolume) && (audioMixerObj && audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            ++(masterVolumeManagerInstance->displayTwoVolume);
            masterVolumeManagerInstance->notifyVolumeSubscriber(displayId, callerId);
            pbnjson::JValue setVolumeResponse = pbnjson::Object();
            setVolumeResponse.put("returnValue", true);
            setVolumeResponse.put("volume", volume);
            setVolumeResponse.put("soundOutput", soundOutput);
            reply = setVolumeResponse.stringify();
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
        }
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else
    {
        if ((masterVolumeManagerInstance->displayOneVolume+1) <= MAX_VOLUME)
        {
            isValidVolume = true;
            volume = masterVolumeManagerInstance->displayOneVolume+1;
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume up value not in range");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
        if ((isValidVolume) && (audioMixerObj && audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            ++(masterVolumeManagerInstance->displayOneVolume);
            masterVolumeManagerInstance->notifyVolumeSubscriber(displayId, callerId);
        }
        envelopeRef *envelope = new (std::nothrow)envelopeRef;
        if(nullptr != envelope)
        {
            envelope->message = message;
            envelope->context = (MasterVolumeManager*)ctx;
            MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)ctx;
            if ((nullptr != masterVolumeManagerObj->mObjAudioMixer) && (isValidVolume))
            {
                if(masterVolumeManagerObj->mObjAudioMixer->masterVolumeUp(soundOutput, _volumeUpCallBack, envelope))
                {
                    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeUp umimixer call successfull");
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeUp umimixer call failed");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_PARAMETER_BE_EMPTY, "Internal error");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeUp envelope is NULL");
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
    return true;
}

bool MasterVolumeManager::_volumeUpCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUpCallBack");
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
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
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: Could not volume up");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        MasterVolumeManager *masterVolumeManagerObj = (MasterVolumeManager*)envelope->context;
        if(true == returnValue)
        {
            if (nullptr != masterVolumeManagerObj)
            {
                masterVolumeManagerObj->setCurrentVolume(iVolume);
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}

bool MasterVolumeManager::_volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(soundOutput, string), PROP(sessionId, integer)) REQUIRED_1(soundOutput)));
    if (!msg.parse(__FUNCTION__,lshandle))
        return true;
    std::string soundOutput;
    bool status = false;
    int displayId = DISPLAY_ONE;
    bool isValidVolume = false;
    int volume = MIN_VOLUME;
    int display = DISPLAY_ONE;
    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("soundOutput", soundOutput);
    msg.get("sessionId", display);

    if (DISPLAY_TWO == display)
        displayId = 2;
    else
        displayId = 1;

    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown with soundout: %s", soundOutput.c_str());
    MasterVolumeManager* masterVolumeManagerInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string callerId = LSMessageGetSenderServiceName(message);
    if (DISPLAY_TWO == display)
    {
        if ((masterVolumeManagerInstance->displayTwoVolume-1) >= MIN_VOLUME)
        {
            isValidVolume = true;
            volume = masterVolumeManagerInstance->displayTwoVolume-1;
        }
        else
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
        if ((isValidVolume) && (audioMixerObj && audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            --(masterVolumeManagerInstance->displayTwoVolume);
            masterVolumeManagerInstance->notifyVolumeSubscriber(displayId, callerId);
            pbnjson::JValue setVolumeResponse = pbnjson::Object();
            setVolumeResponse.put("returnValue", true);
            setVolumeResponse.put("volume", volume);
            setVolumeResponse.put("soundOutput", soundOutput);
            reply = setVolumeResponse.stringify();
        }
        else
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Did not able to set volume %d for display: %d", volume, displayId);
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
        }
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else
    {
        if ((masterVolumeManagerInstance->displayOneVolume-1) >= MIN_VOLUME)
        {
            isValidVolume = true;
            volume = masterVolumeManagerInstance->displayOneVolume-1;
        }
        else
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Volume down value not in range");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE, "SoundOutput volume is not in range");
            CLSError lserror;
            if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
        if ((isValidVolume) && (audioMixerObj && audioMixerObj->setVolume(displayId, volume)))
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "set volume %d for display: %d", volume, displayId);
            --(masterVolumeManagerInstance->displayOneVolume);
            masterVolumeManagerInstance->notifyVolumeSubscriber(displayId, callerId);
        }
        envelopeRef *envelope = new (std::nothrow)envelopeRef;
        if(nullptr != envelope)
        {
            envelope->message = message;
            envelope->context = (MasterVolumeManager*)ctx;
            MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)ctx;
            if ((nullptr != masterVolumeManagerObj->mObjAudioMixer) && (isValidVolume))
            {
                if(masterVolumeManagerObj->mObjAudioMixer->masterVolumeDown(soundOutput, _volumeDownCallBack, envelope))
                {
                    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeDown umimixer call successfull");
                    LSMessageRef(message);
                    status = true;
                }
                else
                {
                    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeDown umimixer call failed");
                    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
                }
            }
            else
            {
                PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: gumiaudiomixer is NULL");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
            }
        }
        else
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: masterVolumeDown envelope is NULL");
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
    return true;
}

bool MasterVolumeManager::_volumeDownCallBack(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDownCallBack");
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
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
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: Could not volume up");
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelope  = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelope->message;
        MasterVolumeManager* masterVolumeManagerObj = (MasterVolumeManager*)envelope->context;
        if(true == returnValue)
        {
            if (nullptr != masterVolumeManagerObj)
            {
                masterVolumeManagerObj->setCurrentVolume(iVolume);
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
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: internal mixer call");
        }
        if (nullptr != envelope)
        {
            delete envelope;
            envelope = nullptr;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: context is null");
    }
    return true;
}


void MasterVolumeManager::setCurrentVolume(int iVolume)
{
    mVolume = iVolume;
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume::updated volume: %d ", mVolume);
}

void MasterVolumeManager::setCurrentMuteStatus(bool bMuteStatus)
{
    mMuteStatus = bMuteStatus;
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,  "MasterVolume::updated mute status: %d ", (int)mMuteStatus);
}

void MasterVolumeManager::eventMasterVolumeStatus()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "eventMasterVolumeStatus: setMuteStatus and setVolume");
    setMuteStatus(DEFAULT_ONE_DISPLAY_ID);
    setVolume(DEFAULT_ONE_DISPLAY_ID);
    setMuteStatus(DEFAULT_TWO_DISPLAY_ID);
    setVolume(DEFAULT_TWO_DISPLAY_ID);
}

/* TODO
currently these luna API's are not in sync with exsting master volume
In future existing master volume will be removed and these below luna API's will be used
for master volume handling as per Generic AV arch and made in sync with remote and other services*/

static LSMethod MasterVolumeMethods[] =
{
    {"setVolume", MasterVolumeManager::_setVolume},
    {"volumeDown", MasterVolumeManager::_volumeDown},
    {"volumeUp", MasterVolumeManager::_volumeUp},
    {"getVolume", MasterVolumeManager::_getVolume},
    {"muteVolume", MasterVolumeManager::_muteVolume},
    { },
};

static LSMethod SoundOutputManagerMethods[] = {
    {"setSoundOut", SoundOutputManager::_SetSoundOut},
    { },
};

void MasterVolumeManager::initSoundOutputManager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "initSoundOutputManager::initialising sound output manager module");
    CLSError lserror;
    if (!mObjSoundOutputManager)
    {
        mObjSoundOutputManager = new (std::nothrow)SoundOutputManager();
        if (mObjSoundOutputManager)
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "load module soundOutputManager successful");
            bool result = LSRegisterCategoryAppend(handle, "/soundSettings", SoundOutputManagerMethods, nullptr, &lserror);
            if (!result || !LSCategorySetData(handle, "/soundSettings", mObjSoundOutputManager, &lserror))
            {
                PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "%s: Registering Service for '%s' category failed", __FUNCTION__, "/soundSettings");
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Could not load module soundOutputManager");
    }
}

void MasterVolumeManager::loadModuleMasterVolumeManager(GMainLoop *loop, LSHandle* handle)
{
    CLSError lserror;
    if (!mMasterVolumeManager)
    {
        mMasterVolumeManager = new (std::nothrow)MasterVolumeManager();
        if (mMasterVolumeManager)
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "load module MasterVolumeManager successful");
            bool bRetVal = LSRegisterCategoryAppend(handle, "/master", MasterVolumeMethods, nullptr, &lserror);
            if (!bRetVal || !LSCategorySetData(handle, "/master", mMasterVolumeManager, &lserror))
            {
               PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s: Registering Service for '%s' category failed", \
                            __FUNCTION__, "/master");
               lserror.Print(__FUNCTION__, __LINE__);
            }
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "load module MasterVolumeManager Init Done");
        }
        else
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Could not load module MasterVolumeManager");
    }
}

MasterVolumeManager::MasterVolumeManager(): mObjAudioMixer(AudioMixer::getAudioMixerInstance()), mVolume(0), mMuteStatus(false), \
                                            displayOneVolume(100), displayTwoVolume(100), displayOneMuteStatus(0), displayTwoMuteStatus(0)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolumeManager: constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
        mObjModuleManager->subscribeModuleEvent(this, true, utils::eEventMasterVolumeStatus);
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "mObjModuleManager is null");
}

MasterVolumeManager::~MasterVolumeManager()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolumeManager: destructor");
    if (mObjSoundOutputManager)
    {
        delete mObjSoundOutputManager;
        mObjSoundOutputManager = nullptr;
    }
}

int load_master_volume_manager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "loading master_volume_manager");
    MasterVolumeManager::loadModuleMasterVolumeManager(loop, handle);
    MasterVolumeManager::initSoundOutputManager(loop, handle);
    return 0;
}

void unload_master_volume_manager()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "unloading master_volume_manager");
    if (MasterVolumeManager::mMasterVolumeManager)
    {
        delete MasterVolumeManager::mMasterVolumeManager;
        MasterVolumeManager::mMasterVolumeManager = nullptr;
    }
}