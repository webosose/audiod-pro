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


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <stdio.h>
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "main.h"
#include "audioMixer.h"
#include "PulseAudioMixer.h"

static bool
_playFeedback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_5(REQUIRED(name, string),
                                  OPTIONAL(sink, string),
                                  OPTIONAL(play, boolean),
                                  OPTIONAL(override, boolean),
                                  OPTIONAL(type, string)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar * reply = STANDARD_JSON_SUCCESS;
    EVirtualAudioSink sink = efeedback;
    std::string    name, sinkName;
    bool bPlay = true;
    bool bOverride = false;
    char *filename = NULL;
    FILE *fp = NULL;

    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();

    if (!msg.get("name", name))
    {
        reply = MISSING_PARAMETER_ERROR(name, string);
        goto error;
    }

    if (msg.get("sink", sinkName))
    {
        sink = getSinkByName(sinkName.c_str());
        if (!IsValidVirtualSink(sink))
        {
            reply = INVALID_PARAMETER_ERROR(sink, string);
            goto error;
        }
    }
    size_t size ;
    size = strlen(SYSTEMSOUNDS_PATH) + strlen(name.c_str()) + strlen("-ondemand.pcm")+ 1;
    filename = (char *)malloc( size );
    if (filename == NULL) {
         reply = STANDARD_JSON_ERROR(4, "Unable to allocate memory");
         goto error;
    }
    snprintf(filename, size, SYSTEMSOUNDS_PATH "%s-ondemand.pcm", name.c_str());
    g_debug("complete file name to playback = %s\n", filename);

    fp = fopen(filename, "r");
    free(filename);
    filename= NULL;
    if (!fp){
         g_debug("Error : %s : file open failed. returning from here\n", __FUNCTION__);
         reply = INVALID_PARAMETER_ERROR(name, string);
         goto error;
    }
    else{
         fclose(fp);
         fp = NULL;
    }

    // if "play" is false, pre-load the sound & do nothing else
    if (!msg.get("play", bPlay))
        bPlay = true;

    if (!msg.get("override", bOverride))
        bOverride = false;
    g_debug("%s play = %d, override = %d\n", __FUNCTION__, bPlay, bOverride);

    if (bPlay)
    {
        std::string type;
        if (msg.get("type", type))
        {
            //Will be implemented as per DAP design
        }

        if (audioMixerObj && bPlay && !audioMixerObj->playSystemSound(name.c_str(), sink))
        {
            reply = STANDARD_JSON_ERROR(3, "unable to connect to pulseaudio.");
            goto error;
        }
    }
    else
    {
        if (audioMixerObj)
            audioMixerObj->preloadSystemSound(name.c_str());
        else
            g_debug("audioMixerObj is null");
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror)){
        lserror.Print(__FUNCTION__, __LINE__);
        g_warning("returning FALSE becuase of invald parameters");
        return false;
    }

    return true;
}

static LSMethod systemsoundsMethods[] = {
    { "playFeedback", _playFeedback},
    { },
};

int
SystemsoundsInterfaceInit(GMainLoop *loop, LSHandle* handle)
{
    bool result;
    CLSError lserror;

    //Register luna-bus handlers
    //luna-send -n 1 luna://com.webos.service.audio/systemsounds/playFeedback
    // '{"name": "samplename","sink":"pfeedback"}'

    result = ServiceRegisterCategory ("/systemsounds", systemsoundsMethods, NULL, NULL);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/systemsounds");
        return (-1);
    }

    return 0;
}

SERVICE_START_FUNC (SystemsoundsInterfaceInit);
