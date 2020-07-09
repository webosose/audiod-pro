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

#ifndef _AUDIO_POLICY_MGR_H_
#define _AUDIO_POLICY_MGR_H_

#include "utils.h"
#include "messageUtils.h"
#include "main.h"
#include <cstdlib>

class AudioPolicyManager
{
    private:

        AudioPolicyManager(const AudioPolicyManager&) = delete;
        AudioPolicyManager& operator=(const AudioPolicyManager&) = delete;
        AudioPolicyManager();

    public:
        ~AudioPolicyManager();
        static AudioPolicyManager* getAudioPolicyManagerInstance();
        static void loadModuleAudioPolicyManager();
        static AudioPolicyManager *mAudioPolicyManager;
};
#endif //_AUDIO_POLICY_MGR_H_