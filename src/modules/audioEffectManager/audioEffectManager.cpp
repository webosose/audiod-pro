/*
 * Copyright (c) 2022-2023 LG Electronics Inc.
 * SPDX-License-Identifier: LicenseRef-LGE-Proprietary
 */

#include "audioEffectManager.h"

bool AudioEffectManager::mIsObjRegistered = AudioEffectManager::RegisterObject();
AudioEffectManager* AudioEffectManager::mAudioEffectManager = nullptr;

AudioEffectManager* AudioEffectManager::getAudioEffectManagerInstance() {
    return mAudioEffectManager;
}

AudioEffectManager::AudioEffectManager(ModuleConfig* const pConfObj):   mObjModuleManager(nullptr),
                                                                        mObjAudioMixer(nullptr),
                                                                        inputDevCnt(0),
                                                                        isAGCEnabled(false)
{
    PM_LOG_DEBUG("%s: constructor()", MSGID_AUDIO_EFFECT_MANAGER);
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (!mObjModuleManager) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "mObjModuleManager is nullptr");
    } else {
         mObjModuleManager->subscribeModuleEvent(this, utils::eEventDeviceConnectionStatus);
    }

    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if (!mObjAudioMixer) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioMixer instance is null");
    }
}

AudioEffectManager::~AudioEffectManager() {
    PM_LOG_DEBUG("%s: destructor()", MSGID_AUDIO_EFFECT_MANAGER);
}

void AudioEffectManager::initialize() {
    PM_LOG_DEBUG("%s: initialize()", MSGID_AUDIO_EFFECT_MANAGER);
    if (mAudioEffectManager) {
        CLSError lserror;

        bool result = LSRegisterCategoryAppend(GetPalmService(), "/", audioeffectMethods, nullptr, &lserror);
        if (!result || !LSCategorySetData(GetPalmService(), "/", mAudioEffectManager, &lserror)) {
            PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,
                    "%s: Registering Service for '%s' category failed", __FUNCTION__, "/");
            lserror.Print(__FUNCTION__, __LINE__);
        }
        PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Successfully initialized mAudioEffectManager");
    } else {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Could not load module mAudioEffectManager");
    }
}

void AudioEffectManager::deInitialize() {
    PM_LOG_DEBUG("%s: deInitialize()", MSGID_AUDIO_EFFECT_MANAGER);
    if (mAudioEffectManager) {
        delete mAudioEffectManager;
        mAudioEffectManager = nullptr;
    } else {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "mAudioEffectManager is nullptr");
    }
}

int AudioEffectManager::getEffectId(std::string effectName) {
    int effectId = -1;
    for (int i = 0; i < audioEffectListSize; i++) {
        if (effectName.compare(audioEffectList[i]) == 0) {
            effectId = i;
        }
    }
    return effectId;
}

//  luna API calls
LSMethod AudioEffectManager::audioeffectMethods[] = {
    { "getAudioEffectList", AudioEffectManager::_getAudioEffectList},
    { "setAudioEffect", AudioEffectManager::_setAudioEffect},
    { "checkAudioEffectStatus", AudioEffectManager::_checkAudioEffectStatus},
    { },
};

bool AudioEffectManager::_getAudioEffectList(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _getAudioEffectList");
    LSMessageJsonParser msg(message, STRICT_SCHEMA());
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    pbnjson::JValue listArray = pbnjson::Array();
    bool returnValue = true;

    for (int i = 0; i < audioEffectListSize; i++) {
        listArray << pbnjson::JValue(audioEffectList[i]);
    }

    pbnjson::JValue effectResponse = pbnjson::Object();
    effectResponse.put("returnValue", returnValue);
    effectResponse.put("audioEffectList", listArray);
    reply = effectResponse.stringify();
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioEffectManager::_setAudioEffect(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _setAudioEffect");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(effectName, string), PROP(enabled, boolean)) REQUIRED_2(effectName, enabled)));
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    bool returnValue = false;
    std::string effectName;
    bool enabled = false;
    AudioEffectManager* obj = AudioEffectManager::getAudioEffectManagerInstance();
    msg.get("effectName", effectName);
    msg.get("enabled", enabled);
    int effectId = getAudioEffectManagerInstance()->getEffectId(effectName);
    if (effectId < 0) {
        reply = STANDARD_JSON_ERROR(19, "Invalid parameters");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }
    PM_LOG_DEBUG("%d ,%d", obj->inputDevCnt ,effectId);
    if (obj->inputDevCnt<=0)
    {
        if (effectId == 1)
        {
            PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"No input device connected");
            reply = STANDARD_JSON_ERROR(5, "SoundInput not connected");
            utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
            return true;
        }
    }

    if (getAudioEffectManagerInstance()->isAGCEnabled && effectId == 1 && enabled)
    {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"AGC feature is already enabled");
        reply = STANDARD_JSON_ERROR(7, "AGC Already Enabled");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    if(((obj->inputDevCnt > 0) && effectId == 1) || effectId == 0)
    {
        AudioMixer* audioMixer = AudioMixer::getAudioMixerInstance();
        if (effectId != -1 && audioMixer && audioMixer->setAudioEffect(effectId, enabled)) {
            returnValue = true;
            if (effectId == 1 && enabled)
               getAudioEffectManagerInstance()->isAGCEnabled = true;
            else if (effectId == 1 && !enabled)
               getAudioEffectManagerInstance()->isAGCEnabled = false;
        }
    }
    else {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Could not load module mAudioEffectManager");
    }

    pbnjson::JValue effectResponse = pbnjson::Object();
    effectResponse.put("returnValue", returnValue);
    reply = effectResponse.stringify();
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;
}

bool AudioEffectManager::_checkAudioEffectStatus(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _checkAudioEffectStatus");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(effectName, string)) REQUIRED_1(effectName)));
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    bool returnValue = false;
    std::string effectName;
    msg.get("effectName", effectName);
    int effectId = getAudioEffectManagerInstance()->getEffectId(effectName);
    if (effectId < 0) {
        reply = STANDARD_JSON_ERROR(19, "Invalid parameters");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    AudioMixer* audioMixer = AudioMixer::getAudioMixerInstance();
    if (effectId != -1 && audioMixer) {
        returnValue = true;
    }

    pbnjson::JValue effectResponse = pbnjson::Object();
    effectResponse.put("returnValue", returnValue);
    effectResponse.put("enabled", audioMixer->checkAudioEffectStatus(effectId));
    reply = effectResponse.stringify();
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;

}

void AudioEffectManager::eventDeviceConnectionStatus(const std::string &deviceName , const std::string &deviceNameDetail, const std::string &deviceIcon, \
    utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,\
        "eventDeviceConnectionStatus with deviceName:%s deviceStatus:%d mixerType:%d",\
        deviceName.c_str(), (int)deviceStatus, (int)mixerType);
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,\
        "eventDeviceConnectionStatus deviceNameDetail : %s, deviceIcon: %s isOutput : %d",deviceNameDetail.c_str(), deviceIcon.c_str(), (int)isOutput);

    if (deviceStatus == utils::eDeviceConnected)
    {
        if(!isOutput && deviceName.find("usb")!=deviceName.npos)
        {
            inputDevCnt++;
            PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"increment devicecount %d", inputDevCnt);
        }
    }
    else if ((deviceStatus == utils::eDeviceDisconnected))
    {
        if(!isOutput && deviceName.find("usb")!=deviceName.npos)
        {
            inputDevCnt--;
            PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"decrement devicecount %d", inputDevCnt);
        }
    }
}

void AudioEffectManager::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {
        case  utils::eEventDeviceConnectionStatus:
        {
            PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,\
                "handleEvent : eEventDeviceConnectionStatus");
            events::EVENT_DEVICE_CONNECTION_STATUS_T * stDeviceConnectionStatus = (events::EVENT_DEVICE_CONNECTION_STATUS_T*)event;
            eventDeviceConnectionStatus(stDeviceConnectionStatus->devicename, stDeviceConnectionStatus->deviceNameDetail, stDeviceConnectionStatus->deviceIcon, \
                stDeviceConnectionStatus->deviceStatus, stDeviceConnectionStatus->mixerType,stDeviceConnectionStatus->isOutput);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}