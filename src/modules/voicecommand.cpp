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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>

#include "voicecommand.h"
#include "AudioDevice.h"
#include "module.h"
#include "state.h"
#include "phone.h"
#include "media.h"
#include "vvm.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "main.h"
#include "vvm.h"

#if AUTOBUILD_FLAG
#define BLUETOOTH_VC_FORCE 0    // never use for official builds!
#else
#define BLUETOOTH_VC_FORCE 0
#endif

static VoiceCommandScenarioModule * sVoiceCommandModule = 0;

VoiceCommandScenarioModule * getVoiceCommandModule()
{
    return sVoiceCommandModule;
}

void
VoiceCommandScenarioModule::programControlVolume ()
{
    Scenario *scenario = nullptr;
    if (VERIFY(mCurrentScenario) && (scenario = 
        dynamic_cast <Scenario*>(mCurrentScenario)) && !mMuted)
    {
        programVolume (evoicedial, mCurrentScenario->getVolume());
        gAudioDevice.setMicGain(mCurrentScenario->mName,
                                scenario->getMicGainTics());
    }
}

void
VoiceCommandScenarioModule::programVoiceCommandVolume(bool ramp)
{
    int volume = mMuted ? 0 : mCurrentScenario->getVolume();
    programVolume (evoicedial, volume, ramp);
}

void
VoiceCommandScenarioModule::programState ()
{
    Scenario *scenario = nullptr;
    if (VERIFY(mCurrentScenario) && (!(scenario = 
        dynamic_cast <Scenario*>(mCurrentScenario))))
    {
    gAudioDevice.setMicGain(mCurrentScenario->mName,
                            scenario->getMicGainTics());
    }
}

void
VoiceCommandScenarioModule::onActivating()
{
    getMediaModule()->rampDownAndMute();
}

#if BLUETOOTH_VC_FORCE
static bool _btVCEnableCallback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_ANY);
    bool    success;
    if (!msg.parse(__FUNCTION__) || !msg.get("returnValue", success) || !success)
    {
        g_warning("Voice Command BT enable failure...");
        getVoiceCommandModule()->stopVoiceCommand("bluetooth");
    }
    return true;
}
#endif

static bool _voiceCommandCallback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_ANY);
    bool    success;
    if (!msg.parse(__FUNCTION__) || !msg.get("returnValue", success) || !success)
    {
        FILE* f = fopen("/usr/sbin/vctest", "rb");
        if (f) {
            fclose(f);
            g_warning("VoiceCommand deamon start/notification failure. NOT disabling voice command for testing...");
        } else {
            g_warning("VoiceCommand deamon start/notification failure. Deactivating Voice Command.");
            getVoiceCommandModule()->endVoiceCommand("audiod");
        }
    }
    return true;
}

bool
VoiceCommandScenarioModule::startVoiceCommand(const char * source, const char ** errorMessage)
{
    const char * localMessagePtr = 0;
    const char * & message = errorMessage ? *errorMessage : localMessagePtr;

    // message is always defined, we can use it safely.
    //It will be passed to the caller if necessary automatically.
    message = 0;
    bool wasActive = this->isCurrentModule();

    if (getVvmModule()->isCurrentModule()) {
        g_warning("stopping vvm cause voice command is starting");
        getVvmModule()->stopVvm("voice command");
    }

    if (wasActive)
        message = "voice command already active";
    else if (!isEnabled())
        message = "voice command is not enabled";
    else if (getPhoneModule()->isCurrentModule())
        message = "phone call active";
    else if (gState.getPhoneLocked())
    {
        std::string    voiceCommandWhenSecureLocked;
        if (gState.getPhoneSecureLockActive() &&
           (!gState.getPreference(cPref_VoiceCommandSupportWhenSecureLocked,
                                  voiceCommandWhenSecureLocked)
                || voiceCommandWhenSecureLocked == "off")) {
            message = "phone locked securely and VoiceCommandSupportWhenSecureLocked is false";
        } else {
            if (strcmp(source, "bluetooth")!=0) {
                CLSError lserror;
                LSHandle * sh = GetPalmService();
                if (!LSCallOneReply(sh, "palm://com.palm.display/control/setState",
                       "{\"state\":\"unlock\"}", NULL, NULL, NULL, &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }
        }
    }

    if (message) {
        g_debug("VoiceCommand::startVoiceCommand(%s): denied: %s", source, message);
        gAudioMixer.playSystemSound("deny", eeffects);
        // play chirp
    } else
    {
        g_message("VoiceCommand::startVoiceCommand(%s): making voice command active", source);

        // Start/notify the Voice Command deamon
        LSHandle * sh = GetPalmService();
        std::string    command = string_printf("{\"source\":\"%s\"}", source);
        CLSError lserror;
        if (!LSCall(sh, "luna://com.palm.pmvoicecommand/startVoiceCommand",
                                               command.c_str(),
                                               _voiceCommandCallback,
                                                NULL, NULL, &lserror))
            lserror.Print(__FUNCTION__, __LINE__);

#if BLUETOOTH_VC_FORCE
        if (ConstString(source) != "bluetooth" &&
             ConstString(getCurrentScenarioName()).hasSuffix(SCENARIO_BLUETOOTH_SCO))
        {
            CLSError lserror;
            if (!LSCall(sh, "luna://com.palm.bluetooth/hfg/scoaudio",
                  "{\"sco\":\"open\"}", _btVCEnableCallback,
                   NULL, NULL, &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        }
#endif

        makeCurrent();
        gState.pauseAllMediaSaved();

        message = "success";
    }

    return !wasActive && this->isCurrentModule();
}

void
VoiceCommandScenarioModule::stopVoiceCommand(const char * source)
{
    g_message("VoiceCommandScenarioModule::stopVoiceCommand(%s)", source);
    CLSError lserror;
    LSHandle * sh = GetPalmService();
    if (!LSCallOneReply(sh, "palm://com.palm.pmvoicecommand/stopVoiceCommand",
                                            "{}", NULL, NULL, NULL, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
}

void
VoiceCommandScenarioModule::endVoiceCommand(const char * source)
{
    g_message("VoiceCommandScenarioModule::endVoiceCommand(%s)", source);

    if (this->isCurrentModule())
    {
        ScenarioModule *phone = getPhoneModule();
        ScenarioModule *media = getMediaModule();
        if (gState.getOnActiveCall())
            phone->makeCurrent();
        else
        {
            media->makeCurrent();

            // since the call ended reset the scenario to
            // the one that should be selected by priority
            phone->setCurrentScenarioByPriority();
            gState.resumeAllMediaSaved();
        }
    }
}

#if defined(AUDIOD_PALM_LEGACY)
static bool
_voiceCommandControl(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message,
                            SCHEMA_2(REQUIRED(active, boolean),
                             OPTIONAL(source, string)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    std::string failureMessage;
    std::string    source;

    bool active;
    if (!msg.get("active", active))
    {
        failureMessage = MISSING_PARAMETER_ERROR(active, boolean);
        goto send;
    }

    if (!msg.get("source", source))
        source = "unknown";

    if (active)
    {
        getVvmModule()->stopVvm("voice command");
        const char * errorMessage = 0;
        if (!getVoiceCommandModule()->isCurrentModule() &&
            !getVoiceCommandModule()->startVoiceCommand(source.c_str(),
             &errorMessage))
            failureMessage = createJsonReplyString(false, 3, errorMessage);
    }
    else
        getVoiceCommandModule()->stopVoiceCommand(source.c_str());

send:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, failureMessage.empty() ?
                     STANDARD_JSON_SUCCESS : failureMessage.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static LSMethod voiceCommandMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_SetMicGain, _setMicGain},
    { cModuleMethod_GetMicGain, _getMicGain},
    { cModuleMethod_OffsetMicGain, _offsetMicGain},
    { cModuleMethod_EnableScenario, _enableScenario},
    { cModuleMethod_DisableScenario, _disableScenario},
    { cModuleMethod_SetCurrentScenario, _setCurrentScenario},
    { cModuleMethod_GetCurrentScenario, _getCurrentScenario},
    { cModuleMethod_ListScenarios, _listScenarios},
    { cModuleMethod_Status, _status},
    { cModuleMethod_BroadcastEvent, _broadcastEvent},
    { cModuleMethod_Control, _voiceCommandControl}, // this is voice command
                                                    //specific control
    { },
};
#endif

VoiceCommandScenarioModule::VoiceCommandScenarioModule() :
    ScenarioModule(ConstString("/voiceCommand")),
    mVoiceCommandEnabled(false),
#if defined(MACHINE_MANTARAY)
    mFrontSpeakerVolume(cVoiceCommand_BackSpeaker, 70),
    mHeadsetVolume(cVoiceCommand_Headset, 70),
#else
    mFrontSpeakerVolume(cVoiceCommand_BackSpeaker, 90),
    mHeadsetVolume(cVoiceCommand_Headset, 50),
#endif
    mBluetoothVolume(cVoiceCommand_BluetoothSCO, 50),
    mFrontMicGain(cVoiceCommand_Mic_Front,
                     gAudioDevice.getDefaultMicGain(cVoiceCommand_BackSpeaker)),
    mHeadsetMicGain(cVoiceCommand_Mic_HeadsetMic,
                      gAudioDevice.getDefaultMicGain(cVoiceCommand_HeadsetMic)),
    mBluetoothSCOMicGain(cVoiceCommand_Mic_BluetoothSCO,
                     gAudioDevice.getDefaultMicGain(cVoiceCommand_BluetoothSCO))
{
    Scenario *s = new Scenario (cVoiceCommand_BackSpeaker,
                                true,
                                eScenarioPriority_VoiceCommand_BackSpeaker,
                                mFrontSpeakerVolume,
                                mFrontMicGain);

    //                 class              destination    switch     enabled
    s->configureRoute (ealerts,           eMainSink,     true,      true);
    s->configureRoute (enotifications,  eMainSink,    true,      false);
    s->configureRoute (eringtones,      eMainSink,    true,    true);
    s->configureRoute (enavigation,     eMainSink,     true,      true);
    s->configureRoute (enavigation,     eMainSink,     false,     true);
    s->configureRoute (evoicedial,        eMainSink,     true,      true);
    s->configureRoute (evoicedial,        eMainSink,     false,     true);
    s->configureRoute (ecalendar,         eMainSink,     true,      false);
    s->configureRoute (ealarm,            eMainSink,     true,      true);
    s->configureRoute (ealarm,            eMainSink,     false,     true);
    s->configureRoute (erecord,            eMainSource,   false,  true);
    s->configureRoute (erecord,            eMainSource,   true,  true);
    s->configureRoute (evoicedialsource,   eMainSource,   false,  true);
    s->configureRoute (evoicedialsource,   eMainSource,   true,  true);

    addScenario (s);


#if defined(AUDIOD_PALM_LEGACY)
    registerMe (voiceCommandMethods);
#endif

    restorePreferences();
}

int
VoiceCommandInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sVoiceCommandModule = new VoiceCommandScenarioModule();

    return 0;
}

MODULE_START_FUNC (VoiceCommandInterfaceInit);
