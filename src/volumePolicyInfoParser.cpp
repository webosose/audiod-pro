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

#include "volumePolicyInfoParser.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#define CONFIG_DIR_PATH "/etc/palm/audiod"

VolumePolicyInfoParser::VolumePolicyInfoParser()
{
    PM_LOG_INFO(MSGID_POLICY_CONFIGURATOR, INIT_KVCOUNT,\
        "VolumePolicyInfoParser Constructor called");
}

VolumePolicyInfoParser::~VolumePolicyInfoParser()
{
    PM_LOG_INFO(MSGID_POLICY_CONFIGURATOR, INIT_KVCOUNT,\
        "VolumePolicyInfoParser destructor called");
}

bool VolumePolicyInfoParser::loadVolumePolicyJsonConfig()
{
    bool loadStatus = true;
    std::stringstream jsonFilePath;
    jsonFilePath << CONFIG_DIR_PATH << "/" << VOLUME_POLICY_CONFIG;

    PM_LOG_INFO(MSGID_POLICY_CONFIGURATOR, INIT_KVCOUNT, "Loading volume policy info from json file %s",\
        jsonFilePath.str().c_str());
    fileJsonVolumePolicyConfig = pbnjson::JDomParser::fromFile(jsonFilePath.str().c_str(),\
        pbnjson::JSchema::AllSchema());
    if (!fileJsonVolumePolicyConfig.isValid() || !fileJsonVolumePolicyConfig.isObject())
    {
        PM_LOG_ERROR(MSGID_INVALID_INPUT, INIT_KVCOUNT,\
            "loadVolumePolicyJsonConfig : Failed to parse json config file, using defaults. File: %s",\
            jsonFilePath.str().c_str());
        loadStatus = false;
    }
    return loadStatus;
}

pbnjson::JValue VolumePolicyInfoParser::getVolumePolicyInfo()
{
    PM_LOG_INFO(MSGID_POLICY_CONFIGURATOR, INIT_KVCOUNT, "getVolumePolicyInfo");
    pbnjson::JValue policyVolumeInfo = pbnjson::Object();
    policyVolumeInfo = fileJsonVolumePolicyConfig["streamDetails"];
    PM_LOG_INFO(MSGID_POLICY_CONFIGURATOR, INIT_KVCOUNT, "Policy info json value is %s",\
        policyVolumeInfo.stringify().c_str());
    return policyVolumeInfo;
}