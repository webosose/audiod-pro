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

#include "soundOutputListParser.h"

SoundOutputListParser::SoundOutputListParser()
{
    PM_LOG_INFO(MSGID_SOUNDOUTPUT_LIST_PARSER, INIT_KVCOUNT,\
        "SoundOutputListParser Constructor called");
}

SoundOutputListParser::~SoundOutputListParser()
{
    PM_LOG_INFO(MSGID_SOUNDOUTPUT_LIST_PARSER, INIT_KVCOUNT,\
        "SoundOutputListParser destructor called");
}

bool SoundOutputListParser::loadSoundOutputListJsonConfig()
{
    bool loadStatus = true;
    std::stringstream jsonFilePath;
    jsonFilePath << CONFIG_DIR_PATH << "/" << SOUNDOUTPUT_LIST_CONFIG;

    PM_LOG_INFO(MSGID_SOUNDOUTPUT_LIST_PARSER, INIT_KVCOUNT, "Loading soundoutput list from json file %s",\
                jsonFilePath.str().c_str());
    fileJsonSoundOutputListConfig = pbnjson::JDomParser::fromFile(jsonFilePath.str().c_str(),\
        pbnjson::JSchema::AllSchema());
    if (!fileJsonSoundOutputListConfig.isValid() || !fileJsonSoundOutputListConfig.isObject())
    {
        PM_LOG_ERROR(MSGID_INVALID_INPUT, INIT_KVCOUNT,\
            "loadSoundOutputListJsonConfig : Failed to parse json config file, using defaults. File: %s",\
            jsonFilePath.str().c_str());
        loadStatus = false;
    }
    return loadStatus;
}

pbnjson::JValue SoundOutputListParser::getSoundOutputListInfo()
{
    PM_LOG_INFO(MSGID_SOUNDOUTPUT_LIST_PARSER, INIT_KVCOUNT, "getSoundOutputListInfo");
    pbnjson::JValue soundOutputListInfo = pbnjson::Object();
    soundOutputListInfo = fileJsonSoundOutputListConfig["soundOutputList"];
    PM_LOG_INFO(MSGID_SOUNDOUTPUT_LIST_PARSER, INIT_KVCOUNT, "Soundoutput list info json value is %s",\
        soundOutputListInfo.stringify().c_str());
    return soundOutputListInfo;
}