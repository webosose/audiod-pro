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

#include "AudioDevice.h"
#include "AudioMixer.h"
#include "state.h"
#include "utils.h"
#include "messageUtils.h"
#include <errno.h>
#include <audiodTracer.h>
#include "main.h"

//to simulate ringtone change the macro to 1
#define TEST_RINGTONE_VIA_AUDIOD 0

static void _standardSuccessResponseCB(void* parameter) {
    Callback* callback = (Callback*)parameter;
    LSMessage* message = (LSMessage*)callback->parameter;
    CLSError lserror;
    if (!LSMessageReply(LSMessageGetConnection(message), message,
                                            STANDARD_JSON_SUCCESS, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    LSMessageUnref(message);
    delete callback;
}

static bool
_playAlert(LSHandle *lshandle, LSMessage *message, void *ctx)
{
   LSMessageJsonParser    msg(message, SCHEMA_1(OPTIONAL(index, integer)));
   PMTRACE_FUNCTION;
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar * answer = STANDARD_JSON_SUCCESS;
    char *filename = NULL;
    FILE *fp = NULL;
    int sound;
    if (!msg.get("index", sound))
        sound = 0;
    if (sound < 0 || sound > 63) {
        answer = INVALID_PARAMETER_ERROR(index, integer);
        goto Error;
    }
    filename = (char *)malloc( strlen(SYSTEMSOUNDS_PATH)+
                                            strlen ("alert_")+
                                            sizeof (sound) +
                                            strlen("-ondemand.pcm")+ 1);
    sprintf(filename, SYSTEMSOUNDS_PATH "/alert_%i-ondemand.pcm",sound);
    g_message ("complete file name to playback = %s\n", filename);

    fp = fopen(filename, "r");
    free(filename);
    filename= NULL;
    if (!fp)
    {
        g_message ("%s : index not found reason %s",__FUNCTION__,strerror(errno));
        answer = INVALID_PARAMETER_ERROR(index, integer);
    }
    else
    {
        fclose (fp);
        fp = NULL;
        /*Add Ref message callback will unref message when generateAlertSound
        is done*/
        LSMessageRef(message);

        /* Playback is not happening through gAudioDevice interfaces
        without the pixie changes, hence gAudioMixer is used to invoke playback
        through software APIs
        */
        gAudioMixer.playSystemSound (string_printf("alert_%i", sound).c_str(),
                                     eeffects);

    }

Error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, answer, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static bool
_emergencyCall(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_0);
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    if (gState.getRingerOn ())
    {
        gAudioDevice.phoneEvent(ePhoneEvent_EmergencyCall);
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, STANDARD_JSON_SUCCESS, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static bool
_busyTone(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    // Add Ref message callback will unref message when ePhoneEvent_BusyTone is done
    LSMessageRef(message);
    Callback* callback = new Callback(&_standardSuccessResponseCB, message);
    gAudioDevice.phoneEvent(ePhoneEvent_BusyTone, *((int*)(callback)));

    return true;
}

#define DTMF_NONE        0
#define DTMF_MODEM       1
#define DTMF_GENERATOR   2
#define DTMF_SOUNDSERVER 3

static int gDTMFstate = DTMF_NONE;

static bool
stopCurrentDtmf (LSHandle *lshandle)
{
    PMTRACE_FUNCTION;
     if (gDTMFstate & DTMF_MODEM) {
        CLSError lserror;
        bool result;

        result = LSCall(lshandle, "palm://com.palm.telephony/dtmfEndLong", "{}",
                NULL, NULL, NULL, &lserror);
        if (!result)
            lserror.Print(__FUNCTION__, __LINE__);
    }
    if (gDTMFstate & DTMF_GENERATOR) {
        gAudioMixer.phoneEvent(ePhoneEvent_DTMFToneEnd, (gDTMFstate)?0:0x80);
    }
    if (gDTMFstate & DTMF_SOUNDSERVER) {
        gAudioMixer.stopDtmf();
    }

    gDTMFstate = DTMF_NONE;

    return true;
}

static bool
_playDTMF(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_3(REQUIRED(id, string),
                                 REQUIRED(feedbackOnly, boolean),
                                 OPTIONAL(oneshot, boolean)));
    PMTRACE_FUNCTION;
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar * answer = STANDARD_JSON_SUCCESS;

    bool feedbackOnly    = true;    // required
    const char * id        = NULL;    // required
    bool oneshot        = true;    // optional, true by default

    std::string ids;
    CLSError lserror;

    if (!msg.get("feedbackOnly", feedbackOnly))
    {
        answer = MISSING_PARAMETER_ERROR(feedbackOnly, boolean);
        goto sendReply;
    }

    if (!msg.get("id", ids))
    {
        answer = MISSING_PARAMETER_ERROR(id, string);
        goto sendReply;
    }
    // don't change id from now on.
    //(reuse old code the expects id to be a const char *...)
    id = ids.c_str();

    if (!msg.get("oneshot", oneshot))
        oneshot = true;

    // if we are already playing one and it is ont oneshot let's top it.
    stopCurrentDtmf(lshandle);

    if (gAudioDevice.isSuspended()) {
        answer = STANDARD_JSON_ERROR(4, "Audio suspended");
        goto sendReply;
    }

    if (gState.getOnActiveCall())
    {
        gDTMFstate = DTMF_NONE;
        if (!feedbackOnly) {
            // use modem
            gDTMFstate = DTMF_MODEM;
            if (oneshot)
            {
                // first check that the tones requested are legal
                int i, length = strlen(id);
                for (i = 0 ; i <  length ; i++)
                {
                    if ((id[i] < '0' || id[i] > '9') && id[i] != '*' && id[i] != '#')
                    {
                        answer = INVALID_PARAMETER_ERROR(id, string);
                        goto sendReply;
                    }
                }
                std::string cmd = string_printf("{\"toneSequence\":\"%s\"}", id);
                g_debug("_playDTMF: sending '%s' to 'palm://com.palm.telephony/sendDtmf'",
                                                                  cmd.c_str());
                if (!LSCall(lshandle, "palm://com.palm.telephony/sendDtmf",
                                      cmd.c_str(), NULL, NULL, NULL, &lserror))
                    lserror.Print(__FUNCTION__, __LINE__);
            }
            else
            {
                char tone = id[0];
                if ((tone >= '0' && tone <= '9') || tone == '*' || tone == '#')
                {
                    std::string cmd = string_printf("{\"tone\":\"%c\"}", tone);
                    g_debug("_playDTMF: sending '%s' to 'palm://com.palm.telephony/dtmfStartLong'",
                                                                 cmd.c_str());
                    if (!LSCall(lshandle, "palm://com.palm.telephony/dtmfStartLong",
                                                         cmd.c_str(), NULL,
                                                         NULL, NULL, &lserror))
                        lserror.Print(__FUNCTION__, __LINE__);
                }
                else
                {
                    answer = INVALID_PARAMETER_ERROR(id, string);
                    goto sendReply;
                }

            }
        }
        // use dtmf generator
        gDTMFstate |= DTMF_GENERATOR;
        int i, length = strlen(id);
        for (i = 0 ; i <  length ; i++)
        {
            char tone = id[i];
            if ((tone >= '0' && tone <= '9') || tone == '*' || tone == '#')
            {
                gAudioMixer.phoneEvent(oneshot ? ePhoneEvent_OneShotDTMFTone :
                                                  ePhoneEvent_DTMFTone,
                                                  feedbackOnly?tone:tone|0x80);
            }
            else
            {
                answer = INVALID_PARAMETER_ERROR(id, string);
                goto sendReply;
            }
        }
    }
    else
    {
        /* Only play if ringer switch is not muted */
        if ((id[0] >= '0' && id[0] <= '9') || id[0] == '*' || id[0] == '#' ) {
            // use sound server
            gDTMFstate = DTMF_SOUNDSERVER;
            if( gState.getRingerOn())
            {
                if (oneshot || true)
                    {
                        gAudioMixer.playOneshotDtmf(id, eDTMF);
                    }
                 else {
                        gAudioMixer.playDtmf(id, eDTMF);
                    }
            }
        }
        else
            {
                answer = INVALID_PARAMETER_ERROR(id, string);
                g_debug("Invalid id is passed %s (first character)",id);
                goto sendReply;
            }
    }

    if (oneshot)
    {
        gDTMFstate = DTMF_NONE;
    }

sendReply:
    if (!LSMessageReply(lshandle, message, answer, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static bool
_stopDTMF(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PMTRACE_FUNCTION;
    LSMessageJsonParser    msg(message, SCHEMA_0);
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    stopCurrentDtmf (lshandle);

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, STANDARD_JSON_SUCCESS, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

//===========_playRingtone API added for ringtone workaround===================

#if TEST_RINGTONE_VIA_AUDIOD

static bool
_playRingtone(LSHandle *lshandle, LSMessage *message, void *ctx)
{
      g_debug ("[cnu].......playRingtone............");
    LSMessageJsonParser    msg(message, SCHEMA_1(OPTIONAL(index, integer)));

    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar * answer = STANDARD_JSON_SUCCESS;

    int sound;
    if (!msg.get("index", sound))
        sound = 0;

    if (sound < 0 || sound > 63)
    {
        answer = INVALID_PARAMETER_ERROR(index, integer);
    }
    else
    {
        // Add Ref message callback will unref message when
        //generateAlertSound is done
        LSMessageRef(message);

        gAudioMixer.playSystemSound (string_printf("ringtone_%i", sound).c_str(),
                                                                  eringtones);

        return true;
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, answer, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

#endif
static LSMethod dtmfMethods[] = {
    { "playAlert", _playAlert},
    { "playDTMF", _playDTMF},
    { "stopDTMF", _stopDTMF},
    { "emergencyCall", _emergencyCall},
    { "busyTone", _busyTone},
#if TEST_RINGTONE_VIA_AUDIOD
    { "playRingtone", _playRingtone},
#endif
    { },
};

int
DtmfInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    /* luna service interface */
    CLSError lserror;
    bool result;
    result = ServiceRegisterCategory ("/dtmf", dtmfMethods, NULL, NULL);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/dtmf");
        return (-1);
    }

    return 0;
}

#if DTMF_SUPPORTED
CONTROL_START_FUNC (DtmfInterfaceInit);
#endif
