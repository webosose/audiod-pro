// Copyright (c) 2012-2018 LG Electronics, Inc.
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

#include <unistd.h>
#include "log.h"
#include "ConstString.h"
#include "PmLogLib.h"
#include <cstdio>
#include <cerrno>
#include <time.h>
#include <fcntl.h>
#include <map>


class GStaticMutexLocker {
public:
    GStaticMutexLocker(GStaticMutex & mutex) : mMutex(mutex)
    {
        g_static_mutex_lock(&mMutex);
    }

    ~GStaticMutexLocker()
    {
        g_static_mutex_unlock(&mMutex);
    }

private:
    GStaticMutex &    mMutex;
};

static GLogLevelFlags    sLogLevel = G_LOG_LEVEL_MESSAGE;
static ELogDestination    sLogDestination = eLogDestination_SystemLog;
static const char *        sProcessName = "unnamed";

static PmLogContext gLogContext = 0;

void setLogDestination(ELogDestination logDestination)
{
    sLogDestination = logDestination;
}

void addLogDestination(ELogDestination logDestination)
{
    sLogDestination = (ELogDestination) (sLogDestination | logDestination);
}

void setLogLevel(GLogLevelFlags level)
{
    sLogLevel = level;
}

GLogLevelFlags getLogLevel (void)
{
    return sLogLevel;
}

// local utility to shift file by renaming them in sequence  "basename", "basename.1", "basename.2", etc
// optional arguments should not be used and left for the recursive implementation.
void static sMoveLogFile(const char * baseName, int maxIndex, int index = 0, std::string * nextFileNamePtr = 0)
{
    const char * name = baseName;
    if (nextFileNamePtr)
    {
        *nextFileNamePtr = string_printf("%s.%d", baseName, index);
        name = nextFileNamePtr->c_str();
    }
    if (index < maxIndex && g_file_test(name, (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)))
    {
        std::string nextFileName;
        sMoveLogFile(baseName, maxIndex, index + 1, &nextFileName);
        ::rename(name, nextFileName.c_str());
    }
}

static const char * logLevelName(GLogLevelFlags logLevel)
{
    const char * name = "unknown";
    switch (logLevel & G_LOG_LEVEL_MASK) {
         case G_LOG_LEVEL_ERROR:
             name = "ERROR";
             break;
         case G_LOG_LEVEL_CRITICAL:
             name = "CRITICAL";
             break;
         case G_LOG_LEVEL_WARNING:
             name = "warning";
             break;
         case G_LOG_LEVEL_MESSAGE:
             name = "message";
             break;
         case G_LOG_LEVEL_DEBUG:
             name = "debug";
             break;
         case G_LOG_LEVEL_INFO:
         default:
             name = "info";
             break;
     }
    return name;
}

static GStaticMutex slogFilter_mutex = G_STATIC_MUTEX_INIT;

void logFilter(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer unused_data)
{
    if (log_level > sLogLevel || sLogDestination == eLogDestination_None || message == 0 || *message == 0)
        return;

    // don't log everything in system log if we are also logging in our private log file...
    if (sLogDestination & eLogDestination_SystemLog && ((sLogDestination & eLogDestination_PrivateLogFiles) == 0 || log_level <= G_LOG_LEVEL_MESSAGE))
    {
         int priority = kPmLogLevel_Info;
       switch (log_level & G_LOG_LEVEL_MASK) {
            case G_LOG_LEVEL_ERROR:
                priority = kPmLogLevel_Critical;
                break;
            case G_LOG_LEVEL_CRITICAL:
                priority = kPmLogLevel_Error;
                break;
            case G_LOG_LEVEL_WARNING:
                priority = kPmLogLevel_Warning;
                break;
            case G_LOG_LEVEL_MESSAGE:
                priority = kPmLogLevel_Notice;
                break;
            case G_LOG_LEVEL_DEBUG:
                priority = kPmLogLevel_Debug;
                break;
            case G_LOG_LEVEL_INFO:
            default:
                priority = kPmLogLevel_Info;
                break;
        }
    }

    if (sLogDestination & (eLogDestination_PrivateLogFiles | eLogDestination_Terminal))
    {
        GStaticMutexLocker            lock(slogFilter_mutex);    // race protection only needed for terminal and file logging

        static int                     sLogFile = 0;
        static struct timespec        sLogStartSeconds = { 0 };
        static struct tm            sLogStartTime = { 0 };
        const char *                indent = LogIndent::getTotalIndent();
        if (sLogFile == 0)
        {
            std::string    baseName = string_printf("/var/log/%s.log", sProcessName);
            sMoveLogFile(baseName.c_str(), 5);
            if (sLogDestination & eLogDestination_PrivateLogFiles)
                sLogFile = ::open(baseName.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_NONBLOCK, 0644);
            else
                sLogFile = -1;    // don't log to private log file
            time_t now = ::time(0);
            ::clock_gettime(CLOCK_MONOTONIC, &sLogStartSeconds);
            ::localtime_r(&now, &sLogStartTime);
            char startTime[64];
            ::asctime_r(&sLogStartTime, startTime);
            if (sLogDestination & eLogDestination_PrivateLogFiles)
            {
                if (sLogFile > 0)
                {
                    ::write(sLogFile, startTime, ::strlen(startTime));
                    std::string    msg;
                    msg = string_printf("Logging to private log file '%s' at %s", baseName.c_str(), startTime);
                }
                else
                {
                    std::string    msg;
                    msg = string_printf("Unable to log to private log file '%s' at %s", baseName.c_str(), startTime);
                }
            }
             if (sLogDestination & eLogDestination_Terminal)
                ::fprintf(stdout, "%s", startTime);
        }
        struct timespec now;
        ::clock_gettime(CLOCK_MONOTONIC, &now);
        int ms = (now.tv_nsec - sLogStartSeconds.tv_nsec) / 1000000;
        int sec = sLogStartTime.tm_sec + int (now.tv_sec - sLogStartSeconds.tv_sec);
        if (ms < 0)
        {
            ms += 1000;
            --sec;
        }
        int min = sLogStartTime.tm_min + sec / 60;
        int hr = sLogStartTime.tm_hour + min / 60;
        min = min % 60;
        sec = sec % 60;
        char timeStamp[128];
        char levelName = *logLevelName(log_level);    // just use one letter
        const char * format = g_ascii_isupper(levelName) ? "%02d:%02d:%02d.%03d*%c*" : "%02d:%02d:%02d.%03d %c ";
        size_t len = ::sprintf(timeStamp, format, hr, min, sec, ms, levelName);
        if (len >= G_N_ELEMENTS(timeStamp))
        {
            len = G_N_ELEMENTS(timeStamp) - 1;
            timeStamp[len] = 0;
        }
        if (sLogFile > 0)
            ::write(sLogFile, timeStamp, len);
        len = ::strlen(indent);
        if (len > 0 && sLogFile > 0)
            ::write(sLogFile, indent, len);
        len = ::strlen(message);
        if (sLogFile > 0)
            ::write(sLogFile, message, len);
        bool    needLF = false;
        if (len < 1 || message[len - 1] != '\n')
        {
            needLF = true;
            if (sLogFile > 0)
                ::write(sLogFile, "\n", 1);
        }
        if (sLogDestination & eLogDestination_Terminal)
        {

#define COLORESCAPE        "\033["

#define RESETCOLOR        COLORESCAPE "0m"

#define BOLDCOLOR        COLORESCAPE "1m"
#define REDOVERBLACK    COLORESCAPE "1;31m"
#define BLUEOVERBLACK    COLORESCAPE "1;34m"
#define YELLOWOVERBLACK    COLORESCAPE "1;33m"

            if (levelName ==  'd')
            {
                if (needLF)
                    ::fprintf(stdout, "%s%s(%d) %s%s\n", timeStamp, indent, getpid(), indent, message);
                else
                    ::fprintf(stdout, "%s%s(%d) %s%s", timeStamp, indent, getpid(), indent, message);
            }
            else if (levelName ==  'w')
            {
                if (needLF)
                    ::fprintf(stdout, YELLOWOVERBLACK "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
                else
                    ::fprintf(stdout, YELLOWOVERBLACK "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
            }
            else if (levelName ==  'm')
            {
                if (needLF)
                    ::fprintf(stdout, BLUEOVERBLACK "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
                else
                    ::fprintf(stdout, BLUEOVERBLACK "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
            }
            else if (g_ascii_isupper(levelName))
            {
                if (needLF)
                    ::fprintf(stdout, REDOVERBLACK "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
                else
                    ::fprintf(stdout, REDOVERBLACK "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
            }
            else
            {
                if (needLF)
                    ::fprintf(stdout, BOLDCOLOR "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
                else
                    ::fprintf(stdout, BOLDCOLOR "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
            }
            ::fflush(stdout);
        }
    }
}

void setProcessName(const gchar * name)
{
    PmLogGetContext(NULL, &gLogContext);
    ConstString    str(name);
    ConstString    fileName;
    if (str.rfind("/", fileName))
        sProcessName = fileName;
    else
        sProcessName = name;
    std::string    prefFileName = string_printf("/var/home/root/enable-%s-logging", sProcessName);
    if (g_file_test(prefFileName.c_str(), (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)))
    {
        GLogLevelFlags    logLevel = G_LOG_LEVEL_DEBUG;    // default value if a file exists there
        FILE * file = fopen(prefFileName.c_str(), "r");
        if (file)
        {
            int count = 0;
            int c;
            while ((c = fgetc(file)) != EOF)
            {
                ++count;
                if (g_ascii_isdigit(c))
                {    // 0 = error, 1 = critical, 2 = warning, 3 = message, 4 = info, 5 = debug, others: debug
                    logLevel = (GLogLevelFlags) (1 << (c - '0' + 2));
                    if (logLevel < G_LOG_LEVEL_ERROR)
                        logLevel = G_LOG_LEVEL_ERROR;
                    else if (logLevel > G_LOG_LEVEL_DEBUG)
                        logLevel = G_LOG_LEVEL_DEBUG;
                }
                else
                {    // only use first letter. That's enough!
                    // e = error, c = critical, w = warning, m = message, i = info, d = debug, others: debug
                    switch (c)
                    {
                    case 'e':    logLevel = G_LOG_LEVEL_ERROR;        break;
                    case 'c':    logLevel = G_LOG_LEVEL_CRITICAL;    break;
                    case 'w':    logLevel = G_LOG_LEVEL_WARNING;        break;
                    case 'm':    logLevel = G_LOG_LEVEL_MESSAGE;        break;
                    case 'i':    logLevel = G_LOG_LEVEL_INFO;        break;
                    case 'd':    logLevel = G_LOG_LEVEL_DEBUG;        break;

                    case 'f':    addLogDestination(eLogDestination_PrivateLogFiles);    break;
                    case 's':    addLogDestination(eLogDestination_SystemLog);        break;
                    case 't':    addLogDestination(eLogDestination_Terminal);        break;

                    default:    /* ignore */ break;
                    }
                }
            }
            fclose(file);
            // default for an empty preference file: debug level to private files, no terminal output.
            if (count == 0)
            {
                logLevel = G_LOG_LEVEL_DEBUG;
                addLogDestination(eLogDestination_PrivateLogFiles);
            }
        }
        g_message("Log level set to \"%s\" by %s", logLevelName(logLevel), prefFileName.c_str());
        setLogLevel(logLevel);
    }
}

// In case a VERIFY fails repeatedly, we do not want to fill-up the log files...
// The strategy is that after a number of systematic reports, we require a minimum delay between reports.
// This delay grows as the same error keep occurring, up to a max delay.

class Location
{
public:
    Location(const gchar * file, int line) : mFile(file), mLine(line) {}

    bool operator<(const Location & rhs) const    { return mFile < rhs.mFile || (mFile == rhs.mFile && mLine < rhs.mLine); }

private:
    const gchar *    mFile;
    int             mLine;
};

struct Details
{
    Details() : mCount(0), mLastReportedOccurenceTime(0), mTimeGap(2) {}

    int            mCount;
    __time_t    mLastReportedOccurenceTime;
    __time_t    mTimeGap;
};

static void traceContext(GLogLevelFlags logLevel)
{
    char    localBuffer[256];
    int        err = errno;
    const char *    errorName = strerror_r(err, localBuffer, G_N_ELEMENTS(localBuffer));
    g_log(G_LOG_DOMAIN, logLevel, "FYI, errno is: %d '%s'", err, errorName);
}

static GStaticMutex sLog_and_filter_mutex = G_STATIC_MUTEX_INIT;

/// log a failure, but filter out repeated occurrences to avoid filling-in the logs
static void log_and_filter(GLogLevelFlags logLevel, const char * name, const char * error, const char * description, const char * function, const char * file, int line)
{
    GStaticMutexLocker    lock(sLog_and_filter_mutex);

    static std::map<Location, Details>    sFailureLog;
    Details    & details = sFailureLog[Location(file, line)];
    struct timespec    now;
    ::clock_gettime(CLOCK_MONOTONIC, &now);
    const int cMaxLogAll = 20;
    const __time_t cMaxGap = 60 * 5; // 5 minutes
    if (++details.mCount < cMaxLogAll)
    {
        g_log(G_LOG_DOMAIN, logLevel, "%s: \"%s\" %sin %s, %s:%d.", name, error, description, function, file, line);
        traceContext(logLevel);
        details.mLastReportedOccurenceTime = now.tv_sec;
    }
    else if (details.mCount == cMaxLogAll)
    {
        g_log(G_LOG_DOMAIN, logLevel, "%s: \"%s\" %sin %s, %s:%d for the %dth time. *** We might NOT report every occurrence anymore! ***", name, error, description, function, file, line, cMaxLogAll);
        traceContext(logLevel);
        details.mLastReportedOccurenceTime = now.tv_sec;
    }
    else
    {
        if (details.mLastReportedOccurenceTime + details.mTimeGap <= now.tv_sec)
        {
            g_log(G_LOG_DOMAIN, logLevel, "%s: \"%s\" %sin %s, %s:%d for the %dth time.", name, error, description, function, file, line, details.mCount);
            traceContext(logLevel);
            details.mLastReportedOccurenceTime = now.tv_sec;
            // increase reporting gap between two failures by 50%, up to a max, that sets the minimum reporting frequency
            details.mTimeGap += details.mTimeGap / 2;
            if (details.mTimeGap > cMaxGap)
                details.mTimeGap = cMaxGap;
        }
    }
}

void logFailure(const gchar * message, const gchar * file, int line, const gchar * function)
{
    log_and_filter(G_LOG_LEVEL_CRITICAL, "Failure", message, "", function, file, line);
}

void logFailedVerify(const gchar * test, const gchar * file, int line, const gchar * function)
{
    log_and_filter(G_LOG_LEVEL_CRITICAL, "Failed verify", test, "is false ", function, file, line);
}

void logCheck(const gchar * test, const gchar * file, int line, const gchar * function)
{
    log_and_filter(G_LOG_LEVEL_WARNING, "Failed check", test, "is false ", function, file, line);
}

static std::string sLogIndent;

LogIndent::LogIndent(const char * indent) : mIndent(indent)
{
    sLogIndent += indent;
}

LogIndent::~LogIndent()
{
    sLogIndent.resize(sLogIndent.size() - ::strlen(mIndent));
}

const char * LogIndent::getTotalIndent()
{
    return sLogIndent.c_str();
}
