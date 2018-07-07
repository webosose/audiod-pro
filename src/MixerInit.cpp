// Copyright (c) 2018 LG Electronics, Inc.
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


#include <pbnjson/cxx/JDomParser.h>
#include "umiaudiomixer.h"
#include  "MixerInit.h"
using namespace pbnjson;

MixerInit::MixerInit(const std::stringstream& configFilePath): mCongifPath(configFilePath.str())
{
  g_debug("MixerInit constructor");
}

MixerInit::~MixerInit()
{
  g_debug("MixerInit distructor");
}

bool MixerInit::readMixerConfig()
{
  g_debug("MixerInit Reading mixer config");
  bool bReturnStatus = false;
  JValue configJson = JDomParser::fromFile(mCongifPath.c_str(),JSchema::AllSchema());
  if (!configJson.isValid() || !configJson.isObject())
  {
    g_debug("Failed to parse mixerconfig file using defaults. File: %s. Error: %s", mCongifPath.c_str(), configJson.errorString().c_str());
    return bReturnStatus;
  }
  if (configJson.hasKey("audioMixer"))
  {
    g_debug("Found audioMixer");
    if (parseAudioMixer(configJson["audioMixer"]))
      bReturnStatus = true;
  }
  else
  {
    g_debug("Did not find audioMixer");
  }
return bReturnStatus;
}

bool MixerInit::parseAudioMixer(pbnjson::JValue object)
{
  if (!object.isArray())
  {
    g_debug("Failed to read audio mixer. using defaults");
    return false;
  }
  std::string strMixer;
  mSetMixer.clear();
  for (auto &elements:object.items())
  {
    strMixer = elements.asString();
    g_debug("MixerInit: Read mixer : %s", strMixer.c_str());
    mSetMixer.insert(strMixer);
  }
  return true;
}
void MixerInit::initMixerInterface()
{
  // Iterate till the end of set
  for (auto &elements:mSetMixer)
  {
    if (!elements.compare("pulseaudio"))
    {
      g_debug("Initializing pulse audio mixer");
      /*TODO pulse audio mixer object should also be created dynamically based on mixer config, currently its static*/
    }
    else if (!elements.compare("UMI"))
    {
      g_debug("Initializing umi audio mixer object");
      umiaudiomixer::createUmiMixerInstance();
    }
    else
    {
      g_debug("default case, no mixer interface will be initialised");
    }
  }
}