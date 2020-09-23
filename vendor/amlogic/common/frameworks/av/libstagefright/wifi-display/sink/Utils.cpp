/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */
 #include "Utils.h"
 #include <cutils/log.h>

 int32_t getPropertyInt(const char *key, int32_t def) {
     int len;
     char* end;
     char buf[92] = {0};
     int32_t result = def;

     len = property_get(key, buf, "");
     if (len > 0) {
         result = strtol(buf, &end, 0);
         if (end == buf) {
             result = def;
         }
     }
     return result;
 }

 void setProperty(const char *key, const char *value) {
     int err;
     err = property_set(key, value);
     if (err < 0) {
         ALOGI("failed to set system property %s\n", key);
     }
}
