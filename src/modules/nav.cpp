// Copyright (c) 2012-2018 LG Electronics, Inc.
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>

#include <lunaservice.h>

#include "nav.h"
#include "module.h"
#include "state.h"
#include "phone.h"
#include "media.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"

static NavigationScenarioModule * sNavModule = 0;

static const ConstString    cNav_Default("nav" SCENARIO_DEFAULT);

NavigationScenarioModule * getNavModule()
{
    return sNavModule;
}

#if defined(AUDIOD_PALM_LEGACY)
static LSMethod navMethods[] = {

    { cModuleMethod_Status, _status},
    { },
};
#endif

NavigationScenarioModule::NavigationScenarioModule() :
    ScenarioModule(ConstString("/nav"))
{
    Scenario *s = new Scenario (cNav_Default,
                                true,
                                eScenarioPriority_Nav_Default,
                                cNoVolume);
    addScenario (s);

#if defined(AUDIOD_PALM_LEGACY)
    registerMe (navMethods);
#endif

    restorePreferences();
}

int
NavInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sNavModule = new NavigationScenarioModule();

    return 0;
}

MODULE_START_FUNC (NavInterfaceInit);
