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
/**\file am_check_scramb.c
 * \brief 获取加密编制模块
 *
 * \author hualing.chen<hualing.chen@amlogic.com>
 * \date 2017-12-07: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 2
#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <pthread.h>
#include <assert.h>
#include <am_debug.h>
#include <am_mem.h>
#include <am_time.h>
#include <am_util.h>
#include <am_av.h>
#include <am_db.h>
#include <am_epg.h>
#include <am_dvr.h>
#include "am_misc.h"
#include "am_check_scramb.h"
/****************************************************************************
 * Macro definitions
 ***************************************************************************/
typedef struct ScrambInfo
{
	int pid;
	//int scramble;
	//int checked_scrambled_count;
	int pkt_num;
	int scramb_pkt_num;
}ScrambInfo_t;

typedef struct CheckScramData
{
	int id;
	int fifo_id;
	int dmx_id;
	int dmx_src;
	int is_running;
	pthread_t thread;
	ScrambInfo_t ScrambInfo[AM_DVB_PID_MAXCOUNT];
	int  buf_pos;
}CheckScramData_t;

CheckScramData_t  ScramData;

/****************************************************************************
 * Type definitions
 ***************************************************************************/
static inline void am_add_pid_and_scramble(int pid, int scramb) {
	int i, begin;

	if ((pid <= 0) || (pid >= 0x1fff))
		return;

	begin = pid & (AM_DVB_PID_MAXCOUNT - 1);
	i     = begin;

	do {
		if ((ScramData.ScrambInfo[i].pid == pid) || !ScramData.ScrambInfo[i].pid) {
			ScramData.ScrambInfo[i].pid = pid;
			ScramData.ScrambInfo[i].pkt_num ++;

			if ((scramb == 2) || (scramb == 3))
				ScramData.ScrambInfo[i].scramb_pkt_num ++;
			break;
		}
		i ++;
		if (i == AM_DVB_PID_MAXCOUNT)
			i = 0;
	} while (i != begin);
}

static void am_get_pid_and_scramble(uint8_t* buf, int len) {
	int pid;
	int scramb;
	char *tmp;
	int i = 0;
	for (i = 0; i < len - 4 * 188; ) {
		if (buf[i] != 0x47 || buf[i + 188] != 0x47 || buf[i + 2*188] != 0x47 || buf[i + 3*188] != 0x47) {
			i++;
			//AM_DEBUG(0, "am-check-scram am_get_pid_and_scramble not get 0x47");
			continue;
		}
		pid = ((buf[i + 1] & 0x1f) << 8 | buf[i + 2]);//13 bit
		scramb = (buf[i + 3] >> 6);//2 bit
		am_add_pid_and_scramble(pid, scramb);

		i = i + 188;
	}
}

/**\brief check scramb 线程*/
static void *am_check_scramble_thread(void* arg)
{
	CheckScramData_t *obj = (CheckScramData_t *)arg;
	int cnt = 0;
	uint8_t buf[512*1024];
	char buf_mode[256];
	AM_DVR_OpenPara_t para;
	AM_DMX_OpenPara_t dmx_para;
	AM_DVR_StartRecPara_t spara;
	/*设置DVR设备参数*/
	memset(&para, 0, sizeof(para));

	printf("am-check-scram Open DVR%d start\r\n", obj->id);
	if (AM_DVR_Open(obj->id, &para) != AM_SUCCESS)
	{
		AM_DEBUG(0, "am-check-scram Open DVR%d failed", obj->id);
		return NULL;
	}

	memset(&dmx_para, 0, sizeof(dmx_para));
	AM_DMX_Open(obj->dmx_id, &dmx_para);

	memset(buf_mode, 0, 256);

	sprintf(buf_mode, "/sys/class/stb/dvr%d_mode", obj->id);
	AM_FileEcho(buf_mode, "ts");
	AM_DVR_SetSource(obj->id, obj->fifo_id);
	AM_DMX_SetSource(obj->dmx_id, obj->dmx_src);
	memset(&spara, 0, sizeof(spara));
	spara.pids[0] = 1;
	spara.pid_count = 1;
	AM_DVR_StartRecord(obj->id, &spara);
	/*从DVR设备读取数据并存入文件*/
	while (obj->is_running)
	{
		cnt = AM_DVR_Read(obj->id, buf, sizeof(buf), 1000);
		if (cnt <= 0)
		{
			AM_DEBUG(1, "am-check-scram No data available from DVR");
			usleep(200*1000);
			continue;
		}
		am_get_pid_and_scramble(buf, cnt);
	}
	AM_DVR_StopRecord(obj->id);
	AM_FileEcho(buf_mode, "pid");
	AM_DVR_Close(obj->id);
	AM_DMX_Close(obj->dmx_id);
	obj->id = 0;
	obj->fifo_id = 0;
	return NULL;
}

/****************************************************************************
 * API functions
 ***************************************************************************/
/**\brief 启动一个检查加扰状态设备
 * \param [in] dvr_dev dvr no
 * \param [in] fifo_id dvr src
 * \param [in] dmx_dev dmx no
 * \param [in] dmx_src dmx src
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_rec.h)
 */
AM_ErrorCode_t AM_Check_Scramb_Start(int dvr_dev, int fifo_id, int dmx_dev, int dmx_src)
{
	int rc, ret = 0;
	if (ScramData.is_running)
	{
		AM_DEBUG(1, "am-check-scram Already CheckScram now, cannot start");
		return AM_FAILURE;
	}
	/*取录像相关参数*/
    memset(&ScramData, 0, sizeof(ScramData));
    ScramData.id = dvr_dev;
	ScramData.fifo_id = fifo_id;
	ScramData.dmx_id = dmx_dev;
	ScramData.dmx_src = dmx_src;
	ScramData.is_running = 1;
	ScramData.buf_pos = 0;
	rc = pthread_create(&ScramData.thread, NULL, am_check_scramble_thread, (void*)&ScramData);
	if (rc)
	{
		AM_DEBUG(0, "am-check-scram Create record thread failed: %s", strerror(rc));
	}
	AM_DEBUG(0, "am-check-scram start record return %d", ret);
	return AM_SUCCESS;
}

/**\brief 停止一个检查加扰状态设备
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_rec.h)
 */
AM_ErrorCode_t AM_Check_Scramb_Stop(void)
{
	int rc, ret = 0;

	if (!ScramData.is_running)
	{
		AM_DEBUG(1, "am-check-scram Already CheckScram now, cannot start");
		return AM_SUCCESS;
	}

	ScramData.is_running = 0;

	pthread_join(ScramData.thread, NULL);
	return AM_SUCCESS;
}
/**\brief 获取加扰的pid
 * \param [out] pids scrambled pids
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_rec.h)
 */
AM_ErrorCode_t AM_Check_Scramb_GetInfo(int pid, int *pkt_num, int *scramb_pkt_num)
{
	int i, begin;

	if (pkt_num)
		*pkt_num = 0;
	if (scramb_pkt_num)
		*scramb_pkt_num = 0;

	if ((pid <= 0) || (pid >= 0x1fff))
		return AM_SUCCESS;

	begin = pid & (AM_DVB_PID_MAXCOUNT - 1);
	i     = begin;

	do {
		if (ScramData.ScrambInfo[i].pid == pid) {
			if (pkt_num)
				*pkt_num = ScramData.ScrambInfo[i].pkt_num;
			if (scramb_pkt_num)
				*scramb_pkt_num = ScramData.ScrambInfo[i].scramb_pkt_num;
			break;
		} else if (!ScramData.ScrambInfo[i].pid) {
			break;
		}
		i ++;
		if (i == AM_DVB_PID_MAXCOUNT)
			i = 0;
	} while (i != begin);

	return AM_SUCCESS;
}

/**\brief 检查PID是否有ts package data.
 * \param pid The stream's PID
 * \param [out] pkt_num The packet number of the PID
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_rec.h)
 */
AM_ErrorCode_t AM_Check_Has_Tspackage(int pid, int *pkt_num)
{
	AM_Check_Scramb_GetInfo(pid, pkt_num, NULL);
	return AM_SUCCESS;
}
