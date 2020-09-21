/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <string.h>
#include <android-base/properties.h>
#include <ziparchive/zip_archive.h>
#include <android-base/logging.h>

#include "common.h"
#include "cutils/properties.h"
#include "security.h"

#define CMD_SECURE_CHECK _IO('d', 0x01)
#define CMD_DECRYPT_DTB  _IO('d', 0x02)

T_KernelVersion kernel_ver = KernelV_3_10;
int SetDtbEncryptFlag(const char *flag) {
    int len = 0;
    int fd = open(DECRYPT_DTB, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DECRYPT_DTB);
        return -1;
    }

    len = write(fd, flag, 1);
    if (len != 1) {
        printf("write %s failed!\n", DECRYPT_DTB);
        close(fd);
        fd = -1;
        return -1;
    }

    close(fd);

    return 0;
}

int SetDtbEncryptFlagByIoctl(const char *flag) {
    int ret = -1;
    unsigned long operation = 0;
    if (!strcmp(flag, "1") ) {
        operation = 1;
    } else {
        operation = 0;
    }

    int fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DEFEND_KEY);
        return -1;
    }

    ret = ioctl(fd, CMD_DECRYPT_DTB, &operation);
    close(fd);

    return ret;
}

int GetSecureBootImageSize(const unsigned char *buffer, const int size) {

    int len = size;
    int encrypt_size = size;
    unsigned char *pstart = (unsigned char *)buffer;

    //"avbtool"
    const unsigned char avbtool[] = { 0x61, 0x76, 0x62, 0x74, 0x6F, 0x6F, 0x6C };

    while (len > 8) {
        if (!memcmp(pstart, avbtool, 7)) {
            printf("find avb0 flag,already enable avb function !\n");
            //encrypt image offset 148 bytes
            pstart += 148;
            //get encrypt image size
            encrypt_size = (pstart[0] << 24) + (pstart[1] << 16) + (pstart[2] << 8) + pstart[3];
            printf("encrypt_size = %d\n", encrypt_size);
            break;
        }
        pstart += 8;
        len -= 8;
    }

    return encrypt_size;
}


/**
  *  --- judge platform whether match with zip image or not
  *
  *  @platformEncryptStatus: 0: platform unencrypted, 1: platform encrypted
  *  @imageEncryptStatus:    0: image unencrypted, 1: image encrypted
  *  @imageName:   image name
  *  @imageBuffer: image data address
  *  @imageSize:   image data size
  *
  *  return value:
  *  <0: failed
  *  =0: not match
  *  >0: match
  */
static int IsPlatformMachWithZipArchiveImage(
        const int platformEncryptStatus,
        const int imageEncryptStatus,
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize)
{
    int fd = -1, ret = -1;
    ssize_t result = -1;
    int encrypt_size = 0;

    if (strcmp(imageName, BOOT_IMG) &&
        strcmp(imageName, RECOVERY_IMG) &&
        strcmp(imageName, BOOTLOADER_IMG)) {
        printf("can't support %s at present\n",
            imageName);
        return -1;
    }

    if (imageBuffer == NULL) {
        printf("havn't malloc space for %s\n",
            imageName);
        return -1;
    }

    if (imageSize <= 0) {
        printf("%s size is %d\n",
            imageName, imageSize);
        return -1;
    }

    switch (platformEncryptStatus) {
        case 0: {
            if (!imageEncryptStatus) {
                ret = 1;
            } else {
                ret = 0;
            }
            break;
        }

        case 1: {
            if (!imageEncryptStatus) {
                ret = 0;
            } else {
                fd = open(DEFEND_KEY, O_RDWR);
                if (fd < 0) {
                    printf("open %s failed (%s)\n",
                        DEFEND_KEY, strerror(errno));
                    return -1;
                }

                //if enable avb, need get the real encrypt data size
                encrypt_size = GetSecureBootImageSize(imageBuffer, imageSize);

                result = write(fd, imageBuffer, encrypt_size);// check rsa
                printf("write %s datas to %s. [imgsize:%d, result:%d, %s]\n",
                    imageName, DEFEND_KEY, imageSize, result,
                    (result == 1) ? "match" :
                    (result == -2) ? "not match" : "failed or not support");
                if (result == 1) {
                    ret = 1;
                } else if(result == -2) {
                    ret = 0;
                } else {    // failed or not support
                    ret = -1;
                }
                close(fd);
                fd = -1;
            }
            break;
        }
    }

    return ret;
}

/**
  *  --- check bootloader.img whether encrypt or not
  *
  *  @imageName:   bootloader.img
  *  @imageBuffer: bootloader.img data address
  *
  *  return value:
  *  <0: failed
  *  =0: unencrypted
  *  >0: encrypted
  */
static int IsBootloaderImageEncrypted(
        const char *imageName,
        const unsigned char *imageBuffer)
{
    int step0=1;
    int step1=0;
    int index=0;
    const unsigned char *pstart = NULL;
    const unsigned char *pImageAddr = imageBuffer;
    const unsigned char *pEncryptedBootloaderInfoBufAddr = NULL;

    // Don't modify. unencrypt bootloader info, for kernel version 3.10
    const int bootloaderEncryptInfoOffset = 0x1b0;
    const unsigned char unencryptedBootloaderInfoBuf[] =
            { 0x4D, 0x33, 0x48, 0x48, 0x52, 0x45, 0x56, 0x30 };

    // Don't modify. unencrypt bootloader info, for kernel version 3.14
    const int newbootloaderEncryptInfoOffset = 0x10;
    const int newbootloaderEncryptInfoOffset1 = 0x70;
    const unsigned char newunencryptedBootloaderInfoBuf[] = { 0x40, 0x41, 0x4D, 0x4C};

    if (strcmp(imageName, BOOTLOADER_IMG)) {
        printf("this image must be %s,but it is %s\n",
            BOOTLOADER_IMG, imageName);
        return -1;
    }

    if (imageBuffer == NULL) {
        printf("havn't malloc space for %s\n",
            imageName);
        return -1;
    }

    if (kernel_ver == KernelV_3_10) {
        //check image whether encrypted for kernel 3.10
        pEncryptedBootloaderInfoBufAddr = pImageAddr + bootloaderEncryptInfoOffset;
        if (!memcmp(unencryptedBootloaderInfoBuf, pEncryptedBootloaderInfoBufAddr,
            ARRAY_SIZE(unencryptedBootloaderInfoBuf))) {
            return 0;   // unencrypted
        } else {
            return 1;
        }
    }

    //check image whether encrypted for kernel 3.14
    pEncryptedBootloaderInfoBufAddr = pImageAddr + newbootloaderEncryptInfoOffset;
    if (!memcmp(newunencryptedBootloaderInfoBuf, pEncryptedBootloaderInfoBufAddr,
        ARRAY_SIZE(newunencryptedBootloaderInfoBuf))) {
        step0 = 0;
    }

    pstart = pImageAddr + newbootloaderEncryptInfoOffset1;
    for (index=0;index<16;index++) {
        if (pstart[index] != 0) {
            step1 = 1;
            break;
        }
    }

    if ((step0 == 1) && (step1 == 1)) {
        return 1;  // encrypted
    }

    return 0;//unencrypted
}

/*  return value:
  *  <0: failed
  *  =0: not match
  *  >0: match
  */
int DtbImgEncrypted(
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize,
        const char *flag,
        unsigned char *encryptedbuf)
{
    int len = 0;
    ssize_t result = -1;
    ssize_t readlen = -1;
    int fd = -1, ret = -1;

     if ((imageBuffer == NULL) || (imageName == NULL)) {
        printf("imageBuffer is null!\n");
        return -1;
     }

    if (access(DECRYPT_DTB, F_OK) ||access(DEFEND_KEY, F_OK)) {
        printf("doesn't support dtb secure check\n");
        return 2;   // kernel doesn't support
    }

    ret = SetDtbEncryptFlag(flag);
    if (ret < 0) {
        printf("set dtb encrypt flag by %s failed!, try ioctl\n", DECRYPT_DTB);
        ret = SetDtbEncryptFlagByIoctl(flag);
        if (ret < 0) {
            printf("set dtb encrypt flag by ioctl failed!\n");
            return -1;
        }
    }

    fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed (%s)\n",DEFEND_KEY, strerror(errno));
        return -1;
    }

    result = write(fd, imageBuffer, imageSize);// check rsa
    printf("write %s datas to %s. [imgsize:%d, result:%d, %s]\n",
        imageName, DEFEND_KEY, imageSize, result,
        (result == 1) ? "match" :
        (result == -2) ? "not match" : "failed or not support");

    if (!strcmp(flag, "1")) {
        printf("dtb.img need to encrypted!\n");
        readlen = read(fd, encryptedbuf, imageSize);
        if (readlen  < 0) {
            printf("read %s error!\n", DEFEND_KEY);
            close(fd);
            return -1;
        }

    }

    if (result == 1) {
        ret = 1;
    } else if(result == -2) {
        ret = 0;
    } else {    // failed or not support
        ret = -1;
    }

    close(fd);
    fd = -1;

    return ret;
}

/**
  *  --- check zip archive image whether encrypt or not
  *   image is bootloader.img/boot.img/recovery.img
  *
  *  @imageName:   image name
  *  @imageBuffer: image data address
  *  @imageSize:   image data size
  *
  *  return value:
  *  <0: failed
  *  =0: unencrypted
  *  >0: encrypted
  */
static int IsZipArchiveImageEncrypted(
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize)
{
    int ret = -1;
    const unsigned char *pImageAddr = imageBuffer;

    if (strcmp(imageName, BOOT_IMG) &&
        strcmp(imageName, RECOVERY_IMG) &&
        strcmp(imageName, BOOTLOADER_IMG)) {
        printf("can't support %s at present\n",
            imageName);
        return -1;
    }

    if (imageBuffer == NULL) {
        printf("havn't malloc space for %s\n",
            imageName);
        return -1;
    }

    if (imageSize <= 0) {
        printf("%s size is %d\n",
            imageName, imageSize);
        return -1;
    }

    if (!strcmp(imageName, BOOTLOADER_IMG)) {
        return IsBootloaderImageEncrypted(imageName, imageBuffer);
    }

    if (kernel_ver == KernelV_3_10) {
        //check image whether encrypted for kernel 3.10
        const pT_SecureBootImgHdr encryptSecureBootImgHdr =
            (const pT_SecureBootImgHdr)pImageAddr;
        const pT_EncryptBootImgInfo encryptBootImgInfo =
            &encryptSecureBootImgHdr->encryptBootImgInfo;

        secureDbg("magic:%s, version:0x%04x, totalLenAfterEncrypted:0x%0x\n",
            encryptBootImgInfo->magic, encryptBootImgInfo->version,
            encryptBootImgInfo->totalLenAfterEncrypted);

        ret = memcmp(encryptBootImgInfo->magic, SECUREBOOT_MAGIC,
            strlen(SECUREBOOT_MAGIC));
        if (!ret && encryptBootImgInfo->version != 0x0) {
            return 1;   // encrypted
        }

        return 0;
    }

    //check image whether encrypted for kernel 3.14
    AmlSecureBootImgHeader encryptSecureBootImgHeader =
        (const AmlSecureBootImgHeader)pImageAddr;
    p_AmlEncryptBootImgInfo encryptBootImgHeader =
        &encryptSecureBootImgHeader->encrypteImgInfo;

    secureDbg("magic:%s, version:0x%04x\n",
        encryptBootImgHeader->magic, encryptBootImgHeader->version);

    ret = memcmp(encryptBootImgHeader->magic, SECUREBOOT_MAGIC,
        strlen(SECUREBOOT_MAGIC));

    //for android P, 2048 offset
    if (ret) {
        encryptSecureBootImgHeader = (AmlSecureBootImgHeader)(pImageAddr+1024);
        encryptBootImgHeader = &encryptSecureBootImgHeader->encrypteImgInfo;
        ret = memcmp(encryptBootImgHeader->magic, SECUREBOOT_MAGIC, strlen(SECUREBOOT_MAGIC));
    }

    if (!ret && encryptBootImgHeader->version != 0x0) {
        return 1;   // encrypted
    }

    //for v3 secureboot image encrypted
    if (ret) {
        const unsigned char V3SecureBootInfoBuf[] = { 0x40, 0x41, 0x4D, 0x4C};
        if (!memcmp(V3SecureBootInfoBuf, pImageAddr, ARRAY_SIZE(V3SecureBootInfoBuf))) {
            return 1;   // encrypted
        }

        //find offset 2048 byte
        if (!memcmp(V3SecureBootInfoBuf, pImageAddr+2048, ARRAY_SIZE(V3SecureBootInfoBuf))) {
            return 1;   // encrypted
        }
    }

    return 0;       // unencrypted
 }

int IsPlatformEncryptedByIoctl(void) {
    int ret = -1;
    unsigned int operation = 0;

    if (access(DEFEND_KEY, F_OK)) {
        printf("kernel doesn't support secure check\n");
        return 2;   // kernel doesn't support
    }

    int fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DEFEND_KEY);
        return -1;
    }

    ret = ioctl(fd, CMD_SECURE_CHECK, &operation);
    kernel_ver = KernelV_3_14;
    close(fd);

    if (ret == 0) {
        printf("check platform: unencrypted\n");
    } else if (ret > 0) {
        printf("check platform: encrypted\n");
    } else {
        printf("check platform: failed\n");
    }

    return ret;
}

/**
  *  --- check platform whether encrypt or not
  *
  *  return value:
  *  <0: failed
  *  =0: unencrypted
  *  >0: encrypted
  */
int IsPlatformEncrypted(void)
{
    int fd = -1, ret = -1;
    ssize_t count = 0;
    char rBuf[128] = {0};
    char platform[PROPERTY_VALUE_MAX+1] = {0};

    if (!(access(SECURE_CHECK, F_OK) || (access(SECURE_CHECK_BAK, F_OK))) \
        || access(DEFEND_KEY, F_OK)) {
        printf("kernel doesn't support secure check\n");
        return 2;   // kernel doesn't support
    }

    fd = open(SECURE_CHECK, O_RDONLY);
    if (fd < 0) {
        fd = open(SECURE_CHECK_BAK, O_RDONLY);
        if (fd < 0) {
            printf("open %s failed (%s)\n",
            SECURE_CHECK, strerror(errno));
            return -1;
        }
        kernel_ver = KernelV_3_14;
    }

    property_get("ro.build.product", platform, "unknow");
    count = read(fd, rBuf, sizeof(rBuf) - 1);
    if (count <= 0) {
        printf("read %s failed (count:%d)\n",
            SECURE_CHECK, count);
        close(fd);
        return -1;
    }
    rBuf[count] = '\0';

    if (!strcmp(rBuf, s_pStatus[UNENCRYPT])) {
        printf("check platform(%s): unencrypted\n", platform);
        ret = 0;
    } else if (!strcmp(rBuf, s_pStatus[ENCRYPT])) {
        printf("check platform(%s): encrypted\n", platform);
        ret = 1;
    } else if (!strcmp(rBuf, s_pStatus[FAIL])) {
        printf("check platform(%s): failed\n", platform);
    } else {
        printf("check platform(%s): %s\n", platform, rBuf);
    }

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

/**
  *  --- get upgrade package image data
  *
  *  @zipArchive: zip archive object
  *  @imageName:  upgrade package image's name
  *  @imageSize:  upgrade package image's size
  *
  *  return value:
  *  <0: failed
  *  =0: can't find image
  *  >0: get image data successful
  */
static unsigned char *s_pImageBuffer = NULL;
static int GetZipArchiveImage(
        const ZipArchiveHandle za,
        const char *imageName,
        int *imageSize)
{
    ZipString zip_path(imageName);
    ZipEntry entry;
    if (FindEntry(za, zip_path, &entry) != 0) {
      printf("no %s in package!\n", imageName);
      return 0;
    }

    *imageSize = entry.uncompressed_length;
    if (*imageSize <= 0) {
        printf("can't get package entry uncomp len(%d) (%s)\n",
            *imageSize, strerror(errno));
        return -1;
    }

    if (s_pImageBuffer != NULL) {
        free(s_pImageBuffer);
        s_pImageBuffer = NULL;
    }

    s_pImageBuffer = (unsigned char *)calloc(*imageSize, sizeof(unsigned char));
    if (!s_pImageBuffer) {
        printf("can't malloc %d size space (%s)\n",
            *imageSize, strerror(errno));
        return -1;
    }

    int32_t ret = ExtractToMemory(za, &entry, s_pImageBuffer, entry.uncompressed_length);
    if (ret != 0) {
        printf("can't extract package entry to image buffer\n");
        goto FREE_IMAGE_MEM;
    }

    return 1;


FREE_IMAGE_MEM:
    if (s_pImageBuffer != NULL) {
        free(s_pImageBuffer);
        s_pImageBuffer = NULL;
    }

    return -1;
}

/**
  *  --- check platform and upgrade package whether
  *  encrypted,if all encrypted,rsa whether all the same
  *
  *  @ziparchive: Archive of  Zip Package
  *
  *  return value:
  *  =-1: failed; not allow upgrade
  *  = 0: check not match; not allow upgrade
  *  = 1: check match; allow upgrade
  *  = 2: kernel not support secure check; allow upgrade
  */
int RecoverySecureCheck(const ZipArchiveHandle zipArchive)
{
    int i = 0, ret = -1, err = -1;
    int ret_dtb = 0;
    int imageSize = 0;
    int platformEncryptStatus = 0, imageEncryptStatus = 0;
    const char *pImageName[] = {
            BOOTLOADER_IMG,
            BOOT_IMG,
            RECOVERY_IMG };

    //if not android 9, need upgrade for two step
    std::string android_version = android::base::GetProperty("ro.build.version.sdk", "");
    if (strcmp("28", android_version.c_str())) {
        printf("now upgrade from android %s to 28\n", android_version.c_str());
        return SECURE_SKIP;
    }

    platformEncryptStatus = IsPlatformEncrypted();
    if (platformEncryptStatus < 0) {
        printf("get platform encrypted by /sys/class/defendkey/secure_check failed, try ioctl!\n");
        platformEncryptStatus = IsPlatformEncryptedByIoctl();
    }

    if (platformEncryptStatus ==  2) {
        return SECURE_SKIP;// kernel doesn't support
    }

    if (platformEncryptStatus < 0) {
        return SECURE_ERROR;
    }

    if (platformEncryptStatus >0 ) {
        ret = GetZipArchiveImage(zipArchive, DTB_IMG, &imageSize);
        if (ret > 0) {
            ret_dtb = DtbImgEncrypted(DTB_IMG, s_pImageBuffer, imageSize, "0", NULL);
            if (ret_dtb == 2) {
                printf("dtb secure check not support!\n");
            } else if (ret_dtb > 0){
                printf("dtb secure check success!\n");

            } else {
                printf("dtb secure check error!\n");
                ret = SECURE_ERROR;
                goto ERR1;
            }
        } else if (ret == 0) {
            printf("check %s: not find,skiping...\n", DTB_IMG);
        } else {
            printf("get %s datas failed\n", DTB_IMG);
            goto ERR1;
        }
    }

    for (i = 0; i < ARRAY_SIZE(pImageName); i++) {
        ret = GetZipArchiveImage(zipArchive, pImageName[i], &imageSize);
        if (ret < 0) {
            printf("get %s datas failed\n", pImageName[i]);
           goto ERR1;
        } else if (ret == 0) {
            printf("check %s: not find,skiping...\n", pImageName[i]);
            continue;
        } else if (ret > 0) {
            secureDbg("get %s datas(size:0x%0x, addr:0x%x) successful\n",
                pImageName[i], imageSize, (int)s_pImageBuffer);
            imageEncryptStatus = IsZipArchiveImageEncrypted(pImageName[i], s_pImageBuffer, imageSize);

            printf("check %s: %s\n",
            pImageName[i], (imageEncryptStatus < 0) ? "failed" :
            !imageEncryptStatus ? "unencrypted" : "encrypted");
            if (imageEncryptStatus < 0) {
                ret = SECURE_ERROR;
                goto ERR1;
            }

            ret = IsPlatformMachWithZipArchiveImage(
                platformEncryptStatus, imageEncryptStatus, pImageName[i],
                s_pImageBuffer, imageSize);
           if (ret < 0) {
                printf("%s match platform failed\n", pImageName[i]);
                goto ERR1;
            } else if (ret == 0) { // if one of image doesn't match with platform,exit
                printf("%s doesn't match platform\n", pImageName[i]);
                goto ERR1;
            } else {
                secureDbg("%s match platform\n", pImageName[i]);
            }

            if (s_pImageBuffer != NULL) {
                free(s_pImageBuffer);
                s_pImageBuffer = NULL;
            }
        }
    }

    return SECURE_MATCH;


ERR1:
    if (s_pImageBuffer != NULL) {
        free(s_pImageBuffer);
        s_pImageBuffer = NULL;
    }

    return ret;
}
