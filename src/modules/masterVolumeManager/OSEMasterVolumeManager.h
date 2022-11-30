// Copyright (c) 2021-2023 LG Electronics, Inc.
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
#include "audioMixer.h"
#define AUDIOD_API_GET_VOLUME                          "/master/getVolume"
#define AUDIOD_API_GET_MIC_VOLUME                      "/master/getMicVolume"
#define MSGID_CLIENT_MASTER_VOLUME_MANAGER             "OSE_MASTER_VOLUME_MANAGER"         //Client Master Volume Manager
#define DISPLAY_ONE 0
#define DISPLAY_TWO 1
#define MIN_VOLUME 0
#define MAX_VOLUME 100
#define DEFAULT_ONE_DISPLAY_ID 1
#define DEFAULT_TWO_DISPLAY_ID 2
#define DISPLAY_ONE_NAME "display1"
#define DISPLAY_TWO_NAME "display2"

#define MUTE_MIC_BOTH_DISPLAY 3

class OSEMasterVolumeManager : public MasterVolumeInterface
{
    private:
        OSEMasterVolumeManager(const OSEMasterVolumeManager&) = delete;
        OSEMasterVolumeManager& operator=(const OSEMasterVolumeManager&) = delete;
        int mVolume;
        int displayOneVolume;
        int displayTwoVolume;
        int displayOneMuteStatus;
        int displayTwoMuteStatus;
        int displayOneMicVolume;
        int displayTwoMicVolume;
        int displayOneMicMuteStatus;
        int displayTwoMicMuteStatus;
        std::string displayOneSoundoutput;
        std::string displayTwoSoundoutput;
        std::string displayOneSoundinput;
        std::string displayTwoSoundinput;
        std::string getDisplaySoundOutput(const int& display);
        std::string getDisplaySoundInput(const int& display);
        bool mMuteStatus;
        static bool mIsObjRegistered;
        utils::mapSoundDevicesInfo msoundOutputDeviceInfo;
        utils::mapSoundDevicesInfo msoundInputDeviceInfo;
        std::map<std::string,bool> mapActiveDevicesInfo;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (MasterVolumeInterface::Register(&OSEMasterVolumeManager::CreateObject));
        }

    public :
        ~OSEMasterVolumeManager();
        OSEMasterVolumeManager();
        static MasterVolumeInterface* CreateObject()
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the OSEMasterVolumeManager handler");
                return new(std::nothrow) OSEMasterVolumeManager();
            }
            return nullptr;
        }
        void setCurrentVolume(int iVolume);
        void setCurrentMuteStatus(bool bMuteStatus);
        void notifyVolumeSubscriber(const std::string &soundOutput, const int &displayId,const std::string &callerId);
        void notifyMicVolumeSubscriber(const std::string &soundInput, const int &displayId, bool subscribed);
        void setVolume(const int &displayId);
        void setMicVolume(const int &displayId, LSHandle *lshandle, LSMessage *message, void *ctx);
        void setMuteStatus(const int &displayId);
        void setDisplaySoundOutput(const std::string& display, const std::string& soundOutput, const bool& isConnected);
        void setDisplaySoundInput(const std::string& display, const std::string& soundInput, const bool& isConnected);
        std::string getVolumeInfo(const std::string &soundOutput, const int &displayId, const std::string &callerId);
        std::string getMicVolumeInfo(const std::string &soundInput, const int &displayId, bool subscribed);
        void setSoundOutputInfo(utils::mapSoundDevicesInfo soundOutputInfo);
        void setSoundInputInfo(utils::mapSoundDevicesInfo soundInputInfo);
        int getDisplayId(const std::string &displayName);

        void setVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void setMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void getVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void getMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
        void muteMic(LSHandle *lshandle, LSMessage *message, void *ctx);
        void volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx);
        void volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx);

        //Luna API callbacks for pulseaudio calls
        static bool _setMicVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
        static bool _muteVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
        static bool _setVolumeCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
        static bool _volumeUpCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
        static bool _volumeDownCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
        static bool _muteMicCallBackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
};
#endif