// Copyright (c) 2022- LG Electronics, Inc.
//

#ifndef _AUDIO_EFFECT_MANAGER_H_
#define _AUDIO_EFFECT_MANAGER_H_

#include "utils.h"
#include "moduleManager.h"
#include "audioMixer.h"
#include "moduleFactory.h"

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
        static const int audioEffectListSize = 1;
        static constexpr const char* audioEffectList[] = {
            "speech enhancement",
        };

        //  Register Object to object factory. This is called automatically
        static bool RegisterObject() {
            PM_LOG_DEBUG("%s, RegisterObject - Registering the AudioEffectManager", MSGID_AUDIO_EFFECT_MANAGER);
            return (ModuleFactory::getInstance()->Register("load_audio_effect_manager", &AudioEffectManager::CreateObject));
        }

        int getEffectId(std::string effectName);

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
        void handleEvent(events::EVENTS_T* ev);

        //  luna API calls
        static LSMethod audioeffectMethods[];
        static bool _getAudioEffectList(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _setAudioEffect(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _checkAudioEffectStatus(LSHandle *lshandle, LSMessage *message, void *ctx);
};

#endif  //  _AUDIO_EFFECT_MANAGER_H_
