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

#include "scenario.h"
#include "umiScenarioModule.h"

class MockScenarioModule : public UMIScenarioModule
{
    private:
        Volume  mMockScenarioModuleVolume;
        bool    mMockScenarioModuleMuted;
    public :
        MockScenarioModule();
        ~MockScenarioModule();
        void programSoftwareMixer (bool ramp, bool muteMediaSink = false) {}
        void onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType);

        bool connectAudioOut(LSHandle *lshandle, LSMessage *message, void *ctx);
        bool disconnectAudioOut(LSHandle *lshandle, LSMessage *message, void *ctx);

        static bool _connectStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _disConnectStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx);

        static MockScenarioModule * getMockScenarioModule();
        void _updateHardwareSettings(bool muteMediaSink = false) {}
};
