/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CAutoPQparam"

#include "../vpp/CVpp.h"
#include "CAutoPQparam.h"
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


CAutoPQparam::CAutoPQparam()
{
    preFmtType = 0;
    curFmtType = 0;
    autofreq_checkcount = 0;
    autofreq_checkflag = 0;
    mAutoPQ_OnOff_Flag = false;
    mAutoPQSource = SOURCE_INVALID;
}

CAutoPQparam::~CAutoPQparam()
{
    mAutoPQ_OnOff_Flag = false;
}

bool CAutoPQparam::isAutoPQing()
{
    return mAutoPQ_OnOff_Flag;
}

void CAutoPQparam::startAutoPQ( tv_source_input_t tv_source_input )
{
#ifndef CC_PROJECT_DISABLE_AUTO_PQ
    mAutoPQSource = tv_source_input;

    LOGD("---------startAutoPQParameters  --------mAutoPQ_OnOff_Flag  = %d", mAutoPQ_OnOff_Flag);
    if (!mAutoPQ_OnOff_Flag) {
        mAutoPQ_OnOff_Flag = true;
        this->run("CAutoPQparam");
    }
#else
    LOGD("AutoPQparam disable.\n");
#endif
}

void CAutoPQparam::stopAutoPQ()
{
#ifndef CC_PROJECT_DISABLE_AUTO_PQ
    LOGD("---------stopAutoPQParameters  -------- mAutoPQ_OnOff_Flag = %d", mAutoPQ_OnOff_Flag);
    if (mAutoPQ_OnOff_Flag) {
        mAutoPQ_OnOff_Flag = false;
    }
#else
    LOGD("AutoPQparam disable.\n");
#endif
}

/**
TVIN_SIG_FMT_HDMI_720X480P_60HZ  = 0x402            nodeVal<900
TVIN_SIG_FMT_HDMI_1920X1080P_60HZ = 0x40a           900<nodeVal<2000
TVIN_SIG_FMT_HDMI_3840_2160_00HZ = 0x445            nodeVal>2000
*/
int CAutoPQparam::adjustPQparameters()
{
    int fd = -1;
    int nodeVal = 0, ret = 0, read_ret = 0;
    int new_frame_count = 0;
    char s[21];
    tvin_sig_fmt_e sig_fmt;
    tvin_trans_fmt trans_fmt = TVIN_TFMT_2D;

    fd = open("/sys/module/amvideo/parameters/new_frame_count", O_RDONLY);
    if (fd < 0) {
        LOGE("open /sys/module/amvideo/parameters/new_frame_count  ERROR!!error = -%s- \n", strerror ( errno ));
        return -1;
    }
    memset(s, 0, sizeof(s));
    read_ret = read(fd, s, sizeof(s));
    close(fd);
    if (read_ret > 0)
    {
        new_frame_count = atoi(s);
    }

    if (new_frame_count != 0) {

        fd = open(SYS_VIDEO_FRAME_HEIGHT, O_RDONLY);
        if (fd < 0) {
            LOGE("open %s ERROR!!error = -%s- \n",SYS_VIDEO_FRAME_HEIGHT, strerror ( errno ));
            return -1;
        }
        memset(s, 0, sizeof(s));
        read_ret = read(fd, s, sizeof(s));
        close(fd);
        if (read_ret > 0)
        {
            nodeVal = atoi(s);
        }

        if (nodeVal <= 576) {
            curFmtType = 1;
            sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
        } else if (nodeVal > 567 && nodeVal <= 1088) {
            curFmtType = 2;
            sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
        } else {
            curFmtType = 3;
            sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
        }

        if (curFmtType != preFmtType) {
            LOGD("adjustPQparameters: nodeVal = %d, sig_fmt = %d.", nodeVal, sig_fmt);
            ret = CVpp::getInstance()->LoadVppSettings (mAutoPQSource, sig_fmt, trans_fmt);
        }

        preFmtType = curFmtType;

    } else {
        if (preFmtType != 0 || curFmtType != 0) {
            preFmtType = 0;
            curFmtType = 0;
        }
    }
    return ret;
}

bool CAutoPQparam::threadLoop()
{
    int sleeptime = 1000;//ms
    while ( mAutoPQ_OnOff_Flag ) {
        usleep ( sleeptime * 1000 );
        adjustPQparameters();
    }

    return false;//return true, run again, return false,not run.
}
