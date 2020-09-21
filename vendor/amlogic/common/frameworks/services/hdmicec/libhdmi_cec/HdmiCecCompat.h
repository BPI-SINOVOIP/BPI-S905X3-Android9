/* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: Header file
*/

#ifndef HDMICECCOMPAT_H
#define HDMICECCOMPAT_H

namespace android {


static const bool COMPAT_LANGUAGES_OF_CHINESE = true;

static const char ZH_LANGUAGE[3] = {0x7a, 0x68, 0x6f};

enum cec_language{
    ZHO_LANGUAGE = (0x7a << 16) + (0x68 << 8) + 0x6f,  //CEC639-2 Simplified Chinese
    CHI_LANGUAGE = (0x63 << 16) + (0x68 << 8) + 0x69,  //CEC639-2 Traditional Chinese
    KOR_LANGUAGE = (0x6b << 16) + (0x6f << 8) + 0x72,  //CEC639-2 korea of cec
    ENG_LANGUAGE = (0x65 << 16) + (0x6e << 8) + 0x67,  //CEC639-2 English of cec
};

enum tv_vendor_id{
    LG_TV_VEDNOR_ID = (0x00 << 16) + (0xe0 << 8) + 0x91, //00 e0 91
    PHILIPS_TV_VEDNOR_ID = (0x4d << 16) + (0x53 << 8) + 0x54, //4d 53 54
    SUMSANG_TV_VENDOR_ID = (0x00 << 16) + (0x00 << 8) + 0xf0, //00 00 f0
};

static const int VENDOR_IDS[] = {
    LG_TV_VEDNOR_ID,
    PHILIPS_TV_VEDNOR_ID,
    SUMSANG_TV_VENDOR_ID
};

void compatLangIfneeded(int vendorId, int language, hdmi_cec_event_t* event);


// Condition of compat for the vendors which use different code for "zho"
bool isCompatVendor(int vendorId, int language);

} // namespace android

#endif // HDMICECCOMPAT_H
