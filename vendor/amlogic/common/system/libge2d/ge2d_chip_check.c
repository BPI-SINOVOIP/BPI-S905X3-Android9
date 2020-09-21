/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>  /* for gettimeofday() */
#include "ge2d_port.h"
#include "ge2d_com.h"
#include "aml_ge2d.h"
#include "color_bar.h"



/* #define FILE_DATA */
#define SRC_FILE_NAME   ("/system/bin/fb.raw")
#define DST_FILE_NAME   ("/system/bin/out1.rgb32")
#define RESULT1_FILE_NAME   ("/system/bin/rotate-180.rgb32")
#define RESULT2_FILE_NAME   ("/system/bin/blend.rgb32")

static int SX_SRC1 = 640;
static int SY_SRC1 = 480;
static int SX_SRC2 = 640;
static int SY_SRC2 = 480;
static int SX_DST = 640;
static int SY_DST = 480;


static int SRC1_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int DST_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;

static int OP = AML_GE2D_BLIT;
static int src1_layer_mode = LAYER_MODE_PREMULTIPLIED;
static int src2_layer_mode = 0;
static int gb1_alpha = 0xff;
static int gb2_alpha = 0xff;
extern aml_ge2d_t amlge2d;


static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo)
{
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].canvas_w = SX_SRC2;
    pge2dinfo->src_info[1].canvas_h = SY_SRC2;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;

    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = OP;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
}

static void print_usage(void)
{
    int i;
    printf ("Usage: ge2d_chip_check [options]\n\n");
    printf ("Options:\n\n");
    printf ("  --op <0:fillrect, 1:blend, 2:strechblit, 3:blit>    ge2d operation case.\n");
    printf ("  --help                       Print usage information.\n");
    printf ("\n");
}

static int parse_command_line(int argc, char *argv[])
{
    int i;
    /* parse command line */
    for (i = 1; i < argc; i++) {
        if (strncmp (argv[i], "--", 2) == 0) {
            if (strcmp (argv[i] + 2, "help") == 0) {
                print_usage();
                return ge2d_fail;
            }
            else if (strcmp (argv[i] + 2, "op") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &OP) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "bo1") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src1_layer_mode) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "bo2") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src2_layer_mode) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "gb1") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &gb1_alpha) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "gb2") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &gb2_alpha) == 1) {
                continue;
            }
        }
    }
    return ge2d_success;
}

int compare_data(char *data1, char *data2, int size)
{
	int i = 0, err_hit = 0;
	int thresh_hold = 1;//size * 5/100;

	if (!data1 || !data2 || !size)
		return 0;
	for (i=0; i< size; i++) {
		if (*data1 != *data2) {
			printf("data1=%x,data2=%x\n",*data1,*data2);
			err_hit++;
		}
		data1++;
		data2++;
		if (err_hit > thresh_hold) {
			printf("bad chip found,err_hit=%d\n",err_hit);
			return -1;
		}
	}
	return 0;
}

int load_result_file(const char* url)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    if (amlge2d.src_size == 0)
        return 0;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.src_data = (char*)malloc(amlge2d.src_size);
    if (!amlge2d.src_data) {
        E_GE2D("malloc for src_data failed\n");
        return ge2d_fail;
    }

    read_num = read(fd,amlge2d.src_data,amlge2d.src_size);
    if (read_num <= 0) {
        E_GE2D("read file read_num=%d error\n",read_num);
        return ge2d_fail;
    }

    close(fd);
    return ge2d_success;
}


int aml_read_file_src1(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    if (amlge2d.src_size == 0)
        return 0;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.src_data = (char*)malloc(amlge2d.src_size);
    if (!amlge2d.src_data) {
        E_GE2D("malloc for src_data failed\n");
        return ge2d_fail;
    }

    read_num = read(fd,amlge2d.src_data,amlge2d.src_size);
    if (read_num <= 0) {
        E_GE2D("read file read_num=%d error\n",read_num);
        return ge2d_fail;
    }

    memcpy(pge2dinfo->src_info[0].vaddr, amlge2d.src_data, amlge2d.src_size);
#if 0
    printf("pixel: 0x%2x, 0x%2x,0x%2x,0x%2x, 0x%2x,0x%2x,0x%2x,0x%2x\n",
        pge2dinfo->src_info[0].vaddr[0],
        pge2dinfo->src_info[0].vaddr[1],
        pge2dinfo->src_info[0].vaddr[2],
        pge2dinfo->src_info[0].vaddr[3],
        pge2dinfo->src_info[0].vaddr[4],
        pge2dinfo->src_info[0].vaddr[5],
        pge2dinfo->src_info[0].vaddr[6],
        pge2dinfo->src_info[0].vaddr[7]);
#endif

    close(fd);
    return ge2d_success;
}

int aml_read_file_src2(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    if (amlge2d.src2_size == 0)
        return 0;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.src2_data = (char*)malloc(amlge2d.src2_size);
    if (!amlge2d.src2_data) {
        E_GE2D("malloc for src_data failed\n");
        return ge2d_fail;
    }

    read_num = read(fd,amlge2d.src2_data,amlge2d.src2_size);
    if (read_num <= 0) {
        E_GE2D("read file read_num=%d error\n",read_num);
        return ge2d_fail;
    }

    memcpy(pge2dinfo->src_info[1].vaddr, amlge2d.src2_data, amlge2d.src2_size);
    close(fd);
    return ge2d_success;
}

int aml_write_file(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int write_num = 0;
    unsigned int *value;

    if (amlge2d.dst_size == 0)
        return 0;
    if ((GE2D_CANVAS_OSD0 == pge2dinfo->dst_info.memtype)
        || (GE2D_CANVAS_OSD1 == pge2dinfo->dst_info.memtype))
        return 0;

    fd = open(url,O_RDWR | O_CREAT,0660);
    if (fd < 0) {
        E_GE2D("write file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.dst_data = (char*)malloc(amlge2d.dst_size);
    if (!amlge2d.dst_data) {
        E_GE2D("malloc for dst_data failed\n");
        return ge2d_fail;
    }

    memcpy(amlge2d.dst_data,pge2dinfo->dst_info.vaddr,amlge2d.dst_size);

    #if 0
    printf("dst pixel: 0x%2x, 0x%2x,0x%2x,0x%2x, 0x%2x,0x%2x,0x%2x,0x%2x\n",
        pge2dinfo->dst_info.vaddr[0],
        pge2dinfo->dst_info.vaddr[1],
        pge2dinfo->dst_info.vaddr[2],
        pge2dinfo->dst_info.vaddr[3],
        pge2dinfo->dst_info.vaddr[4],
        pge2dinfo->dst_info.vaddr[5],
        pge2dinfo->dst_info.vaddr[6],
        pge2dinfo->dst_info.vaddr[7]);
	#endif
    write_num = write(fd,amlge2d.dst_data,amlge2d.dst_size);
    if (write_num <= 0) {
        E_GE2D("read file read_num=%d error\n",write_num);
    }
    close(fd);
    return ge2d_success;
}

static int do_blit(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code = 0;
    ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return ge2d_fail;

    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;

    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;

    pge2dinfo->dst_info.rotation = GE2D_ROTATION_180;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;

    ret = aml_ge2d_process(pge2dinfo);

    return ret;
}

static int do_strechblit(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code = 0;

    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;

    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = SX_SRC1;
    pge2dinfo->src_info[0].rect.h = SY_SRC1;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = SX_DST;
    pge2dinfo->dst_info.rect.h = SY_DST;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_180;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;

    ret = aml_ge2d_process(pge2dinfo);

    return ret;

}

static int do_blend(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code = 0;
    int shared_fd_bakup;
    unsigned long offset_bakup = 0;

    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
    /* do blend src1 blend src2(dst) to dst */

    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->src_info[0].fill_color_en = 0;

    pge2dinfo->src_info[1].canvas_w = SX_SRC2;
    pge2dinfo->src_info[1].canvas_h = SY_SRC2;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;
    pge2dinfo->src_info[1].rect.x = 0;
    pge2dinfo->src_info[1].rect.y = 0;
    pge2dinfo->src_info[1].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[1].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->src_info[1].fill_color_en = 0;
    if (src2_layer_mode == LAYER_MODE_NON)
        pge2dinfo->src_info[0].format = PIXEL_FORMAT_RGBX_8888;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
    pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
    ret = aml_ge2d_process(pge2dinfo);

    return ret;
}

static inline unsigned long myclock()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


int aml_generate_src1(char *src, aml_ge2d_info_t *pge2dinfo)
{
    if (amlge2d.src_size == 0)
        return 0;
    memcpy(pge2dinfo->src_info[0].vaddr, src, amlge2d.src_size);
    return ge2d_success;
}

int aml_generate_src2(int color, aml_ge2d_info_t *pge2dinfo)
{
    int i = 0;

    if (amlge2d.src2_size == 0)
        return 0;

	for (i=0;i<amlge2d.src2_size;) {
		pge2dinfo->src_info[1].vaddr[i] = color & 0xff;
		pge2dinfo->src_info[1].vaddr[i+1] = (color & 0xff00)>>8;
		pge2dinfo->src_info[1].vaddr[i+2] = (color & 0xff0000)>>16;
		pge2dinfo->src_info[1].vaddr[i+3] = (color & 0xff000000)>>24;
		i+=4;
	}
    return ge2d_success;
}

int main(int argc, char **argv)
{
    int ret = -1;
    int i = 0;
    unsigned long stime,etime;
    aml_ge2d_info_t ge2dinfo;
    memset(&amlge2d,0x0,sizeof(aml_ge2d_t));
    amlge2d.pge2d_info = &ge2dinfo;
    memset(&ge2dinfo, 0, sizeof(aml_ge2d_info_t));
    memset(&(ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(ge2dinfo.dst_info), 0, sizeof(buffer_info_t));
    ret = parse_command_line(argc,argv);
    if (ret == ge2d_fail)
        return ge2d_success;

    set_ge2dinfo(&ge2dinfo);

    ret = aml_ge2d_init();
    if (ret < 0)
        return ge2d_fail;

    ret = aml_ge2d_mem_alloc(&ge2dinfo);
    if (ret < 0)
        goto exit;

    printf("Start GE2D chip check...\n");
    stime = myclock();
    aml_generate_src1(color_bar_code,&ge2dinfo);

    ge2dinfo.ge2d_op = AML_GE2D_STRETCHBLIT;
    do_strechblit(&ge2dinfo);
    ret = compare_data(color_bar_rotate, ge2dinfo.dst_info.vaddr, amlge2d.dst_size);
    if (ret < 0)
        printf("GE2D: strechbilt + rotate 180 [FAILED]\n");
    else {
        printf("GE2D: strechbilt + rotate 180 [PASSED]\n");
        aml_generate_src1(color_bar_rotate,&ge2dinfo);
        aml_generate_src2(0x40000000,&ge2dinfo);
        ge2dinfo.ge2d_op = AML_GE2D_BLEND;
        do_blend(&ge2dinfo);
        ret = compare_data(color_bar_blend, ge2dinfo.dst_info.vaddr, amlge2d.dst_size);
        if (ret < 0)
            printf("GE2D: blend [FAILED]\n");
        else
            printf("GE2D: blend [PASSED]\n");
    }
    etime = myclock();
    D_GE2D("used time %d ms\n",etime - stime);
    if (ret < 0)
        printf("GE2D:bad chip found!\n");
    else
        printf("GE2D:chip work fine!\n");
exit:
    if (amlge2d.src_data) {
        free(amlge2d.src_data);
        amlge2d.src_data = NULL;
    }
    if (amlge2d.src2_data) {
        free(amlge2d.src2_data);
        amlge2d.src2_data = NULL;
    }
    if (amlge2d.dst_data) {
        free(amlge2d.dst_data);
        amlge2d.dst_data = NULL;
    }
    aml_ge2d_mem_free(&ge2dinfo);
    aml_ge2d_exit();
    return ge2d_success;
}
