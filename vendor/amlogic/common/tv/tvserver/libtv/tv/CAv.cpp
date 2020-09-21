/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CAv"

#include "CAv.h"
#include <CFile.h>
#include <tvutils.h>
#include <tvconfig.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

CAv::CAv()
{
    mpObserver = NULL;
    mTvPlayDevId = 0;
    mVideoLayerState = VIDEO_LAYER_NONE;
    mFdAmVideo = -1;
}

CAv::~CAv()
{
}

#ifdef SUPPORT_ADTV
void CAv::av_audio_callback(int event_type, AudioParms* param, void *user_data)
{
    LOGD ( "%s\n", __FUNCTION__ );

    CAv *pAv = ( CAv * ) user_data;
    if (NULL == pAv ) {
        LOGD ( "%s, ERROR : av_audio_callback NULL == pTv\n", __FUNCTION__ );
        return ;
    }
    if ( pAv->mpObserver == NULL ) {
        LOGD ( "%s, ERROR : mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return ;
    }

    switch ( event_type ) {
        case AM_AV_EVT_AUDIO_CB:
            pAv->mCurAvEvent.type = AVEvent::EVENT_AUDIO_CB;
            pAv->mCurAvEvent.status = param->cmd;
            pAv->mCurAvEvent.param = param->param1;
            pAv->mCurAvEvent.param1 = param->param2;

            pAv->mpObserver->onEvent(pAv->mCurAvEvent);

            break;
     }
     return ;
}
#endif
int CAv::SetVideoWindow(int x, int y, int w, int h)
{
#ifdef SUPPORT_ADTV
    return AM_AV_SetVideoWindow (mTvPlayDevId, x, y, w, h );
#else
    return -1;
#endif
}

int CAv::Open()
{
#ifdef SUPPORT_ADTV
    AM_AV_OpenPara_t para_av;
    memset ( &para_av, 0, sizeof ( AM_AV_OpenPara_t ) );
    int rt = AM_AV_Open ( mTvPlayDevId, &para_av );
    if ( rt != DVB_SUCCESS ) {
        LOGD ( "%s, dvbplayer_open fail %d %d\n!" , __FUNCTION__,  mTvPlayDevId, rt );
        return -1;
    }

    //open audio channle output
    AM_AOUT_OpenPara_t aout_para;
    memset ( &aout_para, 0, sizeof ( AM_AOUT_OpenPara_t ) );
    rt = AM_AOUT_Open ( mTvPlayDevId, &aout_para );
    if ( DVB_SUCCESS != rt ) {
        LOGD ( "%s,  BUG: CANN'T OPEN AOUT\n", __FUNCTION__);
    }

    mFdAmVideo = open ( PATH_VIDEO_AMVIDEO, O_RDWR );
    if ( mFdAmVideo < 0 ) {
        LOGE ( "mFdAmVideo < 0, error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    /*Register events*/
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AV_NO_DATA, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AV_DATA_RESUME, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_SCAMBLED, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AUDIO_SCAMBLED, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_NOT_SUPPORT, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_AVAILABLE, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_PLAYER_UPDATE_INFO, av_evt_callback, this );

    AM_AV_SetAudioCallback(0,av_audio_callback,this);
    return rt;
#else
    return -1;
#endif
}

int CAv::Close()
{
    int iRet = -1;
#ifdef SUPPORT_ADTV
    iRet = AM_AV_Close ( mTvPlayDevId );
    iRet = AM_AOUT_Close ( mTvPlayDevId );
    if (mFdAmVideo > 0) {
        close(mFdAmVideo);
        mFdAmVideo = -1;
    }

    AM_AV_SetAudioCallback(0,NULL,NULL);
#endif
    return iRet;
}

int CAv::GetVideoStatus(int *w, int *h, int *fps, int *interlace)
{
#ifdef SUPPORT_ADTV
    AM_AV_VideoStatus_t status;
    int ret = AM_AV_GetVideoStatus(mTvPlayDevId, &status);
    *w = status.src_w;
    *h = status.src_h;
    *fps = status.fps;
    *interlace = status.interlaced;
    return ret;

#else
    return -1;
#endif
}

int CAv::GetAudioStatus( int fmt[2], int sample_rate[2], int resolution[2], int channels[2],
    int lfepresent[2], int *frames, int *ab_size, int *ab_data, int *ab_free)
{
#ifdef SUPPORT_ADTV
    AM_AV_AudioStatus_t status;
    int ret = AM_AV_GetAudioStatus(mTvPlayDevId, &status);
    if (fmt) {
        fmt[0] = status.aud_fmt;
        fmt[1] = status.aud_fmt_orig;
    }
    if (sample_rate) {
        sample_rate[0] = status.sample_rate;
        sample_rate[1] = status.sample_rate_orig;
    }
    if (resolution) {
        resolution[0] = status.resolution;
        resolution[1] = status.resolution_orig;
    }
    if (channels) {
        channels[0] = status.channels;
        channels[1] = status.channels_orig;
    }
    if (lfepresent) {
        lfepresent[0] = status.lfepresent;
        lfepresent[1] = status.lfepresent_orig;
    }
    if (frames)
        *frames = status.frames;
    if (ab_size)
        *ab_size = status.ab_size;
    if (ab_data)
        *ab_data = status.ab_data;
    if (ab_free)
        *ab_free = status.ab_free;

    return ret;
#else
    return -1;
#endif
}

int CAv::SwitchTSAudio(int apid, int afmt)
{
#ifdef SUPPORT_ADTV
    return AM_AV_SwitchTSAudio (mTvPlayDevId, ( unsigned short ) apid, ( AM_AV_AFormat_t ) afmt );
#else
    return -1;
#endif
}

int CAv::ResetAudioDecoder()
{
#ifdef SUPPORT_ADTV
    return AM_AV_ResetAudioDecoder ( mTvPlayDevId );
#else
    return -1;
#endif
}

int CAv::SetADAudio(unsigned int enable, int apid, int afmt)
{
#ifdef SUPPORT_ADTV
    return AM_AV_SetAudioAd(mTvPlayDevId, enable, apid, ( AM_AV_AFormat_t ) afmt);
#else
    return -1;
#endif
}

int CAv::SetTSSource(int ts_source)
{
#ifdef SUPPORT_ADTV
    return AM_AV_SetTSSource ( mTvPlayDevId, (AM_AV_TSSource_t) ts_source );
#else
    return -1;
#endif
}

int CAv::StartTS(unsigned short vpid, unsigned short apid, unsigned short pcrid, int vfmt, int afmt)
{
#ifdef SUPPORT_ADTV
    return AM_AV_StartTSWithPCR ( mTvPlayDevId, vpid, apid, pcrid, ( AM_AV_VFormat_t ) vfmt, ( AM_AV_AFormat_t ) afmt );
#else
    return -1;
#endif
}

int CAv::StopTS()
{
#ifdef SUPPORT_ADTV
    return AM_AV_StopTS (mTvPlayDevId);
#else
    return -1;
#endif
}

int CAv::AudioGetOutputMode(int *mode)
{
#ifdef SUPPORT_ADTV
    return AM_AOUT_GetOutputMode ( mTvPlayDevId, (AM_AOUT_OutputMode_t *) mode );
#else
    return -1;
#endif
}

int CAv::AudioSetOutputMode(int mode)
{
#ifdef SUPPORT_ADTV
    return AM_AOUT_SetOutputMode ( mTvPlayDevId, (AM_AOUT_OutputMode_t) mode );
#else
    return -1;
#endif
}

int CAv::AudioSetPreGain(float pre_gain)
{
#ifdef SUPPORT_ADTV
    return AM_AOUT_SetPreGain(0, pre_gain);
#else
    return -1;
#endif

}

int CAv::AudioGetPreGain(float *gain)
{
#ifdef SUPPORT_ADTV
    return AM_AOUT_GetPreGain(0, gain);
#else
    return -1;
#endif

}

int CAv::AudioSetPreMute(unsigned int mute)
{
#ifdef SUPPORT_ADTV
    return AM_AOUT_SetPreMute(0, mute);
#else
    return -1;
#endif

}

int CAv::AudioGetPreMute(unsigned int *mute)
{
    int ret = -1;
#ifdef SUPPORT_ADTV
    bool btemp_mute = 0;
    ret = AM_AOUT_GetPreMute(0, (AM_Bool_t *) &btemp_mute);
    *mute = btemp_mute ? 1 : 0;
#endif
    return ret;
}

int CAv::EnableVideoBlackout()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);
    mVideoLayerState = VIDEO_LAYER_NONE;
#ifdef SUPPORT_ADTV
    return AM_AV_EnableVideoBlackout(mTvPlayDevId);
#else
    return -1;
#endif

}

int CAv::DisableVideoBlackout()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);
    mVideoLayerState = VIDEO_LAYER_NONE;
#ifdef SUPPORT_ADTV
    return AM_AV_DisableVideoBlackout(mTvPlayDevId);
#else
    return -1;
#endif

}

int CAv::DisableVideoWithBlueColor()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);

    mVideoLayerState = VIDEO_LAYER_DISABLE_BLUE;
    SetVideoScreenColor ( 0, 41, 240, 110 ); // Show blue with vdin0, postblending disabled
#ifdef SUPPORT_ADTV
    return AM_AV_DisableVideo(mTvPlayDevId);
#else
    return -1;
#endif

}

int CAv::DisableVideoWithBlackColor()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);

    mVideoLayerState = VIDEO_LAYER_DISABLE_BLACK;
    SetVideoScreenColor ( 0, 16, 128, 128 ); // Show black with vdin0, postblending disabled

#ifdef SUPPORT_ADTV
    return AM_AV_DisableVideo(mTvPlayDevId);
#else
    return -1;
#endif
}

//just enable video
int CAv::EnableVideoNow(bool IsShowTestScreen)
{
    LOGD("%s:IsShowTestScreen is %d, mVideoLayerState is %d\n", __FUNCTION__, IsShowTestScreen, mVideoLayerState);

    if (mVideoLayerState == VIDEO_LAYER_ENABLE) {
        LOGW("video is enabled");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_ENABLE;
    if (IsShowTestScreen) {
        LOGD("%s: eableVideoWithBlackColor SwitchSourceTime = %fs", __FUNCTION__,getUptimeSeconds());
        SetVideoScreenColor ( 0, 16, 128, 128 );
    }

#ifdef SUPPORT_ADTV
    return AM_AV_EnableVideo(mTvPlayDevId);
#else
    return -1;
#endif
}

//call disable video 2
int CAv::ClearVideoBuffer()
{
    LOGD("%s", __FUNCTION__);
#ifdef SUPPORT_ADTV
    return AM_AV_ClearVideoBuffer(mTvPlayDevId);
#else
    return -1;
#endif
}

int CAv::startTimeShift(void *para)
{
    LOGD("%s", __FUNCTION__);
#ifdef SUPPORT_ADTV
    return AM_AV_StartTimeshift(mTvPlayDevId, (AM_AV_TimeshiftPara_t *)para);
#else
    return -1;
#endif
}

int CAv::stopTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    return AM_AV_StopTimeshift(mTvPlayDevId);
#else
    return -1;
#endif
}

int CAv::pauseTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    return AM_AV_PauseTimeshift (mTvPlayDevId);
#else
    return -1;
#endif
}

int CAv::resumeTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    return AM_AV_ResumeTimeshift (mTvPlayDevId);
#else
    return -1;
#endif
}

int CAv::seekTimeShift(int pos, bool start)
{
    LOGD ( "%s: [pos:%d start:%d]", __FUNCTION__, pos, start);

#ifdef SUPPORT_ADTV
    return AM_AV_SeekTimeshift (mTvPlayDevId, pos, start);
#else
    return -1;
#endif
}

int CAv::setTimeShiftSpeed(int speed)
{
    LOGD ( "%s: [%d]", __FUNCTION__, speed);
    int ret = 0;
#ifdef SUPPORT_ADTV
    if (speed == 0)
        ret = AM_AV_ResumeTimeshift (mTvPlayDevId);
    else if (speed < 0)
        ret = AM_AV_FastBackwardTimeshift(mTvPlayDevId, -speed);
    else
        ret = AM_AV_FastForwardTimeshift(mTvPlayDevId, speed);
#endif
    return ret;
}

int CAv::switchTimeShiftAudio(int apid, int afmt)
{
    LOGD ( "%s: [pid:%d, fmt:%d]", __FUNCTION__, apid, afmt);
#ifdef SUPPORT_ADTV
    return AM_AV_SwitchTimeshiftAudio (mTvPlayDevId, apid, afmt);
#else
    return -1;
#endif
}

int CAv::playTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    return AM_AV_PlayTimeshift (mTvPlayDevId);
#else
    return -1;
#endif
}

//auto enable,
int CAv::EnableVideoAuto()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);
    if (mVideoLayerState == VIDEO_LAYER_ENABLE) {
        LOGW("video is enable");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_ENABLE;
    LOGD("%s: eableVideoWithBlackColor\n", __FUNCTION__);
    SetVideoScreenColor ( 0, 16, 128, 128 ); // Show black with vdin0, postblending disabled
    ClearVideoBuffer();//disable video 2
    return 0;
}

int CAv::WaittingVideoPlaying(int minFrameCount , int waitTime )
{
    LOGD("%s", __FUNCTION__);
    static const int COUNT_FOR_TIME = 20;
    int times = waitTime / COUNT_FOR_TIME;

    for (int i = 0; i < times; i++) {
        if (videoIsPlaying(minFrameCount)) {
            return 0;
        }
    }

    return -1;
}

int CAv::EnableVideoWhenVideoPlaying(int minFrameCount, int waitTime)
{
    LOGD("%s", __FUNCTION__);
    int ret = WaittingVideoPlaying(minFrameCount, waitTime);
    if (ret == 0) { //ok to playing
        EnableVideoNow(true);
    }
    return ret;
}

bool CAv::videoIsPlaying(int minFrameCount)
{
    LOGD("%s", __FUNCTION__);
    int value[2] = {0};
    value[0] = getVideoFrameCount();
    usleep(20 * 1000);
    value[1] = getVideoFrameCount();
    LOGD("videoIsPlaying framecount =%d = %d", value[0], value[1]);

    if (value[1] >= minFrameCount && (value[1] > value[0]))
        return true;

    return false;
}

int CAv::getVideoFrameCount()
{
    char buf[32] = {0};

    tvReadSysfs(PATH_FRAME_COUNT, buf);
    return atoi(buf);
}

tvin_sig_fmt_t CAv::getVideoResolutionToFmt()
{
    char buf[32] = {0};
    tvin_sig_fmt_e sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;

    tvReadSysfs(SYS_VIDEO_FRAME_HEIGHT, buf);
    int height = atoi(buf);
    if (height <= 576) {
        sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
    } else if (height > 576 && height <= 720) {
        sig_fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
    } else if (height > 720 && height <= 1088) {
        sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    } else {
        sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
    }
    return sig_fmt;
}

int CAv::SetVideoScreenColor ( int vdin_blending_mask, int y, int u, int v )
{
    unsigned long value = vdin_blending_mask << 24;
    value |= ( unsigned int ) ( y << 16 ) | ( unsigned int ) ( u << 8 ) | ( unsigned int ) ( v );

    LOGD("%s, vdin_blending_mask:%d,y:%d,u:%d,v:%d", __FUNCTION__, vdin_blending_mask, y, u, v);

    char val[64] = {0};
    sprintf(val, "0x%lx", ( unsigned long ) value);
    tvWriteSysfs(VIDEO_TEST_SCREEN, val);
    return 0;
}

int CAv::SetVideoLayerStatus ( int value )
{
    LOGD("%s, value = %d" , __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, val);
    return 0;
}

int CAv::setVideoScreenMode ( int value )
{
    char val[64] = {0};
    sprintf(val, "%d", value);
    tvWriteSysfs(VIDEO_SCREEN_MODE, val);
    return 0;
}

int CAv::getVideoScreenMode()
{
    char buf[32] = {0};

    tvReadSysfs(VIDEO_SCREEN_MODE, buf);
    return atoi(buf);
}

/**
 * @function: set test pattern on video layer.
 * @param r,g,b int 0~255.
 */
int CAv::setRGBScreen(int r, int g, int b)
{
    int value = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
    return tvWriteSysfs(VIDEO_RGB_SCREEN, value, 16);
}

int CAv::getRGBScreen()
{
    char value[32] = {0};
    tvReadSysfs(VIDEO_RGB_SCREEN, value);
    return strtol(value, NULL, 10);
}


int CAv::setVideoAxis ( int h, int v, int width, int height )
{
    LOGD("%s, %d %d %d %d", __FUNCTION__, h, v, width, height);

    char value[64] = {0};
    sprintf(value, "%d %d %d %d", h, v, width, height);
    tvWriteSysfs(VIDEO_AXIS, value);
    return 0;
}

video_display_resolution_t CAv::getVideoDisplayResolution()
{
    video_display_resolution_t  resolution;
    char attrV[SYS_STR_LEN] = {0};

    tvReadSysfs(VIDEO_DEVICE_RESOLUTION, attrV);

    if (strncasecmp(attrV, "1366x768", strlen ("1366x768")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1366X768;
    } else if (strncasecmp(attrV, "3840x2160", strlen ("3840x2160")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_3840X2160;
    } else if (strncasecmp(attrV, "1920x1080", strlen ("1920x1080")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    } else {
        LOGW("video display resolution is = (%s) not define , default it", attrV);
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    }

    return resolution;
}

void CAv::av_evt_callback ( long dev_no, int event_type, void *param, void *user_data )
{
    CAv *pAv = ( CAv * ) user_data;
    if (NULL == pAv ) {
        LOGD ( "%s, ERROR : av_evt_callback NULL == pTv\n", __FUNCTION__ );
        return ;
    }
    if ( pAv->mpObserver == NULL ) {
        LOGD ( "%s, ERROR : mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return;
    }
#ifdef SUPPORT_ADTV
    switch ( event_type ) {
    case AM_AV_EVT_AV_NO_DATA:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_STOP;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_AV_DATA_RESUME:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_RESUEM;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_VIDEO_SCAMBLED:
    case AM_AV_EVT_AUDIO_SCAMBLED:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_SCAMBLED;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_VIDEO_NOT_SUPPORT: {
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_UNSUPPORT;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    }
    case AM_AV_EVT_VIDEO_AVAILABLE: {
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_VIDEO_AVAILABLE;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    }
    case AM_AV_EVT_PLAYER_UPDATE_INFO: {
        AM_AV_TimeshiftInfo_t *info = (AM_AV_TimeshiftInfo_t*)param;
        if (info) {
            pAv->mCurAvEvent.type = AVEvent::EVENT_PLAY_UPDATE;
            pAv->mCurAvEvent.param = info->current_time;
            pAv->mCurAvEvent.status = info->status;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        }
        break;
    }
    default:
        break;
    }
#endif
    LOGD ( "%s, av_evt_callback : dev_no %ld type %d param = %d\n",
        __FUNCTION__, dev_no, pAv->mCurAvEvent.type , (long)param);
}

int CAv::setLookupPtsForDtmb(int enable)
{
    LOGD ( "%s: status %d", __FUNCTION__, enable);

    char value[64] = {0};
    sprintf(value, "%d", enable);
    tvWriteSysfs(PATH_MEPG_DTMB_LOOKUP_PTS_FLAG, value);
    return 0;
}

