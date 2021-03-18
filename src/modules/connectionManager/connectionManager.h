/* @@@LICENSE
*
*      Copyright (c) 2020-2021 LG Electronics Company.
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

#ifndef _CONNECTION_MANAGER_H_
#define _CONNECTION_MANAGER_H_

#include <map>
#include <string>
#include "audioMixer.h"
#include "moduleFactory.h"

#define UMI_CATEGORY_NAME                "/UMI"
#define GET_STATUS_CATEGORY_AND_KEY      "/UMI/getStatus"

class ConnectionManager : public ModuleInterface
{
    private:
        ConnectionManager(const ConnectionManager&) = delete;
        ConnectionManager& operator=(const ConnectionManager&) = delete;
        ConnectionManager(ModuleConfig* const pConfObj);
        AudioMixer* mAudioMixer;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_connection_manager", &ConnectionManager::CreateObject));
        }

        bool getAudioOutputStatus (LSHandle *lshandle, LSMessage *message, void *ctx);
        bool connect(LSHandle *lshandle, LSMessage *message, void *ctx);
        bool disconnect(LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _connectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _disconnectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getStatus (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _mute (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _connectStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _disConnectStatusCallback(LSHandle *sh, LSMessage *reply, void *ctx);
        static bool _getAudioOutputStatusCallback (LSHandle *sh, LSMessage *reply, void *ctx);
    public:
        ~ConnectionManager();
        static ConnectionManager *mConnectionManager;
        static ConnectionManager *getConnectionManager();
        static LSMethod connectionManagerMethods[];
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the ConnectionManager handler");
                mConnectionManager = new(std::nothrow) ConnectionManager(pConfObj);
                if (mConnectionManager)
                    return mConnectionManager;
            }
            return nullptr;
        }
        void initialize();
        void deInitialize();
        void handleEvent(events::EVENTS_T* ev);
        void notifyGetStatusSubscribers();
};
#endif //_CONNECTION_MANAGER_H_