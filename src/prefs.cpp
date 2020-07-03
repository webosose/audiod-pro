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

#include <lunaprefs.h>
#include "messageUtils.h"
#include "state.h"
#include "log.h"
#include "IPC_SharedAudiodProperties.h"
#include "main.h"

bool State::storePreferences()
{
    LPAppHandle prefHandle = NULL;

    if (!VERIFY(LPAppGetHandle(AUDIOD_SERVICE_PATH, &prefHandle) == LP_ERR_NONE) ||
                                                          !VERIFY(prefHandle))
    {
        LPAppFreeHandle(prefHandle, false);
        return false;
    }

    pbnjson::JValue pref = pbnjson::Object();

    // don't store preferences set to default value,
    // that's a waste of time to store & restore
    for (TBooleanPreferences::iterator iter = gState.mBooleanPreferences.begin();
                            iter != gState.mBooleanPreferences.end(); ++iter)
        if (iter->second.mValue != iter->second.mDefaultValue)
            pref.put(iter->first, iter->second.mValue);
    for (TStringPreferences::iterator iter = gState.mStringPreferences.begin();
                               iter != gState.mStringPreferences.end(); ++iter)
        if (iter->second.mValue != iter->second.mDefaultValue)
            pref.put(iter->first, iter->second.mValue);

    for (TintPreferences::iterator iter = gState.mIntegerPreferences.begin();
                               iter != gState.mIntegerPreferences.end(); ++iter)
        if (iter->second.mValue != iter->second.mDefaultValue)
            pref.put(iter->first, iter->second.mValue);

    std::string    prefString = jsonToString(pref);
    g_debug("Storing 'state_preferences': %s", prefString.c_str());
    CHECK(LPAppSetValue(prefHandle, "state_preferences",
                                           prefString.c_str()) == LP_ERR_NONE);

    return VERIFY(LPAppFreeHandle(prefHandle, true) == LP_ERR_NONE);
}

template <class T> bool _restorePreference(LPAppHandle prefHandle,
                                           const char * keyName,
                                           const char * valueName, T & value)
{
    bool result = false;
    char * json = 0;
    if (LPAppCopyValue(prefHandle, keyName, &json) == LP_ERR_NONE && json)
    {
        JsonMessageParser    msg(json, SCHEMA_ANY);
        result = msg.parse(__FUNCTION__) && msg.get(valueName, value);
        g_free(json);
    }
    return result;
}

bool State::restorePreferences()
{
    LPAppHandle prefHandle = NULL;

    if (!VERIFY(LPAppGetHandle(AUDIOD_SERVICE_PATH, &prefHandle) == LP_ERR_NONE) ||
                                                          !VERIFY(prefHandle))
    {
        LPAppFreeHandle(prefHandle, false);
        return false;
    }

    char * json = 0;
    if (LPAppCopyValue(prefHandle, "state_preferences", &json) == LP_ERR_NONE && json)
    {
        g_debug("Restoring 'state_preferences': %s", json);
        JsonMessageParser    msg(json, SCHEMA_ANY);
        if (CHECK(msg.parse(__FUNCTION__)))
        {
            pbnjson::JValue request = msg.get();
            for (pbnjson::JValue::ObjectIterator pair = request.begin();
                                                pair != request.end(); pair++)
            {
                std::string    name;
                if (VERIFY((*pair).first.asString(name) == CONV_OK))
                {
                    bool found = false;
                    bool boolValue;
                    std::string    stringValue;
                    int integerValue;
                    if ((*pair).second.asBool(boolValue) == CONV_OK)
                    {
                        TBooleanPreferences::iterator iter =
                                         gState.mBooleanPreferences.find(name);
                        if (iter != gState.mBooleanPreferences.end())
                        {
                            iter->second.mValue = boolValue;
                            found = true;
                            if (name == "RingerOn") {
                                gState.setPreference(cPref_RingerOn, boolValue);
                                gAudiodProperties->mRingerOn.set(boolValue);
                            }

                            if (name == "PrevRingerOn")
                                gState.setPreference(cPref_PrevRingerOn, boolValue);

                            if (name == "DndOn")
                                gState.setPreference(cPref_DndOn, boolValue);

                            if (name == "TouchOn") {
                                gState.setPreference(cPref_TouchOn, boolValue);
                                gAudiodProperties->mTouchOn.set(boolValue);
                            }

                            if (name == "AlarmOn") {
                                gState.setPreference(cPref_OverrideRingerForAlaram, boolValue);
                                gAudiodProperties->mAlarmOn.set(boolValue);
                            }

                            if (name == "TimerOn") {
                                gState.setPreference(cPref_OverrideRingerForTimer, boolValue);
                                gAudiodProperties->mTimerOn.set(boolValue);
                            }

                            if (name == "RingtonewithVibrationWhenEnabled") {
                                gState.setPreference(cPref_RingtoneWithVibration, boolValue);
                                gAudiodProperties->mRingtoneWithVibration.set(boolValue);
                            }
                        }
                    }
                    else if ((*pair).second.asString(stringValue) == CONV_OK)
                    {
                        TStringPreferences::iterator iter =
                                          gState.mStringPreferences.find(name);
                        if (iter != gState.mStringPreferences.end())
                        {
                            iter->second.mValue = stringValue;
                            found = true;
                        }
                    }

                   else if ((*pair).second.asNumber(integerValue) == CONV_OK) //asNUmber for integers
                    {
                        TintPreferences::iterator iter =
                                         gState.mIntegerPreferences.find(name);
                        if (iter != gState.mIntegerPreferences.end())
                        {
                            iter->second.mValue = integerValue;
                            found = true;
                            if (name == "VolumeBalance") {
                                gState.setPreference(cPref_VolumeBalanceOnHeadphones,integerValue);
                                 gAudiodProperties->mBalanceVolume.set(boolValue);
                            }
                        }
                  }

                 if (!found)
                        g_warning("State::restorePreferences: invalid   \
                              preference '%s'", name.c_str());    // name or type incorrect
                }
            }
        }
        g_free(json);
    }
    else    // fallback on pre-Blowfish persistence code, in case we need to pick-up
    {
        bool settingOn;
        if (_restorePreference(prefHandle, cPref_VibrateWhenRingerOn,
                                                           "value", settingOn))
            mBooleanPreferences[cPref_VibrateWhenRingerOn].mValue = settingOn;

        if (_restorePreference(prefHandle, cPref_PrevVibrateWhenRingerOn,
                                                           "value", settingOn))
            mBooleanPreferences[cPref_PrevVibrateWhenRingerOn].mValue = settingOn;

        bool settingOff;
        if (_restorePreference(prefHandle, cPref_VibrateWhenRingerOff,
                                                          "value", settingOff))
            mBooleanPreferences[cPref_VibrateWhenRingerOff].mValue = settingOff;

        if (_restorePreference(prefHandle, cPref_PrevVibrateWhenRingerOff,
                                                          "value", settingOff))
            mBooleanPreferences[cPref_PrevVibrateWhenRingerOff].mValue = settingOff;
    }

    // close handle and discard stuff
    LPAppFreeHandle(prefHandle, false);

    return true;
}

static
gboolean _storePreferencesCallback(gpointer data)
{
    //Will be removed or updated once DAP design is updated
    /*ScenarioModule * module = (ScenarioModule *) data;

    module->storePreferences();*/

    return FALSE;
}
