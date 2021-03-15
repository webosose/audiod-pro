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
    }EVENT_SINK_STATUS_T;

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
    }EVENTS_T;
}

#endif //_EVENTS_H_