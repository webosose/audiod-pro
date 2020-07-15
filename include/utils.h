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


#ifndef _UTILS_H_
#define _UTILS_H_

#include <glib.h>
#include <lunaservice.h>
#include <pulse/module-palm-policy.h>
#include "log.h"
#include "ConstString.h"
#include <pulse/module-palm-policy-tables.h>
#include "IPC_SharedAudiodDefinitions.h"
#include <map>
#include <string>
#include <vector>
#include <pbnjson.h>
#include <pbnjson.hpp>

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
        EnumT enum_;
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

    typedef enum EMixerType
    {
        eMixerNone,
        ePulseMixer,
        eUmiMixer
    }EMIXER_TYPE;

    typedef enum eventType
    {
        eEventNone,
        eEventSinkStatus,
        eEventServerStatusSubscription,
        eEventKeySubscription,
        eEventMixerStatus,
        eEventMasterVolumeStatus,
        eEventCurrentInputVolume,
        eEventInputVolume,
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

    typedef std::vector<EVirtualAudioSink> vectorVirtualSink;
    typedef std::vector<EVirtualAudioSink>::iterator itVirtualSink;

    typedef std::map<EVirtualAudioSink, std::string> mapSinkToStream;
    typedef std::map<EVirtualAudioSink, std::string>::iterator itMapSinkToStream;
    typedef std::map<std::string, EVirtualAudioSink> mapStreamToSink;
    typedef std::map<std::string, EVirtualAudioSink>::iterator itMapStreamToSink;

    void LSMessageResponse(LSHandle* handle, LSMessage * message,\
        const char* reply, utils::EReplyType eType, bool isReferenced);
}

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
