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


#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>

#include <pulse/module-palm-policy-tables.h>

#include "PulseAudioMixer.h"
#include "AudioDevice.h"
#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "main.h"
#include "media.h"
#include "phone.h"
#include <audiodTracer.h>
#define SHORT_DTMF_LENGTH  200
#define phone_MaxVolume 70
#define phone_MinVolume 0
#define FILENAME "/dev/snd/pcmC"

#define _NAME_STRUCT_OFFSET(struct_type, member) \
                       ((long) ((unsigned char*) &((struct_type*) 0)->member))

PulseAudioMixer gPulseAudioMixer;

AudioMixer & gAudioMixer = gPulseAudioMixer;

const char * controlEventName(EControlEvent event)
{
    static const char * sNames[] = { "None",
                                     "First Stream Opened",
                                     "Last Stream Closed" };

    if (VERIFY(event >= 0 && (size_t) event < G_N_ELEMENTS(sNames)))
        return sNames[event];

    return "<invalid event>";
}

const char * virtualSinkName(EVirtualSink sink, bool prettyName)
{
    const char * name = "<invalid sink>";
    if (IsValidVirtualSink(sink))
        name = systemdependantvirtualsinkmap[sink].virtualsinkname +
                                                      (prettyName ? 1 : 0);
    else if (sink == eVirtualSink_None)
        name = "<none>";
    else if (sink == eVirtualSink_All)
        name = "<all/count>";
    return name;
}

EVirtualSink getSinkByName(const char * name)
{
    EVirtualSink sink = eVirtualSink_None;
    for (int i = eVirtualSink_First; i <= eVirtualSink_Last; i++)
    {
        if (0 == strcmp(name, systemdependantvirtualsinkmap[i].virtualsinkname) ||
             0 == strcmp(name, systemdependantvirtualsinkmap[i].virtualsinkname + 1))
        {
            sink = (EVirtualSink) systemdependantvirtualsinkmap[i].virtualsinkidentifier;
            break;
        }
    }

    return sink;
}

const char * virtualSourceName(EVirtualSource source, bool prettyName)
{
    const char * name = "<invalid source>";
    if (IsValidVirtualSource(source))
        name = systemdependantvirtualsourcemap[source].virtualsourcename +
                                                       (prettyName ? 1 : 0);
    else if (source == eVirtualSource_None)
        name = "<none>";
    else if (source == eVirtualSource_All)
        name = "<all/count>";
    return name;
}

EVirtualSource getSourceByName(const char * name)
{
    EVirtualSource source = eVirtualSource_None;
    for (int i = eVirtualSource_First; i <= eVirtualSource_Last; i++)
    {
        if (0 == strcmp(name, systemdependantvirtualsourcemap[i].virtualsourcename) ||
            0 == strcmp(name, systemdependantvirtualsourcemap[i].virtualsourcename + 1))
        {
            source = (EVirtualSource) systemdependantvirtualsourcemap[i].virtualsourceidentifier;
            break;
        }
    }

    return source;
}

const int cMinTimeout = 50;
const int cMaxTimeout = 5000;

PulseAudioMixer::PulseAudioMixer() : mChannel(0),
                                     mTimeout(cMinTimeout),
                                     mSourceID(-1),
                                     mConnectAttempt(0),
                                     mCurrentDtmf(NULL),
                                     mPulseFilterEnabled(true),
                                     mPulseStateFilter(0),
                                     mPulseStateLatency(0),
                                     mInputStreamsCurrentlyOpenedCount(0),
                                     mOutputStreamsCurrentlyOpenedCount(0),
                                     mCallbacks(0),
                                     voLTE(false),
                                     NRECvalue(1),
                                     BTDeviceType(eBTDevice_NarrowBand),
                                     mIsHfpAgRole(false),
                                     mPreviousVolume(0),
                                     BTvolumeSupport(false)
{
    // initialize table for the pulse state lookup table
    for (int i = eVirtualSink_First; i <= eVirtualSink_Last; i++)
    {
        mPulseStateVolume[i] = -1;
        mPulseStateVolumeHeadset[i] = -1;
        mPulseStateRoute[i] = -1;
        mPulseStateActiveStreamCount[i] = 0;
    }
    for (int i = eVirtualSource_First; i <= eVirtualSource_Last; i++)
    {
        mPulseStateSourceRoute[i] = -1;
    }
}

PulseAudioMixer::~PulseAudioMixer() {
}

/*
  current is the current volume from 0 to 65535
  dB_diff is the amount in dB added to the current volume
  returns the desired volume from 0 to 65535
*/
static int _add_dB (int current, int dB_diff)
{
    if (current <= 0 && dB_diff <= 0)
        return 0;
    else if (current >= 65535 && dB_diff >= 0)
        return 65535;

    float dB_current = (((float)(current) - 65535)/65535.0) * 100;
    float desired = 65535 * ((dB_current + (float)(dB_diff)) / 100 + 1);

    if (desired < 0)
        desired = 0;
    else if (desired > 65535)
        desired = 65535;

    int result = (int) desired;

    return result;
}

/*
 * current is the current volume from 0 to 100
 * dB_diff is the amount in dB added to the current volume
 * returns the desired volume from 0 to 100
 */
int PulseAudioMixer::adjustVolume(int percent, int dB_diff)
{
    if (!VERIFY(percent >= 0 && percent <= 100))
        return percent;

    int tableIndex = gAudioDevice.getHeadsetState() != eHeadsetState_None ? 1 : 0;
    int * table = _mapPercentToPulseVolume[tableIndex];
    int pulse_volume = table[percent];
    pulse_volume = _add_dB(pulse_volume, dB_diff);

    int result = percent;

    int lower = 0;
    int upper = 100;
    if (pulse_volume >= table[lower] && pulse_volume <= table[upper])
    {
        while (upper - lower > 1)
        {
            int mid = (lower + upper) / 2;
            int diff = pulse_volume - table[mid];

            if (diff == 0)
                upper = lower = mid;
            else if (diff < 0)
                upper = mid;
            else
                lower = mid;
        }
        result = lower;
        if (pulse_volume > table[lower])
        {
            result = lower + (dB_diff < 0 ? 0 : 1);
            if (pulse_volume >= table[lower + 1])
                ++result;
            if (result > 100)
                result = 100;
        }
        // defensive: catch that rounding never gets us in the wrong direction!
        if (dB_diff < 0)
        {
            if (result > percent)
                result = percent;
        }
        else
        {
            if (result < percent)
                result = percent;
        }
        VERIFY(table[lower] <= pulse_volume);
        VERIFY(table[upper] >= pulse_volume);
    }

    // arbitrarily force minimal 1% volume...
    if (result == 0 && percent > 1)
        result = 1;

    g_debug("adjust dB Volume: %dp %+d dB = %dp (%+dp)", \
                                 percent, dB_diff, result, result - percent);
    return result;
}

bool
PulseAudioMixer::suspendAll()
{
    return programSource ('s', eVirtualSink_All, 0);
}

bool
PulseAudioMixer::updateRate(int rate)
{
    return programSource ('x', eVirtualSink_All, rate);
}

bool
PulseAudioMixer::programBalance(int balance)
{

   return programSource ('B', eVirtualSink_All, balance);

 }

bool
PulseAudioMixer::programSource (char cmd, int sink, int value)
{
    if (NULL == mChannel)
        return false;

    EHeadsetState headset = gAudioDevice.getHeadsetState();

    bool    sendCmd = true;
    switch (cmd)
    {
        case 'm':
            value = 0;    // the mute command is equivalent to
                       //  setting volume to 0, but faster. There is no unmute!
        case 'v':

        case 'r':

            if (VERIFY(IsValidVirtualSink((EVirtualSink)sink)))
            {
                sendCmd = (mPulseStateVolume[sink] != value ||
                           mPulseStateVolumeHeadset[sink] != headset);
                mPulseStateVolume[sink] = value;
                mPulseStateVolumeHeadset[sink] = headset;
            }
            break;
        case 'h':
            if (VERIFY(IsValidVirtualSource((EVirtualSource)sink))) {
                sendCmd = true;
            }
            break;
        case 'd':
            if (VERIFY(IsValidVirtualSink((EVirtualSink)sink)))
            {
                if (mPulseStateRoute[sink] != value)
                    mPulseStateRoute[sink] = value;
                else
                    sendCmd = false;
            }
            break;
        case 'e':
            if (VERIFY(IsValidVirtualSource((EVirtualSource)sink)))
            {
                if (mPulseStateSourceRoute[sink] != value)
                    mPulseStateSourceRoute[sink] = value;
                else
                    sendCmd = false;
            }
            break;
        case 'f':
            mPulseStateFilter = value;
            break;
        case 'l':
            mPulseStateLatency = value;
            break;

        default:
            break;
    }

    if (sendCmd)
    {
        char buffer[SIZE_MESG_TO_PULSE];
        // some commands pass a sink value, but don't need really one and
        // should use 0. Because 0 is a valid sink,
        // we don't want them to use it for the call,
        // or the name of that sink to be listed in the trace.
        // So we make the substitution here and here only.
        const char * sinkName = "";
        if (cmd == 'l' || cmd == 's' || cmd == 'x')
        {
            sprintf(buffer, "%c 0 %i %i", cmd, value, headset);// ignore sink
                                                               // value. Put 0 always.
        }
        else if (cmd == 'f')
        {
            if (!mPulseFilterEnabled)
                value = 0;
            sprintf(buffer, "%c 0 %i %i", cmd, value, headset);// ignore sink
                                                               // value. Put 0 always.
        }
        else if (cmd == 'e')
        {
            sprintf(buffer, "%c %i %i %i", cmd, sink, value, headset);
            sinkName = virtualSourceName((EVirtualSource)sink);
        }
        else if (cmd == 'b')
        {
            sprintf(buffer, "%c %i %i %i", cmd, mPulseStateVolume[sink], headset, value);
            sinkName = virtualSinkName((EVirtualSink)sink);
        }
        else
        {
            sprintf(buffer, "%c %i %i %i", cmd, sink, value, headset);
            sinkName = virtualSinkName((EVirtualSink)sink);    // sink means something
        }

        g_debug ("%s: sending message '%s' %s", __FUNCTION__, buffer, sinkName);
        int sockfd = g_io_channel_unix_get_fd (mChannel);
        ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
        if (bytes != SIZE_MESG_TO_PULSE)
        {
            if (bytes >= 0)
                g_warning("programSource: only %u bytes sent to Pulse out of %d (%s).", \
                                   bytes, SIZE_MESG_TO_PULSE, strerror(errno));
            else
                g_warning("programSource: send to Pulse failed: %s", strerror(errno));
        }
    }

    return true;
}

bool PulseAudioMixer::programVolume (EVirtualSink sink, int volume, bool ramp)
{
    if (volume && !isNeverMutedSink(sink) &&
        mPulseStateActiveStreamCount[sink] <= 0)
    {    // don't set the volume up if
        // there is no stream playing for high latency sinks
        volume = 0;
    }
    // if nothing's playing, just do the fastest mute,
    // it's the cheapest, as nothing's playing, it makes no difference!
    if (volume == 0 && mPulseStateActiveStreamCount[sink] <= 0)
    {
        return programSource ('m', sink, 0);
    }

    return programSource ( (ramp ? 'r' : 'v'), sink, volume);
}

bool PulseAudioMixer::programCallVoiceOrMICVolume (char cmd, int volume)
{
    return programSource ( cmd, eVirtualSink_None, volume);
}

bool PulseAudioMixer::programMute (EVirtualSource source, int mute)
{
    return programSource ('h', source, mute);
}


bool PulseAudioMixer::programDestination (EVirtualSink sink,
                                          EPhysicalSink destination)
{
    return programSource ('d', sink, destination);
}

void PulseAudioMixer::sendNREC(bool value)
{

    char cmd = 'Z';
    char buffer[SIZE_MESG_TO_PULSE] ;
    ScenarioModule * phone = getPhoneModule();

    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return;
    }

    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return;
    }

    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %s", cmd, 0, value, phone->getCurrentScenarioName());

    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
        g_warning("Error sending msg for sendNREC(%d)", bytes);
}

void PulseAudioMixer::setNREC(bool value)
{

    if (NRECvalue != value) {
        NRECvalue = value;
        sendNREC(NRECvalue);
    }
}


/*Only to suspend the A2DP sink during call*/
bool PulseAudioMixer::suspendSink(int sink)
{
    g_message("PulseAudioMixer::suspendSink: a2dp sink = %d", sink);
    return programSource ('s', sink, sink);
}

#ifdef HAVE_BT_SERVICE_V1
void PulseAudioMixer::setBTvolumeSupport(bool value)
{

    if (BTvolumeSupport != value) {
        g_message ("setBTvolumeSupport: set to %d", value);
        BTvolumeSupport = value;
    }
}

void PulseAudioMixer::sendBTDeviceType (bool type, bool hfpStatus)
{

    char cmd = 'Y';
    char buffer[SIZE_MESG_TO_PULSE] ;

    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return;
    }

    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return;
    }

    g_debug ("Sending BTDeviceType to pulse (type %s and hfpStatus->%d)",\
        type?"wideband":"narrowband", hfpStatus);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d 0", cmd, type, hfpStatus);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE)
       g_warning("Error sending msg for BTDeviceType (%d)", bytes);
}
#endif

bool PulseAudioMixer::programLoadBluetooth (const char *address, const char *profile)
{
    char cmd = 'L';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    if (!address || !profile) {
        g_message("bluetooth  address or/and profile is null");
        return ret;
    }

    g_debug ("check for pulseaudio ");
    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_debug ("programLoadBluetooth sending message ");
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %s", cmd, 0, address, profile);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg for BT load(%d)", bytes);
    }
    else {
       g_warning("msg send for BT load(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programUnLoadBluetooth (const char *profile) {
    char cmd = 'U';
    char buffer[SIZE_MESG_TO_PULSE];
    bool ret = false;
    memset (buffer,0,SIZE_MESG_TO_PULSE);
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s", cmd, 0, profile);

    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_debug ("%s: sending message '%s'", __FUNCTION__, buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);

    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg for BT Unload(%d)", bytes);
    }
    else {
       g_warning("msg send for BT Unload(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programHeadsetRoute(int route) {
    char cmd = 'w';
    char buffer[SIZE_MESG_TO_PULSE]="";
    bool ret  = false;

    g_debug ("check for pulseaudio connection");
    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_debug ("programHeadsetState sending message");
    if (0 == route)
      snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, eHeadsetState_None);
    else if (1 == route)
      snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, eHeadsetState_Headset);
    else {
      g_warning("Wrong argument passed to programHeadsetRoute");
      return ret;
    }

    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
        g_warning("Error sending msg for headset routing from audiod(%d)", bytes);
    }
    else {
       g_debug("msg sent for headset routing from audiod");
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::externalSoundcardPathCheck (std::string filename, int status) {
    struct stat buff = {0};

    if (1 == status) {
        if (!(0 == stat (filename.c_str(), &buff)))
            return false;
    }
    else {
        if (0 == stat (filename.c_str(), &buff))
            return false;
    }
    return true;
}

bool PulseAudioMixer::loadUSBSinkSource(char cmd,int cardno, int deviceno, int status)
{
    char buffer[SIZE_MESG_TO_PULSE] = "";
    bool ret  = false;
    std::string card_no = std::to_string(cardno);
    std::string device_no = std::to_string(deviceno);
    std::string filename;

    g_debug ("check for pulseaudio connection");
    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    switch (cmd) {
        case 'j':
                {
                filename = FILENAME+card_no+"D"+device_no+"c";
                break;
                }
        case 'z':
                {
                filename = FILENAME+card_no+"D"+device_no+"p";
                break;
                }
        default:
                return false;
    }

    ret = externalSoundcardPathCheck(filename, status);
    if (false == ret)
        return ret;

    g_debug ("loadUSBSinkSource sending message");
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %d %d", cmd, cardno, deviceno, status);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg from loadUSBSinkSource", bytes);
    }
    else {
       g_message("msg sent from loadUSBSinkSource from audiod", bytes);
       ret = true;
    }
    return ret;
}






bool PulseAudioMixer::programLoadRTP(const char *type, const char *ip, int port)
{

    char cmd = 't';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    g_debug ("check for pulseaudio connection");
    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_debug ("programLoadRTP sending message ");
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %s %d", cmd, 0, type, ip, port);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg for RTP load(%d)", bytes);
    }
    else {
       g_warning("msg send for RTP load(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programUnloadRTP()
{

    char cmd = 'g';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    g_debug ("check for pulseaudio connection");
    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_debug ("programLoadRTP sending message ");
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d", cmd, 0);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg for RTP load(%d)", bytes);
    }
    else {
       g_warning("msg send for RTP Unload(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::phoneEvent(EPhoneEvent event, int parameter)
{

    bool result = true;

    switch (event)
    {
    case ePhoneEvent_voLTEcallsupported:
        g_debug ("%s :ePhoneEvent_voLTEcallsupported...........",__FUNCTION__);
         voLTE = true;
        break;

    case ePhoneEvent_voLTEcallNOTsupported:
        g_debug ("%s :ePhoneEvent_voLTEcallNOTsupported...........",__FUNCTION__);
         voLTE = false;
        break;

    case ePhoneEvent_DTMFToneEnd:
        // Handled by Modem
        break;
    case ePhoneEvent_DTMFTone:
        if (parameter&0x80) {
            // let modem handle it
            result = true;
            break;
        }
        parameter = parameter & 0x7F;
        gAudioMixer.playDtmf((const char*)&parameter, eDTMF);
        break;

    case ePhoneEvent_OneShotDTMFTone:
        if (parameter&0x80) {
            // let modem handle it
            result = true;
            break;
        }
        parameter = parameter & 0x7F;
        gAudioMixer.playOneshotDtmf((const char*)&parameter, eDTMF);
    break;

    default:
        VERIFY(false);
        break;
    }

    return result;
}

void PulseAudioMixer::setBTDeviceType(int type){
    if (BTDeviceType != type)
        BTDeviceType = type;
}

bool PulseAudioMixer::getBTVolumeSupport()
{
    return BTvolumeSupport;
}


inline void PulseAudioMixer::setHfpAgRole(bool HfpAgRole)
{
    mIsHfpAgRole = HfpAgRole;
}

inline bool PulseAudioMixer::inHfpAgRole(void)
{
    return mIsHfpAgRole;
}

bool PulseAudioMixer::setPhoneVolume(const ConstString & scenario, int volume)
{
    bool    result = false;
    ConstString tail;

    if (VERIFY(scenario.hasPrefix(PHONE_, tail))) {

        if (volume >  phone_MaxVolume)
            volume = phone_MaxVolume;
        if ((tail == "back_speaker") || ((tail == "bluetooth_sco") && !BTvolumeSupport)) {
            g_debug ("%s : calling to set voice volume %d during call\n",__FUNCTION__, volume);
            mPreviousVolume = volume;
            result = programCallVoiceOrMICVolume('a', volume);
        }
        else {
            g_message ("%s : scenario %s, BT Supports Volume, Don't set on DSP",__FUNCTION__, scenario.c_str());
            result = true;
        }
    }

    return result;
}

bool PulseAudioMixer::setPhoneMuted(const ConstString & scenario, bool muted)
{
    if (muted)
        return programCallVoiceOrMICVolume('c', phone_MinVolume);
    else
        return programCallVoiceOrMICVolume('c', mPreviousVolume);
}

bool PulseAudioMixer::setRouting(const ConstString & scenario){
    char cmd = 'C';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;
    ConstString tail;

    g_debug ("check for pulseaudio ");
    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_message ("PulseAudioMixer::setRouting: sceanrio = %s", scenario.c_str());
    if (scenario.hasPrefix(PHONE_, tail)) {
     g_message ("PulseAudioMixer::setRouting: phone device = %s", tail.c_str());
        cmd = 'P';
    } else if (scenario.hasPrefix(MEDIA_, tail)) {
        g_message ("PulseAudioMixer::setRouting: media device = %s", tail.c_str());
        cmd = 'Q';
    }
    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %d", cmd, voLTE, tail.c_str(), BTDeviceType);
    g_debug ("PulseAudioMixer::setRouting sending message : %s ", buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg for sendMixerState(%d)", bytes);
    }
    else {
       g_warning("msg send for sendMixerState(%d)", bytes);
       ret = true;
    }
    return ret;
}

int PulseAudioMixer::loopback_set_parameters(const char * value)
{
    char cmd = 'T';
    char buffer[SIZE_MESG_TO_PULSE] ;
    bool ret  = false;

    g_debug ("check for pulseaudio ");
    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return ret;
    }

    if (!mChannel) {
        g_message("There is no socket connection to pulseaudio");
        return ret;
    }

    g_message ("PulseAudioMixer::loopback_set_parameters: value = %s", value);

    snprintf(buffer, SIZE_MESG_TO_PULSE, "%c %d %s %d", cmd, 0, value, 0);
    g_debug ("PulseAudioMixer::loopback_set_parameters sending message : %s ", buffer);
    int sockfd = g_io_channel_unix_get_fd (mChannel);
    ssize_t bytes = send(sockfd, buffer, SIZE_MESG_TO_PULSE, MSG_DONTWAIT);
    if (bytes != SIZE_MESG_TO_PULSE) {
       g_warning("Error sending msg for loopback_set_parameters(%d)", bytes);
    }
    else {
       g_warning("msg send for loopback_set_parameters(%d)", bytes);
       ret = true;
    }
    return ret;
}

bool PulseAudioMixer::programDestination (EVirtualSource source,
                                          EPhysicalSource destination)
{
    return programSource ('e', source, destination);
}

bool PulseAudioMixer::programFilter (int filterTable)
{
    return programSource ('f', eVirtualSink_None, filterTable);
}

bool PulseAudioMixer::programLatency (int latency)
{
    return programSource ('l', eVirtualSink_None, latency);
}

bool PulseAudioMixer::muteAll ()
{
    for (EVirtualSink sink = eVirtualSink_First;
         sink <= eVirtualSink_Last;
         sink = EVirtualSink(sink + 1))
    {
        if (sink != ecallertone)
            programSource ('m', sink, 0);
    }

    return true;
}

static gboolean
_pulseStatus(GIOChannel *ch, GIOCondition condition, gpointer user_data)
{
    gPulseAudioMixer._pulseStatus(ch, condition, user_data);
    return TRUE;
}

bool
PulseAudioMixer::_connectSocket ()
{
    GIOCondition condition = GIOCondition(G_IO_ERR | G_IO_HUP | G_IO_IN);

    struct sockaddr_un name;
    memset (&name, 0x00, sizeof(name));

    name.sun_family = AF_UNIX;
    name.sun_path[0] = '\0';

    int length = strlen(PALMAUDIO_SOCK_NAME) + 1;

    if (length > _MAX_NAME_LEN)
    {
        g_critical ("%s: socket name '%s' is too long (%i > %i)",__FUNCTION__,\
                                  PALMAUDIO_SOCK_NAME, length, _MAX_NAME_LEN);
        return false;
    }

    strncpy (&name.sun_path[1], PALMAUDIO_SOCK_NAME, length);

    mConnectAttempt++;

    int sockfd = -1;

    if (-1 == (sockfd = socket(AF_UNIX, SOCK_STREAM, 0)))
    {
        g_warning ("%s: socket error '%s' on fd (%i)",__FUNCTION__, strerror(errno), sockfd);
        sockfd = -1;
        return false;
    }

    if (-1 == connect(sockfd, (struct sockaddr *)&name,
                               _NAME_STRUCT_OFFSET (struct sockaddr_un, sun_path)
                                + length))
    {
        g_debug ("%s: connect error '%s' on fd (%i) ", \
                                       __FUNCTION__, strerror(errno), sockfd);
        if (close(sockfd))
        {
            g_warning ("%s: close error '%s' on fd (%i)",__FUNCTION__,
                                                      strerror(errno), sockfd);
        }
        sockfd = -1;
        return false;
    }

    g_message ("%s: successfully connected to Pulse on attempt #%i",\
                                                              __FUNCTION__,
                                                               mConnectAttempt);
    mConnectAttempt = 0;

    // initialize table for the pulse state lookup table
    for (EVirtualSink sink = eVirtualSink_First;
         sink <= eVirtualSink_Last;
         sink = EVirtualSink(sink + 1))
    {
        mPulseStateVolume[sink] = -1;
        mPulseStateVolumeHeadset[sink] = -1;
        mPulseStateRoute[sink] = -1;
        while (mPulseStateActiveStreamCount[sink] > 0)
            outputStreamClosed(sink);
        mPulseStateActiveStreamCount[sink] = 0;    // shouldn't be necessary
    }

    EVirtualSource source;
    for (source = eVirtualSource_First;
           source <= eVirtualSource_Last;
           source = EVirtualSource(source + 1))
    {
        mPulseStateSourceRoute[source] = -1;
    }
    mActiveStreams.clear();
    mPulseStateFilter = 0;
    mPulseStateLatency = SCENARIO_DEFAULT_LATENCY;

    // Reuse the counting logic above rather than just reset the counters,
    // to preserve closing behavior without re-implementing it
    while (mInputStreamsCurrentlyOpenedCount > 0)
        inputStreamClosed(source);
    mInputStreamsCurrentlyOpenedCount = 0;    // shouldn't be necessary
    mOutputStreamsCurrentlyOpenedCount = 0;    // shouldn't be necessary

    // To do? since we are connected setup a watch for data on the file descriptor.
    mChannel = g_io_channel_unix_new(sockfd);

    mSourceID = g_io_add_watch (mChannel, condition, ::_pulseStatus, NULL);

    // Let audiod know that we now have a connection, so that the mixer can be programmed
    if (VERIFY(mCallbacks))
        mCallbacks->onAudioMixerConnected();

    // We've just established our direct socket link to Pulse.
    // It's a good time to see if we can connect using the Pulse APIs
    if (!mPulseLink.checkConnection()) {
        g_message("Pulseaudio is not running");
        return false;
    }

    return true;
}

static gboolean
_timer (gpointer data)
{
    gPulseAudioMixer._timer();
    return FALSE;
}

void PulseAudioMixer::_timer()
{
    if (!_connectSocket())
    {
        g_timeout_add (mTimeout, ::_timer, 0);
        mTimeout *= 2;
        if (mTimeout > cMaxTimeout)
            mTimeout = cMinTimeout;
    }
}

int PulseAudioMixer::getStreamCount (EVirtualSink sink)
{
    return mPulseStateActiveStreamCount[sink];
}

bool PulseAudioMixer::isSinkAudible(EVirtualSink sink)
{
    if (!VERIFY(IsValidVirtualSink(sink)))
        return false;
    return mActiveStreams.contain(sink) && mPulseStateVolume[sink] > 0;
}

void
PulseAudioMixer::openCloseSink (EVirtualSink sink, bool openNotClose)
{
    if (!VERIFY(IsValidVirtualSink(sink)))
        return;

    int & streamCount = mPulseStateActiveStreamCount[sink];

    if (openNotClose)
        ++streamCount;
    else
        --streamCount;

    if (streamCount < 0)
    {
        g_warning ("openCloseSink: adjusting the stream %i-%s count to 0", \
                                                  sink, virtualSinkName(sink));
        streamCount = 0;
    }

    VirtualSinkSet oldstreamflags = mActiveStreams;

    if (0 == streamCount)
        mActiveStreams.remove(sink);
    else
        mActiveStreams.add(sink);

    if (oldstreamflags != mActiveStreams)
    {
        EControlEvent event = openNotClose ? eControlEvent_FirstStreamOpened :
                                           eControlEvent_LastStreamClosed;
        if (mCallbacks)
        {
            mCallbacks->onSinkChanged(sink, event, ePulseAudio);
        }
    }
}

#define LOG_LEVEL_SINK(sink) \
  ((sink == eDTMF || sink == efeedback || sink == eeffects) ? \
   G_LOG_LEVEL_INFO : G_LOG_LEVEL_MESSAGE)

void
PulseAudioMixer::_pulseStatus(GIOChannel *ch,
                              GIOCondition condition,
                              gpointer user_data)
{
    if (condition & G_IO_IN)
    {
        char buffer[SIZE_MESG_TO_AUDIOD];

        int sockfd = g_io_channel_unix_get_fd (ch);
        ssize_t bytes = recv(sockfd, buffer, SIZE_MESG_TO_AUDIOD, 0);

        char cmd;
        int isink;
        int info;
        char ip[28];
        int port;

        if (bytes > 0 && EOF != sscanf (buffer, "%c %i %i", &cmd, &isink, &info))
        {
            g_debug("PulseAudioMixer::_pulseStatus: Pulse says: '%c %i %i'",\
                                  cmd, isink, info);
            EVirtualSink sink = EVirtualSink(isink);
                EVirtualSource source = EVirtualSource(isink);
            switch (cmd)
            {

              case 'a':
                   g_message ("Got A2DP sink running message from PA");
                   getMediaModule()->resumeA2DP();
                   break;

               case 'b':
                   g_message ("Got A2DP sink Suspend message from PA");
                   getMediaModule()->pauseA2DP();
                   break;

               case 'o':
                    if (VERIFY(IsValidVirtualSink(sink)))
                    {
                        outputStreamOpened (sink);
                        g_log(G_LOG_DOMAIN, LOG_LEVEL_SINK(sink), \
                        "%s: sink %i-%s opened (stream %i). Volume: %d, Headset: %d, Route: %d, Streams: %d.",
                                __FUNCTION__, sink, virtualSinkName(sink), \
                                info, mPulseStateVolume[sink],\
                                mPulseStateVolumeHeadset[sink], \
                                mPulseStateRoute[sink], \
                                mPulseStateActiveStreamCount[sink]);
                    }
                    break;

                case 'c':
                    if (VERIFY(IsValidVirtualSink(sink)))
                    {
                        outputStreamClosed (sink);
                        if(eeffects == sink || eDTMF == sink)
                            gAudioDevice.disableHW();

                        g_log(G_LOG_DOMAIN, LOG_LEVEL_SINK(sink), \
                         "%s: sink %i-%s closed (stream %i). Volume: %d, Headset: %d, Route: %d, Streams: %d.", \
                                __FUNCTION__, sink, virtualSinkName(sink),\
                                 info, mPulseStateVolume[sink], \
                                 mPulseStateVolumeHeadset[sink], \
                                 mPulseStateRoute[sink], \
                                 mPulseStateActiveStreamCount[sink]);
                    }
                    break;
                case 'O':
                    if (VERIFY(IsValidVirtualSink(sink)) && VERIFY(info >= 0))
                    {
                        g_warning("%s: pulse says %i sink%s of type %i-%s %s already opened", \
                                   __FUNCTION__, info, \
                                   ((info > 1) ? "s" : ""), \
                                   sink, virtualSinkName(sink), \
                                   ((info > 1) ? "are" : "is"));
                        while (mPulseStateActiveStreamCount[sink] < info)
                            outputStreamOpened (sink);
                        while (mPulseStateActiveStreamCount[sink] > info)
                            outputStreamClosed (sink);
                    }
                    break;
                case 'I':
                    {
                        if (VERIFY(IsValidVirtualSource(source)) && VERIFY(info >= 0))
                        {
                            g_warning("%s: pulse says %i input source%s already opened",\
                                       __FUNCTION__, info, \
                                       ((info > 1) ? "s are" : " is"));
                            while (mInputStreamsCurrentlyOpenedCount < info)
                                inputStreamOpened (source);
                            while (mInputStreamsCurrentlyOpenedCount > info)
                                inputStreamClosed (source);
                        }
                    }
                    break;
                case 'd':
                    inputStreamOpened (source);
                    break;
                case 'k':
                    inputStreamClosed (source);
                    break;
                case 'x':
                    gAudioDevice.prepareForPlayback ();
                    break;
                case 'y':
                    //prepare hw for capture
                    break;
                /* powerd related msg */
                case 'H':
                    getMediaModule()->sendAckToPowerd(true);
                    break;
                case 'R':
                    getMediaModule()->sendAckToPowerd(false);
                    break;
                case 't':
                    if (5 == sscanf (buffer, "%c %i %i %28s %d", &cmd, &isink, &info, ip, &port))
                        gState.rtpSubscriptionReply(info, ip, port);
                default:
                    break;
            }
       }
    }

    if (condition & G_IO_ERR)
    {
        g_warning ("%s: error condition on the socket to pulse", __FUNCTION__);
    }

    if (condition & G_IO_HUP)
    {
        g_warning ("%s: pulse server gone away", __FUNCTION__);
        g_io_channel_shutdown (ch, FALSE, NULL);

        mTimeout = cMinTimeout;
        g_source_remove (mSourceID);
        g_io_channel_unref(mChannel);
        mChannel = NULL;
        gState.setRTPLoaded(false);
        g_timeout_add (0, ::_timer, 0);
    }
}

void PulseAudioMixer::outputStreamOpened (EVirtualSink sink)
{
    if (IsValidVirtualSink(sink))
        openCloseSink (sink, true);
    if (mOutputStreamsCurrentlyOpenedCount == 0)
        gAudioDevice.setActiveOutput (true);
    mOutputStreamsCurrentlyOpenedCount++;
}

void PulseAudioMixer::outputStreamClosed (EVirtualSink sink)
{
    mOutputStreamsCurrentlyOpenedCount--;
    if (mOutputStreamsCurrentlyOpenedCount <= 0)
        gAudioDevice.setActiveOutput (false);

    if (mOutputStreamsCurrentlyOpenedCount < 0)
    {
        g_warning ("%s: stream opened count was less than 0", __FUNCTION__);
        mOutputStreamsCurrentlyOpenedCount = 0;
    }
    if (IsValidVirtualSink(sink))
        openCloseSink (sink, false);
}

void PulseAudioMixer::inputStreamOpened (EVirtualSource source)
{
    if (mInputStreamsCurrentlyOpenedCount == 0 && mCallbacks) {
        gAudioMixer.programMute(source, true);
        if (!gState.getOnActiveCall()) {

            if (source != evoiceactivator)
                gState.setRecordStatus(true);

            if (gState.getHfpStatus() && source == eqvoice) {
                gState.setQvoiceStartedStatus(true);
                g_message("a2dp %s hfp %s", gState.getA2DPStatus().c_str(), gState.getHFPConnectionStatus().c_str());

                if (!((gState.getA2DPStatus() == "playing") || (gState.getA2DPStatus() == "Playing"))) {
                    if (!((gState.getHFPConnectionStatus() == "playing") || (gState.getHFPConnectionStatus() == "Playing"))) {
                        g_message("Opening SCO for QVoice since A2DP is not in use");
                        LSHandle *sh = GetPalmService();
                        if (sh)
                            gState.btOpenSCO(sh);
                        else
                            g_critical("Error : Could not get LSHandle");
                    }
                } else if ((gState.getA2DPStatus() == "Playing" || gState.getA2DPStatus() == "playing")) {
                    g_message("Suspending A2DP Sink");
                    gAudioMixer.suspendSink(eA2DPSink);
                    getMediaModule()->pauseA2DP();
                }
            }
            g_message("Unmuting source %d",source);
            gAudioMixer.programMute(source, false);
            mCallbacks->onInputStreamActiveChanged(true);
        }

        if (gState.getOnActiveCall() && source == evoicecallsource) {
            gAudioMixer.programMute(source, false);
            gAudioDevice.setPhoneRecording(true);
        }
    }
    if (source != evoiceactivator)
        mInputStreamsCurrentlyOpenedCount++;
    g_message("%s: input sink opened (%d total)", __FUNCTION__, \
                                      mInputStreamsCurrentlyOpenedCount);
}

void PulseAudioMixer::inputStreamClosed (EVirtualSource source)
{
    if (source != evoiceactivator)
        mInputStreamsCurrentlyOpenedCount--;

    if (mInputStreamsCurrentlyOpenedCount <= 0 && mCallbacks) {
        g_message("Closing Recording");
        if (!gState.getOnActiveCall() || gAudioDevice.isRecording()) {

            if (source != evoiceactivator)
                gState.setRecordStatus(false);

            if (gState.getHfpStatus() && source == eqvoice) {
                g_message("Closing SCO for QVoice");
                gState.setQvoiceStartedStatus(false);
                LSHandle *sh = GetPalmService();
                if (sh)
                    gState.btCloseSCO(sh);
                else
                    g_critical("Error : Could not get LSHandle");
            }
            mCallbacks->onInputStreamActiveChanged(false);
        }

        if (gState.getOnActiveCall() && source == evoicecallsource) {
            gAudioMixer.programMute(source, true);
            gAudioDevice.setPhoneRecording(false);
        }
    }

    if (mInputStreamsCurrentlyOpenedCount < 0)
    {
        g_warning ("%s: stream opened count was less than 0", __FUNCTION__);
        mInputStreamsCurrentlyOpenedCount = 0;
    }
    g_message("%s: input sink closed (%d left)", __FUNCTION__, \
                                          mInputStreamsCurrentlyOpenedCount);
}

#if defined(AUDIOD_TEST_API)
static bool
_sinkStatus(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return gPulseAudioMixer._sinkStatus(lshandle, message);
}
#endif

bool
PulseAudioMixer::_sinkStatus(LSHandle *lshandle, LSMessage *message)
{
    LSMessageJsonParser    msg(message, SCHEMA_1(OPTIONAL(sink, string)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    std::string reply;

    EVirtualSink sink = eVirtualSink_All;
    std::string sinkName;
    if (msg.get("sink", sinkName) && sinkName != "all")
    {
        sink = getSinkByName(sinkName.c_str());
        if (!IsValidVirtualSink(sink))
        {
            reply = INVALID_PARAMETER_ERROR(sink, string);
            goto sendReply;
        }
    }
    if (IsValidVirtualSink(sink))
    {
        pbnjson::JValue answer = createJsonReply(true);

        answer.put("sink", systemdependantvirtualsinkmap[sink].virtualsinkname);
        answer.put("volume", mPulseStateVolume[sink]);
        answer.put("filter", mPulseStateFilter);
        answer.put("latency", mPulseStateLatency);
        const char * destination = (mPulseStateRoute[sink] >= 0) ?
         systemdependantphysicalsinkmap[mPulseStateRoute[sink]].physicalsinkname :
          "undefined";
        answer.put("destination", destination);
        answer.put("openStreams", mPulseStateActiveStreamCount[sink]);

        reply = jsonToString(answer);
    }
    else    // return all sinks!
    {
        pbnjson::JValue answer = createJsonReply(true);
        for (EVirtualSink sink = eVirtualSink_First;
              sink <= eVirtualSink_Last;
              sink = EVirtualSink(sink + 1))
        {
            pbnjson::JValue sinkstate = pbnjson::Object();
            sinkstate.put("sink", systemdependantvirtualsinkmap[sink].virtualsinkname);
            sinkstate.put("volume", mPulseStateVolume[sink]);
            sinkstate.put("filter", mPulseStateFilter);
            sinkstate.put("latency", mPulseStateLatency);
            const char * destination = (mPulseStateRoute[sink] >= 0) ?
              systemdependantphysicalsinkmap[mPulseStateRoute[sink]].physicalsinkname :
               "undefined";
            sinkstate.put("destination", destination);
            sinkstate.put("openStreams", mPulseStateActiveStreamCount[sink]);
            answer.put(systemdependantvirtualsinkmap[sink].virtualsinkname, sinkstate);
        }
        reply = jsonToString(answer);
    }

sendReply:
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

#if defined(AUDIOD_TEST_API)
static bool
_setFilter(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return gPulseAudioMixer._setFilter(lshandle, message);
}
#endif

bool
PulseAudioMixer::_setFilter(LSHandle *lshandle, LSMessage *message)
{
    LSMessageJsonParser    msg(message, SCHEMA_1(REQUIRED(filter, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const char * errorText = STANDARD_JSON_SUCCESS;
    bool filter;
    if (!msg.get("filter", filter))
    {
        errorText = MISSING_PARAMETER_ERROR(filter, boolean);
    }
    else
    {
        if (filter)
            mPulseFilterEnabled = true;
        else
            mPulseFilterEnabled = false;

        programFilter(mPulseStateFilter);
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, errorText, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

#if defined(AUDIOD_TEST_API)
static bool
_suspend(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    return gPulseAudioMixer._suspend(lshandle, message);
}
#endif

bool
PulseAudioMixer::_suspend(LSHandle *lshandle, LSMessage *message)
{
    LSMessageJsonParser msg(message, SCHEMA_1(REQUIRED(suspend, boolean)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;

    const char * errorText = STANDARD_JSON_SUCCESS;
    bool suspend;
    if (!msg.get("suspend", suspend))
    {
        errorText = MISSING_PARAMETER_ERROR(suspend, boolean);
    }
    else
    {
        suspendAll();
    }

    CLSError lserror;
    if (!LSMessageReply(lshandle, message, errorText, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);

    return true;
}

static int IdToDtmf(const char* snd) {
    if ('0'<=snd[0] && snd[0]<='9') {
        return (snd[0]-(int)'0');
    } else if (snd[0]=='*') {
        return Dtmf_Asterisk;
    } else if (snd[0]=='#') {
        return Dtmf_Pound;
    } else {
        g_warning("unknown dtmf tone");
        return-1;
    }
}

bool PulseAudioMixer::playSystemSound(const char *snd, EVirtualSink sink)
{
    PMTRACE_FUNCTION;
    if (strncmp(snd, "dtmf_", 5) == 0) {
        // support dtmf_x
        // see http://developer.palm.com/index.php?option=
        //com_content&view=article&id=1618&Itemid=20#SystemSounds-playFeedback
        playOneshotDtmf(snd+5, sink);
        return true;
    }
    return mPulseLink.play(snd, sink);
}

void  PulseAudioMixer::playOneshotDtmf(const char *snd, EVirtualSink sink)
{
    PMTRACE_FUNCTION;
    playOneshotDtmf(snd, virtualSinkName(sink, false));
}

void  PulseAudioMixer::playOneshotDtmf(const char *snd, const char* sink)
{
    PMTRACE_FUNCTION;
    g_message("PulseAudioMixer::playOneshotDtmf");
    int tone;
    if ((tone=IdToDtmf(snd))<0) return;
    if (mCurrentDtmf) stopDtmf();
    mCurrentDtmf = new PulseDtmfGenerator((Dtmf)tone, SHORT_DTMF_LENGTH);
    gAudioDevice.prepareHWForPlayback();
    mPulseLink.play(mCurrentDtmf, sink);
}

void  PulseAudioMixer::playDtmf(const char *snd, EVirtualSink sink)
{
    gAudioDevice.prepareForPlayback();

    playDtmf(snd, virtualSinkName(sink, false));
}

void  PulseAudioMixer::playDtmf(const char *snd, const char* sink) {
    g_message("PulseAudioMixer::playDtmf");
    int tone;
    if ((tone=IdToDtmf(snd))<0) return;
    if (mCurrentDtmf && tone==mCurrentDtmf->getTone()) return;
    stopDtmf();
    mCurrentDtmf = new PulseDtmfGenerator((Dtmf)tone, 0);
    gAudioDevice.prepareForPlayback();
    mPulseLink.play(mCurrentDtmf, sink);
}

void PulseAudioMixer::stopDtmf() {
    if (mCurrentDtmf) {
        g_message("PulseAudioMixer::stopDtmf");
        mCurrentDtmf->stopping();
        mCurrentDtmf->unref();
        mCurrentDtmf = NULL;
    }
}


#if defined(AUDIOD_TEST_API)
static LSMethod pulseMethods[] = {
    { "sinkStatus", _sinkStatus},
    { "setFilter", _setFilter},
    { "suspend", _suspend},
    { },
};
#endif

void
PulseAudioMixer::init(GMainLoop * loop, LSHandle * handle,
                                        AudiodCallbacksInterface * interface)
{
    mCallbacks = interface;

#if defined(AUDIOD_TEST_API)
    bool result;
    CLSError lserror;

    result = ServiceRegisterCategory ("/state/pulse", pulseMethods, NULL, loop);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        g_message("%s: Registering Service for '%s' category failed", __FUNCTION__, "/state/pulse");
    }
#endif
    g_timeout_add (0, ::_timer, 0);
}
