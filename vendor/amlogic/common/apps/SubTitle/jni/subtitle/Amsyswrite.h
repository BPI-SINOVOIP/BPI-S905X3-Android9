/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef AMSYSWRITE_UTILS_H
#define AMSYSWRITE_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amSystemControlGetProperty(const char *key, char *value);
    int amSystemControlGetPropertyStr(const char *key, char *def,
                                      char *value);
    int amSystemControlGetPropertyInt(const char *key, int def);
    long amSystemControlGetPropertyLong(const char *key, long def);
    int amSystemControlGetPropertyBool(const char *key, int def);
    void amSystemControlSetProperty(const char *key, const char *value);
    int amSystemControlReadSysfs(const char *path, char *value);
    int amSystemControlReadNumSysfs(const char *path, char *value,
                                    int size);
    int amSystemControlWriteSysfs(const char *path, char *value);
    void amDumpMemoryAddresses(int fd);

#ifdef  __cplusplus
}
#endif
#endif
