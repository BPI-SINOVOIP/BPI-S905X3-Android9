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
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process uevent for system control
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>

#include <utils/Log.h>
#include "common.h"
#include "UEventObserver.h"


UEventObserver::UEventObserver()
    :mFd(-1), mLogLevel(LOG_LEVEL_DEFAULT) {

    mFd = ueventInit();
    mMatchStr.num = 0;
    mMatchStr.strList.buf = NULL;
    mMatchStr.strList.next = NULL;
}

UEventObserver::~UEventObserver() {
    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
}

int UEventObserver::ueventInit() {
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    //addr.nl_pid = pthread_self() << 16 | getpid();
    addr.nl_pid = gettid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    return s;
}

int UEventObserver::ueventGetFd() {
    return mFd;
}

int UEventObserver::ueventNextEvent(char* buffer, int buffer_length) {
    while (1) {
        struct pollfd fds;
        int nr;

        fds.fd = mFd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);

        if (nr > 0 && (fds.revents & POLLIN)) {
            int count = recv(mFd, buffer, buffer_length, 0);
            if (count > 0) {
                return count;
            }
        }
    }

    // won't get here
    return 0;
}

bool UEventObserver::isMatch(const char* buffer, size_t length,
    uevent_data_t* ueventData, const char *matchStr) {
    bool matched = false;

    // Consider all zero-delimited fields of the buffer.
    const char* field = buffer;
    const char* end = buffer + length + 1;
    do {
        if (!strcmp(field, matchStr)) {
            SYS_LOGI("Matched uevent message with pattern: %s", matchStr);

            strcpy(ueventData->matchName, matchStr);
            matched = true;
        }
        //SWITCH_STATE=1, SWITCH_NAME=hdmi
        else if (strstr(field, "STATE=HDMI=")) {
            strcpy(ueventData->switchState, field + strlen("STATE=HDMI="));
        }
        else if (strstr(field, "DEVTYPE=")) {
            strcpy(ueventData->switchName, field + strlen("DEVTYPE="));
        }
        else if (strstr(field, "FRAME_RATE_HINT=")) {
            strcpy(ueventData->switchName, field + strlen("FRAME_RATE_HINT="));
        }
        else if (strstr(field, "FRAME_RATE_END_HINT")) {
            strcpy(ueventData->switchName, "end_hint");
        }
        field += strlen(field) + 1;
    } while (field != end);

    if (matched) {
        ueventData->len = length;
        memcpy(ueventData->buf, buffer, length);
        return matched;
    }

    return matched;
}

bool UEventObserver::isMatch(const char* buffer, size_t length, uevent_data_t* ueventData) {
    bool matched = false;
#ifdef RECOVERY_MODE
    match_node_t *strItem = &mMatchStr.strList;
    //the first node is null
    for (size_t i = 0; i < (unsigned int)mMatchStr.num; i++) {
        const char *matchStr = strItem->buf;
        strItem = strItem->next;
        matched = isMatch(buffer, length, ueventData, matchStr);
        if (matched)
            break;
    }
#else
    AutoMutex _l(gMatchesMutex);
    for (size_t i = 0; i < gMatches.size(); i++) {
        const char *matchStr = gMatches.itemAt(i).string();
        matched = isMatch(buffer, length, ueventData, matchStr);
        if (matched)
            break;
    }
#endif

    return matched;
}

void UEventObserver::waitForNextEvent(uevent_data_t* ueventData) {
    char buffer[1024];

    for (;;) {
        int length = ueventNextEvent(buffer, sizeof(buffer) - 1);
        if (length <= 0) {
            SYS_LOGE("Received uevent message length: %d", length);
            return;
        }
        buffer[length] = '\0';

        ueventPrint(buffer, length);
        if (isMatch(buffer, length, ueventData))
            return;
    }
}

void UEventObserver::addMatch(const char *matchStr) {
#ifdef RECOVERY_MODE
    match_node_t *strItem = &mMatchStr.strList;
    //the first node is null
    if (NULL == strItem->buf) {
        strItem->buf = (char *)malloc(strlen(matchStr) + 1);
        strcpy(strItem->buf, matchStr);
        strItem->next = NULL;

        mMatchStr.num++;
        return;
    }

    while (NULL != strItem->buf) {
        if (!strcmp(strItem->buf, matchStr)) {
            SYS_LOGE("have added uevent : %s before", matchStr);
            return;
        }

        //the last node
        if (NULL == strItem->next) {
            SYS_LOGI("no one match the uevent : %s, add it to list", matchStr);
            break;
        }
        strItem = strItem->next;
    }

    match_node_t *newNode = (match_node_t *)malloc(sizeof(match_node_t));
    newNode->buf = (char *)malloc(strlen(matchStr) + 1);
    strcpy(newNode->buf, matchStr);
    newNode->next = NULL;

    //add the new node to the list
    strItem->next = newNode;

    mMatchStr.num++;

#else
    AutoMutex _l(gMatchesMutex);
    gMatches.add(String8(matchStr));
#endif
}

void UEventObserver::removeMatch(const char *matchStr) {
#ifdef RECOVERY_MODE
    match_node_t *headItem = &mMatchStr.strList;
    match_node_t *curItem = headItem;
    match_node_t *preItem = curItem;
    while (NULL != curItem->buf) {
        if (!strcmp(curItem->buf, matchStr)) {
            SYS_LOGI("find the match uevent : %s, remove it", matchStr);
            match_node_t *tmpNode = curItem->next;
            free(curItem->buf);
            curItem->buf = NULL;
            //head item do not need free
            if (curItem != headItem) {
                free(curItem);
                curItem = NULL;
            }

            preItem->next = tmpNode;
            mMatchStr.num--;
            return;
        }

        //the last node
        if (NULL == curItem->next) {
            SYS_LOGE("can not find the match uevent : %s", matchStr);
            return;
        }
        preItem = curItem;
        curItem = curItem->next;
    }

#else
    AutoMutex _l(gMatchesMutex);
    for (size_t i = 0; i < gMatches.size(); i++) {
        if (gMatches.itemAt(i) == matchStr) {
            gMatches.removeAt(i);
            break; // only remove first occurrence
        }
    }
#endif
}

void UEventObserver::setLogLevel(int level) {
    mLogLevel = level;
}

void UEventObserver::ueventPrint(char* ueventBuf, int len) {
    if (mLogLevel > LOG_LEVEL_1) {
        //change@/devices/virtual/switch/hdmi ACTION=change DEVPATH=/devices/virtual/switch/hdmi
        //SUBSYSTEM=switch SWITCH_NAME=hdmi SWITCH_STATE=0 SEQNUM=2791
        char printBuf[1024] = {0};
        memcpy(printBuf, ueventBuf, len);
        for (int i = 0; i < len; i++) {
            if (printBuf[i] == 0x0)
                printBuf[i] = ' ';
        }

        SYS_LOGI("Received uevent message: %s", printBuf);
    }
}

