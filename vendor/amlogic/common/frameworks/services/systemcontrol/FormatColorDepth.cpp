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
 *  @version  2.0
 *  @date     2014/09/09
 *  @par function description:
 *  - 1 HDMI deep color attribute
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/types.h>
#include "DisplayMode.h"
#include "FormatColorDepth.h"
#include "common.h"

//this is check hdmi mode list
static const char* COLOR_ATTRIBUTE_LIST[] = {
    COLOR_RGB_8BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR422_8BIT,
};
//this is prior selected list  of 4k2k50hz, 4k2k60hz smpte50hz, smpte60hz
static const char* COLOR_ATTRIBUTE_LIST1[] = {
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of other display mode
static const char* COLOR_ATTRIBUTE_LIST2[] = {
    COLOR_YCBCR444_12BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_12BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of Low Power Mode 4k2k50hz, 4k2k60hz smpte50hz, smpte60hz
static const char* COLOR_ATTRIBUTE_LIST3[] = {
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list of Low Power Mode other display mode
static const char* COLOR_ATTRIBUTE_LIST4[] = {
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_12BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_12BIT,
};

FormatColorDepth::FormatColorDepth(Ubootenv *ubootenv) {
    mUbootenv = ubootenv;
}

FormatColorDepth::~FormatColorDepth() {

}

bool FormatColorDepth::initColorAttribute(char* supportedColorList, int len) {
    int count = 0;
    bool result = false;

    if (supportedColorList != NULL)
        memset(supportedColorList, 0, len);

    while (true) {
        //mSysWrite.readSysfsOriginal(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        mSysWrite.readSysfs(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        if (strlen(supportedColorList) > 0) {
            result = true;
            break;
        }

        if (count++ >= 5) {
            break;
        }
        usleep(500000);
    }

    return result;
}

void FormatColorDepth::getHdmiColorAttribute(const char* outputmode, char* colorAttribute, int state) {
    char supportedColorList[MAX_STR_LEN];

    //if read /sys/class/amhdmitx/amhdmitx0/dc_cap is null. return
    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)
              || !strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
            mSysWrite.getPropertyString(PROP_DEFAULT_COLOR, colorAttribute, COLOR_YCBCR420_8BIT);
        } else {
            mSysWrite.getPropertyString(PROP_DEFAULT_COLOR, colorAttribute, COLOR_YCBCR444_8BIT);
        }

        SYS_LOGE("Error!!! Do not find sink color list, use default color attribute:%s\n", colorAttribute);
        return;
    }

    if (mSysWrite.getPropertyBoolean(PROP_HDMIONLY, true)) {
        char curMode[MODE_LEN] = {0};
        char isBestMode[MODE_LEN] = {0};
        mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curMode);

        // if only change deepcolor in MoreSettings, getcolorAttr from ubootenv, it was set in DroidTvSettings
        if ((state == OUPUT_MODE_STATE_SWITCH) && (!strcmp(curMode, outputmode))
                && getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) && (strcmp(isBestMode, "false") == 0)) {
            //note: "outputmode" should be the second parameter of "strcmp", because it maybe prefix of "curMode".
            SYS_LOGI("Only modify deep color mode, get colorAttr from ubootenv.var.colorattribute\n");
            getBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
        } else {
            getProperHdmiColorArrtibute(outputmode,  colorAttribute);
        }
    }

    SYS_LOGI("get hdmi color attribute : [%s], outputmode is: [%s] , and support color list is: [%s]\n",
        colorAttribute, outputmode, supportedColorList);
}

void FormatColorDepth::getProperHdmiColorArrtibute(const char* outputmode, char* colorAttribute) {
    char ubootvar[MODE_LEN] = {0};
    char tmpValue[MODE_LEN] = {0};
    char isBestMode[MODE_LEN] = {0};

    // if auto switch best mode is off, get priority color value of mode in ubootenv,
    // and judge current mode whether this colorValue is supported in This TV device.
    // if not support or auto switch best mode is on, select color value from Lists in next step.
    if (getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) && (strcmp(isBestMode, "false") == 0)) {
        SYS_LOGI("get color attr from ubootenv.var.%s_deepcolor When is not best mode\n", outputmode);
        sprintf(ubootvar, "ubootenv.var.%s_deepcolor", outputmode);
        if (getBootEnv(ubootvar, tmpValue) && strstr(tmpValue, "bit") && isModeSupportDeepColorAttr(outputmode, tmpValue)) {
            strcpy(colorAttribute, tmpValue);
            return;
        }
    }

    getBestHdmiDeepColorAttr(outputmode, colorAttribute);

    //if colorAttr is null above steps, will defines a initial value to it
    if (!strstr(colorAttribute, "bit")) {
        strcpy(colorAttribute, COLOR_YCBCR444_8BIT);
    }

    //SYS_LOGI("get best hdmi color attribute %s\n", colorAttribute);
}

void FormatColorDepth::getBestHdmiDeepColorAttr(const char *outputmode, char* colorAttribute) {
    char *pos = NULL;
    int length = 0;
    const char **colorList = NULL;
    char supportedColorList[MAX_STR_LEN];
    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        SYS_LOGE("initColorAttribute fail\n");
        return;
    }

    //filter some color value options, aimed at some modes.
    if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)
        || !strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
        if (mSysWrite.getPropertyBoolean(LOW_POWER_DEFAULT_COLOR, false)) {
            colorList = COLOR_ATTRIBUTE_LIST3;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST3);
        } else {
            colorList = COLOR_ATTRIBUTE_LIST1;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST1);
        }
    } else {
        if (mSysWrite.getPropertyBoolean(LOW_POWER_DEFAULT_COLOR, false)) {
            colorList = COLOR_ATTRIBUTE_LIST4;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST4);
        } else {
            colorList = COLOR_ATTRIBUTE_LIST2;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST2);
        }
    }

    for (int i = 0; i < length; i++) {
        if ((pos = strstr(supportedColorList, colorList[i])) != NULL) {
            if (isModeSupportDeepColorAttr(outputmode, colorList[i])) {
                SYS_LOGI("support current mode:[%s], deep color:[%s]\n", outputmode, colorList[i]);

                strcpy(colorAttribute, colorList[i]);
                break;
            }
        }
    }
}

bool FormatColorDepth::isFilterEdid() {
    unsigned int edidNumber;
    char disPlayEdid[EDID_SIZE] = {0};
    mSysWrite.readSysfs(DISPLAY_EDID_RAW,disPlayEdid);
    for (edidNumber = 0; edidNumber < mFilterEdid.size(); edidNumber++) {
        if (!strncmp(mFilterEdid[edidNumber].c_str(),disPlayEdid,EDID_SIZE)) {
            return true;
        }
    }
    return false;
}

void FormatColorDepth::setFilterEdidList(std::map<int, std::string> filterEdidList) {
     SYS_LOGI("FormatColorDepth setFilterEdidList size = %d",filterEdidList.size());
     mFilterEdid = filterEdidList;
}

bool FormatColorDepth::isModeSupportDeepColorAttr(const char *mode, const char * color) {
    char valueStr[10] = {0};
    char outputmode[MODE_LEN] = {0};

    strcpy(outputmode, mode);
    strcat(outputmode, color);

    if (isFilterEdid() && !strstr(color,"8bit")) {
        SYS_LOGI("this mode has been filtered");
        return false;
    }

    //try support or not
    mSysWrite.writeSysfs(DISPLAY_HDMI_VALID_MODE, outputmode);
    mSysWrite.readSysfs(DISPLAY_HDMI_VALID_MODE, valueStr);

    return atoi(valueStr) ? true : false;
}

bool FormatColorDepth::isSupportHdmiMode(const char *hdmi_mode, const char *supportedColorList) {
    int length = 0;
    const char **colorList = NULL;

    colorList = COLOR_ATTRIBUTE_LIST;
    length    = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST);

    if (strstr(hdmi_mode, "2160p60hz")  != NULL
        || strstr(hdmi_mode,"2160p50hz") != NULL
        || strstr(hdmi_mode,"smpte50hz") != NULL
        || strstr(hdmi_mode,"smpte60hz") != NULL) {
        for (int i = 0; i < length; i++) {
            if (strstr(supportedColorList, colorList[i]) != NULL) {
                if (isModeSupportDeepColorAttr(hdmi_mode, colorList[i])) {
                    return true;
                }
            }
        }
        return false;
    }

    return true;
}

bool FormatColorDepth::getBootEnv(const char* key, char* value) {
    const char* p_value = mUbootenv->getValue(key);

    if (p_value) {
        SYS_LOGI("getBootEnv key:%s value:%s", key, p_value);
        strcpy(value, p_value);
        return true;
    }
    return false;
}
