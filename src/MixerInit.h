// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef MIXERINIT_H_
#define MIXERINIT_H_

#include <string>
#include <sstream>
#include <iostream>
#include <pbnjson/cxx/JValue.h>
#include <set>
class MixerInit
{
    private:

        /*to store json confgi file path*/
        const std::string mCongifPath;

        std::set<std::string> mSetMixer;

        /*Jason parser for mixer type*/
        bool parseAudioMixer(pbnjson::JValue element);

   public:

        /*Constructor*/
        MixerInit(const std::stringstream &configFilePath);

        /*Distructor*/
        ~MixerInit();

        /*Initializing the mixer objects based on the jason config*/
        void initMixerInterface();

        /*Json config parser*/
        bool readMixerConfig();
};
#endif