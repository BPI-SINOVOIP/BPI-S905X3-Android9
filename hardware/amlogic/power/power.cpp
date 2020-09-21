/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>
#ifdef BOARD_HAVE_CEC_HIDL_SERVICE
#include <HdmiCecHidlClient.h>
#endif
#include <cutils/properties.h>
#define DEBUG 0

#define MP_GPU_CMD                      "/sys/class/mpgpu/mpgpucmd"
#define EARLY_SUSPEND_TRIGGER           "/sys/power/early_suspend_trigger"

#define PLATFORM_SLEEP_MODES 1

struct private_power_module {
    power_module_t base;
    int gpuFp;
    int suspendFp;
};

namespace android {
namespace amlogic {

static void init (struct power_module *module) {
}

static void setInteractive (struct power_module *module, int on) {
    ALOGI("setInteractive ineractive:%s", (on==1)?"yes":"no");

    struct private_power_module *pm = (struct private_power_module *) module;
    if (pm->suspendFp < 0) {
        pm->suspendFp = open(EARLY_SUSPEND_TRIGGER, O_RDWR);
        if (pm->suspendFp < 0) {
            ALOGE("open %s fail, %s", EARLY_SUSPEND_TRIGGER, strerror(errno));
            return;
        }
    }
    #ifdef BOARD_HAVE_CEC_HIDL_SERVICE
    if (0 == on) {
        char value[PROPERTY_VALUE_MAX] = {0};
        const char * split = ",";
        char * type;
        int sender  = 0;
        property_get("ro.vendor.platform.hdmi.device_type", value, "0");
        type = strtok(value, split);
        if (atoi(type) == CEC_ADDR_TV) {
           sender = CEC_ADDR_TV;
        } else if (atoi(type) == CEC_ADDR_PLAYBACK_1) {
           sender = CEC_ADDR_PLAYBACK_1;
        }
        HdmiCecHidlClient *mHdmiCecHidlClient = NULL;
        mHdmiCecHidlClient = HdmiCecHidlClient::connect(CONNECT_TYPE_POWER);
        if (mHdmiCecHidlClient != NULL) {
            cec_message_t message;
            message.initiator = (cec_logical_address_t)sender;
            message.destination = (cec_logical_address_t)CEC_ADDR_BROADCAST;
            message.length = 1;
            message.body[0] = CEC_MESSAGE_STANDBY;
            mHdmiCecHidlClient->setOption(HDMI_OPTION_SYSTEM_CEC_CONTROL, 0);
            mHdmiCecHidlClient->sendMessage(&message, false);
            ALOGI("send <Standby> message before early suspend.");
        }
        delete mHdmiCecHidlClient;
    }
    #endif
    //resume
    if (1 == on) {
        write(pm->suspendFp, "0", 1);
    }
    else {
        write(pm->suspendFp, "1", 1);
    }
}

static void powerHint(struct power_module *module, power_hint_t hint, void *data) {

    struct private_power_module *pm = (struct private_power_module *) module;

    const char *val = "preheat";
    static int bytes = 7;

    if (pm->gpuFp < 0) {
        pm->gpuFp = open(MP_GPU_CMD, O_RDWR);
        if (pm->gpuFp < 0) {
            ALOGE("open %s fail, %s", MP_GPU_CMD, strerror(errno));
            return;
        }
    }

    switch (hint) {
    case POWER_HINT_INTERACTION:
        if (pm->gpuFp >= 0) {
            int len = write(pm->gpuFp, val, bytes);
            if (DEBUG) {
                ALOGD("%s: write sucessfull, fd is %d\n", __FUNCTION__, pm->gpuFp);
            }

            if (len != bytes)
                ALOGE("write preheat faile");
        }
        break;

    default:
        break;
    }
}


static void setFeature (struct power_module *module, feature_t feature, int state) {

}

static int getPlatformLowPowerStats (struct power_module *module,
        power_state_platform_sleep_state_t *list) {

    if (!list) {
        return -EINVAL;
    }

    strcpy(list[0].name, "screen_off");
    list[0].supported_only_in_suspend = false;
    list[0].number_of_voters = 0;

    return 0;
}

static ssize_t geNumberOfPlatformModes (struct power_module *module) {
    ALOGI("%s", __func__);
    return PLATFORM_SLEEP_MODES;
}

static int getVoterList (struct power_module *module, size_t *voter) {
    voter[0] = 0;
    return 0;
}

} // namespace amlogic
} // namespace android


static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct private_power_module HAL_MODULE_INFO_SYM = {
    .base = {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = POWER_MODULE_API_VERSION_0_2,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = POWER_HARDWARE_MODULE_ID,
            .name = "AML Power HAL",
            .author = "aml",
            .methods = &power_module_methods,
        },
        .init = android::amlogic::init,
        .setInteractive = android::amlogic::setInteractive,
        .powerHint = android::amlogic::powerHint,
        .setFeature = android::amlogic::setFeature,
        .get_platform_low_power_stats = android::amlogic::getPlatformLowPowerStats,
        .get_number_of_platform_modes = android::amlogic::geNumberOfPlatformModes,
        .get_voter_list = android::amlogic::getVoterList,
    },
    .gpuFp = -1,
    .suspendFp = -1,
};
