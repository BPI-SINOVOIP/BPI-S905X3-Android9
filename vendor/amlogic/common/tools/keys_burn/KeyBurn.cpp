/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "KeyBurn"
#include "KeyBurn.h"
#include "SystemControlClient.h"
#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
using namespace android;

#define SAVE_PATH        "/data/local/tmp"
#define HDCP_RX_1_4_KEY        "hdcp14_rx"
#define HDCP_RX_2_2_KEY        "hdcprx22"
#define HDCP_TX_1_4_KEY        "hdcp"
#define HDCP_TX_2_2_KEY        "hdcp22_fw_private"
#define PLAYREADY_PUB_KEY        "prpubkeybox"
#define PLAYREADY_PRI_KEY        "prprivkeybox"
#define WIDEVINE_KEY        "widevinekeybox"
#define HDCP_RX_1_4_FILE        "hdcp_rx_key1_4.bin"
#define HDCP_RX_2_2_FILE        "hdcp_rx_key2_2.bin"
#define HDCP_TX_1_4_FILE        "hdcp_tx_key1_4.bin"
#define HDCP_TX_2_2_FILE        "hdcp_rx_key2_2.bin"
#define PLAYREADY_PUB_FILE        "bgroupcert.dat"
#define PLAYREADY_PRI_FILE        "zgpriv.dat"
#define WIDEVINE_FILE        "widevinekeybox.bin"


SystemControlClient* mSystemControlClient;
void dump_mem(char * buffer, int count);
int writeHdcpRX14Key(char * path);
int writeHdcpRX22Key(char * path);
int writeHdcpTX14Key(char * path);
int writeHdcpTX22Key(char * path);
int writePlayreadyKey(char * path);
int writeWidevineKey(char * path);
int writeKeyValue(const char * keyName, char * sourcepath, const char * savepath);
void writeSys(const char *path, const char *val, const int size);


int writeKey(int type, char * path) {
    int ret = 0;
    mSystemControlClient = new SystemControlClient();
    switch (type) {
        case HDCP_RX_1_4:
            ret = writeHdcpRX14Key(path);
            break;
        case HDCP_RX_2_2:
            ret = writeHdcpRX22Key(path);
            break;
        case HDCP_TX_1_4:
            ret = writeHdcpTX14Key(path);
            break;
        case HDCP_TX_2_2:
            ret = writeHdcpTX22Key(path);
            break;
        case PLAYREADY:
            ret = writePlayreadyKey(path);
            break;
        case WIDEVINE:
            ret = writeWidevineKey(path);
            break;
    }
	return ret;
}
void dump_mem(char * buffer, int count)
{
    int i;
    if (NULL == buffer || count == 0)
    {
        ALOGE("%s() %d: %p, %d", __func__, __LINE__, buffer, count);
        return;
    }
    for (i=0; i<count ; i++)
    {
        if (i % 16 == 0)
        printf("\n");
        printf("%02x ", buffer[i]);
    }
    ALOGE("\n");
}
int writeHdcpRX14Key(char * path) {
    int ret = 0;
    char hdcprx14SavePath[256];
    snprintf(hdcprx14SavePath, 256, "%s/%s", SAVE_PATH, HDCP_RX_1_4_FILE);
    ret = writeKeyValue(HDCP_RX_1_4_KEY, path, hdcprx14SavePath);
    return ret;
}
int writeHdcpRX22Key(char * path) {
    int ret = 0;
    char hdcprx22SavePath[256];
    snprintf(hdcprx22SavePath, 256, "%s/%s", SAVE_PATH, HDCP_RX_2_2_FILE);
    ret = writeKeyValue(HDCP_RX_2_2_KEY, path, hdcprx22SavePath);
    return ret;
}
int writeHdcpTX14Key(char * path) {
    int ret = 0;
    char hdcptx14SavePath[256];
    snprintf(hdcptx14SavePath, 256, "%s/%s", SAVE_PATH, HDCP_TX_1_4_FILE);
    ret = writeKeyValue(HDCP_TX_1_4_KEY, path, hdcptx14SavePath);
    return ret;
}
int writeHdcpTX22Key(char * path) {
    int ret = 0;
    char hdcptx22SavePath[256];
    snprintf(hdcptx22SavePath, 256, "%s/%s", SAVE_PATH, HDCP_TX_2_2_FILE);
    ret = writeKeyValue(HDCP_TX_2_2_KEY, path, hdcptx22SavePath);
    return ret;
}
int writePlayreadyKey(char * path) {
    int ret = 0;
    char prpubkeypath[256];
    snprintf(prpubkeypath, 256, "%s/%s", path, PLAYREADY_PUB_FILE);
    char prpubkeySavePath[256];
    snprintf(prpubkeySavePath, 256, "%s/%s", SAVE_PATH, PLAYREADY_PUB_FILE);
    ret = writeKeyValue(PLAYREADY_PUB_KEY, prpubkeypath, prpubkeySavePath);
    if (ret) {
        ALOGE("Fail to burn prpubkey  \n");
        return -1;
    }
    char prprivkeypath[256];
    snprintf(prprivkeypath, 256, "%s/%s", path, PLAYREADY_PRI_FILE);
    char prprivkeySavePath[256];
    snprintf(prprivkeySavePath, 256, "%s/%s", SAVE_PATH, PLAYREADY_PRI_FILE);
    ret = writeKeyValue(PLAYREADY_PRI_KEY, prprivkeypath, prprivkeySavePath);
    if (ret) {
        ALOGE("Fail to burn prprivkey  \n");
        return -1;
    }
    return ret;
}
int writeWidevineKey(char * path) {
    int ret = 0;
    char widevineSavePath[256];
    snprintf(widevineSavePath, 256, "%s/%s", SAVE_PATH, WIDEVINE_FILE);
    ret = writeKeyValue(WIDEVINE_KEY, path, widevineSavePath);
    return ret;
}
int writeKeyValue(const char * keyName, char * sourcepath, const char * savepath) {
    ALOGD("begin to burn %s from %s\n", keyName, sourcepath);
    int fdImg;
    bool ret = false;
    struct stat key;
    if ((fdImg = open(sourcepath, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", sourcepath);
        return -1;
    }
    fstat(fdImg, &key);
    ALOGD("size of %s is %lld\n", sourcepath, key.st_size);
    char *writebuffer = (char *)malloc(key.st_size);
    if (!writebuffer) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = read(fdImg, writebuffer, key.st_size);

    ALOGD("actualReadSz = %d\n", actualReadSz);

    dump_mem(writebuffer, 32);

    if (strcmp(keyName, "hdcprx14") == 0 ) {
        ret = mSystemControlClient->writeHdcpRX14Key(writebuffer, actualReadSz);
    } else if (strcmp(keyName, "hdcprx22") == 0 ) {
        ret = mSystemControlClient->writeHdcpRXImg(sourcepath);
    } else {
        ret = mSystemControlClient->writePlayreadyKey(std::string(keyName), writebuffer, actualReadSz);
    }
    if (!ret) {
        ALOGE("Fail to burn %s \n", keyName);
    } else {
	ALOGD("burn %s success!", keyName);
	writeSys(savepath, writebuffer, actualReadSz);
    }
    free(writebuffer);
    close(fdImg);
    return ret ? 0 : -1;
}

void writeSys(const char *path, const char *val, const int size)
{
    int fd;

    if ((fd = open(path, O_RDWR|O_CREAT, 00600)) < 0) {
        ALOGI("writeSysFs, open %s fail.", path);
        goto exit;
    }

    if (write(fd, val, size) != size) {
        ALOGI("write %s size:%d failed!\n", path, size);
        goto exit;
    }

exit:
    close(fd);
    return;
}
