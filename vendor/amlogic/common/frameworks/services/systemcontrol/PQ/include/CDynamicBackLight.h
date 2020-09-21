/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */
#ifndef _DYNAMIC_BACKLIGHT_H
#define _DYNAMIC_BACKLIGHT_H

#include <utils/Thread.h>
#include <cutils/properties.h>

#include "PQType.h"
#include "CPQLog.h"


#define ONE_FRAME_TIME             15*1000
#define COLOR_RANGE_MODE           0
#define SYSFS_HIST_SEL             "/sys/module/am_vecm/parameters/vpp_hist_sel"

using namespace android;

typedef struct dynamic_backlight_Param_s {
    int CurBacklightValue;
    int UiBackLightValue;
    Dynamic_backlight_status_t CurDynamicBacklightMode;
    ve_hist_s hist;
    int VideoStatus;
} dynamic_backlight_Param_t;

enum THREAD_STATE {
    THREAD_RUNING = 1,
    THREAD_PAUSED = 2,
    THREAD_STOPED = 3,
};

enum BACKLIGHT_VALID_RANGE {
    BACKLIGHT_RANGE_VIDEO = 0,
    BACKLIGHT_RANGE_VIDEOWITHOSD = 1,
};

class CDynamicBackLight : public Thread {
public:
    CDynamicBackLight();
    ~CDynamicBackLight();
    int startDected(void);
    void stopDected(void);
    void pauseDected(int pauseTime);
    static void setOsdStatus(int osd_status);
    void gd_fw_alg_frm(int average, int *tf_bl_value, int *LUT);
    class IDynamicBackLightObserver {
        public:
            IDynamicBackLightObserver() {};
            virtual ~IDynamicBackLightObserver() {};
            virtual void Set_Backlight(int value) {};
            virtual void GetDynamicBacklighConfig(int *thtf, int *lut_mode, int *heigh_param, int *low_param) {};
            virtual void GetDynamicBacklighParam(dynamic_backlight_Param_t *DynamicBacklightParam) {};
    };

    void setObserver (IDynamicBackLightObserver *pOb)
    {
        mpObserver = pOb;
    };
private:
    int mPreBacklightValue;
    int mGD_mvreflsh;
    int mArithmeticPauseTime;
    int GD_LUT_MODE;
    int GD_ThTF;
    THREAD_STATE mRunStatus;
    int backLightScale(int, int);
    bool threadLoop();
    IDynamicBackLightObserver *mpObserver;
};

#endif

