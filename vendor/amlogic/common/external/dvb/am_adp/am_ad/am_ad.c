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
/***************************************************************************
/**\file
 * \brief Audio description模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2017-06-23: create the document
 ***************************************************************************/
#define AM_DEBUG_LEVEL 5
#include <am_debug.h>
#include <signal.h> 
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "memwatch.h"
#include "semaphore.h"
#include <am_ad.h>
#include <am_misc.h>
#include <am_dmx.h>
#include <am_time.h>
#include <am_debug.h>
#include <am_pes.h>

typedef struct{
	AM_AD_Para_t para;
	int          filter;
	AM_PES_Handle_t h_pes;
	AM_AD_Callback_t cb;
	void        *user;
}AM_AD_Data_t;

static FILE *fp;
static FILE *fp_e;
void close_all(void) {
	if (fp) fclose(fp);
	if (fp_e) fclose(fp_e);
}

void sigroutine(int sig) {
	switch (sig) {
		case 1: //printf("Get a signal -- SIGHUP ");
		case 2: //printf("Get a signal -- SIGINT ");
		case 3: //printf("Get a signal -- SIGQUIT ");
			close_all();
			exit(0);
		break;
	}
	return;
}

void prepare_file(){
	char name[256];
	int ret;
	name[0] = 0;

	snprintf(name, 255, "/data/dump.pes.es");
	if (0 != access(name, 0))
		return;
	fp = fopen(name, "r");
	ret = fscanf(fp, "%s", name);
	fclose(fp);
	if (!ret)
		return ;

	strncat(name, "/pes", 255);
	AM_DEBUG(1, "try file open:[%s]\n", name);
	fp = fopen(name, "wb");
	if (!fp) {
		AM_DEBUG(1, "file open:[%s] error\n", name);
	}
	strncat(name, ".es", 255);
	AM_DEBUG(1, "try file open:[%s]\n", name);
	fp_e = fopen(name, "wb");
	if (!fp_e) {
		AM_DEBUG(1, "file open:[%s] error\n", name);
	}
	signal(SIGINT, sigroutine);
}

static void pes_packet_callback(AM_PES_Handle_t handle, uint8_t *buf, int size)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)AM_PES_GetUserData(handle);

	if (!ad)
		return;

	//printf("es[%d] %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", size,
	//	buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);

	//printf("es cb b=%p, s:%d\n", buf, size);

	if (fp_e) {
		AM_DEBUG(1, "write es data");
		int ret = fwrite(buf, 1, size, fp_e);
		if (ret != size)
			AM_DEBUG(1, "es data w lost\n");
	}

	if (ad->cb)
		ad->cb(buf, size, ad->user);
}

static void ad_callback (int dev_no, int fhandle, const uint8_t *data, int len, void *user_data)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)user_data;

	UNUSED(dev_no);
	UNUSED(fhandle);
	UNUSED(data);

	AM_DEBUG(2, "ad data %d", len);
	//printf("[%d] %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", len,
	//	data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);

	if (fp) {
		AM_DEBUG(1, "write pes data");
		int ret = fwrite(data, 1, len, fp);
		if (ret != len)
			AM_DEBUG(1, "pes data w lost\n");
	}

	AM_PES_Decode(ad->h_pes, (uint8_t *)data, len);

}

AM_ErrorCode_t
AM_AD_Create(AM_AD_Handle_t *handle, AM_AD_Para_t *para)
{
	AM_AD_Data_t *ad = NULL;
	AM_Bool_t dmx_openned = AM_FALSE;
	AM_DMX_OpenPara_t dmx_para;
	struct dmx_pes_filter_params pes;
	AM_ErrorCode_t ret;
	AM_PES_Para_t  pes_para;

	if (!handle || !para) {
		ret = AM_AD_ERR_INVALID_PARAM;
		goto error;
	}

	ad = (AM_AD_Data_t *)malloc(sizeof(AM_AD_Data_t));
	if (!ad) {
		ret = AM_AD_ERR_NO_MEM;
		goto error;
	}

	ad->para   = *para;
	ad->filter = -1;

	memset(&dmx_para, 0, sizeof(dmx_para));

	ret = AM_DMX_Open(para->dmx_id, &dmx_para);
	if (ret != AM_SUCCESS) {
		goto error;
	}
	dmx_openned = AM_TRUE;

	memset(&pes, 0, sizeof(pes));
	pes.pid = para->pid;
	pes.input = DMX_IN_FRONTEND;
	pes.output = DMX_OUT_TAP;
	pes.pes_type = DMX_PES_AUDIO3;

	ret = AM_DMX_AllocateFilter(para->dmx_id, &ad->filter);
	if (ret != AM_SUCCESS)
		goto error;

	ret = AM_DMX_SetPesFilter(para->dmx_id, ad->filter, &pes);
	if (ret != AM_SUCCESS)
		goto error;

	ret = AM_DMX_SetCallback(para->dmx_id, ad->filter, ad_callback, ad);
	if (ret != AM_SUCCESS)
		goto error;

	pes_para.packet = pes_packet_callback;
	pes_para.payload_only = AM_TRUE;
	pes_para.user_data = ad;
	pes_para.afmt = para->fmt;
	ret = AM_PES_Create(&ad->h_pes, &pes_para);
	if (ret != AM_SUCCESS)
		goto error;

	prepare_file();

	AM_DEBUG(1, "audio3 AM_AD_Create dmx[%d], pid[%d] DMX_PES_AUDIO3:[%d]", para->dmx_id, para->pid, DMX_PES_AUDIO3);

	*handle = (AM_AD_Handle_t)ad;
	return AM_SUCCESS;
error:
	if (ad && (ad->filter != -1))
		AM_DMX_FreeFilter(para->dmx_id, ad->filter);
	if (dmx_openned)
		AM_DMX_Close(para->dmx_id);
	if (ad)
		free(ad);
	return ret;
}

AM_ErrorCode_t
AM_AD_Destroy(AM_AD_Handle_t handle)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)handle;

	if (!ad)
		return AM_AD_ERR_INVALID_HANDLE;

	AM_DMX_FreeFilter(ad->para.dmx_id, ad->filter);
	AM_DMX_Close(ad->para.dmx_id);
	AM_PES_Destroy(ad->h_pes);
	free(ad);

	AM_DEBUG(1, "AM_AD_Destroy");
	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_AD_Start(AM_AD_Handle_t handle)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)handle;
	#define AD_BUF_SIZE (2*768*1024)

	if (!ad)
		return AM_AD_ERR_INVALID_HANDLE;

	AM_DMX_SetBufferSize(ad->para.dmx_id, ad->filter, AD_BUF_SIZE);
	AM_DMX_StartFilter(ad->para.dmx_id, ad->filter);
	AM_DEBUG(1, "AM_AD_Start %d", AD_BUF_SIZE);

	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_AD_Stop(AM_AD_Handle_t handle)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)handle;

	if (!ad)
		return AM_AD_ERR_INVALID_HANDLE;

	AM_DMX_StopFilter(ad->para.dmx_id, ad->filter);
	AM_DEBUG(1, "AM_AD_Stop");

	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_AD_SetVolume(AM_AD_Handle_t handle, int vol)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)handle;

	UNUSED(vol);

	if (!ad)
		return AM_AD_ERR_INVALID_HANDLE;

	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_AD_SetCallback(AM_AD_Handle_t handle, AM_AD_Callback_t cb, void *user)
{
	AM_AD_Data_t *ad = (AM_AD_Data_t *)handle;
	ad->cb = cb;
	ad->user = user;
	return AM_SUCCESS;
}

