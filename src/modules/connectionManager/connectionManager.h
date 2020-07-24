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

#ifndef _CONNECTION_MANAGER_H_
#define _CONNECTION_MANAGER_H_

#include <map>
#include <string>
#include "audioMixer.h"

#define UMI_CATEGORY_NAME                "/UMI"
#define GET_STATUS_CATEGORY_AND_KEY      "/UMI/getStatus"

class ConnectionManager : public ModuleInterface
{
    private:
        ConnectionManager(const ConnectionManager&) = delete;
        ConnectionManager& operator=(const ConnectionManager&) = delete;
        ConnectionManager();
        ModuleManager* mObjModuleManager;
        AudioMixer* mAudioMixer;

        bool getAudioOutputStatus (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _connectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _disconnectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getStatus (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _mute (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getAudioOutputStatusCallback (LSHandle *sh, LSMessage *reply, void *ctx);
    public:
        ~ConnectionManager();
        static ConnectionManager *mConnectionManager;
        static ConnectionManager *getConnectionManager();
        static LSMethod connectionManagerMethods[];
        static void loadModuleConnectionManager(GMainLoop *loop, LSHandle* handle);
        void notifyGetStatusSubscribers();
};
#endif //_CONNECTION_MANAGER_H_