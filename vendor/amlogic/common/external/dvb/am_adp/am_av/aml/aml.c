#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief AMLogic 音视频解码驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-09: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5
#define _LARGEFILE64_SOURCE

#include <am_debug.h>
#include <am_misc.h>
#include <am_mem.h>
#include <am_evt.h>
#include <am_time.h>
#include "am_dmx.h"
#include <am_thread.h>
#include "../am_av_internal.h"
#include "../../am_aout/am_aout_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <amports/amstream.h>
#ifdef ANDROID
#include <cutils/properties.h>
#endif
#include "am_ad.h"
#include "am_queue.h"

#ifdef SUPPORT_CAS
#include "aml_drm.h"
#endif
#include <am_cond.h>
#include "am_userdata.h"

#ifdef ANDROID
#include <cutils/properties.h>
#include <sys/system_properties.h>
#endif


#ifdef CHIP_8226H
#include <linux/jpegdec.h>
#endif
#if defined(CHIP_8226M) || defined(CHIP_8626X)
#include <linux/amports/jpegdec.h>
#endif

//#define PLAYER_API_NEW
#define ADEC_API_NEW

#include "player.h"
#define PLAYER_INFO_POP_INTERVAL 500
#define FILENAME_LENGTH_MAX 2048

#include <codec_type.h>
#ifdef USE_ADEC_IN_DVB
#include <adec-external-ctrl.h>
#endif
void *adec_handle = NULL;

#ifndef TRICKMODE_NONE
#define TRICKMODE_NONE      0x00
#define TRICKMODE_I         0x01
#define TRICKMODE_FFFB      0x02
#define TRICK_STAT_DONE     0x01
#define TRICK_STAT_WAIT     0x00
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define AOUT_DEV_NO 0

#define ENABLE_AUDIO_RESAMPLE
#if 0
#define ENABLE_DROP_BFRAME
#endif
//#define ENABLE_BYPASS_DI
#ifdef ANDROID
#define ENABLE_PCR
#endif

#define ADEC_START_AUDIO_LEVEL       1024
#define ADEC_START_VIDEO_LEVEL       2048
#define ADEC_FORCE_START_AUDIO_LEVEL 2048
#define DEC_STOP_AUDIO_LEVEL         16
#define DEC_STOP_VIDEO_LEVEL         512
#define UP_RESAMPLE_AUDIO_LEVEL      128
#define UP_RESAMPLE_VIDEO_LEVEL      1024
#define DOWN_RESAMPLE_CACHE_TIME     90000*2
#define NO_DATA_CHECK_TIME           2000
#define VMASTER_REPLAY_TIME          4000
#define SCRAMBLE_CHECK_TIME          1000
#define TIMESHIFT_INJECT_DIFF_TIME	 (90000*4)
#define TIMESHIFT_FFFB_ERROR_CNT	 5
#define VIDEO_AVAILABLE_MIN_CNT     2
#define TIME_UNIT90K                 90000
#define AUDIO_NO_DATA_TIME          20
#define FFFB_JUMP_STEP               500
#ifdef ENABLE_PCR
#ifndef AMSTREAM_IOC_PCRID
#define AMSTREAM_IOC_PCRID        _IOW(AMSTREAM_IOC_MAGIC, 0x4f, int)
#endif
#endif

#if !defined (AMLOGIC_LIBPLAYER)
#define  AUDIO_CTRL_DEVICE    "/dev/amaudio_ctl"
#define AMAUDIO_IOC_MAGIC  'A'
#define AMAUDIO_IOC_SET_LEFT_MONO               _IOW(AMAUDIO_IOC_MAGIC, 0x0e, int)
#define AMAUDIO_IOC_SET_RIGHT_MONO              _IOW(AMAUDIO_IOC_MAGIC, 0x0f, int)
#define AMAUDIO_IOC_SET_STEREO                  _IOW(AMAUDIO_IOC_MAGIC, 0x10, int)
#define AMAUDIO_IOC_SET_CHANNEL_SWAP            _IOW(AMAUDIO_IOC_MAGIC, 0x11, int)
#endif


#ifdef ANDROID
/*for add new path*/
#define DVB_STB_SOURCE_FILE "/sys/class/stb/source"
#define TSYNCPCR_RESETFLAG_FILE "/sys/class/tsync_pcr/tsync_pcr_reset_flag"
#define DI_BYPASS_ALL_FILE "/sys/module/di/parameters/bypass_all"
#define DVB_STB_DEMUXSOURCE_FILE "/sys/class/stb/demux%d_source"
#define DVB_STB_ASYNCFIFO_FLUSHSIZE_FILE "/sys/class/stb/asyncfifo0_flush_size"
#define TSYNC_ENABLE_FILE "/sys/class/tsync/enable"
#define VIDEO_SHOW_FIRSTFRM_NOSYNC_FILE "/sys/class/video/show_first_frame_nosync"


#define STREAM_VBUF_FILE    "/dev/amstream_vbuf"
#define STREAM_ABUF_FILE    "/dev/amstream_abuf"
#define STREAM_TS_FILE      "/dev/amstream_mpts"
#define STREAM_TS_SCHED_FILE      "/dev/amstream_mpts_sched"
#define STREAM_PS_FILE      "/dev/amstream_mpps"
#define STREAM_RM_FILE      "/dev/amstream_rm"
#define JPEG_DEC_FILE       "/dev/amjpegdec"
#define AMVIDEO_FILE        "/dev/amvideo"

#define PPMGR_FILE			"/dev/ppmgr"

#define VID_AXIS_FILE       "/sys/class/video/axis"
#define VID_CROP_FILE       "/sys/class/video/crop"
#define VID_CONTRAST_FILE   "/sys/class/video/contrast"
#define VID_SATURATION_FILE "/sys/class/video/saturation"
#define VID_BRIGHTNESS_FILE "/sys/class/video/brightness"
#define VID_DISABLE_FILE    "/sys/class/video/disable_video"
#define VID_BLACKOUT_FILE   "/sys/class/video/blackout_policy"
#define VID_SCREEN_MODE_FILE  "/sys/class/video/screen_mode"
#define VID_SCREEN_MODE_FILE  "/sys/class/video/screen_mode"
#define VID_ASPECT_RATIO_FILE "/sys/class/video/aspect_ratio"
#define VID_ASPECT_MATCH_FILE "/sys/class/video/matchmethod"


#define VDEC_H264_ERROR_RECOVERY_MODE_FILE "/sys/module/amvdec_h264/parameters/error_recovery_mode"
#define VDEC_H264_FATAL_ERROR_RESET_FILE "/sys/module/amvdec_h264/parameters/fatal_error_reset"
#define DISP_MODE_FILE      "/sys/class/display/mode"
#define ASTREAM_FORMAT_FILE "/sys/class/astream/format"
#define VIDEO_DROP_BFRAME_FILE  "/sys/module/amvdec_h264/parameters/enable_toggle_drop_B_frame"
#define DI_BYPASS_FILE    "/sys/module/di/parameters/bypass_post"
#define ENABLE_RESAMPLE_FILE    "/sys/class/amaudio/enable_resample"
#define RESAMPLE_TYPE_FILE      "/sys/class/amaudio/resample_type"
#define AUDIO_DMX_PTS_FILE	"/sys/class/stb/audio_pts"
#define VIDEO_DMX_PTS_FILE	"/sys/class/stb/video_pts"
#define AUDIO_DMX_PTS_BIT32_FILE	"/sys/class/stb/audio_pts_bit32"
#define VIDEO_DMX_PTS_BIT32_FILE	"/sys/class/stb/video_pts_bit32"
#define AUDIO_PTS_FILE	"/sys/class/tsync/pts_audio"
#define VIDEO_PTS_FILE	"/sys/class/tsync/pts_video"
#define TSYNC_MODE_FILE "/sys/class/tsync/mode"
#define AV_THRESHOLD_MIN_FILE "/sys/class/tsync/av_threshold_min"
#define AV_THRESHOLD_MAX_FILE "/sys/class/tsync/av_threshold_max"
#define AVS_PLUS_DECT_FILE "/sys/module/amvdec_avs/parameters/profile"
#define DEC_CONTROL_H264 "/sys/module/amvdec_h264/parameters/dec_control"
#define DEC_CONTROL_MPEG12 "/sys/module/amvdec_mpeg12/parameters/dec_control"
#define VIDEO_NEW_FRAME_COUNT_FILE "/sys/module/amvideo/parameters/new_frame_count"
#define AUDIO_DSP_DIGITAL_RAW_FILE "/sys/class/audiodsp/digital_raw"
#define VID_FRAME_FMT_FILE   "/sys/class/video/frame_format"
#define VIDEO_NEW_FRAME_TOGGLED_FILE "/sys/module/amvideo/parameters/first_frame_toggled"
#define TSYNC_FIRSTCHECKIN_APTS_FILE "/sys/class/tsync/checkin_firstapts"
#define TSYNC_FIRSTCHECKIN_VPTS_FILE "/sys/class/tsync/checkin_firstvpts"
#define TSYNC_PCR_MODE_FILE	"/sys/class/tsync_pcr/tsync_pcr_mode"
#define TSYNC_AUDIO_STATE_FILE	"/sys/class/tsync_pcr/tsync_audio_state"
#else

/*for add new path*/
#define DVB_STB_SOURCE_FILE "/sys/class/stb/source"
#define TSYNCPCR_RESETFLAG_FILE "/sys/class/tsync_pcr/tsync_pcr_reset_flag"
#define DI_BYPASS_ALL_FILE "/sys/module/di/parameters/bypass_all"
#define DVB_STB_DEMUXSOURCE_FILE "/sys/class/stb/demux%d_source"
#define DVB_STB_ASYNCFIFO_FLUSHSIZE_FILE "/sys/class/stb/asyncfifo0_flush_size"
#define TSYNC_ENABLE_FILE "/sys/class/tsync/enable"
#define VIDEO_SHOW_FIRSTFRM_NOSYNC_FILE "/sys/class/video/show_first_frame_nosync"


#define STREAM_VBUF_FILE    "/dev/amstream_vbuf"
#define STREAM_ABUF_FILE    "/dev/amstream_abuf"
#define STREAM_TS_FILE      "/dev/amstream_mpts"
#define STREAM_TS_SCHED_FILE      "/dev/amstream_mpts_sched"
#define STREAM_PS_FILE      "/dev/amstream_mpps"
#define STREAM_RM_FILE      "/dev/amstream_rm"
#define JPEG_DEC_FILE       "/dev/amjpegdec"
#define AMVIDEO_FILE        "/dev/amvideo"

#define PPMGR_FILE			"/dev/ppmgr"

#define VID_AXIS_FILE       "/sys/class/video/axis"
#define VID_CROP_FILE       "/sys/class/video/crop"
#define VID_CONTRAST_FILE   "/sys/class/video/contrast"

/*not find,but android is exist*/
#define VID_SATURATION_FILE "/sys/class/video/saturation"

#define VID_BRIGHTNESS_FILE "/sys/class/video/brightness"
#define VID_DISABLE_FILE    "/sys/class/video/disable_video"
#define VID_BLACKOUT_FILE   "/sys/class/video/blackout_policy"
#define VID_SCREEN_MODE_FILE  "/sys/class/video/screen_mode"
#define VID_FRAME_FMT_FILE   "/sys/class/video/frame_format"

#define VDEC_H264_ERROR_RECOVERY_MODE_FILE "/sys/module/vh264/parameters/error_recovery_mode"
#define VDEC_H264_FATAL_ERROR_RESET_FILE "/sys/module/vh264/parameters/fatal_error_reset"
#define DISP_MODE_FILE      "/sys/class/display/mode"
#define ASTREAM_FORMAT_FILE "/sys/class/astream/format"

/*not find,android is not find*/
#define VIDEO_DROP_BFRAME_FILE  "/sys/module/amvdec_h264/parameters/enable_toggle_drop_B_frame"


#define DI_BYPASS_FILE    "/sys/module/di/parameters/bypass_post"
#define VIDEO_NEW_FRAME_TOGGLED_FILE "/sys/module/amvideo/parameters/first_frame_toggled"

/*not find,android is not find*/
#define ENABLE_RESAMPLE_FILE    "/sys/class/amaudio/enable_resample"
#define RESAMPLE_TYPE_FILE      "/sys/class/amaudio/resample_type"

#define AUDIO_DMX_PTS_FILE	"/sys/class/stb/audio_pts"
#define VIDEO_DMX_PTS_FILE	"/sys/class/stb/video_pts"
#define AUDIO_PTS_FILE	"/sys/class/tsync/pts_audio"
#define VIDEO_PTS_FILE	"/sys/class/tsync/pts_video"
#define TSYNC_MODE_FILE "/sys/class/tsync/mode"
#define AV_THRESHOLD_MIN_FILE "/sys/class/tsync/av_threshold_min"
#define AV_THRESHOLD_MAX_FILE "/sys/class/tsync/av_threshold_max"
#define TSYNC_FIRSTCHECKIN_APTS_FILE "/sys/class/tsync/checkin_firstapts"
#define TSYNC_FIRSTCHECKIN_VPTS_FILE "/sys/class/tsync/checkin_firstvpts"
#define AUDIO_DMX_PTS_BIT32_FILE	"/sys/class/stb/audio_pts_bit32"
#define VIDEO_DMX_PTS_BIT32_FILE	"/sys/class/stb/video_pts_bit32"

/*not find,android is not find*/
#define AVS_PLUS_DECT_FILE "/sys/module/amvdec_avs/parameters/profile"

#define DEC_CONTROL_H264 "/sys/module/vh264/parameters/dec_control"
#define DEC_CONTROL_MPEG12 "/sys/module/vmpeg12/parameters/dec_control"
#define VIDEO_NEW_FRAME_COUNT_FILE "/sys/module/frame_sink/parameters/new_frame_count"
#define AUDIO_DSP_DIGITAL_RAW_FILE "/sys/class/audiodsp/digital_raw"
#define TSYNC_PCR_MODE_FILE	"/sys/class/tsync_pcr/tsync_pcr_mode"
#define TSYNC_AUDIO_STATE_FILE	"/sys/class/tsync_pcr/tsync_audio_state"
#endif

#ifdef ANDROID
#define DEC_CONTROL_PROP "media.dec_control"
#define AC3_AMASTER_PROP "media.ac3_amaster"
#endif
#define SHOW_FIRSTFRAME_NOSYNC_PROP "tv.dtv.showfirstframe_nosync"
#define REPLAY_ENABLE_PROP	"tv.dtv.replay_enable"

#define CANVAS_ALIGN(x)    (((x)+7)&~7)
#define JPEG_WRTIE_UNIT    (32*1024)
#define AV_SYNC_THRESHOLD	60
#define AV_SMOOTH_SYNC_VAL "100"

#ifdef ANDROID
#define open(a...)\
	({\
	 int ret, times=300;\
	 do{\
	 	ret = open(a);\
	 	if(ret==-1)\
	 		usleep(10000);\
	 }while(ret==-1 && times--);\
	 ret;\
	 })
#endif

#define PPMGR_IOC_MAGIC         'P'
#define PPMGR_IOC_2OSD0         _IOW(PPMGR_IOC_MAGIC, 0x00, unsigned int)
#define PPMGR_IOC_ENABLE_PP     _IOW(PPMGR_IOC_MAGIC, 0X01, unsigned int)
#define PPMGR_IOC_CONFIG_FRAME  _IOW(PPMGR_IOC_MAGIC, 0X02, unsigned int)
#define PPMGR_IOC_VIEW_MODE     _IOW(PPMGR_IOC_MAGIC, 0X03, unsigned int)

#define MODE_3D_DISABLE         0x00000000
#define MODE_3D_ENABLE          0x00000001
#define MODE_AUTO               0x00000002
#define MODE_2D_TO_3D           0x00000004
#define MODE_LR                 0x00000008
#define MODE_BT                 0x00000010
#define MODE_LR_SWITCH          0x00000020
#define MODE_FIELD_DEPTH        0x00000040
#define MODE_3D_TO_2D_L         0x00000080
#define MODE_3D_TO_2D_R         0x00000100
#define LR_FORMAT_INDICATOR     0x00000200
#define BT_FORMAT_INDICATOR     0x00000400

#define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)

static AM_ErrorCode_t set_dec_control(AM_Bool_t enable);

#define VALID_VIDEO(_pid_, _fmt_) (VALID_PID(_pid_))
#define VALID_PCR(_pid_) (VALID_PID(_pid_))

#ifdef USE_ADEC_IN_DVB
#define VALID_AUDIO(_pid_, _fmt_) (VALID_PID(_pid_) && audio_get_format_supported(_fmt_))
#else
#define VALID_AUDIO(_pid_, _fmt_) (VALID_PID(_pid_))
#endif

/*
 * As not inlude "adec-external-ctrl.h", so we need declare
 * audio_decode_set_volume. Otherwise it will lost the value of the second
 * param, AKA volume.
 */
extern int audio_decode_set_volume(void *, float);
#ifdef SUPPORT_CAS
#define SECURE_BLOCK_SIZE 256*1024
/* Round up the even multiple of size, size has to be a multiple of 2 */
#define ROUNDUP(v, size) (((v) + ((__typeof__(v))(size) - 1)) & \
													~((__typeof__(v))(size) - 1))
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 音视频数据注入*/
typedef struct
{
	int             fd;
	int             video_fd;
	pthread_t       thread;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	const uint8_t  *data;
	int             len;
	int             times;
	int             pos;
	int             dev_no;
	AM_Bool_t       is_audio;
	AM_Bool_t       running;
	void           *adec;
} AV_DataSource_t;

/**\brief JPEG解码相关数据*/
typedef struct
{
	int             vbuf_fd;
	int             dec_fd;
} AV_JPEGData_t;

/**\brief JPEG解码器状态*/
typedef enum {
	AV_JPEG_DEC_STAT_DECDEV,
	AV_JPEG_DEC_STAT_INFOCONFIG,
	AV_JPEG_DEC_STAT_INFO,
	AV_JPEG_DEC_STAT_DECCONFIG,
	AV_JPEG_DEC_STAT_RUN
} AV_JPEGDecState_t;

#ifdef PLAYER_API_NEW
/**\brief 文件播放器数据*/
typedef struct
{
	int                media_id;
	play_control_t pctl;
	pthread_mutex_t    lock;
	pthread_cond_t     cond;
	AM_AV_Device_t    *av;
} AV_FilePlayerData_t;
#endif

/**\brief 数据注入播放模式相关数据*/
typedef struct
{
	AM_AV_AFormat_t    aud_fmt;
	AM_AV_VFormat_t    vid_fmt;
	int                sub_type;
	AM_AV_PFormat_t    pkg_fmt;
	int                aud_fd;
	int                vid_fd;
	int                aud_id;
	int                vid_id;
	int                sub_id;
	int                cntl_fd;
	void              *adec;
	AM_AD_Handle_t ad;
} AV_InjectData_t;

/**\brief TS player 相关数据*/
typedef struct
{
	int fd;
	int vid_fd;
	void *adec;
	AM_AD_Handle_t ad;
} AV_TSData_t;

/**\brief Timeshift播放状态*/
typedef enum
{
	AV_TIMESHIFT_STAT_STOP,
	AV_TIMESHIFT_STAT_PLAY,
	AV_TIMESHIFT_STAT_PAUSE,
	AV_TIMESHIFT_STAT_FFFB,
	AV_TIMESHIFT_STAT_EXIT,
	AV_TIMESHIFT_STAT_INITOK,
	AV_TIMESHIFT_STAT_SEARCHOK,
} AV_TimeshiftState_t;

typedef struct {
	AV_PlayCmd_t cmd;
	int done;
	union {
		struct {
			int seek_pos;
		} seek;
		struct {
			int speed;
		} speed;
	};
} AV_PlayCmdPara_t;

#define TIMESHIFT_CMD_Q_SIZE (32)
QUEUE_DECLARATION(timeshift_cmd_q, AV_PlayCmdPara_t, TIMESHIFT_CMD_Q_SIZE);

/**\brief Timeshift播放模式相关数据*/
typedef struct
{
	int					running;
	int					rate;	/**< bytes/s*/
	int					duration;	/**< dur for playback, ms*/
	int					dmxfd;
	int					start;/**< start time, ms*/
	int					end;/**< end time, ms*/
	int					current;/**< current time, ms*/
	int					left;
	int					inject_size;
	int					timeout;
	loff_t				rtotal;
	int					rtime;
	struct timeshift_cmd_q  cmd_q;
	AV_PlayCmdPara_t        last_cmd[2];
	pthread_t			thread;
	pthread_mutex_t		lock;
	pthread_cond_t		cond;
	AV_TimeshiftState_t	state;
	AM_TFile_t	file;
	int offset;
	#define TIMESHIFT_TFILE_AUTOCREATE 1
	#define TIMESHIFT_TFILE_DETACHED 2
	int file_flag;
	AM_AV_Device_t       *dev;
	AM_AV_TimeshiftInfo_t	info;
	char 				last_stb_src[16];
	char 				last_dmx_src[16];

	AV_TimeShiftPlayPara_t para;

	AV_TSPlayPara_t		tp;

	AV_TSData_t			ts;

	int                     pause_time;
#ifdef SUPPORT_CAS
	int cas_open;
	uint8_t *buf;
#endif
	int                     fffb_start;
	int                     fffb_current;
	int                     eof;
} AV_TimeshiftData_t;

struct AM_AUDIO_Driver
{
	void (*adec_start_decode)(int fd, int fmt, int has_video, void **padec);
	void (*adec_stop_decode)(void **padec);
	void (*adec_pause_decode)(void *adec);
	void (*adec_resume_decode)(void *adec);
	void (*adec_set_decode_ad)(int enable, int pid, int fmt, void *adec);
	void (*adec_get_status)(void *adec, AM_AV_AudioStatus_t *para);
	AM_ErrorCode_t (*adec_set_volume)(AM_AOUT_Device_t *dev, int vol);
	AM_ErrorCode_t (*adec_set_mute)(AM_AOUT_Device_t *dev, AM_Bool_t mute);
	AM_ErrorCode_t (*adec_set_output_mode)(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
	AM_ErrorCode_t (*adec_set_pre_gain)(AM_AOUT_Device_t *dev, float gain);
	AM_ErrorCode_t (*adec_set_pre_mute)(AM_AOUT_Device_t *dev, AM_Bool_t mute);
};
typedef struct AM_AUDIO_Driver  AM_AUDIO_Driver_t;

static AM_AV_Audio_CB_t s_audio_cb = NULL;
static void *pUserData = NULL;

enum AV_SyncForce_e {
	FORCE_NONE,
	FORCE_AC3_AMASTER,
};

/****************************************************************************
 * Static data
 ***************************************************************************/

/*音视频设备操作*/
static AM_ErrorCode_t aml_open(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para);
static AM_ErrorCode_t aml_close(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_open_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode);
static AM_ErrorCode_t aml_start_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para);
static AM_ErrorCode_t aml_close_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode);
static AM_ErrorCode_t aml_ts_source(AM_AV_Device_t *dev, AM_AV_TSSource_t src);
static AM_ErrorCode_t aml_file_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
static AM_ErrorCode_t aml_inject_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
static AM_ErrorCode_t aml_file_status(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status);
static AM_ErrorCode_t aml_file_info(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info);
static AM_ErrorCode_t aml_set_video_para(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val);
static AM_ErrorCode_t aml_inject(AM_AV_Device_t *dev, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout);
static AM_ErrorCode_t aml_video_frame(AM_AV_Device_t *dev, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s);
static AM_ErrorCode_t aml_get_astatus(AM_AV_Device_t *dev, AM_AV_AudioStatus_t *para);
static AM_ErrorCode_t aml_get_vstatus(AM_AV_Device_t *dev, AM_AV_VideoStatus_t *para);
static AM_ErrorCode_t aml_timeshift_fill_data(AM_AV_Device_t *dev, uint8_t *data, int size);
static AM_ErrorCode_t aml_timeshift_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para);
static AM_ErrorCode_t aml_timeshift_get_info(AM_AV_Device_t *dev, AM_AV_TimeshiftInfo_t *info);
static AM_ErrorCode_t aml_set_vpath(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_switch_ts_audio_fmt(AM_AV_Device_t *dev, AV_TSData_t *ts, AV_TSPlayPara_t *tp);
static AM_ErrorCode_t aml_switch_ts_audio(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt);
static AM_ErrorCode_t aml_reset_audio_decoder(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_set_drm_mode(AM_AV_Device_t *dev, AM_AV_DrmMode_t drm_mode);
static AM_ErrorCode_t aml_set_audio_ad(AM_AV_Device_t *dev, int enable, uint16_t apid, AM_AV_AFormat_t afmt);
static AM_ErrorCode_t aml_set_inject_audio(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt);
static AM_ErrorCode_t aml_set_inject_subtitle(AM_AV_Device_t *dev, uint16_t spid, int stype);
static AM_ErrorCode_t aml_timeshift_get_tfile(AM_AV_Device_t *dev, AM_TFile_t *tfile);
static void 		  aml_set_audio_cb(AM_AV_Device_t *dev,AM_AV_Audio_CB_t cb,void *user_data);

static int aml_restart_inject_mode(AM_AV_Device_t *dev, AM_Bool_t destroy_thread);
static AM_ErrorCode_t aml_get_pts(AM_AV_Device_t *dev, int type, uint64_t *pts);
static AM_ErrorCode_t aml_get_timeout_real(int timeout, struct timespec *ts);
static void aml_timeshift_update_info(AV_TimeshiftData_t *tshift, AM_AV_TimeshiftInfo_t *info);
static AM_ErrorCode_t aml_set_fe_status(AM_AV_Device_t *dev, int value);


const AM_AV_Driver_t aml_av_drv =
{
.open        = aml_open,
.close       = aml_close,
.open_mode   = aml_open_mode,
.start_mode  = aml_start_mode,
.close_mode  = aml_close_mode,
.ts_source   = aml_ts_source,
.file_cmd    = aml_file_cmd,
.inject_cmd  = aml_inject_cmd,
.file_status = aml_file_status,
.file_info   = aml_file_info,
.set_video_para = aml_set_video_para,
.inject      = aml_inject,
.video_frame = aml_video_frame,
.get_audio_status = aml_get_astatus,
.get_video_status = aml_get_vstatus,
.timeshift_fill = aml_timeshift_fill_data,
.timeshift_cmd = aml_timeshift_cmd,
.get_timeshift_info = aml_timeshift_get_info,
.set_vpath   = aml_set_vpath,
.switch_ts_audio = aml_switch_ts_audio,
.reset_audio_decoder = aml_reset_audio_decoder,
.set_drm_mode = aml_set_drm_mode,
.set_audio_ad = aml_set_audio_ad,
.set_inject_audio = aml_set_inject_audio,
.set_inject_subtitle = aml_set_inject_subtitle,
.timeshift_get_tfile = aml_timeshift_get_tfile,
.set_audio_cb = aml_set_audio_cb,
.get_pts = aml_get_pts,
.set_fe_status = aml_set_fe_status,

};

/*音频控制（通过解码器）操作*/
static AM_ErrorCode_t adec_aout_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t adec_aout_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t adec_aout_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t adec_aout_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t adec_aout_close(AM_AOUT_Device_t *dev);
static AM_ErrorCode_t adec_aout_set_pre_gain(AM_AOUT_Device_t *dev, float gain);
static AM_ErrorCode_t adec_aout_set_pre_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);

const AM_AOUT_Driver_t adec_aout_drv =
{
.open         = adec_aout_open,
.set_volume   = adec_aout_set_volume,
.set_mute     = adec_aout_set_mute,
.set_output_mode = adec_aout_set_output_mode,
.close        = adec_aout_close,
.set_pre_gain = adec_aout_set_pre_gain,
.set_pre_mute = adec_aout_set_pre_mute,
};

static AM_ErrorCode_t adec_aout_open_cb(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t adec_aout_set_volume_cb(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t adec_aout_set_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t adec_aout_set_output_mode_cb(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t adec_aout_close_cb(AM_AOUT_Device_t *dev);
static AM_ErrorCode_t adec_aout_set_pre_gain_cb(AM_AOUT_Device_t *dev, float gain);
static AM_ErrorCode_t adec_aout_set_pre_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute);

const AM_AOUT_Driver_t adec_aout_drv_cb =
{
.open         = adec_aout_open_cb,
.set_volume   = adec_aout_set_volume_cb,
.set_mute     = adec_aout_set_mute_cb,
.set_output_mode = adec_aout_set_output_mode_cb,
.close        = adec_aout_close_cb,
.set_pre_gain = adec_aout_set_pre_gain_cb,
.set_pre_mute = adec_aout_set_pre_mute_cb,
};

#if 0
/*音频控制（通过amplayer2）操作*/
static AM_ErrorCode_t amp_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t amp_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t amp_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t amp_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t amp_close(AM_AOUT_Device_t *dev);

const AM_AOUT_Driver_t amplayer_aout_drv =
{
.open         = amp_open,
.set_volume   = amp_set_volume,
.set_mute     = amp_set_mute,
.set_output_mode = amp_set_output_mode,
.close        = amp_close
};
#endif
static void adec_start_decode(int fd, int fmt, int has_video, void **padec);
static void adec_stop_decode(void **padec);
static void adec_set_decode_ad(int enable, int pid, int fmt, void *adec);
static AM_ErrorCode_t adec_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t adec_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t adec_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t adec_set_pre_gain(AM_AOUT_Device_t *dev, float gain);
static AM_ErrorCode_t adec_set_pre_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static void adec_pause_decode(void *handle);
static void adec_resume_decode(void *handle);
static void adec_get_status(void *adec, AM_AV_AudioStatus_t *para);

#ifdef USE_ADEC_IN_DVB
const AM_AUDIO_Driver_t native_audio_drv =
{
.adec_start_decode = adec_start_decode,
.adec_pause_decode = adec_pause_decode,
.adec_resume_decode = adec_resume_decode,
.adec_stop_decode = adec_stop_decode,
.adec_set_decode_ad = adec_set_decode_ad,
.adec_set_volume = adec_set_volume,
.adec_set_mute = adec_set_mute,
.adec_set_output_mode = adec_set_output_mode,
.adec_set_pre_gain = adec_set_pre_gain,
.adec_set_pre_mute = adec_set_pre_mute,
.adec_get_status = adec_get_status
};
#endif
static void adec_start_decode_cb(int fd, int fmt, int has_video, void **padec);
static void adec_stop_decode_cb(void **padec);
static void adec_set_decode_ad_cb(int enable, int pid, int fmt, void *adec);
static AM_ErrorCode_t adec_set_volume_cb(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t adec_set_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t adec_set_output_mode_cb(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t adec_set_pre_gain_cb(AM_AOUT_Device_t *dev, float gain);
static AM_ErrorCode_t adec_set_pre_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static void adec_pause_decode_cb(void *adec);
static void adec_resume_decode_cb(void *adec);
static void adec_get_status_cb(void *adec, AM_AV_AudioStatus_t *para);

const AM_AUDIO_Driver_t callback_audio_drv =
{
.adec_start_decode = adec_start_decode_cb,
.adec_pause_decode = adec_pause_decode_cb,
.adec_resume_decode = adec_resume_decode_cb,
.adec_stop_decode = adec_stop_decode_cb,
.adec_set_decode_ad = adec_set_decode_ad_cb,
.adec_set_volume = adec_set_volume_cb,
.adec_set_mute = adec_set_mute_cb,
.adec_set_output_mode = adec_set_output_mode_cb,
.adec_set_pre_gain = adec_set_pre_gain_cb,
.adec_set_pre_mute = adec_set_pre_mute_cb,
.adec_get_status = adec_get_status_cb
};

#ifdef USE_ADEC_IN_DVB
static AM_AUDIO_Driver_t *audio_ops = &native_audio_drv;
#else
static AM_AUDIO_Driver_t *audio_ops = &callback_audio_drv;
#endif
int g_festatus = 0;

/*监控AV buffer, PTS 操作*/
static pthread_mutex_t gAVMonLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gAVMonCond = PTHREAD_COND_INITIALIZER;

static void* aml_av_monitor_thread(void *arg);

/*Timeshift 操作*/
static void *aml_timeshift_thread(void *arg);

static AV_TimeshiftData_t *m_tshift = NULL;

#if 0
typedef struct {
	unsigned int    format;  ///< video format, such as H264, MPEG2...
	unsigned int    width;   ///< video source width
	unsigned int    height;  ///< video source height
	unsigned int    rate;    ///< video source frame duration
	unsigned int    extra;   ///< extra data information of video stream
	unsigned int    status;  ///< status of video stream
	float           ratio;   ///< aspect ratio of video source
	void *          param;   ///< other parameters for video decoder
} dec_sysinfo_t;
#endif
static dec_sysinfo_t am_sysinfo;


static AM_ErrorCode_t aml_set_ad_source(AM_AD_Handle_t *ad, int enable, int pid, int fmt, void *user);
static int aml_calc_sync_mode(AM_AV_Device_t *dev, int has_audio, int has_video, int has_pcr, int afmt, int *force_reason);
static int aml_set_sync_mode(AM_AV_Device_t *dev, int mode);
static void *aml_try_open_crypt(AM_Crypt_Ops_t *ops);
/****************************************************************************
 * Static functions
 ***************************************************************************/

/****************************************************************************
 * for now only mpeg2/h264/h265/vp9/avs2/mjpeg/mpeg4
 * support multi instance video decoder
 * use /dev/amstream_mpts_sched
 ***************************************************************************/
static float getUptimeSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (float)(ts.tv_sec +(float)ts.tv_nsec / 1000000000);
}

static AM_Bool_t configure_vdec_info(int fd, char *p_info, uint32_t len)
{
	struct am_ioctl_parm_ptr param;

	memset(&param, 0, sizeof(param));
	param.cmd = AMSTREAM_SET_PTR_CONFIGS;
	param.pointer = p_info;
	param.len = len;

	if (ioctl(fd, AMSTREAM_IOC_SET_PTR, (unsigned long)&param) == -1)
	{
		AM_DEBUG(1, "configure vdec info [%s] failed\n", p_info);
		return AM_FALSE;
	}
	AM_DEBUG(1, "configure vdec info [%s] success\n", p_info);
	return AM_TRUE;
}
static AM_Bool_t check_vfmt_support_sched(AM_AV_VFormat_t vfmt)
{
	if (vfmt == VFORMAT_MPEG12 ||
			vfmt == VFORMAT_H264 ||
			vfmt == VFORMAT_HEVC ||
			vfmt == VFORMAT_MJPEG ||
			vfmt == VFORMAT_MPEG4 ||
			vfmt == VFORMAT_VP9 ||
			vfmt == VFORMAT_AVS2)
		return AM_TRUE;
	else
		return AM_FALSE;
}

static AM_Bool_t show_first_frame_nosync(void)
{
	char buf[32];

	if (AM_FileRead(VIDEO_SHOW_FIRSTFRM_NOSYNC_FILE, buf, sizeof(buf)) >= 0) {
		int v = atoi(buf);

		return (v == 1) ? AM_TRUE : AM_FALSE;
	}

	return AM_FALSE;
}

static void set_first_frame_nosync(void)
{
#ifdef ANDROID
	int syncmode = property_get_int32(SHOW_FIRSTFRAME_NOSYNC_PROP, 1);
	AM_DEBUG(1, "%s %d", __FUNCTION__,syncmode);
	AM_FileEcho(VIDEO_SHOW_FIRSTFRM_NOSYNC_FILE, syncmode?"1":"0");
#else
	AM_FileEcho(VIDEO_SHOW_FIRSTFRM_NOSYNC_FILE, "1");
#endif
}

static int get_amstream(AM_AV_Device_t *dev)
{
	if (dev->mode & AV_PLAY_TS)
	{
		if(dev->ts_player.drv_data)
			return ((AV_TSData_t *)dev->ts_player.drv_data)->fd;
		else
			return -1;
	}
	else if (dev->mode & AV_INJECT)
	{
		AV_InjectData_t *inj = (AV_InjectData_t *)dev->inject_player.drv_data;
		if (inj->vid_fd != -1)
			return inj->vid_fd;
		else
			return inj->aud_fd;
	}
	else if (dev->mode & AV_PLAY_VIDEO_ES)
	{
		AV_DataSource_t *src = (AV_DataSource_t *)dev->vid_player.drv_data;
		return src->fd;
	}
	else if (dev->mode & AV_PLAY_AUDIO_ES)
	{
		AV_DataSource_t *src = (AV_DataSource_t *)dev->aud_player.drv_data;
		return src->fd;
	}
	// else if (dev->mode & (AV_GET_JPEG_INFO | AV_DECODE_JPEG))
	// {
	// 	AV_JPEGData_t *jpeg = (AV_JPEGData_t *)dev->vid_player.drv_data;
	// 	return jpeg->vbuf_fd;
	// }

	return -1;
}


#define AUD_ASSO_PROP "media.audio.enable_asso"
#define AUD_ASSO_MIX_PROP "media.audio.mix_asso"
#define TIMESHIFT_DBG_PROP "tv.dvb.tf.debug"
/*mode/enable -1=ignore*/
#define TSYNC_FORCE_PROP "tv.dvb.tsync.force"
/*v,a,p -1=ignore*/
#define PID_FORCE_PROP "tv.dvb.pid.force"
#define FMT_FORCE_PROP "tv.dvb.fmt.force"

static int _get_prop_int(char *prop, int def) {
	char v[32];
	int val = 0;
#ifdef ANDROID
	property_get(prop, v, "0");
#else
	strcpy(v, "0");
#endif
	if (sscanf(v, "%d", &val) != 1)
		val = def;
	return val;
}

static int _get_prop_int3(char *prop, int *val1, int *val2, int *val3, int def1, int def2, int def3) {
	char v[32];
	int v1 = -1, v2 = -1, v3 = -1;
	int cnt = 0;
#ifdef ANDROID
	property_get(prop, v, "-1,-1,-1");
#else
	strcpy(v, "-1,-1,-1");
#endif
	cnt = sscanf(v, "%i,%i,%i", &v1, &v2, &v3);
	if (cnt < 3)
		v3 = def3;
	if (cnt < 2)
		v2 = def2;
	if (cnt < 1)
		v1 = def1;
	if (val1)
		*val1 = v1;
	if (val2)
		*val2 = v2;
	if (val3)
		*val3 = v3;
	return 0;
}

#define DVB_LOGLEVEL_PROP "tv.dvb.loglevel"

static int _get_asso_enable() {
#ifdef ANDROID
	return property_get_int32(AUD_ASSO_PROP, 0);
#else
	return 0;
#endif
}
static int _get_asso_mix() {
#ifdef ANDROID
	return property_get_int32(AUD_ASSO_MIX_PROP, 50);
#else
	return 0;
#endif
}
static int _get_dvb_loglevel() {
#ifdef ANDROID
	return property_get_int32(DVB_LOGLEVEL_PROP, AM_DEBUG_LOGLEVEL_DEFAULT);
#else
	return 0;//AM_DEBUG_LOGLEVEL_DEFAULT;
#endif
}
static int _get_timethift_dbg() {
#ifdef ANDROID
	return property_get_int32(TIMESHIFT_DBG_PROP, 0);
#else
	return 0;;
#endif
}
static int _get_tsync_mode_force() {
	int v;
	_get_prop_int3(TSYNC_FORCE_PROP, &v, NULL, NULL, -1, -1, -1);
	return v;
}
static int _get_tsync_enable_force() {
	int v;
	_get_prop_int3(TSYNC_FORCE_PROP, NULL, &v, NULL, -1, -1, -1);
	return v;
}

static void setup_forced_pid(AV_TSPlayPara_t *tp)
{
	int v, a, p;

	_get_prop_int3(PID_FORCE_PROP, &v, &a, &p, -1, -1, -1);

	if (v != -1) {
		tp->vpid = v;
		AM_DEBUG(1, "force vpid: %d", tp->vpid);
	}
	if (a != -1) {
		tp->apid = a;
		AM_DEBUG(1, "force apid: %d", tp->apid);
	}
	if (p != -1) {
		tp->pcrpid = p;
		AM_DEBUG(1, "force ppid: %d", tp->pcrpid);
	}

	_get_prop_int3(FMT_FORCE_PROP, &v, &a, NULL, -1, -1, -1);

	if (v != -1) {
		tp->vfmt = v;
		AM_DEBUG(1, "force vfmt: %d", tp->vfmt);
	}
	if (a != -1) {
		tp->afmt = a;
		AM_DEBUG(1, "force afmt: %d", tp->afmt);
	}
}

static AM_ErrorCode_t adec_aout_open_cb(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	return 0;
}
static AM_ErrorCode_t adec_aout_set_volume_cb(AM_AOUT_Device_t *dev, int vol)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_VOLUME;
	audio_parms.param1 = vol;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}

	return 0;
}
static AM_ErrorCode_t adec_aout_set_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_MUTE;
	audio_parms.param1 = mute;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}

	return 0;
}
static AM_ErrorCode_t adec_aout_set_output_mode_cb(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_OUTPUT_MODE;
	audio_parms.param1 = mode;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return 0;
}
static AM_ErrorCode_t adec_aout_close_cb(AM_AOUT_Device_t *dev)
{
	return 0;
}
static AM_ErrorCode_t adec_aout_set_pre_gain_cb(AM_AOUT_Device_t *dev, float gain)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_PRE_GAIN;
	audio_parms.param1 = gain*100;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}

	return 0;
}
static AM_ErrorCode_t adec_aout_set_pre_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_PRE_MUTE;
	audio_parms.param1 = mute;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}

	return 0;
}

static void adec_start_decode_cb(int fd, int fmt, int has_video, void **padec)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_START_DECODE;
	audio_parms.param1 = fmt;
	audio_parms.param2 = has_video;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}

	AM_AOUT_SetDriver(AOUT_DEV_NO, &adec_aout_drv_cb, NULL);

	// just for compatile with native decoder.
	*padec = (void *)1;
	adec_handle = (void *) 1;
}

static void adec_pause_decode_cb(void *adec)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	if (!adec) {
		AM_DEBUG(1, "audio closed, ignore");
		return;
	}

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_PAUSE_DECODE;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
}
static void adec_resume_decode_cb(void *adec)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	if (!adec) {
		AM_DEBUG(1, "audio closed, ignore");
		return;
	}

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_RESUME_DECODE;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
}

static void adec_stop_decode_cb(void **padec)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	if (!padec || !*padec) {
		AM_DEBUG(1, "audio closed, ignore");
		return;
	}

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_STOP_DECODE;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}

	*padec = NULL;
	adec_handle = NULL;
	return ;
}
static void adec_set_decode_ad_cb(int enable, int pid, int fmt, void *adec)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d pid:%d,fmt:%d\n", __FUNCTION__,__LINE__,pid,fmt);

	if (!adec) {
		AM_DEBUG(1, "audio closed, ignore");
		return;
	}

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_DECODE_AD;
	audio_parms.param1 = fmt;
	audio_parms.param2 = pid;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return ;
}
static AM_ErrorCode_t adec_set_volume_cb(AM_AOUT_Device_t *dev, int vol)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_VOLUME;
	audio_parms.param1 = vol;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return 0;
}
static AM_ErrorCode_t adec_set_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_MUTE;
	audio_parms.param1 = mute;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return 0;
}
static AM_ErrorCode_t adec_set_output_mode_cb(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_OUTPUT_MODE;
	audio_parms.param1 = mode;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return 0;
}
static AM_ErrorCode_t adec_set_pre_gain_cb(AM_AOUT_Device_t *dev, float gain)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_PRE_GAIN;
	audio_parms.param1 = gain*100;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return 0;
}
static AM_ErrorCode_t adec_set_pre_mute_cb(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_SET_PRE_MUTE;
	audio_parms.param1 = mute;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	return 0;
}
static void adec_get_status_cb(void *adec, AM_AV_AudioStatus_t *para)
{
	AudioParms audio_parms;

	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	if (!adec) {
		AM_DEBUG(1, "audio closed, ignore");
		return;
	}

	memset(&audio_parms,0,sizeof(AudioParms));
	audio_parms.cmd = ADEC_GET_STATUS;

	if (s_audio_cb) {
	    s_audio_cb(AM_AV_EVT_AUDIO_CB,&audio_parms,pUserData);
	}
	//TBD for get audio para
}
#ifdef USE_ADEC_IN_DVB
static void adec_start_decode(int fd, int fmt, int has_video, void **padec)
{
	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

#if !defined(ADEC_API_NEW)
		adec_cmd("start");
#else
		if (padec) {
			arm_audio_info param;
			memset(&param, 0, sizeof(param));
			param.handle = fd;
			param.format = fmt;
			param.has_video = has_video;
			param.associate_dec_supported = _get_asso_enable();
			param.mixing_level = _get_asso_mix();
#ifndef ANDROID
			param.use_hardabuf = 1; //linux use hardabuf
#endif
			audio_decode_init(padec, &param);
			audio_set_av_sync_threshold(*padec, AV_SYNC_THRESHOLD);
			audio_decode_set_volume(*padec, 1.);
			adec_handle = *padec;
			AM_AOUT_SetDriver(AOUT_DEV_NO, &adec_aout_drv, *padec);
		}
#endif
}
static void adec_pause_decode(void *adec)
{
	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);
	audio_decode_pause(adec);
}
static void adec_resume_decode(void *adec)
{
	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);
	audio_decode_resume(adec);
}

static void adec_stop_decode(void **padec)
{
	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

#if !defined(ADEC_API_NEW)
		adec_cmd("stop");
#else
		if (padec && *padec) {
			AM_AOUT_SetDriver(AOUT_DEV_NO, NULL, NULL);
			audio_decode_stop(*padec);
			audio_decode_release(padec);
			*padec = NULL;
			adec_handle = *padec;
		}
#endif
}

static void adec_set_decode_ad(int enable, int pid, int fmt, void *adec)
{
#if !defined(ADEC_API_NEW)
#else
	UNUSED(pid);
	UNUSED(fmt);

	if (adec)
		audio_set_associate_enable(adec, enable);
#endif
}

/*音频控制（通过解码器）操作*/
static AM_ErrorCode_t adec_cmd(const char *cmd)
{
#if !defined(ADEC_API_NEW)
	AM_ErrorCode_t ret;
	char buf[32];
	int fd;

	ret = AM_LocalConnect("/tmp/amadec_socket", &fd);
	if (ret != AM_SUCCESS)
		return ret;

	ret = AM_LocalSendCmd(fd, cmd);

	if (ret == AM_SUCCESS)
	{
		ret = AM_LocalGetResp(fd, buf, sizeof(buf));
	}

	close(fd);

	return ret;
#else
	UNUSED(cmd);
	return 0;
#endif
}
#endif
static AM_ErrorCode_t adec_aout_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	UNUSED(dev);
	UNUSED(para);
	return AM_SUCCESS;
}
static AM_ErrorCode_t adec_aout_set_volume(AM_AOUT_Device_t *dev, int vol)
{
	return audio_ops->adec_set_volume(dev,vol);
}
#ifdef USE_ADEC_IN_DVB
static AM_ErrorCode_t adec_set_volume(AM_AOUT_Device_t *dev, int vol)
{
#ifndef ADEC_API_NEW
	char buf[32];

	snprintf(buf, sizeof(buf), "volset:%d", vol);

	return adec_cmd(buf);
#else
	int ret=0;

	UNUSED(dev);

#ifdef CHIP_8626X
	ret = audio_decode_set_volume(vol);
#else
	ret = audio_decode_set_volume(dev->drv_data, ((float)vol)/100);
#endif
	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}
#endif

static AM_ErrorCode_t adec_aout_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
	return audio_ops->adec_set_mute(dev,mute);
}
#ifdef USE_ADEC_IN_DVB
static AM_ErrorCode_t adec_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#ifndef ADEC_API_NEW
	const char *cmd = mute?"mute":"unmute";

	return adec_cmd(cmd);
#else
	int ret=0;

	UNUSED(dev);
	AM_DEBUG(1, "set_mute %d\n", mute?1:0);
	ret = audio_decode_set_mute(dev->drv_data, mute?1:0);

	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}
#endif

static AM_ErrorCode_t adec_aout_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
	return audio_ops->adec_set_output_mode(dev,mode);
}
#ifdef USE_ADEC_IN_DVB
static AM_ErrorCode_t adec_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
#ifndef ADEC_API_NEW


	switch (mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
			cmd = "stereo";
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
			cmd = "leftmono";
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
			cmd = "rightmono";
		break;
		case AM_AOUT_OUTPUT_SWAP:
			cmd = "swap";
		break;
	}

	return adec_cmd(cmd);
#else
	int ret=0;

	UNUSED(dev);

	switch(mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
			ret=audio_channel_stereo(dev->drv_data);
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
			ret=audio_channel_left_mono(dev->drv_data);
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
			ret=audio_channel_right_mono(dev->drv_data);
		break;
		case AM_AOUT_OUTPUT_SWAP:
			ret=audio_channels_swap(dev->drv_data);
		break;
	}

	if(ret==-1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}
#endif

static AM_ErrorCode_t adec_aout_close(AM_AOUT_Device_t *dev)
{
	UNUSED(dev);
	return AM_SUCCESS;
}


static AM_ErrorCode_t adec_aout_set_pre_gain(AM_AOUT_Device_t *dev, float gain)
{
	return audio_ops->adec_set_pre_gain(dev,gain);
}

#ifdef USE_ADEC_IN_DVB
static AM_ErrorCode_t adec_set_pre_gain(AM_AOUT_Device_t *dev, float gain)
{
#ifndef ADEC_API_NEW
	return AM_FAILURE;
#else
	int ret=0;

	UNUSED(dev);

#ifdef CHIP_8626X
	ret = -1;
#else
	AM_DEBUG(1, "set_pre_gain %f\n", gain);
	ret = audio_decode_set_pre_gain(dev->drv_data, gain);
#endif
	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}
#endif
static AM_ErrorCode_t adec_aout_set_pre_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
	return audio_ops->adec_set_pre_mute(dev,mute);
}
#ifdef USE_ADEC_IN_DVB
static AM_ErrorCode_t adec_set_pre_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#ifndef ADEC_API_NEW
	return AM_FAILURE;
#else
	int ret=0;

	UNUSED(dev);

#ifdef CHIP_8626X
	ret = -1;
#else
	AM_DEBUG(1, "set_pre_mute %d\n", mute?1:0);
	ret = audio_decode_set_pre_mute(dev->drv_data, mute?1:0);
#endif
	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}

static void adec_get_status(void *adec, AM_AV_AudioStatus_t *para)
{
	int rc;
	struct adec_status armadec;

	para->lfepresent = -1;
	para->aud_fmt_orig = -1;
	para->resolution_orig = -1;
	para->channels_orig = -1;
	para->sample_rate_orig = -1;
	para->lfepresent_orig = -1;
	rc = audio_decpara_get(adec, &para->sample_rate_orig, &para->channels_orig, &para->lfepresent_orig);
	if (rc == -1)
	{
		AM_DEBUG(1, "cannot get decpara");
		return ;
	}

	rc = get_decoder_status(adec, &armadec);
	if (rc == -1)
	{
		AM_DEBUG(1, "cannot get status in this mode");
		return ;
	}

	para->channels    = armadec.channels;
	para->sample_rate = armadec.sample_rate;
	switch (armadec.resolution)
	{
		case 0:
			para->resolution  = 8;
			break;
		case 1:
			para->resolution  = 16;
			break;
		case 2:
			para->resolution  = 32;
			break;
		case 3:
			para->resolution  = 32;
			break;
		case 4:
			para->resolution  = 64;
			break;
		default:
			para->resolution  = 16;
			break;
	}

}
#endif
#if 0
/*音频控制（通过amplayer2）操作*/
static AM_ErrorCode_t amp_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	UNUSED(dev);
	UNUSED(para);
	return AM_SUCCESS;
}
static AM_ErrorCode_t amp_set_volume(AM_AOUT_Device_t *dev, int vol)
{
#ifdef PLAYER_API_NEW
	int media_id = (long)dev->drv_data;

	if (audio_set_volume(media_id, vol) == -1)
	{
		AM_DEBUG(1, "audio_set_volume failed");
		return AM_AV_ERR_SYS;
	}
#endif
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#ifdef PLAYER_API_NEW
	int media_id = (long)dev->drv_data;

	AM_DEBUG(1, "audio_set_mute %d\n", mute);
	if (audio_set_mute(media_id, mute?0:1) == -1)
	{
		AM_DEBUG(1, "audio_set_mute failed");
		return AM_AV_ERR_SYS;
	}
#endif
	return AM_SUCCESS;
}

#if !defined(AMLOGIC_LIBPLAYER)
static int audio_hardware_ctrl( AM_AOUT_OutputMode_t mode)
{
    int fd;

    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        AM_DEBUG(1,"Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (mode) {
    case AM_AOUT_OUTPUT_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case AM_AOUT_OUTPUT_DUAL_LEFT:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case AM_AOUT_OUTPUT_DUAL_RIGHT:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case AM_AOUT_OUTPUT_STEREO:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        AM_DEBUG(1,"Unknow mode %d!", mode);
        break;

    };

    close(fd);

    return 0;
}
#endif

static AM_ErrorCode_t amp_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
#ifdef PLAYER_API_NEW
	int media_id = (long)dev->drv_data;
	int ret=0;

	switch (mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
#ifdef AMLOGIC_LIBPLAYER
                	ret = audio_stereo(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
#ifdef AMLOGIC_LIBPLAYER
			ret = audio_left_mono(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
#ifdef AMLOGIC_LIBPLAYER
			ret = audio_right_mono(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
		case AM_AOUT_OUTPUT_SWAP:
#ifdef AMLOGIC_LIBPLAYER
			ret = audio_swap_left_right(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
	}
	if (ret == -1)
	{
		AM_DEBUG(1, "audio_set_mode failed");
		return AM_AV_ERR_SYS;
	}
#endif

	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_close(AM_AOUT_Device_t *dev)
{
	UNUSED(dev);
	return AM_SUCCESS;
}
#endif


void afd_evt_callback(long dev_no, int event_type, void *param, void *user_data)
{
	UNUSED(user_data);

	if (event_type == AM_USERDATA_EVT_AFD) {
		AM_USERDATA_AFD_t *afd = (AM_USERDATA_AFD_t *)param;
		AM_AV_Device_t *dev = (AM_AV_Device_t *)user_data;
		if (memcmp(afd, &dev->afd, sizeof(*afd))) {
			dev->afd = *afd;
			AM_DEBUG(1, "AFD evt: [%02x %01x]", afd->af_flag, afd->af);
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_AFD_CHANGED, afd);
		}
	}
}


static int aml_start_afd(AM_AV_Device_t *dev, int vfmt)
{
	int afd_dev = 0;
	AM_ErrorCode_t err;
	AM_USERDATA_OpenPara_t para;
	int mode;

	memset(&dev->afd, 0, sizeof(dev->afd));

	memset(&para, 0, sizeof(para));
	para.vfmt = vfmt;
	para.cc_default_stop = 1;

	err = AM_USERDATA_Open(afd_dev, &para);
	if (err != AM_SUCCESS) {
		AM_DEBUG(1, "userdata open fail.");
		return err;
	}
	AM_USERDATA_GetMode(afd_dev, &mode);
	AM_USERDATA_SetMode(afd_dev, mode | AM_USERDATA_MODE_AFD);
	AM_EVT_Subscribe(afd_dev, AM_USERDATA_EVT_AFD, afd_evt_callback, dev);
	AM_DEBUG(1, "AFD started.");
	return 0;
}

static int aml_stop_afd(AM_AV_Device_t *dev)
{
	int afd_dev = 0;
	AM_EVT_Unsubscribe(afd_dev, AM_USERDATA_EVT_AFD, afd_evt_callback, dev);
	AM_USERDATA_Close(afd_dev);
	memset(&dev->afd, 0, sizeof(dev->afd));
	AM_DEBUG(1, "AFD stopped.");
	return 0;
}



/**\brief 音视频数据注入线程*/
static void* aml_data_source_thread(void *arg)
{
	AV_DataSource_t *src = (AV_DataSource_t*)arg;
	struct pollfd pfd;
	struct timespec ts;
	int cnt;

	while (src->running)
	{
		pthread_mutex_lock(&src->lock);

		if (src->pos  >= 0)
		{
			pfd.fd     = src->fd;
			pfd.events = POLLOUT;

			if (poll(&pfd, 1, 20) == 1)
			{
				cnt = src->len-src->pos;
				cnt = write(src->fd, src->data+src->pos, cnt);
				if (cnt <= 0)
				{
					AM_DEBUG(1, "write data failed");
				}
				else
				{
					src->pos += cnt;
				}
			}

			if (src->pos == src->len)
			{
				if (src->times > 0)
				{
					src->times--;
					if (src->times)
					{
						src->pos = 0;
						AM_EVT_Signal(src->dev_no, src->is_audio?AM_AV_EVT_AUDIO_ES_END:AM_AV_EVT_VIDEO_ES_END, NULL);
					}
					else
					{
						src->pos = -1;
					}
				}
			}

			AM_TIME_GetTimeSpecTimeout(20, &ts);
			pthread_cond_timedwait(&src->cond, &src->lock, &ts);
		}
		else
		{
			pthread_cond_wait(&src->cond, &src->lock);
		}

		pthread_mutex_unlock(&src->lock);

	}

	return NULL;
}

/**\brief 创建一个音视频数据注入数据*/
static AV_DataSource_t* aml_create_data_source(const char *fname, int dev_no, AM_Bool_t is_audio)
{
	AV_DataSource_t *src;

	src = (AV_DataSource_t*)malloc(sizeof(AV_DataSource_t));
	if (!src)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}

	memset(src, 0, sizeof(AV_DataSource_t));

	src->fd = open(fname, O_RDWR);
	if (src->fd == -1)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", fname);
		free(src);
		return NULL;
	}

	src->dev_no   = dev_no;
	src->is_audio = is_audio;

	pthread_mutex_init(&src->lock, NULL);
	pthread_cond_init(&src->cond, NULL);

	return src;
}

/**\brief 运行数据注入线程*/
static AM_ErrorCode_t aml_start_data_source(AV_DataSource_t *src, const uint8_t *data, int len, int times)
{
	int ret;

	if (!src->running)
	{
		src->running = AM_TRUE;
		src->data    = data;
		src->len     = len;
		src->times   = times;
		src->pos     = 0;

		ret = pthread_create(&src->thread, NULL, aml_data_source_thread, src);
		if (ret)
		{
			AM_DEBUG(1, "create the thread failed");
			src->running = AM_FALSE;
			return AM_AV_ERR_SYS;
		}
	}
	else
	{
		pthread_mutex_lock(&src->lock);

		src->data  = data;
		src->len   = len;
		src->times = times;
		src->pos   = 0;

		pthread_mutex_unlock(&src->lock);
		pthread_cond_signal(&src->cond);
	}

	return AM_SUCCESS;
}

/**\brief 释放数据注入数据*/
static void aml_destroy_data_source(AV_DataSource_t *src)
{
	if (src->running)
	{
		src->running = AM_FALSE;
		pthread_cond_signal(&src->cond);
		pthread_join(src->thread, NULL);
	}

	close(src->fd);
	pthread_mutex_destroy(&src->lock);
	pthread_cond_destroy(&src->cond);
	free(src);
}

#ifdef PLAYER_API_NEW
/**\brief 释放文件播放数据*/
static void aml_destroy_fp(AV_FilePlayerData_t *fp)
{
	/*等待播放器停止*/
	if (fp->media_id >= 0)
	{
		player_exit(fp->media_id);
	}

	pthread_mutex_destroy(&fp->lock);
	pthread_cond_destroy(&fp->cond);

	free(fp);
}

/**\brief 创建文件播放器数据*/
static AV_FilePlayerData_t* aml_create_fp(AM_AV_Device_t *dev)
{
	AV_FilePlayerData_t *fp;

	fp = (AV_FilePlayerData_t*)malloc(sizeof(AV_FilePlayerData_t));
	if (!fp)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}

	if (player_init() < 0)
	{
		AM_DEBUG(1, "player_init failed");
		free(fp);
		return NULL;
	}

	memset(fp, 0, sizeof(AV_FilePlayerData_t));
	pthread_mutex_init(&fp->lock, NULL);
	pthread_cond_init(&fp->cond, NULL);

	fp->av       = dev;
	fp->media_id = -1;

	return fp;
}

static int aml_update_player_info_callback(int pid,player_info_t * info)
{
	UNUSED(pid);
	if (info)
		AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)info);

	return 0;
}
#endif

int aml_set_tsync_enable(int enable)
{
	int fd;
	char  bcmd[16];
	int f_en = _get_tsync_enable_force();

	if (f_en != -1) {
		enable = f_en;
		AM_DEBUG(1, "force tsync enable to %d", enable);
	}

	snprintf(bcmd,sizeof(bcmd),"%d",enable);

	return AM_FileEcho(TSYNC_ENABLE_FILE, bcmd);

	// fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	// if(fd>=0)
	// {
	// 	snprintf(bcmd,sizeof(bcmd),"%d",enable);
	// 	write(fd,bcmd,strlen(bcmd));
	// 	close(fd);
	// 	return 0;
	// }

	// return -1;
}

/**\brief 初始化JPEG解码器*/
static AM_ErrorCode_t aml_init_jpeg(AV_JPEGData_t *jpeg)
{
// 	if (jpeg->dec_fd != -1)
// 	{
// 		close(jpeg->dec_fd);
// 		jpeg->dec_fd = -1;
// 	}

// 	if (jpeg->vbuf_fd != -1)
// 	{
// 		close(jpeg->vbuf_fd);
// 		jpeg->vbuf_fd = -1;
// 	}

// 	jpeg->vbuf_fd = open(STREAM_VBUF_FILE, O_RDWR|O_NONBLOCK);
// 	if (jpeg->vbuf_fd == -1)
// 	{
// 		AM_DEBUG(1, "cannot open amstream_vbuf");
// 		goto error;
// 	}

// 	if (ioctl(jpeg->vbuf_fd, AMSTREAM_IOC_VFORMAT, VFORMAT_JPEG) == -1)
// 	{
// 		AM_DEBUG(1, "set jpeg video format failed (\"%s\")", strerror(errno));
// 		goto error;
// 	}

// 	if (ioctl(jpeg->vbuf_fd, AMSTREAM_IOC_PORT_INIT) == -1)
// 	{
// 		AM_DEBUG(1, "amstream init failed (\"%s\")", strerror(errno));
// 		goto error;
// 	}

// 	return AM_SUCCESS;
// error:
// 	if (jpeg->dec_fd != -1)
// 	{
// 		close(jpeg->dec_fd);
// 		jpeg->dec_fd = -1;
// 	}
// 	if (jpeg->vbuf_fd != -1)
// 	{
// 		close(jpeg->vbuf_fd);
// 		jpeg->vbuf_fd = -1;
// 	}
	return AM_AV_ERR_SYS;
}

/**\brief 创建JPEG解码相关数据*/
static AV_JPEGData_t* aml_create_jpeg_data(void)
{
	// AV_JPEGData_t *jpeg;

	// jpeg = malloc(sizeof(AV_JPEGData_t));
	// if (!jpeg)
	// {
	// 	AM_DEBUG(1, "not enough memory");
	// 	return NULL;
	// }

	// jpeg->vbuf_fd = -1;
	// jpeg->dec_fd  = -1;

	// if (aml_init_jpeg(jpeg) != AM_SUCCESS)
	// {
	// 	free(jpeg);
	// 	return NULL;
	// }

	return NULL;
}

/**\brief 释放JPEG解码相关数据*/
static void aml_destroy_jpeg_data(AV_JPEGData_t *jpeg)
{
	// if (jpeg->dec_fd != -1)
	// 	close(jpeg->dec_fd);
	// if (jpeg->vbuf_fd != -1)
	// 	close(jpeg->vbuf_fd);

	// free(jpeg);
}

/**\brief 创建数据注入相关数据*/
static AV_InjectData_t* aml_create_inject_data(void)
{
	AV_InjectData_t *inj;

	inj = (AV_InjectData_t *)malloc(sizeof(AV_InjectData_t));
	if (!inj)
		return NULL;

	inj->aud_fd = -1;
	inj->vid_fd = -1;
	inj->aud_id = -1;
	inj->vid_id = -1;
	inj->cntl_fd = -1;
	inj->adec = NULL;
	inj->ad = NULL;

	return inj;
}

/**\brief 设置数据注入参赛*/
static AM_ErrorCode_t aml_start_inject(AM_AV_Device_t *dev, AV_InjectData_t *inj, AV_InjectPlayPara_t *inj_para)
{
	int vfd=-1, afd=-1;
	AM_AV_InjectPara_t *para = &inj_para->para;
	AM_Bool_t has_video = VALID_VIDEO(para->vid_id, para->vid_fmt);
	AM_Bool_t has_audio = VALID_AUDIO(para->aud_id, para->aud_fmt);
	char vdec_info[256];
	int double_write_mode = 3;

	if (para->aud_id != dev->alt_apid || para->aud_fmt != dev->alt_afmt) {
		AM_DEBUG(1, "switch to pending audio: A[%d:%d] -> A[%d:%d]",
			para->aud_id, para->aud_fmt, dev->alt_apid, dev->alt_afmt);
		para->aud_id = dev->alt_apid;
		para->aud_fmt = dev->alt_afmt;

		has_audio = VALID_AUDIO(para->aud_id, para->aud_fmt);
	}

	inj->pkg_fmt = para->pkg_fmt;

	switch (para->pkg_fmt)
	{
		case PFORMAT_ES:
			if (has_video)
			{
				vfd = open(STREAM_VBUF_FILE, O_RDWR);
				if (vfd == -1)
				{
					AM_DEBUG(1, "cannot open device \"%s\"", STREAM_VBUF_FILE);
					return AM_AV_ERR_CANNOT_OPEN_FILE;
				}
				inj->vid_fd = vfd;
			}
			if (has_audio)
			{
				afd = open(STREAM_ABUF_FILE, O_RDWR);
				if (afd == -1)
				{
					AM_DEBUG(1, "cannot open device \"%s\"", STREAM_ABUF_FILE);
					return AM_AV_ERR_CANNOT_OPEN_FILE;
				}
				inj->aud_fd = afd;
			}
		break;
		case PFORMAT_PS:
			vfd = open(STREAM_PS_FILE, O_RDWR);
			if (vfd == -1)
			{
				AM_DEBUG(1, "cannot open device \"%s\"", STREAM_PS_FILE);
				return AM_AV_ERR_CANNOT_OPEN_FILE;
			}
			inj->vid_fd = afd = vfd;
		break;
		case PFORMAT_TS:
			if (check_vfmt_support_sched(para->vid_fmt) == AM_FALSE)
			{
				vfd = open(STREAM_TS_FILE, O_RDWR);
				if (vfd == -1)
				{
					AM_DEBUG(1, "cannot open device \"%s\"", STREAM_TS_FILE);
					return AM_AV_ERR_CANNOT_OPEN_FILE;
				}
			}
			else
			{
				vfd = open(STREAM_TS_SCHED_FILE, O_RDWR);
				if (vfd == -1)
				{
					AM_DEBUG(1, "cannot open device \"%s\"", STREAM_TS_SCHED_FILE);
					return AM_AV_ERR_CANNOT_OPEN_FILE;
				}
			}
			if (inj_para->drm_mode == AM_AV_DRM_WITH_SECURE_INPUT_BUFFER)
			{
				if (ioctl(vfd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)inj_para->drm_mode) == -1)
				{
					AM_DEBUG(1, "set drm_mode with secure buffer failed\n");
					return AM_AV_ERR_SYS;
				}
			}
			if (inj_para->drm_mode == AM_AV_DRM_WITH_NORMAL_INPUT_BUFFER)
			{
				if (ioctl(vfd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)inj_para->drm_mode) == -1)
				{
					AM_DEBUG(1, "set drm_mode with normal buffer failed\n");
					return AM_AV_ERR_SYS;
				}
			}
			inj->vid_fd = afd = vfd;
		break;
		case PFORMAT_REAL:
			vfd = open(STREAM_RM_FILE, O_RDWR);
			if (vfd == -1)
			{
				AM_DEBUG(1, "cannot open device \"%s\"", STREAM_RM_FILE);
				return AM_AV_ERR_CANNOT_OPEN_FILE;
			}
			inj->vid_fd = afd = vfd;
		break;
		default:
			AM_DEBUG(1, "unknown package format %d", para->pkg_fmt);
		return AM_AV_ERR_NOT_SUPPORTED;
	}

	if (has_video)
	{
		dec_sysinfo_t am_sysinfo;

		if (ioctl(vfd, AMSTREAM_IOC_VFORMAT, para->vid_fmt) == -1)
		{
			AM_DEBUG(1, "set video format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(vfd, AMSTREAM_IOC_VID, para->vid_id) == -1)
		{
			AM_DEBUG(1, "set video PID failed");
			return AM_AV_ERR_SYS;
		}

		memset(&am_sysinfo,0,sizeof(dec_sysinfo_t));
		if (para->vid_fmt == VFORMAT_VC1)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (para->vid_fmt == VFORMAT_H264)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (para->vid_fmt == VFORMAT_AVS)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_AVS;
			/*am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;*/
		}
		else if (para->vid_fmt == VFORMAT_HEVC)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			am_sysinfo.width  = 3840;
			am_sysinfo.height = 2160;
		}

		if (ioctl(vfd, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo) == -1)
		{
			AM_DEBUG(1, "set AMSTREAM_IOC_SYSINFO");
			return AM_AV_ERR_SYS;
		}
		/*configure double wirte mode*/
		memset(vdec_info, 0, sizeof(vdec_info));
		if (para->vid_fmt == VFORMAT_HEVC) {
			sprintf(vdec_info, "hevc_double_write_mode:%d", double_write_mode);
			configure_vdec_info(vfd, vdec_info, strlen(vdec_info));
		} else if (para->vid_fmt == VFORMAT_VP9) {
			sprintf(vdec_info, "vp9_double_write_mode:%d", double_write_mode);
			configure_vdec_info(vfd, vdec_info, strlen(vdec_info));
		}

	}

	if (has_audio)
	{
		if (ioctl(afd, AMSTREAM_IOC_AFORMAT, para->aud_fmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(afd, AMSTREAM_IOC_AID, para->aud_id) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
		if ((para->aud_fmt == AFORMAT_PCM_S16LE) || (para->aud_fmt == AFORMAT_PCM_S16BE) ||
				(para->aud_fmt == AFORMAT_PCM_U8)) {
			ioctl(afd, AMSTREAM_IOC_ACHANNEL, para->channel);
			ioctl(afd, AMSTREAM_IOC_SAMPLERATE, para->sample_rate);
			ioctl(afd, AMSTREAM_IOC_DATAWIDTH, para->data_width);
		}
	}

	if (para->sub_id && (para->sub_id<0x1fff))
	{
		if (ioctl(vfd, AMSTREAM_IOC_SID, para->sub_id) == -1)
		{
			AM_DEBUG(1, "set subtitle PID failed");
			return AM_AV_ERR_SYS;
		}

		if (ioctl(vfd, AMSTREAM_IOC_SUB_TYPE, para->sub_type) == -1)
		{
			AM_DEBUG(1, "set subtitle type failed");
			return AM_AV_ERR_SYS;
		}
	}

	if (vfd != -1)
	{
		if (ioctl(vfd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
		{
			AM_DEBUG(1, "amport init failed");
			return AM_AV_ERR_SYS;
		}

		inj->cntl_fd = open(AMVIDEO_FILE, O_RDWR);
		if (inj->cntl_fd == -1)
		{
			AM_DEBUG(1, "cannot open \"%s\"", AMVIDEO_FILE);
			return AM_AV_ERR_CANNOT_OPEN_FILE;
		}
		ioctl(inj->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
	}

	if ((afd != -1) && (afd != vfd))
	{
		if (ioctl(afd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
		{
			AM_DEBUG(1, "amport init failed");
			return AM_AV_ERR_SYS;
		}
	}

	if ((para->pkg_fmt == PFORMAT_ES) && ((afd != -1) || (vfd != -1))) {
		if ((vfd == -1) && (afd != -1)) {
			if (ioctl(afd, AMSTREAM_IOC_TSTAMP, 0) == -1)
				AM_DEBUG(1, "checkin first apts failed [%s]", strerror(errno));
		}
	}

#if defined(ANDROID) || defined(CHIP_8626X)
	if (has_video && has_audio)
		aml_set_tsync_enable(1);
	else
		aml_set_tsync_enable(0);
#endif

	aml_set_sync_mode(dev,
		aml_calc_sync_mode(dev, has_audio, has_video, 0, para->aud_fmt, NULL));


	if (has_audio)
	{
		audio_ops->adec_start_decode(afd, para->aud_fmt, has_video, &inj->adec);
	}

	inj->aud_fmt  = para->aud_fmt;
	inj->vid_fmt  = para->vid_fmt;
	inj->sub_type = para->sub_type;
	inj->aud_id   = para->aud_id;
	inj->vid_id   = para->vid_id;
	inj->sub_id   = para->sub_id;

	return AM_SUCCESS;
}

/**\brief 释放数据注入相关数据*/
static void aml_destroy_inject_data(AV_InjectData_t *inj)
{
	if (inj->aud_id != -1) {
		aml_set_ad_source(&inj->ad, 0, 0, 0, inj->adec);
		audio_ops->adec_set_decode_ad(0, 0, 0, inj->adec);
		audio_ops->adec_stop_decode(&inj->adec);
	}
	if (inj->aud_fd != -1)
		close(inj->aud_fd);
	if (inj->vid_fd != -1)
		close(inj->vid_fd);
	if (inj->cntl_fd != -1) {
		ioctl(inj->cntl_fd, AMSTREAM_IOC_VPAUSE, 0);
		close(inj->cntl_fd);
	}
	free(inj);
}
void aml_term_signal_handler(int signo)
{
	AM_DEBUG(1,"PVR_DEBUG recive a signal:%d", signo);
	if (signo == SIGTERM)
	{
		AM_DEBUG(1,"PVR_DEBUG recive SIGTERM");
		if (m_tshift == NULL)
			return;
		AM_TFile_t tfile = m_tshift->file;
		struct stat tfile_stat;

		if (tfile) {
			if (tfile->opened == 1)
			{
				AM_DEBUG(1, "PVR_DEBUG in timeshift mode, fd:%d ,fname:%s", tfile->sub_files->rfd, tfile->name);
			} else {
				AM_DEBUG(1, "PVR_DEBUG timeshift file was not open");
			}
		}
		else
		{
			AM_DEBUG(1, "PVR_DEBUG tfile is NULL");
		}
	}
}

static char *cmd2string(int cmd)
{
	char buf[64];
	char *string_cmd[] = {
	"play_start",        /*AV_PLAY_START*/
	"play_pause",        /*AV_PLAY_PAUSE*/
	"play_resume",       /*AV_PLAY_RESUME*/
	"play_ff",           /*AV_PLAY_FF*/
	"play_fb",           /*AV_PLAY_FB*/
	"play_seek",         /*AV_PLAY_SEEK*/
	"play_reset_vpath",  /*AV_PLAY_RESET_VPATH*/
	"play_switch_audio"  /*AV_PLAY_SWITCH_AUDIO*/
	};
	if (cmd < 0 || cmd > AV_PLAY_SWITCH_AUDIO) {
		sprintf(buf, "invalid cmd[%d]", cmd);
		return buf;
	}
	return string_cmd[cmd];
}


QUEUE_DEFINITION(timeshift_cmd_q, AV_PlayCmdPara_t);

static void timeshift_cmd_init(AV_PlayCmdPara_t *cmd, int c)
{
	cmd->cmd = c;
	cmd->done = 0;
}
static int timeshift_cmd_diff(AV_PlayCmdPara_t *cmd1, AV_PlayCmdPara_t *cmd2)
{
	if (cmd1->cmd != cmd2->cmd)
		return 1;
	else if ((cmd1->cmd == AV_PLAY_FF) || (cmd1->cmd == AV_PLAY_FB))
		return (cmd1->speed.speed != cmd2->speed.speed) ? 1 : 0;
	else if (cmd1->cmd == AV_PLAY_SEEK)
		return (cmd1->seek.seek_pos != cmd2->seek.seek_pos) ? 1 : 0;
	return 0;
}
static int timeshift_cmd_put(AV_TimeshiftData_t *tshift, AV_PlayCmdPara_t *cmd)
{
	AV_PlayCmdPara_t last;
	enum enqueue_result e_ret;
	enum dequeue_result d_ret = timeshift_cmd_q_dequeue(&tshift->cmd_q, &last);

	if (d_ret == DEQUEUE_RESULT_EMPTY)
		e_ret = timeshift_cmd_q_enqueue(&tshift->cmd_q, cmd);
	else if (last.cmd == cmd->cmd) {
		e_ret = timeshift_cmd_q_enqueue(&tshift->cmd_q, cmd);
	} else {
		e_ret = timeshift_cmd_q_enqueue(&tshift->cmd_q, &last);
		e_ret |= timeshift_cmd_q_enqueue(&tshift->cmd_q, cmd);
	}
	if (e_ret != ENQUEUE_RESULT_SUCCESS) {
		AM_DEBUG(1, "[timeshift] warning: cmd lost, max:%d", TIMESHIFT_CMD_Q_SIZE);
		return -1;
	}
	return 0;
}
static int timeshift_cmd_get(AV_TimeshiftData_t *tshift, AV_PlayCmdPara_t *cmd)
{
	if (timeshift_cmd_q_is_empty(&tshift->cmd_q))
		return -1;
	if (timeshift_cmd_q_dequeue(&tshift->cmd_q, cmd) != DEQUEUE_RESULT_SUCCESS)
		return -1;
	return 0;
}
static int timeshift_cmd_more(AV_TimeshiftData_t *tshift)
{
	return !timeshift_cmd_q_is_empty(&tshift->cmd_q);
}

/**\brief 创建Timeshift相关数据*/
static AV_TimeshiftData_t* aml_create_timeshift_data(void)
{
	AV_TimeshiftData_t *tshift;

	tshift = (AV_TimeshiftData_t *)malloc(sizeof(AV_TimeshiftData_t));
	if (!tshift)
		return NULL;

	memset(tshift, 0, sizeof(AV_TimeshiftData_t));
	tshift->ts.fd = -1;
	tshift->ts.vid_fd = -1;
	timeshift_cmd_q_init(&tshift->cmd_q);
#ifdef SUPPORT_CAS
	tshift->buf = malloc(SECURE_BLOCK_SIZE);
	if (!tshift->buf) {
		AM_DEBUG(0, "alloc timeshihft buffer failed");
		free(tshift);
		return NULL;
	}
#endif

	return tshift;
}

static int aml_timeshift_inject(AV_TimeshiftData_t *tshift, uint8_t *data, int size, int timeout)
{
	int ret;
	int real_written = 0;
	int fd = tshift->ts.fd;

	if (timeout >= 0)
	{
		struct pollfd pfd;

		pfd.fd = fd;
		pfd.events = POLLOUT;

		ret = poll(&pfd, 1, timeout);
		if (ret != 1)
		{
			AM_DEBUG(1, "timeshift poll timeout");
			goto inject_end;
		}
	}

	if (size && fd > 0)
	{
		ret = write(fd, data, size);
		if ((ret == -1) && (errno != EAGAIN))
		{
			AM_DEBUG(1, "inject data failed errno:%d msg:%s", errno, strerror(errno));
			goto inject_end;
		}
		else if ((ret == -1) && (errno == EAGAIN))
		{
			AM_DEBUG(1, "ret=%d,inject data failed errno:%d msg:%s",ret, errno, strerror(errno));
			real_written = 0;
		}
		else if (ret >= 0)
		{
			real_written = ret;
		}
	}

inject_end:
	return real_written;
}

static int aml_timeshift_get_trick_stat(AV_TimeshiftData_t *tshift)
{
	int state;

	if (tshift->ts.vid_fd == -1)
		return -1;

	ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICK_STAT, (unsigned long)&state);

	return state;
}

void tfile_evt_callback ( long dev_no, int event_type, void *param, void *user_data )
{
	if (event_type == AM_TFILE_EVT_RATE_CHANGED) {
		AM_DEBUG(1, "tfile evt: rate[%dBps]", (int)param);
		AV_TimeshiftData_t *tshift = (AV_TimeshiftData_t *)(((AM_AV_Device_t*)user_data)->timeshift_player.drv_data);
		tshift->rate = (int)param;
		pthread_cond_signal(&tshift->cond);
	}
}

static int aml_av_get_delay(int fd, int video_not_audio)
{
	int d = 0;

	if (ioctl(fd, (video_not_audio)?
			AMSTREAM_IOC_GET_VIDEO_CUR_DELAY_MS:
			AMSTREAM_IOC_GET_AUDIO_CUR_DELAY_MS,
			&d) != -1)
		return ((d > (30 * 1000)) || (d < (1 * 1000))) ? 0 : d;

	return 0;
}

static int aml_timeshift_get_delay(AV_TimeshiftData_t *tshift)
{
	AM_Bool_t has_video = VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt);

	return aml_av_get_delay(tshift->ts.fd, has_video);
}

static void aml_timeshift_get_current(AV_TimeshiftData_t *tshift, int *current, int *start, int *end)
{
	int timeshift = (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_TIMESHIFTING)? 1 : 0;

	if (current)
		*current = (timeshift)?
			AM_TFile_TimeGetReadNow(tshift->file)
			: (tshift->rate ? AM_TFile_Tell(tshift->file) * 1000 / tshift->rate : 0);
	if (start)
		*start = (timeshift)? AM_TFile_TimeGetStart(tshift->file) : 0;
	if (end)
		*end = (timeshift)? AM_TFile_TimeGetEnd(tshift->file) : tshift->duration;
}

/*track current status internally*/
static void aml_timeshift_update_current(AV_TimeshiftData_t *tshift)
{
	aml_timeshift_get_current(tshift, &tshift->current, &tshift->start, &tshift->end);
	AM_DEBUG(5, "[timeshift] update: state[%d] start[%d] current[%d] end[%d]", tshift->state, tshift->start, tshift->current, tshift->end);
}

/*update from internal status*/
static void aml_timeshift_update(AV_TimeshiftData_t *tshift, AM_AV_TimeshiftInfo_t *info)
{
	info->current_time = tshift->current;
	info->full_time = tshift->end;
	info->status = tshift->state;
}

/*notification current status*/
static void aml_timeshift_notify(AV_TimeshiftData_t *tshift, AM_AV_TimeshiftInfo_t *info, AM_Bool_t calc_delay)
{
	AM_AV_TimeshiftInfo_t inf;
	AM_Bool_t has_video = VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt);
	int delay = 0;

	if (!info) {
		info = &inf;
		aml_timeshift_update(tshift, info);
	}

	delay = aml_timeshift_get_delay(tshift);

	if (calc_delay)
		info->current_time -= delay;

	if (info->current_time < 0)
		info->current_time = 10;

	AM_DEBUG(1, "[timeshift] notify: status[%d] current[%d] full[%d] -delay[%d]",
		info->status, info->current_time, info->full_time, delay);

	AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)info);
	aml_timeshift_update_info(tshift, info);
}

/**\brief 设置Timeshift参数*/
static AM_ErrorCode_t aml_start_timeshift(AV_TimeshiftData_t *tshift, AV_TimeShiftPlayPara_t *tshift_para, AM_Bool_t create_thread, AM_Bool_t start_audio)
{
	char buf[64];
	AM_AV_TimeshiftPara_t *para = &tshift_para->para;
	AV_TSData_t *ts = &tshift->ts;
	AV_TSPlayPara_t *tp = &tshift->tp;
	AM_AV_Device_t *dev = tshift->dev;
	int val;
	int sync_mode, sync_force;
	char vdec_info[256];
	int double_write_mode = 3;

	AM_Bool_t has_video = VALID_VIDEO(tp->vpid, tp->vfmt);
	AM_Bool_t has_audio = VALID_AUDIO(tp->apid, tp->afmt);

	if (tp->apid != dev->alt_apid || tp->afmt != dev->alt_afmt) {
		AM_DEBUG(1, "switch to pending audio: A[%d:%d] -> A[%d:%d]",
			tp->apid, tp->afmt, dev->alt_apid, dev->alt_afmt);
		tp->apid = dev->alt_apid;
		tp->afmt = dev->alt_afmt;

		has_audio = VALID_AUDIO(tp->apid, tp->afmt);
	}

	AM_DEBUG(1, "aml start timeshift: V[%d:%d] A[%d:%d]", tp->vpid, tp->vfmt, tp->apid, tp->afmt);

	if (create_thread)//check sth critical 1st.
	{
		AM_Bool_t loop = AM_TRUE;
		if (para->mode == AM_AV_TIMESHIFT_MODE_PLAYBACK)
			loop = AM_FALSE;
		if (tshift_para->para.tfile && tshift_para->para.tfile->loop != loop) {
			AM_DEBUG(1, "FATAL: [timeshift]: playmode[%d] != filemode[%d]",
				para->mode,
				tshift_para->para.tfile->is_timeshift? AM_AV_TIMESHIFT_MODE_TIMESHIFTING : AM_AV_TIMESHIFT_MODE_PLAYBACK);
			AM_DEBUG(1, "[timeshift] tmode:%d fmode:%d tdur:%d floop:%d",
				para->mode, tshift_para->para.tfile->is_timeshift, para->media_info.duration, tshift_para->para.tfile->loop);
			return AM_AV_ERR_ILLEGAL_OP;
		}
	}

	/*patch dec control*/
	set_dec_control(has_video);

	AM_DEBUG(1, "Openning demux%d",para->dmx_id);
	snprintf(buf, sizeof(buf), "/dev/dvb0.demux%d", para->dmx_id);
	tshift->dmxfd = open(buf, O_RDWR);
	if (tshift->dmxfd == -1)
	{
		AM_DEBUG(1, "cannot open \"/dev/dvb0.demux%d\" error:%d \"%s\"", para->dmx_id, errno, strerror(errno));
		return AM_AV_ERR_CANNOT_OPEN_DEV;
	}

	AM_FileRead(DVB_STB_SOURCE_FILE, tshift->last_stb_src, 16);
	snprintf(buf, sizeof(buf), "dmx%d", para->dmx_id);
	AM_FileEcho(DVB_STB_SOURCE_FILE, buf);

	snprintf(buf, sizeof(buf), DVB_STB_DEMUXSOURCE_FILE, para->dmx_id);
	AM_FileRead(buf, tshift->last_dmx_src, 16);
	AM_FileEcho(buf, "hiu");
#ifdef SUPPORT_CAS
        if (para->secure_enable)
                snprintf(buf, sizeof(buf), "%d", 256*1024);
        else
#endif
	snprintf(buf, sizeof(buf), "%d", 32*1024);
	AM_FileEcho(DVB_STB_ASYNCFIFO_FLUSHSIZE_FILE, buf);

	if (check_vfmt_support_sched(tp->vfmt) == AM_FALSE)
	{
		AM_DEBUG(1, "Openning mpts");
		ts->fd = open(STREAM_TS_FILE, O_RDWR);
		if (ts->fd == -1)
		{
			AM_DEBUG(1, "cannot open device \"%s\"", STREAM_TS_FILE);
			return AM_AV_ERR_CANNOT_OPEN_FILE;
		}
	}
	else
	{
		AM_DEBUG(1, "Openning mpts_sched");
		ts->fd = open(STREAM_TS_SCHED_FILE, O_RDWR);
		if (ts->fd == -1)
		{
			AM_DEBUG(1, "cannot open device \"%s\"", STREAM_TS_SCHED_FILE);
			return AM_AV_ERR_CANNOT_OPEN_FILE;
		}
	}

	if (has_video) {
		AM_DEBUG(1, "Openning video");
		ts->vid_fd = open(AMVIDEO_FILE, O_RDWR);
		if (ts->vid_fd == -1)
		{
			AM_DEBUG(1, "cannot create data source \"/dev/amvideo\"");
			return AM_AV_ERR_CANNOT_OPEN_DEV;
		}
	}
#ifdef USE_AMSTREAM_SUB_BUFFER
	AM_DEBUG(1, "try to init subtitle ring buf");
	int sid = 0xffff;
	if (ioctl(ts->fd, AMSTREAM_IOC_SID, sid) == -1)
	{
		AM_DEBUG(1, "set sub PID failed");
	}
#endif
#if defined(ANDROID) || defined(CHIP_8626X)
	/*Set tsync enable/disable*/
	if (has_video && (has_audio && start_audio))
	{
		AM_DEBUG(1, "Set tsync enable to 1");
		aml_set_tsync_enable(1);
	}
	else
	{
		AM_DEBUG(1, "Set tsync enable to 0");
		aml_set_tsync_enable(0);
	}
#endif

	AM_DEBUG(1, "Setting play param");
	if (has_video) {
		val = tp->vfmt;
		if (ioctl(ts->fd, AMSTREAM_IOC_VFORMAT, val) == -1)
		{
			AM_DEBUG(1, "set video format failed");
			return AM_AV_ERR_SYS;
		}
		val = tp->vpid;
		if (ioctl(ts->fd, AMSTREAM_IOC_VID, val) == -1)
		{
			AM_DEBUG(1, "set video PID failed");
			return AM_AV_ERR_SYS;
		}
		if (dev->afd_enable) {
			AM_DEBUG(1, "start afd...");
			aml_start_afd(dev, tp->vfmt);
		}
	}

	/*if ((tp->vfmt == VFORMAT_H264) || (tp->vfmt == VFORMAT_VC1))*/
	if (has_video) {
		memset(&am_sysinfo,0,sizeof(dec_sysinfo_t));
		if (tp->vfmt == VFORMAT_VC1)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (tp->vfmt == VFORMAT_H264)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (tp->vfmt == VFORMAT_AVS)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_AVS;
			/*am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;*/
		}
		else if (tp->vfmt == VFORMAT_HEVC)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			am_sysinfo.width  = 3840;
			am_sysinfo.height = 2160;
		}

		if (ioctl(ts->fd, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo) == -1)
		{
			AM_DEBUG(1, "set AMSTREAM_IOC_SYSINFO");
			return AM_AV_ERR_SYS;
		}
		/*configure double write mode*/
		memset(vdec_info, 0, sizeof(vdec_info));
		if (tp->vfmt == VFORMAT_HEVC) {
			sprintf(vdec_info, "hevc_double_write_mode:%d", double_write_mode);
			configure_vdec_info(ts->fd, vdec_info, strlen(vdec_info));
		} else if (tp->vfmt == VFORMAT_VP9) {
			sprintf(vdec_info, "vp9_double_write_mode:%d", double_write_mode);
			configure_vdec_info(ts->fd, vdec_info, strlen(vdec_info));
		}
	}

	if (has_audio && start_audio) {
		if ((tp->afmt == AFORMAT_AC3) || (tp->afmt == AFORMAT_DTS)) {
			//set_arc_freq(1);
		}

		val = tp->afmt;
		if (ioctl(ts->fd, AMSTREAM_IOC_AFORMAT, val) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		val = tp->apid;
		if (ioctl(ts->fd, AMSTREAM_IOC_AID, val) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}

	sync_mode = aml_calc_sync_mode(tshift->dev, (has_audio && start_audio), has_video, 0, tp->afmt, &sync_force);
	aml_set_sync_mode(tshift->dev, sync_mode);

	if ((has_audio && start_audio) && (sync_force != FORCE_AC3_AMASTER)) {
		if (!show_first_frame_nosync()) {
#ifdef ANDROID
			//property_set("sys.amplayer.drop_pcm", "1");
#endif
		}
		AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");

		audio_ops->adec_start_decode(ts->fd, tp->afmt, has_video, &ts->adec);

		if (VALID_PID(tp->sub_apid))
			aml_set_audio_ad(tshift->dev, 1, tp->sub_apid, tp->sub_afmt);
	}
	if (ioctl(ts->fd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
	{
		AM_DEBUG(1, "amport init failed");
		return AM_AV_ERR_SYS;
	}
#ifdef SUPPORT_CAS
	if (para->secure_enable) {
		AM_FileEcho("/sys/module/decoder_common/parameters/force_nosecure_even_drm", "1");
		if (ioctl(ts->fd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)(1)) == -1)
		{
			AM_DEBUG(0, "%s, set drm enable failed", __func__);
			return AM_AV_ERR_SYS;
		}
		AM_DEBUG(3, "%s, set drm enable success", __func__);
	} else {
		AM_FileEcho("/sys/module/decoder_common/parameters/force_nosecure_even_drm", "0");
		if (ioctl(ts->fd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)(0)) == -1)
		{
			AM_DEBUG(0, "%s, set drm disable failed", __func__);
			return AM_AV_ERR_SYS;
		}
		AM_DEBUG(3, "%s, set drm disable success", __func__);
	}
#endif
	/*no blank for timeshifting play*/
	AM_FileEcho(VID_BLACKOUT_FILE, "0");

#ifdef SUPPORT_CAS
	AM_EVT_Signal(tshift->dev->dev_no, AM_AV_EVT_PLAYER_STATE_CHANGED, NULL);
#endif

	if (create_thread)
	{
		tshift->state = AV_TIMESHIFT_STAT_STOP;
		tshift->duration = para->media_info.duration * 1000;

		AM_DEBUG(1, "[timeshift] mode %d, duration %d", para->mode, tshift->duration);
		/*check if use tfile external, for compatible*/
		if (!tshift_para->para.tfile) {
			tshift->file_flag |= TIMESHIFT_TFILE_AUTOCREATE;
			AM_TRY(AM_TFile_Open(&tshift->file,
						para->file_path,
						(para->mode == AM_AV_TIMESHIFT_MODE_TIMESHIFTING && tshift->duration > 0),
						tshift->duration,
						0));
#ifdef SUPPORT_CAS
			if (para->secure_enable && !tshift->cas_open) {
				if (AM_SUCCESS == AM_TFile_CasOpen(para->cas_file_path)) {
					AM_TFile_CasDump();
					tshift->cas_open = 1;
				}
			}
#endif
			if (para->mode == AM_AV_TIMESHIFT_MODE_TIMESHIFTING) {
				AM_TFile_TimeStart(tshift->file);
				AM_DEBUG(1, "[timeshift] start TFile timer %p", tshift->file);
			}
		} else {
			tshift->file = tshift_para->para.tfile;
			tshift->file_flag |= TIMESHIFT_TFILE_DETACHED;
			AM_EVT_Subscribe((long)tshift->file, AM_TFILE_EVT_RATE_CHANGED, tfile_evt_callback, tshift->dev);
			AM_DEBUG(1, "[timeshift] using TFile %p", tshift->file);
		}

		if (! tshift->file->loop && tshift->duration)
		{
			tshift->rate = tshift->file->size * 1000 / tshift->duration;
			AM_DEBUG(1, "[timeshift] Playback file size %lld, duration %d ms, the bitrate is %d bps", tshift->file->size, tshift->duration, tshift->rate);
		}
		tshift->running = 1;
		tshift->offset = tshift_para->para.offset_ms;
		tshift->para = *tshift_para;
		pthread_mutex_init(&tshift->lock, NULL);
		pthread_cond_init(&tshift->cond, NULL);
		if (pthread_create(&tshift->thread, NULL, aml_timeshift_thread, (void*)tshift))
		{
			AM_DEBUG(1, "create the timeshift thread failed");
			tshift->running = 0;
		}
		AM_DEBUG(1, "[timeshift] create aml_term_signal_handler");
		signal(SIGTERM, &aml_term_signal_handler);
	}

	return AM_SUCCESS;
}

static int aml_start_av_monitor(AM_AV_Device_t *dev, AV_Monitor_t *mon)
{
	AM_DEBUG(1, "[avmon] start av monitor SwitchSourceTime = %fs",getUptimeSeconds());
	mon->av_thread_running = AM_TRUE;
	if (pthread_create(&mon->av_mon_thread, NULL, aml_av_monitor_thread, (void*)dev))
	{
		AM_DEBUG(1, "[avmon] create the av buf monitor thread failed");
		mon->av_thread_running = AM_FALSE;
		return -1;
	}
	return 0;
}

static void aml_stop_av_monitor(AM_AV_Device_t *dev, AV_Monitor_t *mon)
{
	AM_DEBUG(1, "[avmon] stop av monitor");
	if (mon->av_thread_running) {
		mon->av_thread_running = AM_FALSE;
		pthread_cond_broadcast(&gAVMonCond);
		AM_DEBUG(1, "[avmon] stop av monitor ---broardcast end join start\r\n");
		pthread_join(mon->av_mon_thread, NULL);
		AM_DEBUG(1, "[avmon] stop av monitor ---join end\r\n");
	}
	if (dev)
		dev->audio_switch = AM_FALSE;
}

static void aml_check_audio_state(void)
{
	char buf[32];
	int tsync_audio_state = 0;
	int check_count = 0;
	while (check_count++ < 10) {
		usleep(100 * 1000);
		if (AM_FileRead(TSYNC_AUDIO_STATE_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%x", &tsync_audio_state);
		} else {
			break;
		}
		AM_DEBUG(1, "tsync_audio_state %d", tsync_audio_state);
		if (tsync_audio_state == 7) {
			break;
		} else if (tsync_audio_state == 0) {
			usleep(100 * 1000);
			break;
		}
	}
}



static void aml_stop_timeshift(AV_TimeshiftData_t *tshift, AM_Bool_t destroy_thread)
{
	char buf[64];
	AM_Bool_t has_audio = VALID_AUDIO(tshift->tp.apid, tshift->tp.afmt);

	if (tshift->dev->afd_enable)
		aml_stop_afd(tshift->dev);

	if (tshift->running && destroy_thread)
	{
		tshift->running = 0;
		pthread_cond_broadcast(&tshift->cond);
		pthread_join(tshift->thread, NULL);

		aml_stop_av_monitor(tshift->dev, &tshift->dev->timeshift_player.mon);

		if (!(tshift->file_flag & TIMESHIFT_TFILE_DETACHED))
			AM_TFile_Close(tshift->file);
#ifdef SUPPORT_CAS
		if (tshift->cas_open) {
			AM_TFile_CasClose();
			tshift->cas_open = 0;
		}
#endif
	}

	if (tshift->ts.fd != -1) {
            AM_DEBUG(1, "Stopping Audio decode");
            if (has_audio) {
                aml_set_ad_source(&tshift->ts.ad, 0, 0, 0, tshift->ts.adec);
                audio_ops->adec_set_decode_ad(0, 0, 0, tshift->ts.adec);
                audio_ops->adec_stop_decode(&tshift->ts.adec);
            }
            AM_DEBUG(1, "Closing mpts");
            close(tshift->ts.fd);
            tshift->ts.fd = -1;
	}
	AM_DEBUG(1, "Closing demux 1");
	if (tshift->dmxfd != -1)
		close(tshift->dmxfd);
	AM_DEBUG(1, "Closing video");
	if (tshift->ts.vid_fd != -1)
		close(tshift->ts.vid_fd);

	AM_FileEcho(DVB_STB_SOURCE_FILE, tshift->last_stb_src);
	snprintf(buf, sizeof(buf), DVB_STB_DEMUXSOURCE_FILE, tshift->para.para.dmx_id);
	AM_FileEcho(buf, tshift->last_dmx_src);

	if (AM_FileRead(DI_BYPASS_ALL_FILE, buf, sizeof(buf)) == AM_SUCCESS) {
		if (!strncmp(buf, "1", 1)) {
			AM_FileEcho(DI_BYPASS_ALL_FILE,"0");
		}
	}
}

/**\brief 释放Timeshift相关数据*/
static void aml_destroy_timeshift_data(AV_TimeshiftData_t *tshift)
{
#ifdef SUPPORT_CAS
	if (tshift->buf) {
		free(tshift->buf);
		tshift->buf = NULL;
	}
#endif
	free(tshift);
}

static int am_timeshift_reset(AV_TimeshiftData_t *tshift, int deinterlace_val, AM_Bool_t start_audio)
{
	UNUSED(deinterlace_val);

	AM_DEBUG(1, "am_timeshift_reset");
	aml_stop_timeshift(tshift, AM_FALSE);
	if (start_audio)
		aml_check_audio_state();
	aml_start_timeshift(tshift, &tshift->para, AM_FALSE, start_audio);

	/*Set the left to 0, we will read from the new point*/
	tshift->left = 0;

	return 0;
}

static int am_timeshift_reset_continue(AV_TimeshiftData_t *tshift, int deinterlace_val, AM_Bool_t start_audio)
{

	UNUSED(deinterlace_val);
	AM_DEBUG(1, "am_timeshift_reset_continue reset inject size");
	tshift->inject_size = 0;
	aml_stop_timeshift(tshift, AM_FALSE);

	aml_start_timeshift(tshift, &tshift->para, AM_FALSE, start_audio);
	tshift->inject_size = 64*1024;
	return 0;
}

static int am_timeshift_playback_time_check(AV_TimeshiftData_t *tshift, int time)
{
	if (time < 0)
		return 0;
	else if (time >= (tshift->duration))
		return tshift->duration;
	return time;
}

static int am_timeshift_fffb(AV_TimeshiftData_t *tshift, AV_PlayCmdPara_t *cmd)
{
	int now, ret, next_time;
	int speed = cmd->speed.speed;

	if (1/*speed*/)
	{
		if (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_TIMESHIFTING) {
			next_time = tshift->fffb_current + speed * FFFB_JUMP_STEP;
			AM_TFile_TimeSeek(tshift->file, next_time);
			AM_DEBUG(2, "[timeshift] fffb cur[%d] -> next[%d]", tshift->fffb_current, next_time);
		} else {
			loff_t offset;
			next_time = tshift->fffb_current + speed * FFFB_JUMP_STEP;
			next_time = am_timeshift_playback_time_check(tshift, next_time);
			offset = (loff_t)next_time * tshift->rate / 1000;
#ifdef SUPPORT_CAS
			if (tshift->para.para.secure_enable) {
				offset = ROUNDUP(offset, SECURE_BLOCK_SIZE);
				if (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_PLAYBACK)
					offset += 188*11;//skip pat and meida info header,clear data not encrypt
			}
#endif
			AM_TFile_Seek(tshift->file, offset);
			AM_DEBUG(2, "[timeshift] fffb cur[%d] -> next[%d],offset[%lld]", tshift->fffb_current, next_time, offset);
			tshift->eof = (next_time == tshift->duration) ? 1 : 0;
			if (tshift->eof) {
				AM_DEBUG(1, "[timeshift] fffb eof");
			}
		}
		tshift->fffb_current = next_time;
		AM_TIME_GetClock(&tshift->fffb_start);
		am_timeshift_reset(tshift, 0, AM_FALSE);
	}
	else
	{
		/*speed is 0/1, turn to play*/
		ret = 1;
		AM_DEBUG(1, "[timeshift] speed == [%d]", speed);
	}

	return 0;
}

static int aml_timeshift_pause_av(AV_TimeshiftData_t *tshift)
{
	if (VALID_AUDIO(tshift->tp.apid, tshift->tp.afmt))
	{
#if defined(ADEC_API_NEW)
		audio_ops->adec_pause_decode(tshift->ts.adec);
#else
		//TODO
#endif
	}

	if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
	{
		ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_VPAUSE, 1);
	}

	return 0;
}

static int aml_timeshift_resume_av(AV_TimeshiftData_t *tshift)
{
	if (VALID_AUDIO(tshift->tp.apid, tshift->tp.afmt))
	{
#if defined(ADEC_API_NEW)
		audio_ops->adec_resume_decode(tshift->ts.adec);
#else
		//TODO
#endif
	}
	if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
	{
		ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_VPAUSE, 0);
	}


	return 0;
}

static int aml_timeshift_do_cmd_start(AV_TimeshiftData_t *tshift)
{
	loff_t offset;
	int seeked = 0;

        AV_TimeshiftState_t last_stat =  tshift->state;
	tshift->inject_size = 64*1024;
	tshift->timeout = 0;
	tshift->state = AV_TIMESHIFT_STAT_PLAY;
	AM_DEBUG(1, "[timeshift] [start] seek to time %d ms...", tshift->current);
	if (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_TIMESHIFTING) {
		if (AM_TFile_TimeGetReadNow(tshift->file) != tshift->current) {
			AM_TFile_TimeSeek(tshift->file, tshift->current);
			seeked = 1;
		}
	} else {//rec play
		loff_t off = AM_TFile_Tell(tshift->file);
		int current = off * 1000 / (loff_t)tshift->rate;
		AM_DEBUG(1, "[timeshift] [start] file now[%lld][%dms] rate[%dbps]", off, current, tshift->rate);
		if (current != tshift->current) {
			offset = (loff_t)tshift->current / 1000 * (loff_t)tshift->rate;
			AM_TFile_Seek(tshift->file, offset);
			seeked = 1;
		}
	}
	if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
		ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);

	if (seeked || last_stat == AV_TIMESHIFT_STAT_FFFB)
		am_timeshift_reset(tshift, 2, AM_TRUE);

	//if (tshift->last_cmd == AV_PLAY_FF || tshift->last_cmd == AV_PLAY_FB)
	{
		//usleep(200*1000);
		AM_DEBUG(1, "set di bypass_all to 0");
		AM_FileEcho("/sys/module/di/parameters/bypass_all","0");
	}
	aml_start_av_monitor(tshift->dev, &tshift->dev->timeshift_player.mon);
	return 0;
}

static int aml_timeshift_do_play_cmd(AV_TimeshiftData_t *tshift, AV_PlayCmdPara_t *cmd)
{
	AV_TimeshiftState_t	last_state = tshift->state;
	loff_t offset;
	int state_changed = 0, cmd_changed = 0;

	if ((tshift->para.para.mode != AM_AV_TIMESHIFT_MODE_TIMESHIFTING && !tshift->rate)
		&& (cmd->cmd == AV_PLAY_FF || cmd->cmd == AV_PLAY_FB || cmd->cmd == AV_PLAY_SEEK))
	{
		AM_DEBUG(1, "zzz [timeshift] Have not got the rate, skip this command");
		return -1;
	}

	if ((tshift->last_cmd[0].cmd == AV_PLAY_SEEK && tshift->last_cmd[0].done)
		|| timeshift_cmd_diff(&tshift->last_cmd[0], cmd))
	{
		switch (cmd->cmd)
		{
		case AV_PLAY_START:
			//if (tshift->state != AV_TIMESHIFT_STAT_PLAY) {
				aml_timeshift_do_cmd_start(tshift);
				aml_timeshift_update_current(tshift);
			//}
			break;
		case AV_PLAY_PAUSE:
			//if (tshift->state != AV_TIMESHIFT_STAT_PAUSE)
			{
				AM_DEBUG(1, "[timeshift] [pause]");
				aml_stop_av_monitor(tshift->dev, &tshift->dev->timeshift_player.mon);
				tshift->inject_size = 0;
				tshift->timeout = 1000;
				tshift->state = AV_TIMESHIFT_STAT_PAUSE;
				if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
					ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
				aml_timeshift_pause_av(tshift);
				aml_timeshift_update_current(tshift);
			}
			break;
		case AV_PLAY_RESUME:
			if (tshift->state == AV_TIMESHIFT_STAT_PAUSE)
			{
				if (tshift->pause_time && tshift->current > tshift->pause_time) {
					AM_DEBUG(1, "[timeshift] [resume] [replay]");
					aml_timeshift_do_cmd_start(tshift);
				} else {
					tshift->inject_size = 64*1024;
					tshift->timeout = 0;
					tshift->state = AV_TIMESHIFT_STAT_PLAY;
					if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
						ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
					aml_timeshift_resume_av(tshift);
					aml_start_av_monitor(tshift->dev, &tshift->dev->timeshift_player.mon);
					AM_DEBUG(1, "[timeshift] [resume]");
				}

				aml_timeshift_update_current(tshift);
				tshift->fffb_start = 0;
				tshift->fffb_current = 0;
			}
			break;
		case AV_PLAY_FF:
		case AV_PLAY_FB:
			if (tshift->state != AV_TIMESHIFT_STAT_FFFB)
			{
				if (cmd->speed.speed == 0 && tshift->state == AV_TIMESHIFT_STAT_PLAY)
					return 0;
				aml_stop_av_monitor(tshift->dev, &tshift->dev->timeshift_player.mon);
				tshift->inject_size = 64*1024;
				tshift->timeout = 0;
				tshift->state = AV_TIMESHIFT_STAT_FFFB;
				AM_DEBUG(1, "[timeshift] [fast] speed: %d", cmd->speed.speed);
				if (tshift->last_cmd[0].cmd == AV_PLAY_START)
				{
					AM_DEBUG(1, "set di bypass_all to 1");
					AM_FileEcho(DI_BYPASS_ALL_FILE,"1");
					usleep(200*1000);
				}
				if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
					ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_FFFB);
				/*reset the fffb start*/
				tshift->fffb_current = tshift->current;
				am_timeshift_fffb(tshift, cmd);
				aml_timeshift_update_current(tshift);
			}
			break;
		case AV_PLAY_SEEK:
			aml_stop_av_monitor(tshift->dev, &tshift->dev->timeshift_player.mon);
			if (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_TIMESHIFTING) {
				AM_DEBUG(1, "[timeshift] [seek] offset: time: %dms", cmd->seek.seek_pos);
				AM_TFile_TimeSeek(tshift->file, cmd->seek.seek_pos);
			} else {
				int pos = am_timeshift_playback_time_check(tshift, cmd->seek.seek_pos);
				offset = (loff_t)pos / 1000 * (loff_t)tshift->rate;
#ifdef SUPPORT_CAS
				if (tshift->para.para.secure_enable) {
					offset = ROUNDUP(offset, SECURE_BLOCK_SIZE);
					offset += 188*11;//skip pat and meida info header,clear data not encrypt
				}
#endif
				AM_DEBUG(1, "[timeshift] [seek] offset: time:%dms off:%lld", pos, offset);
				AM_TFile_Seek(tshift->file, offset);
			}
			if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
				ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_FFFB);
			am_timeshift_reset(tshift, 0, AM_TRUE);
			tshift->inject_size = 64*1024;
			tshift->timeout = 0;
			aml_timeshift_update_current(tshift);
			break;
#if 0
		case AV_PLAY_RESET_VPATH:
			if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
				ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
			/*stop play first*/
			aml_stop_timeshift(tshift, AM_FALSE);
			/*reset the vpath*/
			aml_set_vpath(tshift->dev);
			/*restart play now*/
			aml_start_timeshift(tshift, &tshift->para, AM_FALSE, AM_TRUE);

			/*Set the left to 0, we will read from the new point*/
			tshift->left = 0;
			tshift->inject_size = 0;
			tshift->timeout = 0;
			/* will turn to play state from current_time */
			tshift->state = AV_TIMESHIFT_STAT_SEARCHOK;
			break;
#endif
#if 0
		case AV_PLAY_SWITCH_AUDIO:
			/* just restart play using the new audio para */
			AM_DEBUG(1, "[timeshift] [switch audio] pid=%d, fmt=%d",tshift->tp.apid, tshift->tp.afmt);
			/*Set the left to 0, we will read from the new point*/
			tshift->left = 0;
			tshift->inject_size = 0;
			tshift->timeout = 0;
			if (tshift->state == AV_TIMESHIFT_STAT_PAUSE)
			{
				/* keep pause with the new audio */
				if (VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt))
					ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
				am_timeshift_reset(tshift, 2, AM_TRUE);
				tshift->timeout = 1000;
				aml_timeshift_pause_av(tshift);
			}
			else
			{
				/* will turn to play state from current_time */
				tshift->state = AV_TIMESHIFT_STAT_SEARCHOK;
			}
			break;
#endif
		default:
			AM_DEBUG(1, "[timeshift] Unsupported timeshift play command %d", cmd);
			return -1;
			break;
		}
	}

	if (timeshift_cmd_diff(&tshift->last_cmd[0], cmd)) {
		cmd_changed = 1;
		if (tshift->last_cmd[0].cmd != AV_PLAY_SEEK)/*for resume from seek*/
			tshift->last_cmd[1] = tshift->last_cmd[0];
		tshift->last_cmd[0] = *cmd;
	}

	if (tshift->state != last_state)
		state_changed = 1;

	return (cmd_changed || state_changed);
}

int aml_timeshift_do_cmd(AV_TimeshiftData_t *tshift, AV_PlayCmdPara_t *cmd, int notify)
{
	int state_changed;
	AV_PlayCmdPara_t last;

	if ((tshift->last_cmd[0].cmd == AV_PLAY_FF || tshift->last_cmd[0].cmd == AV_PLAY_FB)
		&& (cmd->cmd == AV_PLAY_PAUSE || cmd->cmd == AV_PLAY_RESUME))
	{
		/*
		 * $ID: 101093,Fixed, timeshift resume decoder is stop when status from FastForward(or FastBack) to Pause directly
		 */
		AM_DEBUG(1, "[timeshift] last cmd is %s, cur_cmd is %s, do AV_PLAY_START first",
			cmd2string(tshift->last_cmd[0].cmd),
			cmd2string(cmd->cmd));
		{
			AV_PlayCmdPara_t c;
			timeshift_cmd_init(&c, AV_PLAY_START);
			aml_timeshift_do_play_cmd(tshift, &c);
		}
	}

	last = tshift->last_cmd[0];
	state_changed = aml_timeshift_do_play_cmd(tshift, cmd);

	AM_DEBUG(5, ">>>done cmd:%d, state_changed:%d, do_notify:%d, last:%d", cmd->cmd, state_changed, notify, last.cmd);

	if ((state_changed || notify)
		&& !((tshift->state == AV_TIMESHIFT_STAT_FFFB || tshift->state == AV_TIMESHIFT_STAT_PAUSE)
			&& (last.cmd == AV_PLAY_SEEK || last.cmd == AV_PLAY_FF || last.cmd == AV_PLAY_FB || last.cmd == AV_PLAY_PAUSE)))
	{
		aml_timeshift_notify(tshift, NULL, AM_TRUE);
	}

	if (cmd->cmd != AV_PLAY_PAUSE)
		tshift->pause_time = 0;

	return 0;
}

static void aml_timeshift_update_info(AV_TimeshiftData_t *tshift, AM_AV_TimeshiftInfo_t *info)
{
	pthread_mutex_lock(&tshift->lock);
	tshift->info = *info;
	pthread_mutex_unlock(&tshift->lock);
}

static int aml_timeshift_fetch_data(AV_TimeshiftData_t *tshift, uint8_t *buf, size_t size, int timeout)
{
	int cnt = 0;
	int ret;
	uint8_t ts_pkt[188];
	AM_Crypt_Ops_t *crypt_ops = tshift->dev->crypt_ops;
	void *cryptor = tshift->dev->cryptor;

	if (CRYPT_SET(crypt_ops)) {
		int skip = 0;

		/*skip the header*/
		if (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_PLAYBACK) {
			if (AM_TFile_Tell(tshift->file) == 0)
				skip = 2;
		}

		while ((cnt + 188) <= size) {
			if (AM_TFile_GetAvailable(tshift->file) < 188)
				break;

			ret = AM_TFile_Read(tshift->file, ts_pkt, 1, timeout);
			if (ret <= 0 || ret != 1) {
				if (!cnt)
					return 0;
				else
					break;
			}

			if (ts_pkt[0] != 0x47)
				continue;

			ret = AM_TFile_Read(tshift->file, ts_pkt+1, sizeof(ts_pkt)-1, timeout);
			if (ret <= 0 || ret != (sizeof(ts_pkt)-1)) {
				if (!cnt)
					return 0;
				else
					break;
			}

			if (!skip) {
				if (crypt_ops && crypt_ops->crypt)
					crypt_ops->crypt(cryptor, buf+cnt, ts_pkt, 188, 1);
			} else {
				skip--;
				memcpy(buf+cnt, ts_pkt, 188);
			}

			cnt += 188;
		}
	} else {
		cnt = AM_TFile_Read(tshift->file, buf, size, timeout);
	}
	return cnt;
}


#ifdef SUPPORT_CAS
static int build_drm_pkg(uint8_t *outpktdata, uint8_t *addr, uint32_t size)
{
	drminfo_t drminfo;

	memset(&drminfo, 0, sizeof(drminfo_t));
	drminfo.drm_level = DRM_LEVEL1;
	drminfo.drm_pktsize = size;
	drminfo.drm_hasesdata = 0;
	drminfo.drm_phy = (unsigned int)addr;
	drminfo.drm_flag = TYPE_DRMINFO;
	memcpy(outpktdata, &drminfo, sizeof(drminfo_t));

	return 0;
}
#endif

/**\brief Timeshift 线程*/
static void *aml_timeshift_thread(void *arg)
{
	AV_TimeshiftData_t *tshift = (AV_TimeshiftData_t *)arg;
	int speed, update_time, now/*, fffb_time*/;
	int len, ret;
	int /*dmxfd,*/ trick_stat;
#ifdef SUPPORT_CAS
	int secure_enable = tshift->para.para.secure_enable;
	uint8_t *buf = tshift->buf;
	AM_CAS_CryptPara_t cparam;
	uint64_t pos, offset, sec_remain_data_len;
	uint8_t drm_pkt[sizeof(drminfo_t)];
	uint8_t store_info[512];
	uint8_t rec_info[16];
#else
	uint8_t buf[64*1024];
#endif
	const int FFFB_STEP = 150;
	struct timespec rt;
	AM_AV_TimeshiftInfo_t info;
	struct am_io_param astatus;
	struct am_io_param vstatus;
	int playback_alen=0, playback_vlen=0;
	AM_Bool_t is_playback_mode = (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_PLAYBACK);
	int vbuf_len = 0;
	int abuf_len = 0;
	int skip_flag_count = 0;
	int dmx_vpts = 0,  vpts = 0, last_vpts = 0;
	int dmx_apts = 0,  apts = 0, last_apts = 0;
	char pts_buf[32];
	int diff = 0, last_diff = 0;
	int adiff = 0, last_adiff = 0;
	int error_cnt = 0;
	int fetch_fail = 0;
	int debug_print = 0;

	AV_TSPlayPara_t *tp = &tshift->tp;
	AM_Bool_t has_video = VALID_VIDEO(tp->vpid, tp->vfmt);
	AM_Bool_t has_audio = VALID_AUDIO(tp->apid, tp->afmt);

	int do_update;
	AV_PlayCmdPara_t cmd;
	int cmd_more;
	int write_pass;

	int alarm_eof = 0;

	memset(&cmd, 0, sizeof(cmd));
	memset(pts_buf, 0, sizeof(pts_buf));
	memset(&info, 0, sizeof(info));
	timeshift_cmd_init(&tshift->last_cmd[0], -1);

	pthread_mutex_lock(&tshift->lock);
	timeshift_cmd_init(&cmd, tshift->para.para.start_paused ? AV_PLAY_PAUSE : AV_PLAY_START);
	timeshift_cmd_put(tshift, &cmd);
	pthread_mutex_unlock(&tshift->lock);
	AM_DEBUG(1, "[timeshift] start in \"%s\" mode", cmd2string(cmd.cmd));

	tshift->current = tshift->offset;
	AM_DEBUG(1, "[timeshift] start from offset(%dms)", tshift->offset);

	tshift->dev->cryptor = aml_try_open_crypt(tshift->dev->crypt_ops);
	AM_DEBUG(1, "[timeshift] crypt mode : %d", (tshift->dev->cryptor)? 1 : 0);

	debug_print = _get_timethift_dbg();

	AM_DEBUG(1, "Starting timeshift player thread ...");
	info.status = AV_TIMESHIFT_STAT_INITOK;

	AM_TIME_GetClock(&update_time);

	while (tshift->running)
	{
		/*check the audio, in case of audio switch*/
		has_audio = VALID_AUDIO(tp->apid, tp->afmt);

		/*consume the cmds*/
		do {
			pthread_mutex_lock(&tshift->lock);
			cmd_more = timeshift_cmd_more(tshift);
			if (cmd_more)
				timeshift_cmd_get(tshift, &cmd);
			AM_DEBUG(5, ">>>exec cmd:%d pos:%d more:%d", cmd.cmd, cmd.seek.seek_pos, cmd_more);
			pthread_mutex_unlock(&tshift->lock);

			AM_TIME_GetClock(&now);

			do_update = 0;
			if ((now - update_time) >= 500) {
				update_time = now;
				do_update = 1;
			}

			aml_timeshift_do_cmd(tshift, &cmd, do_update);
		} while (tshift->running && cmd_more);

		/*start data inject*/
		/*read some bytes*/
		write_pass = 0;

#ifdef SUPPORT_CAS
		len = SECURE_BLOCK_SIZE - tshift->left;
#else
		len = sizeof(buf) - tshift->left;
#endif
		if (len > 0)
		{
			if (has_video) {
				if (AM_FileRead(VIDEO_DMX_PTS_FILE, pts_buf, sizeof(pts_buf)) >= 0) {
					sscanf(pts_buf, "%u", &dmx_vpts);
				} else {
					AM_DEBUG(1, "cannot read \"%s\"", VIDEO_DMX_PTS_FILE);
					dmx_vpts = 0;
				}
				if (AM_FileRead(VIDEO_PTS_FILE, pts_buf, sizeof(pts_buf)) >= 0) {
					sscanf(pts_buf+2, "%x", &vpts);
				} else {
					AM_DEBUG(1, "cannot read \"%s\"", VIDEO_PTS_FILE);
					vpts = 0;
				}
			}

			if (has_audio) {
				if (AM_FileRead(AUDIO_DMX_PTS_FILE, pts_buf, sizeof(pts_buf)) >= 0) {
					sscanf(pts_buf, "%u", &dmx_apts);
				} else {
					AM_DEBUG(1, "cannot read \"%s\"", AUDIO_DMX_PTS_FILE);
					dmx_apts = 0;
				}
				if (AM_FileRead(AUDIO_PTS_FILE, pts_buf, sizeof(pts_buf)) >= 0) {
					sscanf(pts_buf+2, "%x", &apts);
				} else {
					AM_DEBUG(1, "cannot read \"%s\"", AUDIO_PTS_FILE);
					apts = 0;
				}
			}


			if (tshift->state == AV_TIMESHIFT_STAT_PLAY) {
				if (has_video) {
					if (ioctl(tshift->ts.fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != 0) {
						AM_DEBUG(1,"get v_stat_len failed, %d(%s)", errno, strerror(errno));
					}
				}

				if (has_audio) {
					if (ioctl(tshift->ts.fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != 0) {
						AM_DEBUG(1,"get a_stat_len failed, %d(%s)", errno, strerror(errno));
					}
				}

				int wait_next = 0;

				if (vpts > dmx_vpts) {
					diff = 0;
				} else {
					diff = dmx_vpts - vpts;
				}

				if (apts > dmx_apts) {
					adiff = 0;
				} else {
					adiff = dmx_apts - apts;
				}


				/*fixme: may need more condition according to fmt*/
				#define IS_ALEVEL_OK(_data_len, _afmt) ((_data_len) > 768)
				#define IS_ADUR_OK(_adiff, _afmt)      ((_adiff) >= TIMESHIFT_INJECT_DIFF_TIME)
				#define IS_ADATA_OK(_data_len, _adiff, _afmt)  (IS_ALEVEL_OK(_data_len, _afmt) || IS_ADUR_OK(_adiff, _afmt))

				#define IS_VLEVEL_OK(_data_len, _vfmt) ((_data_len) > DEC_STOP_VIDEO_LEVEL)
				#define IS_VDUR_OK(_vdiff, _vfmt)      ((_vdiff) >= TIMESHIFT_INJECT_DIFF_TIME)
				#define IS_VDATA_OK(_data_len, _vdiff, _vfmt)  (IS_VLEVEL_OK(_data_len, _vfmt) && IS_VDUR_OK(_vdiff, _vfmt))

				/*prevent the data sink into buffer all, which causes sub/txt lost*/
				if (
					(has_video ? IS_VDATA_OK(vstatus.status.data_len, diff, tp->vfmt) : AM_TRUE)
					&& (has_audio ? IS_ADATA_OK(astatus.status.data_len, adiff, tp->afmt) : AM_TRUE)
					/*none? just inject*/
					&& (has_audio || has_video)
				)
					wait_next = 1;
				else
					wait_next = 0;

				if (debug_print) {
					AM_DEBUG(1, ">> wait_next[%d] a/v[%d:%d] lvl[0x%x:0x%x] vpts/vdmx/diff[%#x:%#x:%d], apts/admx/diff[%#x:%#x:%d]\n",
						wait_next,
						has_audio, has_video,
						astatus.status.data_len, vstatus.status.data_len,
						vpts, dmx_vpts, diff,
						apts, dmx_apts, adiff);
				}

				if (wait_next) {
					last_diff = diff;
					last_adiff = adiff;
					tshift->timeout = 10;
					write_pass = 1;
					goto wait_for_next_loop;
				}
			}
#ifdef SUPPORT_CAS
                        if (secure_enable && AM_TFile_Tell(tshift->file) == 0 && is_playback_mode) {
                                /*Skip pat and media_info ts packet, not encrypt*/
                                ret = AM_TFile_Read(tshift->file, buf, 188*11, 100);
                                AM_DEBUG(0, "%s skip pat and media info, %#x,%#x,%#x,%#x", __func__,
                                                buf[0], buf[1], buf[2], buf[3]);
                        }
			do {
#endif
				ret = aml_timeshift_fetch_data(tshift, buf+tshift->left, len - tshift->left, 100);
				if (ret > 0)
				{
					tshift->left += ret;
					fetch_fail = 0;
				}
				else if (len - tshift->left != 0)
				{
					int error = errno;
					//AM_DEBUG(4, "read playback file failed: %s", strerror(errno));
					fetch_fail++;
					/*fetch fail, treat as data break*/
					if (fetch_fail == 1000)
					{
						AM_DEBUG(1, "[timeshift] data break, eof, try:%d", len);
						//AM_EVT_Signal(tshift->dev->dev_no, AM_AV_EVT_PLAYER_EOF, NULL);
					}

					if (errno == EIO && is_playback_mode)
					{
						AM_DEBUG(1, "Disk may be plugged out, exit playback.");
						break;
					}
				}
#ifdef SUPPORT_CAS
			} 
while (secure_enable && (tshift->left < len) && tshift->running);
#endif
		}

		/*Inject*/
		if (tshift->inject_size > 0)
		{
			if (has_video &&
				tshift->state == AV_TIMESHIFT_STAT_PLAY &&
				ioctl(tshift->ts.fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != -1 &&
				ioctl(tshift->ts.fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1)
			{
				if (vstatus.status.data_len == 0 && astatus.status.data_len > 100*1024)
				{
					tshift->timeout = 10;
					goto wait_for_next_loop;
				}
				else
				{
					tshift->timeout = 0;
				}
			}
#ifdef SUPPORT_CAS
			ret = AM_MIN(tshift->left , tshift->inject_size);
			if (ret > 0 && secure_enable && tshift->para.para.dec_cb) {
				//AM_TFile_CasDump();
				offset = 0;
				//offset %= SECURE_BLOCK_SIZE;
				AM_DEBUG(0, "tshift cas dec. offset[%lld]", offset);
				if (!offset) {
					/*Last chunk inject completed,*/
					memset(&cparam, 0, sizeof(cparam));
					cparam.store_info.data = store_info;
					cparam.rec_info.data = rec_info;
					ret = AM_TFile_CasGetRecInfo(&cparam.rec_info);
					pos = AM_TFile_Tell(tshift->file);
					AM_DEBUG(0, "%s, get cas rec info:%d, blk_size:%d, left:%d, tell pos:%lld",
							__func__, ret, cparam.rec_info.blk_size, tshift->left, pos);
					ret = AM_TFile_CasGetStoreInfo(pos, &cparam.store_info);
					if (ret == AM_SUCCESS) {
						cparam.buf_in = buf;
						cparam.buf_out = tshift->para.para.secure_buffer;
						cparam.buf_len = tshift->left;
						tshift->para.para.dec_cb(&cparam, tshift->para.para.cb_param);
					} else {
						AM_DEBUG(0, "%s get cas store info failed, ts_pos:%lld", __func__, pos);
					}
				}
				AM_DEBUG(0, "need inject = %#x, inject_unit = %#x", tshift->left, tshift->inject_size);
				while (tshift->left && tshift->running) {
					ret = AM_MIN(tshift->left, tshift->inject_size);
					build_drm_pkg(drm_pkt, cparam.buf_out + offset, ret);
					ret = aml_timeshift_inject(tshift, drm_pkt, ret, -1);
					if (!ret) {
						AM_DEBUG(0, "%s, secure inject failed, offset:%d, remain:%d", __func__, offset, tshift->left);
						continue;
					}
					offset += ret;
					tshift->left -= ret;
					AM_DEBUG(0, "cas injected = %#x, offset = %#x, left = %#x", ret, offset, tshift->left);
				};
			} else
#endif
                        {
				ret = AM_MIN(tshift->left , tshift->inject_size);
				if (ret > 0)
					ret = aml_timeshift_inject(tshift, buf, ret, -1);

				if (ret > 0)
				{
					/*ret bytes written*/
					tshift->left -= ret;
					if (tshift->left > 0)
						memmove(buf, buf+ret, tshift->left);
				}
			}
		}

wait_for_next_loop:
		/*Update the playing info*/
		if (tshift->state == AV_TIMESHIFT_STAT_FFFB
			|| tshift->last_cmd[0].cmd == AV_PLAY_SEEK)
		{
			if(has_video && vpts == 0)
			{
				AM_DEBUG(1, "[timeshift] FFFB/Seek get vpts==0");
				error_cnt++;
			}
			else
			{
				error_cnt = 0;
			}
			if (error_cnt > TIMESHIFT_FFFB_ERROR_CNT)
			{
				AM_DEBUG(1, "[timeshift] FFFB/Seek error_cnt is overflow, reset trick_mode");
				ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
				ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_FFFB);
				error_cnt = 0;

				if (is_playback_mode
					&& tshift->state == AV_TIMESHIFT_STAT_FFFB
					&& tshift->eof)
				{
					AM_DEBUG(1, "[timeshift] seems to be eof");
					alarm_eof = 1;
				}
			}
			tshift->timeout = 0;

			trick_stat = (has_video) ? aml_timeshift_get_trick_stat(tshift) : 1;

			AM_DEBUG(1, "[timeshift] trick_stat is %d vpts 0x%x", trick_stat, vpts);
			if (trick_stat > 0)
			{
				cmd.done = 1;

				AM_DEBUG(1, "[timeshift] last cmd:%d, state:%d", tshift->last_cmd[0].cmd, tshift->state);
				if (cmd.cmd == AV_PLAY_SEEK) {
					AV_PlayCmdPara_t c;

					/*notify for seek ready*/
					aml_timeshift_notify(tshift, NULL, AM_FALSE);

					/*update for cmd next*/
					aml_timeshift_update_current(tshift);

					/*seek is not a state, set the next cmd to back to previous state*/
					if (tshift->state == AV_TIMESHIFT_STAT_PAUSE) {
						timeshift_cmd_init(&cmd, AV_PLAY_PAUSE);
						aml_timeshift_do_cmd(tshift, &cmd, 1);
					} else if (tshift->state == AV_TIMESHIFT_STAT_FFFB) {
						cmd = tshift->last_cmd[1];
						aml_timeshift_do_cmd(tshift, &cmd, 1);
					} else if (tshift->state == AV_TIMESHIFT_STAT_PLAY) {
						timeshift_cmd_init(&cmd, AV_PLAY_START);
						aml_timeshift_do_cmd(tshift, &cmd, 1);
					}
					aml_timeshift_update_current(tshift);
				} else {
					int fffb_now;
					tshift->timeout = FFFB_STEP;
					AM_TIME_GetClock(&fffb_now);
					if ((fffb_now - tshift->fffb_start) > FFFB_JUMP_STEP) {
						AM_DEBUG(1, "[timeshift] now:%d start:%d", fffb_now, tshift->fffb_start);
						/*notify by each step for fffb*/
						aml_timeshift_notify(tshift, NULL, AM_FALSE);

						am_timeshift_fffb(tshift, &cmd);
						aml_timeshift_update_current(tshift);
					}
				}
			} else {
				if (alarm_eof) {
					/*not too noisy*/
					tshift->timeout = 1000;

					AM_AV_TimeshiftInfo_t info;
					AM_DEBUG(1, "[timeshift] alarm eof: Playback End");
					AM_EVT_Signal(tshift->dev->dev_no, AM_AV_EVT_PLAYER_EOF, NULL);
					aml_timeshift_update(tshift, &info);
					info.current_time = info.full_time;
					aml_timeshift_notify(tshift, &info, AM_FALSE);
				}
			}
		} else {
			if (!write_pass)
				aml_timeshift_update_current(tshift);

			if (tshift->state == AV_TIMESHIFT_STAT_PAUSE) {
				if (tshift->pause_time == 0) {
					tshift->pause_time = tshift->current;
					AM_DEBUG(1, "[timeshift] paused time: %d", tshift->pause_time);
				}
			}

			if (do_update)
			{
				if (ioctl(tshift->ts.fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != -1 &&
					((has_video)? (ioctl(tshift->ts.fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1) : AM_TRUE))
				{
					int play_eof = 0;

					AM_DEBUG(1, "[timeshift] is_playback:%d, loop:%d, timeout:%d len:%d inj:%d, cur:%d, delay:%d",
						is_playback_mode,
						tshift->file->loop,
						tshift->timeout,
						len,
						tshift->inject_size,
						tshift->current,
						aml_timeshift_get_delay(tshift));

					/*no data available in playback only mode*/
					if (is_playback_mode
						&& tshift->state != AV_TIMESHIFT_STAT_PAUSE
						&& abs(tshift->current - aml_timeshift_get_delay(tshift)) >= tshift->end)
					{
						if (playback_alen == astatus.status.data_len &&
							playback_vlen == vstatus.status.data_len)
						{
							AM_AV_TimeshiftInfo_t info;
							AM_DEBUG(1, "[timeshift] Playback End");
							aml_timeshift_update(tshift, &info);
							info.current_time = info.full_time;
							aml_timeshift_notify(tshift, &info, AM_FALSE);
							AM_EVT_Signal(tshift->dev->dev_no, AM_AV_EVT_PLAYER_EOF, NULL);
							play_eof = 1;
						}
						else
						{
							playback_alen = astatus.status.data_len;
							playback_vlen = vstatus.status.data_len;
						}
					}

					if (tshift->current <= tshift->start) {
						AM_DEBUG(1, "[timeshift] reaches start");
					} else if (tshift->current >= tshift->end) {
						AM_DEBUG(1, "[timeshift] reaches end");
					}

					if (tshift->state == AV_TIMESHIFT_STAT_PLAY
						&& !play_eof)
					{
						/********Skip inject error*********/
						if (has_audio && (abuf_len != astatus.status.data_len)) {
							abuf_len = astatus.status.data_len;
							skip_flag_count &= 0xFF00;
						}
						if (has_video && (vbuf_len != vstatus.status.data_len)) {
							vbuf_len =vstatus.status.data_len;
							skip_flag_count &= 0xFF;
						}

						if (tshift->current > 0
							&& ((has_video) ? (vstatus.status.data_len == vbuf_len) : AM_FALSE)) {
							skip_flag_count += 0x100;
						}
						if (tshift->current > 0
							&& ((has_audio) ? (astatus.status.data_len == abuf_len) : AM_FALSE)) {
							skip_flag_count += 0x1;
						}

						if ((skip_flag_count & 0xff) >= 4
							|| (skip_flag_count & 0xff00) >= 0x400) {
							AM_DEBUG(1, "[timeshift] av buf stuck, reset DISABLED, skip_cnt:0x%04x", skip_flag_count);
							if (tshift->dev->replay_enable) {
								am_timeshift_reset_continue(tshift, -1, AM_TRUE);
								vbuf_len = 0;
								abuf_len = 0;
								skip_flag_count=0;
							}
						}
					}

				}
			}
		}

		if (tshift->timeout == -1)
		{
			pthread_mutex_lock(&tshift->lock);
			pthread_cond_wait(&tshift->cond, &tshift->lock);
			pthread_mutex_unlock(&tshift->lock);
		}
		else if (tshift->timeout > 0)
		{
			pthread_mutex_lock(&tshift->lock);
			AM_TIME_GetTimeSpecTimeout(tshift->timeout, &rt);
			pthread_cond_timedwait(&tshift->cond, &tshift->lock, &rt);
			pthread_mutex_unlock(&tshift->lock);
		}
	}

	if (tshift->dev->crypt_ops && tshift->dev->crypt_ops->close) {
		tshift->dev->crypt_ops->close(tshift->dev->cryptor);
		tshift->dev->cryptor = NULL;
	}

	AM_DEBUG(1, "[timeshift] timeshift player thread exit now");
	{
		AM_AV_TimeshiftInfo_t info;
		aml_timeshift_update(tshift, &info);
		info.status = AV_TIMESHIFT_STAT_EXIT;
		aml_timeshift_notify(tshift, &info, AM_FALSE);
	}

	ioctl(tshift->ts.vid_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
	AM_FileEcho(VID_BLACKOUT_FILE, tshift->dev->video_blackout ? "1" : "0");
	AM_DEBUG(1, "[timeshift] timeshift player thread exit end");
	return NULL;
}

static AM_ErrorCode_t aml_timeshift_fill_data(AM_AV_Device_t *dev, uint8_t *data, int size)
{
	AV_TimeshiftData_t *tshift = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;
	int now;

	if (tshift)
	{
		ssize_t ret;

		if (size > 0 && !tshift->rate)
		{
			AM_TIME_GetClock(&now);
			if (! tshift->rtime)
			{
				tshift->rtotal = 0;
				tshift->rtime = now;
			}
			else
			{
				if ((now - tshift->rtime) >= 3000)
				{
					tshift->rtotal += size;
					/*Calcaulate the rate*/
					tshift->rate = (tshift->rtotal*1000)/(now - tshift->rtime);
					if (tshift->rate && tshift->file->loop)
					{
						/*Calculate the file size*/
						tshift->file->size = (loff_t)tshift->rate * (loff_t)(tshift->duration / 1000);
						pthread_cond_signal(&tshift->cond);
						AM_DEBUG(1, "zzz @@@wirte record data %lld bytes in %d ms,so the rate is assumed to %d Bps, ring file size %lld",
							tshift->rtotal, now - tshift->rtime, tshift->rate, tshift->file->size);
					}
					else
					{
						tshift->rtime = 0;
						AM_DEBUG(1, "zzz rtime =  0");
					}
				}
				else
				{
					tshift->rtotal += size;
				}
			}
		}

		ret = AM_TFile_Write(tshift->file, data, size);
		if (ret != (ssize_t)size)
		{
			AM_DEBUG(1, "Write timeshift data to file failed");
			/*A write error*/
			return AM_AV_ERR_SYS;
		}
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_timeshift_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
	AV_TimeshiftData_t *data;
	AV_PlayCmdPara_t cmd_para;

	data = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;

	pthread_mutex_lock(&data->lock);
	timeshift_cmd_init(&cmd_para, cmd);
	if (cmd == AV_PLAY_FF || cmd == AV_PLAY_FB)
		cmd_para.speed.speed = (long)para;
	else if (cmd == AV_PLAY_SEEK)
		cmd_para.seek.seek_pos = ((AV_FileSeekPara_t *)para)->pos;
	timeshift_cmd_put(data, &cmd_para);
	AM_DEBUG(5, ">>>set cmd:%d pos:%d", cmd, cmd_para.seek.seek_pos);

#if 0
	/*cmd obsoleted, use aml_switch_ts_audio_fmt instead*/
	if (cmd == AV_PLAY_SWITCH_AUDIO)
	{
		AV_TSPlayPara_t *audio_para = (AV_TSPlayPara_t *)para;
		if (audio_para)
		{
			int i;

			for (i=0; i<data->para.para.media_info.aud_cnt; i++)
			{
				if (data->para.para.media_info.audios[i].pid == audio_para->apid &&
					data->para.para.media_info.audios[i].fmt == audio_para->afmt)
				{
					data->tp.apid = audio_para->apid;
					data->tp.afmt = audio_para->afmt;
					break;
				}
			}
		}
	}
#endif
	pthread_cond_signal(&data->cond);
	pthread_mutex_unlock(&data->lock);

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_timeshift_get_info(AM_AV_Device_t *dev, AM_AV_TimeshiftInfo_t *info)
{
	AV_TimeshiftData_t *data;

	data = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;

	pthread_mutex_lock(&data->lock);
	*info = data->info;
	pthread_mutex_unlock(&data->lock);

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_timeshift_get_tfile(AM_AV_Device_t *dev, AM_TFile_t *tfile)
{
	AV_TimeshiftData_t *data;

	data = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;

	pthread_mutex_lock(&data->lock);
	*tfile = data->file;
	pthread_mutex_unlock(&data->lock);

	return AM_SUCCESS;
}

extern unsigned long CMEM_getPhys(unsigned long virts);

/**\brief 解码JPEG数据*/
static AM_ErrorCode_t aml_decode_jpeg(AV_JPEGData_t *jpeg, const uint8_t *data, int len, int mode, void *para)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
// #if !defined(ANDROID)
// 	AV_JPEGDecodePara_t *dec_para = (AV_JPEGDecodePara_t *)para;
// 	AV_JPEGDecState_t s;
// 	AM_Bool_t decLoop = AM_TRUE;
// 	int decState = 0;
// 	int try_count = 0;
// 	int decode_wait = 0;
// 	const uint8_t *src = data;
// 	int left = len;
// 	AM_OSD_Surface_t *surf = NULL;
// 	jpegdec_info_t info;

// 	char tmp_buf[64];

// 	s = AV_JPEG_DEC_STAT_INFOCONFIG;
// 	while (decLoop)
// 	{
// 		if (jpeg->dec_fd == -1)
// 		{
// 			jpeg->dec_fd = open(JPEG_DEC_FILE, O_RDWR);
// 			if (jpeg->dec_fd != -1)
// 			{
// 				s = AV_JPEG_DEC_STAT_INFOCONFIG;
// 			}
// 			else
// 			{
// 				try_count++;
// 				if (try_count > 40)
// 				{
// 					AM_DEBUG(1, "jpegdec init timeout");
// 					try_count=0;
// 					ret = aml_init_jpeg(jpeg);
// 					if (ret != AM_SUCCESS)
// 						break;
// 				}
// 				usleep(10000);
// 				continue;
// 			}
// 		}
// 		else
// 		{
// 			decState = ioctl(jpeg->dec_fd, JPEGDEC_IOC_STAT);

// 			if (decState & JPEGDEC_STAT_ERROR)
// 			{
// 				AM_DEBUG(1, "jpegdec JPEGDEC_STAT_ERROR");
// 				ret = AM_AV_ERR_DECODE;
// 				break;
// 			}

// 			if (decState & JPEGDEC_STAT_UNSUPPORT)
// 			{
// 				AM_DEBUG(1, "jpegdec JPEGDEC_STAT_UNSUPPORT");
// 				ret = AM_AV_ERR_DECODE;
// 				break;
// 			}

// 			if (decState & JPEGDEC_STAT_DONE)
// 				break;

// 			if (decState & JPEGDEC_STAT_WAIT_DATA)
// 			{
// 				if (left > 0)
// 				{
// 					int send = AM_MIN(left, JPEG_WRTIE_UNIT);
// 					int rc;
// 					rc = write(jpeg->vbuf_fd, src, send);
// 					if (rc == -1)
// 					{
// 						AM_DEBUG(1, "write data to the jpeg decoder failed");
// 						ret = AM_AV_ERR_DECODE;
// 						break;
// 					}
// 					left -= rc;
// 					src  += rc;
// 				}
// 				else if (decode_wait == 0)
// 				{
// 					int i, times = JPEG_WRTIE_UNIT/sizeof(tmp_buf);

// 					memset(tmp_buf, 0, sizeof(tmp_buf));

// 					for (i=0; i<times; i++)
// 						write(jpeg->vbuf_fd, tmp_buf, sizeof(tmp_buf));
// 					decode_wait++;
// 				}
// 				else
// 				{
// 					if (decode_wait > 300)
// 					{
// 						AM_DEBUG(1, "jpegdec wait data error");
// 						ret = AM_AV_ERR_DECODE;
// 						break;
// 					}
// 					decode_wait++;
// 					usleep(10);
// 				}
// 			}

// 			switch (s)
// 			{
// 				case AV_JPEG_DEC_STAT_INFOCONFIG:
// 					if (decState & JPEGDEC_STAT_WAIT_INFOCONFIG)
// 					{
// 						if (ioctl(jpeg->dec_fd, JPEGDEC_IOC_INFOCONFIG, 0) == -1)
// 						{
// 							AM_DEBUG(1, "jpegdec JPEGDEC_IOC_INFOCONFIG error");
// 							ret = AM_AV_ERR_DECODE;
// 							decLoop = AM_FALSE;
// 						}
// 						s = AV_JPEG_DEC_STAT_INFO;
// 					}
// 					break;
// 				case AV_JPEG_DEC_STAT_INFO:
// 					if (decState & JPEGDEC_STAT_INFO_READY)
// 					{
// 						if (ioctl(jpeg->dec_fd, JPEGDEC_IOC_INFO, &info) == -1)
// 						{
// 							AM_DEBUG(1, "jpegdec JPEGDEC_IOC_INFO error");
// 							ret = AM_AV_ERR_DECODE;
// 							decLoop = AM_FALSE;
// 						}
// 						if (mode & AV_GET_JPEG_INFO)
// 						{
// 							AM_AV_JPEGInfo_t *jinfo = (AM_AV_JPEGInfo_t *)para;
// 							jinfo->width    = info.width;
// 							jinfo->height   = info.height;
// 							jinfo->comp_num = info.comp_num;
// 							decLoop = AM_FALSE;
// 						}
// 						AM_DEBUG(2, "jpegdec width:%d height:%d", info.width, info.height);
// 						s = AV_JPEG_DEC_STAT_DECCONFIG;
// 					}
// 					break;
// 				case AV_JPEG_DEC_STAT_DECCONFIG:
// 					if (decState & JPEGDEC_STAT_WAIT_DECCONFIG)
// 					{
// 						jpegdec_config_t config;
// 						int dst_w, dst_h;

// 						switch (dec_para->para.angle)
// 						{
// 							case AM_AV_JPEG_CLKWISE_0:
// 							default:
// 								dst_w = info.width;
// 								dst_h = info.height;
// 							break;
// 							case AM_AV_JPEG_CLKWISE_90:
// 								dst_w = info.height;
// 								dst_h = info.width;
// 							break;
// 							case AM_AV_JPEG_CLKWISE_180:
// 								dst_w = info.width;
// 								dst_h = info.height;
// 							break;
// 							case AM_AV_JPEG_CLKWISE_270:
// 								dst_w = info.height;
// 								dst_h = info.width;
// 							break;
// 						}

// 						if (dec_para->para.width > 0)
// 							dst_w = AM_MIN(dst_w, dec_para->para.width);
// 						if (dec_para->para.height > 0)
// 							dst_h = AM_MIN(dst_h, dec_para->para.height);

// 						ret = AM_OSD_CreateSurface(AM_OSD_FMT_YUV_420, dst_w, dst_h, AM_OSD_SURFACE_FL_HW, &surf);
// 						if (ret != AM_SUCCESS)
// 						{
// 							AM_DEBUG(1, "cannot create the YUV420 surface");
// 							decLoop = AM_FALSE;
// 						}
// 						else
// 						{
// 							config.addr_y = CMEM_getPhys((unsigned long)surf->buffer);
// 							config.addr_u = config.addr_y+CANVAS_ALIGN(surf->width)*surf->height;
// 							config.addr_v = config.addr_u+CANVAS_ALIGN(surf->width/2)*(surf->height/2);
// 							config.opt    = dec_para->para.option;
// 							config.dec_x  = 0;
// 							config.dec_y  = 0;
// 							config.dec_w  = surf->width;
// 							config.dec_h  = surf->height;
// 							config.angle  = dec_para->para.angle;
// 							config.canvas_width = CANVAS_ALIGN(surf->width);

// 							if (ioctl(jpeg->dec_fd, JPEGDEC_IOC_DECCONFIG, &config) == -1)
// 							{
// 								AM_DEBUG(1, "jpegdec JPEGDEC_IOC_DECCONFIG error");
// 								ret = AM_AV_ERR_DECODE;
// 								decLoop = AM_FALSE;
// 							}
// 							s = AV_JPEG_DEC_STAT_RUN;
// 						}
// 					}
// 				break;

// 				default:
// 					break;
// 			}
// 		}
// 	}

// 	if (surf)
// 	{
// 		if (ret == AM_SUCCESS)
// 		{
// 			dec_para->surface = surf;
// 		}
// 		else
// 		{
// 			AM_OSD_DestroySurface(surf);
// 		}
// 	}
// #else
// 	UNUSED(jpeg);
// 	UNUSED(data);
// 	UNUSED(len);
// 	UNUSED(mode);
// 	UNUSED(para);
// #endif

	return ret;
}

static AM_ErrorCode_t aml_open(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para)
{
//#ifndef ANDROID
	char buf[32];
	int v;

	UNUSED(para);
	pthread_cond_init(&gAVMonCond, NULL);
	if (AM_FileRead(VID_AXIS_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		int left, top, right, bottom;

		if (sscanf(buf, "%d %d %d %d", &left, &top, &right, &bottom) == 4)
		{
			dev->video_x = left;
			dev->video_y = top;
			dev->video_w = right-left;
			dev->video_h = bottom-top;
		}
	}
	if (AM_FileRead(VID_CONTRAST_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_contrast = v;
		}
	}
	if (AM_FileRead(VID_SATURATION_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_saturation = v;
		}
	}
	if (AM_FileRead(VID_BRIGHTNESS_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_brightness = v;
		}
	}
	if (AM_FileRead(VID_DISABLE_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_enable = v?AM_FALSE:AM_TRUE;
		}
	}
	if (AM_FileRead(VID_BLACKOUT_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_blackout = v?AM_TRUE:AM_FALSE;
		}
	}
	if (AM_FileRead(VID_SCREEN_MODE_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_mode = v;
#ifndef 	CHIP_8226H
			switch(v) {
				case 0:
				case 1:
					dev->video_ratio = AM_AV_VIDEO_ASPECT_AUTO;
					dev->video_mode = v;
					break;
				case 2:
					dev->video_ratio = AM_AV_VIDEO_ASPECT_4_3;
					dev->video_mode = AM_AV_VIDEO_DISPLAY_FULL_SCREEN;
					break;
				case 3:
					dev->video_ratio = AM_AV_VIDEO_ASPECT_16_9;
					dev->video_mode = AM_AV_VIDEO_DISPLAY_FULL_SCREEN;
					break;
			}
 #endif
		}
	}
	if (AM_FileRead(DISP_MODE_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "576cvbs", 7) || !strncmp(buf, "576i", 4) || !strncmp(buf, "576i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 576;
		}
		else if (!strncmp(buf, "480cvbs", 7) || !strncmp(buf, "480i", 4) || !strncmp(buf, "480i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 480;
		}
		else if (!strncmp(buf, "720p", 4))
		{
			dev->vout_w = 1280;
			dev->vout_h = 720;
		}
		else if (!strncmp(buf, "1080i", 5) || !strncmp(buf, "1080p", 5))
		{
			dev->vout_w = 1920;
			dev->vout_h = 1080;
		}
	}
#ifdef CHIP_8226H
	if (AM_FileRead(VID_ASPECT_RATIO_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "auto", 4))
		{
			dev->video_ratio = AM_AV_VIDEO_ASPECT_AUTO;
		}
		else if (!strncmp(buf, "16x9", 4))
		{
			dev->video_ratio = AM_AV_VIDEO_ASPECT_16_9;
		}
		else if (!strncmp(buf, "4x3", 3))
		{
			dev->video_ratio = AM_AV_VIDEO_ASPECT_4_3;
		}
	}
	if (AM_FileRead(VID_ASPECT_MATCH_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "ignore", 4))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_IGNORE;
		}
		else if (!strncmp(buf, "letter box", 10))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX;
		}
		else if (!strncmp(buf, "pan scan", 8))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN;
		}
		else if (!strncmp(buf, "combined", 8))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_COMBINED;
		}
	}
#else
#ifdef ANDROID
	if (AM_FileRead(VID_ASPECT_MATCH_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			switch (v)
			{
				case 0:
				case 1:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_IGNORE;
					break;
				case 2:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN;
					break;
				case 3:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX;
					break;
				case 4:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_COMBINED;
					break;
			}
		}
	}
#endif
#endif
//#endif

#if !defined(ADEC_API_NEW)
	adec_cmd("stop");
	return AM_SUCCESS;
#else
#ifdef USE_ADEC_IN_DVB
	if (s_audio_cb == NULL)
		audio_decode_basic_init();
#endif
	return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t aml_close(AM_AV_Device_t *dev)
{
	UNUSED(dev);
	pthread_cond_destroy(&gAVMonCond);
	return AM_SUCCESS;
}

#define ARC_FREQ_FILE "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"

static void set_arc_freq(int hi)
{
	static int old_freq = 200000;
	int freq;
	char buf[32];

	if (AM_FileRead(ARC_FREQ_FILE, buf, sizeof(buf)) == AM_SUCCESS) {
		sscanf(buf, "%d", &freq);
		if (hi) {
			old_freq = freq;
			snprintf(buf, sizeof(buf), "%d", 300000);
			AM_FileEcho(ARC_FREQ_FILE, buf);
		} else {
			snprintf(buf, sizeof(buf), "%d", old_freq);
			AM_FileEcho(ARC_FREQ_FILE, buf);
		}
	}
}

static AM_ErrorCode_t set_dec_control(AM_Bool_t enable)
{
	char v[32], vv[32];
	int dc=0;
	char *pch = v;

	static char *dec_control[] = {
		DEC_CONTROL_H264,
		DEC_CONTROL_MPEG12
	};
	int cnt = sizeof(dec_control)/sizeof(dec_control[0]);
	int i;
	if (enable) {//format: "0xaa|0xbb"
#ifdef ANDROID
		property_get(DEC_CONTROL_PROP, v, "");
#endif
		for (i=0; i<cnt; i++) {
			dc = 0;
			if (!pch || !pch[0])
				break;
			int j = sscanf(pch, "%i", &dc);
			if (j) {
				snprintf(vv, 31, "%#x", dc);
				AM_FileEcho(dec_control[i], vv);
				AM_DEBUG(1, "dec control: %d==%s\n", i, vv);
			}
			pch = strchr(pch, '|');
			if (pch)
				pch++;
		}
	} else {
		for (i=0; i<cnt; i++)
			AM_FileEcho(dec_control[i], "0");
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_open_ts_mode(AM_AV_Device_t *dev)
{
	AV_TSData_t *ts;
	ts = (AV_TSData_t *)malloc(sizeof(AV_TSData_t));
	if (!ts)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_AV_ERR_NO_MEM;
	}

	memset(ts, 0, sizeof(*ts));
	ts->fd     = -1;
	ts->vid_fd = -1;

	dev->ts_player.drv_data = (void*)ts;

	return AM_SUCCESS;
}

static int aml_get_audio_digital_raw(void)
{
	int mode;
	char buf[32];
	/*0:pcm 1:spdif 2:hdmi*/
	if (AM_FileRead(AUDIO_DSP_DIGITAL_RAW_FILE, buf, sizeof(buf)) >= 0) {
		sscanf(buf, "%d", &mode);
	} else {
		mode = 0;
	}
	return mode;
}

static int aml_calc_sync_mode(AM_AV_Device_t *dev, int has_audio, int has_video, int has_pcr, int afmt, int *force_reason)
{
	int is_dts_dolby = 0;
	int ac3_amaster = 0;
	int is_digital_output = 0;
	int tsync_mode = 2;

#define VMASTER   0
#define AMASTER   1
#define PCRMASTER 2

	if (force_reason)
		*force_reason = FORCE_NONE;

#ifdef ANDROID
	if ((afmt == AFORMAT_AC3) || (afmt == AFORMAT_EAC3)) {
		char buf[32];
		property_get(AC3_AMASTER_PROP, buf, "0");
		if (!strcmp(buf, "1"))
			ac3_amaster = 1;
	}
#endif

	if (afmt == AFORMAT_AC3 ||
		afmt == AFORMAT_DTS ||
		afmt == AFORMAT_EAC3) {
		is_dts_dolby = 1;
	}

	tsync_mode = PCRMASTER;

	if (ac3_amaster) {
		//force
		tsync_mode = AMASTER;
		if (force_reason)
			*force_reason = FORCE_AC3_AMASTER;
	}

	if ((aml_get_audio_digital_raw() != 0) && is_dts_dolby)
		tsync_mode = AMASTER;

	if ((tsync_mode == AMASTER) && !has_audio)
		tsync_mode = VMASTER;
	else if (ac3_amaster == 0)
		tsync_mode = PCRMASTER; /*Force use pcrmaster for live-pvr-tf*/

	//printf("tsync mode calc:%d v:%d a:%d af:%d force:%d\n",
	//	tsync_mode, has_video, has_audio, afmt, force_reason? *force_reason : 0);

	return tsync_mode;
}

static int aml_set_sync_mode(AM_AV_Device_t *dev, int mode)
{
	char mode_str[4];
	int f_m = _get_tsync_mode_force();

	if (f_m != -1) {
		mode = f_m;
		AM_DEBUG(1, "force tsync mode to %d", mode);
	}

	set_first_frame_nosync();
	snprintf(mode_str, 4, "%d", mode);
	AM_DEBUG(1, "set sync mode: %d", mode);
	return AM_FileEcho(TSYNC_MODE_FILE, mode_str);
}

static AM_ErrorCode_t aml_start_ts_mode(AM_AV_Device_t *dev, AV_TSPlayPara_t *tp, AM_Bool_t create_thread)
{
	AV_TSData_t *ts;
	int val;
	AM_Bool_t has_video = VALID_VIDEO(tp->vpid, tp->vfmt);
	AM_Bool_t has_audio = VALID_AUDIO(tp->apid, tp->afmt);
	AM_Bool_t has_pcr = VALID_PCR(tp->pcrpid);
	int sync_mode, sync_force;
	char vdec_info[256];
	int double_write_mode = 3;

	if (tp->apid != dev->alt_apid || tp->afmt != dev->alt_afmt) {
		AM_DEBUG(1, "switch to pending audio: A[%d:%d] -> A[%d:%d]",
			tp->apid, tp->afmt, dev->alt_apid, dev->alt_afmt);
		tp->apid = dev->alt_apid;
		tp->afmt = dev->alt_afmt;

		has_audio = VALID_AUDIO(tp->apid, tp->afmt);
	}

	AM_DEBUG(1, "aml start ts: V[%d:%d] A[%d:%d] P[%d]", tp->vpid, tp->vfmt, tp->apid, tp->afmt, tp->pcrpid);
	ts = (AV_TSData_t *)dev->ts_player.drv_data;

	/*patch dec control*/
	set_dec_control(has_video);

#ifndef ENABLE_PCR
	if (ts->vid_fd != -1){
		AM_DEBUG(1, "%s, enable video pause", __FUNCTION__);
		ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 1);
	}else{
		AM_DEBUG(1, "%s, disenable video pause", __FUNCTION__);
	}
#else
	AM_DEBUG(1, "%s, enable pcr -------", __FUNCTION__);
#endif

	ts->vid_fd = open(AMVIDEO_FILE, O_RDWR);

#ifndef MPTSONLYAUDIOVIDEO
	if (check_vfmt_support_sched(tp->vfmt) == AM_FALSE)
	{
		ts->fd = open(STREAM_TS_FILE, O_RDWR);
		if (ts->fd == -1)
		{
			AM_DEBUG(1, "cannot open \"/dev/amstream_mpts\" error:%d \"%s\"", errno, strerror(errno));
			free(ts);
			return AM_AV_ERR_CANNOT_OPEN_DEV;
		}
	}
	else
	{
		ts->fd = open(STREAM_TS_SCHED_FILE, O_RDWR);
		if (ts->fd == -1)
		{
			AM_DEBUG(1, "cannot open \"/dev/amstream_mpts_sched\" error:%d \"%s\"", errno, strerror(errno));
			free(ts);
			return AM_AV_ERR_CANNOT_OPEN_DEV;
		}
	}
#ifdef USE_AMSTREAM_SUB_BUFFER
	AM_DEBUG(1, "try to init subtitle ring buf");
	int sid = 0xffff;
	if (ioctl(ts->fd, AMSTREAM_IOC_SID, sid) == -1)
	{
		AM_DEBUG(1, "set sub PID failed");
	}
#endif
#else
	if (ts->fd == -1)
	{

		AM_DEBUG(1, "aml_start_ts_mode used MPTSONLYAUDIOVIDEO %d %d %d %d", tp->vpid, has_video, tp->apid, has_audio);

		if (has_video && has_audio)
		{
			if (check_vfmt_support_sched(tp->vfmt) == AM_FALSE)
			{
				ts->fd = open(STREAM_TS_FILE, O_RDWR);
			}
			else
			{
				ts->fd = open(STREAM_TS_SCHED_FILE, O_RDWR);
			}
		}
		else if ((!has_video) && has_audio)
		{
			ts->fd = open(STREAM_TSONLYAUDIO_FILE, O_RDWR);
		}
		else if((!has_audio) && has_video)
		{
			ts->fd = open(STREAM_TSONLYVIDEO_FILE, O_RDWR);
		}
		else
		{
			if (check_vfmt_support_sched(tp->vfmt) == AM_FALSE)
			{
				ts->fd = open(STREAM_TS_FILE, O_RDWR);
			}
			else
			{
				ts->fd = open(STREAM_TS_SCHED_FILE, O_RDWR);
			}
		}

		if (ts->fd == -1)
		{
			AM_DEBUG(1, "cannot open \"/dev/amstream_mpts\" error:%d \"%s\"", errno, strerror(errno));
			return AM_AV_ERR_CANNOT_OPEN_DEV;
		}
	}
#endif

	if (tp->drm_mode == AM_AV_DRM_WITH_SECURE_INPUT_BUFFER)
	{
		if (ioctl(ts->fd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)tp->drm_mode) == -1)
		{
			AM_DEBUG(1, "set drm_mode with secure buffer failed\n");
			return AM_AV_ERR_SYS;
		}
	}
	if (tp->drm_mode == AM_AV_DRM_WITH_NORMAL_INPUT_BUFFER)
	{
		if (ioctl(ts->fd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)tp->drm_mode) == -1)
		{
			AM_DEBUG(1, "set drm_mode with normal buffer failed\n");
			return AM_AV_ERR_SYS;
		}
	}

	/*Set tsync enable/disable*/
	if ((has_video && has_audio) || has_pcr)
	{
		AM_DEBUG(1, "Set tsync enable to 1");
		aml_set_tsync_enable(1);
	}
	else
	{
		AM_DEBUG(1, "Set tsync enable to 0");
		aml_set_tsync_enable(0);
	}

	if (has_video) {
		val = tp->vfmt;
		if (ioctl(ts->fd, AMSTREAM_IOC_VFORMAT, val) == -1)
		{
			AM_DEBUG(1, "set video format failed");
			return AM_AV_ERR_SYS;
		}
		val = tp->vpid;
		if (ioctl(ts->fd, AMSTREAM_IOC_VID, val) == -1)
		{
			AM_DEBUG(1, "set video PID failed");
			return AM_AV_ERR_SYS;
		}
		if (dev->afd_enable) {
			AM_DEBUG(1, "start afd...");
			aml_start_afd(dev, tp->vfmt);
		}
	}

	/*if ((tp->vfmt == VFORMAT_H264) || (tp->vfmt == VFORMAT_VC1))*/
	if (has_video) {

		memset(&am_sysinfo,0,sizeof(dec_sysinfo_t));
		if (tp->vfmt == VFORMAT_VC1)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (tp->vfmt == VFORMAT_H264)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (tp->vfmt == VFORMAT_AVS)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_AVS;
			/*am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;*/
		}
		else if (tp->vfmt == VFORMAT_HEVC)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			am_sysinfo.width  = 3840;
			am_sysinfo.height = 2160;
		}

		if (ioctl(ts->fd, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo) == -1)
		{
			AM_DEBUG(1, "set AMSTREAM_IOC_SYSINFO");
			return AM_AV_ERR_SYS;
		}
		/*configure double wirte mode*/
		memset(vdec_info, 0, sizeof(vdec_info));
		if (tp->vfmt == VFORMAT_HEVC) {
			sprintf(vdec_info, "hevc_double_write_mode:%d", double_write_mode);
			configure_vdec_info(ts->fd, vdec_info, strlen(vdec_info));
		} else if (tp->vfmt == VFORMAT_VP9) {
			sprintf(vdec_info, "vp9_double_write_mode:%d", double_write_mode);
			configure_vdec_info(ts->fd, vdec_info, strlen(vdec_info));
		}
	}

	if (has_audio) {
		if ((tp->afmt == AFORMAT_AC3) || (tp->afmt == AFORMAT_DTS)) {
			//set_arc_freq(1);
		}

		val = tp->afmt;
		if (ioctl(ts->fd, AMSTREAM_IOC_AFORMAT, val) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		val = tp->apid;
		if (ioctl(ts->fd, AMSTREAM_IOC_AID, val) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}

//#ifdef ENABLE_PCR
	sync_mode = aml_calc_sync_mode(dev, has_audio, has_video, has_pcr, tp->afmt, &sync_force);

	if ((sync_force != FORCE_AC3_AMASTER)) {
		if (has_pcr) {
			val = tp->pcrpid;
			if (ioctl(ts->fd, AMSTREAM_IOC_PCRID, val) == -1)
			{
				AM_DEBUG(1, "set PCR PID failed");
				return AM_AV_ERR_SYS;
			}
		}
	}

	aml_set_sync_mode(dev, sync_mode);
	if (has_audio && (sync_force != FORCE_AC3_AMASTER)) {
		if (!show_first_frame_nosync()) {
#ifdef ANDROID
			//property_set("sys.amplayer.drop_pcm", "1");
#endif
		}
		AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");

		if (VALID_PID(tp->sub_apid))
			aml_set_audio_ad(dev, 1, tp->sub_apid, tp->sub_afmt);

		audio_ops->adec_start_decode(ts->fd, tp->afmt, has_video, &ts->adec);
	}
//#endif /*ENABLE_PCR*/

	if (ioctl(ts->fd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
	{
		AM_DEBUG(1, "amport init failed");
		return AM_AV_ERR_SYS;
	}
	AM_DEBUG(1, "set ts skipbyte to 0");
	if (ioctl(ts->fd, AMSTREAM_IOC_TS_SKIPBYTE, 0) == -1)
	{
		AM_DEBUG(1, "set ts skipbyte failed");
		return AM_AV_ERR_SYS;
	}

	AM_TIME_GetClock(&dev->ts_player.av_start_time);

	/*创建AV监控线程*/
	if (create_thread)
		aml_start_av_monitor(dev, &dev->ts_player.mon);

	dev->ts_player.play_para = *tp;

	return AM_SUCCESS;
}

static int aml_close_ts_mode(AM_AV_Device_t *dev, AM_Bool_t destroy_thread)
{
	AV_TSData_t *ts;
	int fd;

	if (dev->afd_enable)
		aml_stop_afd(dev);

	if (destroy_thread)
		aml_stop_av_monitor(dev, &dev->ts_player.mon);

	ts = (AV_TSData_t*)dev->ts_player.drv_data;
	if (ts->fd != -1)
		close(ts->fd);
	if (ts->vid_fd != -1)
		close(ts->vid_fd);

	aml_set_tsync_enable(0);
	aml_set_ad_source(&ts->ad, 0, 0, 0, ts->adec);
	audio_ops->adec_set_decode_ad(0, 0, 0, ts->adec);
	audio_ops->adec_stop_decode(&ts->adec);

	free(ts);

	dev->ts_player.drv_data = NULL;

//#ifdef ENABLE_PCR
#ifdef ANDROID
	//property_set("sys.amplayer.drop_pcm", "0");
#endif
	AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
	AM_FileEcho(TSYNC_MODE_FILE, "0");
//#endif /*ENABLE_PCR*/

	//set_arc_freq(0);

	/*unpatch dec control*/
	set_dec_control(AM_FALSE);

	AM_DEBUG(1, "close ts mode end.");
	return 0;
}

static int aml_restart_ts_mode(AM_AV_Device_t *dev, AM_Bool_t destroy_thread)
{
	aml_close_ts_mode(dev, destroy_thread);
	aml_open_ts_mode(dev);
	setup_forced_pid(&dev->ts_player.play_para);
	aml_start_ts_mode(dev, &dev->ts_player.play_para, destroy_thread);
	return 0;
}

static inline AM_AV_VideoAspectRatio_t
convert_aspect_ratio(enum E_ASPECT_RATIO euAspectRatio)
{
	switch (euAspectRatio) {
	case ASPECT_RATIO_16_9:
		return AM_AV_VIDEO_ASPECT_16_9;
	case ASPECT_RATIO_4_3:
		return AM_AV_VIDEO_ASPECT_4_3;
	default:
		return AM_AV_VIDEO_ASPECT_AUTO;
	}
	return AM_AV_VIDEO_ASPECT_AUTO;
}

int am_av_restart_pts_repeat_count = 2;

/**\brief AV buffer 监控线程*/
static void* aml_av_monitor_thread(void *arg)
{
	AM_AV_Device_t *dev = (AM_AV_Device_t *)arg;
	AM_Bool_t adec_start = AM_FALSE;
	AM_Bool_t av_paused = AM_TRUE;
	AM_Bool_t has_audio;
	AM_Bool_t has_video;
	AM_Bool_t bypass_di = AM_FALSE;
	AM_Bool_t drop_b_frame = AM_FALSE;
	AM_Bool_t is_hd_video = AM_FALSE;
	AM_Bool_t audio_scrambled = AM_FALSE;
	AM_Bool_t video_scrambled = AM_FALSE;
	AM_Bool_t no_audio_data = AM_TRUE, no_video_data = AM_TRUE, no_data_evt = AM_FALSE;
	AM_Bool_t no_video = AM_TRUE;
	AM_Bool_t has_amaster = AM_FALSE;
	AM_Bool_t need_replay;
	int resample_type = 0;
	int next_resample_type = resample_type;
	int now, next_resample_start_time = 0;
	int abuf_level = 0, vbuf_level = 0;
	int abuf_size = 0, vbuf_size = 0;
	unsigned int abuf_read_ptr = 0, vbuf_read_ptr = 0;
	unsigned int last_abuf_read_ptr = 0, last_vbuf_read_ptr = 0;
	int arp_stop_time = 0, vrp_stop_time = 0;
	int arp_stop_dur = 0, vrp_stop_dur = 0;
	int apts = 0, vpts = 0, last_apts = 0, last_vpts = 0;
	int dmx_apts = 0, dmx_vpts = 0, last_dmx_apts = 0, last_dmx_vpts = 0;
	int apts_stop_time = 0, vpts_stop_time = 0, apts_stop_dur = 0, vpts_stop_dur = 0;
	int dmx_apts_stop_time = 0, dmx_vpts_stop_time = 0, dmx_apts_stop_dur = 0, dmx_vpts_stop_dur = 0;
	int tsync_mode, vmaster_time = 0, vmaster_dur = 0;
	int abuf_level_empty_time = 0, abuf_level_empty_dur = 0, vbuf_level_empty_time = 0, vbuf_level_empty_dur = 0;
	int down_audio_cache_time = 0, down_video_cache_time = 0;
	int vdec_stop_time = 0, vdec_stop_dur = 0;
	int checkin_firstapts = 0, checkin_firstvpts = 0;
	int apts_discontinue = 0, vpts_discontinue = 0;
	struct am_io_param astatus;
	struct am_io_param vstatus;
	int vdec_status, frame_width, frame_height, aspect_ratio;
	int frame_width_old = -1, frame_height_old = -1, aspect_ratio_old = -1;
	struct timespec rt;
	char buf[32];
	AM_Bool_t is_avs_plus = AM_FALSE;
	int avs_fmt = 0;

	unsigned int vframes_now = 0, vframes_last = 0;

	int mChange_audio_flag = -1;
	//int mCur_audio_digital_raw_value = 0;
	int is_dts_dolby = 0, tsync_pcr_mode = 0;

	AV_Monitor_t *mon;
	AV_TSData_t *ts,ts_temp;
	AV_TSPlayPara_t *tp;
	AV_InjectData_t *inj;
	unsigned int cur_time = 0;
	unsigned int last_replay_time = 0;
#define REPLAY_TIME_INTERVAL   1000

	int replay_done = 0;

	if (dev->mode == AV_TIMESHIFT) {
		AV_TimeshiftData_t *tshift_d = (AV_TimeshiftData_t*)dev->timeshift_player.drv_data;
		mon = &dev->timeshift_player.mon;
		tp = &tshift_d->tp;
		ts = &tshift_d->ts;
	} else if (dev->mode == AV_INJECT){
		mon = &dev->ts_player.mon;
		tp = &dev->ts_player.play_para;
		inj = (AV_InjectData_t *)dev->inject_player.drv_data;
		ts_temp.fd = (inj->vid_fd != -1) ? inj->vid_fd : inj-> aud_fd;
		ts_temp.vid_fd = inj->cntl_fd;
		ts_temp.adec = inj->adec;
		ts = &ts_temp;
	} else {//ts mode default
		mon = &dev->ts_player.mon;
		tp = &dev->ts_player.play_para;
		ts = (AV_TSData_t*)dev->ts_player.drv_data;
	}

	has_audio = VALID_AUDIO(tp->apid, tp->afmt);
	has_video = VALID_VIDEO(tp->vpid, tp->vfmt);

#ifndef ENABLE_PCR
	if (!show_first_frame_nosync()) {
#ifdef ANDROID
		//property_set("sys.amplayer.drop_pcm", "1");
#endif
	}
#else
	av_paused  = AM_FALSE;
	adec_start = (adec_handle != NULL);
#endif

	if (dev->mode != AV_TIMESHIFT)
		AM_FileEcho(VID_BLACKOUT_FILE, dev->video_blackout ? "1" : "0");

	pthread_mutex_lock(&gAVMonLock);

	if (tp->afmt == AFORMAT_AC3 ||
		tp->afmt == AFORMAT_DTS ||
		tp->afmt == AFORMAT_EAC3) {
		is_dts_dolby = 1;
	}

	while (mon->av_thread_running) {

		if (!adec_start || (has_video && no_video))
			AM_TIME_GetTimeSpecTimeout(20, &rt);
		else
			AM_TIME_GetTimeSpecTimeout(200, &rt);

		pthread_cond_timedwait(&gAVMonCond, &gAVMonLock, &rt);

		if (! mon->av_thread_running)
		{
			AM_DEBUG(1,"[aml_av_monitor_thread] ending");
			break;
		}

		if (is_dts_dolby == 1) {
			if (mChange_audio_flag == -1) {
				mChange_audio_flag = aml_get_audio_digital_raw();
			} else if (mChange_audio_flag != aml_get_audio_digital_raw()) {
				mChange_audio_flag = aml_get_audio_digital_raw();
				dev->audio_switch = AM_TRUE;
			}
		}
		//switch audio pid or fmt
		if (dev->audio_switch == AM_TRUE)
		{
			aml_switch_ts_audio_fmt(dev, ts, tp);
			dev->audio_switch = AM_FALSE;
			adec_start = (adec_handle != NULL);
		}

		has_audio = VALID_AUDIO(tp->apid, tp->afmt);
		has_video = VALID_VIDEO(tp->vpid, tp->vfmt);
		if ((!has_audio) && (!has_video)) {
			AM_DEBUG(1,"Audio && Video is None");
			continue;
		} else {
			//AM_DEBUG(1,"Audio:%d or Video:%d is Running.",has_audio,has_video);
		}

		AM_TIME_GetClock(&now);
		if (has_audio && (ioctl(ts->fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != -1)) {
			abuf_size  = astatus.status.size;
			abuf_level = astatus.status.data_len;
			abuf_read_ptr = astatus.status.read_pointer;
		} else {
			if (has_audio)
				AM_DEBUG(1, "cannot get audio buffer status [%s]", strerror(errno));
			abuf_size  = 0;
			abuf_level = 0;
			abuf_read_ptr = 0;
		}

		if (has_video && (ioctl(ts->fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1)) {
			vbuf_size  = vstatus.status.size;
			vbuf_level = vstatus.status.data_len;
			vbuf_read_ptr = vstatus.status.read_pointer;
			//is_hd_video = vstatus.vstatus.width > 720;
		} else {
			if (has_video)
				AM_DEBUG(1, "cannot get video buffer status");
			vbuf_size  = 0;
			vbuf_level = 0;
			vbuf_read_ptr = 0;
		}

		if (vbuf_level == 0) {
			if(!vbuf_level_empty_time)
				vbuf_level_empty_time = now;
			vbuf_level_empty_dur = now - vbuf_level_empty_time;
		} else {
			vbuf_level_empty_time = 0;
			vbuf_level_empty_dur = 0;
		}
		if (abuf_level == 0) {
			if (!abuf_level_empty_time)
				abuf_level_empty_time = now;
			abuf_level_empty_dur = now - abuf_level_empty_time;
		} else {
			abuf_level_empty_time = 0;
			abuf_level_empty_dur = 0;
		}
		if (abuf_read_ptr == last_abuf_read_ptr) {
			if (!arp_stop_time)
				arp_stop_time = now;
			arp_stop_dur = now - arp_stop_time;
		} else {
			arp_stop_time = 0;
			arp_stop_dur  = 0;
		}
		last_abuf_read_ptr = abuf_read_ptr;

		if (vbuf_read_ptr == last_vbuf_read_ptr) {
			if(!vrp_stop_time)
				vrp_stop_time = now;
			vrp_stop_dur = now - vrp_stop_time;
		} else {
			vrp_stop_time = 0;
			vrp_stop_dur  = 0;
		}
		last_vbuf_read_ptr = vbuf_read_ptr;
		//check video frame available
		if (has_video && !no_video_data) {
#ifdef ANDROID
			if (AM_FileRead(VIDEO_NEW_FRAME_COUNT_FILE, buf, sizeof(buf)) >= 0) {
				sscanf(buf, "%i", &vframes_now);
				if ((vframes_now >= 1) ) {
					if (no_video) {
						AM_DEBUG(1, "[avmon] video available SwitchSourceTime = %fs",getUptimeSeconds());
						AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_AVAILABLE, NULL);
					}
					no_video = AM_FALSE;
					} else {
						no_video = AM_TRUE;
					}
					vframes_last = vframes_now;
				} else {
				AM_DEBUG(1, "[avmon] cannot read \"%s\"", VIDEO_NEW_FRAME_TOGGLED_FILE);
				vframes_now = 0;
			}
#endif
		}

		if (has_video)
		{
			memset(&vstatus, 0, sizeof(vstatus));
			if (ioctl(ts->fd, AMSTREAM_IOC_VDECSTAT, (unsigned long)&vstatus) != -1) {
				is_hd_video = (vstatus.vstatus.width > 720)? 1 : 0;
				vdec_status = vstatus.vstatus.status;
				frame_width = vstatus.vstatus.width;
				frame_height= vstatus.vstatus.height;
				aspect_ratio = vstatus.vstatus.euAspectRatio;
				//AM_DEBUG(1, "vdec width %d height %d status 0x%08x", frame_width, frame_height, vdec_status);
				if (!no_video) {
					if (frame_width != frame_width_old || frame_height != frame_height_old) {
						AM_DEBUG(1, "[avmon] video resolution changed: %dx%d -> %dx%d",
							frame_width_old, frame_height_old,
							frame_width, frame_height);
						frame_width_old = frame_width;
						frame_height_old = frame_height;
						{
							AM_AV_VideoStatus_t vstatus;
							memset(&vstatus, 0, sizeof(vstatus));
							vstatus.src_w = frame_width;
							vstatus.src_h = frame_height;
							AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_RESOLUTION_CHANGED, &vstatus);
						}
					}
					if (aspect_ratio != aspect_ratio_old) {
						AM_DEBUG(1, "[avmon] video aspect ratio changed: %d -> %d(kernel's definition)",
							aspect_ratio_old, aspect_ratio);
						aspect_ratio_old = aspect_ratio;
						AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, (void*)convert_aspect_ratio(aspect_ratio));
					}
				}
			} else {
				vdec_status = 0;
				frame_width = 0;
				frame_height= 0;
			}
		}
		// if (AM_FileRead(AVS_PLUS_DECT_FILE, buf, sizeof(buf)) >= 0) {
		// 	sscanf(buf, "%d", &avs_fmt);
		// } else {
		// 	//AM_DEBUG(1, "cannot read \"%s\"", AVS_PLUS_DECT_FILE);
		// 	avs_fmt = 0;
		// }
		avs_fmt = 0;
		//AM_DEBUG(1, "avs_fmt: \"%x\"", avs_fmt);
		if (avs_fmt == 0x148) //bit8
			is_avs_plus = AM_TRUE;
		else
			is_avs_plus = AM_FALSE;

		if (AM_FileRead(AUDIO_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf+2, "%x", &apts);
		} else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", AUDIO_PTS_FILE);
			apts = 0;
		}

		if (AM_FileRead(VIDEO_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf+2, "%x", &vpts);
		} else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", VIDEO_PTS_FILE);
			vpts = 0;
		}

		if (AM_FileRead(TSYNC_MODE_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%d", &tsync_mode);
		} else {
			tsync_mode = 1;
		}

		if (tsync_mode == 0) {
			if (vmaster_time == 0) {
				vmaster_time = now;
			}
			vmaster_dur = now - vmaster_time;
		} else {
			vmaster_time = 0;
			vmaster_dur = 0;
			has_amaster = AM_TRUE;
		}

		if (AM_FileRead(AUDIO_DMX_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%u", &dmx_apts);
		} else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", AUDIO_DMX_PTS_FILE);
			dmx_apts = 0;
		}

		if (AM_FileRead(VIDEO_DMX_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%u", &dmx_vpts);
		} else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", VIDEO_DMX_PTS_FILE);
			dmx_vpts = 0;
		}

		if (AM_FileRead(TSYNC_FIRSTCHECKIN_APTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "0x%x", &checkin_firstapts);
		} else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", TSYNC_FIRSTCHECKIN_APTS_FILE);
			checkin_firstapts = 0;
		}

		if (AM_FileRead(TSYNC_FIRSTCHECKIN_VPTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "0x%x", &checkin_firstvpts);
		} else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", TSYNC_FIRSTCHECKIN_VPTS_FILE);
			checkin_firstvpts = 0;
		}

		if (apts == last_apts) {
			if (!apts_stop_time)
				apts_stop_time = now;
			apts_stop_dur = now - apts_stop_time;
		} else {
			last_apts = apts;
			apts_stop_time = 0;
			apts_stop_dur = 0;
			apts_stop_time = 0;
		}

		if (vpts == last_vpts) {
			if(!vpts_stop_time)
				vpts_stop_time = now;
			vpts_stop_dur = now - vpts_stop_time;
		} else {
			last_vpts = vpts;
			vpts_stop_time = 0;
			vpts_stop_dur = 0;
			vpts_stop_time = 0;
		}

		if (dmx_apts == last_dmx_apts) {
			if(!dmx_apts_stop_time)
				dmx_apts_stop_time = now;
			dmx_apts_stop_dur = now - dmx_apts_stop_time;
		} else {
			last_dmx_apts = dmx_apts;
			dmx_apts_stop_dur = 0;
			dmx_apts_stop_time = 0;
		}

		if (dmx_vpts == last_dmx_vpts) {
			if (!dmx_vpts_stop_time)
				dmx_vpts_stop_time = now;
			dmx_vpts_stop_dur = now - dmx_vpts_stop_time;
		} else {
			last_dmx_vpts = dmx_vpts;
			dmx_vpts_stop_dur = 0;
			dmx_vpts_stop_time = 0;
		}

#if 0
		AM_DEBUG(3, "audio level:%d cache:%d, video level:%d cache:%d, resample:%d",
				abuf_level, adec_start ? dmx_apts - apts : dmx_apts - first_dmx_apts,
				vbuf_level, vpts ? dmx_vpts - vpts : dmx_vpts - first_dmx_vpts,
				resample_type);
#endif

#ifndef ENABLE_PCR
		if (has_audio && !adec_start) {
			adec_start = AM_TRUE;

			if (abuf_level < ADEC_START_AUDIO_LEVEL)
				adec_start = AM_FALSE;

			if (has_video) {
				if (vbuf_level < ADEC_START_VIDEO_LEVEL) {
					adec_start = AM_FALSE;
				}
			}

			if (abuf_level >= ADEC_FORCE_START_AUDIO_LEVEL)
				adec_start = AM_TRUE;

			if (adec_start) {
				audio_info_t info;

				/*Set audio info*/
				memset(&info, 0, sizeof(info));
				info.valid  = 1;
				ioctl(ts->fd, AMSTREAM_IOC_AUDIO_INFO, (unsigned long)&info);

				audio_ops->adec_start_decode(ts->fd, tp->afmt, has_video, &ts->adec);
				if (VALID_PID(tp->sub_apid))
					aml_set_audio_ad(dev, 1, tp->sub_apid, tp->sub_afmt);

				if (av_paused) {
					audio_ops->adec_pause_decode(ts->adec);
				}

				audio_scrambled = AM_FALSE;
				video_scrambled = AM_FALSE;
				resample_type = 0;
				next_resample_type = resample_type;
				next_resample_start_time = 0;
				down_audio_cache_time = 0;
				down_video_cache_time = 0;
				AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
				AM_FileEcho(RESAMPLE_TYPE_FILE, "0");

				AM_DEBUG(1, "[avmon] start audio decoder vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (!av_paused) {
			if (has_video && (vbuf_level < DEC_STOP_VIDEO_LEVEL))
				av_paused = AM_TRUE;
			if (has_audio && adec_start && (abuf_level < DEC_STOP_AUDIO_LEVEL))
				av_paused = AM_TRUE;

			if (av_paused) {
				if (has_audio && adec_start) {
					audio_ops->adec_pause_decode(ts->adec);
				}
				if (has_video) {
					ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 1);
				}

				AM_DEBUG(1, "[avmon] pause av play vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (av_paused) {
			av_paused = AM_FALSE;

			if (has_video && (vbuf_level < ADEC_START_VIDEO_LEVEL))
				av_paused = AM_TRUE;
			if (has_audio && (abuf_level < ADEC_START_AUDIO_LEVEL))
				av_paused = AM_TRUE;

			if (!av_paused) {
				if (has_audio && adec_start) {
					audio_ops->adec_resume_decode(ts->adec);
				}
				if (has_video) {
					ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 0);
				}
				apts_stop_time = 0;
				vpts_stop_time = 0;
				resample_type = 0;
				next_resample_type = resample_type;
				next_resample_start_time = 0;
				down_audio_cache_time = 0;
				down_video_cache_time = 0;
				AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
				AM_FileEcho(RESAMPLE_TYPE_FILE, "0");
				AM_DEBUG(1, "[avmon] resume av play vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (has_audio && adec_start && !av_paused) {
			AM_Bool_t af = AM_FALSE, vf = AM_FALSE;
			int resample = 0;

			if (has_audio && (abuf_level < UP_RESAMPLE_AUDIO_LEVEL))
				resample = 2;
			if (has_video && (vbuf_level < UP_RESAMPLE_VIDEO_LEVEL))
				resample = 2;

			if (has_audio && dmx_apts && apts) {
				if (down_audio_cache_time == 0) {
					down_audio_cache_time = dmx_apts - apts;
					if (down_audio_cache_time < DOWN_RESAMPLE_CACHE_TIME)
						down_audio_cache_time = DOWN_RESAMPLE_CACHE_TIME;
					else
						down_audio_cache_time *= 2;
				}
				if (has_audio && (dmx_apts - apts > down_audio_cache_time))
					af = AM_TRUE;
			}

			if (has_video && dmx_vpts && vpts) {
				if (down_video_cache_time == 0) {
					down_video_cache_time = dmx_vpts - vpts;
					if (down_video_cache_time < DOWN_RESAMPLE_CACHE_TIME)
						down_video_cache_time = DOWN_RESAMPLE_CACHE_TIME;
					else
						down_video_cache_time *= 2;
				}
				if (has_video && (dmx_vpts - vpts > down_video_cache_time))
					vf = AM_TRUE;
			}

			if (af && vf)
				resample = 1;

			if (has_audio && (abuf_level * 5 > abuf_size * 4))
				resample = 1;

			if (has_video && (vbuf_level * 5 > vbuf_size * 4))
				resample = 1;

#ifdef ENABLE_AUDIO_RESAMPLE
			if (resample != resample_type) {
				if (resample != next_resample_type) {
					next_resample_type = resample;
					next_resample_start_time = now;
				}

				if (now - next_resample_start_time > 500) {
					const char *cmd = "";

					switch (resample) {
						case 1: cmd = "1"; break;
						case 2: cmd = "2"; break;
						default: cmd = "0"; break;
					}
#ifdef ENABLE_AUDIO_RESAMPLE
					AM_FileEcho(ENABLE_RESAMPLE_FILE, resample?"1":"0");
					AM_FileEcho(RESAMPLE_TYPE_FILE, cmd);
					AM_DEBUG(1, "[avmon] audio resample %d vlevel %d alevel %d",
						resample, vbuf_level, abuf_level);
					resample_type = resample;
#endif
					next_resample_start_time = 0;
				}
			}
#endif
		}

#else /*defined(ENABLE_PCR)*/
		if (has_audio && !adec_start) {
			adec_start = AM_TRUE;

			if (abuf_level < ADEC_START_AUDIO_LEVEL)
				adec_start = AM_FALSE;

			if (has_video) {
				if (vbuf_level < ADEC_START_VIDEO_LEVEL) {
					adec_start = AM_FALSE;
				}
			}

			if (abuf_level >= ADEC_FORCE_START_AUDIO_LEVEL)
				adec_start = AM_TRUE;

			if (adec_start) {
				audio_ops->adec_start_decode(ts->fd, tp->afmt, has_video, &ts->adec);
				AM_DEBUG(1, "[avmon] start audio decoder vlevel %d alevel %d", vbuf_level, abuf_level);
				if (VALID_PID(tp->sub_apid))
				    aml_set_audio_ad(dev, 1, tp->sub_apid, tp->sub_afmt);
			}
		}
#endif /*!defined ENABLE_PCR*/

#ifdef USE_ADEC_IN_DVB
            if (s_audio_cb == NULL) {
		        int status = audio_decoder_get_enable_status(ts->adec);
				if (status == 1) {
					AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AUDIO_AC3_LICENCE_RESUME, NULL);
				}
				else if (status == 0) {
					AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AUDIO_AC3_NO_LICENCE, NULL);
				}
				else if (status == -1) {

				}
	        }
#endif

#ifdef ENABLE_BYPASS_DI
		if (has_video && is_hd_video && !bypass_di && (vbuf_level * 6 > vbuf_size * 5)) {
			AM_FileEcho(DI_BYPASS_FILE, "1");
			bypass_di = AM_TRUE;
			AM_DEBUG(1, "[avmon] bypass HD deinterlace vlevel %d", vbuf_level);
		}
#endif

#ifdef ENABLE_DROP_BFRAME
		if (has_video && is_hd_video && !drop_b_frame) {
			if (vbuf_level * 6 > vbuf_size * 5) {
				drop_b_frame = AM_TRUE;
			}

			if (has_audio && adec_start && has_amaster && AM_ABS(apts - vpts) > 45000) {
				drop_b_frame = AM_TRUE;
			}

			if (drop_b_frame) {
				AM_FileEcho(VIDEO_DROP_BFRAME_FILE ,"1");
				AM_DEBUG(1, "[avmon] drop B frame vlevel %d", vbuf_level);
			}
		}
#endif

		//first no_data
		if (has_audio && adec_start && !no_audio_data && (dmx_apts_stop_dur > NO_DATA_CHECK_TIME) && (arp_stop_dur > NO_DATA_CHECK_TIME)) {
			AM_Bool_t sf[2];
			AM_DEBUG(1, "[avmon] audio stoped");
			no_audio_data = AM_TRUE;
			//second scramble
			if (abuf_level_empty_dur > SCRAMBLE_CHECK_TIME) {
				AM_DMX_GetScrambleStatus(0, sf);
				if (sf[1]) {
					audio_scrambled = AM_TRUE;
					if (!no_data_evt || g_festatus) {
						AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AUDIO_SCAMBLED, NULL);
						AM_DEBUG(1, "[avmon] audio scrambled > stoped");
						no_data_evt = AM_TRUE;
						g_festatus = 0;
					}
				}
			}
		}

		if (has_video && (dmx_vpts_stop_dur > NO_DATA_CHECK_TIME) && (vpts_stop_dur > NO_DATA_CHECK_TIME)) {
			AM_Bool_t sf[2];
			no_video_data = AM_TRUE;
			no_video = AM_TRUE;
			AM_DEBUG(1, "[avmon] video data stopped");
			if (vbuf_level_empty_dur > SCRAMBLE_CHECK_TIME) {
				AM_DMX_GetScrambleStatus(0, sf);
				if (sf[0]) {
					video_scrambled = AM_TRUE;
					if (!no_data_evt || g_festatus) {
						AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_SCAMBLED, NULL);
						AM_DEBUG(1, "[avmon] video scrambled");
						no_data_evt = AM_TRUE;
						g_festatus = 0;
					}
				}
			}
		} else if (is_avs_plus &&(tp->vfmt == VFORMAT_AVS)) {
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_NOT_SUPPORT, NULL);//not  unsupport , just  FORMAT  is AVS
		}
        //this Radio Program Stop
        if (has_audio && !has_video && adec_start && !no_audio_data && (dmx_apts_stop_dur > AUDIO_NO_DATA_TIME) && (arp_stop_dur > AUDIO_NO_DATA_TIME)) {
            AM_DEBUG(1, "[avmon] radio program data stopped");
            no_audio_data = AM_TRUE;
        }
		//add (vpts_stop_dur > NO_DATA_CHECK_TIME) for first into thread,because no_audio_data no_video_data
		//init value is true and no_data_evt init value is false.
		if (no_audio_data && no_video_data && !no_data_evt && vpts_stop_dur > NO_DATA_CHECK_TIME) {
			no_data_evt = AM_TRUE;
			AM_DEBUG(1, "[avmon] send no video data signal");
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_NO_DATA, NULL);
		}

		//when no signal, switch channel, it can trigger no data event
		if (has_audio && !adec_start && no_video_data && !no_data_evt && vpts_stop_dur > NO_DATA_CHECK_TIME) {
			no_data_evt = AM_TRUE;
			AM_DEBUG(1, "[avmon] send no AV data signal");
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_NO_DATA, NULL);
		}

		//AM_DEBUG(3,"no_audio = %d, dmx_a_stop = %d, a_stop= %d, no_video=%d, dmx_v_stop=%d, v_stop=%d, abuf_empty=%d, vbuf_empty=%d\n",no_audio_data,dmx_apts_stop_dur,apts_stop_dur, no_video_data, dmx_vpts_stop_dur, vpts_stop_dur, abuf_level_empty_dur,vbuf_level_empty_dur);

		if (has_audio && no_audio_data && dmx_apts_stop_dur == 0) {
			no_audio_data = AM_FALSE;
			audio_scrambled = AM_FALSE;
			no_data_evt = AM_FALSE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_DATA_RESUME, NULL);
			AM_DEBUG(1, "[avmon] audio data resumed");
		}

		if (has_video && no_video_data && dmx_vpts_stop_dur == 0) {
			no_video_data = AM_FALSE;
			video_scrambled = AM_FALSE;
			no_data_evt = AM_FALSE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_DATA_RESUME, NULL);
			AM_DEBUG(1, "[avmon] video data resumed");
		}

		/*AM_DEBUG(3, "apts_dmx_stop: %d arp_stop: %d vpts_dmx_stop: %d vrp_stop: %d",
					dmx_apts_stop_dur, arp_stop_dur, dmx_vpts_stop_dur, vrp_stop_dur);*/
		need_replay = AM_FALSE;
		if (!no_video_data && /*!av_paused &&*/ (dmx_vpts_stop_dur == 0) && (vrp_stop_dur > NO_DATA_CHECK_TIME))
		{
			if (vdec_stop_time == 0) {
				vdec_stop_time = now;
				vdec_stop_dur  = 0;
			} else {
				vdec_stop_dur = now - vdec_stop_time;
			}
		} else {
			vdec_stop_time = 0;
			vdec_stop_dur  = 0;
		}

		if (dev->mode != AV_TIMESHIFT
			&& (dev->mode != AV_INJECT)
			&& (has_video && (vdec_stop_dur > NO_DATA_CHECK_TIME))) {
			if (AM_ABS(checkin_firstapts - checkin_firstvpts) < TIME_UNIT90K * 5)
			if (dev->replay_enable)
				need_replay = AM_TRUE;
			AM_DEBUG(1, "[avmon] should't replay vdec_stop_dur=%d\n",
				vdec_stop_dur);
			AM_DEBUG(1, "[avmon] apts_dmx_stop: %d arp_stop: %d vpts_dmx_stop: %d vrp_stop: %d",
					dmx_apts_stop_dur, arp_stop_dur, dmx_vpts_stop_dur, vrp_stop_dur);
		}
		if (dev->mode == AV_PLAY_TS) {
			if (has_video && (vbuf_level * 5 > vbuf_size * 4))
			{
				if (dev->replay_enable)
					need_replay = AM_TRUE;
				AM_DEBUG(1, "[avmon] 1 shouldn't replay ts vlevel %d vbuf_size %d",vbuf_level*6, vbuf_size*5);
			}
			if (has_audio && (abuf_level * 5 > abuf_size * 4))
			{
				if (dev->replay_enable)
					need_replay = AM_TRUE;
				AM_DEBUG(1, "[avmon] 2 shouldn't replay ts alevel %d abuf_size %d",abuf_level*6, abuf_size*5);
			}
		}
		//if(adec_start && !av_paused && has_amaster && !apts_stop_dur && !vpts_stop_dur && (vmaster_dur > VMASTER_REPLAY_TIME))
			//need_replay = AM_TRUE;
		//AM_DEBUG(0, "vdec status %08x", vdec_status);
#ifdef DECODER_FATAL_ERROR_SIZE_OVERFLOW
		if (has_video && (tp->vfmt == VFORMAT_H264) && (vdec_status & DECODER_FATAL_ERROR_SIZE_OVERFLOW)) {
			AM_DEBUG(1, "[avmon] switch to h264 4K/2K");
			tp->vfmt = VFORMAT_H264_4K2K;
			AM_DEBUG(1, "DECODER_FTAL_ERROR_SIZE_OVERFLOW, vdec_status=0x%x\n",
				vdec_status);
			if (dev->replay_enable)
				need_replay = AM_TRUE;
		} else
#endif
		if (has_video && (tp->vfmt == VFORMAT_H264) && ((vdec_status >> 16) & 0xFFFF)) {
			AM_DEBUG(1, "[avmon] H264 fatal error,vdec_status=0x%x",
				vdec_status);
			if (dev->replay_enable)
				need_replay = AM_TRUE;
		}
#ifndef USE_ADEC_IN_DVB
#ifdef ANDROID
		if (AM_FileRead(TSYNCPCR_RESETFLAG_FILE, buf, sizeof(buf)) >= 0) {
			int val = 0;
			sscanf(buf, "%d", &val);
			if (val == 1) {
				if (dev->replay_enable)
					need_replay = AM_TRUE;
				AM_DEBUG(1, "[avmon] pcr need reset");
			}
		}

		//AM_DEBUG(1, "tsync_mode:%d--vbuf_level--0x%08x---- abuf_level---0x%08x",
		//	tsync_mode,vbuf_level,abuf_level);
#endif
#endif
		if (!av_paused && dev->mode == AV_INJECT) {
			if (has_video && (vbuf_level < DEC_STOP_VIDEO_LEVEL))
				av_paused = AM_TRUE;
			if (has_audio && adec_start && (abuf_level < DEC_STOP_AUDIO_LEVEL))
				av_paused = AM_TRUE;

			if (av_paused) {
				if (has_audio && adec_start) {
					AM_DEBUG(1, "[avmon] AV_INJECT audio pause");
					audio_ops->adec_pause_decode(ts->adec);
				}
				if (has_video) {
					AM_DEBUG(1, "[avmon] AV_INJECT video pause");
					ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 1);
				}
				//AM_DEBUG(1, "[avmon] pause av play vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (av_paused && dev->mode == AV_INJECT) {
			av_paused = AM_FALSE;
			if (has_audio && (abuf_level < (ADEC_START_AUDIO_LEVEL/2)))
				av_paused = AM_TRUE;
			if (has_video && (vbuf_level < (ADEC_START_VIDEO_LEVEL*16))) {
				av_paused = AM_TRUE;
			} else {
				av_paused = AM_FALSE;
			}

			if (!av_paused) {
				if (has_audio && adec_start) {
					AM_DEBUG(1, "[avmon] AV_INJECT audio resume");
					audio_ops->adec_resume_decode(ts->adec);
				}
				if (has_video) {
					AM_DEBUG(1, "[avmon] AV_INJECT video resume");
					ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 0);
				}
				//AM_DEBUG(1, "[avmon] resume av play vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (has_audio && (apts - dmx_apts) > TIME_UNIT90K*2) {
			apts_discontinue = 1;
			AM_DEBUG(1, "[avmon] dmx_apts:0x%x,apts:0x%x",dmx_apts,apts);
		}

		if (has_video && (vpts - dmx_vpts) > TIME_UNIT90K*2) {
			vpts_discontinue = 1;
			AM_DEBUG(1, "[avmon] dmx_vpts:0x%x,vpts:0x%x",dmx_vpts,vpts);
		}

		if (AM_FileRead(TSYNC_PCR_MODE_FILE, buf, sizeof(buf)) >= 0) {
            sscanf(buf, "%x", &tsync_pcr_mode);
        } else {
			AM_DEBUG(1, "[avmon] cannot read \"%s\"", TSYNC_PCR_MODE_FILE);
            tsync_pcr_mode = 0;
        }

		if (apts_discontinue && vpts_discontinue && tsync_pcr_mode == 1) {
			AM_Bool_t sf[2];
			AM_DEBUG(1, "[avmon] apts_discontinue vpts_discontinue replay %d",need_replay);
			apts_discontinue = 0;
			vpts_discontinue = 0;

			AM_DMX_GetScrambleStatus(0, sf);
			if (sf[0] == 0 && sf[1] == 0) {
				if (dev->replay_enable)
					need_replay = AM_TRUE;
				AM_DEBUG(1, "[avmon] sf[0]=sf[1]=0");
			}
		}

		//if (need_replay && (AM_ABS(checkin_firstapts - checkin_firstvpts) > TIME_UNIT90K * 5)) {
		//	AM_DEBUG(1, "[avmon] avoid replay checkin_firstapts checkin_firstvpts %d",need_replay);
		//	need_replay = AM_FALSE;
		//}

		if (dev->mode == AV_TIMESHIFT
			&& (
				(has_audio && apts_discontinue
					&& (abuf_level > (DEC_STOP_AUDIO_LEVEL*2))
					&& (arp_stop_dur > NO_DATA_CHECK_TIME))
				||
				(has_video && no_video_data
					&& (vbuf_level > (DEC_STOP_VIDEO_LEVEL*2))
					&& (vrp_stop_dur > NO_DATA_CHECK_TIME))
				)
			) {
			AM_DEBUG(1, "[avmon] not replay timeshift adec stuck or vdec stuck");
			if (dev->replay_enable)
				need_replay = AM_TRUE;
		}

		{
			int r2;
			do {
				struct timespec rt2;
				aml_get_timeout_real(20, &rt2);
				r2 = pthread_mutex_timedlock(&dev->lock, &rt2);
			} while (mon->av_thread_running && (r2 == ETIMEDOUT));

			if (!mon->av_thread_running) {
				if (r2 == 0)
					pthread_mutex_unlock(&dev->lock);
				break;
			}
		}

		AM_TIME_GetClock(&cur_time);
		if (need_replay && (dev->mode == AV_PLAY_TS) && (AM_ABS(cur_time - last_replay_time) > REPLAY_TIME_INTERVAL)) {
			AM_DEBUG(1, "[avmon] replay ts vlevel %d alevel %d vpts_stop %d vmaster %d",
				vbuf_level, abuf_level, vpts_stop_dur, vmaster_dur);
			aml_restart_ts_mode(dev, AM_FALSE);
			tp = &dev->ts_player.play_para;
			ts = (AV_TSData_t*)dev->ts_player.drv_data;
#ifndef ENABLE_PCR
			adec_start = AM_FALSE;
			av_paused  = AM_TRUE;
#else
			adec_start = (adec_handle != NULL);
			av_paused  = AM_FALSE;
#endif
			resample_type = 0;
			next_resample_type = resample_type;
			next_resample_start_time = 0;
			last_apts = 0;
			last_vpts = 0;
			last_dmx_apts = 0;
			last_dmx_vpts = 0;
			apts_stop_time = 0;
			vpts_stop_time = 0;
			dmx_apts_stop_time = 0;
			dmx_vpts_stop_time = 0;
			vmaster_time = 0;
			down_audio_cache_time = 0;
			down_video_cache_time = 0;
			vdec_stop_time = 0;
			vdec_stop_dur  = 0;
			has_amaster = AM_FALSE;
			AM_TIME_GetClock(&last_replay_time);
			replay_done = 1;
		}else if (need_replay && (dev->mode == AV_TIMESHIFT) && (AM_ABS(cur_time - last_replay_time) > REPLAY_TIME_INTERVAL)) {
			AM_DEBUG(1, "[avmon] replay timshift vlevel %d alevel %d vpts_stop %d vmaster %d",
				vbuf_level, abuf_level, vpts_stop_dur, vmaster_dur);
			AV_TimeshiftData_t *tshift = NULL;

			tshift = dev->timeshift_player.drv_data;
			am_timeshift_reset_continue(tshift, -1, AM_TRUE);
#ifndef ENABLE_PCR
			adec_start = AM_FALSE;
			av_paused  = AM_TRUE;
#else
			adec_start = (adec_handle != NULL);
			av_paused  = AM_FALSE;
#endif
			resample_type = 0;
			next_resample_type = resample_type;
			next_resample_start_time = 0;
			last_apts = 0;
			last_vpts = 0;
			last_dmx_apts = 0;
			last_dmx_vpts = 0;
			apts_stop_time = 0;
			vpts_stop_time = 0;
			dmx_apts_stop_time = 0;
			dmx_vpts_stop_time = 0;
			vmaster_time = 0;
			down_audio_cache_time = 0;
			down_video_cache_time = 0;
			vdec_stop_time = 0;
			vdec_stop_dur  = 0;
			has_amaster = AM_FALSE;
			AM_TIME_GetClock(&last_replay_time);
			replay_done = 1;
		}
		else if (need_replay && (dev->mode == AV_INJECT) && (AM_ABS(cur_time - last_replay_time) > REPLAY_TIME_INTERVAL)) {
			AM_DEBUG(1, "[avmon] replay AV_INJECT vlevel %d alevel %d vpts_stop %d vmaster %d",
				vbuf_level, abuf_level, vpts_stop_dur, vmaster_dur);
			aml_restart_inject_mode(dev, AM_FALSE);
			tp = &dev->ts_player.play_para;
			inj = (AV_InjectData_t *)dev->inject_player.drv_data;
			ts_temp.fd = inj->vid_fd;
			ts_temp.vid_fd = inj->cntl_fd;
			ts_temp.adec = inj->adec;
			ts = &ts_temp;
#ifndef ENABLE_PCR
			adec_start = AM_FALSE;
			av_paused  = AM_TRUE;
#else
			adec_start = (adec_handle != NULL);
			av_paused  = AM_FALSE;
#endif
			resample_type = 0;
			next_resample_type = resample_type;
			next_resample_start_time = 0;
			last_apts = 0;
			last_vpts = 0;
			last_dmx_apts = 0;
			last_dmx_vpts = 0;
			apts_stop_time = 0;
			vpts_stop_time = 0;
			dmx_apts_stop_time = 0;
			dmx_vpts_stop_time = 0;
			vmaster_time = 0;
			down_audio_cache_time = 0;
			down_video_cache_time = 0;
			vdec_stop_time = 0;
			vdec_stop_dur  = 0;
			has_amaster = AM_FALSE;
			AM_TIME_GetClock(&last_replay_time);
			replay_done = 1;
		}

		pthread_mutex_unlock(&dev->lock);

		if (replay_done) {
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_PLAYER_STATE_CHANGED, NULL);
			replay_done = 0;
		}

	}

	pthread_mutex_unlock(&gAVMonLock);

#ifndef ENABLE_PCR
	if (resample_type) {
		AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
	}
#endif

	if (dev->mode != AV_TIMESHIFT)
		AM_FileEcho(VID_BLACKOUT_FILE, dev->video_blackout ? "1" : "0");

	if (bypass_di) {
		AM_FileEcho(DI_BYPASS_FILE, "0");
	}

	if (drop_b_frame) {
		AM_FileEcho(VIDEO_DROP_BFRAME_FILE, "0");
	}

	AM_DEBUG(1, "[avmon] AV monitor thread exit now");
	return NULL;
}

static AM_ErrorCode_t aml_open_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	AV_DataSource_t *src;
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
#endif
	//AV_JPEGData_t *jpeg;
	AV_InjectData_t *inj;
	AV_TimeshiftData_t *tshift;
	AV_TSData_t *ts;
	int fd;

	AM_DebugSetLogLevel(_get_dvb_loglevel());

	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			src = aml_create_data_source(STREAM_VBUF_FILE, dev->dev_no, AM_FALSE);
			if (!src)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amstream_vbuf\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}

			fd = open(AMVIDEO_FILE, O_RDWR);
			if (fd == -1)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amvideo\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			ioctl(fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_I);
			src->video_fd = fd;
			dev->vid_player.drv_data = src;

		break;
		case AV_PLAY_AUDIO_ES:
			src = aml_create_data_source(STREAM_ABUF_FILE, dev->dev_no, AM_TRUE);
			if (!src)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amstream_abuf\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			dev->aud_player.drv_data = src;
		break;
		case AV_PLAY_TS:
			if (aml_open_ts_mode(dev) != AM_SUCCESS)
				return AM_AV_ERR_SYS;
		break;
		case AV_PLAY_FILE:
#ifdef PLAYER_API_NEW
			data = aml_create_fp(dev);
			if (!data)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->file_player.drv_data = data;
#endif
		break;
		// case AV_GET_JPEG_INFO:
		// case AV_DECODE_JPEG:
		// 	jpeg = aml_create_jpeg_data();
		// 	if (!jpeg)
		// 	{
		// 		AM_DEBUG(1, "not enough memory");
		// 		return AM_AV_ERR_NO_MEM;
		// 	}
		// 	dev->vid_player.drv_data = jpeg;
		// break;
		case AV_INJECT:
			inj = aml_create_inject_data();
			if (!inj)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->inject_player.drv_data = inj;
		break;
		case AV_TIMESHIFT:
			tshift = aml_create_timeshift_data();
			if (!tshift)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			tshift->dev = dev;
			dev->timeshift_player.drv_data = tshift;
		break;
		default:
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_start_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para)
{
	AV_DataSource_t *src;
	int fd, val;
	AV_TSPlayPara_t *tp;
	AV_TSData_t *ts;
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	AV_FilePlayPara_t *pp;
#endif
	// AV_JPEGData_t *jpeg;
	AV_InjectPlayPara_t inj_p;
	AV_InjectData_t *inj;
	AV_TimeShiftPlayPara_t *tshift_p;
	AV_TimeshiftData_t *tshift;

	int ctrl_fd = open(AMVIDEO_FILE, O_RDWR);
	if (ctrl_fd >= 0) {
		ioctl(ctrl_fd, AMSTREAM_IOC_SET_VSYNC_UPINT, 0);
		close(ctrl_fd);
	} else {
		AM_DEBUG(1, "open /dev/amvideo error");
	}
#ifdef ANDROID
	dev->replay_enable = property_get_int32(REPLAY_ENABLE_PROP, 0);
	AM_DEBUG(1, "set replay_enable=%d\n", dev->replay_enable);
#endif
	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			src = dev->vid_player.drv_data;
			val = (long)para;
			if (ioctl(src->fd, AMSTREAM_IOC_VFORMAT, val) == -1)
			{
				AM_DEBUG(1, "set video format failed");
				return AM_AV_ERR_SYS;
			}
			aml_start_data_source(src, dev->vid_player.para.data, dev->vid_player.para.len, dev->vid_player.para.times+1);
		break;
		case AV_PLAY_AUDIO_ES:
			src = dev->aud_player.drv_data;
			val = (long)para;
			if (ioctl(src->fd, AMSTREAM_IOC_AFORMAT, val) == -1)
			{
				AM_DEBUG(1, "set audio format failed");
				return AM_AV_ERR_SYS;
			}
			if (ioctl(src->fd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
			{
				AM_DEBUG(1, "amport init failed");
				return AM_AV_ERR_SYS;
			}
			if (ioctl(src->fd, AMSTREAM_IOC_TSTAMP, 0) == -1)
				AM_DEBUG(1, "checkin first apts failed [%s]", strerror(errno));
			AM_DEBUG(1, "aml start aes:  A[fmt:%d, loop:%d]", val, dev->aud_player.para.times);
			aml_start_data_source(src, dev->aud_player.para.data, dev->aud_player.para.len, dev->aud_player.para.times);
			audio_ops->adec_start_decode(src->fd, val, 0, &src->adec);
		break;
		case AV_PLAY_TS:
			tp = (AV_TSPlayPara_t *)para;
			setup_forced_pid(tp);
			dev->alt_apid = tp->apid;
			dev->alt_afmt = tp->afmt;
			tp->drm_mode = dev->curr_para.drm_mode;
#if defined(ANDROID) || defined(CHIP_8626X)
#ifdef ENABLE_PCR_RECOVER
			AM_FileEcho(PCR_RECOVER_FILE, "1");
#endif
#endif
			AM_TRY(aml_start_ts_mode(dev, tp, AM_TRUE));
		break;
		case AV_PLAY_FILE:
#ifdef PLAYER_API_NEW
			data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
			pp = (AV_FilePlayPara_t*)para;

			if (pp->start)
			{
				memset((void*)&data->pctl,0,sizeof(play_control_t));

				//player_register_update_callback(&data->pctl.callback_fn, aml_update_player_info_callback, PLAYER_INFO_POP_INTERVAL);

				data->pctl.file_name = strndup(pp->name,FILENAME_LENGTH_MAX);
				data->pctl.video_index = -1;
				data->pctl.audio_index = -1;
				data->pctl.hassub = 1;

				/*data->pctl.t_pos = st;*/
				aml_set_tsync_enable(1);

				if (pp->loop)
				{
					data->pctl.loop_mode =1;
				}
				data->pctl.need_start = 1;
				data->pctl.is_variable = 1;

				data->media_id = player_start(&data->pctl,0);

				if (data->media_id < 0)
				{
					AM_DEBUG(1, "play file failed");
					return AM_AV_ERR_SYS;
				}

				player_start_play(data->media_id);
				//AM_AOUT_SetDriver(AOUT_DEV_NO, &amplayer_aout_drv, (void*)(long)data->media_id);
			}
#endif
		break;
		// case AV_GET_JPEG_INFO:
		// case AV_DECODE_JPEG:
		// 	jpeg = dev->vid_player.drv_data;
		// 	return aml_decode_jpeg(jpeg, dev->vid_player.para.data, dev->vid_player.para.len, mode, para);
		// break;
		case AV_INJECT:
			memcpy(&inj_p, para, sizeof(AM_AV_InjectPara_t));
			inj = dev->inject_player.drv_data;
			inj_p.drm_mode = dev->curr_para.drm_mode;
			dev->alt_apid = inj_p.para.aud_id;
			dev->alt_afmt = inj_p.para.aud_fmt;
			if (aml_start_inject(dev, inj, &inj_p) != AM_SUCCESS)
			{
				AM_DEBUG(1,"[aml_start_mode]  AM_AV_ERR_SYS");
				return AM_AV_ERR_SYS;
			} else {
				dev->ts_player.play_para.afmt = inj->aud_fmt;
				dev->ts_player.play_para.apid = inj->aud_id;
				dev->ts_player.play_para.vfmt = inj->vid_fmt;
				dev->ts_player.play_para.vpid = inj->vid_id;
				dev->ts_player.play_para.sub_afmt = inj->sub_type;
				dev->ts_player.play_para.sub_apid = inj->sub_id;
				dev->ts_player.play_para.pcrpid = 0x1fff;
				aml_start_av_monitor(dev, &dev->ts_player.mon);
			}
		break;
		case AV_TIMESHIFT:
			tshift_p = (AV_TimeShiftPlayPara_t *)para;
			tshift = dev->timeshift_player.drv_data;
			m_tshift = tshift;
			tshift->tp.vpid = tshift_p->para.media_info.vid_pid;
			tshift->tp.vfmt = tshift_p->para.media_info.vid_fmt;
			tshift->tp.pcrpid = -1;
			if (tshift_p->para.media_info.aud_cnt > 0) {
				tshift->tp.apid = tshift_p->para.media_info.audios[0].pid;
				tshift->tp.afmt = tshift_p->para.media_info.audios[0].fmt;
			}
			setup_forced_pid(&tshift->tp);
			dev->alt_apid = tshift->tp.apid;
			dev->alt_afmt = tshift->tp.afmt;
			if (aml_start_timeshift(tshift, tshift_p, AM_TRUE, AM_TRUE) != AM_SUCCESS)
				return AM_AV_ERR_SYS;
		break;
		default:
		break;
	}
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_close_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	AV_DataSource_t *src;
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
#endif
	// AV_JPEGData_t *jpeg;
	AV_InjectData_t *inj;
	AV_TimeshiftData_t *tshift;
	int fd, ret;

	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			src = dev->vid_player.drv_data;
			ioctl(src->video_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
			close(src->video_fd);
			aml_destroy_data_source(src);
		break;
		case AV_PLAY_AUDIO_ES:
			src = dev->aud_player.drv_data;
			audio_ops->adec_stop_decode(&src->adec);
			aml_destroy_data_source(src);
		break;
		case AV_PLAY_TS:
#if defined(ANDROID) || defined(CHIP_8626X)
#ifdef ENABLE_PCR_RECOVER
			AM_FileEcho(PCR_RECOVER_FILE, "0");
#endif
#endif
		ret = aml_close_ts_mode(dev, AM_TRUE);
		break;
		case AV_PLAY_FILE:
#ifdef PLAYER_API_NEW
			data = (AV_FilePlayerData_t *)dev->file_player.drv_data;
			aml_destroy_fp(data);
			adec_cmd("stop");
#endif
		break;
		// case AV_GET_JPEG_INFO:
		// case AV_DECODE_JPEG:
		// 	jpeg = dev->vid_player.drv_data;
		// 	aml_destroy_jpeg_data(jpeg);
		// break;
		case AV_INJECT:
			aml_stop_av_monitor(dev, &dev->ts_player.mon);
			inj = dev->inject_player.drv_data;
			aml_destroy_inject_data(inj);
		break;
		case AV_TIMESHIFT:
			tshift = dev->timeshift_player.drv_data;
			aml_stop_timeshift(tshift, AM_TRUE);
			aml_destroy_timeshift_data(tshift);
			dev->timeshift_player.drv_data = NULL;
			m_tshift = NULL;
		break;
		default:
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_ts_source(AM_AV_Device_t *dev, AM_AV_TSSource_t src)
{
	char *cmd;

	UNUSED(dev);

	switch (src)
	{
		case AM_AV_TS_SRC_TS0:
			cmd = "ts0";
		break;
		case AM_AV_TS_SRC_TS1:
			cmd = "ts1";
		break;
#if defined(CHIP_8226M) || defined(CHIP_8626X)
		case AM_AV_TS_SRC_TS2:
			cmd = "ts2";
		break;
#endif
		case AM_AV_TS_SRC_HIU:
			cmd = "hiu";
		break;
		case AM_AV_TS_SRC_HIU1:
			cmd = "hiu1";
		break;
		case AM_AV_TS_SRC_DMX0:
			cmd = "dmx0";
		break;
		case AM_AV_TS_SRC_DMX1:
			cmd = "dmx1";
		break;
#if defined(CHIP_8226M) || defined(CHIP_8626X)
		case AM_AV_TS_SRC_DMX2:
			cmd = "dmx2";
		break;
#endif
		default:
			AM_DEBUG(1, "illegal ts source %d", src);
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}

	return AM_FileEcho(DVB_STB_SOURCE_FILE, cmd);
}

static AM_ErrorCode_t aml_file_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	AV_FileSeekPara_t *sp;
	int rc = -1;

	data = (AV_FilePlayerData_t *)dev->file_player.drv_data;

	if (data->media_id < 0)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}

	switch(cmd)
	{
		case AV_PLAY_START:
			player_start_play(data->media_id);
		break;
		case AV_PLAY_PAUSE:
			player_pause(data->media_id);
		break;
		case AV_PLAY_RESUME:
			player_resume(data->media_id);
		break;
		case AV_PLAY_FF:
			player_forward(data->media_id, (long)para);
		break;
		case AV_PLAY_FB:
			player_backward(data->media_id, (long)para);
		break;
		case AV_PLAY_SEEK:
			sp = (AV_FileSeekPara_t *)para;
			 player_timesearch(data->media_id, sp->pos);
		break;
		default:
			AM_DEBUG(1, "illegal media player command");
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}
#endif

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_inject_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
	AV_InjectData_t *data;

	UNUSED(para);

	data = (AV_InjectData_t *)dev->inject_player.drv_data;

	switch (cmd)
	{
		case AV_PLAY_PAUSE:
			if (data->aud_id != -1)
			{
#if defined(ADEC_API_NEW)
				audio_ops->adec_pause_decode(data->adec);
#else
				//TODO
#endif
			}
			if (data->vid_fd != -1 && data->cntl_fd != -1)
			{
				ioctl(data->cntl_fd, AMSTREAM_IOC_VPAUSE, 1);
			}
			AM_DEBUG(1, "pause inject");
		break;
		case AV_PLAY_RESUME:
			if (data->aud_id != -1)
			{
#if defined(ADEC_API_NEW)
				audio_ops->adec_resume_decode(data->adec);
#else
				//TODO
#endif
			}
			if (data->vid_fd != -1 && data->cntl_fd != -1)
			{
				ioctl(data->cntl_fd, AMSTREAM_IOC_VPAUSE, 0);
			}
			AM_DEBUG(1, "resume inject");
		break;
		default:
			AM_DEBUG(1, "illegal media player command");
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_file_status(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status)
{
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	int rc;

	data = (AV_FilePlayerData_t *)dev->file_player.drv_data;
	if (data->media_id == -1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}

	player_info_t pinfo;

	if (player_get_play_info(data->media_id,&pinfo) < 0)
	{
		AM_DEBUG(1, "player_get_play_info failed");
		return AM_AV_ERR_SYS;
	}
	status->duration = pinfo.full_time;
	status->position = pinfo.current_time;
#endif
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_file_info(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info)
{
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	int rc;

	data = (AV_FilePlayerData_t *)dev->file_player.drv_data;
	if (data->media_id == -1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}

	media_info_t minfo;

	if (player_get_media_info(data->media_id,&minfo) < 0)
	{
		AM_DEBUG(1, "player_get_media_info failed");
		return AM_AV_ERR_SYS;
	}
	info->duration = minfo.stream_info.duration;
	info->size = minfo.stream_info.file_size;
#endif

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_video_para(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val)
{
	const char *name = NULL, *cmd = "";
	char buf[32];
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_VideoWindow_t *win;

	switch (para)
	{
		case AV_VIDEO_PARA_WINDOW:
			name = VID_AXIS_FILE;
			win = (AV_VideoWindow_t *)val;
			snprintf(buf, sizeof(buf), "%d %d %d %d", win->x, win->y, win->x+win->w, win->y+win->h);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_CROP:
			name = VID_CROP_FILE;
			win = (AV_VideoWindow_t *)val;
			snprintf(buf, sizeof(buf), "%d %d %d %d", win->x, win->y, win->w, win->h);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_CONTRAST:
			name = VID_CONTRAST_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_SATURATION:
			name = VID_SATURATION_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_BRIGHTNESS:
			name = VID_BRIGHTNESS_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_ENABLE:
			name = VID_DISABLE_FILE;
			cmd = ((long)val)?"0":"1";
		break;
		case AV_VIDEO_PARA_BLACKOUT:
			if (!(dev->mode & (AV_PLAY_TS | AV_TIMESHIFT))) {
				name = VID_BLACKOUT_FILE;
				cmd = ((long)val)?"1":"0";
			}
			dev->video_blackout = (long)val;
#if 0
#ifdef AMSTREAM_IOC_CLEAR_VBUF
			if((int)val)
			{
				int fd = open(AMVIDEO_FILE, O_RDWR);
				if(fd!=-1)
				{
					ioctl(fd, AMSTREAM_IOC_CLEAR_VBUF, 0);
					close(fd);
				}
			}
#endif
#endif
		break;
		case AV_VIDEO_PARA_RATIO:
#ifndef 	CHIP_8226H
			name = VID_SCREEN_MODE_FILE;
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_AUTO:
					cmd = "0";
				break;
				case AM_AV_VIDEO_ASPECT_16_9:
					cmd = "3";
				break;
				case AM_AV_VIDEO_ASPECT_4_3:
					cmd = "2";
				break;
			}
#else
			name = VID_ASPECT_RATIO_FILE;
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_AUTO:
					cmd = "auto";
				break;
				case AM_AV_VIDEO_ASPECT_16_9:
					cmd = "16x9";
				break;
				case AM_AV_VIDEO_ASPECT_4_3:
					cmd = "4x3";
				break;
			}
 #endif
		break;
		case AV_VIDEO_PARA_RATIO_MATCH:
#ifndef 	CHIP_8226H
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_MATCH_IGNORE:
					cmd = "1";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX:
					cmd = "3";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN:
					cmd = "2";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_COMBINED:
					cmd = "4";
				break;
			}
#else
			name = VID_ASPECT_MATCH_FILE;
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_MATCH_IGNORE:
					cmd = "ignore";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX:
					cmd = "letter box";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN:
					cmd = "pan scan";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_COMBINED:
					cmd = "combined";
				break;
			}
#endif
		break;
		case AV_VIDEO_PARA_MODE:
			name = VID_SCREEN_MODE_FILE;
			cmd = ((AM_AV_VideoDisplayMode_t)val)?"1":"0";
		break;
		case AV_VIDEO_PARA_CLEAR_VBUF:
#if 0
#ifdef AMSTREAM_IOC_CLEAR_VBUF
			{
				int fd = open(AMVIDEO_FILE, O_RDWR);
				if(fd!=-1)
				{
					ioctl(fd, AMSTREAM_IOC_CLEAR_VBUF, 0);
					close(fd);
				}
			}
#endif
#endif
			name = VID_DISABLE_FILE;
			cmd = "2";
		break;
		case AV_VIDEO_PARA_ERROR_RECOVERY_MODE:
			name = VDEC_H264_ERROR_RECOVERY_MODE_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
	}

	if(name)
	{
		ret = AM_FileEcho(name, cmd);
	}

	return ret;
}

static AM_ErrorCode_t aml_inject(AM_AV_Device_t *dev, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout)
{
	AV_InjectData_t *inj = (AV_InjectData_t*)dev->inject_player.drv_data;
	int fd, send, ret;

	if ((inj->pkg_fmt == PFORMAT_ES) && (type == AM_AV_INJECT_AUDIO))
		fd = inj->aud_fd;
	else
		fd = inj->vid_fd;

	if (fd == -1)
	{
		AM_DEBUG(1, "device is not openned");
		return AM_AV_ERR_NOT_ALLOCATED;
	}

	if (timeout >= 0)
	{
		struct pollfd pfd;

		pfd.fd = fd;
		pfd.events = POLLOUT;

		ret = poll(&pfd, 1, timeout);
		if (ret != 1)
			return AM_AV_ERR_TIMEOUT;
	}

	send = *size;
	if (send)
	{
		ret = write(fd, data, send);
		if ((ret == -1) && (errno != EAGAIN))
		{
			AM_DEBUG(1, "inject data failed errno:%d msg:%s", errno, strerror(errno));
			return AM_AV_ERR_SYS;
		}
		else if ((ret == -1) && (errno == EAGAIN))
		{
			ret = 0;
		}
	}
	else
		ret = 0;

	*size = ret;

	return AM_SUCCESS;
}

#ifdef AMSTREAM_IOC_GET_VBUF
extern AM_ErrorCode_t ge2d_blit_yuv(struct vbuf_info *info, AM_OSD_Surface_t *surf);
#endif

static AM_ErrorCode_t aml_video_frame(AM_AV_Device_t *dev, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s)
{
#ifdef AMSTREAM_IOC_GET_VBUF
	int fd = -1, rc;
	struct vbuf_info info;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_OSD_PixelFormatType_t type;
	AM_OSD_Surface_t *surf = NULL;
	int w, h, append;

	fd = open(AMVIDEO_FILE, O_RDWR);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", AMVIDEO_FILE);
		ret = AM_AV_ERR_SYS;
		goto end;
	}

	rc = ioctl(fd, AMSTREAM_IOC_GET_VBUF, &info);
	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_GET_VBUF_INFO failed");
		ret = AM_AV_ERR_SYS;
		goto end;
	}

	type = AM_OSD_FMT_COLOR_RGB_888;
	w = info.width;
	h = info.height;

	ret = AM_OSD_CreateSurface(type, w, h, AM_OSD_SURFACE_FL_HW, &surf);
	if (ret != AM_SUCCESS)
		goto end;

	ret = ge2d_blit_yuv(&info, surf);
	if (ret != AM_SUCCESS)
		goto end;

	*s = surf;
	return AM_SUCCESS;

end:
	if (surf)
		AM_OSD_DestroySurface(surf);
	if (fd != -1)
		close(fd);
	return ret;
#else
	UNUSED(dev);
	UNUSED(para);
	UNUSED(s);

	return AM_AV_ERR_NOT_SUPPORTED;
#endif
}
static AM_Bool_t aml_is_audio_valid(AM_AV_Device_t *dev)
{
	AM_Bool_t has_audio = AM_FALSE;
	AV_TimeshiftData_t *tshift = NULL;

	if ((dev->mode & AV_PLAY_TS) || (dev->mode & AV_INJECT))
	{
		has_audio = VALID_AUDIO(dev->ts_player.play_para.apid, dev->ts_player.play_para.afmt);
	}
	else if(dev->mode & AV_TIMESHIFT)
	{
		tshift = dev->timeshift_player.drv_data;
		if (tshift != NULL)
			has_audio =  VALID_AUDIO(tshift->tp.apid, tshift->tp.afmt);
	}

	return has_audio;
}

static AM_ErrorCode_t aml_get_astatus(AM_AV_Device_t *dev, AM_AV_AudioStatus_t *para)
{
	struct am_io_param astatus;

	int fd, rc;
	char buf[32];

	void *adec = NULL;
	AM_Bool_t has_audio = aml_is_audio_valid(dev);

	if (!has_audio)
	{
		AM_DEBUG(1,"there is no audio pid data ,pls check your parameters");
		return AM_FAILURE;
	}

#if 1
	switch (dev->mode) {
		case AV_PLAY_TS:
			adec = ((AV_TSData_t *)dev->ts_player.drv_data)->adec;
			break;
		case AV_INJECT:
			adec = ((AV_InjectData_t *)dev->inject_player.drv_data)->adec;
			break;
		case AV_TIMESHIFT:
			adec = ((AV_TimeshiftData_t *)dev->timeshift_player.drv_data)->ts.adec;
			break;
		default:
			AM_DEBUG(1, "only valid in TS/INJ/TIMESHIFT mode");
			break;
	}

	audio_ops->adec_get_status(adec,para);

	para->frames      = 1;
	para->aud_fmt     = -1;
#else
	rc = ioctl(fd, AMSTREAM_IOC_ADECSTAT, (int)&astatus);
	if (rc==-1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_ADECSTAT failed");
		goto get_fail;
	}

	para->channels    = astatus.astatus.channels;
	para->sample_rate = astatus.astatus.sample_rate;
	para->resolution  = astatus.astatus.resolution;
	para->frames      = 1;
	para->aud_fmt     = -1;
#endif

	if (AM_FileRead(ASTREAM_FORMAT_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "amadec_mpeg", 11))
			para->aud_fmt = AFORMAT_MPEG;
		else if (!strncmp(buf, "amadec_pcm_s16le", 16))
			para->aud_fmt = AFORMAT_PCM_S16LE;
		else if (!strncmp(buf, "amadec_aac", 10))
			para->aud_fmt = AFORMAT_AAC;
		else if (!strncmp(buf, "amadec_ac3", 10))
			para->aud_fmt = AFORMAT_AC3;
		else if (!strncmp(buf, "amadec_alaw", 11))
			para->aud_fmt = AFORMAT_ALAW;
		else if (!strncmp(buf, "amadec_mulaw", 12))
			para->aud_fmt = AFORMAT_MULAW;
		else if (!strncmp(buf, "amadec_dts", 10))
			para->aud_fmt = AFORMAT_MULAW;
		else if (!strncmp(buf, "amadec_pcm_s16be", 16))
			para->aud_fmt = AFORMAT_PCM_S16BE;
		else if (!strncmp(buf, "amadec_flac", 11))
			para->aud_fmt = AFORMAT_FLAC;
		else if (!strncmp(buf, "amadec_cook", 11))
			para->aud_fmt = AFORMAT_COOK;
		else if (!strncmp(buf, "amadec_pcm_u8", 13))
			para->aud_fmt = AFORMAT_PCM_U8;
		else if (!strncmp(buf, "amadec_adpcm", 12))
			para->aud_fmt = AFORMAT_ADPCM;
		else if (!strncmp(buf, "amadec_amr", 10))
			para->aud_fmt = AFORMAT_AMR;
		else if (!strncmp(buf, "amadec_raac", 11))
			para->aud_fmt = AFORMAT_RAAC;
		else if (!strncmp(buf, "amadec_wma", 10))
			para->aud_fmt = AFORMAT_WMA;
		else if (!strncmp(buf,"amadec_dra",10))
			para->aud_fmt = AFORMAT_DRA;
	}

	if (para->aud_fmt_orig == -1)
		para->aud_fmt_orig = para->aud_fmt;
	if (para->resolution_orig == -1)
		para->resolution_orig = para->resolution;
	if (para->sample_rate_orig == -1)
		para->sample_rate_orig = para->sample_rate;
	if (para->channels_orig == -1)
		para->channels_orig = para->channels;
	if (para->lfepresent_orig == -1)
		para->lfepresent_orig = 0;
	if (para->lfepresent == -1)
		para->lfepresent = 0;

	fd = get_amstream(dev);
	if (fd == -1)
	{
		//AM_DEBUG(1, "cannot get status in this mode");
		goto get_fail;
	}

	rc = ioctl(fd, AMSTREAM_IOC_AB_STATUS, (long)&astatus);
	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_AB_STATUS failed");
		goto get_fail;
	}

	para->ab_size = astatus.status.size;
	para->ab_data = astatus.status.data_len;
	para->ab_free = astatus.status.free_len;

	return AM_SUCCESS;

get_fail:
	return AM_FAILURE;
}
static AM_Bool_t aml_is_video_valid(AM_AV_Device_t *dev)
{
	AM_Bool_t has_video = AM_FALSE;
	AV_TimeshiftData_t *tshift = NULL;

	if ((dev->mode & AV_PLAY_TS) || (dev->mode & AV_INJECT))
	{
		has_video = VALID_VIDEO(dev->ts_player.play_para.vpid, dev->ts_player.play_para.vfmt);
	}
	else if(dev->mode & AV_TIMESHIFT)
	{
		tshift = dev->timeshift_player.drv_data;
		if (tshift != NULL)
			has_video =  VALID_VIDEO(tshift->tp.vpid, tshift->tp.vfmt);
	}

	return has_video;
}
static AM_ErrorCode_t aml_get_vstatus(AM_AV_Device_t *dev, AM_AV_VideoStatus_t *para)
{
	struct am_io_param vstatus;
	char buf[32];
	int fd, rc;
	AM_Bool_t has_video = aml_is_video_valid(dev);

	if (!has_video)
	{
	//	AM_DEBUG(1, "has no video pid data pls check your parameters");
		return AM_FAILURE;
	}

	fd = get_amstream(dev);
	if (fd == -1)
	{
		//AM_DEBUG(1, "cannot get status in this mode");
		goto get_fail;
	}

	rc = ioctl(fd, AMSTREAM_IOC_VDECSTAT, &vstatus);

	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_VDECSTAT failed");
		goto get_fail;
	}

	para->vid_fmt   = dev->ts_player.play_para.vfmt;
	para->src_w     = vstatus.vstatus.width;
	para->src_h     = vstatus.vstatus.height;
	para->fps       = vstatus.vstatus.fps;
	para->vid_ratio = convert_aspect_ratio(vstatus.vstatus.euAspectRatio);
	para->frames    = 1;
	para->interlaced  = 1;

#if 1
	if (AM_FileRead(VID_FRAME_FMT_FILE, buf, sizeof(buf)) >= 0) {
		char *ptr = strstr(buf, "interlace");
		if (ptr) {
			para->interlaced = 1;
		} else {
			para->interlaced = 0;
		}
	}
#else
	if(AM_FileRead("/sys/module/amvdec_mpeg12/parameters/pic_type", buf, sizeof(buf))>=0){
		int i = strtol(buf, NULL, 0);
		if(i==1)
			para->interlaced = 0;
		else if(i==2)
			para->interlaced = 1;
	}
	if(AM_FileRead("/sys/module/amvdec_h264/parameters/pic_type", buf, sizeof(buf))>=0){
		int i = strtol(buf, NULL, 0);
		if(i==1)
			para->interlaced = 0;
		else if(i==2)
			para->interlaced = 1;
       }
       if(AM_FileRead("/sys/module/amvdec_avs/parameters/pic_type", buf, sizeof(buf))>=0){
                int i = strtol(buf, NULL, 0);
                if(i==1)
                        para->interlaced = 0;
                else if(i==2)
                        para->interlaced = 1;
       }
#endif

	rc = ioctl(fd, AMSTREAM_IOC_VB_STATUS, (long)&vstatus);
	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_VB_STATUS failed");
		goto get_fail;
	}

	para->vb_size = vstatus.status.size;
	para->vb_data = vstatus.status.data_len;
	para->vb_free = vstatus.status.free_len;

	return AM_SUCCESS;

get_fail:
	return AM_FAILURE;
}

static AM_ErrorCode_t aml_set_ppmgr_3dcmd(int cmd)
{
	int ppmgr_fd;
	int arg = -1, ret;

	ppmgr_fd = open(PPMGR_FILE, O_RDWR);
	if (ppmgr_fd < 0)
	{
		AM_DEBUG(0, "Open /dev/ppmgr error(%s)!\n", strerror(errno));
		return AM_AV_ERR_SYS;
	}

    switch (cmd)
    {
    case AM_AV_PPMGR_MODE3D_DISABLE:
        arg = MODE_3D_DISABLE;
        AM_DEBUG(1,"3D fucntion (0: Disalbe!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_AUTO:
        arg = MODE_3D_ENABLE|MODE_AUTO;
        AM_DEBUG(1,"3D fucntion (1: Auto!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_2D_TO_3D:
        arg = MODE_3D_ENABLE|MODE_2D_TO_3D;
        AM_DEBUG(1,"3D fucntion (2: 2D->3D!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_LR:
        arg = MODE_3D_ENABLE|MODE_LR;
        AM_DEBUG(1,"3D fucntion (3: L/R!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_BT:
        arg = MODE_3D_ENABLE|MODE_BT;
        AM_DEBUG(1,"3D fucntion (4: B/T!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_LR_SWITCH:
        arg = MODE_3D_ENABLE|MODE_LR;
        AM_DEBUG(1,"3D fucntion (5: LR SWITCH OFF!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_ON_LR_SWITCH:
        arg = MODE_3D_ENABLE|MODE_LR_SWITCH;
        AM_DEBUG(1,"3D fucntion (6: LR SWITCH!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_FIELD_DEPTH:
        arg = MODE_3D_ENABLE|MODE_FIELD_DEPTH;
        AM_DEBUG(1,"3D function (7: FIELD_DEPTH!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_3D_TO_2D:
        arg = MODE_3D_ENABLE|MODE_LR;
        AM_DEBUG(1,"3D fucntion (8: 3D_TO_2D_TURN_OFF!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_L_3D_TO_2D:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_L;
        AM_DEBUG(1,"3D function (9: 3D_TO_2D_L!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_R_3D_TO_2D:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_R;
        AM_DEBUG(1,"3D function (10: 3D_TO_2D_R!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_LR_SWITCH_BT:
        arg = MODE_3D_ENABLE|MODE_BT|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D function (11: BT SWITCH OFF!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_ON_LR_SWITCH_BT:
        arg = MODE_3D_ENABLE|MODE_LR_SWITCH|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D fucntion (12: BT SWITCH!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_3D_TO_2D_BT:
        arg = MODE_3D_ENABLE|MODE_BT;
        AM_DEBUG(1,"3D fucntion (13: 3D_TO_2D_TURN_OFF_BT!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_L_3D_TO_2D_BT:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_L|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D function (14: 3D TO 2D L BT!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_R_3D_TO_2D_BT:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_R|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D function (15: 3D TO 2D R BT!)\n");
        break;
    default:
    	AM_DEBUG(1, "Unkown set 3D cmd %d", cmd);
    	arg = -1;
    	break;
    }

    if (arg != -1)
    {
    	if (ioctl(ppmgr_fd, PPMGR_IOC_ENABLE_PP, arg) == -1)
    	{
    		AM_DEBUG(1, "Set 3D function failed: %s", strerror(errno));
    		close(ppmgr_fd);
    		return AM_AV_ERR_SYS;
    	}
    }

	close(ppmgr_fd);

    return AM_SUCCESS;
}

static int
get_osd_prop(const char *mode, const char *p, const char *defv)
{
	char n[32];
	char v[32];
	int r;

	snprintf(n, sizeof(n), "ubootenv.var.%soutput%s", mode, p);
#ifdef ANDROID
	property_get(n,v,defv);
#endif
	sscanf(v, "%d", &r);

	return r;
}

static void
get_osd_rect(const char *mode, int *x, int *y, int *w, int *h)
{
	const char *m = mode?mode:"720p";
	char defw[16], defh[16];
	int r;

	r = get_osd_prop(m, "x", "0");
	*x = r;
	r = get_osd_prop(m, "y", "0");
	*y = r;
	if (!strncmp(m, "480", 3)) {
		snprintf(defw, sizeof(defw), "%d", 720);
		snprintf(defh, sizeof(defh), "%d", 480);
	} else if (!strncmp(m, "576", 3)) {
		snprintf(defw, sizeof(defw), "%d", 720);
		snprintf(defh, sizeof(defh), "%d", 576);
	} else if (!strncmp(m, "720", 3)) {
		snprintf(defw, sizeof(defw), "%d", 1280);
		snprintf(defh, sizeof(defh), "%d", 720);
	} else if (!strncmp(m, "1080", 4)) {
		snprintf(defw, sizeof(defw), "%d", 1920);
		snprintf(defh, sizeof(defh), "%d", 1080);
	}


	r = get_osd_prop(m, "width", defw);
	*w = r;
	r = get_osd_prop(m, "height", defh);
	*h = r;
}

static AM_ErrorCode_t
aml_set_vpath(AM_AV_Device_t *dev)
{
#if 0
	static char s_bypass_hd[2];
	static char s_bypass_hd_prog[2];
	static char s_bypass_prog[2];
	static char s_bypass_1080p[2];
	static char s_bypass_dynamic[2];

	AM_ErrorCode_t ret;
	int times = 10;

	AM_DEBUG(1, "set video path fs:%d di:%d ppmgr:%d", dev->vpath_fs, dev->vpath_di, dev->vpath_ppmgr);

	//AM_FileEcho("/sys/class/deinterlace/di0/config", "disable");
	/*
	do{
		ret = AM_FileEcho("/sys/class/vfm/map", "rm default");
		if(ret!=AM_SUCCESS){
			usleep(10000);
		}
	}while(ret!=AM_SUCCESS && times--);
	*/

	char video_axis[32];
	AM_FileRead("/sys/class/video/axis", video_axis, sizeof(video_axis));

	if(dev->vpath_fs==AM_AV_FREE_SCALE_ENABLE){
		char mode[16];
		char ppr[32];
		AM_Bool_t blank = AM_TRUE;
#ifdef ANDROID
		{
			int x, y, w, h;

			AM_FileRead("/sys/class/display/mode", mode, sizeof(mode));

			if(!strncmp(mode, "480i", 4)){
				get_osd_rect("480i", &x, &y, &w, &h);
			}else if(!strncmp(mode, "480p", 4)){
				get_osd_rect("480p", &x, &y, &w, &h);
			}else if(!strncmp(mode, "480cvbs", 7)){
				get_osd_rect("480cvbs", &x, &y, &w, &h);
			}else if(!strncmp(mode, "576i", 4)){
				get_osd_rect("576i", &x, &y, &w, &h);
			}else if(!strncmp(mode, "576p", 4)){
				get_osd_rect("576p", &x, &y, &w, &h);
			}else if(!strncmp(mode, "576cvbs", 7)){
				get_osd_rect("576cvbs", &x, &y, &w, &h);
			}else if(!strncmp(mode, "720p", 4)){
				get_osd_rect("720p", &x, &y, &w, &h);
				blank = AM_FALSE;
			}else if(!strncmp(mode, "1080i", 5)){
				get_osd_rect("1080i", &x, &y, &w, &h);
			}else if(!strncmp(mode, "1080p", 5)){
				get_osd_rect("1080p", &x, &y, &w, &h);
			}else{
				get_osd_rect(NULL, &x, &y, &w, &h);
			}
			snprintf(ppr, sizeof(ppr), "%d %d %d %d 0", x, y, x+w, y+h);
		}
#endif
		AM_FileRead("/sys/class/graphics/fb0/request2XScale", mode, sizeof(mode));
		if (blank && !strncmp(mode, "2", 1)) {
			blank = AM_FALSE;
		}
		if(blank){
			//AM_FileEcho("/sys/class/graphics/fb0/blank", "1");
		}
		//AM_FileEcho("/sys/class/graphics/fb0/free_scale", "1");
		//AM_FileEcho("/sys/class/graphics/fb1/free_scale", "1");
#ifdef ANDROID
		{
			AM_FileEcho("/sys/class/graphics/fb0/request2XScale", "2");
			AM_FileEcho("/sys/class/graphics/fb1/scale", "0");

			AM_FileEcho("/sys/module/amvdec_h264/parameters/dec_control", "0");
			AM_FileEcho("/sys/module/amvdec_mpeg12/parameters/dec_control", "0");

			s_bypass_hd[1] = '\0';
			s_bypass_hd_prog[1] = '\0';
			s_bypass_prog[1] = '\0';
			s_bypass_1080p[1] = '\0';
			AM_FileEcho("/sys/module/di/parameters/bypass_hd", s_bypass_hd);
			AM_FileEcho("/sys/module/di/parameters/bypass_hd_prog", s_bypass_hd_prog);
			AM_FileEcho("/sys/module/di/parameters/bypass_prog", s_bypass_prog);
			AM_FileEcho("/sys/module/di/parameters/bypass_1080p", s_bypass_1080p);
#ifdef ENABLE_CORRECR_AV_SYNC
#else
			s_bypass_dynamic[1] = '\0';
			AM_FileEcho("/sys/module/di/parameters/bypass_dynamic", s_bypass_dynamic);
#endif

			AM_FileEcho("/sys/class/ppmgr/ppscaler","1");

			AM_FileEcho("/sys/module/amvideo/parameters/smooth_sync_enable", "0");
			usleep(200*1000);
		}
#endif
	}else{
		AM_Bool_t blank = AM_TRUE;
		char m1080scale[8];
		char mode[16];
		char verstr[32];
		char *reqcmd, *osd1axis, *osd1cmd;
		int version;
		AM_Bool_t scale=AM_TRUE;

#ifdef ANDROID
		{
			property_get("ro.platform.has.1080scale",m1080scale,"fail");
			if(!strncmp(m1080scale, "fail", 4)){
				scale = AM_FALSE;
			}

			AM_FileRead("/sys/class/display/mode", mode, sizeof(mode));

			if(strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5) && strncmp(mode, "720p", 4)))){
				scale = AM_FALSE;
			}

			AM_FileRead("/sys/class/graphics/fb0/request2XScale", verstr, sizeof(verstr));
			if(!strncmp(mode, "480i", 4) || !strncmp(mode, "480p", 4) || !strncmp(mode, "480cvbs", 7)){
				reqcmd   = "16 720 480";
				osd1axis = "1280 720 720 480";
				osd1cmd  = "0x10001";
			}else if(!strncmp(mode, "576i", 4) || !strncmp(mode, "576p", 4) || !strncmp(mode, "576cvbs", 7)){
				reqcmd   = "16 720 576";
				osd1axis = "1280 720 720 576";
				osd1cmd  = "0x10001";
			}else if(!strncmp(mode, "1080i", 5) || !strncmp(mode, "1080p", 5)){
				reqcmd   = "8";
				osd1axis = "1280 720 1920 1080";
				osd1cmd  = "0x10001";
			}else{
				reqcmd   = "2";
				osd1axis = NULL;
				osd1cmd  = "0";
				blank    = AM_FALSE;
			}

			if (blank && !strncmp(verstr, reqcmd, strlen(reqcmd))) {
				blank = AM_FALSE;
			}

			property_get("ro.build.version.sdk",verstr,"10");
			if(sscanf(verstr, "%d", &version)==1){
				if(version < 15){
					blank = AM_FALSE;
				}
			}
		}
#endif
		if(blank && scale){
			//AM_FileEcho("/sys/class/graphics/fb0/blank", "1");
		}

		AM_FileEcho("/sys/class/graphics/fb0/free_scale", "0");
		AM_FileEcho("/sys/class/graphics/fb1/free_scale", "0");

#ifdef ANDROID
		{
			if(scale){
				AM_FileEcho("/sys/class/graphics/fb0/request2XScale", reqcmd);
				if(osd1axis){
					AM_FileEcho("/sys/class/graphics/fb1/scale_axis", osd1axis);
				}
				AM_FileEcho("/sys/class/graphics/fb1/scale", osd1cmd);
			}


			AM_FileEcho("/sys/module/amvdec_h264/parameters/dec_control", "3");
			AM_FileEcho("/sys/module/amvdec_mpeg12/parameters/dec_control", "62");

			AM_FileRead("/sys/module/di/parameters/bypass_hd", s_bypass_hd, sizeof(s_bypass_hd));
			AM_FileRead("/sys/module/di/parameters/bypass_hd_prog", s_bypass_hd_prog, sizeof(s_bypass_hd_prog));
			AM_FileRead("/sys/module/di/parameters/bypass_prog", s_bypass_prog, sizeof(s_bypass_prog));
			AM_FileRead("/sys/module/di/parameters/bypass_1080p", s_bypass_1080p, sizeof(s_bypass_1080p));

#ifdef ENABLE_CORRECR_AV_SYNC
			AM_DEBUG(1, "ENABLE_CORRECR_AV_SYNC");

			AM_FileEcho("/sys/module/di/parameters/bypass_hd","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_hd_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_1080p","0");
#else
			AM_FileRead("/sys/module/di/parameters/bypass_dynamic", s_bypass_dynamic, sizeof(s_bypass_dynamic));

			AM_DEBUG(1, "Not ENABLE_CORRECR_AV_SYNC");

			AM_FileEcho("/sys/module/di/parameters/bypass_hd","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_hd_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_1080p","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_dynamic","1");

			//AM_FileEcho("/sys/module/di/parameters/bypass_hd","1");
#endif
			AM_FileEcho("/sys/class/ppmgr/ppscaler","0");

			//AM_FileEcho("/sys/class/ppmgr/ppscaler_rect","0 0 0 0 1");
			//AM_FileEcho("/sys/class/video/axis", "0 0 0 0");
			AM_FileEcho("/sys/module/amvideo/parameters/smooth_sync_enable", AV_SMOOTH_SYNC_VAL);
			AM_FileEcho(DI_BYPASS_ALL_FILE,"0");
			usleep(2000*1000);
		}
#endif
	}

	/*
	if(dev->vpath_ppmgr!=AM_AV_PPMGR_DISABLE){
		if (dev->vpath_ppmgr == AM_AV_PPMGR_MODE3D_2D_TO_3D){
			AM_FileEcho("/sys/class/vfm/map", "add default decoder deinterlace d2d3 amvideo");
			AM_FileEcho("/sys/class/d2d3/d2d3/debug", "enable");
		}else{
			AM_FileEcho("/sys/class/vfm/map", "add default decoder ppmgr amvideo");
			if (dev->vpath_ppmgr!=AM_AV_PPMGR_ENABLE) {
				//Set 3D mode
				AM_TRY(aml_set_ppmgr_3dcmd(dev->vpath_ppmgr));
			}
		}

	}else if(dev->vpath_di==AM_AV_DEINTERLACE_ENABLE){
		AM_FileEcho("/sys/class/vfm/map", "add default decoder deinterlace amvideo");
		AM_FileEcho("/sys/class/deinterlace/di0/config", "enable");
	}else{
		AM_FileEcho("/sys/class/vfm/map", "add default decoder amvideo");
	}
	*/

	AM_FileEcho("/sys/class/video/axis", video_axis);
	AM_FileRead("/sys/class/video/axis", video_axis, sizeof(video_axis));
#else
	UNUSED(dev);
#endif
	return AM_SUCCESS;
}

/**\brief 切换TS播放的音频*/
static AM_ErrorCode_t aml_switch_ts_audio_legacy(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt)
{
	int fd = -1;
	AM_Bool_t audio_valid = VALID_AUDIO(apid, afmt);
	AM_Bool_t has_video = VALID_VIDEO(dev->ts_player.play_para.vpid, dev->ts_player.play_para.vfmt);
	AV_TSData_t *ts = NULL;

	AM_DEBUG(1, "switch ts audio: A[%d:%d]", apid, afmt);

	if (dev->ts_player.drv_data) {
		ts = (AV_TSData_t*)dev->ts_player.drv_data;
		fd = ts->fd;
	}

	if (fd < 0)
	{
		AM_DEBUG(1, "ts_player fd < 0, error!");
		return AM_AV_ERR_SYS;
	}

	/*Stop Audio first*/
	aml_stop_av_monitor(dev, &dev->ts_player.mon);

	aml_set_ad_source(&ts->ad, 0, 0, 0, ts->adec);
	audio_ops->adec_set_decode_ad(0, 0, 0, ts->adec);
	audio_ops->adec_stop_decode(&ts->adec);

	/*Set Audio PID & fmt*/
	if (audio_valid)
	{
		if (ioctl(fd, AMSTREAM_IOC_AFORMAT, (int)afmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(fd, AMSTREAM_IOC_AID, (int)apid) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}

	/*reset audio*/
	if (ioctl(fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
	{
		AM_DEBUG(1, "audio reset failed");
		return AM_AV_ERR_SYS;
	}

	AM_TIME_GetClock(&dev->ts_player.av_start_time);

	dev->ts_player.play_para.apid = apid;
	dev->ts_player.play_para.afmt = afmt;

#ifdef ENABLE_PCR
	if (!show_first_frame_nosync()) {
#ifdef ANDROID
		//property_set("sys.amplayer.drop_pcm", "1");
#endif
	}
	AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");
	aml_set_sync_mode(dev,
		aml_calc_sync_mode(dev, audio_valid, has_video, 1, afmt, NULL));
	audio_ops->adec_start_decode(fd, afmt, has_video, &ts->adec);
	uint16_t sub_apid = dev->ts_player.play_para.sub_apid ;
	AM_AV_AFormat_t sub_afmt = dev->ts_player.play_para.sub_afmt;
	if (VALID_PID(sub_apid))
		aml_set_audio_ad(dev, 1, sub_apid, sub_afmt);
#endif /*ENABLE_PCR*/

	/*Start Audio*/
	aml_start_av_monitor(dev, &dev->ts_player.mon);

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_switch_ts_audio_fmt(AM_AV_Device_t *dev, AV_TSData_t *ts, AV_TSPlayPara_t *tp)
{
	AM_Bool_t has_audio = AM_FALSE;
	AM_Bool_t has_video = AM_FALSE;
	AM_Bool_t has_pcr = AM_FALSE;

	if (!ts || !tp) {
		AM_DEBUG(1, "do switch ts audio, illegal operation, current mode [%d]", dev->mode);
		return AM_AV_ERR_ILLEGAL_OP;
	}

	if (tp->apid != dev->alt_apid
		|| tp->afmt != dev->alt_afmt) {
		AM_DEBUG(1, "do switch ts audio: A[%d:%d] -> A[%d:%d]",
			tp->apid, tp->afmt, dev->alt_apid, dev->alt_afmt);
		tp->apid = dev->alt_apid;
		tp->afmt = dev->alt_afmt;
	} else {
		AM_DEBUG(1, "do switch ts audio: A[%d:%d] -> A[%d:%d], ignore",
			tp->apid, tp->afmt, dev->alt_apid, dev->alt_afmt);
		return AM_SUCCESS;
	}

	if (ts->fd < 0)
	{
		AM_DEBUG(1, "amstream fd < 0, error!");
		return AM_AV_ERR_SYS;
	}

	has_audio = VALID_AUDIO(tp->apid, tp->afmt);
	has_video = VALID_VIDEO(tp->vpid, tp->vfmt);
	has_pcr = VALID_PCR(tp->pcrpid);

	aml_set_ad_source(&ts->ad, 0, 0, 0, ts->adec);
	audio_ops->adec_set_decode_ad(0, 0, 0, ts->adec);
	audio_ops->adec_stop_decode(&ts->adec);

	/*Set Audio PID & fmt*/
	if (has_audio)
	{
		if (ioctl(ts->fd, AMSTREAM_IOC_AFORMAT, (int)tp->afmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(ts->fd, AMSTREAM_IOC_AID, (int)tp->apid) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}
	else
	{
		return AM_SUCCESS;
	}

	/*reset audio*/
	if (ioctl(ts->fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
	{
		AM_DEBUG(1, "audio reset failed");
		return AM_AV_ERR_SYS;
	}

#ifdef ENABLE_PCR
	if (!show_first_frame_nosync()) {
#ifdef ANDROID
		//property_set("sys.amplayer.drop_pcm", "1");
#endif
	}
	AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");

	if ((has_video && has_audio) || has_pcr)
		aml_set_tsync_enable(1);
	else
		aml_set_tsync_enable(0);

	aml_set_sync_mode(dev,
		aml_calc_sync_mode(dev, has_audio, has_video, has_pcr, tp->afmt, NULL));
	audio_ops->adec_start_decode(ts->fd, tp->afmt, has_video, &ts->adec);

	if (VALID_PID(tp->sub_apid))
		aml_set_audio_ad(dev, 1, tp->sub_apid, tp->sub_afmt);
#endif /*ENABLE_PCR*/
	AM_DEBUG(1, "switch ts audio: end");
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_switch_ts_audio(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AM_ErrorCode_t err = AM_SUCCESS;

	AM_DEBUG(1, "switch ts audio: A[%d:%d]", apid, afmt);

	{
		AV_TSPlayPara_t tp = {.apid = apid, .afmt = afmt};
		setup_forced_pid(&tp);
		apid = tp.apid;
		afmt = tp.afmt;
	}

	dev->alt_apid = apid;
	dev->alt_afmt = afmt;
	dev->audio_switch = AM_TRUE;

	return err;
}


/**\brief set vdec_h264 error_recovery_mode :0 or 2 -> skip display Mosaic  ,3: display mosaic in case of vdec hit error*/
static AM_ErrorCode_t aml_set_vdec_error_recovery_mode(AM_AV_Device_t *dev, uint8_t error_recovery_mode)
{
    char buf[32];

    UNUSED(dev);

    if (error_recovery_mode > 3)
    {
       AM_DEBUG(1, "set error_recovery_mode input parameters error!");
       return AM_FAILURE;
    }

    snprintf(buf, sizeof(buf), "%d", error_recovery_mode);
    AM_FileEcho(VDEC_H264_ERROR_RECOVERY_MODE_FILE, buf);

    return AM_SUCCESS;
}

AM_ErrorCode_t aml_reset_audio_decoder(AM_AV_Device_t *dev)
{
      int fd = -1;

       if (dev->ts_player.drv_data)
               fd = ((AV_TSData_t *)dev->ts_player.drv_data)->fd;

       if (fd < 0)
       {
               AM_DEBUG(1, "ts_player fd < 0, error!");
              return AM_AV_ERR_SYS;
       }

       /*reset audio*/
       if (ioctl(fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
       {
               AM_DEBUG(1, "audio reset failed");
               return AM_AV_ERR_SYS;
       }

       return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_drm_mode(AM_AV_Device_t *dev, AM_AV_DrmMode_t drm_mode)
{
	int fd = -1;

	dev->curr_para.drm_mode = drm_mode;

	return AM_SUCCESS;
}

AM_ErrorCode_t aml_set_inject_audio(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AV_InjectData_t *data;
	AM_Bool_t audio_valid, video_valid;
	int fd;
	AM_Bool_t has_video = VALID_VIDEO(dev->ts_player.play_para.vpid, dev->ts_player.play_para.vfmt);

	data = (AV_InjectData_t *)dev->inject_player.drv_data;

	fd = (data->aud_fd == -1) ? data->vid_fd : data->aud_fd;

	if (data->aud_id != -1) {
		audio_ops->adec_stop_decode(&data->adec);
		data->aud_id = -1;
	}

	if (apid && (apid<0x1fff)) {
		if (ioctl(fd, AMSTREAM_IOC_AFORMAT, (int)afmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}

		if (ioctl(fd, AMSTREAM_IOC_AID, (int)apid) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}

	/*reset audio*/
	if (ioctl(fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
	{
		AM_DEBUG(1, "audio reset failed");
		return AM_AV_ERR_SYS;
	}

	/*Start audio decoder*/
	if (apid && (apid<0x1fff))
	{
		audio_ops->adec_start_decode(fd, afmt, has_video, &data->adec);
	}

	data->aud_id  = apid;
	data->aud_fmt = afmt;

	return AM_SUCCESS;
}

AM_ErrorCode_t aml_set_inject_subtitle(AM_AV_Device_t *dev, uint16_t spid, int stype)
{
	AV_InjectData_t *data;
	int fd;

	data = (AV_InjectData_t *)dev->inject_player.drv_data;

	fd = (data->aud_fd == -1) ? data->vid_fd : data->aud_fd;

	if (spid && (spid<0x1fff)) {
		if (ioctl(fd, AMSTREAM_IOC_SID, spid) == -1)
		{
			AM_DEBUG(1, "set subtitle PID failed");
			return AM_AV_ERR_SYS;
		}

		if (ioctl(fd, AMSTREAM_IOC_SUB_TYPE, stype) == -1)
		{
			AM_DEBUG(1, "set subtitle type failed");
			return AM_AV_ERR_SYS;
		}

		if (ioctl(fd, AMSTREAM_IOC_SUB_RESET) == -1)
		{
			AM_DEBUG(1, "reset subtitle failed");
			return AM_AV_ERR_SYS;
		}
	}

	data->sub_id   = spid;
	data->sub_type = stype;

	return AM_SUCCESS;
}

static void ad_callback(const uint8_t * data,int len,void * user_data)
{
#ifdef USE_ADEC_IN_DVB
	if (s_audio_cb == NULL) {
	    printf("ad_callback [%d:%p] [user:%p]\n", len, data, user_data);
	    audio_send_associate_data(user_data, (uint8_t *)data, len);
	}
#endif
}

static AM_ErrorCode_t aml_set_ad_source(AM_AD_Handle_t *ad, int enable, int pid, int fmt, void *user)
{
	AM_ErrorCode_t err = AM_SUCCESS;

	UNUSED(fmt);

	if (!ad)
		return AM_AV_ERR_INVAL_ARG;

	AM_DEBUG(1, "AD set source enable[%d] pid[%d] fmt[%d]", enable, pid, fmt);

#ifdef USE_ADEC_IN_DVB

	if (enable) {
		AM_AD_Para_t para = {.dmx_id = 0, .pid = pid, .fmt = fmt};
		err = AM_AD_Create(ad, &para);
		if (err == AM_SUCCESS) {
			err = AM_AD_SetCallback(*ad, ad_callback, user);
			err = AM_AD_Start(*ad);
			if (err != AM_SUCCESS)
				AM_AD_Destroy(*ad);
		}
	} else {
		if (*ad) {
			err = AM_AD_Stop(*ad);
			err = AM_AD_Destroy(*ad);
			*ad = NULL;
		}
	}
#else
	if (enable) {
		*ad = (AM_AD_Handle_t) 1;
	} else {
		if (*ad) {
			err = AM_SUCCESS;
			*ad = NULL;
		}
	}
#endif
	return err;
}
static AM_ErrorCode_t aml_set_audio_ad(AM_AV_Device_t *dev, int enable, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AM_AD_Handle_t *pad = NULL;
	void *adec = NULL;
	uint16_t sub_apid;
	AM_AV_AFormat_t sub_afmt;
	AM_Bool_t is_valid_audio = VALID_AUDIO(apid, afmt);

	AM_DEBUG(1, "AD aml set audio ad: enable[%d] pid[%d] fmt[%d]", enable, apid, afmt);

	switch (dev->mode) {
		case AV_PLAY_TS:
			adec = ((AV_TSData_t*)dev->ts_player.drv_data)->adec;
			pad = &((AV_TSData_t*)dev->ts_player.drv_data)->ad;
			sub_apid = dev->ts_player.play_para.sub_apid;
			sub_afmt = dev->ts_player.play_para.sub_afmt;
			break;
		case AV_INJECT:
			adec = ((AV_InjectData_t*)dev->inject_player.drv_data)->adec;
			pad = &((AV_InjectData_t*)dev->inject_player.drv_data)->ad;
			sub_apid = dev->inject_player.para.sub_aud_pid;
			sub_afmt = dev->inject_player.para.sub_aud_fmt;
			break;
		case AV_TIMESHIFT:
			adec = ((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->ts.adec;
			pad = &((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->ts.ad;
			sub_apid = ((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->tp.sub_apid;
			sub_afmt = ((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->tp.sub_afmt;
			break;
		default:
			AM_DEBUG(1, "only valid in TS/INJ/TIMESHIFT mode");
			return AM_AV_ERR_NOT_SUPPORTED;
	}
#ifdef USE_ADEC_IN_DVB
	if (enable && !adec) {
		AM_DEBUG(1, "no main audio, this is associate audio setting");
		return AM_AV_ERR_ILLEGAL_OP;
	}
#endif

	/*assume ad is enabled if ad handle is not NULL*/
	if ((enable && *pad && (apid == sub_apid) && (afmt == sub_afmt))
		|| (!enable && !*pad))
		return AM_SUCCESS;

	if (enable && is_valid_audio) {

		if ((apid != sub_apid) || (afmt != sub_afmt))
			aml_set_ad_source(pad, 0, sub_apid, sub_afmt, adec);

		/*enable adec's ad*/
		audio_ops->adec_set_decode_ad(1, apid, afmt, adec);

		/*setup date source*/
		aml_set_ad_source(pad, 1, apid, afmt, adec);

	} else if (!enable && pad && *pad) {

		/*shutdown date source*/
		aml_set_ad_source(pad, 0, apid, afmt, adec);

		/*disable adec's ad*/
		audio_ops->adec_set_decode_ad(0, apid, afmt, adec);

		apid = -1;
		afmt = 0;
	}

	switch (dev->mode) {
		case AV_PLAY_TS:
			dev->ts_player.play_para.sub_apid = apid;
			dev->ts_player.play_para.sub_afmt = afmt;
			break;
		case AV_INJECT:
			dev->inject_player.para.sub_aud_pid = apid;
			dev->inject_player.para.sub_aud_fmt = afmt;
			break;
		case AV_TIMESHIFT:
			((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->tp.sub_apid = apid;
			((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->tp.sub_afmt = afmt;
			break;
		default:
			break;
	}

	return AM_SUCCESS;
}

static int aml_restart_inject_mode(AM_AV_Device_t *dev, AM_Bool_t destroy_thread)
{
	AV_InjectData_t *inj,pre_inject_data;
	AV_InjectPlayPara_t inj_data;

	AM_DEBUG(1, "aml_restart_inject_mode");

	inj = dev->inject_player.drv_data;
	memcpy(&pre_inject_data,inj,sizeof(AV_InjectData_t));
	aml_destroy_inject_data(inj);
	if (destroy_thread) {
		aml_stop_av_monitor(dev, &dev->ts_player.mon);
	}
	//malloc
	inj = aml_create_inject_data();
	if (!inj)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_AV_ERR_NO_MEM;
	}
	dev->inject_player.drv_data = inj;
	///replace the inject data

	inj_data.para.aud_fmt = pre_inject_data.aud_fmt;
	inj_data.para.aud_id = pre_inject_data.aud_id;
	inj_data.para.pkg_fmt = pre_inject_data.pkg_fmt;
	inj_data.para.vid_fmt = pre_inject_data.vid_fmt;
	inj_data.para.vid_id = pre_inject_data.vid_id;
	inj_data.para.sub_id = pre_inject_data.sub_id;
	inj_data.para.sub_type = pre_inject_data.sub_type;
	inj_data.para.channel = 0;
	inj_data.para.data_width = 0;
	inj_data.para.sample_rate = 0;

	if (aml_start_inject(dev, inj, &inj_data) != AM_SUCCESS)
	{
		AM_DEBUG(1,"[aml_start_mode]  AM_AV_ERR_SYS");
		return AM_AV_ERR_SYS;
	} else {
		dev->ts_player.play_para.afmt = inj->aud_fmt;
		dev->ts_player.play_para.apid = inj->aud_id;
		dev->ts_player.play_para.vfmt = inj->vid_fmt;
		dev->ts_player.play_para.vpid = inj->vid_id;
		dev->ts_player.play_para.sub_afmt = inj->sub_type;
		dev->ts_player.play_para.sub_apid = inj->sub_id;
		dev->ts_player.play_para.pcrpid = 0x1fff;

		//ts = (AV_TSData_t*)dev->ts_player.drv_data;
		//ts->fd = inj->vid_fd;//"/dev/amstream_mpts"
		//ts->vid_fd = inj->cntl_fd;//"/dev/amvideo"
		if (destroy_thread)
			aml_start_av_monitor(dev, &dev->ts_player.mon);
	}

	return AM_SUCCESS;
}
static void aml_set_audio_cb(AM_AV_Device_t *dev,AM_AV_Audio_CB_t cb,void *user_data)
{
	AM_DEBUG(1, "%s %d", __FUNCTION__,__LINE__);

	if (cb != NULL) {
		audio_ops = &callback_audio_drv;
		s_audio_cb = cb;
		pUserData = user_data;
	} else {
#ifdef USE_ADEC_IN_DVB
		audio_ops = &native_audio_drv;
#else
		audio_ops = &callback_audio_drv;
#endif
	}
}

static AM_ErrorCode_t aml_get_pts(AM_AV_Device_t *dev, int type, uint64_t *pts)
{
#define GAP_10SEC (90000*10)/*10s*/
	char pts_buf[32];
	const char *pts_file, *pts_bit32_file, *pts_tsync_file;
	uint32_t pts_dmx = 0, pts_dmx_bit32 = 0, pts_dmx_2 = 0, pts_tsync = 0;
	int retry = 0;

	switch (type) {
	case 1:
		pts_file = AUDIO_DMX_PTS_FILE;
		pts_bit32_file = AUDIO_DMX_PTS_BIT32_FILE;
		pts_tsync_file = AUDIO_PTS_FILE;
		break;
	default:
		pts_file = VIDEO_DMX_PTS_FILE;
		pts_bit32_file = VIDEO_DMX_PTS_BIT32_FILE;
		pts_tsync_file = VIDEO_PTS_FILE;
		break;
	}

	retry = 2;
	do {
		memset(pts_buf, 0, sizeof(pts_buf));
		if (AM_FileRead(pts_file, pts_buf, sizeof(pts_buf)) >= 0)
			sscanf(pts_buf, "%d", &pts_dmx);

		memset(pts_buf, 0, sizeof(pts_buf));
		if (AM_FileRead(pts_bit32_file, pts_buf, sizeof(pts_buf)) >= 0)
			sscanf(pts_buf, "%d", &pts_dmx_bit32);

		memset(pts_buf, 0, sizeof(pts_buf));
		if (AM_FileRead(pts_file, pts_buf, sizeof(pts_buf)) >= 0)
			sscanf(pts_buf, "%d", &pts_dmx_2);
	} while (retry-- && (abs(pts_dmx_2 - pts_dmx) > GAP_10SEC));

	if ((abs(pts_dmx_2 - pts_dmx) > GAP_10SEC)) {
		AM_DEBUG(1, "something wrong with the stream's pts");
		*pts = 0L;
		return AM_SUCCESS;
	}

	memset(pts_buf, 0, sizeof(pts_buf));
	if (AM_FileRead(pts_tsync_file, pts_buf, sizeof(pts_buf)) >= 0)
		sscanf(pts_buf, "%i", &pts_tsync);

	*pts = pts_tsync;
	if (pts_dmx_bit32 && (abs(pts_dmx_2 - pts_tsync) < (GAP_10SEC * 3)))
		*pts += 0x100000000L;

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_fe_status(AM_AV_Device_t *dev, int value)
{
    AM_DEBUG(1,"aml_set_fe_status value = %d",value);
    g_festatus = value;
    return AM_SUCCESS;
}

AM_ErrorCode_t aml_get_timeout_real(int timeout, struct timespec *ts)
{
	struct timespec ots;
	int left, diff;

	clock_gettime(CLOCK_REALTIME, &ots);

	ts->tv_sec  = ots.tv_sec + timeout/1000;
	ts->tv_nsec = ots.tv_nsec;

	left = timeout % 1000;
	left *= 1000000;
	diff = 1000000000-ots.tv_nsec;

	if (diff <= left)
	{
		ts->tv_sec++;
		ts->tv_nsec = left-diff;
	}
	else
	{
		ts->tv_nsec += left;
	}

	return AM_SUCCESS;
}

#ifdef open
#undef open
#endif
static void *aml_try_open_crypt(AM_Crypt_Ops_t *ops)
{
	if (ops && ops->open)
		return ops->open();
	return NULL;
}
