// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#include "AudioMixer.h"
#include "state.h"
#include "utils.h"
#include "messageUtils.h"
#include <errno.h>
#include "main.h"

static bool
_playCallertone(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_1(REQUIRED(name, string)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar *reply = STANDARD_JSON_SUCCESS;
    EVirtualSink sink = ecallertone;
    std::string name;
    char *fileName = NULL;
    int fileExist;

    if (!msg.get("name", name)) {
        reply = MISSING_PARAMETER_ERROR(name, string);
        goto error;
    }

    for(std::string::size_type i = 0; i < name.length(); ++i)
        name[i] = std::tolower(name[i]);

    size_t size ;
    size = strlen(SYSTEMSOUNDS_PATH) + strlen(name.c_str()) + strlen("-ondemand.pcm") + 1 ;
    fileName = (char *)calloc(1, size);
    if (fileName == NULL) {
        g_debug("Error : %s : unable to allocate memory\n", __FUNCTION__);
        reply = INVALID_PARAMETER_ERROR(name, string);
        goto error;
    }
    snprintf(fileName, size, SYSTEMSOUNDS_PATH "%s-ondemand.pcm", name.c_str());
    fileExist = access(fileName, F_OK | R_OK);
    free(fileName);
    fileName = NULL;

    if (fileExist) {
        g_debug("Error : %s : file access failed.\n", __FUNCTION__);
        reply = INVALID_PARAMETER_ERROR(name, string);
        goto error;
    }

    if (!gAudioMixer.playSystemSound(name.c_str(), sink)) {
        reply = STANDARD_JSON_ERROR(3, "unable to connect to pulseaudio.");
        goto error;
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror)) {
        lserror.Print(__FUNCTION__, __LINE__);
        g_warning("returning false becuase of invalid parameters");
        return false;
    }

    return true;
}

static LSMethod tonegeneratorMethods[] = {
    { "playCallertone", _playCallertone},
    { },
};

int
TonegeneratorInterfaceInit(GMainLoop *loop, LSHandle *handle)
{
    /* luna service interface */
    CLSError lserror;
    bool result;
    result = ServiceRegisterCategory ("/tonegenerator", tonegeneratorMethods, NULL, NULL);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/tonegenerator");
        return (-1);
    }

    return 0;
}

CONTROL_START_FUNC(TonegeneratorInterfaceInit);
