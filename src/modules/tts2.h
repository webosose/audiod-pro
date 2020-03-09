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

#ifndef _TTS2_H_
#define _TTS2_H_

#include "scenario.h"

class TTS2ScenarioModule : public ScenarioModule
{
public:
    TTS2ScenarioModule();
    ~TTS2ScenarioModule();
    virtual void    programControlVolume();
    void            programTTS2Volumes(bool ramp);
    void            onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType);

private:
    Volume            mTTS2Volume;
};

TTS2ScenarioModule * getTTS2Module();

#endif // _TTS2_H_
