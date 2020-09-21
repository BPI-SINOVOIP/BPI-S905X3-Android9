/*
 * Copyright (C) 2010 The Android Open Source Project
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
 */

 #define LOG_TAG "sub_io"

#include "sub_io.h"
#include "sub_socket.h"
#include "sub_control.h"

//for dev
int pollFd(int sub_fd, int timeout) {
    int ret = -1;

    if (mIOType == IO_TYPE_DEV) {
        ret = subtitle_poll_sub_fd(sub_fd, timeout);
    }

    return ret;
}

//for socket
void startSocketServer() {
    //if (mIOType == IO_TYPE_SOCKET) {
        startServer();
    //}
}

void stopSocketServer() {
    //if (mIOType == IO_TYPE_SOCKET) {
        stopServer();
    //}
}

int getInfo(int type) {
    int ret = -1;
    if (mIOType == IO_TYPE_SOCKET) {
        ret = getInfoBySkt(type);
    }

    return ret;
}

//for common
int getSize(int sub_fd) {
    int ret = -1;

    if (mIOType == IO_TYPE_DEV) {
        ret = subtitle_get_sub_size_fd(sub_fd);
    }
    else if (mIOType == IO_TYPE_SOCKET) {
        ret = getSizeBySkt();
    }

    return ret;
}

void getData(int sub_fd, char *buf, int size) {
    if (mIOType == IO_TYPE_DEV) {
        subtitle_read_sub_data_fd(sub_fd, buf, size);
    }
    else if (mIOType == IO_TYPE_SOCKET) {
        getDataBySkt(buf, size);
    }
}

void setIOType(IOType type) {
    mIOType = type;
}

IOType getIOType() {
    return mIOType;
}

void getInSubTypeStrs(char *subTypeStr) {
    getInSubTypeStrBySkt(subTypeStr);
}

void getInSubLanStrs(char *subLanStr) {
    getInSubLanStrBySkt(subLanStr);
}

void getPcrscr(char* pcrStr) {
    getPcrscrBySkt(pcrStr);
}
