/*
 * Copyright (c) 2022-2023 LG Electronics Inc.
 * SPDX-License-Identifier: LicenseRef-LGE-Proprietary
 */

#ifndef _AUDIO_EFFECT_MANAGER_H_
#define _AUDIO_EFFECT_MANAGER_H_

#include "utils.h"
#include "moduleManager.h"
#include "audioMixer.h"
#include "moduleFactory.h"

#define AUDIOD_API_GET_AUDIO_EFFECTS_STATUS    "/getAudioEffectsStatus"

class AudioEffectManager : public ModuleInterface
{
    private:
        AudioEffectManager(const AudioEffectManager&) = delete;
        AudioEffectManager& operator=(const AudioEffectManager&) = delete;
        AudioEffectManager(ModuleConfig* const pConfObj);

        ModuleManager* mObjModuleManager;
        AudioMixer *mObjAudioMixer;
        static bool mIsObjRegistered;

        //  change audio effect list size and add to list
        static const int audioEffectListSize = 5;
        static constexpr const char* audioEffectList[] = {
            "speech enhancement",
            "gain control",
            "beamforming",
            "dynamic compressor",
            "equalizer"
        };

        //  equalizer configuration
        static const int audioEffectEqualizerBandSize = 6;
        static const int audioEffectEqualizerLevelMax = 10;
        static const int audioEffectEqualizerPresetSize = 4;

        //  Register Object to object factory. This is called automatically
        static bool RegisterObject() {
            PM_LOG_DEBUG("%s, RegisterObject - Registering the AudioEffectManager", MSGID_AUDIO_EFFECT_MANAGER);
            return (ModuleFactory::getInstance()->Register("load_audio_effect_manager", &AudioEffectManager::CreateObject));
        }

        int getEffectId(std::string effectName);
        std::string getEffecName(int effectId);
        int inputDevCnt;
        static const int arrayMicListSize = 1;
        static constexpr const char* arrayMicList[] = {
            "seeed-4mic-voicecard",
        };
        bool isArrayMic;

    public:
        ~AudioEffectManager();
        static AudioEffectManager* getAudioEffectManagerInstance();
        static AudioEffectManager* mAudioEffectManager;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj) {
            if (mIsObjRegistered) {
                PM_LOG_DEBUG("%s, CreateObject - Creating the AudioEffectManager handler", MSGID_AUDIO_EFFECT_MANAGER);
                mAudioEffectManager = new(std::nothrow) AudioEffectManager(pConfObj);
                if (mAudioEffectManager) {
                    return mAudioEffectManager;
                }
            }
            return nullptr;
        }
        void initialize();
        void deInitialize();
        void setConnStatus(std::string deviceName, int display, bool isOutput, bool connStatus);
        void handleEvent(events::EVENTS_T *event);

        //  luna API calls
        static LSMethod audioeffectMethods[];
        static bool _getAudioEffectList(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _setAudioEffect(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _checkAudioEffectStatus(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getAudioEffectsStatus(LSHandle *lshandle, LSMessage *message, void *ctx);

        static bool _setAudioEqualizerBandLevel(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _setAudioEqualizerPreset(LSHandle *lshandle, LSMessage *message, void *ctx);

        std::string getAudioEffectsStatus(const bool &subscribed);
        void eventDeviceConnectionStatus(const std::string &deviceName , const std::string &deviceNameDetail, const std::string &deviceIcon, \
            utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType, const bool &isOutput);
        void notifyAudioEffectSubscriber();
};

#endif  //  _AUDIO_EFFECT_MANAGER_H_
