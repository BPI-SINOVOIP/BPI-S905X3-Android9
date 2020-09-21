/*
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef C_FILE_H
#define C_FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define CC_MAX_FILE_PATH_LEN (256)

#define BUFFER_SIZE 1024

class CFile {
public:
    CFile(const char *path);
    CFile();
    virtual ~CFile();
    virtual int openFile(const char *path);
    virtual int closeFile();
    virtual int writeFile(const unsigned char *pData, int uLen);
    virtual int readFile(void *pBuf, int uLen);
    int copyTo(const char *dstPath);
    static int delFile(const char *path);
    static int  getFileAttrValue(const char *path);
    static int  setFileAttrValue(const char *path, int value);
    static int getFileAttrStr(const char *path, char *str);
    static int setFileAttrStr(const char *path, const char *str);
    int delFile();
    int flush();
    int seekTo();
    int seekToBegin();
    int seekToEnd();
    int getLength();
    int getFd()
    {
        return mFd;
    };
protected:
    char mPath[CC_MAX_FILE_PATH_LEN];
    int  mFd;
};

#endif
