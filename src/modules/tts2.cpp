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

#include "tts2.h"
#include "module.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"
#include "AudioDevice.h"
#include "log.h"
#include "AudiodCallbacks.h"

static TTS2ScenarioModule * sTTS2Module = 0;

static const ConstString    cTTS2_Default("tts2" SCENARIO_DEFAULT);

TTS2ScenarioModule * getTTS2Module()
{
    return sTTS2Module;
}

void
TTS2ScenarioModule::programControlVolume ()
{
    programTTS2Volumes(false);
}

void
TTS2ScenarioModule::programTTS2Volumes(bool ramp)
{
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();
    programVolume(etts2, activeStreams.contain(etts2) ? 90 : 0, ramp);
}

void
TTS2ScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
    g_debug("TTS2ScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
    virtualSinkName(sink), controlEventName(event),(int)p_eSinkType);

    if(etts2 == sink)
    {
        programTTS2Volumes(false);
    }
    //TODO policies for TTS2
}

TTS2ScenarioModule::TTS2ScenarioModule() :
    ScenarioModule(ConstString("/tts2")),
    mTTS2Volume(cTTS2_Default, 90)
{
    gAudiodCallbacks.registerModuleCallback(this, etts2, true);
    gAudiodCallbacks.registerModuleCallback(this, edefault2, true);
    Scenario *s = new Scenario (cTTS2_Default,
                                true,
                                eScenarioPriority_TTS2_Default,
                                mTTS2Volume);
    addScenario (s);

    restorePreferences();
}

TTS2ScenarioModule::~TTS2ScenarioModule()
{
    delete sTTS2Module;
    sTTS2Module = nullptr;
}

int
TTS2InterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sTTS2Module = new TTS2ScenarioModule();

    return 0;
}

MODULE_START_FUNC (TTS2InterfaceInit);

