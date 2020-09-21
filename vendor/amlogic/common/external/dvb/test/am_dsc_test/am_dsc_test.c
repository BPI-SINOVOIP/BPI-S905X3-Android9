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
 * \brief 解扰器测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <sys/types.h>
#include <am_dsc.h>
#include <am_av.h>
#include <am_fend.h>
#include <am_dmx.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <am_misc.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DMX_DEV_NO  0
#define AV_DEV_NO   0
#define FEND_DEV_NO 0
#define DSC_DEV_NO  0

#define VPID 0x330
#define APID 0x332

/****************************************************************************
 * API functions
 ***************************************************************************/

int main(int argc, char **argv)
{
	AM_AV_OpenPara_t av_para;
	AM_DMX_OpenPara_t dmx_para;
	AM_FEND_OpenPara_t fend_para;
	AM_DSC_OpenPara_t dsc_para;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	int ach, vch;
	uint8_t key[8] = {0xa, 0xa, 0xa, 0x1E, 0xa, 0xa, 0xa, 0x1E};
	
	memset(&dsc_para, 0, sizeof(dsc_para));
	memset(&av_para, 0, sizeof(av_para));
	memset(&dmx_para, 0, sizeof(dmx_para));
	memset(&fend_para, 0, sizeof(fend_para));
	
	AM_FEND_Open(FEND_DEV_NO, &fend_para);
	AM_AV_Open(AV_DEV_NO, &av_para);
	AM_DMX_Open(DMX_DEV_NO, &dmx_para);
	AM_DSC_Open(DSC_DEV_NO, &dsc_para);

#if 1	
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_TS2);
	AM_DSC_SetSource(DSC_DEV_NO, AM_DSC_SRC_DMX0); 
	
	AM_FileEcho("/sys/class/graphics/fb0/blank","1");	
	AM_FileEcho("/sys/class/graphics/fb1/blank","1");	
	
	p.frequency = 714000000;
#if 1	
	p.inversion = INVERSION_AUTO;
	p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
	p.u.ofdm.code_rate_HP = FEC_AUTO;
	p.u.ofdm.code_rate_LP = FEC_AUTO;
	p.u.ofdm.constellation = QAM_AUTO;
	p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
	p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
	p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
#else		
	p.u.qam.symbol_rate = 6875000;
	p.u.qam.fec_inner = FEC_AUTO;
	p.u.qam.modulation = QAM_64;
#endif		

	AM_FEND_Lock(FEND_DEV_NO, &p, &status);
	printf("lock status: 0x%x\n", status);
	
	AM_AV_StartTS(AV_DEV_NO, VPID, APID, 0, 0);
#else
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_HIU);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	AM_AV_StartFile(AV_DEV_NO, argv[1], 1, 0);
#endif

	AM_DSC_AllocateChannel(DSC_DEV_NO, &vch);
	AM_DSC_SetChannelPID(DSC_DEV_NO, vch, VPID);
	AM_DSC_SetKey(DSC_DEV_NO, vch, AM_DSC_KEY_TYPE_EVEN, key);
	AM_DSC_SetKey(DSC_DEV_NO, vch, AM_DSC_KEY_TYPE_ODD, key);

	AM_DSC_AllocateChannel(DSC_DEV_NO, &ach);
	AM_DSC_SetChannelPID(DSC_DEV_NO, ach, APID);
	AM_DSC_SetKey(DSC_DEV_NO, ach, AM_DSC_KEY_TYPE_EVEN, key);
	AM_DSC_SetKey(DSC_DEV_NO, ach, AM_DSC_KEY_TYPE_ODD, key);

	printf("%d %d \n",ach,vch);
	
	sleep(10);

	AM_DSC_FreeChannel(DSC_DEV_NO, vch);
	AM_DSC_FreeChannel(DSC_DEV_NO, ach);
	AM_DSC_Close(DSC_DEV_NO);
	AM_DMX_Close(DMX_DEV_NO);
	AM_AV_Close(AV_DEV_NO);
	AM_FEND_Close(FEND_DEV_NO);

	return 0;
}
