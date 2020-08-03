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

#include "soundOutputManager.h"

void SoundOutputManager::readSoundOutputListInfo()
{
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT,\
        "SoundOutputManager::readSoundOutputListInfo");
    if (mObjSoundOutputListParser)
    {
        if (mObjSoundOutputListParser->loadSoundOutputListJsonConfig())
        {
            PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "loadSoundOutputListJsonConfig success");
            pbnjson::JValue soundOutputListInfo = mObjSoundOutputListParser->getSoundOutputListInfo();
            if (initializeSoundOutputList(soundOutputListInfo))
                PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "initialize soundOutputListInfo success");
            else
                PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "initialize soundOutputListInfo failed");
        }
        else
            PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Unable to load SoundOutputListJsonConfig");
    }
    else
        PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "mObjSoundOutputListParser is null");
}

bool SoundOutputManager::initializeSoundOutputList(const pbnjson::JValue& soundOutputListInfo)
{
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT,\
        "SoundOutputManager::initializeSoundOutputList");
    if (!soundOutputListInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "soundOutputListInfo is not an array");
        return false;
    }
    else
    {
        for (pbnjson::JValue arrItem: soundOutputListInfo.items())
        {
            utils::SOUNDOUTPUT_LIST_T soundOutInfo;
            std::string soundOutputValue;
            bool muteStatus;
            bool activeStatus;
            bool adjustVolume;
            if (arrItem["soundOutput"].asString(soundOutputValue) == CONV_OK)
            {
                soundOutInfo.volume = arrItem["volume"].asNumber<int>();
                soundOutInfo.maxVolume = arrItem["maxVolume"].asNumber<int>();
                if (arrItem["muteStatus"].asBool(muteStatus) == CONV_OK)
                    soundOutInfo.muteStatus = muteStatus;
                else
                    soundOutInfo.muteStatus = false;
                if (arrItem["activeStatus"].asBool(activeStatus) == CONV_OK)
                    soundOutInfo.activeStatus = activeStatus;
                else
                    soundOutInfo.activeStatus = false;
                if (arrItem["adjustVolume"].asBool(adjustVolume) == CONV_OK)
                    soundOutInfo.adjustVolume = adjustVolume;
                else
                    soundOutInfo.adjustVolume = true;
            }
            // Update map to keep the masterVolume info for respective soundoutput(Updating from DB to local map)
            mSoundOutputInfoMap.insert(std::pair<std::string, utils::SOUNDOUTPUT_LIST_T>(soundOutputValue, soundOutInfo));
        }
    }
    printSoundOutputListInfo();
    return true;
}

void SoundOutputManager::printSoundOutputListInfo()
{
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT,\
        "SoundOutputManager::printSoundOutputListInfo: ");
    for (const auto &elements: mSoundOutputInfoMap)
    {
         PM_LOG_DEBUG("soundOut:%s volume:%d mute:%d active:%d",\
             elements.first.c_str(), elements.second.volume,\
             elements.second.muteStatus, elements.second.activeStatus);
    }
    PM_LOG_DEBUG("Size of map:%d", mSoundOutputInfoMap.size());
}

bool SoundOutputManager::_SetSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(soundOut, string)) REQUIRED_1(soundOut)));

    if (!msg.parse(__FUNCTION__, lshandle))
    {
        return true;
    }
    std::string soundOut = "";
    msg.get("soundOut", soundOut);
    envelopeRef *envelopeObj = new (std::nothrow)envelopeRef;
    bool status = false;
    std::string reply = STANDARD_JSON_SUCCESS;
    if (nullptr != envelopeObj)
    {
        envelopeObj->message = message;
        envelopeObj->context = (SoundOutputManager*)ctx;
        SoundOutputManager *soundOutputManagerObj = (SoundOutputManager*)ctx;
        if (nullptr != soundOutputManagerObj->mObjAudioMixer)
        {
            if (soundOutputManagerObj->mObjAudioMixer->setSoundOut(soundOut,_updateSoundOutStatus,envelopeObj))
            {
                PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "SoundSettings: SetSoundOut Successful");
                LSMessageRef(message);
                status = true;
            }
            else
            {
                PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Sound Settings: SetSoundOut call failed");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Sound Settings :mObjAudioMixer is NULL");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE , "Internal error");
    }

    if (status == false)
    {
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
        if (nullptr != envelopeObj)
        {
            delete envelopeObj;
            envelopeObj = nullptr;
        }
    }

    return true;
}

bool SoundOutputManager::_updateSoundOutStatus(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "_updateSoundOutStatus Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                                                 REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;

    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        PM_LOG_ERROR(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "%s:%d no update. Could not updateSoundOutStatus", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData(reply, STRICT_SCHEMA(PROPS_2(PROP(returnValue, boolean),
        PROP(soundOut, string)) REQUIRED_2(returnValue, soundOut)));
        std::string l_strSoundOut;
        msgData.get("soundOut", l_strSoundOut);
        PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "Soundoutmode set Successfully for sound out %s",l_strSoundOut.c_str());
    }
    if (nullptr != ctx)
    {
        envelopeRef *envelopeObj = (envelopeRef*)ctx;
        LSMessage *message = (LSMessage*)envelopeObj->message;
        if (nullptr != message)
        {
            CLSError lserror;
            std::string payload = LSMessageGetPayload(reply);
            if (!LSMessageRespond(message, payload.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
            LSMessageUnref(message);
        }

        if (nullptr != envelopeObj)
        {
            delete envelopeObj;
            envelopeObj = nullptr;
        }
    }
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT, "_updateSoundOutStatus Done");
    return returnValue;
}

SoundOutputManager::SoundOutputManager(): mObjAudioMixer(nullptr), soundMode(""),
                                          mObjSoundOutputListParser(nullptr)
{
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT,\
                "SoundOutputManager: constructor");
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    mObjSoundOutputListParser = new (std::nothrow)SoundOutputListParser();
    if (mObjSoundOutputListParser)
    {
        PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT,\
                    "SoundOutputManager::constructor - mObjSoundOutputListParser created success");
    }
    readSoundOutputListInfo();
}

SoundOutputManager::~SoundOutputManager()
{
    PM_LOG_INFO(MSGID_SOUND_SETTINGS, INIT_KVCOUNT,\
               "SoundOutputManager: destructor");
    if (mObjSoundOutputListParser)
    {
        delete mObjSoundOutputListParser;
        mObjSoundOutputListParser = nullptr;
    }
}
