/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include <string.h>
#include "includes.h"
#include "memwatch.h"

#include "clut_tables.h"
#include "ycbcr_rgb.h"
#include "sub_bits.h"
//#include "sub_memory.h"
#include "stdio.h"

#include "dvb_sub.h"
#include <am_debug.h>

#define DVBSUB_DEBUG                                (0)

#if DVBSUB_DEBUG
#define dvbsub_dbg(args...)                         AM_DEBUG(1, args)
#else
#define dvbsub_dbg(args...)  {do{}while(0);}
#endif

//          {do{}while(0);}

/* define for segment type */
#define DVBSUB_ST_PAGE_COMPOSITION                  (0x10)
#define DVBSUB_ST_REGION_COMPOSITION                (0x11)
#define DVBSUB_ST_CLUT_DEFINITION                   (0x12)
#define DVBSUB_ST_OBJECT_DATA                       (0x13)
#define DVBSUB_ST_DISPLAY_DEFINITION                (0x14)
#define DVBSUB_ST_ENDOFDISPLAY                      (0x80)
#define DVBSUB_ST_STUFFING                          (0xff)

/* define for page composition segment state */
#define DVBSUB_PCS_STATE_NORMAL_CASE                (0x00)
#define DVBSUB_PCS_STATE_ACQUISITION                (0x01)
#define DVBSUB_PCS_STATE_MODE_CHANGE                (0x02)

/* define for object type */
#define DVBSUB_OT_BASIC_BITMAP                      (0x00)
#define DVBSUB_OT_BASIC_CHARACTER                   (0x01)
#define DVBSUB_OT_COMPOSITE_STRING                  (0x02)

/* define for pixel data sub block */
#define DVBSUB_DT_2BP_CODE_STRING                   (0x10)
#define DVBSUB_DT_4BP_CODE_STRING                   (0x11)
#define DVBSUB_DT_8BP_CODE_STRING                   (0x12)
#define DVBSUB_DT_24_MAP_TABLE_DATA                 (0x20)
#define DVBSUB_DT_28_MAP_TABLE_DATA                 (0x21)
#define DVBSUB_DT_48_MAP_TABLE_DATA                 (0x22)
#define DVBSUB_DT_END_OF_OBJECT_LINE                (0xf0)

typedef struct dvbsub_color_s
{
    INT8U                   Y;
    INT8U                   Cr;
    INT8U                   Cb;
    INT8U                   T;

} dvbsub_color_t;

/* The CLUT definition segment */
typedef struct dvbsub_clut_s
{
    INT32U              id;
    INT32U              version;
    dvbsub_color_t      c_2b[4];
    dvbsub_color_t      c_4b[16];
    dvbsub_color_t      c_8b[256];

    struct dvbsub_clut_s *p_next;

} dvbsub_clut_t;

typedef struct dvbsub_display_s
{
    INT32U              version;

    INT32U              width;
    INT32U              height;

    INT32U              windowed;
    /* these values are only relevant if windowed */
    INT32S              x;
    INT32S              y;
    INT32S              max_x;
    INT32S              max_y;

} dvbsub_display_t;

/* The object definition gives the position of the object in a region */
typedef struct dvbsub_objectdef_s
{
    INT32U                  id;
    INT32U                  type;
    INT32S                  left;
    INT32S                  top;
    INT32U                  fg_pc;
    INT32U                  bg_pc;

    INT32U                  length;
    INT16U                  *p_text;  /* for string of characters objects */

} dvbsub_objectdef_t;

/* The Region is an aera on the image
 * with a list of the object definitions associated and a CLUT */
typedef struct dvbsub_region_s
{
    INT32U                  id;
    INT32U                  version;
    INT32U                  width;
    INT32U                  height;
    INT32U                  level_comp;
    INT32U                  depth;
    INT32U                  clut;
    INT32U                  background;

    INT8U                   *p_pixbuf;

    INT32U                  object_defs;
    dvbsub_objectdef_t      *p_object_defs;

    struct dvbsub_region_s  *p_next;

} dvbsub_region_t;

/* The object definition gives the position of the object in a region */
typedef struct dvbsub_regiondef_s
{
    INT32U                  id;
    INT32S                  left;
    INT32S                  top;

} dvbsub_regiondef_t;

/* The page defines the list of regions */
typedef struct dvbsub_page_s
{
    INT32U                  id;
    INT32U                  timeout;
    INT32U                  state;
    INT32U                  version;

    INT32U                  region_defs;
    dvbsub_regiondef_t      *p_region_defs;

} dvbsub_page_t;

/* The dvb subtitle sys */
typedef struct dvbsub_sys_s
{
    bs_t                    bs;

    INT64U                  pts;

    INT32U                  page_flag;

    dvbsub_page_t           *p_page;
    dvbsub_region_t         *p_regions;
    dvbsub_clut_t           *p_cluts;

    /* this is very small, so keep forever */
    dvbsub_display_t        display;
    dvbsub_clut_t           default_clut;

} dvbsub_sys_t;

/* The dvb subtitle decoder */
typedef struct dvbsub_decoder_s
{
    INT32U                  composition_id;
    INT32U                  ancillary_id;

    dvbsub_sys_t            *p_sys;
    dvbsub_picture_t        *p_head;

    dvbsub_callback_t        callback;
} dvbsub_decoder_t;


static void sub_parse_segment(dvbsub_decoder_t* decoder, bs_t* s);
static void sub_parse_page(dvbsub_decoder_t* decoder, bs_t* s);
static void sub_parse_region(dvbsub_decoder_t* decoder, bs_t* s);
static void sub_parse_object(dvbsub_decoder_t* decoder, bs_t* s);
static void sub_parse_clut(dvbsub_decoder_t* decoder, bs_t* s);
static void sub_parse_display(dvbsub_decoder_t* decoder, bs_t* s);

static void sub_parse_pdata(dvbsub_decoder_t* decoder, dvbsub_region_t* region,
                            INT32S x, INT32S y, INT8U *p_field, INT32S field, INT8U non_mod);

static void sub_pdata_2bpp(bs_t* s, INT8U *p, INT32U width, INT32S *p_off, INT8U non_mod, INT8U *map_table);
static void sub_pdata_4bpp(bs_t* s, INT8U *p, INT32U width, INT32S *p_off, INT8U non_mod, INT8U *map_table);
static void sub_pdata_8bpp(bs_t* s, INT8U *p, INT32U width, INT32S *p_off, INT8U non_mod, INT8U *map_table);

static void sub_free_all(dvbsub_decoder_t* decoder);
static void sub_default_clut_init(dvbsub_decoder_t* decoder);
static void sub_default_display_init(dvbsub_decoder_t* decoder);
static void sub_update_display(dvbsub_decoder_t* decoder);
static void sub_remove_display(dvbsub_decoder_t* decoder, dvbsub_picture_t* pic);

//static dvbsub_decoder_t *sub_decoder = NULL;

static void sub_parse_segment(dvbsub_decoder_t* decoder, bs_t* s)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    INT8U  segment_type = 0;
    INT16U page_id = 0;
    INT16U segment_length = 0;

    /* skip sync_byte (already checked) */
    bs_skip(s, 8 );

    /* segment type */
    segment_type = bs_read(s, 8);

    /* page id */
    page_id = bs_read(s, 16);

    /* segment length */
    segment_length = bs_show(s, 16);

    /* check page id */
    if ( (page_id != decoder->composition_id) && \
         (page_id != decoder->ancillary_id))
    {
        dvbsub_dbg("[sub_parse_segment] subtitle skipped page_id(%d) !\r\n", page_id);
        bs_skip(s,  8 * (2 + segment_length));
        return;
    }

    if ( (decoder->ancillary_id != decoder->composition_id) && \
         (decoder->ancillary_id == page_id) && \
         (segment_type == DVBSUB_ST_PAGE_COMPOSITION))
    {
        dvbsub_dbg("[sub_parse_segment] skipped invalid ancillary subtitle packet !\r\n");
        bs_skip(s,  8 * (2 + segment_length));
        return;
    }

    dvbsub_dbg("[sub_parse_segment] %s segment (page id: %d)\r\n", (page_id == decoder->composition_id) ? ("composition") : ("ancillary"), page_id);

    switch (segment_type)
    {
        case DVBSUB_ST_PAGE_COMPOSITION:
            dvbsub_dbg("[sub_parse_segment] parse page composition\r\n");
            sub_parse_page(decoder, s);
            break;

        case DVBSUB_ST_REGION_COMPOSITION:
            dvbsub_dbg("[sub_parse_segment] parse region composition\r\n");
            sub_parse_region(decoder, s);
            break;

        case DVBSUB_ST_CLUT_DEFINITION:
            dvbsub_dbg("[sub_parse_segment] parse clut definition\r\n");
            sub_parse_clut(decoder, s);
            break;

        case DVBSUB_ST_OBJECT_DATA:
            dvbsub_dbg("[sub_parse_segment] parse object data\r\n");
            sub_parse_object(decoder, s);
            break;

        case DVBSUB_ST_DISPLAY_DEFINITION:
            dvbsub_dbg("[sub_parse_segment] display definition\r\n");
            sub_parse_display(decoder, s);
            break;

        case DVBSUB_ST_ENDOFDISPLAY:
            dvbsub_dbg("[sub_parse_segment] end of display\r\n");
            bs_skip(s,  8 * (2 + segment_length));
            break;

        case DVBSUB_ST_STUFFING:
            dvbsub_dbg("[sub_parse_segment] stuffing\r\n");
            bs_skip(s,  8 * (2 + segment_length));
            break;

        default:
            dvbsub_dbg("[sub_parse_segment] unsupported segment type: 0x%02x !\r\n", segment_type);
            bs_skip(s,  8 * (2 + segment_length));
            break;
    }

    return;
}

static void sub_parse_page(dvbsub_decoder_t* decoder, bs_t* s)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    INT16U segment_length = 0;
    INT8U timeout = 0;
    INT8U version = 0xff;
    INT8U state = 0;

    /* segment length */
    segment_length = bs_read(s, 16);

    /* page time out */
    timeout = bs_read(s, 8);

    /* page version number */
    version = bs_read(s, 4);

    /* page state */
    state = bs_read(s, 2);

    /* reserved */
    bs_skip(s, 2);

    /* check state mode change */
    if (state == DVBSUB_PCS_STATE_MODE_CHANGE)
    {
        /* end of an epoch, reset decoder buffer */
        dvbsub_dbg("[sub_parse_page] page composition mode change\r\n");
        sub_free_all(decoder);
    }

    /* check id and version number */
    if (p_sys->p_page && (p_sys->p_page->version == version))
    {
        bs_skip(s,  8 * (segment_length - 2));
        return;
    }
    else if (p_sys->p_page)
    {
        dvbsub_dbg("[sub_parse_page] page version change\r\n");
        if (p_sys->p_page->p_region_defs)
        {
            free(p_sys->p_page->p_region_defs);
        }

        p_sys->p_page->p_region_defs = NULL;
        p_sys->p_page->region_defs = 0;
    }

    if (!p_sys->p_page)
    {
        dvbsub_dbg("[sub_parse_page] new page\r\n");

        /* allocate a new page */
        p_sys->p_page = (dvbsub_page_t*)calloc(1, sizeof(dvbsub_page_t));
        if (!p_sys->p_page)
        {
            dvbsub_dbg("[sub_parse_page] out of memory !\r\n");
            bs_skip(s,  8 * (segment_length - 2));
            return;
        }
    }

    p_sys->p_page->id = decoder->composition_id;
    p_sys->p_page->timeout = (timeout == 0) ? (5) : (timeout);
    p_sys->p_page->version = version;
    p_sys->p_page->state = state;

    p_sys->page_flag = 1;

    /* number of regions */
    p_sys->p_page->region_defs = (segment_length - 2) / 6;

    if (p_sys->p_page->region_defs)
    {
        p_sys->p_page->p_region_defs = (dvbsub_regiondef_t*)malloc(p_sys->p_page->region_defs * sizeof(dvbsub_regiondef_t));
        if (p_sys->p_page->p_region_defs)
        {
            INT32U i = 0;

            for (i = 0; i < p_sys->p_page->region_defs; i++)
            {
                /* region id */
                p_sys->p_page->p_region_defs[i].id = bs_read(s, 8);

                /* reserved */
                bs_skip(s, 8);

                /* region horizontal address */
                p_sys->p_page->p_region_defs[i].left = bs_read(s, 16);

                /* region vertical address */
                p_sys->p_page->p_region_defs[i].top = bs_read(s, 16);

                dvbsub_dbg("[sub_parse_page] page composition region %d (id: %d left: %d top: %d)\r\n", \
                           i, \
                           p_sys->p_page->p_region_defs[i].id, \
                           p_sys->p_page->p_region_defs[i].left, \
                           p_sys->p_page->p_region_defs[i].top);
            }
        }
        else
        {
            dvbsub_dbg("[sub_parse_page] out of memory !\r\n");
            bs_skip(s,  8 * (segment_length - 2));
        }
    }

    return ;
}

static void sub_parse_region(dvbsub_decoder_t* decoder, bs_t* s)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    dvbsub_region_t *p_region, **pp_region = &p_sys->p_regions;
    INT16U segment_length = 0, processed_length = 0;
    INT16U width = 0, height = 0;
    INT8U id = 0, version = 0;
    INT8U fill_flag = 0, level_comp = 0;
    INT8U depth = 0, clut = 0;
    INT8U bg_8b = 0, bg_4b = 0, bg_2b = 0;

    /* segment length */
    segment_length = bs_read(s, 16);

    /* region id */
    id = bs_read(s, 8);

    /* region version number */
    version = bs_read(s, 4);

    /* check if we already have this region */
    for (p_region = p_sys->p_regions; p_region != NULL; p_region = p_region->p_next)
    {
        pp_region = &p_region->p_next;
        if (p_region->id == id)
        {
            break;
        }
    }

    /* check version number */
    if (p_region && (p_region->version == version))
    {
        bs_skip(s, 8 * (segment_length - 1) - 4);
        return;
    }

    if (!p_region)
    {
        dvbsub_dbg("[sub_parse_region] new region: %d\r\n", id);

        p_region = *pp_region = (dvbsub_region_t*)calloc(1, sizeof(dvbsub_region_t));
        if (!p_region)
        {
            dvbsub_dbg("[sub_parse_region] out of memory !\r\n");
            bs_skip(s, 8 * (segment_length - 1) - 4);
            return;
        }

        p_region->object_defs = 0;
        p_region->p_pixbuf = NULL;
        p_region->p_object_defs = NULL;
        p_region->p_pixbuf = NULL;
        p_region->p_next = NULL;
    }

    /* region attributes */
    p_region->id = id;
    p_region->version = version;

    /* region fill flag */
    fill_flag = bs_read(s, 1);

    /* reserved */
    bs_skip(s, 3);

    /* region width */
    width = bs_read(s, 16);

    /* region height */
    height = bs_read(s, 16);

    /* region level of compatibility */
    level_comp = bs_read(s, 3);

    /* region depth */
    depth = bs_read(s, 3);

    /* reserved */
    bs_skip(s, 2);

    /* clut id */
    clut = bs_read(s, 8);

    /* region pixel code background colour */
    bg_8b = bs_read(s, 8);
    bg_4b = bs_read(s, 4);
    bg_2b = bs_read(s, 2);

    /* reserved */
    bs_skip(s, 2);

    /* free old object defs */
    while (p_region->object_defs)
    {
        --p_region->object_defs;

        if (p_region->p_object_defs[p_region->object_defs].p_text)
        {
            free(p_region->p_object_defs[p_region->object_defs].p_text);
            p_region->p_object_defs[p_region->object_defs].p_text = NULL;
        }
    }

    if (p_region->p_object_defs)
    {
        free(p_region->p_object_defs);
    }

    p_region->p_object_defs = NULL;
    p_region->object_defs = 0;

    /* extra sanity checks */
    if ( (p_region->width != width) || \
         (p_region->height != height))
    {
        if (p_region->p_pixbuf)
        {
            dvbsub_dbg("[sub_parse_region] region size changed (%dx%d -> %dx%d)\r\n",
                       p_region->width, p_region->height, width, height);

            free(p_region->p_pixbuf);
            p_region->p_pixbuf = NULL;
        }

        p_region->p_pixbuf = calloc(height, width);

        if (!p_region->p_pixbuf)
        {
            dvbsub_dbg("[sub_parse_region] out of memory !\r\n");
            bs_skip(s, 8 * (segment_length - 10));
            return;
        }

        p_region->depth = 0;
        fill_flag = 1;
    }

    /* region attributes */
    p_region->width = width;
    p_region->height = height;
    p_region->level_comp = level_comp;
    p_region->depth = depth;
    p_region->clut = clut;

    /* erase background of region */
    if (fill_flag)
    {
        p_region->background = (p_region->depth == 1) ? (bg_2b) : ((p_region->depth == 2) ? (bg_4b) : (bg_8b));

        if (p_region->p_pixbuf)
        {
            memset(p_region->p_pixbuf, p_region->background, p_region->width * p_region->height);
        }
    }

    /* list of objects in the region */
    processed_length = 10;
    while (processed_length < segment_length)
    {
        dvbsub_objectdef_t *p_obj = NULL;

        /* create new object */
        p_region->object_defs++;
        p_region->p_object_defs = realloc(p_region->p_object_defs, sizeof(dvbsub_objectdef_t) * p_region->object_defs);

        if (p_region->p_object_defs)
        {
            /* parse object properties */
            p_obj = &p_region->p_object_defs[p_region->object_defs - 1];

            /* object id */
            p_obj->id = bs_read(s, 16);

            /* object type */
            p_obj->type = bs_read(s, 2);

            /* object provider flag */
            bs_skip(s, 2);

            /* object horizontal position */
            p_obj->left = bs_read(s, 12);

            /* reserved */
            bs_skip(s, 4);

            /* object vertical position */
            p_obj->top = bs_read(s, 12);

            /* set object text */
            p_obj->length = 0;
            p_obj->p_text = NULL;

            processed_length += 6;

            if ( (p_obj->type == DVBSUB_OT_BASIC_CHARACTER) || \
                 (p_obj->type == DVBSUB_OT_COMPOSITE_STRING))
            {
                /* foreground pixel code */
                p_obj->fg_pc = bs_read(s, 8);

                /* background pixel code */
                p_obj->bg_pc = bs_read(s, 8);

                processed_length += 2;
            }
        }
        else
        {
            dvbsub_dbg("[sub_parse_region] out of memory !\r\n");
            p_region->object_defs = 0;
            bs_skip(s, 8 * (segment_length - processed_length));
            break;
        }
    }

    return ;
}

static void sub_parse_object(dvbsub_decoder_t* decoder, bs_t* s)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    dvbsub_region_t *p_region = NULL;
    INT16U segment_length = 0, id = 0;
    INT8U version = 0;
    INT8U coding_method = 0;
    INT8U non_modify_color = 0;
    INT32U i = 0;

    /* segment length */
    segment_length = bs_read(s, 16);

    /* object id */
    id = bs_read(s, 16);

    /* object version number */
    version = bs_read(s, 4);

    /* object coding method */
    coding_method = bs_read(s, 2);

    /* non modifying colour flag */
    non_modify_color = bs_read(s, 1);

    /* reserved */
    bs_skip(s, 1);

    if (coding_method > 1)
    {
        dvbsub_dbg("[sub_parse_object] unknown DVB subtitling coding %d is not handled !\r\n", coding_method);
        bs_skip(s, 8 * (segment_length - 3));
        return;
    }

    /* check if the object needs to be rendered in at least one
     * of the regions */
    for (p_region = p_sys->p_regions; p_region != NULL; p_region = p_region->p_next)
    {
        for (i = 0; i < p_region->object_defs; i++)
        {
            if (p_region->p_object_defs[i].id == id)
            {
                break;
            }
        }

        if (i != p_region->object_defs)
        {
            break;
        }
    }

    if (!p_region)
    {
        bs_skip(s, 8 * (segment_length - 3));
        return;
    }

    dvbsub_dbg("[sub_parse_object] new object: %d\r\n", id);

    if (coding_method == 0x00)
    {
        INT16U topfield = 0, bottomfield = 0;
        INT8U *p_topfield = NULL, *p_bottomfield = NULL;

        topfield        = bs_read(s, 16);
        bottomfield     = bs_read(s, 16);

        p_topfield      = s->p_start + bs_pos(s) / 8;
        p_bottomfield   = p_topfield + topfield;

        bs_skip(s, 8 * (segment_length - 7));

        /* sanity check */
        if ((segment_length < (topfield + bottomfield + 7)) || \
            (( p_topfield + topfield + bottomfield) > s->p_end))
        {
            dvbsub_dbg("[sub_parse_object] corrupted object data !\r\n" );
            return;
        }

        for (p_region = p_sys->p_regions; p_region != NULL; p_region = p_region->p_next)
        {
            for (i = 0; i < p_region->object_defs; i++)
            {
                if (p_region->p_object_defs[i].id != id)
                {
                    continue;
                }

                sub_parse_pdata(decoder,
                                p_region,
                                p_region->p_object_defs[i].left,
                                p_region->p_object_defs[i].top,
                                p_topfield,
                                topfield,
                                non_modify_color);

                if (bottomfield)
                {
                    sub_parse_pdata(decoder,
                                    p_region,
                                    p_region->p_object_defs[i].left,
                                    p_region->p_object_defs[i].top + 1,
                                    p_bottomfield,
                                    bottomfield,
                                    non_modify_color);
                }
                else
                {
                    /* duplicate the top field */
                    sub_parse_pdata(decoder,
                                    p_region,
                                    p_region->p_object_defs[i].left,
                                    p_region->p_object_defs[i].top + 1,
                                    p_topfield,
                                    topfield,
                                    non_modify_color);
                }
            }
        }
    }
    else if (coding_method == 0x01)
    {
        /* DVB subtitling as characters */

        INT8U number_of_codes = bs_read(s, 8);
        INT8U *p_start = s->p_start + bs_pos(s) / 8;
        bs_t bs = {NULL,NULL,NULL,0};

        bs_skip(s, 8 * (segment_length - 4));

        /* sanity check */
        if ((segment_length < (number_of_codes * 2 + 4)) || \
            ((p_start + number_of_codes * 2) > s->p_end))
        {
            dvbsub_dbg("[sub_parse_object] corrupted object data !\r\n");
            bs_skip(s, 8 * (segment_length - 4));
            return;
        }

        if (number_of_codes)
        {
            for (p_region = p_sys->p_regions; p_region != NULL; p_region = p_region->p_next)
            {
                for (i = 0; i < p_region->object_defs; i++)
                {
                    INT32U j = 0;

                    if (p_region->p_object_defs[i].id != id)
                    {
                        continue;
                    }

                    p_region->p_object_defs[i].p_text = realloc(p_region->p_object_defs[i].p_text, (p_region->p_object_defs[i].length + number_of_codes) * 2);
                    if (p_region->p_object_defs[i].p_text)
                    {
                        bs_init(&bs, p_start, number_of_codes * 2);

                        p_region->p_object_defs[i].length += number_of_codes;

                        for (j = 0; j < number_of_codes; j++)
                        {
                            p_region->p_object_defs[i].p_text[j] = bs_read(&bs, 16);
                        }
                    }
                    else
                    {
                        p_region->p_object_defs[i].length = 0;
                        dvbsub_dbg("[sub_parse_object] out of memory !\r\n");
                        bs_skip(s, 8 * (segment_length - 4));
                        return;
                    }
                }
            }
        }
    }

    return;
}

static void sub_parse_clut(dvbsub_decoder_t* decoder, bs_t* s)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    dvbsub_clut_t *p_clut = NULL, *p_next = NULL;
    INT16U segment_length = 0, processed_length = 0;
    INT8U id = 0, version = 0;

    /* segment length */
    segment_length = bs_read(s, 16);

    /* CLUT id */
    id = bs_read(s, 8);

    /* CLUT version number */
    version = bs_read(s, 4);

    /* check if we already have this clut */
    for (p_clut = p_sys->p_cluts; p_clut != NULL; p_clut = p_clut->p_next)
    {
        if (p_clut->id == id)
        {
            break;
        }
    }

    /* check version number */
    if (p_clut && (p_clut->version == version))
    {
        /* nothing to do */
        bs_skip(s, 8 * segment_length - 12);
        return;
    }

    if (!p_clut)
    {
        dvbsub_dbg("[sub_parse_clut] new clut: %d\r\n", id);

        p_clut = calloc(1, sizeof( dvbsub_clut_t));
        if (!p_clut)
        {
            dvbsub_dbg("[sub_parse_clut] out of memory !\r\n");
            bs_skip(s, 8 * segment_length - 12);
            return;
        }

        p_clut->p_next = p_sys->p_cluts;
        p_sys->p_cluts = p_clut;
    }

    /* initialize to default clut */
    p_next = p_clut->p_next;
    *p_clut = p_sys->default_clut;
    p_clut->p_next = p_next;

    /* set version and id */
    p_clut->version = version;
    p_clut->id = id;

    /* reserved */
    bs_skip(s, 4);

    processed_length = 2;
    while (processed_length < segment_length)
    {
        INT8U id = 0;
        INT8U type = 0;
        INT8U y = 0, cb = 0, cr = 0, t = 0;

        id = bs_read(s, 8);
        type = bs_read(s, 3);

        /* reserved */
        bs_skip(s, 4);

        /* full range flag */
        if (bs_read(s, 1))
        {
            y  = bs_read(s, 8);
            cr = bs_read(s, 8);
            cb = bs_read(s, 8);
            t  = bs_read(s, 8);

            processed_length += 6;
        }
        else
        {
            y  = bs_read(s, 6) << 2;
            cr = bs_read(s, 4) << 4;
            cb = bs_read(s, 4) << 4;
            t  = bs_read(s, 2) << 6;

            processed_length += 4;
        }

        /* We are not entirely compliant here as full transparency is indicated
         * with a luma value of zero, not a transparency value of 0xff
         * (full transparency would actually be 0xff + 1). */
        if (y == 0)
        {
            cr = cb = 0;
            t  = 0xff;
        }

        /* According to EN 300-743 section 7.2.3 note 1, type should
         * not have more than 1 bit set to one, but some streams don't
         * respect this note. */
        if ((type & 0x04) && (id < 4))
        {
            p_clut->c_2b[id].Y  = y;
            p_clut->c_2b[id].Cr = cr;
            p_clut->c_2b[id].Cb = cb;
            p_clut->c_2b[id].T  = t;
        }

        if ((type & 0x02) && (id < 16))
        {
            p_clut->c_4b[id].Y  = y;
            p_clut->c_4b[id].Cr = cr;
            p_clut->c_4b[id].Cb = cb;
            p_clut->c_4b[id].T  = t;
        }

        if (type & 0x01)
        {
            p_clut->c_8b[id].Y  = y;
            p_clut->c_8b[id].Cr = cr;
            p_clut->c_8b[id].Cb = cb;
            p_clut->c_8b[id].T  = t;
        }
    }

    return;
}

static void sub_parse_display(dvbsub_decoder_t* decoder, bs_t* s)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    INT16U segment_length = 0, processed_length = 5;
    INT8U version = 0;

    /* segment length */
    segment_length = bs_read(s, 16);

    /* dds version number */
    version = bs_read(s, 4);

    /* check version number */
    if (p_sys->display.version == version)
    {
        /* the definition did not change */
        bs_skip( s, 8 * segment_length - 4);
        return;
    }

    dvbsub_dbg("[sub_parse_display] new display definition: %d\r\n", version );

    p_sys->display.version = version;

    /* display window flag */
    p_sys->display.windowed = bs_read(s, 1);

    /* reserved */
    bs_skip(s, 3);

    /* display width */
    p_sys->display.width = bs_read(s, 16) + 1;

    /* display height */
    p_sys->display.height = bs_read(s, 16) + 1;

    if (p_sys->display.windowed)
    {
        dvbsub_dbg("[sub_parse_display] display definition with offsets (windowed)\r\n");

        /* coordinates are measured from the top left corner */
        p_sys->display.x        = bs_read(s, 16);
        p_sys->display.max_x    = bs_read(s, 16);
        p_sys->display.y        = bs_read(s, 16);
        p_sys->display.max_y    = bs_read(s, 16);

        processed_length += 8;
    }
    else
    {
        /* if not windowed, setup the window variables to good defaults */
        /* not necessary, but to avoid future confusion.. */
        p_sys->display.x        = 0;
        p_sys->display.max_x    = p_sys->display.width - 1;
        p_sys->display.y        = 0;
        p_sys->display.max_y    = p_sys->display.height - 1;
    }

    if (processed_length != segment_length)
    {
        bs_skip(s, segment_length - processed_length);
    }

    dvbsub_dbg("[sub_parse_display] version: %d, width: %d, height: %d\r\n",
               p_sys->display.version, p_sys->display.width, p_sys->display.height );

    if (p_sys->display.windowed)
    {
        dvbsub_dbg("[sub_parse_display] xmin: %d, xmax: %d, ymin: %d, ymax: %d\r\n",
                   p_sys->display.x, p_sys->display.max_x, p_sys->display.y, p_sys->display.max_y);
    }

    return;
}

static void sub_parse_pdata(dvbsub_decoder_t* decoder, dvbsub_region_t* region,
                            INT32S x, INT32S y, INT8U *p_field, INT32S field, INT8U non_mod)
{
    dvbsub_region_t* p_region = region;
    INT8U *p_pixbuf = NULL;
    INT8U *map_table = NULL;
    INT8U  map2to4_table[] = {0x0,  0x7,  0x8,  0xf};
    INT8U  map2to8_table[] = {0x00, 0x77, 0x88, 0xff};
    INT8U  map4to8_table[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                              0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
                             };
    INT32S offset = 0;
    INT32U i = 0;
    bs_t bs = {NULL,NULL,NULL,0};

    UNUSED(decoder);

    /* sanity check */
    if (!p_region->p_pixbuf)
    {
        dvbsub_dbg("[sub_parse_pdata] region %d has no pixel buffer !\r\n", p_region->id);
        return;
    }

    if (y < 0 || x < 0 || y >= (INT32S)p_region->height || x >= (INT32S)p_region->width)
    {
        dvbsub_dbg("[sub_parse_pdata] invalid offset (%d, %d) !\r\n", x, y);
        return;
    }

    p_pixbuf = p_region->p_pixbuf + y * p_region->width;
    bs_init(&bs, p_field, field);

    while (!bs_eof(&bs))
    {
        /* sanity check */
        if (y >= (INT32S)p_region->height)
        {
            return;
        }

        switch (bs_read(&bs, 8))
        {
            case DVBSUB_DT_2BP_CODE_STRING:
                if (region->depth == 3)
                {
                    map_table = map2to8_table;
                }
                else if (region->depth == 2)
                {
                    map_table = map2to4_table;
                }
                else
                {
                    map_table = NULL;
                }
                sub_pdata_2bpp(&bs, p_pixbuf + x, p_region->width - x, &offset, non_mod, map_table);
                break;

            case DVBSUB_DT_4BP_CODE_STRING:
                if (region->depth == 3)
                {
                    map_table = map4to8_table;
                }
                else
                {
                    map_table = NULL;
                }
                sub_pdata_4bpp(&bs, p_pixbuf + x, p_region->width - x, &offset, non_mod, map_table);
                break;

            case DVBSUB_DT_8BP_CODE_STRING:
                map_table = NULL;
                sub_pdata_8bpp(&bs, p_pixbuf + x, p_region->width - x, &offset, non_mod, map_table);
                break;

            case DVBSUB_DT_24_MAP_TABLE_DATA:
                for (i = 0; i < 4; i++)
                {
                    map2to4_table[i] = bs_read(&bs, 4);
                }
                break;
            case DVBSUB_DT_28_MAP_TABLE_DATA:
                for (i = 0; i < 4; i++)
                {
                    map2to8_table[i] = bs_read(&bs, 8);
                }
                break;
            case DVBSUB_DT_48_MAP_TABLE_DATA:
                for (i = 0; i < 16; i++)
                {
                    map4to8_table[i] = bs_read(&bs, 8);
                }
                break;

            case DVBSUB_DT_END_OF_OBJECT_LINE:
                p_pixbuf += 2 * p_region->width;
                offset = 0;
                y += 2;
                break;
        }
    }

    return;
}

static void sub_pdata_2bpp(bs_t* s, INT8U *p, INT32U width, INT32S *p_off, INT8U non_mod, INT8U *map_table)
{
    INT8U stop = 0;

    while (!stop && !bs_eof(s))
    {
        INT32U count = 0;
        INT8U  color = 0;
        INT8U  no_modify = 0;

        color = bs_read(s, 2);
        if (color != 0x00)
        {
            count = 1;

            if ((non_mod == 1) && (color == 1))
            {
                no_modify = 1;
            }
            else if (map_table)
            {
                color = map_table[color];
            }
        }
        else
        {
            if (bs_read(s, 1) == 0x01)        // Switch1
            {
                count = 3 + bs_read(s, 3);
                color = bs_read(s, 2);

                if ((non_mod == 1) && (color == 1))
                {
                    no_modify = 1;
                }
                else if (map_table)
                {
                    color = map_table[color];
                }
            }
            else
            {
                if (bs_read(s, 1) == 0x00)    //Switch2
                {
                    switch (bs_read(s, 2))    //Switch3
                    {
                        case 0x00:
                            stop = 1;
                            break;
                        case 0x01:
                            count = 2;

                            if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                        case 0x02:
                            count =  12 + bs_read(s, 4);
                            color = bs_read(s, 2);

                            if ((non_mod == 1) && (color == 1))
                            {
                                no_modify = 1;
                            }
                            else if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                        case 0x03:
                            count =  29 + bs_read(s, 8);
                            color = bs_read(s, 2);

                            if ((non_mod == 1) && (color == 1))
                            {
                                no_modify = 1;
                            }
                            else if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    /* 1 pixel color 0 */
                    count = 1;

                    if (map_table)
                    {
                        color = map_table[color];
                    }
                }
            }
        }

        if (!count)
        {
            continue;
        }

        /* sanity check */
        if (( count + *p_off) > width)
        {
            break;
        }

        if (!no_modify)
        {
            if (count == 1)
            {
                p[*p_off] = color;
            }
            else
            {
                memset((p + *p_off), color, count);
            }
        }

        (*p_off) += count;
    }

    bs_align(s);

    return;
}

static void sub_pdata_4bpp(bs_t* s, INT8U *p, INT32U width, INT32S *p_off, INT8U non_mod, INT8U *map_table)
{
    INT8U stop = 0;

    while (!stop && !bs_eof(s))
    {
        INT32U count = 0;
        INT8U  color = 0;
        INT8U  no_modify = 0;

        color = bs_read(s, 4);
        if (color != 0x00)
        {
            /* Add 1 pixel */
            count = 1;

            if ((non_mod == 1) && (color == 1))
            {
                no_modify = 1;
            }
            else if (map_table)
            {
                color = map_table[color];
            }
        }
        else
        {
            if (bs_read(s, 1) == 0x00)          // Switch1
            {
                if (bs_show(s, 3) != 0x00)
                {
                    count = 2 + bs_read(s, 3);

                    if (map_table)
                    {
                        color = map_table[color];
                    }
                }
                else
                {
                    bs_skip(s, 3);
                    stop = 1;
                }
            }
            else
            {
                if (bs_read(s, 1) == 0x00)       //Switch2
                {
                    count =  4 + bs_read(s, 2);
                    color = bs_read(s, 4);

                    if ((non_mod == 1) && (color == 1))
                    {
                        no_modify = 1;
                    }
                    else if (map_table)
                    {
                        color = map_table[color];
                    }
                }
                else
                {
                    switch (bs_read(s, 2))     //Switch3
                    {
                        case 0x0:
                            count = 1;

                            if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                        case 0x1:
                            count = 2;

                            if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                        case 0x2:
                            count = 9 + bs_read(s, 4);
                            color = bs_read(s, 4);

                            if ((non_mod == 1) && (color == 1))
                            {
                                no_modify = 1;
                            }
                            else if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                        case 0x3:
                            count = 25 + bs_read(s, 8);
                            color = bs_read(s, 4);

                            if ((non_mod == 1) && (color == 1))
                            {
                                no_modify = 1;
                            }
                            else if (map_table)
                            {
                                color = map_table[color];
                            }
                            break;
                    }
                }
            }
        }

        if (!count)
        {
            continue;
        }

        /* sanity check */
        if ((count + *p_off) > width)
        {
            break;
        }

        if (!no_modify)
        {
            if (count == 1)
            {
                p[*p_off] = color;
            }
            else
            {
                memset(( p + *p_off), color, count);
            }
        }

        (*p_off) += count;
    }

    bs_align( s );
}

static void sub_pdata_8bpp(bs_t* s, INT8U *p, INT32U width, INT32S *p_off, INT8U non_mod, INT8U *map_table)
{
    INT8U stop = 0;

    while (!stop && !bs_eof(s))
    {
        INT32U count = 0;
        INT8U  color = 0;
        INT8U  no_modify = 0;

        color = bs_read(s, 8);
        if (color != 0x00)
        {
            /* Add 1 pixel */
            count = 1;

            if ((non_mod == 1) && (color == 1))
            {
                no_modify = 1;
            }
            else if (map_table)
            {
                color = map_table[color];
            }
        }
        else
        {
            if (bs_read(s, 1) == 0x00)          // Switch1
            {
                if (bs_show(s, 7) != 0x00)
                {
                    count = bs_read(s, 7);

                    if (map_table)
                    {
                        color = map_table[color];
                    }
                }
                else
                {
                    bs_skip(s, 7);
                    stop = 1;
                }
            }
            else
            {
                count = bs_read(s, 7);
                color = bs_read(s, 8);

                if ((non_mod == 1) && (color == 1))
                {
                    no_modify = 1;
                }
                else if (map_table)
                {
                    color = map_table[color];
                }
            }
        }

        if (!count)
        {
            continue;
        }

        /* sanity check */
        if ((count + *p_off) > width)
        {
            break;
        }

        if (!no_modify)
        {
            if (count == 1)
            {
                p[*p_off] = color;
            }
            else
            {
                memset((p + *p_off), color, count);
            }
        }

        (*p_off) += count;
    }

    bs_align(s);

    return;
}

static void sub_free_all(dvbsub_decoder_t* decoder)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    dvbsub_region_t *p_reg = NULL, *p_reg_next = NULL;
    dvbsub_clut_t *p_clut = NULL, *p_clut_next = NULL;

    for (p_clut = p_sys->p_cluts; p_clut != NULL; p_clut = p_clut_next)
    {
        p_clut_next = p_clut->p_next;
        free(p_clut);
    }

    p_sys->p_cluts = NULL;

    for (p_reg = p_sys->p_regions; p_reg != NULL; p_reg = p_reg_next)
    {
        INT32U i = 0;

        p_reg_next = p_reg->p_next;
        for (i = 0; i < p_reg->object_defs; i++)
        {
            if (p_reg->p_object_defs[i].p_text)
            {
                free(p_reg->p_object_defs[i].p_text);
            }
        }

        if (p_reg->p_object_defs)
        {
            free(p_reg->p_object_defs);
        }

        if (p_reg->p_pixbuf)
        {
            free(p_reg->p_pixbuf);
        }

        free(p_reg);
    }

    p_sys->p_regions = NULL;

    if (p_sys->p_page)
    {
        if (p_sys->p_page->p_region_defs)
        {
            free(p_sys->p_page->p_region_defs);
        }

        free(p_sys->p_page);
    }

    p_sys->p_page = NULL;

    return;
}

static void sub_default_clut_init(dvbsub_decoder_t* decoder)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    INT32U i = 0;

    p_sys->default_clut.id = (INT32U) - 1;
    p_sys->default_clut.version = (INT32U) - 1;
    p_sys->default_clut.p_next = NULL;

    /* 2bit CLUT */
    for (i = 0; i < 4; i++)
    {
        p_sys->default_clut.c_2b[i].Y   = default_2b_CLUT_YCbCrT[i][0];
        p_sys->default_clut.c_2b[i].Cb  = default_2b_CLUT_YCbCrT[i][1];
        p_sys->default_clut.c_2b[i].Cr	= default_2b_CLUT_YCbCrT[i][2];
        p_sys->default_clut.c_2b[i].T   = default_2b_CLUT_YCbCrT[i][3];
    }

    /* 4bit CLUT */
    for (i = 0; i < 16; i++)
    {
        p_sys->default_clut.c_4b[i].Y   = default_4b_CLUT_YCbCrT[i][0];
        p_sys->default_clut.c_4b[i].Cb  = default_4b_CLUT_YCbCrT[i][1];
        p_sys->default_clut.c_4b[i].Cr	= default_4b_CLUT_YCbCrT[i][2];
        p_sys->default_clut.c_4b[i].T   = default_4b_CLUT_YCbCrT[i][3];
    }

    /* 8bit CLUT */
    for (i = 0; i < 16; i++)
    {
        p_sys->default_clut.c_8b[i].Y   = default_8b_CLUT_YCbCrT[i][0];
        p_sys->default_clut.c_8b[i].Cb  = default_8b_CLUT_YCbCrT[i][1];
        p_sys->default_clut.c_8b[i].Cr	= default_8b_CLUT_YCbCrT[i][2];
        p_sys->default_clut.c_8b[i].T   = default_8b_CLUT_YCbCrT[i][3];
    }

    return;
}

static void sub_default_display_init(dvbsub_decoder_t* decoder)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;

    memset(&p_sys->display, 0, sizeof(dvbsub_display_t));
    p_sys->display.version = (INT32U) - 1;
    p_sys->display.windowed = 0;

    p_sys->display.width = 720;
    p_sys->display.height= 576;

    return;
}

static void sub_update_display(dvbsub_decoder_t* decoder)
{
    dvbsub_sys_t *p_sys = decoder->p_sys;
    dvbsub_picture_t *p_pic = NULL, *p_pic_prev = NULL;
    sub_pic_region_t *p_pic_region = NULL, **pp_pic_region = NULL;
    INT32S base_x = 0, base_y = 0;
    INT32U i = 0, j = 0, timeout = 0;

    for (p_pic = decoder->p_head; p_pic != NULL; p_pic = p_pic->p_next)
    {
        p_pic_prev = p_pic;
    }

    /* new picture */
    p_pic = (dvbsub_picture_t*)calloc(1, sizeof(dvbsub_picture_t));
    if (!p_pic)
    {
        dvbsub_dbg("[sub_update_display] out of memory !\r\n");
        return;
    }

    /* add to picture list */
    p_pic->p_next = NULL;
    p_pic->p_prev = p_pic_prev;

    if (p_pic_prev)
    {
        p_pic_prev->p_next = p_pic;
    }
    else
    {
        decoder->p_head = p_pic;
    }

    /* picture attributes */
    p_pic->pts = p_sys->pts;
    p_pic->timeout = p_sys->p_page->timeout;
    p_pic->original_height = 0; // 720
    p_pic->original_width = 0;  // 576
    p_pic->original_x = 0;
    p_pic->original_y = 0;
    p_pic->p_region = NULL;

    /* set base x/y */
    base_x = p_pic->original_x;
    base_y = p_pic->original_y;

    /* set original display width/height */
    p_pic->original_width   = p_sys->display.width;
    p_pic->original_height  = p_sys->display.height;

    if (p_sys->display.windowed)
    {
        /* From en_300743v01 - */
        /* the DDS is there to indicate intended size/position of text */
        /* the intended video area is ->width/height */
        /* the window is within this... SO... we should leave original_picture_width etc. as is */
        /* and ONLY change base_x.  effectively max_x/y are only there to limit memory requirements*/
        /* we COULD use the upper limits to limit rendering to within these? */

        /* see notes on DDS at the top of the file */
        /*
        p_pic->original_x += p_sys->display.x;
        p_pic->original_y += p_sys->display.y;

        p_pic->original_width   = p_sys->display.max_x - p_sys->display.x;
        p_pic->original_height  = p_sys->display.max_y - p_sys->display.y;
        */
        base_x += p_sys->display.x;
        base_y += p_sys->display.y;
    }

    pp_pic_region = &p_pic->p_region;

    for (i = 0; p_sys->p_page && (i < p_sys->p_page->region_defs); i++)
    {
        dvbsub_region_t     *p_region = NULL;
        dvbsub_regiondef_t  *p_regiondef = NULL;
        dvbsub_clut_t       *p_clut = NULL;
        dvbsub_color_t      *p_color = NULL;
        dvbsub_objectdef_t  *p_object_def = NULL;

        p_regiondef = &p_sys->p_page->p_region_defs[i];

        /* find associated region */
        for (p_region = p_sys->p_regions; p_region != NULL; p_region = p_region->p_next)
        {
            if (p_regiondef->id == p_region->id )
            {
                break;
            }
        }

        if (!p_region)
        {
            dvbsub_dbg("[sub_update_display] region %d not found !\r\n", p_regiondef->id);
            continue;
        }

        /* find associated CLUT */
        for (p_clut = p_sys->p_cluts; p_clut != NULL; p_clut = p_clut->p_next)
        {
            if (p_region->clut == p_clut->id)
            {
                break;
            }
        }

        if (!p_clut)
        {
            dvbsub_dbg("[sub_update_display] clut %d not found use default !\r\n", p_region->clut);
            p_clut = &p_sys->default_clut;
            continue;
        }

        if (p_region->p_pixbuf)
        {
            /* new picture region */
            p_pic_region = (sub_pic_region_t *)calloc(1, sizeof(sub_pic_region_t));
            if (!p_pic_region)
            {
                dvbsub_dbg("[sub_update_display] out of memory !\r\n");

                sub_remove_display(decoder, p_pic);

                return;
            }

            /* add to region list */
            p_pic_region->p_next = NULL;
            *pp_pic_region = p_pic_region;
            pp_pic_region = &p_pic_region->p_next;

            /* picture region attributes */
            p_pic_region->left = base_x + p_regiondef->left;
            p_pic_region->top = base_y + p_regiondef->top;

            p_pic_region->width = p_region->width;
            p_pic_region->height = p_region->height;

            p_pic_region->entry = (p_region->depth == 1) ? 4 : ((p_region->depth == 2) ? 16 : 256);
            p_color = (p_region->depth == 1) ? p_clut->c_2b : ((p_region->depth == 2) ? p_clut->c_4b : p_clut->c_8b);

            for (j = 0; j < p_pic_region->entry; j++)
            {
                p_pic_region->clut[j].r  = YCbCr_TO_R(p_color[j].Y, p_color[j].Cb, p_color[j].Cr);
                p_pic_region->clut[j].g  = YCbCr_TO_G(p_color[j].Y, p_color[j].Cb, p_color[j].Cr);
                p_pic_region->clut[j].b  = YCbCr_TO_B(p_color[j].Y, p_color[j].Cb, p_color[j].Cr);
                p_pic_region->clut[j].a  = 0xff - p_color[j].T;
            }

            p_region->background = p_region->background;

            p_pic_region->p_buf = (INT8U*)calloc(p_region->width, p_region->height);
            if (!p_pic_region->p_buf)
            {
                dvbsub_dbg("[sub_update_display] out of memory !\r\n");

                sub_remove_display(decoder, p_pic);

                return;
            }

            memcpy(p_pic_region->p_buf, p_region->p_pixbuf, p_region->width * p_region->height);
        }

        /* check subtitles encoded as strings of characters */
        for (j = 0; j < p_region->object_defs; j++)
        {
            p_object_def = &p_region->p_object_defs[j];

            if ((p_object_def->type == DVBSUB_OT_BASIC_BITMAP) || (!p_object_def->p_text && p_object_def->length))
            {
                continue;
            }

            /* new picture region */
            p_pic_region = (sub_pic_region_t *)calloc(1, sizeof(sub_pic_region_t));
            if (!p_pic_region)
            {
                dvbsub_dbg("[sub_update_display] out of memory !\r\n");

                sub_remove_display(decoder, p_pic);

                return;
            }

            /* add to region list */
            p_pic_region->p_next = NULL;
            *pp_pic_region = p_pic_region;
            pp_pic_region = &p_pic_region->p_next;

            /* picture region attributes */
            p_pic_region->left = base_x + p_regiondef->left + p_object_def->left;;
            p_pic_region->top = base_y + p_regiondef->top + p_object_def->top;

            p_pic_region->width = p_region->width;
            p_pic_region->height = p_region->height;

            p_pic_region->entry = (p_region->depth == 1) ? 4 : ((p_region->depth == 2) ? 16 : 256);
            p_color = (p_region->depth == 1) ? p_clut->c_2b : ((p_region->depth == 2) ? p_clut->c_4b : p_clut->c_8b);

            for (j = 0; j < p_pic_region->entry; j++)
            {
                p_pic_region->clut[j].r  = YCbCr_TO_R(p_color[j].Y, p_color[j].Cb, p_color[j].Cr);
                p_pic_region->clut[j].g  = YCbCr_TO_G(p_color[j].Y, p_color[j].Cb, p_color[j].Cr);
                p_pic_region->clut[j].b  = YCbCr_TO_B(p_color[j].Y, p_color[j].Cb, p_color[j].Cr);
                p_pic_region->clut[j].a  = 0xff - p_color[j].T;
            }

            p_pic_region->fg = p_object_def->fg_pc;
            p_pic_region->bg = p_object_def->bg_pc;
            p_pic_region->length = p_object_def->length;
            p_pic_region->p_text = calloc(p_object_def->length + 1, sizeof(INT16U));
            if (!p_pic_region->p_text)
            {
                dvbsub_dbg("[sub_update_display] out of memory !\r\n");

                sub_remove_display(decoder, p_pic);

                return;
            }

            memcpy(p_pic_region->p_text, p_object_def->p_text, sizeof(INT16U) * p_object_def->length);
        }
    }

    dvbsub_dbg("[sub_update_display] callback update display\r\n");

    /* callback update display */
    if (decoder->callback)
    {
        decoder->callback((long)decoder, p_pic);
    }
    return;
}

static void sub_remove_display(dvbsub_decoder_t* decoder, dvbsub_picture_t* pic)
{
    dvbsub_picture_t *p_pic = NULL, *p_pic_next = NULL, *p_pic_prev = NULL;
    sub_pic_region_t *p_reg = NULL, *p_reg_next = NULL;

    for (p_pic = decoder->p_head; p_pic != NULL; p_pic = p_pic_next)
    {
        p_pic_prev = p_pic->p_prev;
        p_pic_next = p_pic->p_next;

        if (p_pic == pic)
        {
            if (p_pic_next)
            {
                p_pic_next->p_prev = p_pic_prev;
            }

            if (p_pic_prev)
            {
                p_pic_prev->p_next = p_pic_next;
            }
            else
            {
                decoder->p_head = p_pic_next;
            }

#ifndef WIN32
            dvbsub_dbg("[sub_remove_display] remove picture(pts: %llu timeout: %d)\r\n", p_pic->pts, p_pic->timeout);
#else
            dvbsub_dbg("[sub_remove_display] remove picture(pts: %I64d timeout: %d)\r\n", p_pic->pts, p_pic->timeout);
#endif
            for (p_reg = p_pic->p_region; p_reg != NULL; p_reg = p_reg_next)
            {
                p_reg_next = p_reg->p_next;

                if (p_reg->p_buf)
                {
                    free(p_reg->p_buf);
                    p_reg->p_buf = NULL;
                }

                if (p_reg->p_text)
                {
                    free(p_reg->p_text);
                    p_reg->p_text = NULL;
                }

                free(p_reg);
            }

            p_pic->p_region = NULL;

            free(p_pic);

            break;
        }
    }

    return;
}

INT32S dvbsub_decoder_create(INT16U composition_id, INT16U ancillary_id, dvbsub_callback_t callback, long* handle)
{
    dvbsub_decoder_t *sub_decoder = NULL;
    INT32S retcode = 0;

    if ((composition_id == 0xffff) || /*(callback == NULL) ||*/ (handle == NULL))
    {
        dvbsub_dbg("[dvbsub_decoder_create] bad parameter !\r\n");
        return -1;
    }

    //sub_mem_init();

    if (sub_decoder == NULL)
    {
        sub_decoder = (dvbsub_decoder_t*)calloc(1, sizeof(dvbsub_decoder_t));
        if (!sub_decoder)
        {
            dvbsub_dbg("[dvbsub_decoder_create] out of memory !\r\n");
            retcode = -1;
        }
        else
        {
            sub_decoder->p_sys = (dvbsub_sys_t*)calloc(1, sizeof(dvbsub_sys_t));
            if (!sub_decoder->p_sys)
            {
                dvbsub_dbg("[dvbsub_decoder_create] out of memory !\r\n");
                free(sub_decoder);
                sub_decoder = NULL;
                retcode = -1;
            }
            else
            {
                sub_decoder->p_sys->pts = (INT64U) - 1;
                sub_decoder->p_sys->page_flag = 0;
                sub_decoder->p_sys->p_page = NULL;
                sub_decoder->p_sys->p_regions = NULL;
                sub_decoder->p_sys->p_cluts = NULL;

                sub_decoder->composition_id = composition_id;
                sub_decoder->ancillary_id = ancillary_id;
                sub_decoder->callback = callback;

                sub_default_clut_init(sub_decoder);
                sub_default_display_init(sub_decoder);

                *handle = (long)sub_decoder;
            }
        }
    }
    else
    {
        *handle = (long)sub_decoder;
    }

    if (retcode == 0 && sub_decoder)
    {
        dvbsub_dbg("[dvbsub_decoder_create] decoder created (composition_id: %d ancillary_id: %d)\r\n", sub_decoder->composition_id, sub_decoder->ancillary_id);
    }

    return retcode;
}

INT32S dvbsub_decoder_destroy(long handle)
{
    INT32S retcode = 0;
    dvbsub_picture_t *p_pic = NULL, *p_pic_next = NULL;
    sub_pic_region_t *p_reg = NULL, *p_reg_next = NULL;
    dvbsub_decoder_t *sub_decoder = (dvbsub_decoder_t*)handle;

    if ((handle != (long)sub_decoder) || (handle == 0))
    {
        dvbsub_dbg("[dvbsub_decoder_destroy] invalid handle !\r\n");
        return -1;
    }

    sub_free_all(sub_decoder);
    free(sub_decoder->p_sys);
    sub_decoder->p_sys = NULL;

    for (p_pic = sub_decoder->p_head; p_pic != NULL; p_pic = p_pic_next)
    {
        p_pic_next = p_pic->p_next;

        for (p_reg = p_pic->p_region; p_reg != NULL; p_reg = p_reg_next)
        {
            p_reg_next = p_reg->p_next;

            if (p_reg->p_buf)
            {
                free(p_reg->p_buf);
                p_reg->p_buf = NULL;
            }

            if (p_reg->p_text)
            {
                free(p_reg->p_text);
                p_reg->p_text = NULL;
            }
        }

        p_pic->p_region = NULL;

        free(p_pic);
    }

    sub_decoder->p_head = NULL;

    free(sub_decoder);
    sub_decoder = NULL;

    //sub_mem_deinit();

    dvbsub_dbg("[dvbsub_decoder_destroy] decoder destroyed\r\n");

    return retcode;
}


INT32S dvbsub_parse_pes_packet(long handle, const INT8U* packet, INT32U length)
{
    INT32S retcode = 0;
    INT64U pts = (INT64U)0;
    unsigned long pes_data_start = 0;
    INT32U pes_data_length = 0;
    INT32U pes_packet_length = 0;
    INT8U  scrambling_control = 0;
    INT8U  pts_dts_flag = 0;
    INT8U  pes_header_length = 0;
    dvbsub_sys_t *p_sys = NULL;
    dvbsub_decoder_t *sub_decoder = (dvbsub_decoder_t*)handle;

    if ((handle != (long)sub_decoder) || (handle == 0))
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] invalid handle !\r\n");
        return -1;
    }

    if ((length < 9) || (packet == NULL))
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] invalid pes packet !\r\n");
        return -1;
    }

    /* check pes header */
    if ( packet[0] != 0x00 || \
         packet[1] != 0x00 || \
         packet[2] != 0x01)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] invalid pes header !\r\n");
        return -1;
    }

    /* check stream id private_stream_1 */
    if (packet[3] != 0xbd)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] no private_stream_1 stream !\r\n");
        return -1;
    }

    pes_packet_length = (packet[4] << 8) | packet[5];

    /* check pes packet length */
    if ((pes_packet_length < 3) || ((pes_packet_length + 6) > length))
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes packet length invalid !\r\n");
        return -1;
    }

    scrambling_control = (packet[6] >> 4) & 0x03;
    pts_dts_flag = (packet[7] >> 6) & 0x03;
    pes_header_length = packet[8];

    /* check pes scrambling control */
    if (scrambling_control != 0)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes packet scrambling control: 0x%02x !\r\n", scrambling_control);
    }

    /* check pts/dts flag */
    if (pts_dts_flag != 2 && pts_dts_flag != 3)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes packet no pts/dts (flag: %d) !\r\n", pts_dts_flag);
        return -1;
    }

    /* check pes header length */
    if ((pes_header_length < 5) || ((INT32U)(pes_header_length + 3) > pes_packet_length))
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes packet header length invalid !\r\n");
        return -1;
    }

    /* get packet pts */
    pts = (INT64U)0;
    pts |= (INT64U)((packet[9] >> 1) & 0x07) << 30;
    pts |= (INT64U)((((packet[10] << 8) | packet[11]) >> 1) & 0x7fff) << 15;
    pts |= (INT64U)((((packet[12] << 8) | packet[13]) >> 1) & 0x7fff);

    pes_data_start = (unsigned long)(&packet[9+pes_header_length]);
    pes_data_length = pes_packet_length - 3 - pes_header_length;

    /* check pes data length */
    if (pes_data_length < 3)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes data length invalid !\r\n");
        return -1;
    }

    /* get decoder sys */
    p_sys = sub_decoder->p_sys;

    if (p_sys->pts != pts)
    {
        sub_default_display_init(sub_decoder);
    }

    /* set pts */
    p_sys->pts = pts;

    /* bitstream init */
    bs_init(&p_sys->bs, (const void*)pes_data_start, pes_data_length);

    /* check data identifier */
    if (bs_read(&p_sys->bs, 8) != 0x20)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes data identifier invalid !\r\n");
        return -1;
    }

    /* check subtitle stream id */
    if (bs_read(&p_sys->bs, 8) != 0)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] pes subtitle stream id invalid !\r\n");
        return -1;
    }

#ifndef WIN32
    dvbsub_dbg("[dvbsub_parse_pes_packet] subtitle pes packet received (pts: %llu)\r\n", pts);
#else
    dvbsub_dbg("[dvbsub_parse_pes_packet] subtitle pes packet received (pts: %I64d)\r\n", pts);
#endif

    p_sys->page_flag = 0;

    /* sync byte '00001111'*/
    while (bs_show(&p_sys->bs, 8 ) == 0x0f)
    {
        sub_parse_segment(sub_decoder, &p_sys->bs);
    }

    /* end of pes data field marker '11111111'*/
    if (bs_read(&p_sys->bs, 8) != 0xff)
    {
        dvbsub_dbg("[dvbsub_parse_pes_packet] end marker not found (corrupted subtitle ?)\r\n");
    }

    /* check if the page is to be displayed */
    if (p_sys->page_flag && p_sys->p_page)
    {
        sub_update_display(sub_decoder);
    }

    return retcode;
}

dvbsub_picture_t* dvbsub_get_display_set(long handle)
{
    dvbsub_decoder_t *sub_decoder = (dvbsub_decoder_t*)handle;

    if ((handle != (long)sub_decoder) || (handle == 0))
    {
        dvbsub_dbg("[dvbsub_get_display_set] invalid handle !\r\n");
        return NULL;
    }

    return sub_decoder->p_head;
}

INT32S dvbsub_remove_display_picture(long handle, dvbsub_picture_t* pic)
{
    INT32S retcode = 0;
    dvbsub_decoder_t *sub_decoder = (dvbsub_decoder_t*)handle;

    if ((handle != (long)sub_decoder) || (handle == 0))
    {
        dvbsub_dbg("[dvbsub_remove_display_picture] invalid handle !\r\n");
        return -1;
    }

    if ((pic != NULL) && (sub_decoder->p_head))
    {
        sub_remove_display(sub_decoder, pic);
    }

    return retcode;
}

