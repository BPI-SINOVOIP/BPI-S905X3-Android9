/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#ifndef ANDROID_GETINMEMORY_H
#define ANDROID_GETINMEMORY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utils/Log.h>

namespace android {

struct MemoryStruct {
  char *memory;
  size_t size;
};


size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);


//char* getFileByCurl(char* url);
char* getFileByCurl(char* url);


}

#endif // ANDROID_GETINMEMORY_H

