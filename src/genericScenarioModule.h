// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#ifndef _GENERIC_SCENARIO_MODULE_H_
#define _GENERIC_SCENARIO_MODULE_H_

#include "AudioMixer.h"
#include "ConstString.h"
#include "volume.h"
//#include "AudioDASS.h"
#include <map>
#include <string>

enum EScenarioPriority
{
    eScenarioPriority_Special_Lowest = -1,// used temporarily to force sorting
    eScenarioPriority_Media_Default, // Used only in case of DASS
    eScenarioPriority_Media_BackSpeaker = 10,
    eScenarioPriority_Media_Headset = 70,
    eScenarioPriority_Media_HeadsetMic = 70,
    eScenarioPriority_Media_A2DP = 50,
    eScenarioPriority_Media_Wireless = 50,
    eScenarioPriority_Media_FrontSpeaker = 5,

    eScenarioPriority_Phone_FrontSpeaker = 30,
    eScenarioPriority_Phone_BackSpeaker = 20,
    eScenarioPriority_Phone_Headset = 70,
    eScenarioPriority_Phone_HeadsetMic = 70,
    eScenarioPriority_Phone_BluetoothSCO = 50,
    eScenarioPriority_Phone_TTY_Full = 100,
    eScenarioPriority_Phone_TTY_HCO = 100,
    eScenarioPriority_Phone_TTY_VCO = 100,

    eScenarioPriority_VoiceCommand_BackSpeaker = 30,
    eScenarioPriority_VoiceCommand_Headset = 70,
    eScenarioPriority_VoiceCommand_HeadsetMic = 70,
    eScenarioPriority_VoiceCommand_BluetoothSCO = 50,

    eScenarioPriority_Vvm_BackSpeaker = 30,
    eScenarioPriority_Vvm_FrontSpeaker = 40,
    eScenarioPriority_Vvm_Headset = 70,
    eScenarioPriority_Vvm_HeadsetMic = 70,
    eScenarioPriority_Vvm_BluetoothSCO = 50,

    eScenarioPriority_Feedback = 50,

    eScenarioPriority_Nav_Default = 50,

    eScenarioPriority_Ringtone_Default = 50,

    eScenarioPriority_System_Default = 50,

    eScenarioPriority_Timer_Default = 50,

    eScenarioPriority_Alert_Default = 50,

    eScenarioPriority_TV_MasterVolume = 200,

    eScenarioPriority_TTS2_Default = 50,

    eScenarioPriority_Default2_Default = 50
};

enum ESendUpdate {
    eUpdate_Status,
    eUpdate_GetVolume,
    eUpdate_GetSoundOut
};

class Volume {
public:
    Volume(const ConstString &name, int defaultValue) : mName(name), mVolume(defaultValue) {}

    int get() const {
        return mVolume;
    }
    void set(int volume) {
        mVolume = volume;
    }
    const char * getName() const {
        return mName.c_str(); 
    }

    bool operator==(const Volume & rhs) const {
        return this == &rhs;
    }// are they the same object?

private:
    ConstString mName;
    int mVolume;
};

extern Volume cNoVolume;

class GenericScenario {
public:
    GenericScenario(const ConstString & name,
    bool enabled,
    EScenarioPriority priority,
    Volume & volume,
    Volume & micGain = cNoVolume);
    virtual ~GenericScenario() {}

    const char * getName() const {
        return mName.c_str();
    }

    ConstString mName;
    bool mEnabled;
    bool mHardwired; ///< Hardwired scenario are the ones enabled at creation
    EScenarioPriority mPriority;

    int getVolume() const {
        return mVolume.get();
    }
    void setVolume(int volume) {
        mVolume.set(volume);
    }

    int getMicGain() const 
    {return hasMicGain() ? mMicGain.get() : -1;}

    void setMicGain(int gain) {
        if (hasMicGain())
            mMicGain.set(gain);
    }

    int getVolumeOrMicGain(bool volumeNotMicGain) const 
    {
        return volumeNotMicGain ? mVolume.get() : mMicGain.get();
    }
    void setVolumeOrMicGain(int volume, bool volumeNotMicGain) 
    {
        volumeNotMicGain ? mVolume.set(volume) : mMicGain.set(volume);
    }
    bool  hasMicGain() const 
    {
        return &mMicGain != &cNoVolume;
    }

protected:

    Volume &mVolume;
    Volume &mMicGain;
};

class GenericScenarioModule {
public:
    typedef std::map< std::string, GenericScenario* > ScenarioMap;

    GenericScenarioModule(const ConstString & category);

    virtual ~GenericScenarioModule();

    virtual bool makeCurrent() {return true;};
    bool isCurrentModule() {
        return sCurrentModule && sCurrentModule == this;
    }
    static GenericScenarioModule * getCurrent(){return sCurrentModule;}

    const char * getCategory() const
    {return mCategory.c_str() + 1;}
    const char * getCurrentScenarioName() const
    {return mCurrentScenario ? mCurrentScenario->getName() : "none";}

    const GenericScenarioModule::ScenarioMap & getScenarios() const
    {return mScenarioTable;}
    bool addScenario(GenericScenario *s);

    GenericScenario * getScenario (const char * name);

    bool enableScenario (const char * name);
    bool disableScenario (const char * name);

    bool setCurrentScenario (const char * name);
    bool unsetCurrentScenario (const char * name);

    /// Get or set a scenario's volume
    bool setScenarioVolume (const char * name, int volume)
    {return setScenarioVolumeOrMicGain(name, volume, true);}
    bool getScenarioVolume (const char * name, int & volume)
    {return getScenarioVolumeOrMicGain(name, volume, true);}

    /// Get or set a scenario's mic gain
    bool setScenarioMicGain (const char * name, int micGain)
    {return setScenarioVolumeOrMicGain(name, micGain, false);}
    bool getScenarioMicGain (const char * name, int & micGain)
    {return getScenarioVolumeOrMicGain(name, micGain, false);}

    /// Get or set a scenario's volume or mic gain
    bool setScenarioVolumeOrMicGain (const char * name,
    int volume,
    bool volumeNotMicGain);
    bool getScenarioVolumeOrMicGain (const char * name,
    int & volume,
    bool volumeNotMicGain);

    /// Module store preferences for all scenario in
    // one large message to minimize number of transactions

    /// Save preferences in 10 seconds max
    void scheduleStorePreferences();
    /// Store preferences now
    void storePreferences();
    /// Restore preferences now
    void restorePreferences();

    bool setCurrentScenarioByPriority();

    bool registerMe (LSMethod *methods);
    bool broadcastEvent (const char *event);

    bool sendChangedUpdate (int changedFlags,
    const gchar * broadCastEvent = 0);
    bool sendRequestedUpdate (LSHandle *sh, LSMessage *message, bool subscribed);
    bool sendEnabledUpdate (const char *scenario,
    int enabledFlags);

    bool setVolumeOverride (bool override);
    int getVolumeOverride () {return mVolumeOverride;}

    virtual bool setMuted (bool muted);
    bool isMuted () {return mMuted; }

    /// Adjust volume of an alert depending as necessary in the current scenario
    virtual int adjustAlertVolume (int volume, bool alertStarting = false)
                {return volume;}

    /// Program volume of sinks that are
    //never muted. Return false if not performed.
    virtual int adjustSystemVolume(int volume);

    //virtual void programSoftwareMixer (bool ramp, bool muteMediaSink = false) = 0;
    virtual bool setVolumeUpDown (bool flag)
    {
        return true;
    }

    // When active module changes, these are called in this order
    // deactivated module is notified first
    virtual void onDeactivated() { }
    // module to activate is notified while the "old" routing is still active
    virtual void onActivating() { }

    ///< Called on setScenario. Program all controlled volumes at their expected levels.
    virtual void programControlVolume() { }
    ///< Program volumes controlled by FW rather than pulse. (Phone in particular).
    virtual void programHardwareVolume() { }
    virtual void programState() { }
    virtual void programMuted() { }
    virtual void programMuse(bool enable) { }
    virtual void programHac(bool enable) { }

    ConstString mCategory;

    virtual void setA2DPAddress(std::string address) { }

    bool subscriptionPost(LSHandle * palmService,
    std::string replyString, ESendUpdate update);

    virtual void onSinkChanged(EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType) {}

    GenericScenario * mCurrentScenario;

protected:
    ScenarioMap mScenarioTable;

    guint mStoreTimerID;

    bool mMuted;
    int mVolumeOverride;

    void _setCurrentScenarioByPriority();
    virtual void _updateHardwareSettings(bool muteMediaSink = false) = 0;

    static GenericScenarioModule * sCurrentModule;
};

#endif
