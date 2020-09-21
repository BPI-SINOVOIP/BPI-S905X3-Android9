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

#include "CFile.h"
#include <unistd.h>
#include <stdlib.h>
#include <CFbcLog.h>


CFile::CFile()
{
    mPath[0] = '\0';
    mFd = -1;
}

CFile::~CFile()
{
    closeFile();
}

CFile::CFile(const char *path)
{
    strcpy(mPath, path);
    mFd = -1;
}

int CFile::openFile(const char *path)
{
    LOGD("openFile = %s", path);
    if (mFd < 0) {
        const char *openPath = mPath;
        if (path != NULL)
            strcpy(mPath, path);

        if (strlen(openPath) <= 0) {
            LOGD("openFile openPath is NULL, path:%s", path);
            return -1;
        }
        mFd = open(openPath, O_RDWR);
        if (mFd < 0) {
            LOGD("open file(%s) fail", openPath);
            return -1;
        }
    }

    return mFd;
}

int CFile::closeFile()
{
    if (mFd > 0) {
        close(mFd);
        mFd = -1;
    }
    return 0;
}

int CFile::writeFile(const unsigned char *pData, int uLen)
{
    int ret = -1;
    if (mFd > 0)
        ret = write(mFd, pData, uLen);

    return ret;
}

int CFile::readFile(void *pBuf, int uLen)
{
    int ret = 0;
    if (mFd > 0) {
        ret = read(mFd, pBuf, uLen);
    }
    return ret;
}

int CFile::copyTo(const char *dstPath)
{
    if (strlen(mPath) <= 0)
        return -1;
    int dstFd;
    if (mFd == -1) {
        if ((mFd = open(mPath, O_RDONLY)) == -1) {
            LOGD("Open %s Error:%s/n", mPath, strerror(errno));
            return -1;
        }
    }

    if ((dstFd = open(dstPath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        LOGD("Open %s Error:%s/n", dstPath, strerror(errno));
    }

    int bytes_read, bytes_write;
    char buffer[BUFFER_SIZE];
    char *ptr;
    int ret = 0;
    while ((bytes_read = read(mFd, buffer, BUFFER_SIZE))) {
        /* fatal error happen */
        if ((bytes_read == -1) && (errno != EINTR)) {
            ret = -1;
            break;
        } else if (bytes_read > 0) {
            ptr = buffer;
            while ((bytes_write = write(dstFd, ptr, bytes_read))) {
                /* fatal error happen */
                if ((bytes_write == -1) && (errno != EINTR)) {
                    ret = -1;
                    break;
                }
                /* write finish */
                else if (bytes_write == bytes_read) {
                    ret = 0;
                    break;
                }
                /* write not finish, continue */
                else if (bytes_write > 0) {
                    ptr += bytes_write;
                    bytes_read -= bytes_write;
                }
            }
            /* write error */
            if (bytes_write == -1) {
                ret = -1;
                break;
            }
        }
    }
    fsync(dstFd);
    close(dstFd);
    return ret;
}


int CFile::delFile(const char *path)
{
    if (strlen(path) <= 0) return -1;
    if (unlink(path) != 0) {
        LOGD("delete file(%s) err=%s", path, strerror(errno));
        return -1;
    }
    return 0;
}

int CFile::delFile()
{
    if (strlen(mPath) <= 0) return -1;
    if (unlink(mPath) != 0) {
        LOGD("delete file(%s) err=%s", mPath, strerror(errno));
        return -1;
    }
    return 0;
}


int  CFile::getFileAttrValue(const char *path)
{
    int value;

    int fd = open(path, O_RDONLY);
    if (fd <= 0) {
        LOGD("open (%s)ERROR!! error = -%s- \n", path, strerror(errno));
    }
    char s[8];
    read(fd, s, sizeof(s));
    close(fd);
    value = atoi(s);
    return value;
}

int  CFile::setFileAttrValue(const char *path, int value)
{
    FILE *fp = fopen(path, "w");

    if (fp == NULL) {
        LOGD("Open %s error(%s)!\n", path, strerror(errno));
        return -1;
    }
    fprintf(fp, "%d", value);
    fclose(fp);
    return 0;
}

int CFile::getFileAttrStr(const char *path __unused, char *str __unused)
{
    return 0;
}

int CFile::setFileAttrStr(const char *path, const char *str)
{
    FILE *fp = fopen(path, "w");

    if (fp == NULL) {
        LOGD("Open %s error(%s)!\n", path, strerror(errno));
        return -1;
    }
    fprintf(fp, "%s", str);
    fclose(fp);
    return 0;
}
