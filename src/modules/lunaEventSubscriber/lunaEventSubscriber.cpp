/* @@@LICENSE
*
*      Copyright (c) 2020-2023 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include "lunaEventSubscriber.h"

serverNameMap statusSubscriptionMap =
{
    { "com.palm.bluetooth", eBluetoothService},
    { "com.webos.service.bluetooth2", eBluetoothService2},
    { "com.webos.service.audiooutput", eAudiooutputdService},
    { "com.webos.settingsservice", eSettingsService},
    { "com.webos.service.pdm", ePdmService},
    { "com.webos.service.settings", eSettingsService2},
};

lunaEventSubscriber * lunaEventSubscriber::mLunaEventSubscriber = nullptr;
bool lunaEventSubscriber::mIsObjRegistered = lunaEventSubscriber::RegisterObject();

lunaEventSubscriber * lunaEventSubscriber::getLunaEventSubscriber()
{
    return mLunaEventSubscriber;
}

lunaEventSubscriber::lunaEventSubscriber(ModuleConfig* const pConfObj) : mServerStatusSubscribed(false)
{
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    mArrayKeySubscribed.fill(false);
    mArrayServerConnected.fill(false);
    mArrayKeyReceived.fill(false);
    mArrayServerOfKey.fill(eServiceFirst);
    mLoop = nullptr;
    PM_LOG_DEBUG("lunaEventSubscriber::Constructor");
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventLunaKeySubscription);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventLunaServerStatusSubscription);
        PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,\
            "Subscribed to eEventKeySubscription & eEventServerStatusSubscription");
    }
    else
    {
        PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,\
            "lunaEventSubscriber :: Module manager instance null");
    }
}

lunaEventSubscriber::~lunaEventSubscriber()
{
    PM_LOG_DEBUG("lunaEventSubscriber::Destructor");
}

void lunaEventSubscriber::initialize()
{
    if (mLunaEventSubscriber == nullptr)
    {
        PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER, INIT_KVCOUNT, "Initializing Luna event subscriber module failed");
    }
    else
    {
        PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER, INIT_KVCOUNT, "Successfully initialized Luna event subscriber module");
        mLunaEventSubscriber->mLoop = getMainLoop();
    }
}

void lunaEventSubscriber::deInitialize()
{
    PM_LOG_DEBUG("lunaEventSubscriber deInitialize()");
    if (mLunaEventSubscriber)
    {
        delete mLunaEventSubscriber;
        mLunaEventSubscriber = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER, INIT_KVCOUNT, "mLunaEventSubscriber is nullptr");
}

void lunaEventSubscriber::handleEvent(events::EVENTS_T* event)
{
    switch(event->eventName)
    {
        case utils::eEventLunaServerStatusSubscription:
        {
            PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER, INIT_KVCOUNT,\
                    "handleEvent:: eEventLunaServerStatusSubscription");
            events::EVENT_SUBSCRIBE_SERVER_STATUS_T *serverStatusInfoEvent = (events::EVENT_SUBSCRIBE_SERVER_STATUS_T*)event;
            eventSubscribeServerStatus(serverStatusInfoEvent->serviceName);
        }
        break;
        case utils::eEventLunaKeySubscription:
        {
            PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER, INIT_KVCOUNT,\
                "handleEvent:: eEventLunaKeySubscription");
            events::EVENT_SUBSCRIBE_KEY_T *keySubscribeInfoEvent = (events::EVENT_SUBSCRIBE_KEY_T*)event;
            eventSubscribeKey(keySubscribeInfoEvent->type, keySubscribeInfoEvent->serviceName, keySubscribeInfoEvent->api, keySubscribeInfoEvent->payload);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_LUNA_EVENT_SUBSCRIBER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}

bool lunaEventSubscriber::subscriptionToKeyCallback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    //We pass the subscription Key enum as ctx,
    //To identify from where we recieved a callback
    char *cb = (char*)ctx;
    std::stringstream cbFrom = std::stringstream(cb);
    int ctxVar = -1;
    cbFrom >> ctxVar;
    EModuleEventType eEventToSubscribe = (EModuleEventType)ctxVar;
    if (eEventToSubscribe < eLunaEventKeyFirst || eEventToSubscribe >= eLunaEventCount)
    {
        PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,\
            "Invalid event type");
        return false;
    }
    PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,
        "Subscription Callback recieved from ctx = %s,%d", cb, (int)eEventToSubscribe);
    ModuleManager *pInstance = ModuleManager::getModuleManagerInstance();
    if ( nullptr != pInstance)
    {
        events::EVENT_KEY_INFO_T eventKeyInfo;
        eventKeyInfo.eventName = eEventToSubscribe;
        eventKeyInfo.type = eEventToSubscribe;
        eventKeyInfo.message = message;
        pInstance->publishModuleEvent((events::EVENTS_T*)&eventKeyInfo);
    }
    else
    {
        PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,\
            "module manager instance in null");
    }

    return true;
}

bool lunaEventSubscriber::serviceStatusCallBack( LSHandle *sh,
    const char *serviceName,
    bool connected,
    void *ctx)
{
    PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
        "Got Server Status callback from %s:%d", serviceName, (int)connected);
    SERVER_TYPE_E eServerStatus = statusSubscriptionMap[serviceName];
    ModuleManager *pInstance = ModuleManager::getModuleManagerInstance();
    if (nullptr != pInstance)
    {
        events::EVENT_SERVER_STATUS_INFO_T eventServerStatus;
        eventServerStatus.eventName = utils::eEventServerStatusSubscription;
        eventServerStatus.serviceName = eServerStatus;
        eventServerStatus.connectionStatus = connected;
        pInstance->publishModuleEvent((events::EVENTS_T*)&eventServerStatus);
    }
    else
    {
         PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
            "module manager instance is Null");
    }
    mLunaEventSubscriber->mArrayServerConnected[eServerStatus] = connected;
    if (connected)
    {
        //Loops through the list of subscrpition, and register subscription
        subscribeToKeys(sh);
    }
    else
    {
        //unsubscribe
        for(int i = eLunaEventKeyFirst; i < eLunaEventCount; i++)
        {
            if (mLunaEventSubscriber->mArrayServerOfKey[i] == eServerStatus)
            {
                mLunaEventSubscriber->mArrayKeySubscribed[i] = false;
            }
        }
    }
    return true;
}

void lunaEventSubscriber::subscribeToKeys(LSHandle *handle, EModuleEventType eEventToSubscribe)
{
    PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
        "%s called, eEventToSubscribe = %d", __FUNCTION__, (int)eEventToSubscribe);
    if (eLunaEventCount == eEventToSubscribe)
    {
        for(int it = eLunaEventKeyFirst; it < eLunaEventCount; it++)
        {
            PM_LOG_DEBUG("request for : %d keyrecieved = %d",\
                (int)it, mLunaEventSubscriber->mArrayKeyReceived[it]);
            if (mLunaEventSubscriber->mArrayKeyReceived[it] == false)
            {
                PM_LOG_DEBUG("Not subscribed");
                    continue;
            }
            if (it == eLunaEventBTDeviceStatus ||
                it == eLunaEventA2DPStatus)
            {
                PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
                    "Dynamic subscription");
                //Since BT has dynamic payload,
                //Let it subscribe from BT manager module itself
                continue;
            }
            SERVER_TYPE_E server = mLunaEventSubscriber->mArrayServerOfKey[it];
            if (mLunaEventSubscriber->mArrayServerConnected[server] == false)
            {
                PM_LOG_DEBUG("Service %d not connected",server);
            }
            else if (mLunaEventSubscriber->mArrayKeySubscribed[it] == false)
            {
                CLSError lserror;
                char *ctx = (char*)mLunaEventSubscriber->mListSubscriptionRequests[it].ctx.c_str();

                PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
                    "Register Subscription : Key = %s, Payload = %s, ctx = %s",\
                    mLunaEventSubscriber->mListSubscriptionRequests[it].sKey.c_str(),\
                    mLunaEventSubscriber->mListSubscriptionRequests[it].payload.c_str(),ctx);

                if (!LSCall(handle, mLunaEventSubscriber->mListSubscriptionRequests[it].sKey.c_str(),
                    mLunaEventSubscriber->mListSubscriptionRequests[it].payload.c_str(),
                    subscriptionToKeyCallback,
                    ctx, NULL, &lserror))
                {
                    mLunaEventSubscriber->mArrayKeySubscribed[it] = false;
                    lserror.Print(__func__, __LINE__);
                }
                else
                {
                    PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
                        "Register Subscription success");
                    mLunaEventSubscriber->mArrayKeySubscribed[it] = true;
                }
            }
            else
            {
                PM_LOG_DEBUG("Already subscribed");
            }
        }
    }
    else
    {
        SERVER_TYPE_E server = mLunaEventSubscriber->mArrayServerOfKey[eEventToSubscribe];
        if (mLunaEventSubscriber->mArrayServerConnected[server] == false)
        {
            PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
                "Service not connected %d", server);
        }
        else if (mLunaEventSubscriber->mArrayKeySubscribed[eEventToSubscribe] == false)
        {
            CLSError lserror;
            char *ctx = (char*)mLunaEventSubscriber->mListSubscriptionRequests[eEventToSubscribe].ctx.c_str();
            PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
                "Register Subscription : Key = %s, Payload = %s, ctx = %s",\
                mLunaEventSubscriber->mListSubscriptionRequests[eEventToSubscribe].sKey.c_str(),
                mLunaEventSubscriber->mListSubscriptionRequests[eEventToSubscribe].payload.c_str(),
                ctx);
            if (!LSCall(handle, mLunaEventSubscriber->mListSubscriptionRequests[eEventToSubscribe].sKey.c_str(),
                mLunaEventSubscriber->mListSubscriptionRequests[eEventToSubscribe].payload.c_str(),
                subscriptionToKeyCallback,
                ctx, NULL, &lserror))
            {
                lserror.Print(__func__, __LINE__);
                mLunaEventSubscriber->mArrayKeySubscribed[eEventToSubscribe] = false;
            }
            else
            {
                mLunaEventSubscriber->mArrayKeySubscribed[eEventToSubscribe] = true;
            }
        }
        else
        {
            PM_LOG_DEBUG("Already subscribed");
        }
    }
}

void lunaEventSubscriber::eventSubscribeServerStatus(SERVER_TYPE_E eService)
{
    CLSError lserror;
    PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
        "lunaEventSubscriber::eventSubscribeServerStatus : %d",eService);
    if (mServerStatusSubscribed==false)
    {
        for(auto it:statusSubscriptionMap)
        {
            bool result = LSRegisterServerStatusEx(GetPalmService(), it.first.c_str(),
                serviceStatusCallBack, mLoop, NULL, &lserror);
            if (!result)
            {
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        mServerStatusSubscribed = true;
    }

    if (mObjModuleManager != nullptr)
    {
        bool connected = mArrayServerConnected[eService];
        events::EVENT_SERVER_STATUS_INFO_T eventServerStatus;
        eventServerStatus.eventName = utils::eEventServerStatusSubscription;
        eventServerStatus.serviceName = eService;
        eventServerStatus.connectionStatus = connected;
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventServerStatus);
    }
    else
    {
        PM_LOG_ERROR(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
            "Module manager instance null");
    }
}

void lunaEventSubscriber::eventSubscribeKey(EModuleEventType event,
        SERVER_TYPE_E eServer,
        const std::string& api,
        const std::string& payload)
{
    PM_LOG_INFO(MSGID_LUNA_EVENT_SUBSCRIBER,INIT_KVCOUNT,   \
        "call to subscribe to keys EModuleEventType = %d, SERVER_TYPE_E =%d",\
        event, eServer);
    //Based on the Service connected, We subscribe to the keys.
    mArrayServerOfKey[event] = eServer;

    if (mServerStatusSubscribed == false)
    {
        eventSubscribeServerStatus(eServer);
    }

    //Every time a new subscription is recived, register again
    //So that the new subscriber will get the current value.
    //every subscriber will get a new value, but it is duty of
    //the modules to check whether the value changed or not
    LSHandle *sh = GetPalmService();
    SUBSCRIPTION_DETAILS_T sData;
    sData.sKey = api;
    sData.payload = payload;
    sData.ctx = std::to_string(event);
    mListSubscriptionRequests[event] = sData;
    mArrayKeyReceived[event]=true;
    mArrayKeySubscribed[event]=false;
    subscribeToKeys(sh, event);
}
