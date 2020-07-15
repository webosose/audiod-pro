/* @@@LICENSE
*
* Copyright (c) 2020 LG Electronics Company.
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

#ifndef _PLAYBACK_MANAGER_H_
#define _PLAYBACK_MANAGER_H_

#include "moduleInterface.h"
#include "moduleManager.h"
#include "audioMixer.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "main.h"

class PlaybackManager : public ModuleInterface
{
private:
    bool isValidSampleFormat(const std::string& format);
    bool isValidSampleRate(const int& rate);
    bool isValidChannelCount(const int& channels);
    bool isValidFileExtension(const std::string& filePath);

    PlaybackManager(const PlaybackManager&) = delete;
    PlaybackManager& operator=(const PlaybackManager&) = delete;
    PlaybackManager();
    AudioMixer *mObjAudioMixer;

public:
    ~PlaybackManager();
    static LSMethod playbackMethods[];
    static PlaybackManager* getPlaybackManagerInstance();
    static PlaybackManager* mPlaybackManager;
    static void loadPlaybackModuleManager(GMainLoop *loop, LSHandle* handle);
    static bool _playSound(LSHandle *lshandle, LSMessage *message, void *ctx);
};
#endif // _PLAYBACK_MANAGER_
