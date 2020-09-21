/*
 * drivers/amlogic/amports/vmpeg4.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>

#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/tee.h>
#include <linux/amlogic/media/utils/vdec_reg.h>
#include <linux/amlogic/media/registers/register.h>
#include "../../../stream_input/amports/amports_priv.h"

#include "../utils/amvdec.h"
#include "../utils/vdec_input.h"
#include "../utils/vdec.h"
#include "../utils/firmware.h"
#include "../utils/decoder_mmu_box.h"
#include "../utils/decoder_bmmu_box.h"
#include <linux/amlogic/media/codec_mm/configs.h>
#include "../utils/firmware.h"
#include "../utils/vdec_v4l2_buffer_ops.h"

#define DRIVER_NAME "ammvdec_mpeg4"
#define MODULE_NAME "ammvdec_mpeg4"

#define MEM_NAME "codec_mmpeg4"

#define DEBUG_PTS

#define NV21
#define I_PICTURE   0
#define P_PICTURE   1
#define B_PICTURE   2
#define GET_PIC_TYPE(type) ("IPB####"[type&0x3])

#define ORI_BUFFER_START_ADDR   0x01000000
#define DEFAULT_MEM_SIZE	(32*SZ_1M)

#define INTERLACE_FLAG          0x80
#define TOP_FIELD_FIRST_FLAG    0x40

/* protocol registers */
#define MREG_REF0           AV_SCRATCH_1
#define MREG_REF1           AV_SCRATCH_2
#define MP4_PIC_RATIO       AV_SCRATCH_5
#define MP4_RATE            AV_SCRATCH_3
#define MP4_ERR_COUNT       AV_SCRATCH_6
#define MP4_PIC_WH          AV_SCRATCH_7
#define MREG_INPUT          AV_SCRATCH_8
#define MREG_BUFFEROUT      AV_SCRATCH_9
#define MP4_NOT_CODED_CNT   AV_SCRATCH_A
#define MP4_VOP_TIME_INC    AV_SCRATCH_B
#define MP4_OFFSET_REG      AV_SCRATCH_C
#define MP4_SYS_RATE        AV_SCRATCH_E
#define MEM_OFFSET_REG      AV_SCRATCH_F

#define PARC_FORBIDDEN              0
#define PARC_SQUARE                 1
#define PARC_CIF                    2
#define PARC_10_11                  3
#define PARC_16_11                  4
#define PARC_40_33                  5
#define PARC_RESERVED               6
/* values between 6 and 14 are reserved */
#define PARC_EXTENDED              15

#define VF_POOL_SIZE          16
#define DECODE_BUFFER_NUM_MAX 8
#define PUT_INTERVAL        (HZ/100)
#define MAX_BMMU_BUFFER_NUM (DECODE_BUFFER_NUM_MAX + 1)
#define WORKSPACE_SIZE		(12*SZ_64K)
static u32 buf_size = 32 * 1024 * 1024;

#define CTX_LMEM_SWAP_OFFSET    0
#define CTX_QUANT_MATRIX_OFFSET 0x800
/* dcac buffer must align at 4k boundary */
#define CTX_DCAC_BUF_OFFSET     0x1000
#define CTX_DECBUF_OFFSET       (0x0c0000 + 0x1000)

#define RATE_DETECT_COUNT   5
#define DURATION_UNIT       96000
#define PTS_UNIT            90000
#define CHECK_INTERVAL        (HZ/100)

#define DUR2PTS(x) ((x) - ((x) >> 4))

/* 96000/(60fps* 2field) = 800, 96000/10fps = 9600 */
#define MPEG4_VALID_DUR(x) ((x < 9600) && (x > 799))

#define MAX_MPEG4_SUPPORT_SIZE (1920*1088)

#define DEC_RESULT_NONE     0
#define DEC_RESULT_DONE     1
#define DEC_RESULT_AGAIN    2
#define DEC_RESULT_ERROR    3
#define DEC_RESULT_FORCE_EXIT 4
#define DEC_RESULT_EOS		5
#define DEC_RESULT_UNFINISH	6

#define DEC_DECODE_TIMEOUT         0x21
#define DECODE_ID(hw) (hw_to_vdec(hw)->id)
#define DECODE_STOP_POS         AV_SCRATCH_K
static u32 udebug_flag;

static struct vframe_s *vmpeg_vf_peek(void *);
static struct vframe_s *vmpeg_vf_get(void *);
static void vmpeg_vf_put(struct vframe_s *, void *);
static int vmpeg_vf_states(struct vframe_states *states, void *);
static int vmpeg_event_cb(int type, void *data, void *private_data);
static int pre_decode_buf_level = 0x800;
static int start_decode_buf_level = 0x4000;
static int debug_enable;
static unsigned int radr;
static unsigned int rval;
/* 0x40bit = 8byte */
static unsigned int frmbase_cont_bitlevel = 0x40;

#define VMPEG4_DEV_NUM        9
static unsigned int max_decode_instance_num = VMPEG4_DEV_NUM;
static unsigned int max_process_time[VMPEG4_DEV_NUM];
static unsigned int decode_timeout_val = 200;

static u32 without_display_mode;

#undef pr_info
#define pr_info printk
unsigned int mpeg4_debug_mask = 0xff;

#define PRINT_FLAG_ERROR              0x0
#define PRINT_FLAG_RUN_FLOW           0X0001
#define PRINT_FLAG_TIMEINFO           0x0002
#define PRINT_FLAG_UCODE_DETAIL	      0x0004
#define PRINT_FLAG_VLD_DETAIL         0x0008
#define PRINT_FLAG_DEC_DETAIL         0x0010
#define PRINT_FLAG_BUFFER_DETAIL      0x0020
#define PRINT_FLAG_RESTORE            0x0040
#define PRINT_FRAME_NUM               0x0080
#define PRINT_FLAG_FORCE_DONE         0x0100
#define PRINT_FLAG_COUNTER            0X0200
#define PRINT_FRAMEBASE_DATA          0x0400
#define PRINT_FLAG_VDEC_STATUS        0x0800
#define PRINT_FLAG_TIMEOUT_STATUS     0x1000
#define PRINT_FLAG_V4L_DETAIL         0x8000

int mmpeg4_debug_print(int index, int debug_flag, const char *fmt, ...)
{
	if (((debug_enable & debug_flag) &&
		((1 << index) & mpeg4_debug_mask))
		|| (debug_flag == PRINT_FLAG_ERROR)) {
		unsigned char buf[512];
		int len = 0;
		va_list args;
		va_start(args, fmt);
		len = sprintf(buf, "%d: ", index);
		vsnprintf(buf + len, 512-len, fmt, args);
		pr_info("%s", buf);
		va_end(args);
	}
	return 0;
}

struct pic_info_t {
	int index;
	u32 pic_type;
	u32 pic_info;
	u32 pts;
	u64 pts64;
	bool pts_valid;
	u32 duration;
	u32 repeat_cnt;
	ulong v4l_ref_buf_addr;
};

struct vdec_mpeg4_hw_s {
	spinlock_t lock;
	struct platform_device *platform_dev;
	/* struct device *cma_dev; */

	DECLARE_KFIFO(newframe_q, struct vframe_s *, VF_POOL_SIZE);
	DECLARE_KFIFO(display_q, struct vframe_s *, VF_POOL_SIZE);
	struct vframe_s vfpool[VF_POOL_SIZE];

	s32 vfbuf_use[DECODE_BUFFER_NUM_MAX];
	u32 frame_width;
	u32 frame_height;
	u32 frame_dur;
	u32 frame_prog;

	u32 ctx_valid;
	u32 reg_vcop_ctrl_reg;
	u32 reg_pic_head_info;
	u32 reg_mpeg1_2_reg;
	u32 reg_slice_qp;
	u32 reg_mp4_pic_wh;
	u32 reg_mp4_rate;
	u32 reg_mb_info;
	u32 reg_dc_ac_ctrl;
	u32 reg_iqidct_control;
	u32 reg_resync_marker_length;
	u32 reg_rv_ai_mb_count;
	struct timer_list check_timer;
	u32 decode_timeout_count;
	u32 timeout_cnt;
	unsigned long int start_process_time;

	u32 last_vld_level;
	u8 init_flag;
	u32 eos;
	void *mm_blk_handle;

	struct vframe_chunk_s *chunk;
	u32 chunk_offset;
	u32 chunk_size;
	u32 chunk_frame_count;
	u32 stat;
	unsigned long buf_start;
	u32 buf_size;
	/*
	unsigned long cma_alloc_addr;
	int cma_alloc_count;
	*/
	u32 vmpeg4_ratio;
	u64 vmpeg4_ratio64;
	u32 rate_detect;
	u32 vmpeg4_rotation;
	u32 total_frame;
	u32 last_vop_time_inc;
	u32 last_duration;
	u32 last_anch_pts;
	u32 vop_time_inc_since_last_anch;
	u32 frame_num_since_last_anch;
	u64 last_anch_pts_us64;

	u32 last_pts;
	u64 last_pts64;
	u32 pts_hit;
	u32 pts_missed;
	u32 pts_i_hit;
	u32 pts_i_missed;
	struct pic_info_t pic[DECODE_BUFFER_NUM_MAX];
	u32 canvas_spec[DECODE_BUFFER_NUM_MAX];
#ifdef NV21
	struct canvas_config_s canvas_config[DECODE_BUFFER_NUM_MAX][2];
#else
	struct canvas_config_s canvas_config[DECODE_BUFFER_NUM_MAX][3];
#endif
	struct dec_sysinfo vmpeg4_amstream_dec_info;

	s32 refs[2];
	int dec_result;
	struct work_struct work;

	void (*vdec_cb)(struct vdec_s *, void *);
	void *vdec_cb_arg;
	u32 frame_num;
	u32 put_num;
	u32 sys_mp4_rate;
	u32 run_count;
	u32	not_run_ready;
	u32 buffer_not_ready;
	u32	input_empty;
	u32 peek_num;
	u32 get_num;
	u32 first_i_frame_ready;
	u32 drop_frame_count;
	u32 unstable_pts;
	u32 last_dec_pts;

	struct firmware_s *fw;
	u32 blkmode;
	wait_queue_head_t wait_q;
	bool is_used_v4l;
	void *v4l2_ctx;
	bool v4l_params_parsed;
};
static void vmpeg4_local_init(struct vdec_mpeg4_hw_s *hw);
static int vmpeg4_hw_ctx_restore(struct vdec_mpeg4_hw_s *hw);
static unsigned char
	get_data_check_sum(struct vdec_mpeg4_hw_s *hw, int size);

#define PROVIDER_NAME   "vdec.mpeg4"

/*
 *int query_video_status(int type, int *value);
 */
static const struct vframe_operations_s vf_provider_ops = {
	.peek = vmpeg_vf_peek,
	.get = vmpeg_vf_get,
	.put = vmpeg_vf_put,
	.event_cb = vmpeg_event_cb,
	.vf_states = vmpeg_vf_states,
};

static unsigned char aspect_ratio_table[16] = {
	PARC_FORBIDDEN,
	PARC_SQUARE,
	PARC_CIF,
	PARC_10_11,
	PARC_16_11,
	PARC_40_33,
	PARC_RESERVED, PARC_RESERVED, PARC_RESERVED, PARC_RESERVED,
	PARC_RESERVED, PARC_RESERVED, PARC_RESERVED, PARC_RESERVED,
	PARC_RESERVED, PARC_EXTENDED
};

static void reset_process_time(struct vdec_mpeg4_hw_s *hw);

static int vmpeg4_v4l_alloc_buff_config_canvas(struct vdec_mpeg4_hw_s *hw, int i)
{
	int ret;
	u32 canvas;
	ulong decbuf_start = 0;
	int decbuf_y_size = 0;
	u32 canvas_width = 0, canvas_height = 0;
	struct vdec_s *vdec = hw_to_vdec(hw);
	struct vdec_v4l2_buffer *fb = NULL;

	if (hw->pic[i].v4l_ref_buf_addr)
		return 0;

	ret = vdec_v4l_get_buffer(hw->v4l2_ctx, &fb);
	if (ret) {
		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"[%d] get fb fail.\n",
			((struct aml_vcodec_ctx *)
			(hw->v4l2_ctx))->id);
		return ret;
	}

	hw->pic[i].v4l_ref_buf_addr = (ulong)fb;
	if (fb->num_planes == 1) {
		decbuf_start	= fb->m.mem[0].addr;
		decbuf_y_size	= fb->m.mem[0].offset;
		canvas_width	= ALIGN(hw->frame_width, 64);
		canvas_height	= ALIGN(hw->frame_height, 32);
		fb->m.mem[0].bytes_used = fb->m.mem[0].size;
	} else if (fb->num_planes == 2) {
		decbuf_start	= fb->m.mem[0].addr;
		decbuf_y_size	= fb->m.mem[0].size;
		canvas_width	= ALIGN(hw->frame_width, 64);
		canvas_height	= ALIGN(hw->frame_height, 32);
		fb->m.mem[0].bytes_used = decbuf_y_size;
		fb->m.mem[1].bytes_used = decbuf_y_size << 1;
	}

	mmpeg4_debug_print(DECODE_ID(hw), 0, "[%d] %s(), v4l ref buf addr: 0x%x\n",
		((struct aml_vcodec_ctx *)(hw->v4l2_ctx))->id, __func__, fb);

	if (vdec->parallel_dec == 1) {
		u32 tmp;
		if (canvas_y(hw->canvas_spec[i]) == 0xff) {
			tmp = vdec->get_canvas_ex(CORE_MASK_VDEC_1, vdec->id);
			hw->canvas_spec[i] &= ~0xff;
			hw->canvas_spec[i] |= tmp;
		}
		if (canvas_u(hw->canvas_spec[i]) == 0xff) {
			tmp = vdec->get_canvas_ex(CORE_MASK_VDEC_1, vdec->id);
			hw->canvas_spec[i] &= ~(0xffff << 8);
			hw->canvas_spec[i] |= tmp << 8;
			hw->canvas_spec[i] |= tmp << 16;
		}
		canvas = hw->canvas_spec[i];
	} else {
		canvas = vdec->get_canvas(i, 2);
		hw->canvas_spec[i] = canvas;
	}

	hw->canvas_config[i][0].phy_addr = decbuf_start;
	hw->canvas_config[i][0].width = canvas_width;
	hw->canvas_config[i][0].height = canvas_height;
	hw->canvas_config[i][0].block_mode = hw->blkmode;
	if (hw->blkmode == CANVAS_BLKMODE_LINEAR)
		hw->canvas_config[i][0].endian = 7;
	else
		hw->canvas_config[i][0].endian = 0;
	canvas_config_config(canvas_y(canvas),
			&hw->canvas_config[i][0]);

	hw->canvas_config[i][1].phy_addr =
		decbuf_start + decbuf_y_size;
	hw->canvas_config[i][1].width = canvas_width;
	hw->canvas_config[i][1].height = (canvas_height >> 1);
	hw->canvas_config[i][1].block_mode = hw->blkmode;
	if (hw->blkmode == CANVAS_BLKMODE_LINEAR)
		hw->canvas_config[i][1].endian = 7;
	else
		hw->canvas_config[i][1].endian = 0;
	canvas_config_config(canvas_u(canvas),
			&hw->canvas_config[i][1]);

	return 0;
}

static bool is_enough_free_buffer(struct vdec_mpeg4_hw_s *hw)
{
	int i;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		if (hw->vfbuf_use[i] == 0)
			break;
	}

	return i == DECODE_BUFFER_NUM_MAX ? false : true;
}

static int find_free_buffer(struct vdec_mpeg4_hw_s *hw)
{
	int i;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		if (hw->vfbuf_use[i] == 0)
			break;
	}

	if (i == DECODE_BUFFER_NUM_MAX)
		return -1;

	if (hw->is_used_v4l)
		if (vmpeg4_v4l_alloc_buff_config_canvas(hw, i))
			return -1;

	return i;
}

static int spec_to_index(struct vdec_mpeg4_hw_s *hw, u32 spec)
{
	int i;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		if (hw->canvas_spec[i] == spec)
			return i;
	}

	return -1;
}

static void set_frame_info(struct vdec_mpeg4_hw_s *hw, struct vframe_s *vf,
			int buffer_index)
{
	int ar = 0;
	unsigned int num = 0;
	unsigned int den = 0;
	unsigned int pixel_ratio = READ_VREG(MP4_PIC_RATIO);

	if (hw->vmpeg4_ratio64 != 0) {
		num = hw->vmpeg4_ratio64>>32;
		den = hw->vmpeg4_ratio64 & 0xffffffff;
	} else {
		num = hw->vmpeg4_ratio>>16;
		den = hw->vmpeg4_ratio & 0xffff;

	}
	if ((num == 0) || (den == 0)) {
		num = 1;
		den = 1;
	}

	if (hw->vmpeg4_ratio == 0) {
		vf->ratio_control |= (0x90 << DISP_RATIO_ASPECT_RATIO_BIT);
		/* always stretch to 16:9 */
	} else if (pixel_ratio > 0x0f) {
		num = (pixel_ratio >> 8) *
			hw->frame_width * num;
		ar = div_u64((pixel_ratio & 0xff) *
			hw->frame_height * den * 0x100ULL +
			(num >> 1), num);
	} else {
		switch (aspect_ratio_table[pixel_ratio]) {
		case 0:
			num = hw->frame_width * num;
			ar = (hw->frame_height * den *
				0x100 + (num >> 1)) / num;
			break;
		case 1:
			num = vf->width * num;
			ar = (vf->height * den * 0x100 + (num >> 1)) / num;
			break;
		case 2:
			num = (vf->width * 12) * num;
			ar = (vf->height * den * 0x100 * 11 +
				  ((num) >> 1)) / num;
			break;
		case 3:
			num = (vf->width * 10) * num;
			ar = (vf->height * den * 0x100 * 11 + (num >> 1)) /
				num;
			break;
		case 4:
			num = (vf->width * 16) * num;
			ar = (vf->height * den * 0x100 * 11 + (num >> 1)) /
				num;
			break;
		case 5:
			num = (vf->width * 40) * num;
			ar = (vf->height * den * 0x100 * 33 + (num >> 1)) /
				num;
			break;
		default:
			num = vf->width * num;
			ar = (vf->height * den * 0x100 + (num >> 1)) / num;
			break;
		}
	}

	ar = min(ar, DISP_RATIO_ASPECT_RATIO_MAX);

	vf->signal_type = 0;
	vf->type_original = vf->type;
	vf->ratio_control = (ar << DISP_RATIO_ASPECT_RATIO_BIT);
	vf->canvas0Addr = vf->canvas1Addr = -1;
#ifdef NV21
	vf->plane_num = 2;
#else
	vf->plane_num = 3;
#endif
	vf->canvas0_config[0] = hw->canvas_config[buffer_index][0];
	vf->canvas0_config[1] = hw->canvas_config[buffer_index][1];
#ifndef NV21
	vf->canvas0_config[2] = hw->canvas_config[buffer_index][2];
#endif
	vf->canvas1_config[0] = hw->canvas_config[buffer_index][0];
	vf->canvas1_config[1] = hw->canvas_config[buffer_index][1];
#ifndef NV21
	vf->canvas1_config[2] = hw->canvas_config[buffer_index][2];
#endif
}

static inline void vmpeg4_save_hw_context(struct vdec_mpeg4_hw_s *hw)
{
	hw->reg_mpeg1_2_reg = READ_VREG(MPEG1_2_REG);
	hw->reg_vcop_ctrl_reg = READ_VREG(VCOP_CTRL_REG);
	hw->reg_pic_head_info = READ_VREG(PIC_HEAD_INFO);
	hw->reg_slice_qp = READ_VREG(SLICE_QP);
	hw->reg_mp4_pic_wh = READ_VREG(MP4_PIC_WH);
	hw->reg_mp4_rate = READ_VREG(MP4_RATE);
	hw->reg_mb_info = READ_VREG(MB_INFO);
	hw->reg_dc_ac_ctrl = READ_VREG(DC_AC_CTRL);
	hw->reg_iqidct_control = READ_VREG(IQIDCT_CONTROL);
	hw->reg_resync_marker_length = READ_VREG(RESYNC_MARKER_LENGTH);
	hw->reg_rv_ai_mb_count = READ_VREG(RV_AI_MB_COUNT);
}

static int update_ref(struct vdec_mpeg4_hw_s *hw, int index)
{
	hw->vfbuf_use[index]++;

	if (hw->refs[1] == -1) {
		hw->refs[1] = index;
		index = -1;
	} else if (hw->refs[0] == -1) {
		hw->refs[0] = hw->refs[1];
		hw->refs[1] = index;
		index = hw->refs[0];
	} else {
		hw->vfbuf_use[hw->refs[0]]--;
		hw->refs[0] = hw->refs[1];
		hw->refs[1] = index;
		index = hw->refs[0];
	}

	return index;
}

static int prepare_display_buf(struct vdec_mpeg4_hw_s * hw,
	struct pic_info_t *pic)
{
	struct vframe_s *vf = NULL;
	struct vdec_s *vdec = hw_to_vdec(hw);
	int index = pic->index;

	if (pic->pic_info & INTERLACE_FLAG) {
		if (kfifo_get(&hw->newframe_q, &vf) == 0) {
			mmpeg4_debug_print(DECODE_ID(hw), 0,
				"fatal error, no available buffer slot.");
			return -1;
		}

		if (hw->is_used_v4l) {
			vf->v4l_mem_handle
				= hw->pic[index].v4l_ref_buf_addr;
			mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_V4L_DETAIL,
				"[%d] %s(), v4l mem handle: 0x%lx\n",
				((struct aml_vcodec_ctx *)(hw->v4l2_ctx))->id,
				__func__, vf->v4l_mem_handle);
		}

		vf->index = pic->index;
		vf->width = hw->frame_width;
		vf->height = hw->frame_height;
		vf->bufWidth = 1920;
		vf->flag = 0;
		vf->orientation = hw->vmpeg4_rotation;
		vf->pts = pic->pts;
		vf->pts_us64 = pic->pts64;
		vf->duration = pic->duration >> 1;
		vf->duration_pulldown = 0;
		vf->type = (pic->pic_info & TOP_FIELD_FIRST_FLAG) ?
			VIDTYPE_INTERLACE_TOP : VIDTYPE_INTERLACE_BOTTOM;
#ifdef NV21
		vf->type |= VIDTYPE_VIU_NV21;
#endif
		set_frame_info(hw, vf, pic->index);

		hw->vfbuf_use[pic->index]++;
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
			"field0: pts %d, pts64 %lld, w %d, h %d, dur %d\n",
			vf->pts, vf->pts_us64, vf->width, vf->height, vf->duration);

		if ((hw->first_i_frame_ready == 0)
			 && (pic->pic_type != I_PICTURE)) {
			hw->drop_frame_count++;
			hw->vfbuf_use[index]--;
			kfifo_put(&hw->newframe_q,
				(const struct vframe_s *)vf);
			return 0;
		} else {
			vf->mem_handle =
				decoder_bmmu_box_get_mem_handle(
					hw->mm_blk_handle, index);
			kfifo_put(&hw->display_q,
				(const struct vframe_s *)vf);
			ATRACE_COUNTER(MODULE_NAME, vf->pts);
			vdec->vdec_fps_detec(vdec->id);
			hw->frame_num++;
			if (without_display_mode == 0) {
				vf_notify_receiver(vdec->vf_provider_name,
					VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			} else
				vmpeg_vf_put(vmpeg_vf_get(vdec), vdec);
		}

		if (kfifo_get(&hw->newframe_q, &vf) == 0) {
			mmpeg4_debug_print(DECODE_ID(hw), 0,
				"error, no available buf.\n");
			hw->dec_result = DEC_RESULT_ERROR;
			return -1;
		}

		vf->index = pic->index;
		vf->width = hw->frame_width;
		vf->height = hw->frame_height;
		vf->bufWidth = 1920;
		vf->flag = 0;
		vf->orientation = hw->vmpeg4_rotation;
		vf->pts = 0;
		vf->pts_us64 = 0;
		vf->duration = pic->duration >> 1;
		vf->duration_pulldown = 0;
		vf->type = (pic->pic_info & TOP_FIELD_FIRST_FLAG) ?
			VIDTYPE_INTERLACE_BOTTOM : VIDTYPE_INTERLACE_TOP;
#ifdef NV21
		vf->type |= VIDTYPE_VIU_NV21;
#endif
		set_frame_info(hw, vf, pic->index);

		hw->vfbuf_use[pic->index]++;
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
			"filed1: pts %d, pts64 %lld, w %d, h %d, dur: %d\n",
			vf->pts, vf->pts_us64, vf->width, vf->height, vf->duration);
		if ((hw->first_i_frame_ready == 0) &&
			(pic->pic_type != I_PICTURE)) {
			hw->drop_frame_count++;
			hw->vfbuf_use[index]--;
			kfifo_put(&hw->newframe_q,
				(const struct vframe_s *)vf);
		} else {
			vf->mem_handle =
				decoder_bmmu_box_get_mem_handle(
					hw->mm_blk_handle, index);
			kfifo_put(&hw->display_q,
				(const struct vframe_s *)vf);
			ATRACE_COUNTER(MODULE_NAME, vf->pts);
			vdec->vdec_fps_detec(vdec->id);
			decoder_do_frame_check(vdec, vf);
			hw->frame_num++;
			if (without_display_mode == 0) {
				vf_notify_receiver(vdec->vf_provider_name,
					VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			} else
				vmpeg_vf_put(vmpeg_vf_get(vdec), vdec);
		}
	} else {
		/* progressive */
		if (kfifo_get(&hw->newframe_q, &vf) == 0) {
			mmpeg4_debug_print(DECODE_ID(hw), 0,
				"error, no available buf\n");
			hw->dec_result = DEC_RESULT_ERROR;
			return -1;
		}

		if (hw->is_used_v4l) {
			vf->v4l_mem_handle
				= hw->pic[index].v4l_ref_buf_addr;
			if (vdec_v4l_binding_fd_and_vf(vf->v4l_mem_handle, vf) < 0) {
				mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_V4L_DETAIL,
					"v4l: binding vf fail.\n");
				return -1;
			}
			mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_V4L_DETAIL,
				"[%d] %s(), v4l mem handle: 0x%lx\n",
				((struct aml_vcodec_ctx *)(hw->v4l2_ctx))->id,
				__func__, vf->v4l_mem_handle);
		}

		vf->index = index;
		vf->width = hw->frame_width;
		vf->height = hw->frame_height;
		vf->bufWidth = 1920;
		vf->flag = 0;
		vf->orientation = hw->vmpeg4_rotation;
		vf->pts = pic->pts;
		vf->pts_us64 = pic->pts64;
		vf->duration = pic->duration;
		vf->duration_pulldown = pic->repeat_cnt *
			pic->duration;
#ifdef NV21
		vf->type = VIDTYPE_PROGRESSIVE |
			VIDTYPE_VIU_FIELD | VIDTYPE_VIU_NV21;
#else
		vf->type = VIDTYPE_PROGRESSIVE |
			VIDTYPE_VIU_FIELD;
#endif
		set_frame_info(hw, vf, index);

		hw->vfbuf_use[index]++;
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
			"prog: pts %d, pts64 %lld, w %d, h %d, dur %d\n",
			vf->pts, vf->pts_us64, vf->width, vf->height, vf->duration);

		if ((hw->first_i_frame_ready == 0) &&
			(pic->pic_type != I_PICTURE)) {
			hw->drop_frame_count++;
			hw->vfbuf_use[index]--;
			kfifo_put(&hw->newframe_q,
				(const struct vframe_s *)vf);
		} else {
			vf->mem_handle =
				decoder_bmmu_box_get_mem_handle(
					hw->mm_blk_handle, index);
			kfifo_put(&hw->display_q,
				(const struct vframe_s *)vf);
			ATRACE_COUNTER(MODULE_NAME, vf->pts);
			vdec->vdec_fps_detec(vdec->id);
			decoder_do_frame_check(vdec, vf);
			hw->frame_num++;

			if (without_display_mode == 0) {
				vf_notify_receiver(vdec->vf_provider_name,
					VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			} else
				vmpeg_vf_put(vmpeg_vf_get(vdec), vdec);
		}

	}
	return 0;
}

static void vmpeg4_prepare_input(struct vdec_mpeg4_hw_s *hw)
{
	struct vdec_s *vdec = hw_to_vdec(hw);
	struct vdec_input_s *input = &vdec->input;
	struct vframe_block_list_s *block = NULL;
	struct vframe_chunk_s *chunk = hw->chunk;
	int dummy;

	if (chunk == NULL)
		return;

	/* full reset to HW input */
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 0);

	/* reset VLD fifo for all vdec */
	WRITE_VREG(DOS_SW_RESET0, (1<<5) | (1<<4) | (1<<3));
	WRITE_VREG(DOS_SW_RESET0, 0);

	dummy = READ_RESET_REG(RESET0_REGISTER);
	WRITE_VREG(POWER_CTL_VLD, 1 << 4);

	/*
	 *setup HW decoder input buffer (VLD context)
	 * based on input->type and input->target
	 */
	if (input_frame_based(input)) {
		block = chunk->block;

		WRITE_VREG(VLD_MEM_VIFIFO_START_PTR, block->start);
		WRITE_VREG(VLD_MEM_VIFIFO_END_PTR, block->start +
				block->size - 8);
		WRITE_VREG(VLD_MEM_VIFIFO_CURR_PTR,
				round_down(block->start + hw->chunk_offset,
					VDEC_FIFO_ALIGN));

		WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 1);
		WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 0);

		/* set to manual mode */
		WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 2);
		WRITE_VREG(VLD_MEM_VIFIFO_RP,
				round_down(block->start + hw->chunk_offset,
					VDEC_FIFO_ALIGN));
		dummy = hw->chunk_offset + hw->chunk_size +
			VLD_PADDING_SIZE;
		if (dummy >= block->size)
			dummy -= block->size;
		WRITE_VREG(VLD_MEM_VIFIFO_WP,
			round_down(block->start + dummy,
				VDEC_FIFO_ALIGN));

		WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 3);
		WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 2);

		WRITE_VREG(VLD_MEM_VIFIFO_CONTROL,
			(0x11 << 16) | (1<<10) | (7<<3));

	}
}

static irqreturn_t vmpeg4_isr_thread_fn(struct vdec_s *vdec, int irq)
{
	u32 reg;
	u32 picture_type;
	int index;
	u32 pts, offset = 0;
	u64 pts_us64 = 0;
	u32 frame_size, dec_w, dec_h;
	u32 time_increment_resolution, fixed_vop_rate, vop_time_inc;
	u32 repeat_cnt, duration = 3200;
	struct pic_info_t *dec_pic, *disp_pic;
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)(vdec->private);

	if (hw->eos)
		return IRQ_HANDLED;

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);
	if (READ_VREG(AV_SCRATCH_M) != 0 &&
		(debug_enable & PRINT_FLAG_UCODE_DETAIL)) {
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_UCODE_DETAIL,
		"dbg %x: %x, level %x, wp %x, rp %x, cnt %x\n",
			READ_VREG(AV_SCRATCH_M), READ_VREG(AV_SCRATCH_N),
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_WP),
			READ_VREG(VLD_MEM_VIFIFO_RP),
			READ_VREG(VIFF_BIT_CNT));
		WRITE_VREG(AV_SCRATCH_M, 0);
		return IRQ_HANDLED;
	}
	reg = READ_VREG(MREG_BUFFEROUT);

	time_increment_resolution = READ_VREG(MP4_RATE);
	fixed_vop_rate = time_increment_resolution >> 16;
	time_increment_resolution &= 0xffff;
	if (time_increment_resolution > 0 &&
		fixed_vop_rate == 0)
		hw->sys_mp4_rate = time_increment_resolution;

	if (hw->vmpeg4_amstream_dec_info.rate == 0) {
		if ((fixed_vop_rate != 0) &&
			(time_increment_resolution != 0)) {
			hw->vmpeg4_amstream_dec_info.rate = fixed_vop_rate *
					DURATION_UNIT / time_increment_resolution;
		} else if (time_increment_resolution == 0
			&& hw->sys_mp4_rate > 0)
			time_increment_resolution = hw->sys_mp4_rate;
	}
	mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_DEC_DETAIL,
		"time_inc_res = %d, fixed_vop_rate = %d, rate = %d\n",
		time_increment_resolution, fixed_vop_rate,
		hw->vmpeg4_amstream_dec_info.rate);

	if (reg == 2) {
		/* timeout when decoding next frame */

		/* for frame based case, insufficient result may happen
		 * at the beginning when only VOL head is available save
		 * HW context also, such as for the QTable from VCOP register
		 */
		mmpeg4_debug_print(DECODE_ID(hw),
			PRINT_FLAG_VLD_DETAIL,
			"%s, level = %x, vfifo_ctrl = %x, bitcnt = %d\n",
			__func__,
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_CONTROL),
			READ_VREG(VIFF_BIT_CNT));

		if (vdec_frame_based(vdec)) {
			//vmpeg4_save_hw_context(hw);
			hw->dec_result = DEC_RESULT_DONE;
			vdec_schedule_work(&hw->work);
		} else {
			reset_process_time(hw);
			hw->dec_result = DEC_RESULT_AGAIN;
			vdec_schedule_work(&hw->work);
		}
		return IRQ_HANDLED;
	} else {
		reset_process_time(hw);
		picture_type = (reg >> 3) & 7;
		repeat_cnt = READ_VREG(MP4_NOT_CODED_CNT);
		vop_time_inc = READ_VREG(MP4_VOP_TIME_INC);

		index = spec_to_index(hw, READ_VREG(REC_CANVAS_ADDR));
		if (index < 0) {
			mmpeg4_debug_print(DECODE_ID(hw), 0,
				"invalid buffer index %d. rec = %x\n",
				index, READ_VREG(REC_CANVAS_ADDR));
			hw->dec_result = DEC_RESULT_ERROR;
			vdec_schedule_work(&hw->work);
			return IRQ_HANDLED;
		}
		hw->dec_result = DEC_RESULT_DONE;
		dec_pic = &hw->pic[index];
		dec_pic->pts_valid = false;
		dec_pic->pts = 0;
		dec_pic->pts64 = 0;
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_BUFFER_DETAIL,
			"new pic: index=%d, used=%d, repeat=%d, time_inc=%d\n",
			index, hw->vfbuf_use[index], repeat_cnt, vop_time_inc);

		dec_w = READ_VREG(MP4_PIC_WH)>> 16;
		dec_h = READ_VREG(MP4_PIC_WH) & 0xffff;
		if (dec_w != 0)
			hw->frame_width = dec_w;
		if (dec_h != 0)
			hw->frame_height = dec_h;

		if (hw->is_used_v4l) {
			struct aml_vcodec_ctx *ctx =
				(struct aml_vcodec_ctx *)(hw->v4l2_ctx);

			if (ctx->param_sets_from_ucode && !hw->v4l_params_parsed) {
				struct aml_vdec_pic_infos info;

				info.visible_width	= hw->frame_width;
				info.visible_height	= hw->frame_height;
				info.coded_width	= ALIGN(hw->frame_width, 64);
				info.coded_height	= ALIGN(hw->frame_height, 64);
				info.dpb_size		= MAX_BMMU_BUFFER_NUM - 1;
				hw->v4l_params_parsed	= true;
				vdec_v4l_set_pic_infos(ctx, &info);
			}

			if (!ctx->v4l_codec_ready)
				return IRQ_HANDLED;
		}

		if (hw->vmpeg4_amstream_dec_info.rate == 0) {
			if (vop_time_inc < hw->last_vop_time_inc) {
				duration = vop_time_inc +
					time_increment_resolution -
					hw->last_vop_time_inc;
			} else {
				duration = vop_time_inc -
					hw->last_vop_time_inc;
			}

			if (duration == hw->last_duration) {
				hw->rate_detect++;
				if ((hw->rate_detect >= RATE_DETECT_COUNT) &&
					(time_increment_resolution != 0)) {
					hw->vmpeg4_amstream_dec_info.rate =
						duration * DURATION_UNIT /
						time_increment_resolution;
					duration =
						hw->vmpeg4_amstream_dec_info.rate;
				}
			} else {
				hw->rate_detect = 0;
				hw->last_duration = duration;
			}
			if (MPEG4_VALID_DUR(duration)) {
				mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
					"warn: duration %x, set 0\n", duration);
				duration = 0;
			}
		} else {
			duration = hw->vmpeg4_amstream_dec_info.rate;
#if 0
			pr_info("info rate = %d, ucode rate = 0x%x:0x%x\n",
				   hw->vmpeg4_amstream_dec_info.rate,
				   READ_VREG(MP4_RATE), vop_time_inc);
#endif
		}

		/* frame mode with unstable pts */
		if (hw->unstable_pts && hw->chunk) {
			dec_pic->pts_valid = hw->chunk->pts_valid;
			dec_pic->pts = hw->chunk->pts;
			dec_pic->pts64 = hw->chunk->pts64;
			if ((B_PICTURE == picture_type) ||
				(hw->last_dec_pts == dec_pic->pts))
				dec_pic->pts_valid = 0;

			hw->last_dec_pts = dec_pic->pts;
		} else if ((I_PICTURE == picture_type) ||
			(P_PICTURE == picture_type)) {
			offset = READ_VREG(MP4_OFFSET_REG);
			if (hw->chunk) {
				dec_pic->pts_valid = hw->chunk->pts_valid;
				dec_pic->pts = hw->chunk->pts;
				dec_pic->pts64 = hw->chunk->pts64;
			} else {
				if (pts_lookup_offset_us64(PTS_TYPE_VIDEO, offset,
					&pts, &frame_size, 3000, &pts_us64) == 0) {
					dec_pic->pts_valid = true;
					dec_pic->pts = pts;
					dec_pic->pts64 = pts_us64;
					hw->pts_hit++;
				} else {
					dec_pic->pts_valid = false;
					hw->pts_missed++;
				}
			}
			mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
				"%c, offset=0x%x, pts=0x%x(%d), index=%d, used=%d\n",
				GET_PIC_TYPE(picture_type), offset, dec_pic->pts,
				dec_pic->pts_valid, index, hw->vfbuf_use[index]);
		}

		dec_pic->index = index;
		dec_pic->pic_info = reg;
		dec_pic->pic_type = picture_type;
		dec_pic->duration = duration;
		hw->vfbuf_use[index] = 0;
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
			"mmpeg4: pic_num: %d, index %d, type %c, pts %x\n",
			hw->frame_num, index,
			GET_PIC_TYPE(picture_type),
			dec_pic->pts);

		/* buffer management */
		if ((picture_type == I_PICTURE) ||
			(picture_type == P_PICTURE)) {
			index = update_ref(hw, index);
		} else {
			/* drop B frame or disp immediately.
			 * depend on if there are two ref frames
			 */
			if (hw->refs[1] == -1)
				index = -1;
		}
		vmpeg4_save_hw_context(hw);
		if (index < 0) {
			vdec_schedule_work(&hw->work);
			return IRQ_HANDLED;
		}
		disp_pic = &hw->pic[index];
		if ((hw->first_i_frame_ready == 0) &&
			(I_PICTURE == disp_pic->pic_type))
			hw->first_i_frame_ready = 1;

		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
			"disp: index=%d, pts=%x(%d), used=%d, picout=%c(dec=%c)\n",
			index, disp_pic->pts, disp_pic->pts_valid,
			hw->vfbuf_use[index],
			GET_PIC_TYPE(disp_pic->pic_type),
			GET_PIC_TYPE(picture_type));

		if (disp_pic->pts_valid) {
			hw->last_anch_pts = disp_pic->pts;
			hw->last_anch_pts_us64 = disp_pic->pts64;
			hw->frame_num_since_last_anch = 0;
			hw->vop_time_inc_since_last_anch = 0;
		} else if (vdec_stream_based(vdec)) {
			disp_pic->pts = hw->last_anch_pts;
			disp_pic->pts64 = hw->last_anch_pts_us64;

			if ((time_increment_resolution != 0) &&
				(fixed_vop_rate == 0) &&
				(hw->vmpeg4_amstream_dec_info.rate == 0)) {
				/* variable PTS rate */
				/*bug on variable pts calc,
				 *do as dixed vop first if we
				 *have rate setting before.
				 */
				if (vop_time_inc > hw->last_vop_time_inc) {
					duration = vop_time_inc -
						hw->last_vop_time_inc;
				} else {
					duration = vop_time_inc +
						time_increment_resolution -
						hw->last_vop_time_inc;
				}

				hw->vop_time_inc_since_last_anch += duration;

				disp_pic->pts += hw->vop_time_inc_since_last_anch *
					PTS_UNIT / time_increment_resolution;
				disp_pic->pts64 += (hw->vop_time_inc_since_last_anch *
					PTS_UNIT / time_increment_resolution) *
					100 / 9;

				if (hw->vop_time_inc_since_last_anch >
					(1 << 14)) {
					/* avoid overflow */
					hw->last_anch_pts = disp_pic->pts;
					hw->last_anch_pts_us64 = disp_pic->pts64;
					hw->vop_time_inc_since_last_anch = 0;
				}
			} else {
				/* fixed VOP rate */
				hw->frame_num_since_last_anch++;
				disp_pic->pts += DUR2PTS(hw->frame_num_since_last_anch *
					hw->vmpeg4_amstream_dec_info.rate);
				disp_pic->pts64 += DUR2PTS(
					hw->frame_num_since_last_anch *
					hw->vmpeg4_amstream_dec_info.rate) * 100 / 9;

				if (hw->frame_num_since_last_anch > (1 << 15)) {
					/* avoid overflow */
					hw->last_anch_pts = disp_pic->pts;
					hw->last_anch_pts_us64 = disp_pic->pts64;
					hw->frame_num_since_last_anch = 0;
				}
			}
		} else if (hw->unstable_pts && hw->chunk &&
			MPEG4_VALID_DUR(duration)) {
			/* invalid pts calc */
			hw->frame_num_since_last_anch = hw->chunk_frame_count;
			disp_pic->pts = hw->last_anch_pts +
				DUR2PTS(hw->frame_num_since_last_anch *
				duration);
			disp_pic->pts64 = hw->last_anch_pts_us64 +
				DUR2PTS(hw->frame_num_since_last_anch *
				duration) * 100 / 9;

			if (hw->frame_num_since_last_anch > (1 << 15)) {
				/* avoid overflow */
				hw->last_anch_pts = disp_pic->pts;
				hw->last_anch_pts_us64 = disp_pic->pts64;
				hw->frame_num_since_last_anch = 0;
			} else
				disp_pic->pts_valid = 1;
		}

		if (vdec_frame_based(vdec) &&
			(hw->unstable_pts) &&
			MPEG4_VALID_DUR(duration)) {

			u32 threshold = DUR2PTS(duration) >> 3;

			if (disp_pic->pts <= (hw->last_pts + threshold)) {
				disp_pic->pts = hw->last_pts + DUR2PTS(duration);
				disp_pic->pts64 = hw->last_pts64 +
					(DUR2PTS(duration)*100/9);
			}
			if (!disp_pic->pts_valid) {
				disp_pic->pts = 0;
				disp_pic->pts64 = 0;
			}
		}
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
			"disp: pic_type %c, pts %d(%lld), diff %d, cnt %d\n",
			GET_PIC_TYPE(disp_pic->pic_type),
			disp_pic->pts,
			disp_pic->pts64,
			disp_pic->pts - hw->last_pts,
			hw->chunk_frame_count);
		hw->last_pts = disp_pic->pts;
		hw->last_pts64 = disp_pic->pts64;
		hw->frame_dur = duration;
		disp_pic->duration = duration;
		disp_pic->repeat_cnt = repeat_cnt;

		prepare_display_buf(hw, disp_pic);

		hw->total_frame += repeat_cnt + 1;
		hw->last_vop_time_inc = vop_time_inc;

		if (vdec_frame_based(vdec) &&
			(frmbase_cont_bitlevel != 0) &&
			(hw->first_i_frame_ready)) {
			u32 consume_byte, res_byte, bitcnt;

			bitcnt = READ_VREG(VIFF_BIT_CNT);
			res_byte = bitcnt >> 3;

			if (hw->chunk_size > res_byte) {
				if (bitcnt > frmbase_cont_bitlevel) {
					consume_byte = hw->chunk_size - res_byte;

					mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
						"%s, size %d, consume %d, res %d\n", __func__,
						hw->chunk_size, consume_byte, res_byte);

					if (consume_byte > VDEC_FIFO_ALIGN) {
						consume_byte -= VDEC_FIFO_ALIGN;
						res_byte += VDEC_FIFO_ALIGN;
					}
					hw->chunk_offset += consume_byte;
					hw->chunk_size = res_byte;
					hw->dec_result = DEC_RESULT_UNFINISH;
					hw->chunk_frame_count++;
					hw->unstable_pts = 1;
				} else {
					hw->chunk_size = 0;
					hw->chunk_offset = 0;
				}
			} else {
				mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
					"error: bitbyte %d  hw->chunk_size %d\n", res_byte, hw->chunk_size);
				hw->chunk_size = 0;
				hw->chunk_offset = 0;
			}
		}
		vdec_schedule_work(&hw->work);
	}
	mmpeg4_debug_print(DECODE_ID(hw), PRINT_FRAME_NUM,
		"%s: frame num:%d\n", __func__, hw->frame_num);

	return IRQ_HANDLED;
}

static irqreturn_t vmpeg4_isr(struct vdec_s *vdec, int irq)
{
	struct vdec_mpeg4_hw_s *hw =
		(struct vdec_mpeg4_hw_s *)(vdec->private);

	if (hw->eos)
		return IRQ_HANDLED;

	return IRQ_WAKE_THREAD;
}

static void flush_output(struct vdec_mpeg4_hw_s * hw)
{
	struct pic_info_t *pic;

	if (hw->vfbuf_use[hw->refs[1]] > 0) {
		pic = &hw->pic[hw->refs[1]];
		prepare_display_buf(hw, pic);
	}
}

static int notify_v4l_eos(struct vdec_s *vdec)
{
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;
	struct aml_vcodec_ctx *ctx = (struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct vframe_s *vf = NULL;
	struct vdec_v4l2_buffer *fb = NULL;

	if (hw->is_used_v4l && hw->eos) {
		if (kfifo_get(&hw->newframe_q, &vf) == 0 || vf == NULL) {
			mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
				"%s fatal error, no available buffer slot.\n",
				__func__);
			return -1;
		}

		if (vdec_v4l_get_buffer(hw->v4l2_ctx, &fb)) {
			pr_err("[%d] get fb fail.\n", ctx->id);
			return -1;
		}

		vf->timestamp = ULONG_MAX;
		vf->v4l_mem_handle = (unsigned long)fb;
		vf->flag = VFRAME_FLAG_EMPTY_FRAME_V4L;

		kfifo_put(&hw->display_q, (const struct vframe_s *)vf);
		vf_notify_receiver(vdec->vf_provider_name,
			VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);

		pr_info("[%d] mpeg4 EOS notify.\n", ctx->id);
	}

	return 0;
}

static void vmpeg4_work(struct work_struct *work)
{
	struct vdec_mpeg4_hw_s *hw =
		container_of(work, struct vdec_mpeg4_hw_s, work);
	struct vdec_s *vdec = hw_to_vdec(hw);

	/* finished decoding one frame or error,
	 * notify vdec core to switch context
	 */
	if (hw->dec_result != DEC_RESULT_DONE)
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
			"vmpeg4_work: result=%d,status=%d\n",
			hw->dec_result, hw_to_vdec(hw)->next_status);

	if (hw->dec_result == DEC_RESULT_UNFINISH) {
		if (!hw->ctx_valid)
			hw->ctx_valid = 1;

	} else if ((hw->dec_result == DEC_RESULT_DONE) ||
		((input_frame_based(&vdec->input)) && hw->chunk)) {
		if (!hw->ctx_valid)
			hw->ctx_valid = 1;

		vdec_vframe_dirty(vdec, hw->chunk);
		hw->chunk = NULL;
	} else if (hw->dec_result == DEC_RESULT_AGAIN
		&& (vdec->next_status != VDEC_STATUS_DISCONNECTED)) {
		/*
			stream base: stream buf empty or timeout
			frame base: vdec_prepare_input fail
		*/
		if (!vdec_has_more_input(vdec)) {
			hw->dec_result = DEC_RESULT_EOS;
			vdec_schedule_work(&hw->work);
			return;
		}
	} else if (hw->dec_result == DEC_RESULT_FORCE_EXIT) {
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
			"%s: force exit\n", __func__);
		if (hw->stat & STAT_ISR_REG) {
			amvdec_stop();
			vdec_free_irq(VDEC_IRQ_1, (void *)hw);
			hw->stat &= ~STAT_ISR_REG;
		}
	} else if (hw->dec_result == DEC_RESULT_EOS) {
		hw->eos = 1;
		if (hw->stat & STAT_VDEC_RUN) {
			amvdec_stop();
			hw->stat &= ~STAT_VDEC_RUN;
		}
		vdec_vframe_dirty(vdec, hw->chunk);
		hw->chunk = NULL;
		vdec_clean_input(vdec);
		flush_output(hw);

		if (hw->is_used_v4l)
			notify_v4l_eos(vdec);

		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"%s: eos flushed, frame_num %d\n",
			__func__, hw->frame_num);
	}

	if (hw->stat & STAT_VDEC_RUN) {
		amvdec_stop();
		hw->stat &= ~STAT_VDEC_RUN;
	}
	/*disable mbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 0);
	del_timer_sync(&hw->check_timer);
	hw->stat &= ~STAT_TIMER_ARM;

	/* mark itself has all HW resource released and input released */
	if (vdec->parallel_dec == 1)
		vdec_core_finish_run(vdec, CORE_MASK_VDEC_1);
	else
		vdec_core_finish_run(vdec, CORE_MASK_VDEC_1 | CORE_MASK_HEVC);

	wake_up_interruptible(&hw->wait_q);
	if (hw->vdec_cb)
		hw->vdec_cb(vdec, hw->vdec_cb_arg);
}

static struct vframe_s *vmpeg_vf_peek(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;

	if (!hw)
		return NULL;
	hw->peek_num++;
	if (kfifo_peek(&hw->display_q, &vf))
		return vf;

	return NULL;
}

static struct vframe_s *vmpeg_vf_get(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;
	hw->get_num++;
	if (kfifo_get(&hw->display_q, &vf))
		return vf;

	return NULL;
}

static void vmpeg_vf_put(struct vframe_s *vf, void *op_arg)
{
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;

	hw->vfbuf_use[vf->index]--;
	hw->put_num++;
	mmpeg4_debug_print(DECODE_ID(hw), PRINT_FRAME_NUM,
		"%s: put num:%d\n",__func__, hw->put_num);
	mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_BUFFER_DETAIL,
		"index=%d, used=%d\n", vf->index, hw->vfbuf_use[vf->index]);
	kfifo_put(&hw->newframe_q, (const struct vframe_s *)vf);
}

static int vmpeg_event_cb(int type, void *data, void *private_data)
{
	return 0;
}

static int  vmpeg_vf_states(struct vframe_states *states, void *op_arg)
{
	unsigned long flags;
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;

	spin_lock_irqsave(&hw->lock, flags);

	states->vf_pool_size = VF_POOL_SIZE;
	states->buf_free_num = kfifo_len(&hw->newframe_q);
	states->buf_avail_num = kfifo_len(&hw->display_q);
	states->buf_recycle_num = 0;

	spin_unlock_irqrestore(&hw->lock, flags);

	return 0;
}


static int dec_status(struct vdec_s *vdec, struct vdec_info *vstatus)
{
	struct vdec_mpeg4_hw_s *hw =
		(struct vdec_mpeg4_hw_s *)vdec->private;

	if (!hw)
		return -1;

	vstatus->frame_width = hw->frame_width;
	vstatus->frame_height = hw->frame_height;
	if (0 != hw->vmpeg4_amstream_dec_info.rate)
		vstatus->frame_rate = DURATION_UNIT /
				hw->vmpeg4_amstream_dec_info.rate;
	else
		vstatus->frame_rate = DURATION_UNIT;
	vstatus->error_count = READ_VREG(MP4_ERR_COUNT);
	vstatus->status = hw->stat;
	vstatus->frame_dur = hw->frame_dur;
	snprintf(vstatus->vdec_name, sizeof(vstatus->vdec_name),
			"%s", DRIVER_NAME);

	return 0;
}

/****************************************/
static int vmpeg4_canvas_init(struct vdec_mpeg4_hw_s *hw)
{
	int i, ret;
	u32 canvas_width, canvas_height;
	u32 decbuf_size, decbuf_y_size;
	struct vdec_s *vdec = hw_to_vdec(hw);
	unsigned long decbuf_start;

	if (buf_size <= 0x00400000) {
			/* SD only */
			canvas_width = 768;
			canvas_height = 576;
			decbuf_y_size = 0x80000;
			decbuf_size = 0x100000;
	} else {
		int w = 1920;
		int h = 1088;
		int align_w, align_h;
		int max, min;

		align_w = ALIGN(w, 64);
		align_h = ALIGN(h, 64);
		if (align_w > align_h) {
			max = align_w;
			min = align_h;
		} else {
			max = align_h;
			min = align_w;
		}
		/* HD & SD */
		if ((max > 1920 || min > 1088) &&
			ALIGN(align_w * align_h * 3/2, SZ_64K) * 9 <=
			buf_size) {
			canvas_width = align_w;
			canvas_height = align_h;
			decbuf_y_size =
				ALIGN(align_w * align_h, SZ_64K);
			decbuf_size =
				ALIGN(align_w * align_h * 3/2, SZ_64K);
		} else { /*1080p*/
			if (h > w) {
				canvas_width = 1088;
				canvas_height = 1920;
			} else {
				canvas_width = 1920;
				canvas_height = 1088;
			}
			decbuf_y_size = 0x200000;
			decbuf_size = 0x300000;
		}
	}

	for (i = 0; i < MAX_BMMU_BUFFER_NUM; i++) {

		unsigned canvas;

		if (i == (MAX_BMMU_BUFFER_NUM - 1))
			decbuf_size = WORKSPACE_SIZE;

		if (hw->is_used_v4l && !(i == (MAX_BMMU_BUFFER_NUM - 1))) {
			continue;
		} else {
			ret = decoder_bmmu_box_alloc_buf_phy(hw->mm_blk_handle, i,
					decbuf_size, DRIVER_NAME, &decbuf_start);
			if (ret < 0) {
				pr_err("mmu alloc failed! size %d  idx %d\n",
					decbuf_size, i);
				return ret;
			}
		}

		if (i == (MAX_BMMU_BUFFER_NUM - 1)) {
			hw->buf_start = decbuf_start;
		} else {
			if (vdec->parallel_dec == 1) {
				unsigned tmp;
				if (canvas_y(hw->canvas_spec[i]) == 0xff) {
					tmp =
						vdec->get_canvas_ex(CORE_MASK_VDEC_1, vdec->id);
					hw->canvas_spec[i] &= ~0xff;
					hw->canvas_spec[i] |= tmp;
				}
				if (canvas_u(hw->canvas_spec[i]) == 0xff) {
					tmp =
						vdec->get_canvas_ex(CORE_MASK_VDEC_1, vdec->id);
					hw->canvas_spec[i] &= ~(0xffff << 8);
					hw->canvas_spec[i] |= tmp << 8;
					hw->canvas_spec[i] |= tmp << 16;
				}
				canvas = hw->canvas_spec[i];
			} else {
				canvas = vdec->get_canvas(i, 2);
				hw->canvas_spec[i] = canvas;
			}

			hw->canvas_config[i][0].phy_addr = decbuf_start;
			hw->canvas_config[i][0].width = canvas_width;
			hw->canvas_config[i][0].height = canvas_height;
			hw->canvas_config[i][0].block_mode = hw->blkmode;
			if (hw->blkmode == CANVAS_BLKMODE_LINEAR)
				hw->canvas_config[i][0].endian = 7;
			else
				hw->canvas_config[i][0].endian = 0;
			canvas_config_config(canvas_y(canvas),
					&hw->canvas_config[i][0]);

			hw->canvas_config[i][1].phy_addr =
				decbuf_start + decbuf_y_size;
			hw->canvas_config[i][1].width = canvas_width;
			hw->canvas_config[i][1].height = (canvas_height >> 1);
			hw->canvas_config[i][1].block_mode = hw->blkmode;
			if (hw->blkmode == CANVAS_BLKMODE_LINEAR)
				hw->canvas_config[i][1].endian = 7;
			else
				hw->canvas_config[i][1].endian = 0;
			canvas_config_config(canvas_u(canvas),
					&hw->canvas_config[i][1]);
		}
	}

	return 0;
}

static void vmpeg4_dump_state(struct vdec_s *vdec)
{
	struct vdec_mpeg4_hw_s *hw =
		(struct vdec_mpeg4_hw_s *)(vdec->private);
	u32 i;
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"====== %s\n", __func__);
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"width/height (%d/%d), i_fram:%d, buffer_not_ready %d\n",
		hw->frame_width,
		hw->frame_height,
		hw->first_i_frame_ready,
		hw->buffer_not_ready
		);
	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"index %d, used %d\n", i, hw->vfbuf_use[i]);
	}

	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"is_framebase(%d), eos %d, state 0x%x, dec_result 0x%x dec_frm %d\n",
		vdec_frame_based(vdec),
		hw->eos,
		hw->stat,
		hw->dec_result,
		hw->frame_num
		);
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"is_framebase(%d),  put_frm %d run %d not_run_ready %d input_empty %d,drop %d\n",
		vdec_frame_based(vdec),
		hw->put_num,
		hw->run_count,
		hw->not_run_ready,
		hw->input_empty,
		hw->drop_frame_count
		);

	if (vf_get_receiver(vdec->vf_provider_name)) {
		enum receviver_start_e state =
		vf_notify_receiver(vdec->vf_provider_name,
			VFRAME_EVENT_PROVIDER_QUREY_STATE,
			NULL);
		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"\nreceiver(%s) state %d\n",
			vdec->vf_provider_name,
			state);
	}
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"%s, newq(%d/%d), dispq(%d/%d) vf peek/get/put (%d/%d/%d)\n",
		__func__,
		kfifo_len(&hw->newframe_q), VF_POOL_SIZE,
		kfifo_len(&hw->display_q), VF_POOL_SIZE,
		hw->peek_num, hw->get_num, hw->put_num
	);
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"VIFF_BIT_CNT=0x%x\n",
		READ_VREG(VIFF_BIT_CNT));
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"VLD_MEM_VIFIFO_LEVEL=0x%x\n",
		READ_VREG(VLD_MEM_VIFIFO_LEVEL));
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"VLD_MEM_VIFIFO_WP=0x%x\n",
		READ_VREG(VLD_MEM_VIFIFO_WP));
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"VLD_MEM_VIFIFO_RP=0x%x\n",
		READ_VREG(VLD_MEM_VIFIFO_RP));
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"PARSER_VIDEO_RP=0x%x\n",
		READ_PARSER_REG(PARSER_VIDEO_RP));
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"PARSER_VIDEO_WP=0x%x\n",
		READ_PARSER_REG(PARSER_VIDEO_WP));
	if (vdec_frame_based(vdec) &&
		debug_enable & PRINT_FRAMEBASE_DATA) {
		int jj;
		if (hw->chunk && hw->chunk->block &&
			hw->chunk->size > 0) {
			u8 *data = NULL;

			if (!hw->chunk->block->is_mapped)
				data = codec_mm_vmap(hw->chunk->block->start +
					hw->chunk->offset, hw->chunk->size);
			else
				data = ((u8 *)hw->chunk->block->start_virt) +
					hw->chunk->offset;

			mmpeg4_debug_print(DECODE_ID(hw), 0,
				"frame data size 0x%x\n",
				hw->chunk->size);
			for (jj = 0; jj < hw->chunk->size; jj++) {
				if ((jj & 0xf) == 0)
					mmpeg4_debug_print(DECODE_ID(hw),
					PRINT_FRAMEBASE_DATA,
						"%06x:", jj);
				mmpeg4_debug_print(DECODE_ID(hw),
				PRINT_FRAMEBASE_DATA,
					"%02x ", data[jj]);
				if (((jj + 1) & 0xf) == 0)
					mmpeg4_debug_print(DECODE_ID(hw),
					PRINT_FRAMEBASE_DATA,
						"\n");
			}

			if (!hw->chunk->block->is_mapped)
				codec_mm_unmap_phyaddr(data);
		}
	}
}

static void reset_process_time(struct vdec_mpeg4_hw_s *hw)
{
	if (hw->start_process_time) {
		unsigned process_time =
			1000 * (jiffies - hw->start_process_time) / HZ;
		hw->start_process_time = 0;
		if (process_time > max_process_time[DECODE_ID(hw)])
			max_process_time[DECODE_ID(hw)] = process_time;
	}
}
static void start_process_time(struct vdec_mpeg4_hw_s *hw)
{
	hw->decode_timeout_count = 2;
	hw->start_process_time = jiffies;
}

static void timeout_process(struct vdec_mpeg4_hw_s *hw)
{
	if (hw->stat & STAT_VDEC_RUN) {
		amvdec_stop();
		hw->stat &= ~STAT_VDEC_RUN;
	}
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"%s decoder timeout %d\n", __func__, hw->timeout_cnt);
	if (vdec_frame_based((hw_to_vdec(hw)))) {
		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"%s frame_num %d, chunk size 0x%x, chksum 0x%x\n",
			__func__,
			hw->frame_num,
			hw->chunk->size,
			get_data_check_sum(hw, hw->chunk->size));
	}
	hw->timeout_cnt++;
	/* timeout: data droped, frame_num inaccurate*/
	hw->frame_num++;
	reset_process_time(hw);
	hw->first_i_frame_ready = 0;
	hw->dec_result = DEC_RESULT_DONE;
	vdec_schedule_work(&hw->work);
}


static void check_timer_func(unsigned long arg)
{
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)arg;
	struct vdec_s *vdec = hw_to_vdec(hw);
	unsigned int timeout_val = decode_timeout_val;

	if (radr != 0) {
		if (rval != 0) {
			WRITE_VREG(radr, rval);
			pr_info("WRITE_VREG(%x,%x)\n", radr, rval);
		} else
			pr_info("READ_VREG(%x)=%x\n", radr, READ_VREG(radr));
		rval = 0;
		radr = 0;
	}

	if (((debug_enable & PRINT_FLAG_TIMEOUT_STATUS) == 0) &&
		(vdec_frame_based(vdec) ||
		((u32)READ_VREG(VLD_MEM_VIFIFO_LEVEL) > 0x100)) &&
		(timeout_val > 0) &&
		(hw->start_process_time > 0) &&
		((1000 * (jiffies - hw->start_process_time) / HZ)
			> timeout_val)) {
		if (hw->last_vld_level == READ_VREG(VLD_MEM_VIFIFO_LEVEL)) {
			if (hw->decode_timeout_count > 0)
				hw->decode_timeout_count--;
			if (hw->decode_timeout_count == 0)
				timeout_process(hw);
		}
		hw->last_vld_level = READ_VREG(VLD_MEM_VIFIFO_LEVEL);
	}

	if (vdec->next_status == VDEC_STATUS_DISCONNECTED) {
		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
			"vdec requested to be disconnected\n");
		hw->dec_result = DEC_RESULT_FORCE_EXIT;
		vdec_schedule_work(&hw->work);
		return;
	}

	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
}

static int vmpeg4_hw_ctx_restore(struct vdec_mpeg4_hw_s *hw)
{
	int index, i;
	void *workspace_buf = NULL;

	index = find_free_buffer(hw);
	if (index < 0)
		return -1;

	if (!hw->init_flag) {
		if (vmpeg4_canvas_init(hw) < 0)
			return -1;
	} else {
		if (!hw->is_used_v4l) {
			for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
				canvas_config_config(canvas_y(hw->canvas_spec[i]),
							&hw->canvas_config[i][0]);
				canvas_config_config(canvas_u(hw->canvas_spec[i]),
							&hw->canvas_config[i][1]);
			}
		}
	}
	/* prepare REF0 & REF1
	 * points to the past two IP buffers
	 * prepare REC_CANVAS_ADDR and ANC2_CANVAS_ADDR
	 * points to the output buffer
	 */
	if (hw->refs[0] == -1) {
		WRITE_VREG(MREG_REF0, (hw->refs[1] == -1) ? 0xffffffff :
					hw->canvas_spec[hw->refs[1]]);
	} else {
		WRITE_VREG(MREG_REF0, (hw->refs[0] == -1) ? 0xffffffff :
					hw->canvas_spec[hw->refs[0]]);
	}
	WRITE_VREG(MREG_REF1, (hw->refs[1] == -1) ? 0xffffffff :
				hw->canvas_spec[hw->refs[1]]);

	WRITE_VREG(REC_CANVAS_ADDR, hw->canvas_spec[index]);
	WRITE_VREG(ANC2_CANVAS_ADDR, hw->canvas_spec[index]);

	mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_RESTORE,
	"restore ref0=0x%x, ref1=0x%x, rec=0x%x, ctx_valid=%d,index=%d\n",
	   READ_VREG(MREG_REF0),
	   READ_VREG(MREG_REF1),
	   READ_VREG(REC_CANVAS_ADDR),
	   hw->ctx_valid, index);

	/* notify ucode the buffer start address */
	workspace_buf = codec_mm_vmap(hw->buf_start, WORKSPACE_SIZE);
	if (workspace_buf) {
		/* clear to fix decoder timeout at first time */
		if (!hw->init_flag)
			memset(workspace_buf, 0, WORKSPACE_SIZE);
		codec_mm_dma_flush(workspace_buf,
			WORKSPACE_SIZE, DMA_TO_DEVICE);
		codec_mm_unmap_phyaddr(workspace_buf);
	}
	WRITE_VREG(MEM_OFFSET_REG, hw->buf_start);

	/* disable PSCALE for hardware sharing */
	WRITE_VREG(PSCALE_CTRL, 0);

	WRITE_VREG(MREG_BUFFEROUT, 0);

	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	/* enable mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 1);

	/* clear repeat count */
	WRITE_VREG(MP4_NOT_CODED_CNT, 0);

#ifdef NV21
	SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 17);
#endif

#if 1/* /MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8 */
	WRITE_VREG(MDEC_PIC_DC_THRESH, 0x404038aa);
#endif

	WRITE_VREG(MP4_PIC_WH, (hw->ctx_valid) ?
		hw->reg_mp4_pic_wh :
		((hw->frame_width << 16) | hw->frame_height));
	WRITE_VREG(MP4_SYS_RATE, hw->vmpeg4_amstream_dec_info.rate);

	if (hw->ctx_valid) {
		WRITE_VREG(DC_AC_CTRL, hw->reg_dc_ac_ctrl);
		WRITE_VREG(IQIDCT_CONTROL, hw->reg_iqidct_control);
		WRITE_VREG(RESYNC_MARKER_LENGTH, hw->reg_resync_marker_length);
		WRITE_VREG(RV_AI_MB_COUNT, hw->reg_rv_ai_mb_count);
	}
	WRITE_VREG(MPEG1_2_REG, (hw->ctx_valid) ? hw->reg_mpeg1_2_reg : 1);
	WRITE_VREG(VCOP_CTRL_REG, hw->reg_vcop_ctrl_reg);
	WRITE_VREG(PIC_HEAD_INFO, hw->reg_pic_head_info);
	WRITE_VREG(SLICE_QP, hw->reg_slice_qp);
	WRITE_VREG(MB_INFO, hw->reg_mb_info);

	if (vdec_frame_based(hw_to_vdec(hw)) && hw->chunk) {
		/* frame based input */
		WRITE_VREG(MREG_INPUT, (hw->chunk->offset & 7) | (1<<7) |
							(hw->ctx_valid<<6));
	} else {
		/* stream based input */
		WRITE_VREG(MREG_INPUT, (hw->ctx_valid<<6));
	}

	return 0;
}

static void vmpeg4_local_init(struct vdec_mpeg4_hw_s *hw)
{
	int i;

	hw->vmpeg4_ratio = hw->vmpeg4_amstream_dec_info.ratio;

	hw->vmpeg4_ratio64 = hw->vmpeg4_amstream_dec_info.ratio64;

	hw->vmpeg4_rotation =
		(((unsigned long)hw->vmpeg4_amstream_dec_info.param) >> 16) & 0xffff;
	hw->sys_mp4_rate = hw->vmpeg4_amstream_dec_info.rate;
	hw->frame_width = hw->vmpeg4_amstream_dec_info.width;
	hw->frame_height = hw->vmpeg4_amstream_dec_info.height;
	hw->frame_dur = 0;
	hw->frame_prog = 0;
	hw->unstable_pts =
	   (((unsigned long) hw->vmpeg4_amstream_dec_info.param & 0x40) >> 6);
	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"param = 0x%x unstable_pts = %d\n",
		hw->vmpeg4_amstream_dec_info.param,
		hw->unstable_pts);
	hw->last_dec_pts = -1;

	hw->total_frame = 0;

	hw->last_anch_pts = 0;

	hw->last_anch_pts_us64 = 0;

	hw->last_vop_time_inc = hw->last_duration = 0;

	hw->vop_time_inc_since_last_anch = 0;
	hw->last_pts = 0;
	hw->last_pts64 = 0;
	hw->frame_num_since_last_anch = 0;
	hw->frame_num = 0;
	hw->put_num = 0;
	hw->run_count = 0;
	hw->not_run_ready = 0;
	hw->input_empty = 0;
	hw->peek_num = 0;
	hw->get_num = 0;

	hw->pts_hit = hw->pts_missed = hw->pts_i_hit = hw->pts_i_missed = 0;
	hw->refs[0] = -1;
	hw->refs[1] = -1;
	hw->first_i_frame_ready = 0;
	hw->drop_frame_count = 0;
	hw->buffer_not_ready = 0;
	hw->init_flag = 0;
	hw->dec_result = DEC_RESULT_NONE;
	hw->timeout_cnt = 0;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++)
		hw->vfbuf_use[i] = 0;

	INIT_KFIFO(hw->display_q);
	INIT_KFIFO(hw->newframe_q);

	for (i = 0; i < VF_POOL_SIZE; i++) {
		const struct vframe_s *vf = &hw->vfpool[i];

		hw->vfpool[i].index = DECODE_BUFFER_NUM_MAX;
		kfifo_put(&hw->newframe_q, (const struct vframe_s *)vf);
	}
	if (hw->mm_blk_handle) {
		decoder_bmmu_box_free(hw->mm_blk_handle);
		hw->mm_blk_handle = NULL;
	}
	hw->mm_blk_handle = decoder_bmmu_box_alloc_box(
			DRIVER_NAME,
			0,
			MAX_BMMU_BUFFER_NUM,
			4 + PAGE_SHIFT,
			CODEC_MM_FLAGS_CMA_CLEAR |
			CODEC_MM_FLAGS_FOR_VDECODER);
	INIT_WORK(&hw->work, vmpeg4_work);

	init_waitqueue_head(&hw->wait_q);
}

static s32 vmmpeg4_init(struct vdec_mpeg4_hw_s *hw)
{
	int trickmode_fffb = 0;
	int size = -1, fw_size = 0x1000 * 16;
	struct firmware_s *fw = NULL;

	fw = vmalloc(sizeof(struct firmware_s) + fw_size);
	if (IS_ERR_OR_NULL(fw))
		return -ENOMEM;

	if (hw->vmpeg4_amstream_dec_info.format ==
			VIDEO_DEC_FORMAT_MPEG4_5) {
		size = get_firmware_data(VIDEO_DEC_MPEG4_5_MULTI, fw->data);
		strncpy(fw->name, "mmpeg4_mc_5", sizeof(fw->name));
	} else if (hw->vmpeg4_amstream_dec_info.format ==
			VIDEO_DEC_FORMAT_H263) {
		size = get_firmware_data(VIDEO_DEC_H263_MULTI, fw->data);
		strncpy(fw->name, "mh263_mc", sizeof(fw->name));
	} else
		pr_err("unsupport mpeg4 sub format %d\n",
				hw->vmpeg4_amstream_dec_info.format);
	pr_info("mmpeg4 get fw %s, size %x\n", fw->name, size);
	if (size < 0) {
		pr_err("get firmware failed.");
		vfree(fw);
		return -1;
	}

	fw->len = size;
	hw->fw = fw;

	query_video_status(0, &trickmode_fffb);

	pr_info("%s\n", __func__);

	amvdec_enable();

	init_timer(&hw->check_timer);
	hw->check_timer.data = (unsigned long)hw;
	hw->check_timer.function = check_timer_func;
	hw->check_timer.expires = jiffies + CHECK_INTERVAL;
	hw->stat |= STAT_TIMER_ARM;
	hw->eos = 0;
	WRITE_VREG(DECODE_STOP_POS, udebug_flag);

	vmpeg4_local_init(hw);
	wmb();

	return 0;
}

static unsigned long run_ready(struct vdec_s *vdec, unsigned long mask)
{
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;

	if (hw->eos)
		return 0;
	if (vdec_stream_based(vdec) && (hw->init_flag == 0)
		&& pre_decode_buf_level != 0) {
		u32 rp, wp, level;

		rp = READ_PARSER_REG(PARSER_VIDEO_RP);
		wp = READ_PARSER_REG(PARSER_VIDEO_WP);
		if (wp < rp)
			level = vdec->input.size + wp - rp;
		else
			level = wp - rp;
		if (level < pre_decode_buf_level) {
			hw->not_run_ready++;
			return 0;
		}
	}

	if (!is_enough_free_buffer(hw)) {
		hw->buffer_not_ready++;
		return 0;
	}

	hw->not_run_ready = 0;
	hw->buffer_not_ready = 0;
	if (vdec->parallel_dec == 1)
		return (unsigned long)(CORE_MASK_VDEC_1);
	else
		return (unsigned long)(CORE_MASK_VDEC_1 | CORE_MASK_HEVC);
}

static unsigned char get_data_check_sum
	(struct vdec_mpeg4_hw_s *hw, int size)
{
	int jj;
	int sum = 0;
	u8 *data = NULL;

	if (!hw->chunk->block->is_mapped)
		data = codec_mm_vmap(hw->chunk->block->start +
			hw->chunk->offset, size);
	else
		data = ((u8 *)hw->chunk->block->start_virt) +
			hw->chunk->offset;

	for (jj = 0; jj < size; jj++)
		sum += data[jj];

	if (!hw->chunk->block->is_mapped)
		codec_mm_unmap_phyaddr(data);
	return sum;
}

static void run(struct vdec_s *vdec, unsigned long mask,
		void (*callback)(struct vdec_s *, void *), void *arg)
{
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;
	int size = 0, ret = 0;

	hw->run_count++;
	hw->vdec_cb_arg = arg;
	hw->vdec_cb = callback;
	vdec_reset_core(vdec);

	if ((vdec_frame_based(vdec)) &&
		(hw->dec_result == DEC_RESULT_UNFINISH)) {
		vmpeg4_prepare_input(hw);
		size = hw->chunk_size;
	} else {
		size = vdec_prepare_input(vdec, &hw->chunk);
		if (size < 4) { /*less than start code size 00 00 01 xx*/
			hw->input_empty++;
			hw->dec_result = DEC_RESULT_AGAIN;
			vdec_schedule_work(&hw->work);
			return;
		}
		if ((vdec_frame_based(vdec)) &&
			(hw->chunk != NULL)) {
			hw->chunk_offset = hw->chunk->offset;
			hw->chunk_size = hw->chunk->size;
			hw->chunk_frame_count = 0;
		}
	}
	if (vdec_frame_based(vdec) &&
		(debug_enable & 0xc00)) {
		u8 *data = NULL;

		if (!hw->chunk->block->is_mapped)
			data = codec_mm_vmap(hw->chunk->block->start +
				hw->chunk->offset, size);
		else
			data = ((u8 *)hw->chunk->block->start_virt) +
				hw->chunk->offset;

		mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_VDEC_STATUS,
			"%s: size 0x%x sum 0x%x %02x %02x %02x %02x %02x %02x .. %02x %02x %02x %02x\n",
			__func__, size, get_data_check_sum(hw, size),
			data[0], data[1], data[2], data[3],
			data[4], data[5], data[size - 4],
			data[size - 3],	data[size - 2],
			data[size - 1]);

		if (debug_enable & PRINT_FRAMEBASE_DATA) {
			int jj;

			for (jj = 0; jj < size; jj++) {
				if ((jj & 0xf) == 0)
					mmpeg4_debug_print(DECODE_ID(hw),
					PRINT_FRAMEBASE_DATA,
						"%06x:", jj);
				mmpeg4_debug_print(DECODE_ID(hw),
				PRINT_FRAMEBASE_DATA,
					"%02x ", data[jj]);
				if (((jj + 1) & 0xf) == 0)
					mmpeg4_debug_print(DECODE_ID(hw),
					PRINT_FRAMEBASE_DATA,
						"\n");
			}
		}

		if (!hw->chunk->block->is_mapped)
			codec_mm_unmap_phyaddr(data);
	}

	mmpeg4_debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
		"%s, size=%d, %x %x %x %x %x\n",
		__func__, size,
		READ_VREG(VLD_MEM_VIFIFO_LEVEL),
		READ_VREG(VLD_MEM_VIFIFO_WP),
		READ_VREG(VLD_MEM_VIFIFO_RP),
		READ_PARSER_REG(PARSER_VIDEO_RP),
		READ_PARSER_REG(PARSER_VIDEO_WP));

	hw->dec_result = DEC_RESULT_NONE;
	if (vdec->mc_loaded) {
	/*firmware have load before,
	  and not changes to another.
	  ignore reload.
	*/
	} else {
		ret = amvdec_vdec_loadmc_buf_ex(VFORMAT_MPEG4,hw->fw->name, vdec,
			hw->fw->data, hw->fw->len);
		if (ret < 0) {
			pr_err("[%d] %s: the %s fw loading failed, err: %x\n", vdec->id,
				hw->fw->name, tee_enabled() ? "TEE" : "local", ret);
			hw->dec_result = DEC_RESULT_FORCE_EXIT;
			vdec_schedule_work(&hw->work);
			return;
		}
		vdec->mc_loaded = 1;
		vdec->mc_type = VFORMAT_MPEG4;
	}
	if (vmpeg4_hw_ctx_restore(hw) < 0) {
		hw->dec_result = DEC_RESULT_ERROR;
		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"amvdec_mpeg4: error HW context restore\n");
		vdec_schedule_work(&hw->work);
		return;
	}
	if (vdec_frame_based(vdec)) {
		size = hw->chunk_size +
			(hw->chunk_offset & (VDEC_FIFO_ALIGN - 1));
		WRITE_VREG(VIFF_BIT_CNT, size * 8);
	}
	hw->input_empty = 0;
	hw->last_vld_level = 0;
	start_process_time(hw);
	vdec_enable_input(vdec);
	/* wmb before ISR is handled */
	wmb();

	amvdec_start();
	hw->stat |= STAT_VDEC_RUN;
	hw->init_flag = 1;
	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
}

static int vmpeg4_stop(struct vdec_mpeg4_hw_s *hw)
{
	cancel_work_sync(&hw->work);

	if (hw->mm_blk_handle) {
			decoder_bmmu_box_free(hw->mm_blk_handle);
			hw->mm_blk_handle = NULL;
	}

	if (hw->stat & STAT_TIMER_ARM) {
		del_timer_sync(&hw->check_timer);
		hw->stat &= ~STAT_TIMER_ARM;
	}

	if (hw->fw) {
		vfree(hw->fw);
		hw->fw = NULL;
	}
	return 0;
}
static void reset(struct vdec_s *vdec)
{
	struct vdec_mpeg4_hw_s *hw = (struct vdec_mpeg4_hw_s *)vdec->private;

	pr_info("amvdec_mmpeg4: reset.\n");

	vmpeg4_local_init(hw);

	hw->ctx_valid = 0;
}

static int ammvdec_mpeg4_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	struct vdec_mpeg4_hw_s *hw = NULL;

	if (pdata == NULL) {
		pr_err("%s memory resource undefined.\n", __func__);
		return -EFAULT;
	}

	hw = vmalloc(sizeof(struct vdec_mpeg4_hw_s));
	if (hw == NULL) {
		pr_err("\namvdec_mpeg4 decoder driver alloc failed\n");
		return -ENOMEM;
	}
	memset(hw, 0, sizeof(struct vdec_mpeg4_hw_s));

	/* the ctx from v4l2 driver. */
	hw->v4l2_ctx = pdata->private;

	pdata->private = hw;
	pdata->dec_status = dec_status;
	/* pdata->set_trickmode = set_trickmode; */
	pdata->run_ready = run_ready;
	pdata->run = run;
	pdata->reset = reset;
	pdata->irq_handler = vmpeg4_isr;
	pdata->threaded_irq_handler = vmpeg4_isr_thread_fn;
	pdata->dump_state = vmpeg4_dump_state;

	if (pdata->use_vfm_path)
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			VFM_DEC_PROVIDER_NAME);
	else
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			PROVIDER_NAME ".%02x", pdev->id & 0xff);

	if (pdata->parallel_dec == 1) {
		int i;
		for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++)
			hw->canvas_spec[i] = 0xffffff;
	}

	vf_provider_init(&pdata->vframe_provider,
		pdata->vf_provider_name, &vf_provider_ops, pdata);

	platform_set_drvdata(pdev, pdata);
	hw->platform_dev = pdev;
	hw->blkmode = pdata->canvas_mode;

	if (pdata->sys_info) {
		hw->vmpeg4_amstream_dec_info = *pdata->sys_info;
		if ((hw->vmpeg4_amstream_dec_info.height != 0) &&
			(hw->vmpeg4_amstream_dec_info.width >
			(MAX_MPEG4_SUPPORT_SIZE/hw->vmpeg4_amstream_dec_info.height))) {
			pr_info("ammvdec_mpeg4: oversize, unsupport: %d*%d\n",
				hw->vmpeg4_amstream_dec_info.width,
				hw->vmpeg4_amstream_dec_info.height);
			pdata->dec_status = NULL;
			vfree((void *)hw);
			hw = NULL;
			return -EFAULT;
		}
		mmpeg4_debug_print(DECODE_ID(hw), 0,
			"sysinfo: %d x %d, rate: %d\n",
			hw->vmpeg4_amstream_dec_info.width,
			hw->vmpeg4_amstream_dec_info.height,
			hw->vmpeg4_amstream_dec_info.rate);
		hw->is_used_v4l = (((unsigned long)
			hw->vmpeg4_amstream_dec_info.param & 0x80) >> 7);
	}

	if (vmmpeg4_init(hw) < 0) {
		pr_err("%s init failed.\n", __func__);

		if (hw) {
			vfree((void *)hw);
			hw = NULL;
		}
		pdata->dec_status = NULL;
		return -ENODEV;
	}
	vdec_set_prepare_level(pdata, start_decode_buf_level);

	if (pdata->parallel_dec == 1)
		vdec_core_request(pdata, CORE_MASK_VDEC_1);
	else {
		vdec_core_request(pdata, CORE_MASK_VDEC_1 | CORE_MASK_HEVC
				| CORE_MASK_COMBINE);
	}

	mmpeg4_debug_print(DECODE_ID(hw), 0,
		"%s end.\n", __func__);
	return 0;
}

static int ammvdec_mpeg4_remove(struct platform_device *pdev)
{
	struct vdec_mpeg4_hw_s *hw =
		(struct vdec_mpeg4_hw_s *)
		(((struct vdec_s *)(platform_get_drvdata(pdev)))->private);
	struct vdec_s *vdec = hw_to_vdec(hw);
	int i;

	if (vdec->next_status == VDEC_STATUS_DISCONNECTED
				&& (vdec->status == VDEC_STATUS_ACTIVE)) {
			mmpeg4_debug_print(DECODE_ID(hw), 0,
				"%s  force exit %d\n", __func__, __LINE__);
			hw->dec_result = DEC_RESULT_FORCE_EXIT;
			vdec_schedule_work(&hw->work);
			wait_event_interruptible_timeout(hw->wait_q,
				(vdec->status == VDEC_STATUS_CONNECTED),
				msecs_to_jiffies(1000));  /* wait for work done */
	}

	vmpeg4_stop(hw);

	if (vdec->parallel_dec == 1)
		vdec_core_release(hw_to_vdec(hw), CORE_MASK_VDEC_1);
	else
		vdec_core_release(hw_to_vdec(hw), CORE_MASK_VDEC_1 | CORE_MASK_HEVC);
	vdec_set_status(hw_to_vdec(hw), VDEC_STATUS_DISCONNECTED);

	if (vdec->parallel_dec == 1) {
		for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
			vdec->free_canvas_ex(canvas_y(hw->canvas_spec[i]), vdec->id);
			vdec->free_canvas_ex(canvas_u(hw->canvas_spec[i]), vdec->id);
		}
	}

	mmpeg4_debug_print(DECODE_ID(hw), 0, "%s\n", __func__);
	vfree((void *)hw);
	hw = NULL;

	return 0;
}

/****************************************/
#ifdef CONFIG_PM
static int mmpeg4_suspend(struct device *dev)
{
	amvdec_suspend(to_platform_device(dev), dev->power.power_state);
	return 0;
}

static int mmpeg4_resume(struct device *dev)
{
	amvdec_resume(to_platform_device(dev));
	return 0;
}

static const struct dev_pm_ops mmpeg4_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mmpeg4_suspend, mmpeg4_resume)
};
#endif

static struct platform_driver ammvdec_mpeg4_driver = {
	.probe = ammvdec_mpeg4_probe,
	.remove = ammvdec_mpeg4_remove,
	.driver = {
		.name = DRIVER_NAME,
#ifdef CONFIG_PM
		.pm = &mmpeg4_pm_ops,
#endif
	}
};

static struct codec_profile_t amvdec_mpeg4_profile = {
	.name = "mmpeg4",
	.profile = ""
};

static int __init ammvdec_mpeg4_driver_init_module(void)
{
	pr_info("%s \n", __func__);

	if (platform_driver_register(&ammvdec_mpeg4_driver)) {
		pr_err("failed to register ammvdec_mpeg4 driver\n");
		return -ENODEV;
	}
	vcodec_profile_register(&amvdec_mpeg4_profile);
	return 0;
}

static void __exit ammvdec_mpeg4_driver_remove_module(void)
{
	pr_info("ammvdec_mpeg4 module remove.\n");

	platform_driver_unregister(&ammvdec_mpeg4_driver);
}

/****************************************/
module_param(debug_enable, uint, 0664);
MODULE_PARM_DESC(debug_enable,
					 "\n ammvdec_mpeg4 debug enable\n");

module_param(frmbase_cont_bitlevel, uint, 0664);
MODULE_PARM_DESC(frmbase_cont_bitlevel, "\nfrmbase_cont_bitlevel\n");

module_param(radr, uint, 0664);
MODULE_PARM_DESC(radr, "\nradr\n");

module_param(rval, uint, 0664);
MODULE_PARM_DESC(rval, "\nrval\n");

module_param(decode_timeout_val, uint, 0664);
MODULE_PARM_DESC(decode_timeout_val, "\n ammvdec_mpeg4 decode_timeout_val\n");

module_param_array(max_process_time, uint, &max_decode_instance_num, 0664);

module_param(pre_decode_buf_level, int, 0664);
MODULE_PARM_DESC(pre_decode_buf_level,
		"\n ammvdec_mpeg4 pre_decode_buf_level\n");

module_param(start_decode_buf_level, int, 0664);
MODULE_PARM_DESC(start_decode_buf_level,
		"\n ammvdec_mpeg4 start_decode_buf_level\n");

module_param(udebug_flag, uint, 0664);
MODULE_PARM_DESC(udebug_flag, "\n ammvdec_mpeg4 udebug_flag\n");

module_param(without_display_mode, uint, 0664);
MODULE_PARM_DESC(without_display_mode, "\n ammvdec_mpeg4 without_display_mode\n");

module_init(ammvdec_mpeg4_driver_init_module);
module_exit(ammvdec_mpeg4_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC MPEG4 Video Decoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");

