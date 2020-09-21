/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "FBC"
#include <stdio.h>
#include "include/CFbcLog.h"
#include <android/log.h>

int __fbc_log_print(int prio, const char *tag, const char *fbc_tag, const char *fmt, ...)
{
    char buf[DEFAULT_LOG_BUFFER_LEN];
    sprintf(buf, "[%s]:", fbc_tag);
    int fbc_tag_len = strlen(buf);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + fbc_tag_len, DEFAULT_LOG_BUFFER_LEN - fbc_tag_len, fmt, ap);

    return __android_log_write(prio, tag, buf);
}

