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

#ifndef VOLUMECONTROLCHANGESMONITOR_H_
#define VOLUMECONTROLCHANGESMONITOR_H_

class GenericScenarioModule;

// When changing volume control, use this class to make sure appropriate
// action is taken when control changes
// BT needs to know when the media module explicitly
// controlling the volume to turn A2DP on & off

class VolumeControlChangesMonitor
{
public:
    VolumeControlChangesMonitor();
    ~VolumeControlChangesMonitor();

    // call this when the media module gained or lost explicit volume control
    static void mediaModuleControllingVolumeChanged();

private:
    GenericScenarioModule *    mPreviouslyControllingModule;
};

#endif /* VOLUMECONTROLCHANGESMONITOR_H_ */
