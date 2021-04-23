/* @@@LICENSE
*
*      Copyright (c) 2021 LG Electronics Company.
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
#include "deviceManager.h"

bool DeviceManager::mIsObjRegistered = DeviceManager::RegisterObject();

DeviceManager* DeviceManager::mObjDeviceManager = nullptr;
DeviceManagerInterface* DeviceManager::mClientDeviceManagerInstance = nullptr;
DeviceManagerInterface* DeviceManagerInterface::mClientInstance = nullptr;
pFuncCreateClient DeviceManagerInterface::mClientFuncPointer = nullptr;

void DeviceManager::eventMixerStatus (bool mixerStatus, utils::EMIXER_TYPE mixerType)
{
    if (mixerStatus && (mixerType == utils::ePulseMixer))
    {
        FILE *fp;
        int HDMI0CardNumber = 0;
        int HeadphoneCardNumber = -1;
        char snd_card_info[500];
        char *snd_hdmi0_card_info_parse = NULL;
        char *snd_headphone_card_info_parse = NULL;
        int lengthOfHDMI0Card = strlen("b1");
        int lengthOfHeadphoneCard = strlen("Headphones");
        if ((fp = fopen("/proc/asound/cards", "r")) == NULL )
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Cannot open /proc/asound/cards file to get sound card info");
        }

        while (fgets(snd_card_info, sizeof(snd_card_info), fp) != NULL)
        {
            PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"Found card %s", snd_card_info);
            snd_hdmi0_card_info_parse = strstr(snd_card_info, "b1");
            snd_headphone_card_info_parse = strstr(snd_card_info, "Headphones");
            if (snd_hdmi0_card_info_parse && !strncmp(snd_hdmi0_card_info_parse, "b1", lengthOfHDMI0Card))
            {
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"Found internal card b1");
                char card = snd_card_info[1];
                HDMI0CardNumber =  card - '0';
            }
            if (snd_headphone_card_info_parse && !strncmp(snd_headphone_card_info_parse, "Headphones", lengthOfHeadphoneCard) && (-1 == HeadphoneCardNumber))
            {
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"Found internal card Headphones");
                char card = snd_card_info[1];
                HeadphoneCardNumber =  card - '0';
            }
        }
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
        mObjAudioMixer->loadInternalSoundCard('i', HDMI0CardNumber, 0, 1, true, "pcm_output");
        mObjAudioMixer->loadInternalSoundCard('i', HeadphoneCardNumber, 0, 1, false, "pcm_headphone");
    }
}

bool DeviceManager::_event(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("DeviceManager: event");
    std::string reply = STANDARD_JSON_SUCCESS;
    LSMessageJsonParser    msg(message, SCHEMA_3(REQUIRED(event, string),\
        OPTIONAL(soundcard_no, integer),OPTIONAL(device_no, integer)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;
    //read optional parameters with appropriate default values
    int soundcard_no = -1;
    int device_no = -1;
    if (!msg.get("soundcard_no", soundcard_no))
        soundcard_no = 0;
    if (!msg.get("device_no", device_no))
        device_no = 0;
    std::string event;
    if (!msg.get("event", event))
    {
        reply = MISSING_PARAMETER_ERROR(event, string);
    }
    else
    {
        if (mClientDeviceManagerInstance)
        {
            Device device;
            device.event = event;
            device.soundCardNumber = soundcard_no;
            device.deviceNumber = device_no;
            if ("headset-inserted" == event || "usb-mic-inserted" == event || "usb-headset-inserted" == event)
            {
                reply = mClientDeviceManagerInstance->onDeviceAdded(&device);
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManager: device added event call to device manager client object is success");
            }
            else if ("headset-removed" == event || "usb-mic-removed" == event || "usb-headset-removed" == event)
            {
                reply = mClientDeviceManagerInstance->onDeviceRemoved(&device);
                PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "DeviceManager: device removed event call to device manager client object is success");
            }
            else
            {
                PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Invalid device event received");
                reply = INVALID_PARAMETER_ERROR(event, string);
            }
        }
        else
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "Client DeviceManagerInstance is nullptr");
            reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "DeviceManager Instance is nullptr");
        }
    }
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return true;
}

DeviceManager* DeviceManager::getDeviceManagerInstance()
{
    return mObjDeviceManager;
}

DeviceManager::DeviceManager(ModuleConfig* const pConfObj)
{
    PM_LOG_DEBUG("DeviceManager constructor");
    mClientDeviceManagerInstance = DeviceManagerInterface::getClientInstance();
    if (!mClientDeviceManagerInstance)
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "mClientDeviceManagerInstance is nullptr");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    mObjAudioMixer = AudioMixer::getAudioMixerInstance();
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventMixerStatus);
    }
}

DeviceManager::~DeviceManager()
{
    PM_LOG_DEBUG("DeviceManager destructor");
    if (mClientDeviceManagerInstance)
    {
        delete mClientDeviceManagerInstance;
        mClientDeviceManagerInstance = nullptr;
    }
}

LSMethod DeviceManager::deviceManagerMethods[] = {
    { "event", _event},
    {},
};

void DeviceManager::initialize()
{
    if (mObjDeviceManager)
    {
        bool result = false;
        CLSError lserror;

        result = LSRegisterCategoryAppend(GetPalmService(), "/udev", DeviceManager::deviceManagerMethods, nullptr, &lserror);
        if (!result || !LSCategorySetData(GetPalmService(), "/udev", mObjDeviceManager, &lserror))
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
                "%s: Registering Service for '%s' category failed", __FUNCTION__, "/udev");
        }
        PM_LOG_INFO(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
            "Successfully initialized DeviceManager");
    }
    else
    {
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, \
            "mObjDeviceManager is nullptr");
    }
}

void DeviceManager::deInitialize()
{
    PM_LOG_DEBUG("DeviceManager deInitialize()");
    if (mObjDeviceManager)
    {
        delete mObjDeviceManager;
        mObjDeviceManager = nullptr;
    }
    else
        PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT, "mObjDeviceManager is nullptr");
}

void DeviceManager::handleEvent(events::EVENTS_T* ev)
{
    switch(ev->eventName)
    {
        case utils::eEventMixerStatus:
        {
            events::EVENT_MIXER_STATUS_T *stMixerStatus = (events::EVENT_MIXER_STATUS_T*)ev;
            eventMixerStatus(stMixerStatus->mixerStatus, stMixerStatus->mixerType);
        }
        break;
        default:
        {
            PM_LOG_ERROR(MSGID_DEVICE_MANAGER, INIT_KVCOUNT,"unknown event");
        }
        break;
    }
}
