/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef AMSYSWRITE_UTILS_H
#define AMSYSWRITE_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amSystemWriteGetProperty(const char* key, char* value);
    int amSystemWriteGetPropertyStr(const char* key, char* def, char* value);
    int amSystemWriteGetPropertyInt(const char* key, int def);
    long amSystemWriteGetPropertyLong(const char* key, long def);
    int amSystemWriteGetPropertyBool(const char* key, int def);
    void amSystemWriteSetProperty(const char* key, const char* value);
    int amSystemWriteReadSysfs(const char* path, char* value);
    int amSystemWriteReadNumSysfs(const char* path, char* value, int size);
    int amSystemWriteWriteSysfs(const char* path, char* value);

#if ANDROID_PLATFORM_SDK_VERSION >= 21 //5.0
    int amSystemControlSetNativeWindowRect(int x, int y, int w, int h);
#endif


#ifdef  __cplusplus
}
#endif


#endif
