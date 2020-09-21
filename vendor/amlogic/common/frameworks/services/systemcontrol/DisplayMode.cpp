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
 *  @date     2014/10/23
 *  @par function description:
 *  - 1 set display mode
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <cutils/properties.h>
#include "DisplayMode.h"
#include "SysTokenizer.h"

#ifndef RECOVERY_MODE
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

using namespace android;
#endif

static const char* DISPLAY_MODE_LIST[DISPLAY_MODE_TOTAL] = {
    MODE_480I,
    MODE_480P,
    MODE_480CVBS,
    MODE_576I,
    MODE_576P,
    MODE_576CVBS,
    MODE_720P,
    MODE_720P50HZ,
    MODE_1080P24HZ,
    MODE_1080I50HZ,
    MODE_1080P50HZ,
    MODE_1080I,
    MODE_1080P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
    MODE_4K2KSMPTE,
    MODE_4K2KSMPTE30HZ,
    MODE_4K2KSMPTE50HZ,
    MODE_4K2KSMPTE60HZ,
    MODE_768P,
    MODE_PANEL,
    MODE_PAL_M,
    MODE_PAL_N,
    MODE_NTSC_M,
};
static const char* MODE_RESOLUTION_FIRST[] = {
    MODE_480I,
    MODE_576I,
    MODE_480P,
    MODE_576P,
    MODE_720P50HZ,
    MODE_720P,
    MODE_1080P50HZ,
    MODE_1080P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
};
static const char* MODE_FRAMERATE_FIRST[] = {
    MODE_480I,
    MODE_576I,
    MODE_480P,
    MODE_576P,
    MODE_720P50HZ,
    MODE_720P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_1080P50HZ,
    MODE_1080P,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
};

// Sink reference table, sorted by priority, per CDF
static const char* MODES_SINK[] = {
    "2160p60hz",
    "2160p50hz",
    "2160p30hz",
    "2160p25hz",
    "2160p24hz",
    "1080p60hz",
    "1080p50hz",
    "1080p30hz",
    "1080p25hz",
    "1080p24hz",
    "720p60hz",
    "720p50hz",
    "480p60hz",
    "576p50hz",
};

// Repeater reference table, sorted by priority, per CDF
static const char* MODES_REPEATER[] = {
    "1080p60hz",
    "1080p50hz",
    "1080p30hz",
    "1080p25hz",
    "1080p24hz",
    "720p60hz",
    "720p50hz",
    "480p60hz",
    "576p50hz",
};

static const char* DV_MODE_TYPE[] = {
    "DV_RGB_444_8BIT",
    "DV_YCbCr_422_12BIT",
    "LL_YCbCr_422_12BIT",
    "LL_RGB_444_12BIT",
    "LL_RGB_444_10BIT"
};

/**
 * strstr - Find the first substring in a %NUL terminated string
 * @s1: The string to be searched
 * @s2: The string to search for
 */
char *_strstr(const char *s1, const char *s2)
{
    size_t l1, l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = strlen(s1);
    while (l1 >= l2) {
        l1--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

static void copy_if_gt0(uint32_t *src, uint32_t *dst, unsigned cnt)
{
    do {
        if ((int32_t) *src > 0)
            *dst = *src;
        src++;
        dst++;
    } while (--cnt);
}

static void copy_changed_values(
            struct fb_var_screeninfo *base,
            struct fb_var_screeninfo *set)
{
    //if ((int32_t) set->xres > 0) base->xres = set->xres;
    //if ((int32_t) set->yres > 0) base->yres = set->yres;
    //if ((int32_t) set->xres_virtual > 0)   base->xres_virtual = set->xres_virtual;
    //if ((int32_t) set->yres_virtual > 0)   base->yres_virtual = set->yres_virtual;
    copy_if_gt0(&set->xres, &base->xres, 4);

    if ((int32_t) set->bits_per_pixel > 0) base->bits_per_pixel = set->bits_per_pixel;
    //copy_if_gt0(&set->bits_per_pixel, &base->bits_per_pixel, 1);

    //if ((int32_t) set->pixclock > 0)       base->pixclock = set->pixclock;
    //if ((int32_t) set->left_margin > 0)    base->left_margin = set->left_margin;
    //if ((int32_t) set->right_margin > 0)   base->right_margin = set->right_margin;
    //if ((int32_t) set->upper_margin > 0)   base->upper_margin = set->upper_margin;
    //if ((int32_t) set->lower_margin > 0)   base->lower_margin = set->lower_margin;
    //if ((int32_t) set->hsync_len > 0) base->hsync_len = set->hsync_len;
    //if ((int32_t) set->vsync_len > 0) base->vsync_len = set->vsync_len;
    //if ((int32_t) set->sync > 0)  base->sync = set->sync;
    //if ((int32_t) set->vmode > 0) base->vmode = set->vmode;
    copy_if_gt0(&set->pixclock, &base->pixclock, 9);
}

DisplayMode::DisplayMode(const char *path, Ubootenv *ubootenv)
    :mDisplayType(DISPLAY_TYPE_MBOX),
    mDisplayWidth(FULL_WIDTH_1080),
    mDisplayHeight(FULL_HEIGHT_1080),
    mLogLevel(LOG_LEVEL_DEFAULT) {

    if (NULL == path) {
        pConfigPath = DISPLAY_CFG_FILE;
    }
    else {
        pConfigPath = path;
    }

    if (NULL == ubootenv)
        mUbootenv = new Ubootenv();
    else
        mUbootenv = ubootenv;
    pmDeepColor = new FormatColorDepth(mUbootenv);

    SYS_LOGI("display mode config path: %s", pConfigPath);
    pSysWrite = new SysWrite();
}

DisplayMode::~DisplayMode() {
    delete pSysWrite;
    delete pmDeepColor;
}

void DisplayMode::init() {
    parseConfigFile();
    parseFilterEdidConfigFile();
    pFrameRateAutoAdaption = new FrameRateAutoAdaption(this);

    getBootEnv(UBOOTENV_REBOOT_MODE, mRebootMode);
    SYS_LOGI("reboot_mode :%s\n", mRebootMode);

    SYS_LOGI("display mode init type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s, default UI:%s",
        mDisplayType, mSocType, mDefaultUI);

    if ((pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false))
            && (access(DOLBY_VISION_KO_DIR, 0) == 0)) {
        pSysWrite->setProperty(PROP_SUPPORT_DOLBY_VISION, "true");
    } else {
        pSysWrite->setProperty(PROP_SUPPORT_DOLBY_VISION, "false");
    }
    if (DISPLAY_TYPE_MBOX == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        setSourceDisplay(OUPUT_MODE_STATE_INIT);
        dumpCaps();
    } else if (DISPLAY_TYPE_TV == mDisplayType) {
        setTvModelName();
#ifndef RECOVERY_MODE
        SYS_LOGI("init: not RECOVERY_MODE\n");
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        pRxAuth = new HDCPRxAuth(pTxAuth);
        setSinkDisplay(true);
#else
        setTvRecoveryDisplay();
#endif
    } else if (DISPLAY_TYPE_TABLET == mDisplayType) {
        #ifdef HWC_DYNAMIC_SWITCH_VIU
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        dumpCaps();
        #endif
    } else if (DISPLAY_TYPE_REPEATER == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        pRxAuth = new HDCPRxAuth(pTxAuth);
        setSourceDisplay(OUPUT_MODE_STATE_INIT);
        dumpCaps();
    }
}

void DisplayMode::reInit() {
    char boot_type[MODE_LEN] = {0};
    /*
     * boot_type would be "normal", "fast", "snapshotted", or "instabooting"
     * "normal": normal boot, the boot_type can not be it here;
     * "fast": fast boot;
     * "snapshotted": this boot contains instaboot image making;
     * "instabooting": doing the instabooting operation, the boot_type can not be it here;
     * for fast boot, need to reinit the display, but for snapshotted, reInit display would make a screen flicker
     */
    pSysWrite->readSysfs(SYSFS_BOOT_TYPE, boot_type);
    if (strcmp(boot_type, "snapshotted")) {
        SYS_LOGI("display mode reinit type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s, default UI:%s",
            mDisplayType, mSocType, mDefaultUI);
        if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
            setSourceDisplay(OUPUT_MODE_STATE_POWER);
        } else if (DISPLAY_TYPE_TV == mDisplayType) {
            setSinkDisplay(false);
        }
    }

    SYS_LOGI("open osd0 and disable video\n");
    pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_AUTO_ENABLE);
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

void DisplayMode::setTvModelName() {
    char modelName[MODE_LEN] = {0};
    if (getBootEnv("ubootenv.var.model_name", modelName)) {
        pSysWrite->setProperty("tv.model_name", modelName);
    }
}

void DisplayMode::setRecoveryMode(bool isRecovery) {
    mIsRecovery = isRecovery;
}

HDCPTxAuth *DisplayMode:: geTxAuth() {
    return pTxAuth;
}

void DisplayMode::setLogLevel(int level){
    mLogLevel = level;
}

bool DisplayMode::getBootEnv(const char* key, char* value) {
    const char* p_value = mUbootenv->getValue(key);

    //if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("getBootEnv key:%s value:%s", key, p_value);

	if (p_value) {
        strcpy(value, p_value);
        return true;
	}
    return false;
}

void DisplayMode::setBootEnv(const char* key, char* value) {
    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("setBootEnv key:%s value:%s", key, value);

    mUbootenv->updateValue(key, value);
}

int DisplayMode::parseConfigFile(){
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(pConfigPath, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening display config file %s.", status, pConfigPath);
    } else {
        while (!tokenizer->isEof()) {

            if(mLogLevel > LOG_LEVEL_1)
                SYS_LOGI("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

            tokenizer->skipDelimiters(WHITESPACE);

            if (!tokenizer->isEol() && tokenizer->peekChar() != '#') {

                char *token = tokenizer->nextToken(WHITESPACE);
                if (!strcmp(token, DEVICE_STR_MBOX)) {
                    mDisplayType = DISPLAY_TYPE_MBOX;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                } else if (!strcmp(token, DEVICE_STR_TV)) {
                    mDisplayType = DISPLAY_TYPE_TV;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                } else if (!strcmp(token, DEVICE_STR_MID)) {
                    mDisplayType = DISPLAY_TYPE_TABLET;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                }else {
                    SYS_LOGE("%s: Expected keyword, got '%s'.", tokenizer->getLocation(), token);
                    break;
                }
            }

            tokenizer->nextLine();
        }
        delete tokenizer;
    }
    //if TVSOC as Mbox, change mDisplayType to DISPLAY_TYPE_REPEATER. and it will be in REPEATER process.
    if ((DISPLAY_TYPE_TV == mDisplayType) && (pSysWrite->getPropertyBoolean(PROP_TVSOC_AS_MBOX, false))) {
        mDisplayType = DISPLAY_TYPE_REPEATER;
    }
    return status;
}
int DisplayMode::parseFilterEdidConfigFile(){
    const char* WHITESPACE = " \t\r";
    unsigned int u32EdidCount = 0;
    SysTokenizer* tokenizer;
    std::string edid;
    int status = SysTokenizer::open(FILTER_EDID_CFG_FILE, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening display config file %s.", status, pConfigPath);
    } else {
        while (!tokenizer->isEof()) {

            if (mLogLevel > LOG_LEVEL_1)
                SYS_LOGI("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

            tokenizer->skipDelimiters(WHITESPACE);
            if (tokenizer->peekChar() == '*') {
                mFilterEdid = edid;
                mFilterEdidList[u32EdidCount++] = mFilterEdid;
                edid = "";
                SYS_LOGI("parseFilterEdidConfigFile EdidCount = %d, mFilterEdid = %s",u32EdidCount,mFilterEdid.c_str());
            }
            if (!tokenizer->isEol() && tokenizer->peekChar() != '*' && tokenizer->peekChar() != '#') {
                char *token = tokenizer->nextToken(WHITESPACE);
                edid += token;
            }
            tokenizer->nextLine();
        }
        delete tokenizer;
    }
    pmDeepColor->setFilterEdidList(mFilterEdidList);
    return status;
}

void DisplayMode::setTvRecoveryDisplay() {
    SYS_LOGI("setTvRecoveryDisplay\n");
    char outputmode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE,outputmode);
    updateDefaultUI();

    usleep(1000000LL);
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
    //need close fb1, because uboot logo show in fb1
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);
    pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
    pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

void DisplayMode::setSourceDisplay(output_mode_state state) {
    hdmi_data_t data;
    char outputmode[MODE_LEN] = {0};

    pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_DISABLE);

    memset(&data, 0, sizeof(hdmi_data_t));
    getHdmiData(&data);
    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (HDMI_SINK_TYPE_NONE != data.sinkType) {
            if ((!strcmp(data.current_mode, MODE_480CVBS) || !strcmp(data.current_mode, MODE_576CVBS) || !strcmp(data.current_mode, MODE_PAL_M) || !strcmp(data.current_mode, MODE_PAL_N) || !strcmp(data.current_mode, MODE_NTSC_M)) && (OUPUT_MODE_STATE_INIT == state)) {
                pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
                pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
            }

            getHdmiOutputMode(outputmode, &data);
        } else {
            getBootEnv(UBOOTENV_CVBSMODE, outputmode);
        }
    } else {
        getBootEnv(UBOOTENV_OUTPUTMODE, outputmode);
    }

    //if the tv don't support current outputmode,then switch to best outputmode
    if (HDMI_SINK_TYPE_NONE == data.sinkType) {
        pSysWrite->writeSysfs(H265_DOUBLE_WRITE_MODE, "3");
        //if (strcmp(outputmode, MODE_480CVBS) && strcmp(outputmode, MODE_576CVBS))
        //    strcpy(outputmode, MODE_576CVBS);

        if (strlen(outputmode) == 0)
            strcpy(outputmode, "none");
    } else {
        pSysWrite->writeSysfs(H265_DOUBLE_WRITE_MODE, "0");
    }

    SYS_LOGI("display sink type:%d [0:none, 1:sink, 2:repeater], old outputmode:%s, new outputmode:%s\n",
            data.sinkType,
            data.current_mode,
            outputmode);
    if (strlen(outputmode) == 0)
        strcpy(outputmode, DEFAULT_OUTPUT_MODE);

    if (OUPUT_MODE_STATE_INIT == state) {
        updateDefaultUI();
    }

    //output mode not the same
    if (strcmp(data.current_mode, outputmode)) {
        if (OUPUT_MODE_STATE_INIT == state) {
            //when change mode, need close uboot logo to avoid logo scaling wrong
            pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
            pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
            pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
        }
    }
    DetectDolbyVisionOutputMode(state, outputmode);
    setSourceOutputMode(outputmode, state);
}

bool DisplayMode::getModeSupportDeepColorAttr(const char* outputmode,const char * color){
    bool ret;
    if (outputmode == NULL || color == NULL) {
        SYS_LOGI("outputmode or color is NULL");
        return false;
    }
    ret = pmDeepColor->isModeSupportDeepColorAttr(outputmode,color);
    return ret;
}

void DisplayMode::setSourceOutputMode(const char* outputmode){
    setSourceOutputMode(outputmode, OUPUT_MODE_STATE_SWITCH);
}

void DisplayMode::setSourceOutputMode(const char* outputmode, output_mode_state state) {
    char value[MAX_STR_LEN] = {0};

    bool cvbsMode = false;
    char tmpMode[MODE_LEN] = {0};
    /* not enable phy in systemcontrol by default
     * as phy will be enabled in driver when set mode
     * only enable phy if phy is disabled but not enabled
     */
    bool phy_enabled_already = true;
    if (!strcmp(outputmode, "auto")) {
        hdmi_data_t data;

        SYS_LOGI("outputmode is [auto] mode, need find the best mode\n");
        getHdmiData(&data);
        getHdmiOutputMode((char *)outputmode, &data);
    }

    bool deepColorEnabled = pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, true);
    pSysWrite->readSysfs(HDMI_TX_FRAMRATE_POLICY, value);
    char curDisplayMode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
    if ((OUPUT_MODE_STATE_SWITCH == state) && (strcmp(value, "0") == 0)) {
        if (!strcmp(outputmode, curDisplayMode)) {

            //if cur mode is cvbsmode, and same to outputmode, return.
            if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS) || !strcmp(outputmode, MODE_PANEL) || !strcmp(outputmode, MODE_PAL_M) || !strcmp(outputmode, MODE_PAL_N) || !strcmp(outputmode, MODE_NTSC_M) || !strcmp(outputmode, "null")) {
                return;
            }
            //deep color disabled, only need check output mode same or not
            if (!deepColorEnabled) {
                SYS_LOGI("deep color is Disabled, and curDisplayMode is same to outputmode, return\n");
                return;
            }

            //deep color enabled, check the deep color same or not
            char curColorAttribute[MODE_LEN] = {0};
            char saveColorAttribute[MODE_LEN] = {0};
            pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, curColorAttribute);
            getBootEnv(UBOOTENV_COLORATTRIBUTE, saveColorAttribute);
            //if bestOutputmode is enable, need change deepcolor to best deepcolor.
            if (isBestOutputmode()) {
                pmDeepColor->getBestHdmiDeepColorAttr(outputmode, saveColorAttribute);
            }
            SYS_LOGI("curColorAttribute:[%s] ,saveColorAttribute: [%s]\n", curColorAttribute, saveColorAttribute);
            if (NULL != strstr(curColorAttribute, saveColorAttribute))
                return;
        }
    }
    // 1.set avmute and close phy
    if (OUPUT_MODE_STATE_INIT != state) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "1");
        if (OUPUT_MODE_STATE_POWER != state) {
            usleep(80000);//80ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_HDCP_MODE, "-1");
            //usleep(100000);//100ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
            phy_enabled_already = false;
            usleep(50000);//50ms
        }
    }

    // 2.stop hdcp tx
    pTxAuth->stop();

    if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS) || !strcmp(outputmode, MODE_PANEL) || !strcmp(outputmode, MODE_PAL_M) || !strcmp(outputmode, MODE_PAL_N) || !strcmp(outputmode, MODE_NTSC_M) || !strcmp(outputmode, "null")) {
        cvbsMode = true;
    }

    //write framerate policy
    if (!cvbsMode) {
        setAutoSwitchFrameRate(state);
    }

    // 3. set deep color and outputmode
    updateDeepColor(cvbsMode, state, outputmode);
    char curMode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curMode);

    if (strstr(mRebootMode, "quiescent") && (strstr(outputmode, MODE_PANEL) == NULL)) {
        SYS_LOGI("reboot_mode is quiescent\n");
        #ifndef HWC_DYNAMIC_SWITCH_VIU
        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
        #endif
        return;
    }
    SYS_LOGI("curMode = %s outputmode = %s",curMode,outputmode);
    if (strstr(curMode, outputmode) == NULL) {
        if (cvbsMode && (strstr(outputmode, MODE_PANEL) == NULL)) {
            #ifndef HWC_DYNAMIC_SWITCH_VIU
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
            #endif
        }
        #ifdef HWC_DYNAMIC_SWITCH_VIU
        if (DISPLAY_TYPE_TABLET == mDisplayType &&
            !strcmp("panel", curDisplayMode)) {
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE2, outputmode);
        } else {
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, outputmode);
        }
        #else
        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, outputmode);
        #endif
        /* phy already turned on after write display/mode node */
        phy_enabled_already = true;
    } else {
        //On a normal TV that supports I system, the screen will go black when the power is cut off and reconnect to the DV TV
        if (isDolbyVisionEnable() && isTvSupportDolbyVision(tmpMode) && strstr(curMode,"i")) {
            char highestMode[MODE_LEN] = {0};
            hdmi_data_t data;
            memset(&data, 0, sizeof(hdmi_data_t));
            getHdmiData(&data);
            getHighestHdmiMode(highestMode, &data);
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, highestMode);
        } else {
            SYS_LOGI("cur display mode is equals to outputmode, Do not need set it\n");
        }
    }
    if (pSysWrite->getPropertyBoolean(PROP_DISPLAY_SIZE_CHECK, true)) {
        char resolution[MODE_LEN] = {0};
        char defaultResolution[MODE_LEN] = {0};
        char finalResolution[MODE_LEN] = {0};
        int w = 0, h = 0, w1 =0, h1 = 0;
        pSysWrite->readSysfs(SYS_DISPLAY_RESOLUTION, resolution);
        pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
        sscanf(resolution, "%dx%d", &w, &h);
        sscanf(defaultResolution, "%dx%d", &w1, &h1);
        if ((w != w1) || (h != h1)) {
            sprintf(finalResolution, "%dx%d", w, h);
            pSysWrite->setProperty(PROP_DISPLAY_SIZE, finalResolution);
        }
    }

    char defaultResolution[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
    SYS_LOGI("set display-size:%s\n", defaultResolution);

    // no need to update
    // update free_scale_axis and window_axis in recovery mode
#ifdef RECOVERY_MODE
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);
#endif

    if (!cvbsMode && (pSysWrite->getPropertyBoolean(PROP_SUPPORT_DOLBY_VISION, false) == false)) {
        if (pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false)) {
            char hdr_policy[MODE_LEN] = {0};
            getHdrStrategy(hdr_policy);
            setHdrStrategy(hdr_policy);
        } else {
            initHdrSdrMode();
        }
    }
    /*if (0 == pSysWrite->getPropertyInt(PROP_BOOTCOMPLETE, 0)) {
        setVideoPlayingAxis();
    }*/

    SYS_LOGI("setMboxOutputMode cvbsMode = %d\n", cvbsMode);
    //4. turn on phy and clear avmute
    if (OUPUT_MODE_STATE_INIT != state && !cvbsMode) {
        if (!phy_enabled_already) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
            usleep(20000);
        }
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "0");
        if ((state == OUPUT_MODE_STATE_SWITCH) && isDolbyVisionEnable())
            usleep(20000);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
    }

    //5. start HDMI HDCP authenticate
    if (!cvbsMode) {
        pTxAuth->start();
        pTxAuth->setBootAnimFinished(true);
    }

    if (OUPUT_MODE_STATE_INIT == state) {
        startBootanimDetectThread();

        /* char bootvideo[MODE_LEN] = {0};
        pSysWrite->getPropertyString(PROP_BOOTVIDEO_SERVICE, bootvideo, "0");
        if (strcmp(bootvideo, "1") == 0) {
            startBootvideoDetectThread();
        }*/
        if (!cvbsMode) {
            pSysWrite->readSysfs(DISPLAY_EDID_RAW, mEdid);
        }
    } else {
        pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_ENABLE);
    }

#ifndef RECOVERY_MODE
    notifyEvent(EVENT_OUTPUT_MODE_CHANGE);
#endif

    if (!cvbsMode && pSysWrite->getPropertyBoolean(PROP_SUPPORT_DOLBY_VISION, false)
            && setDolbyVisionState) {
        initDolbyVision(state);
    }
    //audio
    memset(value, 0, sizeof(0));
    getBootEnv(UBOOTENV_DIGITAUDIO, value);
    setDigitalMode(value);

    char finalMode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, finalMode);
    if (DISPLAY_TYPE_TABLET != mDisplayType) {
        setBootEnv(UBOOTENV_OUTPUTMODE, (char *)finalMode);
    }
    if (strstr(finalMode, "cvbs") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, (char *)finalMode);
    } else if (strstr(finalMode, "hz") != NULL) {
        setBootEnv(UBOOTENV_HDMIMODE, (char *)finalMode);
    }
    SYS_LOGI("set output mode:%s done\n", finalMode);
}

void DisplayMode::setDigitalMode(const char* mode) {
    if (mode == NULL) return;

    if (!strcmp("PCM", mode)) {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "0");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    } else if (!strcmp("SPDIF passthrough", mode))  {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "1");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    } else if (!strcmp("HDMI passthrough", mode)) {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "2");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    }
}

void DisplayMode::setVideoPlayingAxis() {
    char currMode[MODE_LEN] = {0};
    int currPos[4] = {0};//x,y,w,h

    int videoPlaying = pSysWrite->getPropertyInt(PROP_BOOTVIDEO_SERVICE, 0);
    if (videoPlaying == 0) {
        SYS_LOGI("video is not playing, don't need set video axis\n");
        return;
    }

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, currMode);
    getPosition(currMode, currPos);

    SYS_LOGD("set video playing axis currMode:%s\n", currMode);

    //scale down or up the native window position
    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            currPos[0], currPos[1], currPos[0] + currPos[2] - 1, currPos[1] + currPos[3] - 1);
    SYS_LOGD("write %s: %s\n", SYSFS_VIDEO_AXIS, axis);
    pSysWrite->writeSysfs(SYSFS_VIDEO_AXIS, axis);
}

int DisplayMode::readHdcpRX22Key(char *value, int size) {
    SYS_LOGI("read HDCP rx 2.2 key \n");
    HDCPRxKey key22(HDCP_RX_22_KEY);
    int ret = key22.getHdcpRX22key(value, size);
    return ret;
}

bool DisplayMode::writeHdcpRX22Key(const char *value, const int size) {
    SYS_LOGI("write HDCP rx 2.2 key \n");
    HDCPRxKey key22(HDCP_RX_22_KEY);
    int ret = key22.setHdcpRX22key(value, size);
    if (ret == 0)
        return true;
    else
        return false;
}

int DisplayMode::readHdcpRX14Key(char *value, int size) {
    SYS_LOGI("read HDCP rx 1.4 key \n");
    HDCPRxKey key14(HDCP_RX_14_KEY);
    int ret = key14.getHdcpRX14key(value, size);
    return ret;
}

bool DisplayMode::writeHdcpRX14Key(const char *value, const int size) {
    SYS_LOGI("write HDCP rx 1.4 key \n");
    HDCPRxKey key14(HDCP_RX_14_KEY);
    int ret = key14.setHdcpRX14key(value,size);
    if (ret == 0)
        return true;
    else
        return false;
}

bool DisplayMode::writeHdcpRXImg(const char *path) {
    SYS_LOGI("write HDCP key from Img \n");
    int ret = setImgPath(path);
    if (ret == 0)
        return true;
    else
        return false;
}


//get the best hdmi mode by edid
void DisplayMode::getBestHdmiMode(char* mode, hdmi_data_t* data) {
    char* pos = strchr(data->edid, '*');
    if (pos != NULL) {
        char* findReturn = pos;
        while (*findReturn != 0x0a && findReturn >= data->edid) {
            findReturn--;
        }
        //*pos = 0;
        //strcpy(mode, findReturn + 1);

        findReturn = findReturn + 1;
        strncpy(mode, findReturn, pos - findReturn);
        SYS_LOGI("set HDMI to best edid mode: %s\n", mode);
    }

    if (strlen(mode) == 0) {
        pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
    }

  /*
    char* arrayMode[MAX_STR_LEN] = {0};
    char* tmp;

    int len = strlen(data->edid);
    tmp = data->edid;
    int i = 0;

    do {
        if (strlen(tmp) == 0)
            break;
        char* pos = strchr(tmp, 0x0a);
        *pos = 0;

        arrayMode[i] = tmp;
        tmp = pos + 1;
        i++;
    } while (tmp <= data->edid + len -1);

    for (int j = 0; j < i; j++) {
        char* pos = strchr(arrayMode[j], '*');
        if (pos != NULL) {
            *pos = 0;
            strcpy(mode, arrayMode[j]);
            break;
        }
    }*/
}

//get the highest hdmi mode by edid
void DisplayMode::getHighestHdmiMode(char* mode, hdmi_data_t* data) {
    char value[MODE_LEN] = {0};
    char tempMode[MODE_LEN] = {0};

    char* startpos;
    char* destpos;

    startpos = data->edid;
    strcpy(value, DEFAULT_OUTPUT_MODE);

    while (strlen(startpos) > 0) {
        //get edid resolution to tempMode in order.
        destpos = strstr(startpos, "\n");
        if (NULL == destpos)
            break;
        memset(tempMode, 0, MODE_LEN);
        strncpy(tempMode, startpos, destpos - startpos);
        startpos = destpos + 1;
        if (!pSysWrite->getPropertyBoolean(PROP_SUPPORT_4K, true)
            &&(strstr(tempMode, "2160") || strstr(tempMode, "smpte"))) {
                SYS_LOGE("This platform not support : %s\n", tempMode);
            continue;
        }

        if (tempMode[strlen(tempMode) - 1] == '*') {
            tempMode[strlen(tempMode) - 1] = '\0';
        }

        if (!pSysWrite->getPropertyBoolean(PROP_SUPPORT_OVER_4K30, true)
                &&(resolveResolutionValue(tempMode) > resolveResolutionValue("2160p30hz")
                    || strstr(tempMode, "smpte"))) {
            SYS_LOGE("This platform not support the mode over 2160p30hz, current mode is:[%s]\n", tempMode);
            continue;
        }
        if (resolveResolutionValue(tempMode) > resolveResolutionValue(value)) {
            memset(value, 0, MODE_LEN);
            strcpy(value, tempMode);
        }
    }

    strcpy(mode, value);
    SYS_LOGI("set HDMI to highest edid mode: %s\n", mode);
}

int64_t DisplayMode::resolveResolutionValue(const char *mode) {
    bool validMode = false;
    if (strlen(mode) != 0) {
        for (int i = 0; i < DISPLAY_MODE_TOTAL; i++) {
            if (strcmp(mode, DISPLAY_MODE_LIST[i]) == 0) {
                validMode = true;
                break;
            }
        }
    }
    if (!validMode) {
        SYS_LOGI("the resolveResolution mode [%s] is not valid\n", mode);
        return -1;
    }

    if (pSysWrite->getPropertyBoolean(PROP_HDMI_FRAMERATE_PRIORITY, false)) {
        for (int64_t index = 0; index < sizeof(MODE_FRAMERATE_FIRST)/sizeof(char *); index++) {
            if (strcmp(mode, MODE_FRAMERATE_FIRST[index]) == 0) {
                return index;
            }
        }
    } else {
        for (int64_t index = 0; index < sizeof(MODE_RESOLUTION_FIRST)/sizeof(char *); index++) {
            if (strcmp(mode, MODE_RESOLUTION_FIRST[index]) == 0) {
                return index;
            }
        }
    }
    return -1;
}

//get the highest priority mode defined by CDF table
void DisplayMode::getHighestPriorityMode(char* mode, hdmi_data_t* data) {
    char **pMode = NULL;
    int modeSize = 0;

    if (HDMI_SINK_TYPE_SINK == data->sinkType) {
        pMode= (char **)MODES_SINK;
        modeSize = ARRAY_SIZE(MODES_SINK);
    }
    else if (HDMI_SINK_TYPE_REPEATER == data->sinkType) {
        pMode= (char **)MODES_REPEATER;
        modeSize = ARRAY_SIZE(MODES_REPEATER);
    }

    for (int i = 0; i < modeSize; i++) {
        if (strstr(data->edid, pMode[i]) != NULL) {
            strcpy(mode, pMode[i]);
            return;
        }
    }

    pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
}

//check if the edid support current hdmi mode
void DisplayMode::filterHdmiMode(char* mode, hdmi_data_t* data) {
    char *pCmp = data->edid;
    while ((pCmp - data->edid) < (int)strlen(data->edid)) {
        char *pos = strchr(pCmp, 0x0a);
        if (NULL == pos)
            break;

        int step = 1;
        if (*(pos - 1) == '*') {
            pos -= 1;
            step += 1;
        }
        if (!strncmp(pCmp, data->ubootenv_hdmimode, pos - pCmp)) {
            strcpy(mode, data->ubootenv_hdmimode);
            return;
        }
        pCmp = pos + step;
    }
    if (DISPLAY_TYPE_TV == mDisplayType) {
        #ifdef TEST_UBOOT_MODE
            getBootEnv(UBOOTENV_TESTMODE, mode);
            if (strlen(mode) != 0)
               return;
        #endif
    }
    //old mode is not support in this TV, so switch to best mode.
#ifdef USE_BEST_MODE
    getBestHdmiMode(mode, data);
#else
    getHighestHdmiMode(mode, data);
    //getHighestPriorityMode(mode, data);
#endif
}

void DisplayMode::getHdmiOutputMode(char* mode, hdmi_data_t* data) {
    char edidParsing[MODE_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidParsing);

    /* Fall back to 480p if EDID can't be parsed */
    if (strcmp(edidParsing, "ok")) {
        strcpy(mode, DEFAULT_OUTPUT_MODE);
        SYS_LOGE("EDID parsing error detected\n");
        return;
    }

    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (isBestOutputmode()) {
        #ifdef USE_BEST_MODE
            getBestHdmiMode(mode, data);
        #else
            getHighestHdmiMode(mode, data);
            //getHighestPriorityMode(mode, data);
        #endif
        } else {
            filterHdmiMode(mode, data);
        }
    }
    //SYS_LOGI("set HDMI mode to %s\n", mode);
}

void DisplayMode::getHdmiData(hdmi_data_t* data) {
    char sinkType[MODE_LEN] = {0};
    char edidParsing[MODE_LEN] = {0};

    //three sink types: sink, repeater, none
    pSysWrite->readSysfsOriginal(DISPLAY_HDMI_SINK_TYPE, sinkType);
    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidParsing);

    data->sinkType = HDMI_SINK_TYPE_NONE;
    if (NULL != strstr(sinkType, "sink"))
        data->sinkType = HDMI_SINK_TYPE_SINK;
    else if (NULL != strstr(sinkType, "repeater"))
        data->sinkType = HDMI_SINK_TYPE_REPEATER;

    if (HDMI_SINK_TYPE_NONE != data->sinkType) {
        int count = 0;
        while (true) {
            pSysWrite->readSysfsOriginal(DISPLAY_HDMI_EDID, data->edid);
            if (strlen(data->edid) > 0)
                break;

            if (count >= 5) {
                strcpy(data->edid, "null edid");
                break;
            }
            count++;
            usleep(500000);
        }
    }
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, data->current_mode);
    getBootEnv(UBOOTENV_HDMIMODE, data->ubootenv_hdmimode);

    //filter mode defined by CDF, default disable this
    if (!strcmp(edidParsing, "ok") && false) {
        const char *delim = "\n";
        char filterEdid[MAX_STR_LEN] = {0};

        char *ptr = strtok(data->edid, delim);
        while (ptr != NULL) {
            //recommend mode or not
            bool recomMode = false;
            int len = strlen(ptr);
            if (ptr[len - 1] == '*') {
                ptr[len - 1] = '\0';
                recomMode = true;
            }

            if (modeSupport(ptr, data->sinkType)) {
                strcat(filterEdid, ptr);
                if (recomMode)
                    strcat(filterEdid, "*");
                strcat(filterEdid, delim);
            }
            ptr = strtok(NULL, delim);
        }

        //this is the real support edid filter by CDF
        strcpy(data->edid, filterEdid);

        SYS_LOGI("CDF filtered modes: %s\n", data->edid);
    }
}

bool DisplayMode::modeSupport(char *mode, int sinkType) {
    char **pMode = NULL;
    int modeSize = 0;

    if (HDMI_SINK_TYPE_SINK == sinkType) {
        pMode= (char **)MODES_SINK;
        modeSize = ARRAY_SIZE(MODES_SINK);
    }
    else if (HDMI_SINK_TYPE_REPEATER == sinkType) {
        pMode= (char **)MODES_REPEATER;
        modeSize = ARRAY_SIZE(MODES_REPEATER);
    }

    for (int i = 0; i < modeSize; i++) {
        //SYS_LOGI("modeSupport mode=%s, filerMode:%s, size:%d\n", mode, pMode[i], modeSize);
        if (!strcmp(mode, pMode[i]))
            return true;
    }

    return false;
}

void DisplayMode::startBootanimDetectThread() {
    pthread_t id;
    int ret = pthread_create(&id, NULL, bootanimDetect, this);
    if (ret != 0) {
        SYS_LOGE("Create BootanimDetect error!\n");
    }
}

//if detected bootanim is running, then close uboot logo
void* DisplayMode::bootanimDetect(void* data) {
    DisplayMode *pThiz = (DisplayMode*)data;
    char fs_mode[MODE_LEN] = {0};
    char recovery_state[MODE_LEN] = {0};
    char bootvideo[MODE_LEN] = {0};

    if (pThiz->mIsRecovery) {
        SYS_LOGI("this is recovery mode");
        usleep(1000000LL);
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        //need close fb1, because uboot logo show in fb1
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
    } else {
        SYS_LOGI("this is android mode");
    }

    return NULL;
}

void DisplayMode::startBootvideoDetectThread() {
    pthread_t id;
    int ret = pthread_create(&id, NULL, bootvideoDetect, this);
    if (ret != 0) {
        SYS_LOGE("Create Bootvideo error!\n");
    }
}

void* DisplayMode::bootvideoDetect(void* data) {
    DisplayMode *pThiz = (DisplayMode*)data;
    int timeout = 1200;
    char value[MODE_LEN] = {0};
    SYS_LOGI("Need to setBootanimStatus 1 after bootvideo complete");
    while (timeout > 0) {
        pThiz->pSysWrite->getProperty("init.svc.bootvideo", value);
        if (strstr(value, "stopped") != NULL) {
            pThiz->setBootanimStatus(1);
            return NULL;
        }
        usleep(100000);
        timeout--;
    }
    pThiz->setBootanimStatus(1);
    return NULL;
}

//get edid crc value to check edid change
bool DisplayMode::isEdidChange() {
    char edid[MAX_STR_LEN] = {0};
    char crcvalue[MAX_STR_LEN] = {0};
    unsigned int crcheadlength = strlen(DEFAULT_EDID_CRCHEAD);
    pSysWrite->readSysfs(DISPLAY_EDID_VALUE, edid);
    char *p = strstr(edid, DEFAULT_EDID_CRCHEAD);
    if (p != NULL && strlen(p) > crcheadlength) {
        p += crcheadlength;
        if (!getBootEnv(UBOOTENV_EDIDCRCVALUE, crcvalue) || strncmp(p, crcvalue, strlen(p))) {
            SYS_LOGI("update edidcrc: %s->%s\n", crcvalue, p);
            setBootEnv(UBOOTENV_EDIDCRCVALUE, p);
            return true;
        }
    }
    return false;
}

bool DisplayMode::isBestOutputmode() {
    char isBestMode[MODE_LEN] = {0};
    if (DISPLAY_TYPE_TV == mDisplayType) {
        return false;
    }
    return !getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) || strcmp(isBestMode, "true") == 0;
}

void DisplayMode::setSinkOutputMode(const char* outputmode) {
    setSinkOutputMode(outputmode, false);
}

void DisplayMode::setSinkOutputMode(const char* outputmode, bool initState) {
    SYS_LOGI("set sink output mode:%s, init state:%d\n", outputmode, initState?1:0);

    setSourceOutputMode(outputmode, initState?OUPUT_MODE_STATE_INIT:OUPUT_MODE_STATE_SWITCH);
}

void DisplayMode::setSinkDisplay(bool initState) {
    char current_mode[MODE_LEN] = {0};
    char outputmode[MODE_LEN] = {0};

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, current_mode);
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode);
    SYS_LOGD("init tv display old outputmode:%s, outputmode:%s\n", current_mode, outputmode);

    if (strlen(outputmode) == 0)
        strcpy(outputmode, mDefaultUI);

    updateDefaultUI();
    if (strcmp(current_mode, outputmode)) {
        //when change mode, need close uboot logo to avoid logo scaling wrong
        pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
    }

    setSinkOutputMode(outputmode, initState);
}

int DisplayMode::getBootenvInt(const char* key, int defaultVal) {
    int value = defaultVal;
    const char* p_value = mUbootenv->getValue(key);
    if (p_value) {
        value = atoi(p_value);
    }
    return value;
}

/*
 * *
 * @Description: select diff policy base on output mode state.
 * @params: outputmode state.
 * author: luan.yuan@amlogic.com
 *
 * only set 'null' to display/mode in switch adaper state.
 * auto switch frame rate need set 1 to /sys/class/amhdmitx/amhdmitx0/frac_rate_policy, to get CLK 0.1% offset.
 * But only change frac_rate_policy can not update CLOCK, unless mode and frac_rate_policy.
 * and can not set same mode to mode node. so need like 1080p60hz--->null--->1080p60hz.
 * this function will set mode to 'null', policy to 1, and set mode to previous value later.
 */
void DisplayMode::setAutoSwitchFrameRate(int state) {
//default force shift 0.001 clk, if you do not want to do that, open this marco.
//so you can control switch it from DroidTvSetings app.
//#define DEFAULT_NO_CLK_OFFSET
#ifdef DEFAULT_NO_CLK_OFFSET
    if ((state == OUPUT_MODE_STATE_SWITCH_ADAPTER) || pFrameRateAutoAdaption->autoSwitchFlag == true) {
        SYS_LOGI("FrameRate video need set mode to null, and policy to 1 to into adapter policy\n");
        #ifndef HWC_DYNAMIC_SWITCH_VIU
        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
        #endif
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLICY, "1");
    } else {
        if (state == OUPUT_MODE_STATE_ADAPTER_END) {
            SYS_LOGI("End Hint FrameRate video need set mode to null to exit adapter policy\n");
            #ifndef HWC_DYNAMIC_SWITCH_VIU
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
            #endif
        }
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLICY, "0");
    }
#endif
}

void DisplayMode::updateDefaultUI() {
    if (!strncmp(mDefaultUI, "720", 3)) {
        mDisplayWidth= FULL_WIDTH_720;
        mDisplayHeight = FULL_HEIGHT_720;
        //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_720P);
        //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1280");
        //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "720");
    } else if (!strncmp(mDefaultUI, "1080", 4)) {
        mDisplayWidth = FULL_WIDTH_1080;
        mDisplayHeight = FULL_HEIGHT_1080;
        //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_1080P);
        //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1920");
        //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "1080");
    } else if (!strncmp(mDefaultUI, "4k2k", 4)) {
        mDisplayWidth = FULL_WIDTH_4K2K;
        mDisplayHeight = FULL_HEIGHT_4K2K;
        //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_2160P);
        //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "3840");
        //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "2160");
    }
}

void DisplayMode::updateDeepColor(bool cvbsMode, output_mode_state state, const char* outputmode) {
    if (!cvbsMode && (mDisplayType != DISPLAY_TYPE_TV)) {
        char colorAttribute[MODE_LEN] = {0};
        if (pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, true)) {
            char mode[MAX_STR_LEN] = {0};
            if (isDolbyVisionEnable() && isTvSupportDolbyVision(mode)) {
                 char type[MODE_LEN] = {0};
                pSysWrite->getPropertyString(PROP_DOLBY_VISION_TV_TYPE, type, "1");
                if (((atoi(type) == 2) && (strstr(mode, DV_MODE_TYPE[2]) == NULL))
                        || ((atoi(type) == 3) && (strstr(mode, DV_MODE_TYPE[3]) == NULL)
                            && (strstr(mode, DV_MODE_TYPE[4]) == NULL))) {
                    strcpy(type, "1");
                }
                switch (atoi(type)) {
                    case DOLBY_VISION_SET_ENABLE:
                        strcpy(colorAttribute, "444,8bit");
                        break;
                    case DOLBY_VISION_SET_ENABLE_LL_YUV:
                        strcpy(colorAttribute, "422,12bit");
                        break;
                    case DOLBY_VISION_SET_ENABLE_LL_RGB:
                        if (strstr(mode, "LL_RGB_444_12BIT") != NULL) {
                            strcpy(colorAttribute, "444,12bit");
                        } else if (strstr(mode, "LL_RGB_444_10BIT") != NULL) {
                            strcpy(colorAttribute, "444,10bit");
                        }
                        break;
                }
            } else {
                pmDeepColor->getHdmiColorAttribute(outputmode, colorAttribute, (int)state);
            }
        } else {
            strcpy(colorAttribute, "default");
        }
        char attr[MODE_LEN] = {0};
        pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, attr);
        if (strstr(attr, colorAttribute) == NULL) {
            SYS_LOGI("set DeepcolorAttr value is different from attr sysfs value\n");
            #ifndef HWC_DYNAMIC_SWITCH_VIU
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
            #endif
            pSysWrite->writeSysfs(DISPLAY_HDMI_COLOR_ATTR, colorAttribute);
        } else {
            SYS_LOGI("cur deepcolor attr value is equals to colorAttribute, Do not need set it\n");
        }
        SYS_LOGI("setMboxOutputMode colorAttribute = %s\n", colorAttribute);
        //save to ubootenv
        saveDeepColorAttr(outputmode, colorAttribute);
        setBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
    }
}

void DisplayMode::updateFreeScaleAxis() {
    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            0, 0, mDisplayWidth - 1, mDisplayHeight - 1);
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE_AXIS, axis);
}

void DisplayMode::updateWindowAxis(const char* outputmode) {
    char axis[MAX_STR_LEN] = {0};
    int position[4] = { 0, 0, 0, 0 };//x,y,w,h
    getPosition(outputmode, position);
    sprintf(axis, "%d %d %d %d",
            position[0], position[1], position[0] + position[2] - 1, position[1] + position[3] -1);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);
}

void DisplayMode::setBootanimStatus(int status) {
    mBootanimStatus = status;
}

void DisplayMode::getBootanimStatus(int *status) {
    *status = mBootanimStatus;
    return;
}

void DisplayMode::getPosition(const char* curMode, int *position) {
    char keyValue[20] = {0};
    char ubootvar[100] = {0};
    int defaultWidth = 0;
    int defaultHeight = 0;
    if (strstr(curMode, MODE_480CVBS)) {
        strcpy(keyValue, MODE_480CVBS);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, "480")) {
        strcpy(keyValue, strstr(curMode, MODE_480P_PREFIX) ? MODE_480P_PREFIX : MODE_480I_PREFIX);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, MODE_576CVBS)) {
        strcpy(keyValue, MODE_576CVBS);
        defaultWidth = FULL_WIDTH_576;
        defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, "576")) {
        strcpy(keyValue, strstr(curMode, MODE_576P_PREFIX) ? MODE_576P_PREFIX : MODE_576I_PREFIX);
        defaultWidth = FULL_WIDTH_576;
        defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, MODE_PAL_M)) {
	strcpy(keyValue, MODE_PAL_M);
	defaultWidth = FULL_WIDTH_480;
	defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, MODE_PAL_N)) {
	strcpy(keyValue, MODE_PAL_N);
	defaultWidth = FULL_WIDTH_576;
	defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, MODE_NTSC_M)) {
	strcpy(keyValue, MODE_NTSC_M);
	defaultWidth = FULL_WIDTH_480;
	defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, MODE_720P_PREFIX)) {
        strcpy(keyValue, MODE_720P_PREFIX);
        defaultWidth = FULL_WIDTH_720;
        defaultHeight = FULL_HEIGHT_720;
    } else if (strstr(curMode, MODE_768P_PREFIX)) {
        strcpy(keyValue, MODE_768P_PREFIX);
        defaultWidth = FULL_WIDTH_768;
        defaultHeight = FULL_HEIGHT_768;
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
        defaultWidth = FULL_WIDTH_4K2K;
        defaultHeight = FULL_HEIGHT_4K2K;
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
        defaultWidth = FULL_WIDTH_4K2KSMPTE;
        defaultHeight = FULL_HEIGHT_4K2KSMPTE;
    } else if (strstr(curMode, MODE_PANEL)) {
        strcpy(keyValue, MODE_PANEL);
        defaultWidth = FULL_WIDTH_PANEL;
        defaultHeight = FULL_HEIGHT_PANEL;
    } else {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    }

    mutex_lock(&mEnvLock);
    sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
    position[0] = getBootenvInt(ubootvar, 0);
    sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
    position[1] = getBootenvInt(ubootvar, 0);
    sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
    position[2] = getBootenvInt(ubootvar, defaultWidth);
    sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
    position[3] = getBootenvInt(ubootvar, defaultHeight);
    mutex_unlock(&mEnvLock);

}

void DisplayMode::setPosition(int left, int top, int width, int height) {
    char x[512] = {0};
    char y[512] = {0};
    char w[512] = {0};
    char h[512] = {0};
    sprintf(x, "%d", left);
    sprintf(y, "%d", top);
    sprintf(w, "%d", width);
    sprintf(h, "%d", height);

    char curMode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curMode);

    char keyValue[20] = {0};
    char ubootvar[100] = {0};
    if (strstr(curMode, MODE_480CVBS)) {
        strcpy(keyValue, MODE_480CVBS);
    } else if (strstr(curMode, "480")) {
        strcpy(keyValue, strstr(curMode, MODE_480P_PREFIX) ? MODE_480P_PREFIX : MODE_480I_PREFIX);
    } else if (strstr(curMode, MODE_576CVBS)) {
        strcpy(keyValue, MODE_576CVBS);
    } else if (strstr(curMode, "576")) {
        strcpy(keyValue, strstr(curMode, MODE_576P_PREFIX) ? MODE_576P_PREFIX : MODE_576I_PREFIX);
    } else if (strstr(curMode, MODE_PAL_M)) {
	strcpy(keyValue, MODE_PAL_M);
    } else if (strstr(curMode, MODE_PAL_N)) {
	strcpy(keyValue, MODE_PAL_N);
    } else if (strstr(curMode, MODE_NTSC_M)) {
	strcpy(keyValue, MODE_NTSC_M);
    } else if (strstr(curMode, MODE_720P_PREFIX)) {
        strcpy(keyValue, MODE_720P_PREFIX);
    } else if (strstr(curMode, MODE_768P_PREFIX)) {
        strcpy(keyValue, MODE_768P_PREFIX);
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
    } else if (strstr(curMode, MODE_PANEL)) {
        strcpy(keyValue, MODE_PANEL);
    }

    mutex_lock(&mEnvLock);
    sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
    setBootEnv(ubootvar, x);
    sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
    setBootEnv(ubootvar, y);
    sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
    setBootEnv(ubootvar, w);
    sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
    setBootEnv(ubootvar, h);
    mutex_unlock(&mEnvLock);

}

void DisplayMode::saveDeepColorAttr(const char* mode, const char* dcValue) {
    char ubootvar[100] = {0};
    sprintf(ubootvar, "ubootenv.var.%s_deepcolor", mode);
    setBootEnv(ubootvar, (char *)dcValue);
}

void DisplayMode::getDeepColorAttr(const char* mode, char* value) {
    char ubootvar[100];
    sprintf(ubootvar, "ubootenv.var.%s_deepcolor", mode);
    if (!getBootEnv(ubootvar, value)) {
        //strcpy(value, (strstr(mode, "420") != NULL) ? DEFAULT_420_DEEP_COLOR_ATTR : DEFAULT_DEEP_COLOR_ATTR);
    }
    SYS_LOGI(": getDeepColorAttr [%s]", value);
}

/* *
 * @Description: Detect Whether TV support Dolby Vision
 * @return: if TV support return true, or false
 * if true, mode is the Highest resolution Tv Dolby Vision supported
 * else mode is ""
 */
bool DisplayMode::isTvSupportDolbyVision(char *mode) {
    char dv_cap[MAX_STR_LEN] = {0};
    strcpy(mode, "");
    if (DISPLAY_TYPE_TV == mDisplayType) {
        SYS_LOGI("Current Device is TV, no dv_cap\n");
        return false;
    }

    pSysWrite->readSysfs(DOLBY_VISION_IS_SUPPORT, dv_cap);
    if (strstr(dv_cap, "The Rx don't support DolbyVision")) {
        return false;
    }
    for (int i = DISPLAY_MODE_TOTAL - 1; i >= 0; i--) {
        if (strstr(dv_cap, DISPLAY_MODE_LIST[i]) != NULL) {
            strcat(mode, DISPLAY_MODE_LIST[i]);
            strcat(mode, ",");
            break;
        }
    }
    for (int i = 0; i < sizeof(DV_MODE_TYPE)/sizeof(DV_MODE_TYPE[0]); i++) {
        if (strstr(dv_cap, DV_MODE_TYPE[i])) {
            strcat(mode, DV_MODE_TYPE[i]);
            strcat(mode, ",");
        }
    }
    SYS_LOGI("Current Tv Support DV type [%s]", mode);
    return true;
}

bool DisplayMode::isDolbyVisionEnable() {
    char mode[MAX_STR_LEN] = {0};
    if (isTvSupportDolbyVision(mode)) {
        return pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_TV_ENABLE, false);
    } else {
        return pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_ENABLE, false);
    }
}

void DisplayMode::setDolbyVisionEnable(int state,  output_mode_state mode_state) {
    char tvmode[MAX_STR_LEN] = {0};
    char outputmode[MODE_LEN] = {0};
    char dvstatus[MODE_LEN] = {0};
    int value_state = state;
    int check_status_count = 0;
    char crcvalue[MAX_STR_LEN] = {0};
    bool dvStatusChanged = checkDolbyVisionStatusChanged(state);
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, outputmode);

    mDvStatus = value_state;
    SYS_LOGI("mDvStatus %d\n", mDvStatus);

    if (dvStatusChanged) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "1");
    }
    SYS_LOGI("Dolby vision set mode %d\n", value_state);
    if (DOLBY_VISION_SET_DISABLE != value_state) {
        if (isTvSupportDolbyVision(tvmode)) {
            pSysWrite->setProperty(PROP_DOLBY_VISION_TV_ENABLE, "true");
        } else {
            pSysWrite->setProperty(PROP_DOLBY_VISION_ENABLE, "true");
        }
        //if TV
        if (DISPLAY_TYPE_TV == mDisplayType) {
            setHdrMode(HDR_MODE_OFF);
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY_OLD, DV_POLICY_FOLLOW_SOURCE);
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY, DV_POLICY_FOLLOW_SOURCE);
        }

        //if OTT
        if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
            char mode[MAX_STR_LEN] = {0};
            if (isTvSupportDolbyVision(mode)) {
                SYS_LOGI("Tv is Support DolbyVision, highest mode is [%s]", mode);
                if (((value_state == 2) && (strstr(mode, DV_MODE_TYPE[2]) == NULL))
                        || ((value_state == 3) && (strstr(mode, DV_MODE_TYPE[3]) == NULL)
                            && (strstr(mode, DV_MODE_TYPE[4]) == NULL))) {
                    value_state = 1;
                }
                switch (value_state) {
                    case DOLBY_VISION_SET_ENABLE:
                        SYS_LOGI("Dolby Vision set Mode [DV_RGB_444_8BIT]\n");
                        pSysWrite->writeSysfs(DOLBY_VISION_LL_POLICY, "0");
                        setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,8bit");
                        break;
                    case DOLBY_VISION_SET_ENABLE_LL_YUV:
                        SYS_LOGI("Dolby Vision set Mode [LL_YCbCr_422_12BIT]\n");
                        pSysWrite->writeSysfs(DOLBY_VISION_LL_POLICY, "0");
                        setBootEnv(UBOOTENV_COLORATTRIBUTE, "422,12bit");
                        pSysWrite->writeSysfs(DOLBY_VISION_LL_POLICY, "1");
                        break;
                    case DOLBY_VISION_SET_ENABLE_LL_RGB:
                        pSysWrite->writeSysfs(DOLBY_VISION_LL_POLICY, "0");
                        if (strstr(mode, "LL_RGB_444_12BIT") != NULL) {
                            SYS_LOGI("Dolby Vision set Mode [LL_RGB_444_12BIT]");
                            setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,12bit");
                        } else if (strstr(mode, "LL_RGB_444_10BIT") != NULL) {
                            SYS_LOGI("Dolby Vision set Mode [LL_RGB_444_10BIT]");
                            setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,10bit");
                        }
                        pSysWrite->writeSysfs(DOLBY_VISION_LL_POLICY, "2");
                        break;
                    default:
                        pSysWrite->writeSysfs(DOLBY_VISION_LL_POLICY, "0");
                        setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,8bit");
                }
                char tmp[10];
                sprintf(tmp, "%d", value_state);
                if (isTvSupportDolbyVision(tvmode)) {
                    pSysWrite->setProperty(PROP_DOLBY_VISION_TV_TYPE, tmp);
                } else {
                    pSysWrite->setProperty(PROP_DOLBY_VISION_TYPE, tmp);
                }
                char tvmode[MODE_LEN] = {0};
                for (int i = DISPLAY_MODE_TOTAL - 1; i >= 0; i--) {
                    if (strstr(mode, DISPLAY_MODE_LIST[i]) != NULL) {
                        strcpy(tvmode, DISPLAY_MODE_LIST[i]);
                    }
                }
                setDolbyVisionState = false;
                char attr[MODE_LEN] = {0};
                char saveAttr[MODE_LEN] = {0};
                getBootEnv(UBOOTENV_COLORATTRIBUTE, saveAttr);
                pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, attr);
                if (strstr(attr, saveAttr) == NULL) {
                    #ifndef HWC_DYNAMIC_SWITCH_VIU
                    pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
                    #endif
                }

                if ((pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_CERTIFICATION, false))
                        && (mode_state != OUPUT_MODE_STATE_SWITCH)) {
                    if (strstr(tvmode, MODE_4K2K60HZ)) {
                        if (value_state == DOLBY_VISION_SET_ENABLE_LL_RGB) {
                            setSourceOutputMode(MODE_1080P);
                        } else {
                            setSourceOutputMode(MODE_4K2K60HZ);
                        }
                    } else if (strstr(tvmode, MODE_1080P) || strstr(tvmode, MODE_4K2K30HZ)){
                        setSourceOutputMode(MODE_1080P);
                    }
                } else {
                    char bestDolbyVision[MODE_LEN] = {0};
                    if (!getBootEnv(UBOOTENV_BESTDOLBYVISION, bestDolbyVision) || strstr(bestDolbyVision, "true") != NULL) {
                        if ((value_state == DOLBY_VISION_SET_ENABLE_LL_RGB)
                                && (resolveResolutionValue(tvmode) > resolveResolutionValue(MODE_1080P))) {
                            setSourceOutputMode(MODE_1080P);
                        } else {
                            if (strstr(tvmode, MODE_4K2K30HZ) || strstr(tvmode, MODE_4K2K24HZ) || strstr(tvmode, MODE_4K2K25HZ)) {
                                strcpy(tvmode, "1080p60hz");
                            }
                            setSourceOutputMode(tvmode);
                        }
                    } else {
                        setSourceOutputMode(outputmode);
                    }
                }
                setDolbyVisionState = true;
            }
            char hdr_policy[MODE_LEN] = {0};
            getHdrStrategy(hdr_policy);
            setHdrStrategy(hdr_policy);
            pSysWrite->writeSysfs(DOLBY_VISION_HDR10_POLICY_OLD, DV_HDR_SINK_PROCESS);
            pSysWrite->writeSysfs(DOLBY_VISION_HDR10_POLICY, DV_HDR_SINK_PROCESS);
        }

        usleep(100000);//100ms
        pSysWrite->writeSysfs(DOLBY_VISION_ENABLE_OLD, DV_ENABLE);
        pSysWrite->writeSysfs(DOLBY_VISION_MODE_OLD, DV_MODE_IPT_TUNNEL);
        pSysWrite->writeSysfs(DOLBY_VISION_ENABLE, DV_ENABLE);
        pSysWrite->writeSysfs(DOLBY_VISION_MODE, DV_MODE_IPT_TUNNEL);
        usleep(100000);//100ms
        if (DISPLAY_TYPE_TV == mDisplayType) {
            setHdrMode(HDR_MODE_AUTO);
        }
        initGraphicsPriority();
        SYS_LOGI("setDolbyVisionEnable Enable [%d]", isDolbyVisionEnable());
    } else {
        char tvmode[MODE_LEN] = {0};
        if (isTvSupportDolbyVision(tvmode)) {
            pSysWrite->setProperty(PROP_DOLBY_VISION_TV_ENABLE, "false");
        } else {
            pSysWrite->setProperty(PROP_DOLBY_VISION_ENABLE, "false");
        }
        char hdr_policy[MODE_LEN] = {0};
        getHdrStrategy(hdr_policy);
        setHdrStrategy(hdr_policy);

        pSysWrite->writeSysfs(DOLBY_VISION_POLICY_OLD, DV_POLICY_FORCE_MODE);
        pSysWrite->writeSysfs(DOLBY_VISION_POLICY, DV_POLICY_FORCE_MODE);
        pSysWrite->writeSysfs(DOLBY_VISION_MODE_OLD, DV_MODE_BYPASS);
        pSysWrite->writeSysfs(DOLBY_VISION_MODE, DV_MODE_BYPASS);
        usleep(100000);//100ms
        pSysWrite->readSysfs(DOLBY_VISION_STATUS, dvstatus);
        if (strcmp(dvstatus, BYPASS_PROCESS)) {
            while (++check_status_count <30) {
                usleep(20000);//20ms
                pSysWrite->readSysfs(DOLBY_VISION_STATUS, dvstatus);
                if (!strcmp(dvstatus, BYPASS_PROCESS)) {
                    break;
                }
            }
        }
        SYS_LOGI("dvstatus %s, check_status_count [%d]", dvstatus, check_status_count);
        pSysWrite->writeSysfs(DOLBY_VISION_ENABLE_OLD, DV_DISABLE);
        pSysWrite->writeSysfs(DOLBY_VISION_ENABLE, DV_DISABLE);

        if (DISPLAY_TYPE_TV == mDisplayType) {
            setHdrMode(HDR_MODE_AUTO);
        }
        if (pSysWrite->getPropertyBoolean(PROP_DISABLE_SDR2HDR, false)) {
            setSdrMode(SDR_MODE_OFF);
        } else {
            setSdrMode(SDR_MODE_AUTO);
        }

        char mode[MAX_STR_LEN] = {0};
        if (isTvSupportDolbyVision(mode)) {
            #ifndef HWC_DYNAMIC_SWITCH_VIU
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
            #endif
            setDolbyVisionState = false;
            setSourceOutputMode(outputmode);
            setDolbyVisionState = true;
        }
        SYS_LOGI("setDolbyVisionEnable Enable [%d]", isDolbyVisionEnable());
    }
    if (dvStatusChanged) {
        usleep(300000);//300ms
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
    }
#ifndef RECOVERY_MODE
    saveHdmiParamToEnv();
#endif
}

void DisplayMode::getHdrStrategy(char* value) {
    char hdr_policy[MODE_LEN] = {0};
    memset(hdr_policy, 0, MODE_LEN);
    if (!getBootEnv(UBOOTENV_HDR_POLICY, hdr_policy)) {
        strcpy(value, HDR_POLICY_SINK);
    }else if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
        strcpy(value, HDR_POLICY_SOURCE);
    } else if (strstr(hdr_policy, HDR_POLICY_SINK)) {
        strcpy(value, HDR_POLICY_SINK);
    } else {
        strcpy(value, HDR_POLICY_SINK);
    }
    SYS_LOGI("getHdrStrategy is [%s]", value);
}

void DisplayMode::setHdrStrategy(const char* type) {
    SYS_LOGI("setHdrStrategy is [%s]",type);
    char dv_mode[MAX_STR_LEN];
    if (strstr(type, HDR_POLICY_SINK)) {
        pSysWrite->writeSysfs(HDR_POLICY, HDR_POLICY_SINK);
        if (isDolbyVisionEnable()) {
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY_OLD, HDR_POLICY_SINK);
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY, HDR_POLICY_SINK);
        }
    } else if (strstr(type, HDR_POLICY_SOURCE)) {
        pSysWrite->writeSysfs(HDR_POLICY, HDR_POLICY_SOURCE);
        if (isDolbyVisionEnable()) {
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY_OLD, HDR_POLICY_SOURCE);
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY, HDR_POLICY_SOURCE);
        }
    }
    setBootEnv(UBOOTENV_HDR_POLICY, (char *)type);
}

void DisplayMode::initDolbyVision(output_mode_state state) {
    char dv_mode[MAX_STR_LEN];
    char bestDolbyVision[MODE_LEN] = {0};
    int var = 0;
    SYS_LOGI("initDolbyVision %d\n",state);
    int type = getDolbyVisionType();
    bool tv_mode = isTvSupportDolbyVision(dv_mode);
    if (tv_mode && (((type == DOLBY_VISION_SET_ENABLE) && strstr(dv_mode, "DV_RGB_444_8BIT") == NULL)
        || ((type == DOLBY_VISION_SET_ENABLE_LL_YUV) && strstr(dv_mode, "LL_YCbCr_422_12BIT") == NULL)
        || ((type == DOLBY_VISION_SET_ENABLE_LL_RGB) && strstr(dv_mode, "LL_RGB_444_12BIT") == NULL && strstr(dv_mode, "LL_RGB_444_10BIT") == NULL))) {
        setBootEnv(UBOOTENV_BESTDOLBYVISION, "true");
    }
    if (tv_mode && (!getBootEnv(UBOOTENV_BESTDOLBYVISION, bestDolbyVision) || strstr(bestDolbyVision, "true") != NULL)) {
        if ((strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) || (strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL)) {
            if (pSysWrite->getPropertyBoolean(PROP_ALWAYS_DOLBY_VISION, false)) {
                if (strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) {
                    var = DOLBY_VISION_SET_ENABLE;
                } else if (strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL) {
                    var = DOLBY_VISION_SET_ENABLE_LL_YUV;
                }
            } else {
                if (strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL) {
                    var = DOLBY_VISION_SET_ENABLE_LL_YUV;
                } else if (strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) {
                    var = DOLBY_VISION_SET_ENABLE;
                }
            }
        } else if ((strstr(dv_mode, "LL_RGB_444_12BIT") != NULL) || (strstr(dv_mode, "LL_RGB_444_10BIT") != NULL)) {
            var = DOLBY_VISION_SET_ENABLE_LL_RGB;
        }
    } else if (isDolbyVisionEnable()) {
        var = getDolbyVisionType();
    } else {
        if (pSysWrite->getPropertyBoolean(PROP_ALWAYS_DOLBY_VISION, false)) {
            var = DOLBY_VISION_SET_ENABLE;
        } else {
            var = DOLBY_VISION_SET_DISABLE;
        }
    }
    if (getCurDolbyVisionState(var,  state)) {
        SYS_LOGI("Current dolby vision type is same as the set value. return\n");
        return;
    }
    setDolbyVisionEnable(var,  state);
}

bool DisplayMode::getCurDolbyVisionState(int state, output_mode_state mode_state) {
    char dv_type[MODE_LEN] = {0};
    char dv_mode[MAX_STR_LEN] = {0};
    pSysWrite->readSysfs(DOLBY_VISION_LL_POLICY, dv_type);
    if ((mode_state != OUPUT_MODE_STATE_INIT)
            || checkDolbyVisionStatusChanged(state)
            || checkDolbyVisionDeepColorChanged(state)) {
        return false;
    }
    return true;
}

bool DisplayMode::checkDolbyVisionDeepColorChanged(int state) {
    char colorAttr[MODE_LEN] = {0};
    char mode[MAX_STR_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, colorAttr);
    if (isTvSupportDolbyVision(mode) && (state == DOLBY_VISION_SET_ENABLE)
            && (strstr(colorAttr, "444,8bit") == NULL)) {
        SYS_LOGI("colorAttr %s is not match with DV STD\n", colorAttr);
        return true;
    } else if ((state == DOLBY_VISION_SET_ENABLE_LL_YUV) && (strstr(colorAttr, "422,12bit") == NULL)) {
        SYS_LOGI("colorAttr %s is not match with DV LL YUV\n", colorAttr);
        return true;
    } else if ((state == DOLBY_VISION_SET_ENABLE_LL_RGB) && (strstr(colorAttr, "444,12bit") == NULL)
                && (strstr(colorAttr, "444,10bit") == NULL)) {
        SYS_LOGI("colorAttr %s is not match with DV LL RGB\n", colorAttr);
         return true;
     }
     return false;
}

/* *
 * @Description: get Current State DolbyVision State
 *
 * @result: if disable Dolby Vision return DOLBY_VISION_SET_DISABLE.
 *          if Current TV support the state saved in Mbox. return that state. like value of saved is 2, and TV support LL_YUV
 *          if Current TV not support the state saved in Mbox. but isDolbyVisionEnable() is enable.
 *             return state in priority queue. like value of saved is 2, But TV only Support LL_RGB, so system will return LL_RGB
 */
int DisplayMode::getDolbyVisionType() {
    char type[MODE_LEN];
    char mode[MAX_STR_LEN];
    if (isTvSupportDolbyVision(mode)) {
        pSysWrite->getPropertyString(PROP_DOLBY_VISION_TV_TYPE, type, "0");
        if ((strstr(type, "1") != NULL) && strstr(mode, "DV_RGB_444_8BIT") != NULL) {
            return DOLBY_VISION_SET_ENABLE;
        } else if ((strstr(type, "2") != NULL) && strstr(mode, "LL_YCbCr_422_12BIT") != NULL) {
            return DOLBY_VISION_SET_ENABLE_LL_YUV;
        } else if ((strstr(type, "3") != NULL)
                && ((strstr(mode, "LL_RGB_444_12BIT") != NULL) || (strstr(mode, "LL_RGB_444_10BIT") != NULL))) {
            return DOLBY_VISION_SET_ENABLE_LL_RGB;
        } else if (strstr(type, "0") != NULL) {
            return DOLBY_VISION_SET_DISABLE;
        }

        if (strstr(type, "0") == NULL) {
            if (strstr(mode, "DV_RGB_444_8BIT") != NULL) {
                return DOLBY_VISION_SET_ENABLE;
            } else if (strstr(mode, "LL_YCbCr_422_12BIT") != NULL) {
                return DOLBY_VISION_SET_ENABLE_LL_YUV;
            } else if ((strstr(mode, "LL_RGB_444_12BIT") != NULL) || (strstr(mode, "LL_RGB_444_10BIT") != NULL)) {
                return DOLBY_VISION_SET_ENABLE_LL_RGB;
            }
        }
    }

    if (isDolbyVisionEnable()) {
        return DOLBY_VISION_SET_ENABLE;
    }
    return DOLBY_VISION_SET_DISABLE;
}

/* *
 * @Description: Detect Whether Dolby vision enable and TV support Dolby Vision
 *               if currentMode is not resolution TV Dolby Vision supported. and replace it by mode TV supported.
 * @params: state: current outputmode state INIT/POWER/SWITCH/ADATER etc
 *          outputmode: resolution needed to modify.
 * @result: the outputmode maybe changed into TV Dolby Vision supported.
 */
void DisplayMode::DetectDolbyVisionOutputMode(output_mode_state state, char* outputmode) {
    bool cvbsMode = false;
    if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS) || !strcmp(outputmode, MODE_PAL_M) || !strcmp(outputmode, MODE_PAL_N) || !strcmp(outputmode, MODE_NTSC_M)) {
        cvbsMode = true;
    }
    if (!cvbsMode && OUPUT_MODE_STATE_INIT == state
            && isDolbyVisionEnable()) {
        char mode[MAX_STR_LEN] = {0};
        if (isTvSupportDolbyVision(mode)) {
            char tvmode[MODE_LEN] = {0};
            for (int i = DISPLAY_MODE_TOTAL - 1; i >= 0; i--) {
                if (strstr(mode, DISPLAY_MODE_LIST[i]) != NULL) {
                    strcpy(tvmode, DISPLAY_MODE_LIST[i]);
                }
            }
            if (resolveResolutionValue(outputmode) > resolveResolutionValue(tvmode)
                    || (strstr(outputmode, "smpte") != NULL)) {
                memset(outputmode, 0, sizeof(outputmode));
                if ((strstr(tvmode, MODE_4K2K30HZ) || strstr(tvmode, MODE_4K2K24HZ) || strstr(tvmode, MODE_4K2K25HZ))
                        && (isBestOutputmode())) {
                    strcpy(outputmode, "1080p60hz");
                } else {
                    strcpy(outputmode, tvmode);
                }
            }
            char type[MODE_LEN] = {0};
            pSysWrite->getPropertyString(PROP_DOLBY_VISION_TV_TYPE, type, "1");
            if (((atoi(type) == 2) && (strstr(mode, DV_MODE_TYPE[2]) == NULL))
                    || ((atoi(type) == 3) && (strstr(mode, DV_MODE_TYPE[3]) == NULL)
                        && (strstr(mode, DV_MODE_TYPE[4]) == NULL))) {
                strcpy(type, "1");
            }
            switch (atoi(type)) {
                case DOLBY_VISION_SET_ENABLE:
                    setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,8bit");
                    break;
                case DOLBY_VISION_SET_ENABLE_LL_YUV:
                    setBootEnv(UBOOTENV_COLORATTRIBUTE, "422,12bit");
                    break;
                case DOLBY_VISION_SET_ENABLE_LL_RGB:
                    if (strstr(mode, "LL_RGB_444_12BIT") != NULL) {
                        setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,12bit");
                    } else if (strstr(mode, "LL_RGB_444_10BIT") != NULL) {
                        setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,10bit");
                    }
                    break;
            }
        }
    }
}

/* *
 * @Description: set dolby vision graphics priority only when dolby vision enable.
 * @params: "0": Video Priority    "1": Graphics Priority
 * */
void DisplayMode::setGraphicsPriority(const char* mode) {
    if (NULL != mode) {
        SYS_LOGI("setGraphicsPriority [%s]", mode);
    }
    if ((NULL != mode) && (atoi(mode) == 0 || atoi(mode) == 1)) {
        pSysWrite->writeSysfs(DOLBY_VISION_GRAPHICS_PRIORITY_OLD, mode);
        pSysWrite->writeSysfs(DOLBY_VISION_GRAPHICS_PRIORITY, mode);
        pSysWrite->setProperty(PROP_DOLBY_VISION_PRIORITY, mode);
        SYS_LOGI("setGraphicsPriority [%s]",
                atoi(mode) == 0 ? "Video Priority" : "Graphics Priority");
    } else {
        pSysWrite->writeSysfs(DOLBY_VISION_GRAPHICS_PRIORITY_OLD, "0");
        pSysWrite->writeSysfs(DOLBY_VISION_GRAPHICS_PRIORITY, "0");
        pSysWrite->setProperty(PROP_DOLBY_VISION_PRIORITY, "0");
        SYS_LOGI("setGraphicsPriority default [Video Priority]");
    }
}

/* *
 * @Description: get dolby vision graphics priority.
 * @params: store current priority mode.
 * */
void DisplayMode::getGraphicsPriority(char* mode) {
    pSysWrite->getPropertyString(PROP_DOLBY_VISION_PRIORITY, mode, "0");
    SYS_LOGI("getGraphicsPriority [%s]",
        atoi(mode) == 0 ? "Video Priority" : "Graphics Priority");
}

/* *
 * @Description: init dolby vision graphics priority when bootup.
 * */
void DisplayMode::initGraphicsPriority() {
    char mode[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_DOLBY_VISION_PRIORITY, mode, "1");
    pSysWrite->writeSysfs(DOLBY_VISION_GRAPHICS_PRIORITY, mode);
    pSysWrite->setProperty(PROP_DOLBY_VISION_PRIORITY, mode);
}

/* *
 * @Description: set hdr mode
 * @params: mode "0":off "1":on "2":auto
 * */
void DisplayMode::setHdrMode(const char* mode) {
    if ((atoi(mode) >= 0) && (atoi(mode) <= 2)) {
        SYS_LOGI("setHdrMode state: %s\n", mode);
        pSysWrite->writeSysfs(DISPLAY_HDMI_HDR_MODE, mode);
        pSysWrite->setProperty(PROP_HDR_MODE_STATE, mode);
    }
}

/* *
 * @Description: set sdr mode
 * @params: mode "0":off "2":auto
 * */
void DisplayMode::setSdrMode(const char* mode) {
    if ((atoi(mode) == 0) || atoi(mode) == 2) {
        SYS_LOGI("setSdrMode state: %s\n", mode);
        pSysWrite->writeSysfs(DISPLAY_HDMI_SDR_MODE, mode);
        pSysWrite->setProperty(PROP_SDR_MODE_STATE, mode);
        setBootEnv(UBOOTENV_SDR2HDR, (char *)mode);
    }
}

void DisplayMode::initHdrSdrMode() {
    char mode[MODE_LEN] = {0};
    char dv_mode[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_HDR_MODE_STATE, mode, HDR_MODE_AUTO);
    setHdrMode(mode);
    memset(mode, 0, sizeof(mode));
    bool flag = pSysWrite->getPropertyBoolean(PROP_ENABLE_SDR2HDR, false);
    if (flag & isDolbyVisionEnable()) {
        strcpy(mode, SDR_MODE_OFF);
    } else {
        pSysWrite->getPropertyString(PROP_SDR_MODE_STATE, mode, flag ? SDR_MODE_AUTO : SDR_MODE_OFF);
    }

    if (pSysWrite->getPropertyBoolean(PROP_DISABLE_SDR2HDR, false)) {
        setSdrMode(SDR_MODE_OFF);
    } else {
        setSdrMode(mode);
    }
}

int DisplayMode::modeToIndex(const char *mode) {
    int index = DISPLAY_MODE_1080P;
    for (int i = 0; i < DISPLAY_MODE_TOTAL; i++) {
        if (!strcmp(mode, DISPLAY_MODE_LIST[i])) {
            index = i;
            break;
        }
    }

    //SYS_LOGI("modeToIndex mode:%s index:%d", mode, index);
    return index;
}

void DisplayMode::isHDCPTxAuthSuccess(int *status) {
    pTxAuth->isAuthSuccess(status);
}

void DisplayMode::onTxEvent (char* switchName, char* hpdstate, int outputState) {
    SYS_LOGI("onTxEvent switchName:%s hpdstate:%s state: %d\n", switchName, hpdstate, outputState);
#ifndef RECOVERY_MODE
    if (!strcmp(switchName, HDMI_UEVENT_HDMI_AUDIO)) {
        notifyEvent(hpdstate[0] == '1' ? EVENT_HDMI_AUDIO_IN : EVENT_HDMI_AUDIO_OUT);
        return;
    }
    if (hpdstate) {
        if (hpdstate[0] == '1' && pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_TV_ENABLE, false)) {
            char temp[1025] = {0};
            pSysWrite->readSysfs(DISPLAY_EDID_RAW, temp);
            if (strstr(mEdid, temp) == NULL) {
                setBootEnv(UBOOTENV_BESTDOLBYVISION, "true");
                strcpy(mEdid, temp);
            }
        }

        notifyEvent((hpdstate[0] == '1') ? EVENT_HDMI_PLUG_IN : EVENT_HDMI_PLUG_OUT);
        if (hpdstate[0] == '1')
            dumpCaps();
    }
    if (strstr(mRebootMode, "quiescent")) {
        SYS_LOGI("reset mRebootMode normal\n");
        strcpy(mRebootMode, "normal");
        setBootEnv(UBOOTENV_REBOOT_MODE, mRebootMode);
    }
#endif
    setSourceDisplay((output_mode_state)outputState);
}

void DisplayMode::onDispModeSyncEvent (const char* outputmode, int state) {
    SYS_LOGI("onDispModeSyncEvent outputmode:%s state: %d\n", outputmode, state);
    setSourceOutputMode(outputmode, (output_mode_state)state);
}

//for debug
void DisplayMode::hdcpSwitch() {
    SYS_LOGI("hdcpSwitch for debug hdcp authenticate\n");
}

#ifndef RECOVERY_MODE
void DisplayMode::notifyEvent(int event) {
    if (mNotifyListener != NULL) {
        mNotifyListener->onEvent(event);
    }
}

void DisplayMode::setListener(const sp<SystemControlNotify>& listener) {
    mNotifyListener = listener;
}
#endif

void DisplayMode::dumpCap(const char * path, const char * hint, char *result) {
    char logBuf[MAX_STR_LEN];
    pSysWrite->readSysfsOriginal(path, logBuf);

    if (mLogLevel > LOG_LEVEL_0)
        SYS_LOGI("%s%s", hint, logBuf);

    if (NULL != result) {
        strcat(result, hint);
        strcat(result, logBuf);
        strcat(result, "\n");
    }
}

void DisplayMode::dumpCaps(char *result) {
    dumpCap(DISPLAY_EDID_STATUS, "\nEDID parsing status: ", result);
    dumpCap(DISPLAY_EDID_VALUE, "General caps\n", result);
    dumpCap(DISPLAY_HDMI_DEEP_COLOR, "Deep color\n", result);
    dumpCap(DISPLAY_HDMI_HDR, "HDR\n", result);
    dumpCap(DISPLAY_HDMI_MODE_PREF, "Preferred mode: ", result);
    dumpCap(DISPLAY_HDMI_SINK_TYPE, "Sink type: ", result);
    dumpCap(DISPLAY_HDMI_AUDIO, "Audio caps\n", result);
    dumpCap(DISPLAY_EDID_RAW, "Raw EDID\n", result);
}

int DisplayMode::dump(char *result) {
    if (NULL == result)
        return -1;

    char buf[2048] = {0};
    sprintf(buf, "\ndisplay type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s\n", mDisplayType, mSocType);
    strcat(result, buf);

    if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
        sprintf(buf, "default ui:%s\n", mDefaultUI);
        strcat(result, buf);
        dumpCaps(result);
    }
    return 0;
}

bool DisplayMode::checkDolbyVisionStatusChanged(int state) {
    char curDvEnable[MODE_LEN] = {0};
    char curDvLLPolicy[MODE_LEN] = {0};
    int curDvMode = -1;

    pSysWrite->readSysfs(DOLBY_VISION_ENABLE, curDvEnable);
    pSysWrite->readSysfs(DOLBY_VISION_LL_POLICY, curDvLLPolicy);

    if (!strcmp(curDvEnable, DV_DISABLE) ||
        !strcmp(curDvEnable, "0"))
        curDvMode = DOLBY_VISION_SET_DISABLE;
    else if (!strcmp(curDvLLPolicy, "0"))
        curDvMode = DOLBY_VISION_SET_ENABLE;
    else if (!strcmp(curDvLLPolicy, "1"))
        curDvMode = DOLBY_VISION_SET_ENABLE_LL_YUV;
    else if (!strcmp(curDvLLPolicy, "2"))
        curDvMode = DOLBY_VISION_SET_ENABLE_LL_RGB;

    if (curDvMode != state) {
        SYS_LOGI("checkDolbyVisionStatusChanged cur status %d, want state %d\n",
                  curDvMode, state);
        return 1;
    }
    return 0;
}
void DisplayMode::saveHdmiParamToEnv(){
    char rawEdid[524] = {0}; //rawedid 128*4
    char colorAttr[MODE_LEN] = {0};
    char colorDepth[MODE_LEN] = {0};
    char colorSpace[MODE_LEN] = {0};
    char dolbyEnable[MODE_LEN] = {0};
    char outputMode[MODE_LEN] = {0};
    char dvstatus[MODE_LEN] = {0};

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, outputMode);

    // 1. check whether the TV changed or not, if changed save crc
    if (isEdidChange()) {
        SYS_LOGD("tv sink changed\n");
    }

    // 2. save rawEdid/coloattr/hdmimode to bootenv if mode is not null
    //if the value we try to save is the same with the last saved value,then Logoparam will prohibited to write
    if (strcmp(outputMode, "null")) {
        pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, colorAttr);

        // 2.1 save color attr
        setBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttr);
        //colorDepth&&colorSpace is used for uboot hdmi to find
        //best color attributes for the selected hdmi mode when TV changed
        pSysWrite->getPropertyString(PROP_DEEPCOLOR_CTL, colorDepth, "8");
        pSysWrite->getPropertyString(PROP_PIXFMT, colorSpace, "auto");
        setBootEnv(UBOOTENV_HDMICOLORDEPTH, colorDepth);
        setBootEnv(UBOOTENV_HDMICOLORSPACE, colorSpace);

        // 2.2 save output mode
        setBootEnv(UBOOTENV_OUTPUTMODE, (char *)outputMode);
        setBootEnv(UBOOTENV_HDMIMODE, outputMode);

        // 2.3 save dolby status - 0:disable 1:STD 2:LL YUV 3: LL RGB
        sprintf(dvstatus, "%d", mDvStatus);
        setBootEnv(UBOOTENV_DOLBYSTATUS, dvstatus);

        SYS_LOGD("saveHdmiParamToEnv: colorattr: %s, outputMode %s, dvstatus %s, cd %s, cs %s\n",
                  colorAttr, outputMode, colorDepth, colorSpace, dvstatus);
    }
    // for debugging, print all logoparam
    if (pSysWrite->getPropertyBoolean("persist.systemcontrol.debug", false))
        mUbootenv->printValues();
}

/* *
 * @Description: get perf hdmi display mode priority.
 * @params: store current perf hdmi mode.
 * */
bool DisplayMode::getPrefHdmiDispMode(char* mode) {
    bool ret = getBootEnv(UBOOTENV_HDMIMODE, mode);
    SYS_LOGI("getPrefHdmiDispMode [%s]", mode);
    return ret;
}
