/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef CODEC_H_
#define CODEC_H_


#define SINGLE_MODE 0
#define STREAM_MODE 1
#define FRAME_MODE 2
// video fromat
#define VFORMAT_UNKNOWN		(-1)
#define VFORMAT_MPEG12		(0)
#define VFORMAT_MPEG4		(1)
#define VFORMAT_H264		(2)
#define VFORMAT_MJPEG		(3)
#define VFORMAT_REAL		(4)
#define VFORMAT_JPEG		(5)
#define VFORMAT_VC1		(6)
#define VFORMAT_AVS		(7)
#define VFORMAT_SW		(8)
#define VFORMAT_H264MVC		(9)
#define VFORMAT_H264_4K2K	(10)
#define VFORMAT_HEVC		(11)
#define VFORMAT_H264_ENC	(12)
#define VFORMAT_JPEG_ENC	(13)
#define VFORMAT_VP9		(14)
#define VFORMAT_AVS2		(15)
#define VFORMAT_AV1		(16)
#define VFORMAT_MAX		(INT_MAX)

// video type for sysinfo
#define VIDEO_DEC_FORMAT_UNKNOW		(0)
#define VIDEO_DEC_FORMAT_MPEG4_3	(1)
#define VIDEO_DEC_FORMAT_MPEG4_4	(2)
#define VIDEO_DEC_FORMAT_MPEG4_5	(3)
#define VIDEO_DEC_FORMAT_H264		(4)
#define VIDEO_DEC_FORMAT_MJPEG		(5)
#define VIDEO_DEC_FORMAT_MP4		(6)
#define VIDEO_DEC_FORMAT_H263		(7)
#define VIDEO_DEC_FORMAT_REAL_8		(8)
#define VIDEO_DEC_FORMAT_REAL_9		(9)
#define VIDEO_DEC_FORMAT_WMV3		(10)
#define VIDEO_DEC_FORMAT_WVC1		(11)
#define VIDEO_DEC_FORMAT_SW		(12)
#define VIDEO_DEC_FORMAT_AVS		(13)
#define VIDEO_DEC_FORMAT_H264_4K2K	(14)
#define VIDEO_DEC_FORMAT_HEVC		(15)
#define VIDEO_DEC_FORMAT_VP9		(16)
#define VIDEO_DEC_FORMAT_AVS2		(17)
#define VIDEO_DEC_FORMAT_MAX		(18)

// err status
#define C_PAE                               (0x01000000)
#define CODEC_ERROR_NONE                    ( 0)
#define CODEC_ERROR_PARAMETER               (C_PAE | 5)
#define CODEC_ERROR_VIDEO_TYPE_UNKNOW       (C_PAE | 7)
#define CODEC_ERROR_STREAM_TYPE_UNKNOW      (C_PAE | 8)
#define CODEC_ERROR_INIT_FAILED             (C_PAE | 10)
#define CODEC_ERROR_SET_BUFSIZE_FAILED      (C_PAE | 11)
#define CODEC_OPEN_HANDLE_FAILED            (C_PAE | 12)

typedef enum {
    STREAM_TYPE_UNKNOW,
    STREAM_TYPE_ES_VIDEO,
    STREAM_TYPE_ES_AUDIO,
    STREAM_TYPE_ES_SUB,
    STREAM_TYPE_PS,
    STREAM_TYPE_TS,
    STREAM_TYPE_RM,
} stream_type_t;

typedef struct {
    unsigned int    format;  ///< video format, such as H264, MPEG2...
    unsigned int    width;   ///< video source width
    unsigned int    height;  ///< video source height
    unsigned int    rate;    ///< video source frame duration
    unsigned int    extra;   ///< extra data information of video stream
    unsigned int    status;  ///< status of video stream
    unsigned int    ratio;   ///< aspect ratio of video source
    void *          param;   ///< other parameters for video decoder
    unsigned long long    ratio64;   ///< aspect ratio of video source
} dec_sysinfo_t;

typedef struct {
    int valid;               ///< audio extradata valid(1) or invalid(0), set by dsp
    int sample_rate;         ///< audio stream sample rate
    int channels;            ///< audio stream channels
    int bitrate;             ///< audio stream bit rate
    int codec_id;            ///< codec format id
    int block_align;         ///< audio block align from ffmpeg
    int extradata_size;      ///< extra data size
    char extradata[4096];;   ///< extra data information for decoder
} audio_info_t;

typedef struct {
    int valid;               ///< audio extradata valid(1) or invalid(0), set by dsp
    int sample_rate;         ///< audio stream sample rate
    int channels;            ///< audio stream channels
    int bitrate;             ///< audio stream bit rate
    int codec_id;            ///< codec format id
    int block_align;         ///< audio block align from ffmpeg
    int extradata_size;      ///< extra data size
    char extradata[512];;   ///< extra data information for decoder
} Asf_audio_info_t;

typedef struct {
    int handle;        ///< codec device handler
    int cntl_handle;   ///< video control device handler
    int sub_handle;    ///< subtile device handler
    int audio_utils_handle;  ///< audio utils handler
    stream_type_t stream_type;  ///< stream type(es, ps, rm, ts)
unsigned int has_video:
    1;                          ///< stream has video(1) or not(0)
unsigned int  has_audio:
    1;                          ///< stream has audio(1) or not(0)
unsigned int has_sub:
    1;                          ///< stream has subtitle(1) or not(0)
unsigned int noblock:
    1;                          ///< codec device is NONBLOCK(1) or not(0)
unsigned int dv_enable:
	1;							///< videois dv data.

    int video_type;             ///< stream video type(H264, VC1...)
    int audio_type;             ///< stream audio type(PCM, WMA...)
    int sub_type;               ///< stream subtitle type(TXT, SSA...)
    int video_pid;              ///< stream video pid
    int audio_pid;              ///< stream audio pid
    int sub_pid;                ///< stream subtitle pid
    int audio_channels;         ///< stream audio channel number
    int audio_samplerate;       ///< steram audio sample rate
    int vbuf_size;              ///< video buffer size of codec device
    int abuf_size;              ///< audio buffer size of codec device
    dec_sysinfo_t am_sysinfo;   ///< system information for video
    audio_info_t audio_info;    ///< audio information pass to audiodsp
    int packet_size;            ///< data size per packet
    int avsync_threshold;    ///<for adec in ms>
    void * adec_priv;          ///<for adec>
    void * amsub_priv;          // <for amsub>
    int SessionID;
    int dspdec_not_supported;//check some profile that audiodsp decoder can not support,we switch to arm decoder
    int switch_audio_flag;      //<switch audio flag switching(1) else(0)
    int automute_flag;
    char *sub_filename;
    int associate_dec_supported;//support associate or not
    int mixing_level;
    unsigned int drmmode;
    int mode;
} vcodec_para_t;

struct buf_status {
    int size;
    int data_len;
    int free_len;
    unsigned int read_pointer;
    unsigned int write_pointer;
};

struct vdec_status {
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    unsigned int error_count;
    unsigned int status;
};

struct usr_crc_info_t {
    unsigned int id;
    unsigned int pic_num;
    unsigned int y_crc;
    unsigned int uv_crc;
};

// codec api
int vcodec_init(vcodec_para_t *pcodec);
int vcodec_close(vcodec_para_t *);
int vcodec_reset(vcodec_para_t *);
int vcodec_init_cntl(vcodec_para_t *);
int vcodec_close_cntl(vcodec_para_t *);
int vcodec_write(vcodec_para_t *pcodec, void *buffer, int len);
int vcodec_read(vcodec_para_t *pcodec, void *buffer, int len);
int vcodec_pause(vcodec_para_t *);
int vcodec_resume(vcodec_para_t *);
int vcodec_get_vbuf_state(vcodec_para_t *p, struct buf_status *buf);
int vcodec_set_frame_cmp_crc(vcodec_para_t *vcodec, const int *crc, int size);
int vcodec_get_crc_check_result(vcodec_para_t *vcodec, int vdec_id);
int is_crc_cmp_ongoing(vcodec_para_t *vcodec, int vdec_id);

#endif //CODEC_H_
