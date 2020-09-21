/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#define LOG_TAG "amavutils"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <sys/ioctl.h>
#include "include/Amvideoutils.h"
#include "include/Amsysfsutils.h"
#include "include/Amdisplayutils.h"
#include "include/Amsyswrite.h"

#include "amports/amstream.h"
#include "ppmgr/ppmgr.h"

#define SYSCMD_BUFSIZE 40
#define DISP_DEVICE_PATH           "/sys/class/video/device_resolution"
#define FB_DEVICE_PATH             "/sys/class/graphics/fb0/virtual_size"
#define ANGLE_PATH                 "/dev/ppmgr"
#define VIDEO_PATH                 "/dev/amvideo"
#define VIDEO_AXIS_PATH            "sys/class/video/axis"
#define PPMGR_ANGLE_PATH           "sys/class/ppmgr/angle"
#define VIDEO_GLOBAL_OFFSET_PATH   "/sys/class/video/global_offset"
#define FREE_SCALE_PATH            "/sys/class/graphics/fb0/free_scale"
#define FREE_SCALE_PATH_FB2        "/sys/class/graphics/fb2/free_scale"
#define FREE_SCALE_PATH_FB1        "/sys/class/graphics/fb1/free_scale"
#define PPSCALER_PATH              "/sys/class/ppmgr/ppscaler"
#define HDMI_AUTHENTICATE_PATH     "/sys/module/hdmitx/parameters/hdmi_authenticated"
#define FREE_SCALE_MODE_PATH       "/sys/class/graphics/fb0/freescale_mode"
#define WINDOW_AXIS_PATH           "/sys/class/graphics/fb0/window_axis"
#define DISPLAY_AXIS_PATH          "/sys/class/display/axis"
#define FREE_SCALE_AXIS_PATH       "/sys/class/graphics/fb0/free_scale_axis"
#define PPSCALER_RECT              "/sys/class/ppmgr/ppscaler_rect"
#define WINDOW_AXIS_PATH_FB1       "/sys/class/graphics/fb1/window_axis"
#define FREE_SCALE_AXIS_PATH_FB1   "/sys/class/graphics/fb1/free_scale_axis"

//static int rotation = 0;
//static int disp_width = 1920;
//static int disp_height = 1080;

#ifndef LOGD
#define LOGV ALOGV
#define LOGD ALOGD
#define LOGI ALOGI
#define LOGW ALOGW
#define LOGE ALOGE
#endif

//#define LOG_FUNCTION_NAME LOGI("%s-%d\n",__FUNCTION__,__LINE__);
#define LOG_FUNCTION_NAME

int amvideo_utils_get_freescale_enable(void)
{
    int ret = 0;
    char buf[32];

    ret = amsysfs_get_sysfs_str("/sys/class/graphics/fb0/free_scale", buf, 32);
    if ((ret >= 0) && strncmp(buf, "free_scale_enalbe:[0x1]",
                              strlen("free_scale_enalbe:[0x1]")) == 0) {

        return 1;

    }
    return 0;
}

int  amvideo_utils_get_global_offset(void)
{
    LOG_FUNCTION_NAME
    int offset = 0;
    char buf[SYSCMD_BUFSIZE];
    int ret;
    ret = amsysfs_get_sysfs_str(VIDEO_GLOBAL_OFFSET_PATH, buf, SYSCMD_BUFSIZE);
    if (ret < 0) {
        return offset;
    }
    if (sscanf(buf, "%d", &offset) == 1) {
        LOGI("video global_offset %d\n", offset);
    }
    return offset;
}

int is_video_on_vpp2(void)
{
    int ret = 0;

    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (property_get("ro.media.vout.dualdisplay4", val, "false")
        && strcmp(val, "true") == 0) {
        memset(val, 0, sizeof(val));
        if (amsysfs_get_sysfs_str("/sys/module/amvideo/parameters/cur_dev_idx", val, sizeof(val)) == 0) {
            if ((strncmp(val, "1", 1) == 0)) {
                ret = 1;
            }
        }
    }

    return ret;
}

int is_vertical_panel(void)
{
    int ret = 0;

    // ro.vout.dualdisplay4.ver-panel
    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (property_get("ro.media.vout.dualdisplay4.ver-panel", val, "false")
        && strcmp(val, "true") == 0) {
        ret = 1;
    }

    return ret;
}

int is_screen_portrait(void)
{
    int ret = 0;

    // ro.vout.dualdisplay4.ver-panel
    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (property_get("ro.media.screen.portrait", val, "false")
        && strcmp(val, "true") == 0) {
        ret = 1;
    }

    return ret;
}

int is_osd_on_vpp2_new(void)
{
    int ret = 0;

    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (amsysfs_get_sysfs_str("/sys/class/graphics/fb2/clone", val, sizeof(val)) == 0) {
        ret = (val[19] == '1') ? 1 : 0;
    }
    return ret;
}

int is_hdmi_on_vpp1_new(void)
{
    int ret1 = 0;
    int ret2 = 0;
    int ret = 0;

    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (amsysfs_get_sysfs_str("/sys/class/graphics/fb1/ver_clone", val, sizeof(val)) == 0) {
        ret1 = (val[11] == 'O') ? 1 : 0;
        ret2 = (val[12] == 'N') ? 1 : 0;
        if ((ret1 == 1) && (ret2 == 1)) {
            ret = 1;
        }
    }
    return ret;
}

int is_vertical_panel_reverse(void)
{
    int ret = 0;

    // ro.vout.dualdisplay4.ver-panel
    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (property_get("ro.media.ver-panel.reverse", val, "false")
        && strcmp(val, "true") == 0) {
        ret = 1;
    }

    return ret;
}

int is_panel_mode(void)
{
    int ret = 0;
    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    if (amsysfs_get_sysfs_str("/sys/class/display/mode", val, sizeof(val)) == 0) {
        ret = (val[0] == 'p') ? 1 : 0;
    }
    return ret;
}


typedef enum _OSD_DISP_MODE {
    OSD_DISP_480I,
    OSD_DISP_480P,
    OSD_DISP_576I,
    OSD_DISP_576P,
    OSD_DISP_720P,
    OSD_DISP_1080I,
    OSD_DISP_1080P,
    OSD_DISP_LVDS1080P,
} OSD_DISP_MODE;

OSD_DISP_MODE get_osd_display_mode()
{
    OSD_DISP_MODE ret = OSD_DISP_1080P;
    char buf[PROPERTY_VALUE_MAX];
    memset(buf, 0, sizeof(buf));
    property_get("ubootenv.var.outputmode", buf, "1080p");
    if (!strncmp(buf, "720p", 4)) {
        ret = OSD_DISP_720P;
    } else if (!strncmp(buf, "480p", 4)) {
        ret = OSD_DISP_480P;
    } else if (!strncmp(buf, "480", 3)) { //for 480i&480cvbs
        ret = OSD_DISP_480I;
    } else if (!strncmp(buf, "576p", 4)) {
        ret = OSD_DISP_576P;
    } else if (!strncmp(buf, "576", 3)) { //for 576i&576cvbs
        ret = OSD_DISP_576I;
    } else if (!strncmp(buf, "1080i", 5)) {
        ret = OSD_DISP_1080I;
    } else if (!strncmp(buf, "1080p", 5)) {
        ret = OSD_DISP_1080P;
    } else if (!strncmp(buf, "lvds1080p", 9)) {
        ret = OSD_DISP_LVDS1080P;
    }
    return ret;
}

int get_device_win(OSD_DISP_MODE dismod, int *x, int *y, int *w, int *h)
{
    const char *prop1080i_h = "ubootenv.var.1080i_h";
    const char *prop1080i_w = "ubootenv.var.1080i_w";
    const char *prop1080i_x = "ubootenv.var.1080i_x";
    const char *prop1080i_y = "ubootenv.var.1080i_y";

    const char *prop1080p_h = "ubootenv.var.1080p_h";
    const char *prop1080p_w = "ubootenv.var.1080p_w";
    const char *prop1080p_x = "ubootenv.var.1080p_x";
    const char *prop1080p_y = "ubootenv.var.1080p_y";

    const char *prop720p_h = "ubootenv.var.720p_h";
    const char *prop720p_w = "ubootenv.var.720p_w";
    const char *prop720p_x = "ubootenv.var.720p_x";
    const char *prop720p_y = "ubootenv.var.720p_y";

    const char *prop480i_h = "ubootenv.var.480i_h";
    const char *prop480i_w = "ubootenv.var.480i_w";
    const char *prop480i_x = "ubootenv.var.480i_x";
    const char *prop480i_y = "ubootenv.var.480i_y";

    const char *prop480p_h = "ubootenv.var.480p_h";
    const char *prop480p_w = "ubootenv.var.480p_w";
    const char *prop480p_x = "ubootenv.var.480p_x";
    const char *prop480p_y = "ubootenv.var.480p_y";

    const char *prop576i_h = "ubootenv.var.576i_h";
    const char *prop576i_w = "ubootenv.var.576i_w";
    const char *prop576i_x = "ubootenv.var.576i_x";
    const char *prop576i_y = "ubootenv.var.576i_y";

    const char *prop576p_h = "ubootenv.var.576p_h";
    const char *prop576p_w = "ubootenv.var.576p_w";
    const char *prop576p_x = "ubootenv.var.576p_x";
    const char *prop576p_y = "ubootenv.var.576p_y";

    char prop_value_h[PROPERTY_VALUE_MAX];
    memset(prop_value_h, 0, PROPERTY_VALUE_MAX);
    char prop_value_w[PROPERTY_VALUE_MAX];
    memset(prop_value_w, 0, PROPERTY_VALUE_MAX);
    char prop_value_x[PROPERTY_VALUE_MAX];
    memset(prop_value_x, 0, PROPERTY_VALUE_MAX);
    char prop_value_y[PROPERTY_VALUE_MAX];
    memset(prop_value_y, 0, PROPERTY_VALUE_MAX);

    switch (dismod) {
    case OSD_DISP_1080P:
        property_get(prop1080p_h, prop_value_h, "1080");
        property_get(prop1080p_w, prop_value_w, "1920");
        property_get(prop1080p_x, prop_value_x, "0");
        property_get(prop1080p_y, prop_value_y, "0");
        break;
    case OSD_DISP_1080I:
        property_get(prop1080i_h, prop_value_h, "1080");
        property_get(prop1080i_w, prop_value_w, "1920");
        property_get(prop1080i_x, prop_value_x, "0");
        property_get(prop1080i_y, prop_value_y, "0");
        break;
    case OSD_DISP_LVDS1080P:
        property_get(prop1080p_h, prop_value_h, "1080");
        property_get(prop1080p_w, prop_value_w, "1920");
        property_get(prop1080p_x, prop_value_x, "0");
        property_get(prop1080p_y, prop_value_y, "0");
        break;
    case OSD_DISP_720P:
        property_get(prop720p_h, prop_value_h, "720");
        property_get(prop720p_w, prop_value_w, "1280");
        property_get(prop720p_x, prop_value_x, "0");
        property_get(prop720p_y, prop_value_y, "0");
        break;
    case OSD_DISP_576P:
        property_get(prop576p_h, prop_value_h, "576");
        property_get(prop576p_w, prop_value_w, "720");
        property_get(prop576p_x, prop_value_x, "0");
        property_get(prop576p_y, prop_value_y, "0");
        break;
    case OSD_DISP_576I:
        property_get(prop576i_h, prop_value_h, "576");
        property_get(prop576i_w, prop_value_w, "720");
        property_get(prop576i_x, prop_value_x, "0");
        property_get(prop576i_y, prop_value_y, "0");
        break;
    case OSD_DISP_480P:
        property_get(prop480p_h, prop_value_h, "480");
        property_get(prop480p_w, prop_value_w, "720");
        property_get(prop480p_x, prop_value_x, "0");
        property_get(prop480p_y, prop_value_y, "0");
        break;
    case OSD_DISP_480I:
        property_get(prop480i_h, prop_value_h, "480");
        property_get(prop480i_w, prop_value_w, "720");
        property_get(prop480i_x, prop_value_x, "0");
        property_get(prop480i_y, prop_value_y, "0");
        break;
    default :
        break;
    }

    LOGD("get_device_win h:%s , w:%s, x:%s, y:%s \n", prop_value_h, prop_value_w, prop_value_x, prop_value_y);
    if (h) {
        *h = atoi(prop_value_h);
    }
    if (w) {
        *w = atoi(prop_value_w);
    }
    if (x) {
        *x = atoi(prop_value_x);
    }
    if (y) {
        *y = atoi(prop_value_y);
    }
    return 0;
}

void get_axis(const char *path, int *x, int *y, int *w, int *h)
{
    //int fd = -1;
    char buf[SYSCMD_BUFSIZE];
    if (amsysfs_get_sysfs_str(path, buf, sizeof(buf)) == 0) {
        if (sscanf(buf, "%d %d %d %d", x, y, w, h) == 4) {
            LOGI("%s axis: %d %d %d %d\n", path, *x, *y, *w, *h);
        }
    }
}

int amvideo_convert_axis(int32_t* x, int32_t* y, int32_t* w, int32_t* h, int *rotation, int osd_rotation)
{
    int fb0_w, fb0_h;
    amdisplay_utils_get_size(&fb0_w, &fb0_h);
    ALOGD("amvideo_convert_axis convert before %d,%d,%d,%d -- %d,%d", *x, *y, *w, *h, *rotation, osd_rotation);
    /*if the video's width  >= fb0_w  and x == 0 , we think this a full screen video,then transfer the whole display size to decode
        either is to y == 0 and hight >= fb0_h.
        this is added for platforms which is 4:3 and hdmi mode are 16:9*/
    if (osd_rotation == 90) {
        *rotation = (*rotation + osd_rotation) % 360;
        int tmp = *w;
        *w = *h;
        *h = tmp;

        tmp = *y;
        *y = *x;
        *x = fb0_h - tmp - *w + 1;
    } else if (osd_rotation == 270) { // 270
        *rotation = (*rotation + osd_rotation) % 360;
        int tmp = *w;
        *w = *h;
        *h = tmp;

        tmp = *x;
        *x = *y;
        *y = fb0_w - tmp - *h + 1;
    } else {
        ALOGE("should no this rotation!");
    }
    ALOGD("amvideo_convert_axis convert end %d,%d,%d,%d -- %d", *x, *y, *w, *h, *rotation);
    return 0;
}

void amvideo_setscreenmode()
{

    float wh_ratio = 0;
    float ratio4_3 = 1.3333;
    //float ratio16_9 = 1.7778;
    float offset = 0.2;
    char val[PROPERTY_VALUE_MAX];
    memset(val, 0, sizeof(val));
    //int enable_fullscreen = 1;
    int default_screen_mode = -1;

    /*if(x<0 || y<0)
        return;*/

    if (property_get("tv.default.screen.mode", val, "-1") && (!(strcmp(val, "-1") == 0))) {
        default_screen_mode = atoi(val);
        if (default_screen_mode >= 0 && default_screen_mode <=3) {
            amvideo_utils_set_screen_mode(default_screen_mode);
            return ;
        }
    }

    int fs_x = 0, fs_y = 0, fs_w = 0, fs_h = 0;
    get_axis(FREE_SCALE_AXIS_PATH, &fs_x, &fs_y, &fs_w, &fs_h);

    if (fs_h > fs_w) {
        int i = fs_w;
        fs_w = fs_h;
        fs_h = i;
    }

    if (fs_h > 0) {
        wh_ratio = fs_w * (float)1.0 / fs_h;
    }

    ALOGD("amvideo_setscreenmode as %f", wh_ratio);
    if ((wh_ratio < (ratio4_3 + offset))
        && (wh_ratio > 0)
        && (is_panel_mode() == 0)) {
        amvideo_utils_set_screen_mode(1);
        ALOGD("set screen mode as 1");
    }/*else{
            amvideo_utils_set_screen_mode(0);
            ALOGD("set screen mode as 0");
    }*/
}

void set_scale(int x, int y, int w, int h, int *dst_x, int *dst_y, int *dst_w, int *dst_h, int disp_w, int disp_h)
{
    float tmp_x,tmp_y,tmp_w,tmp_h;
    tmp_x = (float)((float)((*dst_x) * w) / (float)disp_w);
    tmp_y = (float)((float)((*dst_y) * h) / (float)disp_h);
    tmp_w = (float)((float)((*dst_w) * w) / (float)disp_w);
    tmp_h = (float)((float)((*dst_h) * h) / (float)disp_h);
    *dst_x = (int)(tmp_x+0.5) + x;
    *dst_y = (int)(tmp_y+0.5) + y;
    *dst_w = (int)(tmp_w+0.5);
    *dst_h = (int)(tmp_h+0.5);
}

int amvideo_utils_set_virtual_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation)
{
    LOG_FUNCTION_NAME
    //for osd rotation, need convert the axis first
    int osd_rotation = amdisplay_utils_get_osd_rotation();
    if (osd_rotation > 0) {
        amvideo_convert_axis(&x, &y, &w, &h, &rotation, osd_rotation);
    }
#if ANDROID_PLATFORM_SDK_VERSION >= 21 //5.0
   // amSystemControlSetNativeWindowRect(x, y, w, h);
#endif

    int dev_w, dev_h, disp_w, disp_h, video_global_offset;
    int dst_x, dst_y, dst_w, dst_h;
    char buf[SYSCMD_BUFSIZE];
    //int angle_fd = -1;
    int ret = -1;
    int axis[4];
    //char enable_p2p_play[8] = {0};
    int video_on_vpp2_new = is_osd_on_vpp2_new();
    int screen_portrait = is_screen_portrait();
    int video_on_vpp2 = is_video_on_vpp2();
    int vertical_panel = is_vertical_panel();
    int vertical_panel_reverse = is_vertical_panel_reverse();
#ifndef SINGLE_EXTERNAL_DISPLAY_USE_FB1
    int hdmi_swith_on_vpp1 = is_hdmi_on_vpp1_new();
#else
    int hdmi_swith_on_vpp1 = !is_hdmi_on_vpp1_new();
#endif

    if (video_on_vpp2) {
        int fb0_w, fb0_h, fb2_w, fb2_h;

        amdisplay_utils_get_size(&fb0_w, &fb0_h);
        amdisplay_utils_get_size_fb2(&fb2_w, &fb2_h);

        if (fb0_w > 0 && fb0_h > 0 && fb2_w > 0 && fb2_h > 0) {
            if (vertical_panel) {
                int x1, y1, w1, h1;
                if (vertical_panel_reverse) {
                    x1 = (1.0 * fb2_w / fb0_h) * y;
                    y1 = (1.0 * fb2_h / fb0_w) * x;
                    w1 = (1.0 * fb2_w / fb0_h) * h;
                    h1 = (1.0 * fb2_h / fb0_w) * w;
                } else {
                    x1 = (1.0 * fb2_w / fb0_h) * y;
                    y1 = (1.0 * fb2_h / fb0_w) * (fb0_w - x - w);
                    w1 = (1.0 * fb2_w / fb0_h) * h;
                    h1 = (1.0 * fb2_h / fb0_w) * w;
                }
                x = x1;
                y = y1;
                w = w1;
                h = h1;
            } else {
                int x1, y1, w1, h1;
                x1 = (1.0 * fb2_w / fb0_w) * x;
                y1 = (1.0 * fb2_h / fb0_h) * y;
                w1 = (1.0 * fb2_w / fb0_w) * w;
                h1 = (1.0 * fb2_h / fb0_h) * h;
                x = x1;
                y = y1;
                w = w1;
                h = h1;
            }
        }
    }
#ifndef SINGLE_EXTERNAL_DISPLAY_USE_FB1
    if (screen_portrait && hdmi_swith_on_vpp1) {
        int val = 0 ;
        val = x ;
        x = y;
        y = val;
        val = w;
        w = h;
        h = val;
    }
#endif
    LOGI("amvideo_utils_set_virtual_position :: x=%d y=%d w=%d h=%d\n", x, y, w, h);

    bzero(buf, SYSCMD_BUFSIZE);

    dst_x = x;
    dst_y = y;
    dst_w = w;
    dst_h = h;

    if (amsysfs_get_sysfs_str(DISP_DEVICE_PATH, buf, sizeof(buf)) == 0) {
        if (sscanf(buf, "%dx%d", &dev_w, &dev_h) == 2) {
            LOGI("device resolution %dx%d\n", dev_w, dev_h);
        } else {
            ret = -2;
            goto OUT;
        }
    } else {
        goto OUT;
    }


    if (video_on_vpp2) {
        amdisplay_utils_get_size_fb2(&disp_w, &disp_h);
    } else {
        amdisplay_utils_get_size(&disp_w, &disp_h);
    }

    LOGI("amvideo_utils_set_virtual_position:: disp_w=%d, disp_h=%d\n", disp_w, disp_h);

    video_global_offset = amvideo_utils_get_global_offset();

    int free_scale_enable = 0;
    int ppscaler_enable = 0;
    int freescale_mode_enable = 0;

    if (((disp_w != dev_w) || (disp_h / 2 != dev_h)) &&
        (video_global_offset == 0)) {
        char val[256];
        char freescale_mode[50] = {0};
        char *p;
        memset(val, 0, sizeof(val));
        if (video_on_vpp2) {
            if (amsysfs_get_sysfs_str(FREE_SCALE_PATH_FB2, val, sizeof(val)) == 0) {
                /* the returned string should be "free_scale_enable:[0x%x]" */
                free_scale_enable = (val[21] == '0') ? 0 : 1;
            }
        } else if (hdmi_swith_on_vpp1) {
            if (amsysfs_get_sysfs_str(FREE_SCALE_PATH_FB1, val, sizeof(val)) == 0) {
                /* the returned string should be "free_scale_enable:[0x%x]" */
                free_scale_enable = (val[21] == '0') ? 0 : 1;
            }
        } else {
            if (amsysfs_get_sysfs_str(FREE_SCALE_PATH, val, sizeof(val)) == 0) {
                /* the returned string should be "free_scale_enable:[0x%x]" */
                free_scale_enable = (val[21] == '0') ? 0 : 1;
            }
        }

        memset(val, 0, sizeof(val));
        if (amsysfs_get_sysfs_str(PPSCALER_PATH, val, sizeof(val)) == 0) {
            /* the returned string should be "current ppscaler mode is disabled/enable" */
            ppscaler_enable = (val[25] == 'd') ? 0 : 1;
        }

        memset(val, 0, sizeof(val));
        if (amsysfs_get_sysfs_str(FREE_SCALE_MODE_PATH, val, sizeof(val)) == 0) {
            /* the returned string should be "free_scale_mode:new/default" */
            p = strstr(val, "current");
            if (p) {
                strcpy(freescale_mode, p);
            } else {
                freescale_mode[0] = '\0';
            }
            freescale_mode_enable = (strstr(freescale_mode, "0") == NULL) ? 1 : 0;
        }
    }

#ifndef SINGLE_EXTERNAL_DISPLAY_USE_FB1
    if ((video_on_vpp2 && vertical_panel) || (screen_portrait && video_on_vpp2_new)
        || (screen_portrait && hdmi_swith_on_vpp1))
#else
    if ((video_on_vpp2 && vertical_panel) || (screen_portrait && video_on_vpp2_new)
        /*||(screen_portrait && hdmi_swith_on_vpp1)*/)
#endif
        amsysfs_set_sysfs_int(PPMGR_ANGLE_PATH, 0);
    else {
        amsysfs_set_sysfs_int(PPMGR_ANGLE_PATH, (rotation / 90) & 3);
    }

    LOGI("set ppmgr angle :%d\n", (rotation / 90) & 3);
    /* this is unlikely and only be used when ppmgr does not exist
     * to support video rotation. If that happens, we convert the window
     * position to non-rotated window position.
     * On ICS, this might not work at all because the transparent UI
     * window is still drawn is it's direction, just comment out this for now.
     */
#if 0
    if (((rotation == 90) || (rotation == 270)) && (angle_fd < 0)) {
        if (dst_h == disp_h) {
            int center = x + w / 2;

            if (abs(center - disp_w / 2) < 2) {
                /* a centered overlay with rotation, change to full screen */
                dst_x = 0;
                dst_y = 0;
                dst_w = dev_w;
                dst_h = dev_h;

                LOGI("centered overlay expansion");
            }
        }
    }
#endif
    /*if (free_scale_enable == 0 && ppscaler_enable == 0) {

        OSD_DISP_MODE display_mode = OSD_DISP_1080P;
        int x_d=0,y_d=0,w_d=0,h_d=0;
        LOGI("set screen position:x[%d],y[%d],w[%d],h[%d]", dst_x, dst_y, dst_w, dst_h);

        display_mode = get_osd_display_mode();
        get_device_win(display_mode, &x_d, &y_d, &w_d, &h_d);
        if (display_mode == OSD_DISP_720P) {
            if ((dst_w >= 1279) || (dst_w == 0)) {
                dst_x = x_d;
                dst_y = y_d;
                dst_w = w_d;
                dst_h = h_d;
            }
            else {
                dst_x = dst_x*w_d/1280+x_d;
                dst_y = dst_y*h_d/720+y_d;
                dst_w = dst_w*w_d/1280;
                dst_h = dst_h*h_d/720;
            }
        }
        else if ((display_mode==OSD_DISP_1080I)||(display_mode==OSD_DISP_1080P)||(display_mode==OSD_DISP_LVDS1080P)) {
            if ((dst_w >= 1919) || (dst_w == 0)) {
                dst_x = x_d;
                dst_y = y_d;
                dst_w = w_d;
                dst_h = h_d;
            }
            else {//scaled to 1080p
                dst_x = dst_x*w_d/1920+x_d;
                dst_y = dst_y*h_d/1080+y_d;
                dst_w = dst_w*w_d/1920;
                dst_h = dst_h*h_d/1080;

                LOGI("after scaled to 1080 ,set screen position:x[%d],y[%d],w[%d],h[%d]", dst_x, dst_y, dst_w, dst_h);
            }
        }
        else if ((display_mode==OSD_DISP_480I)||(display_mode==OSD_DISP_480P)) {
            if ((dst_w >= 719) || (dst_w == 0)) {
                dst_x = x_d;
                dst_y = y_d;
                dst_w = w_d;
                dst_h = h_d;
            }
            else {//scaled to 480p/480i
                dst_x = dst_x*w_d/720+x_d;
                dst_y = dst_y*h_d/480+y_d;
                dst_w = dst_w*w_d/720;
                dst_h = dst_h*h_d/480;

                LOGI("after scaled to 480,set screen position:x[%d],y[%d],w[%d],h[%d]", dst_x, dst_y, dst_w, dst_h);
            }
        }
        else if ((display_mode==OSD_DISP_576I)||(display_mode==OSD_DISP_576P)) {
            if ((dst_w >= 719) || (dst_w == 0)) {
                dst_x = x_d;
                dst_y = y_d;
                dst_w = w_d;
                dst_h = h_d;
            }
            else {//scaled to 576p/576i
                dst_x = dst_x*w_d/720+x_d;
                dst_y = dst_y*h_d/576+y_d;
                dst_w = dst_w*w_d/720;
                dst_h = dst_h*h_d/576;

                LOGI("after scaled to 576 ,set screen position:x[%d],y[%d],w[%d],h[%d]", dst_x, dst_y, dst_w, dst_h);
            }
        }
    }*/

    if (free_scale_enable == 0 && ppscaler_enable == 0) {
        char val[256];
        char axis_string[100];
        char num[11]="0123456789\0";
        char *first_num, *last_num;

        if (freescale_mode_enable == 1) {
            int left = 0, top = 0, right = 0, bottom = 0;
            int x = 0, y = 0, w = 0, h = 0;

            memset(val, 0, sizeof(val));
            if (amsysfs_get_sysfs_str(WINDOW_AXIS_PATH, val, sizeof(val)) == 0
                && (is_panel_mode() == 0)) {
                /* the returned string should be "window axis is [a b c d]" */
                first_num = strpbrk(val,num);
                if (first_num != NULL) {
                    memset(axis_string, 0, sizeof(axis_string));
                    strcpy(axis_string, first_num);
                    last_num = strchr(axis_string, ']');
                    if (last_num) {
                        *last_num = '\0';
                    }
                }
                if (sscanf(axis_string, "%d %d %d %d", &left, &top, &right, &bottom) == 4) {
                    if ((right > 0) && (bottom > 0)) {
                        x = left;
                        y = top;
                        w = right - left + 1;
                        h = bottom - top + 1;

                        dst_x = dst_x * w / dev_w + x;
                        dst_y = dst_y * h / dev_h + y;
                        LOGI("after scaled, screen position1: %d %d %d %d", dst_x, dst_y, dst_w, dst_h);
                    }
                }
            }
        } else {
            int x = 0, y = 0, w = 0, h = 0;
            int fb_w = 0, fb_h = 0;
            //int req_2xscale = 0;

            memset(val, 0, sizeof(val));
            if (amsysfs_get_sysfs_str(PPSCALER_RECT, val, sizeof(val)) == 0) {
                /* the returned string should be "a b c" */
                if (sscanf(val, "ppscaler rect:\nx:%d,y:%d,w:%d,h:%d", &x, &y, &w, &h) == 4) {
                    if ((w > 1) && (h > 1)) {
                        if (fb_w == 0  || fb_h == 0) {
                            fb_w = 1280;
                            fb_h = 720;
                        }
                        set_scale(x, y, w - 1, h - 1, &dst_x, &dst_y, &dst_w, &dst_h, fb_w, fb_h);
                        LOGI("after scaled, screen position2: %d %d %d %d", dst_x, dst_y, dst_w, dst_h);
                    }
                }
            }
        }
    } else if (free_scale_enable == 1 && ppscaler_enable == 0) {
        char val[256];
        char axis_string[100];
        char num[11]="0123456789\0";
        char *first_num, *last_num;
        int left = 0, top = 0, right = 0, bottom = 0;
        int x = 0, y = 0, w = 0, h = 0;
        int freescale_x = 0, freescale_y = 0, freescale_w = 0, freescale_h = 0;

        int mGetWinAxis = 0;
        if (hdmi_swith_on_vpp1) {
            mGetWinAxis = amsysfs_get_sysfs_str(WINDOW_AXIS_PATH_FB1, val, sizeof(val));
        } else {
            mGetWinAxis = amsysfs_get_sysfs_str(WINDOW_AXIS_PATH, val, sizeof(val));
        }

        if (mGetWinAxis == 0) {
            /* the returned string should be "window axis is [a b c d]" */
            first_num = strpbrk(val,num);
            if (first_num != NULL) {
                memset(axis_string, 0, sizeof(axis_string));
                strcpy(axis_string, first_num);
                last_num = strchr(axis_string, ']');
                if (last_num) {
                    *last_num = '\0';
                }
            }
            if (sscanf(axis_string, "%d %d %d %d", &left, &top, &right, &bottom) == 4) {
                x = left;
                y = top;
                w = right - left + 1;
                h = bottom - top + 1;

                if (hdmi_swith_on_vpp1) {
                    freescale_x = 0;
                    freescale_y = 0;
                    if (screen_portrait) {
                        freescale_w = disp_h;
                        freescale_h = disp_w;
                    } else {
                        freescale_w = disp_w;
                        freescale_h = disp_h;
                    }
                } else {
                    get_axis(FREE_SCALE_AXIS_PATH, &freescale_x, &freescale_y, &freescale_w, &freescale_h);
                }
                freescale_w = (freescale_w + 1) & (~1);
                freescale_h = (freescale_h + 1) & (~1);

                set_scale(x, y, w, h, &dst_x, &dst_y, &dst_w, &dst_h, freescale_w, freescale_h);
                LOGI("after scaled, screen position3: %d %d %d %d", dst_x, dst_y, dst_w, dst_h);
            }
        }
    }

    axis[0] = dst_x;
    axis[1] = dst_y;
    axis[2] = dst_x + dst_w - 1;
    axis[3] = dst_y + dst_h - 1;
    sprintf(buf, "%d %d %d %d", axis[0], axis[1], axis[2], axis[3]);
    amsysfs_set_sysfs_str(VIDEO_AXIS_PATH, buf);

    ret = 0;
OUT:

    LOGI("amvideo_utils_set_virtual_position (corrected):: x=%d y=%d w=%d h=%d\n", dst_x, dst_y, dst_w, dst_h);
    amvideo_setscreenmode();

    return ret;
}

int amvideo_utils_set_absolute_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation)
{
    LOG_FUNCTION_NAME
    char buf[SYSCMD_BUFSIZE];
    int axis[4];
    int video_on_vpp2 = is_video_on_vpp2();
    int vertical_panel = is_vertical_panel();

    LOGI("amvideo_utils_set_absolute_position:: x=%d y=%d w=%d h=%d\n", x, y, w, h);

    if ((video_on_vpp2 && vertical_panel)) {
        amsysfs_set_sysfs_int(PPMGR_ANGLE_PATH, 0);
    } else {
        amsysfs_set_sysfs_int(PPMGR_ANGLE_PATH, (rotation / 90) & 3);
    }

    axis[0] = x;
    axis[1] = y;
    axis[2] = x + w - 1;
    axis[3] = y + h - 1;

    sprintf(buf, "%d %d %d %d", axis[0], axis[1], axis[2], axis[3]);
    amsysfs_set_sysfs_str(VIDEO_AXIS_PATH, buf);

    return 0;
}

int amvideo_utils_get_position(int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
    LOG_FUNCTION_NAME
    int axis[4];
    get_axis(VIDEO_AXIS_PATH, &axis[0], &axis[1], &axis[2], &axis[3]);
    *x = axis[0];
    *y = axis[1];
    *w = axis[2] - axis[0] + 1;
    *h = axis[3] - axis[1] + 1;

    return 0;
}

int amvideo_utils_get_screen_mode(int *mode)
{
    LOG_FUNCTION_NAME
    int video_fd;
    int screen_mode = 0;

    video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        return -1;
    }

    ioctl(video_fd, AMSTREAM_IOC_GET_SCREEN_MODE, &screen_mode);

    close(video_fd);

    *mode = screen_mode;

    return 0;
}

int amvideo_utils_set_screen_mode(int mode)
{
    LOG_FUNCTION_NAME
    int screen_mode = mode;
    int video_fd;

    video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        return -1;
    }

    ioctl(video_fd, AMSTREAM_IOC_SET_SCREEN_MODE, &screen_mode);

    close(video_fd);

    return 0;
}

int amvideo_utils_get_video_angle(int *angle)
{
    LOG_FUNCTION_NAME
    char buf[SYSCMD_BUFSIZE];
    int angle_val;
    if (amsysfs_get_sysfs_str(PPMGR_ANGLE_PATH, buf, sizeof(buf)) == 0) {
        if (sscanf(buf, "current angel is %d", &angle_val) == 1) {
            *angle = angle_val;
        }
    }
    return 0;
}

int amvideo_utils_get_hdmi_authenticate(void)
{
    LOG_FUNCTION_NAME
    int fd = -1;
    int val = -1;
    char  bcmd[16];
    fd = open(HDMI_AUTHENTICATE_PATH, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    }
    return val;
}
