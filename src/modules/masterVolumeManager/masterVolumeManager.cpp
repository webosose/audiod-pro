// Copyright (c) 2020-2023 LG Electronics, Inc.
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

#include "masterVolumeManager.h"
#define DEFAULT_ONE_DISPLAY_ID 1
#define DEFAULT_TWO_DISPLAY_ID 2

bool MasterVolumeManager::mIsObjRegistered = MasterVolumeManager::RegisterObject();
MasterVolumeManager* MasterVolumeManager::mMasterVolumeManager = nullptr;
MasterVolumeInterface* MasterVolumeManager::mMasterVolumeClientInstance = nullptr;
MasterVolumeInterface* MasterVolumeInterface::mClientInstance = nullptr;
pFuncCreateClient MasterVolumeInterface::mClientFuncPointer = nullptr;

MasterVolumeManager* MasterVolumeManager::getMasterVolumeManagerInstance()
{
    return mMasterVolumeManager;
}

bool MasterVolumeManager::_setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: setVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setVolume call to master client object is success");
        mMasterVolumeClientInstance->setVolume(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_setMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: setMicVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setMicVolume call to master client object is success");
        mMasterVolumeClientInstance->setMicVolume(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_getVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: getVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getVolume call to master client object is success");
        mMasterVolumeClientInstance->getVolume(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_getMicVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: getMicVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getMicVolume call to master client object is success");
        mMasterVolumeClientInstance->getMicVolume(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_muteVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: muteVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume call to master client object is success");
        mMasterVolumeClientInstance->muteVolume(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_muteMic(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: muteMic");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteMic call to master client object is success");
        mMasterVolumeClientInstance->muteMic(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_volumeUp(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: volumeUp");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp call to master client object is success");
        mMasterVolumeClientInstance->volumeUp(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

bool MasterVolumeManager::_volumeDown(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_DEBUG("MasterVolume: volumeDown");
    std::string reply = STANDARD_JSON_SUCCESS;
    if (mMasterVolumeClientInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown call to master client object is success");
        mMasterVolumeClientInstance->volumeDown(lshandle, message, ctx);
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "MasterVolume Instance is nullptr");
        CLSError lserror;
        if (!LSMessageReply(lshandle, message, reply.c_str(), &lserror))
            lserror.Print(__FUNCTION__, __LINE__);
        return true;
    }
    return true;
}

void MasterVolumeManager::eventMasterVolumeStatus()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "eventMasterVolumeStatus: setMuteStatus and setVolume");
    if (mMasterVolumeClientInstance)
    {
        mMasterVolumeClientInstance->setMuteStatus(DEFAULT_ONE_DISPLAY_ID);
        mMasterVolumeClientInstance->setVolume(DEFAULT_ONE_DISPLAY_ID);
        mMasterVolumeClientInstance->setMuteStatus(DEFAULT_TWO_DISPLAY_ID);
        mMasterVolumeClientInstance->setVolume(DEFAULT_TWO_DISPLAY_ID);
    }
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
}

void MasterVolumeManager::eventActiveDeviceInfo(const std::string deviceName,\
    const std::string& display, const bool& isConnected, const bool& isOutput)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
        "eventActiveDeviceInfo deviceName:%s display:%s isConnected:%d isOutput:%d", \
        deviceName.c_str(), display.c_str(), (int)isConnected, (int)isOutput);
    std::string device;
    if (isOutput)
    {
        mMasterVolumeClientInstance->setDisplaySoundOutput(display, deviceName,isConnected);
        if (isConnected)
        {
            mMasterVolumeClientInstance->setMuteStatus(DEFAULT_ONE_DISPLAY_ID);
            mMasterVolumeClientInstance->setVolume(DEFAULT_ONE_DISPLAY_ID);
            mMasterVolumeClientInstance->setMuteStatus(DEFAULT_TWO_DISPLAY_ID);
            mMasterVolumeClientInstance->setVolume(DEFAULT_TWO_DISPLAY_ID);
        }
    }
    else
    {
        mMasterVolumeClientInstance->setDisplaySoundInput(display, deviceName, isConnected);
    }
}

void MasterVolumeManager::eventResponseSoundOutputDeviceInfo(utils::mapSoundDevicesInfo soundOutputInfo)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventResponseSoundOutputDeviceInfo");
    if (mMasterVolumeClientInstance)
    {
        mMasterVolumeClientInstance->setSoundOutputInfo(soundOutputInfo);
    }
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
}

void MasterVolumeManager::eventRequestSoundOutputDeviceInfo()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventRequestSoundOutputDeviceInfo");
    if (mObjModuleManager)
    {
        events::EVENT_REQUEST_SOUNDOUTPUT_INFO_T eventRequestSoundOutputDeviceInfo;
        eventRequestSoundOutputDeviceInfo.eventName = utils::eEventRequestSoundOutputDeviceInfo;
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventRequestSoundOutputDeviceInfo);
    }
    else
        PM_LOG_ERROR (MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
            "eventRequestSoundOutputDeviceInfo: mObjModuleManager is null");
}

void MasterVolumeManager::eventRequestSoundInputDeviceInfo()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventRequestSoundInputDeviceInfo");
    if (mObjModuleManager)
    {
        events::EVENT_REQUEST_SOUNDOUTPUT_INFO_T eventRequestSoundInputDeviceInfo;
        eventRequestSoundInputDeviceInfo.eventName = utils::eEventRequestSoundInputDeviceInfo;
        mObjModuleManager->publishModuleEvent((events::EVENTS_T*)&eventRequestSoundInputDeviceInfo);
    }
    else
        PM_LOG_ERROR (MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
            "eventRequestSoundInputDeviceInfo: mObjModuleManager is null");
}

void MasterVolumeManager::eventResponseSoundInputDeviceInfo(utils::mapSoundDevicesInfo soundInputInfo)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,"eventResponseSoundInputDeviceInfo");
    if (mMasterVolumeClientInstance)
    {
        mMasterVolumeClientInstance->setSoundInputInfo(soundInputInfo);
    }
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
}

/* TODO
currently these luna API's are not in sync with exsting master volume
In future existing master volume will be removed and these below luna API's will be used
for master volume handling as per Generic AV arch and made in sync with remote and other services*/

static LSMethod MasterVolumeMethods[] =
{
    {"setVolume", MasterVolumeManager::_setVolume},
    {"volumeDown", MasterVolumeManager::_volumeDown},
    {"volumeUp", MasterVolumeManager::_volumeUp},
    {"getVolume", MasterVolumeManager::_getVolume},
    {"muteVolume", MasterVolumeManager::_muteVolume},
    {"setMicVolume", MasterVolumeManager::_setMicVolume},
    {"getMicVolume", MasterVolumeManager::_getMicVolume},
    {"muteMic", MasterVolumeManager::_muteMic},
    { },
};

void MasterVolumeManager::initialize()
{
    CLSError lserror;
    if (mMasterVolumeManager)
    {
        bool bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/master", MasterVolumeMethods, nullptr, &lserror);
        if (!bRetVal || !LSCategorySetData(GetPalmService(), "/master", mMasterVolumeManager, &lserror))
        {
           PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s: Registering Service for '%s' category failed", \
                        __FUNCTION__, "/master");
           lserror.Print(__FUNCTION__, __LINE__);
        }
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
            "Successfully initialized MasterVolumeManager");
    }
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Could not load module MasterVolumeManager");
}

void MasterVolumeManager::deInitialize()
{
    PM_LOG_DEBUG("MasterVolumeManager deInitialize()");
    if (mMasterVolumeManager)
    {
        delete mMasterVolumeManager;
        mMasterVolumeManager = nullptr;
    }
    else
    {
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Client MasterVolumeInstance is nullptr");
    }
}

void MasterVolumeManager::handleEvent(events::EVENTS_T *event)
{
    switch(event->eventName)
    {
        case utils::eEventMasterVolumeStatus:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventMasterVolumeStatus");
            events::EVENT_MASTER_VOLUME_STATUS_T *masterVolumeStatusEvent = (events::EVENT_MASTER_VOLUME_STATUS_T*)event;
            eventMasterVolumeStatus();
        }
        break;
        case utils::eEventActiveDeviceInfo:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventActiveDeviceInfo");
            events::EVENT_ACTIVE_DEVICE_INFO_T *stActiveDeviceInfo = (events::EVENT_ACTIVE_DEVICE_INFO_T*)event;
            eventActiveDeviceInfo(stActiveDeviceInfo->deviceName, stActiveDeviceInfo->display, stActiveDeviceInfo->isConnected,
                stActiveDeviceInfo->isOutput);
        }
        break;
        case utils::eEventResponseSoundOutputDeviceInfo:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventResponseSoundOutputDeviceInfo");
            events::EVENT_RESPONSE_SOUNDOUTPUT_INFO_T *stSoundOutputDeviceInfo = (events::EVENT_RESPONSE_SOUNDOUTPUT_INFO_T*)event;
            eventResponseSoundOutputDeviceInfo(stSoundOutputDeviceInfo->soundOutputInfo);
        }
        break;
        case utils::eEventResponseSoundInputDeviceInfo:
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "handleEvent:: eEventResponseSoundInputDeviceInfo");
            events::EVENT_RESPONSE_SOUNDINPUT_INFO_T *stSoundInputDeviceInfo = (events::EVENT_RESPONSE_SOUNDINPUT_INFO_T*)event;
            eventResponseSoundInputDeviceInfo(stSoundInputDeviceInfo->soundInputInfo);
        }
        break;
        default:
        {
            PM_LOG_WARNING(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                "handleEvent:Unknown event");
        }
        break;
    }
}

MasterVolumeManager::MasterVolumeManager(ModuleConfig* const pConfObj): mObjAudioMixer(AudioMixer::getAudioMixerInstance())
{
    PM_LOG_DEBUG("MasterVolumeManager: constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
    {
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventMasterVolumeStatus);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventActiveDeviceInfo);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventResponseSoundOutputDeviceInfo);
        mObjModuleManager->subscribeModuleEvent(this, utils::eEventResponseSoundInputDeviceInfo);
    }
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "mObjModuleManager is nullptr");
    mMasterVolumeClientInstance = MasterVolumeInterface::getClientInstance();
    if (!mMasterVolumeClientInstance)
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "mMasterVolumeClientInstance is nullptr");
    eventRequestSoundOutputDeviceInfo();
    eventRequestSoundInputDeviceInfo();
}

MasterVolumeManager::~MasterVolumeManager()
{
    PM_LOG_DEBUG("MasterVolumeManager: destructor");
    if (mMasterVolumeClientInstance)
    {
        delete mMasterVolumeClientInstance;
        mMasterVolumeClientInstance = nullptr;
    }
}