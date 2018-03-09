#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <sys/ioctl.h>

#include "audiod_test.h"
//#include <alsa/asoundlib.h>
#include <sound/asound.h>

#define LOGI(...)      fprintf(stdout, __VA_ARGS__) && fprintf(stdout, "\n" )
#define LOGE(...)      fprintf(stdout, __VA_ARGS__) && fprintf(stdout, "\n" )
#define LOGV(...)      fprintf(stdout, __VA_ARGS__) && fprintf(stdout, "\n" )

#define DEFAULT_BUF_SIZE 2048

int main (void)
{
    snd_use_case_mgr_t *ucMgr;
    char *use_case = NULL;
    int b = 1;
    int ret = 0;
    int bufsize = DEFAULT_BUF_SIZE;
    struct alsa_handle_t *handle = NULL;

    snd_use_case_mgr_open (&ucMgr, "snd_soc_msm_2x");
    if (!ucMgr)
    {
        LOGV ("Cannot open UCM config file: %s\b", "snd_soc_msm_2x");
        exit(1);
    }

    snd_use_case_get (ucMgr, "_verb", &use_case);
    if (use_case == NULL)
    {
        LOGV ("No use case is set... Trying to set Inactive\n");
        snd_use_case_set (ucMgr, "_verb", "Inactive");
    }
    if (use_case)
        free (use_case);

    snd_use_case_set (ucMgr, "_verb", "Voice Call");
    snd_use_case_set (ucMgr, "_enadev ", "Voice Earpiece");
    snd_use_case_set (ucMgr, "_enadev", "Handset");

    //snd_use_case_mgr_close (ucMgr);

    handle = (struct alsa_handle_t *)malloc(sizeof(struct alsa_handle_t));

    memset (handle, 0x00, sizeof *handle);
    for (b = 1; (~b & bufsize) != 0; b <<= 1)
        bufsize &= ~b;

    printf ("Bufer size: %d\n", bufsize);
    /*
    handle->format = SNDRV_PCM_FORMAT_S16_LE;
    handle->channels  = 1;
    handle->sampleRate = 8000;
    handle->latency = 85333;
    handle->rxHandle = 0;
    handle->ucMgr = 0;
    */

    handle->channels  = 1;
    handle->sampleRate = 8000;
    handle->latency = 0;
    handle->rxHandle = 0;
    handle->ucMgr = 0;

    ret = startVoiceCall(handle);

    while(1)
        sleep(5);

    return 0;
}

int startVoiceCall(struct alsa_handle_t *handle)
{
    char* devName = NULL;
    unsigned flags = 0;
    int err = 0;

    flags = PCM_OUT | PCM_MONO ;

    devName = (char *)calloc(50, sizeof(char));
    strcpy(devName, "hw:0,2");

    handle->handle = pcm_open(flags | DEBUG_ON, (char*)devName);
    if (!handle->handle) {
        printf("------- device opened failed ----------------\n\n");
        goto Error;
    }

    printf(" OUTPUT device opened .........................\n");

    handle->handle->flags = flags;
    err = setHardwareParams(handle);
    if(err !=  NO_ERROR) {
        printf("------- startVoiceCall: setHardwareParams failed -------\n\n ");
        goto Error;
    }

    printf(" OUTPUT hardware parameters set ............................. \n\n");

    err = setSoftwareParams(handle);
    if(err !=  NO_ERROR) {
        LOGE("startVoiceCall: setSoftwareParams failed");
        goto Error;
    }

    printf("OUTPUT software parameters set ............................. \n\n");

    err = pcm_prepare(handle->handle);
    if(err) {
        LOGE("startVoiceCall: pcm_prepare failed");
        goto Error;
    }

    printf("OUTPUT pcm_prepare set ............................. \n\n");

    if (ioctl(handle->handle->fd, SNDRV_PCM_IOCTL_START)) {
        LOGE("startVoiceCall:SNDRV_PCM_IOCTL_START failed\n");
        goto Error;
    }

    // Store the PCM playback device pointer in rxHandle
    handle->rxHandle = handle->handle;

    // Open PCM capture device
    flags = PCM_IN | PCM_MONO;
    if (deviceName(handle, flags, &devName) < 0) {
        LOGE("Failed to get pcm device node");
        goto Error;
    }
    handle->handle = pcm_open(flags | DEBUG_ON, (char*)devName);
    if (!handle->handle) {
        free(devName);
        goto Error;
    }

    printf(" INPUT device opened .........................\n");

    handle->handle->flags = flags;
    err = setHardwareParams(handle);
    if(err !=  NO_ERROR ) {
        LOGE("startVoiceCall: setHardwareParams failed");
        goto Error;
    }

    printf(" INPUT hardware parameters set ............................. \n\n");

    err = setSoftwareParams(handle);
    if(err !=  NO_ERROR) {
        LOGE("startVoiceCall: setSoftwareParams failed");
        goto Error;
    }

    printf("INPUT software parameters set ............................. \n\n");

    err = pcm_prepare(handle->handle);
    if(err ) {
        LOGE("startVoiceCall: pcm_prepare failed");
        goto Error;
    }

    printf("INPUT pcm_prepare set ............................. \n\n");

    if (ioctl(handle->handle->fd, SNDRV_PCM_IOCTL_START)) {
        LOGE("startVoiceCall:SNDRV_PCM_IOCTL_START failed\n");
        goto Error;
    }

    free(devName);
    return NO_ERROR;

Error:
    printf("------------ ERROR -------------------------------\n\n");
    free(devName);
    close(handle);
    return NO_INIT;
}

int deviceName(struct alsa_handle_t *handle, unsigned flags, char **value)
{
   /* int ret = 0;
    char ident[70];
    char *rxDevice, useCase[70];

    if (flags & PCM_IN) {
        strlcpy(ident, "CapturePCM/", sizeof(ident));
    } else {
        strlcpy(ident, "PlaybackPCM/", sizeof(ident));
    }
    if((!strncmp(handle->useCase,SND_USE_CASE_VERB_MI2S,
                          strlen(SND_USE_CASE_VERB_MI2S)) ||
        !strncmp(handle->useCase,SND_USE_CASE_MOD_PLAY_MI2S,
                   strlen(SND_USE_CASE_MOD_PLAY_MI2S))) && !(flags & PCM_IN) ) {
        rxDevice = getUCMDevice(handle->devices, 0);
        strlcpy(useCase,handle->useCase,sizeof(handle->useCase)+1);
        if(rxDevice) {
           strlcat(useCase,rxDevice,sizeof(useCase));
        }
        strlcat(ident, useCase, sizeof(ident));
        free(rxDevice);
    } else {
        strlcat(ident, handle->useCase, sizeof(ident));
    }
    ret = snd_use_case_get(handle->ucMgr, ident, (const char **)value);
    LOGD("Device value returned is %s", (*value));
    return ret;
    */

    return 0;
}

/*char* ALSADevice::getUCMDevice(uint32_t devices, int input)
{
    return SND_USE_CASE_DEV_HANDSET;

}*/

int setHardwareParams(struct alsa_handle_t *handle)
{
    struct snd_pcm_hw_params *params;
    //unsigned long bufferSize, reqBuffSize;
    //unsigned int requestedRate = handle->sampleRate;
    //int format = handle->format;

    //reqBuffSize = handle->bufferSize;

    params = (struct snd_pcm_hw_params*) calloc(1, sizeof(struct snd_pcm_hw_params));
    if (!params) {
        printf("----------- NO INIT ERROR -------------\n");
        return NO_INIT;
    }

    strcpy(handle->useCase, "Voice Call");

    param_init(params);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
               SNDRV_PCM_ACCESS_RW_INTERLEAVED);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
                   SNDRV_PCM_FORMAT_S16_LE);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                   SNDRV_PCM_SUBFORMAT_STD);

   // LOGV("hw params -  before hifi2 condition %s", handle->useCase);

   /* if ((!strncmp(handle->useCase, SND_USE_CASE_VERB_HIFI2,
                           strlen(SND_USE_CASE_VERB_HIFI2))) ||
        (!strncmp(handle->useCase, SND_USE_CASE_MOD_PLAY_MUSIC2,
                           strlen(SND_USE_CASE_MOD_PLAY_MUSIC2)))) {*/

   /* int ALSAbufferSize = getALSABufferSize(handle);*/


    //param_set_int(params, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, MULTI_CHANNEL_MAX_PERIOD_SIZE);
    param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 2048);
    param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS, handle->channels *16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS, handle->channels);

    param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, handle->sampleRate);

    if(param_set_hw_refine(handle->handle, params)){
        printf("----------params NOT REFINED ------------- \n\n");
        return NO_INIT;
    }

    if (param_set_hw_params(handle->handle, params)) {
        printf("----------cannot set hw params------------- \n\n");
         while (1)
            sleep (100000);
        return NO_INIT;
    }

    printf("set hardware parameter done ......................\n\n");
    param_dump(params);

    handle->handle->buffer_size = pcm_buffer_size(params);

    handle->handle->period_size = pcm_period_size(params);

    handle->handle->period_cnt = handle->handle->buffer_size/handle->handle->period_size;

    handle->handle->rate = 8000;

    handle->handle->channels = 2;
    handle->periodSize = handle->handle->period_size;

    handle->bufferSize = handle->handle->period_size;

   /* if ((!strncmp(handle->useCase, SND_USE_CASE_VERB_HIFI2,
                           strlen(SND_USE_CASE_VERB_HIFI2))) ||
        (!strncmp(handle->useCase, SND_USE_CASE_MOD_PLAY_MUSIC2,
                           strlen(SND_USE_CASE_MOD_PLAY_MUSIC2)))) {
        handle->latency += (handle->handle->period_cnt * PCM_BUFFER_DURATION);
    }*/

    return NO_ERROR;
}

int setSoftwareParams(struct alsa_handle_t *handle)
{
    struct snd_pcm_sw_params* params;
    struct pcm* pcm = handle->handle;

    unsigned long periodSize = pcm->period_size;

    params = (struct snd_pcm_sw_params*) calloc(1, sizeof(struct snd_pcm_sw_params));
    if (!params) {
        return NO_INIT;
    }

    // Get the current software parameters
    params->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
    params->period_step = 1;
    if(((!strncmp(handle->useCase,SND_USE_CASE_MOD_PLAY_VOIP,
                            strlen(SND_USE_CASE_MOD_PLAY_VOIP))) ||
        (!strncmp(handle->useCase,SND_USE_CASE_VERB_IP_VOICECALL,
                            strlen(SND_USE_CASE_VERB_IP_VOICECALL))))){
          printf("setparam:  start & stop threshold for Voip \n\n");
          params->avail_min = handle->channels - 1 ? periodSize/4 : periodSize/2;
          params->start_threshold = periodSize/2;
          params->stop_threshold = INT_MAX;
     } else {
         params->avail_min = periodSize/2;
         params->start_threshold = handle->channels - 1 ? periodSize/2 : periodSize/4;
         params->stop_threshold = INT_MAX;
     }
    /*if (!strncmp(handle->useCase, SND_USE_CASE_VERB_HIFI_TUNNEL,
                           strlen(SND_USE_CASE_VERB_HIFI_TUNNEL)) ||
        (!strncmp(handle->useCase, SND_USE_CASE_MOD_PLAY_TUNNEL,
                           strlen(SND_USE_CASE_MOD_PLAY_TUNNEL)))) {
        params->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
        params->period_step = 1;
        params->xfer_align = (handle->handle->flags & PCM_MONO) ?
            handle->handle->period_size/2 : handle->handle->period_size/4;
    }*/
    params->silence_threshold = 0;
    params->silence_size = 0;

    if (param_set_sw_params(handle->handle, params)) {
        LOGE("cannot set sw params");
        return NO_INIT;
    }
    return NO_ERROR;
}
