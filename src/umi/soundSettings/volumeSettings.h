// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef MASTER_VOLUME_H_
#define MASTER_VOLUME_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include "umiaudiomixer.h"
class volumeSettings
{
    private :

      umiaudiomixer *mixerObj;

      int mVolume;

      bool mMuteStatus;

    public :

      volumeSettings();

      ~volumeSettings();

      static volumeSettings* getVolumeSettingsInstance();

      /*master volume store*/
      void setCurrentVolume(int iVolume);

      /*master volume mute status store*/
      void setCurrentMuteStatus(bool bMuteStatus);

      /*volume settings luna calls start*/
      static bool _setVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
      static bool _getVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
      static bool _setMuted(LSHandle *lshandle, LSMessage *message, void *ctx);
      static bool _volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx);
      static bool _volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx);
      /*volume settings luna calls end*/

      /*volume settings luna call back start*/
      static bool _setVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
      static bool _getVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
      static bool _muteVolumeCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
      static bool _volumeUpCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
      static bool _volumeDownCallBack(LSHandle *sh, LSMessage *reply, void *ctx);
      /*volume settings luna call back end*/
};

#endif
