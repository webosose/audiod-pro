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

#ifndef _AUDIO_ROUTER_H_
#define _AUDIO_ROUTER_H_

#include "utils.h"
#include "messageUtils.h"
#include "audioMixer.h"
#include "moduleInterface.h"
#include "moduleFactory.h"
#include "moduleManager.h"
#include "deviceRoutingConfigParser.h"

#define DEFAULT_ONE_DISPLAY_ID 0
#define DEFAULT_TWO_DISPLAY_ID 1
#define DEFAULT_THREE_DISPLAY_ID 2
#define HOST_SESSION        "host"
#define AVN_SESSION         "AVN"
#define RSE_LEFT_SESSION    "RSE-L"
#define RSE_RIGHT_SESSION   "RSE-R"
#define AVN "avn"
#define RSI0 "rsi0"
#define RSI1 "rsi1"

#define MAX_SESSIONS        3

#define AUDIOD_API_GET_SOUNDOUT "/getSoundOutput"
#define AUDIOD_API_GET_SOUNDINPUT "/getSoundInput"
//used only for BT sink remapping
#define BT_DISPLAY_ONE 1
#define BT_DISPLAY_TWO 2
#define BLUETOOTH_ONE "bluetooth_speaker0"
#define BLUETOOTH_TWO "bluetooth_speaker1"
#define BLUETOOTH_SINK_IDENTIFIER "bluez_sink."
#define BT_SINK_IDENTIFIER_LENGTH 11
#define BT_DEVICE_ADDRESS_LENGTH 17

#define DEFAULT_DISPLAY "display1"

#define USB_DEVICE_TYPE_NAME "usb"
#define MULTIPLE_DEVICE_MAX 10
#define MULTIPLE_DEVICE_MIN 1

struct BTInfo
{
    std::string deviceName;
    std::string deviceNameDetail;
    std::string deviceIcon;
};


class AudioRouter : public ModuleInterface
{
    private:
        AudioRouter(AudioRouter const&) = delete;
        AudioRouter& operator=(AudioRouter const&) = delete;
        ModuleManager* mObjModuleManager;
        AudioMixer* mObjAudioMixer;
        AudioRouter(ModuleConfig* const pConfObj);
        static bool mIsObjRegistered;
        bool mSoundDevicesLoaded;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_audio_router_manager", &AudioRouter::CreateObject));
        }

        DeviceRoutingConfigParser* mObjDeviceRoutingParser;

        utils::mapDisplaySoundOutputInfo mDisplaySoundOutputInfo;

        utils::mapSinkRoutingInfo mMapSinkRoutingInfo;
        utils::mapSourceRoutingInfo mMapSourceRoutingInfo;

        utils::mapBTDeviceInfo mMapBTDeviceInfo;

        utils::mapDeviceInfo mSoundOutputInfo;
        utils::mapDeviceInfo mSoundInputInfo;

        utils::mapMultipleDeviceInfo mMutipleOutputInfo;
        utils::mapMultipleDeviceInfo mMutipleInputInfo;

        std::list<BTInfo> mBTDeviceList;
        utils::mapSoundDevicesInfo mSoundOutputDeviceInfo;
        utils::mapSoundDevicesInfo mSoundInputDeviceInfo;

        void setOutputDeviceRouting(const std::string &deviceName, const int &priority,\
            const std::string &display, utils::EMIXER_TYPE mixerType);
        void resetOutputDeviceRouting(const std::string &deviceName, const int &priority,\
            const std::string &display, utils::EMIXER_TYPE mixerType);
        void setOutputDeviceRoutingWithMirror(const std::string &deviceName, const int &priority,\
            const std::string &display, utils::EMIXER_TYPE mixerType);
        void resetOutputDeviceRoutingWithMirror(const std::string &deviceName, const int &priority,\
            const std::string &display, utils::EMIXER_TYPE mixerType);
        void setInputDeviceRouting(const std::string &deviceName, const int &priority,\
            const std::string &display, utils::EMIXER_TYPE mixerType);
        void resetInputDeviceRouting(const std::string &deviceName, const int &priority,\
            const std::string &display, utils::EMIXER_TYPE mixerType);
        void updateDeviceStatus(const std::string& display, const std::string& deviceName,\
            const bool& isConnected, bool const& isActive, const bool& isOutput);
        void printDeviceInfo(const bool& isOutput);
        void setDeviceRoutingInfo(const pbnjson::JValue& deviceRoutingInfo);
        void readDeviceRoutingInfo();
        void setBTDeviceRouting(const std::string &deviceName);
        int getPriority(const std::string& display, const std::string &deviceName, const bool& isOutput);
        int getDisplayId(const std::string &deviceName, const bool &isOutput);
        void notifyGetSoundoutput(const std::string& soundoutput, const std::string& display);
        void notifyGetSoundInput(const std::string& soundInput, const std::string& display);
        std::string getPriorityDevice(const std::string& display, const bool& isOutput);
        std::string getActiveDevice(const std::string& display, const bool& isOutput);
        std::string getActualOutputDevice(const std::string &deviceName);
        utils::SINK_ROUTING_INFO_T getSinkRoutingInfo(const std::string &display);
        utils::SOURCE_ROUTING_INFO_T getSourceRoutingInfo(const std::string &display);

        int getNotificationSessionId(const std::string &displayId);
    public:

        void eventSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType);
        void eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType);
        void eventDeviceConnectionStatus(const std::string &deviceName, const std::string &deviceNameDetail, const std::string &deviceIcon, \
            utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType, const bool& isOutput);
        void eventSinkPolicyInfo(const pbnjson::JValue& sinkPolicyInfo);
        void eventSourcePolicyInfo(const pbnjson::JValue& sourcePolicyInfo);
        void eventBTDeviceDisplayInfo(const bool &connectionStatus, const std::string &deviceAddress, const int &displayId);
        std::string getSoundDeviceList(bool subscribed, const std::string &query);
        void eventResponseSoundDevicesInfo(bool isOutput);
        void setSoundDeviceInfo(bool isOutput);
        utils::mapSoundDevicesInfo getSoundDeviceInfo(bool isOutput);
        bool setSoundOutput(const std::string& soundOutput, const int &displayId);
        bool setSoundInput(const std::string& soundInput, const int &displayId);
        std::string getDisplayName(const int &displayId);
        std::string getSoundDeviceInfo(bool subscribed, const std::string &query);

        void notifyDeviceListSubscribers();

        //luna api
        static bool _setSoundOutput(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _setSoundInput(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getSoundInput(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getSoundOutput(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _SetSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _listSupportedDevices(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _updateSoundOutStatus(LSHandle *sh, LSMessage *reply, void *ctx);

        ~AudioRouter();
        static AudioRouter* mAudioRouter ;
        static AudioRouter* getAudioRouterInstance();
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the AudioRouter handler");
                mAudioRouter = new(std::nothrow) AudioRouter(pConfObj);
                if (mAudioRouter)
                    return mAudioRouter;
            }
            return nullptr;
        }
        void initialize();
        void deInitialize();
        void handleEvent(events::EVENTS_T *event);
};

#endif