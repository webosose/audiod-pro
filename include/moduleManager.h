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
        ModuleManager();

        std::multimap<EModuleEventType, ModuleInterface*> mapEventsSubscribers;
        std::map<std::string, ModuleInterface*> mModuleHandlersMap;
        std::vector<std::string> mSupportedModulesVector;
        ModuleFactory *mModuleFactory;
        using mModulesCreatorMap = std::map<std::string, pFuncModuleCreator>;
        using mModulesCreatorMapItr = std::map<std::string, pFuncModuleCreator>::iterator;

    public:
        bool createModules();
        bool removeModules();
        bool loadConfig(const std::string &audioModuleConfigPath);
        static ModuleManager* initialize();
        static ModuleManager* getModuleManagerInstance();
        static ModuleManager* mObjModuleManager;
        ~ModuleManager();
        //subscription for events
        void subscribeModuleEvent(ModuleInterface* module, EModuleEventType eventType);
        void subscribeKeyInfo(ModuleInterface* module, EModuleEventType event, \
                SERVER_TYPE_E eService, const std::string& key, const std::string& payload);
        void subscribeServerStatusInfo(ModuleInterface* module, SERVER_TYPE_E eStatus);
        //handling events
        void handleEvent(events::EVENTS_T* ev);
};
#endif //_MODULE_MANAGER_H_