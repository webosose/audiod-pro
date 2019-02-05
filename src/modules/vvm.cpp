// Copyright (c) 2012-2019 LG Electronics, Inc.
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

#include "vvm.h"
#include "AudioDevice.h"
#include "module.h"
#include "state.h"
#include "phone.h"
#include "media.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "main.h"

static VvmScenarioModule * sVvmModule = 0;

VvmScenarioModule * getVvmModule()
{
    return sVvmModule;
}

void
VvmScenarioModule::programControlVolume ()
{
        programVolume (evvm, mCurrentScenario->getVolume());
}

void
VvmScenarioModule::programVvmVolume(bool ramp)
{
    int volume = mCurrentScenario->getVolume();
    programVolume (evvm, volume, ramp);
}

void
VvmScenarioModule::programState ()
{
    if (ConstString(getCurrentScenarioName()).hasSuffix(SCENARIO_BLUETOOTH_SCO)) {
        if (!mScoUp) {
            LSHandle * sh = GetPalmService();

            CLSError lserror;
            if (!LSCall(sh, "luna://com.palm.bluetooth/hfg/scoaudio", "{\"sco\":\"open\"}",
                                                  NULL, NULL, NULL, &lserror))
                                         lserror.Print(__FUNCTION__, __LINE__);
                mScoUp = true;
        }
    } else if (mScoUp) {
        LSHandle * sh = GetPalmService();
        CLSError lserror;
        if (!LSCall(sh, "luna://com.palm.bluetooth/hfg/scoaudio",
                        "{\"sco\":\"close\"}", NULL, NULL, NULL, &lserror))
                         lserror.Print(__FUNCTION__, __LINE__);
             mScoUp = false;
    }

    int volume = mCurrentScenario->getVolume();
    programVolume (evvm, volume, false);

}

void
VvmScenarioModule::onActivating()
{
    getMediaModule()->rampDownAndMute();
}

bool
VvmScenarioModule::startVvm(const char * source, const char ** errorMessage)
{
    const char * localMessagePtr = 0;
    const char * & message = errorMessage ? *errorMessage : localMessagePtr;

    // message is always defined, we can use it safely.
    // It will be passed to the caller if necessary automatically.
    message = 0;
    bool wasActive = this->isCurrentModule();

    if (wasActive)
        message = "vvm already active";
    else if (!isEnabled())
        message = "vvm is not enabled";
    else if (getPhoneModule()->isCurrentModule())
        message = "phone call active";
    else if (gState.getPhoneLocked())
    {
        if (strcmp(source, "bluetooth")!=0) {
            CLSError lserror;
            LSHandle * sh = GetPalmService();
            if (!LSCallOneReply(sh, "palm://com.palm.display/control/setState",
                 "{\"state\":\"unlock\"}", NULL, NULL, NULL, &lserror))
                  lserror.Print(__FUNCTION__, __LINE__);
        }
    }

    if (message)
        g_debug("VvmScenarioModule::startVvm(%s): denied: %s", source, message);
    else
    {
        g_message("VvmScenarioModule::startVvm(%s): making vvm active", source);

        makeCurrent();
        this->setActive(true);
        gState.pauseAllMediaSaved();

        message = "success";
    }

    return !wasActive && this->isCurrentModule();
}

void
VvmScenarioModule::stopVvm(const char * source)
{
    g_message("VvmScenarioModule::stopVvm(%s)", source);

    if (mScoUp) {
        LSHandle * sh = GetPalmService();

        CLSError lserror;
        if (!LSCall(sh, "luna://com.palm.bluetooth/hfg/scoaudio",
                            "{\"sco\":\"close\"}", NULL, NULL, NULL, &lserror))
                            lserror.Print(__FUNCTION__, __LINE__);

        mScoUp = false;
    }

    if (this->isCurrentModule())
    {
        ScenarioModule *phone = getPhoneModule();
        ScenarioModule *media = getMediaModule();

        this->setActive(false);

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

void
VvmScenarioModule::endVvm(const char * source)
{
    g_message("VvmScenarioModule::endVvm(%s)", source);

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
_VvmControl(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_2(REQUIRED(active, boolean),
                                                   OPTIONAL(source, string)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    std::string failureMessage;
    std::string    source;

    VvmScenarioModule *vvm = getVvmModule();

    bool active;
    if (!msg.get("active", active))
    {
        failureMessage = MISSING_PARAMETER_ERROR(active, boolean);
        goto send;
    }

    if (!msg.get("source", source))
        source = "unknown";

    if (active && !vvm->isActive())
    {
        const char * errorMessage = 0;
        if (!vvm->isCurrentModule() && !vvm->startVvm(source.c_str(), &errorMessage))
            failureMessage = createJsonReplyString(false, 3, errorMessage);
    }
    else if (!active && vvm->isActive())
        vvm->stopVvm(source.c_str());

send:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, failureMessage.empty() ?
                       STANDARD_JSON_SUCCESS : failureMessage.c_str(), &lserror))
                       lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static LSMethod VvmMethods[] = {
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_OffsetVolume, _offsetVolume},
    { cModuleMethod_EnableScenario, _enableScenario},
    { cModuleMethod_DisableScenario, _disableScenario},
    { cModuleMethod_SetCurrentScenario, _setCurrentScenario},
    { cModuleMethod_GetCurrentScenario, _getCurrentScenario},
    { cModuleMethod_ListScenarios, _listScenarios},
    { cModuleMethod_Status, _status},
    { cModuleMethod_BroadcastEvent, _broadcastEvent},
    { cModuleMethod_Control, _VvmControl}, // this is vvm specific control
    { },
};
#endif

VvmScenarioModule::VvmScenarioModule() :
    ScenarioModule(ConstString("/vvm")),
    mVvmActive (false),
    mVvmEnabled(true),
    mScoUp(false),
    mFrontSpeakerVolume(cVvm_BackSpeaker, 90),
    mHeadsetVolume(cVvm_Headset, 50),
    mBluetoothVolume(cVvm_BluetoothSCO, 50),
    mFrontMicGain(  cVvm_Mic_Front,
                    gAudioDevice.getDefaultMicGain(cVvm_BackSpeaker)),
    mHeadsetMicGain(cVvm_Mic_HeadsetMic,
                    gAudioDevice.getDefaultMicGain(cVvm_HeadsetMic)),
    mBluetoothSCOMicGain(cVvm_Mic_BluetoothSCO,
                         gAudioDevice.getDefaultMicGain(cVvm_BluetoothSCO))
{
    Scenario *s = new Scenario (cVvm_BackSpeaker,
                                true,
                                eScenarioPriority_Vvm_BackSpeaker,
                                mFrontSpeakerVolume, mFrontMicGain);

    //                 class              destination    switch     enabled
    s->configureRoute (ealerts,           eMainSink,     true,      true);
    s->configureRoute (enotifications,  eMainSink,    true,      false);
    s->configureRoute (eringtones,      eMainSink,    true,    true);
    s->configureRoute (enavigation,     eMainSink,     true,      true);
    s->configureRoute (enavigation,     eMainSink,     false,     true);
    s->configureRoute (evvm,        eMainSink,     true,      true);
    s->configureRoute (evvm,        eMainSink,     false,     true);
    s->configureRoute (ecalendar,         eMainSink,     true,      false);
    s->configureRoute (ealarm,            eMainSink,     true,      true);
    s->configureRoute (ealarm,            eMainSink,     false,     true);

    addScenario (s);

#if VVM_SUPPORTED
#if defined(AUDIOD_PALM_LEGACY)
    registerMe (VvmMethods);
#endif
#endif

    restorePreferences();
}

int
VvmInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    sVvmModule = new VvmScenarioModule();

    return 0;
}

MODULE_START_FUNC (VvmInterfaceInit);
