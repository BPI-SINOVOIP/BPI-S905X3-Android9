/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef TSYNC_CTL_H
#define TSYNC_CTL_H

#ifdef  __cplusplus
extern "C" {
#endif
int media_set_tsync_enable(int enable);
int media_set_tsync_discontinue(int discontinue);
int media_set_tsync_vpause_flag(int val);
int media_set_tsync_pts_video(int val);
int media_set_tsync_pts_audio(int val);
int media_set_tsync_dobly_av_sync(int val);
int media_set_tsync_pts_pcrscr(int val);
int media_set_tsync_even_strt(char* buf);
int media_set_tsync_mode(int val);
int media_set_tsync_pcr_recover(int val);
int media_set_tsync_debug_pts_checkin(int val);
int media_set_tsync_debug_pts_checkout(int val);
int media_set_tsync_debug_video_pts(int val);
int media_set_tsync_debug_audio_pts(int val);
int media_set_tsync_av_threshold_min(int val);
int media_set_tsync_av_threshold_max(int val);
int media_set_tsync_last_checkin_apts(int val);
int media_set_tsync_firstvpts(int val);
int media_set_tsync_slowsync_enable(int val);
int media_set_tsync_startsync_mode(int val);
int media_set_tsync_firstapts(int val);
int media_set_tsync_checkin_firstvpts(int val);
#ifdef  __cplusplus
}
#endif

#endif

