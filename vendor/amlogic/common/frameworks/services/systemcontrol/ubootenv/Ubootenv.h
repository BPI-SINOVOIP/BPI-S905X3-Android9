/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef _INIT_BOOTENV_H
#define _INIT_BOOTENV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cutils/properties.h>
#include <cutils/threads.h>

#define MAX_UBOOT_RWRETRY       5

typedef struct env_image {
    uint32_t  crc;/* CRC32 over data bytes*/
    char data[]; /* Environment data*/
} env_image_t;

typedef struct environment {
    void *image;
    uint32_t *crc;
    char *data;
} environment_t;

typedef struct env_attribute {
    struct env_attribute *next;
    char key[256];
    char value[1024];
} env_attribute_t;

struct callback_data {
    void (*callback)(const char* name, const char* value, void* cookie);
    void* cookie;
};

class Ubootenv {
public:
    Ubootenv();
    ~Ubootenv();

    int reInit();
    const char * getValue(const char * key);
    int updateValue(const char* name, const char* value);
    void printValues();

private:
    int init();
    int readPartitionData();
    env_attribute* parseAttribute();
    char* get(const char * key);
    int set(const char * key,  const char * value, bool createNew);
    int save();
    int formatAttribute();
    int isEnv(const char* prop_name);
    void propertyTrampoline(void* raw_data, const char* name, const char* value, unsigned serial);
    void propertyListCallback(const prop_info* pi, void* data);
    void propertyInit(const char *key, const char *value, void *cookie);
    int propertyList(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie);
    void propertyLoad();

    char mEnvPartitionName[32];
    int mEnvPartitionSize;
    int mEnvSize;

    environment_t mEnvData;
    env_attribute_t mEnvAttrHeader;

    mutex_t mEnvLock;
    bool mEnvInitDone;
};

#ifdef __cplusplus
}
#endif
#endif

