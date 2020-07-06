// Copyright (c) 2020 LG Electronics, Inc.
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

#include "moduleManager.h"

ModuleManager* ModuleManager::getModuleManagerInstance()
{
    static ModuleManager moduleManagerInstance;
    return &moduleManagerInstance;
}

ModuleManager::ModuleManager()
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager constructor");
}

ModuleManager::~ModuleManager()
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager destructor");
}

void ModuleManager::subscribeModuleEvent(ModuleInterface* module, bool first, utils::EVENT_TYPE_E eventType)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "subscribeModuleEvent");
    switch (eventType)
    {
        case utils::eEventSinkStatus:
        {
            PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                    "subscribeModuleEvent:: eEventSinkStatus");
            if (first)
                listSinkStatusSubscribers.push_front(module);
            else
                listSinkStatusSubscribers.push_back(module);
        }
        break;
        case utils::eEventMixerStatus:
        {
            PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                "subscribeModuleEvent:: eEventMixerStatus");
            if (first)
                listMixerStatusSubscribers.push_front(module);
            else
                listMixerStatusSubscribers.push_back(module);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                "subscribeModuleEvent:Unknown event");
        }
        break;
    }
}

void ModuleManager::notifySinkStatusInfo(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager:%s source:%s sink:%s, sinkId:%d Sink status:%d Mixer type:%d",\
        __FUNCTION__, source.c_str(), sink.c_str(), (int)audioSink, sinkStatus, mixerType);
    for (const auto &it : listSinkStatusSubscribers)
        it->eventSinkStatus(source, sink, audioSink, sinkStatus, mixerType);
}

void ModuleManager::notifyMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager:notifyMixerStatus status:%d type:%d", mixerStatus, mixerType);
    for (const auto &it:listMixerStatusSubscribers)
    {
        it->eventMixerStatus(mixerStatus, mixerType);
    }
}