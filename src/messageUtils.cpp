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


#include "messageUtils.h"
#include "ConstString.h"

void CLSError::Print(const char * where, int line, GLogLevelFlags logLevel)
{
    if (LSErrorIsSet(this))
    {
        PM_LOG_ERROR(MSGID_LUNA_SERVICE_ERROR, INIT_KVCOUNT,\
            "%s(%d): Luna Service Error #%d \"%s\",  \
            \nin %s line #%d.", where, line,
            this->error_code, this->message,
            this->file, this->line);
        LSErrorFree(this);
    }
}

JsonMessageParser::JsonMessageParser(const char * json, const char * schema) :
                             mJson(json), mSchema(schema)
{
}

bool JsonMessageParser::parse(const char * callerFunction)
{
    if (!mParser.parse(mJson, mSchema))
    {
        const char * errorText = "Could not validate json message against schema";
        pbnjson::JSchemaFragment    genericSchema(SCHEMA_ANY);
        if (!mParser.parse(mJson, genericSchema))
            errorText = "Invalid json message";
        PM_LOG_ERROR(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT,\
            "%s: %s '%s'", callerFunction, errorText, mJson);
        return false;
    }
    return true;
}


LSMessageJsonParser::LSMessageJsonParser(LSMessage * message,
                                         const char * schema) :
                                         mMessage(message),
                                         mSchemaText(schema),
                                         mSchema(schema)
{
}

bool LSMessageJsonParser::parse(const char * callerFunction,
                                LSHandle * lssender,
                                ELogOption logOption)
{
    const char * payload = getPayload();
    const char * context = 0;
    if (logOption == eLogOption_LogMessageWithCategory)
        context = LSMessageGetCategory(mMessage);
    else if (logOption == eLogOption_LogMessageWithMethod)
        context = LSMessageGetMethod(mMessage);
    if (context == 0)
        context = "";

    if (logOption != eLogOption_DontLogMessage)
        PM_LOG_INFO(MSGID_PARSE_JSON, INIT_KVCOUNT,\
            "%s%s: got '%s'", callerFunction, context, payload);
    if (!mParser.parse(payload, mSchema))
    {
        const char *    sender = LSMessageGetSenderServiceName(mMessage);
        if (sender == 0 || *sender == 0)
            sender = LSMessageGetSender(mMessage);
        if (sender == 0)
            sender = "";
        const char * errorText = "Could not validate json message against schema";
        bool notJson = true;
        if (strcmp(mSchemaText, SCHEMA_ANY) != 0)
        {
            pbnjson::JSchemaFragment    genericSchema(SCHEMA_ANY);
            notJson = !mParser.parse(payload, genericSchema);
        }
        if (notJson)
        {
            PM_LOG_ERROR(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT,\
                "%s%s: The message '%s' sent by '%s' is not a valid  \
                json message.", callerFunction, context,
                payload, sender);
            errorText = "Not a valid json message";
        }
        else
        {
            PM_LOG_ERROR(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT,\
                 "%s%s: Could not validate json message '%s' sent by   \
                 '%s' against schema '%s'.",
                 callerFunction, context,
                 payload, sender, mSchemaText);
        }
        if (sender)
        {
            std::string reply = createJsonReplyString(false, 1, errorText);
            CLSError lserror;
            if (!LSMessageReply(lssender, mMessage, reply.c_str(), &lserror))
                lserror.Print(callerFunction, 0);
        }
        return false;
    }
    return true;
}

pbnjson::JValue createJsonReply(bool returnValue,
                                int errorCode,
                                const char *errorText)
{
    pbnjson::JValue reply = pbnjson::Object();
    reply.put("returnValue", returnValue);
    if (errorCode)
        reply.put("errorCode", errorCode);
    if (errorText)
        reply.put("errorText", errorText);
    return reply;
}

std::string createJsonReplyString(bool returnValue,
                                  int errorCode,
                                  const char *errorText)
{
    std::string    reply;
    if (returnValue)
        reply = STANDARD_JSON_SUCCESS;
    else if (errorCode)
    {
        if (errorText)
            reply = string_printf("{\"returnValue\":false,\"errorCode\":%d,  \
                                 \"errorText\":\"%s\"}", errorCode, errorText);
        else
            reply = string_printf("{\"returnValue\":false,\"errorCode\":%d}",
                                                                    errorCode);
    }
    else if (errorText)
        reply = string_printf("{\"returnValue\":false,\"errorText\":\"%s\"}",
                                                                    errorText);
    else
        reply = string_printf("{\"returnValue\":false}");
    return reply;
}

std::string jsonToString(pbnjson::JValue & reply, const char * schema)
{
    pbnjson::JGenerator serializer(NULL);// our schema that we will be using
                                        // does not have any external references
    std::string serialized;
    pbnjson::JSchemaFragment responseSchema(schema);
    if (!serializer.toString(reply, responseSchema, serialized)) {
        PM_LOG_ERROR(MSGID_JSON_PARSE_ERROR, INIT_KVCOUNT,\
            "serializeJsonReply: failed to generate json reply");
        return "{\"returnValue\":false,\"errorText\":\"audiod error: Failed to generate a valid json reply...\"}";
    }
    return serialized;
}
