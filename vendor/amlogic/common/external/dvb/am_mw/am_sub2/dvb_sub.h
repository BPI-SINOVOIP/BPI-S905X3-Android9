/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifndef DVB_SUB_H
#define DVB_SUB_H

#include "includes.h"
#include "am_osd.h"
#include "am_sub2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AM_OSD_Color_t    sub_clut_t;

typedef AM_SUB2_Region_t  sub_pic_region_t;

typedef AM_SUB2_Picture_t dvbsub_picture_t;

typedef void (*dvbsub_callback_t)(long handle, dvbsub_picture_t *display);

INT32S dvbsub_decoder_create(INT16U composition_id, INT16U ancillary_id, dvbsub_callback_t cb, long* handle);
INT32S dvbsub_decoder_destroy(long handle);

INT32S dvbsub_parse_pes_packet(long handle, const INT8U* packet, INT32U length);

dvbsub_picture_t* dvbsub_get_display_set(long handle);
INT32S dvbsub_remove_display_picture(long handle, dvbsub_picture_t* pic);

#ifdef __cplusplus
}
#endif

#endif

