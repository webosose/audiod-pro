// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#include "umiaudiomixer.h"

#define AUDIOOUTPUT_SERVICE                "com.webos.service.audiooutput"
#define CONNECT                            "luna://com.webos.service.audiooutput/audio/connect"
#define DISCONNECT                         "luna://com.webos.service.audiooutput/audio/disconnect"
#define SET_SOUNDOUT                       "luna://com.webos.service.audiooutput/audio/setSoundOut"
#define SET_MASTER_VOLUME                  "luna://com.webos.service.audiooutput/audio/volume/set"
#define GET_MASTER_VOLUME                  "luna://com.webos.service.audiooutput/audio/volume/getStatus"
#define MASTER_VOLUME_UP                   "luna://com.webos.service.audiooutput/audio/volume/up"
#define MASTER_VOLUME_DOWN                 "luna://com.webos.service.audiooutput/audio/volume/down"
#define MASTER_VOLUME_MUTE                 "luna://com.webos.service.audiooutput/audio/volume/muteSoundOut"
#define INPUT_VOLUME_MUTE                  "luna://com.webos.service.audiooutput/audio/mute"
#define GET_CONN_STATUS                    "luna://com.webos.service.audiooutput/audio/getStatus"

bool umiaudiomixer::connectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message)
{
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"source", strSourceName}, {"sink", strPhysicalSinkName}};
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio connect request for source %s,physicalsink %s", \
                strSourceName.c_str(), strPhysicalSinkName.c_str());
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, CONNECT, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::disconnectAudio(std::string strSourceName, std::string strPhysicalSinkName,  LSFilterFunc cb, envelopeRef *message)
{
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"source", strSourceName}, {"sink", strPhysicalSinkName}};
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio disconnect request for source %s,physicalsink %s",
                strSourceName.c_str(), strPhysicalSinkName.c_str());
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, DISCONNECT, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::setSoundOut(std::string strOutputMode, LSFilterFunc cb, envelopeRef *message)
{
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"soundOut", strOutputMode}};
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio SetSoundOut request for outputMode %s", strOutputMode.c_str());
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, SET_SOUNDOUT, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::setMasterVolume(std::string strSoundOutPut, int iVolume, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio SetMasterVolume request outputmode  %s volume %d ", strSoundOutPut.c_str(), iVolume);
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"soundOutput", strSoundOutPut}, {"volume", iVolume}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, SET_MASTER_VOLUME, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::getMasterVolume(LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio GetMasterVolume request");
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, GET_MASTER_VOLUME, payloadSnd.stringify().c_str(), cb,(void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::masterVolumeUp(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio MasterVolumeUp request outputmode  %s ", strSoundOutPut.c_str());
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"soundOutput", strSoundOutPut}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, MASTER_VOLUME_UP, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::masterVolumeDown(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio MasterVolumeDown request outputmode %s ", strSoundOutPut.c_str());
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"soundOutput", strSoundOutPut}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, MASTER_VOLUME_DOWN, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::masterVolumeMute(std::string strSoundOutPut, bool bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio MasterVolumeMute request soundout %s mute status %d ", strSoundOutPut.c_str(), bIsMute);
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"soundOutput", strSoundOutPut}, {"mute", bIsMute}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, MASTER_VOLUME_MUTE, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::inputVolumeMute(std::string strPhysicalSink, std::string strSource, bool bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio InputVolumeMute request Physical sink %s source %s mute status %d ",\
    strPhysicalSink.c_str(), strSource.c_str(), bIsMute);
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"sink", strPhysicalSink}, {"source", strSource}, {"mute", bIsMute}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, INPUT_VOLUME_MUTE, payloadSnd.stringify().c_str(), cb, (void*)message, nullptr, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::getConnectionStatus(LSFilterFunc cb, envelopeRef *message)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "Audio GetConnectionStatus request");
    bool result = false;
    CLSError lserror;
    LSHandle *sh = GetPalmService();
    pbnjson::JValue payloadSnd = pbnjson::JObject{{}};
    if (mIsUmiMixerReadyToProgram)
    {
        result = LSCallOneReply (sh, GET_CONN_STATUS, payloadSnd.stringify().c_str(), cb, (void*)message, NULL, &lserror);
        if (!result)
        {
            lserror.Print(__FUNCTION__, __LINE__);
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

umiaudiomixer::umiaudiomixer(MixerInterface* mixerCallBack):\
              mIsUmiMixerReadyToProgram(false), mObjMixerCallBack(mixerCallBack)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer constructor");
    CLSError lserror;
    bool result = LSRegisterServerStatusEx(GetPalmService(), AUDIOOUTPUT_SERVICE, audioOutputdServiceStatusCallback, this, NULL, &lserror);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
    }
}

umiaudiomixer::~umiaudiomixer()
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer distructor");
}

bool umiaudiomixer::audioOutputdServiceStatusCallback(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    bool returnValue = true;
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT,\
               "umiaudiomixer audioOutputdServiceStatusCallback, status: %d", connected);
    if (nullptr != ctx)
    {
        umiaudiomixer* umiMixerObj = (umiaudiomixer*) ctx;
        if (umiMixerObj)
        {
            PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT,\
                        "umiaudiomixer audioOutputdServiceStatusCallback, calling callBackMixerStatus");
            umiMixerObj->mObjMixerCallBack->callBackMixerStatus(connected, utils::eUmiMixer);
            umiMixerObj->mIsUmiMixerReadyToProgram = connected;
        }
        else
        {
            PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT,\
                        "umiaudiomixer audioOutputdServiceStatusCallback, umiMixerObj is null");
            returnValue = false;
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT,\
                     "umiaudiomixer audioOutputdServiceStatusCallback, ctx is null");
        returnValue = false;
    }
    return returnValue;
}

bool umiaudiomixer::onSinkChangedReply(const std::string& source, const std::string& sink, EVirtualAudioSink eVirtualSink,\
                                         utils::ESINK_STATUS eSinkStatus, utils::EMIXER_TYPE eMixerType)
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "OnSinkChangedReply: source:%s sink:%s sinkId:%d, Sink status: %d sink type %d",\
        source.c_str(), sink.c_str(), eVirtualSink, (int)eSinkStatus, (int)eMixerType);
    if (mObjMixerCallBack)
        mObjMixerCallBack->callBackSinkStatus(source, sink, eVirtualSink, eSinkStatus, utils::eUmiMixer);
    else
    {
        PM_LOG_ERROR(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "onSinkChangedReply: mObjMixerCallBack is null");
        return false;
    }
    return true;
}
