// Copyright (c) 2020-2021 LG Electronics, Inc.
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

#ifndef _MASTER_VOLUME_MGR_H_
#define _MASTER_VOLUME_MGR_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <string>
#include "utils.h"
#include "messageUtils.h"
#include "moduleInterface.h"
#include "moduleManager.h"
#include "audioMixer.h"
#include "soundOutputManager.h"
#include "moduleFactory.h"

#define AUDIOD_API_GET_VOLUME "/master/getVolume"
#define DISPLAY_ONE 0
#define DISPLAY_TWO 1
#define MIN_VOLUME 0
#define MAX_VOLUME 100
#define DEFAULT_ONE_DISPLAY_ID 1
#define DEFAULT_TWO_DISPLAY_ID 2

class MasterVolumeManager : public ModuleInterface
{
    private:
        MasterVolumeManager(const MasterVolumeManager&) = delete;
        MasterVolumeManager& operator=(const MasterVolumeManager&) = delete;
        MasterVolumeManager(ModuleConfig* const pConfObj);

        ModuleManager* mObjModuleManager;
        AudioMixer *mObjAudioMixer;
        static SoundOutputManager *mObjSoundOutputManager;
        int mVolume;
        int displayOneVolume;
        int displayTwoVolume;
        int displayOneMuteStatus;
        int displayTwoMuteStatus;
        bool mMuteStatus;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_master_volume_manager", &MasterVolumeManager::CreateObject));
        }

    public :
        ~MasterVolumeManager();
        static MasterVolumeManager* getMasterVolumeManagerInstance();
        static MasterVolumeManager *mMasterVolumeManager;
        static void initSoundOutputManager();
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the MasterVolumeManager handler");
                mMasterVolumeManager = new(std::nothrow) MasterVolumeManager(pConfObj);
                if (mMasterVolumeManager)
                    return mMasterVolumeManager;
            }
            return nullptr;
        }
        void initialize();

        void setCurrentVolume(int iVolume);
        void setCurrentMuteStatus(bool bMuteStatus);
        void notifyVolumeSubscriber(const int &displayId,const std::string &callerId);
        std::string getVolumeInfo(const int &displayId, const std::string &callerId);
        void setVolume(const int &displayId);
        void setMuteStatus(const int &displayId);
        void eventMasterVolumeStatus();

        //Internal API for Volume and Mute, will be implemented during dynamic audio policy handling redesign
        bool _setVolume(int volume, bool notifySubscriber = true){return true;}
        bool _muteVolume(bool mute){return true;}

        //luna API calls
        static bool _setVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx);

        //Luna API callbacks
        static bool _setVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _muteVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _volumeUpCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _volumeDownCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
};
#endif