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
    ZHO_LANGUAGE = (0x7a << 16) + (0x68 << 8) + 0x6f,       //CEC639-2 Simplified Chinese
    CHI_LANGUAGE = (0x63 << 16) + (0x68 << 8) + 0x69,       //CEC639-2 Traditional Chinese
    KOR_LANGUAGE = (0x6b << 16) + (0x6f << 8) + 0x72,       //CEC639-2 korea of cec
    ENG_LANGUAGE = (0x65 << 16) + (0x6e << 8) + 0x67,       //CEC639-2 English of cec

    CZE_LANGUAGE_BIBLI = (0x63 << 16) + (0x7a << 8) + 0x65, //CEC639-2 Czech bibli of cec
    CES_LANGUAGE = (0x63 << 16) + (0x65 << 8) + 0x73,       //CEC639-2 Czech android of cec

    GER_LANGUAGE_BIBLI = (0x67 << 16) + (0x65 << 8) + 0x72, //CEC639-2 German bibli of cec
    DEU_LANGUAGE = (0x64 << 16) + (0x65 << 8) + 0x75,       //CEC639-2 German android of cec

    FRE_LANGUAGE_BIBLI = (0x66 << 16) + (0x72 << 8) + 0x65, //CEC639-2 French bibli of cec
    FRA_LANGUAGE = (0x66 << 16) + (0x72 << 8) + 0x61,       //CEC639-2 French android of cec

    MAL_LANGUAGE_BIBLI = (0x6d << 16) + (0x61 << 8) + 0x6c, //CEC639-2 Malayalam bibli of cec
    MSA_LANGUAGE = (0x6d << 16) + (0x73 << 8) + 0x61,       //CEC639-2 Malayalam android of cec

    DUT_LANGUAGE_BIBLI = (0x64 << 16) + (0x75 << 8) + 0x74, //CEC639-2 Dutch bibli of cec
    NLD_LANGUAGE = (0x6e << 16) + (0x6c << 8) + 0x64,       //CEC639-2 Dutch android of cec

    NOR_LANGUAGE_BIBLI = (0x6e << 16) + (0x6f << 8) + 0x72, //CEC639-2 Norwegian bibli of cec
    NOB_LANGUAGE = (0x6e << 16) + (0x6f << 8) + 0x62,       //CEC639-2 Norwegian android of cec

    RUM_LANGUAGE_BIBLI = (0x72 << 16) + (0x75 << 8) + 0x6d, //CEC639-2 Romanian bibli of cec
    RON_LANGUAGE = (0x72 << 16) + (0x6f << 8) + 0x6e,       //CEC639-2 Romanian android of cec

    SLO_LANGUAGE_BIBLI = (0x73 << 16) + (0x6c << 8) + 0x6f, //CEC639-2 Slovak bibli of cec
    SLK_LANGUAGE = (0x73 << 16) + (0x6c << 8) + 0x6b,       //CEC639-2 Slovak android of cec

    GRE_LANGUAGE_BIBLI = (0x67 << 16) + (0x72 << 8) + 0x65, //CEC639-2 Greek bibli of cec
    ELL_LANGUAGE = (0x65 << 16) + (0x6c << 8) + 0x6c,       //CEC639-2 Greek android of cec

    PER_LANGUAGE_BIBLI = (0x70 << 16) + (0x65 << 8) + 0x72, //CEC639-2 Persian bibli of cec
    FAS_LANGUAGE = (0x66 << 16) + (0x61 << 8) + 0x73,       //CEC639-2 Persian android of cec
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

int getAndroidCecCode(int language);

} // namespace android

#endif // HDMICECCOMPAT_H
