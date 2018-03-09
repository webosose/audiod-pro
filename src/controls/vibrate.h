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


#ifndef _VIBRATE_H_
#define _VIBRATE_H_

#include <lunaservice.h>

#include "ConstString.h"
#include "log.h"

/// Shared methods to do vibration
class VibrateDevice
{
public:
    VibrateDevice() : mFakeBuzzRunning(false), mVibrateToken(0), isVibrating(false), mAmplitude(5) {}
    virtual ~VibrateDevice() {}

    /// Give access to the appropriate platform specific HW object.
    // There can be only one, and that's how audiod gets to access it.
    static VibrateDevice &    get();

    //! One shot vibration for alerts or notifications. Can NOT be canceled!
    void        realVibrate(bool alertNotNotification);

    //! Vibrate request, with detailed spec, like: '{"period":20, "duration":100}'
    void        realVibrate(const char * vibrateSpec);

    //! Cancel ringtone-pattern like vibration started with startVibrate()
    void            cancelVibrate();

    //! for private use only. Public because global callbacks need them... :-(
    void            fakeBuzz(int runningCount = 0);
    void            startVibrate(bool fakeVibrateIfCantVibrate);
    void            setVibrateAmplitude(int amplitude);
    void            startFakeBuzz();

protected:
    bool            mFakeBuzzRunning;
    LSMessageToken    mVibrateToken;
    bool isVibrating;
    int mAmplitude;
};

VibrateDevice * getVibrateDevice();

void cancelVibrate ();

#endif // _VIBRATE_H_
