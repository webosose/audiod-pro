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

#ifndef IPC_SHAREDAUDIODPROPERTIES_H_
#define IPC_SHAREDAUDIODPROPERTIES_H_

#include "IPC_Property.h"
#include "IPC_SharedAudiodDefinitions.h"

// Map AudiodProperty on server or client property using a
// #define to avoid having to redefine constructors
#ifdef AUDIOD_IPC_SERVER
    #define AudiodProperty IPC_ServerProperty
#else
    #define AudiodProperty IPC_ClientProperty
#endif

template <class T, T initValue> class InitializedProperty : public AudiodProperty<T>
{
public:
    InitializedProperty() : AudiodProperty<T>(initValue) {}
};

template <class T, T min, T max, T initValue = min,
                                        IPC_PropertyBase::
                                        EPropertyFlag propertyFlags =
                                        IPC_PropertyBase::ePropertyFlag_None> class EnumAudiodProperty :
                                        public AudiodProperty<T>
{
public:
    EnumAudiodProperty() : AudiodProperty<T>(initValue)
    {
        if (propertyFlags != IPC_PropertyBase::ePropertyFlag_None)
            this->setFlag(propertyFlags);
    }

    void    setLocalValue(const T & newValue)
    {    // ignore illegal values, don't change them into some random legal
         //one! Use operator< only to maximize compatibility with any type
        if (VERIFY(!(newValue < min || max < newValue)))
            AudiodProperty<T>::setLocalValue(newValue);
    }
};

class PhoneStatus : public EnumAudiodProperty<EPhoneStatus,
                                              ePhoneStatus_First,
                                              ePhonestatus_Last,
                                              ePhoneStatus_Disconnected> {};
class HeadsetState : public EnumAudiodProperty<EHeadsetState,
                                              eHeadsetState_First,
                                              eHeadsetState_Last,
                                              eHeadsetState_None> {};
class MediaserverCommand : public EnumAudiodProperty<EMediaServerCommand,
                                                    eMediaServerCommand_First,
                                                    eMediaServerCommand_Last,
                                                    eMediaServerCommand_Null,
                                                    IPC_PropertyBase::
                                                    ePropertyFlag_NoNotificationOnConnect> {};
class AudiodCommand : public EnumAudiodProperty<EAudiodCommand,
                                               eAudiodCommand_First,
                                               eAudiodCommand_Last,
                                               eAudiodCommand_Null,
                                               IPC_PropertyBase::
                                               ePropertyFlag_NoNotificationOnConnect> {};

class IPC_SharedAudiodProperties : public IPC_SharedProperties {
public:
    static const bool cIsServerNotClient =
                                      AudiodProperty<bool>::cIsServerNotClient;

    // phone state. Are we between calls, dialing,
    // getting an incoming call, or on a call?
    PhoneStatus                mPhoneStatus;

    // Is a headset present? What kind?
    HeadsetState            mHeadsetState;

    // ringer switch state. True for on (should ring),
    // false for off (should not ring)
    AudiodProperty<bool>    mRingerOn;
    AudiodProperty<bool>    mTouchOn;
    AudiodProperty<bool>    mAlarmOn;
    AudiodProperty<bool>    mTimerOn;
    AudiodProperty<bool>    mBalanceVolume;
    AudiodProperty<bool>    mRingtoneWithVibration;

    // phone locked. True when the user must unlock the phone first.
    AudiodProperty<bool>    mPhoneLocked;

    // display on. True when the display is active.
    AudiodProperty<bool>    mDisplayOn;

    // Managed by mediaserver. Used by audiod to
    // unmute mic if necessary (first used by Pixie)
    AudiodProperty<bool>    mRecordingAudio;

    // Signal that all media playback should be paused during a phone call.
    // Might start when the phone is ringing, depending on the ringer switch
    AudiodProperty<bool>    mPausingMediaForPhoneCall;

    // Commands sent by audiod to mediaserver.
    // Interpreted by mediaserver. Use notifications only.
    MediaserverCommand        mMediaServerCmd;

    // Commands sent by mediaserver to audiod.
    // Interpreted by audiod. Value never changed!
    AudiodCommand            mAudiodCmd;

    // Getter static method, that will also initialize things
    // if necessary (just call it once to initialize).
    static IPC_SharedAudiodProperties * getInstance();

protected:
    IPC_SharedAudiodProperties(const std::string & name);
    friend class IPC_SharedProperties;
};

// Obsolete. Use IPC_SharedAudiodProperties::getInstance() instead.
int SharedPropertiesInit();

extern IPC_SharedAudiodProperties * gAudiodProperties;

#endif /* IPC_SHAREDAUDIODPROPERTIES_H_ */
