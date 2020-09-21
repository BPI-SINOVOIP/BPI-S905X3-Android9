/* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: Header file
*/

#define LOG_TAG "hdmicecd"

#include <HdmiCecBase.h>

#include "HdmiCecCompat.h"

namespace android {

void compatLangIfneeded(int vendorId, int language, hdmi_cec_event_t* event) {
    if (event != NULL && isCompatVendor(vendorId, language)) {
        event->cec.body[1] = ZH_LANGUAGE[0];
        event->cec.body[2] = ZH_LANGUAGE[1];
        event->cec.body[3] = ZH_LANGUAGE[2];
    }
}

bool isCompatVendor(int vendorId, int language) {
    // For traditional chinese code
    if (CHI_LANGUAGE == language) {
        for (unsigned int i = 0; i < sizeof(VENDOR_IDS) / sizeof(VENDOR_IDS[0]); i++) {
            if (vendorId == VENDOR_IDS[i]) {
                return true;
            }
        }
    }
    // For korean code
    if (KOR_LANGUAGE == language && LG_TV_VEDNOR_ID == vendorId) {
        return true;
    }

    return false;
}

} //namespace android
