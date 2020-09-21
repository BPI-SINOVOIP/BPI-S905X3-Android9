/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DROID_LOGIC_SERVER_HDMIIN
 */

#ifndef _DROID_LOGIC_SERVER_HDMIIN_H
#define _DROID_LOGIC_SERVER_HDMIIN_H

#define DISABLE_VIDEO_PATH          "/sys/class/video/disable_video"
#define DISPLAY_MODE_PATH           "/sys/class/display/mode"
#define WINDOW_AXIS_PATH            "/sys/class/graphics/fb0/window_axis"
#define FREESCALE_PATH              "/sys/class/graphics/fb0/free_scale"
#define VFM_MAP_PATH                "/sys/class/vfm/map"
#define VIDEO_FRAME_COUNT_PATH      "/sys/module/amvideo/parameters/new_frame_count"
#define HDMIIN_ON_VIDEO_PATH        "/sys/module/amvideo/parameters/hdmi_in_onvideo"
#define BYPASS_PROG_PATH            "/sys/module/di/parameters/bypass_prog"
#define FREESCALE_AXIS_PATH         "/sys/class/graphics/fb0/free_scale_axis"
#define FREESCALE_1_PATH            "/sys/class/graphics/fb1/free_scale"
#define FREESCALE_1_AXIS_PATH       "/sys/class/graphics/fb1/free_scale_axis"
#define OSD_PATH                    "/sys/class/graphics/fb0/blank"
#define OSD_1_PATH                  "/sys/class/graphics/fb1/blank"
#define VIDEO_AXIS_PATH             "/sys/class/video/axis"
#define PPSCALER_PATH               "/sys/class/ppmgr/ppscaler"
#define PPSCALER_RECT_PATH          "/sys/class/ppmgr/ppscaler_rect"
#define SCREEN_MODE_PATH            "/sys/class/video/screen_mode"
#define RECORD_TYPE_PATH            "/sys/class/amaudio/record_type"

//for it660x
#define IT660X_CLASS_PATH           "/sys/class/it660x/it660x_hdmirx0/"
#define IT660X_PARAM_PATH           "/sys/module/tvin_it660x/parameters/"
//for mt box
#define MTBOX_CLASS_PATH            "/sys/class/vdin_ctrl/vdin_ctrl0/"
#define MTBOX_PARAM_PATH            "/sys/module/tvin_it660x/parameters/"
// for sii9233a
#define SII9233A_CLASS_PATH         "/sys/class/sii9233a/"
#define SII9233A_PARAM_PATH         "/sys/class/sii9233a/"
// for sii9293
#define SII9293_CLASS_PATH          "/sys/class/sii9293/"
#define SII9293_PARAM_PATH          "/sys/class/sii9293/"

#define PROP_MUTE                   ("sys.hdmiin.mute")
#define PROP_HDMIIN_VIDEOLAYER      ("mbx.hdmiin.videolayer")
#define PROP_HDMIIN_PPMGR           ("sys.hdmiin.ppmgr")

#endif // _DROID_LOGIC_SERVER_HDMIIN_H
