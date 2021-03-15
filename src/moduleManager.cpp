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

#include "moduleManager.h"

ModuleManager* ModuleManager::mObjModuleManager = nullptr;
ModuleManager::ModuleManager(const std::string &audioModuleConfigPath)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager constructor");
    readConfig(audioModuleConfigPath);
    mModuleFactory = ModuleFactory::getInstance();
    if (!mModuleFactory)
        PM_LOG_ERROR(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
            "ModuleManager: Failed to get ModuleFactory instance");
}

ModuleManager* ModuleManager::initialize(const std::string &audioModuleConfigPath)
{
   if (!ModuleManager::mObjModuleManager)
       ModuleManager::mObjModuleManager = new ModuleManager(audioModuleConfigPath);
   if (ModuleManager::mObjModuleManager)
       PM_LOG_ERROR(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
            "ModuleManager initialized successfully");

   return ModuleManager::mObjModuleManager;
}

ModuleManager* ModuleManager::getModuleManagerInstance()
{
   return ModuleManager::mObjModuleManager;
}

bool ModuleManager::readConfig(const std::string &audioModuleConfigPath)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "readConfig");
    JValue configJson = JDomParser::fromFile(audioModuleConfigPath.c_str(), JSchema::AllSchema());
    if (!configJson.isValid() || !configJson.isObject())
    {
        PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "failed to parse file using defaults. File: %s. Error: %s",\
            audioModuleConfigPath.c_str(), configJson.errorString().c_str());
        return false;
    }
    if (configJson.hasKey("load_module"))
    {
        PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "found load_module key");
        JValue moduleInfo = configJson["load_module"];
        if (!moduleInfo.isArray())
        {
            PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "moduleInfo is not an array");
            return false;
        }
        else
        {
            std::string strModule;
            for (auto &elements:moduleInfo.items())
            {
                strModule = elements.asString();
                std::vector<std::string>::iterator it = std::find(mSupportedModulesVector.begin(), mSupportedModulesVector.end(), strModule);
                if (it == mSupportedModulesVector.end())
                {
                    mSupportedModulesVector.push_back(strModule);
                }
            }
            return true;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "load_module key not found");
        return false;
    }
}

bool ModuleManager::createModules()
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager: createModules");
    if (mModuleFactory)
    {
        mModulesCreatorMap modulesCreatorMap = mModuleFactory->getModulesCreatorMap();
        for (mModulesCreatorMapItr it = modulesCreatorMap.begin(); it != modulesCreatorMap.end(); ++it)
        {
            if (std::find (mSupportedModulesVector.begin(), mSupportedModulesVector.end(), it->first) != mSupportedModulesVector.end())
                mModuleFactory->CreateObject(it->first, nullptr);
        }
    }
    return true;
}

bool ModuleManager::removeModules()
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager: removeModules");
    if (mModuleFactory)
    {
        mRegisteredHandlersMap registeredHandlersMap = mModuleFactory->getRegisteredHandlersMap();
        for (mRegisteredHandlersMapItr it = registeredHandlersMap.begin(); it != registeredHandlersMap.end(); ++it)
            mModuleFactory->UnRegister(it->first);
    }
    return true;
}

ModuleManager::~ModuleManager()
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "ModuleManager destructor");
}

void ModuleManager::subscribeModuleEvent(ModuleInterface* module, EModuleEventType eventType)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT,\
        "subscribeModuleEvent for eventype:%d", (int)eventType);
    mapEventsSubscribers.insert(std::pair<EModuleEventType, ModuleInterface*>(eventType, module));
}

void ModuleManager::subscribeKeyInfo(ModuleInterface* module, EModuleEventType event, SERVER_TYPE_E eService, const std::string& key, const std::string& payload)
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
        mapEventsSubscribers.insert(std::pair<EModuleEventType, ModuleInterface*>(event, module));
        //if payload is required from Module during runtime
        events::EVENT_SUBSCRIBE_KEY_T eventSubscribeKey;
        eventSubscribeKey.eventName = utils::eEventLunaKeySubscription;
        eventSubscribeKey.type = event ;
        eventSubscribeKey.serviceName = eService;
        eventSubscribeKey.api = key;
        eventSubscribeKey.payload = payload;
        handleEvent((events::EVENTS_T*)&eventSubscribeKey);
    }
}

void ModuleManager::subscribeServerStatusInfo(ModuleInterface* module, SERVER_TYPE_E eStatus)
{
    PM_LOG_INFO(MSGID_MODULE_MANAGER, INIT_KVCOUNT, \
        "subscribeServerStatusInfo called");
    if (!(eStatus >= eBluetoothService && eStatus < eServiceCount))
    {
        PM_LOG_WARNING(MSGID_MODULE_MANAGER, INIT_KVCOUNT, "not Valid service");
    }
    else
    {
        mapEventsSubscribers.insert(std::pair<EModuleEventType, ModuleInterface*>(utils::eEventServerStatusSubscription, module));
    }
    events::EVENT_SUBSCRIBE_SERVER_STATUS_T eventSubscribeServerStatus;
    eventSubscribeServerStatus.eventName = utils::eEventLunaServerStatusSubscription;
    eventSubscribeServerStatus.serviceName = eStatus;
    handleEvent((events::EVENTS_T*)&eventSubscribeServerStatus);
}

void ModuleManager::handleEvent(events::EVENTS_T *ev)
{
    PM_LOG_DEBUG("handleEvent for eventType:%d", (int)ev->eventName);
    mapEventsSubscribersPair eventPairs = mapEventsSubscribers.equal_range(ev->eventName);
    for (mapEventsSubscribersItr eventsItr = eventPairs.first; eventsItr != eventPairs.second; eventsItr++)
    {
        (eventsItr->second)->handleEvent(ev);
    }
}
