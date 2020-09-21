/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <zlib.h>

#ifdef MTD_OLD
# include <linux/mtd/mtd.h>
#else
# define  __user	/* nothing */
# include <mtd/mtd-user.h>
#endif

#include "Ubootenv.h"
#include "common.h"


const char *PROFIX_UBOOTENV_VAR = "ubootenv.var.";

Ubootenv::Ubootenv() :
    mEnvLock(MUTEX_INITIALIZER) {

    init();

    //printValues();
}

Ubootenv::~Ubootenv() {
    if (mEnvData.image) {
        free(mEnvData.image);
        mEnvData.image = NULL;
        mEnvData.crc = NULL;
        mEnvData.data = NULL;
    }
    env_attribute * pAttr = mEnvAttrHeader.next;
    memset(&mEnvAttrHeader, 0, sizeof(env_attribute));
    env_attribute * pTmp = NULL;
    while (pAttr) {
        pTmp = pAttr;
        pAttr = pAttr->next;
        free(pTmp);
    }
}

int Ubootenv::updateValue(const char* name, const char* value) {
    if (!mEnvInitDone) {
        SYS_LOGE("[ubootenv] bootenv do not init\n");
        return -1;
    }

    SYS_LOGI("[ubootenv] update value name [%s]: value [%s] \n", name, value);
    const char* envName = NULL;
    if (strcmp(name, "ubootenv.var.bootcmd") == 0) {
        envName = "bootcmd";
    }
    else {
        if (!isEnv(name)) {
            //should assert here.
            SYS_LOGE("[ubootenv] %s is not a ubootenv variable.\n", name);
            return -2;
        }
        envName = name + strlen(PROFIX_UBOOTENV_VAR);
    }

    const char *envValue = get(envName);
    if (!envValue)
        envValue = "";

    if (!strcmp(value, envValue))
        return 0;

    mutex_lock(&mEnvLock);
    set(envName, value, true);

    int i = 0;
    int ret = -1;
    while (i < MAX_UBOOT_RWRETRY && ret < 0) {
        i ++;
        ret = save();
        if (ret < 0)
            SYS_LOGE("[ubootenv] Cannot write %s: %d.\n", mEnvPartitionName, ret);
    }

    if (i < MAX_UBOOT_RWRETRY) {
        SYS_LOGI("[ubootenv] Save ubootenv to %s succeed!\n", mEnvPartitionName);
    }

    mutex_unlock(&mEnvLock);

    return ret;
}

const char * Ubootenv::getValue(const char * key) {
    if (!isEnv(key)) {
        //should assert here.
        SYS_LOGE("[ubootenv] %s is not a ubootenv varible.\n", key);
        return NULL;
    }

    mutex_lock(&mEnvLock);
    const char* envName = key + strlen(PROFIX_UBOOTENV_VAR);
    const char* envValue = get(envName);
    mutex_unlock(&mEnvLock);
    return envValue;
}

void Ubootenv::printValues() {
    env_attribute *attr = &mEnvAttrHeader;
    while (attr != NULL) {
        SYS_LOGI("[ubootenv] key: [%s], value: [%s]\n", attr->key, attr->value);
        attr = attr->next;
    }
}

int Ubootenv::reInit() {
   mutex_lock(&mEnvLock);

   if (mEnvData.image) {
       free(mEnvData.image);
       mEnvData.image = NULL;
       mEnvData.crc = NULL;
       mEnvData.data = NULL;
   }
   env_attribute * pAttr = mEnvAttrHeader.next;
   memset(&mEnvAttrHeader, 0, sizeof(env_attribute));
   env_attribute * pTmp = NULL;
   while (pAttr) {
       pTmp = pAttr;
       pAttr = pAttr->next;
       free(pTmp);
   }
   init();

   mutex_unlock(&mEnvLock);
   return 0;
}

int Ubootenv::init() {
    const char *NAND_ENV = "/dev/nand_env";
    const char *BLOCK_ENV = "/dev/block/env";//normally use that
    const char *BLOCK_UBOOT_ENV = "/dev/block/ubootenv";
    struct stat st;

    //the nand env or block env is the same
    mEnvPartitionSize = 0x10000;
//#if defined(MESON8_ENVSIZE) || defined(GXBABY_ENVSIZE) || defined(GXTVBB_ENVSIZE) || defined(GXL_ENVSIZE)
//    mEnvPartitionSize = 0x10000;
//#endif
    mEnvSize = mEnvPartitionSize - sizeof(uint32_t);

    if (!stat(NAND_ENV, &st)) {
        strcpy (mEnvPartitionName, NAND_ENV);
    }
    else if (!stat(BLOCK_ENV, &st)) {
        strcpy (mEnvPartitionName, BLOCK_ENV);
    }
    else if (!stat(BLOCK_UBOOT_ENV, &st)) {
        int fd;
        struct mtd_info_user info;

        strcpy (mEnvPartitionName, BLOCK_UBOOT_ENV);
        if ((fd = open(mEnvPartitionName, O_RDWR)) < 0) {
            SYS_LOGE("[ubootenv] open device(%s) error\n", mEnvPartitionName );
            return -2;
        }

        memset(&info, 0, sizeof(info));
        int err = ioctl(fd, MEMGETINFO, &info);
        if (err < 0) {
            SYS_LOGE("[ubootenv] get MTD info error\n" );
            close(fd);
            return -3;
        }
        close(fd);

        //mEnvEraseSize = info.erasesize;//0x20000;//128K
        mEnvPartitionSize = info.size;//0x8000;
        mEnvSize = mEnvPartitionSize - sizeof(long);
    }

    //the first four bytes are crc value, others are data
    SYS_LOGI("[ubootenv] using %s with size(%d) (%d)", mEnvPartitionName, mEnvPartitionSize, mEnvSize);

    int i = 0;
    int ret = -1;
    while (i < MAX_UBOOT_RWRETRY && ret < 0) {
        i ++;
        ret = readPartitionData();
        if (ret < 0)
            SYS_LOGE("[ubootenv] Cannot read %s: %d.\n", mEnvPartitionName, ret);
        if (ret < -2)
            free(mEnvData.image);
    }

    if (i >= MAX_UBOOT_RWRETRY) {
        SYS_LOGE("[ubootenv] read %s failed \n", mEnvPartitionName);
        return -2;
    }

#if 0
    char prefix[PROP_VALUE_MAX] = {0};
    property_get("ro.ubootenv.varible.prefix", prefix, "");
    if (prefix[0] == 0) {
        strcpy(prefix , "ubootenv.var");
        SYS_LOGI("[ubootenv] set property ro.ubootenv.varible.prefix: %s\n", prefix);
        property_set("ro.ubootenv.varible.prefix", prefix);
    }

    if (strlen(prefix) > 16) {
        SYS_LOGE("[ubootenv] Cannot r/w ubootenv varibles - prefix length > 16.\n");
        return -4;
    }

    sprintf(PROFIX_UBOOTENV_VAR, "%s.", prefix);
    SYS_LOGI("[ubootenv] ubootenv varible prefix is: %s\n", prefix);
#endif

    propertyLoad();
    return 0;
}

int Ubootenv::readPartitionData() {
    int fd;
    if ((fd = open(mEnvPartitionName, O_RDONLY)) < 0) {
        SYS_LOGE("[ubootenv] open devices error: %s\n", strerror(errno));
        return -1;
    }

    char *addr = (char *)malloc(mEnvPartitionSize);
    if (addr == NULL) {
        SYS_LOGE("[ubootenv] Not enough memory for environment (%u bytes)\n", mEnvPartitionSize);
        close(fd);
        return -2;
    }

    memset(addr, 0, mEnvPartitionSize);
    mEnvData.image = addr;
    struct env_image *image = (struct env_image *)addr;
    mEnvData.crc = &(image->crc);
    mEnvData.data = image->data;

    int ret = read(fd ,mEnvData.image, mEnvPartitionSize);
    if (ret == (int)mEnvPartitionSize) {
        uint32_t crcCalc = crc32(0, (uint8_t *)mEnvData.data, mEnvSize);
        if (crcCalc != *(mEnvData.crc)) {
            SYS_LOGE("[ubootenv] CRC Check SYS_LOGE save_crc=%08x, crcCalc = %08x \n",
                *mEnvData.crc, crcCalc);
            close(fd);
            return -3;
        }

        parseAttribute();
        //printValues();
    } else {
        SYS_LOGE("[ubootenv] read error 0x%x \n",ret);
        close(fd);
        return -5;
    }
    close(fd);
    return 0;
}

/* Parse a session attribute */
env_attribute* Ubootenv::parseAttribute() {
    char *proc = mEnvData.data;
    char *nextProc;
    env_attribute *attr = &mEnvAttrHeader;

    memset(attr, 0, sizeof(env_attribute));

    do {
        nextProc = proc + strlen(proc) + sizeof(char);
        //SYS_LOGV("process %s\n",proc);
        char *key = strchr(proc, (int)'=');
        if (key != NULL) {
            *key=0;
            strcpy(attr->key, proc);
            strcpy(attr->value, key + sizeof(char));
        } else {
            SYS_LOGE("[ubootenv] error need '=' skip this value\n");
        }

        if (!(*nextProc)) {
            //SYS_LOGV("process end \n");
            break;
        }
        proc = nextProc;

        attr->next = (env_attribute *)malloc(sizeof(env_attribute));
        if (attr->next == NULL) {
            SYS_LOGE("[ubootenv] parse attribute malloc error \n");
            break;
        }
        memset(attr->next, 0, sizeof(env_attribute));
        attr = attr->next;
    }while(1);

    return &mEnvAttrHeader;
}

char * Ubootenv::get(const char * key) {
    if (!mEnvInitDone) {
        SYS_LOGE("[ubootenv] don't init done\n");
        return NULL;
    }

    env_attribute *attr = &mEnvAttrHeader;
    while (attr) {
        if (!strcmp(key, attr->key)) {
            return attr->value;
        }
        attr = attr->next;
    }
    return NULL;
}

/*
creat_args_flag : if true , if envvalue don't exists Creat it .
              if false , if envvalue don't exists just exit .
*/
int Ubootenv::set(const char * key,  const char * value, bool createNew) {
    env_attribute *attr = &mEnvAttrHeader;
    env_attribute *last = attr;
    while (attr) {
        if (!strcmp(key, attr->key)) {
            strcpy(attr->value, value);
            return 2;
        }
        last = attr;
        attr = attr->next;
    }

    if (createNew) {
        SYS_LOGV("[ubootenv] ubootenv.var.%s not found, create it.\n", key);

        attr = (env_attribute *)malloc(sizeof(env_attribute));
        last->next = attr;
        memset(attr, 0, sizeof(env_attribute));
        strcpy(attr->key, key);
        strcpy(attr->value, value);
        return 1;
    }
    return 0;
}

//save value to storage flash
int Ubootenv::save() {
    int fd;
    int err;

    formatAttribute();
    *(mEnvData.crc) = crc32(0, (uint8_t *)mEnvData.data, mEnvSize);

    if ((fd = open (mEnvPartitionName, O_RDWR)) < 0) {
        SYS_LOGE("[ubootenv] open devices error\n");
        return -1;
    }

    if (strstr (mEnvPartitionName, "mtd")) {
        struct erase_info_user erase;
        struct mtd_info_user info;
        unsigned char *data = NULL;

        memset(&info, 0, sizeof(info));
        err = ioctl(fd, MEMGETINFO, &info);
        if (err < 0) {
            SYS_LOGE("[ubootenv] Get MTD info error\n");
            close(fd);
            return -4;
        }

        erase.start = 0;
        if (info.erasesize > (unsigned int)mEnvPartitionSize) {
            data = (unsigned char*)malloc(info.erasesize);
            if (data == NULL) {
                SYS_LOGE("[ubootenv] Out of memory!!!\n");
                close(fd);
                return -5;
            }
            memset(data, 0, info.erasesize);
            err = read(fd, (void*)data, info.erasesize);
            if (err != (int)info.erasesize) {
                SYS_LOGE("[ubootenv] Read access failed !!!\n");
                free(data);
                close(fd);
                return -6;
            }
            memcpy(data, mEnvData.image, mEnvPartitionSize);
            erase.length = info.erasesize;
        }
        else {
            erase.length = mEnvPartitionSize;
        }

        err = ioctl (fd, MEMERASE,&erase);
        if (err < 0) {
            SYS_LOGE ("[ubootenv] MEMERASE SYS_LOGE %d\n",err);
            close(fd);
            return  -2;
        }

        if (info.erasesize > (unsigned int)mEnvPartitionSize) {
            lseek(fd, 0L, SEEK_SET);
            err = write(fd , data, info.erasesize);
            free(data);
        }
        else
            err = write(fd ,mEnvData.image, mEnvPartitionSize);

    } else {
        //emmc and nand needn't erase
        err = write(fd, mEnvData.image, mEnvPartitionSize);
    }

    close(fd);
    if (err < 0) {
        SYS_LOGE ("[ubootenv] SYS_LOGE write, size %d \n", mEnvPartitionSize);
        return -3;
    }
    return 0;
}

/*  attribute revert to sava data*/
int Ubootenv::formatAttribute() {
    env_attribute *attr = &mEnvAttrHeader;
    char *data = mEnvData.data;
    memset(mEnvData.data, 0, mEnvSize);
    do {
        int len = sprintf(data, "%s=%s", attr->key, attr->value);
        if (len < (int)(sizeof(char)*3)) {
            SYS_LOGE("[ubootenv] Invalid env data key:%s, value:%s\n", attr->key, attr->value);
        }
        else
            data += len + sizeof(char);

        attr = attr->next;
    } while (attr);
    return 0;
}

int Ubootenv::isEnv(const char* prop_name) {
    if (!prop_name || !(*prop_name))
        return 0;

    if (!(*PROFIX_UBOOTENV_VAR))
        return 0;

    if (strncmp(prop_name, PROFIX_UBOOTENV_VAR, strlen(PROFIX_UBOOTENV_VAR)) == 0
        && strlen(prop_name) > strlen(PROFIX_UBOOTENV_VAR) )
        return 1;

    return 0;
}

#if 0
void Ubootenv::propertyTrampoline(void* raw_data, const char* name, const char* value, unsigned serial) {
    struct callback_data* data = (struct callback_data*)(raw_data);
    data->callback(name, value, data->cookie);
}

void Ubootenv::propertyListCallback(const prop_info* pi, void* data) {
    __system_property_read_callback(pi, propertyTrampoline, data);
}

void Ubootenv::propertyInit(const char *key, const char *value, void *cookie) {
    if (isEnv(key)) {
        const char* varible_name = key + strlen(PROFIX_UBOOTENV_VAR);
        const char *varible_value = get(varible_name);
        if (!varible_value)
            varible_value = "";
        if (strcmp(varible_value, value)) {
            property_set(key, varible_value);
            SYS_LOGI("[ubootenv] bootenv_prop_init set property key:%s value:%s\n", key, varible_value);
            (*((int*)cookie))++;
        }
    }
}

int Ubootenv::propertyList(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie) {
    if (true/*bionic_get_application_target_sdk_version() >= __ANDROID_API_O__*/) {
        struct callback_data data = { propfn, cookie };
        return __system_property_foreach(propertyListCallback, &data);
    }

    char name[PROP_NAME_MAX];
    char value[PROP_VALUE_MAX];
    const prop_info *pi;
    unsigned n;

    for (n = 0; (pi = __system_property_find_nth(n)); n++) {
        __system_property_read(pi, name, value);
        propfn(name, value, cookie);
    }
    return 0;
}
#endif

void Ubootenv::propertyLoad() {
    int count = 0;

    //propertyList(propertyInit, (void*)&count);

    SYS_LOGI("[ubootenv] set property count: %d\n", count);
    mEnvInitDone = true;
}
