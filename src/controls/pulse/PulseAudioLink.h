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

#ifndef PULSEAUDIOLINK_H_
#define PULSEAUDIOLINK_H_

#include <pulse/pulseaudio.h>
#include <set>
#include <string>

#include "AudioMixer.h"
#define AUDIO_EFFECT_FADE_OUT  1
#define AUDIO_EFFECT_FADE_IN   (1<<1)

#define AUDIO_STATUS_NORMAL  1
#define AUDIO_STATUS_STOPPING  2
#define AUDIO_STATUS_STOPPED  3
#define AUDIO_STATUS_DISCONNECTED 4

class RefObj {
public:
    RefObj():refCount(1){
        int rc;
        rc = pthread_mutexattr_init(&mta);
        if (rc!=0) g_error("Failed pthread_mutexattr_init");
        rc = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
        if (rc!=0) g_error("Failed pthread_mutexattr_settype");
        rc = pthread_mutex_init(&mutex, &mta);
        if (rc!=0) g_error("Failed pthread_mutex_init");
    }
    void lock() {
        pthread_mutex_lock(&mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mutex);
    }
    int ref(){
        pthread_mutex_lock(&mutex);
        ++refCount;
        int ret = refCount;
        pthread_mutex_unlock(&mutex);
        return ret;
    }
    void unref(){
        bool needDelete = false;
        pthread_mutex_lock(&mutex);
        refCount--;
        needDelete = refCount==0;
        pthread_mutex_unlock(&mutex);
        if (needDelete) delete(this);
    }
protected:
    virtual ~RefObj(){};
    int refCount;
    pthread_mutexattr_t   mta;
    pthread_mutex_t       mutex;
};

class PulseAudioDataProvider : public RefObj {
public:
    PulseAudioDataProvider();
    int getStatus() {
        pthread_mutex_lock(&mutex);
        int status = mStatus;
        pthread_mutex_unlock(&mutex);
        return status;
    }
    void setStatus(int s) {
        lock();
        mStatus = s;
        unlock();
    }
    void stopping() {
        lock();
        if (mStatus<AUDIO_STATUS_STOPPING) mStatus=AUDIO_STATUS_STOPPING;
        unlock();
    }
    virtual void disconnected(){
        setStatus(AUDIO_STATUS_DISCONNECTED);
        unref();
    }

    pa_sample_spec* getSampleSpec(){ return &mSampleSpec; }
    pa_volume_t getVolume(){ return mVolume; }
    void setVolume(pa_volume_t v) { mVolume=v; }
    const char* getStreamName() {return mStreamName;}
    void setAudioEffect(int e) {mAudioEffect = e;}
    void setStreamName(const char* s) { mStreamName = s; }

    // The callback is only called when AUDIO_STATUS_NORMAL or AUDIO_STATUS_STOPPING
    // return false when no more data then STATUS is set to AUDIO_STATUS_STOPPED
    virtual bool stream_write_callback(pa_stream *s, size_t length)=0;
protected:
    virtual ~PulseAudioDataProvider();
    int mStatus;
    pa_sample_spec mSampleSpec;
    pa_volume_t mVolume;
    const char* mStreamName;
    int mAudioEffect;
};

/*
 * PulseAudioLink handles a connection with Pulse using Pulse official APIs
 * The only purpose of this class is to allow playing a system sound file
 * in a particular sink
 */

class PulseAudioLink {
public:
    PulseAudioLink();

    /// Connection status & management
    bool    isConnected() const    { return mPulseAudioReady; }
    bool    checkConnection()    { return isConnected()|| connectToPulse(); }

    /// API to playback a system sound. Prefer the version using a EVirtualAudiodSink
    bool    play(const char *snd, EVirtualAudiodSink sink);
    bool    play(const char *snd, const char *sink);

    bool    play(PulseAudioDataProvider* data, const char* sink);

    /// on-demand sounds need to be pre-loaded in Pulse for a faster initial playback
    void    preload(const char * samplename);

    /// These should really be private, but they're needed for global callbacks...
    void    pulseAudioStateChanged(pa_context_state_t state);

protected:
    bool     connectToPulse();
    void    killPulseConnection();
    bool    iteratePulse(int block);

    static void* pathread_func(void*);
    static void stream_drain_complete(pa_stream*stream, int success, void *userdata) ;
    static void data_stream_write_callback(pa_stream *s, size_t length, void *userdata);
    static void PlayAudioDataProviderDeferCB(pa_mainloop_api *a,
                                             pa_defer_event *e,
                                             void *userdata);

private:
    pa_context *            mContext;
    pa_mainloop *            mMainLoop;
    bool                    mPulseAudioReady;
    std::set<std::string>    mLoadedSounds;

    pthread_t mThread;
};

enum Dtmf {
    Dtmf_0,
    Dtmf_1,
    Dtmf_2,
    Dtmf_3,
    Dtmf_4,
    Dtmf_5,
    Dtmf_6,
    Dtmf_7,
    Dtmf_8,
    Dtmf_9,
    Dtmf_Asterisk,
    Dtmf_Pound,
    Dtmf_ArrayCount
};

class PulseDtmfGenerator : public PulseAudioDataProvider {
public:
    PulseDtmfGenerator(Dtmf tone, int milliseconds=0);
    Dtmf getTone(){ return (Dtmf)mDtmf; };
    virtual bool stream_write_callback(pa_stream *s, size_t length);
protected:
    virtual ~PulseDtmfGenerator();
    int mDtmf;
    int mWritePos;
    int mAccumulatedSamples;
    int mPlaySamples;
};

#endif /* PULSEAUDIOLINK_H_ */
