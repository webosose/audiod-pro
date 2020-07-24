/* @@@LICENSE
*
*      Copyright (c) 2020 LG Electronics Company.
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

#include "connectionManager.h"

ConnectionManager* ConnectionManager::mConnectionManager = nullptr;

ConnectionManager* ConnectionManager::getConnectionManager()
{
    return mConnectionManager;
}

ConnectionManager::ConnectionManager()
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, \
               "ConnectionManager constructor");
    mAudioMixer = AudioMixer::getAudioMixerInstance();
    if (!mAudioMixer)
    {
        PM_LOG_ERROR(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, \
            "AudioMixer instance is null");
    }
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (!mObjModuleManager)
    {
        PM_LOG_ERROR(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, \
            "mObjModuleManager instance is null");
    }
}

ConnectionManager::~ConnectionManager()
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, \
                "ConnectionManager destructor");
}

void ConnectionManager::notifyGetStatusSubscribers()
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "ConnectionManager::notifyGetStatusSubscribers");
    CLSError lserror;
    LSHandle *lsHandle = GetPalmService();
    if (!getAudioOutputStatus(lsHandle, nullptr, nullptr))
        lserror.Print(__FUNCTION__, __LINE__);
}

bool ConnectionManager::_mute(LSHandle *lshandle, LSMessage *message, void *ctx) {

   /*TBD: Implementation of input volume mute/unmute
     Temporarily, sending success message on receiving this API */

    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_4(PROP(source, string),
    PROP(sourcePort, integer), PROP(sink, string), PROP(mute, boolean))
    REQUIRED_4(source, sourcePort, sink, mute)));

    std::string reply = STANDARD_JSON_SUCCESS;

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;

    CLSError lserror;
    if (!LSMessageRespond(message,reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
    }
    return true;
}

bool ConnectionManager::_connectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_6(PROP(source, string),
    PROP(sourcePort, integer), PROP(sink, string), PROP(audioType,string),
    PROP(outputMode,string), PROP(context,string))
    REQUIRED_5(source, sourcePort, sink, outputMode, audioType)));

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;
    std::string audioType;
    msg.get("audioType", audioType);
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "ConnectionManager::_connectAudioOut Audio connect request");
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;
    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_AUDIO_TYPE, "Internal error");
    if (!LSMessageRespond(message, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
    }
    return true;
}

bool ConnectionManager::_disconnectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_5(PROP(source, string),
    PROP(sourcePort, integer), PROP(sink, string), PROP(audioType,string), PROP(context,string))
    REQUIRED_4(source, sourcePort, sink, audioType)));

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;

    std::string audioType;
    msg.get("audioType",audioType);
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "ConnectionManager::_disconnectAudioOut Audio disconnect request");
    CLSError lserror;
    std::string reply = STANDARD_JSON_SUCCESS;
    reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_AUDIO_TYPE, "Internal error");
    if (!LSMessageRespond(message, reply.c_str(), &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
    }
    return true;
}

bool ConnectionManager::getAudioOutputStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    envelopeRef *handle = nullptr;
    if (nullptr != message)
    {
        handle = new envelopeRef();
        if(nullptr != handle)
        {
            handle->message = message;
        }
        LSMessageRef(message);
    }
    AudioMixer* audioMixerObj = AudioMixer::getAudioMixerInstance();
    std::string reply = STANDARD_JSON_SUCCESS;
    bool returnvalue = false;
    if(nullptr != audioMixerObj)
    {
        if (audioMixerObj->getConnectionStatus(_getAudioOutputStatusCallback, handle))
        {
            returnvalue = true;
        }
        else
        {
            PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "get connect status umimixer call failed");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_FAILED_MIXER_CALL, "Internal error");
        }
    }
    else
    {
        PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "Audio get connect status: audioMixerObj is NULL");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE, "Internal error");
    }
    if (false == returnvalue)
    {
        CLSError lserror;
        if (nullptr != message)
            if (!LSMessageRespond(message, reply.c_str(), &lserror))
                lserror.Print(__FUNCTION__, __LINE__);
        if (nullptr != handle)
        {
            delete handle;
            handle = nullptr;
        }
    }
    return true;
}

bool ConnectionManager::_getStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "ConnectionManager::_getStatus Audio get status request");
    LSMessageJsonParser msg (message, NORMAL_SCHEMA(PROPS_1(PROP(subscribe, boolean))));

    if(!msg.parse(__FUNCTION__,lshandle))
        return true;

    bool subscribed = false;
    CLSError lserror;
    if (!LSSubscriptionProcess(lshandle, message, &subscribed, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        return false;
    }

    if (nullptr != ctx)
        static_cast <ConnectionManager*>(ctx)->getAudioOutputStatus(lshandle, message, ctx);
    return true;
}

bool ConnectionManager::_getAudioOutputStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx)
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "_getAudioOutputStatusCallback Received");

    LSMessageJsonParser msg(reply, NORMAL_SCHEMA(PROPS_1(PROP(returnValue, boolean))
                            REQUIRED_1(returnValue)));
    if(!msg.parse(__FUNCTION__, sh))
        return true;
    bool returnValue = false;
    msg.get("returnValue", returnValue);

    if (false == returnValue)
    {
        PM_LOG_ERROR(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "%s %d Could not get status", __FUNCTION__, __LINE__);
    }
    else
    {
        LSMessageJsonParser msgData (reply, STRICT_SCHEMA(PROPS_2(PROP(returnValue, boolean),
        PROP(audio, array)) REQUIRED_2(returnValue, audio)));
    }
    std::string payload = LSMessageGetPayload(reply);
    if (nullptr != ctx)
    {
        envelopeRef *handle = (envelopeRef *)ctx;
        LSMessage *messageptr = (LSMessage*)handle->message;

        if (nullptr != messageptr)
        {
            CLSError lserror;
            if (!LSMessageRespond(messageptr, payload.c_str(), &lserror))
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
            LSMessageUnref(messageptr);
        }

        delete handle;
        handle = nullptr;
    }
    else
    {
        PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "context is null");
        LSHandle *lsHandle = GetPalmService();
        CLSError lserror;
        if (! LSSubscriptionReply(lsHandle, GET_STATUS_CATEGORY_AND_KEY, payload.c_str(), &lserror))
        {
            lserror.Print(__FUNCTION__, __LINE__);
            PM_LOG_ERROR(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "Notify error");
        }
    }
    return true;
}

LSMethod ConnectionManager::connectionManagerMethods[] =
{
    { "connect", ConnectionManager::_connectAudioOut},
    { "disconnect", ConnectionManager::_disconnectAudioOut},
    { "getStatus", ConnectionManager::_getStatus},
    { "mute", ConnectionManager::_mute},
    { },
};

void ConnectionManager::loadModuleConnectionManager(GMainLoop *loop, LSHandle* handle)
{
    CLSError lserror;
    if (!mConnectionManager)
    {
        mConnectionManager = new (std::nothrow)ConnectionManager();
        if (mConnectionManager)
        {
            PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "load module ConnectionManager successful");
            bool bRetVal = LSRegisterCategoryAppend(handle, UMI_CATEGORY_NAME, connectionManagerMethods, nullptr, &lserror);
            if (!bRetVal || !LSCategorySetData(handle, UMI_CATEGORY_NAME, mConnectionManager, &lserror))
            {
                PM_LOG_ERROR(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT,\
                            "%s: Registering Service for '%s' category failed", __FUNCTION__, UMI_CATEGORY_NAME);
                lserror.Print(__FUNCTION__, __LINE__);
            }
            PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "load module ConnectionManager Init Done");
        }
        else
            PM_LOG_ERROR(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "Could not load module ConnectionManager");
    }
}

int load_connection_manager(GMainLoop *loop, LSHandle* handle)
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "initializing connection_manager");
    ConnectionManager::loadModuleConnectionManager(loop, handle);
    return 0;
}

void unload_connection_manager()
{
    PM_LOG_INFO(MSGID_CONNECTION_MANAGER, INIT_KVCOUNT, "unloading connection_manager");
    if (ConnectionManager::mConnectionManager)
    {
        delete ConnectionManager::mConnectionManager;
        ConnectionManager::mConnectionManager = nullptr;
    }
}