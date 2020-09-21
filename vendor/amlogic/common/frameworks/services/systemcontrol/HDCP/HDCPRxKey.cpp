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
 *  @version  1.0
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process HDCP RX key combie and refresh
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "HDCPRxKey.h"
#include "HdcpKeyDecrypt.h"
#include "sha1.h"

#ifndef RECOVERY_MODE
//using namespace android;
#endif

#define HDCP_RX22_KEY_NAME                  "hdcp22_rx_fw"
#define HDCP_RPRX22_KEY_NAME                "hdcp22_rprx_fw"
#define HDCP_RPRP22_KEY_NAME                "hdcp22_rprp_fw"
#define HDCP_RX14_KEY_NAME                  "hdcp14_rx"

#define HDCP_RX_KEY_ATTACH_DEV_PATH         "/sys/class/unifykeys/attach"
#define HDCP_RX_KEY_NAME_DEV_PATH           "/sys/class/unifykeys/name"
#define HDCP_RX_KEY_DATA_EXIST              "/sys/class/unifykeys/exist"
#define HDCP_RX_KEY_READ_DEV_PATH           "/sys/class/unifykeys/read"
#define HDCP_RX_KEY_WRITE_DEV_PATH          "/sys/class/unifykeys/write"
#define HDCP_RX_KEY_SECURE_DEV_PATH         "/sys/class/unifykeys/secure"


#define HDCP_RX22_TOOL_KEYS                 "hdcprxkeys"
#define HDCP_RX22_TOOL_ESM_SWAP             "esm_swap"
#define HDCP_RX22_TOOL_AIC                  "aictool"
#define HDCP_RX22_KEY_FORMAT                "-b"
#define HDCP_RX22_KEYS_PATH                  "/vendor/bin/hdcprxkeys"
#define HDCP_RX22_ESM_SWAP_PATH              "/vendor/bin/esm_swap"
#define HDCP_RX22_AIC_TOOL_PATH              "/vendor/bin/aictool"

//system/etc/firmware/hdcp_rx22/esm_config.i
//system/etc/firmware/hdcp_rx22/firmware.rom
//system/etc/firmware/hdcp_rx22/firmware.aic
//esm_config.i, firmware.rom, hdcp_keys.le, used by firmware.aic
#define HDCP_RX22_CFG_AIC_SRC               "/vendor/etc/firmware/hdcp_rx22/firmware.aic"
#define HDCP_RX22_CFG_AIC_DES               "/mnt/vendor/param/firmware.aic"
#define HDCP_RX22_KEY_PATH                  "/mnt/vendor/param/hdcp2_rx.bin"
#define HDCP_RP22_CFG_AIC_SRC               "/vendor/etc/firmware/hdcp_rp22/firmware.aic"

#define HDCP_RX22_OUT_KEY_IMG               "/mnt/vendor/param/dcp_rx.out"
#define HDCP_RX22_OUT_KEY_LE                "/mnt/vendor/param/hdcp_keys.le"
#define HDCP_RX22_OUT_2080_BYTE             "/mnt/vendor/param/2080.byte"

#define HDCP_RX22_SRC_FW_PATH               "/vendor/etc/firmware/hdcp_rx22/firmware.le"
#define HDCP_RX22_DES_FW_PATH               "/mnt/vendor/param/firmware.le"
#define HDCP_RX22_KEY_CRC_PATH              "/mnt/vendor/param/hdcprx22.crc"
#define HDCP_RP22_KEY_CRC_PATH              "/mnt/vendor/param/hdcprp22.crc"
#define HDCP_RX22_KEY_CRC_LEN               4

#define HDCP_RP22_DEST_FW_PATH              "/vendor/etc/firmware/hdcp_rp22/firmware.le"
#define HDCP_RX_DEBUG_PATH                  "/sys/class/hdmirx/hdmirx0/debug"
#define HDMI_RX_HPD_OK_FLAG                 "/sys/module/tvin_hdmirx/parameters/hdcp22_firmware_ok_flag"
#define HDCP_RPRX22_SRC_FW_PATH             "/vendor/etc/firmware/hdcp_rp22/firmware_rprx.le"
#define HDCP_RPRX22_DES_FW_PATH             "/mnt/vendor/param/firmware_rprx.le"
#define HDCP_RX14_KEY_MODE_DEV              "/dev/hdmirx0"

#define HDCP_RX22_STORAGE_KEY_SIZE          (10U<<10)//10K

#define HDCP_RX14_KEY_TOTAL_SIZE            (368)
#define HDCP_RX14_KEY_HEAD_SIZE             (40)
#define HDCP_RX14_KEY_CONTENT_SIZE          (HDCP_RX14_KEY_TOTAL_SIZE - HDCP_RX14_KEY_HEAD_SIZE)
#define HDCP_RXRP22_2080BYTE_SIZE             2080

static unsigned char HDCP_RX14_KEY_DEF_HEADER[HDCP_RX14_KEY_HEAD_SIZE] = {
    //40 bytes
    0x53, 0x4B, 0x59, 0x01, 0x00, 0x10, 0x0D, 0x15, 0x3A, 0x8E, // 000~009
    0x99, 0xEE, 0x2A, 0x55, 0x58, 0xEE, 0xED, 0x4B, 0xBE, 0x00, // 010~019
    0x74, 0xA9, 0x00, 0x10, 0x0A, 0x21, 0xE3, 0x30, 0x66, 0x34, // 020~029
    0xCE, 0x9C, 0xC7, 0x8B, 0x51, 0x27, 0xF9, 0x0B, 0xAD, 0x09, // 030~039
};

static unsigned char HDCP_RX14_KEY_DEF_CONTENT[HDCP_RX14_KEY_CONTENT_SIZE] = {
    //328 bytes
    0x5F, 0x4D, 0xC2, 0xCA, 0xA2, 0x13, 0x06, 0x18, 0x8D, 0x34, // 000~009
    0x82, 0x46, 0x2D, 0xC9, 0x4B, 0xB0, 0x1C, 0xDE, 0x3D, 0x49, // 010~019
    0x39, 0x58, 0xEF, 0x2B, 0x68, 0x39, 0x71, 0xC9, 0x4D, 0x25, // 020~029
    0xE9, 0x75, 0x4D, 0xAC, 0x62, 0xF5, 0xF5, 0x87, 0xA0, 0xB2, // 030~039
    0x4A, 0x60, 0xD3, 0xF1, 0x09, 0x3A, 0xB2, 0x3E, 0x19, 0x4F, // 040~049
    0x3B, 0x1B, 0x2F, 0x85, 0x14, 0x28, 0x44, 0xFC, 0x69, 0x6F, // 050~059
    0x50, 0x42, 0x81, 0xBF, 0x7C, 0x2B, 0x3A, 0x17, 0x2C, 0x15, // 060~069
    0xE4, 0x93, 0x77, 0x74, 0xE8, 0x1F, 0x1C, 0x38, 0x54, 0x49, // 070~079
    0x10, 0x64, 0x5B, 0x7D, 0x90, 0x3D, 0xA0, 0xE1, 0x8B, 0x67, // 080~089
    0x5C, 0x19, 0xE6, 0xCA, 0x9D, 0xE9, 0x68, 0x5A, 0xB5, 0x62, // 090~099
    0xDF, 0xA1, 0x28, 0xBC, 0x68, 0x82, 0x9A, 0x22, 0xC4, 0xDC, // 100~109
    0x48, 0x85, 0x0F, 0xF1, 0x3E, 0x05, 0xDD, 0x1B, 0x2D, 0xF5, // 120~119
    0x49, 0x3A, 0x15, 0x29, 0xE7, 0xB6, 0x0B, 0x2A, 0x40, 0xE3, // 120~129
    0xB0, 0x89, 0xD5, 0x75, 0x84, 0x2E, 0x76, 0xE7, 0xBC, 0x63, // 130~139
    0x67, 0xE3, 0x57, 0x67, 0x86, 0x81, 0xF4, 0xD7, 0xEA, 0x4D, // 140~149
    0x89, 0x8E, 0x37, 0x95, 0x59, 0x1C, 0x8A, 0xCD, 0x79, 0xF8, // 150~159
    0x4F, 0x82, 0xF2, 0x6C, 0x7E, 0x7F, 0x79, 0x8A, 0x6B, 0x90, // 160~169
    0xC0, 0xAF, 0x4C, 0x8D, 0x43, 0x47, 0x1F, 0x9A, 0xF1, 0xBB, // 170~179
    0x88, 0x64, 0x49, 0x14, 0x50, 0xD1, 0xC3, 0xDF, 0xA6, 0x87, // 180~189
    0xA0, 0x15, 0x98, 0x51, 0x81, 0xF5, 0x97, 0x55, 0x10, 0x4A, // 190~199
    0x99, 0x30, 0x54, 0xA4, 0xFC, 0xDA, 0x0E, 0xAC, 0x6A, 0xFA, // 200~209
    0x90, 0xEE, 0x12, 0x70, 0x69, 0x74, 0x63, 0x46, 0x63, 0xFB, // 210~219
    0xE6, 0x1F, 0x72, 0xEC, 0x43, 0x5D, 0x50, 0xFF, 0x03, 0x4F, // 220~229
    0x05, 0x33, 0x88, 0x36, 0x93, 0xE4, 0x72, 0xD5, 0xCC, 0x34, // 230~239
    0x52, 0x96, 0x15, 0xCE, 0xD0, 0x32, 0x52, 0x41, 0x4F, 0xBC, // 240~249
    0x2D, 0xDF, 0xC5, 0xD6, 0x7F, 0xD5, 0x74, 0xCE, 0x51, 0xDC, // 250~259
    0x10, 0x5E, 0xF7, 0xAA, 0x4A, 0x2D, 0x20, 0x9A, 0x17, 0xDD, // 260~269
    0x30, 0x89, 0x71, 0x82, 0x36, 0x50, 0x09, 0x1F, 0x7C, 0xF3, // 270~279
    0x12, 0xE9, 0x43, 0x10, 0x5F, 0x51, 0xBF, 0xB8, 0x45, 0xA8, // 280~289
    0x5A, 0x8D, 0x3F, 0x77, 0xE5, 0x96, 0x73, 0x68, 0xAB, 0x73, // 290~299
    0xE5, 0x4C, 0xFB, 0xE5, 0x98, 0xB9, 0xAE, 0x74, 0xEB, 0x51, // 300~309
    0xDB, 0x91, 0x07, 0x7B, 0x66, 0x02, 0x9B, 0x79, 0x03, 0xC5, // 310~319
    0x34, 0x1C, 0x58, 0x13, 0x31, 0xD2, 0x4A, 0xEC, // 320~327
};

static int readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSys, open %s fail.", path);
        return len;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);
    return len;
}

static int writeSys(const char *path, const char *val) {
    int fd, size;

    if ((fd = open(path, O_RDWR)) < 0) {
        SYS_LOGE("writeSys, open %s fail.", path);
        return -1;
    }

    size = strlen(val);
    if (write(fd, val, size) != size) {
        SYS_LOGE("writeSys, write %d size error: %s\n", size, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int writeBufSys(const char *path, const char *buf, const int size) {
    int fd, len = -1;
    if ((fd = open(path, O_WRONLY)) < 0) {
        SYS_LOGE("writeBufSys, open %s fail.", path);
        return len;
    }

    len = write(fd, buf, size);
    close(fd);
    return len;
}

static int saveFile(const char *path, const char *buf, int bufLen) {
    int fd, len = -1;

    if ((fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
        SYS_LOGE("saveFile, open %s error(%s)", path, strerror(errno));
        return len;
    }

    len = write(fd, buf, bufLen);
    close(fd);
    return len;
}

static size_t getFileSize(const char *fpath) {
    struct stat buf;
    if (stat(fpath, &buf) < 0) {
        SYS_LOGE("Can't stat %s : %s\n", fpath, strerror(errno));
        return 0;
    }
    return buf.st_size;
}

HDCPRxKey::HDCPRxKey(int keyType) :
    mKeyType(keyType) {

}

HDCPRxKey::~HDCPRxKey() {

}

void HDCPRxKey::init() {

}

bool HDCPRxKey::refresh() {
    bool ret = false;
    if (HDCP_RX_14_KEY == mKeyType) {
        ret = setHDCP14Key();
        system("/vendor/bin/dec");

        SYS_LOGI("HDCP RX 1.4 key refresh end\n");
    }
    else if (HDCP_RX_22_KEY == mKeyType) {
        ret = setHDCP22Key();
        SYS_LOGI("HDCP RX 2.2 key refresh end\n");
    }

    return ret;
}

int HDCPRxKey::getHdcpRX14key(char *value, int size) {
    int len = 0;
    if (writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1")) {
        SYS_LOGE("getHdcp14key, attach failed!\n");
        return -1;
    }

    if (writeSys(HDCP_RX_KEY_NAME_DEV_PATH, HDCP_RX14_KEY_NAME)) {
        SYS_LOGE("getHdcp14key, name failed!\n");
        return -1;
    }

    len = readSys(HDCP_RX_KEY_READ_DEV_PATH, value, size);
    if (len < HDCP_RX14_KEY_HEAD_SIZE) {
        SYS_LOGE("getHdcp14key, Fail in read (%s) in len %d\n", HDCP_RX14_KEY_NAME, size);
        return -1;
    }

    SYS_LOGD("getHdcp14key success in len %d\n", size);

    return len;
}

int HDCPRxKey::setHdcpRX14key(const char *value, const int size) {
    char keyContentBuf[HDCP_RX14_KEY_CONTENT_SIZE] = { 0 };
    char keyTotalBuf[HDCP_RX14_KEY_TOTAL_SIZE] = {0};
    char sha1_buf1[20] = {0};
    char sha1_buf2[20] = {0};
    char secValue[10] = {0};

    if (writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1")) {
        SYS_LOGE("setHDCP14Key, attach failed!\n");
        return -1;
    }

    if (writeSys(HDCP_RX_KEY_NAME_DEV_PATH, HDCP_RX14_KEY_NAME)) {
        SYS_LOGE("setHDCP14Key, name failed!\n");
        return -1;
    }

    readSys(HDCP_RX_KEY_SECURE_DEV_PATH, (char*)secValue, 10);

    if (!strncmp(secValue, "true", 4)) {
        setHdcpRX14keyMode(HDCP14_KEY_MODE_SECURE);
    }
    else {
        setHdcpRX14keyMode(HDCP14_KEY_MODE_NORMAL);
    }

    if ( size == 348) {
        SYS_LOGE("size: %d bytes\n", size);
        memcpy(keyContentBuf, value, HDCP_RX14_KEY_CONTENT_SIZE);
        memcpy(sha1_buf1, value+HDCP_RX14_KEY_CONTENT_SIZE, 20);
        SHA1((unsigned char *)sha1_buf2, (unsigned char *)keyContentBuf, HDCP_RX14_KEY_CONTENT_SIZE);
        if (strcmp(sha1_buf1, sha1_buf2) != 0)
            SYS_LOGE("sha1sum error \n");
    }
    else if (size == HDCP_RX14_KEY_CONTENT_SIZE) {
        memcpy(keyContentBuf, value, HDCP_RX14_KEY_CONTENT_SIZE);
    }
    else {
        SYS_LOGE("error size. need %d or 348 bytes\n", HDCP_RX14_KEY_CONTENT_SIZE);
        return -1;
    }

    int len = writeBufSys(HDCP_RX_KEY_WRITE_DEV_PATH, keyContentBuf, HDCP_RX14_KEY_CONTENT_SIZE);
    if (HDCP_RX14_KEY_CONTENT_SIZE != len) {
        SYS_LOGE("read key length fail, at least %d bytes, but read len = %d\n", HDCP_RX14_KEY_CONTENT_SIZE, len);
        return -1;
    }

    memcpy(keyTotalBuf, HDCP_RX14_KEY_DEF_HEADER, HDCP_RX14_KEY_HEAD_SIZE);
    memcpy(keyTotalBuf + HDCP_RX14_KEY_HEAD_SIZE, keyContentBuf, HDCP_RX14_KEY_CONTENT_SIZE);
    writeBufSys("/sys/class/hdmirx/hdmirx0/edid", keyTotalBuf, HDCP_RX14_KEY_TOTAL_SIZE);

    return 0;
}

bool HDCPRxKey::setHDCP14Key() {
    char keyContentBuf[HDCP_RX14_KEY_CONTENT_SIZE] = { 0 };
    char keyTotalBuf[HDCP_RX14_KEY_TOTAL_SIZE]     = {0};
    char secValue[10] = {0};

    if (writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1")) {
        SYS_LOGE("setHDCP14Key, attach failed!\n");
        return false;
    }

    if (writeSys(HDCP_RX_KEY_NAME_DEV_PATH, HDCP_RX14_KEY_NAME)) {
        SYS_LOGE("setHDCP14Key, name failed!\n");
        return false;
    }

    readSys(HDCP_RX_KEY_SECURE_DEV_PATH, (char*)secValue, 10);
    SYS_LOGD("Get secure value = %s", secValue);

    if (!strncmp(secValue, "true", 4)) {
        setHdcpRX14keyMode(HDCP14_KEY_MODE_SECURE);
    }
    else {
        int len = readSys(HDCP_RX_KEY_READ_DEV_PATH, keyContentBuf, HDCP_RX14_KEY_CONTENT_SIZE);
        if (HDCP_RX14_KEY_CONTENT_SIZE != len) {
            SYS_LOGE("setHDCP14Key, Fail in read (%s) in len %d\n", HDCP_RX14_KEY_NAME, HDCP_RX14_KEY_CONTENT_SIZE);
            return false;
        }

        memcpy(keyTotalBuf, HDCP_RX14_KEY_DEF_HEADER, HDCP_RX14_KEY_HEAD_SIZE);
        memcpy(keyTotalBuf + HDCP_RX14_KEY_HEAD_SIZE, keyContentBuf, HDCP_RX14_KEY_CONTENT_SIZE);
        writeBufSys("/sys/class/hdmirx/hdmirx0/edid", keyTotalBuf, HDCP_RX14_KEY_TOTAL_SIZE);
        setHdcpRX14keyMode(HDCP14_KEY_MODE_NORMAL);
    }
    return true;
}

int HDCPRxKey::setHdcpRX14keyMode(hdcp14_key_mode_e mode)
{
    int ret = -1;
    int fd, size;
    long long temp = mode;

    SYS_LOGD("%s: HdcpRX14keyMode = %d\n", __FUNCTION__, mode);
    if ((fd = open(HDCP_RX14_KEY_MODE_DEV, O_RDWR)) < 0) {
        SYS_LOGE("%s:open %s fail.", __FUNCTION__, HDCP_RX14_KEY_MODE_DEV);
        return ret;
    }

    ret = ioctl(fd, HDMI_IOC_HDCP14_KEY_MODE, &temp);
    if (ret != 0) {
        SYS_LOGE("ioctl error: %s\n", strerror(errno));
        close(fd);
        return ret;
    }

    close(fd);
    return ret;
}

int HDCPRxKey::getHdcpRX22key(char *value, int size)
{
    int keyLen = 0;
    char existKey[10] = {0};

    writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1");
    writeSys(HDCP_RX_KEY_NAME_DEV_PATH, HDCP_RX22_KEY_NAME);

    readSys(HDCP_RX_KEY_DATA_EXIST, (char*)existKey, 10);
    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        goto _exit;
    }

    keyLen = readSys(HDCP_RX_KEY_READ_DEV_PATH, value, size);
    if (keyLen < HDCP_RX22_KEY_CRC_LEN) {
        SYS_LOGE("read key length fail, at least %d bytes, but read len = %d\n", HDCP_RX22_KEY_CRC_LEN, keyLen);
        goto _exit;
    }

    SYS_LOGD("read success, want %d bytes, but read len = %d\n", size, keyLen);
_exit:
    return keyLen;
}

int HDCPRxKey::setHdcpRX22key(const char *value, const int size)
{
    int ret = -1;
    int keyLen = 0;
    char existKey[10] = {0};

    char keyCrcData[HDCP_RX22_KEY_CRC_LEN] = {0};
    long keyCrcValue = 0;
    char lastCrcData[HDCP_RX22_KEY_CRC_LEN] = {0};
    long lastCrcValue = 0;
    char rpvalue[8] = {0};
    char pSourcePath[60] = {0};
    char pDestPath[60]   = {0};
    char cmd[128] = {0};
    int decryptRetRx = -1;
    int decryptRetRp = -1;

    getRepeaterValue(rpvalue);
    getFirmwareSourcePath(rpvalue, pSourcePath);
    getFirmwareDestPath(rpvalue, pDestPath);

    if (!strcmp(rpvalue, "1")) {
        decryptRetRp = PC_TOOL; //use pc tool to combine firmware
        if (access(HDCP_RX22_SRC_FW_PATH, F_OK) || access(HDCP_RPRX22_SRC_FW_PATH, F_OK)) {
            SYS_LOGE("don't exist path:%s\n", HDCP_RX22_SRC_FW_PATH);
            SYS_LOGE("don't exist path:%s\n", HDCP_RPRX22_SRC_FW_PATH);
            setHdcpRX22SupportStatus();
            goto _exit;
        } else {
            combineFirmwarewithPCTool(HDCP_RPRX22_KEY_NAME, HDCP_RX22_KEY_CRC_PATH, HDCP_RX22_SRC_FW_PATH, HDCP_RX22_DES_FW_PATH);
            combineFirmwarewithPCTool(HDCP_RPRP22_KEY_NAME, HDCP_RP22_KEY_CRC_PATH, HDCP_RPRX22_SRC_FW_PATH, HDCP_RPRX22_DES_FW_PATH);
        }
    }else {
        writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1");
        writeSys(HDCP_RX_KEY_NAME_DEV_PATH, HDCP_RX22_KEY_NAME);

        keyLen = writeBufSys(HDCP_RX_KEY_WRITE_DEV_PATH, value, size);
        if (keyLen != size) {
            SYS_LOGE("write key length fail, we need to write %d bytes, but just write len = %d\n", size, keyLen);
            goto _exit;
        }

        usleep(100*1000);

        readSys(HDCP_RX_KEY_DATA_EXIST, (char*)existKey, 10);
        if (0 == strcmp(existKey, "0")) {
            SYS_LOGE("do not write key to the storage");
            goto _exit;
        }
        SYS_LOGI("hdcp_rx22 write success\n");

        memcpy(keyCrcData, value, HDCP_RX22_KEY_CRC_LEN);
        keyCrcValue = ((keyCrcData[3]<<24)|(keyCrcData[2]<<16)|(keyCrcData[1]<<8)|(keyCrcData[0]&0xff));

        readSys(HDCP_RX22_KEY_CRC_PATH, lastCrcData, HDCP_RX22_KEY_CRC_LEN);
        lastCrcValue = ((lastCrcData[3]<<24)|(lastCrcData[2]<<16)|(lastCrcData[1]<<8)|(lastCrcData[0]&0xff));
        if (access(pDestPath, F_OK) || keyCrcValue != lastCrcValue) {
            SYS_LOGI("HDCP RX 2.2 firmware don't exist or crc different, need create it, last crc value:0x%x, cur crc value:0x%x\n",
                (unsigned int)lastCrcValue, (unsigned int)keyCrcValue);

        //1. unpack random number and key to the files
        decryptRetRx = hdcpKeyUnpack(value, keyLen,
            HDCP_RX22_CFG_AIC_SRC, HDCP_RX22_CFG_AIC_DES, HDCP_RX22_KEY_PATH);
        if (decryptRetRx == -1) {
            SYS_LOGE("unpack hdcp key fail\n");
            setHdcpRX22SupportStatus();
            goto _exit;
        }

        //2. then generate hdcp firmware
        if (decryptRetRx == ARM_TOOL) {
            if (genKeyImg(rpvalue) && esmSwap(rpvalue) && aicTool(rpvalue)) {
                SYS_LOGI("HDCP RX 2.2 firmware generate success, save crc value:0x%x -> %s\n", (unsigned int)keyCrcValue, HDCP_RX22_KEY_CRC_PATH);

                combineFirmwarewithArmTool(HDCP_RX22_SRC_FW_PATH, HDCP_RX22_DES_FW_PATH, HDCP_RX22_OUT_2080_BYTE); //combine firmware RX22

                //remove temporary files
                remove(HDCP_RX22_KEY_PATH);
                remove(HDCP_RX22_CFG_AIC_DES);
                remove(HDCP_RX22_OUT_KEY_IMG);
                remove(HDCP_RX22_OUT_KEY_LE);
                remove(HDCP_RX22_OUT_2080_BYTE);
                ret = 0;
                    //3. generate firmware success, save the key's crc value
                saveFile(HDCP_RX22_KEY_CRC_PATH, keyCrcData, HDCP_RX22_KEY_CRC_LEN);
            }
        } else { // decryptRetRx = PC_TooL
            if (access(HDCP_RX22_SRC_FW_PATH, F_OK)) {
                SYS_LOGE("don't exist path:%s\n", HDCP_RX22_SRC_FW_PATH);
                setHdcpRX22SupportStatus();
                goto _exit;
            } else {
                combineFirmwarewithPCTool(HDCP_RX22_KEY_NAME, HDCP_RX22_KEY_CRC_PATH, HDCP_RX22_SRC_FW_PATH, HDCP_RX22_DES_FW_PATH);
                ret = 0;
            }
        }
        }
        SYS_LOGD("use tool 0 stand for pc tool, 1 stand for arm tool decryptRetRx = %d, decryptRetRp = %d\n", decryptRetRx, decryptRetRp);
    }
    writeBufSys(HDCP_RX_DEBUG_PATH, "load22key", 10);
_exit:
    return ret;
}

bool HDCPRxKey::setHDCP22Key() {
    int ret = false;
    int keyLen = 0;

    char keyCrcData[HDCP_RX22_KEY_CRC_LEN] = {0};
    long keyCrcValue = 0;
    char lastCrcData[HDCP_RX22_KEY_CRC_LEN] = {0};
    long lastCrcValue = 0;
    char rpvalue[8] = {0};
    char pSourcePath[60] = {0};
    char pDestPath[60]   = {0};
    char existKey[10] = {0};
    char cmd[128] = {0};
    int countNum = 0;
    int decryptRetRx = -1;
    int decryptRetRp = -1;
    struct stat st;
    getRepeaterValue(rpvalue);
    getFirmwareSourcePath(rpvalue, pSourcePath);
    getFirmwareDestPath(rpvalue, pDestPath);
    char *pKeyBuf = new char[HDCP_RX22_STORAGE_KEY_SIZE];
    if (!pKeyBuf) {
        SYS_LOGE("Exception: fail to alloc buffer size:%d\n", HDCP_RX22_STORAGE_KEY_SIZE);
        goto _exit;
    }

_reGenetate:
   if (!strcmp(rpvalue, "1")) {
        decryptRetRp = PC_TOOL; //use pc tool to combine firmware
        if (access(HDCP_RX22_SRC_FW_PATH, F_OK) || access(HDCP_RPRX22_SRC_FW_PATH, F_OK)) {
            SYS_LOGE("don't exist path:%s\n", HDCP_RX22_SRC_FW_PATH);
            SYS_LOGE("don't exist path:%s\n", HDCP_RPRX22_SRC_FW_PATH);
            setHdcpRX22SupportStatus();
            goto _exit;
        } else {
            combineFirmwarewithPCTool(HDCP_RPRX22_KEY_NAME, HDCP_RX22_KEY_CRC_PATH, HDCP_RX22_SRC_FW_PATH, HDCP_RX22_DES_FW_PATH);
            combineFirmwarewithPCTool(HDCP_RPRP22_KEY_NAME, HDCP_RP22_KEY_CRC_PATH, HDCP_RPRX22_SRC_FW_PATH, HDCP_RPRX22_DES_FW_PATH);
        }
   } else {
        memset(pKeyBuf, 0, HDCP_RX22_STORAGE_KEY_SIZE);
        //1. unpack random number and key to the files
        writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1");
        writeSys(HDCP_RX_KEY_NAME_DEV_PATH, HDCP_RX22_KEY_NAME);

        readSys(HDCP_RX_KEY_DATA_EXIST, (char*)existKey, 10);
        if (!strcmp(existKey, "0")) {
            SYS_LOGE("do not write key to the secure storage");
            goto _exit;
        }
        SYS_LOGI("hdcp_rx22 write success\n");
            //the first 4 bytes are the crc values
        keyLen = readSys(HDCP_RX_KEY_READ_DEV_PATH, pKeyBuf, HDCP_RX22_STORAGE_KEY_SIZE);
        if (keyLen < HDCP_RX22_KEY_CRC_LEN) {
            SYS_LOGE("read key length fail, at least %d bytes, but read len = %d\n", HDCP_RX22_KEY_CRC_LEN, keyLen);
            goto _exit;
        }

        //for (int i = 0; i < keyLen; i++)
        //    SYS_LOGI("read key [%d] = 0x%x\n", i, pKeyBuf[i]);
        memcpy(keyCrcData, pKeyBuf, HDCP_RX22_KEY_CRC_LEN);
        keyCrcValue = ((keyCrcData[3]<<24)|(keyCrcData[2]<<16)|(keyCrcData[1]<<8)|(keyCrcData[0]&0xff));

        readSys(HDCP_RX22_KEY_CRC_PATH, lastCrcData, HDCP_RX22_KEY_CRC_LEN);
        lastCrcValue = ((lastCrcData[3]<<24)|(lastCrcData[2]<<16)|(lastCrcData[1]<<8)|(lastCrcData[0]&0xff));
        if (access(pDestPath, F_OK) || keyCrcValue != lastCrcValue) {
            SYS_LOGI("HDCP RX 2.2 firmware don't exist or crc different, need create it, last crc value:0x%x, cur crc value:0x%x\n",
                (unsigned int)lastCrcValue, (unsigned int)keyCrcValue);

        decryptRetRx = hdcpKeyUnpack(pKeyBuf, keyLen,
            HDCP_RX22_CFG_AIC_SRC, HDCP_RX22_CFG_AIC_DES, HDCP_RX22_KEY_PATH);
        if (decryptRetRx == -1) {
            SYS_LOGE("unpack hdcp key fail\n");
            goto _exit;
        }

        if (decryptRetRx == ARM_TOOL) {
            //2. then generate hdcp firmware
            if (genKeyImg(rpvalue) && esmSwap(rpvalue) && aicTool(rpvalue)) {
                //3. generate firmware success, save the key's crc value
                SYS_LOGI("HDCP RX 2.2 firmware generate success, save crc value:0x%x -> %s\n", (unsigned int)keyCrcValue, HDCP_RX22_KEY_CRC_PATH);

                combineFirmwarewithArmTool(HDCP_RX22_SRC_FW_PATH, HDCP_RX22_DES_FW_PATH, HDCP_RX22_OUT_2080_BYTE); //combine firmware RX22

                //remove temporary files
                remove(HDCP_RX22_KEY_PATH);
                remove(HDCP_RX22_CFG_AIC_DES);
                remove(HDCP_RX22_OUT_KEY_IMG);
                remove(HDCP_RX22_OUT_KEY_LE);
                remove(HDCP_RX22_OUT_2080_BYTE);
                //3. generate firmware success, save the key's crc value
                saveFile(HDCP_RX22_KEY_CRC_PATH, keyCrcData, HDCP_RX22_KEY_CRC_LEN);
            }
        } else { // decryptRetRx = PC_TOOL
            if (access(HDCP_RX22_SRC_FW_PATH, F_OK)) {
                SYS_LOGE("don't exist path:%s\n", HDCP_RX22_SRC_FW_PATH);
                setHdcpRX22SupportStatus();
                goto _exit;
            } else {
                combineFirmwarewithPCTool(HDCP_RX22_KEY_NAME, HDCP_RX22_KEY_CRC_PATH, HDCP_RX22_SRC_FW_PATH, HDCP_RX22_DES_FW_PATH);
            }
       }
        SYS_LOGD("use tool 0 stand for pc tool, 1 stand for arm tool decryptRetRx = %d, decryptRetRp = %d\n", decryptRetRx, decryptRetRp);
       }
    }

    if (stat(pDestPath, &st) != 0) {
        SYS_LOGE("Error stating file that we just created %s, error:%s", HDCP_RX22_DES_FW_PATH, strerror(errno));
        goto _exit;
    } else {
        //stand for set hdcp22 key and combinefirmware success.
        ret = true;
    }

    if (st.st_size < 100) {
        remove(pDestPath);

        SYS_LOGE("generate firmware.le error, for the %d time", countNum);
        if (countNum < 2) {
            countNum++;
            goto _reGenetate;
        }
        else {
            writeSys(HDMI_RX_HPD_OK_FLAG, "0");
            goto _exit;
        }
    }

    usleep(100*1000);
    #if 0
    if (!strcmp(rpvalue, "1")) {
        sysWrite.setProperty("ctl.start", "hdcp_rp22");
        SYS_LOGI("firmware_rprx.le generated success, start hdcp rp22\n");
        goto _exit;
    } else {
        sysWrite.setProperty("ctl.start", "hdcp_rx22");
        SYS_LOGI("firmware.le generated success, start hdcp rx22\n");
        goto _exit;
    }
    #endif
_exit:
    if (pKeyBuf) delete[] pKeyBuf;
    return ret;
}

bool HDCPRxKey::genKeyImg(const char* rpvalue) {
    char cmd[512] = {0};
    if (access(HDCP_RX22_KEYS_PATH, F_OK)) {
        SYS_LOGE("don't exist path:%s\n", HDCP_RX22_KEYS_PATH);
        setHdcpRX22SupportStatus();
        return false;
    }

    sprintf(cmd, "%s -t 0 %s %s -s 0 -e 1 -o %s",
        HDCP_RX22_TOOL_KEYS, HDCP_RX22_KEY_FORMAT, HDCP_RX22_KEY_PATH, HDCP_RX22_OUT_KEY_IMG);

    SYS_LOGI("hdcpkeys cmd:%s\n", cmd);
    int ret = system (cmd);
    if (0 != ret) {
        SYS_LOGE("exec cmd:%s error\n", cmd);
        return false;
    }

    if (access(HDCP_RX22_OUT_KEY_IMG, F_OK)) {
        SYS_LOGE("generate %s fail \n", HDCP_RX22_OUT_KEY_IMG);
        return false;
    }

    SYS_LOGI("generate %s success \n", HDCP_RX22_OUT_KEY_IMG);
    return true;

}

bool HDCPRxKey::esmSwap(const char* rpvalue) {
    char cmd[512] = {0};
    if (access(HDCP_RX22_ESM_SWAP_PATH, F_OK)) {
        SYS_LOGE("don't exist path:%s\n", HDCP_RX22_ESM_SWAP_PATH);
        setHdcpRX22SupportStatus();
        return false;
    }

    sprintf(cmd, "%s -i %s -o %s -s 1616 -e %s -t 0",
        HDCP_RX22_TOOL_ESM_SWAP, HDCP_RX22_OUT_KEY_IMG, HDCP_RX22_OUT_KEY_LE, HDCP_RX22_CFG_AIC_DES);

    SYS_LOGI("esm swap cmd:%s\n", cmd);
    int ret = system (cmd);
    if (0 != ret) {
        SYS_LOGE("exec cmd:%s error\n", cmd);
        return false;
    }

    if (access(HDCP_RX22_OUT_KEY_LE, F_OK)) {
        SYS_LOGE("generate %s fail \n", HDCP_RX22_OUT_KEY_LE);
        return false;
    }

    SYS_LOGI("generate %s success \n", HDCP_RX22_OUT_KEY_LE);
    return true;
}

bool HDCPRxKey::aicTool(const char* rpvalue) {
    char cmd[512] = {0};
    if (access(HDCP_RX22_AIC_TOOL_PATH, F_OK)) {
        SYS_LOGE("don't exist path:%s\n", HDCP_RX22_AIC_TOOL_PATH);
        setHdcpRX22SupportStatus();
        return false;
    }

    sprintf(cmd, "%s --format=binary-le --dwb-only -o %s -f %s",
        HDCP_RX22_TOOL_AIC, HDCP_RX22_OUT_2080_BYTE, HDCP_RX22_CFG_AIC_DES);

    SYS_LOGI("aic tool cmd:%s\n", cmd);
    int ret = system (cmd);
    if (0 != ret) {
        SYS_LOGE("exec cmd:%s error\n", cmd);
        return false;
    }

    if (access(HDCP_RX22_OUT_2080_BYTE, F_OK)) {
        SYS_LOGE("generate %s fail \n", HDCP_RX22_OUT_2080_BYTE);
        return false;
    }

    SYS_LOGI("generate %s success \n", HDCP_RX22_OUT_2080_BYTE);
    return true;
}

//insert 2080 byte into origin firware, combine a new firware
bool HDCPRxKey::combineFirmwarewithArmTool(const char* pSourcePath, const char* pDestPath, const char* pTempPath) {
    bool ret = false;
    int srcFd = -1;
    int desFd = -1;
    int insertFd = -1;
    char *pSrcData = NULL;
    char *pInsertData = NULL;
    int srcSize = 0;
    int insertSize = 0;
    int writeSize =0;

    if (!pSourcePath || !pDestPath) {
        SYS_LOGE("firmware source path and dest path isn't ready\n");
        return false;
    }
    //read origin firmware.le to buffer
    srcFd = open(pSourcePath, O_RDONLY);

    srcSize = lseek(srcFd, 0, SEEK_END);
    lseek(srcFd, 0, SEEK_SET);
    pSrcData = (char *)malloc(srcSize + 1);
    if (NULL == pSrcData) {
        SYS_LOGE("combine firware, can not malloc source fw:%d memory\n", srcSize);
        goto exit;
    }
    memset((void*)pSrcData, 0, srcSize + 1);
    read(srcFd, (void*)pSrcData, srcSize);
    close(srcFd);
    srcFd = -1;

    //read 2080 bytes to buffer
    insertFd = open(pTempPath, O_RDONLY);
    insertSize = lseek(insertFd, 0, SEEK_END);
    if (2080 != insertSize)
        SYS_LOGE("combine firware, key size is not 2080 bytes\n");
    lseek(insertFd, 0, SEEK_SET);
    pInsertData = (char *)malloc(insertSize + 1);
    if (NULL == pInsertData) {
        SYS_LOGE("combine firware, can not malloc insert:%d memory\n", insertSize);
        goto exit;
    }
    memset((void*)pInsertData, 0, insertSize + 1);
    read(insertFd, (void*)pInsertData, insertSize);
    close(insertFd);
    insertFd = -1;

    //insert 2080 bytes to the origin firmware.le
    memcpy(pSrcData + 0x2800, pInsertData, insertSize);

    if ((desFd = open(pDestPath, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
        SYS_LOGE("combine firware, open %s error(%s)", pDestPath, strerror(errno));
        goto exit;
    }

    //write firmware.le buffer to file
    if ((writeSize = write(desFd, pSrcData, srcSize)) <= 0)
        SYS_LOGE("write firmware.le data error,write size: %d", writeSize);
    close(desFd);
    desFd = -1;

    ret= true;
exit:
    if (srcFd >= 0)
        close(srcFd);
    if (insertFd >= 0)
        close(insertFd);
    if (desFd >= 0)
        close(desFd);

    if (NULL != pSrcData)
        free(pSrcData);
    if (NULL != pInsertData)
        free(pInsertData);
    return ret;
}

//insert 2080 byte into origin firware, combine a new firware
bool HDCPRxKey::combineFirmwarewithPCTool(const char* pKeyName, const char* pCrcName, const char* pSourcePath, const char* pDestPath) {
    bool ret = false;
    int srcFd = -1;
    int desFd = -1;
    char *pSrcData = NULL;
    char *pInsertData = NULL;
    int srcSize = 0;
    int writeSize =0;
    char existKey[10] = {0};
    int keyLen = 0;
    char keyCrcData[HDCP_RX22_KEY_CRC_LEN] = {0};
    long keyCrcValue = 0;
    char lastCrcData[HDCP_RX22_KEY_CRC_LEN] = {0};
    long lastCrcValue = 0;

    if (!pSourcePath || !pDestPath) {
        SYS_LOGE("firmware source path and dest path isn't ready\n");
        return false;
    }

    char *pKeyBuffer = new char[HDCP_RX22_STORAGE_KEY_SIZE];
    if (!pKeyBuffer) {
        SYS_LOGE("Exception: fail to alloc buffer size:%d\n", HDCP_RX22_STORAGE_KEY_SIZE);
        goto exit;
    }
    writeSys(HDCP_RX_KEY_ATTACH_DEV_PATH, "1");
    writeSys(HDCP_RX_KEY_NAME_DEV_PATH, pKeyName);
    readSys(HDCP_RX_KEY_DATA_EXIST, (char*)existKey, 10);
    if (!strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the secure storage");
        goto exit;
    }

    keyLen = readSys(HDCP_RX_KEY_READ_DEV_PATH, pKeyBuffer, HDCP_RX22_STORAGE_KEY_SIZE);
    if (keyLen < HDCP_RX22_KEY_CRC_LEN) {
        SYS_LOGE("read key length fail, at least %d bytes, but read len = %d\n", HDCP_RX22_KEY_CRC_LEN, keyLen);
        goto exit;
    }

    //for (int i = 0; i < keyLen; i++)
        //    SYS_LOGI("read key [%d] = 0x%x\n", i, pKeyBuffer[i]);
    memcpy(keyCrcData, pKeyBuffer, HDCP_RX22_KEY_CRC_LEN);
    keyCrcValue = ((keyCrcData[3]<<24)|(keyCrcData[2]<<16)|(keyCrcData[1]<<8)|(keyCrcData[0]&0xff));

    readSys(pCrcName, lastCrcData, HDCP_RX22_KEY_CRC_LEN);
    lastCrcValue = ((lastCrcData[3]<<24)|(lastCrcData[2]<<16)|(lastCrcData[1]<<8)|(lastCrcData[0]&0xff));

    if (keyCrcValue != lastCrcValue) {
        SYS_LOGI("crc different, need create it, last crc value:0x%x, cur crc value:0x%x\n",
            (unsigned int)lastCrcValue, (unsigned int)keyCrcValue);
        //read origin firmware.le to buffer
        srcFd = open(pSourcePath, O_RDONLY);

        srcSize = lseek(srcFd, 0, SEEK_END);
        lseek(srcFd, 0, SEEK_SET);
        pSrcData = (char *)malloc(srcSize + 1);
        if (NULL == pSrcData) {
            SYS_LOGE("combine firware, can not malloc source fw:%d memory\n", srcSize);
            goto exit;
        }
        memset((void*)pSrcData, 0, srcSize + 1);
        read(srcFd, (void*)pSrcData, srcSize);
        close(srcFd);
        srcFd = -1;

        //read 2080 bytes to buffer
        pInsertData = (char *)malloc(HDCP_RXRP22_2080BYTE_SIZE + 1);
        if (NULL == pInsertData) {
            SYS_LOGE("combine firware, can not malloc insert memory\n");
            goto exit;
        }
        memset((void*)pInsertData, 0, HDCP_RXRP22_2080BYTE_SIZE + 1);

        readSys(HDCP_RX_KEY_READ_DEV_PATH, (char*)pInsertData, HDCP_RXRP22_2080BYTE_SIZE);
        if (!pInsertData) {
            SYS_LOGE("combine firware, can not get insert data\n");
            goto exit;
        }

        //for (int i = 0; i < HDCP_RXRP22_2080BYTE_SIZE; i++)
            //SYS_LOGI("read key [%d] = 0x%x\n", i, pInsertData[i]);

        //insert 2080 bytes to the origin firmware.le
        memcpy(pSrcData + 0x2800, pInsertData, HDCP_RXRP22_2080BYTE_SIZE);

        if ((desFd = open(pDestPath, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
            SYS_LOGE("combine firware, open %s error(%s)", pDestPath, strerror(errno));
            goto exit;
        }

        //write firmware.le buffer to file
        if ((writeSize = write(desFd, pSrcData, srcSize)) <= 0)
            SYS_LOGE("write firmware.le data error,write size: %d", writeSize);
        close(desFd);
        desFd = -1;
        saveFile(pCrcName, keyCrcData, HDCP_RX22_KEY_CRC_LEN);
        ret= true;
    }
exit:
    if (NULL != pKeyBuffer)
        delete[] pKeyBuffer;

    if (srcFd >= 0)
        close(srcFd);
    if (desFd >= 0)
        close(desFd);

    if (NULL != pSrcData)
        free(pSrcData);
    if (NULL != pInsertData)
        free(pInsertData);
    return ret;
}


int HDCPRxKey::setHdcpRX22SupportStatus()
{
    int ret = -1;
    int fd, size;
    if ((fd = open(HDCP_RX14_KEY_MODE_DEV, O_RDWR)) < 0) {
        SYS_LOGE("%s:open %s fail.", __FUNCTION__, HDCP_RX14_KEY_MODE_DEV);
        return ret;
    }

    ret = ioctl(fd, HDMI_IOC_HDCP22_NOT_SUPPORT);
    if (ret != 0) {
        SYS_LOGE("ioctl error: %s\n", strerror(errno));
        close(fd);
        return ret;
    }

    close(fd);
    return ret;
}

void HDCPRxKey::getFirmwareSourcePath(const char* pRpvalue, char* pPath) {
    if (pPath == NULL) {
        SYS_LOGE("Firmware source path memory isn't ready");
        return;
    }

    if (!strcmp(pRpvalue, "1")) {
        strcpy(pPath, HDCP_RPRX22_SRC_FW_PATH);
    } else {
        strcpy(pPath, HDCP_RX22_SRC_FW_PATH);
    }
}

void HDCPRxKey::getFirmwareDestPath(const char* pRpvalue, char* pPath) {
    if (pPath == NULL) {
        SYS_LOGE("Firmware dest path memory isn't ready");
        return;
    }

    if (!strcmp(pRpvalue, "1")) {
        strcpy(pPath, HDCP_RPRX22_DES_FW_PATH);
    } else {
        strcpy(pPath, HDCP_RX22_DES_FW_PATH);
    }
}

void HDCPRxKey::getRepeaterValue(char* pRpvalue) {
    if (pRpvalue != NULL) {
        sysWrite.readSysfs(HDMI_TX_REPEATER_PATH, pRpvalue);
    }
}

int HDCPRxKey::copyHdcpFwToParam(const char* firmwarele, const char* newFw) {
    int iRet = 0;
    FILE* fd_src        = NULL;
    FILE* fd_dest       = NULL;

    //Copy file
    int keyLen = 0;
    int rwLen = 0;
    int wantLen = 0;
    int itemSz = 0;

    SYS_LOGD("generate hdcp tx firmware.le path:%s, des path:%s", firmwarele, newFw);

    size_t filesize = getFileSize(firmwarele);

    char *fwBuf = new char[filesize + 1];
    if (!fwBuf) {
        SYS_LOGE("[%d] Exception: fail to alloc buffer\n", __LINE__);
        return -1;
    }

    fd_src = fopen(firmwarele, "rb");
    if (!fd_src) {
        SYS_LOGE("fail in open file(%s)\n", firmwarele);
        iRet = -1;
        goto _exit4;
    }

    fd_dest = fopen(newFw, "wb");
    if (!fd_dest) {
        SYS_LOGE("fail in open file(%s)\n", newFw);
        iRet = -1;
        goto _exit4;
    }

    do {
        rwLen = fread(fwBuf, 1, filesize, fd_src);
        if (!rwLen) break;
        wantLen = fwrite(fwBuf, 1, rwLen, fd_dest);
        if (wantLen != rwLen) {
            SYS_LOGE("wantLen %d != rwLen %d\n", wantLen, rwLen);
            iRet = -1;
            goto _exit4;
        }
    } while (rwLen);

_exit4:
    if (fwBuf) delete[] fwBuf;
    if (fd_src) fclose(fd_src);
    if (fd_dest) fclose(fd_dest);

    return iRet;
}
