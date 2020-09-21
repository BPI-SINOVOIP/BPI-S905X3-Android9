/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef SUBTITLE_TELETEXT_H
#define SUBTITLE_TELETEXT_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "sub_subtitle.h"


#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)
#define AV_TIME_BASE               1000000
#define AV_TIME_BASE_Q          (AVRational){1, AV_TIME_BASE}

#if 0

#define AVPALETTE_SIZE 1024

typedef struct AVSubtitleRect
{
    int x;         ///< top left corner  of pict, undefined when pict is not set
    int y;         ///< top left corner  of pict, undefined when pict is not set
    int w;         ///< width            of pict, undefined when pict is not set
    int h;         ///< height           of pict, undefined when pict is not set
    int nb_colors; ///< number of colors in pict, undefined when pict is not set

    /**
     * data+linesize for the bitmap of this subtitle.
     * can be set for text/ass as well once they where rendered
     */
    //AVPicture pict;
    uint8_t *data[4];
    int linesize[4];       ///< number of bytes per line
    enum AVSubtitleType type;

    char *text;                     ///< 0 terminated plain UTF-8 text

    /**
     * 0 terminated ASS/SSA compatible event line.
     * The pressentation of this is unaffected by the other values in this
     * struct.
     */
    char *ass;
} AVSubtitleRect;

typedef struct AVSubtitle
{
    uint16_t format; /* 0 = graphics */
    uint32_t start_display_time; /* relative to packet pts, in ms */
    uint32_t end_display_time; /* relative to packet pts, in ms */
    unsigned num_rects;
    AVSubtitleRect **rects;
    int64_t pts;    ///< Same as packet pts, in AV_TIME_BASE
} AVSubtitle;
#endif
int teletext_close_decoder();
int teletext_init_decoder();
int get_dvb_teletext_spu(AML_SPUVAR *spu, int read_handle);

#endif