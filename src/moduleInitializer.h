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

#ifndef MODULE_INITIALIZER_H_
#define MODULE_INITIALIZER_H_

#include <string>
#include <sstream>
#include <iostream>
#include <pbnjson/cxx/JValue.h>
#include <map>
#include <set>
#include "utils.h"
#include <glib.h>
#include <lunaservice.h>
#include <pbnjson/cxx/JDomParser.h>
#include "moduleManager.h"

using namespace pbnjson;

int load_audio_policy_manager(GMainLoop *loop, LSHandle* handle);
void unload_audio_policy_manager();

class ModuleInitializer
{
    public:
        //Constructor
        ModuleInitializer(const std::stringstream &audioModuleConfigPath);
        //Destructor
        ~ModuleInitializer();
        //for registering audio modules from the configuration file
        bool registerAudioModules();
        //initializing the module info with name and function pointer
        void initializeModuleInfo();

    private:
        //to store json config file path
        const std::string mAudioModuleConfigPath;
        std::set<std::string> mSetAudioModules;
        std::map<std::string, ModuleInitFunction> moduleMap;
};
#endif//MODULE_INITIALIZER_H_
