// Copyright (c) 2021 LG Electronics, Inc.
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

#ifndef DEVICEMANAGER_INTERFACE_H
#define DEVICEMANAGER_INTERFACE_H

class DeviceManagerInterface;

typedef DeviceManagerInterface* (*pFuncCreateClient)();

class DeviceManagerInterface {

public:

    static DeviceManagerInterface *mClientInstance;
    static pFuncCreateClient mClientFuncPointer;
    DeviceManagerInterface(){};
    virtual ~DeviceManagerInterface() = default;

    static bool Register(pFuncCreateClient clientFunc)
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManagerInterface: Register");
        mClientFuncPointer = clientFunc;
        if (mClientFuncPointer)
            return true;
        return false;
    }

    static DeviceManagerInterface* getClientInstance()
    {
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManagerInterface: getClientInstance");
        mClientInstance = mClientFuncPointer();
        return mClientInstance;
    }

    virtual bool event(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
};

#endif //DEVICEMANAGER_INTERFACE_H
