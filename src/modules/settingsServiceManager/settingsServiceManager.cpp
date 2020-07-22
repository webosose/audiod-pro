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


#include "settingsServiceManager.h"

SettingsServiceManager* SettingsServiceManager::mSettingsServiceManager = nullptr;

SettingsServiceManager* SettingsServiceManager::getSettingsServiceManager()
{
    return mSettingsServiceManager;
}

void SettingsServiceManager::eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected)
{
    if (serviceName == eSettingsService)
    {
        PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
            "eventServerStatusInfo got call back status = %d", connected);
        if (connected)
        {
            PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, "%s: connecting to '%d' server", __FUNCTION__, serviceName);
        }
    }
}

void SettingsServiceManager::eventKeyInfo(LUNA_KEY_TYPE_E type, LSMessage *message)
{
    PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
        "SettingsServiceManager::eventKeyInfo key = %d", type);
    switch (type)
    {
        case eLunaEventSettingMediaParam:
        case eLunaEventSettingDNDParam:
        {
            PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                        "SettingsServiceManager::eventKeyInfo : eLunaEventSettingMediaParam & eLunaEventSettingDNDParam");
            settingsMediaDNDEvent(message);
        }
        break;
        default:
        {
            PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "SettingsServiceManager::eventKeyInfo unknown event");
        }
        break;
    }
}

bool SettingsServiceManager::settingsMediaDNDEvent(LSMessage *message)
{
    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__func__))
        return true;

    PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "Called settingsMediaDNDEvent() function for Media and DND setting");

    bool touchEnable;
    bool dndEnable;
    int amplitude = 0;
    pbnjson::JValue json_status = msg.get();
    pbnjson::JValue object = json_status["settings"];

    if (!object.isObject())
        return true;

    if (object["dndEnable"].asBool(dndEnable) == CONV_OK)
    {
        PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "dndEnable key is found from DND params");
    }
    else
        PM_LOG_ERROR(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                     "_settingCallback() dndEnable key not found");

    if (object["amplitude"].asNumber(amplitude) == CONV_OK)
    {
        PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "Amplitude key is found from media settings key");
    }
    else
        PM_LOG_ERROR(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                     "_settingCallback() amplitude field not found");

    return true;
}

SettingsServiceManager::SettingsServiceManager()
{
    PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
        "SettingsServiceManager Constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (!mObjModuleManager)
    {
        PM_LOG_ERROR(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
            "mObjModuleManager is null");
    }
}

SettingsServiceManager::~SettingsServiceManager()
{
    PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
        "SettingsServiceManager destructor");
}

void SettingsServiceManager::loadSettingsServiceManager(GMainLoop *loop, LSHandle* handle)
{
    if (!mSettingsServiceManager)
    {
        mSettingsServiceManager = new (std::nothrow) SettingsServiceManager();
        if (mSettingsServiceManager)
        {
            PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                       "SettingsServiceManager init success");
        }
        else
        {
            PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                       "SettingsServiceManager init failed");
        }
        if (mSettingsServiceManager->mObjModuleManager)
        {
            PM_LOG_DEBUG("Subscribing to setting service");
            mSettingsServiceManager->mObjModuleManager->subscribeServerStatusInfo(mSettingsServiceManager, false, eSettingsService);
            mSettingsServiceManager->mObjModuleManager->subscribeKeyInfo(mSettingsServiceManager, false, eLunaEventSettingMediaParam, eSettingsService,\
                                                                         GET_SYSTEM_SETTINGS, MEDIA_PARAMS);
            mSettingsServiceManager->mObjModuleManager->subscribeKeyInfo(mSettingsServiceManager, false, eLunaEventSettingDNDParam, eSettingsService,\
                                                                         GET_SYSTEM_SETTINGS, DND_PARAMS);
        }
        else
        {
            PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "Modulemanager instance is null");
        }
    }
}

int load_settings_service_manager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "loading setting service manager");
    SettingsServiceManager::loadSettingsServiceManager(loop, handle);
    return 0;
}

void unload_settings_service_manager()
{
    PM_LOG_INFO(MSGID_SETTING_SERVICE_MANAGER, INIT_KVCOUNT, \
                "unloading setting service manager");
    if  (SettingsServiceManager::mSettingsServiceManager)
    {
        delete SettingsServiceManager::mSettingsServiceManager;
        SettingsServiceManager::mSettingsServiceManager = nullptr;
    }
}