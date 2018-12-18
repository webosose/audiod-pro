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

#ifndef _STATE_H_
#define _STATE_H_
#include "umiaudiomixer.h"
#include "scenario.h"
#include "IPC_SharedAudiodDefinitions.h"

enum ESliderState
{
    eSlider_Closed = 0,
    eSlider_Sliding,
    eSlider_Open
};

enum ECallMode
{
    eCallMode_None = 0,
    eCallMode_Carrier,
    eCallMode_Voip
};

enum ECallStatus
{
    eCallStatus_NoCall = 0,
    eCallStatus_Connecting,
    eCallStatus_Incoming,
    eCallStatus_Dialing,
    eCallStatus_OnHold,
    eCallStatus_Active,
    eCallStatus_Disconnected,
};

enum ETTYMode
{
    eTTYMode_Off = 0,
    eTTYMode_Full,
    eTTYMode_HCO,
    eTTYMode_VCO
};


template <class T> struct PreferencePair
{
    void    init(const T & value)
    {
        mValue = mDefaultValue = value;
    }
    T    mValue;
    T    mDefaultValue;
};

typedef std::map<std::string, PreferencePair<bool> > TBooleanPreferences;
typedef std::map<std::string, PreferencePair<std::string> > TStringPreferences;
typedef std::map<std::string, PreferencePair<int> > TintPreferences;

// Preference name definitions to avoid typos in the code
extern const char * cPref_VibrateWhenRingerOn;
extern const char * cPref_VibrateWhenRingerOff;
extern const char * cPref_VoiceCommandSupportWhenSecureLocked;
extern const char * cPref_TextEntryCorrectionHapticPolicy;
extern const char * cPref_BeatsOnForHeadphones;
extern const char * cPref_BeatsOnForSpeakers;
extern const char * cPref_RingerOn;
//Used to store current sound profile to support DND
extern const char * cPref_PrevVibrateWhenRingerOn;
extern const char * cPref_PrevVibrateWhenRingerOff;
extern const char * cPref_PrevRingerOn;
extern const char * cPref_DndOn;
extern const char * cPref_TouchOn;
extern const char * cPref_OverrideRingerForAlaram;
extern const char * cPref_OverrideRingerForTimer;
extern const char * cPref_VolumeBalanceOnHeadphones;
extern const char * cPref_RingtoneWithVibration;


class State
{
public:
    State();

    /// Get the module that controls the volume, never 0
    GenericScenarioModule * getCurrentVolumeModule ();
    /// Get the module, if any, that has priority
    // access to the volume control by explicit request. MAYBE 0!
    /// Control may have been acquired using State::
    //setLockedVolumeModule or ScenarioModule::setVolumeOverride
    GenericScenarioModule * getExplicitVolumeControlModule ();
    bool setLockedVolumeModule (ScenarioModule *module);

    bool getOnActiveCall ();
    void setOnActiveCall (bool state, ECallMode mode);

    // called by audio mixer when an input stream has been opened
    void setActiveInputStream (bool active);
    // called by property system
    void setMediaServerIsRecording (const bool & recording);
    // called to artificially force the mic to be on
    void setForceMicOn (bool force);
    bool getForceMicOn ()          { return mForceMicOn; }
    bool isRecording (); // true when an input stream is open OR
                         //when mediaserver says it's recording

    EPhoneStatus getPhoneStatus ();
    void setPhoneStatus (EPhoneStatus state);

    bool getDisplayOn ();
    void setDisplayOn (bool state);

    bool getRingerOn ();
    void setRingerOn (bool state);

    bool getTouchSound ();
    void setTouchSound (bool state);
    void setDndMode (bool state);
    void storeSoundProfile ();
    void retrieveSoundProfile ();

    EHeadsetState getHeadsetState ();
    void setHeadsetState (EHeadsetState state);
    void setHeadsetRoute (EHeadsetState state);
    bool setMicOrHeadset (EHeadsetState state, int cardno, int deviceno, int status);

    ETTYMode getTTYMode ();
    void setTTYMode (ETTYMode mode);
    void hacSet (bool enable);
    bool hacGet () { return mHAC; }

    ECallMode getCallMode ();
    void setCallMode (ECallMode mode, ECallStatus status);

    void setCallWithVideo (bool callWithVideo);
    bool getCallWithVideo();

    ESliderState getSliderState ();
    void setSliderState (ESliderState state);

    bool getPreference(const char * name, bool & outValue);
    bool setPreference(const char * name, bool value);

    bool getPreference(const char * name, std::string & outValue);
    bool setPreference(const char * name, const std::string & value);

    bool getPreference(const char * name, int & outValue );
    bool setPreference(const char * name, int value);

    // getPreference/setPreference implementation
    static bool setPreferenceRequest(LSHandle *lshandle,
                                     LSMessage *message,
                                     void *ctx);
    static bool getPreferenceRequest(LSHandle *lshandle,
                                     LSMessage *message,
                                     void *ctx);

    /// Store/restore all preferences
    bool storePreferences();
    bool restorePreferences();

    // combines vibrate switch state & vibrate preferences
    bool shouldVibrate();

    bool getOnThePuck ();
    void setOnThePuck (bool state);

    void setPhoneLocked (bool state);
    bool getPhoneLocked ();

    void setPhoneSecureLockActive (bool state)        { mPhoneSecureLockActive = state; }
    bool getPhoneSecureLockActive ()      { return mPhoneSecureLockActive; }

    bool getUseUdevForHeadsetEvents ()    { return mUseUdevForHeadsetEvents; }
    void setUseUdevForHeadsetEvents (bool state)    { mUseUdevForHeadsetEvents = state; }

    void setIncomingCallActive (bool state, ECallMode mode);
    bool getIncomingCallActive ();

    bool getBTServerRunning ()                 { return mBTServerRunning; }
    void setBTServerRunning (bool state);

    bool getAdjustMicGain ()                   { return mAdjustMicGain; }
    void setAdjustMicGain (bool state)         { mAdjustMicGain = state; }

    //! Have we *really* paused some media for a phone call?
    void setActiveSinksAtPhoneCallStart (VirtualSinkSet sinks);
    VirtualSinkSet getActiveSinksAtPhoneCallStart ();

    // At some critical routing changes,
    // we want to pause all media (disconnect headset for instance).
    void pauseAllMedia();

    void pauseAllMediaSaved();

    void resumeAllMediaSaved();

    void init ();

    // when turning mic off, we delay before effectively doing so, to avoid
    // burst during fast oscillations of state
    void updateIsRecording();

    bool getScoUp ();
    void setScoUp (bool state);

    bool getTabletConnected ();
    void setTabletConnected (bool state);
    void setHfpStatus(bool state);
    bool getHfpStatus();
    void setQvoiceStatus(bool state);
    bool getQvoiceStatus();
    void setRecordStatus(bool state);
    bool getRecordStatus();
    void btOpenSCO(LSHandle *lshandle);
    void btCloseSCO(LSHandle *lshandle);
    void setA2DPStatus(bool Status);
    bool getA2DPStatus();
    void setHFPConnectionStatus(std::string Status);
    std::string getHFPConnectionStatus();
    void setQvoiceStartedStatus(bool status);
    bool isQvoiceStarted();
    void rtpSubscriptionReply(int info, char *ip, int port);
    bool isRTPLoaded(){ return mRTPLoaded;}
    void setRTPLoaded(bool flag){mRTPLoaded = flag;}

    int getSoundBalance();
    bool setSoundBalance (int balance);
    bool sendUpdatedProfile(LSHandle * sh, LSMessage * message);
    bool respondProfileRequest(LSHandle * sh, LSMessage * message, bool subscribed);
    bool getLoopbackStatus();
    void setLoopbackStatus(bool status);
    bool checkPhoneScenario(ScenarioModule *phone) ;

    /*Initializing umi mixer*/
    void umiMixerInit(GMainLoop *loop, LSHandle *handle);
    umiaudiomixer* mObjUmiMixerInstance;

private:
    bool                mOnActiveCall;
    bool                mVoip;
    int                 mNumVoip;
    bool                mCarrier;
    ECallMode            mCallMode;
    bool                mCallWithVideo;
    bool                mPauseAllMediaSaved;
    bool                mOnThePuck;
    ESliderState        mSliderState;
    ETTYMode            mTTYMode;
    bool                mHAC;
    ScenarioModule *    mLockedVolumeModule;
    bool                mUseUdevForHeadsetEvents;
    bool                mIncomingCallActive;
    bool                mIncomingCarrierCallActive;
    bool                mIncomingVoipCallActive;
    bool                mInputStreamActive;
    bool                mMediaserverIsRecording;
    bool                mForceMicOn;
    VirtualSinkSet        mActiveSinksAtPhoneCallStart;
    bool                mBTServerRunning;
    bool                mAdjustMicGain;
    bool                mPhoneSecureLockActive;
    TBooleanPreferences mBooleanPreferences;
    TStringPreferences    mStringPreferences;
    bool                mScoUp;
    bool                mTabletConnected;
    bool mBTHfpConnected;
    int mBallance;
    TintPreferences      mIntegerPreferences;
    bool mQvoiceOpened;
    bool mRecordOpened;
    bool mLoopback;

    bool                 mA2DPStatus;
    std::string          mHfpStatus;
    bool                 mQvoiceStarted;
    bool                 mRTPLoaded;
};

extern State gState;

class GlobalConf {
public:
    GlobalConf();
    int getCarrierBusyToneRepeats(){ return m_carrierbusyToneRepeats; }
    int getCarrierEmergencyToneRepeats(){ return m_carrierEmergencyToneRepeats; }
protected:
    int m_carrierbusyToneRepeats;
    int m_carrierEmergencyToneRepeats;
};

extern GlobalConf gGlobalConf;
extern bool callbackReceived;


#endif // _STATE_H_
