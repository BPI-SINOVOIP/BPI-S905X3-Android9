/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef DVB_SUB_H
#define DVB_SUB_H

#include "includes.h"
#include "am_osd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AM_OSD_Color_t sub_clut_t;

typedef struct sub_pic_region_s
{
    INT32S                  left;               /* region left most coordinate  */
    INT32S                  top;                /* region top most coordinate   */
    INT32U                  width;              /* region width (pixels)        */
    INT32U                  height;             /* region height (pixels)       */

    INT32U                  entry;              /* region clut entry number  */
    sub_clut_t              clut[256];          /* region clut               */

    /* for background */
    INT32U                  background;         /* region background color      */

    /* for pixel map */
    INT8U                   *p_buf;             /* region pixel buffer          */

    /* for text */
    INT32U                  fg;                 /* region text foreground color */
    INT32U                  bg;                 /* region text background color */

    INT32U                  length;             /* region text length           */
    INT16U                  *p_text;            /* region text string           */

    struct sub_pic_region_s *p_next;

} sub_pic_region_t;

typedef struct dvbsub_picture_s
{
    INT64U                  pts;                /* picture display PTS          */
    INT32U                  timeout;            /* pciture display timeout(s)   */

    INT32S                  original_x;         /* video picture left           */
    INT32S                  original_y;         /* video picture top            */

    INT32U                  original_width;     /* video picture width          */
    INT32U                  original_height;    /* video picture height         */

    sub_pic_region_t        *p_region;          /* picture regions              */

    struct dvbsub_picture_s *p_prev;
    struct dvbsub_picture_s *p_next;

} dvbsub_picture_t;

typedef void (*dvbsub_callback_t)(INT32U handle, dvbsub_picture_t *display);

INT32S dvbsub_decoder_create(INT16U composition_id, INT16U ancillary_id, dvbsub_callback_t callback, INT32U* handle);
INT32S dvbsub_decoder_destroy(INT32U handle);

INT32S dvbsub_parse_pes_packet(INT32U handle, const INT8U* packet, INT32U length);

dvbsub_picture_t* dvbsub_get_display_set(INT32U handle);
INT32S dvbsub_remove_display_picture(INT32U handle, dvbsub_picture_t* pic);

#ifdef __cplusplus
}
#endif

#endif

