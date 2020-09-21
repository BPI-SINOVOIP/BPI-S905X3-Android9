/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#ifndef AMSYSFS_UTILS_H
#define AMSYSFS_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amsysfs_set_sysfs_str(const char *path, const char *val);
    int amsysfs_get_sysfs_str(const char *path, char *valstr, int size);
    int amsysfs_set_sysfs_int(const char *path, int val);
    int amsysfs_get_sysfs_int(const char *path);
    int amsysfs_set_sysfs_int16(const char *path, int val);
    int amsysfs_get_sysfs_int16(const char *path);
    unsigned long amsysfs_get_sysfs_ulong(const char *path);
    void amsysfs_write_prop(const char* key, const char* value);
#ifdef  __cplusplus
}
#endif


#endif

