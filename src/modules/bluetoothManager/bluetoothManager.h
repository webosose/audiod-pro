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
#include "moduleManager.h"
#include "moduleInterface.h"

#define BT_DEVICE_GET_STATUS    "luna://com.webos.service.bluetooth2/device/getStatus"
#define BT_A2DP_GET_STATUS      "luna://com.webos.service.bluetooth2/a2dp/getStatus"

class BluetoothManager : public ModuleInterface
{
    private:

        bool mA2dpConnected;
        std::string mConnectedDevice;

        BluetoothManager (const BluetoothManager&) = delete;
        BluetoothManager& operator=(const BluetoothManager&) = delete;
        BluetoothManager ();
        ModuleManager* mObjModuleManager;
        AudioMixer *mObjAudioMixer;

    public:
        ~BluetoothManager();
        static BluetoothManager* getBluetoothManagerInstance ();
        static void loadModuleBluetoothManager (GMainLoop *loop, LSHandle* handle);
        static BluetoothManager *mBluetoothManager;

        void eventServerStatusInfo (SERVER_TYPE_E serviceName, bool connected);
        void eventKeyInfo (LUNA_KEY_TYPE_E type, LSMessage *message);

        void setBlueToothA2DPActive (bool state, char *address, char *profile);
        void btDeviceGetStatusInfo (LSMessage *message);
        void btA2DPGetStatusInfo (LSMessage *message);
};
#endif //_BLUETOOTH_MANAGER_
