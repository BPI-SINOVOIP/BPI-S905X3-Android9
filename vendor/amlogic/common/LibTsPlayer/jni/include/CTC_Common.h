/*
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _CTC_COMMON_H
#define _CTC_COMMON_H

#include <stdint.h>
#include <amports/vformat.h>
#include <amports/aformat.h>


#define AML_INVALID_HANDLE  (0)

#define AML_SUCCESS     (0)
#define AML_FAILURE     (-1)

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

const int64_t kUnknownPTS = INT64_MIN;

///////////////////////////////////////////////////////////////////////////////
/**
 * \brief initial parameters used for CTC_MediaProcessor object
 */
struct CTC_InitialParameter {
    uint32_t        version;
    uint8_t         useOmx;            //force enable omx decoder
    uint8_t         useSoftDemux;
    uint8_t         isEsSource;        //setParameter KEY_PARAMETER_AML_PLAYER_SET_TS_OR_ES to control.
    uint32_t        drmIdentifier;
    uint32_t        interfaceExtension;
    uint32_t        reserved;
    uint8_t         userDataLength;
    uint8_t         userData[0];
};

#define     CTC_EXTENSION_ZTE     (1 << 0)

///////////////////////////////////////////////////////////////////////////////
typedef struct{
	uint16_t		pid;			//pid
	uint32_t		nVideoWidth;
	uint32_t		nVideoHeight;
	uint32_t		nFrameRate;
	vformat_t		vFmt;
	unsigned long	cFmt;
	uint32_t		nExtraSize;
	uint8_t			*pExtraData;
}VIDEO_PARA_T, *PVIDEO_PARA_T;

typedef struct{
	uint16_t		pid;			//pid
	uint32_t		nChannels;
	uint32_t        nSampleRate;
	aformat_t		aFmt;
    uint32_t        block_align;
    uint32_t        bit_per_sample;
	uint32_t        nExtraSize;
	uint8_t         *pExtraData;
}AUDIO_PARA_T, *PAUDIO_PARA_T;

#define MAX_AUDIO_PARAM_SIZE 10

typedef enum {
    /* subtitle codecs */
    CTC_CODEC_ID_DVD_SUBTITLE = 0x17000,
    CTC_CODEC_ID_DVB_SUBTITLE,
    CTC_CODEC_ID_TEXT,  ///< raw UTF-8 text
    CTC_CODEC_ID_XSUB,
    CTC_CODEC_ID_SSA,
    CTC_CODEC_ID_MOV_TEXT,
    CTC_CODEC_ID_HDMV_PGS_SUBTITLE,
    CTC_CODEC_ID_DVB_TELETEXT,
    CTC_CODEC_ID_SRT,
    CTC_CODEC_ID_MICRODVD,
} SUBTITLE_TYPE;

#define MAX_SUBTITLE_PARAM_SIZE 10

typedef struct {
    uint16_t        pid;            //pid
	SUBTITLE_TYPE   sub_type;
}SUBTITLE_PARA_T, *PSUBTITLE_PARA_T;

typedef enum {
    IPTV_PLAYER_EVT_STREAM_VALID=0, //input stream is invalid
    IPTV_PLAYER_EVT_FIRST_PTS,      //first frame decoded event
    IPTV_PLAYER_EVT_ERROR,          //decoder error

    //EXTENTION, not defined by CTC standard
    IPTV_PLAYER_EVT_VOD_EOS,       //VOD EOS event
    IPTV_PLAYER_EVT_ABEND,         //under flow event
    IPTV_PLAYER_EVT_PLAYBACK_ERROR,// playback error event
    IPTV_PLAYER_EVT_VID_FRAME_ERROR =0x200,// 视频解码错误
    IPTV_PLAYER_EVT_VID_DISCARD_FRAME,// 视频解码丢帧
    IPTV_PLAYER_EVT_VID_DEC_UNDERFLOW,// 视频解码下溢
    IPTV_PLAYER_EVT_VID_PTS_ERROR,// 视频解码Pts错误
    IPTV_PLAYER_EVT_AUD_FRAME_ERROR,// 音频解码错误
    IPTV_PLAYER_EVT_AUD_DISCARD_FRAME,// 音频解码丢弃
    IPTV_PLAYER_EVT_AUD_DEC_UNDERFLOW,//音频解码下溢
    IPTV_PLAYER_EVT_AUD_PTS_ERROR,// 音频PTS错误
    IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_START=0x307,
    IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_END=0x308,
    IPTV_PLAYER_EVT_VID_MOSAIC_START=0x309,
    IPTV_PLAYER_EVT_VID_MOSAIC_END=0x30a,
    IPTV_PLAYER_EVT_BUTT,
    IPTV_PLAYER_EVT_MAX,
}IPTV_PLAYER_EVT_E;

/**
 * \brief if IPTV_PLAYER_EVT_ERROR occurred, we identify which error is by param1 parameter
 */
typedef enum {
    IPTV_PLAYER_ERROR_VID_FRAMEERR = 0x1001,
    IPTV_PLAYER_ERROR_AUD_FRAMEERR,
    IPTV_PLAYER_ERROR_VID_FRAMEDISCARD,     //video frame discarded
    IPTV_PLAYER_ERROR_AUD_FRAMEDISCARD,     //audio frame discarded
    IPTV_PLAYER_ERROR_VID_UNDERFLOW,
    IPTV_PLAYER_ERROR_AUD_UNDERFLOW,
    IPTV_PLAYER_ERROR_MAX
} IPTV_PLAYER_ERROR_E;

typedef enum {
    PLAYER_STREAMTYPE_NULL = -1,
    PLAYER_STREAMTYPE_TS,
    PLAYER_STREAMTYPE_VIDEO,
    PLAYER_STREAMTYPE_AUDIO,
    PLAYER_STREAMTYPE_SUBTITLE,
    PLAYER_STREAMTYPE_MAX,
} PLAYER_STREAMTYPE_E;

typedef enum {
    CONTENTMODE_NULL = -1,
    CONTENTMODE_FULL,               //full screen
    CONTENTMODE_LETTERBOX,          //origin ratio
    CONTENTMODE_WIDTHFULL,          //horizontal scale
    CONTENTMODE_HEIGHFULL,          //vertical scale
    CONTENTMODE_MAX,
} CONTENTMODE_E;

typedef enum {
    ABALANCE_NULL = 0,
    ABALANCE_LEFT,
    ABALANCE_RIGHT,
    ABALANCE_STEREO,
    ABALANCE_MAX,
} ABALANCE_E;

typedef enum {
    VDRM_NONE = 0,
    SAMPLE_AES
} VDRM_METHOD_E;

typedef enum {
    H264 = 0,       //partial encryption h264
    H265,           //partial encryption h265
    H265_F          //full encryption h265
} VDRM_FORMAT_E;

typedef enum {
    DISPLAY_3D_NONE = 0,
    DISPLAY_3D_2DTO3D,
    DISPLAY_3D_SIDE_BY_SIDE,
    DISPLAY_3D_TOP_AND_BOTTOM,
    DISPLAY_3D_FRAME_PACKING,
    DISPLAY_3DTO2D_SIDE_BY_SIDE,
    DISPLAY_3DTO2D_TOP_AND_BOTTOM,
    DISPLAY_3DTO2D_FRAME_PACKING,
    DISPLAY_3D_AUTO = 0x20,
    DISPLAY_3D_MAX
} DISPLAY_3DMODE_E;

typedef enum {
    SURFACE_TOP = 0,        //top most
    SURFACE_BOTTOM,         //bottom most
    SURFACE_UP,             //move up
    SURFACE_DOWN            //move down
} SURFACE_ORDER_E;

typedef struct {
    VDRM_METHOD_E Method;
    VDRM_FORMAT_E Format;
    int KeyFormatVersion;
    char* KeyFormat;
    char* IV;
} DRM_PARA_T, *PDRM_PARA_T;

typedef struct {
    int nVideoWidth;
    int nVideoHeight;
    int nDecodeErrors;      //decoded error frames
    int nDecoded;           //total decoded frames
    int interlaced;
    int nFrameRate;
    int nAspect;
    int nBufSize;           //total buffer size
    int nUsedSize;          //used buffer size
} VIDEO_INFO_T,*PVIDEO_INFO_T;

typedef struct {
    int nSampleRate;
    int nBitDepth;
    int nChannels;
    int nBufSize;
    int nUsedSize;
} AUDIO_INFO_T,*PAUDIO_INFO_T;

typedef void (*IPTV_PLAYER_EVT_CB)(IPTV_PLAYER_EVT_E evt, void* handler, uint32_t param1, uint32_t param2);


///////////////////////////////////////////////////////////////////////////////
typedef enum {
    IPTV_PLAYER_ATTR_VID_ASPECT=0,  /* 视频宽高比 0--640*480，1--720*576，2--1280*720，3--1920*1080,4--3840*2160,5--others等标识指定分辨率*/
    IPTV_PLAYER_ATTR_VID_RATIO,     //视频宽高比, 0代表4：3，1代表16：9
    IPTV_PLAYER_ATTR_VID_SAMPLETYPE,     //帧场模式, 1代表逐行源，0代表隔行源
    IPTV_PLAYER_ATTR_VIDAUDDIFF,     //音视频播放diff
    IPTV_PLAYER_ATTR_VID_BUF_SIZE,     //视频缓冲区大小
    IPTV_PLAYER_ATTR_VID_USED_SIZE,     //视频缓冲区使用大小
    IPTV_PLAYER_ATTR_AUD_BUF_SIZE,     //音频缓冲区大小
    IPTV_PLAYER_ATTR_AUD_USED_SIZE,     //音频缓冲区已使用大小
    IPTV_PLAYER_ATTR_AUD_SAMPLERATE,     //音频缓冲区已使用大小
    IPTV_PLAYER_ATTR_AUD_BITRATE,     //音频缓冲区已使用大小
    IPTV_PLAYER_ATTR_AUD_CHANNEL_NUM,     //音频缓冲区已使用大小
    IPTV_PLAYER_ATTR_VID_FRAMERATE = 18, //video frame rate
    IPTV_PLAYER_ATTR_BUTT,
    IPTV_PLAYER_ATTR_V_HEIGHT, //video height
    IPTV_PLAYER_ATTR_V_WIDTH,  //video width
    IPTV_PLAYER_ATTR_STREAM_BITRATE,//stream bitrate
    IPTV_PLAYER_ATTR_CATON_TIMES,  //the num of caton
    IPTV_PLAYER_ATTR_CATON_TIME,    //the time of caton total time
} IPTV_ATTR_TYPE_e;

typedef struct {
    int abuf_size;
    int abuf_data_len;
    int abuf_free_len;
    int vbuf_size;
    int vbuf_data_len;
    int vbuf_free_len;
}AVBUF_STATUS, *PAVBUF_STATUS;


typedef enum {
    IPTV_PLAYER_PARAM_EVT_VIDFRM_STATUS_REPORT = 0,
    IPTV_PLAYER_PARAM_EVT_FIRSTFRM_REPORT,
    IPTV_PLAYER_PARAM_EVT_BUTT
} IPTV_PLAYER_PARAM_Evt_e;

typedef void (*IPTV_PLAYER_PARAM_EVENT_CB)( void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, void *pParam);

typedef struct {
    int instanceNo;
    int video_width;
    int video_height;
    int64_t mFirstVideoPTSUs;
}VIDEO_FRM_INFO_T;










#endif /* ifndef _CTC_COMMON_H */
