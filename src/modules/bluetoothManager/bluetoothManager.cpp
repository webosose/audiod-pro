// Copyright (c) 2020-2023 LG Electronics, Inc.
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
bool BluetoothManager::mIsObjRegistered = BluetoothManager::RegisterObject();

BluetoothManager* BluetoothManager::getBluetoothManagerInstance()
{
    return mBluetoothManager;
}

bool BluetoothManager::readBluetoothConfigurationInfo()
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "BluetoothManager::readBluetoothConfigurationInfo");
    bool loadStatus = true;
    std::stringstream jsonFilePath;
    jsonFilePath << CONFIG_DIR_PATH << "/" << BLUETOOTH_CONFIG;

    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "Loading bluetooth configuration info from json file %s",\
        jsonFilePath.str().c_str());
    fileJsonBluetoothConfig = pbnjson::JDomParser::fromFile(jsonFilePath.str().c_str(),\
        pbnjson::JSchema::AllSchema());
    if (!fileJsonBluetoothConfig.isValid() || !fileJsonBluetoothConfig.isObject())
    {
        PM_LOG_ERROR(MSGID_INVALID_INPUT, INIT_KVCOUNT,\
            "readBluetoothConfigurationInfo : Failed to parse json config file. File: %s",\
            jsonFilePath.str().c_str());
        loadStatus = false;
        return false;
    }

    if (loadStatus)
    {
        if (initializeBluetoothConfigurationInfo())
            PM_LOG_INFO(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializeBluetoothConfigurationInfo success");
        else
        {
            PM_LOG_ERROR(MSGID_POLICY_MANAGER, INIT_KVCOUNT, "initializeBluetoothConfigurationInfo failed");
            return false;
        }
    }
    return true;
}

bool BluetoothManager::initializeBluetoothConfigurationInfo()
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "BluetoothManager::initializeBluetoothConfigurationInfo");
    pbnjson::JValue bluetoothConfigurationInfo = fileJsonBluetoothConfig["configuration"];
    if (!bluetoothConfigurationInfo.isArray())
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "bluetoothConfigurationInfo is not an array");
        return false;
    }
    else
    {
        for (const pbnjson::JValue& elements : bluetoothConfigurationInfo.items())
        {
            int displays = elements["displays"].asNumber<int>();
            int adapters = elements["adapters"].asNumber<int>();
            if (elements["adapterNameKey"].asString(mAdapterNameKey) == CONV_OK)
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,"BluetoothManager::initializeBluetoothConfigurationInfo: Successfully read adapterNameKey %s",
                mAdapterNameKey.c_str());
            else
            {
                PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,"BluetoothManager::initializeBluetoothConfigurationInfo: Failed to read adapterNameKey");
                return false;
            }

            if (elements["a2dpAdapterName"].asString(mA2dpAdapterName) == CONV_OK)
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,"BluetoothManager::initializeBluetoothConfigurationInfo: Successfully read a2dpAdapterName %s", mA2dpAdapterName.c_str());
            else
            {
                PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,"BluetoothManager::initializeBluetoothConfigurationInfo: Failed to read a2dpAdapterName");
                return false;
            }

            if (elements.hasKey("adapterNames"))
            {
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "found adapterNames key");
                JValue adaptersInfo = elements["adapterNames"];
                if (!adaptersInfo.isArray())
                {
                    PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "adaptersInfo is not an array");
                    return false;
                }
                else
                {
                    std::string adapterName;
                    for (auto &element:adaptersInfo.items())
                    {
                        adapterName = element.asString();
                        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "adapterName %s", adapterName.c_str());
                        mAdapterSubscriptionStatusMap.insert(std::pair<std::string, bool>(adapterName, false));
                    }
                }
            }
            else
            {
                PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "adapterNames key not found");
                return false;
            }
            for (int displayindex=1; displayindex<=displays; displayindex++)
                mDisplayConnectionStatus.insert(std::pair<int, bool>(displayindex, false));
        }
    }
    return true;
}

void BluetoothManager::setBlueToothA2DPActive (bool state, char *address, char *profile, std::string adapterAddress)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
        "%s : state = %d, address = %s, profile = %s, adapterAddress = %s", \
        __FUNCTION__, state, address, profile, adapterAddress.c_str());
    int display = 0;

    std::vector<std::string>::iterator it;
    std::map<std::string, int>::iterator iter;
    std::map<int, bool>::iterator mapIter;

    std::string addressValue(address);
    it = std::find(mConnectedAddressVector.begin(), mConnectedAddressVector.end(), addressValue);

    if (state)
    {
        if (it == mConnectedAddressVector.end())
        {
            mConnectedAddressVector.push_back(addressValue);

            for (iter = mAdapterDisplayIDMap.begin(); iter != mAdapterDisplayIDMap.end(); iter++)
            {
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "mAdapterDisplayIDMap adapter names is %s and display id is %d", iter->first.c_str(), iter->second);
            }
            for (iter = mAdapterDisplayIDMap.begin(); iter != mAdapterDisplayIDMap.end(); iter++)
            {
                if (iter->first == adapterAddress)
                {
                    display = iter->second;
                    break;
                }
            }
            mapIter = mDisplayConnectionStatus.find(display);
            if (mapIter != mDisplayConnectionStatus.end())
            {
                mapIter->second = true;
            }
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "Display value is %d", display);
            if (mObjAudioMixer)
            {
                mObjAudioMixer->programLoadBluetooth(address, profile, display);
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                    "%s : loaded bluetooth device",  __FUNCTION__);
            }
            if (mObjModuleManager)
            {
                events::EVENT_MASTER_VOLUME_STATUS_T eventMasterVolumeStatus;
                eventMasterVolumeStatus.eventName = utils::eEventMasterVolumeStatus;
                mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventMasterVolumeStatus);
            }
            else
                PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                           "mObjModuleManager is NULL");
        }
        else
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "%s BT device is already in connected state",  __FUNCTION__);
    }
    else
    {
        if (it != mConnectedAddressVector.end())
        {
            for (iter = mAdapterDisplayIDMap.begin(); iter != mAdapterDisplayIDMap.end(); iter++)
            {
                if (iter->first == adapterAddress)
                {
                    display = iter->second;
                    break;
                }
            }
            mapIter = mDisplayConnectionStatus.find(display);
            if (mapIter != mDisplayConnectionStatus.end())
            {
                mapIter->second = false;
            }
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "Display value is %d", display);

            if (mObjAudioMixer)
            {
                mObjAudioMixer->programUnloadBluetooth(profile, display);
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "%s : unloaded bluetooth device",  __FUNCTION__);
            }
            mConnectedAddressVector.erase(it);
        }
    }
    if (mObjModuleManager)
    {
        events::EVENT_BT_DEVICE_DISPAY_INFO_T stEventBtDeviceDisplayInfo;
        stEventBtDeviceDisplayInfo.eventName = utils::eEventBTDeviceDisplayInfo;
        stEventBtDeviceDisplayInfo.displayId = display;
        stEventBtDeviceDisplayInfo.address = address;
        stEventBtDeviceDisplayInfo.state = state;
        if (display != 0)
            mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stEventBtDeviceDisplayInfo);
    }
}

void BluetoothManager::btAdapterQueryInfo(LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s", __FUNCTION__);

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_5(PROP(subscribed, boolean),
    PROP(adapters, array), PROP(returnValue, boolean),
    PROP(errorCode, integer), PROP(errorText, string))
    REQUIRED_1(returnValue)));

    if (!msg.parse(__FUNCTION__))
        return;
    bool returnValue = false;
    std::string adapterAddress;
    msg.get("returnValue", returnValue);
    std::map<std::string, int>::iterator iter;
    if (returnValue)
    {
        pbnjson::JValue adapterStatus = msg.get();
        pbnjson::JValue adapters = adapterStatus["adapters"];
        if (!adapters.isArray())
            return;
        if (0 == adapters.arraySize())
        {
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,"adapters array size is zero");
            return;
        }
        bool status = false;

        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,"adapters array size is %d", adapters.arraySize());
        for (int i = 0; i < adapters.arraySize(); i++)
        {
            std::string adapterName = adapters[i][mAdapterNameKey].asString();
            adapterAddress = adapters[i]["adapterAddress"].asString();
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "adapterAddress = %s, a2dpAdapterName = %s, adapterName = %s", adapterAddress.c_str(), mA2dpAdapterName.c_str(), adapterName.c_str());
            std::map<std::string, bool>::iterator it = mAdapterSubscriptionStatusMap.find(adapterName);
            if (it != mAdapterSubscriptionStatusMap.end())
            {
                status = it->second;
            }

            if (adapterName == "hci0")
                mAdapterDisplayIDMap.insert(std::pair<std::string, int >(adapterAddress, 1));
            else if (adapterName == "hci1")
                mAdapterDisplayIDMap.insert(std::pair<std::string, int >(adapterAddress, 2));

            if (mA2dpAdapterName == adapterName)
            {
                if (status == false)
                {
                    if (it != mAdapterSubscriptionStatusMap.end())
                    {
                        it->second = true;
                    }
                    std::string payload = string_printf("{\"adapterAddress\":\"%s\",\"subscribe\":true}", adapterAddress.c_str());
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "a2dp %s : payload = %s", __FUNCTION__, payload.c_str());
                    mObjModuleManager->subscribeKeyInfo(this, eEventA2DPDeviceStatus, eBluetoothService2,
                        BT_DEVICE_GET_STATUS, payload);
                }
                else
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "%s: Already subscribed for adapter: %s", __FUNCTION__, adapterAddress.c_str());
            }
            else if (!adapterName.empty())
            {
                if (false == status)
                {
                    if (it != mAdapterSubscriptionStatusMap.end())
                    {
                        it->second = true;
                    }
                    std::string payload = string_printf("{\"adapterAddress\":\"%s\",\"subscribe\":true}", adapterAddress.c_str());
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "%s : payload = %s", __FUNCTION__, payload.c_str());
                    mObjModuleManager->subscribeKeyInfo(this, eEventBTDeviceStatus, eBluetoothService2,
                        BT_DEVICE_GET_STATUS, payload);
                }
                else
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "%s: Already subscribed for default adapter: %s", __FUNCTION__, adapterAddress.c_str());
            }
        }
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
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "%s parse error", __FUNCTION__);
        return;
    }
    bool returnValue = false;
    std::vector<std::string>::iterator iter;

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
            std::string role;
            std::string address;
            std::string adapterAddress;

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
                    pbnjson::JValue connectedRoles = devices[i]["connectedRoles"];
                    if (0 == connectedRoles.arraySize())
                        return;
                    for (int k = 0; k < connectedRoles.arraySize(); k++)
                    {
                        role = connectedRoles[k].asString();
                        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                            "%s profile = %s and role = %s", __FUNCTION__, profile.c_str(), role.c_str());
                        if (("A2DP_SRC" == role) || ("HFP_AG" == role))
                        {
                            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                                 "%s Not considering bluetooth loading as profile is A2DP_SRC/HFP", __FUNCTION__);
                            return;
                        }
                    }
                    address = devices[i]["address"].asString();
                    adapterAddress = devices[i]["adapterAddress"].asString();
                    mA2dpConnected = true;
                    char * device_address = (char*)address.c_str();
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "Send info to PA for loading the bluez module active = %d", mA2dpConnected);
                    setBlueToothA2DPActive(mA2dpConnected, device_address, (char*)profile.c_str(), adapterAddress);
                    std::string payload = string_printf("{\"adapterAddress\":\"%s\",\"address\":\"%s\",\"subscribe\":true}",adapterAddress.c_str(), address.c_str());
                    PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "payload = %s", payload.c_str());

                    iter = std::find(mA2dpGetStatus.begin(), mA2dpGetStatus.end(), adapterAddress);
                    if (iter == mA2dpGetStatus.end())
                    {
                        mObjModuleManager->subscribeKeyInfo(this, eLunaEventA2DPStatus,
                            eBluetoothService2, BT_A2DP_GET_STATUS, payload);
                        mA2dpGetStatus.push_back(adapterAddress);
                    }
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
        std::string adapterAddress;
        bool connected = false;
        std::string profile = "a2dp";
        msg.get("connected", connected);
        msg.get("address", a2dpDeviceAddress);
        msg.get("adapterAddress", adapterAddress);
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "Device MAC address %s Connection Status %d adapter address %s",\
                a2dpDeviceAddress.c_str(), connected, adapterAddress.c_str());
        if (nullptr == a2dpDeviceAddress.c_str())
        {
            PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "Device MAC address field not found");
            return;
        }
        if (connected)
        {
            char * device_address = (char*)a2dpDeviceAddress.c_str();
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "%s Send info to PA for loading the bluez module active = %d, address = %s",\
                     __FUNCTION__, connected, a2dpDeviceAddress.c_str());
            setBlueToothA2DPActive(connected, device_address, (char*)profile.c_str(), adapterAddress);
        }
        else
        {
            mA2dpConnected = false;
            char * device_address = (char*)a2dpDeviceAddress.c_str();
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "Send info to PA for unloading the bluez module active = %d, address = %s",\
                mA2dpConnected, a2dpDeviceAddress.c_str());
            setBlueToothA2DPActive(false, device_address, (char*)profile.c_str(), adapterAddress);
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

void BluetoothManager::setBluetoothA2DPSource(bool state)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s : state = %d", __FUNCTION__, state);
    if (mObjAudioMixer && mObjAudioMixer->programA2dpSource(state))
    {
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "Sending programA2dpSource to PA is success");
    }
}

void BluetoothManager::btA2DPSourceGetStatus(LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s ", __FUNCTION__);

    LSMessageJsonParser msg(message,SCHEMA_9(REQUIRED(subscribed, boolean),
    REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED
    (connecting, boolean),REQUIRED(connected, boolean),REQUIRED(playing, boolean),
    REQUIRED(address, string),OPTIONAL(errorCode, integer),OPTIONAL(errorText, string)));

    if (!msg.parse(__FUNCTION__))
        return;
    bool returnValue = false;

    msg.get("returnValue", returnValue);

    if (returnValue)
    {
        std::string a2dpDeviceAddress;
        std::string adapterAddress;
        bool connected = false;
        msg.get("connected", connected);
        msg.get("address", a2dpDeviceAddress);
        msg.get("adapterAddress", adapterAddress);
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "Device MAC address %s Connection Status %d adapter address %s", a2dpDeviceAddress.c_str(), connected, adapterAddress.c_str());
        setBluetoothA2DPSource(connected);
    }
    else
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "%s : a2dp status subscription failed", __FUNCTION__);
    }
}

void BluetoothManager::a2dpDeviceGetStatus (LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s ", __FUNCTION__);
    bool result = false;

    std::string payload = LSMessageGetPayload(message);

    LSMessageJsonParser msg(message,SCHEMA_6(REQUIRED(subscribed, boolean),
    REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED
    (devices,array),OPTIONAL(errorCode, integer),OPTIONAL(errorText, string)));
    if (!msg.parse(__FUNCTION__))
        return;
    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (returnValue)
    {
        pbnjson::JValue deviceStatus = msg.get();
        pbnjson::JValue devices = deviceStatus["devices"];
        if (!devices.isArray())
            return;

        for (int i = 0; i < devices.arraySize(); i++)
        {
            pbnjson::JValue connectedProfiles = devices[i]["connectedProfiles"];
            std::string profile;
            std::string role;
            std::string address;
            std::string adapterAddress;
            if (0 == connectedProfiles.arraySize())
                continue;

            for (int j = 0; j < connectedProfiles.arraySize(); j++)
            {
                pbnjson::JValue connectedRoles = devices[i]["connectedRoles"];
                for (int k = 0; k < connectedRoles.arraySize(); k++)
                {
                    role = connectedRoles[k].asString();
                    if ("A2DP_SRC" == role)
                    {
                        mA2dpSource = true;
                        break;
                    }
                    mA2dpSource = false;
                }
                profile = connectedProfiles[j].asString();
                PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "Getting connectedProfiles and connectedRole: %s %s",profile.c_str(), role.c_str());
                if (("a2dp" == profile) && mA2dpSource)
                {
                    address = devices[i]["address"].asString();
                    adapterAddress = devices[i]["adapterAddress"].asString();
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "Send info to PA for a2dpSource %d", mA2dpSource);
                    setBluetoothA2DPSource(true);
                    std::string payload = string_printf("{\"adapterAddress\":\"%s\",\"address\":\"%s\",\"subscribe\":true}",adapterAddress.c_str(), address.c_str());
                    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                        "%s : payload = %s", __FUNCTION__, payload.c_str());
                    mObjModuleManager->subscribeKeyInfo(this, eEventA2DPSourceStatus, eBluetoothService2,
                        BT_A2DP_GET_STATUS, payload);
                    break;
                }
            }
        }
    }
}

void BluetoothManager::eventKeyInfo (LUNA_KEY_TYPE_E type, LSMessage *message)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "%s LUNA_KEY_TYPE_E = %d", __FUNCTION__, (int)type);
    if (type == eEventBTAdapter)
    {
        btAdapterQueryInfo(message);
    }
    else if (type == eEventBTDeviceStatus)
    {
        btDeviceGetStatusInfo(message);
    }
    else if (type == eLunaEventA2DPStatus)
    {
        btA2DPGetStatusInfo(message);
    }
    else if (type == eEventA2DPDeviceStatus)
    {
        a2dpDeviceGetStatus(message);
    }
    else if (type == eEventA2DPSourceStatus)
    {
        btA2DPSourceGetStatus(message);
    }
}

void BluetoothManager::eventServerStatusInfo(SERVER_TYPE_E serviceName, bool connected)
{
    PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
        "Got BT server status event %d : %d", serviceName, connected);
    BluetoothManager *btManagerInstance = BluetoothManager::getBluetoothManagerInstance();
    if (connected && serviceName == eBluetoothService2)
    {
        PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
            "BT service is connected");
    }
}

BluetoothManager::BluetoothManager(ModuleConfig* const pConfObj):
    mA2dpConnected(false), mA2dpSource(false), mDisplayID(0)
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

void BluetoothManager::initialize()
{
    if (mBluetoothManager->readBluetoothConfigurationInfo())
    {
        if (mBluetoothManager)
        {
            mBluetoothManager->mObjModuleManager->subscribeServerStatusInfo(mBluetoothManager, \
                eBluetoothService2);
            mBluetoothManager->mObjModuleManager->subscribeKeyInfo(mBluetoothManager, eEventBTAdapter, \
                eBluetoothService2, BT_ADAPTER_GET_STATUS, BT_ADAPTER_SUBSCRIBE_PAYLOAD);
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                "Successfully initialized BluetoothManager");
        }
        else
        {
            PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                "mBluetoothManager is nullptr");
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, \
                "Failed initializing BluetoothManager");
    }
}

void BluetoothManager::deInitialize()
{
    PM_LOG_DEBUG("BluetoothManager deInitialize()");
    if (mBluetoothManager)
    {
        delete mBluetoothManager;
        mBluetoothManager = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT, "mBluetoothManager is nullptr");
}

void BluetoothManager::handleEvent(events::EVENTS_T* event)
{
    switch(event->eventName)
    {
        case utils::eEventServerStatusSubscription:
        {
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventServerStatusSubscription");
            events::EVENT_SERVER_STATUS_INFO_T *serverStatusInfoEvent = (events::EVENT_SERVER_STATUS_INFO_T*)event;
            eventServerStatusInfo(serverStatusInfoEvent->serviceName, serverStatusInfoEvent->connectionStatus);
        }
        break;
        case eEventBTDeviceStatus:
        case eEventA2DPDeviceStatus:
        case eLunaEventA2DPStatus:
        case eEventA2DPSourceStatus:
        case eEventBTAdapter:
        {
            PM_LOG_INFO(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: Calling eventKeyInfo");
            events::EVENT_KEY_INFO_T *keyInfoEvent = (events::EVENT_KEY_INFO_T*)event;
            eventKeyInfo(keyInfoEvent->type, keyInfoEvent->message);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_BLUETOOTH_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}
