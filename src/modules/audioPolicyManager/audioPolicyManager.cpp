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

#include "audioPolicyManager.h"

AudioPolicyManager* AudioPolicyManager::mAudioPolicyManager = nullptr;

AudioPolicyManager* AudioPolicyManager::getAudioPolicyManagerInstance()
{
    return mAudioPolicyManager;
}

void AudioPolicyManager::loadModuleAudioPolicyManager()
{
    if (!mAudioPolicyManager)
    {
        mAudioPolicyManager = new (std::nothrow)AudioPolicyManager();
        if (mAudioPolicyManager)
            g_debug("load module AudioPolicyManager successfull");
        else
            g_error("Could not load module AudioPolicyManager");
    }
}

AudioPolicyManager::AudioPolicyManager()
{
    g_debug("AudioPolicyManager: constructor");
}

AudioPolicyManager::~AudioPolicyManager()
{
    g_debug("AudioPolicyManager: destructor");
}

int load_audio_policy_manager(GMainLoop *loop, LSHandle* handle)
{
    g_debug("loading audio_policy_manager");
    AudioPolicyManager::loadModuleAudioPolicyManager();
    return 0;
}

void unload_audio_policy_manager()
{
    g_debug("unloading audio_policy_manager");
    if (AudioPolicyManager::mAudioPolicyManager)
    {
        delete AudioPolicyManager::mAudioPolicyManager;
        AudioPolicyManager::mAudioPolicyManager = nullptr;
    }
}