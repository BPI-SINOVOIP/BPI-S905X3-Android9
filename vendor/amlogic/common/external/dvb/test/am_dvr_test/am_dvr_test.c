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


#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_av.h>
#include <am_fend.h>
#include <am_dvr.h>
#include <errno.h>
#include <am_dsc.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

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

#define FEND_DEV_NO 0
#define AV_DEV_NO 0
#define PLAY_DMX_DEV_NO 1

typedef struct
{
	int id;
	char file_name[256];
	pthread_t thread;
	int running;
	int fd;
	int is_socket;
	struct sockaddr_in addr;
	int sockaddr_len;
}DVRData;

static int pat_fid, sdt_fid;
static DVRData data_threads[DVR_DEV_COUNT];

static void section_cbf(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	int i;
	
	printf("section:\n");
	for(i=0;i<8;i++)
	{
		printf("%02x ", data[i]);
		if(((i+1)%16)==0) printf("\n");
	}
	
	if((i%16)!=0) printf("\n");
}

static void pes_cbf(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	int i;

#if 0	
	printf("pes:\n");
	for(i=0;i<len;i++)
	{
		printf("%02x ", data[i]);
		if(((i+1)%16)==0) printf("\n");
	}
	
	if((i%16)!=0) printf("\n");
#endif
}

static void display_usage(void)
{
	printf("usages:\n");
	printf("\tsetsrc\t[dvr_no async_fifo_no ts_src]\n");
	printf("\tstart\t[dvr_no FILE pid_cnt pid0...pidN]\n");
	printf("\tstart\t[dvr_no udp://ip:port pid_cnt pid0...pidN]\n");
	printf("\tstop\t[dvr_no]\n");
	printf("\thelp\n");
	printf("\tquit\n");
}

static void start_si(int dmx)
{
#if 1
	struct dmx_sct_filter_params param;

#define SET_FILTER(f, p, t)\
	AM_DMX_AllocateFilter(dmx, &(f));\
	AM_DMX_SetCallback(dmx, f, section_cbf, NULL);\
	memset(&param, 0, sizeof(param));\
	param.pid = p;\
	AM_DMX_SetSecFilter(dmx, f, &param);\
	AM_DMX_SetBufferSize(dmx, f, 32*1024);\
	AM_DMX_StartFilter(dmx, f);
		
//		param.filter.filter[0] = t;\
//		param.filter.mask[0] = 0xff;\
//		param.flags = DMX_CHECK_CRC;\
	if (pat_fid == -1)
	{
		SET_FILTER(pat_fid, 0x10, 0x40)
	}
/*	if (sdt_fid == -1)
	{
		SET_FILTER(sdt_fid, 0x11, 0x42)
	}
*/
	printf("si started.\n");
#endif
}

static void stop_si(int dmx)
{
#if 1
#define FREE_FILTER(f)\
	AM_DMX_StopFilter(dmx, f);\
	AM_DMX_FreeFilter(dmx, f);
	
	if (pat_fid != -1)
	{
		FREE_FILTER(pat_fid)
		pat_fid = -1;
	}
/*
	if (sdt_fid != -1)
	{
		FREE_FILTER(sdt_fid)
		sdt_fid = -1;
	}
*/
	printf("si stopped.\n");
#endif
}

void start_dsc(int dsc, int src, int pid_cnt, int *pids)
{
	AM_DSC_OpenPara_t dsc_para;
	int dscc[8];
	int i;
	int ret;

	memset(&dsc_para, 0, sizeof(AM_DSC_OpenPara_t));
	ret = AM_DSC_Open(dsc, &dsc_para);

	for(i=0; i<pid_cnt; i++) {
		if(pids[i]>0) {
			int ret;
			char key[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

			ret = AM_DSC_SetSource(dsc, src);
			ret=AM_DSC_AllocateChannel(dsc, &dscc[i]);
			ret=AM_DSC_SetChannelPID(dsc, dscc[i], pids[i]);
			ret=AM_DSC_SetKey(dsc,dscc[i],AM_DSC_KEY_TYPE_ODD, (const uint8_t*)key);
			ret=AM_DSC_SetKey(dsc,dscc[i],AM_DSC_KEY_TYPE_EVEN, (const uint8_t*)key);
			printf("dsc[%d] set default key for pid[%d]\n", dsc, pids[i]);
		}
	}
}

void stop_dsc(int dsc)
{
	AM_DSC_Close(dsc);
}

static int socket_open(char *name, int dev_no)
{
	DVRData *dd = &data_threads[dev_no];
	char ip[64];
	int port = 0;
	char *pch = NULL;

	if (strncmp(name, "udp://", 6) != 0) {
		dd->is_socket = 0;
		return -1;
	}

	//this is a socket file
	dd->is_socket = 1;
	printf("is socket\n");

	memset(ip, 0, sizeof(ip));
	if (pch = strchr(name+6, ':'))
		memcpy(ip, name+6, pch-(name+6));
	else
		return -1;

	if (sscanf(pch+1, "%d", &port) != 1)
		return -1;

	if ((dd->fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("socket create fail.\n");
	}
	else
	{
		int on=1;
		setsockopt(dd->fd, SOL_SOCKET, SO_REUSEADDR|SO_BROADCAST, &on, sizeof(on));

		memset(&dd->addr, 0, sizeof(dd->addr));
		dd->addr.sin_family = AF_INET;
		dd->addr.sin_addr.s_addr = inet_addr(name);
		dd->addr.sin_port = htons(port);
		dd->sockaddr_len = sizeof(struct sockaddr_in);
		printf("socket: %s:%d\n", ip, port);
	}

	return dd->fd;
}

static int socket_data_write(int fd, uint8_t *buf, int size, DVRData *dd)
{
	int loop = size >> 10;
	int left = size & 0x400;
	int count = 0;
	int i;

	for (i=0; i<loop; i++) {
		errno = 0;
		count += sendto(fd, buf+(i<<10), 1024, 0, (struct sockaddr *)&dd->addr, dd->sockaddr_len);
		if (count<0)
			printf("sendto fail, errno[%d:%s]\n", errno, strerror(errno));
	}
	if (left) {
		count += sendto(fd, buf+(i<<10), left, 0, (struct sockaddr *)&dd->addr, dd->sockaddr_len);
	}
	return count;
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
				printf("\nWrite DVR data failed: %s\n", strerror(errno));
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
	uint32_t size=0;
	int nodata = 0;

	printf("Data thread for DVR%d start ,record file will save to '%s'\n", dd->id, dd->file_name);

	while (dd->running)
	{
		cnt = AM_DVR_Read(dd->id, buf, sizeof(buf), 1000);
		if (cnt <= 0)
		{
			if(!nodata) { printf("\n"); nodata = 1; }

			printf("\rNo data available from DVR%d", dd->id);
			fflush(stdout);
			usleep(200*1000);
			continue;
		}
		printf("read from DVR%d return %d bytes\n", dd->id, cnt);
		if (dd->fd != -1)
		{
		        if (dd->is_socket)
				size += socket_data_write(dd->fd, buf, cnt, dd);
			else
				size += dvr_data_write(dd->fd, buf, cnt);

			if(nodata) { printf("\n"); nodata = 0; }

			printf("\r[%d] recorded", size);
			fflush(stdout);
		}
	}

	if (dd->fd != -1)
	{
		close(dd->fd);
		dd->fd = -1;
	}
	printf("Data thread for DVR%d now exit\n", dd->id);

	return NULL;
}
	
static void start_data_thread(int dev_no)
{
	DVRData *dd = &data_threads[dev_no];

	if (dd->running)
		return;

	dd->fd = socket_open(dd->file_name, dev_no);
	if (!dd->is_socket)
		dd->fd = open(dd->file_name, O_TRUNC | O_WRONLY | O_CREAT, 0666);
	if (dd->fd == -1)
	{
		printf("Cannot open record file '%s' for DVR%d\n", dd->file_name, dd->id);
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
	printf("Data thread for DVR%d has exit\n", dd->id);
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
			if(!strncmp(buf, "quit", 4))
			{
				go = AM_FALSE;
				continue;
			}
			else if (!strncmp(buf, "start", 5))
			{
				int dev_no, pid_cnt, i;
				int pids[8];
				char name[256];

				/**仅测试最多8个PID*/
				sscanf(buf + 5, "%d %s %d %d %d %d %d %d %d %d %d", &dev_no, name, &pid_cnt, &pids[0], &pids[1],&pids[2],&pids[3],
						&pids[4],&pids[5],&pids[6],&pids[7]);

				if (dev_no < 0 || dev_no >= DVR_DEV_COUNT)
				{
					printf("Invalid DVR dev no, must [%d-%d]\n", 0, DVR_DEV_COUNT-1);
					continue;
				}
				if (pid_cnt > 8)
					pid_cnt = 8;
					
				strcpy(data_threads[dev_no].file_name, name);
				spara.pid_count = pid_cnt;
				memcpy(&spara.pids, pids, sizeof(pids));
				
				if (AM_DVR_StartRecord(dev_no, &spara) == AM_SUCCESS)
				{
					start_data_thread(dev_no);
				}
			}
			else if (!strncmp(buf, "stop", 4))
			{
				int dev_no;
				
				sscanf(buf + 4, "%d", &dev_no);
				if (dev_no < 0 || dev_no >= DVR_DEV_COUNT)
				{
					printf("Invalid DVR dev no, must [%d-%d]\n", 0, DVR_DEV_COUNT-1);
					continue;
				}
				printf("stop record for %d\n", dev_no);
				AM_DVR_StopRecord(dev_no);
				printf("stop data thread for %d\n", dev_no);
				stop_data_thread(dev_no);
			}
			else if (!strncmp(buf, "help", 4))
			{
				display_usage();
			}
			else if (!strncmp(buf, "sistart", 7))
			{
				int dmx;
				sscanf(buf + 7, "%d", &dmx);
				start_si(dmx);
			}
			else if (!strncmp(buf, "sistop", 6))
			{
				int dmx;
				sscanf(buf + 6, "%d", &dmx);
				stop_si(dmx);
			}
			else if (!strncmp(buf, "setsrc", 6))
			{
				int dev_no, fifo_id;
				int tssrc=0;

				sscanf(buf + 6, "%d %d %d", &dev_no, &fifo_id, &tssrc);

				AM_DVR_SetSource(dev_no, fifo_id);
				AM_DMX_SetSource(dev_no, tssrc);
			}
			else if (!strncmp(buf, "dscstart", 8))
			{
				int dsc, src, pid_cnt;
				int pids[8];
				sscanf(buf + 8, "%d %d %d %d %d %d %d %d %d %d %d", &dsc, &src, &pid_cnt, &pids[0], &pids[1],&pids[2],&pids[3],
					&pids[4],&pids[5],&pids[6],&pids[7]);
				start_dsc(dsc, src, pid_cnt, pids);
			}
			else if (!strncmp(buf, "dscstop", 7))
			{
				int dsc;
				sscanf(buf + 7, "%d", &dsc);
				stop_dsc(dsc);
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

void close_all(void) {
	int i;
	for (i=0; i< DVR_DEV_COUNT; i++)
	{
		printf( "Closing DVR%d...\n", i);
		if (data_threads[i].running)
			stop_data_thread(i);
		AM_DVR_Close(i);
		printf("Closing DMX%d...\n", i);
		AM_DMX_Close(i);
	}

	AM_FEND_Close(FEND_DEV_NO);
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


int main(int argc, char **argv)
{
	AM_FEND_OpenPara_t fpara;
	AM_DMX_OpenPara_t para;
	AM_DVR_OpenPara_t dpara;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	int freq = 0;
	int i;
	AM_ErrorCode_t err = 0;

	if(argc>1)
	{
		sscanf(argv[1], "%d", &freq);
	}
	
	if(freq>0)
	{
		memset(&fpara, 0, sizeof(fpara));
		AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));

		p.frequency = freq;
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
		
		AM_TRY(AM_FEND_Lock(FEND_DEV_NO, &p, &status));
		
		if(status&FE_HAS_LOCK)
		{
			printf("locked\n");
		}
		else
		{
			printf("unlocked\n");
		}
	}

	for (i=0; i< DVR_DEV_COUNT; i++)
	{
		printf("Openning DMX%d...", i);
		memset(&para, 0, sizeof(para));
		err = AM_DMX_Open(i, &para);
		printf("[err=%d]\n", err);

		printf("Openning DVR%d...", i);
		memset(&dpara, 0, sizeof(dpara));
		err = AM_DVR_Open(i, &dpara);
		printf("[err=%d]\n", err);
		data_threads[i].id = i;
		data_threads[i].fd = -1;
		data_threads[i].running = 0;
	}

	signal(SIGINT, sigroutine);

	start_dvr_test();

	for (i=0; i< DVR_DEV_COUNT; i++)
	{
		printf( "Closing DVR%d...\n", i);
		if (data_threads[i].running)
			stop_data_thread(i);
		AM_DVR_Close(i);
		printf("Closing DMX%d...\n", i);
		AM_DMX_Close(i);
	}
	
	AM_FEND_Close(FEND_DEV_NO);

	return 0;
}
