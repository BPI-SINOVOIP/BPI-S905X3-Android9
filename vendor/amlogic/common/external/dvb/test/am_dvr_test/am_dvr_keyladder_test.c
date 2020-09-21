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
 * \brief DVR测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-10: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#define PLAY_FILE


#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_av.h>
#include <am_fend.h>
#include <am_dvr.h>
#include <am_misc.h>
#include <errno.h>
#include <am_dsc.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <am_misc.h>
#include <am_kl.h>
#include <sys/time.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#ifdef CHIP_8226H
#define DVR_DEV_COUNT      (2)
#elif defined(CHIP_8226M) || defined(CHIP_8626X)
#define DVR_DEV_COUNT      (3)
#else
#define DVR_DEV_COUNT      (2)
#endif

#undef DVR_DEV_COUNT
#define DVR_DEV_COUNT      (3)

#define FEND_DEV_NO 0
#define AV_DEV_NO 0
#define PLAY_DMX_DEV_NO 1

#define DMX_DEV_NO 0
#define DVR_DEV_NO 0
#define ASYNC_FIFO 0

typedef struct
{
	int id;
	char file_name[256];
	pthread_t thread;
	int running;
	int fd;
}DVRData;

static int pat_fid, sdt_fid;
static DVRData data_threads[DVR_DEV_COUNT];
static int dmx_no = 0;
static int async_no = 0;
static int hiu_no = 0;

static void section_cbf(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	UNUSED(dev_no);
	UNUSED(fid);
	UNUSED(len);
	UNUSED(user_data);
	int i;

	printf("section:\n");
	for (i=0;i<8;i++)
	{
		printf("%02x ", data[i]);
		if (((i+1)%16) == 0) printf("\n");
	}

	if ((i%16) != 0) printf("\n");
}

static void pes_cbf(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	UNUSED(dev_no);
	UNUSED(fid);
	UNUSED(data);
	UNUSED(len);
	UNUSED(user_data);

#if 0
	int i;
	printf("pes:\n");
	for (i=0;i<len;i++)
	{
		printf("%02x ", data[i]);
		if (((i+1)%16) == 0) printf("\n");
	}

	if ((i%16) != 0) printf("\n");
#endif
}

static void display_usage(void)
{
	printf("usages:\n");
	printf("\tsetsrc\t[dvr_no async_fifo_no]\n");
	printf("\tstart\t[dvr_no FILE pid_cnt pids]\n");
	printf("\tstop\t[dvr_no]\n");
	printf("\thelp\n");
	printf("\tquit\n");
}

static void start_si(void)
{
#if 0
	struct dmx_sct_filter_params param;
#define SET_FILTER(f, p, t)\
	AM_DMX_AllocateFilter(DMX_DEV_NO, &(f));\
	AM_DMX_SetCallback(DMX_DEV_NO, f, section_cbf, NULL);\
	memset(&param, 0, sizeof(param));\
	param.pid = p;\
	param.filter.filter[0] = t;\
	param.filter.mask[0] = 0xff;\
	param.flags = DMX_CHECK_CRC;\
	AM_DMX_SetSecFilter(DMX_DEV_NO, f, &param);\
	AM_DMX_SetBufferSize(DMX_DEV_NO, f, 32*1024);\
	AM_DMX_StartFilter(DMX_DEV_NO, f);

	if (pat_fid == -1)
	{
		SET_FILTER(pat_fid, 0, 0)
	}
	if (sdt_fid == -1)
	{
		SET_FILTER(sdt_fid, 0x11, 0x42)
	}
#endif
}

static void stop_si(void)
{
#if 0
#define FREE_FILTER(f)\
	AM_DMX_StopFilter(DMX_DEV_NO, f);\
	AM_DMX_FreeFilter(DMX_DEV_NO, f);

	if (pat_fid != -1)
	{
		FREE_FILTER(pat_fid)
		pat_fid = -1;
	}

	if (sdt_fid != -1)
	{
		FREE_FILTER(sdt_fid)
		sdt_fid = -1;
	}
#endif
}
#if 1
static void reverse_arr(char *src, char *dest, int len)
{
	int i;
	for (i=0;i<len;i++)
	{
		dest[len-i-1] = src[i];
	}
}

#endif

static int g_from_kl = 0;
void start_dsc(int dsc, int src, int pid_cnt, int *pids)
{
	AM_DSC_OpenPara_t dsc_para;
	int dscc[8];
	int i;
	int ret;
	struct meson_kl_config kl_config;

	uint8_t ekey2[16] = {0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf};//{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	uint8_t ekey1[16] = {0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf};//{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	uint8_t ecw[16] = //{0xe7,0xda,0x69,0xed,0xb7,0x64,0x6b,0xcf,0xe7,0xda,0x69,0xed,0xb7,0x64,0x6b,0xcf}; //For 1234567890abcdef
	//{0x12,0x8e,0x29,0x91,0x28,0x72,0x6e,0x14,0x12,0x8e,0x29,0x91,0x28,0x72,0x6e,0x14};
	//{0xe0,0x63,0xfb,0x95,0xae,0xf1,0xef,0xe5,0xe0,0x63,0xfb,0x95,0xae,0xf1,0xef,0xe5}; //For 10
	//{0x64,0xb0,0x53,0x75,0x10,0x2e,0x55,0x18,0x64,0xb0,0x53,0x75,0x10,0x2e,0x55,0x18}; //For 01
	//{0xd3,0x4b,0x29,0xa3,0x7d,0xd4,0x51,0x68,0xd3,0x4b,0x29,0xa3,0x7d,0xd4,0x51,0x68}; //For key 00000000
	//{0x92,0x8c,0x92,0xd3,0x4c,0x4f,0xc8,0xd4,0x92,0x8c,0x92,0xd3,0x4c,0x4f,0xc8,0xd4};
	{0xf3,0xc1,0xb5,0x54,0xb5,0x28,0xdd,0x7c,0xe9,0x30,0xb2,0x5e,0x70,0x4e,0xd7,0xca};

	memcpy(kl_config.ekey2, ekey2, 16);
	memcpy(kl_config.ekey1, ekey1, 16);
	memcpy(kl_config.ecw, ecw, 16);
	kl_config.kl_levels = 3;

	if (g_from_kl)
	{
		ret = AM_KL_SetKeyladder(&kl_config);
		if (ret == AM_FAILURE)
			printf("Set_keyladder failed!!\n");
	}

	memset(&dsc_para, 0, sizeof(AM_DSC_OpenPara_t));
	ret = AM_DSC_Open(dsc, &dsc_para);
//	for (i=0; i<pid_cnt; i++) {
	for (i=0; i<1; i++) {
		for (i=0; i<pid_cnt; i++) {
			if (pids[i]>0) {
				int ret;
				char key[8] = {0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef};

				ret = AM_DSC_SetSource(dsc, src);
				ret = AM_DSC_AllocateChannel(dsc, &dscc[i]);
				ret = AM_DSC_SetChannelPID(dsc, dscc[i], pids[i]);
#if 1
				ret = AM_DSC_SetKey(dsc, dscc[i],
					AM_DSC_KEY_TYPE_ODD | (g_from_kl ? AM_DSC_KEY_FROM_KL : 0),
					(const uint8_t*)key);
				ret = AM_DSC_SetKey(dsc, dscc[i],
					AM_DSC_KEY_TYPE_EVEN | (g_from_kl ? AM_DSC_KEY_FROM_KL : 0),
					(const uint8_t*)key);
				printf("dsc[%d] set default key for pid[%d] kl=%d\n", dsc, pids[i], g_from_kl);
#endif
			}
		}
	}
}

void start_aes(int dsc, int src, int pid_cnt, int *pids)
{
	AM_DSC_OpenPara_t dsc_para;
	int dscc[8];
	int i;
	int ret;
	struct meson_kl_config kl_config;

	char aes_key[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	uint8_t ekey2[16] ={0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf};//{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	uint8_t ekey1[16] ={0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf}; //{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	uint8_t tmp[16];
	uint8_t ecw[16] =
	{0xe8,0x74,0x80,0xbd,0xc2,0x01,0xc8,0x09,0x49,0xdf,0x7c,0x24,0xd1,0x37,0xcb,0x1a};
	//{0x1e,0x20,0xb6,0x55,0xd4,0x50,0x2c,0xf4,0xe0,0x0a,0xe0,0x50,0x63,0xcf,0xce,0x17};
	//{0x57,0xe7,0x85,0xa9,0x5f,0x5d,0x9e,0x9c,0xfc,0xb5,0x3f,0x23,0xe6,0x16,0x5e,0xf7};

	if (g_from_kl)
	{
		memset(&dsc_para, 0, sizeof(AM_DSC_OpenPara_t));
		ret = AM_DSC_Open(dsc, &dsc_para);

		memcpy(kl_config.ekey2, ekey2, 16);
		memcpy(kl_config.ekey1, ekey1, 16);
		memcpy(kl_config.ecw, ecw, 16);
		kl_config.kl_levels = 3;

		ret = AM_KL_SetKeyladder(&kl_config);
		if (ret == AM_FAILURE)
			printf("set_keyladder failed\n");
	} else {

		memset(&dsc_para, 0, sizeof(AM_DSC_OpenPara_t));
		ret = AM_DSC_Open(dsc, &dsc_para);
	}

	for (i=0; i<pid_cnt; i++) {
		if (pids[i]>0) {
			char key[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
			uint8_t iv_key[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

			ret = AM_DSC_SetSource(dsc, src);
			ret = AM_DSC_AllocateChannel(dsc, &dscc[i]);
			ret = AM_DSC_SetChannelPID(dsc, dscc[i], pids[i]);
#if 0
			//Set iv key
			ret = AM_DSC_SetKey(dsc, dscc[i],
				AM_DSC_KEY_TYPE_AES_IV_EVEN | (g_from_kl ? AM_DSC_KEY_FROM_KL : 0),
				iv_key);
			ret = AM_DSC_SetKey(dsc, dscc[i],
				AM_DSC_KEY_TYPE_AES_IV_ODD |( g_from_kl ? AM_DSC_KEY_FROM_KL : 0),
				iv_key);
#endif
			//Set key
			ret=AM_DSC_SetKey(dsc,dscc[i],
				AM_DSC_KEY_TYPE_AES_EVEN | (g_from_kl ? AM_DSC_KEY_FROM_KL : 0),
				(const uint8_t*)key);
			ret=AM_DSC_SetKey(dsc,dscc[i],
				AM_DSC_KEY_TYPE_AES_ODD | (g_from_kl ? AM_DSC_KEY_FROM_KL : 0),
				(const uint8_t*)key);
			printf("dsc[%d] set 16Byte aes default key for pid[%d]\n", dsc, pids[i]);
		}
	}
}


void stop_dsc(int dsc)
{
	AM_DSC_Close(dsc);
}

static int dvr_data_write(int fd, uint8_t *buf, int size)
{
	int ret;
	int left = size;
	uint8_t *p = buf;

	while (left > 0)
	{
		ret = write(fd, p, left);
		if (ret == -1)
		{
			if (errno != EINTR)
			{
				printf("Write DVR data failed: %s\n", strerror(errno));
				break;
			}
			ret = 0;
		}

		left -= ret;
		p += ret;
	}

	return (size - left);
}
static void* dvr_data_thread(void *arg)
{
	DVRData *dd = (DVRData*)arg;
	int cnt;
	uint8_t buf[256*1024];

	printf("Data thread for DVR%d start ,record file will save to '%s'", dd->id, dd->file_name);

	while (dd->running)
	{
		cnt = AM_DVR_Read(dd->id, buf, sizeof(buf), 1000);
		if (cnt <= 0)
		{
			printf("No data available from DVR%d", dd->id);
			usleep(200*1000);
			continue;
		}
		if (cnt > sizeof(buf))
		{
			printf("return size:0x%0x bigger than 0x%0x input buf\n",cnt,sizeof(buf));
			break;
		}
		printf("read from DVR%d return %d bytes\n", dd->id, cnt);
		if (dd->fd != -1)
		{
			dvr_data_write(dd->fd, buf, cnt);
		}
	}

	if (dd->fd != -1)
	{
		close(dd->fd);
		dd->fd = -1;
	}
	printf("Data thread for DVR%d now exit", dd->id);

	return NULL;
}

static void start_data_thread(int dev_no)
{
	DVRData *dd = &data_threads[dev_no];

	if (dd->running)
		return;
	dd->fd = open(dd->file_name, O_TRUNC | O_WRONLY | O_CREAT, 0666);
	if (dd->fd == -1)
	{
		AM_DEBUG(1, "Cannot open record file '%s' for DVR%d", dd->file_name, dd->id);
		return;
	}
	dd->running = 1;
	pthread_create(&dd->thread, NULL, dvr_data_thread, dd);
}

static void stop_data_thread(int dev_no)
{
	DVRData *dd = &data_threads[dev_no];

	if (! dd->running)
		return;
	dd->running = 0;
	pthread_join(dd->thread, NULL);
	AM_DEBUG(1, "Data thread for DVR%d has exit", dd->id);
}

int start_dvr_test(void)
{
	AM_DVR_StartRecPara_t spara;
	AM_Bool_t go = AM_TRUE;
	char buf[256];
	int pid, apid;

	display_usage();
	pat_fid = sdt_fid = -1;

	while (go)
	{
		if (fgets(buf, sizeof(buf), stdin))
		{
			if (!strncmp(buf, "quit", 4))
			{
				go = AM_FALSE;
				continue;
			}
			else
			{
				printf("Unkown command: %s\n", buf);
				display_usage();
			}
		}
	}

	return 0;
}

static void handle_signal(int signal)
{
	int i;
	UNUSED(signal);
	AM_FileEcho("/sys/class/graphics/fb0/blank","0");
	for (i=0; i< DVR_DEV_COUNT; i++) {
		AM_DVR_Close(i);
		AM_DEBUG(1, "Closing DMX%d...", i);
		AM_DMX_Close(i);
		AM_DSC_Close(i);
	}
#ifndef PLAY_FILE
	AM_FEND_Close(FEND_DEV_NO);
#endif
	if (g_from_kl)
		AM_KL_KeyladderRelease();
	exit(0);
}

static void init_signal_handler()
{
	struct sigaction act;
	act.sa_handler = handle_signal;
	sigaction(SIGINT, &act, NULL);
}

static int start_record(int pid0, int pid1, int len)
{
	AM_DVR_StartRecPara_t spara;
	int dev_no, pid_cnt, i;
	int pids[8];
	char name[256] = "a.ts";

	dev_no = dmx_no;
	pid_cnt = len;
	pids[0] = pid0;
	pids[1] = pid1;
	data_threads[dmx_no].id = dev_no;
	strcpy(data_threads[dmx_no].file_name, name);
	spara.pid_count = pid_cnt;
	memcpy(&spara.pids, pids, sizeof(pids));
	if (AM_DVR_StartRecord(dev_no, &spara) == AM_SUCCESS)
	{
		start_data_thread(dev_no);
	}
	printf("started dvr dev_no=%d pid=%d %d..\n", dev_no, pids[0], pids[1]);
	return 0;
}

void arg_usage(char *argv0)
{
	fprintf(stderr, "Usage: %s [-r] [-d] file\n"
					"  -r record\n"
					"  -d descramble\n",
			argv0);
}

static int inject_running=0;
static int inject_loop=0;
static int inject_type=AM_AV_INJECT_MULTIPLEX;
static void* inject_entry(void *arg)
{
	int sock = (int)(long)arg;
	uint8_t buf[32*1024];
	int len, left=0, send, ret;
	int cnt=50;
	AM_AV_VideoStatus_t vStat;
	struct timeval start_tv;
	struct timeval now_tv;
	unsigned int diff_time = 0;
	unsigned int total_len = 0;

	gettimeofday(&start_tv, NULL);
	AM_DEBUG(1, "inject thread start");
	while (inject_running) {
		len = sizeof(buf) - left;
		ret = read(sock, buf+left, len);
		if (ret > 0) {
			AM_DEBUG(1, "recv %d bytes", ret);
/*			if (!cnt){
				cnt=50;
				printf("recv %d\n", ret);
			}
			cnt--; */
			left += ret;
			total_len += ret;
		} else {
			if (inject_loop && ((ret==0) ||(errno == EAGAIN))) {
				printf("loop\n");
				lseek(sock, 0, SEEK_SET);
				left=0;
				continue;
			} else {
				fprintf(stderr, "read file failed [%d:%s] ret=%d left=%d\n", errno, strerror(errno), ret, left);
				break;
			}
		}
		if (left) {
			#if 0
			AM_AV_GetVideoStatus(AV_DEV_NO, &vStat);
			while ( vStat.vb_data > vStat.vb_size*0.6 ) {
				AM_DEBUG(1, "bufferrate too high, vBuf_size:%d, data:%d, free:%d, (%f)",
						vStat.vb_size, vStat.vb_data, vStat.vb_free, vStat.vb_size*0.6);
				usleep(100*1000);
				AM_AV_GetVideoStatus(AV_DEV_NO, &vStat);
			}
			#endif
			gettimeofday(&now_tv, NULL);
			if (now_tv.tv_usec < start_tv.tv_usec) {
				diff_time = (now_tv.tv_sec - 1 - start_tv.tv_sec) * 1000 + (now_tv.tv_usec + 1*1000*1000 - start_tv.tv_usec) / 1000;
			} else {
				diff_time = (now_tv.tv_sec - start_tv.tv_sec) * 1000 + (now_tv.tv_usec - start_tv.tv_usec) / 1000;
			}

			if ( diff_time != 0 && total_len/1024/1024*8*1000/diff_time > 4) {
				usleep(20*1000);
			}

			send = left;
			AM_AV_InjectData(AV_DEV_NO, inject_type, buf, &send, -1);
			if (send) {
				left -= send;
				if (left)
					memmove(buf, buf+send, left);
				//AM_DEBUG(1, "inject %d bytes", send);
			}
		}
	}
	printf("inject thread end");

	return NULL;
}

int main(int argc, char **argv)
{
	AM_FEND_OpenPara_t fpara;
	AM_DMX_OpenPara_t para;
	AM_DVR_OpenPara_t dpara;
	AM_AV_OpenPara_t avpara;
	int i;
	int pid_cnt;
	int pids[8];

	AM_ErrorCode_t err;
	char *name = NULL;
	int loop = 0;
	int pos = 0;
	int rec = 0;
	int dsc = 0;
	int aes = 0;
	int kl = 0;
	int ofd;
	int opt;
	pthread_t th;
	while ((opt = getopt(argc, argv, "rdkax:s:h:")) != -1) {
		switch (opt) {
			case 'r':
				rec = 1;
				break;
			case 'd':
				dsc = 1;
				break;
			case 'k':
				g_from_kl = kl = 1;
				break;
			case 'a':
				aes = 1;
				break;
			case 'x':
				dmx_no = *optarg - 0x30;
				printf("dmx no:%d\n",dmx_no);
				break;
			case 's':
				async_no = *optarg - 0x30;
				printf("async_no:%d\n",async_no);
				break;
			case 'h':
				hiu_no = *optarg - 0x30;
				printf("hiu:%d\n",hiu_no);
				break;
			default: /* '?' */
				arg_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	printf("record=%d; descramble=%d; keyladder=%d; aes=%d, optind=%d\n", rec, dsc, kl, aes, optind);
	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		arg_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	name = argv[optind];
	printf("file name=%s\n", name);

	//VillageFlowSampleTS3, 2nd program
	//pids[0] = 0xd2;
	//pids[1] = 0xdc;
	//dvr orig / redbull video
	//pids[0] = 33;//518;
	//pids[0] = 0x81;
	pids[0] = 128;
	//pids[1] = 34;//0x21;//69;
	//pid_cnt = 2;
	pid_cnt = 1;

	init_signal_handler();
	AM_FileEcho("/sys/class/graphics/fb0/blank","1");

	if (g_from_kl) {
		if (AM_KL_KeyladderInit() != AM_SUCCESS) {
			printf("Keyladder init failed!\n");
			return AM_FAILURE;
		}
	}
	AM_AV_InjectPara_t av_para;
	AM_AV_OpenPara_t av_open_para;
	av_para.vid_fmt = VFORMAT_MPEG12;//VFORMAT_H264
	av_para.aud_fmt = AFORMAT_MPEG;
	av_para.pkg_fmt = PFORMAT_TS;
	av_para.vid_id  = pids[0];
	av_para.aud_id  = pids[1];

	AM_DEBUG(1, "Openning DMX%d...", 0);
	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(dmx_no, &para));
	if (hiu_no == 1)
		AM_DMX_SetSource(dmx_no, AM_DMX_SRC_HIU1);
	else
		AM_DMX_SetSource(dmx_no, AM_DMX_SRC_HIU);

	AM_DEBUG(1, "Openning DVR%d...", 0);
	memset(&dpara, 0, sizeof(dpara));
	AM_TRY(AM_DVR_Open(dmx_no, &dpara));

	data_threads[dmx_no].id =0;
	data_threads[dmx_no].fd = -1;
	data_threads[dmx_no].running = 0;

	AM_TRY(AM_AV_Open(AV_DEV_NO, &av_open_para));
	if (hiu_no == 1)
		AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU1);
	else
		AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);

	AM_AV_StartInject(AV_DEV_NO, &av_para);
	AM_DVR_SetSource(dmx_no,async_no);
	if (dsc)
		start_dsc(0 /*dsc*/, 0 /*src*/, pid_cnt, pids);
	if (aes)
		start_aes(0 /*dsc*/, 0 /*src*/, pid_cnt, pids);
	if (rec)
		start_record(pids[0], pids[1], pid_cnt);
	ofd = open(name, O_RDWR, S_IRUSR);
	printf("open file name:%s, handle:%d \n",name,ofd);
	inject_loop = loop;
	inject_running = 1;
	inject_type = (pids[1]==-1)? AM_AV_INJECT_VIDEO :
		(pids[0]==-1)? AM_AV_INJECT_AUDIO :
		AM_AV_INJECT_MULTIPLEX;
	pthread_create(&th, NULL, inject_entry, (void*)(long)ofd);

	start_dvr_test();

	for (i=0; i< DVR_DEV_COUNT; i++)
	{
		AM_DEBUG(1, "Closing DVR%d...", i);
		if (data_threads[i].running)
			stop_data_thread(i);
		AM_DVR_Close(i);
		AM_DEBUG(1, "Closing DMX%d...", i);
		AM_DMX_Close(i);
	}

#ifndef PLAY_FILE
	AM_FEND_Close(FEND_DEV_NO);
#endif

end:
	AM_FileEcho("/sys/class/graphics/fb0/blank","0");
	if (g_from_kl)
		AM_KL_KeyladderRelease();
	return 0;
}
