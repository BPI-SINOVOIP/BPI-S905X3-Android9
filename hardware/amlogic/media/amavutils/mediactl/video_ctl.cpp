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
#include <amports/amstream.h>
#include "common_ctl.h"
#include <video_ctl.h>

#ifdef  __cplusplus
extern "C" {
#endif
int media_get_disable_video()
{
    return media_video_get_int(AMSTREAM_IOC_GET_VIDEO_DISABLE);
}

int media_set_disable_video(int disable)
{
    return media_video_set_int(AMSTREAM_IOC_SET_VIDEO_DISABLE, disable);
}

int media_get_black_policy()
{
    return media_video_get_int(AMSTREAM_IOC_GET_BLACKOUT_POLICY);
}

int media_set_black_policy(int val)
{
    return media_video_set_int(AMSTREAM_IOC_SET_BLACKOUT_POLICY, val);
}

int media_get_screen_mode()
{
    return media_video_get_int(AMSTREAM_IOC_GET_SCREEN_MODE);
}

int media_set_screen_mode(int mode)
{
    return media_video_set_int(AMSTREAM_IOC_SET_SCREEN_MODE, mode);
}

int media_clear_video_buf(int val)
{
    return media_video_set_int(AMSTREAM_IOC_CLEAR_VBUF,val);
}

int media_set_amports_debugflags(int flag)
{
	return media_set_ctl("media.amports.debugflags",flag);
}

int media_get_amports_debugflags()
{
    return media_get_ctl("media.amports.debugflags");
}

int media_set_amports_def_4k_vstreambuf_sizeM(int size)
{
	return media_set_ctl("media.amports.def_4k_vstreambuf_sizeM",size);
}

int media_get_amports_def_4k_vstreambuf_sizeM()
{
    return media_get_ctl("media.amports.def_4k_vstreambuf_sizeM");
}

int media_set_amports_def_vstreambuf_sizeM(int size)
{
	return media_set_ctl("media.amports.def_vstreambuf_sizeM",size);
}

int media_get_amports_def_vstreambuf_sizeM()
{
    return media_get_ctl("media.damports.def_vstreambuf_sizeM");
}

int media_set_amports_slow_input(int flag)
{
	return media_set_ctl("media.amports.slow_input",flag);
}

int media_get_amports_slow_input()
{
    return media_get_ctl("media.amports.slow_input");
}
int media_set_video_pause_one_3d_fl_frame(int val)
{
	return media_video_set_ctl("media.video.pause_one_3d_fl_frame", val);
}

int media_get_video_pause_one_3d_fl_frame()
{
    return media_video_get_ctl("media.video.pause_one_3d_fl_frame");
}

int media_set_video_debug_flag(int val)
{
	return media_video_set_ctl("media.video.debug_flag", val);
}

int media_get_video_debug_flag()
{
    return media_video_get_ctl("media.video.debug_flag");
}

int media_set_video_force_3d_scaler(int val)
{
	return media_video_set_ctl("media.video.force_3d_scaler", val);
}

int media_get_video_force_3d_scaler()
{
    return media_video_get_ctl("media.video.force_3d_scaler");
}

int media_set_vdieo_video_3d_format(int val)
{
	return media_video_set_ctl("media.video.video_3d_format", val);
}

int media_get_video_video_3d_format()
{
    return media_video_get_ctl("media.video.video_3d_format");
}

int media_set_video_vsync_enter_line_max(int val)
{
	return media_video_set_ctl("media.video.vsync_enter_line_max", val);
}

int media_get_video_vsync_enter_line_max()
{
    return media_video_get_ctl("media.video.vsync_enter_line_max");
}

int media_set_video_vsync_exit_line_max(int val)
{
	return media_sync_set_ctl("media.video.vsync_exit_line_max", val);
}

int media_get_video_vsync_exit_line_max()
{
    return media_sync_get_ctl("media.video.vsync_exit_line_max");
}

int media_set_video_vsync_rdma_line_max(int val)
{
	return media_video_set_ctl("media.video.vsync_rdma_line_max", val);
}

int media_get_video_vsync_rdma_line_max()
{
    return media_video_get_ctl("media.video.vsync_rdma_line_max");
}

int media_set_video_underflow(int val)
{
	return media_video_set_ctl("media.video.underflow", val);
}

int media_get_video_underflow()
{
    return media_video_get_ctl("media.video.underflow");
}

int media_set_video_next_peek_underflow(int val)
{
	return media_video_set_ctl("media.video.next_peek_underflow", val);
}

int media_get_video_next_peek_underflow()
{
    return media_video_get_ctl("media.video.next_peek_underflow");
}

int media_set_video_smooth_sync_enable(int val)
{
	return media_video_set_ctl("media.video.smooth_sync_enable", val);
}

int media_get_video_smooth_sync_enable()
{
    return media_video_get_ctl("media.video.smooth_sync_enable");
}

int media_set_video_hdmi_in_onvideo(int val)
{
	return media_video_set_ctl("media.video.hdmi_in_onvideo", val);
}

int media_get_video_hdmi_in_onvideo()
{
    return media_video_get_ctl("media.video.hdmi_in_onvideo");
}

int media_set_video_play_clone_rate(int val)
{
	return media_video_set_ctl("media.video.video_play_clone_rate", val);
}

int media_get_video_play_clone_rate()
{
    return media_video_get_ctl("media.video.video_play_clone_rate");
}

int media_set_video_android_clone_rate(int val)
{
	return media_video_set_ctl("media.video.android_clone_rate", val);
}

int media_get_video_android_clone_rate()
{
    return media_video_get_ctl("media.video.android_clone_rate");
}

int media_set_video_noneseamless_play_clone_rate(int val)
{
	return media_video_set_ctl("media.video.noneseamless_play_clone_rate", val);
}

int media_get_video_noneseamless_play_clone_rate()
{
    return media_video_get_ctl("media.video.noneseamless_play_clone_rate");
}

int media_set_video_cur_dev_idx(int val)
{
	return media_video_set_ctl("media.video.cur_dev_idx", val);
}

int media_get_video_cur_dev_idx()
{
    return media_video_get_ctl("media.video.cur_dev_idx");
}

int media_set_video_new_frame_count(int val)
{
	return media_video_set_ctl("media.video.new_frame_count", val);
}

int media_get_video_new_frame_count()
{
    return media_video_get_ctl("media.video.new_frame_count");
}

int media_set_video_omx_pts(int val)
{
	return media_video_set_ctl("media.video.omx_pts", val);
}

int media_get_video_omx_pts()
{
    return media_video_get_ctl("media.video.omx_pts");
}

int media_set_video_omx_pts_interval_upper(int val)
{
	return media_video_set_ctl("media.video.omx_pts_interval_upper", val);
}

int media_get_video_omx_pts_interval_upper()
{
    return media_video_get_ctl("media.video.omx_pts_interval_upper");
}

int media_set_video_omx_pts_interval_lower(int val)
{
	return media_video_set_ctl("media.video.omx_pts_interval_lower", val);
}

int media_get_video_omx_pts_interval_lower()
{
    return media_video_get_ctl("media.video.omx_pts_interval_lower");
}

int media_set_video_bypass_pps(int val)
{
	return media_video_set_ctl("media.video.bypass_pps", val);
}

int media_get_video_bypass_pps()
{
    return media_video_get_ctl("media.video.bypass_pps");
}

int media_set_video_platform_type(int val)
{
	return media_video_set_ctl("media.video.platform_type", val);
}

int media_get_video_platform_type()
{
    return media_video_get_ctl("media.video.platform_type");
}

int media_set_video_process_3d_type(int val)
{
	return media_video_set_ctl("media.video.process_3d_type", val);
}

int media_get_video_process_3d_type()
{
    return media_video_get_ctl("media.video.process_3d_type");
}

int media_set_video_framepacking_support(int val)
{
	return media_video_set_ctl("media.video.framepacking_support", val);
}

int media_get_video_framepacking_support()
{
    return media_video_get_ctl("media.video.framepacking_support");
}

int media_set_video_framepacking_width(int val)
{
	return media_video_set_ctl("media.video.framepacking_width", val);
}

int media_get_video_framepacking_width()
{
    return media_video_get_ctl("media.video.framepacking_width");
}

int media_set_video_framepacking_height(int val)
{
	return media_video_set_ctl("media.video.framepacking_height", val);
}

int media_get_video_framepacking_height()
{
    return media_video_get_ctl("media.video.framepacking_height");
}

int media_set_video_framepacking_blank(int val)
{
	return media_video_set_ctl("media.video.framepacking_blank", val);
}

int media_get_video_framepacking_blank()
{
    return media_video_get_ctl("media.video.framepacking_blank");
}

int media_set_video_reverse(int val)
{
	return media_video_set_ctl("media.video.reverse", val);
}

int media_get_video_reverse()
{
    return media_video_get_ctl("media.video.reverse");
}

#ifdef  __cplusplus
}
#endif
