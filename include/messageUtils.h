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


#ifndef MESSAGEUTILS_H_
#define MESSAGEUTILS_H_

#include <lunaservice.h>
#include <pbnjson.h>
#include <pbnjson.hpp>
#include "log.h"

/*
 * Small wrapper around LSError. User is responsible for calling Print or Free after the error has been set.
 */
struct CLSError : public LSError
{
    CLSError()
    {
        LSErrorInit(this);
    }
    void Print(const char * where, int line, GLogLevelFlags logLevel = G_LOG_LEVEL_WARNING);
    void Free()
    {
        LSErrorFree(this);
    }
};

typedef struct envelope
{
  LSMessage *message;
  void *context;
}envelopeRef;

/*
 * Helper macros to build schemas in a more reliable, readable & editable way in C++
 */

#define STR(x) #x

//Strict schema utils changes

#define STRICT_SCHEMA(attributes)                     "{\"type\":\"object\"" attributes ",\"additionalProperties\":false}"

#define NORMAL_SCHEMA(attributes)                            "{\"type\":\"object\"" attributes ",\"additionalProperties\":true}"

#define PROPS_1(p1)                                   ",\"properties\":{" p1 "}"
#define PROPS_2(p1, p2)                               ",\"properties\":{" p1 "," p2 "}"
#define PROPS_3(p1, p2, p3)                           ",\"properties\":{" p1 "," p2 "," p3 "}"
#define PROPS_4(p1, p2, p3, p4)                       ",\"properties\":{" p1 "," p2 "," p3 "," p4 "}"
#define PROPS_5(p1, p2, p3, p4, p5)                   ",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "}"
#define PROPS_6(p1, p2, p3, p4, p5, p6)               ",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "}"
#define PROPS_9(p1, p2, p3, p4, p5, p6, p7, p8, p9)   ",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," p8 "," p9 "}"

#define PROP(name, type)                              "\"" #name "\":{\"type\":\"" #type "\"}"
#define PROP_WITH_VAL_1(name, type, v1)               "\"" #name "\":{\"type\":\"" #type "\", \"enum\": [" #v1 "]}"

#define REQUIRED_1(p1)                                ",\"required\":[\"" #p1 "\"]"
#define REQUIRED_2(p1, p2)                            ",\"required\":[\"" #p1 "\",\"" #p2 "\"]"
#define REQUIRED_3(p1, p2, p3)                        ",\"required\":[\"" #p1 "\",\"" #p2 "\",\"" #p3 "\"]"
#define REQUIRED_4(p1, p2, p3, p4)                    ",\"required\":[\"" #p1 "\",\"" #p2 "\",\"" #p3 "\",\"" #p4 "\"]"
#define REQUIRED_5(p1, p2, p3, p4, p5)                ",\"required\":[\"" #p1 "\",\"" #p2 "\",\"" #p3 "\",\"" #p4 "\",\"" #p5 "\"]"

extern const char * STANDARD_JSON_SUCCESS;

// Build a standard reply as a const char * string consistently
#define STANDARD_JSON_SUCCESS                         "{\"returnValue\":true}"
#define STANDARD_JSON_ERROR(errorCode, errorText)    "{\"returnValue\":false,\"errorCode\":"STR(errorCode)",\"errorText\":\"" errorText "\"}"
#define MISSING_PARAMETER_ERROR(name, type)            "{\"returnValue\":false,\"errorCode\":2,\"errorText\":\"Missing '" STR(name) "' " STR(type) " parameter.\"}"
#define INVALID_PARAMETER_ERROR(name, type)            "{\"returnValue\":false,\"errorCode\":3,\"errorText\":\"Invalid '" STR(name) "' " STR(type) " parameter value or file not found.\"}"

// Test the name of a json parameter to determine if it's a system parameter that we should ignore. Pass a (char *)
#define IS_SYSTEM_PARAMETER(x)    ((x) && *(x) == '$')
#define SYSTEM_PARAMETERS "\"$activity\":{\"type\":\"object\",\"optional\":true}"

// Build a schema as a const char * string without any execution overhead
#define SCHEMA_ANY                        "{}"
#define SCHEMA_0                        "{\"type\":\"object\",\"properties\":{" SYSTEM_PARAMETERS "},\"additionalProperties\":false}"                                // Rejects any parameter. Only valid message is "{}"
#define SCHEMA_1(param)                    "{\"type\":\"object\",\"properties\":{" param "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"                        // Ex: SCHEMA_1(REQUIRED(age,integer))
#define SCHEMA_2(p1, p2)                "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"                    // Ex: SCHEMA_2(REQUIRED(age,integer),OPTIONAL(nickname,string)
#define SCHEMA_3(p1, p2, p3)            "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_4(p1, p2, p3, p4)        "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_5(p1, p2, p3, p4, p5)    "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_6(p1, p2, p3, p4, p5, p6)    "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_9(p1, p2, p3, p4, p5, p6, p7, p8, p9)    "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," p8 "," p9 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"

// Macros to use in place of the parameters in the SCHEMA_xxx macros above
#define REQUIRED(name, type) "\"" #name "\":{\"type\":\"" #type "\"}"
#define OPTIONAL(name, type) "\"" #name "\":{\"type\":\"" #type "\",\"optional\":true}"

// Macros to use in place of the error codes
#define UNSUPPORTED_ERROR_CODE 3
#define REPEATED_REQUEST_ERROR_CODE 4

/*
 * Helper class to parse a json message using a schema (if specified)
 */
class JsonMessageParser
{
public:
    JsonMessageParser(const char * json, const char * schema);
    bool                    parse(const char * callerFunction);
    pbnjson::JValue            get()                                        { return mParser.getDom(); }

    // convenience functions to get a parameter directly.
    bool                    get(const char * name, std::string & str)    { return get()[name].asString(str) == CONV_OK; }
    bool                    get(const char * name, bool & boolean)        { return get()[name].asBool(boolean) == CONV_OK; }
    template <class T> bool    get(const char * name, T & number)            { return get()[name].asNumber<T>(number) == CONV_OK; }

private:
    const char *                mJson;
    pbnjson::JSchemaFragment    mSchema;
    pbnjson::JDomParser            mParser;
};

// LSMessageJson::parse can log the message received, or not, with more or less parameters...
enum ELogOption
{
    eLogOption_DontLogMessage = 0,
    eLogOption_LogMessage,
    eLogOption_LogMessageWithCategory,
    eLogOption_LogMessageWithMethod
};

/*
 * Helper class to parse json messages coming from an LS service using pbnjson
 */
class LSMessageJsonParser
{
public:
    // Default no using any specific schema. Will simply validate that the message is a valid json message.
    LSMessageJsonParser(LSMessage * message, const char * schema);

    // Parse the message using the schema passed in constructor.
    // If 'sender' is specified, automatically reply in case of bad syntax using standard format.
    // Option to log the text of the message by default.
    bool                    parse(const char * callerFunction, LSHandle * sender = 0, ELogOption logOption = eLogOption_LogMessage);
    pbnjson::JValue            get()                                        { return mParser.getDom(); }
    const char *            getPayload()                                { return LSMessageGetPayload(mMessage); }

    // convenience functions to get a parameter directly.
    bool                    get(const char * name, std::string & str)    { return get()[name].asString(str) == CONV_OK; }
    bool                    get(const char * name, bool & boolean)        { return get()[name].asBool(boolean) == CONV_OK; }
    template <class T> bool    get(const char * name, T & number)            { return get()[name].asNumber<T>(number) == CONV_OK; }

private:
    LSMessage *                    mMessage;
    const char *                mSchemaText;
    pbnjson::JSchemaFragment    mSchema;
    pbnjson::JDomParser            mParser;

};

// build a standard reply returnValue & errorCode/errorText if defined
pbnjson::JValue createJsonReply(bool returnValue = true, int errorCode = 0, const char * errorText = 0);

// build a standard json reply string without the overhead of using json schema
std::string createJsonReplyString(bool returnValue = true, int errorCode = 0, const char * errorText = 0);

// serialize a reply
std::string jsonToString(pbnjson::JValue & reply, const char * schema = SCHEMA_ANY);

#endif /* MESSAGEUTILS_H_ */
