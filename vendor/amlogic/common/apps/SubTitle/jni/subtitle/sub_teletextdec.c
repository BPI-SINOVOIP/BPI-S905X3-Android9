/*
 * Teletext decoding for ffmpeg
 * Copyright (c) 2005-2010, 2012 Wolfram Gloger
 * Copyright (c) 2013 Marton Balint
 *
 * This library is free software; you can redistribute it and/or
/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/

//#include "avcodec.h"
//#include "libavcodec/ass.h"
//#include "libavcodec/dvbtxt.h"
//#include "libavutil/opt.h"
//#include "libavutil/bprint.h"
//#include "libavutil/internal.h"
//#include "libavutil/intreadwrite.h"
//#include "libavutil/log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bprint.h"
#include <android/log.h>

#include "sub_teletextdec.h"
#include "sub_dvb_sub.h"


#include <libzvbi.h>
//#include <vbi.h>

#define  LOG_TAG    "sub_dvb_teletxt"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  TRACE()    LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)


#define TEXT_MAXSZ    (25 * (56 + 1) * 4 + 2)
#define VBI_NB_COLORS 40
#define VBI_TRANSPARENT_BLACK 8
#define RGBA(r,g,b,a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define VBI_R(rgba)   (((rgba) >> 0) & 0xFF)
#define VBI_G(rgba)   (((rgba) >> 8) & 0xFF)
#define VBI_B(rgba)   (((rgba) >> 16) & 0xFF)
#define VBI_A(rgba)   (((rgba) >> 24) & 0xFF)
#define MAX_BUFFERED_PAGES 25
#define BITMAP_CHAR_WIDTH  12
#define BITMAP_CHAR_HEIGHT 10
#define MAX_SLICES 64


typedef struct TeletextPage
{
    AVSubtitleRect *sub_rect;
    int pgno;
    int subno;
    int64_t pts;
} TeletextPage;

typedef struct TeletextContext
{
    //AVClass        *class;
    char           *pgno;
    int             x_offset;
    int             y_offset;
    int             format_id; /* 0 = bitmap, 1 = text/ass */
    int             chop_top;
    int             sub_duration; /* in msec */
    int             transparent_bg;
    int             opacity;
    int             chop_spaces;

    int             lines_processed;
    TeletextPage    *pages;
    int             nb_pages;
    int64_t         pts;
    int             handler_ret;

    vbi_decoder *   vbi;
#ifdef DEBUG
    vbi_export *    ex;
#endif
    vbi_sliced      sliced[MAX_SLICES];

    int             readorder;
} TeletextContext;

static TeletextContext *g_ctx = NULL;
extern int subtitle_status;
static unsigned SPU_RD_HOLD_SIZE = 0x20;
#define OSD_HALF_SIZE (1920*1280/8)
static int fileno_index = 0;



void *av_realloc_array(void *ptr, size_t nmemb, size_t size)
{
    if (!size || nmemb >= INT_MAX / size)
        return NULL;
    return av_realloc(ptr, nmemb * size);
}



static  int ff_data_identifier_is_teletext(int data_identifier)
{
    LOGI("[ff_data_identifier_is_teletext]---data_identifier:0x%x--\n",data_identifier);
    return (data_identifier >= 0x10 && data_identifier <= 0x1F ||
            data_identifier >= 0x99 && data_identifier <= 0x9B);
}

/* Returns true if data unit id matches EBU teletext data according to
 * EN 301 775 section 4.4.2 */
static  int ff_data_unit_id_is_teletext(int data_unit_id)
{
    /*teletext: 0x02,  teletext subtitle 0x03*/
    return (data_unit_id == 0x02 || data_unit_id == 0x03);
}


static int chop_spaces_utf8(const unsigned char* t, int len)
{
    t += len;
    while (len > 0) {
        if (*--t != ' ' || (len-1 > 0 && *(t-1) & 0x80))
            break;
        --len;
    }
    return len;
}

static void subtitle_rect_free(AVSubtitleRect **sub_rect)
{
    av_freep(&(*sub_rect)->pict.data[0]);
    av_freep(&(*sub_rect)->pict.data[1]);
    av_freep(&(*sub_rect)->ass);
    av_freep(sub_rect);
}

static char *create_ass_text(TeletextContext *ctx, const char *text)
{
    char *dialog;
    AVBPrint buf;

    av_bprint_init(&buf, 0, AV_BPRINT_SIZE_UNLIMITED);
//    ff_ass_bprint_text_event(&buf, text, strlen(text), "", 0);
    if (!av_bprint_is_complete(&buf)) {
        av_bprint_finalize(&buf, NULL);
        return NULL;
    }
    //dialog = ff_ass_get_dialog(ctx->readorder++, 0, NULL, NULL, buf.str);
    av_bprint_finalize(&buf, NULL);
    return dialog;
}

/* Draw a page as text */
static int gen_sub_text(TeletextContext *ctx, AVSubtitleRect *sub_rect, vbi_page *page, int chop_top)
{
    const char *in;
    AVBPrint buf;
   char *vbi_text = av_malloc(TEXT_MAXSZ);
    int sz;

    if (!vbi_text)
        return -1;//AVERROR(ENOMEM);

    sz = vbi_print_page_region(page, vbi_text, TEXT_MAXSZ-1, "UTF-8",
                                   /*table mode*/ TRUE, FALSE,
                                   0,             chop_top,
                                   page->columns, page->rows-chop_top);
    if (sz <= 0) {
        LOGI("vbi_print error\n");
        av_free(vbi_text);
        return -1;
    }
    vbi_text[sz] = '\0';
    in  = vbi_text;
    av_bprint_init(&buf, 0, TEXT_MAXSZ);

    if (ctx->chop_spaces) {
        for (;;) {
            int nl, sz;

            // skip leading spaces and newlines
            in += strspn(in, " \n");
            // compute end of row
            for (nl = 0; in[nl]; ++nl)
                if (in[nl] == '\n' && (nl == 0 || !(in[nl-1] & 0x80)))
                    break;
            if (!in[nl])
                break;
            // skip trailing spaces
            sz = chop_spaces_utf8(in, nl);
            //av_bprint_append_data(&buf, in, sz);
            av_bprintf(&buf, "\n");
            in += nl;
        }
    } else {
        av_bprintf(&buf, "%s\n", vbi_text);
    }
    av_free(vbi_text);

    if (!av_bprint_is_complete(&buf)) {
        av_bprint_finalize(&buf, NULL);
        return -1;//AVERROR(ENOMEM);
    }

    if (buf.len) {
       sub_rect->type = SUBTITLE_ASS;
        sub_rect->ass = create_ass_text(ctx, buf.str);

        if (!sub_rect->ass) {
            av_bprint_finalize(&buf, NULL);
            return -1;//AVERROR(ENOMEM);
       }
        LOGI("subtext:%s:txetbus\n", sub_rect->ass);
    } else {
        sub_rect->type = SUBTITLE_NONE;
   }
    av_bprint_finalize(&buf, NULL);
    return 0;
}

static void fix_transparency(TeletextContext *ctx, AVSubtitleRect *sub_rect, vbi_page *page,
                             int chop_top, int resx, int resy)
{
    int iy;

    // Hack for transparency, inspired by VLC code...
    for (iy = 0; iy < resy; iy++) {
        uint8_t *pixel = sub_rect->pict.data[0] + iy * sub_rect->pict.linesize[0];
        vbi_char *vc = page->text + (iy / BITMAP_CHAR_HEIGHT + chop_top) * page->columns;
        vbi_char *vcnext = vc + page->columns;
        for (; vc < vcnext; vc++) {
            uint8_t *pixelnext = pixel + BITMAP_CHAR_WIDTH;
            switch (vc->opacity) {
                case VBI_TRANSPARENT_SPACE:
                    memset(pixel, VBI_TRANSPARENT_BLACK, BITMAP_CHAR_WIDTH);
                    break;
                case VBI_OPAQUE:
                    if (!ctx->transparent_bg)
                        break;
                case VBI_SEMI_TRANSPARENT:
                   if (ctx->opacity > 0) {
                        if (ctx->opacity < 255)
                            for (; pixel < pixelnext; pixel++)
                                if (*pixel == vc->background)
                                    *pixel += VBI_NB_COLORS;
                        break;
                    }
                case VBI_TRANSPARENT_FULL:
                    for (; pixel < pixelnext; pixel++)
                        if (*pixel == vc->background)
                            *pixel = VBI_TRANSPARENT_BLACK;
                    break;
            }
            pixel = pixelnext;
        }
    }
}

#if 1
static void png_save2(const char *filename, uint32_t *bitmap, int w, int h)
{
    LOGI("png_save2:%s\n",filename);
    int x, y, v;
    FILE *f;
    char fname[40], fname2[40];
    char command[1024];

    snprintf(fname, sizeof(fname), "%s.bmp", filename);
    f = fopen(fname, "w");
    if (!f) {
        perror(fname);
        return;
    }
    fprintf(f, "P6\n"
            "%d %d\n"
            "%d\n",
            w, h, 255);
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            v = bitmap[y * w + x];
            putc((v >> 16) & 0xff, f);
            putc((v >> 8) & 0xff, f);
            putc((v >> 0) & 0xff, f);
        }
    }
    fclose(f);

    snprintf(fname2, sizeof(fname2), "%s-a.bmp", filename);

    f = fopen(fname2, "w");
    if (!f) {
        perror(fname2);
        return;
    }
    fprintf(f, "P5\n"
            "%d %d\n"
            "%d\n",
            w, h, 255);
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            v = bitmap[y * w + x];
            putc((v >> 24) & 0xff, f);
        }
    }
    fclose(f);
/*
    snprintf(command, sizeof(command), "pnmtopng -alpha %s %s > %s.png 2> /dev/null", fname2, fname, filename);
    system(command);

    snprintf(command, sizeof(command), "rm %s %s", fname, fname2);
    system(command);
    */
}


static int save_display_set(AML_SPUVAR *spu, TeletextContext *ctx, AVSubtitleRect *sub_rect)
{
    LOGI("save_display_set\n");
    int resx = sub_rect->w;
    int resy = sub_rect->h;
    char filename[32];

    uint32_t *pbuf;

    spu->spu_width = resx;
    spu->spu_height = resy;

    pbuf = av_malloc(resx * resy * sizeof(uint32_t));
    if (!pbuf)
        return -1;
       spu->buffer_size = resx * resy * sizeof(uint32_t);
       spu->spu_data = av_malloc(spu->buffer_size);
       if (!spu->spu_data) {
           av_free(pbuf);
           pbuf = NULL;
           return -1;
       }

    int x,y;
    for (y = 0; y < resy; y++) {
        for (x = 0; x < resx; x++) {
               pbuf[(y * resx) + x] =
                     ((uint32_t *)sub_rect->pict.data[1])[(sub_rect->pict.data[0])[y * resx + x]];
               spu->spu_data[(y * resx * 4) + x * 4] =
                                  pbuf[(y * resx) + x] & 0xff;
               spu->spu_data[(y * resx * 4) + x * 4 + 1] =
                                  (pbuf[(y * resx) + x] >> 8) & 0xff;
               spu->spu_data[(y * resx * 4) + x * 4 + 2] =
                                  (pbuf[(y * resx) + x] >> 16) & 0xff;
                /*spu->spu_data[(y * resx * 4) + x * 4 + 3] = spu->spu_data[(y * resx * 4) + x * 4]
                & spu->spu_data[(y * resx * 4) + x * 4+1]
                & spu->spu_data[(y * resx * 4) + x * 4+2];*/             //transparent style
                spu->spu_data[(y * resx * 4) + x * 4 + 3] = (pbuf[(y * resx) + x] >> 24) & 0xff;   //color style

        }
    }

    snprintf(filename, sizeof(filename), "/data/dvb_teletext/dvbs_%d", fileno_index);

    //png_save2(filename, spu->spu_data, resx, resy);

    av_freep(&pbuf);
    fileno_index++;
    return 0;
}
#endif



/* Draw a page as bitmap */
static int gen_sub_bitmap(TeletextContext *ctx, AVSubtitleRect *sub_rect, vbi_page *page, int chop_top)
{
    int resx = page->columns * BITMAP_CHAR_WIDTH;
    int resy = (page->rows - chop_top) * BITMAP_CHAR_HEIGHT;
    uint8_t ci;
    vbi_char *vc = page->text + (chop_top * page->columns);
    vbi_char *vcend = page->text + (page->rows * page->columns);
    LOGI("--%s--\n",__FUNCTION__);
    for (; vc < vcend; vc++) {
        if (vc->opacity != VBI_TRANSPARENT_SPACE)
            break;
    }

    if (vc >= vcend) {
        LOGI("dropping empty page %3x\n", page->pgno);
        sub_rect->type = SUBTITLE_NONE;
        return 0;
    }

   sub_rect->pict.data[0] = av_mallocz(resx * resy);
    sub_rect->pict.linesize[0] = resx;
    if (!sub_rect->pict.data[0])
        return -1;//AVERROR(ENOMEM);

    vbi_draw_vt_page_region(page, VBI_PIXFMT_PAL8,
                            sub_rect->pict.data[0], sub_rect->pict.linesize[0],
                            0, chop_top, page->columns, page->rows - chop_top,
                            /*reveal*/ 1, /*flash*/ 1, /*Subtitle*/1, 0, 0, 0, NULL, 0);

    fix_transparency(ctx, sub_rect, page, chop_top, resx, resy);
    sub_rect->x = ctx->x_offset;
   sub_rect->y = ctx->y_offset + chop_top * BITMAP_CHAR_HEIGHT;
    sub_rect->w = resx;
    sub_rect->h = resy;
    sub_rect->nb_colors = ctx->opacity > 0 && ctx->opacity < 255 ? 2 * VBI_NB_COLORS : VBI_NB_COLORS;
    sub_rect->pict.data[1] = av_mallocz(AVPALETTE_SIZE);
    if (!sub_rect->pict.data[1]) {
        av_freep(&sub_rect->pict.data[0]);
        return -1;//AVERROR(ENOMEM);
    }
    for (ci = 0; ci < VBI_NB_COLORS; ci++) {
        int r, g, b, a;

        r = VBI_R(page->color_map[ci]);
        g = VBI_G(page->color_map[ci]);
        b = VBI_B(page->color_map[ci]);
        a = VBI_A(page->color_map[ci]);
        ((uint32_t *)sub_rect->pict.data[1])[ci] = RGBA(r, g, b, a);
        ((uint32_t *)sub_rect->pict.data[1])[ci + VBI_NB_COLORS] = RGBA(r, g, b, ctx->opacity);
        //LOGI("palette %0x\n", ((uint32_t *)sub_rect->data[1])[ci]);
    }
    ((uint32_t *)sub_rect->pict.data[1])[VBI_TRANSPARENT_BLACK] = RGBA(0, 0, 0, 0);
    ((uint32_t *)sub_rect->pict.data[1])[VBI_TRANSPARENT_BLACK + VBI_NB_COLORS] = RGBA(0, 0, 0, 0);
    sub_rect->type = SUBTITLE_BITMAP;
    return 0;
}

static void handler(vbi_event *ev, void *user_data)
{
    LOGI("[handler]\n");
    TeletextContext *ctx = user_data;
    TeletextPage *new_pages;
    vbi_page page;
    int res;
    char pgno_str[12];
    vbi_subno subno;
    vbi_page_type vpt;
    int chop_top;
    char *lang;
    int page_type;

    snprintf(pgno_str, sizeof pgno_str, "%03x", ev->ev.ttx_page.pgno);
    LOGI("decoded page %s.;  %02x, ctx->pgno=%s\n",
           pgno_str, ev->ev.ttx_page.subno & 0xFF, ctx->pgno);

    if (strcmp(ctx->pgno, "*") && !strstr(ctx->pgno, pgno_str)) {
        LOGI("%s, return ,pageno = * \n",__FUNCTION__);
        return;
    }
    if (ctx->handler_ret < 0) {
        LOGI("%s, return , ctx->handler_ret=%d\n",__FUNCTION__, ctx->handler_ret);
        return;
    }

    res = vbi_fetch_vt_page(ctx->vbi, &page,
                            ev->ev.ttx_page.pgno,
                            ev->ev.ttx_page.subno,
                            VBI_WST_LEVEL_3p5, 25, TRUE, &page_type);

    if (!res) {
        LOGI("%s, return, page get error\n",__FUNCTION__);
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "\nSaving res=%d dy0=%d dy1=%d...\n",
            res, page.dirty.y0, page.dirty.y1);
    fflush(stderr);

    if (!vbi_export_stdio(ctx->ex, stderr, &page))
        fprintf(stderr, "failed: %s\n", vbi_export_errstr(ctx->ex));
#endif

    vpt = vbi_classify_page(ctx->vbi, ev->ev.ttx_page.pgno, &subno, &lang);
    chop_top = ctx->chop_top ||
        ((page.rows > 1) && (vpt == VBI_SUBTITLE_PAGE));

    LOGI("%d x %d page chop:%d\n",
           page.columns, page.rows, chop_top);

    if (ctx->nb_pages < MAX_BUFFERED_PAGES) {
        if ((new_pages = av_realloc_array(ctx->pages, ctx->nb_pages + 1, sizeof(TeletextPage)))) {
            TeletextPage *cur_page = new_pages + ctx->nb_pages;
            ctx->pages = new_pages;
            cur_page->sub_rect = av_mallocz(sizeof(*cur_page->sub_rect));
            cur_page->pts = ctx->pts;
            cur_page->pgno = ev->ev.ttx_page.pgno;
            cur_page->subno = ev->ev.ttx_page.subno;
            //LOGI("%s, ret cur_page->pts:0x%x\n",__FUNCTION__,cur_page->pts);
            if (cur_page->sub_rect) {
                res = (ctx->format_id == 0) ?
                    gen_sub_bitmap(ctx, cur_page->sub_rect, &page, chop_top) :
                    gen_sub_text  (ctx, cur_page->sub_rect, &page, chop_top);
                if (res < 0) {
                    av_freep(&cur_page->sub_rect);
                    ctx->handler_ret = res;
                } else {
                    ctx->pages[ctx->nb_pages++] = *cur_page;
                }
            } else {
                ctx->handler_ret = -1;//AVERROR(ENOMEM);
            }
        } else {
            ctx->handler_ret = -1;//AVERROR(ENOMEM);
        }
    } else {
        //TODO: If multiple packets contain more than one page, pages may got queued up, and this may happen...
        LOGI("Buffered too many pages, dropping page %s.\n", pgno_str);
        ctx->handler_ret = -1;//AVERROR(ENOSYS);
    }

    vbi_unref_page(&page);
}

static int slice_to_vbi_lines(TeletextContext *ctx, uint8_t* buf, int size)
{
    int lines = 0;
    while (size >= 2 && lines < MAX_SLICES) {
        int data_unit_id     = buf[0];     //02
        int data_unit_length = buf[1];  //2c  value: 44
        LOGI("[slice_to_vbi_lines]-id:%x,id:%d, length:%d,length:%x ,size:%d\n",buf[0],data_unit_id,data_unit_length,data_unit_length,size);
        if (data_unit_length + 2 > size)
            return lines;
        if (ff_data_unit_id_is_teletext(data_unit_id)) {
            LOGI("[slice_to_vbi_lines]-length==:%x",data_unit_length);
            if (data_unit_length != 0x2c)          //44
                return -1;
            else {
                int line_offset  = buf[2] & 0x1f;  //e8  value: 8
                int field_parity = buf[2] & 0x20;  //e8  value:0x20  32
                LOGI("[slice_to_vbi_lines]-buf[2]:%x,line_offset::%d,field_parity:%d",buf[2],line_offset,field_parity);
                int i;
                ctx->sliced[lines].id = VBI_SLICED_TELETEXT_B;
                ctx->sliced[lines].line = (line_offset > 0 ? (line_offset + (field_parity ? 0 : 13)) : 0);
                LOGI("[slice_to_vbi_lines]-buf[2]:line:%d",ctx->sliced[lines].line);
                for (i = 0; i < 42; i++)
                    ctx->sliced[lines].data[i] = vbi_rev8(buf[4 + i]);
                lines++;
            }
        }
        size -= data_unit_length + 2;
        buf += data_unit_length + 2;
    }
    if (size)
        LOGI("%d bytes remained after slicing data\n", size);
    return lines;
}

//static int teletext_decode_frame(AVCodecContext *avctx, void *data, int *data_size, AVPacket *pkt)
static int teletext_decode_frame( AML_SPUVAR *spu, const uint8_t *s_data, const int s_len)
{
    TeletextContext *ctx = g_ctx;
    //AVSubtitle      *sub = data;
    int             ret = 0;
    int j;

	if (ctx->format_id == 0) {
          spu->spu_width  = 41 * BITMAP_CHAR_WIDTH;
          spu->spu_height = 25 * BITMAP_CHAR_HEIGHT;
    }
    LOGI("[teletext_decode_frame]---0--\n");
    if (!ctx->vbi) {
        LOGI("[teletext_decode_frame]---00==--\n");
        if (!(ctx->vbi = vbi_decoder_new()))
            return -1;//AVERROR(ENOMEM);
        LOGI("[teletext_decode_frame]---01:%x--\n",ctx->vbi);
        //vbi_teletext_set_default_region(ctx->vbi, 32);
        if (!vbi_event_handler_add(ctx->vbi, VBI_EVENT_TTX_PAGE, handler, ctx)) {
            LOGI("[teletext_decode_frame]---03--\n");
            vbi_decoder_delete(ctx->vbi);
            ctx->vbi = NULL;
            return -1;//AVERROR(ENOMEM);
        }
#if 0
    vbi_event_handler_register(parser->dec, VBI_EVENT_TTX_PAGE, handler, ctx);
        if (!vbi_event_handler_add(ctx->vbi, VBI_EVENT_TTX_PAGE, handler, ctx)) {
            vbi_decoder_delete(ctx->vbi);
            ctx->vbi = NULL;
            return -1;//AVERROR(ENOMEM);
        }
#endif
    }

   // if (avctx->pkt_timebase.num && pkt->pts != AV_NOPTS_VALUE)
   //     ctx->pts = av_rescale_q(pkt->pts, avctx->pkt_timebase, AV_TIME_BASE_Q);
    LOGI("[teletext_decode_frame]---1--\n");
    if (s_len) {
        int lines;
        const int full_pes_size = s_len + 45; /* PES header is 45 bytes */
        LOGI("[teletext_decode_frame]---1-size-%d: \n",full_pes_size % 184);
        // We allow unreasonably big packets, even if the standard only allows a max size of 1472
        if (full_pes_size < 184 || full_pes_size > 65504 /*|| full_pes_size % 184 != 0*/)
            return -1;
        LOGI("[teletext_decode_frame]---2--\n");
        ctx->handler_ret = s_len;

        if (ff_data_identifier_is_teletext(*s_data)) {
            LOGI("[teletext_decode_frame]---3--\n");
            if ((lines = slice_to_vbi_lines(ctx, s_data + 1, s_len - 1)) < 0)
                return lines;
            LOGI("ctx=%p buf_size=%d lines=%u\n",
                    ctx, s_len, lines);
            if (lines > 0) {
#if 1
                int i;
               LOGI("line numbers:");
                for (i = 0; i < lines; i++)
                    LOGI(" %d", ctx->sliced[i].line);
                LOGI("\n");
#endif
                //LOGI("--%s, ctx->vbi->time=%f\n", __FUNCTION__, ctx->vbi->time);
                vbi_decode(ctx->vbi, ctx->sliced, lines, 0.0);
                ctx->lines_processed += lines;
				LOGI("%s, after vbi decode,ctx->lines_processed=%d\n",__FUNCTION__,ctx->lines_processed);
            }
        }
        ctx->pts = AV_NOPTS_VALUE;
        ret = ctx->handler_ret;
    }

    if (ret < 0) {
		LOGI("--%s,ret = %d,ctx->handler_ret=%d\n",__FUNCTION__,ret,ctx->handler_ret);
        return ret;
    }

	LOGI("--%s,ret = %d,ctx->handler_ret=%d,ctx->nb_pages=%d,ctx->format_id=%d\n",
		__FUNCTION__,ret,ctx->handler_ret,ctx->nb_pages,ctx->format_id);

            LOGI("[teletext_decode_frame]---4--\n");
            LOGI("[teletext_decode_frame]---1:%d--\n",ctx == NULL);
            //LOGI("[teletext_decode_frame]---2:%d--\n",ctx->pages == NULL);
            //LOGI("[teletext_decode_frame]---3:%d--\n",ctx->pages->sub_rect == NULL);
            //LOGI("[teletext_decode_frame]--1:%d,2:%d,3:%d--\n",ctx == NULL,ctx->pages == NULL,ctx->pages->sub_rect == NULL);
            //LOGI("[teletext_decode_frame]--w:%d,h:%d--\n",ctx->pages->sub_rect->w,ctx->pages->sub_rect->h);
            LOGI("[teletext_decode_frame]---5--\n");

#if 1
    // is there a subtitle to pass?
    if (ctx->nb_pages) {
        int i;
        //sub->format = ctx->format_id;
        //sub->start_display_time = 0;
       // sub->end_display_time = ctx->sub_duration;
       // sub->num_rects = 0;
       //sub->pts = ctx->pages->pts;

       if (ctx->pages && ctx->pages->sub_rect &&
			ctx->pages->sub_rect->type != SUBTITLE_NONE) {
            //sub->rects = av_malloc(sizeof(*sub->rects));
            if (1/*sub->rects*/) {
                //sub->num_rects = 1;
                //sub->rects[0] = ctx->pages->sub_rect;
				if (ctx->format_id == 0) {
                    ret = save_display_set(spu, ctx, ctx->pages->sub_rect);
					if (ret < 0)
						return -1;
                }
#if 0 /*FF_API_AVPICTURE*/
FF_DISABLE_DEPRECATION_WARNINGS
                for (j = 0; j < 4; j++) {
                    sub->rects[0]->pict.data[j] = sub->rects[0]->data[j];
                    sub->rects[0]->pict.linesize[j] = sub->rects[0]->linesize[j];
                }
FF_ENABLE_DEPRECATION_WARNINGS
#endif
            } else {
                ret = -1;//AVERROR(ENOMEM);
            }
        } else {
            LOGI("sending empty sub\n");
            //sub->rects = NULL;
        }
        //if (!sub->rects) // no rect was passed
        //    subtitle_rect_free(&ctx->pages->sub_rect);

        for (i = 0; i < ctx->nb_pages - 1; i++)
            ctx->pages[i] = ctx->pages[i + 1];
        ctx->nb_pages--;

    } else {

    }

#endif
    return ret;
}

int teletext_init_decoder()
{
    //TeletextContext *ctx = avctx->priv_data;

    g_ctx = av_mallocz(sizeof(TeletextContext));
    if (!g_ctx)
        LOGE("[%s::%d]malloc error! \n", __FUNCTION__, __LINE__);
    TeletextContext *ctx = g_ctx;

    unsigned int maj, min, rev;

    vbi_version(&maj, &min, &rev);
    if (!(maj > 0 || min > 2 || min == 2 && rev >= 26)) {
       LOGI("decoder needs zvbi version >= 0.2.26.\n");
        return -1;
    }
	ctx->format_id = 0;
/*
    if (ctx->format_id == 0) {
        avctx->width  = 41 * BITMAP_CHAR_WIDTH;
        avctx->height = 25 * BITMAP_CHAR_HEIGHT;
    }
*/
    ctx->vbi = NULL;
    ctx->pts = AV_NOPTS_VALUE;
    ctx->sub_duration = 30000;
    ctx->opacity = -1;
    ctx->chop_top = 1;
    ctx->x_offset = 0;
    ctx->y_offset = 0;
    ctx->chop_spaces = 1;
    ctx->transparent_bg = 0;
    ctx->pgno = "*";

    if (ctx->opacity == -1)
        ctx->opacity = ctx->transparent_bg ? 0 : 255;

#ifdef DEBUG
    {
        char *t;
        ctx->ex = vbi_export_new("text", &t);
    }
#endif
    LOGI("page filter: %s\n", ctx->pgno);
	fileno_index = 0;

    //return (ctx->format_id == 1) ? ff_ass_subtitle_header_default(avctx) : 0;
    return 0;
}

int teletext_close_decoder()
{
    TeletextContext *ctx = g_ctx;
    if (g_ctx != NULL)  {
	    LOGI("lines_total=%u\n", ctx->lines_processed);
	    while (ctx->nb_pages) {
			if (ctx->pages[--ctx->nb_pages].sub_rect) {
	            subtitle_rect_free(&ctx->pages[--ctx->nb_pages].sub_rect);
			}
	    }
	    av_freep(&ctx->pages);

	    vbi_decoder_delete(ctx->vbi);
	    ctx->vbi = NULL;
	    ctx->pts = AV_NOPTS_VALUE;
		av_freep(&g_ctx);
    }
	fileno_index = 0;
  //  if (!(avctx->flags2 & AV_CODEC_FLAG2_RO_FLUSH_NOOP))
  //      ctx->readorder = 0;
    return 0;
}

static int read_spu_buf(int read_handle, char *buf, int len)
{
    LOGI("read_spu_buf len = %d\n", len);
    if (len > 3 * 1024 * 1024)
        abort();
        /*while (getSize(read_handle) < len) {
        usleep(20000);
        if (subtitle_status == SUB_STOP)
            return 0;
        }*/
    getData(read_handle, buf, len);
    return len;
}


/* check subtitle hw buffer has enough data to read */
static int check_sub_buffer_length(int read_handle, int size)
{
    int ret = getSize(read_handle);
    LOGI("[check_sub_buffer_length]read_handle:%d, size:%d, sub buffer level = %d \n", read_handle, size, ret);
    if (ret >= size)
        return 1;
    return 0;
}

int get_dvb_teletext_spu(AML_SPUVAR *spu, int read_handle)
{
    char tmpbuf[256];
    unsigned int dvb_pts = 0, dvb_dts = 0, dvb_pts_end;
    unsigned int dvb_temp_pts, dvb_temp_dts;
    int dvb_packet_length = 0, dvb_pes_header_length = 0;
    int64_t packet_header = 0;
    char skip_packet_flag = 0;
    int ret = 0;
   LOGI("enter get_dvb_teletext_spu\n");
    while (1)
    {
        if (subtitle_status == SUB_STOP)
            return 0;
        LOGI("enter get_dvb_teletext_spu-1-\n");
        dvb_pts = dvb_dts = 0;
        dvb_packet_length = dvb_pes_header_length = 0;
        packet_header = 0;
        skip_packet_flag = 0;
        if ((getSize(read_handle)) < SPU_RD_HOLD_SIZE)
        {
           LOGI("current dvb teletext sub buffer size %d\n",
                 (getSize(read_handle)));
            break;
        }
        while (read_spu_buf(read_handle, tmpbuf, 1) == 1)
        {
            packet_header = (packet_header << 8) | tmpbuf[0];
            LOGI("## get_dvb_spu %x, %llx,-------------\n",tmpbuf[0], packet_header);
            if ((packet_header & 0xffffffff) == 0x000001bd)
            {
                LOGI("## 222  get_dvb_teletext_spu hardware demux dvb %x,%llx,-----------\n", tmpbuf[0], packet_header & 0xffffffffff);
                break;
            }
            else if ((packet_header & 0xffffffffff) == 0x414d4c5577 ||(packet_header & 0xffffffffff) ==
                    0x414d4c55aa)
            {
                LOGI("## 222  get_dvb_teletext_spu soft demux dvb %x,%llx,-----------\n", tmpbuf[0], packet_header & 0xffffffffff);
                goto aml_soft_demux;

            }
        }
        if ((packet_header & 0xffffffff) == 0x000001bd)
       {
            LOGI("find header 0x000001bd\n");
            //dvb_sub_valid_flag = 0;
            if (read_spu_buf(read_handle, tmpbuf, 2) == 2)
            {
                dvb_packet_length =
                    (tmpbuf[0] << 8) | tmpbuf[1];
                if (dvb_packet_length >= 3)
                {
                    if (read_spu_buf(read_handle, tmpbuf, 3)
                            == 3)
                    {
                        dvb_packet_length -= 3;
                        dvb_pes_header_length =
                            tmpbuf[2];
                        if (dvb_packet_length >=
                                dvb_pes_header_length)
                        {
                            if ((tmpbuf[1] & 0xc0)
                                   == 0x80)
                            {
                                if (read_spu_buf
                                        (read_handle,
                                         tmpbuf,
                                         dvb_pes_header_length)
                                        ==
                                        dvb_pes_header_length)
                                {
                                    dvb_temp_pts
                                        = 0;
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[0] & 0xe) << 29);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[1] & 0xff) << 22);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[2] & 0xfe) << 14);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[3] & 0xff) << 7);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                           |
                                            ((tmpbuf[4] & 0xfe) >> 1);
                                    dvb_pts = dvb_temp_pts; // - pts_aligned;
                                    dvb_packet_length
                                    -=
                                       dvb_pes_header_length;
                                }
                            }
                            else if ((tmpbuf[1] &
                                      0xc0) ==
                                     0xc0)
                           {
                                if (read_spu_buf
                                        (read_handle,
                                        tmpbuf,
                                         dvb_pes_header_length)
                                        ==
                                        dvb_pes_header_length)
                                {
                                    dvb_temp_pts
                                        = 0;
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[0] & 0xe) << 29);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[1] & 0xff) << 22);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[2] & 0xfe) << 14);
                                    dvb_temp_pts
                                        =
                                           dvb_temp_pts
                                            |
                                            ((tmpbuf[3] & 0xff) << 7);
                                    dvb_temp_pts
                                        =
                                            dvb_temp_pts
                                            |
                                            ((tmpbuf[4] & 0xfe) >> 1);
                                    dvb_pts = dvb_temp_pts; //- pts_aligned;
                                    dvb_temp_dts
                                        = 0;
                                    dvb_temp_dts
                                        =
                                            dvb_temp_dts
                                            |
                                            ((tmpbuf[5] & 0xe) << 29);
                                    dvb_temp_dts
                                        =
                                            dvb_temp_dts
                                            |
                                            ((tmpbuf[6] & 0xff) << 22);
                                    dvb_temp_dts
                                        =
                                            dvb_temp_dts
                                            |
                                            ((tmpbuf[7] & 0xfe) << 14);
                                    dvb_temp_dts
                                        =
                                            dvb_temp_dts
                                            |
                                            ((tmpbuf[8] & 0xff) << 7);
                                    dvb_temp_dts
                                        =
                                            dvb_temp_dts
                                            |
                                            ((tmpbuf[9] & 0xfe) >> 1);
                                    dvb_dts = dvb_temp_dts; // - pts_aligned;
                                    dvb_packet_length
                                    -=
                                        dvb_pes_header_length;
                                }
                            }
                            else
                            {
                                skip_packet_flag
                                    = 1;
                            }
                        }
                        else
                        {
                            skip_packet_flag = 1;
                        }
                    }
                }
                else
                {
                   skip_packet_flag = 1;
                }
                if (skip_packet_flag)
                {
                    int iii;
                   char tmp;
                    for (iii = 0; iii < dvb_packet_length;
                            iii++)
                    {
                        if (read_spu_buf
                                (read_handle, &tmp, 1) == 0)
                            break;
                    }
                }
                else if ((dvb_pts) && (dvb_packet_length > 0))
               {
                    char *buf = NULL;
                    if ((dvb_packet_length) >
                            (OSD_HALF_SIZE * 4))
                   {
                        LOGE("dvb packet is too big\n\n");
                        break;
                   }
                    spu->subtitle_type = SUBTITLE_DVB_TELETEXT;
                    //spu->buffer_size = DVB_SUB_SIZE;
                   // spu->spu_data = malloc(DVB_SUB_SIZE);
                   // if (!spu->spu_data)
                   //     LOGE("[%s::%d]malloc error! \n",
                  //          __FUNCTION__, __LINE__);
                   // LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n", __FUNCTION__, __LINE__, spu->spu_data, DVB_SUB_SIZE);
                   spu->pts = dvb_pts;
                    read_spu_buf(read_handle, tmpbuf, 2);
                    dvb_packet_length -= 2;
                    dvb_temp_pts =
                        ((tmpbuf[0] << 8) | tmpbuf[1]);
                    buf = malloc(dvb_packet_length);
                    if (buf)
                    {
                        LOGI("dvb_packet_length is %d, pts is %x, delay is %x,\n", dvb_packet_length, spu->pts, dvb_temp_pts);
                    }
                    else
                    {
                        LOGI("dvb_packet_length buf malloc fail!!!,\n");
                    }
                    if (buf)
                    {
                        memset(buf, 0x0,
                               dvb_packet_length);
                        while (!check_sub_buffer_length
                                (read_handle,
                                 dvb_packet_length))
                        {
                            usleep(100000);
                           if (subtitle_status ==
                                    SUB_STOP)
                            {
                                LOGI("subtitle_status == SUB_STOP  out of check sub buffer \n");
                                return 0;
                            }
                            LOGI("waiting for hw buffer getting enough data for decoder");
                        }
                        if (read_spu_buf
                               (read_handle, buf,
                                 dvb_packet_length) ==
                                dvb_packet_length)
                        {
                            LOGI("start decode dvb subtitle\n\n");
                            ret =
                                teletext_decode_frame(spu,
                                             buf,
                                              dvb_packet_length);
                            if (ret != -1)
                            {
                                write_subtitle_file
                                (spu);
                           }
                            else
                            {
                                LOGI("dvb data error skip one frame !!\n\n");
                               //return -1;
                            }
                        }
                       if (buf)
                        {
                            LOGI("dvb_packet_length buf free = 0x%x \n", buf);
                            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__, __LINE__, buf);
                            av_free(buf);
                        }
                        buf = NULL;
                    }
                    continue;
                }
            }
        }
        else
        {
            LOGI("header is not 0x000001bd\n");
            break;
        }
    }
aml_soft_demux:
    if ((packet_header & 0xffffffffff) == 0x414d4c5577 ||(packet_header & 0xffffffffff) == 0x414d4c55aa)
    {
        int data_len = 0;
        char *data = NULL;
        char *pdata = NULL;
        //dvb_sub_valid_flag = 0;
        LOGI("## 333 get_dvb_spu %x,%x,%x,%x,-------------\n",
             tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3]);
        if (read_spu_buf(read_handle, tmpbuf, 15) == 15)
        {
             LOGI("## 333 get_dvb_spu %x,%x,%x,  %x,%x,%x,  %x,%x,-------------\n",
             tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3],tmpbuf[4], tmpbuf[5], tmpbuf[6], tmpbuf[7]);
            data_len = tmpbuf[3] << 24;
            data_len |= tmpbuf[4] << 16;
            data_len |= tmpbuf[5] << 8;
            data_len |= tmpbuf[6];
            dvb_temp_pts = tmpbuf[7] << 24;
            dvb_temp_pts |= tmpbuf[8] << 16;
            dvb_temp_pts |= tmpbuf[9] << 8;
            dvb_temp_pts |= tmpbuf[10];
            dvb_pts = dvb_temp_pts;
            dvb_temp_pts = 0;
            dvb_temp_pts = tmpbuf[11] << 24;
            dvb_temp_pts |= tmpbuf[12] << 16;
            dvb_temp_pts |= tmpbuf[13] << 8;
            dvb_temp_pts |= tmpbuf[14];
            //spu->m_delay = dvb_temp_pts;
            //if (dvb_temp_pts != 0) {
           //    spu->m_delay += dvb_pts;
            //}
            spu->subtitle_type = SUBTITLE_DVB_TELETEXT;
            //spu->buffer_size = DVB_SUB_SIZE;
            //spu->spu_data = malloc(DVB_SUB_SIZE);

            spu->pts = dvb_pts;
            LOGI("## spu-> pts:%d,dvb_pts:%d\n",spu->pts,dvb_pts);
            LOGI("## 4444 datalen=%d,pts=%u,delay=%x,diff=%x, data: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,--%x,%x,%x,-------------\n", data_len, dvb_pts, spu->m_delay, dvb_temp_pts, tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5], tmpbuf[6], tmpbuf[7], tmpbuf[8], tmpbuf[9], tmpbuf[10], tmpbuf[11], tmpbuf[12], tmpbuf[13], tmpbuf[14]);
            data = malloc(data_len);
            if (!data)
                LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                     __LINE__);
            LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
                 __FUNCTION__, __LINE__, data, data_len);
            memset(data, 0x0, data_len);
            ret = read_spu_buf(read_handle, data, data_len);
            LOGI("## ret=%d,data_len=%d, %x,%x,%x,%x,%x,%x,%x,%x,---------\n", ret, data_len, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
            pdata = data;
            ret = teletext_decode_frame(spu, data, data_len);
            LOGI("## dvb: (width=%d,height=%d), (x=%d,y=%d),ret =%d,spu->buffer_size=%d--------\n",
			spu->spu_width, spu->spu_height, spu->spu_start_x, spu->spu_start_y, ret, spu->buffer_size);
            if (ret != -1 && spu->buffer_size > 0)
           {
                write_subtitle_file(spu);
            }
            else
            {
                LOGI("dvb data error skip one frame !!\n\n");
                //return -1;
            }
            //write_subtitle_file(spu);
            if (pdata)
            {
                free(pdata);
                pdata = NULL;
            }
        }
    }
    return ret;
}


#if 0
static void teletext_flush(AVCodecContext *avctx)
{
    teletext_close_decoder(avctx);
}


#define OFFSET(x) offsetof(TeletextContext, x)
#define SD AV_OPT_FLAG_SUBTITLE_PARAM | AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    {"txt_page",        "list of teletext page numbers to decode, * is all", OFFSET(pgno),           AV_OPT_TYPE_STRING, {.str = "*"},      0, 0,        SD},
    {"txt_chop_top",    "discards the top teletext line",                    OFFSET(chop_top),       AV_OPT_TYPE_INT,    {.i64 = 1},        0, 1,        SD},
    {"txt_format",      "format of the subtitles (bitmap or text)",          OFFSET(format_id),      AV_OPT_TYPE_INT,    {.i64 = 0},        0, 1,        SD,  "txt_format"},
    {"bitmap",          NULL,                                                0,                      AV_OPT_TYPE_CONST,  {.i64 = 0},        0, 0,        SD,  "txt_format"},
    {"text",            NULL,                                                0,                      AV_OPT_TYPE_CONST,  {.i64 = 1},        0, 0,        SD,  "txt_format"},
    {"txt_left",        "x offset of generated bitmaps",                     OFFSET(x_offset),       AV_OPT_TYPE_INT,    {.i64 = 0},        0, 65535,    SD},
    {"txt_top",         "y offset of generated bitmaps",                     OFFSET(y_offset),       AV_OPT_TYPE_INT,    {.i64 = 0},        0, 65535,    SD},
    {"txt_chop_spaces", "chops leading and trailing spaces from text",       OFFSET(chop_spaces),    AV_OPT_TYPE_INT,    {.i64 = 1},        0, 1,        SD},
    {"txt_duration",    "display duration of teletext pages in msecs",       OFFSET(sub_duration),   AV_OPT_TYPE_INT,    {.i64 = 30000},    0, 86400000, SD},
    {"txt_transparent", "force transparent background of the teletext",      OFFSET(transparent_bg), AV_OPT_TYPE_INT,    {.i64 = 0},        0, 1,        SD},
    {"txt_opacity",     "set opacity of the transparent background",         OFFSET(opacity),        AV_OPT_TYPE_INT,    {.i64 = -1},      -1, 255,      SD},
    { NULL },
};


static const AVClass teletext_class = {
    .class_name = "libzvbi_teletextdec",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_libzvbi_teletext_decoder = {
    .name      = "libzvbi_teletextdec",
    .long_name = NULL_IF_CONFIG_SMALL("Libzvbi DVB teletext decoder"),
    .type      = AVMEDIA_TYPE_SUBTITLE,
    .id        = AV_CODEC_ID_DVB_TELETEXT,
    .priv_data_size = sizeof(TeletextContext),
    .init      = teletext_init_decoder,
    .close     = teletext_close_decoder,
    .decode    = teletext_decode_frame,
    .capabilities = AV_CODEC_CAP_DELAY,
   .flush     = teletext_flush,
    .priv_class= &teletext_class,
};
#endif
