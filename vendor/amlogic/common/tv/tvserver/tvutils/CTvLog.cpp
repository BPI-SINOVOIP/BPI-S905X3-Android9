/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#include <stdio.h>
#include "include/CTvLog.h"
#include <android/log.h>

int __tv_log_print(int prio, const char *tag, const char *tv_tag, const char *fmt, ...)
{
    char buf[DEFAULT_LOG_BUFFER_LEN];
    sprintf(buf, "[%s]:", tv_tag);
    int tv_tag_len = strlen(buf);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + tv_tag_len, DEFAULT_LOG_BUFFER_LEN - tv_tag_len, fmt, ap);
    va_end(ap);

    return __android_log_write(prio, tag, buf);
}

