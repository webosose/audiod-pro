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
#include <cstdlib>
#include <cstring>
#include <glib.h>

#include <lunaservice.h>
#include "media.h"
#include "module.h"
#include "state.h"
#include "AudioMixer.h"
#include "volume.h"
#include "utils.h"
#include "messageUtils.h"
#include "AudioDevice.h"
#include "log.h"
#include "AudiodCallbacks.h"
#include "main.h"

static MediaScenarioModule * sMediaModule = 0;

#ifdef HAVE_BT_SERVICE_V1
#define A2DP_PLAY "palm://com.palm.bluetooth/a2dp/play"
#define A2DP_PAUSE "palm://com.palm.bluetooth/a2dp/pause"
#else
#define A2DP_PLAY "luna://com.webos.service.bluetooth2/a2dp/internal/startStreaming"
#define A2DP_PAUSE "luna://com.webos.service.bluetooth2/a2dp/internal/stopStreaming"
#endif

MediaScenarioModule * getMediaModule()
{
    return sMediaModule;
}

void
MediaScenarioModule::programControlVolume ()
{
    Scenario *scenario = nullptr;
    if (VERIFY(scenario = dynamic_cast <Scenario*>(mCurrentScenario)))
    {
        programMediaVolumes(false, false, false);
        if (isCurrentModule())
        {
            if (gState.getAdjustMicGain())
                gAudioDevice.setMicGain(mCurrentScenario->mName,
                                        scenario->getMicGainTics());
        }
    }
}


void
MediaScenarioModule::rampDownAndMute()
{
    // only wait for media to ramp down if we
    //indeed ramp down some audible media...
    if (someMediaIsAudible())
    {
        rampVolume (emedia, 0);
        rampVolume (eflash, 0);
        rampVolume (edefaultapp, 0);
        rampVolume (edefault1, 0);
        rampVolume (enavigation, 0);
        usleep(400000);
    }
    setMuted(true);
}

void MediaScenarioModule::setCurrentState(int value)
{
    mA2DPCurrentState = (EA2DP)value;
}

bool MediaScenarioModule::getCurrentState(void)
{
    return mA2DPCurrentState;
}

void MediaScenarioModule::setRequestedState(int value)
{
    mA2DPRequestedState = (EA2DP)value;
}

bool MediaScenarioModule::getRequestedState(void)
{
    return mA2DPRequestedState;
}



void
MediaScenarioModule::programMediaVolumes(bool rampVolumes,
                                         bool rampMedia,
                                         bool muteMedia)
{

    const int cNavigationDuck = -18;    // dB

    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();

    int baseVolume = mCurrentScenario->getVolume();

    int    mediaVolume = !mMuted && activeStreams.contain(emedia) ?
                                                                baseVolume : 0;
    int flashVolume = !mMuted && activeStreams.contain(eflash) ?
                                                                baseVolume : 0;
    int defaultAppVolume = !mMuted && activeStreams.contain(edefaultapp) ?
                                                                baseVolume : 0;
    int default1Volume = !mMuted && activeStreams.contain(edefault1) ?
                                                                baseVolume : 0;
    int navigationVolume = !mMuted && activeStreams.contain(enavigation) ?
                                                                baseVolume : 0;
    int volume_to_set = 0;

    int duckMedia = 0;
    int duckFlash = 0;
    int duckDefaultApp = 0;
    int duckDefault1 = 0;
    int duckNavigation = 0;

    // when navigation is playing, duck other media volumes
    if (activeStreams.contain(enavigation))
    {
        duckMedia = duckFlash = duckDefaultApp = cNavigationDuck;
    }

    // when an alert is playing, duck media volumes not played on the same output
    if (someAlertIsPlaying())
    {
        duckDefaultApp += cAlertDuck;
        if (!_isWirelessStreamingScenario())
        {
               duckMedia += cAlertDuck;
               duckFlash += cAlertDuck;
               duckNavigation += cAlertDuck;
         }
    }

    if (mediaVolume > 0 && duckMedia != 0)
        mediaVolume = gAudioMixer.adjustVolume(mediaVolume, duckMedia);

    if (flashVolume > 0 && duckFlash != 0)
        flashVolume = gAudioMixer.adjustVolume(flashVolume, duckFlash);

    if (defaultAppVolume > 0 && duckDefaultApp != 0)
        defaultAppVolume = gAudioMixer.adjustVolume(defaultAppVolume,
                                                    duckDefaultApp);

    if (navigationVolume > 0 && duckNavigation != 0)
        navigationVolume = gAudioMixer.adjustVolume(navigationVolume,
                                                    duckNavigation);

    // Apply as necessary, with or without ramping

    //ScenarioModule::disableScenario...mute back speaker for media playback
    // if changing to back speaker scenario done here to make sure it doesn't
    // ramp either, cause that would take a while and might still be heard
    if (muteMedia)
    {
        programVolume(emedia, 0, false);
        programVolume(edefaultapp, 0, false);
        programVolume(eflash, 0, false);
        programVolume(enavigation, 0, false);
        programVolume(edefault1, 0, false);
        return;
    }
    else
    {

        if(mPreviousSink == emedia)
            volume_to_set = mediaVolume;
        else if(mPreviousSink == edefaultapp)
            volume_to_set = defaultAppVolume;
        else if(mPreviousSink == edefault1)
            volume_to_set = default1Volume;
        else if(mPreviousSink == eflash)
            volume_to_set = flashVolume;
        else if(mPreviousSink == enavigation)
            volume_to_set = navigationVolume;

        if(-1 != mPreviousSink )
            programVolume(mPreviousSink, volume_to_set, rampVolumes || rampMedia);
    }

    if(mPreviousSink != emedia)
        programVolume(emedia, mediaVolume, rampVolumes);

    if(mPreviousSink != eflash)
        programVolume(eflash, flashVolume, rampVolumes);

    if(mPreviousSink != edefaultapp)
    {
        programVolume(edefaultapp, defaultAppVolume, rampVolumes);
    }

    if(mPreviousSink != edefault1)
        programVolume(edefault1, default1Volume, rampVolumes);

    if(mPreviousSink != enavigation)
        programVolume(enavigation, navigationVolume, rampVolumes);

}

gboolean
MediaScenarioModule::_A2DPDelayedUpdate(gpointer)
{
    MediaScenarioModule * media = getMediaModule();

    if (media->mA2DPRequestedState == eA2DP_Playing) {
        if (gState.getScoUp()) {
            g_debug("Sco is still up, wait til it is down before issuing a2dpPlay");
            return TRUE;
        }
        else
            media->_updateA2DP(true, true);
    } else {
        media->_updateA2DP(false, true);
    }

    return FALSE;
}

bool updatePlayStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__func__)) {
        g_critical("%s : message parsing failed .......", __FUNCTION__);
        return true;
    }
    g_debug("%s : %s", __FUNCTION__, msg.getPayload());

    return true;
}

bool updatePauseStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser msg(message, SCHEMA_ANY);
    if (!msg.parse(__func__)) {
        g_critical("%s : message parsing failed .......", __FUNCTION__);
        return true;
    }
    g_debug("%s : %s", __FUNCTION__, msg.getPayload());

    return true;
}

void
MediaScenarioModule::_updateA2DP(bool play, bool immediate)
{
    CLSError lserror;

    g_debug("_updateA2DP : play = %d immediate = %d", play, immediate);

#ifdef HAVE_BT_SERVICE_V1

    if (mA2DPUpdateTimerID) {
        g_source_remove(mA2DPUpdateTimerID);
        mA2DPUpdateTimerID = 0;
    }

    mA2DPRequestedState = play ? eA2DP_Playing : eA2DP_Paused;

    bool result = true;
    LSHandle * sh = GetPalmService();
    if (!sh) {
        g_critical("ERROR : no LSHandle acquired. aborting\n");
        return;
    }

    if (play) {
        if (immediate) {
            if (mA2DPCurrentState == eA2DP_Paused && (!gState.getOnActiveCall())) {
                //Required to support switch from hfp to a2dp
                g_debug("MediaScenarioModule::_updateA2DP: PLAYING");
                result = LSCallOneReply (sh, A2DP_PLAY,
                                             "{}", updatePlayStatus, NULL, NULL, &lserror);
                gAudioDevice.a2dpStartStreaming();
                mA2DPCurrentState = eA2DP_Playing;
            }
        } else
            mA2DPUpdateTimerID = g_timeout_add (500, _A2DPDelayedUpdate, NULL);
    } else {
        if (immediate || mA2DPCurrentState == mA2DPRequestedState) {
            if (mA2DPCurrentState == eA2DP_Playing) {
                //Required to support switch from a2dp to hfp
                g_debug("MediaScenarioModule::_updateA2DP: PAUSED");
                result = LSCallOneReply (sh, A2DP_PAUSE,
                                             "{}", updatePauseStatus, NULL, NULL, &lserror);
                gAudioDevice.a2dpStopStreaming();
                mA2DPCurrentState = eA2DP_Paused;
            }
        } else
            mA2DPUpdateTimerID = g_timeout_add (2000, _A2DPDelayedUpdate, NULL);
    }

#else

    bool result = true;
    std::string param;
    if (mA2DPUpdateTimerID) {
        g_source_remove(mA2DPUpdateTimerID);
        mA2DPUpdateTimerID = 0;
    }

   mA2DPRequestedState = play ? eA2DP_Playing : eA2DP_Paused;

    LSHandle * sh = GetPalmService();
    if (!sh) {
        g_critical("Error: %s no lshandle received\n", __FUNCTION__);
        return;
    }
    param = string_printf("{\"address\":\"%s\"}", mA2DPAddress.c_str());
    g_debug("MediaScenarioModule::_updateA2DP: params = %s\n", param.c_str());
    if (play) {
        if (immediate) {
            //Required to support switch from hfp to a2dp
            if (mA2DPCurrentState == eA2DP_Paused) {
                g_debug("MediaScenarioModule::_updateA2DP: PLAYING");
                result = LSCallOneReply (sh, A2DP_PLAY,
                                             param.c_str(), updatePlayStatus, NULL, NULL, &lserror);
                mA2DPCurrentState = eA2DP_Playing;
            }
        } else
            mA2DPUpdateTimerID = g_timeout_add (500, _A2DPDelayedUpdate, NULL);
    } else {
        /* will pause immediately */
        if (mA2DPCurrentState == eA2DP_Playing) {
            g_debug("MediaScenarioModule::_updateA2DP: PAUSED");
            result = LSCallOneReply (sh, A2DP_PAUSE,
                                             param.c_str(), updatePauseStatus, NULL, NULL, &lserror);
            mA2DPCurrentState = eA2DP_Paused;
        }
    }

#endif
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);

}

void
MediaScenarioModule::onActivating()
{
    setMuted(false);
}

int
MediaScenarioModule::adjustAlertVolume(int volume, bool alertStarting)
{
    int        adjustedVolume = volume;
    bool    playingMedia = someMediaIsPlaying();
    bool    useHeadsetVolume = playingMedia ||
                               gState.getActiveSinksAtPhoneCallStart(). \
                               containAnyOf(emedia,
                                            eflash,
                                            edefaultapp,
                                            edefault1,
                                            enavigation
                                            );
    if (gState.getHeadsetState() != eHeadsetState_None)
    {    // a headset is connected. Don't blast the user's ear drums!
        int headsetVolume = mHeadsetVolume.get();
        // alert both in back speaker & headset
        if (mPriorityToAlerts || (alertStarting && !useHeadsetVolume))
        {
            // in case media is playing, make sure we match that
            if (adjustedVolume < headsetVolume && useHeadsetVolume)
                adjustedVolume = headsetVolume;
            // possible problem only if the alert/alarm
            //volume is above the headset level
            if (adjustedVolume > headsetVolume)
            {
              // Needs to be loud enough to hear
              //; while not blasting the user's ear drums
                int    maxVolume = 80;// Maximum volume we are willing to use to
                                   // not hurt a user with a quiet headset level
                if (headsetVolume > maxVolume)
                    maxVolume = headsetVolume;// User is listening at a very
                                              // loud level: he may be in a very
                                              // loud environment
                if (adjustedVolume > maxVolume)
                    adjustedVolume = maxVolume;
            }
        }
        else    // note that normally media has to be playing for us
                // to get here, but *we are playing all in the headset only*...
        {        // this can also be called during scenario changes where the
                // volume won't really matter in the end (no alert is really playing)
            if (useHeadsetVolume)
                adjustedVolume = headsetVolume;    // just be sure alerts
                                                // are as loud as the playing media
        }
    }
    else if (playingMedia)
    {
        // if playing over wireless or a2dp adjust volume according to
        // system volume as we are playing alerts over completely different
        // output device and do not mix.
        if (!_isWirelessStreamingScenario() ||
            gAudioMixer.getActiveStreams().contain(edefaultapp))
        {
            int mediaVolume = mCurrentScenario->getVolume();
            adjustedVolume = mediaVolume; // use media volume,
                                          // no matter what the alert volume is
        }
    }

    adjustedVolume = _applyMinVolume(adjustedVolume, volume);

    if (volume != adjustedVolume)
        g_debug("Media::adjustAlertVolume: %d -> %d", volume, adjustedVolume);

    return adjustedVolume;
}

int
MediaScenarioModule::adjustToHeadsetVolume(int volume)
{
    int    adjustedVolume = volume;

    if (gState.getHeadsetState() != eHeadsetState_None)
    {    // a headset is connected. Don't blast the user's ear drums!
        int headsetVolume = mHeadsetVolume.get();
        if (adjustedVolume > headsetVolume)
            adjustedVolume = headsetVolume;

        adjustedVolume = _applyMinVolume(adjustedVolume, volume);

        if (volume != adjustedVolume)
            g_debug("Media::adjustToHeadsetVolume: %d -> %d", volume, adjustedVolume);
    }

    return adjustedVolume;
}

int
MediaScenarioModule::_applyMinVolume(int adjustedVolume, int originalVolume)
{
    int minVolume = 30;                 // min level, to avoid muting
                                        // completely a constrained volume

    if (originalVolume < minVolume)
        minVolume = originalVolume;     // we should not go over the original volume

    if (adjustedVolume < minVolume)
        adjustedVolume = minVolume;

    return adjustedVolume;
}

int
MediaScenarioModule::adjustSystemVolume(int volume)
{
    int adjustedVolume = volume;
    if (someMediaIsPlaying())
    {
        int mediaVolume = mCurrentScenario->getVolume();

        if (adjustedVolume > mediaVolume)
            adjustedVolume = mediaVolume;

        adjustedVolume = _applyMinVolume(adjustedVolume, volume);
    }
    else
    {
        adjustedVolume = ScenarioModule::adjustSystemVolume(volume);
    }

    return adjustedVolume;
}

bool
MediaScenarioModule::someMediaIsPlaying ()
{
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams();

    return activeStreams.containAnyOf(emedia, eflash, edefaultapp, enavigation, edefault1);
}

bool
MediaScenarioModule::someMediaIsAudible ()
{
    return gAudioMixer.isSinkAudible(emedia) ||
           gAudioMixer.isSinkAudible(eflash) ||
           gAudioMixer.isSinkAudible(edefaultapp) ||
           gAudioMixer.isSinkAudible(edefault1) ||
           gAudioMixer.isSinkAudible(enavigation);
}

bool
MediaScenarioModule::someAlertIsPlaying ()
{
    bool playing = false;

    if (gAudioMixer.getActiveStreams().containAnyOf(ealerts,
                                                    enotifications,
                                                    ecalendar,
                                                    eringtones,
                                                    evoicerecognition))
        playing = true;

    return playing || gAudioMixer.isSinkAudible(ealarm);
}

/* to handle ringtone scenario seperately with luna message
and not with pulseaudio events on onSinkChanged() */
bool
MediaScenarioModule::enableRingtoneRouting(bool enable)
{
    if(enable){
        /*to unmute in case volume keys are used in between
        since ringtone sink is always open , pressing vol up/down
        at any point of time mutes ringtone sink*/
        if (gState.getRingerOn())
            programVolume(eringtones, 89, true);
        PriorityToRingtone= true;
    }
    else
        PriorityToRingtone = false;

      /* this call is required here audiod does not
      receive onSinkChanged for ringtone playback
      from second time onwards */
    _updateRouting();

    return true;
}

bool
MediaScenarioModule::_isWirelessStreamingScenario()
{
    return mCurrentScenario && (mCurrentScenario->mName == cMedia_A2DP ||
                                mCurrentScenario->mName == cMedia_Wireless);
}

bool
MediaScenarioModule::_isWirelessStreamActive()
{
    VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams();

    /*Allowing all streams except voicedial and voip streams.These streams
    are played on BT headset either directly(bsaa2dp-sink) or through combined sink*/
    return activeStreams.containAnyOf(emedia, eflash, enavigation, edefaultapp, edefault1)
        || activeStreams.containAnyOf(etts, eringtones, ealarm, etimer, etts1)
        || activeStreams.containAnyOf(ealerts, eeffects, enotifications, efeedback)
        || activeStreams.containAnyOf(eDTMF, ecalendar, ecallertone);
}


void MediaScenarioModule::setA2DPAddress(std::string address)
{
    mA2DPAddress = address;
}

void MediaScenarioModule::resumeA2DP ()
{
    if (_isWirelessStreamingScenario()) {
        if (gState.getScoUp())
            _updateA2DP(true, false);
        else
            _updateA2DP(true, true);
    }
}

void MediaScenarioModule::pauseA2DP ()
{
    if (_isWirelessStreamingScenario()) {
        g_message ("MediaScenarioModule::pauseA2DP");
        _updateA2DP(false, true);
    }
}

void
MediaScenarioModule::programState ()
{
    Scenario *scenario = nullptr;
    _updateRouting();
    if ((scenario = dynamic_cast <Scenario*>(mCurrentScenario))
        && (gState.getAdjustMicGain()))
    {
            gAudioDevice.setMicGain(mCurrentScenario->mName,
                                    scenario->getMicGainTics());
    }

    // update media streaming
    if (_isWirelessStreamingScenario()) {
        if (gState.getScoUp())
            _updateA2DP(true, false);
        else
            _updateA2DP(true, true);
    }
}

void
MediaScenarioModule::_startSinkPlayback (EVirtualAudiodSink sink)
{

    programMediaVolumes(false, false, false);

    if (_isWirelessStreamingScenario() && isWirelessStreamedSink(sink)) {
        if (gState.getScoUp())
            _updateA2DP(true, false);
        else
            _updateA2DP(true, true);
    }
}

void
MediaScenarioModule::_endSinkPlayback (EVirtualAudiodSink sink)
{
    g_debug ("MediaScenarioModule::_endSinkPlayback");
    //TO BE IMPLEMENTED
}

void
MediaScenarioModule::_updateRouting()
{
    g_debug ("_updateRouting");
    if (!isCurrentModule())
    {
        g_debug("- %s !isCurrentModule current is %s", __FUNCTION__,
                     getCurrentScenarioName() );
        return;
    }
    if (mPriorityToAlerts || PriorityToRingtone)
    {
        /*Alerts routings only matter if headphones are inserted,
        otherwise dont change routes!
        change to route ringtone&alerts to both speaker and headset */
       if (gState.getHeadsetState() != eHeadsetState_None)
       {
            g_debug("Media :_updateRouting activating routing for alerts.");
            LogIndent    indentLogs("> ");
            gAudioMixer.setRouting(mCurrentScenario->mName);
            mAlertsRoutingActive = true;
        }
    }
    else
    {
        if (mAlertsRoutingActive)
        {
            g_debug("Media:_updateRouting disabling routing for alerts.");
            LogIndent    indentLogs("> ");
            gAudioMixer.setRouting(mCurrentScenario->mName);
            mAlertsRoutingActive = false;
        }
    }
}

gboolean timerTimeout (gpointer data)
{
    g_message("Timer expired... Calling onSinkChanged for media module when timer sink is closed");
    MediaScenarioModule *mediaModule = (MediaScenarioModule *)data;
    mediaModule->onSinkChanged(etimer, eControlEvent_LastStreamClosed,ePulseAudio);
    return FALSE;
}

gboolean alarmTimeout (gpointer data)
{
    g_message("Timer expired... Calling onSinkChanged for media module when alarm sink is closed");
    MediaScenarioModule *mediaModule = (MediaScenarioModule *)data;
    mediaModule->onSinkChanged(ealarm, eControlEvent_LastStreamClosed,ePulseAudio);
    return FALSE;
}

gboolean ringtoneTimeout (gpointer data)
{
    g_message("Ringtone expired... Calling onSinChanged for media module when ringtone sink is closed");
    MediaScenarioModule *mediaModule = (MediaScenarioModule *)data;
    mediaModule->onSinkChanged(eringtones, eControlEvent_LastStreamClosed,ePulseAudio);
    return FALSE;
}

void MediaScenarioModule::sendAckToPowerd(bool isWakeup)
{

    g_message("setting PowerdSetWakeLock to:%d", isWakeup);
    //TO BE IMPLEMENTED
}

void
MediaScenarioModule::onSinkChanged (EVirtualAudiodSink sink, EControlEvent event, ESinkType p_eSinkType)
{
    g_debug("MediaScenarioModule::onSinkChanged: sink %i-%s %s %d", sink,
    virtualSinkName(sink), controlEventName(event),(int)p_eSinkType);
  if(p_eSinkType==ePulseAudio)
  {
      g_debug("MediaScenarioModule: pulseaudio onsink changed");
      LogIndent    indentLogs("| ");

      if (!VERIFY(mCurrentScenario != 0))
          return;

      bool priorityMedia = someMediaIsPlaying() ||
                gState.getActiveSinksAtPhoneCallStart().containAnyOf(eflash,
                edefaultapp, enavigation, edefault1);
      bool alertIsPlaying = someAlertIsPlaying();

      g_debug("MediaSecnarioModule: priorityMedia = %d alertIsPlaying = %d ",
                   priorityMedia, alertIsPlaying);

      // this is the only time when priority between media & alerts is changed
      if (!alertIsPlaying)
          mPriorityToAlerts = false;
      else if (priorityMedia && (event == eControlEvent_FirstStreamOpened || alertIsPlaying))
          mPriorityToAlerts = true;

      g_debug("MediaSecnarioModule: mPriorityToAlerts = %d ", mPriorityToAlerts);

      _updateRouting();

      VirtualSinkSet activeStreams = gAudioMixer.getActiveStreams ();

      if (emedia == sink || eflash == sink || edefaultapp == sink ||
           enavigation == sink || edefault1 == sink)
      {
          if (eControlEvent_FirstStreamOpened == event)
          {
              mPreviousSink = sink;

              if (!mPriorityToAlerts)
              {
                  _startSinkPlayback(sink);
              }
              else
              {
                  programMediaVolumes(true, false, true);
              }

          }
          else if (eControlEvent_LastStreamClosed == event)
          {
              // if we are in active media routing
              // we need to stop playing
              if (!mPriorityToAlerts)
              {
                      _endSinkPlayback (sink);
              }
              programMediaVolumes(true, false, false);
              mPreviousSink = eVirtualSink_None;

          }
      }
      else if (ealarm == sink ||
                 eringtones == sink ||
                 eeffects ==  sink ||
                 etimer == sink ||/*eeffects added to mute
                                             media during alert playback, etimer is added */
                 etts == sink || /*tts added*/
                 etts1 == sink ||
                 evoicerecognition == sink) /* voicerecogniton added */
      {
          // if this is an open event
          if (eControlEvent_FirstStreamOpened == event)
          {
              /* identified : possible place to send 3rd param=true setting the
              3rd parameter to true enables the code to mute the media in case
              playback through some other sinks are starting */
              //programMediaVolumes(true, false, false);
              programMediaVolumes(true, false, true);
              if(sink == ealarm ){
                  if(alarmTimeoutSource != 0)
                      if(g_source_remove(alarmTimeoutSource)){
                          g_message("Removed timer since new alarm stream is created");
                          alarmTimeoutSource = 0;
                      }
              }
              if(sink == etimer){
                  if(timerTimeoutSource != 0)
                      if(g_source_remove(timerTimeoutSource)){
                          g_message("Removed timer since new timer stream is created");
                          timerTimeoutSource = 0;
                      }
              }
              if (sink == eringtones) {
                  if (mRingtoneTimeoutSource != 0)
                      if (g_source_remove(mRingtoneTimeoutSource)) {
                          g_message("Removed ringtone timer since new ringtone stream is created");
                          mRingtoneTimeoutSource = 0;
                      }
              }
          }
          else if (eControlEvent_LastStreamClosed == event)
          {
              if ((sink == ealarm && !alarmTimeoutSource) || (sink == etimer && !timerTimeoutSource)
                          || (sink == eringtones && !mRingtoneTimeoutSource)) {
                  if (someMediaIsPlaying() || gState.isRecording ()) {
                       g_message("Setting timer for 400ms");
                       if(etimer == sink)
                           timerTimeoutSource = g_timeout_add (400, timerTimeout, this);
                       else if (ealarm == sink)
                           alarmTimeoutSource = g_timeout_add (400, alarmTimeout, this);
                       else
                           mRingtoneTimeoutSource = g_timeout_add (400, ringtoneTimeout, this);
                  }
              }
              else
              {
                  alarmTimeoutSource = 0;
                  timerTimeoutSource = 0;
                  mRingtoneTimeoutSource = 0;
                  if (activeStreams.contain(emedia))
                      _startSinkPlayback (emedia);
                  else if (activeStreams.contain(eflash))
                      _startSinkPlayback (eflash);
                  else if (activeStreams.contain(edefaultapp))
                      _startSinkPlayback (edefaultapp);
                  else if (activeStreams.contain(edefault1))
                      _startSinkPlayback (edefault1);
                  else if (activeStreams.contain(enavigation))
                      _startSinkPlayback (enavigation);
                  else
                  {
                      programMediaVolumes(true, false, false);
                  }
              }
          }
      }
      else if (efeedback ==sink )
      {
          if (eControlEvent_FirstStreamOpened == event)
          {
              if (gState.getOnActiveCall()) {
                  g_message("Sending mute for sink %d", sink);
                  gAudioMixer.programVolume(sink, 0);
              }
          }
      }
      else if (ealerts == sink || enotifications == sink || ecalendar == sink)
      {
          if (eControlEvent_FirstStreamOpened == event) {
              /* When Notification/Alerts/Calender comes in Vibrate/Silent Profile,
                   dont mute media (CCWMP-24809) */
              if (gState.getRingerOn())
                  programMediaVolumes(true, false, true);
          }
          else if (eControlEvent_LastStreamClosed == event) {
              if (activeStreams.contain(emedia))
                  _startSinkPlayback(emedia);
              else if (activeStreams.contain(eflash))
                  _startSinkPlayback(eflash);
              else if (activeStreams.contain(edefaultapp))
                  _startSinkPlayback(edefaultapp);
              else if (activeStreams.contain(edefault1))
                  _startSinkPlayback(edefault1);
              else if (activeStreams.contain(enavigation))
                  _startSinkPlayback(enavigation);
              else
                  programMediaVolumes(true, false, false);
          }
      }
  }
  else
  {
    g_debug("MediaScenarioModule: UMI onsink changed");
  }
}

static LSMethod mediaMethods[] = {
    { cModuleMethod_OffsetVolume, _offsetVolume},
#if defined(AUDIOD_PALM_LEGACY)
    { cModuleMethod_SetMicGain, _setMicGain},
    { cModuleMethod_GetMicGain, _getMicGain},
    { cModuleMethod_OffsetMicGain, _offsetMicGain},
    { cModuleMethod_SetLatency, _setLatency},
    { cModuleMethod_GetLatency, _getLatency},
#endif
    { cModuleMethod_EnableScenario, _enableScenario},
    { cModuleMethod_DisableScenario, _disableScenario},
    { cModuleMethod_SetCurrentScenario, _setCurrentScenario},
    { cModuleMethod_GetCurrentScenario, _getCurrentScenario},
    { cModuleMethod_ListScenarios, _listScenarios},
    { cModuleMethod_BroadcastEvent, _broadcastEvent},
    { cModuleMethod_SetVolume, _setVolume},
    { cModuleMethod_GetVolume, _getVolume},
    { cModuleMethod_Status, _status},
    { },
};

MediaScenarioModule::MediaScenarioModule(int default_volume) :
    ScenarioModule(ConstString("/media")),
    mPriorityToAlerts(false),
    mAlertsRoutingActive(false),
    PriorityToRingtone(false),
    mA2DPCurrentState(eA2DP_Paused),
    mA2DPRequestedState(eA2DP_Paused),
    mA2DPUpdateTimerID(0),
    mFrontSpeakerVolume(cMedia_FrontSpeaker, 50),
    mBackSpeakerVolume(cMedia_BackSpeaker, 45),
    mHeadsetVolume(cMedia_Headset, 50),
    mA2DPVolume(cMedia_A2DP, 70),
    mWirelessVolume(cMedia_Wireless, 50),
    mFrontMicGain(cMedia_Mic_Front, -1),
    mHeadsetMicGain(cMedia_Mic_HeadsetMic, -1),
    alarmTimeoutSource(0),
    timerTimeoutSource(0),
    mRingtoneTimeoutSource(0)
{
    mPreviousSink = eVirtualSink_None;
    g_debug("%s: MediaScenarioModule with arg '%d' volume", __FUNCTION__, default_volume);
}

void MediaScenarioModule::MediaInterfaceExit() {
    delete sMediaModule;
}


MediaScenarioModule::MediaScenarioModule() :
    ScenarioModule(ConstString("/media")),
    mPriorityToAlerts(false),
    mAlertsRoutingActive(false),
    PriorityToRingtone(false),
    mA2DPCurrentState(eA2DP_Paused),
    mA2DPRequestedState(eA2DP_Paused),
    mA2DPUpdateTimerID(0),
    mFrontSpeakerVolume(cMedia_FrontSpeaker, 50),
    mBackSpeakerVolume(cMedia_BackSpeaker, 45),
    mHeadsetVolume(cMedia_Headset, 50),
    mA2DPVolume(cMedia_A2DP, 70),
    mWirelessVolume(cMedia_Wireless, 50),
    mFrontMicGain(cMedia_Mic_Front, -1),
    mHeadsetMicGain(cMedia_Mic_HeadsetMic, -1),
    alarmTimeoutSource(0),
    timerTimeoutSource(0),
    mRingtoneTimeoutSource(0)
{
    mPreviousSink = eVirtualSink_None;

        gAudiodCallbacks.registerModuleCallback (this, eVirtualSink_All);

        Scenario *s = new Scenario (cMedia_BackSpeaker,
                                    true,
                                    eScenarioPriority_Media_BackSpeaker,
                                    mBackSpeakerVolume,
                                    mFrontMicGain);
        //s->setFilter(gAudioDevice.getBackSpeakerFilter());

        //                 class               destination     switch     enabled
        s->configureRoute (ealerts,            eMainSink,     true,      true);
        s->configureRoute (eeffects,           eMainSink,     true,      true);
        s->configureRoute (enotifications,     eMainSink,     true,      true);
        s->configureRoute (efeedback,          eMainSink,     true,      true);
        s->configureRoute (efeedback,          eMainSink,     false,     true);
        s->configureRoute (etts,               eMainSink,     true,      true);
        s->configureRoute (etts,               eMainSink,     false,     true);
        s->configureRoute (eringtones,         eMainSink,     true,      true);
        s->configureRoute (emedia,             eMainSink,     true,      true);
        s->configureRoute (emedia,             eMainSink,     false,     true);
        s->configureRoute (eflash,             eMainSink,     true,      true);
        s->configureRoute (eflash,             eMainSink,     false,     false);
        s->configureRoute (enavigation,        eMainSink,     true,      true);
        s->configureRoute (enavigation,        eMainSink,     false,     true);
        s->configureRoute (edefaultapp,        eMainSink,     true,      true);
        s->configureRoute (edefault1,          eMainSink,     true,      true);
        s->configureRoute (edefault1,          eMainSink,     false,     true);
        s->configureRoute (edefault2,          eMainSink,     true,      true);
        s->configureRoute (edefault2,          eMainSink,     false,     true);
        s->configureRoute (etts1,              eMainSink,     true,      true);
        s->configureRoute (etts1,              eMainSink,     false,     true);
        s->configureRoute (etts2,              eMainSink,     true,      true);
        s->configureRoute (etts2,              eMainSink,     false,     true);
        s->configureRoute (eDTMF,              eMainSink,     true,      true);
        s->configureRoute (eDTMF,              eMainSink,     false,     true);
        s->configureRoute (ecalendar,          eMainSink,     true,      true);
        s->configureRoute (ealarm,             eMainSink,     true,      true);
        s->configureRoute (ealarm,             eMainSink,     false,     true);
        s->configureRoute (etimer,             eMainSink,     true,     true);
        s->configureRoute (etimer,             eMainSink,     false,     true);
        s->configureRoute (evoicedial,         eMainSink,     true,      true);
        s->configureRoute (evoicedial,         eMainSink,     false,     true);
        s->configureRoute (evoip,              eMainSink,     true,      true);
        s->configureRoute (evoip,              eMainSink,     false,     true);
        s->configureRoute (ecallertone,        eMainSink,     true,      true);
        s->configureRoute (ecallertone,        eMainSink,     false,     true);
        s->configureRoute (evoicerecognition,  eMainSink,     true,      true);
        s->configureRoute (evoicerecognition,  eMainSink,     false,     true);
        s->configureRoute (erecord,            eMainSource,   false,     true);
        s->configureRoute (erecord,            eMainSource,   true,      true);
        s->configureRoute (evoicedialsource,   eMainSource,   false,     true);
        s->configureRoute (evoicedialsource,   eMainSource,   true,      true);
        s->configureRoute (evoipsource,        eMainSource,   false,     true);
        s->configureRoute (evoipsource,        eMainSource,   true,      true);

        addScenario (s);

        registerMe (mediaMethods);

        restorePreferences();

        // Only media claims to be default
        makeCurrent();

}

int
MediaInterfaceInit(GMainLoop *loop, LSHandle *handle)
{

    sMediaModule = new MediaScenarioModule();

    if(NULL == sMediaModule ) {
        g_error("ERROR : Unable to allocate memory for Media Module\n");
        return -1;
    }

     return 0;
}

MODULE_START_FUNC (MediaInterfaceInit);

