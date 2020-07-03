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

enum EUMISink
{
    eUmiSink
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
}

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
