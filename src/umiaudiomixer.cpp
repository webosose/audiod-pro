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
    g_debug("Audio connect request for source %s,physicalsink %s",
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::disconnectAudio(std::string strSourceName, std::string strPhysicalSinkName,  LSFilterFunc cb, envelopeRef *message)
{
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"source", strSourceName}, {"sink", strPhysicalSinkName}};
    g_debug("Audio disconnect request for source %s,physicalsink %s",
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::setSoundOut(std::string strOutputMode, LSFilterFunc cb, envelopeRef *message)
{
    pbnjson::JValue payloadSnd = pbnjson::JObject{{"soundOut", strOutputMode}};
    g_debug("Audio SetSoundOut request for outputMode %s", strOutputMode.c_str());
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::setMasterVolume(std::string strSoundOutPut, int iVolume, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio SetMasterVolume request outputmode  %s volume %d ", strSoundOutPut.c_str(), iVolume);
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::getMasterVolume(LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio GetMasterVolume request");
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::masterVolumeUp(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio MasterVolumeUp request outputmode  %s ", strSoundOutPut.c_str());
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::masterVolumeDown(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio MasterVolumeDown request outputmode %s ", strSoundOutPut.c_str());
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}
bool umiaudiomixer::masterVolumeMute(std::string strSoundOutPut, bool bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio MasterVolumeMute request soundout %s mute status %d ", strSoundOutPut.c_str(), bIsMute);
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::inputVolumeMute(std::string strPhysicalSink, std::string strSource, bool bIsMute, LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio InputVolumeMute request Physical sink %s source %s mute status %d ",\
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::getConnectionStatus(LSFilterFunc cb, envelopeRef *message)
{
    g_debug("Audio GetConnectionStatus request");
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
        g_debug("umiaudiomixer: audioouputd server is not running, status: %d", (int)mIsUmiMixerReadyToProgram);
    }
    return result;
}

bool umiaudiomixer::getUmiMixerReadyStatus()
{
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer:getUmiMixerReadyStatus ready status:%d", \
                mIsUmiMixerReadyToProgram);
    return mIsUmiMixerReadyToProgram;
}

void umiaudiomixer::setUmiMixerReadyStatus(bool eStatus)
{
    mIsUmiMixerReadyToProgram  = eStatus;
    PM_LOG_INFO(MSGID_UMIAUDIO_MIXER, INIT_KVCOUNT, "umiaudiomixer:setUmiMixerReadyStatus ready status:%d", \
                mIsUmiMixerReadyToProgram);
}

umiaudiomixer* umiaudiomixer::getUmiMixerInstance()
{
    static umiaudiomixer umiMixerInstance;
    return &umiMixerInstance;
}

umiaudiomixer::umiaudiomixer(): mIsUmiMixerReadyToProgram(false)
{
    g_debug("umiaudiomixer constructor");
}

umiaudiomixer::~umiaudiomixer()
{
    g_debug("umiaudiomixer distructor");
}

void umiaudiomixer::onSinkChangedReply(EVirtualAudioSink eVirtualSink, utils::ECONN_STATUS eConnStatus, utils::EMIXER_TYPE eMixerType)
{
    g_debug("OnSinkChangedReply Sink Name :%d, Connection status: %d sink type %d", eVirtualSink, eConnStatus, eMixerType);
    utils::ECONN_STATUS eSinkStatus = utils::eConnectionNone;
    if (utils::eConnected == eConnStatus)
    {
        eSinkStatus = utils::eConnected;
    }
    else if (utils::eDisconnected == eConnStatus)
    {
        eSinkStatus = utils::eDisconnected;
    }
    else
    {
        g_debug("umiaudiomixer: OnSinkChangedReply Invalid Connection status");
    }
    updateStreamStatus(eVirtualSink, eConnStatus);
}

void umiaudiomixer::updateStreamStatus(EVirtualAudioSink eVirtualSink, utils::ECONN_STATUS eConnStatus)
{
    if (utils::eConnected == eConnStatus)
    {
        mVectActiveStreams.push_back(eVirtualSink);
    }
    else if (utils::eDisconnected == eConnStatus)
    {
        for (std::vector<EVirtualAudioSink>::iterator itStream = mVectActiveStreams.begin() ; itStream != mVectActiveStreams.end(); ++itStream)
        {
            if (*itStream == eVirtualSink)
            {
                mVectActiveStreams.erase(itStream);
                return;
            }
        }
    }
    else
    {
        g_debug("umiaudiomixer: UpdateStreamStatus Invalid Connection status");
    }
}

bool umiaudiomixer::isStreamActive(EVirtualAudioSink eVirtualSink)
{
    for (auto &elements : mVectActiveStreams)
    {
        if (elements == eVirtualSink)
        {
            return true;
        }
    }
    return false;
}
