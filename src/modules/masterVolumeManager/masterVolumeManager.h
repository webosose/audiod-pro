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
#include "masterVolumeInterface.h"

class MasterVolumeManager : public ModuleInterface
{
    private:
        MasterVolumeManager(const MasterVolumeManager&) = delete;
        MasterVolumeManager& operator=(const MasterVolumeManager&) = delete;
        MasterVolumeManager(ModuleConfig* const pConfObj);

        ModuleManager* mObjModuleManager;
        AudioMixer *mObjAudioMixer;
        static SoundOutputManager *mObjSoundOutputManager;
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
        static MasterVolumeInterface* mClientMasterInstance;
        static void setInstance(MasterVolumeInterface* clientMasterVolumeInstance);
        MasterVolumeInterface* getClientMasterInstance();
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
        void handleEvent(events::EVENTS_T* ev);
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
};
#endif