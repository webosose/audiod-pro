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

#ifndef _SOUNDSETTINGS_H_
#define _SOUNDSETTINGS_H_

#include <string>

class soundSettings
{
private:
    std::string soundMode;
    umiaudiomixer *mixerObj;
public:
    static soundSettings * getSoundSettingsInstance();
    static bool _SetSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx);
    static bool _updateSoundOutStatus(LSHandle *sh, LSMessage *reply, void *ctx);
    soundSettings();
    ~soundSettings();
};

#endif // _SOUNDSETTINGS_H_
