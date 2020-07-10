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

#ifndef _MODULE_INTERFACE_H_
#define _MODULE_INTERFACE_H_

#include <string>
#include "utils.h"

class ModuleInterface
{
    public:
        //constructor
        ModuleInterface();
        //destructor
        virtual ~ModuleInterface();

        virtual void eventSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
            utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType){}
        virtual void eventMixerStatus(bool mixerStatus, utils::EMIXER_TYPE mixerType){}
        virtual void eventInputVolume(EVirtualAudioSink audioSink, const int& volume, const bool& ramp){}
};

#endif//_MODULE_INTERFACE_H_
