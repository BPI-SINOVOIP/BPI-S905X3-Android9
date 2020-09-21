/*
 * Copyright (C) 2010 The Android Open Source Project
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
 */

#define LOG_TAG "SystemControlTest"

#include <../ISystemControlService.h>

#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>

#include <android/log.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <../SystemControlClient.h>
#include <string>


using namespace android;

#define UNIFYKEY_NMAE        "hdcp22_fw_private"

#define HDCP_RX_PRIVATE         "hdcp22_rx_private"
#define HDCP_RX                        "hdcp2_rx"
#define HDCP_RX_FW              "extractedKey"

#define IMG_HEAD_SZ                 sizeof(AmlResImgHead_t)
#define ITEM_HEAD_SZ               sizeof(AmlResItemHead_t)
#define ITEM_READ_BUF_SZ (512)

#define UNIFYKEY_ATTACH     "/sys/class/unifykeys/attach"
#define UNIFYKEY_NAME        "/sys/class/unifykeys/name"
#define UNIFYKEY_WRITE      "/sys/class/unifykeys/write"
#define UNIFYKEY_READ        "/sys/class/unifykeys/read"
#define UNIFYKEY_EXIST       "/sys/class/unifykeys/exist"

#define WRITE_SIZE     (200*1024)

#define KEY_UNIFY_NAME_LEN    (48)
/* for ioctrl transfer paramters. */
struct key_item_info_t {
    unsigned int id;
    char name[KEY_UNIFY_NAME_LEN];
    unsigned int size;
    unsigned int permit;
    unsigned int flag;/*bit 0: 1 exsit, 0-none;*/
    unsigned int reserve;
};

#ifndef __HDCP22_HEY_H__
#define __HDCP22_HEY_H__

typedef unsigned int    __u32;
typedef signed int      __s32;
typedef unsigned char   __u8;
typedef signed char     __s8;
typedef __s32 fpi_error;

#define IH_NMLEN     32      /* Image Name Length    */

#define AML_RES_IMG_V1_MAGIC_LEN    8
#define AML_RES_IMG_V1_MAGIC        "AML_HDK!"//8 chars

#pragma pack(push, 1)
typedef struct pack_header {
    unsigned int    totalSz;/* Item Data total Size*/
    unsigned int    dataSz;/* Item Data used  Size*/
    unsigned int    dataOffset;/* Item data offset*/
    unsigned char   type;/* Image Type, not used yet*/
    unsigned char   comp;/* Compression Type*/
    unsigned short  reserv;
    char    name[IH_NMLEN];/* Image Name*/
}AmlResItemHead_t;
#pragma pack(pop)

//typedef for amlogic resource image
#pragma pack(push, 4)
typedef struct {
    __u32   crc;    //crc32 value for the resouces image
    __s32   version;//0x01 means 'AmlResItemHead_t' attach to each item , 0x02 means all 'AmlResItemHead_t' at the head

    __u8    magic[AML_RES_IMG_V1_MAGIC_LEN];  //resources images magic

    __u32   imgSz;  //total image size in byte
    __u32   imgItemNum;//total item packed in the image

}AmlResImgHead_t;
#pragma pack(pop)
#endif

SystemControlClient* mSysClient;

void getSystemControlService()
{
    mSysClient = new SystemControlClient();
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

static unsigned add_sum(const void* pBuf, const unsigned size, unsigned int sum)
{
    const unsigned* data = (const unsigned*)pBuf;
    unsigned wordLen = size>>2;
    unsigned rest = size & 3;

    for (; wordLen/4; wordLen -= 4)
    {
        sum += *data++;
        sum += *data++;
        sum += *data++;
        sum += *data++;
    }
    while (wordLen--)
    {
        sum += *data++;
    }

    if (rest == 0)
    {
        ;
    }
    else if (rest == 1)
    {
        sum += (*data) & 0xff;
    }
    else if (rest == 2)
    {
        sum += (*data) & 0xffff;
    }
    else if (rest == 3)
    {
        sum += (*data) & 0xffffff;
    }

    return sum;
}


//Generate crc32 value with file steam, which from 'offset' to end if checkSz==0
unsigned calc_img_crc(FILE* fp, off_t offset, unsigned checkSz)
{
    unsigned char* buf = NULL;
    unsigned MaxCheckLen = 0;
    unsigned totalLenToCheck = 0;
    const int oneReadSz = 12 * 1024;
    unsigned int crc = 0;

    if (fp == NULL) {
        ALOGE("bad param!!\n");
        return 0;
    }

    buf = (unsigned char*)malloc(oneReadSz);
    if (!buf) {
        ALOGE("Fail in malloc for sz %d\n", oneReadSz);
        return 0;
    }

    fseeko(fp, 0, SEEK_END);
    MaxCheckLen  = ftell(fp);
    MaxCheckLen -= offset;
    if (!checkSz) {
            checkSz = MaxCheckLen;
    }
    else if (checkSz > MaxCheckLen) {
            ALOGE( "checkSz %u > max %u\n", checkSz, MaxCheckLen);
            free(buf);
            return 0;
    }
    fseeko(fp,offset,SEEK_SET);

    while (totalLenToCheck < checkSz)
    {
            int nread;
            unsigned leftLen = checkSz - totalLenToCheck;
            int thisReadSz = leftLen > oneReadSz ? oneReadSz : leftLen;

            nread = fread(buf,1, thisReadSz, fp);
            if (nread < 0) {
                    ALOGE("%d:read %s.\n", __LINE__, strerror(errno));
                    free(buf);
                    return 0;
            }
            crc = add_sum(buf, thisReadSz, crc);

            totalLenToCheck += thisReadSz;
    }

    free(buf);
    return crc;
}

static char generalDataChange(const char input)
{
    int i;
    char result = 0;
    for (i=0; i<8; i++) {
        if ((input & (1<<i)) != 0)
            result |= (1<<(7-i));
         else
            result &= ~(1<<(7-i));
    }
    return result;
}

static void hdcp2DataEncryption(const unsigned len, const char *input, char *out)
{
     int i = 0;

     for (i=0; i<len; i++)
         *out++ = generalDataChange(*input++);
}

static void hdcp2DataDecryption(const unsigned len, const char *input, char *out)
{
     int i = 0;

     for (i=0; i<len; i++)
         *out++ = generalDataChange(*input++);
}

static int write_partiton_raw(const char *partition, const char *data)
{
    std::string writeValue = std::string(data);
    if (mSysClient->writeSysfs(std::string(partition), writeValue)) {
        ALOGI("Write OK!");
        return 0;
    } else {
        ALOGI("Write failed!");
        return -1;
    }
}

static int write_partiton_keyvalue(const char *partition, const char *data, const int size)
{
    if (mSysClient->writeSysfs(std::string(partition), data, size)) {
        ALOGI("Write OK!");
        return 0;
    } else {
        ALOGI("Write failed!");
        return -1;
    }
}

static char *read_partiton_raw(const char *partition)
{
    char *data;
    std::string readValue;
    /*if (mSysClient->readSysfs(std::string(partition), readValue)) {
        data = (char *)String8(readValue).string();
        ALOGI("Read OK!");
        ALOGI("data : %s",data);
        return data;
    } else {
        ALOGI("Read failed!");
        return NULL;
    }*/
    return NULL;
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



static int write_hdcp_key(const char *data, const char *key_name, const int size)
{
    char *status;

    if (write_partiton_raw(UNIFYKEY_ATTACH, "1")) {
        ALOGE("attach failed!\n");
        return -1;
    }

    if (write_partiton_raw(UNIFYKEY_NAME, key_name)) {
        ALOGE("name failed!\n");
        return -1;
    }

    if  (write_partiton_keyvalue(UNIFYKEY_WRITE, data, size) == -1) {
        ALOGE("write failed!\n");
        return -1;
    }

    status = read_partiton_raw(UNIFYKEY_EXIST);

    ALOGI("status : %s",status);

    if (status == NULL)
    {
        ALOGE("read status failed!\n");
        return -1;
    }

    if (strcmp(status, "exist"))
    {
        ALOGE("get status: not burned!\n");
        return -1;
    }

    return 0;
}

void dump_keyitem_info(struct key_item_info_t *info)
{
    if (info == NULL)
        return;
    ALOGE("id: %d\n", info->id);
    ALOGE("name: %s\n", info->name);
    ALOGE("size: %d\n", info->size);
    ALOGE("permit: 0x%x\n", info->permit);
    ALOGE("flag: 0x%x\n", info->flag);
    return;
}

static int res_img_unpack(const char *path)
{
    int ret = 0;
    int num = 0;
    int result = -1;
    FILE* fdImg = NULL;
    unsigned int crc32 = 0;
    AmlResImgHead_t *pImgHead = NULL;
    AmlResItemHead_t *pItemHead = NULL;

    if (path == NULL) {
        ALOGE("Fail path(%s) is null\n", path);
        return -1;
    }

    fdImg = fopen(path, "rb");
    if (!fdImg) {
        ALOGE("Fail to open res image at path %s\n", path);
        return -1;
    }

    char* itemReadBuf = (char*)malloc(ITEM_READ_BUF_SZ);
    if (!itemReadBuf) {
        ALOGE("Fail to malloc buffer at size 0x%x\n", ITEM_READ_BUF_SZ);
        fclose(fdImg);
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = fread(itemReadBuf, 1, IMG_HEAD_SZ, fdImg);
    if (actualReadSz != IMG_HEAD_SZ) {
        ALOGE("Want to read %d, but only read %d\n", IMG_HEAD_SZ, actualReadSz);
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    pImgHead = (AmlResImgHead_t *)itemReadBuf;

    if (strncmp(AML_RES_IMG_V1_MAGIC, (char*)pImgHead->magic, AML_RES_IMG_V1_MAGIC_LEN)) {
        ALOGE("magic error.\n");
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    crc32 = calc_img_crc(fdImg, 4,  pImgHead->imgSz - 4);
    if (pImgHead->crc != crc32) {
        ALOGE("Error when check crc\n");
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    fseek(fdImg, IMG_HEAD_SZ, SEEK_SET);
    int ItemHeadSz = (pImgHead->imgItemNum)*ITEM_HEAD_SZ;
    actualReadSz = fread(itemReadBuf+IMG_HEAD_SZ, 1, ItemHeadSz, fdImg);
    if (actualReadSz != ItemHeadSz) {
        ALOGE("Want to read 0x%x, but only read 0x%x\n", ItemHeadSz, actualReadSz);
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    pItemHead = (AmlResItemHead_t *)(itemReadBuf+IMG_HEAD_SZ);
    for (num=0; num < pImgHead->imgItemNum; num++, pItemHead++)
    {
        ALOGE("pItemHead->name:%s\n", pItemHead->name);
        ALOGE("pItemHead->size:%d\n", pItemHead->dataSz);
        ALOGE("pItemHead->dataOffset:%d\n", pItemHead->dataOffset);

        if (!strcmp(pItemHead->name, HDCP_RX_PRIVATE)) {
            void *tmpbuffer = (void *)malloc(pItemHead->dataSz+4);
            if (!tmpbuffer) {
                ALOGE("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz+4);
                fclose(fdImg);
                free(itemReadBuf);
                return -1;
            }
            char *writebuffer = (char *)malloc(pItemHead->dataSz+4);
            if (!writebuffer) {
                ALOGE("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz+4);
                fclose(fdImg);
                free(itemReadBuf);
                free(tmpbuffer);
                return -1;
            }

            memset(tmpbuffer, 0, pItemHead->dataSz+4);
            memset(writebuffer, 0, pItemHead->dataSz+4);
            fseek(fdImg, pItemHead->dataOffset, SEEK_SET);
            int readlen = fread(tmpbuffer, 1, pItemHead->dataSz, fdImg);
            if (readlen != pItemHead->dataSz) {
                fclose(fdImg);
                free(itemReadBuf);
                free(tmpbuffer);
                free(writebuffer);
                return -1;
            }

            for (int i=0; i< pItemHead->dataSz; i++) {
                ALOGE("tmpbuffer[%d]:%x\n", i,((unsigned char *)tmpbuffer)[i]);
            }

            hdcp2DataDecryption(pItemHead->dataSz, (char *)tmpbuffer, (char *)writebuffer);
            free(tmpbuffer);

            for (int i=0; i< pItemHead->dataSz; i++) {
                ALOGE("writebuffer[%d]:%x\n", i,((unsigned char *)writebuffer)[i]);
            }

            result = write_hdcp_key(writebuffer, HDCP_RX_PRIVATE, pItemHead->dataSz);
            free(writebuffer);
            if (result) {
                ALOGE("write hdcp key failed1!\n");
                free(itemReadBuf);
                fclose(fdImg);
                return -1;
            }
            else
            {
                ALOGE("write hdcp key OK1!\n");
            }
        }else if (!strcmp(pItemHead->name, HDCP_RX)) {
            #if 1
            char *writebuffer = (char *)malloc(pItemHead->dataSz+4);
            if (!writebuffer) {
                ALOGE("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz+4);
                fclose(fdImg);
                free(itemReadBuf);
                return -1;
            }

            memset(writebuffer, 0, pItemHead->dataSz+4);
            fseek(fdImg, pItemHead->dataOffset, SEEK_SET);
            int readlen = fread(writebuffer, 1, pItemHead->dataSz, fdImg);
            if (readlen != pItemHead->dataSz) {
                fclose(fdImg);
                free(itemReadBuf);
                free(writebuffer);
                return -1;
            }

            result = write_hdcp_key(writebuffer, HDCP_RX, pItemHead->dataSz);
            free(writebuffer);
            if (result) {
                ALOGE("write hdcp key failed2!\n");
                free(itemReadBuf);
                fclose(fdImg);
                return -1;
            }
            #endif
        } else if (!strcmp(pItemHead->name, HDCP_RX_FW)) {
            #if 1
            char *writebuffer = (char *)malloc(pItemHead->dataSz+4);
            if (!writebuffer) {
                ALOGE("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz+4);
                fclose(fdImg);
                free(itemReadBuf);
                return -1;
            }

            memset(writebuffer, 0, pItemHead->dataSz+4);
            fseek(fdImg, pItemHead->dataOffset, SEEK_SET);
            int readlen = fread(writebuffer, 1, pItemHead->dataSz, fdImg);
            if (readlen != pItemHead->dataSz) {
                fclose(fdImg);
                free(itemReadBuf);
                free(writebuffer);
                return -1;
            }

            result = write_hdcp_key(writebuffer, "hdcp22_rx_fw", pItemHead->dataSz);
            writeSys("/mnt/vendor/param/test-write-bin_w.txt", writebuffer, pItemHead->dataSz);

            char *data = (char *)malloc(pItemHead->dataSz);
            if (!data) {
                ALOGE("Fail to malloc buffer  \n");
                return -1;
            }
            memset(data, 0, pItemHead->dataSz);
            ALOGE("readHdcpRX22Key data:%p\n", (void *)data);
            /*int len = mSysClient->readHdcpRX22Key(data, pItemHead->dataSz);
            ALOGE("readHdcpRX22Key want to read %d, actually read %d\n", pItemHead->dataSz, len);
            dump_mem(data, len);
            writeSys("/mnt/vendor/param/test-write-bin_r.txt", data, pItemHead->dataSz);*/

            free(writebuffer);
            free(data);
            if (result) {
                ALOGE("write hdcp key failed3!\n");
                free(itemReadBuf);
                fclose(fdImg);
                return -1;
            }
            #endif
        }
    }

    fclose(fdImg);
    free(itemReadBuf);
    return result;
}

static int read_write_test14(const char *path14)
{
    int fdImg;
    struct stat hdcprx14;
    if ((fdImg = open(path14, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", path14);
        return -1;
    }
    fstat(fdImg, &hdcprx14);
    ALOGE("size of %s is %d\n", path14, hdcprx14.st_size);
    char *writebuffer = (char *)malloc(hdcprx14.st_size);
    if (!writebuffer) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = read(fdImg, writebuffer, hdcprx14.st_size);

    ALOGE("actualReadSz = %d\n", actualReadSz);
    printf("*********************************************\n");
    dump_mem(writebuffer, 32);

    mSysClient->writeHdcpRX14Key(writebuffer, actualReadSz);
    writeSys("/tee/read_write_test14_w.txt", writebuffer, actualReadSz);

    char *data2 = (char *)malloc(512);
    if (!data2) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    memset(data2, 0, 512);
    int len2 = mSysClient->readHdcpRX14Key(data2, 512);
    writeSys("/tee/read_write_test14_r.txt", data2, len2);

    printf("*********************************************\n");
    dump_mem(data2, 32);


    free(writebuffer);
    free(data2);
    close(fdImg);
    return 0;
}

static int read_write_test22(const char *path22)
{
    int fdImg;
    struct stat hdcprx22;
    if ((fdImg = open(path22, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", path22);
        return -1;
    }
    fstat(fdImg, &hdcprx22);
    ALOGE("size of %s is %d\n", path22, hdcprx22.st_size);
    char *writebuffer = (char *)malloc(hdcprx22.st_size);
    if (!writebuffer) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = read(fdImg, writebuffer, hdcprx22.st_size);

    ALOGE("actualReadSz = %d\n", actualReadSz);

    printf("*********************************************\n");
    dump_mem(writebuffer, 32);

    mSysClient->writeHdcpRX22Key(writebuffer, actualReadSz);
    writeSys("/tee/read_write_test22_w.txt", writebuffer, actualReadSz);

    char *data2 = (char *)malloc(4096);
    if (!data2) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    memset(data2, 0, 4096);
    int len2 = mSysClient->readHdcpRX22Key(data2, 4096);
    writeSys("/tee/read_write_test22_r.txt", data2, len2);

    printf("*********************************************\n");
    dump_mem(data2, 32);

    free(writebuffer);
    free(data2);
    close(fdImg);
    return 0;
}

static int read_write_test_atte(const char *path)
{
    int fdImg;
    struct stat atte;
    if ((fdImg = open(path, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", path);
        return -1;
    }
    fstat(fdImg, &atte);
    ALOGE("size of %s is %lld\n", path, atte.st_size);
    char *writebuffer = (char *)malloc(atte.st_size);
    if (!writebuffer) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = read(fdImg, writebuffer, atte.st_size);

    ALOGE("actualReadSz = %d\n", actualReadSz);

    dump_mem(writebuffer, 32);

    mSysClient->writeAttestationKey(std::string("/dev/unifykeys"), std::string("usid"), writebuffer, actualReadSz);

    //writeSys("/tee/read_write_test22_w.txt", writebuffer, actualReadSz);

    printf("*********************************************\n");

    char *data2 = (char *)malloc(10240);
    if (!data2) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    memset(data2, 0, 10240);

    int len = mSysClient->readAttestationKey(std::string("/dev/unifykeys"), std::string("usid"), data2, 10240);
    ALOGE("len = %d\n", len);

    dump_mem(data2, 64);

    free(writebuffer);
    //free(data2);
    close(fdImg);
    return 0;
}

static int read_write_bin(const char *path)
{
    int fdImg;
    struct stat hdcprx;
    if ((fdImg = open(path, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", path);
        return -1;
    }
    fstat(fdImg, &hdcprx);
    ALOGE("size of %s is %d\n", path, hdcprx.st_size);
    char *writebuffer = (char *)malloc(hdcprx.st_size);
    if (!writebuffer) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = read(fdImg, writebuffer, hdcprx.st_size);

    ALOGE("actualReadSz = %d\n", actualReadSz);
    printf("*********************************************\n");
    dump_mem(writebuffer, 32);

    mSysClient->writeSysfs(std::string("/sys/class/unifykeys/attach"), std::string("1"));
    mSysClient->writeSysfs(std::string("/sys/class/unifykeys/name"), std::string("usid"));
    mSysClient->writeSysfs(std::string("/sys/class/unifykeys/write"), writebuffer, actualReadSz);

    return 0;
}

static int read_write_test(const char *path14, const char *path22)
{
    int fdImg;
    struct stat hdcprx14;
    if ((fdImg = open(path14, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", path14);
        return -1;
    }
    fstat(fdImg, &hdcprx14);
    ALOGE("size of %s is %d\n", path14, hdcprx14.st_size);
    char *writebuffer = (char *)malloc(hdcprx14.st_size);
    if (!writebuffer) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = read(fdImg, writebuffer, hdcprx14.st_size);

    char *data = (char *)malloc(hdcprx14.st_size);
    if (!data) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    memset(data, 0, hdcprx14.st_size);
    int len1 = mSysClient->readHdcpRX14Key(data, hdcprx14.st_size);
    writeSys("/tee/hdcp14_1_r.txt", data, len1);

    mSysClient->writeHdcpRX14Key(writebuffer, actualReadSz);
    writeSys("/tee/hdcp14_2_w.txt", writebuffer, actualReadSz);

    char *data2 = (char *)malloc(328);
    if (!data2) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    memset(data2, 0, 328);
    int len2 = mSysClient->readHdcpRX14Key(data2, 328);
    writeSys("/tee/hdcp14_2_r.txt", data2, len2);

    int fdImg2;
    struct stat hdcprx22;
    if ((fdImg2 = open(path22, O_RDONLY)) < 0) {
        ALOGE("Fail to open res image at path %s\n", path22);
        return -1;
    }
    fstat(fdImg2, &hdcprx22);
    ALOGE("size of %s is %d\n", path22, hdcprx22.st_size);
    char *writebuffer4 = (char *)malloc(hdcprx22.st_size);
    if (!writebuffer4) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    int actualReadSz4 = 0;
    actualReadSz4 = read(fdImg2, writebuffer4, hdcprx22.st_size);
    mSysClient->writeHdcpRX22Key(writebuffer4, actualReadSz4);
    writeSys("/tee/hdcp22_4_w.txt", writebuffer4, actualReadSz4);

    char *data4 = (char *)malloc(4096);
    if (!data4) {
        ALOGE("Fail to malloc buffer  \n");
        return -1;
    }
    memset(data4, 0, 4096);
    int len4 = mSysClient->readHdcpRX22Key(data4, 4096);
    ALOGE("readHdcpRX22Key want to read %d, actually read %d\n", 4096, len4);
    writeSys("/tee/hdcp22_4_r.txt", data4, len4);

    free(writebuffer);
    free(data);
    free(data2);
    free(writebuffer4);
    free(data4);
    close(fdImg);
    close(fdImg2);
    return 0;
}

int main(int argc __unused, char** argv __unused)
{
    getSystemControlService();

    ALOGI("argc: %d\n", argc);

    for (int i=0;i<argc;i++)
        ALOGI("argv: %s\n", argv[i]);

    /*if (strcmp(argv[1], "test-write-bin") == 0)
        res_img_unpack(argv[2]);
    else if (strcmp(argv[1], "test-write22-img") == 0) {
        mSysClient->writeHdcpRXImg(std::string(argv[2]));
        char *data4 = (char *)malloc(4096);
        if (!data4) {
            ALOGE("Fail to malloc buffer  \n");
            return -1;
        }
        memset(data4, 0, 4096);
        int len4 = mSysClient->readHdcpRX22Key(data4, 4096);
        ALOGE("readHdcpRX22Key want to read %d, actually read %d\n", 4096, len4);
        writeSys("/mnt/vendor/param/test-write22-img.txt", data4, len4);
        free(data4);
    }
    else */
    if (strcmp(argv[1], "test-RDWR-Atte") == 0)
        read_write_test_atte(argv[2]);
    else if (strcmp(argv[1], "test-RDWR-hdcp14") == 0)
        read_write_test14(argv[2]);
    else if (strcmp(argv[1], "test-RDWR-hdcp22") == 0)
        read_write_test22(argv[2]);
    else if (strcmp(argv[1], "test-WR-bin") == 0)
        read_write_bin(argv[2]);
    return 0;
}
