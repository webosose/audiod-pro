// Copyright (c) 2012-2020 LG Electronics, Inc.
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

#ifndef _MODULE_H_
#define _MODULE_H_

#include <lunaservice.h>

#define AUDIOD_ERRORCODE_PARAMETER_BE_EMPTY 1
#define AUDIOD_ERRORCODE_SCENARIO_NOT_FOUND 2
#define AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE 3
#define AUDIOD_ERRORCODE_FAIL_TO_SET_PARAM 4
#define AUDIOD_ERRORCODE_FAIL_TO_GET_PARAM 5
#define AUDIOD_ERRORCODE_FAIL_TO_SET_LATENCY 6
#define AUDIOD_ERRORCODE_FAIL_TO_GET_LATENCY 7
#define AUDIOD_ERRORCODE_CANT_ENABLE_SCENARIO 8
#define AUDIOD_ERRORCODE_CANT_DISABLE_SCENARIO 9
#define AUDIOD_ERRORCODE_CANT_SET_CURRENT_SCENARIO 10
#define AUDIOD_ERRORCODE_CANT_BROADCAST_EVENT 11
#define AUDIOD_ERRORCODE_NOT_SUPPORT_MUTE 12
#define AUDIOD_ERRORCODE_CANT_LOCK_VOLUME_KEY 13
#define AUDIOD_ERRORCODE_FAIL_TO_SUBSCRIBE_LOCKVOLUMEKEY 14
#define AUDIOD_ERRORCODE_FAIL_TO_CREATE_ENVELOPE 15
#define AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE 16
#define AUDIOD_ERRORCODE_FAILED_MIXER_CALL 17
#define AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE 18
#define AUDIOD_ERRORCODE_INVALID_AUDIO_TYPE 19
#define AUDIOD_ERRORCODE_INVALID_SOUNDOUT 204

typedef const char *TModuleMethod;

const TModuleMethod cModuleMethod_VolumeUp = "volumeUp";
const TModuleMethod cModuleMethod_VolumeDown = "volumeDown";
const TModuleMethod cModuleMethod_SetVolume = "setVolume";
const TModuleMethod cModuleMethod_GetVolume = "getVolume";
const TModuleMethod cModuleMethod_OffsetVolume = "offsetVolume";
const TModuleMethod cModuleMethod_SetMicGain = "setMicGain";
const TModuleMethod cModuleMethod_GetMicGain = "getMicGain";
const TModuleMethod cModuleMethod_OffsetMicGain = "offsetMicGain";
const TModuleMethod cModuleMethod_SetLatency = "setLatency";
const TModuleMethod cModuleMethod_GetLatency = "getLatency";
const TModuleMethod cModuleMethod_EnableScenario = "enableScenario";
const TModuleMethod cModuleMethod_DisableScenario = "disableScenario";
const TModuleMethod cModuleMethod_SetCurrentScenario = "setCurrentScenario";
const TModuleMethod cModuleMethod_GetCurrentScenario = "getCurrentScenario";
const TModuleMethod cModuleMethod_ListScenarios = "listScenarios";
const TModuleMethod cModuleMethod_Status = "status";
const TModuleMethod cModuleMethod_BroadcastEvent = "broadcastEvent";
const TModuleMethod cModuleMethod_LockVolumeKeys = "lockVolumeKeys";
const TModuleMethod cModuleMethod_Play = "play";
const TModuleMethod cModuleMethod_muteVolume = "muteVolume";
const TModuleMethod cModuleMethod_SetMuted = "setMuted";
const TModuleMethod cModuleMethod_Control = "control";
const TModuleMethod cModuleMethod_MuseSet = "museSet";
const TModuleMethod cModuleMethod_HacSet = "hacSet";
const TModuleMethod cModuleMethod_HacGet = "hacGet";
const TModuleMethod cModuleMethod_CallStatusUpdate = "CallStatusUpdate";
const TModuleMethod cModuleMethod_BluetoothAudioPropertiesSet = "bluetoothAudioPropertiesSet";
const TModuleMethod cModuleMethod_SetAlarmOn = "setAlarmOn";
const TModuleMethod cModuleMethod_SetTimerOn = "setTimerOn";
const TModuleMethod cModuleMethod_GetAlarmOn = "getAlarmOn";
const TModuleMethod cModuleMethod_GetTimerOn = "getTimerOn";
const TModuleMethod cModuleMethod_SetRingtoneWithVibration = "setRingtonewithVibration";
const TModuleMethod cModuleMethod_GetRingtoneWithVibration = "getRingtonewithVibration";
const TModuleMethod cModuleMethod_GetSoundOut = "getSoundOut";
bool _setVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _getVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _offsetVolume(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _volumeUp(LSHandle *sh, LSMessage *message, void *ctx);
bool _volumeDown(LSHandle *sh, LSMessage *message, void *ctx);

bool _setMicGain(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _getMicGain(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _offsetMicGain(LSHandle *lshandle, LSMessage *message, void *ctx);

bool _setLatency(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _getLatency(LSHandle *lshandle, LSMessage *message, void *ctx);

bool _enableScenario(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _disableScenario(LSHandle *lshandle, LSMessage *message, void *ctx);

bool _setCurrentScenario(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _getCurrentScenario(LSHandle *lshandle, LSMessage *message, void *ctx);

bool _listScenarios(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _status(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _broadcastEvent(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _setMuted(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _lockVolumeKeys(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _museSet(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _hacSet(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _hacGet(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _callStatusUpdate(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _bluetoothAudioPropertiesSet(LSHandle *lshandle, LSMessage *message, void *ctx);
bool _getSoundOut(LSHandle *lshandle, LSMessage *message, void *ctx);

#endif // _MODULE_H_
