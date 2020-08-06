// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <syslog.h>

#include "scenario.h"
#include "utils.h"
#include "messageUtils.h"
#include "media.h"

#define BLUETOOTH_SERVICE       "com.webos.service.bluetooth2"
#define BT_DEVICE_GET_STATUS    "luna://com.webos.service.bluetooth2/device/getStatus"
#define BT_A2DP_GET_STATUS      "luna://com.webos.service.bluetooth2/a2dp/getStatus"
#define BT_ADAPTER_GET_STATUS   "luna://com.webos.service.bluetooth2/adapter/getStatus"
#define BT_DEVICE_SUBSCRIBE_PAYLOAD "{\"subscribe\":true}"

static bool a2dpConnected = false;
static bool defaultDeviceConnected = false;
static std::string connectedDevice;
static std::string mDefaultAdapterAddress;

static void _setBlueToothA2DPActive (bool state, char *address, char *profile)
{
    ScenarioModule * media = getMediaModule();

    g_message ("_setBlueToothA2DPActive : state = %d, address = %s, profile = %s", state, address, profile);

    if (state)
    {
        if (!defaultDeviceConnected)
        {
            gAudioMixer.programLoadBluetooth(address, profile);
            g_message ("_setBlueToothA2DPActive : loaded bluetooth device");
            defaultDeviceConnected = true;
        }
        else
            g_message ("_setBlueToothA2DPActive : BT device is already in connected state");
    }
    else
    {
        gAudioMixer.programUnloadBluetooth(profile);
        g_message ("_setBlueToothA2DPActive : unloaded bluetooth device");
        defaultDeviceConnected = false;
    }
}

static bool btA2DPGetStatusCallback (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    g_message ("%s", __FUNCTION__);

    std::string a2dpDeviceAddress;

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_9(PROP(subscribed, boolean),
    PROP(adapterAddress, string), PROP(returnValue, boolean), PROP(connecting, boolean),
    PROP(connected, boolean), PROP(playing, boolean), PROP(address, string),
    PROP(errorCode, integer), PROP(errorText, string))
    REQUIRED_1(returnValue)));

    if (!msg.parse(__FUNCTION__))
        return true;
    bool returnValue = false;
    bool connected = false;
    bool streamStatus = false;
    std::string adapterAddress;
    getMediaModule()->setCurrentState(false);
    msg.get("returnValue", returnValue);

    if (returnValue)
    {
        msg.get("connected", connected);
        msg.get("address", a2dpDeviceAddress);
        msg.get("adapterAddress", adapterAddress);
        g_debug ("Device MAC address %s Connection Status %d adapter address %s", a2dpDeviceAddress.c_str(), connected, adapterAddress.c_str());
        if(nullptr == a2dpDeviceAddress.c_str())
        {
            g_debug ("Device MAC address field not found");
            return true;
        }
        if (connected)
        {
            char * device_address = (char*)a2dpDeviceAddress.c_str();
            g_debug("btA2DPGetStatusCallback : Send info to PA for loading the bluez module active = %d, address = %s",\
                     connected, a2dpDeviceAddress.c_str());
            _setBlueToothA2DPActive(connected, device_address, "a2dp");
        }
        else
        {
            a2dpConnected = false;
            char * device_address = (char*)a2dpDeviceAddress.c_str();
            g_debug("btA2DPGetStatusCallback : Send info to PA for unloading the bluez module active = %d, address = %s",\
                     a2dpConnected, a2dpDeviceAddress.c_str());
            _setBlueToothA2DPActive(false, device_address, "a2dp");
        }
    }
    else
    {
        g_message ("a2dp status subscription failed", __FUNCTION__);
        connectedDevice.clear();
        a2dpConnected = false;
    }
    return true;
}

static bool btDeviceGetStatusCallback (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    g_message ("%s", __FUNCTION__);
    bool result = false;
    CLSError lserror;

    std::string payload = LSMessageGetPayload(message);

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_6(PROP(subscribed, boolean),
    PROP(adapterAddress, string), PROP(returnValue, boolean), PROP(devices,array),
    PROP(errorCode, integer), PROP(errorText, string))
    REQUIRED_1(returnValue)));
    if (!msg.parse(__FUNCTION__))
        return true;
    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (returnValue)
    {
        pbnjson::JValue deviceStatus = msg.get();
        pbnjson::JValue devices = deviceStatus["devices"];
        if (!devices.isArray())
            return true;

        if (0 == devices.arraySize())
        {
            a2dpConnected = false;
            return true;
        }
        for (int i = 0; i < devices.arraySize(); i++)
        {
            pbnjson::JValue connectedProfiles = devices[i]["connectedProfiles"];
            std::string profile;
            std::string address;
            std::string adapterAddress;
            if (0 == connectedProfiles.arraySize())
                continue;

            for (int j = 0; j < connectedProfiles.arraySize(); j++)
            {
                profile = connectedProfiles[j].asString();
                g_debug(" inside for getting connectedProfiles : %s", profile.c_str());
                if ("a2dp" == profile )
                {
                    address = devices[i]["address"].asString();
                    adapterAddress = devices[i]["adapterAddress"].asString();
                    a2dpConnected = true;
                    char * device_address = (char*)address.c_str();

                    g_debug("Send info to PA for loading the bluez module active = %d", a2dpConnected);
                    _setBlueToothA2DPActive(a2dpConnected, device_address, "a2dp");
                    std::string payload = string_printf("{\"adapterAddress\":\"%s\",\"address\":\"%s\",\"subscribe\":true}",adapterAddress.c_str(),address.c_str());
                    result = LSCall(lshandle, BT_A2DP_GET_STATUS, (char*)payload.c_str(), btA2DPGetStatusCallback,
                                    ctx, NULL, &lserror);
                    if (!result)
                        lserror.Print(__FUNCTION__, __LINE__);
                    break;
                }
            }
        }
        if (nullptr == connectedDevice.c_str())
        {
            a2dpConnected = false;
            return true;
        }
    }
    else
    {
        g_message ("device status subscription failed", __FUNCTION__);
        connectedDevice.clear();
        a2dpConnected = false;
        return true;
    }
    return true;
}

static bool _btAdapterQueryCallback (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    g_message ("%s", __FUNCTION__);

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_5(PROP(subscribed, boolean),
    PROP(adapters, array), PROP(returnValue, boolean),
    PROP(errorCode, integer), PROP(errorText, string))
    REQUIRED_1(returnValue)));

    if (!msg.parse(__FUNCTION__))
        return false;
    bool returnValue = false;
    bool result = false;
    std::string adapterAddress;
    CLSError lserror;
    msg.get("returnValue", returnValue);
    if (returnValue)
    {
        pbnjson::JValue adapterStatus = msg.get();
        pbnjson::JValue adapters = adapterStatus["adapters"];
        if (!adapters.isArray())
            return true;
        if (0 == adapters.arraySize())
        {
            g_message ("%s: adapters array size is zero", __FUNCTION__);
            return true;
        }
        for (int i = 0; i < adapters.arraySize(); i++)
        {
            std::string adapterName = adapters[i]["interfaceName"].asString();
            adapterAddress = adapters[i]["adapterAddress"].asString();
            g_message ("%s: adapterAddress = %s ", __FUNCTION__, adapterAddress.c_str());
            if ("hci0" == adapterName)
            {
                if (mDefaultAdapterAddress != adapterAddress)
                {
                    mDefaultAdapterAddress = adapterAddress;
                    std::string payload = string_printf("{\"adapterAddress\":\"%s\",\"subscribe\":true}", adapterAddress.c_str());
                    g_message ("%s : payload = %s", __FUNCTION__, payload.c_str());
                    result = LSCall(lshandle, BT_DEVICE_GET_STATUS, (char*)payload.c_str(), btDeviceGetStatusCallback,
                                    ctx, NULL, &lserror);
                    if (!result)
                        lserror.Print(__FUNCTION__, __LINE__);
                    break;
                }
                else
                    g_message ("%s : Already subscribe for default adapter: %s", __FUNCTION__, adapterAddress.c_str());
            }
        }
    }
    return true;
}

static bool
bluetoothServerStatus(LSHandle *sh,
                      const char *serviceName,
                      bool connected,
                      void *ctx)
{
    g_message ("%s", __FUNCTION__);
    bool result;
    CLSError lserror;

    if (connected)
    {
        result = LSCall(sh, BT_ADAPTER_GET_STATUS,
                        BT_DEVICE_SUBSCRIBE_PAYLOAD,
                        _btAdapterQueryCallback, ctx, NULL, &lserror);
        if (!result)
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else
    {
        g_warning("%s: lost connection to '%s' server", __FUNCTION__, serviceName);
    }
    return true;
}

int
BluetoothInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    bool result;
    CLSError lserror;
    result = LSRegisterServerStatusEx(handle, BLUETOOTH_SERVICE,
                 bluetoothServerStatus, loop, NULL, &lserror);
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);

    return 0;
}

SERVICE_START_FUNC (BluetoothInterfaceInit);
