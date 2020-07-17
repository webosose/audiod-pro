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

#ifndef _SOUND_OUTPUT_LIST_PARSER_H_
#define _SOUND_OUTPUT_LIST_PARSER_H_

#include "utils.h"
#include <map>
#include <string>
#include <pbnjson.h>
#include <pbnjson.hpp>
#include <pulse/module-palm-policy.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#define CONFIG_DIR_PATH "/etc/palm/audiod"
#define SOUNDOUTPUT_LIST_CONFIG "audiod_sound_output_config.json"

class SoundOutputListParser
{
    private:
    SoundOutputListParser(const SoundOutputListParser&) = delete;
    SoundOutputListParser& operator=(const SoundOutputListParser&) = delete;
    pbnjson::JValue fileJsonSoundOutputListConfig;

    public:
    SoundOutputListParser();
    ~SoundOutputListParser();
    pbnjson::JValue getSoundOutputListInfo();
    bool loadSoundOutputListJsonConfig();
};

#endif // _SOUND_OUTPUT_LIST_PARSER_H_