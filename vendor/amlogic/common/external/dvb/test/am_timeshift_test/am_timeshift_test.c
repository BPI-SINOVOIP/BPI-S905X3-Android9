/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Timeshift≤‚ ‘≥Ã–Ú
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2011-11-25: create the document
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_av.h>
#include <am_fend.h>
#include <am_dvr.h>
#include <errno.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define DMX_DEV_NO 0
#define DMX_DEV_NO_AV 1
#define DVR_DEV_NO 0
#define ASYNC_FIFO_ID 0
#define FEND_DEV_NO 0
#define AV_DEV_NO 0

#define OUTPUT_FILE "/storage/external_storage/sda1/timeshift.ts"

typedef struct
{
	int id;
	pthread_t thread;
	int running;
	int fd;

	int ts;

	AM_DVR_StartRecPara_t dvr;
}DVRData;

static DVRData data_thread;


static void display_usage(void)
{
	fprintf(stderr, "==================\n");
	fprintf(stderr, "*start\n");
	fprintf(stderr, "*stop\n");
	fprintf(stderr, "*play\n");
	fprintf(stderr, "*pause\n");
	fprintf(stderr, "*resume\n");
	fprintf(stderr, "*ff speed(1=2X,2=4X,3=8X)\n");
	fprintf(stderr, "*fb speed(1=2X,2=4X,3=8X)\n");
	fprintf(stderr, "*seek time_in_second\n");
	fprintf(stderr, "*quit\n");
	fprintf(stderr, "==================\n");
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
				fprintf(stderr, "Write DVR data failed: %s\n", strerror(errno));
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

	fprintf(stderr, "Record start...\n");
	AM_DVR_StartRecord(dd->id, &dd->dvr);

	while (dd->running)
	{
		cnt = AM_DVR_Read(dd->id, buf, sizeof(buf), 1000);
		fprintf(stderr, "AM_DVR_Read cnt is %d \n",cnt);
		if (cnt <= 0)
		{
			fprintf(stderr, "No data available from DVR%d\n", dd->id);
			usleep(200*1000);
			continue;
		}

		if (AM_AV_TimeshiftFillData(AV_DEV_NO, buf, cnt) != AM_SUCCESS)
		{
			fprintf(stderr, "timeshift inject data failed!\n");
			break;
		}
	}

	AM_DVR_StopRecord(dd->id);

	fprintf(stderr, "Record now exit\n");

	return NULL;
}

static void start_data_thread()
{
	DVRData *dd = &data_thread;

	if (dd->running)
		return;

	dd->running = 1;
	pthread_create(&dd->thread, NULL, dvr_data_thread, dd);
}

static void stop_data_thread()
{
	DVRData *dd = &data_thread;

	if (! dd->running)
		return;
	dd->running = 0;
	pthread_join(dd->thread, NULL);
	fprintf(stderr, "Data thread for DVR%d has exit\n", dd->id);
}

static void player_info_callback(long dev_no, int event_type, void *param, void *data)
{
	if (event_type == AM_AV_EVT_PLAYER_UPDATE_INFO)
	{
	#if 0
		player_info_t *info = (player_info_t*)param;
		int cur = info->current_time/1000;

		fprintf(stderr, "=====Timeshift: cur time:%08d %02d:%02d:%02d / full time:%08d %02d:%02d:%02d ======\r", cur,cur/3600,
			(cur%3600)/60, cur%60,info->full_time, info->full_time/3600, (info->full_time%3600)/60, info->full_time%60);
	#endif
	}
}

int start_timeshift_test(int vpid, int apid, int vfmt, int afmt, char *file, int duration)
{
	AM_Bool_t go = AM_TRUE;
	char buf[256];
	DVRData *dd = &data_thread;

	display_usage();

	while (go)
	{
		if (fgets(buf, sizeof(buf), stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				go = AM_FALSE;
				if (AM_AV_StopTimeshift(AV_DEV_NO) == AM_SUCCESS)
					stop_data_thread();
				continue;
			}
			else if (!strncmp(buf, "start", 5))
			{
				AM_AV_TimeshiftPara_t tpara;
				int prepare = 0;

				if (!strncmp(buf, "start-pre", 9))
					prepare = 1;

				memset(&tpara, 0, sizeof(tpara));

				tpara.dmx_id = DMX_DEV_NO_AV;
				strncpy(tpara.file_path, file, 256);
				tpara.mode = AM_AV_TIMESHIFT_MODE_TIMESHIFTING;
				tpara.media_info.duration = duration;
				tpara.media_info.vid_pid = (prepare) ? 0x1fff : vpid;
				tpara.media_info.vid_fmt = vfmt;
				tpara.media_info.aud_cnt = 1;
				tpara.media_info.audios[0].pid = (prepare) ? 0x1fff : apid;
				tpara.media_info.audios[1].fmt = afmt;
				tpara.start_paused = (prepare) ? 1 : 0;

				dd->dvr.pids[0] = vpid;
				dd->dvr.pids[1] = apid;
				dd->dvr.pids[2] = 0;//pat
				dd->dvr.pid_count = 3;
				if (AM_AV_StartTimeshift(AV_DEV_NO, &tpara) == AM_SUCCESS)
				{
					start_data_thread();
				}

			}
			else if (!strncmp(buf, "stop", 4))
			{
				if (AM_AV_StopTimeshift(AV_DEV_NO) == AM_SUCCESS)
					stop_data_thread();
			}
			else if (!strncmp(buf, "play", 4))
			{
				fprintf(stderr, "before time shift play \n");
				AM_AV_PlayTimeshift(AV_DEV_NO);
				fprintf(stderr, "after time shift play \n");
			}
			else if (!strncmp(buf, "pause", 5))
			{
				AM_AV_PauseTimeshift(AV_DEV_NO);
			}
			else if (!strncmp(buf, "resume", 6))
			{
				AM_AV_ResumeTimeshift(AV_DEV_NO);
			}
			else if (!strncmp(buf, "ff", 2))
			{
				int speed;

				sscanf(buf + 2, "%d", &speed);

				AM_AV_FastForwardTimeshift(AV_DEV_NO, speed);
				printf("fast forward timeshift dev_no is %d, speed is %d  \n",AV_DEV_NO,speed);
			}
			else if (!strncmp(buf, "fb", 2))
			{
				int speed;

				sscanf(buf + 2, "%d", &speed);

				AM_AV_FastBackwardTimeshift(AV_DEV_NO, speed);
			}
			else if (!strncmp(buf, "seek", 4))
			{
				int time;

				sscanf(buf + 4, "%d", &time);

				AM_AV_SeekTimeshift(AV_DEV_NO, time, AM_TRUE);
			}
			else if (!strncmp(buf, "av", 2))
			{
				AM_AV_StartTS(AV_DEV_NO, vpid, apid, vfmt, afmt);
			}
			else
			{
				fprintf(stderr, "Unkown command: %s\n", buf);
				display_usage();
			}
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	AM_FEND_OpenPara_t fpara;
	AM_DMX_OpenPara_t para;
	AM_DVR_OpenPara_t dpara;
	AM_AV_OpenPara_t apara;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	int freq = -1;
	int vpid=0x100, apid=0x100, vfmt=0, afmt=0;
	int duration=60;
	char *file = OUTPUT_FILE;
	int tssrc=0;

	if(argc<2)
	{
		printf("Usage: %s freq vpid apid vfmt afmt file tssrc\n\n", argv[0]);
		return 1;
	}
#if 1
	if(argc>1)
	{
		sscanf(argv[1], "%d", &freq);
	}
	if(argc>2)
	{
		sscanf(argv[2], "%i", &vpid);
	}
	if(argc>3)
	{
		sscanf(argv[3], "%i", &apid);
	}
	if(argc>4)
	{
		sscanf(argv[4], "%i", &vfmt);
	}
	if(argc>5)
	{
		sscanf(argv[5], "%i", &afmt);
	}
	if(argc>6)
	{
		file = argv[6];
	}
	if(argc>7)
	{
		sscanf(argv[7], "%i", &tssrc);
	}

	if(freq > 0)
	{
		memset(&fpara, 0, sizeof(fpara));
		fpara.mode = FE_OFDM;
		AM_FEND_Open(FEND_DEV_NO, &fpara);

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
#endif

	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));
	AM_DMX_SetSource(DMX_DEV_NO, tssrc);

	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO_AV, &para));
	AM_DMX_SetSource(DMX_DEV_NO_AV, AM_DMX_SRC_HIU);

	memset(&apara, 0, sizeof(apara));
	AM_TRY(AM_AV_Open(AV_DEV_NO, &apara));
	AM_AV_SetTSSource(AV_DEV_NO, tssrc);

	memset(&dpara, 0, sizeof(dpara));
	AM_TRY(AM_DVR_Open(DVR_DEV_NO, &dpara));
	AM_DVR_SetSource(DVR_DEV_NO, ASYNC_FIFO_ID);


	data_thread.id = 0;
	data_thread.fd = -1;
	data_thread.running = 0;

	AM_EVT_Subscribe(0, AM_AV_EVT_PLAYER_UPDATE_INFO, player_info_callback, NULL);

	start_timeshift_test(vpid, apid, vfmt, afmt, file, duration);

	AM_EVT_Unsubscribe(0, AM_AV_EVT_PLAYER_UPDATE_INFO, player_info_callback, NULL);

	if (data_thread.running)
		stop_data_thread();
	AM_DVR_Close(DVR_DEV_NO);
	AM_AV_Close(AV_DEV_NO);
	AM_DMX_Close(DMX_DEV_NO);
	AM_FEND_Close(FEND_DEV_NO);

	return 0;
}
