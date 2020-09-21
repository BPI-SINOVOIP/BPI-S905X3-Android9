/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef _ATSC_VCT_O_H
#define _ATSC_VCT_O_H

#include "atsc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define VCT_SECTION_HEADER_LEN          (10)
#define AM_SI_IS_CVCT(_vct) ((_vct)->i_table_id == ATSC_PSIP_CVCT_TID)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

#pragma pack(1)

typedef struct vct_section_header
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
    INT8U transport_stream_id_hi	:8;
    INT8U transport_stream_id_lo	:8;
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
    INT8U num_channels_in_section	:8;

}vct_section_header_t;

typedef struct vct_sect_chan_info
{
    INT8U short_name[14];
#if BYTE_ORDER == BIG_ENDIAN
    INT8U 				:4;
    INT8U major_channel_number_hi  :4;
#else
    INT8U major_channel_number_hi  :4;
    INT8U 				:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    INT8U major_channel_number_lo	:6;
    INT8U minor_channel_number_hi 	:2;
#else
    INT8U minor_channel_number_hi	:2;
    INT8U major_channel_number_lo 	:6;
#endif	
    INT8U minor_channel_number_lo     :8;
    INT8U modulation_mode 			:8;
    INT8U carrier_frequency_hi			:8;
    INT8U carrier_frequency_mh			:8;
    INT8U carrier_frequency_ml			:8;
    INT8U carrier_frequency_lo			:8;
    INT8U channel_TSID_hi			:8;
    INT8U channel_TSID_lo			:8;
    INT8U program_number_hi		:8;
    INT8U program_number_lo		:8;
#if BYTE_ORDER == BIG_ENDIAN
   INT8U  ETM_location		:2;
   INT8U  access_controlled	:1;
   INT8U  hidden			:1;
   INT8U  	path_select			:1; /**< reserved for tvct*/
   INT8U  	out_of_band			:1; /**< reserved for tvct*/
   INT8U  hide_guide		:1;
   INT8U  					:1;
#else
   INT8U  					:1;
   INT8U  hide_guide		:1;
   INT8U  	out_of_band			:1;
   INT8U  	path_select			:1;
   INT8U  hidden			:1;
   INT8U  access_controlled	:1;
   INT8U  ETM_location		:2;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    INT8U  			:2;
    INT8U service_type   :6;
#else
    INT8U service_type   :6;    
    INT8U  			:2;
#endif
    INT8U source_id_hi :8;
    INT8U source_id_lo 	:8;
#if BYTE_ORDER == BIG_ENDIAN
    INT8U 			:6;
    INT8U  descriptors_length_hi  :2;
#else
    INT8U  descriptors_length_hi  :2;
    INT8U 			:6;
#endif
    INT8U descriptors_length_lo 	:8;
}vct_sect_chan_info_t;

#pragma pack()

typedef struct vct_channel_info
{
    INT8U short_name[14];
    INT8U modulation_mode;
    INT8U access_controlled;
    INT8U service_type;
    INT8U hidden;
    INT8U hide_guide;
    INT8U path_select; /**< reserved for tvct*/
    INT8U out_of_band; /**< reserved for tvct*/
    INT16U major_channel_number;
    INT16U minor_channel_number;
    INT16U source_id;
    INT16U channel_TSID;
    INT16U program_number;
    INT32U carrier_frequency;
    atsc_descriptor_t *desc;                // --
    struct vct_channel_info *p_next;
}vct_channel_info_t;

typedef struct vct_section_info
{
	struct vct_section_info *p_next;
	INT8U i_table_id;
    INT16U transport_stream_id;
    INT8U  version_number;
    INT8U  num_channels_in_section;
    struct vct_channel_info  *vct_chan_info;
}vct_section_info_t;

/*****************************************************************************
 * Function prototypes	
 *****************************************************************************/

INT32S atsc_psip_parse_vct(INT8U* data, INT32U length, vct_section_info_t *info);
void   atsc_psip_clear_vct_info(vct_section_info_t *info);

vct_section_info_t *atsc_psip_new_vct_info(void);
void   atsc_psip_free_vct_info(vct_section_info_t *info);

void   atsc_psip_dump_vct_info(vct_section_info_t *info);

#ifdef __cplusplus
}

#endif

#endif
