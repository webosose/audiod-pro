// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#ifndef _UMI_DISPATCHER_H_
#define _UMI_DISPATCHER_H_

#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include "main.h"
#include <string>
#include <lunaservice.h>
#include "utils.h"
#include "messageUtils.h"
#include "audioMixer.h"

#define UMI_CATEGORY_NAME                "/UMI"
#define GET_STATUS_CATEGORY_AND_KEY      "/UMI/getStatus"

class Dispatcher
{
    private :
        //Will be removed or updated once DAP design is updated
        //typedef  std::map< std::string, UMIScenarioModule * > DispatchstreamMap;
        //DispatchstreamMap dispatcher_map;
        //UMIScenarioModule * getModule (std::string &name);
        bool getAudioOutputStatus (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _connectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _disconnectAudioOut (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getStatus (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _mute (LSHandle *lshandle, LSMessage *message, void *ctx);
        static bool _getAudioOutputStatusCallback (LSHandle *sh, LSMessage *reply, void *ctx);
    public :
        static LSMethod DispatcherMethods[];
        //bool registerModule (std::string name, UMIScenarioModule *obj);
        static Dispatcher * getDispatcher ();
        void notifyGetStatusSubscribers ();
        ~Dispatcher ();
};
#endif //_UMI_DISPATCHER_H_
