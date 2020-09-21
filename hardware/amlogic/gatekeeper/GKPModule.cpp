/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <hardware/hardware.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

//#include "trusty/trusty_gatekeeper.h"
#include "soft/SoftGateKeeperDevice.h"

//using gatekeeper::TrustyGateKeeperDevice;
using android::SoftGateKeeperDevice;

static int gatekeeper_open(const hw_module_t *module, const char *name,
        hw_device_t **device) {

    if (strcmp(name, HARDWARE_GATEKEEPER) != 0) {
        return -EINVAL;
    }

    //trusty gatekeeper don't implement, so use software instead of
    if (true) {
        SoftGateKeeperDevice *gatekeeper = new SoftGateKeeperDevice(module);
        if (gatekeeper == NULL) return -ENOMEM;
        *device = gatekeeper->sw_device();
    }
    else {
        //TrustyGateKeeperDevice *gatekeeper = new TrustyGateKeeperDevice(module);
        //if (gatekeeper == NULL) return -ENOMEM;
        //*device = gatekeeper->hw_device();
    }

    return 0;
}

static struct hw_module_methods_t gatekeeper_module_methods = {
    .open = gatekeeper_open,
};

struct gatekeeper_module HAL_MODULE_INFO_SYM __attribute__((visibility("default"))) = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = GATEKEEPER_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = GATEKEEPER_HARDWARE_MODULE_ID,
        .name = "AML GateKeeper HAL",
        .author = "aml",
        .methods = &gatekeeper_module_methods,
        .dso = 0,
        .reserved = {}
    },
};
