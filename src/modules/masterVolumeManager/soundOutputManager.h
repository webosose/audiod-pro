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

#ifndef _SOUNDOUTPUT_MGR_H_
#define _SOUNDOUTPUT_MGR_H_

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <lunaservice.h>
#include "utils.h"
#include "main.h"
#include "audioMixer.h"
#include "soundOutputListParser.h"

class SoundOutputManager
{
private:
    SoundOutputManager(const SoundOutputManager&) = delete;
    SoundOutputManager& operator=(const SoundOutputManager&) = delete;
    void readSoundOutputListInfo();
    void printSoundOutputListInfo();
    bool initializeSoundOutputList(const pbnjson::JValue& soundOutputListInfo);
    std::string soundMode;
    std::map<std::string, utils::SOUNDOUTPUT_LIST_T> mSoundOutputInfoMap;
    AudioMixer *mObjAudioMixer;
    SoundOutputListParser* mObjSoundOutputListParser;

public:
    SoundOutputManager();
    ~SoundOutputManager();

    static bool _SetSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx);
    static bool _updateSoundOutStatus(LSHandle *sh, LSMessage *reply, void *ctx);
};

#endif // _SOUNDOUTPUT_MGR_H_