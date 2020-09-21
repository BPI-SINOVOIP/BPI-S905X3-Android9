/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "AutoBackLight"

#include "AutoBackLight.h"
#include "../tvsetting/CTvSetting.h"
#include <tvconfig.h>
#include <tvutils.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <cutils/properties.h>

AutoBackLight::AutoBackLight()
{
    mAutoBacklightSource = SOURCE_TV;
    mCur_source_default_backlight = 100;
    mCur_sig_state = SIG_STATE_NOSIG;
    mAutoBacklight_OnOff_Flag = false;
    mCurrent_backlight = 100;
    mCur_dest_backlight = 100;
    mCur_sig_state = 0;
}

AutoBackLight::~AutoBackLight()
{
    mAutoBacklight_OnOff_Flag = false;
}

bool AutoBackLight::isAutoBacklightOn()
{
    return mAutoBacklight_OnOff_Flag;
}

void AutoBackLight::updateSigState(int state)
{
    mCur_sig_state = state;
    LOGD("updateSigState = %d", mCur_sig_state);
}

void AutoBackLight::startAutoBacklight( tv_source_input_t tv_source_input )
{
    mAutoBacklightSource = tv_source_input;
    mCur_source_default_backlight = CVpp::getInstance()->GetBacklight(tv_source_input);
    mCurrent_backlight = mCur_source_default_backlight;
    CVpp::getInstance()->SetBacklight(mCur_source_default_backlight, tv_source_input, 1);

    /*
    mDefault_auto_bl_value = def_source_bl_value;
    dynamicGamma = mDefault_auto_bl_value * mCur_source_default_backlight / 100;
    // this if should not happen
    if (dynamicGamma > mCur_source_default_backlight) {
        dynamicGamma = mCur_source_default_backlight;
    }
    */

    if (!mAutoBacklight_OnOff_Flag) {
        mAutoBacklight_OnOff_Flag = true;
        this->run("AutoBackLight");
    }
}

void AutoBackLight::stopAutoBacklight()
{
    if (mAutoBacklight_OnOff_Flag) {
        mAutoBacklight_OnOff_Flag = false;
        CVpp::getInstance()->SetBacklight(mCur_source_default_backlight, mAutoBacklightSource, 1);
    }
}

/**
 * @ description: tpv project
 * @ return:value
 *         value <= 20:  mCur_dest_backlight is 14
 * 20 < value <= 160: mCur_dest_backlight is 57
 *160 < value:            mCur_dest_backlight is 100
 */
void AutoBackLight::adjustDstBacklight()
{
    if (mCur_sig_state == SIG_STATE_STABLE) {
        //the node is used to adjust current ts is static or dynamtic frame
        char temp_str = 0;
        int fd = open("/sys/module/di/parameters/frame_dynamic", O_RDWR);
        if (fd < 0) {
            LOGE("open /sys/module/di/parameters/frame_dynamic ERROR!!\n");
            return;
        }

        if (read(fd, &temp_str, 1) > 0) {

            if (temp_str == 'N') {
                mCur_dest_backlight = mCur_source_default_backlight;
            } else if (temp_str == 'Y') {
                int pwm = HistogramGet_AVE();
                if (pwm <= 20) {
                    mCur_dest_backlight = 14;
                } else if (pwm > 20 && pwm <= 160) {
                    mCur_dest_backlight = 57;
                } else {
                    mCur_dest_backlight = 100;
                }
                //LOGD("pwm = %d, mCur_dest_backlight = %d", pwm, mCur_dest_backlight);
            }
        }
        close(fd);
    } else {
        mCurrent_backlight = mCur_dest_backlight = mCur_source_default_backlight;
        CVpp::getInstance()->SetBacklight(mCurrent_backlight, mAutoBacklightSource, 0);
    }

    /*
    if (pwm > 0)
        pwm_max = pwm;
    else
        pwm_min = pwm;
    pwm = 255 - pwm;
    int average = (pwm_min + pwm_max) / 2;
    dynamicGammaOffset = (pwm - average) / 10;
    dynamicGammaOffset = dynamicGammaOffset * mDefault_auto_bl_value / 100;

    //the node is used to adjust current ts is static or dynamtic frame
    char temp_str = 0;
    int fd = open("/sys/module/di/parameters/frame_dynamic", O_RDWR);
    if (fd <= 0) {
        LOGE("open /sys/module/di/parameters/frame_dynamic ERROR!!\n");
        return;
    }

    if (read(fd, &temp_str, 1) > 0) {
        if (temp_str== 'N') {
            mCur_dest_backlight = mCur_source_default_backlight;
        }
        else if (temp_str == 'Y') {
            mCur_dest_backlight = dynamicGamma + dynamicGammaOffset;

            if (mCur_dest_backlight > mCur_source_default_backlight) {
                mCur_dest_backlight = mCur_source_default_backlight;
            }
            else if (mCur_dest_backlight < 0) {
                mCur_dest_backlight = 0;
            }
        }
    }
    close(fd);
    */
}

void AutoBackLight::adjustBacklight()
{
    if (mCurrent_backlight == mCur_dest_backlight) {
        return;
    } else if ((mCurrent_backlight - mCur_dest_backlight) > -2 && (mCurrent_backlight - mCur_dest_backlight) < 2) {
        mCurrent_backlight = mCur_dest_backlight;
        CVpp::getInstance()->SetBacklight(mCurrent_backlight, mAutoBacklightSource, 0);
    } else if (mCurrent_backlight < mCur_dest_backlight) {
        mCurrent_backlight = mCurrent_backlight + 2;
        CVpp::getInstance()->SetBacklight(mCurrent_backlight, mAutoBacklightSource, 0);
    } else if (mCurrent_backlight > mCur_dest_backlight) {
        mCurrent_backlight = mCurrent_backlight - 2;
        CVpp::getInstance()->SetBacklight(mCurrent_backlight, mAutoBacklightSource, 0);
    }

    //LOGD("mCurrent_backlight = %d", mCurrent_backlight);
}

/**
 * @ description: get current picture's average brightness
 * @ return: 0~255,0 is darkest,255 is brightest
 */
int AutoBackLight::HistogramGet_AVE()
{
    int hist_ave = 0;
    tvin_parm_t vdinParam;
    if (0 == CTvin::getInstance()->VDIN_GetVdinParam(&vdinParam)) {
        if (vdinParam.pixel_sum != 0) {
            hist_ave = vdinParam.luma_sum / vdinParam.pixel_sum;
            LOGD("[hist_ave][%d].", hist_ave);
            return hist_ave;
        }
        LOGE("vdinParam.pixel_sum is zero, so the value is infinity\n");
        return -1;
    }
    LOGE("VDIN_GetVdinParam get data error!!!\n");
    return -1;
}

bool AutoBackLight::threadLoop()
{
    int sleeptime = 50;//ms
    int adjustBacklightCount = 0;
    while ( mAutoBacklight_OnOff_Flag ) {
        usleep ( sleeptime * 1000 );
        adjustBacklightCount++;
        if (adjustBacklightCount == 24) {
            adjustBacklightCount = 0;
            adjustDstBacklight();
        }
        adjustBacklight();
    }

    return false;//return true, run again, return false,not run.
}
