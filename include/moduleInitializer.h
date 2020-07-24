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
#include <glib.h>
#include <lunaservice.h>
#include <pbnjson/cxx/JDomParser.h>
#include "moduleManager.h"
#include "audioMixer.h"
#include "utils.h"
#include "log.h"

using namespace pbnjson;

int load_audio_policy_manager(GMainLoop *loop, LSHandle* handle);
int load_udev_event_manager(GMainLoop *loop, LSHandle* handle);
int load_master_volume_manager(GMainLoop *loop, LSHandle* handle);
int load_bluetooth_manager (GMainLoop *loop, LSHandle* handle);
int load_luna_event_subscriber(GMainLoop *loop, LSHandle* handle);
int load_playback_manager(GMainLoop *loop, LSHandle* handle);
int load_settings_service_manager(GMainLoop *loop, LSHandle* handle);
int load_system_sounds_manager(GMainLoop *loop, LSHandle* handle);
int load_connection_manager(GMainLoop *loop, LSHandle* handle);
void unload_audio_policy_manager();
void unload_udev_event_manager();
void unload_master_volume_manager();
void unload_bluetooth_manager ();
void unload_luna_event_subscriber();
void unload_playback_manager();
void unload_settings_service_manager();
void unload_system_sounds_manager();
void unload_connection_manager();

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
