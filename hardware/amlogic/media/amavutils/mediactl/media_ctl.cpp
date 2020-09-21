/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "media_ctl"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cutils/log.h>
#include <../mediaconfig/media_config.h>
#include <amports/amstream.h>
#include "common_ctl.h"
#include <media_ctl.h>
#include <video_ctl.h>

#ifdef  __cplusplus
extern "C" {
#endif

static int mediactl_set_black_policy(const char* path, int blackout)
{
	int para=blackout;
    if (path == NULL)
        return -1;
    return media_video_set_int(AMSTREAM_IOC_SET_BLACKOUT_POLICY, para);
}

static int mediactl_get_black_policy(const char* path)
{
    if (path == NULL)
        return -1;
    return media_video_get_int(AMSTREAM_IOC_GET_BLACKOUT_POLICY);
}

static int mediactl_get_disable_video(const char* path)
{
    if (path == NULL)
        return -1;
    return media_video_get_int(AMSTREAM_IOC_GET_VIDEO_DISABLE);
}

static int mediactl_set_disable_video(const char* path, int mode)
{
	int para=mode;
    if (path == NULL)
        return -1;
    return media_video_set_int(AMSTREAM_IOC_SET_VIDEO_DISABLE, para);
}

static int mediactl_set_screen_mode(const char* path, int mode)
{
	int para=mode;
    if (path == NULL)
        return -1;
    return media_video_set_int(AMSTREAM_IOC_SET_SCREEN_MODE, para);
}

static int mediactl_get_screen_mode(const char* path)
{
    if (path == NULL)
        return -1;
    return media_video_get_int(AMSTREAM_IOC_GET_SCREEN_MODE);
}

static int mediactl_clear_video_buf( const char * path, int val)
{
    int para=val;
    if (path == NULL)
        return -1;
    return media_video_set_int(AMSTREAM_IOC_CLEAR_VBUF, para);
}
int mediactl_get_subtitle_fps(const char* path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_FPS);
}

int mediactl_set_subtitle_fps(const char * path,int fps)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_FPS, fps);
}

int mediactl_get_subtitle_total(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_TOTAL);
}

int mediactl_set_subtitle_total(const char * path, int total)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_TOTAL, total);
}

int mediactl_get_subtitle_enable(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_ENABLE);
}

int mediactl_set_subtitle_enable(const char * path,int enable)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_ENABLE, enable);
}

int mediactl_get_subtitle_index(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_INDEX);
}

int mediactl_set_subtitle_index(const char * path,int index)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_INDEX, index);
}

int mediactl_get_subtitle_width(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_WIDTH);
}

int mediactl_set_subtitle_width(const char * path, int width)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_WIDTH, width);
}

int mediactl_get_subtitle_height(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_HEIGHT);
}

int mediactl_set_subtitle_height(const char * path, int height)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_HEIGHT, height);
}

int mediactl_get_subtitle_subtype(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_SUBTYPE);
}

int mediactl_set_subtitle_subtype(const char * path, int type)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_SUBTYPE, type);
}

int mediactl_get_subtitle_curr(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_CURRENT);
}

int mediactl_set_subtitle_curr(const char * path, int num)
{
    if (path == NULL)
        return -1;
    return media_sub_setinfo(SUB_CURRENT, num);
}

int mediactl_get_subtitle_startpts(const char * path)
{
    if (path == NULL)
        return -1;
    return media_sub_getinfo(SUB_START_PTS);
}

int mediactl_set_subtitle_startpts(const char * path, int startpts)
{
    if (path == NULL)
        return -1;
	return media_sub_setinfo(SUB_START_PTS, startpts);
}

static  MediaCtlPool Mediactl_SetPool[] = {
    {"/sys/class/tsync/enable","media.tsync.enable",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/slowsync_enable","media.tsync.slowsync_enable",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/startsync_mode","media.tsync.startsync_mode",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/av_threshold_min","media.tsync.av_threshold_min",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/vpause_flag","media.tsync.vpause_flag",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/pts_pcrscr","media.tsync.pts_pcrscr",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/pts_video","media.tsync.pts_video",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/pts_audio","media.tsync.pts_audio",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/vpause_flag","media.tsync.vpause_flag",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/mode","media.tsync.mode",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/firstapts","media.tsync.firstapts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/event","media.tsync.event",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/discontinue","media.tsync.discontinue",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/dobly_av_sync","media.tsync.dobly_av_sync",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/pcr_recover","media.tsync.pcr_recover",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/debug_pts_checkin","media.tsync.debug_pts_checkin",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/debug_pts_checkout","media.tsync.debug_pts_checkout",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/debug_video_pts","media.tsync.debug_video_pts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/debug_audio_pts","media.tsync.debug_audio_pts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/av_threshold_max","media.tsync.av_threshold_max",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/last_checkin_apts","media.tsync.last_checkin_apts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/firstvpts","media.tsync.firstvpts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/tsync/checkin_firstvpts","media.tsync.checkin_firstvpts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/audiodsp/digital_raw","/sys/class/audiodsp/digital_raw",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/audiodsp/ac3_drc_control","media.audio.audiodsp.ac3_drc_control",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/audiodsp/audio_samesource","media.audio.audiodsp.audio_samesource",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/audiodsp/codec_fatal_err","/sys/class/audiodsp/codec_fatal_err",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/audiodsp/digital_codec","media.audio.audiodsp.digital_codec",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/audiodsp/dts_dec_control","media.audio.audiodsp.dts_dec_control",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/new_frame_count","media.video.new_frame_count",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/hdmi_in_onvideo","media.video.hdmi_in_onvideo",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/pause_one_3d_fl_frame","media.video.pause_one_3d_fl_frame",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/pause_one_3d_fl_frame","media.video.pause_one_3d_fl_frame",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/debug_flag","media.video.debug_flag",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/force_3d_scaler","media.video.force_3d_scaler",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/video_3d_format","media.video.video_3d_format",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/vsync_enter_line_max","media.video.vsync_enter_line_max",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/vsync_enter_line_max","media.video.vsync_enter_line_max",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/vsync_exit_line_max","media.video.vsync_exit_line_max",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/vsync_rdma_line_max","media.video.vsync_rdma_line_max",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/underflow","media.video.underflow",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/next_peek_underflow","media.video.next_peek_underflow",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/smooth_sync_enable","media.video.smooth_sync_enable",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/hdmi_in_onvideo","media.video.hdmi_in_onvideo",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/video_play_clone_rate","media.video.video_play_clone_rate",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/android_clone_rate","media.video.android_clone_rate",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/noneseamless_play_clone_rate","media.video.noneseamless_play_clone_rate",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/cur_dev_idx","media.video.cur_dev_idx",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/omx_pts","media.video.omx_pts",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/omx_pts_interval_upper","media.video.omx_pts_interval_upper",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/bypass_pps","media.video.bypass_pps",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/platform_type","media.video.platform_type",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/process_3d_type","media.video.process_3d_type",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/framepacking_support","media.video.framepacking_support",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/framepacking_width","media.video.framepacking_width",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/framepacking_height","media.video.framepacking_height",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/framepacking_blank","media.video.framepacking_blank",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvideo/parameters/reverse","media.video.reverse",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_h265/parameters/double_write_mode","media.decoder.h265.double_write_mode",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_h265/parameters/buf_alloc_width","media.decoder.h265.buf_alloc_width",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_h265/parameters/buf_alloc_height","media.decoder.h265.buf_alloc_height",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_h265/parameters/buffer_mode","media.decoder.h265.buffer_mode",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_h265/parameters/dynamic_buf_num_margin","media.decoder.h265.dynamic_buf_num_margin",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_h265/parameters/debug","media.decoder.h265.debug",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_vp9/parameters/double_write_mode","media.decoder.vp9.double_write_mode",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_vp9/parameters/buf_alloc_width","media.decoder.vp9.buf_alloc_width",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/module/amvdec_vp9/parameters/buf_alloc_height","media.decoder.vp9.buf_alloc_height",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/vfm/map","media.vfm.map",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/video/video_seek_flag","media.video.video_seek_flag",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/video/slowsync_repeat_enable","media.video.slowsync_repeat_enable",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/video/show_first_frame_nosync","media.video.show_first_frame_nosync",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/amstream/videobufused","media.amports.videobufused",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/vdec/poweron_clock_level","media.decoder.vdec.poweron_clock_level",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/codec_mm/tvp_enable","media.codec_mm.trigger.tvp_enable",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/codec_mm/fastplay","media.codec_mm.trigger.fastplay",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
     {"/sys/class/codec_mm/debug","media.codec_mm.trigger.debug",
     media_set_ctl,media_get_ctl,
     media_set_ctl_str,media_get_ctl_str},
    {"/sys/class/video/blackout_policy","/dev/amvideo",
     mediactl_set_black_policy,mediactl_get_black_policy,
     NULL,NULL},
     {"/sys/class/video/screen_mode","/dev/amvideo",
     mediactl_set_screen_mode,mediactl_get_screen_mode,
     NULL,NULL},
     {"/sys/class/video/disable_video","/dev/amvideo",
     mediactl_set_disable_video,mediactl_get_disable_video,
     NULL,NULL},
     {"/sys/class/video/clear_video_buf","/dev/amvideo",
     mediactl_clear_video_buf,NULL,
     NULL,NULL},
	 {"/sys/class/subtitle/fps","/dev/amsubtitle",
     mediactl_set_subtitle_fps,mediactl_get_subtitle_fps,
     NULL,NULL},
     {"/sys/class/subtitle/total","/dev/amsubtitle",
     mediactl_set_subtitle_total,mediactl_get_subtitle_total,
     NULL,NULL},
     {"/sys/class/subtitle/curr","/dev/amsubtitle",
     mediactl_set_subtitle_curr,mediactl_get_subtitle_curr,
     NULL,NULL},
     {"/sys/class/subtitle/subtype","/dev/amsubtitle",
     mediactl_set_subtitle_subtype,mediactl_get_subtitle_subtype,
     NULL,NULL},
     {"/sys/class/subtitle/startpts","/dev/amsubtitle",
     mediactl_set_subtitle_startpts,mediactl_get_subtitle_startpts,
     NULL,NULL},
     {"/sys/class/subtitle/index","/dev/amsubtitle",
     mediactl_set_subtitle_index,mediactl_get_subtitle_index,
     NULL,NULL},
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

int mediactl_set_str_func(const char* path, char* val)
{
    MediaCtlPool *p  = Mediactl_SetPool;
    int ret = UnSupport;
    while (p && p->device) {
        if (!strcmp(p->device, path)) {
            if (p->mediactl_setstr) {
               ret = p->mediactl_setstr(p->path,val);
            }
            break;
        }
        p++;
    }
    return ret;
}

int mediactl_get_str_func(const char* path, char* valstr, int size)
{
    MediaCtlPool *p = Mediactl_SetPool;
    int ret = UnSupport;
    while (p && p->device) {
        if (!strcmp(p->device, path)) {
            if (p->mediactl_getstr) {
               ret = p->mediactl_getstr(p->path,valstr,size);
            }
            break;
        }
        p++;
    }
    return ret;
}

int mediactl_set_int_func(const char* path, int val)
{
    MediaCtlPool *p  = Mediactl_SetPool;
    int ret = UnSupport;
    while (p && p->device) {
        if (!strcmp(p->device, path)) {
            if (p->mediactl_setval) {
               ret = p->mediactl_setval(p->path,val);
            }
            break;
        }
        p++;
    }
    return ret;
}

int  mediactl_get_int_func(const char* path)
{
    MediaCtlPool *p  = Mediactl_SetPool;
    int ret = UnSupport;
    while (p && p->device) {
        if (!strcmp(p->device, path)) {
            if (p->mediactl_getval) {
               ret = p->mediactl_getval(p->path);
            }
            break;
        }
        p++;
    }
    return ret;
}

#ifdef  __cplusplus
}
#endif