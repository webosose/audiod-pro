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

#ifndef AUDIODCALLBACKS_H_
#define AUDIODCALLBACKS_H_

#include <vector>

#include "AudioMixer.h"
#include "AudioDevice.h"

class ScenarioModule;

class AudiodCallbacks : public AudiodCallbacksInterface
{
public:
    /// Methods called by HW/SW implementation layers
    virtual void        onAudioMixerConnected();
    virtual void        onSinkChanged(EVirtualSink sink, EControlEvent event,ESinkType sinkType);
    virtual void        onInputStreamActiveChanged(bool active);

    // Modules can register to be notified via onSinkChanged()
    // that a sink was opened or closed.
    void                registerModuleCallback(GenericScenarioModule * module,
                                               EVirtualSink sink,
                                                bool notifyFirst = false);
    void                unregisterModuleCallback(GenericScenarioModule * module);

private:
    typedef std::vector<GenericScenarioModule *> CallbackVector;

    CallbackVector        mSinkCallbackModules[eumiCount];
};


extern AudiodCallbacks gAudiodCallbacks;

#endif /* AUDIODCALLBACKS_H_ */
