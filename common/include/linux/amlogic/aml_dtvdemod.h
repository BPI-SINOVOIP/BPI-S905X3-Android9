/*
 * include/linux/amlogic/aml_dtvdemod.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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

#ifndef __AML_DTVDEMOD_H__
#define __AML_DTVDEMOD_H__

#include <linux/amlogic/aml_demod_common.h>
#include <dvb_frontend.h>

/* For demod ctrl */
struct demod_priv {
	bool inited;
	bool suspended;
	bool serial_mode;

	struct mutex mutex;

	struct dvb_frontend fe;
	struct demod_config cfg;
	enum fe_delivery_system delivery_system;

	void *private;

	int index;
};

/* For demod */
struct demod_frontend {
	struct platform_device *pdev;
	struct class class;
	struct mutex mutex;

	struct list_head demod_list;
	int demod_count;
};

#if (defined CONFIG_AMLOGIC_DTV_DEMOD)
struct dvb_frontend *aml_dtvdm_attach(const struct demod_config *cfg);
#else
static inline struct dvb_frontend *aml_dtvdm_attach(
		const struct demod_config *cfg)
{
	return NULL;
}
#endif

enum dtv_demod_type aml_get_dtvdemod_type(const char *name);

struct dvb_frontend *aml_attach_detach_dtvdemod(
		enum dtv_demod_type type,
		struct demod_config *cfg,
		int attch);

static __maybe_unused struct dvb_frontend *aml_attach_dtvdemod(
		enum dtv_demod_type type,
		struct demod_config *cfg)
{
	return aml_attach_detach_dtvdemod(type, cfg, 1);
}

static __maybe_unused int aml_detach_dtvdemod(const enum dtv_demod_type type)
{
	aml_attach_detach_dtvdemod(type, NULL, 0);

	return 0;
}

int aml_get_dts_demod_config(struct device_node *node,
		struct demod_config *cfg, int index);

/* For attach demod driver end*/
#endif /* __AML_DTVDEMOD_H__ */
