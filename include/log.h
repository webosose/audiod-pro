// Copyright (c) 2012-2022 LG Electronics, Inc.
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

#ifndef LOG_H_
#define LOG_H_

#include <PmLogLib.h>
#include <glib.h>

//get audiod pm log context
PmLogContext getPmLogContext();

//set audiod pm log context
PmLogErr setPmLogContext(const char* logContextName);

enum ELogDestination
{
    eLogDestination_None = 0,

    eLogDestination_SystemLog       = 1 << 0, // System log
    eLogDestination_Terminal        = 1 << 1, // Default handler (terminal)
    eLogDestination_PrivateLogFiles = 1 << 2, // Private log files in
                                              // /var/log/<processname>.log
    eLogDestination_All = eLogDestination_SystemLog |
                          eLogDestination_Terminal |
                          eLogDestination_PrivateLogFiles
};

/// Where should log traces be sent?
// You can have them sent to more than one place at a time
void setLogDestination(ELogDestination logDestination);

/// Add log destination to the current ones
void addLogDestination(ELogDestination logDestination);

/// Set the log level. Everything higher than that & that level will be logged.
void setLogLevel(GLogLevelFlags level);
GLogLevelFlags getLogLevel(void);

void logFilter(const gchar *log_domain,
               GLogLevelFlags log_level,
               const gchar *message,
               gpointer unused_data);

/// Set the process's name, used for settings & private log file names
/// set the debug log level using the file /var/home/root/enable-<processname>-logging.
// If the file is missing, use default non-debug log level
/// if the file is empty, but present, use max log level,
//otherwise read the log level from the file's content
/// Level by numbers: 0 = error, 1 = critical, 2 = warning, 3 = message,
// 4 = info, 5 = debug, others: debug
/// Level by text: e = error, c = critical, w = warning, m = message,
// i = info, d = debug, others: debug
/// Destination: f = private log files, s = system log files,
// t = terminal. You're adding to the current destinations.
/// Note that system is enabled by default.
/// If you use private log files, only messages at message level and higher
// priority are logged in the system log (to avoid duplication).
void setProcessName(const char * name);

#define SHOULD_NOT_REACH_HERE FAILURE("This line should never be reached")

/// Functions that implement the macros above.
// You probably don't want to call them directly...
void logFailedVerify(const gchar * test,
                     const gchar * file,
                     int line, const gchar * function);
void logFailure(const gchar * message,
                const gchar * file,
                int line, const gchar * function);
void logCheck(const gchar * message,
              const gchar * file,
              int line, const gchar * function);

/// Helper class for log presentation purposes.
//Indents the logs with the text passed as long as the object exists.
class LogIndent {
public:
    LogIndent(const char * indent);
    ~LogIndent();

    static const char *    getTotalIndent();

private:
    const char *    mIndent;
};

//This is for PmLog implementation
extern PmLogContext audiodLogContext;
#define INIT_KVCOUNT                0

#define PM_LOG_CRITICAL(msgid, kvcount, ...)  PmLogCritical(getPmLogContext(), msgid, kvcount, ##__VA_ARGS__)
#define PM_LOG_ERROR(msgid, kvcount, ...)     PmLogError(getPmLogContext(), msgid, kvcount, ##__VA_ARGS__)
#define PM_LOG_WARNING(msgid, kvcount, ...)   PmLogWarning(getPmLogContext(), msgid, kvcount, ##__VA_ARGS__)
#define PM_LOG_INFO(msgid, kvcount, ...)      PmLogInfo(getPmLogContext(), msgid, kvcount, ##__VA_ARGS__)
#define PM_LOG_DEBUG(...)                     PmLogDebug(getPmLogContext(), ##__VA_ARGS__)

//If log level is higher than DEBUG(lowest), you need to use Message ID.
//Start up and shutdown message ID's
#define MSGID_STARTUP                                  "AUDIOD_STARTUP"                    //AudioD start up logs
#define MSGID_SHUTDOWN                                 "AUDIOD_SHUTDOWN"                   //AudioD shutdown logs
#define MSGID_INIT                                     "AUDIOD_INIT"                       //AudioD inialization logs like control init, module init etc

//Luna related message ID's
#define MSGID_LUNA_SEND_FAILED                         "LUNA_SEND_FAILED"                  //Failed to send luna command
#define MSGID_LUNA_CREATE_JSON_FAILED                  "LUNA_CREATE_JSON_FAILED"           //Failed to create json payload
#define MSGID_LUNA_FAILED_TO_PARSE_PARAMETERS          "LUNA_FAILED_TO_PARSE_PARAMETERS"   //Failed to parse luna parameters
#define MSGID_LUNA_SERVICE_ERROR                       "LUNA_SERVICE_ERROR"                //For logging LSError

//Json related message ID's
#define MSGID_MALFORMED_JSON                           "MALFORMED_JSON"                    //Malformed json data
#define MSGID_JSON_PARSE_ERROR                         "JSON_PARSE_ERROR"                  //Error while parsing json data
#define MSGID_PARSE_JSON                               "PARSE_JSON"                        //Parse json message

//Other message ID's
#define MSGID_DATA_NULL                                "NULL_MESSAGE"                      //luna message is null
#define MSGID_DEBUG_SOCKETS                            "SOCKET_COMMUNICATION"              //For Socket communication
#define MSGID_VERIFY_FAILED                            "VERIFY_FAILED"                     //Test failed
#define MSGID_DEBUG_IPC                                "IPC_DEBUG"                         //For IPC communication

#define MSGID_MODULE_INITIALIZER                       "INITIALIZE_MODULE"                 //module initialiser
#define MSGID_MODULE_MANAGER                           "AUDIO_MODULE_MANAGER"              //module manager
#define MSGID_MODULE_FACTORY                           "AUDIO_MODULE_FACTORY"              //module factory
#define MSGID_MODULE_CONFIG                            "AUDIO_MODULE_CONFIG"               //module config
#define MSGID_MODULE_INTERFACE                         "MODULE_INTERFACE"                  //module interface
#define MSGID_POLICY_CONFIGURATOR                      "POLICY_CONFIGURATOR"               //Policy Configuration
#define MSGID_INVALID_INPUT                            "INVALID_INPUT"                     //For invalid input values
#define MSGID_AUDIO_MIXER                              "AUDIO_MIXER"                       //audio, umi and pulse mixer functions
#define MSGID_PULSEAUDIO_MIXER                         "PULSEAUDIO_MIXER"                  //pulse mixer functions
#define MSGID_UMIAUDIO_MIXER                           "UMIAUDIO_MIXER"                    //UMI mixer functions
#define MSGID_POLICY_MANAGER                           "AUDIO_POLICY_MANAGER"              //Audio policy manager
#define MSGID_SETTING_SERVICE_MANAGER                  "SETTINGS_SERVICE_MANAGER"          //Settings service manager
#define MSGID_DEVICE_MANAGER                           "DEVICE_EVENT_MANAGER"              //For Device event manager
#define MSGID_MASTER_VOLUME_MANAGER                    "MASTER_VOLUME_MANAGER"             //Master Volume manager
#define MSGID_BLUETOOTH_MANAGER                        "BLUETOOTH_MANAGER"                 //For Bluetooth manager
#define MSGID_LUNA_EVENT_SUBSCRIBER                    "LUNA_EVENT_SUBSCRIBER"             //For Luna event subscriber
#define MSGID_PULSE_LINK                               "PULSE_LINK"                        //For PulseAudio link
#define MSGID_PLAYBACK_MANAGER                         "PLAYBACK_MANAGER"                  //For Playback manager
#define MSGID_SOUND_SETTINGS                           "SOUND_SETTINGS"                    //For sound Settings
#define MSGID_DEVICE_ROUTING_CONFIG_PARSER             "DEVICE_ROUTING_CONFIG_PARSER"      //Device routing config parser
#define MSGID_SOUNDOUTPUT_LIST_PARSER                   "DEVICE_ROUTING_CONFIG_PARSER"      //Device routing config parser //TODO
#define MSGID_AUDIOROUTER                              "AUDIO_ROUTER"                      //For Auto class
#define MSGID_TRACKMANAGER                              "TRACK_MANAEGR"                     //For audio track management
#define MSGID_SYSTEMSOUND_MANAGER                      "SYSTEM_SOUND"                      //For System sound plays
#define MSGID_CONNECTION_MANAGER                       "CONNECTION_MANAGER"                //Connection manager
#define MSGID_GINIT_FUNTION                            "INIT_FUNCTIONS"                    //For utils, init and hook functions
#define MSGID_AUDIO_EFFECT_MANAGER                    "AUDIO_EFFECT_MANAGER"             //For audio effect manager

/// Test macro that will make a critical log entry if the test fails
#define VERIFY(t) (G_LIKELY(t) || (PM_LOG_ERROR(MSGID_VERIFY_FAILED, INIT_KVCOUNT,\
                                                        "%s %s %d %s",   \
                                                        #t, __FILE__, __LINE__,  \
                                                        __FUNCTION__), false))

/// Test macro that will make a warning log entry if the test fails
#define CHECK(t) (G_LIKELY(t) || ((PM_LOG_ERROR(MSGID_VERIFY_FAILED, INIT_KVCOUNT,\
                                                        "%s %s %d %s",   \
                                                        #t, __FILE__, __LINE__,  \
                                                        __FUNCTION__), false))

/// Direct critical message to put in the logs with
// file & line number, with filtering of repeats
#define FAILURE(m) PM_LOG_ERROR(MSGID_VERIFY_FAILED, INIT_KVCOUNT,\
                                                        "%s %s %d %s", \
                                                        m, __FILE__, __LINE__, __FUNCTION__)
#endif // LOG_H_
