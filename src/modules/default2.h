/* @@@LICENSE
*
* Copyright (c) 2019-2020 LG Electronics Company
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

#ifndef _DEFAULT2_H_
#define _DEFAULT2_H_

#include "scenario.h"

class Default2ScenarioModule : public ScenarioModule
{
public:
    Default2ScenarioModule();
    ~Default2ScenarioModule();
    void            setVolume (int volume);
    virtual void    programControlVolume();
    void            programDefault2Volumes(bool ramp);
    void            onSinkChanged (EVirtualSink sink, EControlEvent event, ESinkType p_eSinkType);

private:
    Volume            mDefault2Volume;
    int               default2Volume;
};

Default2ScenarioModule * getDefault2Module();

#endif // _DEFAULT2_H_

