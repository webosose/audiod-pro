/* @@@LICENSE
*
*      Copyright (c) 2024 LG Electronics Company.
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

#ifndef _TRACK_MGR_H_
#define _TRACK_MGR_H_

#include "utils.h"
#include "messageUtils.h"
#include "main.h"
#include "moduleInterface.h"
#include "moduleFactory.h"
#include "moduleManager.h"

#define LUNA_COMMAND "com.webos.lunasend"
#define MAX_TRACK_COUNT 64
typedef struct ClientInfo
{
        ClientInfo() : serviceName(""), serverCookie(nullptr) {};
        std::string serviceName;
        void* serverCookie;
}CLIENT_INFO_T;

class TrackManager : public ModuleInterface
{
    private:

        static bool mIsObjRegistered;
        ModuleManager* mObjModuleManager;
        TrackManager(const TrackManager&) = delete;
        TrackManager &operator=(const TrackManager&) = delete;
        TrackManager(ModuleConfig* const pConfObj);
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_track_manager", &TrackManager::CreateObject));
        }

        std::map<std::string, std::string> mMapTrackIdList;
        std::map<std::string, CLIENT_INFO_T> mMapPipelineTrackId;
    public:
        static TrackManager *getTrackManagerObj();
        static TrackManager *mObjTrackManager;
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the TrackManager handler");
                mObjTrackManager = new(std::nothrow) TrackManager(pConfObj);
                if (mObjTrackManager)
                    return mObjTrackManager;
            }
            return nullptr;
        }
        ~TrackManager();
        void initialize();
        void deInitialize();
        void handleEvent(events::EVENTS_T *event);

        static bool _registerTrack(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _unregisterTrack(LSHandle *lshandle, LSMessage *message, void *ctx);

        static bool disconnetedCb( LSHandle *sh, const char *serviceName, bool connected, void *ctx);
};


#endif