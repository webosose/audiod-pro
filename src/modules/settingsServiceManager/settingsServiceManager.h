/* @@@LICENSE
*
*      Copyright (c) 2020 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef _SETTING_SERVICE_MANAGER_H_
#define _SETTING_SERVICE_MANAGER_H_

#include "messageUtils.h"
#include "utils.h"
#include "log.h"
#include "moduleInterface.h"
#include "moduleManager.h"

#define GET_SYSTEM_SETTINGS  "luna://com.webos.settingsservice/getSystemSettings"
#define DND_PARAMS           "{\"category\":\"DND\",\"subscribe\":true}"
#define MEDIA_PARAMS         "{\"category\":\"Media\",\"subscribe\":true}"

class SettingsServiceManager : public ModuleInterface
{
    private:
        SettingsServiceManager(const SettingsServiceManager&) = delete;
        SettingsServiceManager& operator=(const SettingsServiceManager&) = delete;
        SettingsServiceManager();

        ModuleManager *mObjModuleManager;
    public:
        ~SettingsServiceManager();
        static SettingsServiceManager *mSettingsServiceManager;
        static SettingsServiceManager *getSettingsServiceManager();
        static void loadSettingsServiceManager(GMainLoop *loop, LSHandle* handle);

        //Modulemanager events implementation
        void eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected);
        void eventKeyInfo(LUNA_KEY_TYPE_E type, LSMessage *message);

        //Events for Luna key subscriptions
        bool settingsMediaDNDEvent(LSMessage *message);
};
#endif  //_SETTING_SERVICE_MANAGER_H_
