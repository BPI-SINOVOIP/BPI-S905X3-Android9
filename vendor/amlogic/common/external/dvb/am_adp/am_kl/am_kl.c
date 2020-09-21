/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 解扰器模块
 *
 * \author Yanyan <yan.yan@amlogic.com>
 * \date 2016-10-10: create the document
 ***************************************************************************/

#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <am_types.h>
#include <am_kl.h>
#include <am_dsc.h>
#include <dlfcn.h>

static int g_kl_fd = -1;
static int g_kl_work_mode;
static void* g_dsc_sec_lib_handle;

static struct meson_kl_config g_sec_config;
/****************************************************************************
 * static functions
 ***************************************************************************/
static AM_ErrorCode_t get_work_mode()
{
	g_dsc_sec_lib_handle = dlopen(DSC_SEC_CaLib, RTLD_NOW);
	if (!g_dsc_sec_lib_handle)
	{
		printf("AM_KL: dlopen %s failed, work mode: NORMAL\n", DSC_SEC_CaLib);
		g_kl_work_mode = DSC_MODE_NORMAL;
	}
	else
	{
		printf("AM_KL: dlopen %s ok, work mode: SEC\n", DSC_SEC_CaLib);
		g_kl_work_mode = DSC_MODE_SEC;
	}
	return AM_SUCCESS;
}

/* For am_dsc module use,
 *  In seclinux mode, kl module only store keys,
 */
extern AM_ErrorCode_t am_kl_get_config(struct meson_kl_config *kl_config)
{
	if (g_kl_work_mode != DSC_MODE_SEC)
		return AM_FAILURE;

	if (!kl_config)
		return AM_FAILURE;

	memcpy(kl_config, &g_sec_config, sizeof(struct meson_kl_config));
	return AM_SUCCESS;
}

/****************************************************************************
 * APIS
 ***************************************************************************/
/* keyladder_init
 * open keyladder device
 *  ret:
 *  	Success:	AM_SUCCESS
 *           Failure:	AM_FAILURE
 */
int AM_KL_KeyladderInit()
{
	int ret;
	ret = get_work_mode();
	if (ret != AM_SUCCESS)
		return AM_FAILURE;

	if (g_kl_work_mode == DSC_MODE_NORMAL)
	{
		if (g_kl_fd < 0)
		{
			g_kl_fd = open(MESON_KL_DEV, O_RDWR);
			if (g_kl_fd < 0)
			{
				printf("Keyladder open failed\n");
				return AM_FAILURE;
			}
			printf("Open keyladder driver node ok\n");
		}
		return AM_SUCCESS;
	}
	else
		return AM_SUCCESS;
}

/* keyladder_release
 * release keyladder device
 */
void AM_KL_KeyladderRelease()
{
	if (g_kl_work_mode == DSC_MODE_NORMAL)
	{
		if (g_kl_fd >= 0) {
			close(g_kl_fd);
			g_kl_fd = -1;
		}
	}
	else
	{
		if (g_dsc_sec_lib_handle)
			dlclose(g_dsc_sec_lib_handle);
		g_dsc_sec_lib_handle = NULL;
		return;
	}
}

/*  set_keyladder:
 *  Currently support 3 levels keyladder, use this function
 *  	to set kl regs, the cw will set to descrambler automatically.
 *  key2:  for 3 levels kl, it means ekn1
 *  key1:  ekn2
 *  ecw:    ecw
 *  ret:
 *  	Success:	AM_SUCCESS
 *           Failure:	AM_FAILURE
 */
int AM_KL_SetKeyladder(struct meson_kl_config *kl_config)
{
	struct meson_kl_run_args arg;
	int ret;
	if (g_kl_work_mode == DSC_MODE_NORMAL)
	{
		if (g_kl_fd<0)
			return AM_FAILURE;
		/* Calculate root key */
		//set keys
		if (kl_config->kl_levels != 3) {
			printf("Only support 3 levels keyladder by now\n");
			return AM_FAILURE;
		}else{
			memcpy(arg.keys[2], kl_config->ekey2, 16);
			memcpy(arg.keys[1], kl_config->ekey1, 16);
			memcpy(arg.keys[0], kl_config->ecw, 16);
		}
		arg.kl_num = MESON_KL_NUM;
		arg.kl_levels = kl_config->kl_levels;
		ret = ioctl(g_kl_fd, MESON_KL_RUN, &arg);
		if (ret != -1)
			return AM_SUCCESS;
		else
		{
			printf("Keyladder ioctl failed\n");
			return AM_FAILURE;
		}
	}
	else
	{
		memcpy(g_sec_config.ekey2, kl_config->ekey2, 16);
		memcpy(g_sec_config.ekey1, kl_config->ekey1, 16);
		memcpy(g_sec_config.ecw, kl_config->ecw, 16);
		g_sec_config.kl_levels = kl_config->kl_levels;
		return AM_SUCCESS;
	}
}

/*  set_keyladder:
 *  Currently support 3 levels keyladder, use this function
 *  	to set kl regs, the cw will set to descrambler automatically.
 *  key2:  for 3 levels kl, it means ekn1
 *  nounce: it is used for challenge
 *  dnounce: response of cr
 *  ret:
 *  	Success:	AM_SUCCESS
 *           Failure:	AM_FAILURE
 */
int AM_KL_SetKeyladderCr(unsigned char key2[16], unsigned nounce[16], unsigned dnounce[16])
{
	int i, ret;
	struct meson_kl_cr_args arg;

	if (g_kl_work_mode == DSC_MODE_NORMAL)
	{
		if (g_kl_fd<0)
			return AM_FAILURE;

		memset(&arg, 0, sizeof(arg));
		arg.kl_num = MESON_KL_NUM;
		memcpy(arg.cr, nounce, 16);
		memcpy(arg.ekn1, key2, 16);
		ret = ioctl(g_kl_fd, MESON_KL_CR, &arg);
		memcpy(dnounce, arg.cr, 16);
		if (ret != -1)
			return AM_SUCCESS;
		else
			return AM_FAILURE;
	}
	return AM_SUCCESS;
}
