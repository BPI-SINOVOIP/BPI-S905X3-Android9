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
 * \brief 音视频解码测试
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-11: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <sys/types.h>
#include <am_debug.h>
#include <am_av.h>
#include <am_fend.h>
#include <am_dmx.h>
#include <am_vout.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef ANDROID
#include <sys/socket.h>
#endif
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
/*
*/
#include <errno.h>

#include <am_misc.h>
#include <am_aout.h>
#include <am_dsc.h>
#include "fcntl.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
//#define ENABLE_JPEG_TEST

#define FEND_DEV_NO 0
#define DMX_DEV_NO  1
#define AV_DEV_NO   0
#define OSD_DEV_NO  0
#define AOUT_DEV_NO 0

#ifdef ENABLE_JPEG_TEST
#define OSD_FORMAT  AM_OSD_FMT_COLOR_8888
#define OSD_W       1280
#define OSD_H       720
#endif

#define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)

/****************************************************************************
 * Functions
 ***************************************************************************/

static void usage(void)
{
	printf("usage: am_av_test INPUT\n");
	printf("    INPUT: FILE [loop] [POSITION]\n");
	printf("    INPUT: dvb vpid apid vfmt afmt frequency tssrc fe_modulation\n");
	printf("    INPUT: aes FILE\n");
	printf("    INPUT: ves FILE\n");
#ifdef ENABLE_JPEG_TEST	
	printf("    INPUT: jpeg FILE\n");
#endif
	printf("    INPUT: inject IPADDR PORT vpid apid vfmt afmt\n");
	printf("    INPUT: finject fmt(ES|PS|TS|REAL) FILE vpid apid vfmt afmt\n");
	exit(0);
}

static void normal_help(void)
{
	printf("* blackout\n");
	printf("* noblackout\n");
	printf("* enablev\n");
	printf("* disablev\n");
	printf("* window X Y W H\n");
	printf("* contrast VAL\n");
	printf("* saturation VAL\n");
	printf("* brightness VAL\n");
	printf("* fullscreen\n");
	printf("* normaldisp\n");
#ifdef ENABLE_JPEG_TEST		
	printf("* snapshot\n");
#endif
	printf("* ltrack\n");
	printf("* rtrack\n");
	printf("* swaptrack\n");
	printf("* stereo\n");
	printf("* mute\n");
	printf("* unmute\n");
	printf("* vol VAL\n");
	printf("* aout\n");
	printf("* 16x9\n");
	printf("* 4x3\n");
}

static int normal_cmd(const char *cmd)
{
	printf("cmd:%s\n", cmd);
	
	if(!strncmp(cmd, "blackout", 8))
	{
		AM_AV_EnableVideoBlackout(AV_DEV_NO);
	}
	else if(!strncmp(cmd, "noblackout", 10))
	{
		AM_AV_DisableVideoBlackout(AV_DEV_NO);
	}
	else if(!strncmp(cmd, "enablev", 7))
	{
		AM_AV_EnableVideo(AV_DEV_NO);
	}
	else if(!strncmp(cmd, "disablev", 8))
	{
		AM_AV_DisableVideo(AV_DEV_NO);
	}
	else if(!strncmp(cmd, "window", 6))
	{
		int x, y, w, h;
		
		sscanf(cmd+6, "%d %d %d %d", &x, &y, &w, &h);
		AM_AV_SetVideoWindow(AV_DEV_NO, x, y, w, h);
	}
	else if(!strncmp(cmd, "contrast", 8))
	{
		int v;
		
		sscanf(cmd+8, "%d", &v);
		AM_AV_SetVideoContrast(AV_DEV_NO, v);
	}
	else if(!strncmp(cmd, "saturation", 10))
	{
		int v;
		
		sscanf(cmd+10, "%d", &v);
		AM_AV_SetVideoSaturation(AV_DEV_NO, v);
	}
	else if(!strncmp(cmd, "brightness", 10))
	{
		int v;
		
		sscanf(cmd+10, "%d", &v);
		AM_AV_SetVideoBrightness(AV_DEV_NO, v);
	}
	else if(!strncmp(cmd, "fullscreen", 10))
	{
		AM_AV_SetVideoDisplayMode(AV_DEV_NO, AM_AV_VIDEO_DISPLAY_FULL_SCREEN);
	}
	else if(!strncmp(cmd, "normaldisp", 10))
	{
		AM_AV_SetVideoDisplayMode(AV_DEV_NO, AM_AV_VIDEO_DISPLAY_NORMAL);
	}
	else if(!strncmp(cmd, "snapshot", 8))
	{
#ifndef ANDROID
#ifdef ENABLE_JPEG_TEST
		AM_OSD_Surface_t *s;
		AM_AV_SurfacePara_t para;
		AM_ErrorCode_t ret = AM_SUCCESS;
		
		para.option = 0;
		para.angle  = 0;
		para.width  = 0;
		para.height = 0;
		
		ret = AM_AV_GetVideoFrame(AV_DEV_NO, &para, &s);
		
		if(ret==AM_SUCCESS)
		{
			AM_OSD_Rect_t sr, dr;
			AM_OSD_BlitPara_t bp;
			AM_OSD_Surface_t *disp;
			
			sr.x = 0;
			sr.y = 0;
			sr.w = s->width;
			sr.h = s->height;
			dr.x = 200;
			dr.y = 200;
			dr.w = 200;
			dr.h = 200;
			memset(&bp, 0, sizeof(bp));
			
			AM_OSD_GetSurface(OSD_DEV_NO, &disp);
			AM_OSD_Blit(s, &sr, disp, &dr, &bp);
			AM_OSD_Update(OSD_DEV_NO, NULL);
			AM_OSD_DestroySurface(s);
		}
#endif		
#endif
	}
	else if(!strncmp(cmd, "ltrack", 6))
	{
		printf("ltrack\n");
		AM_AOUT_SetOutputMode(AOUT_DEV_NO, AM_AOUT_OUTPUT_DUAL_LEFT);
	}	
	else if(!strncmp(cmd, "rtrack", 6))
	{
		printf("rtrack\n");
		AM_AOUT_SetOutputMode(AOUT_DEV_NO, AM_AOUT_OUTPUT_DUAL_RIGHT);
	}	
	else if(!strncmp(cmd, "swaptrack", 9))
	{
		printf("strack\n");
		AM_AOUT_SetOutputMode(AOUT_DEV_NO, AM_AOUT_OUTPUT_SWAP);
	}	
	else if(!strncmp(cmd, "stereo", 6))
	{
		printf("stereo\n");
		AM_AOUT_SetOutputMode(AOUT_DEV_NO, AM_AOUT_OUTPUT_STEREO);
	}	
	else if(!strncmp(cmd, "mute", 4))
	{
		printf("mute\n");
		AM_AOUT_SetMute(AOUT_DEV_NO, AM_TRUE);
	}	
	else if(!strncmp(cmd, "unmute", 6))
	{
		printf("unmute\n");
		AM_AOUT_SetMute(AOUT_DEV_NO, AM_FALSE);
	}	
	else if(!strncmp(cmd, "vol", 3))
	{
		int v;
		
		sscanf(cmd+3, "%d", &v);
		AM_AOUT_SetVolume(AOUT_DEV_NO, v);
	}	
	else if(!strncmp(cmd, "aout", 4))
	{
		AM_Bool_t mute;
		AM_AOUT_OutputMode_t out;
		int vol;
		
		AM_AOUT_GetMute(AOUT_DEV_NO, &mute);
		AM_AOUT_GetOutputMode(AOUT_DEV_NO, &out);
		AM_AOUT_GetVolume(AOUT_DEV_NO, &vol);

		printf("Audio Out Info:\n");
		printf("Mute: %s\n", mute?"true":"false");
		printf("Mode: %s\n", (out==AM_AOUT_OUTPUT_STEREO)?"STEREO":
							(out==AM_AOUT_OUTPUT_DUAL_LEFT)?"DUALLEFT":
							(out==AM_AOUT_OUTPUT_DUAL_RIGHT)?"DUALRIGHT":
							(out==AM_AOUT_OUTPUT_SWAP)?"SWAP":"???");
		printf("Volume: %d\n", vol);
	}
	else if(!strncmp(cmd, "vout", 4))
	{
		AM_AV_VideoAspectRatio_t as;
		AM_AV_VideoAspectMatchMode_t am;
		
		AM_AV_GetVideoAspectRatio(AV_DEV_NO, &as);
		AM_AV_GetVideoAspectMatchMode(AV_DEV_NO, &am);

		printf("Video Out Info:\n");
		printf("AspectRatio: %s\n", (as==AM_AV_VIDEO_ASPECT_AUTO)?"AUTO":
							(as==AM_AV_VIDEO_ASPECT_16_9)?"16x9":
							(as==AM_AV_VIDEO_ASPECT_4_3)?"4x3":"???");
		printf("AspectMatch: %s\n", (am==AM_AV_VIDEO_ASPECT_MATCH_IGNORE)?"IGNORE":
							(am==AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX)?"LETTERBOX":
							(am==AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN)?"PANSCAN":
							(am==AM_AV_VIDEO_ASPECT_MATCH_COMBINED)?"COMBINED":"???");
	}
	else if(!strncmp(cmd, "auto", 4))
	{
		AM_AV_SetVideoAspectRatio(AV_DEV_NO, AM_AV_VIDEO_ASPECT_AUTO);
	}	
	else if(!strncmp(cmd, "16x9", 4))
	{
		AM_AV_SetVideoAspectRatio(AV_DEV_NO, AM_AV_VIDEO_ASPECT_16_9);
	}
	else if(!strncmp(cmd, "4x3", 3))
	{
		AM_AV_SetVideoAspectRatio(AV_DEV_NO, AM_AV_VIDEO_ASPECT_4_3);
	}
	else if(!strncmp(cmd, "igno", 4))
	{
		AM_AV_SetVideoAspectMatchMode(AV_DEV_NO, AM_AV_VIDEO_ASPECT_MATCH_IGNORE);
	}
	else if(!strncmp(cmd, "lett", 4))
	{
		AM_AV_SetVideoAspectMatchMode(AV_DEV_NO, AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX);
	}
	else if(!strncmp(cmd, "pans", 4))
	{
		AM_AV_SetVideoAspectMatchMode(AV_DEV_NO, AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN);
	}
	else if(!strncmp(cmd, "comb", 4))
	{
		AM_AV_SetVideoAspectMatchMode(AV_DEV_NO, AM_AV_VIDEO_ASPECT_MATCH_COMBINED);
	}
	else if(!strncmp(cmd, "dsc", 3))
	{
		#define DSC_DEV_NO 0
		#define PID_PATH "/sys/class/amstream/ports"
		AM_DSC_OpenPara_t dsc_para;
		int dsccv, dscca;
		int vpid=0, apid=0;
		char buf[512];
		char *p = buf;
		int dev = (cmd[3]=='0')? 0 : (cmd[3]=='1')? 1 : 0;
		int fd = open(PID_PATH, O_RDONLY);
		if(fd<0) {
			printf("Can not open "PID_PATH);
			return 0;
		}
		read(fd, buf, 512);
		while((p[0]!='V') || (p[0]!='A'))
		{
			if(p[0]=='V' && p[1]=='i' && p[2]=='d' && p[3]==':')
				sscanf(&p[4], "%d", &vpid);
			else if(p[0]=='A' && p[1]=='i' && p[2]=='d' && p[3]==':')
				sscanf(&p[4], "%d", &apid);
			if(vpid>0 && apid>0)
				break;
			if(p++ == &buf[512])
				return 1;
		}

		if(vpid>0 || apid>0) {
			int ret;
			//unsigned char key[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
			unsigned char key[8] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef};
			ret = AM_DSC_Open(dev, &dsc_para);
			ret = AM_DSC_SetSource(dev, AM_DSC_SRC_DMX1);
			printf("keys(Default/key[format-HH HH HH HH HH HH HH HH]):");
			//p = fgets(buf, sizeof(buf), stdin);
			if(p[0]!='\n') {
			}
			if(vpid>0) {
				ret=AM_DSC_AllocateChannel(dev, &dsccv);
				ret=AM_DSC_SetChannelPID(dev, dsccv, vpid);
				ret=AM_DSC_SetKey(dev,dsccv,AM_DSC_KEY_TYPE_ODD, key);
				ret=AM_DSC_SetKey(dev,dsccv,AM_DSC_KEY_TYPE_EVEN, key);
				printf("set default key for pid[%d]\n", vpid);
			}
			if(apid>0) {
				ret=AM_DSC_AllocateChannel(dev, &dscca);
				ret=AM_DSC_SetChannelPID(dev, dscca, apid);
				ret=AM_DSC_SetKey(dev,dscca,AM_DSC_KEY_TYPE_ODD, key);
				ret=AM_DSC_SetKey(dev,dscca,AM_DSC_KEY_TYPE_EVEN, key);
				printf("set default key for pid[%d]\n", apid);
			}
		}
	}
	else if(!strncmp(cmd, "adon", 4))
	{
		int pid, fmt;
		AM_ErrorCode_t err;
		sscanf(cmd+5, "%i %i", &pid, &fmt);
		err = AM_AV_SetAudioAd(AV_DEV_NO, 1, pid, fmt);
		printf("SetAudioAd [on] [%d:%d] [err:%d]\n", pid, fmt, err);
	}
	else if(!strncmp(cmd, "adoff", 5))
	{
		AM_ErrorCode_t err = AM_AV_SetAudioAd(AV_DEV_NO, 0, 0, 0);
		printf("SetAudioAd [off] [err:%d]\n", err);
	}
	else if(!strncmp(cmd, "pts", 3))
	{
		uint64_t vpts, apts;
		AM_ErrorCode_t err = AM_AV_GetVideoPts(AV_DEV_NO, &vpts);
		err |= AM_AV_GetAudioPts(AV_DEV_NO, &apts);
		printf("PTS v[0x%llx]\n    a[0x%llx]\n[err:%d]\n", (unsigned long long)vpts, (unsigned long long)apts, err);
	}
	else
	{
		return 0;
	}
	
	return 1;
}

static void file_play(const char *name, int loop, int pos)
{
	int running = 1;
	char buf[256];
	int paused = 0;
	AM_DMX_OpenPara_t para;
	
	memset(&para, 0, sizeof(para));
	
	AM_DMX_Open(DMX_DEV_NO, &para);
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_HIU);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	AM_AV_StartFile(AV_DEV_NO, name, loop, pos);
	printf("play \"%s\" loop:%s from:%d\n", name, loop?"y":"n", pos);
	
	while(running)
	{
		printf("********************\n");
		printf("* commands:\n");
		printf("* quit\n");
		printf("* pause\n");
		printf("* resume\n");
		printf("* ff SPEED\n");
		printf("* fb SPEED\n");
		printf("* seek POS\n");
		printf("* status\n");
		printf("* info\n");
		normal_help();
		printf("********************\n");
		
		if(fgets(buf, 256, stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				running = 0;
			}
			else if(!strncmp(buf, "pause", 5))
			{
				AM_AV_PauseFile(AV_DEV_NO);
				paused = 1;
			}
			else if(!strncmp(buf, "resume", 6))
			{
				AM_AV_ResumeFile(AV_DEV_NO);
				paused = 0;
			}
			else if(!strncmp(buf, "ff", 2))
			{
				int speed = 0;
				
				sscanf(buf+2, "%d", &speed);
				AM_AV_FastForwardFile(AV_DEV_NO, speed);
			}
			else if(!strncmp(buf, "fb", 2))
			{
				int speed = 0;
				
				sscanf(buf+2, "%d", &speed);
				AM_AV_FastBackwardFile(AV_DEV_NO, speed);
			}
			else if(!strncmp(buf, "seek", 4))
			{
				int pos = 0;
				
				sscanf(buf+4, "%d", &pos);
				AM_AV_SeekFile(AV_DEV_NO, pos, !paused);
			}
			else if(!strncmp(buf, "status", 6))
			{
				AM_AV_PlayStatus_t status;
				
				AM_AV_GetPlayStatus(AV_DEV_NO, &status);
				printf("stats: duration:%d current:%d\n", status.duration, status.position);
			}
			else if(!strncmp(buf, "info", 4))
			{
				AM_AV_FileInfo_t info;
				
				AM_AV_GetCurrFileInfo(AV_DEV_NO, &info);
				printf("info: duration:%d size:%lld\n", info.duration, info.size);
			}
			else
			{
				normal_cmd(buf);
			}
		}
	}
	
	AM_DMX_Close(DMX_DEV_NO);
}

static void video_callback(long dev_no, int event_type, void *param, void *data)
{
	switch (event_type) {
	case AM_AV_EVT_VIDEO_RESOLUTION_CHANGED:
		{
			AM_AV_VideoStatus_t *status = param;
			printf("[evt] video resolution changed: %dx%d\n",
				status->src_w, status->src_h);
		}
		break;
	case AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED:
		printf("[evt] video aspect ratio changed: %ld\n", (long)param);
		break;
	case AM_AV_EVT_VIDEO_AVAILABLE:
		printf("[evt] video available\n");
		break;
	case AM_AV_EVT_VIDEO_AFD_CHANGED:
		{
			AM_USERDATA_AFD_t *afd = param;
			printf("[evt] video afd changed: flg[0x%x] fmt[0x%x]\n",
				afd->af_flag, afd->af);
		}
		break;
	default:
		break;
	}
}

static void dvb_play(int vpid, int apid, int ppid, int vfmt, int afmt, int freq, int src, int dmx, int femod, int argc, char *argv[])
{
	int running = 1;
	char buf[256];
	AM_DMX_OpenPara_t para;
	int SNR;
	
	memset(&para, 0, sizeof(para));
	AM_DMX_Open(dmx, &para);
	AM_DMX_SetSource(dmx, src);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_DMX0 + dmx);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_RESOLUTION_CHANGED, video_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, video_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_AVAILABLE, video_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_AFD_CHANGED, video_callback, NULL);

	if(freq>0)
	{
		AM_FEND_OpenPara_t para;
		struct dvb_frontend_parameters p;
		fe_status_t status;
		int mode;
		
		memset(&para, 0, sizeof(para));

		printf("Input fontend mode: (0-DVBC, 1-DVBT, 2-DVBS, 3-DTMB,4-ATSC)\n");
		printf("mode(0/1/2/3/4): ");
		scanf("%d", &mode);
		switch(mode)
		{
			case 0: para.mode = FE_QAM; break;
			case 1: para.mode = FE_OFDM; break;
			case 2: para.mode = FE_QPSK; break;
			case 3: para.mode = FE_DTMB; break;
			case 4: para.mode = FE_ATSC; break;
			default: para.mode = FE_DTMB; break;
		}
		
		AM_FEND_Open(FEND_DEV_NO, &para);

		AM_FEND_SetMode(FEND_DEV_NO, para.mode);
	
		
		p.frequency = freq;
		if(para.mode == FE_ATSC)
		{
			printf("femode: %d\n",femod);
			p.u.vsb.modulation = femod;
		}
		else if(para.mode == FE_QAM)
		{
			p.u.qam.symbol_rate = 6875000;
			p.u.qam.fec_inner = FEC_AUTO;
			p.u.qam.modulation = femod;//QAM_64;
		}
		else
		{
			p.inversion = INVERSION_AUTO;
			p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
			p.u.ofdm.code_rate_HP = FEC_AUTO;
			p.u.ofdm.code_rate_LP = FEC_AUTO;
			p.u.ofdm.constellation = QAM_AUTO;
			p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
			p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
			p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;

		}
		AM_FEND_Lock(FEND_DEV_NO, &p, &status);

		printf("lock status: 0x%x\n", status);
	}
	
	AM_AV_StartTSWithPCR(AV_DEV_NO, vpid, apid, ppid, vfmt, afmt);

	while(argc--)
		normal_cmd(argv[argc]);

	while(running)
	{
	#if 1
		printf("********************\n");
		printf("* commands:\n");
		printf("* quit\n");
		normal_help();
		printf("********************\n");
		if(fgets(buf, 256, stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				running = 0;
			}
			else
			{
				normal_cmd(buf);
			}
		}
	//#else
		SNR = 0;
		AM_FEND_GetSNR(FEND_DEV_NO,&SNR);
		printf("SNR = %d\n",SNR);
		sleep(1);
	#endif
		
	}
	
	if(freq>0)
	{
		AM_FEND_Close(FEND_DEV_NO);
	}
	
	AM_DMX_Close(dmx);
}

static void ves_play(const char *name, int vfmt)
{
	int running = 1;
	char buf[256];
	
	AM_AV_StartVideoES(AV_DEV_NO, vfmt, name);
	
	while(running)
	{
		printf("********************\n");
		printf("* commands:\n");
		printf("* quit\n");
		normal_help();
		printf("********************\n");
		
		if(fgets(buf, 256, stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				running = 0;
			}
			else
			{
				normal_cmd(buf);
			}
		}
	}
}

static void aes_play(const char *name, int afmt, int times)
{
	int running = 1;
	char buf[256];
	
	AM_AV_StartAudioES(AV_DEV_NO, afmt, name, times);
	
	while(running)
	{
		printf("********************\n");
		printf("* commands:\n");
		printf("* quit\n");
		printf("********************\n");
		
		if(fgets(buf, 256, stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				running = 0;
			}
		}
	}
}

static void jpeg_show(const char *name)
{
#ifndef ANDROID
#ifdef ENABLE_JPEG_TEST
	AM_AV_JPEGInfo_t info;
	AM_OSD_Surface_t *surf, *osd_surf;
	AM_OSD_BlitPara_t blit_para;
	AM_AV_SurfacePara_t s_para;
	
	AM_AV_GetJPEGInfo(AV_DEV_NO, name, &info);
	
	printf("JPEG:\"%s\" width:%d height:%d comp_num:%d\n", name, info.width, info.height, info.comp_num);
	
	s_para.angle  = AM_AV_JPEG_CLKWISE_0;
	s_para.width  = info.width*2;
	s_para.height = info.height*2;
	s_para.option = 0;
	
	if(AM_AV_DecodeJPEG(AV_DEV_NO, name, &s_para, &surf)==AM_SUCCESS)
	{
		AM_OSD_Rect_t sr, dr;
		
		AM_OSD_Enable(AV_DEV_NO);
		AM_OSD_GetSurface(AV_DEV_NO, &osd_surf);
		
		blit_para.enable_alpha = AM_FALSE;
		blit_para.enable_key   = AM_FALSE;
		blit_para.enable_global_alpha = AM_FALSE;
		blit_para.enable_op    = AM_FALSE;
		
		sr.x = 0;
		sr.y = 0;
		sr.w = surf->width;
		sr.h = surf->height;
		dr.x = 0;
		dr.y = 0;
		dr.w = osd_surf->width;
		dr.h = osd_surf->height;
		dr.w = surf->width;
		dr.h = surf->height;
		
		AM_OSD_Blit(surf, &sr, osd_surf, &dr, &blit_para);
		AM_OSD_Update(AV_DEV_NO, NULL);
		AM_OSD_DestroySurface(surf);
	}
#endif
#endif
}

static int inject_running=0;
static int inject_loop=0;
static int inject_type=AM_AV_INJECT_MULTIPLEX;

static void* inject_entry(void *arg)
{
	int sock = (int)(long)arg;
	static uint8_t buf[32*1024];
	int len, left=0, send, ret;
	int cnt=10;
	
	AM_DEBUG(1, "inject thread start");
	while(inject_running)
	{
		len = sizeof(buf)-left;
		ret = read(sock, buf+left, len);
		if(ret>0)
		{
			//AM_DEBUG(1, "recv %d bytes", ret);
			if(!cnt){
				cnt=10;
				printf("recv %d\n", ret);
			}
			cnt--;
			left += ret;
		}
		else
		{
			if(inject_loop && ((ret==0) ||(errno == EAGAIN))){
				printf("loop\n");
				lseek(sock, 0, SEEK_SET);
				left=0;
				continue;
			} else {
				fprintf(stderr, "read file failed [%d:%s]\n", errno, strerror(errno));
				break;
			}
		}
/*		else if(ret<0)
		{
			AM_DEBUG(1, "read failed");
			break;
		}
*/
		while (left)
		{
			send = left;
			AM_AV_InjectData(AV_DEV_NO, inject_type, buf, &send, -1);
			if(send)
			{
				left -= send;
				if(left)
					memmove(buf, buf+send, left);
				//AM_DEBUG(1, "inject %d bytes", send);
			}
			usleep(100);
		}
	}
	AM_DEBUG(1, "inject thread end");
	
	return NULL;
}

static void inject_play(struct in_addr *addr, int port, int vpid, int apid, int vfmt, int afmt)
{
	int sock, ret;
	struct sockaddr_in in_addr;
	AM_AV_InjectPara_t para;
	pthread_t th;
	int running = 1;
	char buf[256];
	AM_DMX_OpenPara_t dmx_p;

	memset(&dmx_p, 0, sizeof(dmx_p));
	
	AM_DMX_Open(DMX_DEV_NO, &dmx_p);
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_HIU);
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock==-1)
	{
		AM_DEBUG(1, "create socket failed");
		return;
	}
	
	in_addr.sin_family = AF_INET;
	in_addr.sin_port   = htons(port);
	in_addr.sin_addr   = *addr;
	
	ret = connect(sock, (struct sockaddr*)&in_addr, sizeof(in_addr));
	if(ret==-1)
	{
		AM_DEBUG(1, "connect failed");
		goto end;
	}
	
	AM_DEBUG(1, "connect ok");
	
	para.vid_fmt = vfmt;
	para.aud_fmt = afmt;
	para.pkg_fmt = PFORMAT_TS;
	para.vid_id  = vpid;
	para.aud_id  = apid;
	
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	AM_AV_StartInject(AV_DEV_NO, &para);
	
	inject_running = 1;
	pthread_create(&th, NULL, inject_entry, (void*)(long)sock);
	
	while(running)
	{
		printf("********************\n");
		printf("* commands:\n");
		printf("* quit\n");
		normal_help();
		printf("********************\n");
		
		if(fgets(buf, 256, stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				running = 0;
			}
			else
			{
				normal_cmd(buf);
			}
		}
	}
	
	inject_running = 0;
	pthread_join(th, NULL);
	
	AM_AV_StopInject(AV_DEV_NO);
end:
	AM_DMX_Close(DMX_DEV_NO);
	close(sock);
}

static int pall=0;
static int timeout = 60*3;
#define USER_MAX 10
static int u_pid[USER_MAX]={[0 ... USER_MAX-1] = -1};
static int u_para[USER_MAX]={[0 ... USER_MAX-1] = 0};
static char *u_ext[USER_MAX]={[0 ... USER_MAX-1] = NULL};
static int u_bs[USER_MAX]={[0 ... USER_MAX-1] = 0};
static int u_para_g;
static FILE *fp[USER_MAX];
/*
   u_para format:
	d1 - 0:sec 1:pes
	d2 - 1:crc : sec only
	d3 - 1:print
	d5 - 1:swfilter
	d6 - 1:ts_tap :pes only
	d4 - 1:w2file
*/
#define UPARA_TYPE     0xf
#define UPARA_CRC      0xf0
#define UPARA_PR       0xf00
#define UPARA_SF       0xf000
#define UPARA_DMX_TAP  0xf0000
#define UPARA_FILE     0xf00000

#define get_upara(_i) (u_para[(_i)]? u_para[(_i)] : u_para_g)


static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	int u = (int)(long)user_data;

	if(pall) {
		int i;
		printf("data:\n");
		for(i=0;i<len;i++)
		{
			printf("%02x ", data[i]);
			if(((i+1)%16)==0) printf("\n");
		}
		if((i%16)!=0) printf("\n");
	}

	{
		if(!user_data) {
			printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8]);
			return;
		}

		if(get_upara(u-1)&UPARA_PR)
			printf("[%d:%d] %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", u-1, u_pid[u-1],
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
		if(get_upara(u-1)&UPARA_FILE){
			int ret = fwrite(data, 1, len, fp[(int)(long)user_data-1]);
			if(ret!=len)
				printf("data w lost\n");
		}
	}

}

static int get_section(int dmx, int timeout)
{
	int fid_user[USER_MAX];
	int i;

	struct dmx_sct_filter_params param;
	struct dmx_pes_filter_params pparam;

	for(i=0; i<USER_MAX; i++) {
		if (VALID_PID(u_pid[i])) {
			AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_user[i]));

			AM_TRY(AM_DMX_SetCallback(dmx, fid_user[i], dump_bytes, (void*)(long)(i+1)));

			if(u_bs[i])
				AM_TRY(AM_DMX_SetBufferSize(dmx, fid_user[i], u_bs[i]));

			if(get_upara(i)&UPARA_TYPE) {/*pes*/
				memset(&pparam, 0, sizeof(pparam));
				pparam.pid = u_pid[i];
				pparam.pes_type = DMX_PES_OTHER;
				pparam.input = DMX_IN_FRONTEND;
				pparam.output = DMX_OUT_TAP;
				if(get_upara(i)&UPARA_DMX_TAP)
					pparam.output = DMX_OUT_TSDEMUX_TAP;
				if(get_upara(i)&UPARA_SF)
					pparam.flags |= 0x100;
				AM_TRY(AM_DMX_SetPesFilter(dmx, fid_user[i], &pparam));

			} else {/*sct*/
				int v[16] = {[0 ... 15] = 0};
				int m[16] = {[0 ... 15] = 0};
				int ii;
				memset(&param, 0, sizeof(param));
				param.pid = u_pid[i];
				if(u_ext[i]) {
				sscanf(u_ext[i], "%x:%x,%x:%x,%x:%x,%x:%x"
					",%x:%x,%x:%x,%x:%x,%x:%x"
					",%x:%x,%x:%x,%x:%x,%x:%x"
					",%x:%x,%x:%x,%x:%x,%x:%x",
					&v[0], &m[0], &v[1], &m[1],
					&v[2], &m[2], &v[3], &m[3],
					&v[4], &m[4], &v[5], &m[5],
					&v[6], &m[6], &v[7], &m[7],
					&v[8], &m[8], &v[9], &m[9],
					&v[10], &m[10], &v[11], &m[11],
					&v[12], &m[12], &v[13], &m[13],
					&v[14], &m[14], &v[15], &m[15]);
				for(ii=0; ii<16; ii++) {
					if(m[ii]) {
						param.filter.filter[ii] = v[ii];
						param.filter.mask[ii] = m[ii];
						printf("ext%d: [%d]%x:%x\n", i, ii, v[ii], m[ii]);
					}
				}
				}
				if(get_upara(i)&UPARA_CRC)
					param.flags = DMX_CHECK_CRC;
				if(get_upara(i)&UPARA_SF)
					param.flags |= 0x100;
				AM_TRY(AM_DMX_SetSecFilter(dmx, fid_user[i], &param));
			}

			AM_TRY(AM_DMX_SetBufferSize(dmx, fid_user[i], 64*1024));

			if (get_upara(i)&UPARA_FILE) {
				char name[32];
				sprintf(name, "/storage/external_storage/u_%d.dump", i);
				fp[i] = fopen(name, "wb");
				if(fp[i])
					printf("file open:[%s]\n", name);
			}

			AM_TRY(AM_DMX_StartFilter(dmx, fid_user[i]));
		}
	}

	sleep(timeout);

	for (i=0; i<USER_MAX; i++) {
		if (VALID_PID(u_pid[i])) {
			AM_TRY(AM_DMX_StopFilter(dmx, fid_user[i]));
			AM_TRY(AM_DMX_FreeFilter(dmx, fid_user[i]));
			if((get_upara(i)&UPARA_FILE) && fp[i])
				fclose(fp[i]);
		}
	}

	return 0;
}


int get_para(char *argv)
{
	#define CASE(name, len, type, var) \
		if(!strncmp(argv, name"=", (len)+1)) { \
			sscanf(&argv[(len)+1], type, &var); \
			printf("param["name"] => "type"\n", var); \
		}
	#define CASESTR(name, len, type, var) \
		if(!strncmp(argv, name"=", (len)+1)) { \
			var = &argv[(len)+1]; \
			printf("param["name"] => "type"\n", var); \
		}

	CASE("timeout", 7, "%i", timeout)
	else CASE("pall", 4, "%i", pall)
	else CASE("pid0", 4, "%i", u_pid[0])
	else CASE("pid1", 4, "%i", u_pid[1])
	else CASE("pid2", 4, "%i", u_pid[2])
	else CASE("pid3", 4, "%i", u_pid[3])
	else CASE("pid4", 4, "%i", u_pid[4])
	else CASE("para0", 5, "%x", u_para[0])
	else CASE("para1", 5, "%x", u_para[1])
	else CASE("para2", 5, "%x", u_para[2])
	else CASE("para3", 5, "%x", u_para[3])
	else CASE("para4", 5, "%x", u_para[4])
	else CASESTR("ext0", 4, "%s", u_ext[0])
	else CASESTR("ext1", 4, "%s", u_ext[1])
	else CASESTR("ext2", 4, "%s", u_ext[2])
	else CASESTR("ext3", 4, "%s", u_ext[3])
	else CASESTR("ext4", 4, "%s", u_ext[4])
	else CASE("bs0", 3, "%i", u_bs[0])
	else CASE("bs1", 3, "%i", u_bs[1])
	else CASE("bs2", 3, "%i", u_bs[2])
	else CASE("bs3", 3, "%i", u_bs[3])
	else CASE("bs4", 3, "%i", u_bs[4])
	else CASE("para", 4, "%x", u_para_g)

	return 0;
}


static void inject_play_file(char *name, int fmt, int vpid, int apid, int vfmt, int afmt, int loop)
{
	AM_AV_InjectPara_t para;
	pthread_t th;
	int running = 1;
	char buf[256];
	AM_DMX_OpenPara_t dmx_p;
	int fd;

	memset(&dmx_p, 0, sizeof(dmx_p));

	AM_DMX_Open(DMX_DEV_NO, &dmx_p);
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_HIU);

	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_RESOLUTION_CHANGED, video_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, video_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_AVAILABLE, video_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_AFD_CHANGED, video_callback, NULL);

	fd = open(name, O_RDWR, S_IRUSR);
	if(fd<0)
		printf("file[%s] open fail. [%d:%s]\n", name, errno, strerror(errno));
	else
		printf("file[%s] opened loop\n", name);

	para.vid_fmt = vfmt;
	para.aud_fmt = afmt;
	para.pkg_fmt = fmt;
	para.vid_id  = vpid;
	para.aud_id  = apid;

	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	AM_AV_StartInject(AV_DEV_NO, &para);

	inject_loop = loop;
	inject_running = 1;
	inject_type = (!VALID_PID(apid))? AM_AV_INJECT_VIDEO :
					(!VALID_PID(vpid))? AM_AV_INJECT_AUDIO :
							AM_AV_INJECT_MULTIPLEX;

	printf("file inject: [fmt:%d][file:%s][v:%d-%d][a:%d-%d][loop:%d]\n", fmt, name, vpid, vfmt, apid, afmt, loop);
	pthread_create(&th, NULL, inject_entry, (void*)(long)fd);

	get_section(DMX_DEV_NO, timeout);

	while(running)
	{
		printf("********************\n");
		printf("* commands:\n");
		printf("* quit\n");
		normal_help();
		printf("********************\n");

		if(fgets(buf,sizeof(buf), stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				running = 0;
			}
			else
			{
				normal_cmd(buf);
			}
		}
	}

	inject_running = 0;
	pthread_join(th, NULL);

	AM_AV_StopInject(AV_DEV_NO);
end:
	AM_DMX_Close(DMX_DEV_NO);
	close(fd);
}


int main(int argc, char **argv)
{
	AM_AV_OpenPara_t para;
#ifdef ENABLE_JPEG_TEST	
	AM_OSD_OpenPara_t osd_p;
#endif
	int apid=-1, vpid=-1, vfmt=0, afmt=0, freq=0;
	int i, v, loop=0, pos=0, port=1234;
	struct in_addr addr;
	AM_AOUT_OpenPara_t aout_para;

	int afd = 0;

	if(argc<2)
		usage();
	
	memset(&para, 0, sizeof(para));
	AM_TRY(AM_AV_Open(AV_DEV_NO, &para));
	
#ifdef ENABLE_JPEG_TEST
	memset(&osd_p, 0, sizeof(osd_p));
	osd_p.format = AM_OSD_FMT_COLOR_8888;
	osd_p.width  = 1280;
	osd_p.height = 720;
	osd_p.output_width  = 1280;
	osd_p.output_height = 720;
	osd_p.enable_double_buffer = AM_FALSE;
#ifndef ANDROID
	AM_TRY(AM_OSD_Open(OSD_DEV_NO, &osd_p));
#endif
#endif

	memset(&aout_para, 0, sizeof(aout_para));
	AM_AOUT_Open(AOUT_DEV_NO, &aout_para);
	
	if(strcmp(argv[1], "dvb")==0)
	{
		int tssrc=AM_DMX_SRC_TS0;
		int fe_mod = VSB_8;

		if(argc<3)
			usage();
		
		vpid = strtol(argv[2], NULL, 0);
		
		if(argc>3)
			apid = strtol(argv[3], NULL, 0);

		if(argc>4)
			vfmt = strtol(argv[4], NULL, 0);
		
		if(argc>5)
			afmt = strtol(argv[5], NULL, 0);
		
		if(argc>6)
			freq = strtol(argv[6], NULL, 0);

		if(argc>7)
			tssrc = strtol(argv[7], NULL, 0);
		
		if(argc>8)
			fe_mod = strtol(argv[8], NULL, 0);
	
		dvb_play(vpid, apid, vpid, vfmt, afmt,
			freq, tssrc, DMX_DEV_NO, fe_mod, argc, argv);
	}
	else if(strcmp(argv[1], "ves")==0)
	{
		char *name;
		int vfmt = 0;
		
		if(argc<3)
			usage();
		
		name = argv[2];
		
		if(argc>3)
			vfmt = strtol(argv[3], NULL, 0);
		
		ves_play(name, vfmt);
	}
	else if(strcmp(argv[1], "aes")==0)
	{
		char *name;
		int afmt = 0;
		int times = 1;
		
		if(argc<3)
			usage();
		
		name = argv[2];
		
		if(argc>3)
			afmt = strtol(argv[3], NULL, 0);
		if(argc>4)
			times = strtol(argv[4], NULL, 0);
		
		aes_play(name, afmt, times);
	}
#ifdef ENABLE_JPEG_TEST	
	else if(strcmp(argv[1], "jpeg")==0)
	{
		if(argc<3)
			usage();
		
		jpeg_show(argv[2]);
	}
#endif	
	else if(strcmp(argv[1], "inject")==0)
	{
		if(argc<5)
			usage();
		
		inet_pton(AF_INET, argv[2], &addr);
		port = strtol(argv[3], NULL, 0);
		vpid = strtol(argv[4], NULL, 0);
		
		if(argc>5)
			apid = strtol(argv[5], NULL, 0);
		
		if(argc>6)
			vfmt = strtol(argv[6], NULL, 0);
		
		if(argc>7)
			afmt = strtol(argv[7], NULL, 0);
		
		inject_play(&addr, port, vpid, apid, vfmt, afmt);
	}
	else if(strcmp(argv[1], "finject")==0)
	{
		int fmt = PFORMAT_TS;
		int loop = 0;

		if(argc<5)
			usage();

		fmt = strtol(argv[2], NULL, 0);
		printf("argv[2]=%s fmt=%d\n", argv[2], fmt);
		vpid = strtol(argv[4], NULL, 0);

		if(argc>5)
			apid = strtol(argv[5], NULL, 0);

		if(argc>6)
			vfmt = strtol(argv[6], NULL, 0);

		if(argc>7)
			afmt = strtol(argv[7], NULL, 0);

		if(argc>8)
			loop = strtol(argv[8], NULL, 0);

		for(i=1; i< argc; i++)
			get_para(argv[i]);

		inject_play_file(argv[3], fmt, vpid, apid, vfmt, afmt, loop);
	}
	else if (strcmp(argv[1], "dvb2") == 0)
	{
		int i;
		int ppid = -1;
		int tssrc = AM_DMX_SRC_TS0;
		int fe_mod = VSB_8;
		int dmx = DMX_DEV_NO;

		for (i = 2; i < argc; i++) {
			const char *s = argv[i];
			if (strncmp(s, "v=", 2) == 0)
				sscanf(s, "v=%i:%i", &vpid, &vfmt);
			else if (strncmp(s, "a=", 2) == 0)
				sscanf(s, "a=%i:%i", &apid, &afmt);
			else if (strncmp(s, "p=", 2) == 0)
				sscanf(s, "p=%i", &ppid);
			else if (strncmp(s, "src=", 4) == 0)
				sscanf(s, "src=%i", &tssrc);
			else if (strncmp(s, "dmx=", 4) == 0)
				sscanf(s, "dmx=%i", &dmx);
			else if (strncmp(s, "afd=", 4) == 0)
				sscanf(s, "afd=%i", &afd);
		}
		if ((ppid == -1) && VALID_PID(vpid))
			ppid = vpid;
		if (afd != 0) {
			AM_AV_Close(AV_DEV_NO);
			para.afd_enable = afd;
			AM_TRY(AM_AV_Open(AV_DEV_NO, &para));
		}
		dvb_play(vpid, apid, ppid, vfmt, afmt,
			freq, tssrc, dmx, fe_mod, argc, argv);
	}
	else
	{
		for(i=2; i<argc; i++)
		{
			if(!strcmp(argv[i], "loop"))
				loop = 1;
			else if(sscanf(argv[i], "%d", &v)==1)
				pos = v;
		}

		file_play(argv[1], loop, pos);
	}
	
#ifndef ANDROID
#ifdef ENABLE_JPEG_TEST
	AM_OSD_Close(OSD_DEV_NO);
#endif
#endif
	AM_AV_Close(AV_DEV_NO);
	AM_AOUT_Close(AOUT_DEV_NO);

	AM_DSC_Close(0);
	AM_DSC_Close(1);

	return 0;	
}

