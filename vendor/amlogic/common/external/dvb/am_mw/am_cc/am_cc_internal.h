/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_cc_internal.h
 * \brief CC模块内部数据
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2011-12-27: create the document
 ***************************************************************************/

#ifndef _AM_CC_INTERNAL_H
#define _AM_CC_INTERNAL_H

#include <pthread.h>
#include <libzvbi.h>
#include <dtvcc.h>
#include <am_cc.h>
#include <am_types.h>

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

enum
{
	AM_CC_EVT_SET_USER_OPTIONS,
};

enum
{
	FLASH_NONE,
	FLASH_SHOW,
	FLASH_HIDE
};

enum
{
	UNKNOWN = -1,
	MPEG12,
	MPEG4,
	H264,
	MJPEG,
	REAL,
	JPEG,
	VC1,
	AVS,
	SW,
	H264MVC,
	H264_4K2K,
	HEVC,
	H264_ENC,
	JPEG_ENC,
	VP9,
};


typedef struct AM_CC_Decoder AM_CC_Decoder_t;

typedef struct json_chain AM_CC_JsonChain_t;
#define JSON_STRING_LENGTH 8192
struct json_chain
{
	char* buffer;
	uint32_t pts;
	struct timespec decode_time;
	struct timespec display_time;
	struct json_chain* json_chain_next;
	struct json_chain* json_chain_prior;
	int count;
};

/**\brief ClosedCaption数据*/
struct AM_CC_Decoder
{
	int vfmt;
	int evt;
	int vbi_pgno;
	int flash_stat;
	int timeout;
	AM_Bool_t running;
	AM_Bool_t hide;
	AM_Bool_t render_flag;
	AM_Bool_t need_clear;
	unsigned int decoder_param;
	char lang[12];
	pthread_t render_thread;
	pthread_t data_thread;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	struct tvcc_decoder decoder;

	AM_CC_JsonChain_t* json_chain_head;
	AM_CC_CreatePara_t cpara;
	AM_CC_StartPara_t spara;

	int curr_data_mask;
	int curr_switch_mask;
	int process_update_flag;

	uint32_t decoder_cc_pts;
	uint32_t video_pts;
};


/****************************************************************************
 * Function prototypes
 ***************************************************************************/
int tvcc_to_json (struct tvcc_decoder *td, int pgno, char *buf, size_t len);
int tvcc_empty_json (int pgno, char *buf, size_t len);



#ifdef __cplusplus
}
#endif

#endif

