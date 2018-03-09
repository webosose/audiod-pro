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

#ifndef _VOLUME_H_
#define _VOLUME_H_

const int cVolumePercent_Min = 0;
const int cVolumePercent_Max = 100;

/* sound balnace supports from -1.0 to 1.0, these values will be scaled down */
const int cBalance_Max = 10;
const int cBalance_Min = -10;

int SCVolumeConvertPercentToTic(int percent);

int SCVolumeUp(int volume);
int SCVolumeDown(int volume);

#endif // _VOLUME_H_
