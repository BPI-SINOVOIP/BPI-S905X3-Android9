
/*
// Copyright (c) 2017 Amlogic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
*/

/*
* !!!ATTENTATION:
* MOST COPY FROM KERNEL, DONT MODIFY.
*/

#include "AmVinfo.h"
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define VOUT_DEV "/dev/display"
#define VOUT2_DEV "/dev/display2"


/* vout_ioctl */
#define VOUT_IOC_TYPE            'C'
#define VOUT_IOC_NR_GET_VINFO    0x0
#define VOUT_IOC_NR_SET_VINFO    0x1
#define VOUT_IOC_CMD_GET_VINFO   \
		_IOR(VOUT_IOC_TYPE, VOUT_IOC_NR_GET_VINFO, struct vinfo_base_s)
#define VOUT_IOC_CMD_SET_VINFO   \
		_IOW(VOUT_IOC_TYPE, VOUT_IOC_NR_SET_VINFO, struct vinfo_base_s)

/*
*                COPY FROM Vinfo.c
*/

struct vmode_match_s {
	const char *name;
	enum vmode_e mode;
};

static struct vmode_match_s vmode_match_table[] = {
	{"480i60hz",      VMODE_480I},
	{"480irpt",       VMODE_480I_RPT},
	{"480cvbs",       VMODE_480CVBS},
	{"480p60hz",	  VMODE_480P},
	{"480prtp", 	  VMODE_480P_RPT},
	{"576i50hz",      VMODE_576I},
	{"576irpt",       VMODE_576I_RPT},
	{"576cvbs",       VMODE_576CVBS},
	{"576p50hz",      VMODE_576P},
	{"576prpt",       VMODE_576P_RPT},
	{"720p60hz",      VMODE_720P},
	{"720p50hz",      VMODE_720P_50HZ},
	{"768p60hz",      VMODE_768P},
	{"768p50hz",      VMODE_768P_50HZ},
	{"1080i60hz",     VMODE_1080I},
	{"1080i50hz",     VMODE_1080I_50HZ},
	{"1080p60hz",     VMODE_1080P},
	{"1080p25hz",     VMODE_1080P_25HZ},
	{"1080p30hz",     VMODE_1080P_30HZ},
	{"1080p50hz",     VMODE_1080P_50HZ},
	{"1080p24hz",     VMODE_1080P_24HZ},
	{"2160p30hz",     VMODE_4K2K_30HZ},
	{"2160p25hz",     VMODE_4K2K_25HZ},
	{"2160p24hz",     VMODE_4K2K_24HZ},
	{"smpte24hz",     VMODE_4K2K_SMPTE},
	{"smpte25hz",     VMODE_4K2K_SMPTE_25HZ},
	{"smpte30hz",     VMODE_4K2K_SMPTE_30HZ},
	{"smpte50hz420",  VMODE_4K2K_SMPTE_50HZ_Y420},
	{"smpte50hz",     VMODE_4K2K_SMPTE_50HZ},
	{"smpte60hz420",  VMODE_4K2K_SMPTE_60HZ_Y420},
	{"smpte60hz",     VMODE_4K2K_SMPTE_60HZ},
	{"4k2k5g",        VMODE_4K2K_FAKE_5G},
	{"2160p60hz420",  VMODE_4K2K_60HZ_Y420},
	{"2160p60hz",     VMODE_4K2K_60HZ},
	{"2160p50hz420",  VMODE_4K2K_50HZ_Y420},
	{"2160p50hz",     VMODE_4K2K_50HZ},
	{"2160p5g",       VMODE_4K2K_5G},
	{"1024x768p60hz", VMODE_1024x768P},
	{"1440x900p60hz", VMODE_1440x900P},
	{"640x480p60hz",  VMODE_640x480P},
	{"1280x1024p60hz",VMODE_1280x1024P},
	{"800x600p60hz",  VMODE_800x600P},
	{"1680x1050p60hz",  VMODE_1680x1050P},
	{"1024x600p60hz", VMODE_1024x600P},
	{"2560x1600p60hz",  VMODE_2560x1600P},
	{"2560x1440p60hz",VMODE_2560x1440P},
	{"2560x1080p60hz",  VMODE_2560x1080P},
	{"1920x1200p60hz",  VMODE_1920x1200P},
	{"1600x1200p60hz", VMODE_1600x1200P},
	{"1600x900p60hz",  VMODE_1600x900P},
	{"1360x768p60hz",VMODE_1360x768P},
	{"1280x800p60hz",  VMODE_1280x800P},
	{"480x320p60hz",  VMODE_480x320P},
	{"800x480p60hz",  VMODE_800x480P},
	{"1280x480p60hz",  VMODE_1280x480P},
	{"4k1k120hz420",  VMODE_4K1K_120HZ_Y420},
	{"4k1k120hz",     VMODE_4K1K_120HZ},
	{"4k1k100hz420",  VMODE_4K1K_100HZ_Y420},
	{"4k1k100hz",     VMODE_4K1K_100HZ},
	{"4k05k240hz420", VMODE_4K05K_240HZ_Y420},
	{"4k05k240hz",    VMODE_4K05K_240HZ},
	{"4k05k200hz420", VMODE_4K05K_200HZ_Y420},
	{"4k05k200hz",    VMODE_4K05K_200HZ},
	{"panel",         VMODE_LCD},
	{"pal_m",         VMODE_PAL_M},
	{"pal_n",         VMODE_PAL_N},
	{"ntsc_m",        VMODE_NTSC_M},
	{"3440x1440p50hz",VMODE_3440_1440_50HZ},
	{"3440x1440p60hz",VMODE_3440_1440_60HZ},
	{"invalid",       VMODE_INIT_NULL},
};

/*
* Modified.
*/
enum vmode_e vmode_name_to_mode(const char *str)
{
	uint32_t i;
	enum vmode_e vmode = VMODE_MAX;

	for (i = 0; i < ARRAY_SIZE(vmode_match_table); i++) {
		if (strstr(str, vmode_match_table[i].name)) {
			vmode = vmode_match_table[i].mode;
			break;
		}
#if 0
		if (strcmp(vmode_match_table[i].name, str) == 0) {
			vmode = vmode_match_table[i].mode;
			break;
		}
#endif
	}

	return vmode;
}

const char *vmode_mode_to_name(enum vmode_e vmode)
{
	uint32_t i;
	const char *str = "invalid";

	for (i = 0; i < ARRAY_SIZE(vmode_match_table); i++) {
		if (vmode == vmode_match_table[i].mode) {
			str = vmode_match_table[i].name;
			break;
		}
	}

	return str;
}


/*
*                COPY FROM TV_VOUT.h/TV_VOUT.c
*/
static const struct vinfo_s tv_info[] = {
	{ /* VMODE_480I */
		.name              = "480i60hz",
		.mode              = VMODE_480I,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_480I_RPT */
		.name              = "480i_rpt",
		.mode              = VMODE_480I_RPT,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_480CVBS*/
		.name              = "480cvbs",
		.mode              = VMODE_480CVBS,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_480P */
	    .name			   = "480p60hz",
		.mode			   = VMODE_480P,
		.width			   = 720,
		.height 		   = 480,
		.field_height	   = 480,
        .aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk		   = 27000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_480P_RPT */
       .name              = "480p_rpt",
       .mode              = VMODE_480P_RPT,
       .width             = 720,
       .height            = 480,
       .field_height      = 480,
       .aspect_ratio_num  = 4,
       .aspect_ratio_den  = 3,
       .sync_duration_num = 60,
       .sync_duration_den = 1,
       .video_clk         = 27000000,
       .viu_color_fmt     = TVIN_YUV444,
    },
	{ /* VMODE_1024x768P */
		.name		   = "1024x768p60hz",
		.mode		   = VMODE_1024x768P,
		.width		   = 1024,
		.height 	   = 768,
		.field_height	   = 768,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 65000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1440x900P */
		.name		   = "1440x900p60hz",
		.mode		   = VMODE_1440x900P,
		.width		   = 1440,
		.height 	   = 900,
		.field_height	   = 900,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 106500000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_640x480P */
		.name		   = "640x480p60hz",
		.mode		   = VMODE_640x480P,
		.width		   = 640,
		.height 	   = 480,
		.field_height	   = 480,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 25175000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1280x1024P */
		.name		   = "1280x1024p60hz",
		.mode		   = VMODE_1280x1024P,
		.width		   = 1280,
		.height 	   = 1024,
		.field_height	   = 1024,
		.aspect_ratio_num  = 5,
		.aspect_ratio_den  = 4,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 108000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_800x600P */
		.name		   = "800x600p60hz",
		.mode		   = VMODE_800x600P,
		.width		   = 800,
		.height 	   = 600,
		.field_height	   = 600,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 40000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1680x1050P */
		.name		   = "1680x1050p60hz",
		.mode		   = VMODE_1680x1050P,
		.width		   = 1680,
		.height 	   = 1050,
		.field_height	   = 1050,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 119000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},

	{ /* VMODE_1024x600P */
		.name		   = "1024x600p60hz",
		.mode		   = VMODE_1024x600P,
		.width		   = 1024,
		.height 	   = 600,
		.field_height	   = 600,
		.aspect_ratio_num  = 17,
		.aspect_ratio_den  = 10,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 50400000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_2560x1600P */
		.name		   = "2560x1600p60hz",
		.mode		   = VMODE_2560x1600P,
		.width		   = 2560,
		.height 	   = 1600,
		.field_height	   = 1600,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 348500000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_2560x1440P */
		.name		   = "2560x1440p60hz",
		.mode		   = VMODE_2560x1440P,
		.width		   = 2560,
		.height 	   = 1440,
		.field_height	   = 1440,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 312250000,//241500000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_2560x1080P */
		.name		   = "2560x1080p60hz",
		.mode		   = VMODE_2560x1080P,
		.width		   = 2560,
		.height 	   = 1080,
		.field_height	   = 1080,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 27,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 198000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1920x1200P */
		.name		   = "1920x1200p60hz",
		.mode		   = VMODE_1920x1200P,
		.width		   = 1920,
		.height 	   = 1200,
		.field_height	   = 1200,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 193250000,
		.viu_color_fmt	   = TVIN_YUV444,
	},

	{ /* VMODE_1600x1200P */
		.name		   = "1600x1200p60hz",
		.mode		   = VMODE_1600x1200P,
		.width		   = 1600,
		.height 	   = 1200,
		.field_height	   = 1200,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 162000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1600x900P */
		.name		   = "1600x900p60hz",
		.mode		   = VMODE_1600x900P,
		.width		   = 1600,
		.height 	   = 900,
		.field_height	   = 900,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 108000000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1360x768P */
		.name		   = "1360x768p60hz",
		.mode		   = VMODE_1360x768P,
		.width		   = 1360,
		.height 	   = 768,
		.field_height	   = 768,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 85500000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1280x800P */
		.name		   = "1280x800p60hz",
		.mode		   = VMODE_1280x800P,
		.width		   = 1280,
		.height 	   = 800,
		.field_height	   = 800,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 83500000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_480x320P */
		.name		   = "480x320p60hz",
		.mode		   = VMODE_480x320P,
		.width		   = 480,
		.height 	   = 320,
		.field_height	   = 320,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 25200000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_800x480P */
		.name		   = "800x480p60hz",
		.mode		   = VMODE_800x480P,
		.width		   = 800,
		.height 	   = 480,
		.field_height	   = 480,
		.aspect_ratio_num  = 5,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 297600,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_1280x480P */
		.name		   = "1280x480p60hz",
		.mode		   = VMODE_1280x480P,
		.width		   = 1280,
		.height 	   = 480,
		.field_height	   = 480,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk	   = 432000,
		.viu_color_fmt	   = TVIN_YUV444,
	},
	{ /* VMODE_576I */
		.name              = "576i50hz",
		.mode              = VMODE_576I,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_576I_RPT */
		.name              = "576i_rpt",
		.mode              = VMODE_576I_RPT,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_576I */
		.name              = "576cvbs",
		.mode              = VMODE_576CVBS,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_576P */
		.name              = "576p50hz",
		.mode              = VMODE_576P,
		.width             = 720,
		.height            = 576,
		.field_height      = 576,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_576P_RPT */
		.name              = "576p_rpt",
		.mode              = VMODE_576P_RPT,
		.width             = 720,
		.height            = 576,
		.field_height      = 576,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_720P */
		.name              = "720p60hz",
		.mode              = VMODE_720P,
		.width             = 1280,
		.height            = 720,
		.field_height      = 720,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_768P_50hz */
		.name              = "768p50hz",
		.mode              = VMODE_768P_50HZ,
		.width             = 1280,
		.height            = 768,
		.field_height      = 768,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_768P_60hz */
		.name              = "768p60hz",
		.mode              = VMODE_768P,
		.width             = 1280,
		.height            = 768,
		.field_height      = 768,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080I */
		.name              = "1080i60hz",
		.mode              = VMODE_1080I,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 540,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080P */
		.name              = "1080p60hz",
		.mode              = VMODE_1080P,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_720P_50hz */
		.name              = "720p50hz",
		.mode              = VMODE_720P_50HZ,
		.width             = 1280,
		.height            = 720,
		.field_height      = 720,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080I_50HZ */
		.name              = "1080i50hz",
		.mode              = VMODE_1080I_50HZ,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 540,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080P_30HZ */
		.name              = "1080p30hz",
		.mode              = VMODE_1080P_30HZ,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080P_50HZ */
		.name              = "1080p50hz",
		.mode              = VMODE_1080P_50HZ,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080P_25HZ */
		.name              = "1080p25hz",
		.mode              = VMODE_1080P_25HZ,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080P_24HZ */
		.name              = "1080p24hz",
		.mode              = VMODE_1080P_24HZ,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 74250000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_30HZ */
		.name              = "2160p30hz",
		.mode              = VMODE_4K2K_30HZ,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_25HZ */
		.name              = "2160p25hz",
		.mode              = VMODE_4K2K_25HZ,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_24HZ */
		.name              = "2160p24hz",
		.mode              = VMODE_4K2K_24HZ,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE */
		.name              = "smpte24hz",
		.mode              = VMODE_4K2K_SMPTE,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 256,
		.aspect_ratio_den  = 135,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE_25HZ */
		.name              = "smpte25hz",
		.mode              = VMODE_4K2K_SMPTE_25HZ,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 256,
		.aspect_ratio_den  = 135,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE_30HZ */
		.name              = "smpte30hz",
		.mode              = VMODE_4K2K_SMPTE_30HZ,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 256,
		.aspect_ratio_den  = 135,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE_50HZ */
		.name              = "smpte50hz",
		.mode              = VMODE_4K2K_SMPTE_50HZ,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 256,
		.aspect_ratio_den  = 135,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE_60HZ */
		.name              = "smpte60hz",
		.mode              = VMODE_4K2K_SMPTE_60HZ,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 256,
		.aspect_ratio_den  = 135,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_FAKE_5G */
		.name              = "4k2k5g",
		.mode              = VMODE_4K2K_FAKE_5G,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 495000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_60HZ_Y420 */
		.name              = "2160p60hz420",
		.mode              = VMODE_4K2K_60HZ_Y420,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE_60HZ_Y420 */
		.name              = "smpte60hz420",
		.mode              = VMODE_4K2K_SMPTE_60HZ_Y420,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_60HZ */
		.name              = "2160p60hz",
		.mode              = VMODE_4K2K_60HZ,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K1K_100HZ_Y420 */
		.name              = "4k1k100hz420",
		.mode              = VMODE_4K1K_100HZ_Y420,
		.width             = 3840,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 32,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 100,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K1K_100HZ */
		.name              = "4k1k100hz",
		.mode              = VMODE_4K1K_100HZ,
		.width             = 3840,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 32,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 100,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K1K_120HZ_Y420 */
		.name              = "4k1k120hz420",
		.mode              = VMODE_4K1K_120HZ_Y420,
		.width             = 3840,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 32,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 120,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K1K_120HZ */
		.name              = "4k1k120hz",
		.mode              = VMODE_4K1K_120HZ,
		.width             = 3840,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 32,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 120,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K05K_200HZ_Y420 */
		.name              = "4k05k200hz420",
		.mode              = VMODE_4K05K_200HZ_Y420,
		.width             = 3840,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 200,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K05K_200HZ */
		.name              = "4k05k200hz",
		.mode              = VMODE_4K05K_200HZ,
		.width             = 3840,
		.height            = 540,
		.field_height      = 540,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 200,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K05K_240HZ_Y420 */
		.name              = "4k05k240hz420",
		.mode              = VMODE_4K05K_240HZ_Y420,
		.width             = 3840,
		.height            = 540,
		.field_height      = 540,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 240,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K05K_240HZ */
		.name              = "4k05k240hz",
		.mode              = VMODE_4K05K_240HZ,
		.width             = 3840,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 240,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_50HZ_Y420 */
		.name              = "2160p50hz420",
		.mode              = VMODE_4K2K_50HZ_Y420,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_SMPTE_50HZ_Y420 */
		.name              = "smpte50hz420",
		.mode              = VMODE_4K2K_SMPTE_50HZ_Y420,
		.width             = 4096,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_4K2K_50HZ */
		.name              = "2160p50hz",
		.mode              = VMODE_4K2K_50HZ,
		.width             = 3840,
		.height            = 2160,
		.field_height      = 2160,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_vga */
		.name              = "vga",
		.mode              = VMODE_VGA,
		.width             = 640,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 25175000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_SVGA */
		.name              = "svga",
		.mode              = VMODE_SVGA,
		.width             = 800,
		.height            = 600,
		.field_height      = 600,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 40000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_XGA */
		.name              = "xga",
		.mode              = VMODE_XGA,
		.width             = 1024,
		.height            = 768,
		.field_height      = 768,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 65000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_sxga */
		.name              = "sxga",
		.mode              = VMODE_SXGA,
		.width             = 1280,
		.height            = 1024,
		.field_height      = 1024,
		.aspect_ratio_num  = 5,
		.aspect_ratio_den  = 4,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 108000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_wsxga */
		.name              = "wsxga",
		.mode              = VMODE_WSXGA,
		.width             = 1440,
		.height            = 900,
		.field_height      = 900,
		.aspect_ratio_num  = 8,
		.aspect_ratio_den  = 5,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 88750000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_fhdvga */
		.name              = "fhdvga",
		.mode              = VMODE_FHDVGA,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
/* VMODE for 3D Frame Packing */
	{ /* VMODE_1080FP60HZ */
		.name              = "1080fp60hz",
		.mode              = VMODE_1080FP60HZ,
		.width             = 1920,
		.height            = 1080 + 1125,
		.field_height      = 1080 + 1125,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080FP50HZ */
		.name              = "1080fp50hz",
		.mode              = VMODE_1080FP50HZ,
		.width             = 1920,
		.height            = 1080 + 1125,
		.field_height      = 1080 + 1125,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 297000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080FP30HZ */
		.name              = "1080fp30hz",
		.mode              = VMODE_1080FP30HZ,
		.width             = 1920,
		.height            = 1080 + 1125,
		.field_height      = 1080 + 1125,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 30,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080FP25HZ */
		.name              = "1080fp25hz",
		.mode              = VMODE_1080FP25HZ,
		.width             = 1920,
		.height            = 1080 + 1125,
		.field_height      = 1080 + 1125,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 25,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_1080FP24HZ */
		.name              = "1080fp24hz",
		.mode              = VMODE_1080FP24HZ,
		.width             = 1920,
		.height            = 1080 + 1125,
		.field_height      = 1080 + 1125,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 24,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_720FP60HZ */
		.name              = "720fp60hz",
		.mode              = VMODE_720FP60HZ,
		.width             = 1280,
		.height            = 720 + 750,
		.field_height      = 720 + 750,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_720FP50HZ */
		.name              = "720fp50hz",
		.mode              = VMODE_720FP50HZ,
		.width             = 1280,
		.height            = 720 + 750,
		.field_height      = 720 + 750,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 148500000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_PAL_M */
		.name              = "pal_m",
		.mode              = VMODE_PAL_M,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_PAL_N */
		.name              = "pal_n",
		.mode              = VMODE_PAL_N,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_NTSC_M */
		.name              = "ntsc_m",
		.mode              = VMODE_NTSC_M,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_3440_1440_50HZ */
		.name              = "3440x1440p50hz",
		.mode              = VMODE_3440_1440_50HZ,
		.width             = 3440,
		.height            = 1440,
		.field_height      = 1440,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_3440_1440_60HZ */
		.name              = "3440x1440p60hz",
		.mode              = VMODE_3440_1440_60HZ,
		.width             = 3440,
		.height            = 1440,
		.field_height      = 1440,
		.aspect_ratio_num  = 64,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 594000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
/* VMODE for 3D Frame Packing END */
	{ /* NULL mode, used as temporary witch mode state */
		.name              = "null",
		.mode              = VMODE_NULL,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 1485000000,
		.viu_color_fmt     = TVIN_YUV444,
	},
};

const struct vinfo_s *get_tv_info(enum vmode_e mode)
{
	uint32_t i = 0;
	for (i = 0; i < ARRAY_SIZE(tv_info); i++) {
		if (mode == tv_info[i].mode)
			return &tv_info[i];
	}
	return NULL;
}

/* for hdmi (un)plug during fps automation */
int want_hdmi_mode(enum vmode_e mode)
{
	int ret = 0;
	if ((mode == VMODE_480I)
	    || (mode == VMODE_480I_RPT)
	    || (mode == VMODE_480P)
	    || (mode == VMODE_480P_RPT)
	    || (mode == VMODE_576I)
	    || (mode == VMODE_576I_RPT)
	    || (mode == VMODE_576P)
	    || (mode == VMODE_576P_RPT)
	    || (mode == VMODE_720P)
	    || (mode == VMODE_720P_50HZ)
	    || (mode == VMODE_1080I)
	    || (mode == VMODE_1080I_50HZ)
	    || (mode == VMODE_1080P)
	    || (mode == VMODE_1080P_50HZ)
	    || (mode == VMODE_1080P_30HZ)
	    || (mode == VMODE_1080P_25HZ)
	    || (mode == VMODE_1080P_24HZ)
	    || (mode == VMODE_4K2K_24HZ)
	    || (mode == VMODE_4K2K_25HZ)
	    || (mode == VMODE_4K2K_30HZ)
	    || (mode == VMODE_4K2K_SMPTE)
	    || (mode == VMODE_4K2K_SMPTE_25HZ)
	    || (mode == VMODE_4K2K_SMPTE_30HZ)
	    || (mode == VMODE_4K2K_SMPTE_50HZ)
	    || (mode == VMODE_4K2K_SMPTE_60HZ)
	    || (mode == VMODE_4K2K_SMPTE_50HZ_Y420)
	    || (mode == VMODE_4K2K_SMPTE_60HZ_Y420)
	    || (mode == VMODE_4K2K_FAKE_5G)
	    || (mode == VMODE_4K2K_5G)
	    || (mode == VMODE_4K2K_50HZ)
	    || (mode == VMODE_4K2K_50HZ_Y420)
	    || (mode == VMODE_4K2K_60HZ)
	    || (mode == VMODE_4K2K_60HZ_Y420)
	   )
		ret = 1;
	return ret;
}


/*
*               NEW ADDED
*/
//search
const struct vinfo_s * findMatchedMode(u32 width, u32 height, u32 refreshrate) {
	uint32_t i = 0;
	for (i = 0; i < ARRAY_SIZE(tv_info); i++) {
		if (tv_info[i].width == width && tv_info[i].height == height &&
			tv_info[i].field_height == height && tv_info[i].sync_duration_num == refreshrate) {
		return &(tv_info[i]);
		}
	}
	return NULL;
}

int read_vout_info(int idx, struct vinfo_base_s * info) {
	if (!info)
		return -ENOBUFS;

        const char * devpath = VOUT_DEV;
        if (idx == 2) {
            devpath = VOUT2_DEV;
        }

	int voutdev = open(devpath, O_RDONLY);
	if (voutdev < 0) {
		return -EBADFD;
	}

	if (ioctl(voutdev, VOUT_IOC_CMD_GET_VINFO, (unsigned long)info) != 0) {
		close(voutdev);
		return -EINVAL;
	}

	close(voutdev);
	return 0;
}

