/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2014/11/04
 *  @par function description:
 *  - 1 parse file use tokenizer
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "SysTokenizer.h"

// Enables debug output for the tokenizer.
#define DEBUG_TOKENIZER 0

static inline bool isDelimiter(char ch, const char* delimiters) {
    return strchr(delimiters, ch) != NULL;
}

SysTokenizer::SysTokenizer(const char*filename, char* buffer,
        bool ownBuffer, size_t length) :
        mFilename(filename),
        mBuffer(buffer), mOwnBuffer(ownBuffer), mLength(length),
        mCurrent(buffer), mLineNumber(1) {
}

SysTokenizer::~SysTokenizer() {
    if (mOwnBuffer) {
        delete[] mBuffer;
    }
}

int SysTokenizer::open(const char*filename, SysTokenizer** outTokenizer) {
    *outTokenizer = NULL;

    int result = 0;
    int fd = ::open(filename, O_RDONLY);
    if (fd < 0) {
        result = -errno;
        SYS_LOGE("Error opening file '%s', %s.", filename, strerror(errno));
    } else {
        struct stat stat;
        if (fstat(fd, &stat)) {
            result = -errno;
            SYS_LOGE("Error getting size of file '%s', %s.", filename, strerror(errno));
        } else {
            size_t length = size_t(stat.st_size);
            bool ownBuffer = false;
            char* buffer;

            // Fall back to reading into a buffer since we can't mmap files in sysfs.
            // The length we obtained from stat is wrong too (it will always be 4096)
            // so we must trust that read will read the entire file.
            buffer = new char[length];
            ownBuffer = true;
            ssize_t nrd = read(fd, buffer, length);
            if (nrd < 0) {
                result = -errno;
                SYS_LOGE("Error reading file '%s', %s.", filename, strerror(errno));
                delete[] buffer;
                buffer = NULL;
            } else {
                length = size_t(nrd);
            }

            if (!result) {
                *outTokenizer = new SysTokenizer(filename, buffer, ownBuffer, length);
            }
        }
        close(fd);
    }
    return result;
}

int SysTokenizer::fromContents(const char*filename,
        const char* contents, SysTokenizer** outTokenizer) {
    *outTokenizer = new SysTokenizer(filename,
            const_cast<char*>(contents), false, strlen(contents));
    return 0;
}

char* SysTokenizer::getLocation() const {
    memset((void*)mStrs, 0 , MAX_STR_LEN);
    sprintf((char *)mStrs, "%s:%d", mFilename, mLineNumber);
    return (char *)mStrs;
}

char* SysTokenizer::peekRemainderOfLine() const {
    const char* end = getEnd();
    const char* eol = mCurrent;
    while (eol != end) {
        char ch = *eol;
        if (ch == '\n') {
            break;
        }
        eol += 1;
    }

    memset((void*)mLine, 0 , MAX_STR_LEN);
    strncpy((char *)mLine, mCurrent, eol - mCurrent);
    return (char *)mLine;
}

char* SysTokenizer::nextToken(const char* delimiters) {
    const char* end = getEnd();
    const char* tokenStart = mCurrent;

#if DEBUG_TOKENIZER
    SYS_LOGI("nextToken mCurrent:%s, delimiters:%s\n", mCurrent, delimiters);
#endif

    while (mCurrent != end) {
        char ch = *mCurrent;
        if (ch == '\n' || isDelimiter(ch, delimiters)) {
            break;
        }
        mCurrent += 1;
    }

    memset((void*)mStrs, 0 , MAX_STR_LEN);
    strncpy((char *)mStrs, tokenStart, mCurrent - tokenStart);

#if DEBUG_TOKENIZER
    SYS_LOGI("nextToken parse end str:%s, num:%d \n", mStrs, mCurrent - tokenStart);
#endif
    return (char *)mStrs;
}

void SysTokenizer::nextLine() {
#if DEBUG_TOKENIZER
    SYS_LOGV("nextLine");
#endif
    const char* end = getEnd();
    while (mCurrent != end) {
        char ch = *(mCurrent++);
        if (ch == '\n') {
            mLineNumber += 1;
            break;
        }
    }
}

void SysTokenizer::skipDelimiters(const char* delimiters) {
#if DEBUG_TOKENIZER
    SYS_LOGV("skipDelimiters");
#endif
    const char* end = getEnd();
    while (mCurrent != end) {
        char ch = *mCurrent;
        if (ch == '\n' || !isDelimiter(ch, delimiters)) {
            break;
        }
        mCurrent += 1;
    }
}

