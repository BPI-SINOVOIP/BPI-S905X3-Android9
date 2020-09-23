/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */
#ifndef _UTILS_H_
#define _UTILS_H_
#include <stdlib.h>
#include <cutils/properties.h>
#include <stdint.h>

    int32_t getPropertyInt(const char *key, int32_t def);
    void    setProperty(const char *key, const char *value);
#endif
