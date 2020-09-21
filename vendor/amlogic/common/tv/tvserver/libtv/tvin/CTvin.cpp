/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvin"

#include "CTvin.h"
#include <CTvLog.h>
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
#include <cutils/log.h>
#include <sys/mman.h>
#include "CAv.h"
#include "../tvsetting/CTvSetting.h"
#include <tvutils.h>
#include <tvconfig.h>

#define AFE_DEV_PATH        "/dev/tvafe0"
#define AMLVIDEO2_DEV_PATH  "/dev/video11"
#define VDIN0_ATTR_PATH     "/sys/class/vdin/vdin0/attr"
#define VDIN1_ATTR_PATH     "/sys/class/vdin/vdin1/attr"



#define CC_SEL_VDIN_DEV   (0)

/* ADC calibration pattern & format define */
/* default 100% 8 color-bar */
//#define VGA_SOURCE_RGB444
#define VGA_H_ACTIVE    (1024)
#define VGA_V_ACTIVE    (768)
#define COMP_H_ACTIVE   (1280)
#define COMP_V_ACTIVE   (720)
#define CVBS_H_ACTIVE   (720)
#define CVBS_V_ACTIVE   (480)

int CTvin::mSourceInputToPortMap[SOURCE_MAX];
CTvin *CTvin::mInstance;

CTvin *CTvin::getInstance()
{
    if (NULL == mInstance) mInstance = new CTvin();
    return mInstance;
}

CTvin::CTvin() : mAfeDevFd(-1), mVdin0DevFd(-1), mSourcePort(TVIN_PORT_NULL)
{
    m_snow_status = false;

    memset(&gTvinAFEParam, 0, sizeof(gTvinAFEParam));
    memset(&gTvinAFESignalInfo, 0, sizeof(gTvinAFESignalInfo));
    memset(&m_tvin_param, 0, sizeof(m_tvin_param));

    memset(&gTvinVDINParam, 0, sizeof(gTvinVDINParam));
    memset(&gTvinVDINSignalInfo, 0, sizeof(gTvinVDINSignalInfo));

    mDecoderStarted = false;
    gVideoPath[0] = '\0';

    Tvin_LoadSourceInputToPortMap();
    init_vdin();
}

CTvin::~CTvin()
{
}

int CTvin::OpenTvin()
{
    const char *config_value;
    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_TVPATH_SET_MANUAL, "null" );
    strcpy ( config_tv_path, config_value );
    memset ( config_default_path, 0x0, 64 );
    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_TVPATH_SET_DEFAULT, "null" );
    strcpy ( config_default_path, config_value );

    config_value = config_get_str(CFG_SECTION_TV, CFG_TVIN_DISPLAY_RESOLUTION_FIRST, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbResolutionPriority = true;
    } else {
        mbResolutionPriority = false;
    }

    return 0;
}

int CTvin::setMpeg2Vdin(int enable)
{
    /* let output data loop to vdin */
    char str[32] = {0};
    sprintf (str, "%d", enable);
    return tvWriteSysfs("/sys/module/di/parameters/mpeg2vdin_en", str);
}

int CTvin::VDIN_AddPath ( const char *videopath )
{
    if (strlen(videopath) > 1024) {
        LOGE("video path too long\n");
        return -1;
    }

    char str[1024 + 1] = {0};
    sprintf (str, "%s", videopath);
    return tvWriteSysfs(SYS_VFM_MAP_PATH, str);
}

int CTvin::VDIN_RmDefPath ( void )
{
    return tvWriteSysfs(SYS_VFM_MAP_PATH, "rm default");
}

int CTvin::VDIN_RmTvPath ( void )
{
    return tvWriteSysfs(SYS_VFM_MAP_PATH, "rm tvpath");
}

int CTvin::VDIN_AddVideoPath ( int selPath )
{
    int ret = -1;
    std::string vdinPath;
    std::string suffixVideoPath("deinterlace amvideo");
    bool amlvideo2Exist = isFileExist(AMLVIDEO2_DEV_PATH);
    switch ( selPath ) {
    case TV_PATH_VDIN_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO:
        if (amlvideo2Exist)
            vdinPath = "add tvpath vdin0 amlvideo2.0 ";
        else
            vdinPath = "add tvpath vdin0 ";
        break;

    case TV_PATH_DECODER_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO:
        if (amlvideo2Exist)
            vdinPath = "add default decoder amlvideo2.0 ppmgr ";
        else
            vdinPath = "add default decoder ppmgr ";
        break;
    default:
        break;
    }

    vdinPath += suffixVideoPath;
    ret = VDIN_AddPath (vdinPath.c_str());
    return ret;
}

int CTvin::VDIN_GetVdinDeviceFd() {
    if (mVdin0DevFd < 0) {
        mVdin0DevFd = VDIN_OpenModule();
    }

    return mVdin0DevFd;
}

int CTvin::VDIN_OpenModule()
{
    char file_name[64] = {0};
    sprintf ( file_name, "/dev/vdin%d", CC_SEL_VDIN_DEV );
    int fd = open ( file_name, O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "Open %s error(%s)!\n", file_name, strerror ( errno ) );
        return -1;
    }

    LOGD ( "Open vdin%d module fd = [%d]", CC_SEL_VDIN_DEV, fd );

    return fd;
}

int CTvin::VDIN_CloseModule()
{
    if ( mVdin0DevFd != -1 ) {
        close ( mVdin0DevFd );
        mVdin0DevFd = -1;
    }

    return 0;
}

int CTvin::VDIN_DeviceIOCtl ( int request, ... )
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;

    if (mVdin0DevFd < 0) {
        mVdin0DevFd = VDIN_OpenModule();
    }

    if ( mVdin0DevFd >= 0 ) {
        va_start ( ap, request );
        arg = va_arg ( ap, void * );
        va_end ( ap );

        tmp_ret = ioctl ( mVdin0DevFd, request, arg );
        return tmp_ret;
    }

    return -1;
}

int CTvin::VDIN_OpenPort ( tvin_port_t port )
{
    struct tvin_parm_s vdinParam;
    vdinParam.port = port;
    vdinParam.index = 0;
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_OPEN, &vdinParam );
    if ( rt < 0 ) {
        LOGW ( "Vdin open port, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_ClosePort()
{
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_CLOSE );
    if ( rt < 0 ) {
        LOGW ( "Vdin close port, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_StartDec ( const struct tvin_parm_s *vdinParam )
{
    if ( vdinParam == NULL ) {
        return -1;
    }

    LOGD ( "VDIN_StartDec: index = [%d] port = [0x%x] format = [0x%x]\n",
        vdinParam->index, ( unsigned int ) vdinParam->port, ( unsigned int ) ( vdinParam->info.fmt ));
    if ( !isVideoInuse() ) {
        return -1;
    }
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_START_DEC, vdinParam );
    if ( rt < 0 ) {
        LOGW ( "Vdin start decode, error(%s)!\n", strerror ( errno ) );
        tvWriteSysfs(SYS_VIDEO_INUSE_PATH, 0);
    }

    return rt;
}

int CTvin::VDIN_StopDec()
{
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_STOP_DEC );
    if ( rt < 0 ) {
        LOGW ( "Vdin stop decode, error(%s)", strerror ( errno ) );
    } else {
        tvWriteSysfs(SYS_VIDEO_INUSE_PATH, 0);
    }
    return rt;
}

int CTvin::VDIN_GetSignalInfo ( struct tvin_info_s *SignalInfo )
{
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_G_SIG_INFO, SignalInfo );
    if ( rt < 0 ) {
        LOGW ( "Vdin get signal info, error(%s), ret = %d.\n", strerror ( errno ), rt );
    }
    return rt;
}

int CTvin::VDIN_SetVdinParam ( const struct tvin_parm_s *vdinParam )
{
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_S_PARM, vdinParam );
    if ( rt < 0 ) {
        LOGW ( "Vdin set signal param, error(%s)\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_GetVdinParam ( const struct tvin_parm_s *vdinParam )
{
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_G_PARM, vdinParam );
    if ( rt < 0 ) {
        LOGW ( "Vdin get signal param, error(%s)\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_GetDisplayVFreq (int need_freq, int *iSswitch, char *display_mode)
{
    int ret = -1;
    char buf[32] = {0};
    char CurrentDisplayMode[32] = {0};
    ret = tvReadSysfs(SYS_DISPLAY_MODE_PATH, CurrentDisplayMode);
    if ((ret < 0) || (strlen(CurrentDisplayMode) == 0)) {
        LOGW("Read /sys/class/display/mode failed!\n");
        *iSswitch = 0;
        return ret;
    }

    ret = 0;
    LOGD( "%s: current display mode: %s\n", __FUNCTION__, CurrentDisplayMode);
    if (strstr(CurrentDisplayMode, "480cvbs") || strstr(CurrentDisplayMode, "576cvbs")) {//hdmi plug out
        *iSswitch = 0;
        LOGD("%s, plug out, no set!\n", __FUNCTION__);
    } else if (strstr(display_mode, "panel") != NULL) {//panel output
        LOGD("%s, Now is panel mode!\n", __FUNCTION__);
        *iSswitch = 1;
        strcpy(display_mode, CurrentDisplayMode);
    } else {
        //get curentFreq
        std::string TempStr = std::string(CurrentDisplayMode);
        std::string TimingStr = TempStr.substr(0, TempStr.length()-4);
        std::string FreqStr = TempStr.substr(TempStr.length()-4,2);
        int CurrentFreq = atoi(FreqStr.c_str());

        //check output mode
        std::vector<std::string> SupportDisplayModes;
        ret = VDIN_GetDisplayModeSupportList(&SupportDisplayModes);
        if (ret == 1) {//lvds output
            //calculate newFreq
            int NewFreq = 50;
            if ((30 == need_freq) || (60 == need_freq) || (24 == need_freq)) {
                NewFreq = 60;
            } else {
                NewFreq = 50;
            }

            if (NewFreq != CurrentFreq) {
                *iSswitch = 1;
                memset(buf, 0, strlen(buf));
                sprintf(buf, "%s%dhz", TimingStr.c_str(), NewFreq);
                strcpy(display_mode, buf);
            } else {
                *iSswitch = 0;
            }
        } else if (ret == 0) {//tx output
            memset(buf, 0, strlen(buf));
            sprintf(buf, "%s%dhz", TimingStr.c_str(), need_freq);
            std::string DisplayModeStr = buf;
            /*for (std::vector<std::string>::iterator it1 = SupportDisplayModes.begin(); it1 != SupportDisplayModes.end(); it1++) {
                LOGD("%s: %s\n", __FUNCTION__, (*it1).c_str());
            }*/
            //check output device support or not
            std::vector<std::string>::iterator result = find(SupportDisplayModes.begin( ), SupportDisplayModes.end( ), DisplayModeStr);
            if (result != SupportDisplayModes.end()) {
                LOGD("%s: Output device support %s!\n", __FUNCTION__, buf);
            } else {
                LOGD("%s: Output device don't support %s!\n", __FUNCTION__, buf);
                memset(buf, 0, strlen(buf));
                if (mbResolutionPriority) {//resolution first priority
                    VDIN_GetBestDisplayMode(BEST_DISPLAYMODE_CONDITION_RESOLUTION, TimingStr, SupportDisplayModes, buf);
                } else {//frame rate first priority
                    std::string FrameRateStr = std::to_string(need_freq);
                    VDIN_GetBestDisplayMode(BEST_DISPLAYMODE_CONDITION_FRAMERATE, FrameRateStr, SupportDisplayModes, buf);
                }
            }

            if (strstr(CurrentDisplayMode, buf) != NULL) {//same displaymode
                *iSswitch = 0;
            } else {
                *iSswitch = 1;
                strcpy(display_mode, buf);
            }
        } else {
            LOGD("%s: Get TxMode SupportList failed!\n", __FUNCTION__);
            *iSswitch = 0;
        }
    }

    return ret;
}

int CTvin::VDIN_SetDisplayVFreq(int freq)
{
    int iSswitch = 0;
    char display_mode[32] = {0};

    int ret = VDIN_GetDisplayVFreq(freq, &iSswitch, display_mode);
    if (ret < 0) {
        LOGE("%s: VDIN_GetDisplayVFreq failed!\n", __FUNCTION__);
        return -1;
    } else {
        if (0 == iSswitch) {
            LOGW("%s: same display mode, no need set!\n", __FUNCTION__);
            return 0;
        } else {
            if (strstr(display_mode, "panel") != NULL) {//Panel
                LOGD("%s, Now is panel mode!\n", __FUNCTION__);
                tvWriteSysfs(SYS_PANEL_FRAME_RATE, freq, 10);
            } else {
                LOGD("%s: set display mode to %s\n", __FUNCTION__, display_mode);
                tvWriteDisplayMode(display_mode);
            }
        }

        return 0;
    }
}

int CTvin::VDIN_Get_avg_luma(void)
{
    unsigned int lum_sum, pixel_sum, luma_avg;
    struct tvin_parm_s vdinParam;

    if ( 0 == VDIN_GetVdinParam( &vdinParam )) {
        lum_sum = vdinParam.luma_sum;
        pixel_sum = vdinParam.pixel_sum * 2;
        if (pixel_sum != 0 && mDecoderStarted) {
            luma_avg = lum_sum / pixel_sum;
        } else {
            luma_avg = 116;
        }
    } else {
        return -1;
    }
    LOGD ( "VDIN_get_avg_lut lum_sum =%d,pixel_sum=%d,lut_avg=%d\n", lum_sum, pixel_sum, luma_avg);
    return luma_avg;
}

int CTvin::VDIN_SetMVCViewMode ( int mode )
{
    char str[32] = {0};
    sprintf (str, "%d", mode);
    tvWriteSysfs("/sys/module/amvdec_h264mvc/parameters/view_mode", str);
    return 0;
}

int CTvin::VDIN_GetMVCViewMode ( void )
{
    char buf[32] = {0};

    tvReadSysfs("/sys/module/amvdec_h264mvc/parameters/view_mode", buf);
    int mode = atoi(buf);
    return mode;
}

int CTvin::VDIN_SetDIBuffMgrMode ( int mgr_mode )
{
    char str[32] = {0};
    sprintf (str, "%d", mgr_mode);
    tvWriteSysfs("sys/module/di/parameters/buf_mgr_mode", str);
    return 0;
}

int CTvin::VDIN_SetDICFG ( int cfg )
{
    tvWriteSysfs("sys/class/deinterlace/di0/config", (0 == cfg)?"disable":"enable");
    return 0;
}

int CTvin::VDIN_SetDI3DDetc ( int enable )
{
    char str[32] = {0};
    sprintf (str, "%d", enable);
    tvWriteSysfs("/sys/module/di/parameters/det3d_en", str);
    return 0;
}

int CTvin::VDIN_Get3DDetc ( void )
{
    char buf[32] = {0};

    tvReadSysfs("/sys/module/di/parameters/det3d_en", buf);
    if ( strcmp ( "enable", buf ) == 0 ) {
        return 1;
    }
    return 0;
}


int CTvin::VDIN_GetVscalerStatus ( void )
{
    char buf[32] = {0};

    tvReadSysfs("/sys/class/video/vscaler", buf);
    int ret = atoi(buf);

    ret = ( ( ret & 0x40000 ) == 0 ) ? 1 : 0;
    if ( ret == 1 ) {
        sleep ( 1 );
    }

    return ret;
}

int CTvin::VDIN_TurnOnBlackBarDetect ( int isEnable )
{
    char str[32] = {0};
    sprintf (str, "%d", isEnable);
    tvWriteSysfs("/sys/module/tvin_vdin/parameters/black_bar_enable", str);
    return 0;
}

int CTvin::VDIN_SetVideoFreeze ( int enable )
{
    tvWriteSysfs(VDIN0_ATTR_PATH, (enable == 1)?"freeze":"unfreeze");
    return 0;
}

int CTvin::VDIN_SetDIBypasshd ( int enable )
{
    char str[32] = {0};
    sprintf (str, "%d", enable);
    tvWriteSysfs("/sys/module/di/parameters/bypass_hd", str);
    return 0;
}

int CTvin::VDIN_SetDIBypassAll ( int enable )
{
    char str[32] = {0};
    sprintf (str, "%d", enable);
    tvWriteSysfs("/sys/module/di/parameters/bypass_all", str);
    return 0;
}

int CTvin::VDIN_SetDIBypass_Get_Buf_Threshold ( int enable )
{
    char str[32] = {0};
    sprintf (str, "%d", enable);
    tvWriteSysfs("/sys/module/di/parameters/bypass_get_buf_threshold", str);
    return 0;
}

int CTvin::VDIN_SetDIBypassProg ( int enable )
{
    char str[32] = {0};
    sprintf (str, "%d", enable);
    tvWriteSysfs("/sys/module/di/parameters/bypass_prog", str);
    return 0;
}

int CTvin::VDIN_SetDIBypassDynamic ( int flag )
{
    char str[32] = {0};
    sprintf (str, "%d", flag);
    tvWriteSysfs("/sys/module/di/parameters/bypass_dynamic", str);
    return 0;
}

int CTvin::VDIN_EnableRDMA ( int enable )
{
    char str[32] = {0};
    sprintf (str, "%d", enable);
    tvWriteSysfs("/sys/module/rdma/parameters/enable", str);
    return 0;
}

int CTvin::VDIN_SetColorRangeMode(tvin_color_range_t range_mode)
{
    LOGD("%s: mode = %d\n", __FUNCTION__, range_mode);
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_SET_COLOR_RANGE, &range_mode );
    if ( rt < 0 ) {
        LOGW ( "Vdin Set ColorRange Mode error(%s)!\n", strerror(errno ));
    }

    return rt;
}

int CTvin::VDIN_GetColorRangeMode(void)
{
    int range_mode = TVIN_COLOR_RANGE_AUTO;
    int rt = VDIN_DeviceIOCtl ( TVIN_IOC_GET_COLOR_RANGE, &range_mode );
    if ( rt < 0 ) {
        LOGW ( "Vdin Get ColorRange Mode error(%s)!\n", strerror(errno ));
    }
    LOGD("%s: mode = %d\n", __FUNCTION__, range_mode);

    return range_mode;
}

int CTvin::VDIN_UpdateForPQMode(pq_status_update_e gameStatus, pq_status_update_e pcStatus)
{
    int ret = 0;
    tvin_info_t signal_info;
    memset(&signal_info, 0, sizeof(tvin_info_t));
    LOGD("%s: gameStatus= %d, pcStatus= %d\n", __FUNCTION__, gameStatus, pcStatus);
    if (gameStatus != MODE_STABLE) {
        ret = VDIN_SetGameMode(gameStatus);
        if (ret < 0)
            return -1;
    }

    if ((gameStatus != MODE_STABLE) || (pcStatus != MODE_STABLE)) {
        if (mDecoderStarted) {
            ret = VDIN_StopDec();
            if (ret >= 0) {
                LOGD("%s: stopDecoder success!\n", __FUNCTION__ );
                mDecoderStarted = false;
                VDIN_GetSignalInfo(&signal_info);
                if (signal_info.status != TVIN_SIG_STATUS_STABLE) {
                    usleep(100000);//100ms
                    VDIN_GetSignalInfo(&signal_info);
                }

                m_tvin_param.info = signal_info;
                if ( VDIN_StartDec ( &m_tvin_param ) >= 0 ) {
                    mDecoderStarted = true;
                    ret = 0;
                } else {
                    LOGE("%s: StartDecoder failed!\n", __FUNCTION__ );
                    ret = -1;
                }
            } else {
                LOGE("%s: StopDecoder failed!\n", __FUNCTION__);
                ret = -1;
            }
        }
    }else {
        LOGD("no need update!\n");
        ret = 0;
    }

    return ret;
}

int CTvin::VDIN_SetWssStatus ( int status )
{
    LOGD("%s: status = %d\n", __FUNCTION__, status);
    int ret = VDIN_DeviceIOCtl ( TVIN_IOC_SET_AUTO_RATIO_EN, &status );
    if ( ret < 0 ) {
        LOGW ( "Vdin Set WSS status error(%s)!\n", strerror(errno ));
    }

    return ret;
}

int CTvin::VDIN_GetDisplayModeSupportList(std::vector<std::string> *SupportDisplayModes)
{
    int ret = -1;
    char buf[SYS_STR_LEN] = {0};
    char TempBuf[32] = {0};

    if (access(FRAME_RATE_SUPPORT_LIST_PATH, 0) != 0) {
        LOGD("%s: don't support TX output!\n", __FUNCTION__);
        ret = 1;
    } else {
        ret = tvReadSysfs(FRAME_RATE_SUPPORT_LIST_PATH, buf);
        if (ret < 0) {
            LOGE("%s failed!\n", __FUNCTION__);
            ret = -1;
        } else {
            char *ptr = strtok(buf, "hz");
            while (ptr != NULL) {
                std::string TempStr = std::string(ptr);
                if (TempStr[0] == '*') {
                    TempStr = TempStr.substr(1);
                }
                memset(TempBuf, 0, sizeof(TempBuf));
                sprintf(TempBuf, "%shz", TempStr.c_str());
                std::string str = std::string(TempBuf);
                (*SupportDisplayModes).push_back(str);
                ptr = strtok(NULL, "hz");
            }
            ret = 0;
        }
    }

    return ret;
}

int CTvin::VDIN_GetBestDisplayMode(best_displaymode_condition_t condition,
                                            std::string ConditionParam,
                                            std::vector<std::string> SupportDisplayModes,
                                            char *BestDisplayMode)
{
    std::string TempStr;
    std::vector<std::string> TempVector;
    std::vector<std::string>::iterator i;
    int BestResolution = 0, BestFrameRate = 0;
    char buf[32] = {0};
    for (i = SupportDisplayModes.begin(); i != SupportDisplayModes.end(); i++) {
        if (strstr((*i).c_str(), ConditionParam.c_str()) != NULL) {
            //LOGD("###%s\n", (*i).c_str());
            TempVector.push_back(*i);
        }
    }

    switch (condition) {
    case BEST_DISPLAYMODE_CONDITION_FRAMERATE:
        for (i = TempVector.begin(); i != TempVector.end(); i++) {
            TempStr = *i;
            int ResolutionVal = atoi(TempStr.substr(0, TempStr.length()-5).c_str());
            if (BestResolution <= ResolutionVal) {
                BestResolution = ResolutionVal;
            }
        }
        sprintf(buf, "%dp%shz", BestResolution, ConditionParam.c_str());
        break;
    case BEST_DISPLAYMODE_CONDITION_RESOLUTION:
        for (i = TempVector.begin(); i != TempVector.end(); i++) {
            TempStr = *i;
            int FrameRateVal = atoi(TempStr.substr(TempStr.length()-4,2).c_str());
            if (BestFrameRate <= FrameRateVal) {
                BestFrameRate = FrameRateVal;
            }
        }
        sprintf(buf, "%s%dhz", ConditionParam.c_str(), BestFrameRate);
        break;
    default:
        sprintf(buf, "1080p60hz");
        break;
    }

    strcpy(BestDisplayMode, buf);
    LOGD("%s: BestDisplayMode is %s\n", __FUNCTION__, BestDisplayMode);

    return 0;
}

int CTvin::VDIN_GetFrameRateSupportList(std::vector<std::string> *supportFrameRates)
{
    int ret = -1;
    char buf[SYS_STR_LEN] = {0};

    ret = tvReadSysfs(FRAME_RATE_SUPPORT_LIST_PATH, buf);
    if (ret < 0) {
        LOGE("%s failed!\n", __FUNCTION__);
        return -1;
    } else {
        char *ptr = strtok(buf, "\n");
        while (ptr != NULL) {
            int len = strlen(ptr);
            if (ptr[len - 1] == '*') {
                ptr[len - 1] = '\0';
            }
            char *ptr1 = strtok(buf, "hz");
            while (ptr1 != NULL) {
                std::string str = std::string(ptr1);
                std::string str1 = str.substr(str.length()-2, 2);
                (*supportFrameRates).push_back(str1);
                ptr1 = strtok(NULL, "hz");
            }
            ptr = strtok(NULL, "\n");
        }
        return 0;
    }
}

// AFE
int CTvin::AFE_OpenModule ( void )
{
    int fd = open ( AFE_DEV_PATH, O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "Open tvafe module, error(%s).\n", strerror ( errno ) );
        return -1;
    }

    LOGD ( "Open %s module fd = [%d]", AFE_DEV_PATH, fd );
    return fd;
}

void CTvin::AFE_CloseModule ( void )
{
    if ( mAfeDevFd >= 0 ) {
        close ( mAfeDevFd );
        mAfeDevFd = -1;
    }
}

int CTvin::AFE_DeviceIOCtl ( int request, ... )
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;

    if (mAfeDevFd < 0) {
        mAfeDevFd = AFE_OpenModule();
    }

    if ( mAfeDevFd >= 0 ) {
        va_start ( ap, request );
        arg = va_arg ( ap, void * );
        va_end ( ap );

        tmp_ret = ioctl ( mAfeDevFd, request, arg );

        AFE_CloseModule();

        return tmp_ret;
    }

    return -1;
}

int CTvin::AFE_SetVGAEdid ( const unsigned char *ediddata )
{
    int rt = -1;
    struct tvafe_vga_edid_s vgaEdid;

#ifdef NO_IC_TEST

    for ( int i = 0; i < 256; i++ ) {
        test_edid[i] = ediddata[i];
    }

#endif

    for ( int i = 0; i < 256; i++ ) {
        vgaEdid.value[i] = ediddata[i];
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_EDID, &vgaEdid );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetVGAEdid, error(%s).!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_GetVGAEdid ( unsigned char *ediddata )
{
    int rt = -1;
    struct tvafe_vga_edid_s vgaEdid;

#ifdef NO_IC_TEST

    for ( int i = 0; i < 256; i++ ) {
        ediddata[i] = test_edid[i];
    }

    LOGD ( "AFE_GetVGAEdid:\n" );
    LOGD ( "===================================================\n" );

    for ( int i = 0; i < 256; i++ ) {
        LOGD ( "vag edid[%d] = [0x%x].\n", i, ediddata[i] );
    }

    LOGD ( "===================================================\n" );
#endif

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_VGA_EDID, &vgaEdid );

    for ( int i = 0; i < 256; i++ ) {
        ediddata[i] = vgaEdid.value[i];
    }

    if ( rt < 0 ) {
        LOGW ( "AFE_GetVGAEdid, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_SetADCTimingAdjust ( const struct tvafe_vga_parm_s *timingadj )
{
    int rt = -1;

    if ( timingadj == NULL ) {
        return rt;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_PARM, timingadj );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetADCTimingAdjust, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_GetADCCurrentTimingAdjust ( struct tvafe_vga_parm_s *timingadj )
{
    int rt = -1;

    if ( timingadj == NULL ) {
        return rt;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_VGA_PARM, timingadj );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetADCCurrentTimingAdjust, error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    return 0;
}

int CTvin::AFE_VGAAutoAdjust ( struct tvafe_vga_parm_s *timingadj )
{
    enum tvafe_cmd_status_e CMDStatus = TVAFE_CMD_STATUS_PROCESSING;
    struct tvin_parm_s tvin_para;
    int rt = -1, i = 0;

    if ( timingadj == NULL ) {
        return -1;
    }

    for ( i = 0, CMDStatus = TVAFE_CMD_STATUS_PROCESSING; i < 50; i++ ) {
        rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, &CMDStatus );

        if ( rt < 0 ) {
            LOGD ( "get afe CMD status, error(%s),  return(%d).\n", strerror ( errno ),  rt);
        }

        if ( ( CMDStatus == TVAFE_CMD_STATUS_IDLE ) || ( CMDStatus == TVAFE_CMD_STATUS_SUCCESSFUL ) ) {
            break;
        }

        usleep ( 10 * 1000 );
    }

    if ( ( CMDStatus == TVAFE_CMD_STATUS_PROCESSING ) || ( CMDStatus == TVAFE_CMD_STATUS_FAILED ) ) {
        return -1;
    }

    for ( i = 0; i < 100; i++ ) {
        rt = VDIN_DeviceIOCtl ( TVIN_IOC_G_PARM, &tvin_para );

        if ( tvin_para.info.status == TVIN_SIG_STATUS_STABLE ) {
            break;
        }

        usleep ( 10 * 1000 );
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_AUTO );

    if ( rt < 0 ) {
        timingadj->clk_step = 0;
        timingadj->phase = 0;
        timingadj->hpos_step = 0;
        timingadj->vpos_step = 0;
        AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_PARM, timingadj );
        return rt;
    } else {
        ;//AFE_DeviceIOCtl(TVIN_IOC_G_AFE_VGA_PARM, timingadj);
    }

    for ( i = 0; i < 10; i++ ) {
        sleep ( 1 );

        rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, &CMDStatus );

        if ( rt < 0 ) {
            return rt;
        } else {
            if ( CMDStatus == TVAFE_CMD_STATUS_SUCCESSFUL ) {
                usleep ( 100 * 1000 );
                AFE_GetADCCurrentTimingAdjust ( timingadj );
                LOGD ( "AFE_VGAAutoAdjust, successfull!\n" );
                return 0;
            }
        }
    }

    return -1;
}

int CTvin::AFE_SetVGAAutoAjust ( void )
{
    int rt = -1;
    tvafe_vga_parm_t timingadj;
    tvafe_cmd_status_t Status;
    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, &Status );

    if ( ( Status == TVAFE_CMD_STATUS_IDLE ) || ( Status == TVAFE_CMD_STATUS_SUCCESSFUL ) ) {
        ;
    } else {
        LOGW ( "AFE_SetVGAAutoAjust, TVIN_IOC_G_AFE_CMD_STATUS failed!\n" );
        return -1;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_AUTO );

    if ( rt < 0 ) {
        timingadj.clk_step = 0;
        timingadj.phase = 0;
        timingadj.hpos_step = 0;
        timingadj.vpos_step = 0;
        AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_PARM, &timingadj );
        return rt;
    }

    return 0;
}

int CTvin::AFE_GetVGAAutoAdjustCMDStatus ( tvafe_cmd_status_t *Status )
{
    int rt = -1;

    if ( Status == NULL ) {
        return rt;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, Status );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetVGAAutoAdjustStatus, get status, error(%s) return(%d)\n", strerror ( errno ), rt );
        return rt;
    }

    return 0;
}

int CTvin::AFE_GetAdcCal ( struct tvafe_adc_cal_s *adccalvalue )
{
    int rt = -1;

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_ADC_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetADCGainOffset, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_SetAdcCal ( struct tvafe_adc_cal_s *adccalvalue )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_ADC_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetAdcCal, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_GetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue )
{
    int rt = -1;

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_ADC_COMP_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetYPbPrADCGainOffset, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_SetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_ADC_COMP_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetYPbPrADCGainOffset, error(%s)!", strerror ( errno ) );
    }

    return rt;
}
int CTvin::AFE_GetYPbPrWSSinfo ( struct tvafe_comp_wss_s *wssinfo )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_COMP_WSS, wssinfo );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetYPbPrWSSinfo, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

#define RGB444      3
#define YCBCR422    2
#define YCBCR444    3
#define Y422_POS    0
#define CB422_POS   1
#define CR422_POS   3
#define Y444_POS    0
#define CB444_POS   1
#define CR444_POS   2
#define R444_POS    0
#define G444_POS    1
#define B444_POS    2

//=========== VGA =====================
#define VGA_BUF_WID         (VGA_H_ACTIVE)

#ifdef PATTERN_7_COLOR_BAR
#define VGA_BAR_WID     (VGA_H_ACTIVE/7)
#define VGA_H_CUT_WID   (10)
#else
#define VGA_BAR_WID     (VGA_H_ACTIVE/8)
#define VGA_H_CUT_WID   (10)
#endif

#define VGA_V_CUT_WID       (40)
#define VGA_V_CAL_WID       (200+VGA_V_CUT_WID)

#define VGA_WHITE_HS        (VGA_BAR_WID*0+VGA_H_CUT_WID)
#define VGA_WHITE_HE        (VGA_BAR_WID*1-VGA_H_CUT_WID-1)
#define VGA_WHITE_VS        (VGA_V_CUT_WID)
#define VGA_WHITE_VE        (VGA_V_CAL_WID-1)
#define VGA_WHITE_SIZE      ((VGA_WHITE_HE-VGA_WHITE_HS+1)*(VGA_WHITE_VE-VGA_WHITE_VS+1))
#ifdef PATTERN_7_COLOR_BAR
#define VGA_BLACK_HS    (VGA_BAR_WID*6+VGA_H_CUT_WID)
#define VGA_BLACK_HE    (VGA_BAR_WID*7-VGA_H_CUT_WID-1)
#define VGA_BLACK_VS        (768-140)
#define VGA_BLACK_VE    (768-40-1)
#define VGA_BLACK_SIZE  ((VGA_BLACK_HE-VGA_BLACK_HS+1)*(VGA_BLACK_VE-VGA_BLACK_VS+1))
#else
#define VGA_BLACK_HS    (VGA_BAR_WID*7+VGA_H_CUT_WID)
#define VGA_BLACK_HE    (VGA_BAR_WID*8-VGA_H_CUT_WID-1)
#define VGA_BLACK_VS    (VGA_V_CUT_WID)
#define VGA_BLACK_VE    (VGA_V_CAL_WID-1)
#define VGA_BLACK_SIZE  ((VGA_BLACK_HE-VGA_BLACK_HS+1)*(VGA_BLACK_VE-VGA_BLACK_VS+1))
#endif

//=========== YPBPR =====================
#define COMP_BUF_WID        (COMP_H_ACTIVE)

#define COMP_BAR_WID        (COMP_H_ACTIVE/8)
#define COMP_H_CUT_WID      (20)
#define COMP_V_CUT_WID      (100)
#define COMP_V_CAL_WID      (200+COMP_V_CUT_WID)

#define COMP_WHITE_HS       (COMP_BAR_WID*0+COMP_H_CUT_WID)
#define COMP_WHITE_HE       (COMP_BAR_WID*1-COMP_H_CUT_WID-1)
#define COMP_WHITE_VS       (COMP_V_CUT_WID)
#define COMP_WHITE_VE       (COMP_V_CAL_WID-1)
#define COMP_WHITE_SIZE     ((COMP_WHITE_HE-COMP_WHITE_HS+1)*(COMP_WHITE_VE-COMP_WHITE_VS+1))
#define CB_WHITE_SIZE       ((COMP_WHITE_HE-COMP_WHITE_HS+1)*(COMP_WHITE_VE-COMP_WHITE_VS+1)/2)
#define CR_WHITE_SIZE       ((COMP_WHITE_HE-COMP_WHITE_HS+1)*(COMP_WHITE_VE-COMP_WHITE_VS+1)/2)

#define COMP_YELLOW_HS      (COMP_BAR_WID*1+COMP_H_CUT_WID)
#define COMP_YELLOW_HE      (COMP_BAR_WID*2-COMP_H_CUT_WID-1)
#define COMP_YELLOW_VS      (COMP_V_CUT_WID)
#define COMP_YELLOW_VE      (COMP_V_CAL_WID-1)
#define COMP_YELLOW_SIZE    ((COMP_YELLOW_HE-COMP_YELLOW_HS+1)*(COMP_YELLOW_VE-COMP_YELLOW_VS+1)/2)

#define COMP_CYAN_HS        (COMP_BAR_WID*2+COMP_H_CUT_WID)
#define COMP_CYAN_HE        (COMP_BAR_WID*3-COMP_H_CUT_WID-1)
#define COMP_CYAN_VS        (COMP_V_CUT_WID)
#define COMP_CYAN_VE        (COMP_V_CAL_WID-1)
#define COMP_CYAN_SIZE      ((COMP_CYAN_HE-COMP_CYAN_HS+1)*(COMP_CYAN_VE-COMP_CYAN_VS+1)/2)

#define COMP_RED_HS         (COMP_BAR_WID*5+COMP_H_CUT_WID)
#define COMP_RED_HE         (COMP_BAR_WID*6-COMP_H_CUT_WID-1)
#define COMP_RED_VS         (COMP_V_CUT_WID)
#define COMP_RED_VE         (COMP_V_CAL_WID-1)
#define COMP_RED_SIZE       ((COMP_RED_HE-COMP_RED_HS+1)*(COMP_RED_VE-COMP_RED_VS+1)/2)

#define COMP_BLUE_HS        (COMP_BAR_WID*6+COMP_H_CUT_WID)
#define COMP_BLUE_HE        (COMP_BAR_WID*7-COMP_H_CUT_WID-1)
#define COMP_BLUE_VS        (COMP_V_CUT_WID)
#define COMP_BLUE_VE        (COMP_V_CAL_WID-1)
#define COMP_BLUE_SIZE      ((COMP_BLUE_HE-COMP_BLUE_HS+1)*(COMP_BLUE_VE-COMP_BLUE_VS+1)/2)

#define COMP_BLACK_HS       (COMP_BAR_WID*7+COMP_H_CUT_WID)
#define COMP_BLACK_HE       (COMP_BAR_WID*8-COMP_H_CUT_WID-1)
#define COMP_BLACK_VS       (COMP_V_CUT_WID)
#define COMP_BLACK_VE       (COMP_V_CAL_WID-1)
#define COMP_BLACK_SIZE     ((COMP_BLACK_HE-COMP_BLACK_HS+1)*(COMP_BLACK_VE-COMP_BLACK_VS+1))
#define CB_BLACK_SIZE       ((COMP_BLACK_HE-COMP_BLACK_HS+1)*(COMP_BLACK_VE-COMP_BLACK_VS+1)/2)
#define CR_BLACK_SIZE       ((COMP_BLACK_HE-COMP_BLACK_HS+1)*(COMP_BLACK_VE-COMP_BLACK_VS+1)/2)

//=========== CVBS =====================
#define CVBS_BUF_WID        (CVBS_H_ACTIVE)
#define CVBS_BAR_WID        (CVBS_H_ACTIVE/8)
#define CVBS_H_CUT_WID      (20)

#define CVBS_V_CUT_WID      (40)
#define CVBS_V_CAL_WID      (140+CVBS_V_CUT_WID)

#define CVBS_WHITE_HS       (CVBS_BAR_WID*0+CVBS_H_CUT_WID)
#define CVBS_WHITE_HE       (CVBS_BAR_WID*1-CVBS_H_CUT_WID-1)
#define CVBS_WHITE_VS       (CVBS_V_CUT_WID)
#define CVBS_WHITE_VE       (CVBS_V_CAL_WID-1)
#define CVBS_WHITE_SIZE     ((CVBS_WHITE_HE-CVBS_WHITE_HS+1)*(CVBS_WHITE_VE-CVBS_WHITE_VS+1))

#define CVBS_BLACK_HS       (CVBS_BAR_WID*7+CVBS_H_CUT_WID)
#define CVBS_BLACK_HE       (CVBS_BAR_WID*8-CVBS_H_CUT_WID-1)
#define CVBS_BLACK_VS       (CVBS_V_CUT_WID)
#define CVBS_BLACK_VE       (CVBS_V_CAL_WID-1)
#define CVBS_BLACK_SIZE     ((CVBS_BLACK_HE-CVBS_BLACK_HS+1)*(CVBS_BLACK_VE-CVBS_BLACK_VS+1))

#define COMP_CAP_SIZE       (COMP_H_ACTIVE*COMP_V_ACTIVE*YCBCR422)
#ifdef VGA_SOURCE_RGB444
#define VGA_CAP_SIZE    (VGA_H_ACTIVE*VGA_V_ACTIVE*RGB444)
#else
#define VGA_CAP_SIZE    (VGA_H_ACTIVE*VGA_V_ACTIVE*YCBCR444)
#endif
#define CVBS_CAP_SIZE       (CVBS_H_ACTIVE*CVBS_V_ACTIVE)

#define PRE_0       -16
#define PRE_1       -128
#define PRE_2       -128
#define COEF_00     1.164
#define COEF_01     0
#define COEF_02     1.793
#define COEF_10     1.164
#define COEF_11     -0.213
#define COEF_12     -0.534
#define COEF_20     1.164
#define COEF_21     2.115
#define COEF_22     0
#define POST_0      0
#define POST_1      0
#define POST_2      0

unsigned int CTvin::data_limit ( float data )
{
    if ( data < 0 ) {
        return ( 0 );
    } else if ( data > 255 ) {
        return ( 255 );
    } else {
        return ( ( unsigned int ) data );
    }
}

void CTvin::matrix_convert_yuv709_to_rgb ( unsigned int y, unsigned int u, unsigned int v, unsigned int *r, unsigned int *g, unsigned int *b )
{
    *r = data_limit ( ( ( float ) y + PRE_0 ) * COEF_00 + ( ( float ) u + PRE_1 ) * COEF_01 + ( ( float ) v + PRE_2 ) * COEF_02 + POST_0 + 0.5 );
    *g = data_limit ( ( ( float ) y + PRE_0 ) * COEF_10 + ( ( float ) u + PRE_1 ) * COEF_11 + ( ( float ) v + PRE_2 ) * COEF_12 + POST_1 + 0.5 );
    *b = data_limit ( ( ( float ) y + PRE_0 ) * COEF_20 + ( ( float ) u + PRE_1 ) * COEF_21 + ( ( float ) v + PRE_2 ) * COEF_22 + POST_2 + 0.5 );
}

void CTvin::re_order ( unsigned int *a, unsigned int *b )
{
    unsigned int c = 0;

    if ( *a > *b ) {
        c = *a;
        *a = *b;
        *b = c;
    }
}

char *CTvin::get_cap_addr ( enum adc_cal_type_e calType )
{
    int n;
    char *dp;

    for ( n = 0; n < 0x00ff; n++ ) {
        if ( VDIN_DeviceIOCtl ( TVIN_IOC_G_SIG_INFO, &gTvinAFESignalInfo ) < 0 ) {
            LOGW ( "get_cap_addr, get signal info, error(%s),fd(%d).\n", strerror ( errno ), mVdin0DevFd );
            return NULL;
        } else {
            if ( gTvinAFESignalInfo.status == TVIN_SIG_STATUS_STABLE ) {
                gTvinAFEParam.info.fmt = gTvinAFESignalInfo.fmt;
                break;
            }
        }
    }

    if ( gTvinAFESignalInfo.status != TVIN_SIG_STATUS_STABLE ) {
        LOGD ( "get_cap_addr, signal isn't stable, out of calibration!\n" );
        return NULL;
    } else {
        if ( VDIN_DeviceIOCtl ( TVIN_IOC_STOP_DEC ) < 0 ) {
            LOGW ( "get_cap_addr, stop vdin, error (%s).\n", strerror ( errno ) );
            return NULL;
        }

        usleep ( 1000 );

        if ( calType == CAL_YPBPR ) {
            dp = ( char * ) mmap ( NULL, COMP_CAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mVdin0DevFd, 0 );
        } else {
            dp = ( char * ) mmap ( NULL, VGA_CAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mVdin0DevFd, 0 );
        }

        if ( dp == NULL ) {
            LOGE ( "get_cap_addr, mmap failed!\n" );
        }

        return dp;
    }

    return NULL;
}

inline unsigned char CTvin::get_mem_data ( char *dp, unsigned int addr )
{
    return ( * ( dp + ( addr ^ 7 ) ) );
}

int CTvin::get_frame_average ( enum adc_cal_type_e calType, struct adc_cal_s *mem_data )
{
    unsigned int y = 0, cb = 0, cr = 0;
    unsigned int r = 0, g = 0, b = 0;
    unsigned int i = 0, j = 0;
    char *dp = get_cap_addr ( calType );

    if ( calType == CAL_YPBPR ) {
        for ( j = COMP_WHITE_VS; j <= COMP_WHITE_VE; j++ ) {
            for ( i = COMP_WHITE_HS; i <= COMP_WHITE_HE; i++ ) {
                mem_data->g_y_max += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_max /= COMP_WHITE_SIZE;

        for ( j = COMP_WHITE_VS; j <= COMP_WHITE_VE; j++ ) {
            for ( i = COMP_WHITE_HS; i <= COMP_WHITE_HE; ) {
                mem_data->cb_white += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS ) );
                mem_data->cr_white += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->cb_white /= CB_WHITE_SIZE;
        mem_data->cr_white /= CR_WHITE_SIZE;

        for ( j = COMP_RED_VS; j <= COMP_RED_VE; j++ ) {
            for ( i = COMP_RED_HS; i <= COMP_RED_HE; ) {
                mem_data->rcr_max += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->rcr_max /= COMP_RED_SIZE;

        for ( j = COMP_BLUE_VS; j <= COMP_BLUE_VE; j++ ) {
            for ( i = COMP_BLUE_HS; i <= COMP_BLUE_HE; ) {
                mem_data->bcb_max += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS ) );
                i = i + 2;
            }
        }

        mem_data->bcb_max /= COMP_BLUE_SIZE;

        for ( j = COMP_BLACK_VS; j <= COMP_BLACK_VE; j++ ) {
            for ( i = COMP_BLACK_HS; i <= COMP_BLACK_HE; i++ ) {
                mem_data->g_y_min += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_min /= COMP_BLACK_SIZE;

        for ( j = COMP_BLACK_VS; j <= COMP_BLACK_VE; j++ ) {
            for ( i = COMP_BLACK_HS; i <= COMP_BLACK_HE; ) {
                mem_data->cb_black += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS ) );
                mem_data->cr_black += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->cb_black /= CB_BLACK_SIZE;
        mem_data->cr_black /= CR_BLACK_SIZE;

        /*
         for (j=COMP_BLACK_VS; j<=COMP_BLACK_VE; j++) {
         for (i=COMP_BLACK_HS; i<=COMP_BLACK_HE;) {
         //mem_data->cb_black += get_mem_data(dp, ((COMP_BUF_WID*j+i)*YCBCR422+CB422_POS));
         mem_data->cr_black += get_mem_data(dp, ((COMP_BUF_WID*j+i)*YCBCR422+CR422_POS));
         i = i+2;
         }
         }
         mem_data->cr_black /= CR_BLACK_SIZE;
         */
        for ( j = COMP_CYAN_VS; j <= COMP_CYAN_VE; j++ ) {
            for ( i = COMP_CYAN_HS; i <= COMP_CYAN_HE; ) {
                mem_data->rcr_min += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->rcr_min /= COMP_CYAN_SIZE;

        for ( j = COMP_YELLOW_VS; j <= COMP_YELLOW_VE; j++ ) {
            for ( i = COMP_YELLOW_HS; i <= COMP_YELLOW_HE; ) {
                mem_data->bcb_min += get_mem_data ( dp, ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS );
                i = i + 2;
            }
        }

        mem_data->bcb_min /= COMP_YELLOW_SIZE;

    } else if ( calType == CAL_VGA ) {
        for ( j = VGA_WHITE_VS; j <= VGA_WHITE_VE; j++ ) {
            for ( i = VGA_WHITE_HS; i <= VGA_WHITE_HE; i++ ) {
#ifdef VGA_SOURCE_RGB444
                r = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + R444_POS ) );
                g = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + G444_POS ) );
                b = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + B444_POS ) );
#else
                y = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + Y444_POS ) );
                cb = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CB444_POS ) );
                cr = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CR444_POS ) );
                matrix_convert_yuv709_to_rgb ( y, cb, cr, &r, &g, &b );
#endif
                mem_data->rcr_max = mem_data->rcr_max + r;
                mem_data->g_y_max = mem_data->g_y_max + g;
                mem_data->bcb_max = mem_data->bcb_max + b;
            }
        }

        mem_data->rcr_max = mem_data->rcr_max / VGA_WHITE_SIZE;
        mem_data->g_y_max = mem_data->g_y_max / VGA_WHITE_SIZE;
        mem_data->bcb_max = mem_data->bcb_max / VGA_WHITE_SIZE;

        for ( j = VGA_BLACK_VS; j <= VGA_BLACK_VE; j++ ) {
            for ( i = VGA_BLACK_HS; i <= VGA_BLACK_HE; i++ ) {
#ifdef VGA_SOURCE_RGB444
                r = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + R444_POS ) );
                g = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + G444_POS ) );
                b = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + B444_POS ) );
#else
                y = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + Y444_POS ) );
                cb = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CB444_POS ) );
                cr = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CR444_POS ) );
                matrix_convert_yuv709_to_rgb ( y, cb, cr, &r, &g, &b );
#endif
                mem_data->rcr_min = mem_data->rcr_min + r;
                mem_data->g_y_min = mem_data->g_y_min + g;
                mem_data->bcb_min = mem_data->bcb_min + b;
            }
        }

        mem_data->rcr_min = mem_data->rcr_min / VGA_BLACK_SIZE;
        mem_data->g_y_min = mem_data->g_y_min / VGA_BLACK_SIZE;
        mem_data->bcb_min = mem_data->bcb_min / VGA_BLACK_SIZE;

    } else { //CVBS
        for ( j = CVBS_WHITE_VS; j <= CVBS_WHITE_VE; j++ ) {
            for ( i = CVBS_WHITE_HS; i <= CVBS_WHITE_HE; i++ ) {
                mem_data->g_y_max += mem_data->g_y_max + get_mem_data ( dp, ( ( CVBS_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_max /= COMP_WHITE_SIZE;

        for ( j = CVBS_BLACK_VS; j <= CVBS_BLACK_VE; j++ ) {
            for ( i = CVBS_BLACK_HS; i <= CVBS_BLACK_HE; i++ ) {
                mem_data->g_y_min += mem_data->g_y_min + get_mem_data ( dp, ( ( CVBS_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_min /= CVBS_BLACK_SIZE;
    }

    if ( calType == CAL_YPBPR ) {
        munmap ( dp, COMP_CAP_SIZE );
    } else if ( calType == CAL_VGA ) {
        munmap ( dp, VGA_CAP_SIZE );
    } else {
        munmap ( dp, CVBS_CAP_SIZE );
    }

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_START_DEC, &gTvinAFEParam ) < 0 ) {
        LOGW ( "get_frame_average, get vdin signal info, error(%s),fd(%d).\n", strerror ( errno ), mVdin0DevFd );
        return 0;
    }

    return 0;
}

#define ADC_CAL_FRAME_QTY_ORDER 2 //NOTE:  MUST >=2!!
struct adc_cal_s CTvin::get_n_frame_average ( enum adc_cal_type_e calType )
{
    struct adc_cal_s mem_data = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int rcrmax[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int rcrmin[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int g_ymax[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int g_ymin[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int bcbmax[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int bcbmin[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int cbwhite[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int crwhite[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int cbblack[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int crblack[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int i = 0, j = 0;

    for ( i = 0; i < ( 1 << ADC_CAL_FRAME_QTY_ORDER ); i++ ) {
        get_frame_average ( calType, &mem_data );
        rcrmax[i] = mem_data.rcr_max;
        rcrmin[i] = mem_data.rcr_min;
        g_ymax[i] = mem_data.g_y_max;
        g_ymin[i] = mem_data.g_y_min;
        bcbmax[i] = mem_data.bcb_max;
        bcbmin[i] = mem_data.bcb_min;
        cbwhite[i] = mem_data.cb_white;
        crwhite[i] = mem_data.cr_white;
        cbblack[i] = mem_data.cb_black;
        crblack[i] = mem_data.cr_black;
    }

    for ( i = 0; i < ( 1 << ADC_CAL_FRAME_QTY_ORDER ) - 1; i++ ) {
        for ( j = 1; j < ( 1 << ADC_CAL_FRAME_QTY_ORDER ); j++ ) {
            re_order ( & ( rcrmax[i] ), & ( rcrmax[j] ) );
            re_order ( & ( rcrmin[i] ), & ( rcrmin[j] ) );
            re_order ( & ( g_ymax[i] ), & ( g_ymax[j] ) );
            re_order ( & ( g_ymin[i] ), & ( g_ymin[j] ) );
            re_order ( & ( bcbmax[i] ), & ( bcbmax[j] ) );
            re_order ( & ( bcbmin[i] ), & ( bcbmin[j] ) );
            re_order ( & ( cbwhite[i] ), & ( cbwhite[j] ) );
            re_order ( & ( crwhite[i] ), & ( crwhite[j] ) );
            re_order ( & ( cbblack[i] ), & ( cbblack[j] ) );
            re_order ( & ( crblack[i] ), & ( crblack[j] ) );
        }
    }

    memset ( &mem_data, 0, sizeof ( mem_data ) );

    for ( i = 0; i < ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 1 ) ); i++ ) { //(1<<(ADC_CAL_FRAME_QTY_ORDER-1))
        mem_data.rcr_max += rcrmax[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.rcr_min += rcrmin[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.g_y_max += g_ymax[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.g_y_min += g_ymin[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.bcb_max += bcbmax[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.bcb_min += bcbmin[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cb_white += cbwhite[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cr_white += crwhite[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cb_black += cbblack[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cr_black += crblack[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
    }


    mem_data.rcr_max >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.rcr_min >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.g_y_max >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.g_y_min >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.bcb_max >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.bcb_min >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cb_white >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cr_white >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cb_black >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cr_black >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );

    return mem_data;
}

int CTvin::AFE_GetMemData ( int typeSel, struct adc_cal_s *mem_data )
{
    if ( mVdin0DevFd < 0 || mem_data == NULL ) {
        LOGW ( "AFE_GetMemData, didn't open vdin fd, return!\n" );
        return -1;
    }

    memset ( &gTvinAFEParam, 0, sizeof ( gTvinAFEParam ) );
    memset ( &gTvinAFESignalInfo, 0, sizeof ( gTvinAFESignalInfo ) );

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_G_PARM, &gTvinAFEParam ) < 0 ) {
        LOGW ( "AFE_GetMemData, get vdin param, error(%s), fd(%d)!\n", strerror ( errno ), mVdin0DevFd );
        return -1;
    }

    gTvinAFEParam.flag = gTvinAFEParam.flag | TVIN_PARM_FLAG_CAP;

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_S_PARM, &gTvinAFEParam ) < 0 ) {
        LOGW ( "AFE_GetMemData, set vdin param error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( typeSel == 0 ) {
        get_frame_average ( CAL_YPBPR, mem_data );
    } else if ( typeSel == 1 ) {
        get_frame_average ( CAL_VGA, mem_data );
    } else {
        *mem_data = get_n_frame_average ( CAL_CVBS );
    }

    gTvinAFEParam.flag &= 0x11111110;

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_S_PARM, &gTvinAFEParam ) < 0 ) {
        LOGW ( "AFE_GetMemData, set vdin param error(%s)\n", strerror ( errno ) );
        return -1;
    }

    LOGD ( "AFE_GetMemData, MAX ======> :\n Y(White)->%d \n Cb(Blue)->%d \n Cr(Red)->%d\n", mem_data->g_y_max, mem_data->bcb_max, mem_data->rcr_max );
    LOGD ( "AFE_GetMemData, MIN ======>:\n Y(Black)->%d \n Cb(Yellow)->%d \n Cr(Cyan)->%d\n Cb(White) ->%d\n Cb(Black)->%d\n Cr(Black)->%d\n", mem_data->g_y_min, mem_data->bcb_min, mem_data->rcr_min,
           mem_data->cb_white, mem_data->cb_black, mem_data->cr_black );
    return 0;
}

int CTvin::AFE_GetCVBSLockStatus ( enum tvafe_cvbs_video_e *cvbs_lock_status )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CVBS_LOCK, cvbs_lock_status );
    if ( rt < 0 ) {
        LOGD ( "AFE_GetCVBSLockStatus, error: return(%d), error(%s)!\n", rt, strerror ( errno ) );
    } else {
        LOGD ( "AFE_GetCVBSLockStatus, value=%d.\n", *cvbs_lock_status );
    }

    return *cvbs_lock_status;
}

int CTvin::AFE_SetCVBSStd ( tvin_sig_fmt_t fmt )
{
    LOGD ( "AFE_SetCVBSStd, sig_fmt = %d\n", fmt );
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_CVBS_STD, &fmt );
    if ( rt < 0 ) {
        LOGD ( "AFE_SetCVBSStd, error: return(%d), error(%s)!\n", rt, strerror ( errno ) );
    }

    return rt;
}

int CTvin::TvinApi_SetStartDropFrameCn ( int count )
{
    char set_str[4] = {0};
    sprintf ( set_str, "%d", count );
    return tvWriteSysfs ( "/sys/module/di/parameters/start_frame_drop_count", set_str );
}

int CTvin::TvinApi_SetVdinHVScale ( int vdinx, int hscale, int vscale )
{
    int ret = -1;
    char set_str[32]= {0};

    sprintf ( set_str, "%s %d %d", "hvscaler", hscale, vscale );
    if ( vdinx == 0 ) {
        ret = tvWriteSysfs ( VDIN0_ATTR_PATH, set_str );
    } else {
        ret = tvWriteSysfs ( VDIN1_ATTR_PATH, set_str );
    }

    return ret;
}

tvin_trans_fmt CTvin::TvinApi_Get3DDectMode()
{
    char buf[32] = {0};

    tvReadSysfs("/sys/module/di/parameters/det3d_mode", buf);
    int mode3d = atoi(buf);
    return (tvin_trans_fmt)mode3d;
}

int CTvin::TvinApi_SetCompPhaseEnable ( int enable )
{
    int ret = -1;
    if ( enable == 1 ) {
        ret = tvWriteSysfs ( "/sys/module/tvin_afe/parameters/enable_dphase", "Y" );
        LOGD ( "%s, enable TvinApi_SetCompPhase.", CFG_SECTION_TV );
    }

    return ret;
}

int CTvin::VDIN_GetPortConnect ( int port )
{
    int status = 0;

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_CALLMASTER_SET, &port ) < 0 ) {
        LOGW ( "TVIN_IOC_CALLMASTER_SET error(%s) port %d\n", strerror ( errno ), port );
        return 0;
    }

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_CALLMASTER_GET, &status ) < 0 ) {
        LOGW ( "TVIN_IOC_CALLMASTER_GET error(%s)\n", strerror ( errno ) );
        return 0;
    }

    //LOGD("%s, port:%x,status:%d", CFG_SECTION_TV,port,status);
    return status;
}

int CTvin::VDIN_OpenHDMIPinMuxOn ( bool flag )
{
    char buf[32] = {0};
    if ( flag ) {
        sprintf ( buf, "%s", "pinmux_on" );
    } else {
        sprintf ( buf, "%s", "pinmux_off" );
    }

    tvWriteSysfs("/sys/class/hdmirx/hdmirx0/debug", buf);
    return 1;
}

int CTvin::TvinApi_GetHDMIAudioStatus ( void )
{
    char buf[32] = {0};

    tvReadSysfs("/sys/module/tvin_hdmirx/parameters/auds_rcv_sts", buf);
    int val = atoi(buf);
    return val;
}

tv_source_input_type_t CTvin::Tvin_SourceInputToSourceInputType ( tv_source_input_t source_input )
{
    tv_source_input_type_t ret = SOURCE_TYPE_MPEG;
    switch (source_input) {
        case SOURCE_TV:
            ret = SOURCE_TYPE_TV;
            break;
        case SOURCE_AV1:
        case SOURCE_AV2:
            ret = SOURCE_TYPE_AV;
            break;
        case SOURCE_YPBPR1:
        case SOURCE_YPBPR2:
            ret = SOURCE_TYPE_COMPONENT;
            break;
        case SOURCE_VGA:
            ret = SOURCE_TYPE_VGA;
            break;
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        case SOURCE_HDMI4:
            ret = SOURCE_TYPE_HDMI;
            break;
        case SOURCE_DTV:
            ret = SOURCE_TYPE_DTV;
            break;
        case SOURCE_IPTV:
            ret = SOURCE_TYPE_IPTV;
            break;
        case SOURCE_SPDIF:
            ret = SOURCE_TYPE_SPDIF;
            break;
        default:
            ret = SOURCE_TYPE_MPEG;
            break;
    }

    return ret;
}

tv_source_input_type_t CTvin::Tvin_SourcePortToSourceInputType ( tvin_port_t source_port )
{
    tv_source_input_t source_input = Tvin_PortToSourceInput(source_port);
    return Tvin_SourceInputToSourceInputType(source_input);
}

tvin_port_t CTvin::Tvin_GetSourcePortBySourceType ( tv_source_input_type_t source_type )
{
    tv_source_input_t source_input;

    switch (source_type) {
        case SOURCE_TYPE_TV:
            source_input = SOURCE_TV;
            break;
        case SOURCE_TYPE_AV:
            source_input = SOURCE_AV1;
            break;
        case SOURCE_TYPE_COMPONENT:
            source_input = SOURCE_YPBPR1;
            break;
        case SOURCE_TYPE_VGA:
            source_input = SOURCE_VGA;
            break;
        case SOURCE_TYPE_HDMI:
            source_input = SOURCE_HDMI1;
            break;
        case SOURCE_TYPE_IPTV:
            source_input = SOURCE_IPTV;
            break;
        case SOURCE_TYPE_DTV:
            source_input = SOURCE_DTV;
            break;
        case SOURCE_TYPE_SPDIF:
            source_input = SOURCE_SPDIF;
            break;
        default:
            source_input = SOURCE_MPEG;
            break;
    }

    return Tvin_GetSourcePortBySourceInput(source_input);
}

tvin_port_t CTvin::Tvin_GetSourcePortBySourceInput ( tv_source_input_t source_input )
{
    tvin_port_t source_port = TVIN_PORT_NULL;

    if ( source_input < SOURCE_TV || source_input >= SOURCE_MAX ) {
        source_port = TVIN_PORT_NULL;
    } else {
        source_port = ( tvin_port_t ) mSourceInputToPortMap[ ( int ) source_input];
    }

    return source_port;
}

tv_source_input_t CTvin::Tvin_PortToSourceInput ( tvin_port_t port )
{
    for ( int i = SOURCE_TV; i < SOURCE_MAX; i++ ) {
        if ( mSourceInputToPortMap[i] == (int)port ) {
            return (tv_source_input_t)i;
        }
    }

    return SOURCE_MAX;
}

unsigned int CTvin::Tvin_TransPortStringToValue(const char *port_str)
{
    if (strcasecmp(port_str, "TVIN_PORT_CVBS0") == 0) {
        return TVIN_PORT_CVBS0;
    } else if (strcasecmp(port_str, "TVIN_PORT_CVBS1") == 0) {
        return TVIN_PORT_CVBS1;
    } else if (strcasecmp(port_str, "TVIN_PORT_CVBS2") == 0) {
        return TVIN_PORT_CVBS2;
    } else if (strcasecmp(port_str, "TVIN_PORT_CVBS3") == 0) {
        return TVIN_PORT_CVBS3;
    } else if (strcasecmp(port_str, "TVIN_PORT_COMP0") == 0) {
        return TVIN_PORT_COMP0;
    } else if (strcasecmp(port_str, "TVIN_PORT_COMP1") == 0) {
        return TVIN_PORT_COMP1;
    } else if (strcasecmp(port_str, "TVIN_PORT_VGA0") == 0) {
        return TVIN_PORT_VGA0;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI0") == 0) {
        return TVIN_PORT_HDMI0;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI1") == 0) {
        return TVIN_PORT_HDMI1;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI2") == 0) {
        return TVIN_PORT_HDMI2;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI3") == 0) {
        return TVIN_PORT_HDMI3;
    }

    return TVIN_PORT_NULL;
}

void CTvin::Tvin_LoadSourceInputToPortMap()
{
    const char *config_value = NULL;
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_ATV, "TVIN_PORT_CVBS3");
    mSourceInputToPortMap[SOURCE_TV] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_AV1, "TVIN_PORT_CVBS1");
    mSourceInputToPortMap[SOURCE_AV1] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_AV2, "TVIN_PORT_CVBS2");
    mSourceInputToPortMap[SOURCE_AV2] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_YPBPR1, "TVIN_PORT_COMP0");
    mSourceInputToPortMap[SOURCE_YPBPR1] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_YPBPR2, "TVIN_PORT_COMP1");
    mSourceInputToPortMap[SOURCE_YPBPR2] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_HDMI1, "");
    mSourceInputToPortMap[SOURCE_HDMI1] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_HDMI2, "");
    mSourceInputToPortMap[SOURCE_HDMI2] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_HDMI3, "");
    mSourceInputToPortMap[SOURCE_HDMI3] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_HDMI4, "");
    mSourceInputToPortMap[SOURCE_HDMI4] = Tvin_TransPortStringToValue(config_value);
    config_value = config_get_str(CFG_SECTION_SRC_INPUT, CFG_TVCHANNEL_VGA, "TVIN_PORT_VGA0");
    mSourceInputToPortMap[SOURCE_VGA] = Tvin_TransPortStringToValue(config_value);
    mSourceInputToPortMap[SOURCE_MPEG] = TVIN_PORT_MPEG0;
    mSourceInputToPortMap[SOURCE_DTV] = TVIN_PORT_DTV;
    mSourceInputToPortMap[SOURCE_IPTV] = TVIN_PORT_BT656;
    mSourceInputToPortMap[SOURCE_SPDIF] = TVIN_PORT_CVBS3;
}

int CTvin::Tvin_GetSourcePortByCECPhysicalAddress(int physical_addr)
{
    if (physical_addr == 0x1000) {
        return TVIN_PORT_HDMI0;
    } else if (physical_addr == 0x2000) {
        return TVIN_PORT_HDMI1;
    } else if (physical_addr == 0x3000) {
        return TVIN_PORT_HDMI2;
    }

    return TVIN_PORT_MAX;
}

tv_audio_in_source_type_t CTvin::Tvin_GetAudioInSourceType ( tv_source_input_t source_input )
{
    const char *config_value = NULL;
    tv_audio_in_source_type_t ret = TV_AUDIO_IN_SOURCE_TYPE_LINEIN;
    switch (source_input) {
        case SOURCE_TV:
            config_value = config_get_str(CFG_SECTION_TV, CFG_TVIN_AUDIO_INSOURCE_ATV, "TV_AUDIO_IN_SOURCE_TYPE_LINEIN");
            if (strcasecmp(config_value, "TV_AUDIO_IN_SOURCE_TYPE_ATV") == 0) {
                ret = TV_AUDIO_IN_SOURCE_TYPE_ATV;
            }
            break;
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        case SOURCE_HDMI4:
            ret = TV_AUDIO_IN_SOURCE_TYPE_HDMI;
            break;
        case SOURCE_SPDIF:
            ret = TV_AUDIO_IN_SOURCE_TYPE_SPDIF;
            break;
        default:
            ret = TV_AUDIO_IN_SOURCE_TYPE_LINEIN;
            break;
    }

    return ret;
}

int CTvin::isVgaFmtInHdmi ( tvin_sig_fmt_t fmt )
{
    if ( fmt == TVIN_SIG_FMT_HDMI_640X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_800X600_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1024X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X400_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X800_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X960_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X1024_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1360X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1366X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1600X1200_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1200_00HZ ) {
        LOGD ( "%s, HDMI source : VGA format.", CFG_SECTION_TV );
        return 1;
    }

    return -1;
}

int CTvin::isSDFmtInHdmi ( tvin_sig_fmt_t fmt )
{
    if ( fmt == TVIN_SIG_FMT_HDMI_640X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X240P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X240P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X480P_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_200HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_200HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X480P_240HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_240HZ
            || fmt == TVIN_SIG_FMT_HDMI_800X600_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X400_00HZ ) {
        LOGD ( "%s, SD format.", CFG_SECTION_TV );
        return true;
    } else {
        LOGD ( "%s, HD format.", CFG_SECTION_TV );
        return false;
    }
}

bool CTvin::Tvin_is50HzFrameRateFmt ( tvin_sig_fmt_t fmt )
{
    /** component **/
    if ( fmt == TVIN_SIG_FMT_COMP_576P_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_576I_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_720P_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080P_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_A
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_B
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_C
            || fmt == TVIN_SIG_FMT_COMP_1080P_24HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080P_25HZ_D000
            /** hdmi **/
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X720P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B
            || fmt == TVIN_SIG_FMT_HDMI_1280X720P_50HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING
            /** cvbs **/
            || fmt == TVIN_SIG_FMT_CVBS_PAL_I
            || fmt == TVIN_SIG_FMT_CVBS_NTSC_50
            || fmt == TVIN_SIG_FMT_CVBS_PAL_CN
            || fmt == TVIN_SIG_FMT_CVBS_SECAM ) {
        LOGD ( "%s, Frame rate == 50Hz.", CFG_SECTION_TV );
        return true;
    } else {
        LOGD ( "%s, Frame rate != 50Hz.", CFG_SECTION_TV );
        return false;
    }
}

bool CTvin::Tvin_IsDeinterlaceFmt ( tvin_sig_fmt_t fmt )
{
    if ( fmt == TVIN_SIG_FMT_COMP_480I_59HZ_D940
            || fmt == TVIN_SIG_FMT_COMP_576I_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080I_47HZ_D952
            || fmt == TVIN_SIG_FMT_COMP_1080I_48HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_A
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_B
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_C
            || fmt == TVIN_SIG_FMT_COMP_1080I_60HZ_D000
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_240HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_200HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_2880X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576I_50HZ
            || fmt == TVIN_SIG_FMT_CVBS_NTSC_M
            || fmt == TVIN_SIG_FMT_CVBS_NTSC_443
            || fmt == TVIN_SIG_FMT_CVBS_PAL_60
            || fmt == TVIN_SIG_FMT_CVBS_PAL_CN
            || fmt == TVIN_SIG_FMT_CVBS_PAL_I
            || fmt == TVIN_SIG_FMT_CVBS_PAL_M
            || fmt == TVIN_SIG_FMT_CVBS_SECAM ) {
        LOGD ( "%s, Interlace format.", CFG_SECTION_TV );
        return true;
    } else {
        LOGD ( "%s, Progressive format.", CFG_SECTION_TV );
        return false;
    }
}

int CTvin::Tvin_StartDecoder ( tvin_info_t &info )
{
    if (mDecoderStarted)
        return -2;

    m_tvin_param.info = info;

    if ( VDIN_StartDec ( &m_tvin_param ) >= 0 ) {
        LOGD ( "StartDecoder succeed." );
        mDecoderStarted = true;
        return 0;
    }

    LOGW ( "StartDecoder failed." );
    return -1;
}

int CTvin::AFE_EnableSnowByConfig ( bool enable )
{
    LOGD("%s: enable or not: %d\n", __FUNCTION__, enable);
    unsigned int value = 0;
    if (enable) {
        value = 1;
    } else {
        value = 0;
    }

    return AFE_DeviceIOCtl(TVIN_IOC_S_AFE_SONWCFG, &value);
}

int CTvin::SwitchSnow(bool enable)
{
    int ret = -1;
    //static bool last_status = false;
    LOGD("%s: m_snow_status is %d, enable is %d\n", __FUNCTION__, m_snow_status, enable);
    if ( m_snow_status == enable ) {
        return 0;
    } else {
        m_snow_status = enable;
    }
    if ( enable ) {
        ret = AFE_DeviceIOCtl( TVIN_IOC_S_AFE_SONWON );
        ret = VDIN_DeviceIOCtl( TVIN_IOC_SNOWON );
    } else {
        ret = AFE_DeviceIOCtl( TVIN_IOC_S_AFE_SONWOFF );
        ret = VDIN_DeviceIOCtl( TVIN_IOC_SNOWOFF );
    }

    return ret;
}

bool CTvin::getSnowStatus()
{
    return m_snow_status;
}

int CTvin::SwitchPort (tvin_port_t source_port )
{
    int ret = 0;
    LOGD ("%s, source_port = %x", __FUNCTION__,  source_port);
    ret = Tvin_StopDecoder();
    if (ret < 0) {
        LOGW ( "%s,stop decoder failed.", __FUNCTION__);
        return -1;
    }

    VDIN_ClosePort();
    if (Tvin_WaitPathInactive (source_port) == -1) {
        return -1;
    }

    // Open Port
    if ( VDIN_OpenPort ( source_port ) < 0 ) {
        LOGD ( "%s, OpenPort failed, source_port =%x ", __FUNCTION__,  source_port );
    }
    m_tvin_param.port = source_port;

    return 0;
}

int CTvin::Tvin_StopDecoder()
{
    if (!mDecoderStarted)
        return 1;

    if ( VDIN_StopDec() >= 0 ) {
        LOGD ( "StopDecoder ok!" );
        mDecoderStarted = false;
        return 0;
    }
    return -1;
}

int CTvin::init_vdin ( void )
{
    VDIN_OpenModule();
    return 0;
}

int CTvin::uninit_vdin ( void )
{
    VDIN_ClosePort();
    VDIN_CloseModule();
    return 0;
}

int CTvin::Tvin_WaitPathInactive (tvin_port_t source_port)
{
    int ret = -1;
    int i = 0, dly = 20;
    mSourcePort = source_port;

    for ( i = 0; i<150; i++ ) {
        ret = Tvin_CheckPathActive (mSourcePort);
        if ( ret == TV_PATH_STATUS_INACTIVE ) {
            LOGD ( "%s, check path is inactive, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
            break;
        } else if ( ret == TV_PATH_STATUS_ACTIVE ) {
            usleep ( dly * 1000 );
        }
    }

    if ( i == 150 ) {
        LOGE ( "%s, check path active faild, %d ms gone.\n", "TV", ( dly * i ) );
        return -1;
    }
    return 0;
}

int CTvin::Tvin_RemovePath ( tv_path_type_t pathtype )
{
    int ret = -1;
    int i = 0, dly = 10;

    if (Tvin_WaitPathInactive(mSourcePort) == -1)
        return ret;
    if ( pathtype == TV_PATH_TYPE_DEFAULT ) {
        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmDefPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove default path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove default path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }
    } else if ( pathtype == TV_PATH_TYPE_TVIN ) {
        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmTvPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove tvin path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove tvin path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }

    }

    return ret;
}

int CTvin::Tvin_CheckPathActive (tvin_port_t source_port)
{
    FILE *f = NULL;
    char path[100] = {0};

    char *str_find = NULL;
    char active_str[4] = "(1)";
    char amlvideo2_str[] = "default_amlvideo2";
    int is_active = TV_PATH_STATUS_INACTIVE;
    bool flag = false;
    f = fopen ( SYS_VFM_MAP_PATH, "r" );
    if ( !f ) {
        LOGE ( "%s, can not open %s!\n", CFG_SECTION_TV, SYS_VFM_MAP_PATH );
        return TV_PATH_STATUS_NO_DEV;
    }

    while ( fgets ( path, sizeof(path)-1, f ) ) {
        char *tmp = strstr ( path, amlvideo2_str);
        str_find = strstr ( path, active_str );
        if (str_find) {
            if (!(tmp && (source_port == TVIN_PORT_CVBS3
                    || source_port == TVIN_PORT_HDMI0
                    || source_port == TVIN_PORT_HDMI1
                    || source_port == TVIN_PORT_HDMI2))) {
                is_active = TV_PATH_STATUS_ACTIVE;
                break;
            }
        }
    }

    fclose ( f );
    f = NULL;

    return is_active;
}

int CTvin::Tvin_CheckVideoPathComplete(tv_path_type_t path_type)
{
    int ret = -1;
    FILE *fp = NULL;
    char path[100] = {0};
    char decoder_str[20] = "default {";
    char tvin_str[20] = "tvpath {";
    char *str_find = NULL;
    char di_str[16] = "deinterlace";
    char ppmgr_str[16] = "ppmgr";
    char amvideo_str[16] = "amvideo";

    fp = fopen(SYS_VFM_MAP_PATH, "r");
    if (!fp) {
        LOGE ("%s, can not open %s!\n", __FUNCTION__, SYS_VFM_MAP_PATH);
        return ret;
    }

    while (fgets(path, sizeof(path)-1, fp)) {
        if (path_type == TV_PATH_TYPE_DEFAULT) {
            str_find = strstr(path, decoder_str);
        } else if (path_type == TV_PATH_TYPE_TVIN) {
            str_find = strstr(path, tvin_str);
        } else {
            break;
        }

        if (str_find != NULL) {
            if ((strstr(str_find, di_str)) &&
                 (strstr(str_find, ppmgr_str)) &&
                 (strstr(str_find, amvideo_str))) {
                LOGD("%s: VideoPath is complete!\n", __FUNCTION__);
                ret = 0;
            } else {
                LOGE("%s: VideoPath is not complete!\n", __FUNCTION__);
                ret = -1;
            }
            break;
        }
    }

    fclose(fp);
    fp = NULL;

    return ret;
}

int CTvin::Tv_init_afe ( void )
{
    //AFE_OpenModule();
    return 0;
}

int CTvin::Tv_uninit_afe ( void )
{
    AFE_CloseModule();
    return 0;
}

unsigned long CTvin::CvbsFtmToV4l2ColorStd(tvin_sig_fmt_t fmt)
{
    unsigned long v4l2_std = 0;
#ifdef SUPPORT_ADTV
    if (fmt == TVIN_SIG_FMT_CVBS_NTSC_M ||  fmt == TVIN_SIG_FMT_CVBS_NTSC_443) {
        v4l2_std = V4L2_COLOR_STD_NTSC;
    } else if (fmt >= TVIN_SIG_FMT_CVBS_PAL_I && fmt <= TVIN_SIG_FMT_CVBS_PAL_CN) {
        v4l2_std = V4L2_COLOR_STD_PAL;
    } else if (fmt == TVIN_SIG_FMT_CVBS_SECAM) {
        v4l2_std = V4L2_COLOR_STD_SECAM;
    } else {
        v4l2_std = V4L2_COLOR_STD_PAL;
    }
#endif
    return v4l2_std;
}

int CTvin::CvbsFtmToColorStdEnum(tvin_sig_fmt_t fmt)
{
    unsigned long v4l2_std = 0;
#ifdef SUPPORT_ADTV
    if (fmt == TVIN_SIG_FMT_CVBS_NTSC_M ||  fmt == TVIN_SIG_FMT_CVBS_NTSC_443) {
        v4l2_std = CC_ATV_VIDEO_STD_NTSC;
    } else if (fmt >= TVIN_SIG_FMT_CVBS_PAL_I && fmt <= TVIN_SIG_FMT_CVBS_PAL_CN) {
        v4l2_std = CC_ATV_VIDEO_STD_PAL;
    } else if (fmt == TVIN_SIG_FMT_CVBS_SECAM) {
        v4l2_std = CC_ATV_VIDEO_STD_SECAM;
    } else {
        v4l2_std = CC_ATV_VIDEO_STD_PAL;
    }
#endif
    return v4l2_std;
}

int CTvin::VDIN_GetAllmInfo(tvin_latency_s *AllmInfo)
{
    int ret = -1;
    if (AllmInfo == NULL) {
        LOGE("%s: param is NULL;\n", __FUNCTION__);
    } else {
        ret = VDIN_DeviceIOCtl(TVIN_IOC_GET_LATENCY_MODE, AllmInfo);
        if (ret < 0) {
            LOGE("%s: ioctl failed!\n", __FUNCTION__);
        }
    }

    return ret;
}

int CTvin::VDIN_SetGameMode(pq_status_update_e mode)
{
    int ret = -1;
    LOGD("%s: game mode: %d!\n", __FUNCTION__, mode);
    ret = VDIN_DeviceIOCtl(TVIN_IOC_GAME_MODE, &mode);
    if (ret < 0) {
        LOGE("%s failed, error(%s)!", __FUNCTION__, strerror(errno));
    }

    return ret;
}
