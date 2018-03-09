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


#include <boost/shared_ptr.hpp>

#include <glib.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include <media/LunaConnector.h>
#include <media/MediaClient.h>
#include <media/MediaCaptureV3.h>

#include "log.h"
#include "messageUtils.h"
#include "ConstString.h"
#include "IPC_SharedAudiodDefinitions.h"

using namespace media;
using namespace boost;
using namespace std;

static GMainLoop *		gMainLoop = NULL;
static LSPalmService *	gPalmService = NULL;

void
term_handler(int signal)
{
    g_main_loop_quit(gMainLoop);
}

#define SEGMENT_COUNT 20
#define STR(x) #x
#define STR_SEGMENT_COUNT STR(SEGMENT_COUNT)

#define LEVEL_0 0.05f
#define LEVEL_1 0.2f
#define LEVEL_2 0.5f
#define LEVEL_3 0.8f
#define LEVEL_4 0.999f

#define LEVEL_COUNT 6	// 0 to 5, 0 nearly nothing, 5 clipped
#define LEVEL(x) ((x) < LEVEL_2 ? ((x) < LEVEL_1 ? ((x) < LEVEL_0 ? 0 : 1) : 2) : ((x) < LEVEL_3 ? 3 : ((x) < LEVEL_4 ? 4 : 5)))

#define COLORESCAPE		"\033["

#define RESETCOLOR		COLORESCAPE "0m"

#define BOLDCOLOR		COLORESCAPE "1m"
#define REDOVERBLACK	COLORESCAPE "1;31m"
#define BLUEOVERBLACK	COLORESCAPE "1;34m"
#define YELLOWOVERBLACK	COLORESCAPE "1;33m"

const float cMaxDuration = 600.f;
unsigned long int gSamplerate = 8000;

void	interpretLevel(float inRawLevel, int & outSegments, int & outLevel)
{
	outSegments = inRawLevel * float(SEGMENT_COUNT) + 0.1f;
	if (outSegments > SEGMENT_COUNT)
		outSegments = SEGMENT_COUNT;
	else if (outSegments < 0)
		outSegments = 0;
	outLevel = LEVEL(inRawLevel);
}

#define VOLUME_KEY_STEP 11
#define PERCENT_TO_TIC(percent) int((int(percent) + 6) / VOLUME_KEY_STEP)
const int cVolumePercent_Min = 0;
const int cVolumePercent_Max = 100;

int SCVolumeConvertPercentToTic(int percent)
{
    if (percent < cVolumePercent_Min)
    	percent = cVolumePercent_Min;
    else if (percent > cVolumePercent_Max)
    	percent = cVolumePercent_Max;

    return PERCENT_TO_TIC(percent);
}

class Capturer: public LunaConnector, public MediaCaptureV3ChangeListener {
public:
	Capturer() : mRecordingCount(0), mMicGain(-1), mConnected(false), mLoading(false), mVoiceCommandActive(false), mCapturing(false), mBT(false)
	{
		mLoadOptions = boost::shared_ptr<LoadOptions>(new LoadOptions());
		mLoadOptions->imageCaptureFormat = shared_ptr<CaptureFormat>(new CaptureFormat());
		mLoadOptions->videoCaptureFormat = shared_ptr<CaptureFormat>(new CaptureFormat());

		clearStats();
	}

	void start()
	{
		mLunaMediaCapture = media::MediaClient::createLunaMediaCapture(*this);
        mLunaMediaCapture->addMediaCaptureV3ChangeListener(this);
	}

	/**
	 * Called by the media object when a connection has been made.
	 */
	virtual void connected()
	{
		g_debug("Capturer::%s", __FUNCTION__);
		mConnected = true;
		nextAction();
	}

	virtual void captureDevicesChanged()
	{
		std::vector< boost::shared_ptr<CaptureDevice> > devices = mLunaMediaCapture->getCaptureDevices();
		bool foundInput = false;
		for (size_t deviceIdx = 0; !foundInput && (deviceIdx < devices.size()); ++deviceIdx)
		{
			const CaptureDevice & device = *devices[deviceIdx];
			for (size_t inputIdx = 0; !foundInput && (inputIdx < devices[deviceIdx]->inputtype.size()); ++inputIdx)
			{
				if (INPUT_TYPE_AUDIO == device.inputtype[inputIdx]){
					mLoadOptions->deviceUri = device.deviceUri;
					foundInput = true;
				}
			}
		}
		if (!foundInput)
		{
			g_warning("No audio device found! Using fall-back audio device...");
			mLoadOptions->deviceUri = "audio:";
		}
		if (mBT)
			mLoadOptions->deviceUri = "sco:";
		g_debug("Using audiod device '%s'", mLoadOptions->deviceUri.c_str());
		nextAction();
	}

	virtual void supportedAudioFormatsChanged()
	{
		/*
		 * Look for a capture format that's raw audio at 16kHz
		 */
		std::vector< boost::shared_ptr<CaptureFormat> > formats = mLunaMediaCapture->getSupportedAudioFormats();
		bool foundFormat = false;
		for (size_t formatIdx = 0; !foundFormat && (formatIdx < formats.size()); ++formatIdx)
		{
			if (formats[formatIdx])
			{
				const CaptureFormat & format = *(formats[formatIdx]);
				if ( (format.mimetype == "audio/vnd.wave") && (format.samplerate == gSamplerate) )
				{
					mLoadOptions->audioCaptureFormat = formats[formatIdx];
					foundFormat = true;
				}
			}
		}
		if (!foundFormat)
		{
			g_warning("No audio format found! Using fall-back format...");
			mLoadOptions->audioCaptureFormat = shared_ptr<CaptureFormat>(new CaptureFormat());
			mLoadOptions->audioCaptureFormat->mimetype = "audio/vnd.wave";
			mLoadOptions->audioCaptureFormat->bitrate = 256000;
			mLoadOptions->audioCaptureFormat->samplerate = gSamplerate;
		}
		g_debug("Audio format: '%s', Sample Rate: %lu, Bitrate: %lu", mLoadOptions->audioCaptureFormat->mimetype.c_str(), mLoadOptions->audioCaptureFormat->samplerate, mLoadOptions->audioCaptureFormat->bitrate);
		nextAction();
	}

	/**
	 * Called by the media object to create a connection to the luna bus.
	 */
	virtual LSPalmService* connectToBus()
	{
		g_debug("Capturer::%s", __FUNCTION__);

	    return gPalmService;
	}

	virtual void readyChanged()
	{
		g_debug("Capturer::%s", __FUNCTION__);
		nextAction();
	};

	void	nextAction()
	{
		if (mConnected)
		{
			if (mLoading)
			{
				if (mVoiceCommandActive)
				{
					if (mLunaMediaCapture->getReady() && !mCapturing && !mLunaMediaCapture->getAudioCapture())
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
						    pthread_create(&namePipeReaderThread, NULL, &namePipeReaderThreadFunc, this);
							g_message("Capturer::%s: Starting to record '%s'", __FUNCTION__, mFileName.c_str());
							boost::shared_ptr<AudioCaptureOptions> aopts(new AudioCaptureOptions());
							aopts->duration = cMaxDuration;
							aopts->size = aopts->duration * 2 * gSamplerate;	// 2 bytes samples
							// record a new file each time, overwriting previous runs (for now...)
							mLunaMediaCapture->startAudioCapture(mFileName.c_str(), aopts);
							mCapturing = true;
						}
					}
				}
				else	// mVoiceCommandActive false
				{
					if (mCapturing)
					{
						g_message("Capturer::%s: Stop recording!", __FUNCTION__);
						mLunaMediaCapture->stopAudioCapture();
						mCapturing = false;
                        pthread_join(namePipeReaderThread, NULL);
					}
				}
			}
			else	// mLoading false
			{	// don't load before capture device & capture format are set, which will be once the callbacks are called...
				if (!mLoadOptions->deviceUri.empty() && mLoadOptions->audioCaptureFormat)
				{
					mLunaMediaCapture->load(mLoadOptions);
					mLoading = true;
				}
			}
		}
	}

	virtual void elapsedTimeChanged()
	{
		float elapsedTime = mLunaMediaCapture->getElapsedTime();
		g_debug("Capturer::%s: %0.3f sec", __FUNCTION__, elapsedTime);
	}
	virtual void audioCaptureChanged()
	{
		struct stat filestat;
		if (stat(mLunaMediaCapture->getLastAudioPath().c_str(), &filestat) == 0 &&  filestat.st_size == 0)
			unlink(mLunaMediaCapture->getLastAudioPath().c_str());
		nextAction();
	}

	virtual void vuDataChanged()
	{
		const char * rmsMeter =		"********************";
		const char * peakMeter =	"--------------------";
		const char * blankMeter =	"                    ";
		std::vector< boost::shared_ptr<AudioVolume> > volume = mLunaMediaCapture->getVuData();
		if (volume.size() > 0 && volume[0]->rms.size() > 0 && volume[0]->peak.size() > 0)
		{
			int peakSegments, peakLevel, rmsSegments, rmsLevel;
			interpretLevel(volume[0]->peak[0], peakSegments, peakLevel);
			interpretLevel(volume[0]->rms[0], rmsSegments, rmsLevel);
			if (VERIFY(peakSegments >= rmsSegments))
				peakSegments -= rmsSegments;
			else
				peakSegments = 0;
			const char * colorStart = "";
			const char * colorEnd = "";
			if (peakLevel == 5)
			{
				colorStart = REDOVERBLACK;
				colorEnd = RESETCOLOR;
			}
			else if (peakLevel >= 1)
			{
				colorStart = BOLDCOLOR;
				colorEnd = RESETCOLOR;
			}
			g_debug("Capturer::%s: %s%s%s%s%s %d-%d", __FUNCTION__, colorStart, rmsMeter + SEGMENT_COUNT - rmsSegments, peakMeter + SEGMENT_COUNT - peakSegments, colorEnd,
					blankMeter + rmsSegments + peakSegments, rmsLevel, peakLevel);
			if (mCapturing)
			{
				++mStatCount;
				mPeakCount[peakLevel] += 1;
				mRMSCount[rmsLevel] += 1;
			}
		}
	};
	virtual void errorChanged()
	{
		g_debug("Capturer::%s: %d", __FUNCTION__, mLunaMediaCapture->getError());
	};
	void	setVoiceCommandActive(bool active, int mic_gain, bool bt)
	{
		if (mic_gain >= 0)
			mic_gain = SCVolumeConvertPercentToTic(mic_gain);
		if (mic_gain != mMicGain)
		{
			mMicGain = mic_gain;
			mRecordingCount = 0;
			if (mCapturing)
			{
				// if we're changing gain very quickly and the recorded file is short, just get rid of it...
				if (mLunaMediaCapture->getElapsedTime() < 2.0f)
					unlink(mLunaMediaCapture->getCurrentAudioCapturePath().c_str());
				mLunaMediaCapture->stopAudioCapture();
				mCapturing = false;
			}
		}
		if (mBT != bt)
		{
			mBT = bt;
			captureDevicesChanged();
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
	        g_debug("Pipe thread reads %d", reads);
	        if (reads!=sizeof(capture->namePipeReaderBuffer)) break;
	    }
	    if (feof(f)) {
	        g_debug("Pipe thread read EOF");
	    } else {
	        g_error("Pipe thread read error, code=%x", ferror(f));
	    }
	    fclose(f);
	    return 0;
	}

private:

	void	clearStats()
	{
		mStatCount = 0;
		memset(mPeakCount, 0, sizeof(mPeakCount));
		memset(mRMSCount, 0, sizeof(mRMSCount));
	}

	boost::shared_ptr<MediaCaptureV3>	mLunaMediaCapture;
	boost::shared_ptr<LoadOptions>		mLoadOptions;

	int				mPeakCount[LEVEL_COUNT];
	int				mRMSCount[LEVEL_COUNT];
	int				mStatCount;
	int				mRecordingCount;
	int				mMicGain;
	bool			mConnected;
	bool			mLoading;
	bool			mVoiceCommandActive;
	bool			mCapturing;
	bool			mBT;
	std::string		mFileName;
	pthread_t       namePipeReaderThread;
	unsigned char namePipeReaderBuffer[8192];
};

static Capturer gCapturer;

static bool
_audiodUpdate(LSHandle *lshandle, LSMessage *message, void *ctx)
{
	LSMessageJsonParser	msg(message, SCHEMA_ANY);
	if (!msg.parse(__FUNCTION__))
		return true;

	std::string	scenario;
	bool		active;

	if (msg.get("active", active) && msg.get("scenario", scenario))
	{
		ConstString	name(scenario.c_str());
		ConstString	tail;
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
	char 	pub[PATH_MAX];
	char 	prv[PATH_MAX];
	snprintf(pub, PATH_MAX, "/usr/share/ls2/roles/pub/%s.json", serviceName);
	snprintf(prv, PATH_MAX, "/usr/share/ls2/roles/prv/%s.json", serviceName);

	if (forceSetup || !g_file_test(pub, G_FILE_TEST_EXISTS) || !g_file_test(prv, G_FILE_TEST_EXISTS))
	{
		g_warning("Creating security files for exe '%s' as service '%s'. Will restart device...", exePath, serviceName);

		const char * security =
			"{\n"
			"	\"role\": {\n"
			"		\"exeName\": \"%s\",\n"
			"		\"type\": \"regular\",\n"
			"		\"allowedNames\": [\"%s\"]\n"
			"	},\n"
			"	\"permissions\": [\n"
			"		{\n"
			"			\"service\": \"%s\",\n"
			"			\"inbound\": [\"*\"],\n"
			"			\"outbound\": [\"*\"]\n"
			"		}\n"
			"	]\n"
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
			g_message("Sample Rate: %lu", gSamplerate);
			break;
		case 'h':
		default:
			g_debug(argv[0]);
			return 0;
		}
	}

    g_message("Starting Voice Command Test");

#define THIS_TEST "nptest"
#define OTHER_TEST "vctest"
#define SERVICE_NAME(x) "com.palm." x

    // Verify presence of security files. If missing, create for all test apps at once to save time...
   	if (initialSetup("/usr/sbin/" THIS_TEST, SERVICE_NAME(THIS_TEST), doInitialSetup, false))
   		initialSetup("/usr/sbin/" OTHER_TEST, SERVICE_NAME(OTHER_TEST), doInitialSetup, true);

    mkdir("/media/internals/VoiceCommandTest", ACCESSPERMS);

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

    gCapturer.start();

    g_main_loop_run (gMainLoop);

	return 0;
}
