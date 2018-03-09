// Copyright (c) 2012-2018 LG Electronics, Inc.
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

#include "volume.h"
#include "log.h"

#define VOLUME_KEY_STEP 11

// Formula shared with UI team.
#define PERCENT_TO_TIC(percent) int((int(percent) + 6) / VOLUME_KEY_STEP)

int SCVolumeConvertPercentToTic(int percent)
{
    if (percent < cVolumePercent_Min)
        percent = cVolumePercent_Min;
    else if (percent > cVolumePercent_Max)
        percent = cVolumePercent_Max;

    return PERCENT_TO_TIC(percent);
}

int SCVolumeUp(int volume)
{
    int newVolume = volume + VOLUME_KEY_STEP;
    // Snap high values to absolute max
    if (newVolume > cVolumePercent_Max - VOLUME_KEY_STEP)
        newVolume = cVolumePercent_Max;
    int    oldTic = PERCENT_TO_TIC(volume);
    // Ensure we moved at least one tic
    while (PERCENT_TO_TIC(newVolume) == oldTic && newVolume < cVolumePercent_Max)
    {
        ++newVolume;
    }
    // Ensure we moved no more than one tic
    while (PERCENT_TO_TIC(newVolume) > oldTic + 1 &&
                                 newVolume > cVolumePercent_Min)
    {
        --newVolume;
    }
    return newVolume;
}

int SCVolumeDown(int volume)
{
    int newVolume = volume - VOLUME_KEY_STEP;
    // Snap low values to absolute min
    if (newVolume < cVolumePercent_Min + VOLUME_KEY_STEP)
        newVolume = cVolumePercent_Min;
    int    oldTic = PERCENT_TO_TIC(volume);
    // Ensure we moved at least one tic
    while (PERCENT_TO_TIC(newVolume) == oldTic && newVolume > cVolumePercent_Min)
    {
        --newVolume;
    }
    // Ensure we moved no more than one tic
    while (PERCENT_TO_TIC(newVolume) < oldTic - 1 && newVolume < cVolumePercent_Max)
    {
        ++newVolume;
    }
    return newVolume;
}
