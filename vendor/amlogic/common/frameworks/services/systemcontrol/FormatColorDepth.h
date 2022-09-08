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
 *  @author   Jinping Wang
 *  @version  2.0
 *  @date     2017/01/24
 *  @par function description:
 *  - 1 HDMI deep color attribute
 */

#ifndef FORMATCOLORDEPTH_H
#define FORMATCOLORDEPTH_H

#include "SysWrite.h"
#include "ubootenv/Ubootenv.h"
#include <string>
#include <map>

#define DISPLAY_HDMI_COLOR_ATTR         "/sys/class/amhdmitx/amhdmitx0/attr"//set deep color fmt and dept
#define DISPLAY_HDMI_VALID_MODE         "/sys/class/amhdmitx/amhdmitx0/valid_mode"//testing if tv support this displaymode and  deepcolor combination, then if cat result is 1: support, 0: not
#define PROP_DEFAULT_COLOR              "ro.vendor.platform.default_color"
#define LOW_POWER_DEFAULT_COLOR        "ro.vendor.low_power_default_color"  /* Set default deep color is 8bit for Low Power Mode */
#define UBOOTENV_COLORATTRIBUTE         "ubootenv.var.colorattribute"


#define COLOR_YCBCR444_12BIT             "444,12bit"
#define COLOR_YCBCR444_10BIT             "444,10bit"
#define COLOR_YCBCR444_8BIT              "444,8bit"
#define COLOR_YCBCR422_12BIT             "422,12bit"
#define COLOR_YCBCR422_10BIT             "422,10bit"
#define COLOR_YCBCR422_8BIT              "422,8bit"
#define COLOR_YCBCR420_12BIT             "420,12bit"
#define COLOR_YCBCR420_10BIT             "420,10bit"
#define COLOR_YCBCR420_8BIT              "420,8bit"
#define COLOR_RGB_12BIT                  "rgb,12bit"
#define COLOR_RGB_10BIT                  "rgb,10bit"
#define COLOR_RGB_8BIT                   "rgb,8bit"

/* In HDMI 2.1 CTS, the blocks of EDID data will increase to 4 blocks,
 * and the binary data size will be 128 x 4 = 512bytes
 * So cat /sys/class/amhdmitx/amhdmitx0/rawedid will be double
 * to 1024, plus the '\0' char.
 */
#define EDID_SIZE 1025

class FormatColorDepth
{
public:
    FormatColorDepth(Ubootenv *ubootenv);
    ~FormatColorDepth();
    void getHdmiColorAttribute(const char *outputmode, char * colorAttribute, int state);
    bool isModeSupportDeepColorAttr(const char *mode, const char * color);
    bool isSupportHdmiMode(const char *hdmi_mode, const char *supportedColorList);
    void getBestHdmiDeepColorAttr(const char *outputmode, char *colorAttribute);
    void setFilterEdidList(std::map<int, std::string> filterEdidList);
    bool initColorAttribute(char* supportedColorList, int len);
private:
    bool getBootEnv(const char* key, char* value);

    Ubootenv *mUbootenv;
    void getProperHdmiColorArrtibute(const char * outputmode, char * colorAttribute);
    bool isFilterEdid();
    SysWrite mSysWrite;
    std::map<int, std::string> mFilterEdid;
};
#endif //FORMATCOLORDEPTH_H
