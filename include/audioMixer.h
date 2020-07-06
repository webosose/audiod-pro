/* @@@LICENSE
*
*      Copyright (c) 2020 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef _AUDIO_MIXER_H_
#define _AUDIO_MIXER_H_

#include "utils.h"
#include "messageUtils.h"
#include "PulseAudioMixer.h"
#include "umiaudiomixer.h"
#include "main.h"
#include <cstdlib>

class Mixer
{
    private:
        Mixer(const Mixer&) = delete;
        Mixer& operator=(const Mixer&) = delete;
        Mixer();
        umiaudiomixer *mObjUmiMixer;
        PulseAudioMixer *mObjPulseMixer;
        AudiodCallbacksInterface *mCallbacks;

    public:
        ~Mixer();
        static Mixer* getMixerInstance();

        //umimixer calls
        bool connectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message);
        bool disconnectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message);
        bool setSoundOut(std::string strOutputMode, LSFilterFunc cb, envelopeRef *message);
        bool setMasterVolume(std::string strSoundOutPut, int iVolume, LSFilterFunc cb, envelopeRef *message);
        bool getMasterVolume(LSFilterFunc cb, envelopeRef *message);
        bool masterVolumeUp(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message);
        bool masterVolumeDown(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message);
        bool masterVolumeMute(std::string strSoundOutPut, bool bIsMute, LSFilterFunc cb, envelopeRef *message);
        bool inputVolumeMute(std::string strPhysicalSink, std::string strSource, bool bIsMute, LSFilterFunc cb, envelopeRef *message);
        bool getConnectionStatus(LSFilterFunc cb, envelopeRef *message);
        bool readyToProgram();
        bool isStreamActive(EVirtualAudiodSink eVirtualSink);

        void onSinkChangedReply(EVirtualAudiodSink eVirtualSink, E_CONNSTATUS eConnStatus, ESinkType eSinkType);
        void updateStreamStatus(EVirtualAudiodSink eVirtualSink, E_CONNSTATUS eConnStatus);
        void setMixerReadyStatus(bool eStatus);

        //To know audiooutputd server status - Need to implement from adapter class
        //static bool audiodOutputdServiceStatusCallBack(LSHandle *sh, const char *serviceName, bool connected, void *ctx);

        //pulsemixer calls
        bool programVolume(EVirtualAudiodSink sink, int volume, bool ramp = false);
        bool programCallVoiceOrMICVolume(char cmd, int volume);
        bool programMute(EVirtualSource source, int mute);
        bool rampVolume(EVirtualAudiodSink sink, int endVolume);
        bool programDestination(EVirtualAudiodSink sink, EPhysicalSink destination);
        bool programDestination(EVirtualSource source, EPhysicalSource destination);
        bool programFilter(int filterTable);
        bool programBalance(int balance);
        bool muteAll();
        bool suspendAll();
        bool updateRate(int rate);
        bool setMute(int sink, int mutestatus);
        bool setVolume(int display, int volume);
        bool playSystemSound(const char *snd, EVirtualAudiodSink sink);
        bool programHeadsetRoute(int route);
        bool externalSoundcardPathCheck(std::string filename,  int status);
        bool loadUSBSinkSource(char cmd,int cardno, int deviceno, int status);
        bool _connectSocket();
        bool suspendSink(int sink);
        bool programLoadBluetooth(const char * address , const char *profile);
        bool programUnloadBluetooth(const char *profile);
        bool setRouting(const ConstString & scenario);
        bool programSource(char cmd, int sink, int value);

        void outputStreamOpened(EVirtualAudiodSink sink);
        void outputStreamClosed(EVirtualAudiodSink sink);
        void inputStreamOpened(EVirtualSource source);
        void inputStreamClosed(EVirtualSource source);
        void preloadSystemSound(const char * snd);
        void playOneshotDtmf(const char *snd, EVirtualAudiodSink sink) ;
        void playOneshotDtmf(const char *snd, const char* sink) ;
        void playDtmf(const char *snd, EVirtualAudiodSink sink) ;
        void playDtmf(const char *snd, const char* sink) ;
        void stopDtmf();
        void _pulseStatus(GIOChannel * ch, GIOCondition condition, gpointer user_data);
        void _timer();
        void setNREC(bool value);
        void openCloseSink(EVirtualAudiodSink sink, bool openNotClose);

        VirtualSinkSet getActiveStreams();

        int loopback_set_parameters(const char * value);
};

#endif //_AUDIO_MIXER_H_