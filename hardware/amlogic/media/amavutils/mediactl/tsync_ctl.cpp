/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "tsync_ctl"
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
#include <tsync_ctl.h>

#ifdef  __cplusplus
extern "C" {
#endif

int media_set_tsync_enable(int enable)
{
	return media_sync_set_ctl("media.tsync.enable",enable);
}

int media_get_tsync_enable()
{
    return media_sync_get_ctl("media.tsync.enable");
}

int media_set_tsync_discontinue(int discontinue)
{
	return media_sync_set_ctl("media.tsync.discontinue",discontinue);
}

int media_get_pts_discontinue()
{
    return media_sync_get_ctl("media.tsync.discontinue");
}

int media_set_tsync_vpause_flag(int val)
{
	return media_sync_set_ctl("media.tsync.vpause_flag",val);
}

int media_get_tsync_vpause_flag()
{
    return media_sync_get_ctl("media.tsync.vpause_flag");
}

int media_set_tsync_pts_video(int val)
{
	return media_sync_set_ctl("media.tsync.pts_video",val);
}

int media_get_tsync_pts_video()
{
    return media_sync_get_ctl("media.tsync.pts_video");
}

int media_set_tsync_pts_audio(int val)
{
	return media_sync_set_ctl("media.tsync.pts_audio",val);
}

int media_get_tsync_pts_audio()
{
    return media_sync_get_ctl("media.tsync.pts_audio");
}

int media_set_tsync_dobly_av_sync(int val)
{
	return media_sync_set_ctl("media.tsync.dobly_av_sync",val);
}

int media_get_tsync_dobly_av_sync()
{
    return media_sync_get_ctl("media.tsync.dobly_av_sync");
}

int media_set_tsync_pts_pcrscr(int val)
{
	return media_sync_set_ctl("media.tsync.pts_pcrscr",val);
}

int media_get_tsync_pts_pcrscr()
{
    return media_sync_get_ctl("media.tsync.pts_pcrscr");
}

int media_set_tsync_even_strt(char* buf)
{
	return media_sync_set_ctl_str("media.tsync.event",buf);
}

int media_set_tsync_mode(int val)
{
	return media_sync_set_ctl("media.tsync.mode",val);
}

int media_get_tsync_mode()
{
    return media_sync_get_ctl("media.tsync.mode");
}

int media_set_tsync_pcr_recover(int val)
{
	return media_sync_set_ctl("media.tsync.pcr_recover",val);
}

int media_get_tsync_pcr_recover()
{
    return media_sync_get_ctl("media.tsync.pcr_recover");
}

int media_set_tsync_debug_pts_checkin(int val)
{
	return media_sync_set_ctl("media.tsync.debug_pts_checkin",val);
}

int media_get_tsync_debug_pts_checkin()
{
    return media_sync_get_ctl("media.tsync.debug_pts_checkin");
}

int media_set_tsync_debug_pts_checkout(int val)
{
	return media_sync_set_ctl("media.tsync.debug_pts_checkout",val);
}

int media_get_tsync_debug_pts_checkout()
{
    return media_sync_get_ctl("media.tsync.debug_pts_checkout");
}

int media_set_tsync_debug_video_pts(int val)
{
	return media_sync_set_ctl("media.tsync.debug_video_pts",val);
}

int media_get_tsync_debug_video_pts()
{
    return media_sync_get_ctl("media.tsync.debug_video_pts");
}

int media_set_tsync_debug_audio_pts(int val)
{
	return media_sync_set_ctl("media.tsync.debug_audio_pts",val);
}

int media_get_tsync_debug_audio_pts()
{
    return media_sync_get_ctl("media.tsync.debug_audio_pts");
}

int media_set_tsync_av_threshold_min(int val)
{
	return media_sync_set_ctl("media.tsync.av_threshold_min",val);
}

int media_get_tsync_av_threshold_min()
{
    return media_sync_get_ctl("media.tsync.av_threshold_min");
}

int media_set_tsync_av_threshold_max(int val)
{
	return media_sync_set_ctl("media.tsync.av_threshold_max",val);
}

int media_get_tsync_av_threshold_max()
{
    return media_sync_get_ctl("media.tsync.av_threshold_max");
}

int media_set_tsync_last_checkin_apts(int val)
{
	return media_sync_set_ctl("media.tsync.last_checkin_apts",val);
}

int media_get_tsync_last_checkin_apts()
{
    return media_sync_get_ctl("media.tsync.last_checkin_apts");
}

int media_set_tsync_firstvpts(int val)
{
	return media_sync_set_ctl("media.tsync.firstvpts",val);
}

int media_get_tsync_firstvpts()
{
    return media_sync_get_ctl("media.tsync.firstvpts");
}

int media_set_tsync_slowsync_enable(int val)
{
	return media_sync_set_ctl("media.tsync.slowsync_enable",val);
}

int media_get_tsync_slowsync_enable()
{
    return media_sync_get_ctl("media.tsync.slowsync_enable");
}

int media_set_tsync_startsync_mode(int val)
{
	return media_sync_set_ctl("media.tsync.startsync_mode",val);
}

int media_get_tsync_startsync_mode()
{
    return media_sync_get_ctl("media.tsync.startsync_mode");
}

int media_set_tsync_firstapts(int val)
{
	return media_sync_set_ctl("media.tsync.firstapts",val);
}

int media_get_tsync_firstapts()
{
    return media_sync_get_ctl("media.tsync.firstapts");
}

int media_set_tsync_checkin_firstvpts(int val)
{
	return media_sync_set_ctl("media.tsync.checkin_firstvpts",val);
}

int media_get_tsync_checkin_firstvpts()
{
    return media_sync_get_ctl("media.tsync.checkin_firstvpts");
}

#ifdef  __cplusplus
}
#endif