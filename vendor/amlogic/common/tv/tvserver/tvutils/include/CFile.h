/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef C_FILE_H
#define C_FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define CC_MAX_FILE_PATH_LEN       (256)

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
