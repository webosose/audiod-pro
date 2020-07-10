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

#include "bluetoothManager.h"

BluetoothManager* BluetoothManager::mBluetoothManager = nullptr;

BluetoothManager* BluetoothManager::getBluetoothManagerInstance()
{
    return mBluetoothManager;
}

void BluetoothManager::setBlueToothA2DPActive (bool state, char *address,char *profile)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
        "%s : state = %d, address = %s, profile = %s", \
        __FUNCTION__, state, address, profile);

    if (state)
    {
        if (mObjAudioMixer)
        {
            mObjAudioMixer->programLoadBluetooth(address, profile);
        }
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                "%s : loaded bluetooth device",  __FUNCTION__);

    }
    else if (!state)
    {
        if (mObjAudioMixer)
            mObjAudioMixer->programUnloadBluetooth(profile);

        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "%s : unloaded bluetooth device",  __FUNCTION__);
    }
}

void BluetoothManager::btDeviceGetStatusInfo (LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s", __FUNCTION__);

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_6(PROP(subscribed, boolean),
    PROP(adapterAddress, string), PROP(returnValue, boolean), PROP(devices,array),
    PROP(errorCode, integer), PROP(errorText, string))
    REQUIRED_1(returnValue)));

    if (!msg.parse(__FUNCTION__))
    {
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "%s parse error", __FUNCTION__);
        return;
    }
    bool returnValue = false;
    msg.get("returnValue", returnValue);
    if (returnValue)
    {
        pbnjson::JValue deviceStatus = msg.get();
        pbnjson::JValue devices = deviceStatus["devices"];
        if (!devices.isArray())
        {
            PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "%s devices.isArray false", __FUNCTION__);
            return;
        }
        if (0 == devices.arraySize())
        {
            PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "%s devices.arraySize zero", __FUNCTION__);
            mA2dpConnected = false;
            return;
        }
        for (int i = 0; i < devices.arraySize(); i++)
        {
            pbnjson::JValue connectedProfiles = devices[i]["connectedProfiles"];
            std::string profile;
            std::string address;
            if (0 == connectedProfiles.arraySize())
            {
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "%s connectedProfiles.arraySize is zero", __FUNCTION__);
                continue;
            }
            for (int j = 0; j < connectedProfiles.arraySize(); j++)
            {
                profile = connectedProfiles[j].asString();
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    " inside for getting connectedProfiles : %s", profile.c_str());
                if ("a2dp" == profile )
                {
                    address = devices[i]["address"].asString();
                    mA2dpConnected = true;
                    char * device_address = (char*)address.c_str();

                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "Send info to PA for loading the bluez module active = %d of display = %d", mA2dpConnected);
                    setBlueToothA2DPActive(mA2dpConnected, device_address, "a2dp");
                    std::string payload = string_printf("{\"address\":\"%s\",\"subscribe\":true}", address.c_str());
                    PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "payload = %s", payload.c_str());

                    mObjModuleManager->subscribeKeyInfo(this, true, eLunaEventA2DPStatus,
                        eBluetoothService2, BT_A2DP_GET_STATUS, payload);
                    break;
                }
            }
        }
        if (nullptr == mConnectedDevice.c_str())
        {
            mA2dpConnected = false;
            return;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "%s device status subscription failed", __FUNCTION__);
        mConnectedDevice.clear();
        mA2dpConnected = false;
        return;
    }
}

void BluetoothManager:: btA2DPGetStatusInfo (LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s", __FUNCTION__);

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_9(PROP(subscribed, boolean),
    PROP(adapterAddress, string), PROP(returnValue, boolean), PROP(connecting, boolean),
    PROP(connected, boolean), PROP(playing, boolean), PROP(address, string),
    PROP(errorCode, integer), PROP(errorText, string))
    REQUIRED_1(returnValue)));

    if (!msg.parse(__FUNCTION__))
        return;

    bool returnValue = false;
    msg.get("returnValue", returnValue);
    if (returnValue)
    {
        std::string a2dpDeviceAddress;
        bool connected = false;
        msg.get("connected", connected);
        msg.get("address", a2dpDeviceAddress);
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "Device MAC address %s Connection Status %d",\
                a2dpDeviceAddress.c_str(), connected);
        if (nullptr == a2dpDeviceAddress.c_str())
        {
            PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "Device MAC address field not found");
            return;
        }
        if (connected)
        {
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "BT device is already in connected state");
        }
        else
        {
            mA2dpConnected = false;
            char * device_address = (char*)a2dpDeviceAddress.c_str();
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "Send info to PA for unloading the bluez module active = %d, address = %s",\
                mA2dpConnected, (char*)a2dpDeviceAddress.c_str());
            setBlueToothA2DPActive(false, device_address, "a2dp");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "%s a2dp status subscription failed", __FUNCTION__);
        mConnectedDevice.clear();
        mA2dpConnected = false;
    }
}

void BluetoothManager::eventKeyInfo (LUNA_KEY_TYPE_E type, LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s LUNA_KEY_TYPE_E = %d", __FUNCTION__, (int)type);
    BluetoothManager *btManagerInstance = BluetoothManager::getBluetoothManagerInstance();
    if (!btManagerInstance)
    {
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "btManagerInstance = null");
    }
    if (type == eLunaEventBTDeviceStatus)
    {
        btDeviceGetStatusInfo(message);
    }
    else if (type == eLunaEventA2DPStatus)
    {
        btA2DPGetStatusInfo(message);
    }
}

void BluetoothManager::eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "Got BT server status event %d : %d", serviceName, connected);
    if (connected)
    {
        mBluetoothManager->mObjModuleManager->subscribeKeyInfo(mBluetoothManager, true, eLunaEventBTDeviceStatus, \
                    eBluetoothService2, BT_DEVICE_GET_STATUS, "");
    }
    else
    {
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "Lost connection to %d server",serviceName);

    }
}

BluetoothManager::BluetoothManager()
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
        "BT manager constructor");
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if (!mObjAudioMixer)
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
            "AudioMixer instance is null");
    }
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (!mObjModuleManager)
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
            "mObjModuleManager instance is null");
    }
}

BluetoothManager::~BluetoothManager()
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
        "BT manager destructor");
}

void BluetoothManager::loadModuleBluetoothManager(GMainLoop *loop, LSHandle* handle)
{
    if (!mBluetoothManager)
    {
        mBluetoothManager = new (std::nothrow) BluetoothManager();
        if (mBluetoothManager)
        {
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                "BluetoothManager load sucess");
            mBluetoothManager->mObjModuleManager->subscribeServerStatusInfo(mBluetoothManager, true, \
                eBluetoothService2);
        }
        else
        {
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                "load BT manger module failed, memory error");
        }
    }
}

int load_bluetooth_manager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
        "Load bluetooth Manager");
    BluetoothManager::loadModuleBluetoothManager(loop, handle);
    return 0;
}

void unload_bluetooth_manager()
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
        "unLoad bluetooth manager");
    if (BluetoothManager::mBluetoothManager)
    {
        delete BluetoothManager::mBluetoothManager;
        BluetoothManager::mBluetoothManager = nullptr;
    }
}
