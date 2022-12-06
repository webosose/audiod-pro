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

#ifndef CONSTSTRING_H_
#define CONSTSTRING_H_

#include <glib.h>
#include <cstring>
#include <string>

/**
 * Simple class to handle constant string that are never modified (edited),
 * only assigned to a new brand new constant value.
 * This is meant to be used with literal string declarations, as in:
 * ConstString    value("tag");
 * This class expects the string pointed to to remain valid throughout the
 * life of the object. It will not copy it, modify or delete it.
 * This class will allow more readable & safer code for compares, as it
 * handles the  pointers safely, treating them as an empty string "".
 */

class ConstString
{
public:
    /// Constructor, which default to the null string. The string pointed to
    // must never be deleted/modified, hence the explicit constructor!
    explicit ConstString(const gchar * string = 0) : mString(string) {}

    /// Assignment operator
    ConstString &    operator=(const ConstString & rhs)
                                       { mString = rhs.mString; return *this; }

    /// Compare operator, case sensitive
    bool        operator==(const gchar * rhs) const
          { return mString == rhs || ::strcmp(c_str(), rhs ? rhs : "") == 0; }
    /// Compare operator, case sensitive
    bool        operator==(const ConstString & rhs) const
                                        { return mString == rhs.mString ||
                                         ::strcmp(c_str(), rhs.c_str()) == 0; }

    /// Diff operator, case sensitive
    bool        operator!=(const gchar * rhs) const{ return !operator==(rhs); }
    /// Diff operator, case sensitive
    bool        operator!=(const ConstString & rhs) const
                                                  { return !operator==(rhs); }

    /// Safe utility method checking if the string ends
    //with an other string, case sensitive
    bool        hasSuffix(const gchar * rhs) const;
    /// Safe utility method checking if the string starts
    // with an other string, case sensitive
    bool        hasPrefix(const gchar * rhs) const;

    /// Verify that the string has a prefix and returns the suffix.
    // Returned suffix is valid as long as the original ConstString.
    bool        hasPrefix(const gchar * prefix, ConstString & suffix) const;

    /// Find a string within the string. Returns what's past the found string.
    bool        find(const gchar * string, ConstString & rest) const;

    /// Find the last occurence of a string within the string.
    // Returns what's past the found string.
    bool        rfind(const gchar * string, ConstString & rest) const;

    /// Implicit cast to const gchar * type.
    operator const gchar *() const { return mString ? mString : ""; }
    /// Explicit cast to const gchar *, in particular for use in
    //printf and other functions using variable parameter arguments ("...")
    const gchar * c_str() const  { return mString ? mString : ""; }

    /// Tells if this is the empty string "" (default value)
    bool        isEmpty() const    { return mString == 0 || *mString == 0; }

    /// Build an std::string using a ConstString. Will copy the text
    // and the resulting std::string will survive the ConstString
    std::string    operator+(const gchar * inRhs) const
                                 { return std::string(c_str()) += (inRhs ? inRhs : ""); }

    /// Built-in unit test for the class: Returns true on success.
    static bool    UnitTest();

private:
    const gchar * mString;
};

/// Build an std::string using printf-style formatting
std::string string_printf(const char *format, ...) G_GNUC_PRINTF(1, 2);

/// Append a printf-style string to an existing std::string
std::string & append_format(std::string & str,
                            const char * format, ...) G_GNUC_PRINTF(2, 3);

#endif /* CONSTSTRING_H_ */
