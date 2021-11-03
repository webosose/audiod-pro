/* @@@LICENSE
*
*      Copyright (c) 2022 LG Electronics Company.
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

#include "trackManager.h"

bool TrackManager::mIsObjRegistered = TrackManager::RegisterObject();
TrackManager* TrackManager::mObjTrackManager = nullptr;

TrackManager* TrackManager::getTrackManagerObj()
{
    return mObjTrackManager;
}

bool TrackManager::_registerTrack(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_TRACKMANAGER, INIT_KVCOUNT, "TrackManager: _registerTrack");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(streamType,string)) REQUIRED_1(streamType)));
    std::string reply ;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;

    std::string streamType;
    msg.get("streamType",streamType);
    pbnjson::JValue resp = pbnjson::JObject();
    TrackManager *trackManagerInstance = TrackManager::getTrackManagerObj();
    if (trackManagerInstance)
    {
        if (getSinkByName(streamType.c_str()) != eVirtualSink_None)
        {
            std::string trackId = GenerateUniqueID()();
            trackManagerInstance->mMapTrackIdList[trackId] = streamType;

            //send event to audiopolicy manager
            events::EVENT_REGISTER_TRACK_T stRegisterTrack;
            stRegisterTrack.eventName = utils::eEventRegisterTrack;
            stRegisterTrack.streamType = streamType;
            stRegisterTrack.trackId = trackId;
            trackManagerInstance->mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stRegisterTrack);

            //send success response
            resp.put("trackId",trackId);
            resp.put("returnValue",true);
            utils::LSMessageResponse(lshandle, message, resp.stringify().c_str(), utils::eLSRespond, false);
        }
        else
        {
            std::string reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_STREAM,"Audiod Unknown Stream");
            utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
        }
    }
    else
    {
        std::string reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR,"Audiod internal error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);

    }
    return true;
}

bool TrackManager::_unregisterTrack(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_TRACKMANAGER, INIT_KVCOUNT, "TrackManager: _unregisterTrack");
    LSMessageJsonParser msg(message, STRICT_SCHEMA(PROPS_1(PROP(trackId,string)) REQUIRED_1(trackId)));
    std::string reply;
    if (!msg.parse(__FUNCTION__,lshandle))
       return true;

    std::string trackId;
    msg.get("trackId", trackId);
    TrackManager *trackManagerInstance = TrackManager::getTrackManagerObj();
    if (trackManagerInstance)
    {
        if (trackManagerInstance->mMapTrackIdList.find(trackId) != trackManagerInstance->mMapTrackIdList.end())
        {
            PM_LOG_INFO(MSGID_TRACKMANAGER, INIT_KVCOUNT, "TrackManager: _unregisterTrack trackId found");
            // notify audio policy manager about track unregister
            events::EVENT_UNREGISTER_TRACK_T stUnregisterTrack;
            stUnregisterTrack.eventName = utils::eEventUnregisterTrack;
            stUnregisterTrack.trackId = trackId;
            trackManagerInstance->mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&stUnregisterTrack);
            //
            trackManagerInstance->mMapTrackIdList.erase(trackId);
            PM_LOG_ERROR(MSGID_TRACKMANAGER, INIT_KVCOUNT, "AudioPolicyManager: removeTrackId success");
            pbnjson::JValue response = pbnjson::JObject();
            response.put("returnValue", true);
            response.put("trackId", trackId);
            reply = response.stringify();
        }
        else
        {
            PM_LOG_ERROR(MSGID_TRACKMANAGER, INIT_KVCOUNT, "AudioPolicyManager: removeTrackId failed");
            reply =  STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_UNKNOWN_TRACKID, "Audiod Unknown trackId");
        }

        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    }
    else
    {
        PM_LOG_ERROR(MSGID_TRACKMANAGER, INIT_KVCOUNT, "AudioPolicyManager: trackManagerInstance is null");
        std::string reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR,"Audiod internal error");
        utils::LSMessageResponse(lshandle, message, reply.c_str(), utils::eLSRespond, false);
    }
    return true;
}


static LSMethod trackManagerMethods[] = {
    {"registerTrack", TrackManager::_registerTrack},
    {"unregisterTrack", TrackManager::_unregisterTrack},
    {}
};

void TrackManager::initialize()
{
    if (mObjTrackManager)
    {
        PM_LOG_INFO(MSGID_TRACKMANAGER, INIT_KVCOUNT, "TrackManager: initialize completed");
        CLSError lserror;
        bool bRetVal;
        bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/", trackManagerMethods, nullptr, &lserror);

        if (!bRetVal)
        {
           PM_LOG_ERROR(MSGID_TRACKMANAGER, INIT_KVCOUNT, \
            "%s: Registering Service for '%s' category failed", __FUNCTION__, "/");
           lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
        PM_LOG_ERROR(MSGID_TRACKMANAGER, INIT_KVCOUNT, "mObjTrackManager is nullptr");
}

void TrackManager::deInitialize()
{
    PM_LOG_DEBUG("TrackManager deinitialise");
    if (mObjTrackManager)
    {
        delete mObjTrackManager;
        mObjTrackManager = nullptr;
    }
}

TrackManager::TrackManager(ModuleConfig* const pConfObj)
{
    PM_LOG_DEBUG("TrackManager: constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
    {

    }

}

TrackManager::~TrackManager()
{
    PM_LOG_DEBUG("TrackManager: destructor");
}

void TrackManager::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {

    }

}