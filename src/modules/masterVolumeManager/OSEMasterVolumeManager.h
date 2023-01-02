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
#include <list>
#include <map>

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
#define STORED_VOLUME_SIZE 15
#define DEFAULT_INITIAL_VOLUME 90

#define MUTE_MIC_BOTH_DISPLAY 3

struct deviceInfo
{
    std::string deviceName;
    std::string deviceNameDetail;
    int volume;
    bool connected;
    deviceInfo():connected(false),volume(100){}
    deviceInfo(std::string deviceName, std::string deviceNameDetail,int volume):deviceName(deviceName),
        deviceNameDetail(deviceNameDetail), volume(volume),connected(false) { }
};

struct masterVolumeCallbackDetails
{
    int volume;
    char deviceName[50];
    void *context;
    char isOutput;
};
struct connectedDevices
{
    std::string deviceName;
    std::string deviceNameDetail;
    bool isOutput;
};

struct deviceDetail
{
    std::string deviceNameDetail;
    int volume;
    int display;
    bool isConnected;
    bool isActive;
    bool muteStatus;
    bool isOutput;

    deviceDetail(bool isOutput)
    {
        isConnected = false;
        isActive = false;
        this->isOutput = isOutput;
        muteStatus = false;
    }
};

class OSEMasterVolumeManager : public MasterVolumeInterface
{
    private:
        OSEMasterVolumeManager(const OSEMasterVolumeManager&) = delete;
        OSEMasterVolumeManager& operator=(const OSEMasterVolumeManager&) = delete;

        static bool mIsObjRegistered;
        utils::mapSoundDevicesInfo msoundOutputDeviceInfo;
        utils::mapSoundDevicesInfo msoundInputDeviceInfo;
        std::map<std::string,bool> mapActiveDevicesInfo;

        std::string bluetoothName;

        std::list<deviceInfo> mExtOutputDeviceListDB;
        std::list<deviceInfo> mIntOutputDeviceListDB;
        std::list<deviceInfo> mIntInputDeviceListDB;
        std::list<deviceInfo> mExtInputDeviceListDB;
        bool mCacheRead;
        std::map<std::string,std::string> deviceNameMap;
        std::map<std::string,int> deviceVolumeMap;

        std::map<std::string, deviceDetail> deviceDetailMap;

        std::list<std::string> mInternalOutputDeviceList;
        std::list<std::string> mInternalInputDeviceList;
        std::list<connectedDevices> mConnectedDevicesList;

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
        void notifyVolumeSubscriber(const std::string &soundOutput, const int &displayId,const std::string &callerId);
        void notifyMicVolumeSubscriber(const std::string &soundInput, const int &displayId, bool subscribed);

        std::string getVolumeInfo(const std::string &soundOutput, const int &displayId, const std::string &callerId);
        std::string getMicVolumeInfo(const std::string &soundInput, const int &displayId, bool subscribed);
        void setSoundOutputInfo(utils::mapSoundDevicesInfo soundOutputInfo);
        void setSoundInputInfo(utils::mapSoundDevicesInfo soundInputInfo);
        int getDisplayId(const std::string &displayName);
        bool readInitialVolume(pbnjson::JValue settingsObj);
        bool updateMasterVolumeInMap(std::string deviceName, int displayId, int volume,bool isOutput);
        bool isInternalDevice(std::string device, bool isOutput);
        int getVolumeFromDB(std::string deviceName, bool isOutput, bool isInternal, bool &isFound);
        void updateConnStatusAndReorder(std::string deviceName, bool isOutput, bool isInternal, bool isConnected);
        void addNewItemToDB(std::string deviceName, std::string deviceNameDetail, int volume, bool isOutput);
        void printDb();
        void deviceConnectOp(std::string deviceName, std::string deviceNameDetail,bool isOutput);
        void deviceDisconnectOp(std::string deviceName, std::string deviceNameDetail,bool isOutput);
        std::list<deviceInfo>&getDBListRef(bool isInternal, bool isOutput);
        void sendDataToDB();

        void setActiveStatus(std::string deviceName, int display, bool isOutput, bool isActive);
        std::string getActiveDevice(int display, bool isOutput);
        void setConnStatus(std::string deviceName, int display, bool isOutput, bool connStatus);
        void setDeviceNameDetail(std::string deviceName, int display, bool isOutput, std::string deviceNameDetail);
        std::string getDeviceNameDetail(std::string deviceName, int display, bool isOutput);
        bool getConnStatus(std::string deviceName, int display, bool isOutput);
        void setDeviceVolume(std::string deviceName, int display, bool isOutput, int volume);
        int getDeviceVolume(std::string deviceName, int display, bool isOutput);
        void setDeviceMute(std::string deviceName, int display, bool isOutput, bool mute);
        bool getDeviceMute(std::string deviceName, int display, bool isOutput);
        int getDeviceDisplay(std::string deviceName, bool isOutput);
        bool isValidSoundDevice(std::string deviceName, bool isOutput);

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

        static bool DBSetVoulumeCallbackPA(LSHandle *sh, LSMessage *reply, void *ctx, bool status);
        static bool VolumeFromSettingService(LSHandle *sh, LSMessage *reply, void *ctx);

        void handleEvent(events::EVENTS_T *event);
        void requestInternalDevices();
        void eventMasterVolumeStatus();
        void eventServerStatusInfo (SERVER_TYPE_E serviceName, bool connected);
        void eventActiveDeviceInfo(const std::string deviceName,\
            const std::string& display, const bool& isConnected, const bool& isOutput, const bool& isActive);
        void requestSoundOutputDeviceInfo();
        void requestSoundInputDeviceInfo();
        void eventResponseSoundOutputDeviceInfo(utils::mapSoundDevicesInfo soundOutputInfo);
        void eventResponseSoundInputDeviceInfo(utils::mapSoundDevicesInfo soundInputInfo);
        void eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType);
        void eventDeviceConnectionStatus(const std::string &deviceName , const std::string &deviceNameDetail,\
            utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType, const bool &isOutput);
};
#endif