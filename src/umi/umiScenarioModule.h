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

#ifndef _UMI_SCENARIO_MODULE_H_
#define _UMI_SCENARIO_MODULE_H_

#include "umiaudiomixer.h"

typedef struct
{
    std::string sinkName;
    std::string sourceName;
}sourceSinkInfos;

class UMIScenarioModule : public GenericScenarioModule {
protected :
    umiaudiomixer * mixerHandle;
    sourceSinkInfos sourceSinkInfo;
public:
    UMIScenarioModule(const ConstString & category) : GenericScenarioModule(category)
    {
        mixerHandle = umiaudiomixer::getUmiMixerInstance();
    }

    virtual bool connectAudioOut(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
    virtual bool disconnectAudioOut(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;

    virtual ~UMIScenarioModule() {};
};

#endif //_UMI_SCENARIO_MODULE_H_
