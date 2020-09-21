/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef _ATSC_STT_O_H
#define _ATSC_STT_O_H

#include "atsc_types.h"

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

#pragma pack(1)

typedef struct stt_section
{
	INT8U table_id                      :8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U section_syntax_indicator      :1;
    INT8U private_indicator		:1;	
    INT8U                               :2;
    INT8U section_length_hi             :4;
#else
    INT8U section_length_hi             :4;
    INT8U                               :2;
    INT8U private_indicator     :1;
    INT8U section_syntax_indicator      :1;
#endif
    INT8U section_length_lo             :8;
    INT8U table_id_extern_hi 		:8;
    INT8U table_id_extern_lo		:8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U 					   :2;
    INT8U version_number    :5;
    INT8U current_next_indicator :1;
#else
    INT8U current_next_indicator :1;
    INT8U version_number    :5;
    INT8U 					   :2;
#endif
    INT8U section_number 	:8;
    INT8U last_section_number :8;
    INT8U protocol_version 	:8;
    INT8U system_time_hi		:8;
    INT8U system_time_mh		:8;
    INT8U system_time_ml 		:8;
    INT8U system_time_lo		:8;
    INT8U GPS_UTC_offset		:8;
    INT8U dalight_saving_hi 	:8;
    INT8U dalight_saving_lo 	:8;
	
}stt_section_t;

#pragma pack()

typedef struct stt_section_info
{
    struct stt_section_info *p_next;
	INT8U i_table_id;
    INT32U	utc_time;        
}stt_section_info_t;

/*****************************************************************************
 * Function prototypes	
 *****************************************************************************/

INT32S atsc_psip_parse_stt(INT8U* data, INT32U length, stt_section_info_t *info);

stt_section_info_t *atsc_psip_new_stt_info(void);
void   atsc_psip_free_stt_info(stt_section_info_t *info);
void   atsc_psip_dump_stt_time(INT32U utc_time);

// the next function for atsc epg
#define ATSC_EPG_TIME_RELATE (1)
#if ATSC_EPG_TIME_RELATE
INT32U GetCurrentGPSSeconds(void);
INT32U GetCurrentGPSOffset(void);
#endif

#ifdef __cplusplus
}

#endif

#endif

