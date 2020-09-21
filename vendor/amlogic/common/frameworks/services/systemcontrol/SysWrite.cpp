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

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/types.h>
#include <SysWrite.h>
#include <common.h>

#include <sys/ioctl.h>
#include <sys/types.h>


SysWrite::SysWrite()
    :mLogLevel(LOG_LEVEL_DEFAULT){
}

SysWrite::~SysWrite() {
}

bool SysWrite::getProperty(const char *key, char *value){
    property_get(key, value, "");
    /*
    char buf[PROPERTY_VALUE_MAX] = {0};
    property_get(key, buf, "");
    value.setTo(String16(buf));
    */
    return true;
}

bool SysWrite::getPropertyString(const char *key, char *value,  const char *def){
    property_get(key, value, def);
    return true;
}

int32_t SysWrite::getPropertyInt(const char *key, int32_t def){
    int len;
    char* end;
    char buf[PROPERTY_VALUE_MAX] = {0};
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

int64_t SysWrite::getPropertyLong(const char *key, int64_t def){

    int len;
    char buf[PROPERTY_VALUE_MAX] = {0};
    char* end;
    int64_t result = def;

    len = property_get(key, buf, "");
    if (len > 0) {
        result = strtoll(buf, &end, 0);
        if (end == buf) {
            result = def;
        }
    }

    return result;
}

bool SysWrite::getPropertyBoolean(const char *key, bool def){

    int len;
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool result = def;

    len = property_get(key, buf, "");
    if (len == 1) {
        char ch = buf[0];
        if (ch == '0' || ch == 'n')
            result = false;
        else if (ch == '1' || ch == 'y')
            result = true;
    } else if (len > 1) {
         if (!strcmp(buf, "no") || !strcmp(buf, "false") || !strcmp(buf, "off")) {
            result = false;
        } else if (!strcmp(buf, "yes") || !strcmp(buf, "true") || !strcmp(buf, "on")) {
            result = true;
        }
    }

    return result;
}

void SysWrite::setProperty(const char *key, const char *value){
    int err;
    err = property_set(key, value);
    if (err < 0) {
        SYS_LOGE("failed to set system property %s\n", key);
    }
}

bool SysWrite::readSysfs(const char *path, char *value){
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, false);
    strcpy(value, buf);
    return true;
}

bool SysWrite::readUnifyKey(const char *path, char *value) {
    char buf[MAX_STR_LEN+1] = {0};
    int ret = readUnifyKeyfs(path, (char*)buf, MAX_STR_LEN);
    if (ret > 1) {
        strcpy(value, buf);
        SYS_LOGE("readUnifyKey, value: %s", value);
        return true;
    } else
        return false;
}

int32_t SysWrite::readPlayreadyKey(const char *path, char *value, int size) {
    char buf[4096] = {0};
    int i;
    int ret = readUnifyKeyfs(path, (char*)buf, size);
    if (ret > 1) {
        for (i = 0; i < ret; ++i) {
            value[i] = buf[i];
        }
        SYS_LOGE("readPlayreadyKey, ret = %d", ret);
    }
    return ret;
}


// get the original data from sysfs without any change.
bool SysWrite::readSysfsOriginal(const char *path, char *value){
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, true);
    strcpy(value, buf);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value){
    writeSys(path, value);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value, const int size){
    writeSys(path, value, size);
    return true;
}

bool SysWrite::writeUnifyKey(const char *path, const char *value){
    int ret;
    ret = writeUnifyKeyfs(path, value);
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::writePlayreadyKey(const char *path, const char *value, const int size){
    int ret;
    ret = writePlayreadyKeyfs(path, value, size);
    if (ret == 0)
        return true;
    else
        return false;
}

int32_t SysWrite::readAttestationKey(const char * node, const char *name, char *value, int size) {
    char buf[ATTESTATION_KEY_LEN] = {0};
    int i;
    int ret = readAttestationKeyfs(node, name, (char*)buf, size);

    if (ret > 1) {
        for (i = 0; i < ret; ++i) {
            value[i] = buf[i];
        }
        SYS_LOGE("readUnifyKey, ret = %d", ret);
    }
    return ret;
}

bool SysWrite::writeAttestationKey(const char * node, const char *name, const char *buff, const int size) {
    int ret;
    SYS_LOGE("come to SysWrite::writeAttestationKey size = %d\n", size);

    ret = writeAttestationKeyfs(node, name, buff, size);
    SYS_LOGI("writeAttestationKey ret = %d\n", ret);
    if (ret == 0)
        return true;
    else
        return false;
}


void SysWrite::setLogLevel(int level){
    mLogLevel = level;
}

void SysWrite::writeSys(const char *path, const char *val){
    int fd;

    if((fd = open(path, O_RDWR)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        goto exit;
    }

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("write %s, val:%s\n", path, val);

    write(fd, val, strlen(val));

exit:
    SYS_LOGI("write %s, val:%s end\n", path, val);

    close(fd);
}

int SysWrite::writeSys(const char *path, const char *val, const int size){
    int fd;

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("writeSysFs, size = %d \n", size);

    if ((fd = open(path, O_WRONLY)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        return -1;
    }

    if (write(fd, val, size) != size) {
        SYS_LOGE("write %s size:%d failed!\n", path, size);
        return -1;
    }

    close(fd);
    return 0;
}

int SysWrite::readUnifyKeyfs(const char *path, char *value, int count) {
    int keyLen = 0;
    char existKey[10] = {0};

    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        goto _exit;
    }

    keyLen = readSys(UNIFYKEY_READ, value, count);
    if (keyLen < 1) {
        SYS_LOGE("read key length fail, at least 1 bytes, but read len = %d\n", keyLen);
        goto _exit;
    }

    SYS_LOGE("read success, read len = %d\n", keyLen);
_exit:
    return keyLen;
}

int SysWrite::writeUnifyKeyfs(const char *path, const char *value) {
    int keyLen;
    char existKey[10] = {0};
    int ret;
    char lock_str[10] = {0};
    int size = 0;
    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);
    size = strlen(value);

    do {
        readSys(UNIFYKEY_EXIST, (char*)lock_str, 10);
        ret = atoi(lock_str);
        SYS_LOGE("ret = %d\n", ret);
    }while(ret != 0);

    writeSys(UNIFYKEY_LOCK, "1");

    keyLen = writeSys(UNIFYKEY_WRITE, value, size);
    if (keyLen != 0) {
        SYS_LOGE("write key length fail\n");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }

    usleep(100*1000);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");
    return 0;
}

int SysWrite::writePlayreadyKeyfs(const char *path, const char *value, const int size) {
    int keyLen;
    char existKey[10] = {0};
    int ret;
    char lock_str[10] = {0};
    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    do {
        readSys(UNIFYKEY_EXIST, (char*)lock_str, 10);
        ret = atoi(lock_str);
        SYS_LOGE("ret = %d\n", ret);
    }while(ret != 0);

    writeSys(UNIFYKEY_LOCK, "1");

    keyLen = writeSys(UNIFYKEY_WRITE, value, size);
    if (keyLen != 0) {
        SYS_LOGE("write key length fail\n");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }

    usleep(100*1000);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");
    return 0;
}

void SysWrite::dump_keyitem_info(struct key_item_info_t *info) {
    if (info == NULL)
        return;
    SYS_LOGI("id: %d\n", info->id);
    SYS_LOGI("name: %s\n", info->name);
    SYS_LOGI("size: %d\n", info->size);
    SYS_LOGI("permit: 0x%x\n", info->permit);
    SYS_LOGI("flag: 0x%x\n", info->flag);
    return;
}

int SysWrite::readAttestationKeyfs(const char * node, const char *name, char *value, int size) {
    int ret = 0;
    unsigned long ppos;
    int readsize = 0;
    int fp;
    int i;
    struct key_item_info_t key_item_info;
    if ((NULL == node) || (NULL == name)) {
        SYS_LOGE("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }
    SYS_LOGI("path=%s\n", node);
    fp  = open(node, O_RDWR);
    if (fp < 0) {
        SYS_LOGE("no %s found\n", node);
        return -1;
    }
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    ppos = key_item_info.id;
    lseek(fp, ppos, SEEK_SET);
    SYS_LOGI("%s() %d: key ioctl  KEYUNIFY_GET_INFO is %d\n", __func__, __LINE__, ret);
    if (ret < 0) {
        close(fp);
        return ret;
    }
    dump_keyitem_info(&key_item_info);
    SYS_LOGI("size =  %d", size);
    if (key_item_info.flag) {
        readsize = read(fp, value, key_item_info.size);
        SYS_LOGI("readsize =  %d", readsize);
    }

    close(fp);
    return readsize;
}

int SysWrite::writeAttestationKeyfs(const char * node, const char *name, const char *buff, const int size) {
    int ret = 0;
    unsigned long ppos;
    int writesize;
    int fp;
    int i;
    struct key_item_info_t key_item_info;
    if ((NULL == node) || (NULL == buff) || (NULL == name)) {
        SYS_LOGE("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }

    SYS_LOGI("path=%s\n", node);
    fp  = open(node, O_RDWR);
    if (fp < 0) {
        SYS_LOGE("no %s found\n", node);
        return -1;
    }/* seek the key index need operate. */
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    ppos = key_item_info.id;
    lseek(fp, ppos, SEEK_SET);
    SYS_LOGI("%s() %d: ret is %d\n", __func__, __LINE__, ret);
    if (ret < 0) {
        close(fp);
        return ret;
    }
    dump_keyitem_info(&key_item_info);

    writesize = write(fp, buff, size);
    if (writesize != size) {
        SYS_LOGI("%s() %d: write %s failed!\n", __func__, __LINE__, key_item_info.name);
    }
    SYS_LOGI("%s() %d, write %ld down!\n", __func__, __LINE__, writesize);
    close(fp);
    SYS_LOGI("ret = %d \n", ret);
    return ret;
}


int SysWrite::readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSys, open %s fail. Error info [%s]", path, strerror(errno));
        return len;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);
    return len;
}


void SysWrite::readSys(const char *path, char *buf, int count, bool needOriginalData){
    int fd, len;
    char *pos;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSysFs, open %s fail. Error info [%s]", path, strerror(errno));
        goto exit;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("read error: %s, %s\n", path, strerror(errno));
        goto exit;
    }

    if (!needOriginalData) {
        int i , j;
        for (i = 0, j = 0; i <= len -1; i++) {
            /*change '\0' to 0x20(spacing), otherwise the string buffer will be cut off
             * if the last char is '\0' should not replace it
             */
            if (0x0 == buf[i] && i < len - 1) {
                buf[i] = 0x20;

                if (mLogLevel > LOG_LEVEL_1)
                    SYS_LOGI("read buffer index:%d is a 0x0, replace to spacing \n", i);
            }

            /* delete all the character of '\n' */
            if (0x0a != buf[i]) {
                buf[j++] = buf[i];
            }
        }

        buf[j] = 0x0;
    }

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("read %s, result length:%d, val:%s\n", path, len, buf);

exit:
    close(fd);
}

#if 0
status_t SysWrite::dump(int fd, const Vector<String16>& args){
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump sys.write from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);

        result.appendFormat("sys write service wrote by multi-user mode, normal process will have not system privilege\n");
        /*
        int n = args.size();
        for (int i = 0; i + 1 < n; i++) {
            String16 verboseOption("-v");
            if (args[i] == verboseOption) {
                String8 levelStr(args[i+1]);
                int level = atoi(levelStr.string());
                result = String8::format("\nSetting log level to %d.\n", level);
                setLogLevel(level);
                write(fd, result.string(), result.size());
            }
        }*/
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}
#endif
