/*

   nsjail - logging
   -----------------------------------------

   Copyright 2014 Google Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "log.h"
#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

#if defined(_HF_ARCH_LINUX)
#include <sys/syscall.h>
#define __hf_pid() (pid_t) syscall(__NR_gettid)
#else /* defined(_HF_ARCH_LINUX) */
#define __hf_pid() getpid()
#endif /* defined(_HF_ARCH_LINUX) */

static int log_fd = STDERR_FILENO;
static bool log_fd_isatty = false;
enum llevel_t log_level = INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

__attribute__((constructor)) static void log_init(void) {
    log_level = INFO;
    log_fd = fcntl(STDERR_FILENO, F_DUPFD_CLOEXEC, 0);
    if (log_fd == -1) {
        log_fd = STDERR_FILENO;
    }
    log_fd_isatty = isatty(log_fd);
}

/*
 * Log to stderr by default. Use a dup()d fd, because in the future we'll associate the
 * connection socket with fd (0, 1, 2).
 */
bool logInitLogFile(const char* logfile, enum llevel_t ll) {
    log_level = ll;

    if (logfile == NULL) {
        return true;
    }

    log_fd = open(logfile, O_CREAT | O_RDWR | O_APPEND, 0640);
    if (log_fd == -1) {
        log_fd = STDERR_FILENO;
        PLOG_E("Couldn't open logfile open('%s')", logfile);
        return false;
    }
    log_fd_isatty = (isatty(log_fd) == 1 ? true : false);
    return true;
}

void logLog(enum llevel_t ll, const char* fn, int ln, bool perr, const char* fmt, ...) {
    char strerr[512];
    if (perr == true) {
        snprintf(strerr, sizeof(strerr), "%s", strerror(errno));
    }
    struct ll_t {
        const char* descr;
        const char* prefix;
        const bool print_funcline;
        const bool print_time;
    };
    static const struct ll_t logLevels[] = {
        {"F", "\033[7;35m", true, true},
        {"E", "\033[1;31m", true, true},
        {"W", "\033[0;33m", true, true},
        {"I", "\033[1m", false, false},
        {"D", "\033[0;4m", true, true},
        {"HR", "\033[0m", false, false},
        {"HB", "\033[1m", false, false},
    };

    time_t ltstamp = time(NULL);
    struct tm utctime;
    localtime_r(&ltstamp, &utctime);
    char timestr[32];
    if (strftime(timestr, sizeof(timestr) - 1, "%FT%T%z", &utctime) == 0) {
        timestr[0] = '\0';
    }

    /* Start printing logs */
    {
        MX_LOCK(&log_mutex);

        if (log_fd_isatty) {
            dprintf(log_fd, "%s", logLevels[ll].prefix);
        }
        if (logLevels[ll].print_time) {
            dprintf(log_fd, "[%s][%s][%d] ", timestr, logLevels[ll].descr, __hf_pid());
        }
        if (logLevels[ll].print_funcline) {
            dprintf(log_fd, "%s():%d ", fn, ln);
        }

        va_list args;
        va_start(args, fmt);
        vdprintf(log_fd, fmt, args);
        va_end(args);

        if (perr == true) {
            dprintf(log_fd, ": %s", strerr);
        }
        if (log_fd_isatty) {
            dprintf(log_fd, "\033[0m");
        }
        dprintf(log_fd, "\n");

        MX_UNLOCK(&log_mutex);
    }
    /* End printing logs */

    if (ll == FATAL) {
        exit(1);
    }
}

void logStop(int sig) { LOG_I("Server stops due to fatal signal (%d) caught. Exiting", sig); }

void logRedirectLogFD(int fd) { log_fd = fd; }

void logDirectlyToFD(const char* msg) { dprintf(log_fd, "%s", msg); }

pthread_mutex_t* logMutexGet(void) { return &log_mutex; }

void logMutexReset(void) { pthread_mutex_init(&log_mutex, NULL); }

bool logIsTTY(void) { return log_fd_isatty; }

int logFd(void) { return log_fd; }
