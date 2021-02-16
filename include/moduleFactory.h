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

#ifndef MODULEFACTORY_H_
#define MODULEFACTORY_H_

#include <map>
#include <string>
#include "moduleInterface.h"
#include "moduleConfig.h"
#include "utils.h"

typedef ModuleInterface* (*pFuncModuleCreator)(ModuleConfig* const pConfObj); //ModuleConfig pointer can be passed as parameter. Reserved for future use

class ModuleFactory
{
private:

    ModuleFactory(const ModuleFactory&) = delete;
    ModuleFactory& operator=(const ModuleFactory&) = delete;
    ModuleFactory();
    std::map<std::string, pFuncModuleCreator> mModuleCreatorMap;
    std::map<std::string, ModuleInterface*> mModuleHandlersMap;

public:

    static ModuleFactory* getInstance();
    ~ModuleFactory();
    bool Register(const std::string &moduleName, pFuncModuleCreator modulefuncpointer);
    bool UnRegister(const std::string &moduleName);
    //ModuleConfig pointer can be passed as parameter. Reserved for future use
    ModuleInterface* CreateObject(const std::string &moduleName, ModuleConfig* const pConfObj);
    std::map<std::string, ModuleInterface*> getRegisteredHandlersMap();
    std::map<std::string, pFuncModuleCreator> getModulesCreatorMap();
};



#endif /* MODULEFACTORY_H_ */
