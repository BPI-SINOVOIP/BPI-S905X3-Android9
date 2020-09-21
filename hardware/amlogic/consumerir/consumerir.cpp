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
 *  @date     2018/11/1
 *  @par function description:
 *  - 1 irblaster hal
 */

#define LOG_TAG "ConsumerIrHal"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/consumerir.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*
//used by ioctl
#define IR_DEV "/dev/irblaster1"
#define CONSUMERIR_TRANSMIT     0x5500
#define GET_CARRIER             0x5501
#define SET_CARRIER             0x5502
#define SET_MODLEVEL            0x5503

struct aml_blaster {
    unsigned int consumerir_freqs;
    unsigned int pattern[MAX_PLUSE];
    unsigned int pattern_len;
};

static int dev_fd = 0;
static struct aml_blaster gst_aml_br;
*/

#define MAX_PLUSE 1024
//used by sysfs
#define IR_CARRIER_FREQ     "/sys/devices/virtual/meson-irblaster/irblaster1/carrier_freq"
#define IR_DUTY_CYCLE       "/sys/devices/virtual/meson-irblaster/irblaster1/duty_cycle"
#define IR_FREQ_SEND        "/sys/devices/virtual/meson-irblaster/irblaster1/send"

static const consumerir_freq_range_t consumerirFreqs[] = {
    //{.min = 30000, .max = 30000},
    //{.min = 33000, .max = 33000},
    //{.min = 36000, .max = 36000},
    //{.min = 38000, .max = 38000},
    //{.min = 40000, .max = 40000},
    //{.min = 56000, .max = 56000},
    {.min = 25000, .max = 60000},
};

/*
static int readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if ( NULL == buf ) {
        ALOGE("buf is NULL");
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        ALOGE("readSys, open %s fail. Error info [%s]", path, strerror(errno));
        return len;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        ALOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);
    return len;
}
*/

static int writeSys(const char *path, const char *val, const int size) {
    int fd;

    //ALOGI("writeSysFs, size = %d \n", size);

    if ((fd = open(path, O_WRONLY)) < 0) {
        ALOGE("writeSysFs, open %s fail. %s", path, strerror(errno));
        return -1;
    }

    if (write(fd, val, size) != size) {
        ALOGE("write %s size:%d failed! %s\n", path, size, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int consumerir_transmit(struct consumerir_device *dev __unused,
    int carrier_freq, const int pattern[], int pattern_len) {

    if (pattern_len > MAX_PLUSE) {
        ALOGE("pattern length > max pluse :%d\n", MAX_PLUSE);
        return -1;
    }

    if (carrier_freq < 0) {
        ALOGE("carrier freq <0");
        return -1;
    }
    //set duty cycle, it's a static value
    char dutyCycle[] = "55";
    writeSys(IR_DUTY_CYCLE, dutyCycle, strlen(dutyCycle));

    //write carrier frequence
    char freqs[10] = {0};
    sprintf(freqs, "%d", carrier_freq);
    writeSys(IR_CARRIER_FREQ, freqs, strlen(freqs));

    //write send frequence
    int realLen = 0;
    int allocLen = 5*MAX_PLUSE + 1;
    char *pPatterns = (char *)malloc(allocLen);
    memset(pPatterns, 0, allocLen);
    for (int i = 0; i < pattern_len; i++) {
        ALOGI("pattern[%d] : %d \n", i, pattern[i]);

        char str[10] = {0};
        //use 's' to split the pattern string
        sprintf(str, "%ds", pattern[i]/10);
        strcat(pPatterns, str);
        realLen += strlen(str);
    }

    ALOGI("transmit pattern :%s \n", pPatterns);
    ALOGI("transmit pattern allocate len:%d, real len:%d \n", allocLen, realLen);
    if (realLen > allocLen) {
        ALOGE("transmit need allocate more memory to store patterns \n");
    }

    writeSys(IR_FREQ_SEND, pPatterns, strlen(pPatterns));
    free(pPatterns);
    pPatterns = NULL;

    return 0;
}

static int consumerir_get_num_carrier_freqs(struct consumerir_device *dev __unused) {
    return ARRAY_SIZE(consumerirFreqs);
}

static int consumerir_get_carrier_freqs(struct consumerir_device *dev __unused,
    size_t len, consumerir_freq_range_t *ranges) {
    size_t targetLen = ARRAY_SIZE(consumerirFreqs);

    targetLen = len < targetLen ? len : targetLen;
    memcpy(ranges, consumerirFreqs, targetLen * sizeof(consumerir_freq_range_t));
    return targetLen;
}

static int consumerir_close(hw_device_t *dev) {
    free(dev);

    ALOGI("consumerir hal close success");
    return 0;
}

/*
 * Generic device handling
 */
static int consumerir_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {

    if (strcmp(name, CONSUMERIR_TRANSMITTER) != 0) {
        ALOGE("the ir name is: %s, but we need: %s", name, CONSUMERIR_TRANSMITTER);
        return -EINVAL;
    }

    if (device == NULL) {
        ALOGE("NULL device on open");
        return -EINVAL;
    }

    consumerir_device_t *dev = (consumerir_device_t *)malloc(sizeof(consumerir_device_t));
    memset(dev, 0, sizeof(consumerir_device_t));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = consumerir_close;
    dev->transmit = consumerir_transmit;
    dev->get_num_carrier_freqs = consumerir_get_num_carrier_freqs;
    dev->get_carrier_freqs = consumerir_get_carrier_freqs;

    *device = (hw_device_t*) dev;

    ALOGI("consumerir hal open success");
    return 0;
}

static struct hw_module_methods_t consumerir_module_methods = {
    .open = consumerir_open,
};

consumerir_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                    = HARDWARE_MODULE_TAG,
        .module_api_version     = CONSUMERIR_MODULE_API_VERSION_1_0,
        .hal_api_version        = HARDWARE_HAL_API_VERSION,
        .id                     = CONSUMERIR_HARDWARE_MODULE_ID,
        .name                   = "AML IR HAL",
        .author                 = "Amlogic Corp.",
        .methods                = &consumerir_module_methods,
    },
};


