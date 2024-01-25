/* @@@LICENSE
*
* Copyright (c) 2020-2024 LG Electronics Company.
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
bool PlaybackManager::mIsObjRegistered = PlaybackManager::RegisterObject();

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


void PlaybackManager::stopInternal(std::string playbackId)
{
    PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
        "stopInternal: playbackId: %s",playbackId.c_str());
    PlaybackManager* playbackObj = PlaybackManager::getPlaybackManagerInstance();

    AudioMixer* audioMixerObj = playbackObj->mObjAudioMixer;

    if (audioMixerObj && !(audioMixerObj->controlPlayback(playbackId, "stop")))
    {
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, "stopPlayback throwed error");
    }
}

void PlaybackManager::notifyGetPlayabackStatus(std::string& playbackId, std::string& state)
{
    PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
        "notifyGetPlayabackStatus: playbackId: %s %s",playbackId.c_str(), state.c_str());

    CLSError lserror;
    std::string reply;
    std::string key(AUDIOD_API_GET_PLAYBACK_STATUS);
    pbnjson::JValue returnPayload  = pbnjson::Object();
    bool retValAcquire = false;
    LSSubscriptionIter *iter = NULL;

    returnPayload.put("playbackId", playbackId);
    returnPayload.put("state", state);
    key.append("/" + playbackId);
    if (!LSSubscriptionReply(GetPalmService(), \
        key.c_str(), returnPayload.stringify().c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, "Notify error");
    }
    else
    {
        if(state == "stopped")
        {
            retValAcquire = LSSubscriptionAcquire(GetPalmService(), key.c_str(), &iter, &lserror);
            if(retValAcquire)
            {
              while(LSSubscriptionHasNext(iter)){
                    LSMessage *subscribeMessage = LSSubscriptionNext(iter);
                    LSSubscriptionRemove(iter);
                    break;

                }
                LSSubscriptionRelease(iter);
            }
            std::thread th(PlaybackManager::stopInternal, playbackId);
            th.detach();
        }
    }
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
    pbnjson::JValue resp = pbnjson::JObject();
    CLSError lserror;

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
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_INPUT_PARAMS, "Invalid virtual sink name");
            LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            return true;
        }
        if (!playbackObj->isValidFileExtension(filePath))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_INPUT_PARAMS, "Invalid file format");
            LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            return true;
        }
        if (!playbackObj->isValidSampleFormat(format))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_INPUT_PARAMS, "Invalid sample format");
            LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            return true;
        }
        if (!playbackObj->isValidSampleRate(sampleRate))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_INPUT_PARAMS, "Invalid sample rate");
            LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            return true;
        }
        if (!playbackObj->isValidChannelCount(channels))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_INPUT_PARAMS, "Invalid channel count");
            LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            return true;
        }

        fp = fopen(filePath.c_str(), "r");
        if (!fp)
        {
            PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                "Error : %s : file %s open failed. returning from here\n", __FUNCTION__, filePath.c_str());
            reply = STANDARD_JSON_ERROR(19, "Invalid Params");
            LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            return true;
        }
        else
        {
            fclose(fp);
            fp = NULL;
        }
        AudioMixer* audioMixerObj = playbackObj->mObjAudioMixer;
        if (audioMixerObj)
        {
            std::string playbackID = audioMixerObj->playSound(filePath.c_str(), \
                virtualSink, format.c_str(), sampleRate, channels);

            if(playbackID.empty())
            {
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Could not play the audio file");
                LSMessageReply(lshandle, message, reply.c_str(), &lserror);
                return true;
            }
            else
            {
                resp.put("playbackId",playbackID);
                resp.put("returnValue",true);
                utils::LSMessageResponse(lshandle, message, resp.stringify().c_str(), utils::eLSRespond, false);
                return true;
            }
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Could not get the playbck instance");
        LSMessageReply(lshandle, message, reply.c_str(), &lserror);
        return true;
    }

    return true;
}

bool PlaybackManager::_controlPlayback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(\
        PROP(playbackId, string), PROP(requestType, string))\
        REQUIRED_2(playbackId, requestType)));


    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    std::string reply = STANDARD_JSON_SUCCESS;
    std::string playbackId;
    std::string requestType;
    CLSError lserror;

    msg.get("playbackId", playbackId);
    msg.get("requestType", requestType);
    PlaybackManager* playbackObj = PlaybackManager::getPlaybackManagerInstance();

    if (nullptr != playbackObj)
    {
        AudioMixer* audioMixerObj = playbackObj->mObjAudioMixer;

        if (audioMixerObj && !(audioMixerObj->controlPlayback(playbackId, requestType)))
        {
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid Params");
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, \
            "Could not get the playbck instance");
    }
    LSMessageReply(lshandle, message, reply.c_str(), &lserror);
    return true;

}

bool PlaybackManager::_getPlaybackStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_2(\
        PROP(subscribe, boolean),
        PROP(playbackId, string))\
        REQUIRED_1(playbackId)));

    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    std::string reply = STANDARD_JSON_SUCCESS;
    std::string playbackId;
    CLSError lserror;
    bool subscribed;
    if (!msg.get("subscribe", subscribed))
        subscribed=false;
    pbnjson::JValue resp = pbnjson::JObject();


    msg.get("playbackId", playbackId);
    PlaybackManager* playbackObj = PlaybackManager::getPlaybackManagerInstance();

    if (nullptr != playbackObj)
    {
        AudioMixer* audioMixerObj = playbackObj->mObjAudioMixer;

        if (audioMixerObj)
        {
            std::string state = audioMixerObj->getPlaybackStatus(playbackId);
            if(state.empty())
            {
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_PARAMS, "Invalid Params");
                LSMessageReply(lshandle, message, reply.c_str(), &lserror);
            }
            else
            {
                if (LSMessageIsSubscription(message))
                {
                    const char *key = LSMessageGetKind(message);
                    std::string streamKey(key);
                    streamKey.append("/" + playbackId);
                    if(!LSSubscriptionAdd(lshandle, streamKey.c_str(), message, NULL))
                    {
                        lserror.Print(__FUNCTION__, __LINE__);
                        PM_LOG_CRITICAL(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                            "LSSubscriptionAdd failed");
                        return true;
                    }
                }
                resp.put("playbackStatus",state);
                resp.put("returnValue",true);
                utils::LSMessageResponse(lshandle, message, resp.stringify().c_str(), \
                    utils::eLSRespond, false);
            }
        }
    }
    else
    {
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, \
            "Could not get the playbck instance");
        LSMessageReply(lshandle, message, reply.c_str(), &lserror);
    }
    return true;
}

LSMethod PlaybackManager::playbackMethods[] = {
    { "playSound", PlaybackManager::_playSound},
    { "controlPlayback", PlaybackManager::_controlPlayback},
    { "getPlaybackStatus", PlaybackManager::_getPlaybackStatus},
    { },
};

PlaybackManager::PlaybackManager(ModuleConfig* const pConfObj)
{
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if (!mObjAudioMixer)
    {
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
            "AudioMixer instance is null");
    }
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventGetPlaybackStatus);
    }
    else
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT,\
            "PlaybackManager:mObjModuleManager is null");
    PM_LOG_DEBUG("playback manager constructor");
}

PlaybackManager::~PlaybackManager()
{
    PM_LOG_DEBUG("Playback manager destructor");
}

void PlaybackManager::initialize()
{
    if (mPlaybackManager)
    {
        bool result = false;
        CLSError lserror;
        result = LSRegisterCategoryAppend(GetPalmService(), "/", PlaybackManager::playbackMethods, nullptr, &lserror);
        if (!result || !LSCategorySetData(GetPalmService(), "/", mPlaybackManager, &lserror))
        {
            PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
                "%s registering for %s category faled",  __FUNCTION__, "/");
            lserror.Print(__FUNCTION__, __LINE__);
        }
        PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
            "Successfully initialized PlaybackManager");
    }
    else
    {
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, \
            "mPlaybackManager is nullptr");
    }
}

void PlaybackManager::deInitialize()
{
    PM_LOG_DEBUG("PlaybackManager deInitialize");
    if (mPlaybackManager)
    {
        delete mPlaybackManager;
        mPlaybackManager = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT, "mPlaybackManager is nullptr");
}

void PlaybackManager::handleEvent(events::EVENTS_T* event)
{
    switch(event->eventName)
    {
        case utils::eEventGetPlaybackStatus:
        {
            PM_LOG_INFO(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT,\
                "handleEvent : eEventGetPlaybackStatus");
            events::EVENT_GET_PLAYBACK_STATUS_INFO_T *stEventPlaybackStatus = \
                (events::EVENT_GET_PLAYBACK_STATUS_INFO_T*)event;
            notifyGetPlayabackStatus(stEventPlaybackStatus->playbackId, \
                stEventPlaybackStatus->state);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_PLAYBACK_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}