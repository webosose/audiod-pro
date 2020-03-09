/* @@@LICENSE
*
* Copyright (c) 2019-2020 LG Electronics Company.
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

#include "default2.h"
#include "module.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"
#include "AudioDevice.h"
#include "log.h"
#include "AudiodCallbacks.h"

#define DUCK_VOLUME 40
#define DEFAULT_VOLUME 90

static Default2ScenarioModule * sDefault2Module = 0;

static const ConstString cDefault2_Default("default2" SCENARIO_DEFAULT);

Default2ScenarioModule * getDefault2Module()
{
    return sDefault2Module;
}

void
Default2ScenarioModule::programControlVolume ()
{
    programDefault2Volumes(false);
}

void Default2ScenarioModule::setVolume (int volume)
{
    default2Volume = volume;
    programDefault2Volumes(false);
}

void
Default2ScenarioModule::programDefault2Volumes(bool ramp)
{
    int volume=0;
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();
    // Applying policy on default2 as tts2 is playing
    if (activeStreams.contain(edefault2) && activeStreams.contain(etts2))
        volume = DUCK_VOLUME;
    else if (activeStreams.contain(edefault2))
        volume = DEFAULT_VOLUME;
    g_debug("Default2ScenarioModule::programDefault2Volumes: sink volume: %d", volume);
    programVolume(edefault2, volume, ramp);
}

void
Default2ScenarioModule::onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType)
{
    g_debug("Default2ScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
    virtualSinkName(sink), controlEventName(event),(int)p_eSinkType);

    if(edefault2 == sink)
    {
        if (!gAudioMixer.getActiveStreams().contain(etts2))
            programDefault2Volumes(false);
    }

    //TODO policies for Default2
    if (eControlEvent_FirstStreamOpened == event)
    {
        if(etts2 == sink)
        {
            if (default2Volume > DUCK_VOLUME)
            {
                programVolume(edefault2, DUCK_VOLUME, true);
            }
        }
    }
    else if (eControlEvent_LastStreamClosed == event)
    {
        if (etts2 == sink)
        {
            if (!gAudioMixer.getActiveStreams().contain(etts2))
                programDefault2Volumes(false);
        }
    }
}

Default2ScenarioModule::Default2ScenarioModule() :
    ScenarioModule(ConstString("/default2")),
    mDefault2Volume(cDefault2_Default, DEFAULT_VOLUME),
    default2Volume(DEFAULT_VOLUME)
{
    gAudiodCallbacks.registerModuleCallback(this, edefault2, true);
    gAudiodCallbacks.registerModuleCallback(this, etts2, true);

    Scenario *s = new Scenario (cDefault2_Default,
                                true,
                                eScenarioPriority_Default2_Default,
                                mDefault2Volume);

    addScenario (s);

    restorePreferences();
}

Default2ScenarioModule::~Default2ScenarioModule()
{
    delete sDefault2Module;
    sDefault2Module = nullptr;
}

int
Default2InterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sDefault2Module = new Default2ScenarioModule();

    return 0;
}

MODULE_START_FUNC(Default2InterfaceInit);
