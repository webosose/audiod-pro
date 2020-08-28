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
        case utils::eEventMasterVolumeStatus:
        {
            PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                       "subscribeModuleEvent:: eEventMasterVolumeStatus");
            if (first)
                listMasterVolumeStatusSubscribers.push_front(module);
            else
                listMasterVolumeStatusSubscribers.push_back(module);
        }
        break;
        case utils::eEventServerStatusSubscription:
        {
            PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                    "subscribeModuleEvent:: eEventServerStatusSubscription");
            if (first)
                listServerStatusSubscriptionSubscribers.push_front(module);
            else
                listServerStatusSubscriptionSubscribers.push_back(module);
        }
        break;
        case utils::eEventKeySubscription:
        {
            PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                    "subscribeModuleEvent:: eEventKeySubscription");
            if (first)
                listKeySubscriptionSubscribers.push_front(module);
            else
                listKeySubscriptionSubscribers.push_back(module);
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

void ModuleManager::subscribeKeyInfo(ModuleInterface* module, bool first, LUNA_KEY_TYPE_E event,
SERVER_TYPE_E eService, const std::string& key, const std::string& payload)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, \
        "subscribeKeyInfo called %d:%d", event, eService );
    bool success = true;
    if (!(eService >= eServiceFirst && eService < eServiceCount))
    {
        PM_LOG_ERROR(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "not Valid service");
        success = false;
    }
    if (!(event >= eLunaEventKeyFirst && event < eLunaEventCount))
    {
        PM_LOG_ERROR(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "not Valid Subscription");
        success = false;
    }
    if (success)
    {
        bool found = (std::find(listKeyInfoSubscribers[event].begin(),
            listKeyInfoSubscribers[event].end(), module) != listKeyInfoSubscribers[event].end());
        if (!found)
        {
            if (first)
                listKeyInfoSubscribers[event].push_front(module);
            else
                listKeyInfoSubscribers[event].push_back(module);
        }
        //if payload is required from Module during runtime
        notifyKeySubscription(event, eService, key, payload);
    }
}

void ModuleManager::subscribeServerStatusInfo(ModuleInterface* module, bool first, SERVER_TYPE_E eStatus)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, \
        "subscribeServerStatusInfo called");
    if (!(eStatus >= eBluetoothService && eStatus < eServiceCount))
    {
        PM_LOG_WARNING(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "not Valid service");
    }
    else
    {
        if (first)
            listServerStatusInfoSubscribers[eStatus].push_front(module);
        else
            listServerStatusInfoSubscribers[eStatus].push_back(module);
    }
    notifyServerStatusSubscription(eStatus);
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

void ModuleManager::notifyInputVolume(EVirtualAudioSink audioSink, const int& volume, const bool& ramp)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "notifyInputVolume sink : %d, volume : %d, ramp : %d", (int)audioSink, volume, ramp);
    for (const auto &it:listInputVolumeSubscribers)
    {
        it->eventInputVolume(audioSink, volume, ramp);
    }
}

void ModuleManager::notifyMasterVolumeStatus()
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
                "ModuleManager: notifyMasterVolumeStatus");
    for (const auto &it:listMasterVolumeStatusSubscribers)
    {
        it->eventMasterVolumeStatus();
    }
}

void ModuleManager::notifyKeySubscription(LUNA_KEY_TYPE_E event, SERVER_TYPE_E eService, const std::string& key,\
        const std::string& payload)
{

    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager:notifyKeySubscription %s LUNA_KEY_TYPE_E = %d key = %s, payload = %s",
        __FUNCTION__, (int)event, key.c_str(), payload.c_str());
    for (const auto &it : listKeySubscriptionSubscribers)
       it->eventSubscribeKey(event, eService, key, payload);
}

void ModuleManager::notifyServerStatusInfo(SERVER_TYPE_E serviceName,
            bool connected)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager:notifyServerStatusInfo Server : %d, status %d",\
        (int)serviceName, connected);
    for (const auto it:listServerStatusInfoSubscribers[serviceName])
    {
        it->eventServerStatusInfo(serviceName, connected);
    }
}

void ModuleManager::notifyKeyInfo(LUNA_KEY_TYPE_E type, LSMessage *message)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager::notifyKeyInfo Callback recived LUNA_KEY_TYPE_E:%d", type);
    for (const auto it:listKeyInfoSubscribers[type])
    {
        it->eventKeyInfo(type, message);
    }
}

void ModuleManager::notifyServerStatusSubscription(SERVER_TYPE_E eService)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "%s ModuleManager:notifyServerStatusSubscription LUNA_KEY_TYPE_E = %d",
        __FUNCTION__, eService);
    for (const auto &it : listServerStatusSubscriptionSubscribers)
       it->eventSubscribeServerStatus(eService);
}
