/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/

#include "sub_dvb_sub.h"
#include "sub_subtitle.h"
#include "sub_io.h"

static unsigned SPU_RD_HOLD_SIZE = 0x20;
#define OSD_HALF_SIZE (1920*1280/8)

#define  LOG_TAG    "sub_dvb"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  TRACE()    LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)

#define DVBSUB_PAGE_SEGMENT     0x10
#define DVBSUB_REGION_SEGMENT   0x11
#define DVBSUB_CLUT_SEGMENT     0x12
#define DVBSUB_OBJECT_SEGMENT   0x13
#define DVBSUB_DISPLAYDEFINITION_SEGMENT 0x14
#define DVBSUB_DISPLAY_SEGMENT  0x80
/* pixel operations */
#define MAX_NEG_CROP 1024

#define times4(x) x, x, x, x

extern int subtitle_status;
const uint8_t ff_cropTbl[256 + 2 * 1024] =
{
    times4(times4(times4(times4(times4(0x00))))),
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
    0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
    0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
    0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
    0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B,
    0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
    0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B,
    0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB,
    0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB,
    0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
    0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB,
    0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB,
    0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF,
    times4(times4(times4(times4(times4(0xFF)))))
};

#define cm (ff_cropTbl + MAX_NEG_CROP)

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))

#define YUV_TO_RGB1_CCIR(cb1, cr1)\
    {\
        cb = (cb1) - 128;\
        cr = (cr1) - 128;\
        r_add = FIX(1.40200*255.0/224.0) * cr + ONE_HALF;\
        g_add = - FIX(0.34414*255.0/224.0) * cb - FIX(0.71414*255.0/224.0) * cr + \
                ONE_HALF;\
        b_add = FIX(1.77200*255.0/224.0) * cb + ONE_HALF;\
    }

#define YUV_TO_RGB2_CCIR(r, g, b, y1)\
    {\
        y = ((y1) - 16) * FIX(255.0/219.0);\
        r = cm[(y + r_add) >> SCALEBITS];\
        g = cm[(y + g_add) >> SCALEBITS];\
        b = cm[(y + b_add) >> SCALEBITS];\
    }
#define DEBUG
#ifdef DEBUG
#undef fprintf
#undef perror
#if 0
static void png_save(const char *filename, uint8_t *bitmap, int w, int h,
                     uint32_t *rgba_palette)
{
    int x, y, v;
    FILE *f;
    char fname[40], fname2[40];
    char command[1024];
    snprintf(fname, 40, "%s.ppm", filename);
    f = fopen(fname, "w");
    if (!f)
    {
        perror(fname);
        return;
    }
    fprintf(f, "P6\n" "%d %d\n" "%d\n", w, h, 255);
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            v = rgba_palette[bitmap[y * w + x]];
            putc((v >> 16) & 0xff, f);
            putc((v >> 8) & 0xff, f);
            putc((v >> 0) & 0xff, f);
        }
    }
    fclose(f);
    snprintf(fname2, 40, "%s-a.pgm", filename);
    f = fopen(fname2, "w");
    if (!f)
    {
        perror(fname2);
        return;
    }
    fprintf(f, "P5\n" "%d %d\n" "%d\n", w, h, 255);
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            v = rgba_palette[bitmap[y * w + x]];
            putc((v >> 24) & 0xff, f);
        }
    }
    fclose(f);
    snprintf(command, 1024, "pnmtopng -alpha %s %s > %s.png 2> /dev/null",
             fname2, fname, filename);
    system(command);
    snprintf(command, 1024, "rm %s %s", fname, fname2);
    system(command);
}
#endif

#if 1
void *av_malloc(unsigned int size)
{
    void *ptr = NULL;
    ptr = malloc(size);
    return ptr;
}

void *av_realloc(void *ptr, unsigned int size)
{
    return realloc(ptr, size);
}

void av_freep(void **arg)
{
    if (*arg)
        free(*arg);
    *arg = NULL;
}

void av_free(void *arg)
{
    if (arg)
        free(arg);
}

void *av_mallocz(unsigned int size)
{
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

static inline uint32_t bytestream_get_be32(const uint8_t **ptr)
{
    uint32_t tmp;
    tmp =
        (*ptr)[3] | ((*ptr)[2] << 8) | ((*ptr)[1] << 16) | ((*ptr)[0] <<
                24);
    *ptr += 4;
    return tmp;
}

static inline uint32_t bytestream_get_be24(const uint8_t **ptr)
{
    uint32_t tmp;
    tmp = (*ptr)[2] | ((*ptr)[1] << 8) | ((*ptr)[0] << 16);
    *ptr += 3;
    return tmp;
}

static inline uint32_t bytestream_get_be16(const uint8_t **ptr)
{
    uint32_t tmp;
    tmp = (*ptr)[1] | ((*ptr)[0] << 8);
    *ptr += 2;
    return tmp;
}

static inline uint8_t bytestream_get_byte(const uint8_t **ptr)
{
    uint8_t tmp;
    tmp = **ptr;
    *ptr += 1;
    return tmp;
}
#endif

static int dvb_sub_valid_flag = 0;
int errnums = 0;

static void png_save2(const char *filename, uint32_t *bitmap, int w, int h)
{
    int x, y, v;
    FILE *f;
    char fname[40], fname2[40];
    char command[1024];
    snprintf(fname, sizeof(fname), "%s.ppm", filename);
    f = fopen(fname, "w");
    if (!f)
    {
        perror(fname);
        return;
    }
    fprintf(f, "P6\n" "%d %d\n" "%d\n", w, h, 255);
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            v = bitmap[y * w + x];
            putc((v >> 16) & 0xff, f);
            putc((v >> 8) & 0xff, f);
            putc((v >> 0) & 0xff, f);
        }
    }
    fclose(f);
    snprintf(fname2, sizeof(fname2), "%s-a.pgm", filename);
    f = fopen(fname2, "w");
    if (!f)
    {
        perror(fname2);
        return;
    }
    fprintf(f, "P5\n" "%d %d\n" "%d\n", w, h, 255);
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            v = bitmap[y * w + x];
            putc((v >> 24) & 0xff, f);
        }
    }
    fclose(f);
    //snprintf(command, sizeof(command), "pnmtopng -alpha %s %s > %s.png 2> /dev/null", fname2, fname, filename);
    //system(command);
    //snprintf(command, sizeof(command), "rm %s %s", fname, fname2);
    //system(command);
}
#endif

#define RGBA(r,g,b,a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

typedef struct DVBSubCLUT
{
    int id;

    uint32_t clut4[4];
    uint32_t clut16[16];
    uint32_t clut256[256];

    struct DVBSubCLUT *next;
} DVBSubCLUT;

static DVBSubCLUT default_clut;

typedef struct DVBSubObjectDisplay
{
    int object_id;
    int region_id;

    int x_pos;
    int y_pos;

    int fgcolor;
    int bgcolor;

    struct DVBSubObjectDisplay *region_list_next;
    struct DVBSubObjectDisplay *object_list_next;
} DVBSubObjectDisplay;

typedef struct DVBSubObject
{
    int id;

    int type;

    DVBSubObjectDisplay *display_list;

    struct DVBSubObject *next;
} DVBSubObject;

typedef struct DVBSubRegionDisplay
{
    int region_id;

    int x_pos;
    int y_pos;

    struct DVBSubRegionDisplay *next;
} DVBSubRegionDisplay;

typedef struct DVBSubRegion
{
    int id;

    int width;
    int height;
    int depth;

    int clut;
    int bgcolor;

    uint8_t *pbuf;
    int buf_size;

    DVBSubObjectDisplay *display_list;

    struct DVBSubRegion *next;
} DVBSubRegion;

typedef struct DVBSubDisplayDefinition
{
    int version;

    int x;
    int y;
    int width;
    int height;
} DVBSubDisplayDefinition;

typedef struct DVBSubContext
{
    int composition_id;
    int ancillary_id;

    int time_out;
    DVBSubRegion *region_list;
    DVBSubCLUT *clut_list;
    DVBSubObject *object_list;

    int display_list_size;
    DVBSubRegionDisplay *display_list;
    DVBSubDisplayDefinition *display_definition;
} DVBSubContext;

static DVBSubContext *g_ctx = NULL;
static AVSubtitle g_sub;

static DVBSubObject *get_object(DVBSubContext *ctx, int object_id)
{
    DVBSubObject *ptr = ctx->object_list;
    while (ptr && ptr->id != object_id)
    {
        ptr = ptr->next;
    }
    return ptr;
}

static DVBSubCLUT *get_clut(DVBSubContext *ctx, int clut_id)
{
    DVBSubCLUT *ptr = ctx->clut_list;
    while (ptr && ptr->id != clut_id)
    {
        ptr = ptr->next;
    }
    return ptr;
}

static DVBSubRegion *get_region(DVBSubContext *ctx, int region_id)
{
    DVBSubRegion *ptr = ctx->region_list;
    while (ptr && ptr->id != region_id)
    {
        ptr = ptr->next;
    }
    return ptr;
}

static void delete_region_display_list(DVBSubContext *ctx,
                                       DVBSubRegion *region)
{
    DVBSubObject *object, *obj2, **obj2_ptr;
    DVBSubObjectDisplay *display, *obj_disp, **obj_disp_ptr;
    while (region->display_list)
    {
        display = region->display_list;
        object = get_object(ctx, display->object_id);
        if (object)
        {
            obj_disp_ptr = &object->display_list;
            obj_disp = *obj_disp_ptr;
            while (obj_disp && obj_disp != display)
            {
                obj_disp_ptr = &obj_disp->object_list_next;
                obj_disp = *obj_disp_ptr;
            }
            if (obj_disp)
            {
                *obj_disp_ptr = obj_disp->object_list_next;
                if (!object->display_list)
                {
                    obj2_ptr = &ctx->object_list;
                    obj2 = *obj2_ptr;
                    while (obj2 != object)
                    {
                        assert(obj2);
                        obj2_ptr = &obj2->next;
                        obj2 = *obj2_ptr;
                    }
                    *obj2_ptr = obj2->next;
                    if (obj2)
                    {
                        LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__, __LINE__, obj2);
                        av_free(obj2);
                        obj2 = NULL;
                    }
                }
            }
        }
        region->display_list = display->region_list_next;
        if (display)
        {
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, display);
            av_free(display);
            display = NULL;
        }
    }
}

static void delete_cluts(DVBSubContext *ctx)
{
    DVBSubCLUT *clut;
    while (ctx->clut_list)
    {
        clut = ctx->clut_list;
        ctx->clut_list = clut->next;
        if (clut)
        {
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, clut);
            av_free(clut);
            clut = NULL;
        }
    }
}

static void delete_objects(DVBSubContext *ctx)
{
    DVBSubObject *object;
    while (ctx->object_list)
    {
        object = ctx->object_list;
        ctx->object_list = object->next;
        if (object)
        {
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, object);
            av_free(object);
            object = NULL;
        }
    }
}

static void delete_regions(DVBSubContext *ctx)
{
    DVBSubRegion *region;
    while (ctx->region_list)
    {
        region = ctx->region_list;
        ctx->region_list = region->next;
        delete_region_display_list(ctx, region);
        if (region)
        {
            if (region->pbuf)
            {
                LOGI("[%s::%d] region->pbuf =  0x%x, region = 0x%x,  size = %d\n", __FUNCTION__, __LINE__, region->pbuf, region, region->buf_size);
                LOGI("@@[%s::%d]av_free ptr = 0x%x, region->id = %d\n", __FUNCTION__, __LINE__, region->pbuf, region->id);
                av_free(region->pbuf);
                region->pbuf = NULL;
            }
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, region);
            av_free(region);
            region = NULL;
        }
    }
}

av_cold int dvbsub_init_decoder()
{
    int i, r, g, b, a = 0;
    g_ctx = av_mallocz(sizeof(DVBSubContext));
    if (!g_ctx)
        LOGE("[%s::%d]malloc error! \n", __FUNCTION__, __LINE__);
    DVBSubContext *ctx = g_ctx;
    //if (!avctx->extradata || avctx->extradata_size != 4) {
    LOGI("[%s::%d] Invalid extradata, subtitle streams may be combined!\n",
         __FUNCTION__, __LINE__);
    ctx->composition_id = -1;
    ctx->ancillary_id = -1;
    //} else {
    //    ctx->composition_id = AV_RB16(avctx->extradata);
    //    ctx->ancillary_id   = AV_RB16(avctx->extradata + 2);
    //}
    default_clut.id = -1;
    default_clut.next = NULL;
    default_clut.clut4[0] = RGBA(0, 0, 0, 0);
    default_clut.clut4[1] = RGBA(255, 255, 255, 255);
    default_clut.clut4[2] = RGBA(0, 0, 0, 255);
    default_clut.clut4[3] = RGBA(127, 127, 127, 255);
    default_clut.clut16[0] = RGBA(0, 0, 0, 0);
    for (i = 1; i < 16; i++)
    {
        if (i < 8)
        {
            r = (i & 1) ? 255 : 0;
            g = (i & 2) ? 255 : 0;
            b = (i & 4) ? 255 : 0;
        }
        else
        {
            r = (i & 1) ? 127 : 0;
            g = (i & 2) ? 127 : 0;
            b = (i & 4) ? 127 : 0;
        }
        default_clut.clut16[i] = RGBA(r, g, b, 255);
    }
    default_clut.clut256[0] = RGBA(0, 0, 0, 0);
    for (i = 1; i < 256; i++)
    {
        if (i < 8)
        {
            r = (i & 1) ? 255 : 0;
            g = (i & 2) ? 255 : 0;
            b = (i & 4) ? 255 : 0;
            a = 63;
        }
        else
        {
            switch (i & 0x88)
            {
                case 0x00:
                    r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
                    g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
                    b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
                    a = 255;
                    break;
                case 0x08:
                    r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
                    g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
                    b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
                    a = 127;
                    break;
                case 0x80:
                    r = 127 + ((i & 1) ? 43 : 0) +
                        ((i & 0x10) ? 85 : 0);
                    g = 127 + ((i & 2) ? 43 : 0) +
                        ((i & 0x20) ? 85 : 0);
                    b = 127 + ((i & 4) ? 43 : 0) +
                        ((i & 0x40) ? 85 : 0);
                    a = 255;
                    break;
                case 0x88:
                    r = ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
                    g = ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
                    b = ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
                    a = 255;
                    break;
            }
        }
        default_clut.clut256[i] = RGBA(r, g, b, a);
    }
    return 0;
}

av_cold int dvbsub_close_decoder()
{
    if (g_ctx != NULL)
    {
        DVBSubContext *ctx = g_ctx;
        DVBSubRegionDisplay *display;
        delete_regions(ctx);
        delete_objects(ctx);
        delete_cluts(ctx);
        errnums = 0;
        av_freep(&ctx->display_definition);
        while (ctx->display_list)
        {
            display = ctx->display_list;
            ctx->display_list = display->next;
            if (display)
            {
                LOGI("@@[%s::%d]av_free ptr = 0x%x\n",
                     __FUNCTION__, __LINE__, display);
                av_free(display);
                display = NULL;
            }
        }
    }
    return 0;
}

static int dvbsub_read_2bit_string(uint8_t *destbuf, int dbuf_len,
                                   const uint8_t **srcbuf, int buf_size,
                                   int non_mod, uint8_t *map_table)
{
    GetBitContext gb;
    int bits;
    int run_length;
    int pixels_read = 0;
    init_get_bits(&gb, *srcbuf, buf_size << 3);
    while (get_bits_count(&gb) < buf_size << 3 && pixels_read < dbuf_len)
    {
        bits = get_bits(&gb, 2);
        if (bits)
        {
            if (non_mod != 1 || bits != 1)
            {
                if (map_table)
                    *destbuf++ = map_table[bits];
                else
                    *destbuf++ = bits;
            }
            pixels_read++;
        }
        else
        {
            bits = get_bits1(&gb);
            if (bits == 1)
            {
                run_length = get_bits(&gb, 3) + 3;
                bits = get_bits(&gb, 2);
                if (non_mod == 1 && bits == 1)
                    pixels_read += run_length;
                else
                {
                    if (map_table)
                        bits = map_table[bits];
                    while (run_length-- > 0
                            && pixels_read < dbuf_len)
                    {
                        *destbuf++ = bits;
                        pixels_read++;
                    }
                }
            }
            else
            {
                bits = get_bits1(&gb);
                if (bits == 0)
                {
                    bits = get_bits(&gb, 2);
                    if (bits == 2)
                    {
                        run_length =
                            get_bits(&gb, 4) + 12;
                        bits = get_bits(&gb, 2);
                        if (non_mod == 1 && bits == 1)
                            pixels_read +=
                                run_length;
                        else
                        {
                            if (map_table)
                                bits =
                                    map_table
                                    [bits];
                            while (run_length-- > 0
                                    && pixels_read <
                                    dbuf_len)
                            {
                                *destbuf++ =
                                    bits;
                                pixels_read++;
                            }
                        }
                    }
                    else if (bits == 3)
                    {
                        run_length =
                            get_bits(&gb, 8) + 29;
                        bits = get_bits(&gb, 2);
                        if (non_mod == 1 && bits == 1)
                            pixels_read +=
                                run_length;
                        else
                        {
                            if (map_table)
                                bits =
                                    map_table
                                    [bits];
                            while (run_length-- > 0
                                    && pixels_read <
                                    dbuf_len)
                            {
                                *destbuf++ =
                                    bits;
                                pixels_read++;
                            }
                        }
                    }
                    else if (bits == 1)
                    {
                        pixels_read += 2;
                        if (map_table)
                            bits = map_table[0];
                        else
                            bits = 0;
                        if (pixels_read <= dbuf_len)
                        {
                            *destbuf++ = bits;
                            *destbuf++ = bits;
                        }
                    }
                    else
                    {
                        (*srcbuf) +=
                            (get_bits_count(&gb) +
                             7) >> 3;
                        return pixels_read;
                    }
                }
                else
                {
                    if (map_table)
                        bits = map_table[0];
                    else
                        bits = 0;
                    *destbuf++ = bits;
                    pixels_read++;
                }
            }
        }
    }
    if (get_bits(&gb, 6))
        LOGI("[%s::%d] DVBSub error: line overflow\n", __FUNCTION__,
             __LINE__);
    (*srcbuf) += (get_bits_count(&gb) + 7) >> 3;
    return pixels_read;
}

static int dvbsub_read_4bit_string(uint8_t *destbuf, int dbuf_len,
                                   const uint8_t **srcbuf, int buf_size,
                                   int non_mod, uint8_t *map_table)
{
    GetBitContext gb;
    int bits;
    int run_length;
    int pixels_read = 0;
    init_get_bits(&gb, *srcbuf, buf_size << 3);
    while (get_bits_count(&gb) < buf_size << 3 && pixels_read < dbuf_len)
    {
        if (subtitle_status == SUB_STOP)
            return 0;
        bits = get_bits(&gb, 4);
        if (bits)
        {
            if (non_mod != 1 || bits != 1)
            {
                if (map_table)
                    *destbuf++ = map_table[bits];
                else
                    *destbuf++ = bits;
            }
            pixels_read++;
        }
        else
        {
            bits = get_bits1(&gb);
            if (bits == 0)
            {
                run_length = get_bits(&gb, 3);
                if (run_length == 0)
                {
                    (*srcbuf) +=
                        (get_bits_count(&gb) + 7) >> 3;
                    return pixels_read;
                }
                run_length += 2;
                if (map_table)
                    bits = map_table[0];
                else
                    bits = 0;
                while (run_length-- > 0
                        && pixels_read < dbuf_len)
                {
                    *destbuf++ = bits;
                    pixels_read++;
                }
            }
            else
            {
                bits = get_bits1(&gb);
                if (bits == 0)
                {
                    run_length = get_bits(&gb, 2) + 4;
                    bits = get_bits(&gb, 4);
                    if (non_mod == 1 && bits == 1)
                        pixels_read += run_length;
                    else
                    {
                        if (map_table)
                            bits = map_table[bits];
                        while (run_length-- > 0
                                && pixels_read <
                                dbuf_len)
                        {
                            if (subtitle_status ==
                                    SUB_STOP)
                                return 0;
                            *destbuf++ = bits;
                            pixels_read++;
                        }
                    }
                }
                else
                {
                    bits = get_bits(&gb, 2);
                    if (bits == 2)
                    {
                        run_length =
                            get_bits(&gb, 4) + 9;
                        bits = get_bits(&gb, 4);
                        if (non_mod == 1 && bits == 1)
                            pixels_read +=
                                run_length;
                        else
                        {
                            if (map_table)
                                bits =
                                    map_table
                                    [bits];
                            while (run_length-- > 0
                                    && pixels_read <
                                    dbuf_len)
                            {
                                if (subtitle_status == SUB_STOP)
                                    return
                                        0;
                                *destbuf++ =
                                    bits;
                                pixels_read++;
                            }
                        }
                    }
                    else if (bits == 3)
                    {
                        run_length =
                            get_bits(&gb, 8) + 25;
                        bits = get_bits(&gb, 4);
                        if (non_mod == 1 && bits == 1)
                            pixels_read +=
                                run_length;
                        else
                        {
                            if (map_table)
                                bits =
                                    map_table
                                    [bits];
                            while (run_length-- > 0
                                    && pixels_read <
                                    dbuf_len)
                            {
                                if (subtitle_status == SUB_STOP)
                                    return
                                        0;
                                *destbuf++ =
                                    bits;
                                pixels_read++;
                            }
                        }
                    }
                    else if (bits == 1)
                    {
                        pixels_read += 2;
                        if (map_table)
                            bits = map_table[0];
                        else
                            bits = 0;
                        if (pixels_read <= dbuf_len)
                        {
                            *destbuf++ = bits;
                            *destbuf++ = bits;
                        }
                    }
                    else
                    {
                        if (map_table)
                            bits = map_table[0];
                        else
                            bits = 0;
                        *destbuf++ = bits;
                        pixels_read++;
                    }
                }
            }
        }
    }
    if (get_bits(&gb, 8))
        LOGI("[%s::%d] DVBSub error: line overflow\n", __FUNCTION__,
             __LINE__);
    (*srcbuf) += (get_bits_count(&gb) + 7) >> 3;
    return pixels_read;
}

static int dvbsub_read_8bit_string(uint8_t *destbuf, int dbuf_len,
                                   const uint8_t **srcbuf, int buf_size,
                                   int non_mod, uint8_t *map_table)
{
    const uint8_t *sbuf_end = (*srcbuf) + buf_size;
    int bits;
    int run_length;
    int pixels_read = 0;
    while (*srcbuf < sbuf_end && pixels_read < dbuf_len)
    {
        bits = *(*srcbuf)++;
        if (bits)
        {
            if (non_mod != 1 || bits != 1)
            {
                if (map_table)
                    *destbuf++ = map_table[bits];
                else
                    *destbuf++ = bits;
            }
            pixels_read++;
        }
        else
        {
            bits = *(*srcbuf)++;
            run_length = bits & 0x7f;
            if ((bits & 0x80) == 0)
            {
                if (run_length == 0)
                {
                    return pixels_read;
                }
                if (map_table)
                    bits = map_table[0];
                else
                    bits = 0;
                while (run_length-- > 0
                        && pixels_read < dbuf_len)
                {
                    *destbuf++ = bits;
                    pixels_read++;
                }
            }
            else
            {
                bits = *(*srcbuf)++;
                if (non_mod == 1 && bits == 1)
                    pixels_read += run_length;
                if (map_table)
                    bits = map_table[bits];
                else
                    while (run_length-- > 0
                            && pixels_read < dbuf_len)
                    {
                        *destbuf++ = bits;
                        pixels_read++;
                    }
            }
        }
    }
    if (*(*srcbuf)++)
        LOGI("[%s::%d] DVBSub error: line overflow\n", __FUNCTION__,
             __LINE__);
    return pixels_read;
}

static int dvbsub_parse_pixel_data_block(DVBSubObjectDisplay *display,
        const uint8_t *buf, int buf_size,
        int top_bottom, int non_mod)
{
    DVBSubContext *ctx = g_ctx;
    DVBSubRegion *region = get_region(ctx, display->region_id);
    const uint8_t *buf_end = buf + buf_size;
    uint8_t *pbuf;
    int x_pos, y_pos;
    int i;
    uint8_t map2to4[] = { 0x0, 0x7, 0x8, 0xf };
    uint8_t map2to8[] = { 0x00, 0x77, 0x88, 0xff };
    uint8_t map4to8[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
                        };
    uint8_t *map_table;
    LOGI("[%s::%d] DVB pixel block size %d, %s field:\n", __FUNCTION__,
         __LINE__, buf_size, top_bottom ? "bottom" : "top");
#if 0
    for (i = 0; i < buf_size; i++)
    {
        if (i % 16 == 0)
            LOGI("[%s::%d] 0x%8p: ", __FUNCTION__, __LINE__,
                 buf + i);
        LOGI("%02x ", buf[i]);
        if (i % 16 == 15)
            LOGI("\n");
    }
    if (i % 16)
        LOGI("\n");
#endif
    if (region == 0)
        return 0;
    pbuf = region->pbuf;
    x_pos = display->x_pos;
    y_pos = display->y_pos;
    if ((y_pos & 1) != top_bottom)
        y_pos++;
    while (buf < buf_end)
    {
        if (x_pos > region->width || y_pos > region->height)
        {
            LOGE("Invalid object location!\n");
            return -1;
        }
        switch (*buf++)
        {
            case 0x10:
                if (region->depth == 8)
                    map_table = map2to8;
                else if (region->depth == 4)
                    map_table = map2to4;
                else
                    map_table = NULL;
                x_pos +=
                    dvbsub_read_2bit_string(pbuf +
                                            (y_pos * region->width) +
                                            x_pos,
                                            region->width - x_pos, &buf,
                                            buf_end - buf, non_mod,
                                            map_table);
                break;
            case 0x11:
                if (region->depth < 4)
                {
                    LOGE("4-bit pixel string in %d-bit region!\n",
                         region->depth);
                    return 0;
                }
                if (region->depth == 8)
                    map_table = map4to8;
                else
                    map_table = NULL;
                x_pos +=
                    dvbsub_read_4bit_string(pbuf +
                                            (y_pos * region->width) +
                                            x_pos,
                                            region->width - x_pos, &buf,
                                            buf_end - buf, non_mod,
                                            map_table);
                break;
            case 0x12:
                if (region->depth < 8)
                {
                    LOGE("8-bit pixel string in %d-bit region!\n",
                         region->depth);
                    return 0;
                }
                x_pos +=
                    dvbsub_read_8bit_string(pbuf +
                                            (y_pos * region->width) +
                                            x_pos,
                                            region->width - x_pos, &buf,
                                            buf_end - buf, non_mod,
                                            NULL);
                break;
            case 0x20:
                map2to4[0] = (*buf) >> 4;
                map2to4[1] = (*buf++) & 0xf;
                map2to4[2] = (*buf) >> 4;
                map2to4[3] = (*buf++) & 0xf;
                break;
            case 0x21:
                for (i = 0; i < 4; i++)
                    map2to8[i] = *buf++;
                break;
            case 0x22:
                for (i = 0; i < 16; i++)
                    map4to8[i] = *buf++;
                break;
            case 0xf0:
                x_pos = display->x_pos;
                y_pos += 2;
                break;
            default:
                LOGI("[%s::%d] Unknown/unsupported pixel block 0x%x\n",
                     __FUNCTION__, __LINE__, *(buf - 1));
                break;                       //when account for unknown pixel block, don't discard current segment. SWPL-7481
        }
    }
    return 0;
}

static int dvbsub_parse_object_segment(const uint8_t *buf, int buf_size)
{
    DVBSubContext *ctx = g_ctx;
    const uint8_t *buf_end = buf + buf_size;
    const uint8_t *block;
    int object_id;
    DVBSubObject *object;
    DVBSubObjectDisplay *display;
    int top_field_len, bottom_field_len;
    int coding_method, non_modifying_color;
    object_id = AV_RB16(buf);
    buf += 2;
    object = get_object(ctx, object_id);
    if (!object)
        return 0;
    coding_method = ((*buf) >> 2) & 3;
    non_modifying_color = ((*buf++) >> 1) & 1;
    if (coding_method == 0)
    {
        top_field_len = AV_RB16(buf);
        buf += 2;
        bottom_field_len = AV_RB16(buf);
        buf += 2;
        if (buf + top_field_len + bottom_field_len > buf_end)
        {
            LOGE("Field data size too large\n");
            return 0;
        }
        for (display = object->display_list; display;
                display = display->object_list_next)
        {
            int ret = 0;
            block = buf;
            if (subtitle_status == SUB_STOP)
                return -1;
            ret =
                dvbsub_parse_pixel_data_block(display, block,
                                              top_field_len, 0,
                                              non_modifying_color);
            if (ret == -1)
                return ret;
            if (subtitle_status == SUB_STOP)
                return -1;
            if (bottom_field_len > 0)
                block = buf + top_field_len;
            else
                bottom_field_len = top_field_len;
            ret =
                dvbsub_parse_pixel_data_block(display, block,
                                              bottom_field_len, 1,
                                              non_modifying_color);
            if (ret == -1)
                return ret;
        }
        /*  } else if (coding_method == 1) { */
    }
    else
    {
        LOGE("Unknown object coding %d\n", coding_method);
        return -1;
    }
    return 0;
}

static void dvbsub_parse_clut_segment(const uint8_t *buf, int buf_size)
{
    DVBSubContext *ctx = g_ctx;
    const uint8_t *buf_end = buf + buf_size;
    int i, clut_id;
    DVBSubCLUT *clut;
    int entry_id, depth, full_range;
    int y, cr, cb, alpha;
    int r, g, b, r_add, g_add, b_add;
    LOGE("DVB clut packet:\n");
#if 0
    for (i = 0; i < buf_size; i++)
    {
        LOGE("%02x ", buf[i]);
        if (i % 16 == 15)
            LOGE("\n");
    }
    if (i % 16)
        LOGE("\n");
#endif
    clut_id = *buf++;
    buf += 1;
    clut = get_clut(ctx, clut_id);

    if (!clut) {
        clut = av_malloc(sizeof(DVBSubCLUT));
        if (!clut) {
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__, __LINE__);
            return;
        }
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n", __FUNCTION__, __LINE__, clut, sizeof(DVBSubCLUT));

        memcpy(clut, &default_clut, sizeof(DVBSubCLUT));
        clut->id = clut_id;
        clut->next = ctx->clut_list;
        ctx->clut_list = clut;
    }
    while (buf + 4 < buf_end)
    {
        entry_id = *buf++;
        depth = (*buf) & 0xe0;
        if (depth == 0)
        {
            LOGE("Invalid clut depth 0x%x!\n", *buf);
            return;
        }
        full_range = (*buf++) & 1;
        if (full_range)
        {
            y = *buf++;
            cr = *buf++;
            cb = *buf++;
            alpha = *buf++;
        }
        else
        {
            y = buf[0] & 0xfc;
            cr = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
            cb = (buf[1] << 2) & 0xf0;
            alpha = (buf[1] << 6) & 0xc0;
            buf += 2;
        }
        if (y == 0) {
            cr = 0;
            cb = 0;
            alpha = 0xff;
        }
        YUV_TO_RGB1_CCIR(cb, cr);
        YUV_TO_RGB2_CCIR(r, g, b, y);
        LOGE("clut %d := (%d,%d,%d,%d)\n", entry_id, r, g, b, alpha);
        if (depth & 0x80)
            clut->clut4[entry_id] = RGBA(r, g, b, 255 - alpha);
        if (depth & 0x40)
            clut->clut16[entry_id] = RGBA(r, g, b, 255 - alpha);
        if (depth & 0x20)
            clut->clut256[entry_id] = RGBA(r, g, b, 255 - alpha);
    }
}

static void dvbsub_parse_region_segment(const uint8_t *buf, int buf_size)
{
    DVBSubContext *ctx = g_ctx;
    const uint8_t *buf_end = buf + buf_size;
    int region_id, object_id;
    DVBSubRegion *region;
    DVBSubObject *object;
    DVBSubObjectDisplay *display;
    int fill;
    if (buf_size < 10)
        return;
    region_id = *buf++;
    region = get_region(ctx, region_id);
    if (!region)
    {
        region = av_mallocz(sizeof(DVBSubRegion));
        if (!region) {
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__, __LINE__);
            return;
        }
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n", __FUNCTION__, __LINE__, region, sizeof(DVBSubRegion));

        region->id = region_id;
        region->next = ctx->region_list;
        ctx->region_list = region;
    }
    fill = ((*buf++) >> 3) & 1;
    region->width = AV_RB16(buf);
    buf += 2;
    region->height = AV_RB16(buf);
    buf += 2;
    if (region->width * region->height != region->buf_size)
    {
        if (region->pbuf)
        {
            LOGI("free region->buf = 0x%x \n", region->pbuf);
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, region->pbuf);
            av_free(region->pbuf);
            region->pbuf = NULL;
        }
        region->buf_size = region->width * region->height;
        region->pbuf = av_malloc(region->buf_size);
        if (!region->pbuf)
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                 __LINE__);
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
             __FUNCTION__, __LINE__, region->pbuf, region->buf_size);
        LOGI("malloc region->buf = 0x%x, size = %d \n", region->pbuf,
             region->buf_size);
        fill = 1;
    }
    region->depth = 1 << (((*buf++) >> 2) & 7);
    if (region->depth < 2 || region->depth > 8)
    {
        LOGI("region depth %d is invalid\n", region->depth);
        region->depth = 4;
    }
    region->clut = *buf++;

    if (region->depth == 8) {
        region->bgcolor = *buf++;
        buf += 1;

    } else {
        buf += 1;
        if (region->depth == 4)
            region->bgcolor = (((*buf++) >> 4) & 15);
        else
            region->bgcolor = (((*buf++) >> 2) & 3);
    }
    LOGI("Region %d, (%dx%d)\n", region_id, region->width, region->height);
    if (fill)
    {
        memset(region->pbuf, region->bgcolor, region->buf_size);
        LOGI("Fill region (%d)\n", region->bgcolor);
    }
    delete_region_display_list(ctx, region);
    while (buf + 5 < buf_end)
    {
        object_id = AV_RB16(buf);
        buf += 2;
        object = get_object(ctx, object_id);
        if (!object)
        {
            object = av_mallocz(sizeof(DVBSubObject));
            if (!object)
                LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                     __LINE__);
            LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
                 __FUNCTION__, __LINE__, object,
                 sizeof(DVBSubObject));
            object->id = object_id;
            object->next = ctx->object_list;
            ctx->object_list = object;
        }
        object->type = (*buf) >> 6;
        display = av_mallocz(sizeof(DVBSubObjectDisplay));
        if (!display) {
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__, __LINE__);
            return;
        }
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n", __FUNCTION__, __LINE__, display, sizeof(DVBSubObjectDisplay));

        display->object_id = object_id;
        display->region_id = region_id;
        display->x_pos = AV_RB16(buf) & 0xfff;
        buf += 2;
        display->y_pos = AV_RB16(buf) & 0xfff;
        buf += 2;
        if ((object->type == 1 || object->type == 2)
                && buf + 1 < buf_end)
        {
            display->fgcolor = *buf++;
            display->bgcolor = *buf++;
        }
        display->region_list_next = region->display_list;
        region->display_list = display;
        display->object_list_next = object->display_list;
        object->display_list = display;
    }
    return;
}

static void dvbsub_parse_page_segment(const uint8_t *buf, int buf_size)
{
    DVBSubContext *ctx = g_ctx;
    DVBSubRegionDisplay *display;
    DVBSubRegionDisplay *tmp_display_list, **tmp_ptr;
    const uint8_t *buf_end = buf + buf_size;
    int region_id;
    int page_state;
    if (buf_size < 1)
        return;
    ctx->time_out = *buf++;
    page_state = ((*buf++) >> 2) & 3;
    LOGI("Page time out %ds, state %d\n", ctx->time_out, page_state);
    if (page_state == 1 || page_state == 2)
    {
        LOGI("[%s::%d] dvbsub_parse_page_segment\n", __FUNCTION__,
             __LINE__);
        delete_regions(ctx);
        delete_objects(ctx);
        delete_cluts(ctx);
    }
    tmp_display_list = ctx->display_list;
    ctx->display_list = NULL;
    ctx->display_list_size = 0;
    while (buf + 5 < buf_end)
    {
        region_id = *buf++;
        buf += 1;
        display = tmp_display_list;
        tmp_ptr = &tmp_display_list;
        while (display && display->region_id != region_id)
        {
            tmp_ptr = &display->next;
            display = display->next;
        }
        if (!display)
            display = av_mallocz(sizeof(DVBSubRegionDisplay));
        if (!display) {
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__, __LINE__);
            return;
        }
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n", __FUNCTION__, __LINE__, display, sizeof(DVBSubObjectDisplay));

        display->region_id = region_id;
        display->x_pos = AV_RB16(buf);
        buf += 2;
        display->y_pos = AV_RB16(buf);
        buf += 2;
        *tmp_ptr = display->next;
        display->next = ctx->display_list;
        ctx->display_list = display;
        ctx->display_list_size++;
        LOGI("Region %d, (%d,%d)\n", region_id, display->x_pos,
             display->y_pos);
    }
    while (tmp_display_list)
    {
        display = tmp_display_list;
        tmp_display_list = display->next;
        if (display)
        {
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, display);
            av_free(display);
            display = NULL;
        }
    }
}

#if 1
static void save_display_set(DVBSubContext *ctx, AML_SPUVAR *spu)
{
    DVBSubRegion *region;
    DVBSubRegionDisplay *display;
    DVBSubCLUT *clut;
    uint32_t *clut_table;
    int x_pos, y_pos, width, height;
    int x, y, y_off, x_off;
    uint32_t *pbuf = NULL;
    char filename[32];
    static int fileno_index = 0;
    DVBSubDisplayDefinition *display_def = ctx->display_definition;
    x_pos = -1;
    y_pos = -1;
    width = 0;
    height = 0;
    for (display = ctx->display_list; display; display = display->next)
    {
        region = get_region(ctx, display->region_id);
        if (!region)
            return;
        if (x_pos == -1) {
            x_pos = display->x_pos;
            y_pos = display->y_pos;
            width = region->width;
            height = region->height;
        }
        else
        {
            if (display->x_pos < x_pos)
            {
                width += (x_pos - display->x_pos);
                x_pos = display->x_pos;
            }

            if (display->y_pos < y_pos) {
                // avoid different region bitmap cross;20150906 bug111420
                if (y_pos - display->y_pos < region->height) {
                    height += region->height;
                    y_pos = y_pos - region->height;
                    display->y_pos = y_pos;
                } else {
                    height += (y_pos - display->y_pos);
                    y_pos = display->y_pos;
                }

            }
            if (display->x_pos + region->width > x_pos + width)
            {
                width = display->x_pos + region->width - x_pos;
            }
            if (display->y_pos + region->height > y_pos + height)
            {
                height =
                    display->y_pos + region->height - y_pos;
            }
        }
        LOGI("@@[%s::%d]x_pos = %d, y_pos = %d, width = %d, height = %d, region->id = %d\n", __FUNCTION__, __LINE__, x_pos, y_pos, width, height, region->id);
    }
    if (x_pos >= 0)
    {
        //spu->buffer_size = width * height * 4;
        pbuf = av_malloc(width * height * 4);
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
             __FUNCTION__, __LINE__, pbuf, width * height * 4);
        LOGI("malloc pbuf = 0x%x, size = %d \n", region->pbuf,
             width * height * 4);
        if (!pbuf)
        {
            LOGE("save_display_set av_malloc fail, width =  %d, height = %d \n", width, height);
            return;
        }
        memset(pbuf,0,width * height * 4);
        memset(spu->spu_data,0,DVB_SUB_SIZE);
        //spu->spu_data = pbuf;
        spu->spu_width = width;
        spu->spu_height = height;
        spu->spu_start_x = x_pos;
        spu->spu_start_y = y_pos;
        if (display_def && display_def->width !=0 && display_def->height !=0) {
            spu->spu_origin_display_w = display_def->width;
            spu->spu_origin_display_h = display_def->height;
        }
        for (display = ctx->display_list; display;
                display = display->next)
        {
            unsigned check_err = 0;
            region = get_region(ctx, display->region_id);
            if (!region)
                return;
            x_off = display->x_pos - x_pos;
            y_off = display->y_pos - y_pos;
            clut = get_clut(ctx, region->clut);
            if (clut == 0)
                clut = &default_clut;
            switch (region->depth)
            {
                case 2:
                    clut_table = clut->clut4;
                    break;
                case 8:
                    clut_table = clut->clut256;
                    break;
                case 4:
                default:
                    clut_table = clut->clut16;
                    break;
            }

            LOGI("@@[%s::%d]x_off = %d, y_off = %d, width = %d, height = %d, region->width = %d, region->height = %d\n", __FUNCTION__, __LINE__, x_off, y_off, width, height, region->width, region->height);
            check_err = (region->height - 1 + y_off) * width * 4 + (x_off + region->width - 1) * 4 + 3;
            if ((check_err >= DVB_SUB_SIZE) || (region->height > 1080) || (region->width > 1920)) {
                LOGI("@@[%s::%d] sub data err!! check_err = %d\n", __FUNCTION__, __LINE__,check_err);
                return;
            }

            for (y = 0; y < region->height; y++) {
                for (x = 0; x < region->width; x++) {
                    pbuf[((y + y_off) * width) + x_off + x] =
                        clut_table[region->pbuf[y * region->width + x]];
                    spu->spu_data[((y + y_off) * width * 4) + (x_off + x) * 4] =
                        pbuf[((y + y_off) * width) + x_off + x] & 0xff;
                    spu->spu_data[((y + y_off) * width * 4) + (x_off + x) * 4 + 1] =
                        (pbuf[((y + y_off) * width) + x_off + x] >> 8) & 0xff;
                    spu->spu_data[((y + y_off) * width * 4) + (x_off + x) * 4 + 2] =
                        (pbuf[((y + y_off) * width) + x_off + x] >> 16) & 0xff;
                    spu->spu_data[((y + y_off) * width * 4) + (x_off + x) * 4 + 3] =
                        (pbuf[((y + y_off) * width) + x_off + x] >> 24) & 0xff;
                }
            }
            dvb_sub_valid_flag = 1;
        }
        //snprintf(filename, sizeof(filename), "./data/DVB/dvbs.%d", spu->pts);
        //png_save2(filename, pbuf, width, height);
        set_subtitle_height(height);
        LOGI("## save_display_set: %d, (%d,%d)--(%d,%d),(%d,%d)---\n",
             fileno_index, width, height, spu->spu_width,
             spu->spu_height, spu->spu_start_x, spu->spu_start_y);
        if (pbuf)
        {
            LOGI("free pbuf = 0x%x\n", region->pbuf);
            LOGI("@@[%s::%d]av_free ptr = 0x%x\n", __FUNCTION__,
                 __LINE__, pbuf);
            av_free(pbuf);
            pbuf = NULL;
        }
        pbuf = NULL;
    }
    fileno_index++;
}
#endif

static void dvbsub_parse_display_definition_segment(const uint8_t *buf,
        int buf_size)
{
    DVBSubContext *ctx = g_ctx;
    DVBSubDisplayDefinition *display_def = ctx->display_definition;
    int dds_version, info_byte;
    if (buf_size < 5)
        return;
    info_byte = bytestream_get_byte(&buf);
    dds_version = info_byte >> 4;
    if (display_def && display_def->version == dds_version)
        return;     // already have this display definition version
    if (!display_def)
    {
        display_def = av_mallocz(sizeof(*display_def));
        if (!display_def)
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                 __LINE__);
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
             __FUNCTION__, __LINE__, display_def, sizeof(*display_def));
        ctx->display_definition = display_def;
    }
    if (!display_def)
        return;
    display_def->version = dds_version;
    display_def->x = 0;
    display_def->y = 0;
    display_def->width = bytestream_get_be16(&buf) + 1;
    display_def->height = bytestream_get_be16(&buf) + 1;
    LOGI("-[%s],display_def->width=%d,height=%d,info_byte=%d,buf_size=%d--\n",__FUNCTION__,
        display_def->width,display_def->height,info_byte,buf_size);

    if (info_byte & 1 << 3) // display_window_flag
    {
        if (buf_size < 13)
            return;
        display_def->x = bytestream_get_be16(&buf);
        display_def->y = bytestream_get_be16(&buf);
        display_def->width =
            bytestream_get_be16(&buf) - display_def->x + 1;
        display_def->height =
            bytestream_get_be16(&buf) - display_def->y + 1;
    }
    return;
}

static int dvbsub_display_end_segment(AML_SPUVAR *spu, const uint8_t *buf,
                                      int buf_size, AVSubtitle *sub)
{
    DVBSubContext *ctx = g_ctx;
    DVBSubDisplayDefinition *display_def = ctx->display_definition;
    DVBSubRegion *region;
    DVBSubRegionDisplay *display;
    AVSubtitleRect *rect;
    DVBSubCLUT *clut;
    uint32_t *clut_table;
    int i;
    int offset_x = 0, offset_y = 0;
    int last_sub_num_rects = 0;
    spu->m_delay = spu->pts + ctx->time_out * 1000 * 90;
#if 0
    sub->end_display_time = ctx->time_out * 1000;
    spu->m_delay = spu->pts + sub->end_display_time * 90;
    if (display_def)
    {
        offset_x = display_def->x;
        offset_y = display_def->y;
    }
    sub->num_rects = ctx->display_list_size;
    last_sub_num_rects = sub->num_rects;
    if (sub->num_rects > 0)
    {
        sub->rects = av_mallocz(sizeof(*sub->rects) * sub->num_rects);
        if (!sub->rects)
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                 __LINE__);
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
             __FUNCTION__, __LINE__, sub->rects,
             sizeof(*sub->rects) * sub->num_rects);
        for (i = 0; i < sub->num_rects; i++)
        {
            sub->rects[i] = av_mallocz(sizeof(*sub->rects[i]));
            if (!sub->rects[i])
                LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                     __LINE__);
            LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
                 __FUNCTION__, __LINE__, sub->rects[i],
                 sizeof(*sub->rects[i]));
        }
    }
    i = 0;
    for (display = ctx->display_list; display; display = display->next)
    {
        region = get_region(ctx, display->region_id);
        rect = sub->rects[i];
        if (!region)
            continue;
        rect->x = display->x_pos + offset_x;
        rect->y = display->y_pos + offset_y;
        rect->w = region->width;
        rect->h = region->height;
        rect->nb_colors = 16;
        rect->type = SUBTITLE_BITMAP;
        rect->pict.linesize[0] = region->width;
        clut = get_clut(ctx, region->clut);
        if (!clut)
            clut = &default_clut;
        switch (region->depth)
        {
            case 2:
                clut_table = clut->clut4;
                break;
            case 8:
                clut_table = clut->clut256;
                break;
            case 4:
            default:
                clut_table = clut->clut16;
                break;
        }
        rect->pict.data[1] = av_mallocz(AVPALETTE_SIZE);
        if (!rect->pict.data[1])
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                 __LINE__);
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
             __FUNCTION__, __LINE__, rect->pict.data[1],
             AVPALETTE_SIZE);
        memcpy(rect->pict.data[1], clut_table,
               (1 << region->depth) * sizeof(uint32_t));
        rect->pict.data[0] = av_malloc(region->buf_size);
        if (!rect->pict.data[0])
            LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                 __LINE__);
        LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
             __FUNCTION__, __LINE__, rect->pict.data[0],
             region->buf_size);
        memcpy(rect->pict.data[0], region->pbuf, region->buf_size);
        i++;
    }
    sub->num_rects = i;
#endif
    save_display_set(ctx, spu);
    return 1;
}

int dvbsub_decode(AML_SPUVAR *spu, const uint8_t *psrc, const int size)
{
    LOGI("src size=%d:   %x %x %x %x %x %x %x %x --  %x %x %x %x %x %x %x %x --  %x %x %x %x %x %x %x %x -- %x %x %x %x %x %x %x %x\n", size, psrc[0], psrc[1], psrc[2], psrc[3], psrc[4], psrc[5], psrc[6], psrc[7], psrc[8], psrc[9], psrc[10], psrc[11], psrc[12], psrc[13], psrc[14], psrc[15], psrc[16], psrc[17], psrc[18], psrc[19], psrc[20], psrc[21], psrc[22], psrc[23], psrc[24], psrc[25], psrc[26], psrc[27], psrc[28], psrc[29], psrc[30], psrc[31]);
    const uint8_t *buf = psrc;
    int buf_size = size;
    DVBSubContext *ctx = g_ctx;
    AVSubtitle *sub = &g_sub;
    const uint8_t *p, *p_end;
    int segment_type;
    int page_id;
    int segment_length;
    int i;
    int data_size;
    spu->spu_width = 0;
    spu->spu_height = 0;
    spu->spu_start_x = 0;
    spu->spu_start_y = 0;
    //default spu display in windows width and height
    spu->spu_origin_display_w = 720;
    spu->spu_origin_display_h = 576;
#if 0
    LOGI("DVB sub packet:\n");
    for (i = 0; i < buf_size; i++)
    {
        LOGI("%02x ", buf[i]);
        if (i % 16 == 15)
            LOGI("\n");
    }
    if (i % 16)
        LOGI("\n");
#endif
    if (buf_size <= 6 || *buf != 0x0f)
    {
        LOGI("incomplete or broken packet");
        return -1;
    }
    p = buf;
    p_end = buf + buf_size;
    while (p_end - p >= 6 && *p == 0x0f)
    {
        if (subtitle_status == SUB_STOP)
            return -1;
        p += 1;
        segment_type = *p++;
        page_id = AV_RB16(p);
        p += 2;
        segment_length = AV_RB16(p);
        p += 2;
        if (p_end - p < segment_length)
        {
            LOGI("incomplete or broken packet");
            return -1;
        }
        LOGI("## dvbsub_decode segment_type = %x, -----\n",
             segment_type);
        if (page_id == ctx->composition_id
                || page_id == ctx->ancillary_id || ctx->composition_id == -1
                || ctx->ancillary_id == -1)
        {
            int ret = 0;
            switch (segment_type)
            {
                case DVBSUB_PAGE_SEGMENT:
                    dvbsub_parse_page_segment(p, segment_length);
                    break;
                case DVBSUB_REGION_SEGMENT:
                    dvbsub_parse_region_segment(p, segment_length);
                    break;
                case DVBSUB_CLUT_SEGMENT:
                    dvbsub_parse_clut_segment(p, segment_length);
                    break;
                case DVBSUB_OBJECT_SEGMENT:
                    ret =
                        dvbsub_parse_object_segment(p,
                                                    segment_length);
                    if (ret == -1)
                        return ret;
                    break;
                case DVBSUB_DISPLAYDEFINITION_SEGMENT:
                    dvbsub_parse_display_definition_segment(p,
                                                            segment_length);
                    break;
                case DVBSUB_DISPLAY_SEGMENT:
                    data_size =
                        dvbsub_display_end_segment(spu, p,
                                                   segment_length,
                                                   sub);
                    break;
                default:
                    LOGI("Subtitling segment type 0x%x, page id %d, length %d\n", segment_type, page_id, segment_length);
                    break;
            }
        }
        p += segment_length;
    }
    //if ((spu->spu_width == 0) && (spu->spu_height == 0)) {
    //    LOGI("## dvbsub_decode: not sub data ----------\n");
    //    return -1;
    //}
    return p - buf;
}

static int read_spu_buf(int read_handle, char *buf, int len)
{
    LOGI("read_spu_buf len = %d\n", len);
    if (len > 3 * 1024 * 1024)
        abort();
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

int get_dvb_spu(AML_SPUVAR *spu, int read_handle)
{
    char tmpbuf[256];
    unsigned int dvb_pts = 0, dvb_dts = 0, dvb_pts_end;
    unsigned int dvb_temp_pts, dvb_temp_dts;
    int dvb_packet_length = 0, dvb_pes_header_length = 0;
    int64_t packet_header = 0;
    char skip_packet_flag = 0;
    int ret = 0;
    LOGI("enter get_dvb_spu\n");
    while (1)
    {
        if (subtitle_status == SUB_STOP)
            return 0;
        dvb_pts = dvb_dts = 0;
        dvb_packet_length = dvb_pes_header_length = 0;
        packet_header = 0;
        skip_packet_flag = 0;
        if (getSize(read_handle) < SPU_RD_HOLD_SIZE)
        {
            LOGI("current dvb sub buffer size %d\n", getSize(read_handle));
            break;
        }
        while (read_spu_buf(read_handle, tmpbuf, 1) == 1)
        {
            packet_header = (packet_header << 8) | tmpbuf[0];
            //LOGI("## get_dvb_spu %x, %llx,-------------\n",tmpbuf[0], packet_header);
            if ((packet_header & 0xffffffff) == 0x000001bd)
            {
                LOGI("## 222  get_dvb_spu hardware demux dvb %x,%llx,-----------\n", tmpbuf[0], packet_header & 0xffffffffff);
                break;
            }
            else if ((packet_header & 0xffffffffff) == 0x414d4c5577 || (packet_header & 0xffffffffff) == 0x414d4c55aa)
            {
                LOGI("## 222  get_dvb_spu soft demux dvb %x,%llx,-----------\n", tmpbuf[0], packet_header & 0xffffffffff);
                goto aml_soft_demux;
            }
        }
        if ((packet_header & 0xffffffff) == 0x000001bd)
        {
            LOGI("find header 0x000001bd\n");
            dvb_sub_valid_flag = 0;
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
                    spu->subtitle_type = SUBTITLE_DVB;
                    spu->buffer_size = DVB_SUB_SIZE;
                    spu->spu_data = malloc(DVB_SUB_SIZE);
                    if (!spu->spu_data)
                        LOGE("[%s::%d]malloc error! \n",
                             __FUNCTION__, __LINE__);
                    LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n", __FUNCTION__, __LINE__, spu->spu_data, DVB_SUB_SIZE);
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
                                dvbsub_decode(spu,
                                              buf,
                                              dvb_packet_length);
                            if (ret != -1)
                            {
                                write_subtitle_file
                                (spu);
                            }
                            else
                            {
                                errnums++;
                                set_subtitle_errnums(errnums);
                                LOGI("dvb data error skip one frame errnums:%d!!\n\n", errnums);
                                return -1;
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
    if ((packet_header & 0xffffffffff) == 0x414d4c5577 || (packet_header & 0xffffffffff) == 0x414d4c55aa)
    {
        int data_len = 0;
        char *data = NULL;
        char *pdata = NULL;
        dvb_sub_valid_flag = 0;
        LOGI("## 333 get_dvb_spu %x,%x,%x,%x,%x,-------------\n",
             tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3]);
        if (read_spu_buf(read_handle, tmpbuf, 15) == 15)
        {
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
            spu->subtitle_type = SUBTITLE_DVB;
            spu->buffer_size = DVB_SUB_SIZE;
            spu->spu_data = malloc(DVB_SUB_SIZE);
            if (!spu->spu_data)
                LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                     __LINE__);
            LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
                 __FUNCTION__, __LINE__, spu->spu_data,
                 DVB_SUB_SIZE);
            spu->pts = dvb_pts;
            LOGI("socket send pts:%d\n", spu->pts);
            LOGI("## 4444 datalen=%d,pts=%x,delay=%x,diff=%x, data: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,--%d,%x,%x,-------------\n", data_len, dvb_pts, spu->m_delay, dvb_temp_pts, tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5], tmpbuf[6], tmpbuf[7], tmpbuf[8], tmpbuf[9], tmpbuf[10], tmpbuf[11], tmpbuf[12], tmpbuf[13], tmpbuf[14]);
            data = malloc(data_len);
            if (!data)
                LOGE("[%s::%d]malloc error! \n", __FUNCTION__,
                     __LINE__);
            LOGI("@@[%s::%d]av_malloc ptr = 0x%x, size = %d\n",
                 __FUNCTION__, __LINE__, data, data_len);
            memset(data, 0x0, data_len);
            ret = read_spu_buf(read_handle, data, data_len);
            LOGI("## ret=%d,data_len=%d, %x,%x,%x,%x,%x,%x,%x,%x,---------\n", ret, data_len, data[0], data[1], data[2], data[3], data[data_len - 4], data[data_len - 3], data[data_len - 2], data[data_len - 1]);
            pdata = data;
            ret = dvbsub_decode(spu, data, data_len);
            LOGI("socket receive decode pts:%d\n", spu->pts);
            LOGI("## dvb: (width=%d,height=%d), (x=%d,y=%d), dvb_sub_valid_flag=%d,---------\n", spu->spu_width, spu->spu_height, spu->spu_start_x, spu->spu_start_y, dvb_sub_valid_flag);
            write_subtitle_file(spu);
            if (pdata)
            {
                free(pdata);
                pdata = NULL;
            }
        }
    }
    return ret;
}
