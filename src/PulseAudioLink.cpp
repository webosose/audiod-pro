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


#include "PulseAudioLink.h"

#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_CHANNELS 1
#define DEFAULT_SAMPLE_FORMAT "PA_SAMPLE_S16LE"

static const size_t kSampleNameMaxSize = 64;

struct ssound_t {
    FILE *          file;
    char            samplename[kSampleNameMaxSize];
    size_t          length;
    size_t          tot_written;
    pa_sample_spec  spec;
    bool            loading;
    bool            isSuccess;
};


static void initializeDtmf();

PulseAudioLink::PulseAudioLink() : mContext(0), mMainLoop(0), mPulseAudioReady(false)
{
    initializeDtmf();
}

PulseAudioLink::~PulseAudioLink()
{
    for (const auto& it : mMapAudioList)
        delete it.second;
    mMapAudioList.clear();
}

void PulseAudioLink::pulseAudioStateChanged(pa_context_state_t state)
{
    switch (state)
    {
    case PA_CONTEXT_FAILED:
        mPulseAudioReady = false;
        pa_mainloop_get_api(mMainLoop)->quit(pa_mainloop_get_api(mMainLoop), -1);
        break;
    case PA_CONTEXT_READY:
        mPulseAudioReady = true;
        break;
    case PA_CONTEXT_TERMINATED:
        mPulseAudioReady = false;
    default:
        break;
    }
}

void PulseAudioLink::killPulseConnection()
{
    if (mContext)
        pa_context_unref(mContext);
    if (mMainLoop)
        pa_mainloop_free(mMainLoop);
    mMainLoop = 0;
    mContext = 0;
    mPulseAudioReady = false;
    mLoadedSounds.clear();
}

bool PulseAudioLink::iteratePulse(int block)
{
    int processed, ret;

    while ((processed = pa_mainloop_iterate(mMainLoop, block, &ret)) > 0)
        ;
    if (processed < 0 && ret < 0)
    {
        killPulseConnection();
        return false;
    }
    return true;
}

static void pulseAudioCallback(pa_context * c, void * user)
{
    if (user)
        reinterpret_cast<PulseAudioLink *>(user)->pulseAudioStateChanged(pa_context_get_state(c));
}

bool PulseAudioLink::play(const char *snd, EVirtualAudioSink sink)
{
    PMTRACE_FUNCTION;
    if (!IsValidVirtualSink(sink))
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
            "'%d' is not a valid sink id", (int)sink);
        return false;
    }
    //will be implemented as per DAP design
    else
    {
        return play(snd, virtualSinkName(sink, false));
    }
}

struct PlaySampleDeferData {
    char samplename[kSampleNameMaxSize];
    const char* sink;
    pa_context* pacontext;
};

static void PlaySampleDeferCB(pa_mainloop_api *a, pa_defer_event *e, void *userdata)
{
    PlaySampleDeferData* data  = (PlaySampleDeferData*)userdata;

    // prepare HW for playing audio. Will unmute Pixie in particular...
    //gAudioDevice.prepareForPlayback();  //Will be removed or updated once DAP design is updated

    // confidentiality: don't log dtmf tone names to hide phone numbers & pass codes!
    const char * name = data->samplename;
    if (strncmp(name, "dtmf_", 5) == 0)
        name = "dtmf_X";
    PM_LOG_DEBUG("PulseAudioLink::play: '%s' in '%s'", name, data->sink);

    // prepare HW for playing audio. will unmute speaker in=f in music+headset case
    if(strstr (name, "alert_"))
    {
        //Will be removed or updated once DAP design is updated
        //gAudioDevice.prepareHWForPlayback();
    }

    pa_operation * op = pa_context_play_sample(data->pacontext,
                                               data->samplename,
                                               data->sink,
                                               PA_VOLUME_NORM,
                                               NULL, NULL);
    if (op)
    {
        pa_operation_unref(op);
    }
    free(data);
    a->defer_free(e);
}

bool PulseAudioLink::play(const char * samplename, const char * sink)
{
    PMTRACE_FUNCTION;
    std::string path = SYSTEMSOUNDS_PATH;
    if (nullptr == samplename)
        return false;
    path += samplename;
    path += "-ondemand.pcm";

    preload(samplename, DEFAULT_SAMPLE_FORMAT, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS, path.c_str());

    // This will affect latency severely, but then again, how often will
    //we lose connection (ie, pulseaudio crashed)
    if (!checkConnection())
        return false;

    PlaySampleDeferData* data = (PlaySampleDeferData*)malloc(sizeof(PlaySampleDeferData));
    if (data)
    {
        strncpy(data->samplename, samplename, sizeof(data->samplename)-1);
        data->sink = sink;
        data->pacontext = mContext;
        pa_mainloop_get_api(mMainLoop)->defer_new(pa_mainloop_get_api(mMainLoop),
                                                  &PlaySampleDeferCB, data);
    }
    else
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::play: data handle is NULL");
        return false;
    }
    return true;
}

std::string PulseAudioLink::playSound(const char * samplename, const char * sink, const char * format, int rate, int channels)
{
    std::string playbackID = GenerateUniqueID()();
    PlaybackThread *thread = new PlaybackThread(mCallback, playbackID);;
    bool ret = thread->play(samplename, sink, format, rate, channels);
    if (ret)
    {
        mMapAudioList[playbackID] = thread;
        return playbackID;
    }
    delete thread;
    return std::string();
}

bool PulseAudioLink::controlPlayback(std::string playbackId, std::string requestType)
{
    std::map<std::string, PlaybackThread *>::iterator it = mMapAudioList.find (playbackId);
    if (it == mMapAudioList.end())
        return false;

    if (requestType == "pause")
        return it->second->pause();
    if (requestType == "resume")
        return it->second->resume();
    if(requestType == "stop")
    {
        if (it->second->stop())
        {
            delete it->second;
            mMapAudioList.erase(it);
            return true;
        }
    }
    return false;
}

std::string PulseAudioLink::getPlaybackStatus(std::string playbackId)
{
    std::map<std::string, PlaybackThread *>::iterator it = mMapAudioList.find (playbackId);
    if(it == mMapAudioList.end())
        return std::string();
    return it->second->getPlaybackStatus();
}

bool PulseAudioLink::play(const char * samplename, const char * sink, const char * format, int rate, int channels)
{
    PMTRACE_FUNCTION;
    std::string sample = samplename;
    std::size_t found = sample.find_last_of("/");
    sample.substr(0,found);
    std::string filename = sample.substr(found+1);
    found = filename.find_last_of(".");
    std::string preloadName = filename.substr(0, found);

    preload(preloadName.c_str(), format, rate, DEFAULT_CHANNELS, samplename);

    // This will affect latency severely, but then again, how often will
    //we lose connection (ie, pulseaudio crashed)
    if (!checkConnection())
        return false;

    PlaySampleDeferData* data = (PlaySampleDeferData*)malloc(sizeof(PlaySampleDeferData));
    if (data)
    {
        strncpy(data->samplename, preloadName.c_str(), sizeof(data->samplename)-1);
        data->sink = sink;
        data->pacontext = mContext;
        pa_mainloop_get_api(mMainLoop)->defer_new(pa_mainloop_get_api(mMainLoop),
                                              &PlaySampleDeferCB, data);
    }
    else
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::play: data handle is NULL");
        return false;
    }

    return true;
}

PulseAudioDataProvider::PulseAudioDataProvider()
:mStatus(AUDIO_STATUS_NORMAL)
,mVolume(PA_VOLUME_NORM)
,mStreamName("AudiodStream")
,mAudioEffect(0)
{
    mSampleSpec.format = PA_SAMPLE_S16LE;
    mSampleSpec.rate = 44100;
    mSampleSpec.channels = 1;
}

PulseAudioDataProvider::~PulseAudioDataProvider()
{
}

void PulseAudioLink::stream_drain_complete(pa_stream*stream,
                                             int success,
                                             void *userdata)
{
    PulseAudioDataProvider* data = (PulseAudioDataProvider*)userdata;

    if (!success) {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
            "drain failed");
        // close connection??
    }

    pa_stream_disconnect(stream);
    pa_stream_unref(stream);

    data->disconnected();

}

void PulseAudioLink::data_stream_write_callback(pa_stream *s,
                                                  size_t length,
                                                  void *userdata)
{
    PMTRACE_FUNCTION;
    PulseAudioDataProvider* data = (PulseAudioDataProvider*) userdata;
    int status = data->getStatus();
    if (status<=AUDIO_STATUS_STOPPING) {
        if (!data->stream_write_callback(s, length)) {
            data->setStatus(AUDIO_STATUS_STOPPED);
            pa_operation_unref(pa_stream_drain(s, stream_drain_complete, data));
        }
    }
}


struct PlayAudioDataProviderDeferData {
    PulseAudioDataProvider* dataProvider;
    const char* sinkname;
    pa_context* pacontext;
};

void PulseAudioLink::PlayAudioDataProviderDeferCB(pa_mainloop_api *a,
                                                  pa_defer_event *e,
                                                   void *userdata)
{
    PMTRACE_FUNCTION;
    struct PlayAudioDataProviderDeferData* datacb =
                              (struct PlayAudioDataProviderDeferData*)userdata;
    pa_stream* stream = pa_stream_new(datacb->pacontext,
                                      datacb->dataProvider->getStreamName(),
                                      datacb->dataProvider->getSampleSpec(),
                                      NULL);
    if (stream==NULL) goto exit;

    int r;
    pa_cvolume cv;
    //pa_stream_set_state_callback(stream, data_stream_state_callback, NULL);
    pa_stream_set_write_callback(stream,
                                 PulseAudioLink::data_stream_write_callback,
                                 datacb->dataProvider);
    r = pa_stream_connect_playback(stream,
                                   datacb->sinkname,
                                   NULL,
                                  (pa_stream_flags)0,
                                  pa_cvolume_set(&cv,
                                                  datacb->dataProvider->getSampleSpec()->channels,
                                                  datacb->dataProvider->getVolume()
                                                  ),
                                  NULL);
exit:
    free(datacb);
    a->defer_free(e);

}

bool PulseAudioLink::play(PulseAudioDataProvider* data, const char* sinkname)
{
    PMTRACE_FUNCTION;
    if (!checkConnection())
        return false;

    data->ref();
    struct PlayAudioDataProviderDeferData* dataCB =
                           (struct PlayAudioDataProviderDeferData*)malloc
                               (sizeof(struct PlayAudioDataProviderDeferData));
    if (dataCB)
    {
        dataCB->dataProvider = data;
        dataCB->sinkname = sinkname;
        dataCB->pacontext = mContext;
        pa_mainloop_get_api(mMainLoop)->defer_new(pa_mainloop_get_api(mMainLoop),
                            &PlayAudioDataProviderDeferCB,
                            dataCB);

    }
    else
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::play: data handle is NULL");
        return false;
    }
    return true;
}

std::string PulseAudioLink::play(const char *snd, EVirtualAudioSink sink, const char *format, int rate, int channels)
{
    PMTRACE_FUNCTION;
    PM_LOG_INFO(MSGID_PULSE_LINK, INIT_KVCOUNT,\
        "Inside play function with filename %s, sink %d, format %s, rate %d and channels %d", \
                 snd, (int)sink, format, rate, channels);
    if (!IsValidVirtualSink(sink))
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
            "'%d' is not a valid sink id", (int)sink);
        return std::string();
    }
    return playSound(snd, virtualSinkName(sink, false), format, rate, channels);
}

bool PulseAudioLink::connectToPulse()
{
    PMTRACE_FUNCTION;
    killPulseConnection();
    mMainLoop = pa_mainloop_new();
    mContext = pa_context_new(pa_mainloop_get_api(mMainLoop), "AudioD");
    pa_context_set_state_callback(mContext, pulseAudioCallback, (void*) this);
    if (pa_context_connect(mContext, NULL, (pa_context_flags_t) 0, NULL) < 0)
    {
        killPulseConnection();
        return false;
    }
    ///iterate until we receive mod->paReady, or an error connecting
    while (iteratePulse(0))
    {
        if (mPulseAudioReady)
        {
            PM_LOG_INFO(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "Connected to Pulse for system sounds");
            if (pthread_create(&mThread, NULL, &pathread_func, this)==0) {
                pthread_detach(mThread);
                return true;
            } else {
                mPulseAudioReady = false;
            }
        }
    }
    return false;
}

void PulseAudioLink::registerCallback(MixerInterface *mixerCallBack)
{
    mCallback = mixerCallBack;
}

class PreloadDeferCBData : public RefObj
{
public:
    PreloadDeferCBData()
    {
        memset(&snd, 0, sizeof(snd));
        mainloop = NULL;
        context = NULL;
        s = NULL;
    }

    ~PreloadDeferCBData()
    {
        if (s)
        {
        pa_stream_set_write_callback(s, NULL, NULL);
        pa_stream_unref(s);
        }

        if (snd.file)
        {
        if (fclose(snd.file) != 0)
        {
            PM_LOG_ERROR(MSGID_SYSTEMSOUND_MANAGER, INIT_KVCOUNT,"Error closing audio file:");
        }
        }
    }

    ssound_t snd;
    pa_mainloop* mainloop;
    pa_context* context;
    pa_stream *s;
};

static void preload_stream_state_cb(pa_stream * s, void *userdata)
{
    PreloadDeferCBData* data = (PreloadDeferCBData*)userdata;
    bool unref=false;
    data->lock();
    ssound_t * snd = &(data->snd);

    switch (pa_stream_get_state(s)) {
        case PA_STREAM_CREATING:
        case PA_STREAM_READY:
            break;

        case PA_STREAM_FAILED:
        default:
            PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "stream_state_cb: Failed to upload sample '%s': %s (%d)", \
                snd->samplename,
                pa_strerror(pa_context_errno(pa_stream_get_context(s))),
                pa_stream_get_state(s));
            //Fall through and clean up, and then try the next sample.

            snd->loading = false;
            snd->isSuccess = false;
            unref = true;
            break;

        case PA_STREAM_TERMINATED:
            PM_LOG_INFO(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "stream_state_cb: Successfully pre-loaded '%s'", snd->samplename);
            snd->loading = false;
            snd->isSuccess = true;
            unref = true;
            break;
   }
    data->unlock();
    if (unref)
        data->unref();
}

static void preload_stream_write_cb(pa_stream * s, size_t length, void * userdata)
{
    PMTRACE_FUNCTION;
    PreloadDeferCBData* cbdata = (PreloadDeferCBData*)userdata;
    cbdata->lock();
    ssound_t * snd = &(cbdata->snd);

    void * data = pa_xmalloc(length);

    size_t len = fread(data, 1, length, snd->file);
    if(len < length){
    PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,
              "PulseAudioLink::preload_stream_write_cb: Error reading from file");
    }
    snd->tot_written += len;

    pa_stream_write(s, data, len, pa_xfree, 0, PA_SEEK_RELATIVE);

    if (snd->tot_written == snd->length)
    {
        pa_stream_set_write_callback(s, NULL, NULL);
        pa_stream_finish_upload(s);
    }
    cbdata->unlock();
}

static void preloadDeferCB(pa_mainloop_api *a, pa_defer_event *e, void *userdata) {
    PMTRACE_FUNCTION;
    a->defer_free(e);
    struct PreloadDeferCBData* cbdata = (struct PreloadDeferCBData*)userdata;
    bool unref= false;
    cbdata->lock();
    PM_LOG_DEBUG("PulseAudioLink::preload: Pre-loading '%s', %u bytes.",\
        cbdata->snd.samplename,\
        cbdata->snd.length);
    cbdata->s = pa_stream_new(cbdata->context,
                              cbdata->snd.samplename,
                              &cbdata->snd.spec,
                              NULL);
    if (VERIFY(cbdata->s))
    {
        pa_stream_set_state_callback(cbdata->s, preload_stream_state_cb, userdata);
        pa_stream_set_write_callback(cbdata->s, preload_stream_write_cb, userdata);
        pa_stream_connect_upload(cbdata->s, cbdata->snd.length);
    } else {
        unref = true;
    }
    cbdata->unlock();
    if (unref)
        cbdata->unref();
}

void PulseAudioLink::preload(const char * samplename, const char * format, int rate, int channels, const char * path)
{
    // is the sound file loaded?
    PMTRACE_FUNCTION;
    if (strlen(samplename) >= kSampleNameMaxSize ||
                              mLoadedSounds.find(samplename) != mLoadedSounds.end())
        return;

    struct stat fileStat;
    FILE* f = fopen(path, "r");
    if (!f)
    {
        PM_LOG_DEBUG("PulseAudioLink::preload:File Open Failed for %s. Returning from Here", path);
        return;
    }
    if (stat(path, &fileStat) == 0)
    {
        // can we talk to Pulse to load it?
        if (!VERIFY(checkConnection()))
        {
            if (fclose(f))
            {
                PM_LOG_DEBUG("PulseAudioLink::preload: Failed to close the file");
            }
            else
            {
                PM_LOG_DEBUG("PulseAudioLink::preload: Successfully closed the file");
            }
            return;
        }
        PreloadDeferCBData* data = new PreloadDeferCBData();
        data->snd.file = f;
        strncpy(data->snd.samplename, samplename, sizeof(data->snd.samplename)-1);
        data->snd.length = fileStat.st_size;
        data->snd.tot_written = 0;
        if (std::strncmp(format, "PA_SAMPLE_S16LE", 15) == 0)
            data->snd.spec.format = PA_SAMPLE_S16LE;
        else if (std::strncmp(format,"PA_SAMPLE_S24LE", 15) == 0)
            data->snd.spec.format = PA_SAMPLE_S24LE;
        else
            data->snd.spec.format = PA_SAMPLE_S32LE;
        data->snd.spec.rate = rate;
        data->snd.spec.channels = channels;
        data->snd.loading = true;
        data->snd.isSuccess = false;
        data->context = mContext;
        data->mainloop = mMainLoop;
        data->ref();
        pa_mainloop_get_api(mMainLoop)->defer_new(pa_mainloop_get_api(mMainLoop),
                                                      &preloadDeferCB,
                                                      data);

        // make sure we never loop for ever trying to load...
        guint64 maxTime = getCurrentTimeInMs() + 5000;
        while (data->snd.loading && getCurrentTimeInMs() < maxTime)
        {
            usleep(1000);
        }
        data->lock();
        if (data->snd.loading)
        {
            PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                    "PulseAudioLink::preload: failed to load sample");
        }
        else
        {
            if (!data->snd.isSuccess && data->s) pa_stream_disconnect(data->s);
        }
        data->unlock();
        data->unref();
    }
    else
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT, \
                "fstat Operation failed returning from here");
        if (fclose(f))
        {
            PM_LOG_DEBUG("PulseAudioLink::preload: Failed to close the file");
        }
        else
        {
            PM_LOG_DEBUG("PulseAudioLink::preload: Successfully closed the file");
        }
        return;
    }

    // success or failure, no need to try again
    if (mLoadedSounds.find(samplename) == mLoadedSounds.end())
        mLoadedSounds.insert(samplename);
    else
        PM_LOG_DEBUG("sample name %s already loaded", samplename);
}

void* PulseAudioLink::pathread_func(void* p) {
    PulseAudioLink* link = (PulseAudioLink*)p;
    int ret;
    if (pa_mainloop_run(link->mMainLoop, &ret) < 0) {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
            "pa_mainloop_run() failed");
    }
    PM_LOG_DEBUG("pathread_func() exit %d", ret);
    return (void*)ret;
}

#define DTMF_SAMPLE_RATE 44100
#ifdef MACHINE_NO_LONGDTMF
#define DTMF_SAMPLE (DTMF_SAMPLE_RATE/5)  // 0.2s for short dtmf
#else
#define DTMF_SAMPLE DTMF_SAMPLE_RATE
#endif
#define DTMF_SAMPLE_BYTES_PER_FRAME 2
#define DTMF_FADE_SAMPLES (DTMF_SAMPLE_RATE/50)   // 0.02s

enum DtmfSine {
    Sine_697,
    Sine_770,
    Sine_852,
    Sine_941,
    Sine_1209,
    Sine_1336,
    Sine_1477,
    Sine_1633,
    Sine_ArrayCount
};
static int sine_frequency[] = {
    697, 770, 852, 941, 1209, 1336, 1477, 1633
};

static int dtmf_mapping[Dtmf_ArrayCount][2] = {
    {Sine_941, Sine_1336},
    {Sine_697, Sine_1209},
    {Sine_697, Sine_1336},
    {Sine_697, Sine_1477},
    {Sine_770, Sine_1209},
    {Sine_770, Sine_1336},
    {Sine_770, Sine_1477},
    {Sine_852, Sine_1209},
    {Sine_852, Sine_1336},
    {Sine_852, Sine_1477},
    {Sine_941, Sine_1209},
    {Sine_941, Sine_1477},
};

static bool dtmf_buffer_intiailized = false;
static gint16 dtmf_buffer[Dtmf_ArrayCount][DTMF_SAMPLE];

static void initializeDtmf() {
    if (dtmf_buffer_intiailized) return;
    dtmf_buffer_intiailized = true;

    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
#define FLOAT_TYPE double
    FLOAT_TYPE* sine_buffer= new FLOAT_TYPE[Sine_ArrayCount*DTMF_SAMPLE];
    for (DtmfSine i=(DtmfSine)0; i<Sine_ArrayCount; i=(DtmfSine)(i+1) ) {
        FLOAT_TYPE temp = M_PI*2*sine_frequency[i]/DTMF_SAMPLE_RATE;
        FLOAT_TYPE* b = sine_buffer + i * DTMF_SAMPLE;
        for (int j = 0; j < DTMF_SAMPLE; ++j)
            b[j] = sin((FLOAT_TYPE) j * temp)/2;
    }
    for (Dtmf i=(Dtmf)0; i<Dtmf_ArrayCount; i=(Dtmf)(i+1)) {
        gint16* dtmf_buffer_i = dtmf_buffer[i];
        FLOAT_TYPE* sine1 = sine_buffer + dtmf_mapping[i][0] * DTMF_SAMPLE;
        FLOAT_TYPE* sine2 = sine_buffer + dtmf_mapping[i][1] * DTMF_SAMPLE;
        for (int j=0; j<DTMF_SAMPLE; ++j) {
            dtmf_buffer_i[j] = (gint16)((sine1[j] + sine2[j]) *16384);
        }
    }
    delete[] sine_buffer;
#undef FLOAT_TYPE
    gettimeofday(&tv2, NULL);
    PM_LOG_INFO(MSGID_PULSEAUDIO_MIXER, INIT_KVCOUNT,\
        "initializeDtmf in %d ms\n",(int)((tv2.tv_sec - tv1.tv_sec)*1000) + \
        (int)((tv2.tv_usec - tv1.tv_usec)/1000));

}

PulseDtmfGenerator::PulseDtmfGenerator(Dtmf tone, int milliseconds)
:PulseAudioDataProvider(),mDtmf(tone),mWritePos(0)
,mAccumulatedSamples(0),mPlaySamples(0)
{
    if (milliseconds>0) mPlaySamples = DTMF_SAMPLE_RATE / 1000 * milliseconds;
    setAudioEffect(AUDIO_EFFECT_FADE_OUT | AUDIO_EFFECT_FADE_IN);
}

PulseDtmfGenerator::~PulseDtmfGenerator(){}

// stream_write_callback() is invoked from single thread, safe to make it global
static gint16 fadebuffer[DTMF_FADE_SAMPLES];

bool PulseDtmfGenerator::stream_write_callback(pa_stream *stream, size_t length)
{
    PMTRACE_FUNCTION;
    bool isStopping;
    pthread_mutex_lock(&mutex);
    if (mStatus==AUDIO_STATUS_STOPPING) {
        isStopping = true;
    } else if (mStatus==AUDIO_STATUS_NORMAL) {
        isStopping = false;
    } else {
        PM_LOG_WARNING(MSGID_PULSE_LINK, INIT_KVCOUNT,\
            "stream_write_callback IllegalStatus %d", mStatus);
        pthread_mutex_unlock(&mutex);
        return false;
    }
    gint16* b = dtmf_buffer[mDtmf];
    int samples = length/DTMF_SAMPLE_BYTES_PER_FRAME;
    while (samples>0) {
        int samplesToPlay;
        if (mWritePos+samples>=DTMF_SAMPLE) samplesToPlay = DTMF_SAMPLE-mWritePos;
        else samplesToPlay = samples;
        if (mPlaySamples>0 && mAccumulatedSamples+samplesToPlay>=mPlaySamples) {
            samplesToPlay = mPlaySamples-mAccumulatedSamples;
        }

        if (samplesToPlay>0) {
            int fadeInSamples = 0;
            int normalSamples = samplesToPlay;
            int fadeOutSamples = 0;
            if ((mAudioEffect | AUDIO_EFFECT_FADE_IN)) {
                if (mAccumulatedSamples < DTMF_FADE_SAMPLES) {
                    fadeInSamples = DTMF_FADE_SAMPLES - mAccumulatedSamples;
                    if (fadeInSamples>samplesToPlay) fadeInSamples = samplesToPlay;
                    normalSamples = samplesToPlay - fadeInSamples;
                    gint16 * originalbuffer = b+mWritePos;
                    for (int i=0; i<fadeInSamples; i++) {
                        fadebuffer[i] = originalbuffer[i] *
                                        (mAccumulatedSamples+i) /
                                         DTMF_FADE_SAMPLES;
                    }
                }
            }
            if (fadeInSamples>0) pa_stream_write(stream,
                                                 fadebuffer,
                                                 fadeInSamples*DTMF_SAMPLE_BYTES_PER_FRAME,
                                                 NULL,
                                                 0,
                                                 PA_SEEK_RELATIVE );


            if (normalSamples>0 && (mAudioEffect | AUDIO_EFFECT_FADE_OUT)) {
                if ((mPlaySamples>0 && mAccumulatedSamples + samplesToPlay >
                                       mPlaySamples - DTMF_FADE_SAMPLES) ) {
                    normalSamples = mPlaySamples -
                                    DTMF_FADE_SAMPLES -
                                    mAccumulatedSamples -
                                    fadeInSamples; // could be <0
                    fadeOutSamples = normalSamples>=0?
                                     (samplesToPlay - normalSamples - fadeInSamples) :
                                     (samplesToPlay-fadeInSamples);
                    gint16 * originalbuffer = b+mWritePos+fadeInSamples + normalSamples;
                    if (normalSamples>0) {
                        for (int i=0; i<fadeOutSamples; i++) {
                            fadebuffer[i] = originalbuffer[i] *
                                            (DTMF_FADE_SAMPLES-i) /
                                             DTMF_FADE_SAMPLES;
                        }
                    } else {
                        for (int i=0; i<fadeOutSamples; i++) {
                            fadebuffer[i] = originalbuffer[i] *
                                            (DTMF_FADE_SAMPLES-i-normalSamples) /
                                            DTMF_FADE_SAMPLES;
                        }
                        normalSamples = 0;
                    }
                } else if (isStopping && samples - samplesToPlay < DTMF_FADE_SAMPLES) {
                    fadeOutSamples = samplesToPlay - (samples-DTMF_FADE_SAMPLES); // >0
                    if (fadeOutSamples > normalSamples) fadeOutSamples = normalSamples;
                    normalSamples = normalSamples - fadeOutSamples;
                    gint16 * originalbuffer = b+mWritePos+fadeInSamples + normalSamples;
                    for (int i=0; i<fadeOutSamples; i++) {
                        fadebuffer[i] = originalbuffer[i] *
                                        (samples - samplesToPlay + fadeOutSamples - i) /
                                         DTMF_FADE_SAMPLES;
                    }
                }
            }
            if (normalSamples>0) {
                pa_stream_write(stream,
                                b+mWritePos+fadeInSamples,
                                normalSamples*DTMF_SAMPLE_BYTES_PER_FRAME,
                                NULL,
                                0,
                                PA_SEEK_RELATIVE );
            }
            if (fadeOutSamples>0) {
                pa_stream_write(stream,
                                fadebuffer,
                                fadeOutSamples*DTMF_SAMPLE_BYTES_PER_FRAME,
                                NULL,
                                0,
                                PA_SEEK_RELATIVE );
            }

            mWritePos += samplesToPlay;
            if (mWritePos==DTMF_SAMPLE) mWritePos=0;
            mAccumulatedSamples+=samplesToPlay;
            samples -= samplesToPlay;
        } else {
            isStopping = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    if (isStopping) {
        return false;
    } else return true;

}

PlaybackThread::PlaybackThread(MixerInterface* mixerCallBack, std::string playbackID)
{
    mCallback = mixerCallBack;
    mPlaybackId = playbackID;
    mDataAvailable = false;
    fptr = NULL;
    mStream = NULL;
    mPlayData = NULL;
}

bool PlaybackThread::playThread()
{
    // Read and write data in chunks of 1024 bytes
    size_t dataSize = 1024;
    mPlayData = (uint8_t*)malloc(dataSize);
    if (!mPlayData)
    {
        pa_simple_free(mStream);
        if(fptr != nullptr)
        {
            if(fclose(fptr))
            {
                PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,"PulseAudioLink::playThread: Error closing file");
            }
        }
        playbackState = "error";
        PM_LOG_DEBUG(" PlaybackThread::playThread playbackState = %s", playbackState.c_str());
        playbackStatusChanged(mPlaybackId, playbackState);
        return false;
    }
    playbackState = "playing";
    PM_LOG_DEBUG(" PlaybackThread::playThread playbackState = %s", playbackState.c_str());
    while (mDataAvailable) {
    // Read data from the file
    size_t n = fread(mPlayData, 1, dataSize, fptr);
    if (n <= 0) {
        playbackState = "stopped";
        playbackStatusChanged(mPlaybackId, playbackState);
        mDataAvailable = false;
        break;
    }
    // Write data to the stream
    if (pa_simple_write(mStream, mPlayData, n, NULL) < 0) {
        break;
    }
    }
    mDataAvailable = false;
    return true;
}


bool PlaybackThread::play(const char * samplename, const char * sink, const char * format, int rate, int channels)
{
    PMTRACE_FUNCTION;
    PM_LOG_DEBUG(" PlaybackThread::play");

    if (nullptr == samplename)
        return false;

    fptr = fopen(samplename, "r");
    if (!fptr)
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::play: File Open Failed");
        return false;
    }
    pa_sample_spec ss;
    if (std::strncmp(format, "PA_SAMPLE_S16LE", 15) == 0)
        ss.format = PA_SAMPLE_S16LE;
    else if (std::strncmp(format,"PA_SAMPLE_S24LE", 15) == 0)
        ss.format = PA_SAMPLE_S24LE;
    else
        ss.format = PA_SAMPLE_S32LE;
    ss.channels = channels;
    ss.rate = rate;
    mStream = pa_simple_new(NULL, NULL, PA_STREAM_PLAYBACK, sink,
            "playback", &ss, NULL, NULL, NULL);
    if (!mStream)
    {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::play: failed to create stream");
        playbackState = "error";
        PM_LOG_DEBUG(" PlaybackThread::play playbackState = %s", playbackState.c_str());
        return false;
    }
    mDataAvailable = true;
    playbackThread = std::thread(&PlaybackThread::playThread, this);
    return true;
}

bool PlaybackThread::pause()
{
    PM_LOG_DEBUG(" PlaybackThread::pause");
    if (playbackState != "playing")
        return false;
    if (pa_simple_cork(mStream, 1, NULL) < 0) {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::pause: Failed to pause stream");
        free((void*)mPlayData);
        pa_simple_free(mStream);
        playbackState = "error";
        PM_LOG_DEBUG(" PlaybackThread::pause playbackState = %s", playbackState.c_str());
        playbackStatusChanged(mPlaybackId, playbackState);
        return false;
    }
    playbackState = "paused";
    PM_LOG_DEBUG(" PlaybackThread::pause playbackState = %s", playbackState.c_str());
    playbackStatusChanged(mPlaybackId, playbackState);
    return true;
}

bool PlaybackThread::resume()
{
    PM_LOG_DEBUG(" PlaybackThread::resume");
    if (playbackState != "paused")
        return false;
    if (pa_simple_cork(mStream, 0, NULL) < 0) {
        PM_LOG_ERROR(MSGID_PULSE_LINK, INIT_KVCOUNT,\
                "PulseAudioLink::resume: Failed to resume stream");
        free((void*)mPlayData);
        pa_simple_free(mStream);
        playbackState = "error";
        PM_LOG_DEBUG(" PlaybackThread::resume playbackState = %s", playbackState.c_str());
        playbackStatusChanged(mPlaybackId, playbackState);
        return false;
    }
    playbackState = "playing";
    PM_LOG_DEBUG(" PlaybackThread::resume playbackState = %s", playbackState.c_str());
    playbackStatusChanged(mPlaybackId, playbackState);
    return true;
}

bool PlaybackThread::stop()
{
    PM_LOG_DEBUG(" PlaybackThread::stop");
    if (pa_simple_flush(mStream, NULL) < 0) {
        free((void*)mPlayData);
        pa_simple_free(mStream);
        playbackState = "error";
        PM_LOG_DEBUG(" PlaybackThread::stop playbackState = %s", playbackState.c_str());
        playbackStatusChanged(mPlaybackId, playbackState);
        return false;
    }
    mDataAvailable = false;
    playbackThread.join();
    free(mPlayData);

    pa_simple_free(mStream);
    if(playbackState != "stopped")
    {
        playbackState = "stopped";
        PM_LOG_DEBUG(" PlaybackThread::stop playbackState = %s", playbackState.c_str());
        playbackStatusChanged(mPlaybackId, playbackState);
    }
    return true;
}

std::string PlaybackThread::getPlaybackStatus()
{
    return playbackState;
}

void PlaybackThread::playbackStatusChanged(std::string playbackID, std::string state)
{
   mCallback->callBackPlaybackStatusChanged(playbackID, state);
}
