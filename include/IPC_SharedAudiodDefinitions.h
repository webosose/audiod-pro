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

#ifndef IPC_SHAREDAUDIODDEFINITIONS_H_
#define IPC_SHAREDAUDIODDEFINITIONS_H_

enum EPhoneStatus
{
    ePhoneStatus_Disconnected,
    ePhoneStatus_Dialing,
    ePhoneStatus_Incoming,
    ePhoneStatus_Connected,

    ePhoneStatus_First = ePhoneStatus_Disconnected,
    ePhonestatus_Last = ePhoneStatus_Connected
};

enum EHeadsetState
{
    eHeadsetState_None = 0,
    eHeadsetState_Headset,
    eHeadsetState_HeadsetMic,
    eHeadsetState_UsbMic_Connected,
    eHeadsetState_UsbMic_DisConnected,
    eHeadsetState_UsbHeadset_Connected,
    eHeadsetState_UsbHeadset_DisConnected,

    eHeadsetState_First = eHeadsetState_None,
    eHeadsetState_Last = eHeadsetState_UsbHeadset_DisConnected
};

enum EMediaServerCommand
{
    eMediaServerCommand_Null, // No command (for init purposes).
    eMediaServerCommand_PauseAllMedia,// Pause all media. Will return an
                                      // eAudiodCommand_UnmuteMedia if
                                      // any media can't be paused.
    eMediaServerCommand_PauseAllMediaSaved, // Pause all media temporarily
    eMediaServerCommand_ResumeAllMediaSaved,// Resume all media paused in last
                                            // eMediaServerCommand_PauseAllMediaSaved
                                            // command

    eMediaServerCommand_First = eMediaServerCommand_Null,
    eMediaServerCommand_Last = eMediaServerCommand_ResumeAllMediaSaved
};

enum EAudiodCommand
{
    eAudiodCommand_Null, // No command (for init purposes).
    eAudiodCommand_UnmuteMedia, // Unmute media that's playing,
                               // for instance, after headset removal.

    eAudiodCommand_First = eAudiodCommand_Null,
    eAudiodCommand_Last = eAudiodCommand_UnmuteMedia
};

// Names of the modules
#define MEDIA_                     "media_"
#define PHONE_                     "phone_"
#define VOICE_COMMAND_            "voice_command_"
#define VVM_                    "vvm_"


// Names of the sub-scenario
#define SCENARIO_BACK_SPEAKER    "back_speaker"
#define SCENARIO_FRONT_SPEAKER    "front_speaker"
#define SCENARIO_HEADSET        "headset"
#define SCENARIO_HEADSET_MIC    "headset_mic"
#define SCENARIO_A2DP            "a2dp"
#define SCENARIO_WIRELESS        "wireless"
#define SCENARIO_BLUETOOTH_SCO    "bluetooth_sco"
#define SCENARIO_TTY_FULL        "tty_full"
#define SCENARIO_TTY_HCO        "tty_hco"
#define SCENARIO_TTY_VCO        "tty_vco"

// Microphone names
#define MIC_FRONT                "mic_front"
#define MIC_HEADSET_MIC            "mic_headset_mic"
#define MIC_BLUETOOTH_SCO        "mic_bluetooth_sco"

// Some module are really volume place holders
#define SCENARIO_DEFAULT        "_default"

// Some module are really volume place holders
#define MEDIA_SCENARIO_DEFAULT        "media__default"
#define MEDIA_SCENARIO_BACK_SPEAKER    "media_back_speaker"
#define MEDIA_SCENARIO_FRONT_SPEAKER   "media_front_speaker"
#define MEDIA_SCENARIO_HEADSET         "media_headset"
#define MEDIA_SCENARIO_HEADSET_MIC     "media_headset_mic"
#define MEDIA_SCENARIO_A2DP            "media_a2dp"
#define MEDIA_SCENARIO_WIRELESS        "media_wireless"
#define MEDIA_MIC_FRONT                "media_mic_front"
#define MEDIA_MIC_HEADSET_MIC          "media_mic_headset_mic"

#define PHONE_SCENARIO_BACK_SPEAKER    "phone_back_speaker"
#define PHONE_SCENARIO_FRONT_SPEAKER   "phone_front_speaker"
#define PHONE_SCENARIO_HEADSET         "phone_headset"
#define PHONE_SCENARIO_HEADSET_MIC     "phone_headset_mic"
#define PHONE_SCENARIO_BLUETOOTH_SCO   "phone_bluetooth_sco"
#define PHONE_SCENARIO_TTY_FULL        "phone_tty_full"
#define PHONE_SCENARIO_TTY_HCO         "phone_tty_hco"
#define PHONE_SCENARIO_TTY_VCO         "phone_tty_vco"

#define VVM_SCENARIO_BACK_SPEAKER      "vvm_back_speaker"
#define VVM_SCENARIO_FRONT_SPEAKER     "vvm_front_speaker"
#define VVM_SCENARIO_HEADSET           "vvm_headset"
#define VVM_SCENARIO_HEADSET_MIC       "vvm_headset_mic"
#define VVM_SCENARIO_BLUETOOTH_SCO     "vvm_bluetooth_sco"
#define VVM_MIC_HEADSET_MIC            "vvm_mic_headset_mic"
#define VVM_MIC_BLUETOOTH_SCO          "vvm_mic_bluetooth_sco"

#define VOICE_COMMAND_SCENARIO_BACK_SPEAKER  "voice_command_back_speaker"
#define VOICE_COMMAND_SCENARIO_HEADSET       "voice_command_headset"
#define VOICE_COMMAND_SCENARIO_HEADSET_MIC   "voice_command_headset_mic"
#define VOICE_COMMAND_SCENARIO_BLUETOOTH_SCO "voice_command_bluetooth_sco"
#define VOICE_COMMAND_MIC_HEADSET_MIC        "voice_command_mic_headset_mic"
#define VOICE_COMMAND_MIC_BLUETOOTH_SCO      "voice_command_mic_bluetooth_sco"

// Names of the TV master scenario
#define SCENARIO_TV_SPEAKER         "tv_speaker"
#define SCENARIO_EXT_SPEAKER_OPTICAL        "ext_speaker_optical"
#define SCENARIO_EXT_SPEAKER_LG_OPTICAL     "ext_speaker_lg_optical"
#define SCENARIO_EXT_SPEAKER_ARC        "ext_speaker_arc"
#define SCENARIO_EXT_SPEAKER_BT         "ext_speaker_bt"
#define SCENARIO_LINEOUT            "lineout"
#define SCENARIO_HEADPHONE          "headphone"
#define SCENARIO_EXT_URCU_OSS       "ext_speaker_urcu_oss"  // URCU, OSS
#define SCENARIO_EXT_SPEAKER_BUILTIN_LG_OPTICAL     "ext_speaker_builtin_lg_optical"
#define SCENARIO_WIRED_SPEAKERBAR     "wired_speakerbar"
#define SCENARIO_WIRELESS_SPEAKERBAR  "wireless_speakerbar"

#define MASTERVOLUME_           "mastervolume_"

#endif /* IPC_SHAREDAUDIODDEFINITIONS_H_ */
