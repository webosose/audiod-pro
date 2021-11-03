// Copyright (c) 2021-2022 LG Electronics, Inc.
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


#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "utils.h"

namespace events
{
    typedef struct
    {
        EModuleEventType eventName;
        std::string source;
        std::string sink;
        EVirtualAudioSink audioSink;
        utils::ESINK_STATUS sinkStatus;
        utils::EMIXER_TYPE mixerType;
        int sinkIndex;
        std::string trackId;
    }EVENT_SINK_STATUS_T;

    typedef struct
    {
        EModuleEventType eventName;
        std::string source;
        std::string sink;
        EVirtualSource audioSource;
        utils::ESINK_STATUS sourceStatus;
        utils::EMIXER_TYPE mixerType;
    }EVENT_SOURCE_STATUS_T;

    typedef struct
    {
        EModuleEventType eventName;
        std::string trackId;
        int sinkIndex;
        int sinkId;
    }EVENT_SINK_APP_ID;

    typedef struct
    {
        EModuleEventType eventName;
        EModuleEventType type;
        LSMessage *message;
    }EVENT_KEY_INFO_T;

    typedef struct
    {
        EModuleEventType eventName;
        EModuleEventType type;
        SERVER_TYPE_E serviceName;
        std::string api;
        std::string payload;
    }EVENT_SUBSCRIBE_KEY_T;

    typedef struct
    {
        EModuleEventType eventName;
        SERVER_TYPE_E serviceName;
        bool connectionStatus;
    }EVENT_SERVER_STATUS_INFO_T;

    typedef struct
    {
        EModuleEventType eventName;
        bool mixerStatus;
        utils::EMIXER_TYPE mixerType;
    }EVENT_MIXER_STATUS_T;

    typedef struct
    {
        EModuleEventType eventName;
        EVirtualAudioSink audioSink;
        int volume;
        bool ramp;
    }EVENT_INPUT_VOLUME_T;

    typedef struct
    {
        EModuleEventType eventName;
    }EVENT_MASTER_VOLUME_STATUS_T;

    typedef struct
    {
        EModuleEventType eventName;
        SERVER_TYPE_E serviceName;
    }EVENT_SUBSCRIBE_SERVER_STATUS_T;

    typedef struct
    {
        EModuleEventType eventName;
        pbnjson::JValue policyInfo;
    }EVENT_SINK_POLICY_INFO_T;

    typedef struct
    {
        EModuleEventType eventName;
        pbnjson::JValue sourcePolicyInfo;
    }EVENT_SOURCE_POLICY_INFO_T;

    typedef struct
    {
        EModuleEventType eventName;
        std::string devicename;
        utils::E_DEVICE_STATUS deviceStatus;
        utils::EMIXER_TYPE mixerType;
    }EVENT_DEVICE_CONNECTION_STATUS_T;

    typedef struct
    {
        EModuleEventType eventName;
        std::string deviceName;
        std::string display;
        bool isConnected;
        bool isOutput;
    }EVENT_ACTIVE_DEVICE_INFO_T;

    typedef struct
    {
        EModuleEventType eventName;
        bool state;
        std::string address;
        int displayId;
    }EVENT_BT_DEVICE_DISPAY_INFO_T;

    typedef struct
    {
        EModuleEventType eventName;
        std::string trackId;
        std::string streamType;
    }EVENT_REGISTER_TRACK_T;

    typedef struct
    {
        EModuleEventType eventName;
        std::string trackId;
    }EVENT_UNREGISTER_TRACK_T;


    typedef struct
    {
        EModuleEventType eventName;
    }EVENTS_T;
}

#endif //_EVENTS_H_