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

#ifndef _VOICECOMMAND_H_
#define _VOICECOMMAND_H_

#include "scenario.h"

class VoiceCommandScenarioModule : public ScenarioModule
{
public:
    VoiceCommandScenarioModule();

    virtual void    programControlVolume();
    virtual void    programState();

    // evoicedial
    void            programVoiceCommandVolume(bool ramp);

    virtual void    onActivating();

    // startVoiceCommand: Request to engage voice command.
    //    Returns true if voice command was successfully engaged.
    //     Returns false if mode was not changed and errorMessage
    // is set to describe why (if a value is passed).
    bool startVoiceCommand(const char * source,
                                       const char ** errorMessage = 0);
    void stopVoiceCommand(const char * source);
    void endVoiceCommand(const char * source);

    // Carrier/device policy. Does this device do Voice Command?
    void setIsEnabled(bool available){ mVoiceCommandEnabled = available; }
    bool isEnabled()    { return mVoiceCommandEnabled; }

private:
    // external users should use startVoiceCommand()
    bool makeCurrent()    { return ScenarioModule::makeCurrent(); }

    bool    mVoiceCommandEnabled;

    Volume    mFrontSpeakerVolume;
    Volume    mHeadsetVolume;
    Volume    mBluetoothVolume;

    Volume    mFrontMicGain;
    Volume    mHeadsetMicGain;
    Volume    mBluetoothSCOMicGain;
};

VoiceCommandScenarioModule * getVoiceCommandModule();

#endif // _VOICECOMMAND_H_
