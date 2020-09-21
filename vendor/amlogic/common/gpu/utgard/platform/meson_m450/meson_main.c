/*
 * ../vendor/amlogic/common/gpu/utgard/platform/meson_m450/meson_main.c
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

#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 29))
#include <mach/register.h>
#include <mach/irqs.h>
#include <mach/io.h>
#endif
#include <asm/io.h>

#include "meson_main.h"
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include "common/mali_pmu.h"
#include "common/mali_osk_profiling.h"

int mali_pm_statue = 0;
u32 mali_gp_reset_fail = 0;
module_param(mali_gp_reset_fail, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH); /* rw-rw-r-- */
MODULE_PARM_DESC(mali_gp_reset_fail, "times of failed to reset GP");
u32 mali_core_timeout = 0;
module_param(mali_core_timeout, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH); /* rw-rw-r-- */
MODULE_PARM_DESC(mali_core_timeout, "times of failed to reset GP");

static struct mali_gpu_device_data mali_gpu_data =
{
	.shared_mem_size = 1024 * 1024 * 1024,
	.max_job_runtime = 60000, /* 60 seconds */
	.pmu_switch_delay = 0xFFFF, /* do not have to be this high on FPGA, but it is good for testing to have a delay */
#if defined(CONFIG_ARCH_MESON8B)||defined(CONFIG_ARCH_MESONG9BB)
	.pmu_domain_config = {0x1, 0x2, 0x4, 0x0,
						  0x0, 0x0, 0x0, 0x0,
						  0x0, 0x1, 0x2, 0x0},
#else
	.pmu_domain_config = {0x1, 0x2, 0x4, 0x4,
                          0x0, 0x8, 0x8, 0x8,
                          0x0, 0x1, 0x2, 0x8},
#endif
};

static void mali_platform_device_release(struct device *device);
static struct platform_device mali_gpu_device =
{
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
	.dev.release = mali_platform_device_release,
	.dev.coherent_dma_mask = DMA_BIT_MASK(32),
	.dev.platform_data = &mali_gpu_data,
	.dev.type = &mali_pm_device, /* We should probably use the pm_domain instead of type on newer kernels */
};

int mali_pdev_pre_init(struct platform_device* ptr_plt_dev)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));
	if (mali_gpu_data.shared_mem_size < 10) {
		MALI_DEBUG_PRINT(2, ("mali os memory didn't configered, set to default(512M)\n"));
		mali_gpu_data.shared_mem_size = 1024 * 1024 *1024;
	}
	return mali_meson_init_start(ptr_plt_dev);
}

void mali_pdev_post_init(struct platform_device* pdev)
{
	mali_gp_reset_fail = 0;
	mali_core_timeout = 0;
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	pm_runtime_set_autosuspend_delay(&(pdev->dev), 1000);
	pm_runtime_use_autosuspend(&(pdev->dev));
#endif
	pm_runtime_enable(&(pdev->dev));
#endif
	mali_meson_init_finish(pdev);
}

int mali_pdev_dts_init(struct platform_device* mali_gpu_device)
{
	struct device_node     *cfg_node = mali_gpu_device->dev.of_node;
	struct device_node     *child;
	u32 prop_value;
	int err;

	for_each_child_of_node(cfg_node, child) {
		err = of_property_read_u32(child, "shared_memory", &prop_value);
		if (err == 0) {
			MALI_DEBUG_PRINT(2, ("shared_memory configurate  %d\n", prop_value));
			mali_gpu_data.shared_mem_size = prop_value * 1024 * 1024;
		}
	}

	err = mali_pdev_pre_init(mali_gpu_device);
	if (err == 0)
		mali_pdev_post_init(mali_gpu_device);
	return err;
}

int mali_platform_device_register(void)
{
	int err = -1;
	err = mali_pdev_pre_init(&mali_gpu_device);
	if (err == 0) {
		err = platform_device_register(&mali_gpu_device);
		if (0 == err)
			mali_pdev_post_init(&mali_gpu_device);
	}
	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));
	mali_core_scaling_term();
	platform_device_unregister(&mali_gpu_device);
	platform_device_put(&mali_gpu_device);
}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}


