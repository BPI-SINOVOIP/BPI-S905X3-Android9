/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 音视频解码模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_AV_INTERNAL_H
#define _AM_AV_INTERNAL_H

#include <am_av.h>
#include <limits.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct AM_AV_Driver    AM_AV_Driver_t;
typedef struct AM_AV_Device    AM_AV_Device_t;
typedef struct AV_TSPlayer     AV_TSPlayer_t;
typedef struct AV_FilePlayer   AV_FilePlayer_t;
typedef struct AV_DataPlayer   AV_DataPlayer_t;
typedef struct AV_InjectPlayer AV_InjectPlayer_t;
typedef struct AV_TimeshiftPlayer AV_TimeshiftPlayer_t;

typedef struct {
	pthread_t av_mon_thread;
	AM_Bool_t av_thread_running;
} AV_Monitor_t;

/**\brief TS播放参数*/
typedef struct
{
	uint16_t        vpid;            /**< 视频流PID*/
	uint16_t        apid;            /**< 音频流PID*/
	uint16_t        pcrpid;          /**< PCR PID*/
	AM_AV_VFormat_t vfmt;            /**< 视频流格式*/
	AM_AV_AFormat_t afmt;            /**< 音频流格式*/
	uint16_t        sub_apid;        /**< sub音频流PID*/
	AM_AV_AFormat_t sub_afmt;        /**< sub音频流格式*/
	AM_AV_DrmMode_t drm_mode;
} AV_TSPlayPara_t;

/**\brief TS流播放器*/
struct AV_TSPlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	AM_AV_TSSource_t src;            /**< TS源*/
	AV_Monitor_t     mon;
	int             av_start_time;
	AV_TSPlayPara_t play_para;	/**< 播放参数*/
};

/**\brief 文件播放器*/
struct AV_FilePlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	char             name[PATH_MAX]; /**< 文件名*/
	int              speed;          /**< 播放速度*/
};

/**\brief 数据播放参数*/
typedef struct
{
	const uint8_t  *data;            /**< 数据缓冲区指针*/
	int             len;             /**< 数据缓冲区长度*/
	int             times;           /**< 播放次数*/
	int             fd;              /**< 数据对应的文件描述符*/
	AM_Bool_t       need_free;       /**< 缓冲区播放完成后需要释放*/
	void           *para;            /**< 播放器相关参数指针*/
} AV_DataPlayPara_t;

/**\brief 数据播放器(ES, JPEG)*/
struct AV_DataPlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	AV_DataPlayPara_t para;          /**< 播放参数*/
};

typedef struct {
	AM_AV_InjectPara_t para;
	int                sub_aud_pid;
	AM_AV_AFormat_t    sub_aud_fmt;
	AM_AV_DrmMode_t    drm_mode;
}AV_InjectPlayPara_t;

/**\brief 数据注入播放器参数*/
struct AV_InjectPlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	AV_InjectPlayPara_t para;
};

typedef struct {
	AM_AV_TimeshiftPara_t para;
	int                sub_aud_pid;
	AM_AV_AFormat_t    sub_aud_fmt;
}AV_TimeShiftPlayPara_t;

/**\brief Timeshift播放器参数*/
struct AV_TimeshiftPlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	AV_Monitor_t    mon;
	AV_TimeShiftPlayPara_t para;
};

/**\brief JPEG解码参数*/
typedef struct
{
	AM_OSD_Surface_t *surface;       /**< 返回JPEG图像*/
	AM_AV_SurfacePara_t para;        /**< 图片参数*/
} AV_JPEGDecodePara_t;

/**\brief 播放模式*/
typedef enum
{
	AV_PLAY_STOP     = 0,            /**< 播放停止*/
	AV_PLAY_VIDEO_ES = 1,            /**< 播放视频ES流*/
	AV_PLAY_AUDIO_ES = 2,            /**< 播放音频ES流*/
	AV_PLAY_TS       = 4,            /**< TS流*/
	AV_PLAY_FILE     = 8,            /**< 播放文件*/
	AV_GET_JPEG_INFO = 16,           /**< 读取JPEG图片信息*/
	AV_DECODE_JPEG   = 32,           /**< 解码JPEG图片*/
	AV_INJECT        = 64,           /**< 数据注入*/
	AV_TIMESHIFT	 = 128			 /**< Timeshift*/
} AV_PlayMode_t;

#define AV_MODE_ALL  (AV_PLAY_VIDEO_ES|AV_PLAY_AUDIO_ES|AV_PLAY_TS|AV_PLAY_FILE|AV_GET_JPEG_INFO|AV_DECODE_JPEG|AV_INJECT|AV_TIMESHIFT)

/**\brief 播放命令*/
typedef enum
{
	AV_PLAY_START,                   /**< 开始播放*/
	AV_PLAY_PAUSE,                   /**< 暂停播放*/
	AV_PLAY_RESUME,                  /**< 恢复播放*/
	AV_PLAY_FF,                      /**< 快速前进*/
	AV_PLAY_FB,                      /**< 快速后退*/
	AV_PLAY_SEEK,                    /**< 设定播放位置*/
	AV_PLAY_RESET_VPATH,             /**< 重新设定Video path*/
	AV_PLAY_SWITCH_AUDIO			 /**< 切换音频*/
} AV_PlayCmd_t;

/**\brief 文件播放参数*/
typedef struct
{
	const char     *name;            /**< 文件名*/
	AM_Bool_t       loop;            /**< 是否循环播放*/
	AM_Bool_t       start;           /**< 是否开始播放*/
	int             pos;             /**< 开始播放的位置*/
} AV_FilePlayPara_t;

/**\brief 文件播放位置设定参数*/
typedef struct
{
	int             pos;             /**< 播放位置*/
	AM_Bool_t       start;           /**< 是否开始播放*/
} AV_FileSeekPara_t;

/**\brief 视频参数设置类型*/
typedef enum
{
	AV_VIDEO_PARA_WINDOW,
	AV_VIDEO_PARA_CONTRAST,
	AV_VIDEO_PARA_SATURATION,
	AV_VIDEO_PARA_BRIGHTNESS,
	AV_VIDEO_PARA_ENABLE,
	AV_VIDEO_PARA_BLACKOUT,
	AV_VIDEO_PARA_RATIO,
	AV_VIDEO_PARA_RATIO_MATCH,
	AV_VIDEO_PARA_MODE,
	AV_VIDEO_PARA_CLEAR_VBUF,
	AV_VIDEO_PARA_ERROR_RECOVERY_MODE,
	AV_VIDEO_PARA_CROP,
} AV_VideoParaType_t;

/**\brief 视频窗口参数*/
typedef struct
{
	int    x;
	int    y;
	int    w;
	int    h;
} AV_VideoWindow_t;

/**\brief 音视频解码驱动*/
struct AM_AV_Driver
{
	AM_ErrorCode_t (*open)(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para);
	AM_ErrorCode_t (*close)(AM_AV_Device_t *dev);
	AM_ErrorCode_t (*open_mode)(AM_AV_Device_t *dev, AV_PlayMode_t mode);
	AM_ErrorCode_t (*start_mode)(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para);
	AM_ErrorCode_t (*close_mode)(AM_AV_Device_t *dev, AV_PlayMode_t mode);
	AM_ErrorCode_t (*ts_source)(AM_AV_Device_t *dev, AM_AV_TSSource_t src);
	AM_ErrorCode_t (*file_cmd)(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
	AM_ErrorCode_t (*inject_cmd)(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
	AM_ErrorCode_t (*file_status)(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status);
	AM_ErrorCode_t (*file_info)(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info);
	AM_ErrorCode_t (*set_video_para)(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val);
	AM_ErrorCode_t (*inject)(AM_AV_Device_t *dev, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout);
	AM_ErrorCode_t (*video_frame)(AM_AV_Device_t *dev, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s);
	AM_ErrorCode_t (*get_audio_status)(AM_AV_Device_t *dev, AM_AV_AudioStatus_t *s);
	AM_ErrorCode_t (*get_video_status)(AM_AV_Device_t *dev, AM_AV_VideoStatus_t *s);
	AM_ErrorCode_t (*timeshift_fill)(AM_AV_Device_t *dev, uint8_t *data, int size);
	AM_ErrorCode_t (*timeshift_cmd)(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para);
	AM_ErrorCode_t (*get_timeshift_info)(AM_AV_Device_t *dev, AM_AV_TimeshiftInfo_t *info);
	AM_ErrorCode_t (*set_vpath)(AM_AV_Device_t *dev);
	AM_ErrorCode_t (*switch_ts_audio)(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt);
	AM_ErrorCode_t (*reset_audio_decoder)(AM_AV_Device_t *dev);
	AM_ErrorCode_t (*set_drm_mode)(AM_AV_Device_t *dev, int enable);
	AM_ErrorCode_t (*set_audio_ad)(AM_AV_Device_t *dev, int enable, uint16_t apid, AM_AV_AFormat_t afmt);
	AM_ErrorCode_t (*set_inject_audio)(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt);
	AM_ErrorCode_t (*set_inject_subtitle)(AM_AV_Device_t *dev, uint16_t spid, int stype);
	AM_ErrorCode_t (*timeshift_get_tfile)(AM_AV_Device_t *dev, AM_TFile_t *tfile);
	void (*set_audio_cb)(AM_AV_Device_t *dev,AM_AV_Audio_CB_t cb,void *user_data);
	AM_ErrorCode_t (*get_pts)(AM_AV_Device_t *dev, int type, uint64_t *pts);
    AM_ErrorCode_t (*set_fe_status)(AM_AV_Device_t *dev, int value);
};

/**\brief 音视频播放参数*/
typedef struct {
	AV_InjectPlayPara_t   inject;
	AV_TSPlayPara_t       ts;
	AV_FilePlayPara_t     file;
	char                  file_name[256];
	AM_Bool_t             pause;
	AM_Bool_t             loop;
	AM_Bool_t             start;
	int                   speed;
	int                   pos;
	AV_DataPlayPara_t     aes;
	AV_DataPlayPara_t     ves;
	AV_TimeShiftPlayPara_t   time_shift;
	AM_AV_DrmMode_t       drm_mode;
}AM_AV_PlayPara_t;

/**\brief 音视频解码设备*/
struct AM_AV_Device
{
	int              dev_no;         /**< 设备ID*/
	int              vout_dev_no;    /**< 视频解码设备对应的视频输出设备*/
	int              vout_w;         /**< 视频输出宽度*/
	int              vout_h;         /**< 视频输出高度*/
	const AM_AV_Driver_t  *drv;      /**< 解码驱动*/
	void            *drv_data;       /**< 解码驱动相关数据*/
	AV_TSPlayer_t    ts_player;      /**< TS流播放器*/
	AV_FilePlayer_t  file_player;    /**< 文件播放器*/
	AV_DataPlayer_t  vid_player;     /**< 视频数据播放器*/
	AV_DataPlayer_t  aud_player;     /**< 音频数据播放器*/
	AV_InjectPlayer_t inject_player; /**< 数据注入播放器*/
	AV_TimeshiftPlayer_t timeshift_player;	/**< Timeshift 播放器*/
	pthread_mutex_t  lock;           /**< 设备资源保护互斥体*/
	AM_Bool_t        openned;        /**< 设备是否已经打开*/
	AV_PlayMode_t    mode;           /**< 当前播放模式*/
	int              video_x;        /**< 当前视频窗口左上角X坐标*/
	int              video_y;        /**< 当前视频窗口左上角Y坐标*/
	int              video_w;        /**< 当前视频窗口宽度*/
	int              video_h;        /**< 当前视频窗口高度*/
	int              video_contrast;       /**< 视频对比度*/
	int              video_saturation;     /**< 视频饱和度*/
	int              video_brightness;     /**< 视频亮度*/
	AM_Bool_t        audio_switch;         /**< 音频是否需要切换*/
	AM_Bool_t        video_enable;         /**< 视频层是否显示*/
	AM_Bool_t        video_blackout;       /**< 无数据时是否黑屏*/
	AM_AV_VideoAspectRatio_t     video_ratio;  /**< 视频长宽比*/
	AM_AV_VideoAspectMatchMode_t video_match;  /**< 长宽比匹配模式*/
	AM_AV_VideoDisplayMode_t     video_mode;   /**< 视频显示模式*/
	AM_AV_FreeScalePara_t        vpath_fs;       /**< free scale参数*/
	AM_AV_DeinterlacePara_t      vpath_di;       /**< deinterlace参数*/
	AM_AV_PPMGRPara_t            vpath_ppmgr;    /**< ppmgr参数*/
	AM_AV_PlayPara_t curr_para;      /**< 当前播放参数*/
	void				*ad_date;
	int                          afd_enable;
	AM_USERDATA_AFD_t            afd;
	AM_Crypt_Ops_t               *crypt_ops;
	void                         *cryptor;
	AM_Bool_t	replay_enable;		/*enable or disable replay when abnormal*/

	/*for audio switching*/
	uint16_t                     alt_apid;
	AM_AV_AFormat_t              alt_afmt;
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

