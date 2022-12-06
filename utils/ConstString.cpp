// Copyright (c) 2002-2022 LG Electronics, Inc.
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

#include "ConstString.h"
#include "log.h"
#include <cstdio>

bool ConstString::hasSuffix(const gchar * rhs) const
{
    return g_str_has_suffix(c_str(), rhs);
}

bool ConstString::hasPrefix(const gchar * rhs) const
{
    return g_str_has_prefix(c_str(), rhs);
}

bool ConstString::hasPrefix(const gchar * prefix, ConstString & suffix) const
{
    if (!g_str_has_prefix(c_str(), prefix))
    {
        suffix.mString = 0;        // clear returned suffix as well
        return false;
    }
    suffix.mString = mString + strlen(prefix);
    return true;
}

bool ConstString::find(const gchar * string, ConstString & rest) const
{
    const gchar * s = strstr(c_str(), string);
    if (s)
    {
        rest.mString = s + strlen(string);
        return true;
    }
    rest.mString = 0;
    return false;
}

bool ConstString::rfind(const gchar * string, ConstString & rest) const
{
    const gchar * s = g_strrstr(c_str(), string);
    if (s)
    {
        rest.mString = s + strlen(string);
        return true;
    }
    rest.mString = 0;
    return false;
}

#define UT_CHECK(t) ((t) || (failedUnitTest(#t, __FILE__, __LINE__, __FUNCTION__), false))

static int sUnitTestFailureCount = 0;

static void failedUnitTest(const gchar * test, const gchar * file, int line, const gchar * function)
{
    g_critical("Failed unit test: \"%s\" is false in %s, %s:%d.", test, function, file, line);
    sUnitTestFailureCount++;
}

bool ConstString::UnitTest()
{
    ConstString        nullstring, nullstring2;
    ConstString        one("one");
    ConstString        two("two");

    UT_CHECK(nullstring.c_str() != 0 && *nullstring.c_str() == 0);
    UT_CHECK(nullstring == nullstring2);
    UT_CHECK(!(nullstring != nullstring2));

    UT_CHECK(strcmp(one, "one") == 0);
    UT_CHECK(strcmp(nullstring, "") == 0);

    UT_CHECK(nullstring == 0);
    UT_CHECK(!(nullstring != 0));

    UT_CHECK(one == one);
    UT_CHECK(one == "one");
    UT_CHECK(!(one == two));
    UT_CHECK(!(one == "two"));

    UT_CHECK(one != two);
    UT_CHECK(one != "two");
    UT_CHECK(!(one != one));
    UT_CHECK(!(one != "one"));

    UT_CHECK(one.hasPrefix(""));
    UT_CHECK(one.hasPrefix("o"));
    UT_CHECK(one.hasPrefix("on"));
    UT_CHECK(one.hasPrefix("one"));
    UT_CHECK(!one.hasPrefix("two"));
    UT_CHECK(!one.hasPrefix("ow"));
    UT_CHECK(!one.hasPrefix("tn"));

    UT_CHECK(one.hasSuffix(""));
    UT_CHECK(one.hasSuffix("e"));
    UT_CHECK(one.hasSuffix("ne"));
    UT_CHECK(one.hasSuffix("one"));
    UT_CHECK(!one.hasSuffix("we"));
    UT_CHECK(!one.hasSuffix("two"));

    ConstString    suffix;
    UT_CHECK(one.hasPrefix("on", suffix) && suffix == "e");
    UT_CHECK(!one.hasPrefix("oe", suffix));

    UT_CHECK(one + two == "onetwo");

    ConstString s("abcbcd");
    UT_CHECK(s.find("bc", suffix) && suffix == "bcd");
    UT_CHECK(s.rfind("bc", suffix) && suffix == "d");

    return sUnitTestFailureCount == 0;
}

std::string string_printf(const char *format, ...)
{
    if (format == 0)
        return "";
    va_list args;
    va_start(args, format);
    char stackBuffer[1024];
    int result = vsnprintf(stackBuffer, G_N_ELEMENTS(stackBuffer), format, args);
    if (result > -1 && result < (int) G_N_ELEMENTS(stackBuffer))
    {    // stack buffer was sufficiently large. Common case with no temporary dynamic buffer.
        va_end(args);
        return std::string(stackBuffer, result);
    }

    int length = result > -1 ? result + 1 : G_N_ELEMENTS(stackBuffer) * 3;
    char * buffer = 0;
    do
    {
        if (buffer)
        {
            delete[] buffer;
            length *= 3;
        }
        buffer = new char[length];
        result = vsnprintf(buffer, length, format, args);
    } while (result == -1 && result < length);
    va_end(args);
    std::string    str(buffer, result);
    delete[] buffer;

    return str;
}

std::string & append_format(std::string & str, const char * format, ...)
{
    if (format == 0)
        return str;
    va_list args;
    va_start(args, format);
    char stackBuffer[1024];
    int result = vsnprintf(stackBuffer, G_N_ELEMENTS(stackBuffer), format, args);
    if (result > -1 && result < (int) G_N_ELEMENTS(stackBuffer))
    {    // stack buffer was sufficiently large. Common case with no temporary dynamic buffer.
        va_end(args);
        str.append(stackBuffer, result);
        return str;
    }

    int length = result > -1 ? result + 1 : G_N_ELEMENTS(stackBuffer) * 3;
    char * buffer = 0;
    do
    {
        if (buffer)
        {
            delete[] buffer;
            length *= 3;
        }
        buffer = new char[length];
        result = vsnprintf(buffer, length, format, args);
    } while (result == -1 && result < length);
    va_end(args);
    if (buffer)
    {
        str.append(buffer, result);
        delete[] buffer;
    }
    return str;
}
