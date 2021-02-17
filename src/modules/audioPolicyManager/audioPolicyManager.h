/* @@@LICENSE
*
*      Copyright (c) 2020-2021 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef _AUDIO_POLICY_MGR_H_
#define _AUDIO_POLICY_MGR_H_

#include "utils.h"
#include "messageUtils.h"
#include "main.h"
#include <cstdlib>
#include "moduleInterface.h"
#include "moduleFactory.h"
#include "moduleManager.h"
#include "volumePolicyInfoParser.h"
#include "audioMixer.h"

class AudioPolicyManager : public ModuleInterface
{
    private:
        AudioPolicyManager(const AudioPolicyManager&) = delete;
        AudioPolicyManager& operator=(const AudioPolicyManager&) = delete;
        ModuleManager* mObjModuleManager;
        VolumePolicyInfoParser* mObjPolicyInfoParser;
        AudioMixer* mObjAudioMixer;
        std::vector<utils::VOLUME_POLICY_INFO_T> mVolumePolicyInfo;
        utils::mapSinkToStream mSinkToStream;
        utils::mapStreamToSink mStreamToSink;
        static bool mIsObjRegistered;
        AudioPolicyManager(ModuleConfig* const pConfObj);
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_audio_policy_manager", &AudioPolicyManager::CreateObject));
        }
        void readPolicyInfo();
        void printPolicyInfo();
        void printActivePolicyInfo();
        void createMapSinkToStream();
        void createMapStreamToSink();
        void updateCurrentVolume(const std::string& streamType, const int &volume);
        void applyVolumePolicy(EVirtualAudioSink audioSink, const std::string& streamType, const int& priority);
        void removeVolumePolicy(EVirtualAudioSink audioSink, const std::string& streamType, const int& priority);
        void updatePolicyStatus(const std::string& streamType, const bool& status);
        void initStreamVolume();
        bool setVolume(EVirtualAudioSink audioSink, const int &volume, utils::EMIXER_TYPE mixerType, bool ramp = false);
        bool initializePolicyInfo(const pbnjson::JValue& policyInfo);
        bool getPolicyStatus(const std::string& streamType);
        bool isHighPriorityStreamActive(const int& policyPriority, utils::vectorVirtualSink activeStreams);
        bool isRampPolicyActive(const std::string& streamType);
        bool getStreamActiveStatus(const std::string& streamType);
        int getPriority(const std::string& streamType);
        int getPolicyVolume(const std::string& streamType);
        int getCurrentVolume(const std::string& streamType);
        utils::EMIXER_TYPE getMixerType(const std::string& streamType);
        EVirtualAudioSink getSinkType(const std::string& streamType);
        std::string getStreamType(EVirtualAudioSink audioSink);
        std::string getCategory(const std::string& streamType);

    public:
        ~AudioPolicyManager();
        static AudioPolicyManager* getAudioPolicyManagerInstance();
        static AudioPolicyManager *mAudioPolicyManager;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the AudioPolicyManager handler");
                mAudioPolicyManager = new(std::nothrow) AudioPolicyManager(pConfObj);
                if (mAudioPolicyManager)
                    return mAudioPolicyManager;
            }
            return nullptr;
        }
        void initialize();

        //luna api's
        static bool _setInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getStreamStatus(LSHandle *lshandle, LSMessage *message, void *ctx);

        void notifyGetVolumeSubscribers(const std::string& streamType, const int& volume);
        void notifyGetStreamStatusSubscribers(const std::string& payload) const;
        static bool _setMediaInputVolume(LSHandle *lshandle, LSMessage *message, void *ctx);

        void eventSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType);
        void eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType);
        void eventCurrentInputVolume(EVirtualAudioSink audioSink, const int& volume);
        std::string getStreamStatus(const std::string& streamType, bool subscribed);
        std::string getStreamStatus(bool subscribed);
};
#endif //_AUDIO_POLICY_MGR_H_
