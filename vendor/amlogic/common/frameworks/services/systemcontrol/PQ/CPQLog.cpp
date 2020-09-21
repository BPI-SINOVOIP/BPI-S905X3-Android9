/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */
#include "CPQLog.h"
#include <cutils/properties.h>


int __pq_log_print(int prio, const char *tag, const char *pq_tag, const char *fmt, ...)
{
    char buf[PROPERTY_VALUE_MAX] = {0};
    int len = property_get("vendor.pq.log.enable", buf, "enable");
    if (strcmp(buf, "disable") == 0) {
        return 0;
    } else {
        char buf[DEFAULT_LOG_BUFFER_LEN];
        sprintf(buf, "[%s]:", pq_tag);
        int pq_tag_len = strlen(buf);

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf + pq_tag_len, DEFAULT_LOG_BUFFER_LEN - pq_tag_len, fmt, ap);

        return __android_log_write(prio, tag, buf);
    }
}

