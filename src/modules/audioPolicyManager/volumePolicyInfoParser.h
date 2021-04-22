// Copyright (c) 2020-2021 LG Electronics, Inc.
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

#ifndef _VOLUME_POLICY_INFO_PARSER_H_
#define _VOLUME_POLICY_INFO_PARSER_H_

#include <map>
#include <string>
#include <pbnjson.h>
#include <pbnjson.hpp>
#include <pulse/module-palm-policy.h>
#include "utils.h"
#define VOLUME_POLICY_CONFIG "audiod_sink_volume_policy_config.json"
#define SOURCE_VOLUME_POLICY_CONFIG "audiod_source_volume_policy_config.json"

class VolumePolicyInfoParser
{
    private:
        VolumePolicyInfoParser(const VolumePolicyInfoParser&) = delete;
        VolumePolicyInfoParser& operator=(const VolumePolicyInfoParser&) = delete;
        pbnjson::JValue fileJsonVolumePolicyConfig;
        pbnjson::JValue fileJsonSourceVolumePolicyConfig;

    public:
        VolumePolicyInfoParser();
        ~VolumePolicyInfoParser();
        pbnjson::JValue getVolumePolicyInfo(const bool& isSink);
        bool loadVolumePolicyJsonConfig();
};

#endif // _VOLUME_POLICY_INFO_PARSER_H_
