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

#include "IPC_SharedAudiodProperties.h"

#include "IPC_Property.hpp"

#include <cstdlib>

IPC_SharedAudiodProperties * gAudiodProperties = 0;

IPC_SharedAudiodProperties::
                 IPC_SharedAudiodProperties(const std::string & name) :
                 IPC_SharedProperties(name, sizeof(IPC_SharedAudiodProperties))
{
    /* mRingerOn.init done from false to true */
    mRingerOn.init(true);
    mTouchOn.init(false);
    mAlarmOn.init(true);
    mPhoneLocked.init(false);
    mDisplayOn.init(false);
    mRecordingAudio.init(false);
    mPausingMediaForPhoneCall.init(false);
    mTimerOn.init(true);
    mRingtoneWithVibration.init(true);
    // make sure every transaction is sent
    //(repeating the same request will be send a new request)
    mMediaServerCmd.setFlag(IPC_PropertyBase::
                              ePropertyFlag_NotifyClientsEvenIfNoChange);
}

IPC_SharedAudiodProperties * IPC_SharedAudiodProperties::getInstance()
{
    if (gAudiodProperties == 0)
        gAudiodProperties = IPC_SharedProperties::
                               createSharedProperties
                                 <IPC_SharedAudiodProperties>
                                    ("Audiod-Shared-Properties");
    if (!VERIFY(gAudiodProperties))
        exit(1);
    return gAudiodProperties;
}

int SharedPropertiesInit()
{
    IPC_SharedAudiodProperties::getInstance();
    return 0;
}
