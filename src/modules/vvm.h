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

#ifndef _Vvm_H_
#define _Vvm_H_

#include "scenario.h"

class VvmScenarioModule : public ScenarioModule
{
public:
   VvmScenarioModule();

    virtual void   programControlVolume();
    virtual void   programState();

    // evoicedial
    void           programVvmVolume(bool ramp);

    virtual void   onActivating();

    // startVvm: Request to engage voice command.
    //   Returns true if voice command was successfully engaged.
    //    Returns false if mode was not changed
    // and errorMessage is set to describe why (if a value is passed).
    bool           startVvm(const char * source, const char ** errorMessage = 0);
    void            stopVvm(const char * source);
    void           endVvm(const char * source);

    // Carrier/device policy. Does this device do Voice Command?
    void           setIsEnabled(bool available)    { mVvmEnabled = available; }
    bool           isEnabled()                        { return mVvmEnabled; }

    bool            isActive()    { return mVvmActive; }
    void            setActive(bool active)   { mVvmActive = active; }

private:
    // external users should use startVvm()
    bool           makeCurrent()        { return ScenarioModule::makeCurrent(); }

    bool    mVvmActive;
    bool   mVvmEnabled;
   bool    mScoUp;

   Volume    mFrontSpeakerVolume;
   Volume    mHeadsetVolume;
   Volume    mBluetoothVolume;

    Volume   mFrontMicGain;
    Volume   mHeadsetMicGain;
    Volume   mBluetoothSCOMicGain;
};

VvmScenarioModule * getVvmModule();

#endif // _Vvm_H_
