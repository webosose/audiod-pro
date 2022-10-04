// Copyright (c) 2012-2022 LG Electronics, Inc.
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
#define DYNAMIC_ROUTING    0

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
                                     mEffectSpeechEnhancementEnabled(false),
                                     mObjMixerCallBack(mixerCallBack)
{
    // initialize table for the pulse state lookup table
    PM_LOG_DEBUG("PulseAudioMixer constructor");
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
        mPulseStateActiveSourceCount[i] = 0;
    }
    createPulseSocketCommunication();
}

PulseAudioMixer::~PulseAudioMixer()
{
    PM_LOG_DEBUG("PulseAudioMixer destructor");
}

paudiodMsgHdr PulseAudioMixer::addAudioMsgHeader(uint8_t msgType, uint8_t msgID)
{
    struct paudiodMsgHdr audioMsgHdr;
    audioMsgHdr.msgType = msgType;
    audioMsgHdr.msgTmp = 0x01;
    audioMsgHdr.msgVer = 1;
    audioMsgHdr.msgLen = sizeof(paudiodMsgHdr);
    audioMsgHdr.msgID = msgID;
    return audioMsgHdr;
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
PulseAudioMixer::suspendAll() // to be removed
{
    return programSource ('s', eVirtualSink_All, 0);
}

bool
PulseAudioMixer::updateRate(int rate) // to be removed
{
    return programSource ('x', eVirtualSink_All, rate);
}

bool
PulseAudioMixer::setMute(const char* deviceName, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setMute:deviceName:%s, mutestatus:%d", deviceName, mutestatus);
    /*char cmd = 'k';
    bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", cmd, mutestatus, deviceName);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, esink_set_master_mute_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(esink_set_master_mute_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SINK_MUTE;
    volumeSet.id = 0;
    volumeSet.volume = 0;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = mutestatus;
    volumeSet.parm1 = 0;
    volumeSet.parm2 = 0;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, deviceName, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::setPhysicalSourceMute(const char* source, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    /*char msgToBuf[SIZE_MESG_TO_PULSE];
    snprintf(msgToBuf, SIZE_MESG_TO_PULSE, "%c %s %d", '5', source, mutestatus);
    return msgToPulse(msgToBuf, __FUNCTION__);*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, esource_set_master_mute_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(esource_set_master_mute_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SOURCE_MUTE;
    volumeSet.id = 0;
    volumeSet.volume = 0;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = mutestatus;
    volumeSet.parm1 = 0;
    volumeSet.parm2 = 0;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, source, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool
PulseAudioMixer::setVirtualSourceMute(int sink, int mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    //return programSource ('h', sink, mutestatus);

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, evirtual_source_set_mute_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(evirtual_source_set_mute_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SOURCEOUTPUT_MUTE;
    volumeSet.id = sink;
    volumeSet.volume = 0;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = mutestatus;
    volumeSet.parm1 = 0;
    volumeSet.parm2 = 0;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    volumeSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool
PulseAudioMixer::muteSink(const int& sink, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    //return programSource ('m', sink, mutestatus);

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, evirtual_sink_input_set_mute_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(evirtual_sink_input_set_mute_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SINKINPUT_MUTE;
    volumeSet.id = sink;
    volumeSet.volume = 0;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = mutestatus;
    volumeSet.parm1 = 0;
    volumeSet.parm2 = 0;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    volumeSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool
PulseAudioMixer::setVolume(const char* deviceName, const int& volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setVolume:deviceName:%s, volume:%d", deviceName, volume);
    /*char cmd = 'n';
    bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", cmd, volume, deviceName);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, esink_set_master_volume_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(esink_set_master_volume_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SINK_VOLUME;
    volumeSet.id = 0;
    volumeSet.volume = volume;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = 0;
    volumeSet.parm1 = 0;
    volumeSet.parm2 = 0;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, deviceName, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::sendHeaderToPA(char *data, paudiodMsgHdr audioMsgHdr)
{
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, data, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);

    if (bytes != SIZE_MESG_TO_PULSE)
    {
        if (bytes >= 0)
            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sendHeaderToPA: only %u bytes sent to Pulse out of %d (%s).", \
                                   bytes, SIZE_MESG_TO_PULSE, strerror(errno));
        else
            PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sendHeaderToPA: send to Pulse failed: %s", strerror(errno));
    }
    return true;
}

bool
PulseAudioMixer::setMicVolume(const char* deviceName, const int& volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setMicVolume:deviceName:%s, volume:%d", deviceName, volume);
    /*char cmd = '8';
    bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", cmd, volume, deviceName);
    ret = msgToPulse(buffer, __FUNCTION__);*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, esource_set_master_volume_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(esource_set_master_volume_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SOURCE_MIC_VOLUME;
    volumeSet.id = 0;
    volumeSet.volume = volume;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = 0;
    volumeSet.parm1 = 0;
    volumeSet.parm2 = 0;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, deviceName, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
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
               // the mute command is equivalent to
                       //  setting volume to 0, but faster. There is no unmute!
            if (VERIFY(IsValidVirtualSink((EVirtualAudioSink)sink))){
                sendCmd = true;
                }
            break;
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


bool PulseAudioMixer::msgToPulse(const char *buffer, const std::string& fname)
{
    bool returnValue = false;
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "msgToPulse");
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
    {
        PM_LOG_WARNING(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "msgToPulse: Error sending msg from %s:%d", fname.c_str(), (int)bytes);
    }
    else
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "msgToPulse: msg sent from %s of audiod:%d buffer:{%s}", fname.c_str(), (int)bytes, buffer);
       returnValue = true;
    }
    return returnValue;
}


bool PulseAudioMixer::programVolume (EVirtualAudioSink sink, int volume, bool ramp) // to be removed
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "programVolume: sink:%d, volume:%d, ramp%d", (int)sink, volume, ramp);
    return programSource ( (ramp ? 'r' : 'v'), sink, volume);
}

bool PulseAudioMixer::programTrackVolume(EVirtualAudioSink sink, int sinkIndex, int volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb, bool ramp)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "programTrackVolume: sink:%d, sinkIndex:%d volume:%d, ramp%d", (int)sink, sinkIndex, volume, ramp);
    /*bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %d %d", '6', (int)sink, sinkIndex, volume, ramp);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, evirtual_sink_input_index_set_volume_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(evirtual_sink_input_index_set_volume_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SINKINPUT_INDEX;
    volumeSet.id = sink;
    volumeSet.volume = 0;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = 0;
    volumeSet.parm1 = volume;
    volumeSet.parm2 = ramp;
    volumeSet.parm3 = 0;
    volumeSet.index = sinkIndex;
    volumeSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::programVolume (EVirtualSource source, int volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb, bool ramp)
{
    /*bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "programVolume: source:%d, volume:%d", (int)source, volume);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %d", 'f', source, volume, ramp);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_VOLUME, evirtual_source_input_set_volume_reply);

    pulseCallBackInfo pci;
    pci.lshandle = lshandle;
    pci.message=message;
    pci.ctx=ctx;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(evirtual_source_input_set_volume_reply, pci));

    struct paVolumeSet volumeSet;
    volumeSet.Type = PAUDIOD_VOLUME_SOURCEOUTPUT_VOLUME;
    volumeSet.id = source;
    volumeSet.volume = 0;
    volumeSet.table = 0;
    volumeSet.ramp = 0;
    volumeSet.mute = 0;
    volumeSet.parm1 = volume;
    volumeSet.parm2 = ramp;
    volumeSet.parm3 = 0;
    volumeSet.index = 0;
    volumeSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paVolumeSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &volumeSet, sizeof(struct paVolumeSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::programCallVoiceOrMICVolume (char cmd, int volume) // to be removed
{
    return programSource ( cmd, eVirtualSink_None, volume);
}

bool PulseAudioMixer::programMute (EVirtualSource source, int mute) // to be removed
{
    return programSource ('h', source, mute);
}

bool PulseAudioMixer::moveInputDeviceRouting(EVirtualSource source, const char* deviceName) // to be removed
{
    bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "moveInputDeviceRouting: source:%d, deviceName:%s", (int)source, deviceName);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", 'e', (int)source, deviceName);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;
}

bool PulseAudioMixer::setSoundOutputOnRange(EVirtualAudioSink startSink,\
    EVirtualAudioSink endSink, const char* deviceName)
{
    /*bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setSoundOutputOnRange: startsink:%d, endsink:%d, deviceName:%s", (int)startSink, (int)endSink, deviceName);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %s", 'o', (int)startSink, (int)endSink, deviceName);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_sink_outputdevice_on_range_reply);

    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SINKINPUT_RANGE;
    routingSet.startID = startSink;
    routingSet.endID = endSink;
    routingSet.id = 0;
    strncpy(routingSet.device, deviceName, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paRoutingSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &routingSet, sizeof(struct paRoutingSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::setSoundInputOnRange(EVirtualSource startSource,\
            EVirtualSource endSource, const char* deviceName)
{
    /*bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setSoundInputOnRange: startSource:%d, endSource:%d, deviceName:%s", (int)startSource, (int)endSource, deviceName);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %s", 'a', (int)startSource, (int)endSource, deviceName);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_source_inputdevice_on_range_reply);

    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SOURCEOUTPUT_RANGE;
    routingSet.startID = startSource;
    routingSet.endID = endSource;
    routingSet.id = 0;
    strncpy(routingSet.device, deviceName, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paRoutingSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &routingSet, sizeof(struct paRoutingSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::setDefaultSinkRouting(EVirtualAudioSink startSink, EVirtualAudioSink endSink)
{
    /*bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setDefaultSinkRouting: startSink:%d, endSink:%d", (int)startSink, (int)endSink);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d", '2', (int)startSink, (int)endSink);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_default_sink_routing_reply);

    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SINKINPUT_DEFAULT;
    routingSet.startID = startSink;
    routingSet.endID = endSink;
    routingSet.id = 0;
    routingSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paRoutingSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &routingSet, sizeof(struct paRoutingSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::setDefaultSourceRouting(EVirtualSource startSource, EVirtualSource endSource)
{
    /*bool ret = false;
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setDefaultSourceRouting: startSource:%d, endSource:%d", (int)startSource, (int)endSource);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d", '3', (int)startSource, (int)endSource);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_default_source_routing_reply);

    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SOURCEOUTPUT_DEFAULT;
    routingSet.startID = startSource;
    routingSet.endID = endSource;
    routingSet.id = 0;
    routingSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paRoutingSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &routingSet, sizeof(struct paRoutingSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

void PulseAudioMixer::setNREC(bool value)
{

    if (NRECvalue != value) {
        NRECvalue = value;
    }
}

bool PulseAudioMixer::setAudioEffect(int effectId, bool enabled) {
    //char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "setAudioEffect: effectId: %d, enabled:%d", effectId, enabled);

    //  command, effectId, parameter1, ...
    /*snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d", '4', effectId, enabled);
    switch (effectId) {
        case 0:
            mEffectSpeechEnhancementEnabled = enabled;
            break;
        default:
            break;
    }
    return msgToPulse(buffer, __FUNCTION__);*/
    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_SETPARAM, eload_Bluetooth_module_reply);

    struct paParamSet paramSet;
    paramSet.Type = PAUDIOD_MODULE_SPEECH_ENHANCEMENT_LOAD;
    paramSet.ID = 0;
    paramSet.param1 = enabled;
    paramSet.param2 = effectId;
    paramSet.param3 = 0;

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paParamSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &paramSet, sizeof(struct paParamSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}
bool PulseAudioMixer::checkAudioEffectStatus(int effectId) {
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "checkAudioEffectStatus: effectId: %d", effectId);

    //  return value
    switch (effectId) {
        case 0:
            return mEffectSpeechEnhancementEnabled;
        default:
            return false;
    }
}


/*Only to suspend the A2DP sink during call*/
bool PulseAudioMixer::suspendSink(int sink) // to be removed
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::suspendSink: a2dp sink = %d", sink);
    return programSource ('s', sink, sink);
}

bool PulseAudioMixer::programLoadBluetooth (const char *address, const char *profile, const int displayID)
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

    PM_LOG_DEBUG("programLoadBluetooth sending message");
    /*snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %s", cmd, 0, address, profile);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_MODULE, eload_Bluetooth_module_reply);

    struct paModuleSet moduleSet;
    moduleSet.Type = PAUDIOD_MODULE_BLUETOOTH_LOAD;
    moduleSet.id = 0;
    moduleSet.a2dpSource = 0;
    strncpy(moduleSet.address, address, 20);
    strncpy(moduleSet.profile, profile, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paModuleSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &moduleSet, sizeof(struct paModuleSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::programUnloadBluetooth (const char *profile, const int displayID)
{
    char cmd = 'u';
    char buffer[SIZE_MESG_TO_PULSE];
    bool ret = false;
    //memset(buffer, 0, SIZE_MESG_TO_PULSE);
    //snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", cmd, 0, profile);

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

    PM_LOG_DEBUG("%s: sending message '%s'", __FUNCTION__, buffer);
    //ret = msgToPulse(buffer, __FUNCTION__);
    //return ret;

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_MODULE, eunload_BlueTooth_module_reply);

    struct paModuleSet moduleSet;
    moduleSet.Type = PAUDIOD_MODULE_BLUETOOTH_UNLOAD;
    moduleSet.id = 0;
    moduleSet.a2dpSource = 0;
    moduleSet.address[18] = {'\0'};
    moduleSet.profile[5] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paModuleSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &moduleSet, sizeof(struct paModuleSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
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

    /*char cmd = 'O';
    char buffer[SIZE_MESG_TO_PULSE];
    memset(buffer, 0, SIZE_MESG_TO_PULSE);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, a2dpSource);*/

    //PM_LOG_DEBUG("%s: sending message '%s'", __FUNCTION__, buffer);
    /*ret = msgToPulse(buffer, __FUNCTION__);
    return ret;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_MODULE, ea2dpSource_reply);

    struct paModuleSet moduleSet;
    moduleSet.Type = PAUDIOD_MODULE_BLUETOOTH_A2DPSOURCE;
    moduleSet.id = 0;
    moduleSet.a2dpSource = a2dpSource;
    moduleSet.address[18] = {'\0'};
    moduleSet.profile[5] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paModuleSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &moduleSet, sizeof(struct paModuleSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::programHeadsetRoute(EHeadsetState route) { // To be removed
    char cmd = 'w';
    char buffer[SIZE_MESG_TO_PULSE]="";
    bool ret  = false;

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_DEBUG("programHeadsetState sending message");
    if (eHeadsetState_None == route)
        snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, eHeadsetState_None);
    else if (eHeadsetState_Headset == route)
        snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, eHeadsetState_Headset);
    else
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Wrong argument passed to programHeadsetRoute");
        return ret;
    }

    ret = msgToPulse(buffer, __FUNCTION__);
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

bool PulseAudioMixer::loadInternalSoundCard(char cmd, int cardNumber, int deviceNumber, int status, bool isOutput, const char* deviceName)
{
    char buffer[SIZE_MESG_TO_PULSE];
    bool returnValue  = false;
    std::string card_no;
    std::string device_no;
    std::string filename;
    try
    {
        card_no = std::to_string(cardNumber);
        device_no = std::to_string(deviceNumber);
    }
    catch (const std::bad_alloc&)
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "caught exception for to_string converion");
        return returnValue;
    }
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "check for pulseaudio connection");
    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "There is no socket connection to pulseaudio");
        return returnValue;
    }

    switch (isOutput)
    {
        case 0:
        {
            filename = FILENAME+card_no+"D"+device_no+"c";
            break;
        }
        case 1:
        {
            filename = FILENAME+card_no+"D"+device_no+"p";
            break;
        }
        default:
            return returnValue;
    }
    returnValue = externalSoundcardPathCheck(filename, status);
    if (false == returnValue){
        PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "externalSoundcardPathCheck file %s",filename.c_str());
        return returnValue;
    }

    //snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %d %d %s", cmd, cardNumber, deviceNumber, status, isOutput, deviceName);

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "loadInternalSoundCard sending message %s", buffer);

    //returnValue=msgToPulse(buffer, __FUNCTION__);
    //return returnValue;

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_DEVICE, eload_lineout_alsa_sink_reply);

    struct paDeviceSet deviceSet;
    deviceSet.Type = PAUDIOD_DEVICE_LOAD_LINEOUT_ALSA_SINK;
    deviceSet.cardNo = cardNumber;
    deviceSet.deviceNo = deviceNumber;
    deviceSet.isLoad = 0;
    deviceSet.isMmap = 0;
    deviceSet.isTsched = 0;
    deviceSet.bufSize = 0;
    deviceSet.status = status;
    deviceSet.isOutput = isOutput;
    deviceSet.maxDeviceCnt = 0;
    strncpy(deviceSet.device, deviceName, 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paDeviceSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &deviceSet, sizeof(struct paDeviceSet));

    int ret = sendHeaderToPA(data, audioMsgHdr);

    return ret;
}

bool PulseAudioMixer::sendUsbMultipleDeviceInfo(int isOutput, int maxDeviceCount, const std::string &deviceBaseName)
{
    char buffer[SIZE_MESG_TO_PULSE];
    bool returnValue  = false;

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "There is no socket connection to pulseaudio");
        return returnValue;
    }

    //snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %s", 'Z', isOutput, maxDeviceCount, deviceBaseName.c_str());

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "sendUsbMultipleDeviceInfo sending message %s", buffer);

    //returnValue=msgToPulse(buffer, __FUNCTION__);
    //return returnValue;

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_DEVICE, einit_multiple_usb_device_info_reply);

    struct paDeviceSet deviceSet;
    deviceSet.Type = PAUDIOD_DEVICE_LOAD_USB_MULTIPLE_DEVICE;
    deviceSet.cardNo = 0;
    deviceSet.deviceNo = 0;
    deviceSet.isLoad = 0;
    deviceSet.isMmap = 0;
    deviceSet.isTsched = 0;
    deviceSet.bufSize = 0;
    deviceSet.status = 0;
    deviceSet.isOutput = isOutput;
    deviceSet.maxDeviceCnt = maxDeviceCount;
    strncpy(deviceSet.device, deviceBaseName.c_str(), 20);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paDeviceSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &deviceSet, sizeof(struct paDeviceSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::sendInternalDeviceInfo(int isOutput, int maxDeviceCount)
{
    char buffer[SIZE_MESG_TO_PULSE];
    bool returnValue  = false;

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "There is no socket connection to pulseaudio");
        return returnValue;
    }

    //snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d", 'I', isOutput, maxDeviceCount);

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "sendInternalDeviceInfo sending message %s", buffer);

    /*returnValue=msgToPulse(buffer, __FUNCTION__);
    return returnValue;*/

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_DEVICE, einitialise_internal_card_reply);

    struct paDeviceSet deviceSet;
    deviceSet.Type = PAUDIOD_DEVICE_LOAD_INTERNAL_CARD;
    deviceSet.cardNo = 0;
    deviceSet.deviceNo = 0;
    deviceSet.isLoad = 0;
    deviceSet.isMmap = 0;
    deviceSet.isTsched = 0;
    deviceSet.bufSize = 0;
    deviceSet.status = 0;
    deviceSet.isOutput = isOutput;
    deviceSet.maxDeviceCnt = maxDeviceCount;
    deviceSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paDeviceSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &deviceSet, sizeof(struct paDeviceSet));

    int status = sendHeaderToPA(data, audioMsgHdr);

    return status;
}

bool PulseAudioMixer::loadUSBSinkSource(char cmd,int cardno, int deviceno, int status)
{
    PM_LOG_DEBUG("PulseAudioMixer::loadUSBSinkSource");
    char buffer[SIZE_MESG_TO_PULSE] = "";
    bool ret  = false;
    std::string card_no = std::to_string(cardno);
    std::string device_no = std::to_string(deviceno);
    std::string filename;
    struct paDeviceSet deviceSet;

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    switch (cmd) {
        case 'j':
                {
                filename = FILENAME+card_no+"D"+device_no+"c";
                deviceSet.Type = PAUDIOD_DEVICE_LOAD_CAPTURE_SOURCE;
                break;
                }
        case 'z':
                {
                filename = FILENAME+card_no+"D"+device_no+"p";
                deviceSet.Type = PAUDIOD_DEVICE_LOAD_PLAYBACK_SINK;
                break;
                }
        default:
                return false;
    }

    ret = externalSoundcardPathCheck(filename, status);
    if (false == ret)
        return ret;

    PM_LOG_DEBUG("loadUSBSinkSource sending message");
    //snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %d", cmd, cardno, deviceno, status);
    //ret = msgToPulse(buffer, __FUNCTION__);
    if (mObjMixerCallBack)
        mObjMixerCallBack->callBackMasterVolumeStatus();
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                     "programLoadBluetooth: mObjMixerCallBack is null");
    //return ret;

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_DEVICE, edetect_usb_device_reply);

    deviceSet.cardNo = cardno;
    deviceSet.deviceNo = deviceno;
    deviceSet.isLoad = 0;
    deviceSet.isMmap = 0;
    deviceSet.isTsched = 0;
    deviceSet.bufSize = 0;
    deviceSet.status = status;
    deviceSet.isOutput = 0;
    deviceSet.maxDeviceCnt = 0;
    deviceSet.device[20] = {'\0'};

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paDeviceSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &deviceSet, sizeof(struct paDeviceSet));

    ret = sendHeaderToPA(data, audioMsgHdr);

    return ret;
}

bool PulseAudioMixer::setRouting(const ConstString & scenario){ // To be removed
    char cmd = 'C';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;
    ConstString tail;

    if (!mPulseLink.checkConnection()) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_DEBUG("PulseAudioMixer::setRouting: sceanrio = %s", scenario.c_str());
    if (scenario.hasPrefix(PHONE_, tail)) {
     PM_LOG_DEBUG("PulseAudioMixer::setRouting: phone device = %s", tail.c_str());
        cmd = 'P';
    } else if (scenario.hasPrefix(MEDIA_, tail)) {
        PM_LOG_DEBUG("PulseAudioMixer::setRouting: media device = %s", tail.c_str());
        cmd = 'Q';
    }
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %d", cmd, voLTE, tail.c_str(), BTDeviceType);
    PM_LOG_DEBUG("PulseAudioMixer::setRouting sending message : %s ", buffer);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;
}

int PulseAudioMixer::loopback_set_parameters(const char * value) // To be removed
{
    char cmd = 'T';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    if (!mPulseLink.checkConnection()) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return ret;
    }

    PM_LOG_DEBUG("PulseAudioMixer::loopback_set_parameters: value = %s", value);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %d", cmd, 0, value, 0);
    PM_LOG_DEBUG("PulseAudioMixer::loopback_set_parameters sending message : %s ", buffer);
    ret = msgToPulse(buffer, __FUNCTION__);
    return ret;
}

bool PulseAudioMixer::programFilter (int filterTable) // To be removed
{
    return programSource ('f', eVirtualSink_None, filterTable);
}

bool PulseAudioMixer::muteAll() // To be removed
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
PulseAudioMixer::_connectSocket()
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
            outputStreamClosed(sink, 0,  "");
        mPulseStateActiveStreamCount[sink] = 0;    // shouldn't be necessary
    }

    EVirtualSource source;
    for (source = eVirtualSource_First;
           source <= eVirtualSource_Last;
           source = EVirtualSource(source + 1))
    {
        mPulseStateSourceRoute[source] = -1;
        mPulseStateActiveSourceCount[source] = 0;
    }
    mActiveStreams.clear();
    mActiveSources.clear();
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

void PulseAudioMixer::openCloseSource(EVirtualSource source, bool openNotClose)
{
    if(!IsValidVirtualSource(source))
        return;

    int& sourceCount = mPulseStateActiveSourceCount[source];

    if (openNotClose)
        ++sourceCount;
    else
        --sourceCount;

    if (sourceCount < 0)
    {
        PM_LOG_WARNING(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "openCloseSource: adjusting the stream %i-%s count to 0", \
            (int)source, virtualSourceName(source));
        sourceCount = 0;
    }

    VirtualSourceSet oldstreamflags = mActiveSources;

    if (0 == sourceCount)
        mActiveSources.remove(source);
    else
        mActiveSources.add(source);
    if (oldstreamflags != mActiveSources)
    {
        utils::ESINK_STATUS eSourceStatus = openNotClose ? utils::eSinkOpened :
                                           utils::eSinkClosed;
        if (mObjMixerCallBack)
        {
            std::string sourceId = "source";
            std::string sinkId = "sink";
            mObjMixerCallBack->callBackSourceStatus(sourceId, sinkId, source, eSourceStatus, utils::ePulseMixer);
        }
        else
             PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                 "mObjMixerCallBack is null");
    }
}

void
PulseAudioMixer::openCloseSink (EVirtualAudioSink sink, bool openNotClose, int sinkIndex, std::string trackId)
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
                                                  (int)sink, virtualSinkName(sink));
        streamCount = 0;
    }

    VirtualSinkSet oldstreamflags = mActiveStreams;

    if (0 == streamCount)
        mActiveStreams.remove(sink);
    else
        mActiveStreams.add(sink);

    //check for oldstreamflags != mActiveStreams removed,
    //to get event for allsink open/close events
    utils::ESINK_STATUS eSinkStatus = openNotClose ? utils::eSinkOpened :
                                        utils::eSinkClosed;
    if (mObjMixerCallBack)
    {
        std::string sourceId = "source";
        std::string sinkId = "sink";
        mObjMixerCallBack->callBackSinkStatus(sourceId, sinkId, sink, eSinkStatus, utils::ePulseMixer, sinkIndex, trackId);
    }
    else
        PM_LOG_ERROR(MSGID_AUDIO_MIXER, INIT_KVCOUNT,\
                "mObjMixerCallBack is null");

}

void PulseAudioMixer::deviceConnectionStatus (const std::string &deviceName, const std::string &deviceNameDetail, const bool &connectionStatus)
{
    PM_LOG_DEBUG("deviceName:%s %s status:%d", deviceName.c_str(),deviceNameDetail.c_str(), (int)connectionStatus);
    if (mObjMixerCallBack)
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "notifiying status to audio mixer");
        if (connectionStatus)   //TODO
            mObjMixerCallBack->callBackDeviceConnectionStatus(deviceName, deviceNameDetail, utils::eDeviceConnected, utils::ePulseMixer);
        else
            mObjMixerCallBack->callBackDeviceConnectionStatus(deviceName, deviceNameDetail, utils::eDeviceDisconnected, utils::ePulseMixer);
    }
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "mObjMixerCallBack is null");
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
        char deviceName[SIZE_MESG_TO_AUDIOD];
        char deviceNameDetail[SIZE_MESG_TO_AUDIOD];

        int HdrLen = sizeof(struct paudiodMsgHdr);
        struct paudiodMsgHdr *msgHdr = (struct paudiodMsgHdr*) buffer;
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"len = %d message type=%x, message ID=%x",msgHdr->msgLen,msgHdr->msgType, msgHdr->msgID);

        switch(msgHdr->msgType)
        {
            case PAUDIOD_REPLY_MSGTYPE_POLICY:
            {
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                    "PulseAudioMixer::_pulseStatus: received command PAUDIOD_REPLY_MSGTYPE_POLICY");
                struct paReplyToPolicySet *sndHdr  = (paReplyToPolicySet*)(buffer+HdrLen);
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                    "recieved subcommand %d",sndHdr->Type);
                switch(sndHdr->Type)
                {
                    case PAUDIOD_REPLY_POLICY_SINK_CATEGORY:           //case 'O'
                    {

                        int sinkNumber = sndHdr->stream;
                        int info = sndHdr->count;
                        EVirtualAudioSink sink = EVirtualAudioSink(sinkNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_SINK_CATEGORY:%d,%d",sink,info);
                        if (IsValidVirtualSink(sink) && VERIFY(info >= 0))
                        {
                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "%s: pulse says %i sink%s of type %i-%s %s already opened", \
                                    __FUNCTION__, info, \
                                    ((info > 1) ? "s" : ""), \
                                    (int)sink, virtualSinkName(sink), \
                                    ((info > 1) ? "are" : "is"));
                            while (mPulseStateActiveStreamCount[sink] < info)
                                outputStreamOpened (sink , -1 , "");
                            while (mPulseStateActiveStreamCount[sink] > info)
                                outputStreamClosed (sink , -1, "");
                        }
                    }
                    break;
                    case PAUDIOD_REPLY_POLICY_SOURCE_CATEGORY:         //case 'I'
                    {
                        int sourceNumber = sndHdr->stream;
                        int info = sndHdr->count;
                        EVirtualSource source = EVirtualSource(sourceNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_POLICY_SOURCE_CATEGORY:%d,%d",source,info);
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
                    case PAUDIOD_REPLY_MSGTYPE_SINK_OPEN:         //case 'o'
                    {
                        int sinkNumber = sndHdr->id;
                        int sinkIndex = sndHdr->index;
                        std::string appname(sndHdr->appName);
                        EVirtualAudioSink sink = EVirtualAudioSink(sinkNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_STREAM_OPEN:%d,%d,%s",sink,sinkIndex,appname.c_str());
                        if (VERIFY(IsValidVirtualSink(sink)))
                        {
                            outputStreamOpened (sink , sinkIndex, appname);
                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, \
                            "%s: sink %i-%s opened (stream %i). Volume: %d, Headset: %d, Route: %d, Streams: %d.",
                                    __FUNCTION__, (int)sink, virtualSinkName(sink), \
                                    info, mPulseStateVolume[sink],\
                                    mPulseStateVolumeHeadset[sink], \
                                    mPulseStateRoute[sink], \
                                    mPulseStateActiveStreamCount[sink]);
                        }
                    }
                    break;
                    case PAUDIOD_REPLY_MSGTYPE_SOURCE_OPEN:         //case 'd'
                    {
                        int sourceNumber = sndHdr->id;
                        EVirtualSource source = EVirtualSource(sourceNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_STREAM_OPEN:%d",source);

                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "InputStream opened recieved");
                        inputStreamOpened (source);
                    }
                    break;
                    case PAUDIOD_REPLY_MSGTYPE_SINK_CLOSE:          //case 'c'
                    {
                        int sinkNumber = sndHdr->id;
                        int sinkIndex = sndHdr->index;
                        std::string appname(sndHdr->appName);
                        EVirtualAudioSink sink = EVirtualAudioSink(sinkNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_SINK_CLOSE:%d,%d,%s",sink,sinkIndex,appname.c_str());
                        if (VERIFY(IsValidVirtualSink(sink)))
                        {
                            outputStreamClosed (sink,sinkIndex,appname);

                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, \
                            "%s: sink %i-%s closed (stream %i). Volume: %d, Headset: %d, Route: %d, Streams: %d.", \
                                    __FUNCTION__, (int)sink, virtualSinkName(sink),\
                                    info, mPulseStateVolume[sink], \
                                    mPulseStateVolumeHeadset[sink], \
                                    mPulseStateRoute[sink], \
                                    mPulseStateActiveStreamCount[sink]);
                        }
                    }
                    break;
                    case PAUDIOD_REPLY_MSGTYPE_SOURCE_CLOSE:            //case 'k'
                    {
                        int sourceNum = sndHdr->id;
                        EVirtualSource source = EVirtualSource(sourceNum);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_SOURCE_CLOSE:%d",source);
                        inputStreamClosed (source);
                    }
                    break;
                    default:
                    {
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"Unknown command");
                    }
                    break;
                }
            }
            break;
            case PAUDIOD_REPLY_MSGTYPE_ROUTING:
            {
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                    "PulseAudioMixer::_pulseStatus: received command PAUDIOD_REPLY_MSGTYPE_ROUTING");
                struct paReplyToRoutingSet *sndHdr  = (paReplyToRoutingSet*)(buffer+HdrLen);
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                    "recieved subcommand %d",sndHdr->Type);
                switch (sndHdr->Type)
                {
                    case PAUDIOD_REPLY_MSGTYPE_DEVICE_CONNECTION:       //case '3'
                    {
                        std::string deviceName(sndHdr->device);
                        std::string deviceNameDetail(sndHdr->deviceNameDetail);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                            "PAUDIOD_REPLY_MSGTYPE_DEVICE_CONNECTION : %s:%s",deviceName.c_str(),deviceNameDetail.c_str());
                        deviceConnectionStatus(deviceName, deviceNameDetail, true);
                    }
                    break;
                    case PAUDIOD_REPLY_MSGTYPE_DEVICE_REMOVED:
                    {
                        std::string deviceName(sndHdr->device);
                        std::string deviceNameDetail(sndHdr->deviceNameDetail);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                            "PAUDIOD_REPLY_MSGTYPE_DEVICE_CONNECTION : %s:%s",deviceName.c_str(),deviceNameDetail.c_str());
                        deviceConnectionStatus(deviceName, deviceNameDetail, false);
                    }
                    break;
                    default:
                        break;
                }
            }
            break;
            case PAUDIOD_REPLY_MSGTYPE_CALLBACK:
            {

                paReplyToAudiod *replyHdr = (paReplyToAudiod*)(buffer+HdrLen);
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                    "callback from pulseaudio id :%d", replyHdr->id);
                auto it = mPulseCallBackInfo.find(replyHdr->id);
                if (it != mPulseCallBackInfo.end())
                {
                    pulseCallBackInfo cbk = it->second;
                    PulseCallBackFunc fun = cbk.cb;
                    if (fun)
                        fun(cbk.lshandle, cbk.message, cbk.ctx, true);
                    else
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "CallBAck function is null");
                    mPulseCallBackInfo.erase(replyHdr->id);
                }
                else
                {
                    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "CallBack map cannot find the entry");
                }

            }
            break;
            default:
            {
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                    "invalid command given");
            }
        }

        /*if (bytes > 0 && EOF != sscanf (buffer, "%c %i %i", &cmd, &isink, &info))
        {

            else
            {
                sscanf (buffer, "%c %i %i", &cmd, &isink, &info);
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "PulseAudioMixer::_pulseStatus: Pulse says: '%c %i %i'",\
                                  cmd, isink, info);
                EVirtualAudioSink sink = EVirtualAudioSink(isink);
                    EVirtualSource source = EVirtualSource(isink);
                switch (cmd)
                {

                    case 's':
                    {
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sita s command received");
                        auto it = mPulseCallBackInfo.find(1);
                        if (it != mPulseCallBackInfo.end())
                        {
                            pulseCallBackInfo cbk = it->second;
                            PulseCallBackFunc fun = cbk.cb;
                            if (fun)
                                fun(cbk.lshandle, cbk.message, cbk.ctx, true);
                            else
                                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sita func is null");
                            mPulseCallBackInfo.erase(1);
;                        }
                        else
                        {
                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sita map is end");
                        }

                        break;
                    }
                    case 't':
                        if (5 == sscanf (buffer, "%c %i %i %28s %d", &cmd, &isink, &info, ip, &port))
                            //Will be updated once DAP design is updated
                        break;
                    case '4':
                        int effectId;
                        sscanf(buffer, "%c %d", &cmd, &effectId);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "effect module unloaded Id: %d", effectId);
                        switch (effectId) {
                            case 0:     //  SpeechEnhancement module
                                mEffectSpeechEnhancementEnabled = false;
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
       }*/
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
        g_timeout_add (0, ::_timer, this);
    }
}

void PulseAudioMixer::outputStreamOpened (EVirtualAudioSink sink, int sinkIndex, std::string trackId)
{
    if (IsValidVirtualSink(sink))
        openCloseSink (sink, true, sinkIndex, trackId);
    if (mOutputStreamsCurrentlyOpenedCount == 0)
    {
        //Will be removed or updated once DAP design is updated
        //gAudioDevice.setActiveOutput (true);
    }
    mOutputStreamsCurrentlyOpenedCount++;
}

void PulseAudioMixer::outputStreamClosed (EVirtualAudioSink sink, int sinkIndex, std::string trackId)
{
    mOutputStreamsCurrentlyOpenedCount--;
    if (mOutputStreamsCurrentlyOpenedCount <= 0)
    {
        //Will be removed or updated once DAP design is updated
        //gAudioDevice.setActiveOutput (false);
    }

    if (mOutputStreamsCurrentlyOpenedCount < 0)
    {
        PM_LOG_DEBUG("%s: stream opened count was less than 0", __FUNCTION__);
        mOutputStreamsCurrentlyOpenedCount = 0;
    }
    if (IsValidVirtualSink(sink))
        openCloseSink (sink, false, sinkIndex,  trackId);
}

void PulseAudioMixer::inputStreamOpened (EVirtualSource source)
{
    mInputStreamsCurrentlyOpenedCount++;
    openCloseSource(source, true);
}

void PulseAudioMixer::inputStreamClosed (EVirtualSource source)
{
    mInputStreamsCurrentlyOpenedCount--;
    if (mInputStreamsCurrentlyOpenedCount < 0)
        mInputStreamsCurrentlyOpenedCount = 0;
    openCloseSource(source, false);
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
    PM_LOG_DEBUG("PulseAudioMixer::playDtmf");
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
        PM_LOG_DEBUG("PulseAudioMixer::stopDtmf");
        mCurrentDtmf->stopping();
        mCurrentDtmf->unref();
        mCurrentDtmf = NULL;
    }
}

bool PulseAudioMixer::closeClient(int sinkIndex)
{
    PM_LOG_DEBUG("PulseAudioMixer::closeClient");
    char buffer[SIZE_MESG_TO_PULSE];
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", '7', sinkIndex);
    bool ret = msgToPulse(buffer, __FUNCTION__);
    return ret;

    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(PAUDIOD_MSGTYPE_SETPARAM, eclose_playback_by_sink_input_reply);

    struct paParamSet paramSet;
    paramSet.Type = PAUDIOD_SETPARAM_CLOSE_PLAYBACK;
    paramSet.ID = sinkIndex;
    paramSet.param1 = 0;
    paramSet.param2 = 0;
    paramSet.param3 = 0;

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(struct paParamSet));

    //copying....
    memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
	memcpy(data + sizeof(struct paudiodMsgHdr), &paramSet, sizeof(struct paParamSet));

    //int status = sendHeaderToPA(data, audioMsgHdr);

    //return ret;
}

void PulseAudioMixer::createPulseSocketCommunication()
{
    PM_LOG_DEBUG("PulseAudioMixer::createPulseSocketCommunication");
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
