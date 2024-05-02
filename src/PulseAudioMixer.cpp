// Copyright (c) 2012-2024 LG Electronics, Inc.
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
                                     mEffectGainControlEnabled(false),
                                     mEffectBeamformingEnabled(false),
                                     mEffectDynamicRangeCompressorEnabled(false),
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
    mPulseLink.registerCallback(mixerCallBack);
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

template<typename T>
bool PulseAudioMixer::sendDataToPulse (uint32_t msgType, uint32_t msgID, T subObj)
{
    paudiodMsgHdr audioMsgHdr = addAudioMsgHeader(msgType, msgID);

    char *data=(char*)malloc(sizeof(struct paudiodMsgHdr) + sizeof(T));

    if (data)
    {
        //copying....
        memcpy(data, &audioMsgHdr, sizeof(struct paudiodMsgHdr));
        memcpy(data + sizeof(struct paudiodMsgHdr), &subObj, sizeof(T));

        int sockfd = g_io_channel_unix_get_fd (mChannel);
        ssize_t bytes = send(sockfd, data, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);

        free(data);

        if (bytes != SIZE_MESG_TO_PULSE)
        {
            if (bytes >= 0)
                PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sendDataToPulse: only %u bytes sent to Pulse out of %d (%s).", \
                                       bytes, SIZE_MESG_TO_PULSE, strerror(errno));
            else
                PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "sendHeaderToPA: send to Pulse failed: %s", strerror(errno));
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                "PulseAudioMixer::sendDataToPulse: data handle is NULL");
        return false;
    }
    return true;
}

bool
PulseAudioMixer::setMute(const char* deviceName, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setMute:deviceName:%s, mutestatus:%d", deviceName, mutestatus);

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
    volumeSet.param1 = 0;
    volumeSet.param2 = 0;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, deviceName, DEVICE_NAME_LENGTH);
    volumeSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, esink_set_master_mute_reply, volumeSet);

    return status;
}

bool PulseAudioMixer::setPhysicalSourceMute(const char* source, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
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
    volumeSet.param1 = 0;
    volumeSet.param2 = 0;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, source, DEVICE_NAME_LENGTH);
    volumeSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, esource_set_master_mute_reply, volumeSet);

    return status;
}

bool
PulseAudioMixer::setVirtualSourceMute(int sink, int mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
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
    volumeSet.param1 = 0;
    volumeSet.param2 = 0;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    volumeSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, evirtual_source_set_mute_reply, volumeSet);

    return status;
}

bool
PulseAudioMixer::muteSink(const int& sink, const int& mutestatus, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
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
    volumeSet.param1 = 0;
    volumeSet.param2 = 0;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    volumeSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, evirtual_sink_input_set_mute_reply, volumeSet);

    return status;
}

bool
PulseAudioMixer::setVolume(const char* deviceName, const int& volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "setVolume:deviceName:%s, volume:%d", deviceName, volume);

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
    volumeSet.param1 = 0;
    volumeSet.param2 = 0;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, deviceName, DEVICE_NAME_LENGTH);
    volumeSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, esink_set_master_volume_reply, volumeSet);

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
    volumeSet.param1 = 0;
    volumeSet.param2 = 0;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    strncpy(volumeSet.device, deviceName, DEVICE_NAME_LENGTH);
    volumeSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, esource_set_master_volume_reply, volumeSet);

    return status;
}

bool PulseAudioMixer::programTrackVolume(EVirtualAudioSink sink, int sinkIndex, int volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb, bool ramp)
{
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "programTrackVolume: sink:%d, sinkIndex:%d volume:%d, ramp%d", (int)sink, sinkIndex, volume, ramp);

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
    volumeSet.param1 = volume;
    volumeSet.param2 = ramp;
    volumeSet.param3 = 0;
    volumeSet.index = sinkIndex;
    volumeSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, evirtual_sink_input_index_set_volume_reply, volumeSet);

    return status;
}

bool PulseAudioMixer::programVolume (EVirtualSource source, int volume, LSHandle *lshandle, LSMessage *message, void *ctx, PulseCallBackFunc cb, bool ramp)
{
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
    volumeSet.param1 = volume;
    volumeSet.param2 = ramp;
    volumeSet.param3 = 0;
    volumeSet.index = 0;
    volumeSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    int status = sendDataToPulse<paVolumeSet>(PAUDIOD_MSGTYPE_VOLUME, evirtual_source_input_set_volume_reply, volumeSet);

    return status;
}

bool PulseAudioMixer::setSoundOutputOnRange(EVirtualAudioSink startSink,\
    EVirtualAudioSink endSink, const char* deviceName)
{
    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SINKINPUT_RANGE;
    routingSet.startID = startSink;
    routingSet.endID = endSink;
    routingSet.id = 0;
    strncpy(routingSet.device, deviceName, DEVICE_NAME_LENGTH);
    routingSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    bool status = sendDataToPulse<paRoutingSet>(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_sink_outputdevice_on_range_reply, routingSet);

    return status;
}

bool PulseAudioMixer::setSoundInputOnRange(EVirtualSource startSource,\
            EVirtualSource endSource, const char* deviceName)
{
    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SOURCEOUTPUT_RANGE;
    routingSet.startID = startSource;
    routingSet.endID = endSource;
    routingSet.id = 0;
    strncpy(routingSet.device, deviceName, DEVICE_NAME_LENGTH);
    routingSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    bool status = sendDataToPulse<paRoutingSet>(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_source_inputdevice_on_range_reply, routingSet);

    return status;
}

bool PulseAudioMixer::setDefaultSinkRouting(EVirtualAudioSink startSink, EVirtualAudioSink endSink)
{
    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SINKINPUT_DEFAULT;
    routingSet.startID = startSink;
    routingSet.endID = endSink;
    routingSet.id = 0;
    routingSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    bool status = sendDataToPulse<paRoutingSet>(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_default_sink_routing_reply, routingSet);

    return status;
}

bool PulseAudioMixer::setDefaultSourceRouting(EVirtualSource startSource, EVirtualSource endSource)
{
    struct paRoutingSet routingSet;
    routingSet.Type = PAUDIOD_ROUTING_SOURCEOUTPUT_DEFAULT;
    routingSet.startID = startSource;
    routingSet.endID = endSource;
    routingSet.id = 0;
    routingSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    bool status = sendDataToPulse<paRoutingSet>(PAUDIOD_REPLY_MSGTYPE_ROUTING, eset_default_source_routing_reply, routingSet);

    return status;
}

bool PulseAudioMixer::setAudioEffect(std::string effectName, bool enabled) {
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "setAudioEffect: effectName: %s, enabled:%d", effectName.c_str(), enabled);
    struct paEffectSet effectSet;
    int status = false;

    if (effectName.compare("speech enhancement") == 0)
    {
        mEffectSpeechEnhancementEnabled = enabled;
        effectSet.Type = PAUDIOD_EFFECT_SPEECH_ENHANCEMENT_LOAD;
    }
    else if (effectName.compare("gain control") == 0)
    {
        mEffectGainControlEnabled = enabled;
        effectSet.Type = PAUDIOD_EFFECT_GAIN_CONTROL_LOAD;
    }
    else if (effectName.compare("beamforming") == 0)
    {
        mEffectBeamformingEnabled = enabled;
        effectSet.Type = PAUDIOD_EFFECT_BEAMFORMING_LOAD;
    }
    else if (effectName.compare("dynamic compressor") == 0)
    {
        mEffectDynamicRangeCompressorEnabled = enabled;
        effectSet.Type = PAUDIOD_EFFECT_DYNAMIC_COMPRESSOR_LOAD;
    }
    else if (effectName.compare("equalizer") == 0)
    {
        mEffectEqualizerEnabled = enabled;
        effectSet.Type = PAUDIOD_EFFECT_EQUALIZER_LOAD;
    }
    else if (effectName.compare("bass boost") == 0)
    {
        mEffectBassBoostEnabled = enabled;
        effectSet.Type = PAUDIOD_EFFECT_BASS_BOOST_LOAD;
    }
    else
    {
        return false;
    }

    effectSet.id = 0; //Not used
    effectSet.param[0] = enabled;
    effectSet.param[1] = 0;
    effectSet.param[2] = 0;
    status = sendDataToPulse<paEffectSet>(PAUDIOD_MSGTYPE_EFFECT, eparse_effect_message_reply, effectSet);

    return status;
}

//  socket to module-palm-policy
bool PulseAudioMixer::setAudioEqualizerParam(int preset, int band, int level) {
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "setAudioEqualizerParam: preset[%d] band[%d] level[%d]", preset, band, level);
    struct paEffectSet effectSet;
    int status = false;

    effectSet.Type = PAUDIOD_EFFECT_EQUALIZER_SETPARAM;
    effectSet.id = 0;
    effectSet.param[0] = preset;
    effectSet.param[1] = band;
    effectSet.param[2] = level;
    status = sendDataToPulse<paEffectSet>(PAUDIOD_MSGTYPE_EFFECT, eparse_effect_message_reply, effectSet);

    return status;
}

bool PulseAudioMixer::checkAudioEffectStatus(std::string effectName) {
    char buffer[SIZE_MESG_TO_PULSE];
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "checkAudioEffectStatus: effectId: %s", effectName.c_str());

    //  return value
    if (effectName.compare("speech enhancement") == 0)
        return mEffectSpeechEnhancementEnabled;
    else if (effectName.compare("gain control") == 0)
        return mEffectGainControlEnabled;
    else if (effectName.compare("beamforming") == 0)
        return mEffectBeamformingEnabled;
    else if (effectName.compare("dynamic compressor") == 0)
        return mEffectDynamicRangeCompressorEnabled;
    else if (effectName.compare("equalizer") == 0)
        return mEffectEqualizerEnabled;
    else if (effectName.compare("bass boost") == 0)
        return mEffectBassBoostEnabled;
    else
        return false;
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

    struct paModuleSet moduleSet;
    moduleSet.Type = PAUDIOD_MODULE_BLUETOOTH_LOAD;
    moduleSet.id = 0;
    moduleSet.a2dpSource = 0;
    moduleSet.info=0;
    moduleSet.port=0;
    moduleSet.ip[27] = {'\0'};
    moduleSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};
    strncpy(moduleSet.address, address, BLUETOOTH_MAC_ADDRESS_SIZE);
    moduleSet.address[BLUETOOTH_MAC_ADDRESS_SIZE-1] = '\0';
    strncpy(moduleSet.profile, profile, BLUETOOTH_PROFILE_SIZE);
    moduleSet.profile[BLUETOOTH_PROFILE_SIZE-1]='\0';

    int status = sendDataToPulse<paModuleSet>(PAUDIOD_MSGTYPE_MODULE, eload_Bluetooth_module_reply, moduleSet);

    return status;
}

bool PulseAudioMixer::programUnloadBluetooth (const char *profile, const int displayID)
{
    int status = false;
    if (!mPulseLink.checkConnection())
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Pulseaudio is not running");
        return status;
    }
    if (!mChannel)
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "There is no socket connection to pulseaudio");
        return status;
    }

    PM_LOG_DEBUG("%s: sending message", __FUNCTION__);

    struct paModuleSet moduleSet;
    moduleSet.Type = PAUDIOD_MODULE_BLUETOOTH_UNLOAD;
    moduleSet.id = 0;
    moduleSet.a2dpSource = 0;
    moduleSet.info=0;
    moduleSet.port=0;
    moduleSet.ip[27] = {'\0'};
    moduleSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};
    moduleSet.address[BLUETOOTH_MAC_ADDRESS_SIZE-1] = {'\0'};
    moduleSet.profile[BLUETOOTH_PROFILE_SIZE-1] = {'\0'};

    status = sendDataToPulse<paModuleSet>(PAUDIOD_MSGTYPE_MODULE, eunload_BlueTooth_module_reply, moduleSet);

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

    struct paModuleSet moduleSet;
    moduleSet.Type = PAUDIOD_MODULE_BLUETOOTH_A2DPSOURCE;
    moduleSet.id = 0;
    moduleSet.a2dpSource = a2dpSource;
    moduleSet.info=0;
    moduleSet.port=0;
    moduleSet.ip[27] = {'\0'};
    moduleSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};
    moduleSet.address[BLUETOOTH_MAC_ADDRESS_SIZE-1] = {'\0'};
    moduleSet.profile[BLUETOOTH_PROFILE_SIZE-1] = {'\0'};

    int status = sendDataToPulse<paModuleSet>(PAUDIOD_MSGTYPE_MODULE, ea2dpSource_reply, moduleSet);

    return status;
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

bool PulseAudioMixer::loadInternalSoundCard(char cmd, int cardNumber, int deviceNumber, int status, bool isOutput, const char* deviceName, PulseCallBackFunc cb)
{
    char buffer[SIZE_MESG_TO_PULSE];
    bool returnValue  = false;
    std::string card_no;
    std::string device_no;
    std::string filename;
    card_no = std::to_string(cardNumber);

    if (card_no.empty())
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Failed to convert card number to string");
        return returnValue;
    }

    device_no = std::to_string(deviceNumber);
    if (device_no.empty())
    {
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "Failed to convert device number to string");
        return returnValue;
    }
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"check for pulseaudio connection");
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

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "loadInternalSoundCard sending message %s", buffer);


    pulseCallBackInfo pci;
    pci.lshandle = nullptr;
    pci.message = nullptr;
    pci.ctx = nullptr;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(eload_lineout_alsa_sink_reply, pci));

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
    strncpy(deviceSet.device, deviceName, DEVICE_NAME_LENGTH);
    deviceSet.device[DEVICE_NAME_LENGTH-1] = '\0';

    returnValue = sendDataToPulse<paDeviceSet>(PAUDIOD_MSGTYPE_DEVICE, eload_lineout_alsa_sink_reply, deviceSet);

    return returnValue;
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

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "sendUsbMultipleDeviceInfo sending message %s", buffer);

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
    strncpy(deviceSet.device, deviceBaseName.c_str(), DEVICE_NAME_LENGTH);
    deviceSet.device[DEVICE_NAME_LENGTH-1]='\0';

    int status = sendDataToPulse<paDeviceSet>(PAUDIOD_MSGTYPE_DEVICE, einit_multiple_usb_device_info_reply, deviceSet);

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

    PM_LOG_INFO (MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "sendInternalDeviceInfo sending message %s", buffer);

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
    deviceSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    int status = sendDataToPulse<paDeviceSet>(PAUDIOD_MSGTYPE_DEVICE, einitialise_internal_card_reply, deviceSet);

    return status;
}

bool PulseAudioMixer::loadUSBSinkSource(char cmd,int cardno, int deviceno, int status, PulseCallBackFunc cb)
{
    PM_LOG_DEBUG("PulseAudioMixer::loadUSBSinkSource");
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

    if (mObjMixerCallBack)
        mObjMixerCallBack->callBackMasterVolumeStatus();
    else
        PM_LOG_ERROR(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                     "programLoadBluetooth: mObjMixerCallBack is null");

    pulseCallBackInfo pci;
    pci.lshandle = nullptr;
    pci.message = nullptr;
    pci.ctx = nullptr;
    pci.cb=cb;
    mPulseCallBackInfo.insert(std::make_pair(edetect_usb_device_reply, pci));

    deviceSet.cardNo = cardno;
    deviceSet.deviceNo = deviceno;
    deviceSet.isLoad = 0;
    deviceSet.isMmap = 0;
    deviceSet.isTsched = 0;
    deviceSet.bufSize = 0;
    deviceSet.status = status;
    deviceSet.isOutput = 0;
    deviceSet.maxDeviceCnt = 0;
    deviceSet.device[DEVICE_NAME_LENGTH-1] = {'\0'};

    ret = sendDataToPulse<paDeviceSet>(PAUDIOD_MSGTYPE_DEVICE, edetect_usb_device_reply, deviceSet);

    return ret;
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

void PulseAudioMixer::deviceConnectionStatus (const std::string &deviceName, const std::string &deviceNameDetail, const std::string &deviceIcon, const bool &connectionStatus, const bool &isOutput)
{
    PM_LOG_DEBUG("deviceName:%s %s status:%d", deviceName.c_str(),deviceNameDetail.c_str(), (int)connectionStatus);
    if (mObjMixerCallBack)
    {
        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
            "notifiying status to audio mixer");
        if (connectionStatus)   //TODO
            mObjMixerCallBack->callBackDeviceConnectionStatus(deviceName, deviceNameDetail, deviceIcon, utils::eDeviceConnected, utils::ePulseMixer, isOutput);
        else
            mObjMixerCallBack->callBackDeviceConnectionStatus(deviceName, deviceNameDetail, deviceIcon, utils::eDeviceDisconnected, utils::ePulseMixer,isOutput);
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
                        char appname[APP_NAME_LENGTH];
                        strncpy(appname, sndHdr->appName, APP_NAME_LENGTH);
                        appname[APP_NAME_LENGTH-1]='\0';
                        EVirtualAudioSink sink = EVirtualAudioSink(sinkNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_STREAM_OPEN:%d,%d,%s",sink,sinkIndex,appname);
                        if (VERIFY(IsValidVirtualSink(sink)))
                        {
                            outputStreamOpened (sink , sinkIndex, appname);
                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, \
                            "%s sink %i-%s opened. Volume: %d, Headset: %d, Route: %d, Streams: %d.",
                                    __FUNCTION__, (int)sink, virtualSinkName(sink), \
                                    mPulseStateVolume[sink],\
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
                        char appname[APP_NAME_LENGTH];
                        strncpy(appname, sndHdr->appName, APP_NAME_LENGTH);
                        appname[APP_NAME_LENGTH-1]='\0';
                        EVirtualAudioSink sink = EVirtualAudioSink(sinkNumber);
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,"PAUDIOD_REPLY_MSGTYPE_SINK_CLOSE:%d,%d,%s",sink,sinkIndex,appname);
                        if (VERIFY(IsValidVirtualSink(sink)))
                        {
                            outputStreamClosed (sink,sinkIndex,appname);

                            PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, \
                            "%s sink %i-%s closed stream. Volume: %d, Headset: %d, Route: %d, Streams: %d.", \
                                    __FUNCTION__, (int)sink, virtualSinkName(sink),\
                                    mPulseStateVolume[sink], \
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
                        char deviceName[DEVICE_NAME_LENGTH];
                        char deviceIcon[DEVICE_NAME_LENGTH];
                        char deviceNameDetail[DEVICE_NAME_DETAILS_LENGTH];

                        strncpy(deviceName, sndHdr->device, DEVICE_NAME_LENGTH);
                        strncpy(deviceNameDetail, sndHdr->deviceNameDetail, DEVICE_NAME_DETAILS_LENGTH);
                        strncpy(deviceIcon, sndHdr->deviceIcon, DEVICE_NAME_LENGTH);
                        deviceName[DEVICE_NAME_LENGTH-1]='\0';
                        deviceNameDetail[DEVICE_NAME_DETAILS_LENGTH-1]='\0';
                        deviceIcon[DEVICE_NAME_LENGTH-1]='\0';

                        char isOutput = sndHdr->isOutput;
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                            "PAUDIOD_REPLY_MSGTYPE_DEVICE_CONNECTION : %s:%s",deviceName,deviceNameDetail);
                        deviceConnectionStatus(deviceName, deviceNameDetail, deviceIcon, true, (isOutput==0)?false:true);
                    }
                    break;
                    case PAUDIOD_REPLY_MSGTYPE_DEVICE_REMOVED:
                    {
                        char deviceName[DEVICE_NAME_LENGTH];
                        strncpy(deviceName, sndHdr->device, DEVICE_NAME_LENGTH);
                        deviceName[DEVICE_NAME_LENGTH-1]='\0';
                        char deviceNameDetail[DEVICE_NAME_DETAILS_LENGTH];
                        strncpy(deviceNameDetail, sndHdr->deviceNameDetail, DEVICE_NAME_DETAILS_LENGTH);
                        deviceNameDetail[DEVICE_NAME_DETAILS_LENGTH-1]='\0';
                        char isOutput = sndHdr->isOutput;
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
                            "PAUDIOD_REPLY_MSGTYPE_DEVICE_REMOVED : %s:%s, %d",deviceName,deviceNameDetail, isOutput);
                        deviceConnectionStatus(deviceName, deviceNameDetail, "", false, (isOutput==0)?false:true);
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
                        PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT, "CallBack function is null");
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

std::string PulseAudioMixer::playSound(const char *snd, EVirtualAudioSink sink, const char *format, int rate, int channels)
{
    return mPulseLink.play(snd, sink, format, rate, channels);
}

bool PulseAudioMixer::controlPlayback(std::string playbackId, std::string requestType)
{
    return mPulseLink.controlPlayback(playbackId, requestType);
}

std::string PulseAudioMixer::getPlaybackStatus(std::string playbackId)
{
    return mPulseLink.getPlaybackStatus(playbackId);
}

void PulseAudioMixer::preloadSystemSound(const char * snd)
{
    std::string path = SYSTEMSOUNDS_PATH;
    if (nullptr == snd)
        return;
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
    bool ret = false;

    struct paParamSet paramSet;
    paramSet.Type = PAUDIOD_SETPARAM_CLOSE_PLAYBACK;
    paramSet.ID = sinkIndex;
    paramSet.param1 = 0;
    paramSet.param2 = 0;
    paramSet.param3 = 0;

    int status = sendDataToPulse<paParamSet>(PAUDIOD_MSGTYPE_SETPARAM, eclose_playback_by_sink_input_reply, paramSet);

    return ret;
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
