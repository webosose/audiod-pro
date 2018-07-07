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

#ifndef _SCENARIO_H_
#define _SCENARIO_H_

#include "AudioMixer.h"
#include "ConstString.h"
#include "volume.h"

#include <map>

#define VOLUME_STORE_DELAY 10000

enum EScenarioPriority
{
    eScenarioPriority_Special_Lowest = -1,// used temporarily to force sorting
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

};

enum ESendUpdate {
    eUpdate_Status,
    eUpdate_GetVolume,
    eUpdate_GetSoundOut
};

struct ScenarioRoute {
    ScenarioRoute() : mDestination(eMainSink), mRouted(false) {}

    int            mDestination;
    bool            mRouted;
};

/// Scenario names: always use these definitions rather than constants!
extern const ConstString    cMedia_Default;
extern const ConstString    cMedia_BackSpeaker;
extern const ConstString    cMedia_Headset;
extern const ConstString    cMedia_HeadsetMic;
extern const ConstString    cMedia_A2DP;
extern const ConstString    cMedia_Wireless;
extern const ConstString    cMedia_FrontSpeaker;

extern const ConstString    cMedia_Mic_Front;
extern const ConstString    cMedia_Mic_HeadsetMic;

extern const ConstString    cPhone_FrontSpeaker;
extern const ConstString    cPhone_BackSpeaker;
extern const ConstString    cPhone_Headset;
extern const ConstString    cPhone_HeadsetMic;
extern const ConstString    cPhone_BluetoothSCO;
extern const ConstString    cPhone_TTY_Full;
extern const ConstString    cPhone_TTY_HCO;
extern const ConstString    cPhone_TTY_VCO;

extern const ConstString    cVoiceCommand_BackSpeaker;
extern const ConstString    cVoiceCommand_Headset;
extern const ConstString    cVoiceCommand_HeadsetMic;
extern const ConstString    cVoiceCommand_BluetoothSCO;

extern const ConstString    cVoiceCommand_Mic_Front;
extern const ConstString    cVoiceCommand_Mic_HeadsetMic;
extern const ConstString    cVoiceCommand_Mic_BluetoothSCO;

extern const ConstString    cVvm_BackSpeaker;
extern const ConstString    cVvm_FrontSpeaker;
extern const ConstString    cVvm_Headset;
extern const ConstString    cVvm_HeadsetMic;
extern const ConstString    cVvm_BluetoothSCO;

extern const ConstString    cVvm_Mic_Front;
extern const ConstString    cVvm_Mic_HeadsetMic;
extern const ConstString    cVvm_Mic_BluetoothSCO;


///
//    The Volume class holds a volume for a particular configuration case.
//  A volume can be shared between scenario,
// hence the referencing to a volume rather than the inclusion in a scenario.
//  Volume objects are best members of the scenario
// module that owns the scenario that uses it.
//  Volumes are named for convenience.
///
class Volume {
public:
    Volume(const ConstString & name, int defaultValue) : mName(name), mVolume(defaultValue) {}

    int                get() const   { return mVolume; }
    void            set(int volume)  { mVolume = volume; }
    const char *    getName() const  { return mName.c_str(); }

    bool            operator==(const Volume & rhs) const
                        { return this == &rhs; }    // are they the same object?

private:
    ConstString    mName;
    int            mVolume;
};

extern Volume    cNoVolume;

class Scenario {
public:
    Scenario(const ConstString & name,
             bool enabled,
             EScenarioPriority priority,
             Volume & volume,
             Volume & micGain = cNoVolume);
    virtual                ~Scenario() {}

    bool                setFilter(int filter);

    const char *        getName() const   { return mName.c_str(); }

    ConstString            mName;
    bool                mEnabled;
    bool                mHardwired;        ///< Hardwired scenario are the ones enabled at creation
    EScenarioPriority    mPriority;
    int                    mFilter;
    int                    mLatency;

    int    getVolume() const
                                                { return mVolume.get(); }
    int    getVolumeTics() const
                          { return SCVolumeConvertPercentToTic(getVolume()); }
    void   setVolume(int volume)             { mVolume.set(volume); }

    int    getMicGain() const
                   { return hasMicGain() ? mMicGain.get() : -1; }
    int   getMicGainTics() const
                   { return hasMicGain() &&
                            mMicGain.get() >= 0 ?
                             SCVolumeConvertPercentToTic(mMicGain.get()) : -1; }
    void  setMicGain(int gain)   { if (hasMicGain()) mMicGain.set(gain); }

    int   getVolumeOrMicGain(bool volumeNotMicGain) const
                    { return volumeNotMicGain ? mVolume.get() : mMicGain.get(); }
    void  setVolumeOrMicGain(int volume, bool volumeNotMicGain)
             { volumeNotMicGain ? mVolume.set(volume) : mMicGain.set(volume); }
    bool  hasMicGain() const
                                           { return &mMicGain != &cNoVolume; }

    void                configureRoute (EVirtualSink sink,
                                        EPhysicalSink destination,
                                        bool ringerSwitchOn,
                                        bool enabled = true);
    void                configureRoute (EVirtualSource source,
                                        EPhysicalSource destination,
                                        bool ringerSwitchOn,
                                        bool enabled = true);
    bool                isRouted(EVirtualSink sink);
    bool                isRouted(EVirtualSource source);
    EPhysicalSink        getDestination(EVirtualSink sink);
    EPhysicalSource     getDestination(EVirtualSource source);

    void                logRoutes() const;

private:
    ScenarioRoute *        getScenarioRoute(EVirtualSink sink, bool ringerOn);
    ScenarioRoute *     getScenarioRoute(EVirtualSource source, bool ringerOn);

    ScenarioRoute        mRoutesRingerOn[eVirtualSink_Count];
    ScenarioRoute        mRoutesRingerOff[eVirtualSink_Count];
    ScenarioRoute       mRoutesSource[eVirtualSource_Count];
    Volume &            mVolume;
    Volume &            mMicGain;
};

class ScenarioModule {
public:
    typedef std::map< std::string, Scenario * > ScenarioMap;

    ScenarioModule(const ConstString & category);

    virtual            ~ScenarioModule() {}

    bool            makeCurrent();
    bool            isCurrentModule()
                           { return sCurrentModule && sCurrentModule == this; }
    static ScenarioModule * getCurrent()
                            { return sCurrentModule; }

    const char *    getCategory() const
                             { return mCategory.c_str() + 1; }
    const char *    getCurrentScenarioName() const
            { return mCurrentScenario ? mCurrentScenario->getName() : "none"; }

    const ScenarioModule::ScenarioMap &    getScenarios() const
                                                   { return mScenarioTable; }
    bool            addScenario(Scenario *s);

    Scenario *        getScenario (const char * name);

    bool            enableScenario (const char * name);
    bool            disableScenario (const char * name);

    bool            setCurrentScenario (const char * name);
    bool            unsetCurrentScenario (const char * name);

    /// Get or set a scenario's volume
    bool            setScenarioVolume (const char * name, int volume)
                     { return setScenarioVolumeOrMicGain(name, volume, true); }
    bool            getScenarioVolume (const char * name, int & volume)
                     { return getScenarioVolumeOrMicGain(name, volume, true); }

    /// Get or set a scenario's mic gain
    bool            setScenarioMicGain (const char * name, int micGain)
                   { return setScenarioVolumeOrMicGain(name, micGain, false); }
    bool            getScenarioMicGain (const char * name, int & micGain)
                   { return getScenarioVolumeOrMicGain(name, micGain, false); }

    /// Get or set a scenario's volume or mic gain
    bool            setScenarioVolumeOrMicGain (const char * name,
                                                int volume,
                                                bool volumeNotMicGain);
    bool            getScenarioVolumeOrMicGain (const char * name,
                                                int & volume,
                                                bool volumeNotMicGain);

    bool            setScenarioLatency (const char * name, int latency);
    bool            getScenarioLatency (const char * name, int & latency);

    /// Module store preferences for all scenario in
    // one large message to minimize number of transactions

    ///    Save preferences in 10 seconds max
    void            scheduleStorePreferences();
    /// Store preferences now
    void            storePreferences();
    /// Restore preferences now
    void            restorePreferences();

    bool            setCurrentScenarioByPriority();

    bool            registerMe (LSMethod *methods);
    bool            broadcastEvent (const char *event);

    bool             sendChangedUpdate (int changedFlags,
                                        const gchar * broadCastEvent = 0);
    bool            sendRequestedUpdate (LSHandle *sh,
                                         LSMessage *message, bool subscribed);
    bool            sendEnabledUpdate (const char *scenario,
                                       int enabledFlags);

    bool            setVolumeOverride (bool override);
    int                getVolumeOverride ()                { return mVolumeOverride; }

    virtual bool    setMuted (bool muted);
    bool            isMuted ()                            { return mMuted; }

    void changeMuted(bool muted)            { mMuted = muted; }
    virtual bool    museSet (bool enabled);

    /// Adjust volume of an alert depending as necessary in the current scenario
    virtual int        adjustAlertVolume(int volume,
                                         bool alertStarting = false)
                                                 { return volume; }

    /// Program volume of sinks that are
    //never muted. Return false if not performed.
    virtual int        adjustSystemVolume(int volume);

    void            programHardwareState ();
    void            programSoftwareMixer (bool ramp, bool muteMediaSink = false);

    bool            programVolume (EVirtualSink sink, int volume, bool ramp = false);
    bool            programMute (EVirtualSource source, int mute);
    bool            rampVolume (EVirtualSink sink, int volume)
                                { return programVolume (sink, volume, true); }

    virtual bool        setVolumeUpDown(bool flag)
    {
          return true;
    }

    // When active module changes, these are called in this order
    // deactivated module is notified first
    virtual void    onDeactivated()                    { }
    // module to activate is notified while the "old" routing is still active
    virtual void    onActivating()                        { }

    virtual void    onSinkChanged(EVirtualSink sink, EControlEvent event)    { }

    ///< Called on setScenario. Program all controlled volumes at their expected levels.
    virtual void    programControlVolume()                { }
    ///< Program volumes controlled by FW rather than pulse. (Phone in particular).
    virtual void    programHardwareVolume()                { }
    virtual void    programState()                        { }
    virtual void    programMuted()                        { }
    virtual void    programMuse(bool enable)            { }
    virtual void    programHac(bool enable)             { }

    ConstString        mCategory;

    virtual void setA2DPAddress(std::string address) { }

    bool subscriptionPost(LSHandle * palmService,
               std::string replyString,ESendUpdate update);

protected:
    ScenarioMap        mScenarioTable;
    Scenario *        mCurrentScenario;
    guint            mStoreTimerID;

    bool            mMuted;
    int                mVolumeOverride;

    void            _setCurrentScenarioByPriority();
    void            _updateHardwareSettings(bool muteMediaSink = false);

    static ScenarioModule *        sCurrentModule;
};

#endif // _SCENARIO_H_
