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

#ifndef _MODULE_MANAGER_H_
#define _MODULE_MANAGER_H_

#include "utils.h"
#include "log.h"
#include <list>
#include <string>
#include <algorithm>
#include "moduleInterface.h"

class ModuleManager
{
    private:

        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;
        ModuleManager();
        std::list<ModuleInterface*> listSinkStatusSubscribers;
        std::list<ModuleInterface*> listMixerStatusSubscribers;
        std::list<ModuleInterface*> listInputVolumeSubscribers;
        std::list<ModuleInterface*> listMasterVolumeStatusSubscribers;
        std::list<ModuleInterface*> listServerStatusInfoSubscribers[eServiceCount];
        std::list<ModuleInterface*> listServerStatusSubscriptionSubscribers;
        std::list<ModuleInterface*> listKeySubscriptionSubscribers;
        std::list<ModuleInterface*> listKeyInfoSubscribers[eLunaEventCount];

    public:
        ~ModuleManager();
        static ModuleManager* getModuleManagerInstance();
        //subscription events
        void subscribeModuleEvent(ModuleInterface* module, bool first, utils::EVENT_TYPE_E eventType);
        void subscribeKeyInfo(ModuleInterface* module, bool first, LUNA_KEY_TYPE_E event, \
                SERVER_TYPE_E eService, const std::string& key, const std::string& payload);
        //Notification events start
        //To notify sink status
        void notifySinkStatusInfo(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType);
        //To notify all subscribers
        void notifyServerStatusInfo(SERVER_TYPE_E serviceName, bool connected);
        //To notify modules about Callbacks for the Luna event subscriptions
        void notifyKeyInfo(LUNA_KEY_TYPE_E type, LSMessage *message);
        //To notify mixer status
        void notifyMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType);
        //To notify Input volume change
        void notifyInputVolume(EVirtualAudioSink audioSink, const int& volume, const bool& ramp);
        void notifyMasterVolumeStatus();
        void subscribeServerStatusInfo(ModuleInterface* module, bool first, SERVER_TYPE_E eStatus);
        void notifyServerStatusSubscription(SERVER_TYPE_E eService);
        //To notify new event subscription requests (To Luna Event Subscription module)
        void notifyKeySubscription(LUNA_KEY_TYPE_E event, SERVER_TYPE_E eService, const std::string& key, \
                const std::string& payload);
};
#endif //_MODULE_MANAGER_H_
