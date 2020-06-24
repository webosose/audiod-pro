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

#ifndef _SCENARIO_H_
#define _SCENARIO_H_

#include "AudioMixer.h"
#include "ConstString.h"
#include "volume.h"
#include "genericScenarioModule.h"

#include <map>

#define VOLUME_STORE_DELAY 10000

struct ScenarioRoute {
    ScenarioRoute() : mDestination(eMainSink), mRouted(false) {}

    int mDestination;
    bool mRouted;
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
/*class Volume {
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

extern Volume    cNoVolume;*/

class Scenario : public GenericScenario {
public:
    //Scenario(const Scenario&);
    Scenario(const ConstString & name,
             bool enabled,
             EScenarioPriority priority,
             Volume & volume,
             Volume & micGain = cNoVolume);
    virtual ~Scenario() {}

    int mFilter;
    int mLatency;

    int getVolumeTics() const
        {return SCVolumeConvertPercentToTic(getVolume());}
    int getMicGainTics() const
        {return hasMicGain() && mMicGain.get() >= 0 ?
    SCVolumeConvertPercentToTic(mMicGain.get()) : -1;}

    void configureRoute (EVirtualSink sink,
                         EPhysicalSink destination,
                         bool ringerSwitchOn,
                         bool enabled = true);
    void configureRoute (EVirtualSource source,
                         EPhysicalSource destination,
                         bool ringerSwitchOn,
                         bool enabled = true);
    bool setFilter(int filter);
    bool isRouted(EVirtualSink sink);
    bool isRouted(EVirtualSource source);
    EPhysicalSink getDestination(EVirtualSink sink);
    EPhysicalSource getDestination(EVirtualSource source);

    void logRoutes() const;

private:
    ScenarioRoute * getScenarioRoute(EVirtualSink sink, bool ringerOn);
    ScenarioRoute * getScenarioRoute(EVirtualSource source, bool ringerOn);

    ScenarioRoute mRoutesRingerOn[eVirtualSink_Count];
    ScenarioRoute mRoutesRingerOff[eVirtualSink_Count];
    ScenarioRoute mRoutesSource[eVirtualSource_Count];

};


class ScenarioModule : public GenericScenarioModule {
public:
    
    bool museSet (bool enabled);
    void programHardwareState ();

    ScenarioModule(const ConstString & category) : GenericScenarioModule(category){}

    

    bool            makeCurrent();
    

    

    bool            setScenarioLatency (const char * name, int latency);
    bool            getScenarioLatency (const char * name, int & latency);

    void            programSoftwareMixer (bool ramp, bool muteMediaSink = false);

    bool            programVolume (EVirtualSink sink, int volume, bool ramp = false);
    bool            programMute (EVirtualSource source, int mute);
    bool            rampVolume (EVirtualSink sink, int volume)
                                { return programVolume (sink, volume, true); }

    


    

protected:
    
    void            _updateHardwareSettings(bool muteMediaSink = false);

};

#endif // _SCENARIO_H_
