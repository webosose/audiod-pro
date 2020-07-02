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

#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <glib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sstream>

#include <lunaservice.h>

#include "log.h"
#include "media.h"
#include "state.h"
#include "AudioDevice.h"
#include "AudioMixer.h"
#include "utils.h"
#include "messageUtils.h"
#include "MixerInit.h"
#include "main.h"
#include "moduleInitializer.h"

#define CONFIG_DIR_PATH "/etc/palm/audiod"

#define str(s)      # s
#define xstr(s)     str(s)

#if AUTOBUILD
#define BUILD_INFO ""
#else
#define BUILD_INFO ", locally-built on " __DATE__ " at " __TIME__
#endif

static GMainLoop * gMainLoop = NULL;
static LSHandle *gLSHandle = NULL;

static const char* const logContextName = "AudioD";
static const char* const logPrefix= "[audiod]";

void
term_handler(int signal)
{
    g_main_loop_quit(gMainLoop);
}

void
PrintUsage(const char* progname)
{
    printf("%s:\n", progname);
    printf(" -h this help screen\n"
           " --version version information about this binary\n"
           " -r <directory> specify the directory where register files live\n"
           " -d turn debug-level logging on and send logs to the terminal\n"
            " -f send all log entries to audio's private log file\n"
            " -t send all log entries to the terminal\n"
           " -g turn debug logging on and use the system log (only)\n"
           " -s N sleep N milliseconds & quit\n"
           " -n <priority> set the priority level\n");
}

GMainContext *
GetMainLoopContext()
{
    return g_main_loop_get_context(gMainLoop);
}

//return palmservice handle of "com.webos.service.audio"
LSHandle *
GetPalmService()
{
    return gLSHandle;
}


bool RegisterPalmService()
{
    CLSError lserror;
    bool retVal;
    retVal = LSRegister(AUDIOD_SERVICE_PATH, &gLSHandle, &lserror);
    if(retVal)
    {
        if(!LSGmainAttach(gLSHandle, gMainLoop, &lserror))
        {
          lserror.Print(__FUNCTION__, __LINE__, G_LOG_LEVEL_CRITICAL);
          return false ;
        }
    }
    else
    {
        lserror.Print(__FUNCTION__, __LINE__, G_LOG_LEVEL_CRITICAL);
        lserror.Free();
    }

    return retVal;
}


int
main(int argc, char **argv)
{
    int opt;
    int niceme = 0;

    signal(SIGTERM, term_handler);
    signal(SIGINT, term_handler);

    if (argc > 1 && strcmp(argv[1], "--version") == 0)
    {
        //printf("audiod-" xstr(AUDIOD_SUBMISSION) "-%c%s.\n",
                                         //*gAudioDevice.getName(), BUILD_INFO);
        return 0;
    }

    setProcessName(argv[0]);

    while ((opt = getopt(argc, argv, "hdr:n:gfts:")) != -1)
    {
        switch (opt)
        {
        case 'g':
            setLogLevel(G_LOG_LEVEL_DEBUG);
            break;
        case 'd':
            setLogLevel(G_LOG_LEVEL_DEBUG);
            addLogDestination(eLogDestination_Terminal);
            break;
        case 'f':
            addLogDestination(eLogDestination_PrivateLogFiles);
            break;
        case 't':
            addLogDestination(eLogDestination_Terminal);
            break;
        case 'r':
            gAudioDevice.setConfigRegisterSetDirectory(optarg);
            break;
        case 'n':
            niceme = atoi(optarg);
            break;
        // simple sleep in ms, not available on device
        case 's':
            usleep(atoi(optarg) * 1000);
            return 0;
        case 'h':
        default:

            PrintUsage(argv[0]);
            return 0;
        }
    }

    g_log_set_default_handler(logFilter, NULL);

    PmLogErr error = setPmLogContext(logContextName);
    if (error != kPmLogErr_None)
    {
        std::cerr << logPrefix << "Failed to setup up pmlog context " << logContextName << std::endl;
        exit(EXIT_FAILURE);
    }

    //PM_LOG_INFO(MSGID_STARTUP, INIT_KVCOUNT, "Starting audiod-" xstr(AUDIOD_SUBMISSION) "-%c%s.",
                                          //*gAudioDevice.getName(), BUILD_INFO);

    setpriority(PRIO_PROCESS,getpid(),niceme);

    // Initialized audiod shared properties. Do not use them before this point!
    gState.init();

    // Initialize HW and verify, but before all registered inits,
    // except for static initializations & shared properties.
    VERIFY(gAudioDevice.pre_init());

    gMainLoop = g_main_loop_new(NULL, FALSE);

    /**
     *  initialize the lunaservice and we want it before all the init
     *  stuff happening.
     */
    if(RegisterPalmService() == false)
        return -1;
    PM_LOG_INFO(MSGID_STARTUP, INIT_KVCOUNT, "Register [com.webos.service.audio] Successful");


    std::stringstream configPath;
    configPath << CONFIG_DIR_PATH << "/" << "mixerconfig.json";
    MixerInit mObjMixerInit(configPath);
    if(mObjMixerInit.readMixerConfig())
    {
        mObjMixerInit.initMixerInterface();
    }
    else
    {
        PM_LOG_ERROR(MSGID_STARTUP, INIT_KVCOUNT, "Could not reaad mixer config json file");
    }
    std::stringstream moduleConfigPath;
    moduleConfigPath << CONFIG_DIR_PATH << "/" << "audiod_module_config.json";
    ModuleInitializer mObjModuleInit(moduleConfigPath);
    if (mObjModuleInit.registerAudioModules())
        g_message("audio modules registered successfully");
    else
        g_error("could not register audio modules");
    oneInitForAll (gMainLoop, GetPalmService());
    // Verify HW initialization, but after all registered inits,
    // static initializations & shared properties.
    VERIFY(gAudioDevice.post_init());
    PM_LOG_INFO(MSGID_STARTUP, INIT_KVCOUNT, "Starting main loop!");
    g_main_loop_run(gMainLoop);

    g_main_loop_unref(gMainLoop);

    oneFreeForAll();

    PM_LOG_INFO(MSGID_SHUTDOWN, INIT_KVCOUNT, "audiod terminated");

    exit(0);
}
