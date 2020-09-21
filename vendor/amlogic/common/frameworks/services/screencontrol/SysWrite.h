/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  2.0
 *  @date     2014/09/09
 *  @par function description:
 *  - 1 write property or sysfs in daemon
 */
#ifndef SYS_WRITE_H
#define SYS_WRITE_H

#include <utils/Log.h>
#define SYS_LOGE            ALOGE
#define SYS_LOGD            ALOGD
#define SYS_LOGV            ALOGV
#define SYS_LOGI            ALOGI
enum {
    LOG_LEVEL_0             = 0,
    LOG_LEVEL_1             = 1,//debug property
    LOG_LEVEL_2             = 2,//debug sysfs read write, parse config file
    LOG_LEVEL_TOTAL         = 3
};
#define MAX_STR_LEN         4096
#define LOG_LEVEL_DEFAULT   LOG_LEVEL_1

class SysWrite {
  public:
    SysWrite();
    ~SysWrite();

    bool getProperty(const char *key, char *value);
    bool getPropertyString(const char *key, char *value, const char *def);
    int32_t getPropertyInt(const char *key, int32_t def);
    int64_t getPropertyLong(const char *key, int64_t def);

    bool getPropertyBoolean(const char *key, bool def);
    void setProperty(const char *key, const char *value);

    bool readSysfs(const char *path, char *value);
    bool readSysfsOriginal(const char *path, char *value);
    bool writeSysfs(const char *path, const char *value);
    bool writeSysfs(const char *path, const char *value, const int size);
    void setLogLevel(int level);
  private:
    void writeSys(const char *path, const char *val);
    int writeSys(const char *path, const char *val, const int size);
    void readSys(const char *path, char *buf, int count, bool needOriginalData);
    int readSys(const char *path, char *buf, int count);
    int mLogLevel;
};

#endif // SYS_WRITE_H
