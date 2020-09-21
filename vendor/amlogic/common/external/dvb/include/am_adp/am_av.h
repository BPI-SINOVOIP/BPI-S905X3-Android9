/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief AV decoder module
 *
 * AV decoder device can work in the following modes:
 *	- play TS stream
 *	- decode audio ES
 *	- decode video ES
 *	- inject AV data to decoder
 *	- timeshifting play
 *	- local file playing (not available on android)
 *	- JPEG hardware decode (not available on android)
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_AV_H
#define _AM_AV_H

#include "am_types.h"
#include "am_osd.h"
#include "am_evt.h"
#include "am_misc.h"
#include "am_tfile.h"
#include "am_userdata.h"
#include "am_crypt.h"
#include <amports/vformat.h>
#include <amports/aformat.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_AV_VIDEO_CONTRAST_MIN   -1024  /**< The minimum contrast value*/
#define AM_AV_VIDEO_CONTRAST_MAX   1024   /**< The maximum contrast value*/
#define AM_AV_VIDEO_SATURATION_MIN -1024  /**< The minimum saturation value*/
#define AM_AV_VIDEO_SATURATION_MAX 1024   /**< The maximum saturation value*/
#define AM_AV_VIDEO_BRIGHTNESS_MIN -1024  /**< The minimum brightness value*/
#define AM_AV_VIDEO_BRIGHTNESS_MAX 1024   /**< The maximum brightness value*/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of the AV decoder module*/
enum AM_AV_ErrorCode
{
	AM_AV_ERROR_BASE=AM_ERROR_BASE(AM_MOD_AV),
	AM_AV_ERR_INVALID_DEV_NO,          /**< Invalid decoder device number*/
	AM_AV_ERR_BUSY,                    /**< The device has already been openned*/
	AM_AV_ERR_ILLEGAL_OP,              /**< Illegal operation*/
	AM_AV_ERR_INVAL_ARG,               /**< Invalid argument*/
	AM_AV_ERR_NOT_ALLOCATED,           /**< The device has not been allocated*/
	AM_AV_ERR_CANNOT_CREATE_THREAD,    /**< Cannot create a new thread*/
	AM_AV_ERR_CANNOT_OPEN_DEV,         /**< Cannot open the device*/
	AM_AV_ERR_CANNOT_OPEN_FILE,        /**< Cannot open the file*/
	AM_AV_ERR_NOT_SUPPORTED,           /**< Not supported*/
	AM_AV_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_AV_ERR_TIMEOUT,                 /**< Timeout*/
	AM_AV_ERR_SYS,                     /**< System error*/
	AM_AV_ERR_DECODE,                  /**< Decoder error*/
	AM_AV_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief Event type of the AV decoder module*/
enum AM_AV_EventType
{
	AM_AV_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_AV),
	AM_AV_EVT_PLAYER_STATE_CHANGED,    /**< File player's state changed, the parameter is the new state(AM_AV_MPState_t)*/
	AM_AV_EVT_PLAYER_SPEED_CHANGED,    /**< File player's playing speed changed, the parameter is the new speed(0:normal，<0:backward，>0:fast forward)*/
	AM_AV_EVT_PLAYER_TIME_CHANGED,     /**< File player's current time changed，the parameter is the current time*/
	AM_AV_EVT_PLAYER_UPDATE_INFO,      /**< Update the current player information*/
	AM_AV_EVT_VIDEO_WINDOW_CHANGED,    /**< Video window change, the parameter is the new window(AM_AV_VideoWindow_t)*/
	AM_AV_EVT_VIDEO_CONTRAST_CHANGED,  /**< Video contrast changed，the parameter is the new contrast(int 0~100)*/
	AM_AV_EVT_VIDEO_SATURATION_CHANGED,/**< Video saturation changed, the parameter is the new saturation(int 0~100)*/
	AM_AV_EVT_VIDEO_BRIGHTNESS_CHANGED,/**< Video brightness changed, the parameter is the new brightness(int 0~100)*/
	AM_AV_EVT_VIDEO_ENABLED,           /**< Video layer enabled*/
	AM_AV_EVT_VIDEO_DISABLED,          /**< Video layer disabled*/
	AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, /**< Video aspect ratio changed，the parameter is the new aspect ratio(AM_AV_VideoAspectRatio_t)*/
	AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED, /**< Video display mode changed, the parameter is the new display mode(AM_AV_VideoDisplayMode_t)*/
	AM_AV_EVT_AV_NO_DATA,		   /**< Audio/Video data stopped*/
	AM_AV_EVT_AV_DATA_RESUME,	/**< After event AM_AV_EVT_AV_NO_DATA，audio/video data resumed*/
	AM_AV_EVT_VIDEO_ES_END,     /**< Injected video ES data end*/
	AM_AV_EVT_AUDIO_ES_END,     /**< Injected audio ES data end*/
	AM_AV_EVT_VIDEO_SCAMBLED,   /**< Video is scrambled*/
	AM_AV_EVT_AUDIO_SCAMBLED,   /**< Audio is scrambled*/
	AM_AV_EVT_AUDIO_AC3_NO_LICENCE,     /**< AC3 audio has not licence*/
	AM_AV_EVT_AUDIO_AC3_LICENCE_RESUME, /**< AC3 audio resumed*/
	AM_AV_EVT_VIDEO_NOT_SUPPORT,        /**< Video format is not supported*/
	AM_AV_EVT_VIDEO_AVAILABLE,  /**< Cannot get valid video information*/
	AM_AV_EVT_AUDIO_CB, /**< Audio function will implement in cb */
	AM_AV_EVT_VIDEO_RESOLUTION_CHANGED, /**< Video resolution changed, the parameter is the AM_AV_VideoStatus_t with new width&height valid only */
	AM_AV_EVT_VIDEO_AFD_CHANGED, /**< Video AFD info changed, parameter is AM_USERDATA_AFD_t*/
	AM_AV_EVT_VIDEO_CROPPING_CHANGED,    /**< Video cropping change, the parameter is the new cropping window(AM_AV_VideoWindow_t)*/
	AM_AV_EVT_PLAYER_EOF,              /**< Update the current player information*/
	AM_AV_EVT_END
};


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\cond */
/**\brief File player's state*/
typedef enum
{
	AM_AV_MP_STATE_UNKNOWN = 0,        /**< Unknown*/
	AM_AV_MP_STATE_INITING,            /**< Initializing*/
	AM_AV_MP_STATE_NORMALERROR,        /**< Error occured*/
	AM_AV_MP_STATE_FATALERROR,         /**< Fatal error occured*/
	AM_AV_MP_STATE_PARSERED,           /**< The file's header has been parsed*/
	AM_AV_MP_STATE_STARTED,            /**< Start playing*/
	AM_AV_MP_STATE_PLAYING,            /**< Playing*/
	AM_AV_MP_STATE_PAUSED,             /**< Paused*/
	AM_AV_MP_STATE_CONNECTING,         /**< Connecting to the server*/
	AM_AV_MP_STATE_CONNECTDONE,        /**< Connected*/
	AM_AV_MP_STATE_BUFFERING,          /**< Buffering*/
	AM_AV_MP_STATE_BUFFERINGDONE,      /**< Buffering done*/
	AM_AV_MP_STATE_SEARCHING,          /**< Set new playing position*/
	AM_AV_MP_STATE_TRICKPLAY,          /**< Play in trick mode*/
	AM_AV_MP_STATE_MESSAGE_EOD,        /**< Message begin*/
	AM_AV_MP_STATE_MESSAGE_BOD,        /**< Message end*/
	AM_AV_MP_STATE_TRICKTOPLAY,        /**< Trick mode to normal playing mode*/
	AM_AV_MP_STATE_FINISHED,           /**< File finished*/
	AM_AV_MP_STATE_STOPED              /**< Palyer stopped*/
} AM_AV_MPState_t;
/**\endcond */

/**\brief Video window*/
typedef struct
{
	int x;                               /**< X coordinate of the top left corner*/
	int y;                               /**< Y coordinate of the top left corner*/
	int w;                               /**< Width of the window*/
	int h;                               /**< Height of the window*/
} AM_AV_VideoWindow_t;

/**\brief TS stream input source*/
typedef enum
{
	AM_AV_TS_SRC_TS0,                    /**< TS input port 0*/
	AM_AV_TS_SRC_TS1,                    /**< TS input port 1*/
	AM_AV_TS_SRC_TS2,                    /**< TS input port 2*/
	AM_AV_TS_SRC_HIU,                    /**< HIU port (file input)*/
	AM_AV_TS_SRC_HIU1,
	AM_AV_TS_SRC_DMX0,                   /**< Demux 0*/
	AM_AV_TS_SRC_DMX1,                   /**< Demux 1*/
	AM_AV_TS_SRC_DMX2                    /**< Demux 2*/
} AM_AV_TSSource_t;

#if 0
typedef enum {
	AFORMAT_MPEG   = 0,
	AFORMAT_PCM_S16LE = 1,
	AFORMAT_AAC   = 2,
	AFORMAT_AC3   =3,
	AFORMAT_ALAW = 4,
	AFORMAT_MULAW = 5,
	AFORMAT_DTS = 6,
	AFORMAT_PCM_S16BE = 7,
	AFORMAT_FLAC = 8,
	AFORMAT_COOK = 9,
	AFORMAT_PCM_U8 = 10,
	AFORMAT_ADPCM = 11,
	AFORMAT_AMR  = 12,
	AFORMAT_RAAC  = 13,
	AFORMAT_WMA  = 14,
	AFORMAT_WMAPRO    = 15,
	AFORMAT_PCM_BLURAY	= 16,
	AFORMAT_ALAC  = 17,
	AFORMAT_VORBIS    = 18,
	AFORMAT_AAC_LATM   = 19,
	AFORMAT_UNSUPPORT = 20,
	AFORMAT_MAX    = 21
} AM_AV_AFormat_t;
#else
/**
 * Audio format
 *
 * detail definition in "linux/amlogic/amports/aformat.h"
 */
typedef aformat_t AM_AV_AFormat_t;
#endif

#if 0
typedef enum
{
	VFORMAT_MPEG12 = 0,
	VFORMAT_MPEG4,
	VFORMAT_H264,
	VFORMAT_MJPEG,
	VFORMAT_REAL,
	VFORMAT_JPEG,
	VFORMAT_VC1,
	VFORMAT_AVS,
	VFORMAT_YUV,    // Use SW decoder
	VFORMAT_H264MVC,
	VFORMAT_MAX
} AM_AV_VFormat_t;
#else
/**
 * Video format
 *
 * detail definition in "linux/amlogic/amports/vformat.h"
 */
typedef vformat_t AM_AV_VFormat_t;
#endif

/**\brief AV stream package format*/
typedef enum
{
	PFORMAT_ES = 0, /**< ES stream*/
	PFORMAT_PS,     /**< PS stream*/
	PFORMAT_TS,     /**< TS stream*/
	PFORMAT_REAL    /**< REAL file*/
} AM_AV_PFormat_t;

/**\brief Video aspect ratio*/
typedef enum
{
	AM_AV_VIDEO_ASPECT_AUTO,      /**< Automatic*/
	AM_AV_VIDEO_ASPECT_4_3,       /**< 4:3*/
	AM_AV_VIDEO_ASPECT_16_9       /**< 16:9*/
} AM_AV_VideoAspectRatio_t;

/**\brief Video aspect ratio match mode*/
typedef enum
{
	AM_AV_VIDEO_ASPECT_MATCH_IGNORE,     /**< Ignoring orignal aspect ratio*/
	AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX, /**< Letter box match mode*/
	AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN,   /**< Pan scan match mode*/
	AM_AV_VIDEO_ASPECT_MATCH_COMBINED    /**< Combined letter box/pan scan match mode*/
} AM_AV_VideoAspectMatchMode_t;

/**\brief Video display mode*/
typedef enum
{
	AM_AV_VIDEO_DISPLAY_NORMAL,     /**< Normal display mode*/
	AM_AV_VIDEO_DISPLAY_FULL_SCREEN /**< Full screaan display mode*/
} AM_AV_VideoDisplayMode_t;

/**\cond */
/**\brief Freescale parameter*/
typedef enum
{
	AM_AV_FREE_SCALE_DISABLE,     /**< Disable freescale*/
	AM_AV_FREE_SCALE_ENABLE       /**< Enable freescale*/
} AM_AV_FreeScalePara_t;

/**\brief Deinterlace parameter*/
typedef enum
{
	AM_AV_DEINTERLACE_DISABLE,    /**< Disable deinterlace*/
	AM_AV_DEINTERLACE_ENABLE      /**< Enable deinterlace*/
} AM_AV_DeinterlacePara_t;

/**\brief PPMGR parameter*/
typedef enum
{
	AM_AV_PPMGR_DISABLE,          /**< Disable PPMGR*/
	AM_AV_PPMGR_ENABLE,           /**< Enable PPMGR*/
	/*以下为3D模式设置*/
	AM_AV_PPMGR_MODE3D_DISABLE,
	AM_AV_PPMGR_MODE3D_AUTO,
	AM_AV_PPMGR_MODE3D_2D_TO_3D,
	AM_AV_PPMGR_MODE3D_LR,
	AM_AV_PPMGR_MODE3D_BT,
	AM_AV_PPMGR_MODE3D_OFF_LR_SWITCH,
	AM_AV_PPMGR_MODE3D_ON_LR_SWITCH,
	AM_AV_PPMGR_MODE3D_FIELD_DEPTH,
	AM_AV_PPMGR_MODE3D_OFF_3D_TO_2D,
	AM_AV_PPMGR_MODE3D_L_3D_TO_2D,
	AM_AV_PPMGR_MODE3D_R_3D_TO_2D,
	AM_AV_PPMGR_MODE3D_OFF_LR_SWITCH_BT,
	AM_AV_PPMGR_MODE3D_ON_LR_SWITCH_BT,
	AM_AV_PPMGR_MODE3D_OFF_3D_TO_2D_BT,
	AM_AV_PPMGR_MODE3D_L_3D_TO_2D_BT,
	AM_AV_PPMGR_MODE3D_R_3D_TO_2D_BT,
	AM_AV_PPMGR_MODE3D_MAX,
} AM_AV_PPMGRPara_t;
/**\endcond */

/**\brief AV decoder device open parameters*/
typedef struct
{
	int      vout_dev_no;         /**< Video output device number*/
	int      afd_enable;          /**< enable AFD*/
} AM_AV_OpenPara_t;

/**\brief Media file information*/
typedef struct
{
	uint64_t size;                /**< File size in bytes*/
	int      duration;            /**< Total duration in seconds*/
} AM_AV_FileInfo_t;

/**\brief File playing status*/
typedef struct
{
	int      duration;            /**< Total duration in seconds.*/
	int      position;            /**< Current time*/
} AM_AV_PlayStatus_t;

/**\brief JPEG file properties*/
typedef struct
{
	int      width;               /**< JPEG image width*/
	int      height;              /**< JPEG image height*/
	int      comp_num;            /**< Color number*/
} AM_AV_JPEGInfo_t;

/**\brief JPEG rotation parameter*/
typedef enum
{
	AM_AV_JPEG_CLKWISE_0    = 0,  /**< Normal*/
	AM_AV_JPEG_CLKWISE_90   = 1,  /**< Rotate 90 degrees clockwise*/
	AM_AV_JPEG_CLKWISE_180  = 2,  /**< Rotate 180 degrees clockwise*/
	AM_AV_JPEG_CLKWISE_270  = 3   /**< Rotate 270 degrees clockwise*/
} AM_AV_JPEGAngle_t;

/**\brief JPEG decoder options*/
typedef enum
{
	AM_AV_JPEG_OPT_THUMBNAIL_ONLY     = 1, /**< Decode in thumbnail only mode*/
	AM_AV_JPEG_OPT_THUMBNAIL_PREFERED = 2, /**< Decode in thumbnail prefered mode*/
	AM_AV_JPEG_OPT_FULLRANGE          = 4  /**< Normal*/
} AM_AV_JPEGOption_t;

/**\brief Image surface parameters (used in JPEG decoder)*/
typedef struct
{
	int    width;                 /**< Image width, <=0 means use orignal size*/
	int    height;                /**< Image height, <=0 means use orignal size*/
	AM_AV_JPEGAngle_t  angle;     /**< JPEG image ratation parameter*/
	AM_AV_JPEGOption_t option;    /**< JPEG decoder options*/
} AM_AV_SurfacePara_t;

/**\brief Decoder injection data type*/
typedef enum
{
	AM_AV_INJECT_AUDIO,           /**< Audio data*/
	AM_AV_INJECT_VIDEO,           /**< Video data*/
	AM_AV_INJECT_MULTIPLEX        /**< Multiplexed data*/
} AM_AV_InjectType_t;

typedef enum
{
	AM_AV_NO_DRM,
	AM_AV_DRM_WITH_SECURE_INPUT_BUFFER, /**< Use for HLS, input buffer is clear and protected*/
	AM_AV_DRM_WITH_NORMAL_INPUT_BUFFER  /**< Use for IPTV, input buffer is normal and scramble*/
} AM_AV_DrmMode_t;

/**\brief Decoder injection parameters*/
typedef struct
{
	AM_AV_AFormat_t  aud_fmt;     /**< Audio format*/
	AM_AV_VFormat_t  vid_fmt;     /**< Video format*/
	AM_AV_PFormat_t  pkg_fmt;     /**< Package format*/
	int              sub_type;    /**< Subtitle type.*/
	int              aud_id;      /**< Audio ID, -1 means no audio data*/
	int              vid_id;      /**< Video ID, -1 means no video data*/
	int              sub_id;      /**< Subtitle ID, -i means no subtitle data*/
	int              channel;     /**< Audio channel number (used in playing audio PCM data)*/
	int              sample_rate; /**< Audio sample rate (used in playing audio PCM data)*/
	int              data_width;  /**< Audio data width (used in playing audio PCM data)*/
} AM_AV_InjectPara_t;

/**\brief Video decoder status*/
typedef struct
{
	AM_AV_VFormat_t  vid_fmt;     /**< Video format*/
	int              src_w;       /**< Video source width*/
	int              src_h;       /**< Video source height*/
	int              fps;         /**< Frames per second*/
	char             interlaced;  /**< Is interlaced ? */
	int              frames;      /**< Decoded frames number*/
	int              vb_size;     /**< Video buffer size*/
	int              vb_data;     /**< Data size in the video buffer*/
	int              vb_free;     /**< Free size in the video buffer*/
	AM_AV_VideoAspectRatio_t vid_ratio; /**< Video source aspect ratio*/
}AM_AV_VideoStatus_t;

/**\brief Audio decoder status*/
typedef struct
{
	AM_AV_AFormat_t  aud_fmt;     		/**< Audio format*/
	int              sample_rate; 		/**< Sample rate*/
	int              resolution;  		/**< Data width (8/16bits)*/
	int              channels;    		/**< Channel number*/
	unsigned int     frames;      		/**< Decoded frames number*/
	int              ab_size;     		/**< Audio buffer size*/
	int              ab_data;     		/**< Data size in the audio buffer*/
	int              ab_free;     		/**< Free size in the audio buffer*/
	AM_AV_AFormat_t  aud_fmt_orig;     	/**< original audio format*/
	int              sample_rate_orig; 	/**< original sample rate*/
	int              resolution_orig;  	/**< original resolution*/
	int              channels_orig;    	/**< original channel number*/
	int              lfepresent;       	/**< low frequency effects present*/
	int              lfepresent_orig;  	/**< original low frequency effects present*/

}AM_AV_AudioStatus_t;

/**\brief Timeshifting play mode*/
typedef enum
{
	AM_AV_TIMESHIFT_MODE_TIMESHIFTING, /**< Normal timeshifting mode*/
	AM_AV_TIMESHIFT_MODE_PLAYBACK      /**< PVR playback mode*/
}AM_AV_TimeshiftMode_t;

/**\brief Timeshifting media information*/
typedef struct
{
	int duration;              /**< Duration in seconds*/
	char program_name[16];     /**< Program name*/
	
	int vid_pid; /**< Video PID*/
	int vid_fmt; /**< Video format*/

	int aud_cnt; /**< Audio number*/
	struct
	{
		int pid; /**< Audio PID*/
		int fmt; /**< Audio format*/
		char lang[4]; /**< Lanuguage descripton*/
	}audios[8]; /**< Audio information array*/

	int sub_cnt; /**< Subtitle number*/
	struct
	{
		int pid;  /**< Subtitle PID*/
		int type; /**< Subtitle type*/
		int composition_page; /**< DVB subtitle's composition page*/
		int ancillary_page;   /**< DVB subtitle's ancillary page*/
		int magzine_no; /**< Teletext subtitle's magzine number*/
		int page_no;  /**< Teletext subtitle's page number*/
		char lang[4]; /**< Language description*/
	}subtitles[8]; /**< Subtitle information array*/

	int ttx_cnt; /**< Teletext number*/
	struct
	{
		int pid; /**< Teletext PID*/
		int magzine_no; /**< Teletext magzine number*/
		int page_no;    /**< Teletext page number*/
		char lang[4];   /**< Teletext language description*/
	}teletexts[8]; /**< Teletext information array*/
}AM_AV_TimeshiftMediaInfo_t;

/**\brief Timeshift playing parameters*/
typedef struct
{
	int              dmx_id;      /**< Demux device index used*/
	char             file_path[256];     /**< File path*/
	
	AM_AV_TimeshiftMode_t mode;   /**< Playing mode*/
	AM_AV_TimeshiftMediaInfo_t media_info; /**< Media information*/

	AM_TFile_t tfile;
	int offset_ms;
	AM_Bool_t start_paused;
#ifdef SUPPORT_CAS
	int  secure_enable;			/*Use for cas PVR/TimeShift playback*/
	uint8_t *secure_buffer;	/*Use for sotre cas decrypt pvr data**/
	char cas_file_path[256];/*Store Cas info file*/
	AM_CAS_decrypt dec_cb;
	uint32_t cb_param;
#endif
} AM_AV_TimeshiftPara_t;

/**\brief Timeshift lpaying information*/
typedef struct
{
	int                   current_time; /**< Current time in seconds*/
	int                   full_time;    /**< Total length in seconds*/
	int                   status;       /**< Status*/
}AM_AV_TimeshiftInfo_t;

/**\brief Player information*/
typedef struct player_info
{
	char *name;
	int last_sta;
	int status;		   /*stop,pause	*/
	int full_time;	   /*Seconds	*/
	int current_time;  /*Seconds	*/
	int current_ms;	/*ms*/
	int last_time;		
	int error_no;  
	int start_time;
	int pts_video;
	int pts_pcrscr;
	int current_pts;
	long curtime_old_time;    
	unsigned int video_error_cnt;
	unsigned int audio_error_cnt;
	float audio_bufferlevel;
	float video_bufferlevel;
}player_info_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Open an AV decoder device
 * \param dev_no AV decoder device number
 * \param[in] para Device open parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_Open(int dev_no, const AM_AV_OpenPara_t *para);

/**\brief Close an AV decoder device
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_Close(int dev_no);

/**\brief Set the decoder's TS input source
 * \param dev_no AV decoder device number
 * \param src TS input source
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetTSSource(int dev_no, AM_AV_TSSource_t src);

/**\brief Start TS stream playing with PCR parameter
 * \param dev_no AV decoder device number
 * \param vpid Video PID
 * \param apid Audio PID
 * \param pcrpid PCR PID
 * \param vfmt Video format
 * \param afmt Audio format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartTSWithPCR(int dev_no, uint16_t vpid, uint16_t apid, uint16_t pcrpid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt);

/**\brief Start TS stream playing
 * \param dev_no AV decoder device number
 * \param vpid Video PID
 * \param apid Audio PID
 * \param vfmt Video format
 * \param afmt Audio format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartTS(int dev_no, uint16_t vpid, uint16_t apid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt);


/**\brief Stop TS stream playing
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StopTS(int dev_no);

/**\brief Start a media file playing
 * \param dev_no AV decoder device number
 * \param[in] fname Filename
 * \param loop If play in loop mode.
 * \param pos Start position in seconds
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartFile(int dev_no, const char *fname, AM_Bool_t loop, int pos);

/**\brief Switch to file play mode but do not start
 * \param dev_no AV decoder device number
 * \param[in] fname Filename
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_AddFile(int dev_no, const char *fname);

/**\brief Start play the file. Invoke after AM_AV_AddFile
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartCurrFile(int dev_no);

/**\brief Start file playing
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StopFile(int dev_no);

/**\brief Pause the file playing
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_PauseFile(int dev_no);

/**\brief Resume the file playing
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_ResumeFile(int dev_no);

/**\brief Set the current file playing position
 * \param dev_no AV decoder device number
 * \param pos Playing position in seconds
 * \param start Start directly or pause on the position
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SeekFile(int dev_no, int pos, AM_Bool_t start);

/**\brief Fast forward the file
 * \param dev_no AV decoder device number
 * \param speed Playing speed (0 means normal)
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_FastForwardFile(int dev_no, int speed);

/**\brief Fast backward the file
 * \param dev_no AV decoder device number
 * \param speed Playing speed
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_FastBackwardFile(int dev_no, int speed);

/**\brief Get the current playing file's information
 * \param dev_no AV decoder device number
 * \param[out] info Return the file's information
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetCurrFileInfo(int dev_no, AM_AV_FileInfo_t *info);

/**\brief Get the current player status
 * \param dev_no AV decoder device number
 * \param[out] status Return the current player status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetPlayStatus(int dev_no, AM_AV_PlayStatus_t *status);

/**\brief Get the JPEG image file's information
 * \param dev_no AV decoder device number
 * \param[in] fname JPEG file name
 * \param[out] info Return the image's information
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetJPEGInfo(int dev_no, const char *fname, AM_AV_JPEGInfo_t *info);

/**\brief Get the JPEG image data's information
 * \param dev_no AV decoder device number
 * \param[in] data JPEG image data buffer
 * \param len JPEG data length in bytes
 * \param[out] info Return the image's information
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetJPEGDataInfo(int dev_no, const uint8_t *data, int len, AM_AV_JPEGInfo_t *info);

/**\brief Decode the JPEG image file
 * \param dev_no AV decoder device number
 * \param[in] fname JPEG filename
 * \param[in] para JPEG output parameters
 * \param[out] surf Return the JPEG image
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_DecodeJPEG(int dev_no, const char *fname, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **surf);

/**\brief Decode the JPEG data
 * \param dev_no AV decoder device number
 * \param[in] data JPEG data buffer
 * \param len JPEG data length in bytes
 * \param[in] para JPEG output parameters
 * \param[out] surf Return the JPEG image
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_DacodeJPEGData(int dev_no, const uint8_t *data, int len, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **surf);

/**\brief Decode video ES data from a file
 * \param dev_no AV decoder device number
 * \param format Video format
 * \param[in] fname ES filename
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartVideoES(int dev_no, AM_AV_VFormat_t format, const char *fname);

/**\brief Decode video ES data from a buffer
 * \param dev_no AV decoder device number
 * \param format Video format
 * \param[in] data Video ES buffer
 * \param len Video data length in bytes
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartVideoESData(int dev_no, AM_AV_VFormat_t format, const uint8_t *data, int len);

/**\brief Stop video ES data decoding
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StopVideoES(int dev_no);

/**\brief Start audio ES data decoding from a file
 * \param dev_no AV decoder device number
 * \param format Audio format
 * \param[in] fname Audio ES filename
 * \param times Playing times, <=0 means loop forever
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartAudioES(int dev_no, AM_AV_AFormat_t format, const char *fname, int times);

/**\brief Start audio ES data decoding from a buffer
 * \param dev_no AV decoder device number
 * \param format Audio format
 * \param[in] data Audio ES buffer
 * \param len Audio data length in bytes
 * \param times Playing times, <=0 means loop forever
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartAudioESData(int dev_no, AM_AV_AFormat_t format, const uint8_t *data, int len, int times);

/**\brief Stop audio ES decoding
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StopAudioES(int dev_no);

/**\brief Enable/diable decoder's DRM mode
 * \param dev_no AV decoder device number
 * \param[in] enable enable or disable DRM mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetDRMMode(int dev_no, AM_AV_DrmMode_t drm_mode);

/**\brief Start AV data injection playing mode
 * \param dev_no AV decoder device number
 * \param[in] para Injection parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartInject(int dev_no, const AM_AV_InjectPara_t *para);

/**\brief In injection mode, inject AV data to decoder
 * \param dev_no AV decoder device number
 * \param type Input data type
 * \param[in] data AV data buffer
 * \param[in,out] size Input the data length, and return the injected data length
 * \param timeout Timeout in milliseconds
 * The function will wait until the decoder's input buffer has enough space to receive the AV data.
 * >0 the function will return immediately to this time.
 * <0 means waiting forever.
 * =0 means return immediately.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_InjectData(int dev_no, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout);

/**\brief Pause the decoder in injection mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_PauseInject(int dev_no);

/**\brief Resume the decoder in injection mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_ResumeInject(int dev_no);

/**\brief In injection mode, change the video
 * \param dev_no AV decoder device number
 * \param vid Video PID
 * \param vfmt Video format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetInjectVideo(int dev_no, int vid, AM_AV_VFormat_t vfmt);

/**\brief In injection mode, change the audio
 * \param dev_no AV decoder device number
 * \param aid Audio PID
 * \param afmt Audio format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetInjectAudio(int dev_no, int aid, AM_AV_AFormat_t afmt);

/**\brief In injection mode, change the subtitle
 * \param dev_no AV decoder device number
 * \param sid Subtitle's PID
 * \param stype The subtitle's type.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetInjectSubtitle(int dev_no, int sid, int stype);

/**\brief Stop the injection mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StopInject(int dev_no);

/**\brief Set the video window
 * \param dev_no AV decoder device number
 * \param x X coordinate of the top left corner
 * \param y Y coordinate of the top left corner
 * \param w Window width
 * \param h Window height
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoWindow(int dev_no, int x, int y, int w, int h);

/**\brief Set the video cropping
 * \param dev_no AV decoder device number
 * \param Voffset0 vertical crop of the top left corner
 * \param Hoffset0 horizontal crop of the top left corner
 * \param Voffset1 vertical crop of the bottom right corner
 * \param Hoffset1 horizontal crop of the bottom right corner
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoCropping(int dev_no, int Voffset0, int Hoffset0, int Voffset1, int Hoffset1);

/**\brief Get the video window
 * \param dev_no AV decoder device number
 * \param[out] x Return x coordinate of the top left corner
 * \param[out] y Return y coordinate of the top left corner
 * \param[out] w Return the window width
 * \param[out] h Return the window height
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoWindow(int dev_no, int *x, int *y, int *w, int *h);

/**\brief Set the video contrast (0~100)
 * \param dev_no AV decoder device number
 * \param val New contrast value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoContrast(int dev_no, int val);

/**\brief Get the current video contrast value
 * \param dev_no AV decoder device number
 * \param[out] val Return the current video contrast value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoContrast(int dev_no, int *val);

/**\brief Set the video saturation value (0~100)
 * \param dev_no AV decoder device number
 * \param val The new video saturation value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoSaturation(int dev_no, int val);

/**\brief Get the current video saturation value
 * \param dev_no AV decoder device number
 * \param[out] val Return the saturation value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoSaturation(int dev_no, int *val);

/**\brief Set the video brightness value (0~100)
 * \param dev_no AV decoder device number
 * \param val New video brightness value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoBrightness(int dev_no, int val);

/**\brief Get the current video brightness value
 * \param dev_no AV decoder device number
 * \param[out] val Return the current video brightness value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoBrightness(int dev_no, int *val);

/**\brief Enable the video layer
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_EnableVideo(int dev_no);

/**\brief Disable the video layer
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_DisableVideo(int dev_no);

/**\brief Set the video aspect ratio
 * \param dev_no AV decoder device number
 * \param ratio New aspect ratio value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t ratio);

/**\brief Get the video aspect ratio
 * \param dev_no AV decoder device number
 * \param[out] ratio Return the video aspect ratio
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t *ratio);

/**\brief Set the video aspect ratio match mode
 * \param dev_no AV decoder device number
 * \param mode New match mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoAspectMatchMode(int dev_no, AM_AV_VideoAspectMatchMode_t mode);

/**\brief Get the video aspect ratio match mode
 * \param dev_no AV decoder device number
 * \param[out] mode Return the current match mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoAspectMatchMode(int dev_no, AM_AV_VideoAspectMatchMode_t *mode);

/**\brief Set the video layer disabled automatically when video stopped
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_EnableVideoBlackout(int dev_no);

/**\brief Set the video layer do not disabled when video stopped
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_DisableVideoBlackout(int dev_no);

/**\brief Set the video display mode
 * \param dev_no AV decoder device number
 * \param mode New display mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t mode);

/**\brief Get the current video display mode
 * \param dev_no AV decoder device number
 * \param[out] mode Return the current display mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t *mode);

/**\brief Clear the video layer
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_ClearVideoBuffer(int dev_no);

/**\brief Get the video frame data (after video stopped)
 *
 * This function is not available on android.
 * \param dev_no AV decoder device number
 * \param[in] para Surface create parameters
 * \param[out] s Return the video frame
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoFrame(int dev_no, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s);

/**\brief Get the current video decoder's status
 * \param dev_no AV decoder device number
 * \param[out] status Return the current video decoder's status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoStatus(int dev_no, AM_AV_VideoStatus_t *status);

/**\brief Get the current audio decoder's status
 * \param dev_no AV decoder device number
 * \param[out] status Return the current audio decoder's status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetAudioStatus(int dev_no, AM_AV_AudioStatus_t *status);


/**\brief Start the timeshifting mode
 * \param dev_no AV decoder device number
 * \param[in] para Timeshifting parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StartTimeshift(int dev_no, const AM_AV_TimeshiftPara_t *para);

/**\brief In timeshifting mode, write AV data to decoder
 * \param dev_no AV decoder device number
 * \param[in] data Injected data buffer
 * \param size Data length in the buffer
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_TimeshiftFillData(int dev_no, uint8_t *data, int size);

/**\brief Stop timeshifting mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_StopTimeshift(int dev_no);

/**\brief Start playing in timeshifting mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_PlayTimeshift(int dev_no);

/**\brief Pause playing in timeshifting mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_PauseTimeshift(int dev_no);

/**\brief Resume playing in timeshifting mode
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_ResumeTimeshift(int dev_no);

/**\brief Set the current playing position in timeshifting mode
 * \param dev_no AV decoder device number
 * \param pos New position in seconds
 * \param start Start playing directly after seek
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SeekTimeshift(int dev_no, int pos, AM_Bool_t start);

/**\brief Fast forward playing in timeshifting mode
 * \param dev_no AV decoder device number
 * \param speed Playing speed
 * 0 means normal speed.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_FastForwardTimeshift(int dev_no, int speed);

/**\brief Fast backward playing in timeshifting mode
 * \param dev_no AV decoder device number
 * \param speed Playing speed
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_FastBackwardTimeshift(int dev_no, int speed);

/**\brief Switch audio in timeshifting mode
 * \param dev_no AV decoder device number
 * \param apid The new audio PID
 * \param afmt The new audio format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SwitchTimeshiftAudio(int dev_no, int apid, int afmt);

/**\brief Get the current timeshifting play information
 * \param dev_no AV decoder device number
 * \param [out] info Return the timeshifting information
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetTimeshiftInfo(int dev_no, AM_AV_TimeshiftInfo_t *info);

/**\brief Get the current timeshifting play information
 * \param dev_no AV decoder device number
 * \param [out] tfile the tfile used for timeshift
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetTimeshiftTFile(int dev_no, AM_TFile_t *tfile);

/**\cond */
/**\brief Set video path parameters
 * \param dev_no AV decoder device number
 * \param fs free scale parameter
 * \param di deinterlace parameter
 * \param pp PPMGR parameter
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVPathPara(int dev_no, AM_AV_FreeScalePara_t fs, AM_AV_DeinterlacePara_t di, AM_AV_PPMGRPara_t pp);
/**\endcond */

/**\brief Switch audio in TS playing mode
 * \param dev_no AV decoder device number
 * \param apid New audio PID
 * \param afmt New audio format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SwitchTSAudio(int dev_no, uint16_t apid, AM_AV_AFormat_t afmt);

/**
 * Reset the audio decoder
 * \param dev_no AV decoder device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_ResetAudioDecoder(int dev_no);


/**\brief Used to set /sys/module/amvdec_h264/parameters/error_recovery_mode to choose display mosaic or not
 * \param dev_no AV decoder device number
 * \param error_recovery_mode : 0 ,skip mosaic and reset vdec,2 skip mosaic ,3 display mosaic
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetVdecErrorRecoveryMode(int dev_no, uint8_t error_recovery_mode);

/**
 * Set audio description output in TS playing mode
 * \param dev_no AV decoder device number
 * \param enable 1 to enable AD output, 0 to disable AD output
 * \param apid AD audio PID
 * \param afmt AD audio format
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetAudioAd(int dev_no, int enable, uint16_t apid, AM_AV_AFormat_t afmt);

typedef struct _AUDIO_PARMS_
{
	int cmd;
	int param1;
	int param2;
}AudioParms;

#if 0
typedef struct _AUDIO_STATUS_
{
	int sample_rate_orig;
	int channels_orig;
	int lfepresent_orig;
	int channels;
	int sample_rate;
}AudioStatus;
#endif

#define ADEC_START_DECODE 		1
#define ADEC_PAUSE_DECODE 		2
#define ADEC_RESUME_DECODE 		3
#define ADEC_STOP_DECODE  		4
#define ADEC_SET_DECODE_AD 		5
#define ADEC_SET_VOLUME 		6
#define ADEC_SET_MUTE			7
#define ADEC_SET_OUTPUT_MODE 	8
#define ADEC_SET_PRE_GAIN		9
#define ADEC_SET_PRE_MUTE 		10
#define ADEC_GET_STATUS			11

typedef void (*AM_AV_Audio_CB_t)(int event_type, AudioParms* parm, void *user_data);

extern AM_ErrorCode_t AM_AV_SetAudioCallback(int dev_no,AM_AV_Audio_CB_t cb,void *user_data);

/**
 * brief Get current video pts
 * \param dev_no AV decoder device number
 * \param[out] Return the 33-bit pts value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetVideoPts(int dev_no, uint64_t *pts);

/**
 * brief Get current audio pts
 * \param dev_no AV decoder device number
 * \param[out] Return the 33-bit pts value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_GetAudioPts(int dev_no, uint64_t *pts);

/**
 * brief Set Crypt operators
 * \param dev_no AV decoder device number
 * \param ops Crypt ops
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_SetCryptOps(int dev_no, AM_Crypt_Ops_t *ops);

/**
 * brief Set Crypt operators
 * \param dev_no AV decoder device number
 * \param value Fendend Status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AV_setFEStatus(int dev_no,int value);

#ifdef __cplusplus
}
#endif

#endif

