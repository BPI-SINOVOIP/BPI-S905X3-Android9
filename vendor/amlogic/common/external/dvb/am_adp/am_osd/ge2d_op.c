#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief GE2D 绘图操作
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "am_osd_internal.h"
#include <amports/amstream.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#ifdef CHIP_8226H
#include "cmemlib.h"
#else
#include <linux/cmem.h>
#endif


/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#ifdef CHIP_8226H
/*GE2D 定义*/
#define GE2D_BLIT_WITHOUTKEY_NOBLOCK		0x4709
#define GE2D_STRETCHBLIT_NOALPHA_NOBLOCK   	0x4708
#define GE2D_BLIT_NOALPHA_NOBLOCK 			0x4707
#define GE2D_BLEND_NOBLOCK 	 				0x4706
#define GE2D_BLIT_NOBLOCK 	 				0x4705
#define GE2D_STRETCHBLIT_NOBLOCK 			0x4704
#define GE2D_FILLRECTANGLE_NOBLOCK 			0x4703

#define GE2D_STRETCHBLIT_NOALPHA   			0x4702
#define GE2D_BLIT_NOALPHA	 					0x4701
#define GE2D_BLEND			 					0x4700
#define GE2D_BLIT    			 				0x46ff
#define GE2D_STRETCHBLIT   						0x46fe
#define GE2D_FILLRECTANGLE   					0x46fd
#define GE2D_SRCCOLORKEY   					0x46fc
#define GE2D_SET_COEF							0x46fa
#define GE2D_CONFIG							0x46f9
#define GE2D_ANTIFLICKER_ENABLE				0x46f8
#define GE2D_BLIT_WITHOUTKEY				       0x46f7
#define GE2D_SRC2COLORKEY                                    0x46f6 

#define BLENDOP_ADD           0    //Cd = Cs*Fs+Cd*Fd
#define BLENDOP_SUB           1    //Cd = Cs*Fs-Cd*Fd
#define BLENDOP_REVERSE_SUB   2    //Cd = Cd*Fd-Cs*Fs
#define BLENDOP_MIN           3    //Cd = Min(Cd*Fd,Cs*Fs)
#define BLENDOP_MAX           4    //Cd = Max(Cd*Fd,Cs*Fs)
#define BLENDOP_LOGIC         5    //Cd = Cs op Cd
#define BLENDOP_LOGIC_CLEAR       (BLENDOP_LOGIC+0 )
#define BLENDOP_LOGIC_COPY        (BLENDOP_LOGIC+1 )
#define BLENDOP_LOGIC_NOOP        (BLENDOP_LOGIC+2 )
#define BLENDOP_LOGIC_SET         (BLENDOP_LOGIC+3 )
#define BLENDOP_LOGIC_COPY_INVERT (BLENDOP_LOGIC+4 )
#define BLENDOP_LOGIC_INVERT      (BLENDOP_LOGIC+5 )
#define BLENDOP_LOGIC_AND_REVERSE (BLENDOP_LOGIC+6 )
#define BLENDOP_LOGIC_OR_REVERSE  (BLENDOP_LOGIC+7 )
#define BLENDOP_LOGIC_AND         (BLENDOP_LOGIC+8 )
#define BLENDOP_LOGIC_OR          (BLENDOP_LOGIC+9 )
#define BLENDOP_LOGIC_NAND        (BLENDOP_LOGIC+10)
#define BLENDOP_LOGIC_NOR         (BLENDOP_LOGIC+11)
#define BLENDOP_LOGIC_XOR         (BLENDOP_LOGIC+12)
#define BLENDOP_LOGIC_EQUIV       (BLENDOP_LOGIC+13)
#define BLENDOP_LOGIC_AND_INVERT  (BLENDOP_LOGIC+14)
#define BLENDOP_LOGIC_OR_INVERT   (BLENDOP_LOGIC+15)

#define GE2D_ENDIAN_SHIFT               24
#define GE2D_ENDIAN_MASK            (0x1 << GE2D_ENDIAN_SHIFT)
#define GE2D_BIG_ENDIAN             (0 << GE2D_ENDIAN_SHIFT)
#define GE2D_LITTLE_ENDIAN          (1 << GE2D_ENDIAN_SHIFT)

#define GE2D_COLOR_MAP_SHIFT        20
#define GE2D_COLOR_MAP_MASK         (0xf << GE2D_COLOR_MAP_SHIFT)
/* 16 bit */
#define GE2D_COLOR_MAP_YUV422           (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB655           (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV655           (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB844           (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV844           (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6442     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6442     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4444     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4444     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB565       (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV565       (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB4444         (6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV4444         (6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB1555     (7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV1555     (7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4642     (8 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4642     (8 << GE2D_COLOR_MAP_SHIFT)
/* 24 bit */
#define GE2D_COLOR_MAP_RGB888       (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV444       (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA5658     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA5658     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8565     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8565     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6666     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6666     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB6666     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV6666     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGR888           (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUY888           (5 << GE2D_COLOR_MAP_SHIFT)
/* 32 bit */
#define GE2D_COLOR_MAP_RGBA8888         (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA8888         (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8888     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8888     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ABGR8888     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AVUY8888     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGRA8888     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUYA8888     (3 << GE2D_COLOR_MAP_SHIFT)

#define GE2D_FMT_S8_Y                   0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_CB                  0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_CR                  0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_R                   0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_G                   0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_B                   0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_A                   0x000c0 /* 00_11_0_00_0_00 */
#define GE2D_FMT_S8_LUT                 0x00020 /* 00_00_1_00_0_00 */
#define GE2D_FMT_S16_YUV422             0x20100 /* 01_00_0_00_0_00 */
#define GE2D_FMT_S16_RGB                (GE2D_LITTLE_ENDIAN|0x00100) /* 01_00_0_00_0_00 */
#define GE2D_FMT_S24_YUV444             0x20200 /* 10_00_0_00_0_00 */
#define GE2D_FMT_S24_RGB                (GE2D_LITTLE_ENDIAN|0x00200) /* 10_00_0_00_0_00 */
#define GE2D_FMT_S32_YUVA444            0x20300 /* 11_00_0_00_0_00 */
#define GE2D_FMT_S32_RGBA               (GE2D_LITTLE_ENDIAN|0x00300) /* 11_00_0_00_0_00 */
#define GE2D_FMT_M24_YUV420             0x20007 /* 00_00_0_00_1_11 */
#define GE2D_FMT_M24_YUV422             0x20006 /* 00_00_0_00_1_10 */
#define GE2D_FMT_M24_YUV444             0x20004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_RGB                0x00004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_YUV420T            0x20017 /* 00_00_0_10_1_11 */
#define GE2D_FMT_M24_YUV420B            0x2001f /* 00_00_0_11_1_11 */
#define GE2D_FMT_S16_YUV422T            0x20110 /* 01_00_0_10_0_00 */
#define GE2D_FMT_S16_YUV422B            0x20138 /* 01_00_0_11_0_00 */
#define GE2D_FMT_S24_YUV444T            0x20210 /* 10_00_0_10_0_00 */
#define GE2D_FMT_S24_YUV444B            0x20218 /* 10_00_0_11_0_00 */

/* back compatible defines */
#define GE2D_FORMAT_S8_Y            GE2D_FMT_S8_Y            
#define GE2D_FORMAT_S8_CB           GE2D_FMT_S8_CB           
#define GE2D_FORMAT_S8_CR           GE2D_FMT_S8_CR           
#define GE2D_FORMAT_S8_R            GE2D_FMT_S8_R            
#define GE2D_FORMAT_S8_G            GE2D_FMT_S8_G            
#define GE2D_FORMAT_S8_B            GE2D_FMT_S8_B            
#define GE2D_FORMAT_S8_A            GE2D_FMT_S8_A            
#define GE2D_FORMAT_S8_LUT          GE2D_FMT_S8_LUT          
#define GE2D_FORMAT_S16_YUV422      (GE2D_FMT_S16_YUV422  | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_RGB_655         (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_RGB655)  
#define GE2D_FORMAT_S24_YUV444      (GE2D_FMT_S24_YUV444  | GE2D_COLOR_MAP_YUV444) 
#define GE2D_FORMAT_S24_RGB         (GE2D_FMT_S24_RGB     | GE2D_COLOR_MAP_RGB888)   
#define GE2D_FORMAT_S32_YUVA444     (GE2D_FMT_S32_YUVA444 | GE2D_COLOR_MAP_YUVA4444)   
#define GE2D_FORMAT_S32_RGBA        (GE2D_FMT_S32_RGBA    | GE2D_COLOR_MAP_RGBA8888) 
#define GE2D_FORMAT_M24_YUV420      GE2D_FMT_M24_YUV420    
#define GE2D_FORMAT_M24_YUV422      GE2D_FMT_M24_YUV422
#define GE2D_FORMAT_M24_YUV444      GE2D_FMT_M24_YUV444
#define GE2D_FORMAT_M24_RGB         GE2D_FMT_M24_RGB
#define GE2D_FORMAT_M24_YUV420T     GE2D_FMT_M24_YUV420T
#define GE2D_FORMAT_M24_YUV420B     GE2D_FMT_M24_YUV420B
#define GE2D_FORMAT_S16_YUV422T     (GE2D_FMT_S16_YUV422T | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_YUV422B     (GE2D_FMT_S16_YUV422B | GE2D_COLOR_MAP_YUV422)   
#define GE2D_FORMAT_S24_YUV444T     (GE2D_FMT_S24_YUV444T | GE2D_COLOR_MAP_YUV444)   
#define GE2D_FORMAT_S24_YUV444B     (GE2D_FMT_S24_YUV444B | GE2D_COLOR_MAP_YUV444)
//format added in A1H
/*16 bit*/
#define GE2D_FORMAT_S16_RGB_565         (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_RGB565) 
#define GE2D_FORMAT_S16_RGB_844         (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_RGB844) 
#define GE2D_FORMAT_S16_RGBA_6442        (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_RGBA6442)
#define GE2D_FORMAT_S16_RGBA_4444        (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_RGBA4444)
#define GE2D_FORMAT_S16_ARGB_4444        (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_ARGB4444)
#define GE2D_FORMAT_S16_ARGB_1555        (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_ARGB1555)
#define GE2D_FORMAT_S16_RGBA_4642        (GE2D_FMT_S16_RGB     | GE2D_COLOR_MAP_RGBA4642)
/*24 bit*/
#define GE2D_FORMAT_S24_RGBA_5658         (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGBA5658)  
#define GE2D_FORMAT_S24_ARGB_8565         (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_ARGB8565) 
#define GE2D_FORMAT_S24_RGBA_6666         (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGBA6666)
#define GE2D_FORMAT_S24_ARGB_6666         (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_ARGB6666)
#define GE2D_FORMAT_S24_BGR                        (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_BGR888)
/*32 bit*/
#define GE2D_FORMAT_S32_ARGB        (GE2D_FMT_S32_RGBA    | GE2D_COLOR_MAP_ARGB8888) 
#define GE2D_FORMAT_S32_ABGR        (GE2D_FMT_S32_RGBA    | GE2D_COLOR_MAP_ABGR8888) 
#define GE2D_FORMAT_S32_BGRA        (GE2D_FMT_S32_RGBA    | GE2D_COLOR_MAP_BGRA8888) 

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct {
	int            x;   /* X coordinate of its top-left point */
	int            y;   /* Y coordinate of its top-left point */
	int            w;   /* width of it */
	int            h;   /* height of it */
} rectangle_t;

typedef  struct {
	unsigned int    color ;
	rectangle_t src1_rect;
	rectangle_t src2_rect;
	rectangle_t	dst_rect;
	int			op;
}ge2d_para_t ;

typedef struct {
	unsigned long  addr;
	unsigned int   w;
	unsigned int   h;
} config_planes_t;

typedef  struct{
	int            key_enable;
	int            key_color;
	int            key_mask;
} src_key_ctrl_t;

enum GE2D_SRC_DST_TYPE
{
	OSD0_OSD0 =0,
	OSD0_OSD1,
	OSD1_OSD1,
	OSD1_OSD0,
	ALLOC_OSD0,
	ALLOC_OSD1,
	ALLOC_ALLOC,
	TYPE_INVALID
};

typedef    struct {
	int  src_dst_type;
	int  alu_const_color;
	unsigned int src_format;
	unsigned int dst_format ; //add for src&dst all in user space.

	config_planes_t src_planes[4];
	config_planes_t dst_planes[4];
	src_key_ctrl_t  src_key;
       src_key_ctrl_t  src2_key;
}config_para_t;

#else  //!defined(CHIP_8226H)
#include <linux/ge2d/ge2d_main.h>
#endif //CHIP_8226H

/****************************************************************************
 * Static data
 ***************************************************************************/

static int ge2d_sd_types[]={
OSD0_OSD0,OSD0_OSD1,TYPE_INVALID,
OSD1_OSD0,OSD1_OSD1,TYPE_INVALID,
ALLOC_OSD0,ALLOC_OSD1,ALLOC_ALLOC
};

#define CANVAS_ALIGN(x)    (((x)+7)&~7)

/**\brief GE2D设备文件句柄*/
static int ge2d_fd;

static AM_ErrorCode_t ge2d_init (void);
static AM_ErrorCode_t ge2d_create_surface (AM_OSD_Surface_t *s);
static AM_ErrorCode_t ge2d_destroy_surface (AM_OSD_Surface_t *s);
static AM_ErrorCode_t ge2d_filled_rect (AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, uint32_t pix);
static AM_ErrorCode_t ge2d_blit (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para);
static AM_ErrorCode_t ge2d_deinit (void);

const AM_OSD_SurfaceOp_t ge2d_draw_op = {
.init         = ge2d_init,
.create_surface  = ge2d_create_surface,
.destroy_surface = ge2d_destroy_surface,
.filled_rect  = ge2d_filled_rect,
.blit         = ge2d_blit,
.deinit       = ge2d_deinit
};

static CMEM_AllocParams cmem_params = {CMEM_HEAP, CMEM_NONCACHED, 8};


/****************************************************************************
 * Functions
 ***************************************************************************/

static AM_Bool_t ge2d_valid_format(AM_OSD_PixelFormatType_t type)
{
	switch(type)
	{
		//case AM_OSD_FMT_PALETTE_256:
		case AM_OSD_FMT_COLOR_BGRA_4444:
		case AM_OSD_FMT_COLOR_BGRA_5551:
		case AM_OSD_FMT_COLOR_BGR_565:
		case AM_OSD_FMT_COLOR_RGB_888:
		case AM_OSD_FMT_COLOR_BGR_888:
		case AM_OSD_FMT_COLOR_ARGB_8888:
		case AM_OSD_FMT_COLOR_BGRA_8888:
		case AM_OSD_FMT_YUV_420:
		case AM_OSD_FMT_YUV_422:
		case AM_OSD_FMT_YUV_444:
			return AM_TRUE;
		default:
			return AM_FALSE;
	}
}

static void ge2d_format(AM_OSD_Surface_t *s, unsigned int *fmt, config_planes_t *plane)
{
	switch(s->format->type)
	{
		case AM_OSD_FMT_PALETTE_256:
			*fmt = GE2D_FORMAT_S8_LUT;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_BGRA_4444:
			*fmt = GE2D_FORMAT_S16_ARGB_4444;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_BGRA_5551:
			*fmt = GE2D_FORMAT_S16_ARGB_1555;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_BGR_565:
			*fmt = GE2D_FORMAT_S16_RGB_565;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_RGB_888:
			*fmt = GE2D_FORMAT_S24_BGR;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_BGR_888:
			*fmt = GE2D_FORMAT_S24_RGB;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_ARGB_8888:
			*fmt = GE2D_FORMAT_S32_BGRA;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_COLOR_BGRA_8888:
			*fmt = GE2D_FORMAT_S32_ARGB;
			plane->addr = CMEM_getPhys(s->buffer);
			plane->w    = s->width;
			plane->h    = s->height;
		break;
		case AM_OSD_FMT_YUV_420:
			*fmt = GE2D_FORMAT_M24_YUV420;
			plane[0].addr = CMEM_getPhys(s->buffer);
			plane[0].w    = CANVAS_ALIGN(s->width);
			plane[0].h    = s->height;
			plane[1].addr = plane[0].addr+CANVAS_ALIGN(s->width)*s->height;
			plane[1].w    = CANVAS_ALIGN(s->width)/2;
			plane[1].h    = s->height/2;
			plane[2].addr = plane[1].addr+CANVAS_ALIGN(s->width/2)*(s->height/2);
			plane[2].w    = CANVAS_ALIGN(s->width)/2;
			plane[2].h    = s->height/2;
		break;
		case AM_OSD_FMT_YUV_422:
			*fmt = GE2D_FORMAT_M24_YUV420;
			plane[0].addr = CMEM_getPhys(s->buffer);
			plane[0].w    = CANVAS_ALIGN(s->width);
			plane[0].h    = s->height;
			plane[1].addr = plane[0].addr+CANVAS_ALIGN(s->width)*s->height;
			plane[1].w    = CANVAS_ALIGN(s->width)/2;
			plane[1].h    = s->height;
			plane[2].addr = plane[1].addr+CANVAS_ALIGN(s->width/2)*s->height;
			plane[2].w    = CANVAS_ALIGN(s->width)/2;
			plane[2].h    = s->height;
		break;
		case AM_OSD_FMT_YUV_444:
			*fmt = GE2D_FORMAT_M24_YUV420;
			plane[0].addr = CMEM_getPhys(s->buffer);
			plane[0].w    = CANVAS_ALIGN(s->width);
			plane[0].h    = s->height;
			plane[1].addr = plane[0].addr+CANVAS_ALIGN(s->width)*s->height;
			plane[1].w    = CANVAS_ALIGN(s->width);
			plane[1].h    = s->height;
			plane[2].addr = plane[1].addr+CANVAS_ALIGN(s->width)*s->height;
			plane[2].w    = CANVAS_ALIGN(s->width);
			plane[2].h    = s->height;
		break;
		default:
		break;
	}
}

static AM_ErrorCode_t ge2d_init (void)
{
	ge2d_fd = open("/dev/ge2d", O_RDWR);
	if(ge2d_fd==-1)
	{
		AM_DEBUG(1, "cannot open ge2d device \"%s\"", strerror(errno));
		return AM_OSD_ERR_CANNOT_OPEN;
	}
	
#ifndef ANDROID
	if(CMEM_init()==-1)
	{
		AM_DEBUG(1, "cannot open cmem device \"%s\"", strerror(errno));
		close(ge2d_fd);
		return AM_OSD_ERR_CANNOT_OPEN;
	}
#endif
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ge2d_create_surface (AM_OSD_Surface_t *s)
{
	if(!ge2d_valid_format(s->format->type))
	{
		AM_DEBUG(1, "do not support hardware graphics format %d", s->format->type);
		return AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	if(!s->buffer)
	{
		int size = 0;
		
		if(AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(s->format->type))
		{
			switch(s->format->type)
			{
				case AM_OSD_FMT_YUV_420:
					size =  CANVAS_ALIGN(s->width)*s->height;
					size += CANVAS_ALIGN(s->width/2)*(s->height/2);
					size += CANVAS_ALIGN(s->width/2)*(s->height/2);
				break;
				case AM_OSD_FMT_YUV_422:
					size =  CANVAS_ALIGN(s->width)*s->height;
					size += CANVAS_ALIGN(s->width/2)*s->height;
					size += CANVAS_ALIGN(s->width/2)*s->height;
				break;
				case AM_OSD_FMT_YUV_444:
					size =  CANVAS_ALIGN(s->width)*s->height;
					size += CANVAS_ALIGN(s->width)*s->height;
					size += CANVAS_ALIGN(s->width)*s->height;
				break;
				default:
				break;
			}
		}
		else
		{
			if(!s->pitch)
				s->pitch = CANVAS_ALIGN(s->format->bytes_per_pixel*s->width);
			size = s->pitch*s->height;
		}
		
		if(size)
		{
			s->buffer = CMEM_alloc(0, size, &cmem_params);
			if(!s->buffer)
			{
				AM_DEBUG(1, "not enough memory %d", size);
				s->pitch = 0;
				return AM_OSD_ERR_NO_MEM;
			}
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ge2d_destroy_surface (AM_OSD_Surface_t *s)
{
	if(s->buffer && !(s->flags&AM_OSD_SURFACE_FL_EXTERNAL))
	{
		CMEM_free(s->buffer, &cmem_params);
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ge2d_filled_rect (AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, uint32_t pix)
{
	AM_OSD_Device_t *dev = s->drv_data;
	config_para_t conf;
	uint32_t color;
	ge2d_para_t   op;
	AM_OSD_Rect_t dr;
	
	if(!AM_OSD_ClipRect(&s->clip, rect, &dr))
		return AM_SUCCESS;
	
	memset(&conf, 0, sizeof(conf));
	memset(&op, 0, sizeof(op));
	
	if(dev)
	{
		conf.src_dst_type = dev->dev_no?OSD1_OSD1:OSD0_OSD0;
		if(s->flags&AM_OSD_SURFACE_FL_DBUF)
		{
			dr.x += dev->para.output_width;
		}
	}
	else
	{
		conf.src_dst_type = ALLOC_ALLOC;
		ge2d_format(s, &conf.src_format, conf.src_planes);
		conf.dst_format = conf.src_format;
		memcpy(conf.dst_planes, conf.src_planes, sizeof(conf.src_planes));
	}
	
	if(ioctl(ge2d_fd, GE2D_CONFIG, &conf)==-1)
	{
		AM_DEBUG(1, "ge2d config failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(s->format->type))
	{
		if(pix<s->format->palette.color_count)
		{
			AM_OSD_Color_t *pal = &s->format->palette.colors[pix];
			color = (pal->r<<24)|(pal->g<<16)|(pal->b<<8)|pal->a;
		}
		else
		{
			color = 0;
		}
	}
	else
	{
		AM_OSD_Color_t c;
		
		AM_RGB_PIXEL_TO_COLOR(s->format, pix, &c);
		
		color = (c.r<<24)|(c.g<<16)|(c.b<<8)|c.a;
	}
	
	op.color = color;
	op.src1_rect.x = dr.x;
	op.src1_rect.y = dr.y;
	op.src1_rect.w = dr.w;
	op.src1_rect.h = dr.h;
	
	if(ioctl(ge2d_fd, GE2D_FILLRECTANGLE, &op)==-1)
	{
		AM_DEBUG(1, "ge2d fill rect failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ge2d_blit (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para)
{
	AM_OSD_Device_t *sdev = src->drv_data;
	AM_OSD_Device_t *ddev = dst->drv_data;
	config_para_t conf;
	ge2d_para_t   op;
	AM_OSD_Rect_t tmpr, dsr, ddr;
	int cmd = 0, sd_type = 0;
	
	tmpr.x = 0;
	tmpr.y = 0;
	tmpr.w = src->width;
	tmpr.h = src->height;
	
	if(!AM_OSD_ClipRect(&tmpr, sr, &dsr))
		return AM_SUCCESS;
	
	tmpr.x = dsr.x+dr->x-sr->x;
	tmpr.y = dsr.y+dr->y-sr->y;
	tmpr.w = dsr.w*dr->w/sr->w;
	tmpr.h = dsr.h*dr->h/sr->h;
	
	if(!AM_OSD_ClipRect(&dst->clip, &tmpr, &ddr))
		return AM_SUCCESS;
	
	dsr.x = ddr.x+sr->x-dr->x;
	dsr.y = ddr.y+sr->y-dr->y;
	dsr.w = ddr.w*sr->w/dr->w;
	dsr.h = ddr.h*sr->h/dr->h;
	
	memset(&conf, 0, sizeof(conf));
	memset(&op, 0, sizeof(op));
	
	if(sdev)
	{
		sd_type = sdev->dev_no?3:0;
		if(src->flags&AM_OSD_SURFACE_FL_DBUF)
		{
			dsr.x += sdev->para.output_width;
		}
	}
	else
	{
		sd_type = 6;
		ge2d_format(src, &conf.src_format, conf.src_planes);
	}
	
	if(ddev)
	{
		sd_type += ddev->dev_no?1:0;
		if(dst->flags&AM_OSD_SURFACE_FL_DBUF)
		{
			ddr.x += ddev->para.output_width;
		}
	}
	else
	{
		sd_type += 2;
		ge2d_format(dst, &conf.dst_format, conf.dst_planes);
	}
	
	
	conf.src_dst_type = ge2d_sd_types[sd_type];
	
	op.src1_rect.x = dsr.x;
	op.src1_rect.y = dsr.y;
	op.src1_rect.w = dsr.w;
	op.src1_rect.h = dsr.h;
	
	op.dst_rect.x = ddr.x;
	op.dst_rect.y = ddr.y;
	op.dst_rect.w = ddr.w;
	op.dst_rect.h = ddr.h;
	
	if((dsr.w==ddr.w) && (dsr.h==ddr.h))
	{
		if(AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(src->format->type))
		{
			//cmd = GE2D_BLIT_NOALPHA;
			cmd = GE2D_STRETCHBLIT_NOALPHA;
		}
		else if(para->enable_op)
		{
			cmd = GE2D_BLEND;
			op.op = ((BLENDOP_LOGIC+para->op)<<24)|(2<<20)|(4<<16)|((BLENDOP_LOGIC+para->op)<<8)|(2<<4)|(4);
			op.src2_rect = op.dst_rect;
		}
		else if(para->enable_alpha || para->enable_global_alpha)
		{
			cmd = GE2D_BLEND;
			op.src2_rect = op.dst_rect;
			if(para->enable_global_alpha)
			{
				op.op = (BLENDOP_ADD<<24)|(12<<20)|(13<<16)|(4<<8)|(1<<4)|(1);
				conf.alu_const_color = para->alpha;
			}
			else
			{
				op.op = (BLENDOP_ADD<<24)|(6<<20)|(7<<16)|(4<<8)|(1<<4)|(1);
			}
		}
		else if(src->flags&AM_OSD_SURFACE_FL_DBUF)
		{
			cmd = GE2D_STRETCHBLIT;
		}
		else
		{
			//cmd = src->format->a_mask?GE2D_BLIT:GE2D_BLIT_NOALPHA;
			cmd = src->format->a_mask?GE2D_STRETCHBLIT:GE2D_STRETCHBLIT_NOALPHA;
		}
	}
	else
	{
		if(AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(src->format->type))
		{
			cmd = GE2D_STRETCHBLIT_NOALPHA;
		}
		else if(para->enable_op)
		{
			cmd = GE2D_BLEND;
			//cmd = GE2D_STRETCHBLIT_NOALPHA;
			op.op = ((BLENDOP_LOGIC+para->op)<<24)|(2<<20)|(4<<16)|((BLENDOP_LOGIC+para->op)<<8)|(2<<4)|(4);
			op.src2_rect = op.dst_rect;
		}
		else if(para->enable_alpha || para->enable_global_alpha)
		{
			cmd = GE2D_BLEND;
			//cmd = GE2D_STRETCHBLIT_NOALPHA;

			op.src2_rect = op.dst_rect;
			if(para->enable_global_alpha)
			{
				op.op = (BLENDOP_ADD<<24)|(12<<20)|(13<<16)|(4<<8)|(1<<4)|(1);
				conf.alu_const_color = para->alpha;
			}
			else
			{
				op.op = (BLENDOP_ADD<<24)|(6<<20)|(7<<16)|(4<<8)|(1<<4)|(1);
			}
		}
		else
		{
			cmd = src->format->a_mask?GE2D_STRETCHBLIT:GE2D_STRETCHBLIT_NOALPHA;
		}
	}
	
	if(para->enable_key)
	{
		AM_OSD_Color_t c;
		uint32_t color;
		
		AM_RGB_PIXEL_TO_COLOR(src->format, para->key, &c);
		
		color = (c.r<<24)|(c.g<<16)|(c.b<<8)|c.a;
		conf.src_key.key_enable = 1;
		conf.src_key.key_color  = color;
		conf.src_key.key_mask   = 0xffffffff;
	}

	/*AM_DEBUG(1, "GE2D BLIT(%d) %d %d %d %d ->(%d) %d %d %d %d, cmd:0x%x, op:0x%x",
			src->format->type, dsr.x, dsr.y, dsr.w, dsr.h,
			dst->format->type, ddr.x, ddr.y, ddr.w, ddr.h,
			cmd, op.op);*/

	
	if(ioctl(ge2d_fd, GE2D_CONFIG, &conf)==-1)
	{
		AM_DEBUG(1, "ge2d config failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	if(ioctl(ge2d_fd, GE2D_SRCCOLORKEY, &conf)==-1)
	{
		AM_DEBUG(1, "ge2d config failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	if(ioctl(ge2d_fd, cmd, &op)==-1)
	{
		AM_DEBUG(1, "ge2d blit failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

#ifndef ANDROID
#ifdef AMSTREAM_IOC_GET_VBUF

/**\brief Blit a YUV frame to a surface*/

AM_ErrorCode_t ge2d_blit_yuv (struct vbuf_info *info, AM_OSD_Surface_t *dst)
{
	config_para_t conf;
	ge2d_para_t   op;
	int cmd;
	
	memset(&conf, 0, sizeof(conf));
	memset(&op, 0, sizeof(op));
	
	conf.src_planes[0].addr = info->y_addr;
	conf.src_planes[0].w    = info->y_width;
	conf.src_planes[0].h    = info->y_height;
	conf.src_planes[1].addr = info->u_addr;
	conf.src_planes[1].w    = info->u_width;
	conf.src_planes[1].h    = info->u_height;
	conf.src_planes[2].addr = info->v_addr;
	conf.src_planes[2].w    = info->v_width;
	conf.src_planes[2].h    = info->v_height;
	
	switch(info->type)
	{
		case VBUF_YUV_420:
			conf.src_format = GE2D_FORMAT_M24_YUV420;
		break;
		case VBUF_YUV_422:
			conf.src_format = GE2D_FORMAT_M24_YUV422;
		break;
		case VBUF_YUV_444:
			conf.src_format = GE2D_FORMAT_M24_YUV444;
		break;
		default:
			AM_DEBUG(1, "unknown frame type");
			return AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	ge2d_format(dst, &conf.dst_format, conf.dst_planes);
	
	conf.src_canvas_mode = 1;
	conf.src_dst_type = ALLOC_ALLOC;
	
	op.src1_rect.x = 0;
	op.src1_rect.y = 0;
	op.src1_rect.w = info->width;
	op.src1_rect.h = info->height;
	
	op.dst_rect.x = 0;
	op.dst_rect.y = 0;
	op.dst_rect.w = dst->width;
	op.dst_rect.h = dst->height;
	
	cmd = GE2D_STRETCHBLIT_NOALPHA;
	
	if(ioctl(ge2d_fd, GE2D_CONFIG, &conf)==-1)
	{
		AM_DEBUG(1, "ge2d config failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	if(ioctl(ge2d_fd, GE2D_SRCCOLORKEY, &conf)==-1)
	{
		AM_DEBUG(1, "ge2d config failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	if(ioctl(ge2d_fd, cmd, &op)==-1)
	{
		AM_DEBUG(1, "ge2d blit failed \"%s\"", strerror(errno));
		return AM_OSD_ERR_SYS;
	}
	
	return AM_SUCCESS;
}
#endif
#endif

static AM_ErrorCode_t ge2d_deinit (void)
{
	close(ge2d_fd);
	CMEM_exit();
	return AM_SUCCESS;
}

AM_ErrorCode_t ge2d_set_coef(int type)
{
	ioctl(ge2d_fd, GE2D_SET_COEF, type);
	return AM_SUCCESS;
}

