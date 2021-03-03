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

#ifndef MASTERVOLUME_INTERFACE_H
#define MASTERVOLUME_INTERFACE_H

#include "utils.h"
#include "messageUtils.h"

class MasterVolumeInterface
{
public:
    //constructor
    MasterVolumeInterface(){};
    //destructor
    virtual ~MasterVolumeInterface(){};

    virtual void setVolume(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
    virtual void getVolume(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
    virtual void muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
    virtual void volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
    virtual void volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx) = 0;
};

#endif //MASTERVOLUME_INTERFACE_H