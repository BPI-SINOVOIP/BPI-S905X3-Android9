/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _ATSC_ETT_O_H
#define _ATSC_ETT_O_H

#include "atsc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define ETT_SECTION_HEADER_LEN          (11)

#pragma pack(1)

/****************************************************************************
 * Type definitions
 ***************************************************************************/
typedef struct ett_section_header
{
	INT8U table_id							:8;
#if BYTE_ORDER == BIG_ENDIAN
	INT8U section_syntax_indicator	:1;
	INT8U private_indicator 		:1;
	INT8U										:2;
	INT8U section_length_hi 			:4;
#else
	INT8U section_length_hi 			:4;
	INT8U										:2;
	INT8U private_indicator 		:1;
	INT8U section_syntax_indicator	:1;
#endif
	INT8U section_length_lo 			:8;
	INT8U table_id_ext_hi	:8;
	INT8U table_id_ext_lo	:8;
#if BYTE_ORDER == BIG_ENDIAN    
	INT8U										:2;
	INT8U version_number			:5;
	INT8U current_next_indicator	:1;
#else
	INT8U current_next_indicator	:1;
	INT8U version_number			:5;
	INT8U										:2;
#endif
	INT8U section_number				:8;
	INT8U last_section_number		:8;
	INT8U protocol_version				  :8;
	INT8U source_id_hi 				:8;
	INT8U source_id_lo  			:8;	
	
}ett_section_header_t;

typedef struct ett_event_sect_info_s
{
	INT8U event_id_hi				:8;

#if BYTE_ORDER == BIG_ENDIAN
	INT8U event_id_lo				:6;
	INT8U event_id_flag 			:1;
	INT8U							:1;	
#else
	INT8U							:1;
	INT8U event_id_flag 			:1;
	INT8U event_id_lo				:6;
#endif
}ett_event_sect_info_t;

typedef struct ett_event_info_s
{
    INT16U event_id;
	INT8U  event_id_flag;
    atsc_multiple_string_t text;   
}ett_event_info_t;


typedef struct ett_section_info
{
	struct ett_section_info *p_next;
	INT8U i_table_id;
    INT16U source_id;
    INT8U  version_number;
    INT16U table_id_extension;
    ett_event_info_t  *ett_event_info;
}ett_section_info_t;

#pragma pack()

/*****************************************************************************
 * Function prototypes	
 *****************************************************************************/

INT32S atsc_psip_parse_ett(INT8U* data, INT32U length, ett_section_info_t *info);
void   atsc_psip_clear_ett_info(ett_section_info_t *info);

ett_section_info_t *atsc_psip_new_ett_info(void);
void   atsc_psip_free_ett_info(ett_section_info_t *info);

#ifdef __cplusplus
}

#endif

#endif

