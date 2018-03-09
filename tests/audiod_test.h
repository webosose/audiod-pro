#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sound/asound.h>
#include "alsa_audio.h"

   // ToDo: For now we are depending on 8960, it should be made generic
   #include "msm8960_use_cases.h"


#define NO_ERROR 1
#define NO_INIT 2

struct alsa_handle_t {
    //alsa_device_t *     module;
    uint32_t            devices;
    char                useCase[MAX_STR_LEN];
    struct pcm *        handle;
    snd_pcm_format_t    format;
    uint32_t            channels;
    uint32_t            sampleRate;
    unsigned int        latency;         // Delay in usec
    unsigned int        bufferSize;      // Size of sample buffer
    unsigned int        periodSize;
    struct pcm *        rxHandle;
    snd_use_case_mgr_t  *ucMgr;
	
#ifdef LGE_COMPRESSED_PATH //keyman.kim@lge.com
    uint32_t            CompressedConnectFlag;
    uint32_t            CompressedSampleRate;
#endif //keyman.kim@lge.com	
};
    
