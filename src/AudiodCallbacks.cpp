// Copyright (c) 2012-2020 LG Electronics, Inc.
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

#include "AudiodCallbacks.h"
#include "genericScenarioModule.h"
#include "scenario.h"
#include "state.h"

#include <algorithm>

AudiodCallbacks gAudiodCallbacks;

void AudiodCallbacks::onAudioMixerConnected()
{
    if (ScenarioModule * module = dynamic_cast <ScenarioModule *> (ScenarioModule::getCurrent()))
    {
        module -> programSoftwareMixer(false);
    }
}

void AudiodCallbacks::onSinkChanged(EVirtualAudiodSink sink, EControlEvent event,ESinkType sinkType)
{
    g_debug ("onSinkChanged: sink(%d) Control Event(%d) Sink Type %d", sink, event,(int)sinkType);
    CallbackVector &    callbacks =  mSinkCallbackModules[sink];
    for (CallbackVector::iterator iter = callbacks.begin();
          iter != callbacks.end(); ++iter)
    {
         g_message ("Firing onSinkChanged for module: (%s)",
                                            (*iter)->getCategory());
        (*iter)->onSinkChanged(sink, event, sinkType);
    }
}

void AudiodCallbacks::onInputStreamActiveChanged(bool active)
{
    gState.setActiveInputStream(active);
}

void AudiodCallbacks::registerModuleCallback (GenericScenarioModule * module,
                                              EVirtualAudiodSink sink,
                                              bool notifyFirst)
{
    g_debug("entering function %s : SINK = %d notifyFirst = %d ", __FUNCTION__,
                                                            sink, notifyFirst);
    int first = sink;
    int last = sink;

    if (sink == eVirtualSink_All)
    {
        first = eVirtualSink_First;
        last = eVirtualSink_Last;
    }
    else if (sink == eVirtualUMISink_All)
    {
        first = eVirtualUMISink_First;
        last = eVirtualUMISink_Last;
    }
    else if (sink == eAllSink)
    {
        first = eVirtualSink_First;
        last = eVirtualUMISink_Last;
    }
    for (int i = first ; i <= last ; i++)
    {
        if (notifyFirst)
        {
            mSinkCallbackModules[i].insert(mSinkCallbackModules[i].begin(), module);
        }
        else
        {
            mSinkCallbackModules[i].push_back(module);
        }
    }
}

void AudiodCallbacks::unregisterModuleCallback (GenericScenarioModule * module)
{
    for (int i = eVirtualSink_First ; i <= eVirtualSink_Last ; i++)
    {
        CallbackVector &    callbacks =  mSinkCallbackModules[i];
        // 'remove' moves the selected elements
        // at the end and they need to be erased...
        callbacks.erase(std::remove(callbacks.begin(),
                                    callbacks.end(), module),
                                    callbacks.end());
    }
}


