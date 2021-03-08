// Copyright (c) 2020-2021 LG Electronics, Inc.
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
MasterVolumeInterface* MasterVolumeManager::mClientMasterInstance = nullptr;
MasterVolumeManager* MasterVolumeManager::mMasterVolumeManager = nullptr;
SoundOutputManager* MasterVolumeManager::mObjSoundOutputManager = nullptr;

MasterVolumeManager* MasterVolumeManager::getMasterVolumeManagerInstance()
{
    return mMasterVolumeManager;
}

void MasterVolumeManager::setInstance(MasterVolumeInterface* clientMasterVolumeInstance)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setInstance");
    MasterVolumeManager::mClientMasterInstance = clientMasterVolumeInstance;
}

MasterVolumeInterface* MasterVolumeManager::getClientMasterInstance()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getClientMasterInstance");
    return mClientMasterInstance;
}

bool MasterVolumeManager::_setVolume(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    MasterVolumeInterface* clientMasterInstance = nullptr;
    clientMasterInstance = masterVolumeInstance->getClientMasterInstance();
    if (clientMasterInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: setVolume call to master client object is success");
        clientMasterInstance->setVolume(lshandle, message, ctx);
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
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    MasterVolumeInterface* clientMasterInstance = nullptr;
    clientMasterInstance = masterVolumeInstance->getClientMasterInstance();
    if (clientMasterInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: getVolume call to master client object is success");
        clientMasterInstance->getVolume(lshandle, message, ctx);
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
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume");
    std::string reply = STANDARD_JSON_SUCCESS;
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    MasterVolumeInterface* clientMasterInstance = nullptr;
    clientMasterInstance = masterVolumeInstance->getClientMasterInstance();
    if (clientMasterInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: muteVolume call to master client object is success");
        clientMasterInstance->muteVolume(lshandle, message, ctx);
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
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp");
    std::string reply = STANDARD_JSON_SUCCESS;
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    MasterVolumeInterface* clientMasterInstance = nullptr;
    clientMasterInstance = masterVolumeInstance->getClientMasterInstance();
    if (clientMasterInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeUp call to master client object is success");
        clientMasterInstance->volumeUp(lshandle, message, ctx);
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
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown");
    std::string reply = STANDARD_JSON_SUCCESS;
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    MasterVolumeInterface* clientMasterInstance = nullptr;
    clientMasterInstance = masterVolumeInstance->getClientMasterInstance();
    if (clientMasterInstance)
    {
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolume: volumeDown call to master client object is success");
        clientMasterInstance->volumeDown(lshandle, message, ctx);
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
    MasterVolumeManager* masterVolumeInstance = MasterVolumeManager::getMasterVolumeManagerInstance();
    MasterVolumeInterface* clientMasterInstance = nullptr;
    clientMasterInstance = masterVolumeInstance->getClientMasterInstance();
    if (clientMasterInstance)
    {
        clientMasterInstance->setMuteStatus(DEFAULT_ONE_DISPLAY_ID);
        clientMasterInstance->setVolume(DEFAULT_ONE_DISPLAY_ID);
        clientMasterInstance->setMuteStatus(DEFAULT_TWO_DISPLAY_ID);
        clientMasterInstance->setVolume(DEFAULT_TWO_DISPLAY_ID);
    }
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
    { },
};

static LSMethod SoundOutputManagerMethods[] = {
    {"setSoundOut", SoundOutputManager::_SetSoundOut},
    { },
};

void MasterVolumeManager::initSoundOutputManager()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "initSoundOutputManager::initialising sound output manager module");
    CLSError lserror;
    if (!mObjSoundOutputManager)
    {
        mObjSoundOutputManager = new (std::nothrow)SoundOutputManager();
        if (mObjSoundOutputManager)
        {
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "load module soundOutputManager successful");
            bool result = LSRegisterCategoryAppend(GetPalmService(), "/soundSettings", SoundOutputManagerMethods, nullptr, &lserror);
            if (!result || !LSCategorySetData(GetPalmService(), "/soundSettings", mObjSoundOutputManager, &lserror))
            {
                PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT,\
                    "%s: Registering Service for '%s' category failed", __FUNCTION__, "/soundSettings");
                lserror.Print(__FUNCTION__, __LINE__);
            }
        }
        else
            PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Could not load module soundOutputManager");
    }
}

void MasterVolumeManager::initialize()
{
    CLSError lserror;
    if (mMasterVolumeManager)
    {
        bool bRetVal = LSRegisterCategoryAppend(GetPalmService(), "/master", MasterVolumeMethods, nullptr, &lserror);
        if (!bRetVal || !LSCategorySetData(GetPalmService(), "/master", mMasterVolumeManager, &lserror))
        {
           PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "%s: Registering Service for '%s' category failed", \
                        __FUNCTION__, "/master");
           lserror.Print(__FUNCTION__, __LINE__);
        }
        initSoundOutputManager();
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, \
            "Successfully initialized MasterVolumeManager");
    }
    else
        PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "Could not load module MasterVolumeManager");
}

MasterVolumeManager::MasterVolumeManager(ModuleConfig* const pConfObj): mObjAudioMixer(AudioMixer::getAudioMixerInstance())
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolumeManager: constructor");
    mObjModuleManager = ModuleManager::getModuleManagerInstance();
    if (mObjModuleManager)
        mObjModuleManager->subscribeModuleEvent(this, true, utils::eEventMasterVolumeStatus);
    else
        PM_LOG_ERROR(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "mObjModuleManager is null");
}

MasterVolumeManager::~MasterVolumeManager()
{
    PM_LOG_INFO(MSGID_MASTER_VOLUME_MANAGER, INIT_KVCOUNT, "MasterVolumeManager: destructor");
    if (mObjSoundOutputManager)
    {
        delete mObjSoundOutputManager;
        mObjSoundOutputManager = nullptr;
    }
}