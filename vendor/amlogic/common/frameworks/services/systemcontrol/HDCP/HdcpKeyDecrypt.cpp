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
 *  @date     2016/5/17
 *  @par function description:
 *  - 1 decrypt key from storage, generate 2 files
 *  - 2 one is random number, another is key file
 */

#define LOG_TAG "SystemControl"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "common.h"

#include "HdcpKeyDecrypt.h"
//#include "aes.h"
#include "HDCPRxKey.h"

#define KEY_MAX_SIZE   (1024 * 2)
#define AES_KEY_BIT 128
#define AES_IV_BIT 128

const unsigned char default_fixed_aeskey[] = {
    'h',    'e',    'l',    'l',    'o',    '!' ,
    'a' ,   'm' ,   'l' ,   'o' ,   'g' ,   'i' ,
    'c' ,   '!' ,   'y',    'l',
};
unsigned char default_fixed_iv[] = {
    'j' ,   'i' ,   'a' ,   'n' ,   'g' ,   'z' ,
    'e' ,   'm' ,   'i' ,   'n' ,   'b' ,   't' ,
    '.' ,   'c' ,   'o' ,   'm' ,
};


unsigned addSum(const void* pBuf, const unsigned size)
{
    unsigned sum		 =	0;
    const unsigned* data = (const unsigned*)pBuf;
    unsigned wordLen 	 = size>>2;
    unsigned rest 		 = size & 3;

    for (; wordLen/4; wordLen -= 4) {
        sum += *data++;
        sum += *data++;
        sum += *data++;
        sum += *data++;
    }

    while (wordLen--) {
        sum += *data++;
    }

    if (rest == 0) {
        ;
    }
    else if (rest == 1) {
        sum += (*data) & 0xff;
    }
    else if (rest == 2) {
        sum += (*data) & 0xffff;
    }
    else if (rest == 3) {
        sum += (*data) & 0xffffff;
    }

    return sum;
}

int do_aes(bool isEncrypt, unsigned char* pIn, int nInLen, unsigned char* pOut, int* pOutLen)
{
    int nRet = -1;
    unsigned char* data = pIn;
    const int dataLen         = ( ( nInLen + 0xf ) >> 4 ) << 4;
    unsigned char* transferBuf = NULL;

    if (!pIn || !nInLen || !pOut || !pOutLen) {
        //_MSG_BOX_ERR(_T("arg nul"));
        return -__LINE__;
    }
    if ( nInLen & 0xf ) {
        transferBuf = new unsigned char [dataLen];
        data = transferBuf;
    }

#if 0 // avoid GPL lisence
    struct aes_context ctx;
    unsigned char iv[16];
    //memset(iv, 0, sizeof(iv));
    memcpy(iv, default_fixed_iv, 16);

    unsigned char key[AES_KEY_BIT/8];
    memcpy(key, default_fixed_aeskey, sizeof(default_fixed_aeskey));

    if (isEncrypt) {
        aes_setkey_enc(&ctx, key, AES_KEY_BIT);
    }
    else {
        aes_setkey_dec(&ctx, key, AES_KEY_BIT);
    }

    nRet = aes_crypt_cbc(&ctx, isEncrypt, dataLen, iv, pIn, pOut);
#endif
    *pOutLen = dataLen;
    if (transferBuf) delete[] transferBuf;
    return nRet;
}

/*
 * inBuf: include random number and hdcp key
 * */
int hdcpKeyUnpack(const char* inBuf, int inBufLen,
    const char *srcAicPath, const char *desAicPath, const char *keyPath)
{
    AmlResImgHead_t *packedImgHead = NULL;
    AmlResItemHead_t *packedImgItem = NULL;
    unsigned gensum = 0;
    int i = 0;

    packedImgHead = (AmlResImgHead_t*)inBuf;
    packedImgItem = (AmlResItemHead_t*)(packedImgHead + 1);

    if (strncmp(AML_RES_IMG_V1_MAGIC, (char*)packedImgHead->magic, AML_RES_IMG_V1_MAGIC_LEN)) {
        SYS_LOGE("magic error!!!\n");
        return PC_TOOL;
    }

    gensum = addSum(inBuf + 4, inBufLen - 4);
    if (packedImgHead->crc != gensum) {
        SYS_LOGE("crc chcked failed, origsum[%8x] != gensum[%8x]\n", packedImgHead->crc, gensum);
        return PC_TOOL;
    }

    if ( inBufLen > KEY_MAX_SIZE ) {
        SYS_LOGE("key size %d > max(%d)\n", inBufLen, KEY_MAX_SIZE);
        return -1;
    }

    if (access(srcAicPath, F_OK)) {
        SYS_LOGE("do not exist path:%s\n", srcAicPath);
        return -1;
    }

    for (i = 0; i < (int)packedImgHead->imgItemNum; ++i) {
        const AmlResItemHead_t* pItem = packedImgItem + i;
        const char* itemName = pItem->name;
    #if 1
        int itemSz = 0;
        unsigned char itembuf[KEY_MAX_SIZE] = {0};
        do_aes(false, (unsigned char *)(inBuf + pItem->dataOffset), pItem->dataSz, itembuf, &itemSz);
    #else
        char *itembuf = (char*)inBuf + pItem->dataOffset;
        const int itemSz = pItem->dataSz;
    #endif
        //this item is random number
        if (!strcmp(itemName, "firmware")) {
            int desFd;
            if ((desFd = open(desAicPath, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
                SYS_LOGE("unpack dhcp key, open %s error(%s)", desAicPath, strerror(errno));
                return -1;
            }
            //write random number to destination aic file
            write(desFd, itembuf, itemSz);

            //origin firmware.aic append the end
            int srcFd = open(srcAicPath, O_RDONLY);
            int srcSize = lseek(srcFd, 0, SEEK_END);
            lseek(srcFd, 0, SEEK_SET);
            char *pSrcData = (char *)malloc(srcSize + 1);
            if (NULL == pSrcData) {
                SYS_LOGE("unpack dhcp key, can not malloc:%d memory\n", srcSize);
                close(desFd);
                close(srcFd);
                return -1;
            }
            memset((void*)pSrcData, 0, srcSize + 1);
            read(srcFd, (void*)pSrcData, srcSize);
            //write source aic file data to destination aic file
            write(desFd, pSrcData, srcSize);

            close(srcFd);
            free(pSrcData);
            close(desFd);

            SYS_LOGI("unpack dhcp key, write random number -> (%s) done\n", desAicPath);
        }
        //this item is hdcp rx 22 key
        else if (!strcmp(itemName, "hdcp2_rx")) {
            int keyFd;
            if ((keyFd = open(keyPath, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
                SYS_LOGE("unpack dhcp key, open %s error(%s)", keyPath, strerror(errno));
                return -1;
            }

            //write key to file
            write(keyFd, itembuf, itemSz);
            close(keyFd);

            SYS_LOGI("unpack dhcp key, write key -> (%s) done\n", keyPath);
        }
    }

    return ARM_TOOL;
}
