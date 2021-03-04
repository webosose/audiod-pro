// Copyright (c) 2021 LG Electronics, Inc.
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

#ifndef _OSE_MASTER_VOLUME_MGR_H_
#define _OSE_MASTER_VOLUME_MGR_H_

#include "masterVolumeInterface.h"
#include "masterVolumeManager.h"

class OSEMasterVolumeManager : public MasterVolumeInterface
{
    private:
        OSEMasterVolumeManager(const OSEMasterVolumeManager&) = delete;
        OSEMasterVolumeManager& operator=(const OSEMasterVolumeManager&) = delete;
        static bool mIsObjRegistered;
        static bool CreateInstance()
        {
            PM_LOG_INFO("OSEMasterVolumeManager", INIT_KVCOUNT, "OSEMasterVolumeManager: CreateInstance");
            MasterVolumeManager::setInstance(getInstance());
            return true;
        }
        int mVolume;
        int displayOneVolume;
        int displayTwoVolume;
        int displayOneMuteStatus;
        int displayTwoMuteStatus;
        bool mMuteStatus;

    public :
        ~OSEMasterVolumeManager();
        OSEMasterVolumeManager();
        static OSEMasterVolumeManager* getInstance();
        void setCurrentVolume(int iVolume);
        void notifyVolumeSubscriber(const int &displayId,const std::string &callerId);
        std::string getVolumeInfo(const int &displayId, const std::string &callerId);

        void setVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void getVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx);
        void volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx);

        //Luna API callbacks
        static bool _setVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _muteVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _volumeUpCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _volumeDownCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
};
#endif