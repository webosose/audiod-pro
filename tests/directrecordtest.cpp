// Copyright (c) 2012-2018 LG Electronics, Inc.
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


#include <glib.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include "log.h"
#include "messageUtils.h"
#include "ConstString.h"
#include "IPC_SharedAudiodDefinitions.h"

using namespace std;

static GMainLoop *      gMainLoop = NULL;
static LSPalmService *  gPalmService = NULL;

void
term_handler(int signal)
{
    g_main_loop_quit(gMainLoop);
}

unsigned int gSamplerate = 8000;

class Capturer {
public:
    Capturer() : mVoiceCommandActive(false), mBT(false), mCapturing(false)
    {
    }

    void    nextAction()
    {
        if (mVoiceCommandActive)
        {
            if (!mCapturing)
            {
                const char * name = "voiceCommand_named_pipe";
                mFileName = std::string("/tmp/") + name;
                if ((mkfifo(mFileName.c_str(), 
                        S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) < 0) && (errno != EEXIST))
                {
                    g_critical("Could not create name pipe %x!", errno);
                    unlink(mFileName.c_str());
                }
                else
                {
                    mCapturing = true;
                    pthread_create(&namePipeReaderThread, NULL, &namePipeReaderThreadFunc, this);
                    g_message("Capturer::%s: Starting to record '%s'", __FUNCTION__, mFileName.c_str());
                    mpid=fork();
                    if (mpid==0) {
                        //child
                        int fd=open(mFileName.c_str(), O_WRONLY);
                        if(fd<0) {
                            perror("open()");
                            exit(1);
                        }
                        if(dup2(fd,fileno(stdout))<0) {
                           perror("dup2()");
                           exit(1);
                        }
                        close(fd);
                        char srate[16];
                        snprintf(srate, sizeof(srate), "--rate=%u", gSamplerate);
                        const char* argv[]= {"arecord", srate, "--channels=1", "--format=S16_LE", NULL};
                        execvp("arecord", (char* const*)argv);
                        exit(-1);
                        return;
                    } else {
                    }
                }
            }
        }
        else    // mVoiceCommandActive false
        {
            if (mCapturing)
            {
                g_message("Capturer::%s: Stop recording!", __FUNCTION__);
                if (mpid) {
                    g_message("Capturer::%s: kill %d", __FUNCTION__, mpid);
                    kill(mpid, SIGTERM);
                    mpid = 0;
                }
                mCapturing = false;
                pthread_join(namePipeReaderThread, NULL);
            }
        }
    }

    void    setVoiceCommandActive(bool active, int mic_gain, bool bt)
    {
        if (mBT != bt)
        {
            mBT = bt;
        }
        if (mVoiceCommandActive != active)
        {
            g_debug("Capturer::%s: %s", __FUNCTION__, active ? "ACTIVE" : "off");
            mVoiceCommandActive = active;
            nextAction();
        }
    }
    
    static void *namePipeReaderThreadFunc(void* data)
    {
        Capturer* capture = ((Capturer*)data);
        FILE* f = fopen(capture->mFileName.c_str(), "rb");
        
        int reads = 0;
        while (TRUE) {
            reads=fread(capture->namePipeReaderBuffer, 1, sizeof(capture->namePipeReaderBuffer), f);
            g_message("Pipe thread reads %d", reads);
            if (reads!=sizeof(capture->namePipeReaderBuffer)) break;
        }
        if (feof(f)) {
            g_message("Pipe thread read EOF");
        } else {
            g_warning("Pipe thread read error, code=%x", ferror(f));
        }
        fclose(f);
        return 0;
    }

private:

    bool            mVoiceCommandActive;
    bool            mBT;
    bool            mCapturing;
    int             mpid;
    std::string     mFileName;
    char cmd[256];
    pthread_t       namePipeReaderThread;
    unsigned char namePipeReaderBuffer[8192];
};

static Capturer gCapturer;

static bool
_audiodUpdate(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__))
        return true;

    std::string scenario;
    bool        active;

    if (msg.get("active", active) && msg.get("scenario", scenario))
    {
        ConstString name(scenario.c_str());
        ConstString tail;
        if (name.hasPrefix(VOICE_COMMAND_, tail))
        {
            int mic_gain;
            if (!msg.get("mic_gain", mic_gain))
                mic_gain = -1;
            gCapturer.setVoiceCommandActive(active, mic_gain, tail == SCENARIO_BLUETOOTH_SCO);
        }
    }

    return true;
}

static bool
_audiodStatus(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    static LSMessageToken sKeysAudiodSessionToken = 0;

    bool result;
    CLSError lserror;

    if (connected)
    {
        // we have a winner!
        g_message ("%s: connected to '%s' server", __FUNCTION__, serviceName);
        result = LSCall(sh, "luna://com.palm.audio/voiceCommand/status", "{\"subscribe\":true}", _audiodUpdate, ctx, &sKeysAudiodSessionToken, &lserror);
        if (!result)
            lserror.Print(__FUNCTION__, __LINE__);
    }
    else
    {
        // we lost it :-(
        g_warning("%s: lost connection to key server", __FUNCTION__);
        if (sKeysAudiodSessionToken)
        {
            result = LSCallCancel (sh, sKeysAudiodSessionToken, &lserror);
            if (!result)
                lserror.Print(__FUNCTION__, __LINE__);
            sKeysAudiodSessionToken = 0;
        }
    }
    return true;
}

static bool initialSetup(const char * exePath, const char * serviceName, bool forceSetup, bool restart)
{
    char    pub[PATH_MAX];
    char    prv[PATH_MAX];
    snprintf(pub, PATH_MAX, "/usr/share/ls2/roles/pub/%s.json", serviceName);
    snprintf(prv, PATH_MAX, "/usr/share/ls2/roles/prv/%s.json", serviceName);

    if (forceSetup || !g_file_test(pub, G_FILE_TEST_EXISTS) || !g_file_test(prv, G_FILE_TEST_EXISTS))
    {
        g_warning("Creating security files for exe '%s' as service '%s'. Will restart device...", exePath, serviceName);

        const char * security =
            "{\n"
            "   \"role\": {\n"
            "       \"exeName\": \"%s\",\n"
            "       \"type\": \"regular\",\n"
            "       \"allowedNames\": [\"%s\"]\n"
            "   },\n"
            "   \"permissions\": [\n"
            "       {\n"
            "           \"service\": \"%s\",\n"
            "           \"inbound\": [\"*\"],\n"
            "           \"outbound\": [\"*\"]\n"
            "       }\n"
            "   ]\n"
            "}\n";

        FILE * file = fopen(pub, "w");
        if (VERIFY(file))
        {
            fprintf(file, security, exePath, serviceName, serviceName);
            fclose(file);
        }
        file = fopen(prv, "w");
        if (VERIFY(file))
        {
            fprintf(file, security, exePath, serviceName, serviceName);
            fclose(file);
        }

        if (restart)
        {
            // enable voice command global preference. Since we don't have security clearance at this point, we have to use luna-send to do it for us!
            system("/usr/bin/luna-send -n 1 palm://com.palm.systemservice/setPreferences '{\"enableVoiceCommand\":true}'");

            // reboot device
            execlp("reboot", "", NULL);
            exit(0);
        }
        return true;
    }
    return false;
}

int main(int argc, char **argv)
{
    signal(SIGTERM, term_handler);
    signal(SIGINT, term_handler);

    setProcessName(argv[0]);

    setLogLevel(G_LOG_LEVEL_DEBUG);
    addLogDestination(eLogDestination_Terminal);
    g_log_set_default_handler(logFilter, NULL);

    bool doInitialSetup = false;
    int opt;
    while ((opt = getopt(argc, argv, "gdhsr:")) != -1)
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
        case 's':
            doInitialSetup = true;
            break;
        case 'r':
            gSamplerate = atoi(optarg);
            g_message("Sample Rate: %u", gSamplerate);
            break;
        case 'h':
        default:
            g_debug(argv[0]);
            return 0;
        }
    }

    g_message("Starting Voice Command Test");

#define THIS_TEST "directrecord"
#define SERVICE_NAME(x) "com.palm." x

    // Verify presence of security files. If missing, create for all test apps at once to save time...
    initialSetup("/usr/sbin/" THIS_TEST, SERVICE_NAME(THIS_TEST), doInitialSetup, true);

    GMainContext * context = g_main_context_new();
    gMainLoop = g_main_loop_new(context, FALSE);

    CLSError lserror;
    if (!LSRegisterPalmService(SERVICE_NAME(THIS_TEST), &gPalmService, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__, G_LOG_LEVEL_CRITICAL);
        g_critical("Could not register service as '%s'.\nMaybe the service security files need to be recreated, which you can force that by starting this program using the '-s' argument.", SERVICE_NAME(THIS_TEST));

        return -1;
    }
    if (!LSGmainAttachPalmService(gPalmService, gMainLoop, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__, G_LOG_LEVEL_CRITICAL);
        return -1;
    }

    LSHandle * sh = LSPalmServiceGetPrivateConnection(gPalmService);
    bool result = LSRegisterServerStatus(sh, "com.palm.audio", _audiodStatus, gMainLoop, &lserror);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        return -1;
    }

    // Enable mic gain adjustments while recording
    if (!LSCall(sh, "luna://com.palm.audio/state/set", "{\"adjustMicGain\":true}", NULL, NULL, NULL, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    g_main_loop_run (gMainLoop);

    return 0;
}
