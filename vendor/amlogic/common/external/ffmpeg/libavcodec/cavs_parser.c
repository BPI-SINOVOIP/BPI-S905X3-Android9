/*
 * Chinese AVS video (AVS1-P2, JiZhun profile) parser.
 * Copyright (c) 2006  Stefan Gehrer <stefan.gehrer@gmx.de>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Chinese AVS video (AVS1-P2, JiZhun profile) parser
 * @author Stefan Gehrer <stefan.gehrer@gmx.de>
 */

#include "parser.h"
#include "cavs.h"


/**
 * Find the end of the current frame in the bitstream.
 * @return the position of the first byte of the next frame, or -1
 */
 const AVRational avs2_fps_tab[] = {
    {   0,   0},
    {1001, 24000},
    {   1,   24},
    {   1,   25},
    {1001, 30000},
    {   1,   30},
    {   1,   50},
    {1001, 60000},
    {   1,   60},
    {   1,  100},
    {   1,  120},
    {   1,  200},
    {   1,  240},
    {   1,  300},
    {   0,   0},
};

static int cavs_find_frame_end(ParseContext *pc, const uint8_t *buf,
                               int buf_size) {
    int pic_found, i;
    uint32_t state;

    pic_found= pc->frame_start_found;
    state= pc->state;

    i=0;
    if(!pic_found){
        for(i=0; i<buf_size; i++){
            state= (state<<8) | buf[i];
            if(state == PIC_I_START_CODE || state == PIC_PB_START_CODE){
                i++;
                pic_found=1;
                break;
            }
        }
    }

    if(pic_found){
        /* EOF considered as end of frame */
        if (buf_size == 0)
            return 0;
        for(; i<buf_size; i++){
            state= (state<<8) | buf[i];
            if((state&0xFFFFFF00) == 0x100){
                if(state > SLICE_MAX_START_CODE){
                    pc->frame_start_found=0;
                    pc->state=-1;
                    return i-3;
                }
            }
        }
    }
    pc->frame_start_found= pic_found;
    pc->state= state;
    return END_NOT_FOUND;
}
const uint8_t *find_start_code(const uint8_t * restrict p, const uint8_t *end,
                          uint32_t * restrict state)
{
    int i;
    assert(p <= end);
    if (p >= end) {
        return end;
    }

    for (i=0; i<3; i++) {
        uint32_t tmp= *state << 8;
        *state= tmp + *(p++);
        if (tmp == 0x100 || p == end)
            return p;
    }

    while (p < end) {
        if (p[-1] > 1) p+= 3;
        else if (p[-2]) p+= 2;
        else if (p[-3]|(p[-1]-1)) p++;
        else {
        p++;
        break;
        }
    }

    p= FFMIN(p, end)-4;
    *state = AV_RB32(p);

    return p+4;
}

static void cavs2video_extract_headers(AVCodecParserContext *s,
                           AVCodecContext *avctx,
                           const uint8_t *buf, int buf_size)
{
  //  ParseContext1 *pc = s->priv_data;
    //uint8_t * buf = buf_in;
    const uint8_t *buf_end = buf + buf_size;
    int bytes_left;
    uint32_t start_code = -1;
    int profile_id, level_id, progressive_seq, field_coded_seq;
    int chroma_format, sample_precision, enc_precision;
    int aspect_ratio, frame_rate_code;
    while (buf < buf_end) {
        buf = find_start_code(buf, buf_end, &start_code);
        bytes_left = buf_end - buf;

        switch (start_code) {
        case CAVS_START_CODE:
            profile_id		= AV_RB8(buf);
            level_id		= AV_RB8(buf + 1);
            progressive_seq	= AV_RB16(buf + 2) >> 15;
            field_coded_seq	= AV_RB16(buf + 2) >> 14 & 0x1;
            chroma_format	= AV_RB16(buf + 4) & 0x3;
            sample_precision	= AV_RB16(buf + 6) >> 13;
            aspect_ratio	= AV_RB16(buf + 6) >> 6 & 0xf;
            frame_rate_code	= AV_RB16(buf + 6) >> 2 & 0xf;

            if (profile_id == 0x22)
                enc_precision	= AV_RB16(buf + 6) >> 10 & 0x3;

            avctx->width	= AV_RB16(buf + 2) & 0x3fff;
            avctx->height	= AV_RB16(buf + 4) >> 2 & 0x3fff;
            avctx->time_base.den = avs2_fps_tab[frame_rate_code].den;
            avctx->time_base.num = avs2_fps_tab[frame_rate_code].num;

            break;
        default:
            break;
        }
    }
}

static int cavsvideo_parse(AVCodecParserContext *s,
                           AVCodecContext *avctx,
                           const uint8_t **poutbuf, int *poutbuf_size,
                           const uint8_t *buf, int buf_size)
{
    ParseContext *pc = s->priv_data;
    int next;

    if(s->flags & PARSER_FLAG_COMPLETE_FRAMES){
        next= buf_size;
    }else{
        next= cavs_find_frame_end(pc, buf, buf_size);

        if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
            *poutbuf = NULL;
            *poutbuf_size = 0;
            return buf_size;
        }
    }
    *poutbuf = buf;
    *poutbuf_size = buf_size;
    return next;
}

static int cavs2video_parse(AVCodecParserContext *s,
                        AVCodecContext *avctx,
                        const uint8_t **poutbuf, int *poutbuf_size,
                        const uint8_t *buf, int buf_size)
{
    ParseContext *pc = s->priv_data;
    int next;

    if (s->flags & PARSER_FLAG_COMPLETE_FRAMES) {
        next= buf_size;
    } else {
        next= cavs_find_frame_end(pc, buf, buf_size);

       if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
            *poutbuf = NULL;
            *poutbuf_size = 0;
            return buf_size;
        }
    }

    if (1) cavs2video_extract_headers(s, avctx, buf, buf_size);
    //avctx->time_base.den = 30000;
    //avctx->time_base.num = 1000;

    *poutbuf = buf;
    *poutbuf_size = buf_size;
    return next;
}

AVCodecParser ff_cavsvideo_parser = {
    .codec_ids      = { AV_CODEC_ID_CAVS },
    .priv_data_size = sizeof(ParseContext),
    .parser_parse   = cavsvideo_parse,
    .parser_close   = ff_parse_close,
    .split          = ff_mpeg4video_split,
};

AVCodecParser ff_cavs2video_parser = {
    .codec_ids      = { AV_CODEC_ID_CAVS2 },
    .priv_data_size = sizeof(ParseContext),
    .parser_parse   = cavs2video_parse,
    .parser_close   = ff_parse_close,
    .split          = ff_mpeg4video_split,
};
