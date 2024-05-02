/* @@@LICENSE
*
*      Copyright (c) 2020-2024 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include "systemSoundsManager.h"

bool SystemSoundsManager::mIsObjRegistered = SystemSoundsManager::RegisterObject();
SystemSoundsManager* SystemSoundsManager::mSystemSoundsManager = nullptr;

SystemSoundsManager* SystemSoundsManager::getSystemSoundsManagerInstance()
{
    return mSystemSoundsManager;
}

SystemSoundsManager::SystemSoundsManager(ModuleConfig* const pConfObj)
{
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if (!mObjAudioMixer)
    {
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
            "AudioMixer instance is null");
    }
    PM_LOG_DEBUG("systemsounds constructor");
}

SystemSoundsManager::~SystemSoundsManager()
{
    PM_LOG_DEBUG("systemsounds destructor");
}

LSMethod SystemSoundsManager::systemsoundsMethods[] = {
    { "playFeedback", SystemSoundsManager::_playFeedback},
    { },
};

bool SystemSoundsManager::_playFeedback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_5(REQUIRED(name, string),
                                  OPTIONAL(sink, string),
                                  OPTIONAL(play, boolean),
                                  OPTIONAL(override, boolean),
                                  OPTIONAL(type, string)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const gchar * reply = STANDARD_JSON_SUCCESS;
    EVirtualAudioSink sink = edefaultapp;
    std::string    name, sinkName;
    bool play = true;
    bool override = false;
    char *filename = NULL;
    FILE *fp = NULL;
    size_t size = 0;

    SystemSoundsManager* SystemSoundsManagerInstance = SystemSoundsManager::getSystemSoundsManagerInstance();
    AudioMixer* audioMixerObj = SystemSoundsManagerInstance->mObjAudioMixer;

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
    size = strlen(SYSTEMSOUNDS_PATH) + strlen(name.c_str()) + strlen("-ondemand.pcm")+ 1;
    filename = (char *)malloc( size );
    if (filename == NULL)
    {
         reply = STANDARD_JSON_ERROR(4, "Unable to allocate memory");
         goto error;
    }
    if (snprintf(filename, size, SYSTEMSOUNDS_PATH "%s-ondemand.pcm", name.c_str()) < 0)
    {
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT,"snprintf failed to generate filename. Returning from here\n");
        goto error;
    }
    else if (snprintf(filename, size, SYSTEMSOUNDS_PATH "%s-ondemand.pcm", name.c_str()) >= size)
    {
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT,"snprintf buffer overflow. Filename might be truncated\n");
    }

    fp = fopen(filename, "r");
    free(filename);
    filename= NULL;
    if (!fp){
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
            "Error : %s : file open failed. returning from here\n", __FUNCTION__);
         reply = INVALID_PARAMETER_ERROR(name, string);
         goto error;
    }
    else{
        if(fclose(fp))
        {
            PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT,"Failed to close file");
        }
        fp = NULL;
    }

    // if "play" is false, pre-load the sound & do nothing else
    if (!msg.get("play", play))
        play = true;

    if (msg.get("override", override))
        override = true;
    PM_LOG_INFO(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
        "%s override = %d\n", __FUNCTION__, override);

    if (!audioMixerObj)
    {
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
                "audioMixerObj is null");
        goto error;
    }
    if (play)
    {
        if (!audioMixerObj->playSystemSound(name.c_str(), sink))
        {
            reply = STANDARD_JSON_ERROR(3, "unable to connect to pulseaudio.");
            goto error;
        }
    }
    else
    {
        audioMixerObj->preloadSystemSound(name.c_str());
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror)){
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
            "returning FALSE becuase of invald parameters");
        return false;
    }

    return true;
}

void SystemSoundsManager::initialize()
{
    if (mSystemSoundsManager)
    {
        bool result = false;
        CLSError lserror;

        result = ServiceRegisterCategory ("/systemsounds", SystemSoundsManager::systemsoundsMethods, NULL, NULL);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
                "%s: Registering Service for '%s' category failed", __FUNCTION__, "/systemsounds");
        }
        PM_LOG_INFO(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
            "Successfully initialized SystemSoundsManager");
    }
    else
    {
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, \
            "mSystemSoundsManager is nullptr");
    }
}

void SystemSoundsManager::deInitialize()
{
    PM_LOG_DEBUG("SystemSoundsManager deInitialize");
    if (mSystemSoundsManager)
    {
        delete mSystemSoundsManager;
        mSystemSoundsManager = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT, "mSystemSoundsManager is nullptr");
}

void SystemSoundsManager::handleEvent(events::EVENTS_T* ev)
{
}
