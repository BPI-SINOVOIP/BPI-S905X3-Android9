/*
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
#include <linux/amlogic/media/ppmgr/tbff.h>
#else
#include <linux/amlogic/ppmgr/tbff.h>
#endif

#include "tbff_internal.h"

static char tbff_version_str[512] = "TB detect: v2016.11.15a ***";

/* dir 0:  internal -> wrap; dir 1: wrap -> internal */
static void sync_wrapdata(u8 dir, struct _tbff_stats *internal, struct tbff_stats *wrap)
{
	if (!internal || !wrap)
		return;

	if (!dir) {
		wrap->reg_polar5_v_mute =
			internal->reg_polar5_v_mute;
		wrap->reg_polar5_h_mute =
			internal->reg_polar5_h_mute;
		wrap->reg_polar5_ro_reset =
			internal->reg_polar5_ro_reset;
		wrap->reg_polar5_mot_thrd =
			internal->reg_polar5_mot_thrd;
		wrap->reg_polar5_edge_rat =
			internal->reg_polar5_edge_rat;
		wrap->reg_polar5_ratio =
			internal->reg_polar5_ratio;
		wrap->reg_polar5_ofset =
			internal->reg_polar5_ofset;
		wrap->buf_col =
			internal->buf_col;
		wrap->buf_row =
			internal->buf_row;
		wrap->ro_polar5_numofpix =
			internal->ro_polar5_numofpix;
		wrap->ro_polar5_f4_m2 =
			internal->ro_polar5_f4_m2;
		wrap->ro_polar5_f4_p2 =
			internal->ro_polar5_f4_p2;
		wrap->ro_polar5_f6_m2 =
			internal->ro_polar5_f6_m2;
		wrap->ro_polar5_f6_p2 =
			internal->ro_polar5_f6_p2;
		wrap->ro_polar5_f2_m2 =
			internal->ro_polar5_f2_m2;
		wrap->ro_polar5_f2_p2 =
			internal->ro_polar5_f2_p2;
		wrap->ro_polar5_f4_i5 =
			internal->ro_polar5_f4_i5;
		wrap->ro_polar5_f4_i3 =
			internal->ro_polar5_f4_i3;
	} else {
		internal->reg_polar5_v_mute =
			wrap->reg_polar5_v_mute;
		internal->reg_polar5_h_mute =
			wrap->reg_polar5_h_mute;
		internal->reg_polar5_ro_reset =
			wrap->reg_polar5_ro_reset;
		internal->reg_polar5_mot_thrd =
			wrap->reg_polar5_mot_thrd;
		internal->reg_polar5_edge_rat =
			wrap->reg_polar5_edge_rat;
		internal->reg_polar5_ratio =
			wrap->reg_polar5_ratio;
		internal->reg_polar5_ofset =
			wrap->reg_polar5_ofset;
		internal->buf_col =
			wrap->buf_col;
		internal->buf_row =
			wrap->buf_row;
		internal->ro_polar5_numofpix =
			wrap->ro_polar5_numofpix;
		internal->ro_polar5_f4_m2 =
			wrap->ro_polar5_f4_m2;
		internal->ro_polar5_f4_p2 =
			wrap->ro_polar5_f4_p2;
		internal->ro_polar5_f6_m2 =
			wrap->ro_polar5_f6_m2;
		internal->ro_polar5_f6_p2 =
			wrap->ro_polar5_f6_p2;
		internal->ro_polar5_f2_m2 =
			wrap->ro_polar5_f2_m2;
		internal->ro_polar5_f2_p2 =
			wrap->ro_polar5_f2_p2;
		internal->ro_polar5_f4_i5 =
			wrap->ro_polar5_f4_i5;
		internal->ro_polar5_f4_i3 =
			wrap->ro_polar5_f4_i3;
	}
}

static void tbff_stats_inital_wrap(struct tbff_stats *pReg, int irow, int icol)
{
	struct _tbff_stats internal_data;

	tbff_stats_inital(&internal_data, irow, icol);

	sync_wrapdata(0, &internal_data, pReg);
}

static void get_tbff_stats_wrap(unsigned long *in, struct tbff_stats *pReg)
{
	struct _tbff_stats internal_data;

	sync_wrapdata(1, &internal_data, pReg);

	get_tbff_stats(in, &internal_data);

	sync_wrapdata(0, &internal_data, pReg);
}

static int tbff_fwalg_wrap(struct tbff_stats *pReg, int fld_id, int is_tff, int frm, int skip_flg, int print_flg)
{
	int ret;
	struct _tbff_stats internal_data;

	sync_wrapdata(1, &internal_data, pReg);

	ret = tbff_fwalg(&internal_data, fld_id, is_tff, frm, skip_flg, print_flg);

	sync_wrapdata(0, &internal_data, pReg);

	return ret;
}

const struct TB_DetectFuncPtr gTB_Func = {
	tbff_stats_inital_wrap,
	get_tbff_stats_wrap,
	tbff_fwalg_inital,
	tbff_fwalg_wrap,
	get_tbff_majority_flg,
};

static int __init amlogic_tb_detect_init(void)
{
	const char *tb_ver = get_version_info();
	int src_len = strlen(tbff_version_str);
	int total_len = src_len + 1;

	if (tb_ver) {
		total_len += strlen(tb_ver);
		if (total_len < sizeof(tbff_version_str))
			snprintf(&tbff_version_str[src_len],
				sizeof(tbff_version_str) - src_len - 1,
				" %s", tb_ver);
	} else {
		snprintf(&tbff_version_str[src_len],
			sizeof(tbff_version_str) - src_len - 1,
			" %s", "None Version Info");
	}

	return RegisterTB_Function(
		(struct TB_DetectFuncPtr *)&gTB_Func,
		tbff_version_str);
}

static void __exit amlogic_tb_detect_exit(void)
{
	UnRegisterTB_Function(
		(struct TB_DetectFuncPtr *)&gTB_Func);
}
module_init(amlogic_tb_detect_init);
module_exit(amlogic_tb_detect_exit);

MODULE_DESCRIPTION("Amlogic TB Detect Driver");
MODULE_AUTHOR("Amlogic SH MM team");
MODULE_LICENSE("GPL");
