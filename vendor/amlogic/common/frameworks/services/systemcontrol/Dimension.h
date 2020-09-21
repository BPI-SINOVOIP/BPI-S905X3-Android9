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
 *  @author   XiaoLiang.Wang
 *  @version  1.0
 *  @date     2016/06/13
 *  @par function description:
 *  - 1 3d set for player, include tv and mbox set
 */

 #ifndef ANDROID_DIMENSION_H
#define ANDROID_DIMENSION_H

#include "DisplayMode.h"
#include "SysWrite.h"
#include "common.h"

#include <sys/ioctl.h>

//should sync with SurfaceFlinger.h
#define SURFACE_3D_OFF                  0
#define SURFACE_3D_SIDE_BY_SIDE         0x01//8
#define SURFACE_3D_TOP_BOTTOM           0x02//16
#define SURFACE_3D_FRAME_PACKING        0x04//32

#define VIDEO_3D_OFF                    "3doff"
#define VIDEO_3D_SIDE_BY_SIDE           "3dlr"
#define VIDEO_3D_TOP_BOTTOM             "3dtb"
#define VIDEO_3D_FRAME_PACKING          "3dfp"

#define KEY_SIDE_BY_SIDE                "SidebySide"
#define KEY_TOP_BOTTOM                  "TopBottom"
#define KEY_FRAME_PACKING               "FramePacking"

//should bigger than DISPLAY_MODE_TOTAL 28
#define NUM_MAX                         64
#define LINE_LEN                        128

//should sync with vendor\amlogic\frameworks\av\LibPlayer\amcodec\include\amports\Amstream.h
#define VIDEO_PATH                      "/dev/amvideo"
#define DI_BYPASS_ALL                   "/sys/module/di/parameters/bypass_all"
#define DI_BYPASS_POST                  "/sys/module/di/parameters/bypass_post"
#define DET3D_MODE_SYSFS                "/sys/module/di/parameters/det3d_mode"
#define PROG_PROC_SYSFS                 "/sys/module/di/parameters/prog_proc_config"

#define AMSTREAM_IOC_MAGIC   'S'
#define AMSTREAM_IOC_SET_3D_TYPE                    _IOW((AMSTREAM_IOC_MAGIC), 0x3c, unsigned int)
#define AMSTREAM_IOC_GET_3D_TYPE                    _IOW((AMSTREAM_IOC_MAGIC), 0x3d, unsigned int)
#define AMSTREAM_IOC_GET_SOURCE_VIDEO_3D_TYPE       _IOW((AMSTREAM_IOC_MAGIC), 0x3e, unsigned int)

//should sync with common\drivers\amlogic\amports\Vpp.h
/*when the output mode is field alterlative*/
/* LRLRLRLRL mode */
#define MODE_3D_OUT_FA_L_FIRST      0x00001000
#define MODE_3D_OUT_FA_R_FIRST      0x00002000
/* LBRBLBRB */
#define MODE_3D_OUT_FA_LB_FIRST     0x00004000
#define MODE_3D_OUT_FA_RB_FIRST     0x00008000

#define MODE_3D_OUT_FA_MASK \
    (MODE_3D_OUT_FA_L_FIRST | \
    MODE_3D_OUT_FA_R_FIRST|MODE_3D_OUT_FA_LB_FIRST|MODE_3D_OUT_FA_RB_FIRST)

#define MODE_3D_DISABLE             0x00000000
#define MODE_3D_ENABLE              0x00000001
#define MODE_3D_AUTO                (0x00000002 | MODE_3D_ENABLE)
#define MODE_3D_LR                  (0x00000004 | MODE_3D_ENABLE)
#define MODE_3D_TB                  (0x00000008 | MODE_3D_ENABLE)
#define MODE_3D_LA                  (0x00000010 | MODE_3D_ENABLE)
#define MODE_3D_FA                  (0x00000020 | MODE_3D_ENABLE)
#define MODE_3D_LR_SWITCH           (0x00000100 | MODE_3D_ENABLE)
#define MODE_3D_TO_2D_L             (0x00000200 | MODE_3D_ENABLE)
#define MODE_3D_TO_2D_R             (0x00000400 | MODE_3D_ENABLE)
#define MODE_3D_MVC                 (0x00000800 | MODE_3D_ENABLE)
#define MODE_3D_OUT_TB              (0x00010000 | MODE_3D_ENABLE)
#define MODE_3D_OUT_LR              (0x00020000 | MODE_3D_ENABLE)
#define MODE_FORCE_3D_TO_2D_LR      (0x00100000 | MODE_3D_ENABLE)
#define MODE_FORCE_3D_TO_2D_TB      (0x00200000 | MODE_3D_ENABLE)

#define VPP_3D_MODE_NULL 0x0
#define VPP_3D_MODE_LR 0x1
#define VPP_3D_MODE_TB 0x2
#define VPP_3D_MODE_LA 0x3
#define VPP_3D_MODE_FA 0x4

#define RETRY_MAX 5

enum {
    FORMAT_3D_OFF                           = 0,
    FORMAT_3D_AUTO                          = 1,
    FORMAT_3D_SIDE_BY_SIDE                  = 2,
    FORMAT_3D_TOP_AND_BOTTOM                = 3,
    FORMAT_3D_LINE_ALTERNATIVE              = 4,
    FORMAT_3D_FRAME_ALTERNATIVE             = 5,
    FORMAT_3D_TO_2D_LEFT_EYE                = 6,
    FORMAT_3D_TO_2D_RIGHT_EYE               = 7,
    FORMAT_3D_SIDE_BY_SIDE_FORCE            = 8,
    FORMAT_3D_TOP_AND_BOTTOM_FORCE          = 9
};

namespace android {
// ----------------------------------------------------------------------------
class Dimension
{
public:
    Dimension(DisplayMode *displayMode, SysWrite *sysWrite);
    ~Dimension();

    void setLogLevel(int level);

    // for mbox
    int32_t set3DMode(const char* mode3d);
    int32_t getDisplay3DFormat(void);
    bool setDisplay3DFormat(int format);

    bool isSupportFramePacking(const char* mode3d);

    //for tv
    void init3DSetting(void);
    int32_t getVideo3DFormat(void);
    int32_t getDisplay3DTo2DFormat(void);
    bool setDisplay3DTo2DFormat(int format);
    bool setOsd3DFormat(int format);
    bool switch3DTo2D(int format);
    bool switch2DTo3D(int format);
    void autoDetect3DForMbox();
    static void* detect3DThread(void* data);
    int dump(char *result);

private:
    void init(void);
    int32_t getLineTotal(char* buf);
    void getLine(int idx, int total, char* src, char* dst);
    void parseLine(int idx, char *line);
    void setDispMode(const char* mode3d);
    void setWindowAxis(const char* mode3d);
    void mode3DImpl(const char* mode3d);
    void get3DFormatStr(int format, char *str);
    unsigned int get3DOperationByFormat(int format);
    int get3DFormatByOperation(unsigned int operation);
    int get3DFormatByVpp(int vpp3Dformat);
    void setDiBypassAll(int format);

    char mMode3d[32];//this used for video 3d set
    bool mInitDone;
    int mLogLevel;
    int mDisplay3DFormat;
    DisplayMode *pDisplayMode;
    SysWrite *pSysWrite;
    HDCPTxAuth *pTxAuth;

    struct info_t {
        char mode[MODE_LEN];
        char list[LINE_LEN];
        int mo;
        int hz;
    };
    struct support_info_t {
        int total;
        char stored[MODE_LEN];//display mode for resume
        char dst[MODE_LEN];
        info_t info[NUM_MAX];
    };
    support_info_t mSupport;

    struct axis_t {
        int x0;
        int y0;
        int x1;
        int y1;
    };
    struct window_axis_t {
        axis_t stored;//original window axis stored for resume
        axis_t dst;
    };
    window_axis_t mWindowAxis;
};
// ----------------------------------------------------------------------------
} // namespace android
#endif
