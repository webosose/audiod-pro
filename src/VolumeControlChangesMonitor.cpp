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

#include "VolumeControlChangesMonitor.h"
#include "state.h"
#include "media.h"
#include "main.h"
#include "messageUtils.h"

VolumeControlChangesMonitor::VolumeControlChangesMonitor()
{
    mPreviouslyControllingModule = gState.getExplicitVolumeControlModule();
}

VolumeControlChangesMonitor::~VolumeControlChangesMonitor()
{
    ScenarioModule * newController = gState.getExplicitVolumeControlModule();
    if (newController != mPreviouslyControllingModule
            && (newController == getMediaModule() ||
               mPreviouslyControllingModule == getMediaModule()))
    {
        mediaModuleControllingVolumeChanged();
    }
}

/// BT server needs to know if the media module is explicitly
// controlling the media volume or not
/// so that it can turn on or off A2DP in time
void VolumeControlChangesMonitor::mediaModuleControllingVolumeChanged()
{
    if (gState.getBTServerRunning())
    {
        std::string param = string_printf("{\"enabled\": %s}",
                         (gState.getExplicitVolumeControlModule() ==
                                                           getMediaModule()) ?
                                                            "true" : "false");
        g_debug("VolumeControlChangesMonitor::mediaModuleControllingVolumeChanged:  \
                         sending '%s' to bluetooth server", param.c_str());

        CLSError lserror;
        LSHandle * sh = GetPalmService();
        if (!LSCall(sh, "palm://com.palm.bluetooth/a2dp/audioActivity",
                                    param.c_str(), NULL, NULL, NULL, &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
    }
}
