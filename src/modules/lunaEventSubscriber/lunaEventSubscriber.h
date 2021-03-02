/* @@@LICENSE
*
*      Copyright (c) 2020-2021 LG Electronics Company.
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

#ifndef _LUNA_EVENT_SUBSCRIBER_H_
#define _LUNA_EVENT_SUBSCRIBER_H_

#include <cstdlib>
#include <list>
#include <array>
#include <sstream>
#include "utils.h"
#include "messageUtils.h"
#include "main.h"
#include "moduleManager.h"
#include "main.h"
#include "moduleFactory.h"

class lunaEventSubscriber: public ModuleInterface
{
    private:
        lunaEventSubscriber(ModuleConfig* const pConfObj);
        std::array <SUBSCRIPTION_DETAILS_T, eLunaEventCount> mListSubscriptionRequests;    //keep the payload, api, and enum of all subscription
        std::array<bool, eLunaEventCount> mArrayKeySubscribed;                           //To keep list of successfully subscribed Keys
        std::array<bool, eServiceCount> mArrayServerConnected;                      //To keep list of Connection status of all services
        std::array<bool, eLunaEventCount> mArrayKeyReceived;                            //To keep list of Whic API key subscriptions recieved
        std::array<SERVER_TYPE_E, eLunaEventCount> mArrayServerOfKey;                   //Keep a track of servers corresponding to API subscriptions
        bool mServerStatusSubscribed;                                               //Whether the server status is subscribed
        ModuleManager *mObjModuleManager;                                           //Module manager instance
        GMainLoop *mLoop;
        static bool mIsObjRegistered;
        //Register Object to object factory. This is called automatically
        static bool RegisterObject()
        {
            return (ModuleFactory::getInstance()->Register("load_luna_event_subscriber", &lunaEventSubscriber::CreateObject));
        }

    public:
        ~lunaEventSubscriber();
        static lunaEventSubscriber *getLunaEventSubscriber();
        static lunaEventSubscriber *mLunaEventSubscriber;                            //Luna event subscriber instance
        static ModuleInterface* CreateObject(ModuleConfig* const pConfObj)
        {
            if (mIsObjRegistered)
            {
                PM_LOG_DEBUG("CreateObject - Creating the Luna Event Subscriber handler");
                mLunaEventSubscriber = new(std::nothrow)lunaEventSubscriber(pConfObj);
                if (mLunaEventSubscriber)
                    return mLunaEventSubscriber;
            }
            return nullptr;
        }
        void initialize();
        static void subscribeToKeys(LSHandle * handle, LUNA_KEY_TYPE_E event = eLunaEventCount);
        static bool serviceStatusCallBack( LSHandle *sh, const char *serviceName,
            bool connected, void *ctx);
        static bool subscriptionToKeyCallback(LSHandle *lshandle, LSMessage *message, void *ctx);
        //Notification from module manager for a new key subscription request
        void eventSubscribeKey(LUNA_KEY_TYPE_E event, SERVER_TYPE_E eServer,
            const std::string& api, const std::string& payload);
        //Notification from module manager for a server status request
        void eventSubscribeServerStatus(SERVER_TYPE_E eService);
};
#endif
