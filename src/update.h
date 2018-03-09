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

#ifndef _UPDATE_H_
#define _UPDATE_H_

#include "scenario.h"

#define UPDATE_CHANGED_VOLUME     (1 << 0)
#define UPDATE_CHANGED_MICGAIN    (1 << 1)
#define UPDATE_CHANGED_SCENARIO   (1 << 2)
#define UPDATE_CHANGED_ACTIVE     (1 << 3)
#define UPDATE_CHANGED_RINGER     (1 << 4)
#define UPDATE_CHANGED_MUTED      (1 << 5)
#define UPDATE_BROADCAST_EVENT    (1 << 6)
#define UPDATE_CHANGED_SLIDER     (1 << 7)
#define UPDATE_CHANGED_HAC        (1 << 8)
#define UPDATE_DISABLED           (1 << 9)
#define CAUSE_VOLUME_UP           (1 << 10)
#define CAUSE_VOLUME_DOWN         (1 << 11)
#define CAUSE_SET_VOLUME          (1 << 12)
#define UPDATE_ENABLED_SCENARIO   (1 << 0)
#define UPDATE_DISABLED_SCENARIO  (1 << 1)
#define NOTIFY_SOUNDOUT           (1 << 13)
#define UPDATE_RINGTONE_WITH_VIBRATION 0x000F

#endif // _UPDATE_H_
