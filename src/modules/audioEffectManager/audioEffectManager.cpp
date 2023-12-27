/*
 * Copyright (c) 2022-2023 LG Electronics Inc.
 * SPDX-License-Identifier: LicenseRef-LGE-Proprietary
 */

#include <string.h>

#include "audioEffectManager.h"

bool AudioEffectManager::mIsObjRegistered = AudioEffectManager::RegisterObject();
AudioEffectManager* AudioEffectManager::mAudioEffectManager = nullptr;

std::vector<std::string> AudioEffectManager::audioEffectList {
    "dynamic compressor",
    "equalizer",
    "bass boost"
};

AudioEffectManager* AudioEffectManager::getAudioEffectManagerInstance() {
    return mAudioEffectManager;
}

AudioEffectManager::AudioEffectManager(ModuleConfig* const pConfObj):   mObjModuleManager(nullptr),
                                                                        mObjAudioMixer(nullptr),
                                                                        inputDevCnt(0),
                                                                        isArrayMic(false)
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
    pbnjson::JValue fileInfo =  pbnjson::JDomParser::fromFile("/etc/pulse/preprocessingAudioEffect.json",pbnjson::JSchema::AllSchema());
    for (const pbnjson::JValue& elements : fileInfo.items())
    {
            std::string name;
            if (elements["name"].asString(name) != CONV_OK)
            continue;
            //handling if name is different from preprocessingAudioEffect.json
            if (name.compare("gain_control") == 0)
                AudioEffectManager::audioEffectList.push_back("gain control");
            else if (name.compare("speech_enhancement") == 0)
                AudioEffectManager::audioEffectList.push_back("speech enhancement");
            else if (name.compare("beamforming") == 0)
                AudioEffectManager::audioEffectList.push_back("beamforming");
            else
                AudioEffectManager::audioEffectList.push_back(name);
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
    int size = AudioEffectManager::audioEffectList.size();
    for (int i = 0; i < size; i++) {
        if (effectName.compare(AudioEffectManager::audioEffectList[i]) == 0) {
            effectId = i;
        }
    }
    return effectId;
}

std::string AudioEffectManager::getEffecName(int effectId) {
    std::string effectName;
    int size = AudioEffectManager::audioEffectList.size();
    for (int i = 0; i < size; i++) {
        if (i == effectId) {
            effectName = AudioEffectManager::audioEffectList[i];
        }
    }
    return effectName;
}

//  luna API calls
LSMethod AudioEffectManager::audioeffectMethods[] = {
    { "getAudioEffectList", AudioEffectManager::_getAudioEffectList},
    { "setAudioEffect", AudioEffectManager::_setAudioEffect},
    { "checkAudioEffectStatus", AudioEffectManager::_checkAudioEffectStatus},
    { "getAudioEffectsStatus", AudioEffectManager::_getAudioEffectsStatus},
    { "setAudioEqualizerBandLevel", AudioEffectManager::_setAudioEqualizerBandLevel},
    { "setAudioEqualizerPreset", AudioEffectManager::_setAudioEqualizerPreset},
    { },
};

bool AudioEffectManager::_getAudioEffectList(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _getAudioEffectList");
    LSMessageJsonParser msg(message, STRICT_SCHEMA());
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    pbnjson::JValue listArray = pbnjson::Array();
    bool returnValue = true;
    AudioEffectManager* obj = AudioEffectManager::getAudioEffectManagerInstance();
    int size = AudioEffectManager::audioEffectList.size();

    for (int i = 0; i < size; i++) {

        if (AudioEffectManager::audioEffectList[i].compare("beamforming") == 0) {
            if (obj->isArrayMic) listArray << pbnjson::JValue(AudioEffectManager::audioEffectList[i]);
        } else {
            listArray << pbnjson::JValue(AudioEffectManager::audioEffectList[i]);
        }
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
    std::string effectName;
    bool enabled = false;
    AudioEffectManager* obj = AudioEffectManager::getAudioEffectManagerInstance();
    msg.get("effectName", effectName);
    msg.get("enabled", enabled);

    //  effectId boundary check
    int effectId = getAudioEffectManagerInstance()->getEffectId(effectName);
    int size = AudioEffectManager::audioEffectList.size();
    if (effectId < 0 || effectId >= size) {
        reply = STANDARD_JSON_ERROR(19, "Invalid parameters");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  AudioMixer instance check
    AudioMixer* audioMixer = AudioMixer::getAudioMixerInstance();
    if (!audioMixer) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Invalid AudioMixer Instance");
            reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
            utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
            return true;
    }

    //  previous status check
    if (audioMixer->checkAudioEffectStatus(effectName) == enabled)
    {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"Audio Effect Status is same, not changed");
        reply = STANDARD_JSON_ERROR(7, "Status is same, not changed");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  input device check
    if (obj->inputDevCnt<=0)
    {
        if (effectName.compare("beamforming") == 0 || effectName.compare("speech enhancement") == 0 || effectName.compare("gain control") == 0)
        {
            PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"No input device connected");
            reply = STANDARD_JSON_ERROR(5, "SoundInput not connected");
            utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
            return true;
        }
    }
    //  input device check (array mic)
    if ((obj->isArrayMic == false) && effectName.compare("beamforming") == 0)
    {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Your input device does not support beamforming");
        reply = STANDARD_JSON_ERROR(5, "SoundInput not connected");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  set audio effect enabled
    if (!audioMixer->setAudioEffect(effectName, enabled))
    {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Unexpected AudioMixer error");
        reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  notify to subscribers
    getAudioEffectManagerInstance()->notifyAudioEffectSubscriber();

    //  make json return message
    pbnjson::JValue effectResponse = pbnjson::Object();
    effectResponse.put("returnValue", true);
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
    effectResponse.put("enabled", audioMixer->checkAudioEffectStatus(effectName));
    reply = effectResponse.stringify();
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    return true;

}

bool AudioEffectManager::_getAudioEffectsStatus(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _getAudioEffectsStatus");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(subscribe, boolean))));
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    CLSError lserror;
    bool returnValue = true;
    bool subscribed = false;
    msg.get("subscribe", subscribed);

    if (LSMessageIsSubscription(message))
    {
        if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "LSSubscriptionProcess failed");
            return true;
        }
    }

    reply = getAudioEffectManagerInstance()->getAudioEffectsStatus(subscribed);
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "%s : Reply:%s", __FUNCTION__, reply.c_str());
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
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
        else if (!isOutput && deviceName == "pcm_input")
        {
            inputDevCnt++;
            PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"increment devicecount %d", inputDevCnt);
        }
        for (int i = 0; i < arrayMicListSize; i++)
        {
            if (strcmp(deviceNameDetail.c_str(), arrayMicList[i]) == 0) {
                isArrayMic = true;
                PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"Array mic Attached %s", arrayMicList[i]);
            }
        }
    }
    else if ((deviceStatus == utils::eDeviceDisconnected))
    {
        if(!isOutput && deviceName.find("usb")!=deviceName.npos)
        {
            inputDevCnt--;
            PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"decrement devicecount %d", inputDevCnt);
        }
        else if (!isOutput && deviceName == "pcm_input")
        {

            inputDevCnt--;
            PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"decrement devicecount %d", inputDevCnt);
        }
        for (int i = 0; i < arrayMicListSize; i++)
        {
            if (strcmp(deviceNameDetail.c_str(), arrayMicList[i]) == 0) {
                isArrayMic = false;
                PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT,"Array mic Dettached %s", arrayMicList[i]);
            }
        }
    }
}

void AudioEffectManager::notifyAudioEffectSubscriber()
{
    CLSError lserror;
    std::string reply;

    reply = getAudioEffectManagerInstance()->getAudioEffectsStatus(true);
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "[%s] reply message to subscriber: %s", __FUNCTION__, reply.c_str());
    if (!LSSubscriptionReply(GetPalmService(), AUDIOD_API_GET_AUDIO_EFFECTS_STATUS, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Notify volume subscriber error");
    }
}

std::string AudioEffectManager::getAudioEffectsStatus(const bool &subscribed)
{
    PM_LOG_DEBUG("%s ", __FUNCTION__);
    pbnjson::JValue effectStatusArray = pbnjson::Array();
    int size = AudioEffectManager::audioEffectList.size();

    AudioMixer* audioMixer = AudioMixer::getAudioMixerInstance();
    if (!audioMixer) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Invalid AudioMixer Instance");
        return effectStatusArray.stringify();
    }

    for (int i = 0; i < size; i++)
    {
        pbnjson::JValue effectStatusObject = pbnjson::Object();
        effectStatusObject.put("name", getAudioEffectManagerInstance()->getEffecName(i));
        effectStatusObject.put("enabled", audioMixer->checkAudioEffectStatus(AudioEffectManager::audioEffectList[i]));
        effectStatusArray.append(effectStatusObject);
    }

    pbnjson::JValue returnPayload  = pbnjson::Object();
    returnPayload .put("returnValue", true);
    returnPayload .put("status", effectStatusArray);
    returnPayload .put("subscribed", subscribed);
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, \
                    "%s returning payload = %s", __FUNCTION__, returnPayload .stringify().c_str());
    return returnPayload .stringify();
}

bool AudioEffectManager::_setAudioEqualizerBandLevel(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _setAudioEqualizerBandLevel");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(PROP(band, integer), PROP(level, integer)) REQUIRED_2(band, level)));
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    int band, level;
    msg.get("band", band);
    msg.get("level", level);
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "equalizer params: band[%d] level[%d]", band, level);

    //  AudioMixer instance check
    AudioMixer* audioMixer = AudioMixer::getAudioMixerInstance();
    if (!audioMixer) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Invalid AudioMixer Instance");
            reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
            utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
            return true;
    }

    //  Equalizer enabled check
    if (!audioMixer->checkAudioEffectStatus("equalizer")) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Equalizer feature is not enabled");
        reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  parameter boundary check
    if (band < 0 || band >= audioEffectEqualizerBandSize || level < -audioEffectEqualizerLevelMax || level > audioEffectEqualizerLevelMax) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Equalizer band range [0, 5], level range [-10, 10]");
        reply = STANDARD_JSON_ERROR(19, "Invalid parameters");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  set audio effect param
    if (!audioMixer->setAudioEqualizerParam(-1, band, level)) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Unexpected AudioMixer error");
        reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  make json return message
    pbnjson::JValue effectResponse = pbnjson::Object();
    effectResponse.put("returnValue", true);
    reply = effectResponse.stringify();
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);

    return true;
}

bool AudioEffectManager::_setAudioEqualizerPreset(LSHandle *lshandle, LSMessage *message, void *ctx) {
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "AudioEffectManager: _setAudioEqualizerPreset");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(preset, integer)) REQUIRED_1(preset)));
    if (!msg.parse(__FUNCTION__,lshandle)) return true;

    std::string reply ;
    int preset;
    msg.get("preset", preset);
    PM_LOG_INFO(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "equalizer params: preset[%d]", preset);

    //  AudioMixer instance check
    AudioMixer* audioMixer = AudioMixer::getAudioMixerInstance();
    if (!audioMixer) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Invalid AudioMixer Instance");
            reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
            utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
            return true;
    }

    //  Equalizer enabled check
    if (!audioMixer->checkAudioEffectStatus("equalizer")) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Equalizer feature is not enabled");
        reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  parameter boundary check
    if (preset < 0 || preset >= audioEffectEqualizerPresetSize) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Equalizer preset range [0, 3]");
        reply = STANDARD_JSON_ERROR(19, "Invalid parameters");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  set audio effect param
    if (!audioMixer->setAudioEqualizerParam(preset, -1, 0)) {
        PM_LOG_ERROR(MSGID_AUDIO_EFFECT_MANAGER, INIT_KVCOUNT, "Unexpected AudioMixer error");
        reply = STANDARD_JSON_ERROR(15, "Audiod Internal Error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        return true;
    }

    //  make json return message
    pbnjson::JValue effectResponse = pbnjson::Object();
    effectResponse.put("returnValue", true);
    reply = effectResponse.stringify();
    utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);

    return true;
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
