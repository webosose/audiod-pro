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

#include "PulseAudioMixer.h"

#define SHORT_DTMF_LENGTH  200
#define phone_MaxVolume 70
#define phone_MinVolume 0
#define FILENAME "/dev/snd/pcmC"
#define DISPLAY_ONE 1
#define DISPLAY_TWO 2
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_CHANNELS 1
#define DEFAULT_SAMPLE_FORMAT "PA_SAMPLE_S16LE"

#define _NAME_STRUCT_OFFSET(struct_type, member) \
                       ((long) ((unsigned char*) &((struct_type*) 0)->member))

const int cMinTimeout = 50;
const int cMaxTimeout = 5000;

PulseAudioMixer::PulseAudioMixer(MixerInterface* mixerCallBack) : mChannel(0),
                                     mTimeout(cMinTimeout),
                                     mSourceID(-1),
                                     mConnectAttempt(0),
                                     mCurrentDtmf(NULL),
                                     mPulseFilterEnabled(true),
                                     mPulseStateFilter(0),
                                     mPulseStateLatency(0),
                                     mInputStreamsCurrentlyOpenedCount(0),
                                     mOutputStreamsCurrentlyOpenedCount(0),
                                     voLTE(false),
                                     NRECvalue(1),
                                     BTDeviceType(eBTDevice_NarrowBand),
                                     mPreviousVolume(0),
                                     BTvolumeSupport(false),
                                     mObjMixerCallBack(mixerCallBack)
{
    // initialize table for the pulse state lookup table
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer constructor");
    for (int i = eVirtualSink_First; i <= eVirtualSink_Last; i++)
    {
        mPulseStateVolume[i] = -1;
        mPulseStateVolumeHeadset[i] = -1;
        mPulseStateRoute[i] = -1;
        mPulseStateActiveStreamCount[i] = 0;
    }
    for (int i = eVirtualSource_First; i <= eVirtualSource_Last; i++)
    {
        mPulseStateSourceRoute[i] = -1;
    }
    createPulseSocketCommunication();
}

PulseAudioMixer::~PulseAudioMixer()
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer destructor");
}

/*
  current is the current volume from 0 to 65535
  dB_diff is the amount in dB added to the current volume
  returns the desired volume from 0 to 65535
*/
static int _add_dB (int current, int dB_diff)
{
    if (current <= 0 && dB_diff <= 0)
        return 0;
    else if (current >= 65535 && dB_diff >= 0)
        return 65535;

    float dB_current = (((float)(current) - 65535)/65535.0) * 100;
    float desired = 65535 * ((dB_current + (float)(dB_diff)) / 100 + 1);

    if (desired < 0)
        desired = 0;
    else if (desired > 65535)
        desired = 65535;

    int result = (int) desired;

    return result;
}

bool
PulseAudioMixer::suspendAll()
{
    return programSource ('s', eVirtualSink_All, 0);
}

bool
PulseAudioMixer::updateRate(int rate)
{
    return programSource ('x', eVirtualSink_All, rate);
}

bool
PulseAudioMixer::programBalance(int balance)
{

   return programSource ('B', eVirtualSink_All, balance);

}

bool
PulseAudioMixer::setVolume(int display, int volume)
{
    return programSource ('n', display, volume);
}

bool
PulseAudioMixer::setMute(int sink, int mutestatus)
{
    return programSource ('k', sink, mutestatus);
}

bool
PulseAudioMixer::programSource (char cmd, int sink, int value)
{
    if (NULL == mChannel)
        return false;

    EHeadsetState headset = eHeadsetState_None;

    bool    sendCmd = true;
    switch (cmd)
    {
        case 'm':
            value = 0;    // the mute command is equivalent to
                       //  setting volume to 0, but faster. There is no unmute!
        case 'v':

        case 'r':

            if (VERIFY(IsValidVirtualSink((EVirtualAudioSink)sink)))
            {
                sendCmd = (mPulseStateVolume[sink] != value ||
                           mPulseStateVolumeHeadset[sink] != headset);
                mPulseStateVolume[sink] = value;
                mPulseStateVolumeHeadset[sink] = headset;
            }
            break;
        case 'h':
            if (VERIFY(IsValidVirtualSource((EVirtualSource)sink))) {
                sendCmd = true;
            }
            break;
        case 'd':
            if (VERIFY(IsValidVirtualSink((EVirtualAudioSink)sink)))
            {
                if (mPulseStateRoute[sink] != value)
                    mPulseStateRoute[sink] = value;
                else
                    sendCmd = false;
            }
            break;
        case 'e':
            if (VERIFY(IsValidVirtualSource((EVirtualSource)sink)))
            {
                if (mPulseStateSourceRoute[sink] != value)
                    mPulseStateSourceRoute[sink] = value;
                else
                    sendCmd = false;
            }
            break;
        case 'f':
            mPulseStateFilter = value;
            break;
        case 'l':
            mPulseStateLatency = value;
            break;

        default:
            break;
    }

    if (sendCmd)
    {
        char buffer[SIZE_MESG_TO_PULSE];
        // some commands pass a sink value, but don't need really one and
        // should use 0. Because 0 is a valid sink,
        // we don't want them to use it for the call,
        // or the name of that sink to be listed in the trace.
        // So we make the substitution here and here only.
        const char * sinkName = "";
        if (cmd == 'l' || cmd == 's' || cmd == 'x')
        {
            sprintf(buffer, "%c 0 %i %i", cmd, value, headset);// ignore sink
                                                               // value. Put 0 always.
        }
        else if (cmd == 'f')
        {
            if (!mPulseFilterEnabled)
                value = 0;
            sprintf(buffer, "%c 0 %i %i", cmd, value, headset);// ignore sink
                                                               // value. Put 0 always.
        }
        else if (cmd == 'e')
        {
            sprintf(buffer, "%c %i %i %i", cmd, sink, value, headset);
            sinkName = virtualSourceName((EVirtualSource)sink);
        }
        else if (cmd == 'b')
        {
            sprintf(buffer, "%c %i %i %i", cmd, mPulseStateVolume[sink], headset, value);
            sinkName = virtualSinkName((EVirtualAudioSink)sink);
        }
        else if ((cmd == 'n') || (cmd == 'k'))
        {
            sprintf(buffer, "%c %i %i", cmd, sink, value);
        }
        else
        {
            sprintf(buffer, "%c %i %i %i", cmd, sink, value, headset);
            sinkName = virtualSinkName((EVirtualAudioSink)sink);    // sink means something
        }
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: sending message '%s' %s", __FUNCTION__, buffer, sinkName);
        int sockfd = g_io_channel_unix_get_fd (mChannel);
        ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
        if (bytes != SIZE_MESG_TO_PULSE)
        {
            if (bytes >= 0)
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "programSource: only %u bytes sent to Pulse out of %d (%s).", \
                                   bytes, SIZE_MESG_TO_PULSE, strerror(errno));
            else
                PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "programSource: send to Pulse failed: %s", strerror(errno));
        }
    }

    return true;
}

bool PulseAudioMixer::programVolume (EVirtualAudioSink sink, int volume, bool ramp)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "programVolume: sink:%d, volume:%d, ramp%d", sink, volume, ramp);
    return programSource ( (ramp ? 'r' : 'v'), sink, volume);
}

bool PulseAudioMixer::programCallVoiceOrMICVolume (char cmd, int volume)
{
    return programSource ( cmd, eVirtualSink_None, volume);
}

bool PulseAudioMixer::programMute (EVirtualSource source, int mute)
{
    return programSource ('h', source, mute);
}


bool PulseAudioMixer::programDestination (EVirtualAudioSink sink,
                                          EPhysicalSink destination)
{
    return programSource ('d', sink, destination);
}

void PulseAudioMixer::setNREC(bool value)
{

    if (NRECvalue != value) {
        NRECvalue = value;
    }
}


/*Only to suspend the A2DP sink during call*/
bool PulseAudioMixer::suspendSink(int sink)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::suspendSink: a2dp sink = %d", sink);
    return programSource ('s', sink, sink);
}

bool PulseAudioMixer::programLoadBluetooth (const char *address, const char *profile)
{
    char cmd = 'l';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    if (!mPulseLink.checkConnection())
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return ret;
    }
    if (!mChannel)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "programLoadBluetooth sending message");
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %s", cmd, 0, address, profile);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
    {
       PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Error sending msg for BT load(%d)", bytes);
    }
    else
    {
       PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msg send for BT load(%d)", bytes);
       ret = true;
    }

    if (mObjMixerCallBack)
        mObjMixerCallBack->callBackMasterVolumeStatus();
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                     "programLoadBluetooth: mObjMixerCallBack is null");

    return ret;
}

bool PulseAudioMixer::programUnloadBluetooth (const char *profile)
{
    char cmd = 'u';
    char buffer[SIZE_MESG_TO_PULSE];
    bool ret = false;
    memset(buffer, 0, SIZE_MESG_TO_PULSE);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", cmd, 0, profile);

    if (!mPulseLink.checkConnection())
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return ret;
    }
    if (!mChannel)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: sending message '%s'", __FUNCTION__, buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);

    if (bytes != SIZE_MESG_TO_PULSE)
    {
       PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Error sending msg for BT Unload(%d)", bytes);
    }
    else
    {
       PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msg send for BT Unload(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programA2dpSource (const bool & a2dpSource)
{
    bool ret = false;

    if (!mPulseLink.checkConnection())
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "Pulseaudio is not running");
        return ret;
    }
    if (!mChannel)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "There is no socket connection to pulseaudio");
        return ret;
    }

    char cmd = 'o';
    char buffer[SIZE_MESG_TO_PULSE];
    memset(buffer, 0, SIZE_MESG_TO_PULSE);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, a2dpSource);

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "%s: sending message '%s'", __FUNCTION__, buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);

    if (bytes != SIZE_MESG_TO_PULSE)
    {
       PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "Error sending msg for A2DP source(%d)", bytes);
    }
    else
    {
       PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "msg send for A2DP Source(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programHeadsetRoute(int route) {
    char cmd = 'w';
    char buffer[SIZE_MESG_TO_PULSE]="";
    bool ret  = false;

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "check for pulseaudio connection");
    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "programHeadsetState sending message");
    if (0 == route)
        snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, eHeadsetState_None);
    else if (1 == route)
        snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, eHeadsetState_Headset);
    else
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Wrong argument passed to programHeadsetRoute");
        return ret;
    }

    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Error sending msg for headset routing from audiod(%d)", bytes);
    }
    else
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msg sent for headset routing from audiod");
        ret = true;
    }
    return ret;
}

bool PulseAudioMixer::externalSoundcardPathCheck (std::string filename, int status) {
    struct stat buff = {0};

    if (1 == status) {
        if (!(0 == stat (filename.c_str(), &buff)))
            return false;
    }
    else {
        if (0 == stat (filename.c_str(), &buff))
            return false;
    }
    return true;
}

bool PulseAudioMixer::loadUSBSinkSource(char cmd,int cardno, int deviceno, int status)
{
    char buffer[SIZE_MESG_TO_PULSE] = "";
    bool ret  = false;
    std::string card_no = std::to_string(cardno);
    std::string device_no = std::to_string(deviceno);
    std::string filename;

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "check for pulseaudio connection");
    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    switch (cmd) {
        case 'j':
                {
                filename = FILENAME+card_no+"D"+device_no+"c";
                break;
                }
        case 'z':
                {
                filename = FILENAME+card_no+"D"+device_no+"p";
                break;
                }
        default:
                return false;
    }

    ret = externalSoundcardPathCheck(filename, status);
    if (false == ret)
        return ret;

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "loadUSBSinkSource sending message");
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %d", cmd, cardno, deviceno, status);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Error sending msg from loadUSBSinkSource", bytes);
    }
    else
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msg sent from loadUSBSinkSource from audiod", bytes);
        ret = true;
    }

    if (mObjMixerCallBack)
        mObjMixerCallBack->callBackMasterVolumeStatus();
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                     "programLoadBluetooth: mObjMixerCallBack is null");
    return ret;
}

bool PulseAudioMixer::setRouting(const ConstString & scenario){
    char cmd = 'C';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;
    ConstString tail;

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "check for pulseaudio ");
    if (!mPulseLink.checkConnection()) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::setRouting: sceanrio = %s", scenario.c_str());
    if (scenario.hasPrefix(PHONE_, tail)) {
     PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::setRouting: phone device = %s", tail.c_str());
        cmd = 'P';
    } else if (scenario.hasPrefix(MEDIA_, tail)) {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::setRouting: media device = %s", tail.c_str());
        cmd = 'Q';
    }
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %d", cmd, voLTE, tail.c_str(), BTDeviceType);
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::setRouting sending message : %s ", buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Error sending msg for sendMixerState(%d)", bytes);
    }
    else
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msg send for sendMixerState(%d)", bytes);
        ret = true;
    }
    return ret;
}

int PulseAudioMixer::loopback_set_parameters(const char * value)
{
    char cmd = 'T';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "check for pulseaudio ");
    if (!mPulseLink.checkConnection()) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::loopback_set_parameters: value = %s", value);

    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %d", cmd, 0, value, 0);
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::loopback_set_parameters sending message : %s ", buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Error sending msg for loopback_set_parameters(%d)", bytes);
    }
    else
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msg send for loopback_set_parameters(%d)", bytes);
        ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programDestination (EVirtualSource source,
                                          EPhysicalSource destination)
{
    return programSource ('e', source, destination);
}

bool PulseAudioMixer::programFilter (int filterTable)
{
    return programSource ('f', eVirtualSink_None, filterTable);
}

bool PulseAudioMixer::muteAll()
{
    for (EVirtualAudioSink sink = eVirtualSink_First;
         sink <= eVirtualSink_Last;
         sink = EVirtualAudioSink(sink + 1))
    {
        programSource ('m', sink, 0);
    }
    return true;
}

static gboolean
_pulseStatus(GIOChannel *ch, GIOCondition condition, gpointer user_data)
{
    PulseAudioMixer *pulseMixerObj = (PulseAudioMixer*)user_data;
    if (pulseMixerObj)
        pulseMixerObj->_pulseStatus(ch, condition, user_data);
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "_timer: pulseMixerObj is null");
    return TRUE;
}

bool
PulseAudioMixer::_connectSocket ()
{
    GIOCondition condition = GIOCondition(G_IO_ERR | G_IO_HUP | G_IO_IN);

    struct sockaddr_un name;
    memset (&name, 0x00, sizeof(name));

    name.sun_family = AF_UNIX;
    name.sun_path[0] = '\0';

    int length = strlen(PALMAUDIO_SOCK_NAME) + 1;

    if (length > _MAX_NAME_LEN)
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: socket name '%s' is too long (%i > %i)",__FUNCTION__,\
                                  PALMAUDIO_SOCK_NAME, length, _MAX_NAME_LEN);
        return false;
    }

    strncpy (&name.sun_path[1], PALMAUDIO_SOCK_NAME, length);

    mConnectAttempt++;

    int sockfd = -1;

    if (-1 == (sockfd = socket(AF_UNIX, SOCK_STREAM, 0)))
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: socket error '%s' on fd (%i)",__FUNCTION__, strerror(errno), sockfd);
        sockfd = -1;
        return false;
    }

    if (-1 == connect(sockfd, (struct sockaddr *)&name,
                               _NAME_STRUCT_OFFSET (struct sockaddr_un, sun_path)
                                + length))
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: connect error '%s' on fd (%i) ", \
                                       __FUNCTION__, strerror(errno), sockfd);
        if (close(sockfd))
        {
            PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: close error '%s' on fd (%i)",__FUNCTION__,
                                                      strerror(errno), sockfd);
        }
        sockfd = -1;
        return false;
    }

    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: successfully connected to Pulse on attempt #%i",\
                                                              __FUNCTION__,
                                                               mConnectAttempt);
    mConnectAttempt = 0;

    // initialize table for the pulse state lookup table
    for (EVirtualAudioSink sink = eVirtualSink_First;
         sink <= eVirtualSink_Last;
         sink = EVirtualAudioSink(sink + 1))
    {
        mPulseStateVolume[sink] = -1;
        mPulseStateVolumeHeadset[sink] = -1;
        mPulseStateRoute[sink] = -1;
        while (mPulseStateActiveStreamCount[sink] > 0)
            outputStreamClosed(sink);
        mPulseStateActiveStreamCount[sink] = 0;    // shouldn't be necessary
    }

    EVirtualSource source;
    for (source = eVirtualSource_First;
           source <= eVirtualSource_Last;
           source = EVirtualSource(source + 1))
    {
        mPulseStateSourceRoute[source] = -1;
    }
    mActiveStreams.clear();
    mPulseStateFilter = 0;
    mPulseStateLatency = 65536;

    // Reuse the counting logic above rather than just reset the counters,
    // to preserve closing behavior without re-implementing it
    while (mInputStreamsCurrentlyOpenedCount > 0)
        inputStreamClosed(source);
    mInputStreamsCurrentlyOpenedCount = 0;    // shouldn't be necessary
    mOutputStreamsCurrentlyOpenedCount = 0;    // shouldn't be necessary

    // To do? since we are connected setup a watch for data on the file descriptor.
    mChannel = g_io_channel_unix_new(sockfd);

    mSourceID = g_io_add_watch (mChannel, condition, ::_pulseStatus, this);

    // Let audiod know that we now have a connection, so that the mixer can be programmed
    if (mObjMixerCallBack)
        mObjMixerCallBack->callBackMixerStatus(true, utils::ePulseMixer);
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "_connectSocket: mObjMixerCallBack is null");

    // We've just established our direct socket link to Pulse.
    // It's a good time to see if we can connect using the Pulse APIs
    if (!mPulseLink.checkConnection()) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return false;
    }

    return true;
}

static gboolean
_timer (gpointer data)
{
    PulseAudioMixer *pulseMixerObj = (PulseAudioMixer*)data;
    if (pulseMixerObj)
        pulseMixerObj->_timer();
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "_timer: pulseMixerObj is null");
    return FALSE;
}

void PulseAudioMixer::_timer()
{
    if (!_connectSocket())
    {
        g_timeout_add (mTimeout, ::_timer, this);
        mTimeout *= 2;
        if (mTimeout > cMaxTimeout)
            mTimeout = cMinTimeout;
    }
}

void
PulseAudioMixer::openCloseSink (EVirtualAudioSink sink, bool openNotClose)
{
    if (!VERIFY(IsValidVirtualSink(sink)))
        return;

    int & streamCount = mPulseStateActiveStreamCount[sink];

    if (openNotClose)
        ++streamCount;
    else
        --streamCount;

    if (streamCount < 0)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "openCloseSink: adjusting the stream %i-%s count to 0", \
                                                  sink, virtualSinkName(sink));
        streamCount = 0;
    }

    VirtualSinkSet oldstreamflags = mActiveStreams;

    if (0 == streamCount)
        mActiveStreams.remove(sink);
    else
        mActiveStreams.add(sink);

    if (oldstreamflags != mActiveStreams)
    {
        utils::ESINK_STATUS eSinkStatus = openNotClose ? utils::eSinkOpened :
                                           utils::eSinkClosed;
        if (mObjMixerCallBack)
        {
            std::string sourceId = "source";
            std::string sinkId = "sink";
            mObjMixerCallBack->callBackSinkStatus(sourceId, sinkId, sink, eSinkStatus, utils::ePulseMixer);
        }
        else
             PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                 "mObjMixerCallBack is null");
    }
}

#define LOG_LEVEL_SINK(sink) \
  ((sink == eDTMF || sink == efeedback || sink == eeffects) ? \
   G_LOG_LEVEL_INFO : G_LOG_LEVEL_MESSAGE)

void
PulseAudioMixer::_pulseStatus(GIOChannel *ch,
                              GIOCondition condition,
                              gpointer user_data)
{
    if (condition & G_IO_IN)
    {
        char buffer[SIZE_MESG_TO_AUDIOD];

        int sockfd = g_io_channel_unix_get_fd (ch);
        ssize_t bytes = recv(sockfd, buffer, SIZE_MESG_TO_AUDIOD, 0);

        char cmd;
        int isink;
        int info;
        char ip[28];
        int port;

        if (bytes > 0 && EOF != sscanf (buffer, "%c %i %i", &cmd, &isink, &info))
        {
            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::_pulseStatus: Pulse says: '%c %i %i'",\
                                  cmd, isink, info);
            EVirtualAudioSink sink = EVirtualAudioSink(isink);
                EVirtualSource source = EVirtualSource(isink);
            switch (cmd)
            {

              case 'a':
                   PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Got A2DP sink running message from PA");
                   //Will be updated once DAP design is updated
                   break;

               case 'b':
                   PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Got A2DP sink Suspend message from PA");
                   //Will be  updated once DAP design is updated
                   break;

               case 'o':
                    if (VERIFY(IsValidVirtualSink(sink)))
                    {
                        outputStreamOpened (sink);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, \
                        "%s: sink %i-%s opened (stream %i). Volume: %d, Headset: %d, Route: %d, Streams: %d.",
                                __FUNCTION__, sink, virtualSinkName(sink), \
                                info, mPulseStateVolume[sink],\
                                mPulseStateVolumeHeadset[sink], \
                                mPulseStateRoute[sink], \
                                mPulseStateActiveStreamCount[sink]);
                    }
                    break;

                case 'c':
                    if (VERIFY(IsValidVirtualSink(sink)))
                    {
                        outputStreamClosed (sink);

                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, \
                         "%s: sink %i-%s closed (stream %i). Volume: %d, Headset: %d, Route: %d, Streams: %d.", \
                                __FUNCTION__, sink, virtualSinkName(sink),\
                                 info, mPulseStateVolume[sink], \
                                 mPulseStateVolumeHeadset[sink], \
                                 mPulseStateRoute[sink], \
                                 mPulseStateActiveStreamCount[sink]);
                    }
                    break;
                case 'O':
                    if (VERIFY(IsValidVirtualSink(sink)) && VERIFY(info >= 0))
                    {
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: pulse says %i sink%s of type %i-%s %s already opened", \
                                   __FUNCTION__, info, \
                                   ((info > 1) ? "s" : ""), \
                                   sink, virtualSinkName(sink), \
                                   ((info > 1) ? "are" : "is"));
                        while (mPulseStateActiveStreamCount[sink] < info)
                            outputStreamOpened (sink);
                        while (mPulseStateActiveStreamCount[sink] > info)
                            outputStreamClosed (sink);
                    }
                    break;
                case 'I':
                    {
                        if (VERIFY(IsValidVirtualSource(source)) && VERIFY(info >= 0))
                        {
                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: pulse says %i input source%s already opened",\
                                       __FUNCTION__, info, \
                                       ((info > 1) ? "s are" : " is"));
                            while (mInputStreamsCurrentlyOpenedCount < info)
                                inputStreamOpened (source);
                            while (mInputStreamsCurrentlyOpenedCount > info)
                                inputStreamClosed (source);
                        }
                    }
                    break;
                case 'd':
                    inputStreamOpened (source);
                    break;
                case 'k':
                    inputStreamClosed (source);
                    break;
                case 'x':
                    //Will be updated once DAP design is updated
                    break;
                case 'y':
                    //prepare hw for capture
                    break;
                case 'H':
                    //Will be updated once DAP design is updated
                    break;
                case 'R':
                    //Will be updated once DAP design is updated
                    break;
                case 't':
                    if (5 == sscanf (buffer, "%c %i %i %28s %d", &cmd, &isink, &info, ip, &port))
                        //Will be updated once DAP design is updated
                default:
                    break;
            }
       }
    }

    if (condition & G_IO_ERR)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: error condition on the socket to pulse", __FUNCTION__);
    }

    if (condition & G_IO_HUP)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: pulse server gone away", __FUNCTION__);
        g_io_channel_shutdown (ch, FALSE, NULL);

        mTimeout = cMinTimeout;
        g_source_remove (mSourceID);
        g_io_channel_unref(mChannel);
        mChannel = NULL;
        //Will be updated once DAP design is updated
        //gState.setRTPLoaded(false);
        g_timeout_add (0, ::_timer, this);
    }
}

void PulseAudioMixer::outputStreamOpened (EVirtualAudioSink sink)
{
    if (IsValidVirtualSink(sink))
        openCloseSink (sink, true);
    if (mOutputStreamsCurrentlyOpenedCount == 0)
    {
        //Will be removed or updated once DAP design is updated
        //gAudioDevice.setActiveOutput (true);
    }
    mOutputStreamsCurrentlyOpenedCount++;
}

void PulseAudioMixer::outputStreamClosed (EVirtualAudioSink sink)
{
    mOutputStreamsCurrentlyOpenedCount--;
    if (mOutputStreamsCurrentlyOpenedCount <= 0)
    {
        //Will be removed or updated once DAP design is updated
        //gAudioDevice.setActiveOutput (false);
    }

    if (mOutputStreamsCurrentlyOpenedCount < 0)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: stream opened count was less than 0", __FUNCTION__);
        mOutputStreamsCurrentlyOpenedCount = 0;
    }
    if (IsValidVirtualSink(sink))
        openCloseSink (sink, false);
}

void PulseAudioMixer::inputStreamOpened (EVirtualSource source)
{
    //Will be updated once DAP design is updated
}

void PulseAudioMixer::inputStreamClosed (EVirtualSource source)
{
    //Will be updated once DAP design is updated
}

static int IdToDtmf(const char* snd) {
    if ('0'<=snd[0] && snd[0]<='9') {
        return (snd[0]-(int)'0');
    } else if (snd[0]=='*') {
        return Dtmf_Asterisk;
    } else if (snd[0]=='#') {
        return Dtmf_Pound;
    } else {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "unknown dtmf tone");
        return-1;
    }
}

bool PulseAudioMixer::playSystemSound(const char *snd, EVirtualAudioSink sink)
{
    PMTRACE_FUNCTION;
    if (strncmp(snd, "dtmf_", 5) == 0) {
        // support dtmf_x
        // see http://developer.palm.com/index.php?option=
        //com_content&view=article&id=1618&Itemid=20#SystemSounds-playFeedback
        playOneshotDtmf(snd+5, sink);
        return true;
    }
    return mPulseLink.play(snd, sink);
}

bool PulseAudioMixer::playSound(const char *snd, EVirtualAudioSink sink, const char *format, int rate, int channels)
{
    return mPulseLink.play(snd, sink, format, rate, channels);
}

void PulseAudioMixer::preloadSystemSound(const char * snd)
{
    std::string path = SYSTEMSOUNDS_PATH;
    path += snd;
    path += "-ondemand.pcm";

    mPulseLink.preload(snd, DEFAULT_SAMPLE_FORMAT, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS, path.c_str());
}

void PulseAudioMixer::playOneshotDtmf(const char *snd, EVirtualAudioSink sink)
{
    PMTRACE_FUNCTION;
    playOneshotDtmf(snd, virtualSinkName(sink, false));
}

void PulseAudioMixer::playOneshotDtmf(const char *snd, const char* sink)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::playOneshotDtmf");
}

void PulseAudioMixer::playDtmf(const char *snd, EVirtualAudioSink sink)
{
    //Will be updated once DAP design is updated
    playDtmf(snd, virtualSinkName(sink, false));
}

void PulseAudioMixer::playDtmf(const char *snd, const char* sink)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::playDtmf");
    int tone;
    if ((tone=IdToDtmf(snd))<0) return;
    if (mCurrentDtmf && tone==mCurrentDtmf->getTone()) return;
    stopDtmf();
    mCurrentDtmf = new PulseDtmfGenerator((Dtmf)tone, 0);
    //Will be updated once DAP design is updated
    mPulseLink.play(mCurrentDtmf, sink);
}

void PulseAudioMixer::stopDtmf() {
    if (mCurrentDtmf) {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::stopDtmf");
        mCurrentDtmf->stopping();
        mCurrentDtmf->unref();
        mCurrentDtmf = NULL;
    }
}

void PulseAudioMixer::createPulseSocketCommunication()
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::createPulseSocketCommunication");
    g_timeout_add (0, ::_timer, this);
}

#if defined(AUDIOD_TEST_API)
static LSMethod pulseMethods[] = {
    { },
};
#endif

void PulseAudioMixer::init(GMainLoop * loop, LSHandle * handle)
{
#if defined(AUDIOD_TEST_API)
    bool result;
    CLSError lserror;

    result = ServiceRegisterCategory ("/state/pulse", pulseMethods, NULL, loop);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: Registering Service for '%s' category failed", __FUNCTION__, "/state/pulse");
    }
#endif
}
