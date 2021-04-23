/* @@@LICENSE
*
*      Copyright (c) 2021 LG Electronics Company.
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

#include "deviceRoutingConfigParser.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#define CONFIG_DIR_PATH "/etc/palm/audiod"

DeviceRoutingConfigParser::DeviceRoutingConfigParser()
{
    PM_LOG_DEBUG("DeviceRoutingConfigParser Constructor called");
}

DeviceRoutingConfigParser::~DeviceRoutingConfigParser()
{
    PM_LOG_DEBUG("DeviceRoutingConfigParser destructor called");
}

bool DeviceRoutingConfigParser::loadDeviceRoutingJsonConfig()
{
    bool loadStatus = true;
    std::stringstream jsonFilePath;
    jsonFilePath << CONFIG_DIR_PATH << "/" << DEVICE_ROUTING_CONFIG;

    PM_LOG_INFO(MSGID_DEVICE_ROUTING_CONFIG_PARSER, INIT_KVCOUNT, "Loading device rouitng config from json file %s",\
                jsonFilePath.str().c_str());
    fileJsonDeviceRoutingConfig = pbnjson::JDomParser::fromFile(jsonFilePath.str().c_str(),\
        pbnjson::JSchema::AllSchema());
    if (!fileJsonDeviceRoutingConfig.isValid() || !fileJsonDeviceRoutingConfig.isObject())
    {
        PM_LOG_ERROR(MSGID_INVALID_INPUT, INIT_KVCOUNT,\
            "fileJsonDeviceRoutingConfig : Failed to parse json config file, using defaults. File: %s",\
            jsonFilePath.str().c_str());
        loadStatus = false;
    }
    return loadStatus;
}

pbnjson::JValue DeviceRoutingConfigParser::getDeviceRoutingInfo()
{
    PM_LOG_INFO(MSGID_DEVICE_ROUTING_CONFIG_PARSER, INIT_KVCOUNT, "getDeviceRoutingInfo");
    pbnjson::JValue deviceRoutingInfo = pbnjson::Object();
    deviceRoutingInfo = fileJsonDeviceRoutingConfig["routingInfo"];
    PM_LOG_INFO(MSGID_DEVICE_ROUTING_CONFIG_PARSER, INIT_KVCOUNT, "deviceRoutingInfo json value is %s",\
        deviceRoutingInfo.stringify().c_str());
    return deviceRoutingInfo;
}