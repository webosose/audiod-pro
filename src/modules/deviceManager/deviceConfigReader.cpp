/* @@@LICENSE
*
*      Copyright (c) 2022 LG Electronics Company.
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

#include "deviceConfigReader.h"

bool deviceConfigReader::loadDeviceInfoJson()
{
    bool loadStatus = true;
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s",__FUNCTION__);

    mFileJsonDeviceConfg = pbnjson::JDomParser::fromFile(mJsonFilePath.c_str(),\
        pbnjson::JSchema::AllSchema());

    if (!mFileJsonDeviceConfg.isValid() || !mFileJsonDeviceConfg.isObject())
    {
        PM_LOG_ERROR(MSGID_INVALID_INPUT, INIT_KVCOUNT,\
            "loadDeviceInfoJson : Failed to parse json config file, using defaults. File: %s",\
            mJsonFilePath.c_str());
        loadStatus = false;
    }
    return loadStatus;
}

pbnjson::JValue deviceConfigReader::getDeviceInfo()
{
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"%s",__FUNCTION__);
    pbnjson::JValue deviceInfo = pbnjson::Object();
    deviceInfo = mFileJsonDeviceConfg["DeviceList"];
    PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "getDeviceInfo json value is %s",\
        deviceInfo.stringify().c_str());
    return deviceInfo;
}