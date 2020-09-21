/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef VIDEO_CTL_H
#define VIDEO_CTL_H
#ifdef  __cplusplus
extern "C" {
#endif
#include "../mediactl/common_ctl.h"
int media_get_disable_video();
int media_set_disable_video(int disable);
int media_get_black_policy();
int media_set_black_policy(int val);
int media_get_screen_mode();
int media_set_screen_mode(int mode);
int media_clear_video_buf(int val);
int media_set_amports_debugflags(int flag);
int media_get_amports_debugflags();
int media_set_amports_def_4k_vstreambuf_sizeM(int size);
int media_get_amports_def_4k_vstreambuf_sizeM();
int media_set_amports_def_vstreambuf_sizeM(int size);
int media_get_amports_def_vstreambuf_sizeM();
int media_set_amports_slow_input(int flag);
int media_get_amports_slow_input();
int media_set_video_pause_one_3d_fl_frame(int val);
int media_get_video_pause_one_3d_fl_frame();
int media_set_video_debug_flag(int val);
int media_get_video_debug_flag();
int media_set_video_force_3d_scaler(int val);
int media_get_video_force_3d_scaler();
int media_set_vdieo_video_3d_format(int val);
int media_get_video_video_3d_format();
int media_set_video_vsync_enter_line_max(int val);
int media_get_video_vsync_enter_line_max();
int media_set_video_vsync_exit_line_max(int val);
int media_get_video_vsync_exit_line_max();
int media_set_video_vsync_rdma_line_max(int val);
int media_get_video_vsync_rdma_line_max();
int media_set_video_underflow(int val);
int media_get_video_underflow();
int media_set_video_next_peek_underflow(int val);
int media_get_video_next_peek_underflow();
int media_set_video_smooth_sync_enable(int val);
int media_get_video_smooth_sync_enable();
int media_set_video_hdmi_in_onvideo(int val);
int media_get_video_hdmi_in_onvideo();
int media_set_video_play_clone_rate(int val);
int media_get_video_play_clone_rate();
int media_set_video_android_clone_rate(int val);
int media_get_video_android_clone_rate();
int media_set_video_noneseamless_play_clone_rate(int val);
int media_get_video_noneseamless_play_clone_rate();
int media_set_video_cur_dev_idx(int val);
int media_get_video_cur_dev_idx();
int media_set_video_new_frame_count(int val);
int media_get_video_new_frame_count();
int media_set_video_omx_pts(int val);
int media_get_video_omx_pts();
int media_set_video_omx_pts_interval_upper(int val);
int media_get_video_omx_pts_interval_upper();
int media_set_video_omx_pts_interval_lower(int val);
int media_get_video_omx_pts_interval_lower();
int media_set_video_bypass_pps(int val);
int media_get_video_bypass_pps();
int media_set_video_platform_type(int val);
int media_get_video_platform_type();
int media_set_video_process_3d_type(int val);
int media_get_video_process_3d_type();
int media_set_video_framepacking_support(int val);
int media_get_video_framepacking_support();
int media_set_video_framepacking_width(int val);
int media_get_video_framepacking_width();
int media_set_video_framepacking_height(int val);
int media_get_video_framepacking_height();
int media_set_video_framepacking_blank(int val);
int media_get_video_framepacking_blank();
int media_set_video_reverse(int val);
int media_get_video_reverse();

#ifdef  __cplusplus
}
#endif

#endif
