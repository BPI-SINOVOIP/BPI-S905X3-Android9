/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef _ATSC_EIT_O_H
#define _ATSC_EIT_O_H

#include "atsc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define EIT_SECTION_HEADER_LEN          (10)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

#pragma pack(1)

typedef struct eit_section_header
{
	INT8U table_id                      	:8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U section_syntax_indicator	:1;
    INT8U private_indicator 		:1;
    INT8U                               		:2;
    INT8U section_length_hi             :4;
#else
    INT8U section_length_hi             :4;
    INT8U                               		:2;
    INT8U private_indicator 		:1;
    INT8U section_syntax_indicator	:1;
#endif
    INT8U section_length_lo             :8;
    INT8U source_id_hi	:8;
    INT8U source_id_lo	:8;
#if BYTE_ORDER == BIG_ENDIAN    
    INT8U                               		:2;
    INT8U version_number         	:5;
    INT8U current_next_indicator 	:1;
#else
    INT8U current_next_indicator	:1;
    INT8U version_number         	:5;
    INT8U                               		:2;
#endif
    INT8U section_number                :8;
    INT8U last_section_number		:8;
    INT8U protocol_version                :8;
    INT8U num_events_in_section		:8;
}eit_section_header_t;

typedef struct event_sect_info_s
{
#if BYTE_ORDER == BIG_ENDIAN    
	INT8U                       :2;
	INT8U event_id_hi         	:6;
#else
	INT8U event_id_hi         	:6;
	INT8U                       :2;
#endif
	INT8U event_id_lo			:8;
	INT8U start_time_hi			:8;
	INT8U start_time_mh			:8;
	INT8U start_time_ml			:8;
	INT8U start_time_lo			:8;
#if BYTE_ORDER == BIG_ENDIAN    
    INT8U                       :2;
    INT8U ETM_location         	:2;
    INT8U length_in_seconds_mh			:4;
#else
    INT8U length_in_seconds_mh			:4;
    INT8U ETM_location         	:2;
    INT8U                       :2;
#endif
	INT8U length_in_seconds_ml			:8;
	INT8U length_in_seconds_lo			:8;
	INT8U title_length		:8;
}event_sect_info_t;

#pragma pack()


typedef struct eit_event_info_s
{
    INT16U event_id;
    INT32U start_time;
    INT8U ETM_location;
    INT32U length_in_seconds;
    atsc_multiple_string_t title;
    atsc_descriptor_t *desc;
    struct eit_event_info_s *p_next;
}eit_event_info_t;

typedef struct eit_section_info
{
	struct eit_section_info *p_next;
	INT8U i_table_id;
    INT16U source_id;
    INT8U  version_number;
    INT8U  num_events_in_section;
    eit_event_info_t  *eit_event_info;
}eit_section_info_t;

/*****************************************************************************
 * Function prototypes	
 *****************************************************************************/

INT32S atsc_psip_parse_eit(INT8U* data, INT32U length, eit_section_info_t *info);
void   atsc_psip_clear_eit_info(eit_section_info_t *info);

eit_section_info_t *atsc_psip_new_eit_info(void);
void   atsc_psip_free_eit_info(eit_section_info_t *info);

#ifdef __cplusplus
}

#endif

#endif
