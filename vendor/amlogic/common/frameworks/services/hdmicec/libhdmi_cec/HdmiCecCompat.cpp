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
    if (!event) {
        return;
    }

    // Replace the vendors' wrong code to simlified Chinese.
    if (isCompatVendor(vendorId, language)) {
        ALOGI("replace Simplified Chinese code for %x", vendorId);
        event->cec.body[1] = ZH_LANGUAGE[0];
        event->cec.body[2] = ZH_LANGUAGE[1];
        event->cec.body[3] = ZH_LANGUAGE[2];
        return;
    }

    // Replace the bibliography code to android's terminology 639-2 code
    int androidCode = getAndroidCecCode(language);
    if (androidCode != language) {
        ALOGI("replace bibliography %2x to android identified %2x", language, androidCode);
        event->cec.body[1] = (androidCode >> 16) & 0xFF;
        event->cec.body[2] = (androidCode >> 8) & 0xFF;
        event->cec.body[3] = androidCode & 0xFF;
    }
    return;
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

int getAndroidCecCode(int language) {
    int androidCode = language;
    switch (language) {
        case CZE_LANGUAGE_BIBLI:
            androidCode = CES_LANGUAGE;
            break;
        case GER_LANGUAGE_BIBLI:
            androidCode = DEU_LANGUAGE;
            break;
        case FRE_LANGUAGE_BIBLI:
            androidCode = FRA_LANGUAGE;
            break;
        case MAL_LANGUAGE_BIBLI:
            androidCode = MSA_LANGUAGE;
            break;
        case DUT_LANGUAGE_BIBLI:
            androidCode = NLD_LANGUAGE;
            break;
        case NOR_LANGUAGE_BIBLI:
            androidCode = NOB_LANGUAGE;
            break;
        case RUM_LANGUAGE_BIBLI:
            androidCode = RON_LANGUAGE;
            break;
        case SLO_LANGUAGE_BIBLI:
            androidCode = SLK_LANGUAGE;
            break;
        case GRE_LANGUAGE_BIBLI:
            androidCode = ELL_LANGUAGE;
            break;
        case PER_LANGUAGE_BIBLI:
            androidCode = FAS_LANGUAGE;
            break;
        default:
            break;
    }
    return androidCode;
}

} //namespace android
