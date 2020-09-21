/*
 *
 * honggfuzz - file operations
 * -----------------------------------------
 *
 * Author: Robert Swiecki <swiecki@google.com>
 *
 * Copyright 2010-2015 by Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 */

#ifndef _HF_FILES_H_
#define _HF_FILES_H_

#include "common.h"

#include <stdint.h>
#include <unistd.h>

extern ssize_t files_readFileToBufMax(char* fileName, uint8_t* buf, size_t fileMaxSz);

extern bool files_writeBufToFile(
    const char* fileName, const uint8_t* buf, size_t fileSz, int flags);

extern bool files_writeToFd(int fd, const uint8_t* buf, size_t fileSz);

extern bool files_writeStrToFd(int fd, const char* str);

extern ssize_t files_readFromFd(int fd, uint8_t* buf, size_t fileSz);

extern bool files_writePatternToFd(int fd, off_t size, unsigned char p);

bool files_sendToSocketNB(int fd, const uint8_t* buf, size_t fileSz);

bool files_sendToSocket(int fd, const uint8_t* buf, size_t fileSz);

extern bool files_exists(const char* fileName);

extern const char* files_basename(const char* fileName);

extern bool files_copyFile(
    const char* source, const char* destination, bool* dstExists, bool try_link);

extern uint8_t* files_mapFile(const char* fileName, off_t* fileSz, int* fd, bool isWritable);

extern uint8_t* files_mapFileShared(const char* fileName, off_t* fileSz, int* fd);

extern void* files_mapSharedMem(size_t sz, int* fd, const char* dir);

extern bool files_readPidFromFile(const char* fileName, pid_t* pidPtr);

extern size_t files_parseSymbolFilter(const char* inFIle, char*** filterList);

#endif
