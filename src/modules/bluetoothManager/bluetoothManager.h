/* @@@LICENSE
*
*      Copyright (c) 2020-2021 LG Electronics Company.
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

#ifndef _BLUETOOTH_MANAGER_H
#define _BLUETOOTH_MANAGER_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "utils.h"
#include "messageUtils.h"
#include "audioMixer.h"
#include "log.h"
#include "moduleFactory.h"

#define BT_ADAPTER_SUBSCRIBE_PAYLOAD "{\"subscribe\":true}"
#define BT_DEVICE_GET_STATUS    "luna://com.webos.service.bluetooth2/device/getStatus"
#define BT_A2DP_GET_STATUS      "luna://com.webos.service.bluetooth2/a2dp/getStatus"
#define BT_ADAPTER_GET_STATUS   "luna://com.webos.service.bluetooth2/adapter/getStatus"

typedef std::map<std::string, bool> adapterInfo;
typedef std::map<int, std::string> adapterDisplayIDMap;
typedef std::map<std::string, int> adapterAddressDisplayIDMap;
typedef std::vector<std::string> connectedAddressVector;

#define CONFIG_DIR_PATH "/etc/palm/audiod"

#define BLUETOOTH_CONFIG "bluetooth_configuration.json"

class BluetoothManager : public ModuleInterface
{
    private:

        bool mA2dpConnected;
        bool mA2dpSource;
        int mDisplayID;
        std::string mConnectedDevice;
        std::string mAdapterNameKey;
        std::string mA2dpAdapterName;

        pbnjson::JValue fileJsonBluetoothConfig;
        adapterInfo mAdapterNamesMap;
        adapterInfo mAdapterSubscriptionStatusMap;
        adapterDisplayIDMap mAdapterDisplayIDMap;
        adapterAddressDisplayIDMap mAdapterAddressDisplayIDMap;
        connectedAddressVector mConnectedAddressVector;

        BluetoothManager (const BluetoothManager&) = delete;
        BluetoothManager& operator=(const BluetoothManager&) = delete;
        BluetoothManager(ModuleConfig* const pConfObj);
        ModuleManager* mObjModuleManager;
        AudioMixer *mObjAudioMixer;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_bluetooth_manager", &BluetoothManager::CreateObject));
        }

    public:
        ~BluetoothManager();
        static BluetoothManager* getBluetoothManagerInstance();
        static BluetoothManager *mBluetoothManager;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the BluetoothManager handler");
                mBluetoothManager = new(std::nothrow) BluetoothManager(pConfObj);
                if (mBluetoothManager)
                    return mBluetoothManager;
            }
            return nullptr;
        }
        void initialize();
        void deInitialize();
        void handleEvent(events::EVENTS_T* event);

        void eventServerStatusInfo (SERVER_TYPE_E serviceName, bool connected);
        void eventKeyInfo (LUNA_KEY_TYPE_E type, LSMessage *message);

        void setBlueToothA2DPActive (bool state, char *address, char *profile);
        void btAdapterQueryInfo (LSMessage *message);
        void btDeviceGetStatusInfo (LSMessage *message);
        void btA2DPGetStatusInfo (LSMessage *message);
        void a2dpDeviceGetStatus(LSMessage *message);
        void btA2DPSourceGetStatus(LSMessage *message);
        void setBluetoothA2DPSource (bool state);

        bool readBluetoothConfigurationInfo();
        bool initializeBluetoothConfigurationInfo();
};
#endif //_BLUETOOTH_MANAGER_
