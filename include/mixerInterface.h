/* @@@LICENSE
*
*      Copyright (c) 2020-2024 LG Electronics Company.
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

#ifndef _MIXER_INTERFACE_H_
#define _MIXER_INTERFACE_H_

#include <iostream>
#include <cstdlib>
#include "utils.h"

class MixerInterface
{
    public:
        MixerInterface (){};
        virtual ~MixerInterface (){};
        virtual void callBackSinkStatus(const std::string& source, const std::string& sink, EVirtualAudioSink audioSink, \
              utils::ESINK_STATUS sinkStatus, utils::EMIXER_TYPE mixerType,const int& sinkIndex = -1, const std::string& trackId="") = 0;
         virtual void callBackSourceStatus(const std::string& source, const std::string& sink, EVirtualSource audioSource, \
            utils::ESINK_STATUS sourceStatus, utils::EMIXER_TYPE mixerType) = 0;
        virtual void callBackMixerStatus(const bool& mixerStatus, utils::EMIXER_TYPE mixerType) = 0;
        virtual void callBackMasterVolumeStatus() = 0;
        virtual void callBackDeviceConnectionStatus(const std::string &deviceName, const std::string &deviceNameDetail, const std::string &deviceIcon, utils::E_DEVICE_STATUS deviceStatus, utils::EMIXER_TYPE mixerType, const bool& isOutput) = 0;
        virtual void callBackPlaybackStatusChanged(const std::string &playbackId, const std::string &state) = 0;

};
#endif