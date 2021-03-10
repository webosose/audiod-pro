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

#ifndef _MODULE_MANAGER_H_
#define _MODULE_MANAGER_H_

#include "utils.h"
#include "log.h"
#include "events.h"
#include "moduleInterface.h"
#include "moduleFactory.h"

#include <list>
#include <string>
#include <algorithm>
#include <pbnjson/cxx/JValue.h>
#include <pbnjson/cxx/JDomParser.h>

using namespace pbnjson;

class ModuleManager
{
    private:

        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;
        ModuleManager(const std::string &audioModuleConfigPath);

        std::multimap<utils::EVENT_TYPE_E, ModuleInterface*> mapEventsSubscribers;
        using mapEventsSubscribersItr = std::multimap<utils::EVENT_TYPE_E, ModuleInterface*>::iterator;
        using mapEventsSubscribersPair = std::pair<mapEventsSubscribersItr, mapEventsSubscribersItr>;
        std::vector<std::string> mSupportedModulesVector;
        ModuleFactory *mModuleFactory;
        using mModulesCreatorMap = std::map<std::string, pFuncModuleCreator>;
        using mModulesCreatorMapItr = std::map<std::string, pFuncModuleCreator>::iterator;
        using mRegisteredHandlersMap = std::map<std::string, ModuleInterface*>;
        using mRegisteredHandlersMapItr = std::map<std::string, ModuleInterface*>::iterator;

    public:
        bool createModules();
        bool removeModules();
        bool readConfig(const std::string &audioModuleConfigPath);
        static ModuleManager* initialize(const std::string &audioModuleConfigPath);
        static ModuleManager* getModuleManagerInstance();
        static ModuleManager* mObjModuleManager;
        ~ModuleManager();
        //subscription for events
        void subscribeModuleEvent(ModuleInterface* module, bool first, utils::EVENT_TYPE_E eventType);
        //handling events
        void handleEvent(events::EVENTS_T* ev);
};
#endif //_MODULE_MANAGER_H_