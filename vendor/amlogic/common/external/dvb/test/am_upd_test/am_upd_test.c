/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "am_time.h"

#include "am_dmx.h"
#include "am_upd.h"


static int monitor_callback(unsigned char *pnit, unsigned int len, void *user)
{
	if(!len) {
		printf("NIT (%x) timeout\n", (int)user);
		return 0;
	}

	printf("NIT[%d](%x): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			len, (int)user,
			pnit[0], pnit[1], pnit[2], pnit[3], pnit[4], pnit[5], pnit[6], pnit[7], pnit[8], pnit[9]);
	printf("tid[%d], syntax[%d] priv[%d] len[%d] ext[%d] ver[%d] cn[%d] sec[%d of %d]\n", 
			pnit[0], /*tid*/
			pnit[1]&0x80>>7, /*syntax*/
			pnit[1]&0x40>>6, /*priv*/
			(pnit[1]&0xf)<<8|pnit[2], /*len*/
			pnit[3]<<8|pnit[4], /*ext*/
			(pnit[5]&0x3E)>>1, /*version*/
			(pnit[5]&0x1), /*cn*/
			pnit[6], /*secnum*/
			pnit[7] /*last_sec_num*/
			);
	
	return 0;
}

int nit_test(int time)
{
	AM_TSUPD_MonHandle_t mid;
	AM_ErrorCode_t err = AM_SUCCESS;
	AM_TSUPD_OpenMonitorParam_t openpara;
	AM_TSUPD_MonitorParam_t monitorpara;

	memset(&openpara, 0 , sizeof(openpara));

	openpara.dmx = 0;
	err = AM_TSUPD_OpenMonitor(&openpara, &mid);
	printf("open mid(%x) err[%x]\n", mid, err);

	memset(&monitorpara, 0, sizeof(monitorpara));

	monitorpara.callback = monitor_callback;
	monitorpara.callback_args = (void*)mid;
	monitorpara.use_ext_nit = 0;
	//monitorpara.timeout = 10000;
	//monitorpara.nit_check_interval = 30000;
	err = AM_TSUPD_StartMonitor(mid, &monitorpara);
	printf("start err[%x]\n", err);


	sleep(time);

	err = AM_TSUPD_StopMonitor(mid);
	printf("stop err[%x]\n", err);


	err = AM_TSUPD_CloseMonitor(mid);	
	printf("close err[%x]\n", err);


	return 0;
}

static int dl_callback(unsigned char *pdata, unsigned int len, void *user)
{
	if(!len) {
		printf("DL (%x) timeout\n", (int)user);
		return 0;
	}

	printf("DATA[%d](%x): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			len, (int)user,
			pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7], pdata[8], pdata[9]);
	
	return 0;
}

int download_test(int pid, int tid, int ext)
{
	AM_TSUPD_DlHandle_t did;
	AM_ErrorCode_t err = AM_SUCCESS;
	AM_TSUPD_OpenDownloaderParam_t openpara;
	AM_TSUPD_DownloaderParam_t dlpara;

	memset(&openpara, 0 , sizeof(openpara));

	openpara.dmx = 0;
	err = AM_TSUPD_OpenDownloader(&openpara, &did);
	printf("open did(%x) err[%x]\n", did, err);

	memset(&dlpara, 0, sizeof(dlpara));

	dlpara.callback = dl_callback;
	dlpara.callback_args = (void*)did;
	dlpara.pid = pid;
	dlpara.tableid = tid;
	dlpara.ext = ext;
	//dlpara.timeout = 10000;
	err = AM_TSUPD_StartDownloader(did, &dlpara);
	printf("start download err[%x]\n", err);


	sleep(30);

	err = AM_TSUPD_StopDownloader(did);
	printf("stop download err[%x]\n", err);


	err = AM_TSUPD_CloseDownloader(did);	
	printf("close download err[%x]\n", err);

	return 0;
}



typedef struct dpt_s{
	pthread_mutex_t     lock;
	pthread_cond_t      cond;
	pthread_t          	thread;
	int                 quit;

	int pid;
	int tid;

#define MAX_PART_SUPPORT 256
	unsigned int last_part;

	struct {
		unsigned int stat;
#define slot_idle    0
#define slot_started 1
#define slot_timeout 2
#define slot_complete 3
#define slot_done    4

		AM_TSUPD_DlHandle_t did;
	}part_slot[MAX_PART_SUPPORT];
	pthread_mutex_t    slot_lock;

#define MAX_DP 5

	FILE *fp;
	unsigned int part_size_max;

	unsigned int total_size;

	unsigned int timeout;

}dpt_t;

static dpt_t g_dpt;



static int dl_part_callback(unsigned char *pdata, unsigned int len, void *user)
{

	if(!len)
		printf("DL (%x) timeout\n", (int)user);
	else 
		printf("DATA[%d](%x): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			len, (int)user,
			pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7], pdata[8], pdata[9]);

	pthread_mutex_lock(&g_dpt.slot_lock);
	
	g_dpt.part_slot[(int)user].stat = len? slot_complete : slot_timeout;
	
	pthread_mutex_unlock(&g_dpt.slot_lock);

	pthread_cond_signal(&g_dpt.cond);

	return 0;
}

AM_TSUPD_DlHandle_t dl_part_start(int pid, int tid, int ext, int slot)
{
	AM_TSUPD_DlHandle_t did;
	AM_ErrorCode_t err = AM_SUCCESS;
	AM_TSUPD_OpenDownloaderParam_t openpara;
	AM_TSUPD_DownloaderParam_t dlpara;

	printf("dl_part_start %x %x %x\n", pid, tid, ext);

	memset(&openpara, 0 , sizeof(openpara));

	openpara.dmx = 0;
	err = AM_TSUPD_OpenDownloader(&openpara, &did);
	printf("open did(%x) err[%x]\n", did, err);
	if(err)
		return 0;

	memset(&dlpara, 0, sizeof(dlpara));

	dlpara.callback = dl_part_callback;
	dlpara.callback_args = (void*)(long)slot;
	dlpara.pid = pid;
	dlpara.tableid = tid;
	dlpara.ext = ext;
	//dlpara.timeout = 10000;
	err = AM_TSUPD_StartDownloader(did, &dlpara);
	printf("start downloader(%x) err[%x]\n", did, err);
	if(err) {
		AM_TSUPD_CloseDownloader(did);
		return 0;
	}

	return did;
}


int dl_part_stop(AM_TSUPD_DlHandle_t did)
{
	AM_ErrorCode_t err = AM_SUCCESS;

	err = AM_TSUPD_StopDownloader(did);
	printf("stop download(%x) err[%x]\n", did, err);


	err = AM_TSUPD_CloseDownloader(did);	
	printf("close download(%x) err[%x]\n", did, err);

	return 0;
}

int part_start_slots(dpt_t *dpt, int num)
{
	int i;
	int running=0;
	int start=0;
	int need_start=0;

	pthread_mutex_lock(&dpt->slot_lock);

	for(i=0; i<(dpt->last_part+1); i++) {
		if(dpt->part_slot[i].stat==slot_started)
			running++;
	}

	need_start = (num<0)? MAX_DP-running : ((running>=num)? 0 : (num-running));

	for(i=0; i<(dpt->last_part+1) && (start<need_start); i++) {
		if(dpt->part_slot[i].stat==slot_idle) {
			dpt->part_slot[i].did = dl_part_start(dpt->pid, dpt->tid, i, i);
			if(dpt->part_slot[i].did) {
				dpt->part_slot[i].stat = slot_started;
				start++;
			}
		}
	}

	pthread_mutex_unlock(&dpt->slot_lock);

	return start+running;
}

int part_stop_slots(dpt_t *dpt)
{
	int i;
	int cnt=0;

	pthread_mutex_lock(&dpt->slot_lock);

	for(i=0; i<(dpt->last_part+1); i++) {
		if(dpt->part_slot[i].stat==slot_started) {
			dl_part_stop(dpt->part_slot[i].did);
			dpt->part_slot[i].stat = 0;
			cnt++;
		}
	}

	pthread_mutex_unlock(&dpt->slot_lock);

	return cnt;
}



static int save_data(dpt_t *dpt, unsigned char *pdata, unsigned int size)
{
	unsigned int part_no = (pdata[0]<<8) | pdata[1];
	unsigned int part_last=(pdata[2]<<8) | pdata[3]; 

	if(part_no==0)
	{
		unsigned int valid_size = (pdata[4]<<24)|(pdata[5]<<16)|(pdata[6]<<8)|pdata[7];
		dpt->total_size = valid_size;
	}

	unsigned int offset = (part_no==0)? 0 : 
						(dpt->part_size_max-4)+((part_no-1)*dpt->part_size_max);


	pdata = (part_no==0)? (pdata+8) : (pdata+4);
	size = (part_no==0)? (size-8) : (size-4);

	if((offset+size) > dpt->total_size)
		size -= ((offset+size) - dpt->total_size);

	printf("part %d of %d arrive, save data(%d) @ %d\n", part_no, part_last, size, offset);

	if(dpt->fp){
		int ret;
		fseek(dpt->fp, offset, SEEK_SET);
		ret = fwrite(pdata, 1, size, dpt->fp);
		printf("write %d\n", ret);
	}
	return 0;
}

static void *dpt_thread(void *para)
{
	dpt_t *dpt = (dpt_t*)para;
	
	int ret = 0;
	struct timespec to;
	int i;

	printf("dpt thread start.\n");

	int parts_running=0;
	
	parts_running = part_start_slots(dpt, -1);

	printf("dlt started.\n");

	
	AM_TIME_GetTimeSpecTimeout(dpt->timeout, &to);

	while (!dpt->quit && parts_running)
	{
		pthread_mutex_lock(&dpt->lock);

		ret = pthread_cond_timedwait(&dpt->cond, &dpt->lock, &to);

		if (ret == ETIMEDOUT) {
			printf("dpt thread Timeout.\n");
			break;
		}

		pthread_mutex_lock(&dpt->slot_lock);
		for(i=0; i<(dpt->last_part+1); i++) {
			if(dpt->part_slot[i].stat==slot_complete) {
				unsigned char *pdata=NULL;
				unsigned int len=0;
				int ret = AM_TSUPD_GetDownloaderData(dpt->part_slot[i].did, &pdata, &len);
				if(ret){
					printf("get download data fail.[ret=%x]\n", ret);
				} else {
					printf("got data part %d of %d\n", (pdata[0]<<8)|pdata[1], (pdata[2]<<8)|pdata[3]);
					save_data(dpt, pdata, len);
					dl_part_stop(dpt->part_slot[i].did);
					dpt->part_slot[i].stat = slot_done;
				}
			} else if(dpt->part_slot[i].stat==slot_timeout) {
				dl_part_stop(dpt->part_slot[i].did);
				dpt->part_slot[i].stat = 0;
			}
		}
		pthread_mutex_unlock(&dpt->slot_lock);

		parts_running = part_start_slots(dpt, -1);

		pthread_mutex_unlock(&dpt->lock);
	}

	if(parts_running) {
		/*quit force*/
		dpt->total_size = 0;
		part_stop_slots(dpt);
	}
	
	AM_DEBUG(1, "dpt thread exit.");
	return NULL;
}


int download_part_test(int pid, int tid, FILE *fp, int timeout)
{
	int rc;
	int i;

	dpt_t *dpt = &g_dpt;
	
	pthread_mutexattr_t mta;

	pthread_mutexattr_init(&mta);
//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&dpt->lock, &mta);
	pthread_mutex_init(&dpt->slot_lock, &mta);
	pthread_cond_init(&dpt->cond, NULL);
	pthread_mutexattr_destroy(&mta);


	dpt->fp = fp;
	dpt->pid = pid;
	dpt->tid = tid;
	dpt->timeout = (unsigned int)timeout;
	dpt->last_part = 0;
	for(i=0; i<MAX_PART_SUPPORT; i++)
		dpt->part_slot[i].stat = 0;
	dpt->part_size_max = 0;
	
	if(part_start_slots(dpt, 1))
	{
		int ret;
		unsigned char *pdata=NULL;
		unsigned int len=0;
		
		pthread_mutex_lock(&dpt->lock);
		
		ret = pthread_cond_wait(&dpt->cond, &dpt->lock);

		pthread_mutex_lock(&dpt->slot_lock);
		if(dpt->part_slot[0].stat == slot_complete) {
			ret = AM_TSUPD_GetDownloaderData(dpt->part_slot[0].did, &pdata, &len);
			if(ret) {
				printf("get download data fail.[ret:%x]\n", ret);
			} else {
				dpt->last_part = (pdata[2]<<8 | pdata[3]);
				dpt->part_size_max = len-4;
				save_data(dpt, pdata, len);
				dpt->part_slot[0].stat = slot_done;

				unsigned int part_no = (pdata[0]<<8 | pdata[1]);
				printf("part %d got, last part: %d, part_size(%d)\n", part_no, dpt->last_part, dpt->part_size_max);
			}
		}
		pthread_mutex_unlock(&dpt->slot_lock);

		pthread_mutex_unlock(&dpt->lock);

		dl_part_stop(dpt->part_slot[0].did);
		if(!pdata || !len) {
			/*no data recerived*/
			printf("data timeout.\n");
			return -1;
		}
	}

	if(dpt->last_part) {

		rc = pthread_create(&dpt->thread, NULL, dpt_thread, (void*)dpt);
		if(rc)
		{
			AM_DEBUG(1, "dpt thread create fail: %s", strerror(rc));
			pthread_mutex_destroy(&dpt->lock);
			pthread_cond_destroy(&dpt->cond);
			return -1;
		}

		pthread_t t;

		t = dpt->thread;
		if (t != pthread_self())
			pthread_join(t, NULL);
	}

	return dpt->total_size;
}

int main(int argc, char * argv[])
{
	AM_ErrorCode_t err = AM_SUCCESS;
	int pid=-1, tid=-1, ext=-1;
	char *filename=NULL;
	
	AM_DMX_OpenPara_t dmxpara;
	memset(&dmxpara, 0, sizeof(dmxpara));
	err = AM_DMX_Open(0, &dmxpara);
	printf("dmx open [%x]\n", err);
	err = AM_DMX_SetSource(0, AM_DMX_SRC_TS0);
	printf("dmx setsrc [%x]\n", err);

	if(argc<2) {
		nit_test(60);
		goto end;
	}

	if(argc>1)
		sscanf(argv[1], "%i", &pid);

	if(argc>2)
		sscanf(argv[2], "%i", &tid);
	
	if(argc>3)
		sscanf(argv[3], "%i", &ext);

	if(argc>4)
		filename = argv[4];

	if(ext>=0) {
		download_test(pid, tid, ext);
		goto end;
	}

	if(tid>=0 && filename) {
		unsigned int size;

		FILE *fp = fopen(filename, "wb");
		if(!fp) {
			printf("can not create file [%s]\n", filename);
			goto end;
		}
		
		size = download_part_test(pid, tid, fp, 120000/*2min*/);

		fclose(fp);

		printf("total download size: %d\n", size);

		goto end;
	}


end:
	err = AM_DMX_Close(0);	
	printf("dmx close err[%x]\n", err);

	return 0;
}


