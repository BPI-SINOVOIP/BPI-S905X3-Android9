/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef _ATSC_MGT_O_H
#define _ATSC_MGT_O_H

#include "atsc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define MGT_SECTION_HEADER_LEN          (11)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

#pragma pack(1)

typedef struct mgt_section_header
{
	INT8U table_id                      :8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U section_syntax_indicator      :1;
    INT8U private_indicator      :1;
    INT8U                               :2;
    INT8U section_length_hi             :4;
#else
    INT8U section_length_hi             :4;
    INT8U                               :2;
    INT8U private_indicator      :1;
    INT8U section_syntax_indicator      :1;
#endif
    INT8U section_length_lo             :8;
    INT8U table_id_extern_hi        :8;
    INT8U  table_id_extern_lo        :8;
#if BYTE_ORDER == BIG_ENDIAN    
    INT8U                               :2;
    INT8U version_number                :5;
    INT8U current_next_indicator        :1;
#else
    INT8U current_next_indicator        :1;
    INT8U version_number                :5;
    INT8U                               :2;
#endif
    INT8U section_number                :8;
    INT8U last_section_number           :8;
    INT8U protocol_version 	:8;
    INT8U tables_defined_hi 	:8;
    INT8U tables_defined_lo	:8;
}mgt_section_header_t;

typedef struct mgt_table_info
{
    INT8U table_type_hi             :8;
    INT8U table_type_lo             :8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U 			                      :3;
    INT8U table_type_pid_hi           	 :5;
#else
    INT8U table_type_pid_hi        	 :5;
    INT8U 			                      :3;
#endif
    INT8U table_type_pid_lo		:8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U 			                      :3;
    INT8U table_type_version	:5;
#else
    INT8U table_type_version	:5;
    INT8U 			                      :3;
#endif
    INT8U number_bytes_hi		:8;
    INT8U number_bytes_mh		:8;
    INT8U number_bytes_ml		:8;
    INT8U number_bytes_lo		:8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U 			                      :4;
    INT8U table_type_desc_len_hi	:4;
#else
    INT8U table_type_desc_len_hi	:4;
    INT8U 			                      :4;
#endif
    INT8U table_type_desc_len_lo 	:8;
}mgt_table_info_t;

#pragma pack()

typedef struct com_table_info
{
    INT16U  table_type;
    INT16U  table_type_pid;
    INT8U	table_type_version;
    struct   atsc_descriptor_s *desc;
    struct   com_table_info *p_next;
}com_table_info_t;

typedef struct mgt_section_info
{
    struct mgt_section_info *p_next;
	INT8U i_table_id;
    INT8U  version_number;
    INT16U  tables_defined;
    INT32U  is_cable;
    struct com_table_info *com_table_info;
    struct atsc_descriptor_s *desc;
}mgt_section_info_t;

/*****************************************************************************
 * Function prototypes	
 *****************************************************************************/

INT32S atsc_psip_parse_mgt(INT8U* data, INT32U length, mgt_section_info_t *info);
void   atsc_psip_clear_mgt_info(mgt_section_info_t *info);

mgt_section_info_t *atsc_psip_new_mgt_info(void);

void   atsc_psip_free_mgt_info(mgt_section_info_t *info);

void   atsc_psip_dump_mgt_info(mgt_section_info_t *info);

#ifdef __cplusplus
}

#endif

#endif
