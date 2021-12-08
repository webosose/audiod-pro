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

#ifndef _TRACK_MGR_H_
#define _TRACK_MGR_H_

#include "utils.h"
#include "messageUtils.h"
#include "main.h"
#include "moduleInterface.h"
#include "moduleFactory.h"
#include "moduleManager.h"

#include <functional>
#include <random>


#define AUDIOD_UNIQUE_ID_LENGTH 10
#define LUNA_COMMAND "com.webos.lunasend"

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


        class GenerateUniqueID {
            const std::string           source_;
            const int                   base_;
            const std::function<int()>  rand_;

            public:
            GenerateUniqueID(GenerateUniqueID&) = delete;
            GenerateUniqueID& operator= (const GenerateUniqueID&) = delete;

            explicit
            GenerateUniqueID(const std::string& src = "0123456789ABCDEFGIJKLMNOPQRSTUVWXYZabcdefgijklmnopqrstuvwxyz") :
                source_(src),
                base_(source_.size()),
                rand_(std::bind(
                    std::uniform_int_distribution<int>(0, base_ - 1),
                    std::mt19937( std::random_device()() )
                ))
            { }

            std::string operator ()()
            {
                struct timespec time;
                std::string s(AUDIOD_UNIQUE_ID_LENGTH, '0');

                clock_gettime(CLOCK_MONOTONIC, &time);

                s[0] = '_'; // Prepend uid with _ to comply with luna requirements
                for (int i = 1; i < AUDIOD_UNIQUE_ID_LENGTH; ++i) {
                    if (i < 5 && i < AUDIOD_UNIQUE_ID_LENGTH - 6) {
                        s[i] = source_[time.tv_nsec % base_];
                        time.tv_nsec /= base_;
                    } else if (time.tv_sec > 0 && i < AUDIOD_UNIQUE_ID_LENGTH - 3) {
                        s[i] = source_[time.tv_sec % base_];
                        time.tv_sec /= base_;
                    } else {
                        s[i] = source_[rand_()];
                    }
                }

                return s;
            }
        };

        static bool _registerTrack(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _unregisterTrack(LSHandle *lshandle, LSMessage *message, void *ctx);

        static bool disconnetedCb( LSHandle *sh, const char *serviceName, bool connected, void *ctx);
};


#endif