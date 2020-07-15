/* @@@LICENSE
*
* Copyright (c) 2020 LG Electronics Company.
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

#include "playbackManager.h"

#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_CHANNELS 2
#define DEFAULT_SAMPLE_FORMAT "PA_SAMPLE_S32LE"

PlaybackManager* PlaybackManager::mPlaybackManager = nullptr;

PlaybackManager* PlaybackManager::getPlaybackManagerInstance()
{
    return mPlaybackManager;
}

bool PlaybackManager::isValidSampleFormat(const std::string& format)
{
    bool success = false;
    if (("PA_SAMPLE_S16LE" == format) || ("PA_SAMPLE_S24LE" == format) || ("PA_SAMPLE_S32LE" == format))
        success = true;
    return success;
}

bool PlaybackManager::isValidSampleRate(const int& rate)
{
    bool success = false;
    if ((44100 == rate) || (48000 == rate) || (32000 == rate) || (22050 == rate))
        success = true;
    return success;
}

bool PlaybackManager::isValidChannelCount(const int& channels)
{
    bool success = false;
    if ((1 <= channels) && (6 >= channels))
        success = true;
    return success;
}

bool PlaybackManager::isValidFileExtension(const std::string& filePath)
{
    bool success = false;
    // Find the last position of '.' in given string
    std::size_t pos = filePath.rfind('.');
    std::string extension;
    // If last '.' is found
    if (pos != std::string::npos)
    {
        // return the substring
        extension = filePath.substr(pos);
        if ((".wav" == extension) || (".pcm" == extension))
            success = true;
    }
    return success;
}

bool PlaybackManager::_playSound(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_5(\
        PROP(fileName, string), PROP(sink, string), PROP(format, string), \
        PROP(sampleRate , integer), PROP(channels, integer))\
        REQUIRED_2(fileName, sink)));

    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    FILE *fp = NULL;
    std::string filePath;
    std::string sink;
    std::string format;
    int sampleRate = 0;
    int channels = 0;

    std::string reply = STANDARD_JSON_SUCCESS;

    msg.get("fileName", filePath);
    msg.get("sink", sink);
    if (!msg.get("format", format))
        format = DEFAULT_SAMPLE_FORMAT;
    if (!msg.get("sampleRate", sampleRate))
        sampleRate = DEFAULT_SAMPLE_RATE;
    if (!msg.get("channels", channels))
        channels = DEFAULT_CHANNELS;

    PlaybackManager* playbackObj = PlaybackManager::getPlaybackManagerInstance();

    if (nullptr != playbackObj)
    {
        EVirtualAudioSink virtualSink = getSinkByName(sink.c_str());

        if (eVirtualSink_None == virtualSink)
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid virtual sink name");
            goto error;
        }
        if (!playbackObj->isValidFileExtension(filePath))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid file format");
            goto error;
        }
        if (!playbackObj->isValidSampleFormat(format))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid sample format");
            goto error;
        }
        if (!playbackObj->isValidSampleRate(sampleRate))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid sample rate");
            goto error;
        }
        if (!playbackObj->isValidChannelCount(channels))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid channel count");
            goto error;
        }

        fp = fopen(filePath.c_str(), "r");
        if (!fp)
        {
            PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                "Error : %s : file %s open failed. returning from here\n", __FUNCTION__, filePath.c_str());
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Could not open audio file");
            goto error;
        }
        else
        {
            fclose(fp);
            fp = NULL;
        }
        AudioMixer* audioMixerObj = playbackObj->mObjAudioMixer;
        if (audioMixerObj && !(audioMixerObj->playSound(filePath.c_str(), virtualSink, format.c_str(), \
            sampleRate, channels)))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Could not play the audio file");
            goto error;
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Could not get the playbck instance");
        goto error;
    }

error:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
            "returning FALSE becuase of invald parameters");
    }
    return true;
}

LSMethod PlaybackManager::playbackMethods[] = {
    { "playSound", PlaybackManager::_playSound},
    { },
};

PlaybackManager::PlaybackManager()
{
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if (!mObjAudioMixer)
    {
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
            "AudioMixer instance is null");
    }
    PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
        "playback manager constructor");
}

PlaybackManager::~PlaybackManager()
{
    PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
        "Playback manager destructor");
}

void PlaybackManager::loadPlaybackModuleManager(GMainLoop *loop, LSHandle* handle)
{
    if (!mPlaybackManager)
    {
        mPlaybackManager = new (std::nothrow) PlaybackManager();
        if (mPlaybackManager)
        {
            PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                "load playback manager successful");
            bool result = false;
            CLSError lserror;
            result = LSRegisterCategoryAppend(handle, "/", PlaybackManager::playbackMethods, nullptr, &lserror);
            if (!result || !LSCategorySetData(handle, "/", mPlaybackManager, &lserror))
            {
                PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                    "%s registering for %s category faled",  __FUNCTION__, "/");
                lserror.Print(__FUNCTION__, __LINE__);
            }
            PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                "loadPlaybackModuleManager done");
        }
        else
        {
            PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                "loadPlaybackModuleManager failed");
        }
    }
}

void unload_playback_manager()
{
    PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
        "unloading playback manager");
    if (PlaybackManager::mPlaybackManager)
    {
        delete PlaybackManager::mPlaybackManager;
        PlaybackManager::mPlaybackManager = nullptr;
    }
}

int load_playback_manager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
        "loading playback manager");
    PlaybackManager::loadPlaybackModuleManager(loop, handle);
    return 0;
}
