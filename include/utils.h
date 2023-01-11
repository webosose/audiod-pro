// Copyright (c) 2012-2023 LG Electronics, Inc.
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


#ifndef _UTILS_H_
#define _UTILS_H_

#include <glib.h>
#include <lunaservice.h>
#include <pulse/module-palm-policy.h>
#include "log.h"
#include "ConstString.h"
#include <pulse/module-palm-policy-tables.h>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <pbnjson.h>
#include <pbnjson.hpp>

#define AUDIOD_ERRORCODE_NOT_SUPPORT_VOLUME_CHANGE 3
#define AUDIOD_ERRORCODE_INVALID_MIXER_INSTANCE 2
#define AUDIOD_ERRORCODE_FAILED_MIXER_CALL 3
#define AUDIOD_ERRORCODE_INVALID_ENVELOPE_INSTANCE 4
#define AUDIOD_ERRORCODE_INVALID_SOUNDOUT 204
#define AUDIOD_ERRORCODE_PARAMETER_BE_EMPTY 1
#define AUDIOD_ERRORCODE_SCENARIO_NOT_FOUND 2
#define AUDIOD_ERRORCODE_NOT_SUPPORT_MUTE 12
#define AUDIOD_ERRORCODE_INTERNAL_ERROR   15
#define AUDIOD_ERRORCODE_UNKNOWN_STREAM 17
#define AUDIOD_ERRORCODE_INVALID_PARAMS 21
#define AUDIOD_ERRORCODE_STREAM_NOT_ACTIVE 22
#define AUDIOD_ERRORCODE_SOUNDOUTPUT_NOT_SUPPORT 204
#define AUDIOD_ERRORCODE_INVALID_SESSIONID 20
#define AUDIOD_ERRORCODE_SOUNDINPUT_NOT_SUPPORT 24
#define AUDIOD_ERRORCODE_UNKNOWN_TRACKID 25
enum EBTDeviceType
{
    eBTDevice_NarrowBand =1,
    eBTDevice_Wideband =2
};

enum EUMISink
{
    eUmiMedia = eVirtualSink_Count+1,
    eVirtualUMISink_Count,
    eVirtualUMISink_First = eUmiMedia,
    eVirtualUMISink_Last = eUmiMedia,
    eVirtualUMISink_All = eVirtualUMISink_Count,
    eAllSink = eVirtualUMISink_Count
};

typedef enum serverType{
    eBluetoothService = 0,
    eBluetoothService2,
    eAudiooutputdService,
    eSettingsService,
    eSettingsService2,
    ePdmService,
    eServiceCount,        //Last element should be eServiceCount
    eServiceFirst = eBluetoothService,
}SERVER_TYPE_E;

typedef struct SubscriptionDetails
{
    std::string sKey;
    std::string payload;
    std::string ctx;
}SUBSCRIPTION_DETAILS_T;
typedef std::map<std::string,SERVER_TYPE_E> serverNameMap;

template <typename EnumT, typename BaseEnumT>
class ExtendEnum
{
    public:
        ExtendEnum() {}
        ExtendEnum(EnumT e):enum_(e) {}
        ExtendEnum(BaseEnumT e):enum_(static_cast<EnumT>(e)) {}
        explicit ExtendEnum( int val):enum_(static_cast<EnumT>(val)) {}
        operator EnumT() const { return enum_; }
    private:
        EnumT enum_ = eVirtualUMISink_First;
};

typedef ExtendEnum<EUMISink, EVirtualSink> EVirtualAudioSink;

namespace utils
{
    typedef enum ESinkStatus
    {
        eSinkNone    = 0,
        eSinkOpened  = 1,
        eSinkClosed  = 2
    }ESINK_STATUS;

    typedef enum EDeviceStatus
    {
        eDeviceNone    = 0,
        eDeviceConnected  = 1,
        eDeviceDisconnected  = 2
    }E_DEVICE_STATUS;

    typedef enum EMixerType
    {
        eMixerNone,
        ePulseMixer,
        eUmiMixer
    }EMIXER_TYPE;

    typedef enum eventType
    {
        eEventNone = -1,
        eEventSinkStatus = 0,
        eEventSourceStatus,
        eEventServerStatusSubscription,
        eEventKeySubscription,
        eEventMixerStatus,
        eEventMasterVolumeStatus,
        eEventCurrentInputVolume,
        eEventInputVolume,
        eEventDeviceConnectionStatus,
        eEventSinkPolicyInfo,
        eEventSourcePolicyInfo,
        eEventSoundOutputInfo,
        eEventBTDeviceDisplayInfo,
        eEventActiveDeviceInfo,
        eEventSinkAppId,
        eEventRegisterTrack,
        eEventUnregisterTrack,
        eEventLunaServerStatusSubscription,
        eEventLunaKeySubscription,
        eEventRequestSoundOutputDeviceInfo,
        eEventResponseSoundOutputDeviceInfo,
        eEventRequestSoundInputDeviceInfo,
        eEventResponseSoundInputDeviceInfo,
        eEventRequestInternalDevices,
        eEventType_Count,
        eEventType_First = 0,
        eEventType_Last = eEventResponseSoundInputDeviceInfo
    }EVENT_TYPE_E;

    typedef enum EConnStatus
    {
        eConnectionNone,
        eConnected,
        eDisconnected
    }ECONN_STATUS;

    typedef enum ReplyType
    {
        eLSReply,
        eLSRespond
    }EReplyType;

    typedef struct volumePolicyInfo
    {
        std::string streamType;
        int policyVolume;
        int priority;
        int groupId;
        int defaultVolume;
        int maxVolume;
        int minVolume;
        bool volumeAdjustable;
        int currentVolume;
        bool muteStatus;
        std::string sink;
        std::string source;
        EMIXER_TYPE mixerType;
        bool isPolicyInProgress;
        bool isStreamActive;
        bool ramp;
        std::string category;
        volumePolicyInfo()
        {
            streamType = "";
            policyVolume = 0;
            priority  = 0;
            groupId = 0;
            defaultVolume = 100;
            maxVolume = 100;
            minVolume = 0;
            volumeAdjustable = true;
            currentVolume = 0;
            muteStatus = false;
            sink = "";
            source = "";
            mixerType = eMixerNone;
            isPolicyInProgress = false;
            isStreamActive = false;
            ramp = false;
            category = "";
        }
    }VOLUME_POLICY_INFO_T;
    //TODO: delete this
    typedef struct soundOutputListInfo
    {
        bool activeStatus;
        bool muteStatus;
        bool adjustVolume;
        int volume;
        int maxVolume;
        int minVolume;
        int volumeStep;
        soundOutputListInfo()
        {
            activeStatus = false;
            muteStatus = false;
            adjustVolume = true;
            volume = 100;
            maxVolume = 100;
            minVolume = 0;
            volumeStep = 1;
        }
    }SOUNDOUTPUT_LIST_T;

    typedef struct deviceInfo
    {
        std::string deviceName;
        std::string deviceIcon;
        std::string deviceNameDetail;
        bool activeStatus;
        bool muteStatus;
        bool adjustVolume;
        int volume;
        int maxVolume;
        int minVolume;
        int volumeStep;
        std::string display;
        int priority;
        bool isConnected;
        std::string deviceType;
        deviceInfo()
        {
            activeStatus = false;
            muteStatus = false;
            adjustVolume = true;
            volume = 100;
            maxVolume = 100;
            minVolume = 0;
            volumeStep = 1;
            priority = -1;
            isConnected  = false;
        }
    }DEVICE_INFO_T;

    typedef struct multipleDeviceInfo
    {
        std::string deviceBaseName;
        int maxDeviceCount;

        multipleDeviceInfo()
        {
            maxDeviceCount = 1;
        }
    }MULTIPLE_DEVICE_INFO_T;

    typedef struct sinkRoutingInfo
    {
        EVirtualAudioSink startSink;
        EVirtualAudioSink endSink;
        std::list<EVirtualAudioSink> sinkList;
        sinkRoutingInfo()
        {
            startSink = eVirtualSink_None;
            endSink = eVirtualSink_None;
        }
    }SINK_ROUTING_INFO_T;

    typedef struct sourceRoutingInfo
    {
        EVirtualSource startSource;
        EVirtualSource endSource;
        std::list<EVirtualSource> sourceList;
        sourceRoutingInfo()
        {
            startSource = eVirtualSource_None ;
            endSource = eVirtualSource_None ;
        }
    }SOURCE_ROUTING_INFO_T;

    typedef struct appVolumeInfo
    {
        EVirtualAudioSink audioSink;
        int sinkInputIndex;
        int volume;
        appVolumeInfo()
        {
            audioSink = eVirtualSink_None;
            volume = 100;
            sinkInputIndex = -1;
        }
    }TRACK_VOLUME_INFO_T;

    typedef struct deviceDetails
    {
        int cardNumber;
        std::string deviceName;
        std::string deviceDetail;
        deviceDetails()
        {
            cardNumber = -1;
        }
    }DEVICE_DETAIL;

    typedef struct cardDetail
    {
        std::string cardId;
        std::string cardName;
        std::string name;
        std::string type;
        std::string deviceType;
        std::string devPath;
        int cardNumber;
        int mmap;
        int tsched;
        int fragmentSize;
        int deviceID;
        bool isOutput;
        bool isConnected;
        std::list<std::string> preConditionList;
        cardDetail()
        {
            cardNumber = -1;
            deviceID = -1;
            isConnected = false;
            mmap = 0;
            tsched = 0;
            fragmentSize = 0;
            isOutput = false;
        }
    }CARD_INFO_T;

    typedef std::vector<EVirtualAudioSink> vectorVirtualSink;
    typedef std::vector<EVirtualAudioSink>::iterator itVirtualSink;

    typedef std::vector<EVirtualSource> vectorVirtualSource;
    typedef std::vector<EVirtualSource>::iterator itVirtualSource;


    typedef std::map<EVirtualAudioSink, std::string> mapSinkToStream;
    typedef std::map<EVirtualAudioSink, std::string>::iterator itMapSinkToStream;
    typedef std::map<std::string, EVirtualAudioSink> mapStreamToSink;
    typedef std::map<std::string, EVirtualAudioSink>::iterator itMapStreamToSink;

    typedef std::map<EVirtualSource, std::string> mapSourceToStream;
    typedef std::map<EVirtualSource, std::string>::iterator itMapSourceToStream;
    typedef std::map<std::string, EVirtualSource> mapStreamToSource;
    typedef std::map<std::string, EVirtualSource>::iterator itMapStreamToSource;

    typedef std::map<std::string, utils::SINK_ROUTING_INFO_T> mapSinkRoutingInfo;
    typedef std::map<std::string, utils::SINK_ROUTING_INFO_T>::iterator itMapSinkRoutingInfo;

    typedef std::map<std::string, utils::SOURCE_ROUTING_INFO_T> mapSourceRoutingInfo;
    typedef std::map<std::string, utils::SOURCE_ROUTING_INFO_T>::iterator itMapSourceRoutingInfo;

    typedef std::map<int, std::vector<std::string>> mapDisplaySoundOutputInfo;

    typedef std::map<std::string, int> mapBTDeviceInfo;
    typedef std::map<std::string, int>::iterator itMapBTDeviceInfo;

    typedef std::map<std::string, std::vector<DEVICE_INFO_T>> mapDeviceInfo;
    typedef std::map<std::string, std::vector<TRACK_VOLUME_INFO_T>> mapTrackVolumeInfo;

    typedef std::map<std::string, MULTIPLE_DEVICE_INFO_T> mapMultipleDeviceInfo;

    typedef std::list<DEVICE_DETAIL> listDeviceDetail;

    typedef std::map<std::string, std::vector<CARD_INFO_T>> mapPhysicalInfo;

    typedef std::map<std::string, std::list<std::string>> mapSoundDevicesInfo;

    void LSMessageResponse(LSHandle* handle, LSMessage * message,\
        const char* reply, utils::EReplyType eType, bool isReferenced);
}

typedef enum LunaKeyType {
    eLunaEventBTDeviceStatus = utils::eEventType_Count+1,
    eLunaEventA2DPStatus,
    eLunaEventSettingMediaParam,
    eLunaEventSettingDNDParam,
    eEventBTAdapter,
    eEventBTDeviceStatus,
    eEventA2DPDeviceStatus,
    eEventA2DPSourceStatus,
    eEventPdmDeviceStatus,
    eLunaEventCount,     //Last element should be eLunaEventCount
    eLunaEventKeyFirst = eLunaEventBTDeviceStatus,
    eLunaEventKeyLast = eEventA2DPSourceStatus
}LUNA_KEY_TYPE_E;

template <typename EnumT, typename BaseEnumT>
class ExtendEventEnum
{
    public:
        ExtendEventEnum() {}
        ExtendEventEnum(EnumT e):enum_(e) {}
        ExtendEventEnum(BaseEnumT e):enum_(static_cast<EnumT>(e)) {}
        explicit ExtendEventEnum( int val):enum_(static_cast<EnumT>(val)) {}
        operator EnumT() const { return enum_; }
    private:
        EnumT enum_ = eLunaEventKeyFirst;
};

typedef ExtendEventEnum<LUNA_KEY_TYPE_E, utils::EVENT_TYPE_E> EModuleEventType;

class VirtualSourceSet
{
    public:
        VirtualSourceSet() : mSet(0) {}
        void clear() { mSet = 0; }
        void add(EVirtualSource source) { mSet |= mask(source); }
        void remove(EVirtualSource source) { mSet &= ~mask(source); }
        bool operator == (const VirtualSourceSet & rhs) const { return mSet == rhs.mSet; }
        bool operator != (const VirtualSourceSet & rhs) const { return mSet != rhs.mSet; }
    private:
        long mask(EVirtualSource sink) const
        {
            if (sink >= 0)
                return (long)1 << sink;
            else
                return -1;
        }
        long mSet;
};

//Simple set class to hold a set of sinks & test it
class VirtualSinkSet
{
    public:
        VirtualSinkSet() : mSet(0) {}
        void clear() { mSet = 0; }
        void add(EVirtualAudioSink sink) { mSet |= mask(sink); }
        void remove(EVirtualAudioSink sink) { mSet &= ~mask(sink); }
        bool operator == (const VirtualSinkSet & rhs) const { return mSet == rhs.mSet; }
        bool operator != (const VirtualSinkSet & rhs) const { return mSet != rhs.mSet; }

    private:
        long mask(EVirtualAudioSink sink) const { return (long)1 << sink; }
        long mSet;
};

inline bool IsValidVirtualSink(EVirtualAudioSink sink)
    { return sink >= eVirtualSink_First && sink <= eVirtualUMISink_Last; }

inline bool IsValidVirtualSource(EVirtualSource source)
    { return source >= eVirtualSource_First && source <= eVirtualSource_Last; }

const char * controlEventName(utils::ESINK_STATUS eSinkStatus);
const char * virtualSinkName(EVirtualAudioSink sink, bool prettyName = true);
EVirtualAudioSink getSinkByName(const char * name);
const char * virtualSourceName(EVirtualSource source, bool prettyName = true);
EVirtualSource getSourceByName(const char * name);
LSHandle * GetPalmService();
LSHandle** GetAddressPalmService();
GMainLoop* getMainLoop();

class LSMessageJsonParser;

typedef int (*InitFunction)(void);
typedef int (*StartFunction)(GMainLoop *loop, LSHandle* handle);
typedef void (*CancelSubscriptionCallback)(LSMessage * message, LSMessageJsonParser & msgParser);
typedef int (*ModuleInitFunction)(GMainLoop *loop, LSHandle* handle);

guint64 getCurrentTimeInMs ();

void registerAudioModule(ModuleInitFunction function);
void registerInitFunction (InitFunction function);
void registerModuleFunction (StartFunction function);
void registerControlFunction (StartFunction function);
void registerServiceFunction (StartFunction function);
void registerCancelSubscriptionCallback (CancelSubscriptionCallback function);
void registerCancelSubscription(LSHandle *handle);
void oneInitForAll(GMainLoop *loop, LSHandle *handle);
void oneFreeForAll();
bool ServiceRegisterCategory(const char *category, LSMethod *methods,
        LSSignal *signal, void *category_user_data);

#define INIT_FUNC(func)                                     \
static void __attribute__ ((constructor))                   \
ModuleInitializer##func(void)                               \
{                                                           \
    registerInitFunction(func);                             \
}

#define MODULE_START_FUNC(func)                             \
static void __attribute__ ((constructor))                   \
ModuleInitializer##func(void)                               \
{                                                           \
    registerModuleFunction(func);                           \
}

#define CONTROL_START_FUNC(func)                            \
static void __attribute__ ((constructor))                   \
ModuleInitializer##func(void)                               \
{                                                           \
    registerControlFunction(func);                          \
}

#define SERVICE_START_FUNC(func)                            \
static void __attribute__ ((constructor))                   \
ModuleInitializer##func(void)                               \
{                                                           \
    registerServiceFunction(func);                          \
}

#endif //_UTILS_H_
