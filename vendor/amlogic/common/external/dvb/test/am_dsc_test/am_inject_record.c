#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVR测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-10: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_av.h>
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
#define DVR_DEV_COUNT      (2)

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

static DVRData data_threads[DVR_DEV_COUNT];

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
				printf("Write DVR data failed: %s", strerror(errno));
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
		printf("read from DVR%d return %d bytes", dd->id, cnt);
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

	exit(0);
}

static void init_signal_handler()
{
	struct sigaction act;
	act.sa_handler = handle_signal;
	sigaction(SIGINT, &act, NULL);
}

static int start_record(int pid0, int pid1, int len, char *record_name)
{
	AM_DVR_StartRecPara_t spara;
	int dev_no, pid_cnt, i;
	int pids[8];
	char name[256] = "a.ts";

	dev_no = DVR_DEV_NO;
	pid_cnt = len;
	pids[0] = pid0;
	pids[1] = pid1;
	data_threads[DVR_DEV_NO].id = dev_no;

	if (record_name)
		strcpy(data_threads[dev_no].file_name, record_name);
	else
		strcpy(data_threads[dev_no].file_name, name);

	spara.pid_count = pid_cnt;
	memcpy(&spara.pids, pids, sizeof(pids));
	if (AM_DVR_StartRecord(dev_no, &spara) == AM_SUCCESS)
	{
		start_data_thread(dev_no);
	}
	printf("started dvr dev_no=%d pid=%d %d out file:%s ..\n", dev_no, pids[0], pids[1],data_threads[dev_no].file_name);
	return 0;
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
			if (inject_loop && ((ret == 0) || (errno == EAGAIN))) {
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

int inject_file_and_rec_open(char *inject_name,int vpid, int apid, char *record_name)
{
	AM_DMX_OpenPara_t para;
	AM_DVR_OpenPara_t dpara;
	int pid_cnt = 0;
	int pids[8];
	int loop = 0;
	int ofd;
	pthread_t th;

	printf("inject file name=%s\n", inject_name);

	if (vpid > 0) {
		pids[0] = vpid;
		pid_cnt ++;
	}
	if (apid > 0) {
		pids[1] = apid;
		pid_cnt ++;
	}

	init_signal_handler();
	AM_FileEcho("/sys/class/graphics/fb0/blank","1");

	AM_AV_InjectPara_t av_para;
	AM_AV_OpenPara_t av_open_para;
	av_para.vid_fmt = 0;//VFORMAT_H264
	av_para.aud_fmt = 0;
	av_para.pkg_fmt = PFORMAT_TS;
	av_para.vid_id  = pids[0];
	av_para.aud_id  = pids[1];

	AM_DEBUG(1, "Openning DMX%d...", 0);
	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_HIU);
	AM_DEBUG(1, "Openning DVR%d...", 0);
	memset(&dpara, 0, sizeof(dpara));
	AM_TRY(AM_DVR_Open(DVR_DEV_NO, &dpara));

	data_threads[0].id =0;
	data_threads[0].fd = -1;
	data_threads[0].running = 0;

	AM_TRY(AM_AV_Open(AV_DEV_NO, &av_open_para));
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	AM_AV_StartInject(AV_DEV_NO, &av_para);
	AM_DVR_SetSource(DVR_DEV_NO,ASYNC_FIFO);

	start_record(pids[0], pids[1], pid_cnt,record_name);
	ofd = open(inject_name, O_RDWR, S_IRUSR);

	inject_loop = loop;
	inject_running = 1;
	inject_type = (pids[1]==-1)? AM_AV_INJECT_VIDEO :
		(pids[0]==-1)? AM_AV_INJECT_AUDIO :
		AM_AV_INJECT_MULTIPLEX;
	pthread_create(&th, NULL, inject_entry, (void*)(long)ofd);

	return 0;
}
int inject_file_and_rec_close(void) {
	int i = 0;

	for (i=0; i< DVR_DEV_COUNT; i++)
	{
		AM_DEBUG(1, "Closing DVR%d...", i);
		if (data_threads[i].running)
			stop_data_thread(i);
		AM_DVR_Close(i);
		AM_DEBUG(1, "Closing DMX%d...", i);
		AM_DMX_Close(i);
	}
	AM_FileEcho("/sys/class/graphics/fb0/blank","0");
	return 0;
}

