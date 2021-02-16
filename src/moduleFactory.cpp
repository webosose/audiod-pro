// Copyright (c) 2021 LG Electronics, Inc.
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

#include "moduleFactory.h"

ModuleFactory* ModuleFactory::getInstance()
{
    static ModuleFactory moduleManagerInstance;
    return &moduleManagerInstance;
}

ModuleFactory::ModuleFactory()
{
    PM_LOG_INFO(MSGID_MODULE_FACTORY, INIT_KVCOUNT,\
        "ModuleFactory constructor");
}

ModuleFactory::~ModuleFactory()
{
    PM_LOG_INFO(MSGID_MODULE_FACTORY, INIT_KVCOUNT,\
        "ModuleFactory destructor");
}

bool ModuleFactory::Register(const std::string &moduleName, pFuncModuleCreator modulefuncpointer)
{
    PM_LOG_DEBUG("ModuleFactory::Register: registering module: %s", moduleName.c_str());
    if (mModuleCreatorMap.find(moduleName)!= mModuleCreatorMap.end())
        return false;
    mModuleCreatorMap.insert(std::pair<const std::string, pFuncModuleCreator>(moduleName, modulefuncpointer));
    return true;
}

bool ModuleFactory::UnRegister(const std::string &moduleName)
{
    PM_LOG_DEBUG("ModuleFactory::UnRegister: UnRegistering module: %s", moduleName.c_str());
    ModuleInterface* handle = nullptr;
    std::map<std::string, ModuleInterface*>::iterator it = mModuleHandlersMap.begin();

    for ( ; it != mModuleHandlersMap.end(); ++it)
    {
        if (moduleName == it->first)
        {
            handle = it->second;
            if (handle)
                delete handle;
            mModuleHandlersMap.erase(moduleName);
            return true;
        }
    }

    return false;
}

std::map<std::string, ModuleInterface*> ModuleFactory::getRegisteredHandlersMap()
{
    PM_LOG_DEBUG("ModuleFactory::getRegisteredHandlersMap");
    return mModuleHandlersMap;
}

std::map<std::string, pFuncModuleCreator> ModuleFactory::getModulesCreatorMap()
{
    PM_LOG_DEBUG("ModuleFactory::getModulesCreatorMap");
    return mModuleCreatorMap;
}

ModuleInterface* ModuleFactory::CreateObject(const std::string &moduleName, ModuleConfig* const pConfObj)
{
     ModuleInterface* handle = nullptr;
     std::map<std::string, pFuncModuleCreator>::iterator it = mModuleCreatorMap.begin();

     PM_LOG_INFO(MSGID_MODULE_FACTORY, INIT_KVCOUNT,\
        "ModuleFactory Creating object of module: %s", moduleName.c_str());

     for( ; it != mModuleCreatorMap.end(); ++it)
     {
         if (moduleName == it->first)
         {
             handle = it->second(pConfObj);
             if (handle)
             {
                 mModuleHandlersMap.insert(std::pair<const std::string, ModuleInterface*>(moduleName,handle));
                 handle->initialize();
             }
         }
     }
     return handle;
}
