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

#ifndef AM_VINFO_H_
#define AM_VINFO_H_
#include <sys/types.h>

typedef uint32_t u32;
typedef unsigned char	   u8;


/*
  * !!!ATTENTATION:
  * MOST COPY FROM KERNEL, DONT MODIFY.
  */

/*
  * from kernel/include/driver/vout/vinfo.h
  */
enum vmode_e {
	VMODE_480I  = 0,
	VMODE_480I_RPT,
	VMODE_480CVBS,
	VMODE_480P,
	VMODE_480P_RPT,
	VMODE_576I,
	VMODE_576I_RPT,
	VMODE_576CVBS,
	VMODE_576P,
	VMODE_576P_RPT,
	VMODE_720P,
	VMODE_1024x768P,
	VMODE_1440x900P,
	VMODE_640x480P,
	VMODE_1280x1024P,
	VMODE_800x600P,
	VMODE_1680x1050P,
	VMODE_1024x600P,
	VMODE_2560x1600P,
	VMODE_2560x1440P,
	VMODE_2560x1080P,
	VMODE_1920x1200P,
	VMODE_1600x1200P,
	VMODE_1600x900P,
	VMODE_1360x768P,
	VMODE_1280x800P,
	VMODE_480x320P,	
	VMODE_800x480P,
	VMODE_1280x480P,
	VMODE_720P_50HZ,
	VMODE_768P,
	VMODE_768P_50HZ,
	VMODE_1080I,
	VMODE_1080I_50HZ,
	VMODE_1080P,
	VMODE_1080P_30HZ,
	VMODE_1080P_50HZ,
	VMODE_1080P_25HZ,
	VMODE_1080P_24HZ,
	VMODE_4K2K_30HZ,
	VMODE_4K2K_25HZ,
	VMODE_4K2K_24HZ,
	VMODE_4K2K_SMPTE,
	VMODE_4K2K_SMPTE_25HZ,
	VMODE_4K2K_SMPTE_30HZ,
	VMODE_4K2K_SMPTE_50HZ,
	VMODE_4K2K_SMPTE_50HZ_Y420,
	VMODE_4K2K_SMPTE_60HZ,
	VMODE_4K2K_SMPTE_60HZ_Y420,
	VMODE_4K2K_FAKE_5G,
	VMODE_4K2K_60HZ,
	VMODE_4K2K_60HZ_Y420,
	VMODE_4K2K_50HZ,
	VMODE_4K2K_50HZ_Y420,
	VMODE_4K2K_5G,
	VMODE_4K1K_120HZ,
	VMODE_4K1K_120HZ_Y420,
	VMODE_4K1K_100HZ,
	VMODE_4K1K_100HZ_Y420,
	VMODE_4K05K_240HZ,
	VMODE_4K05K_240HZ_Y420,
	VMODE_4K05K_200HZ,
	VMODE_4K05K_200HZ_Y420,
	VMODE_VGA,
	VMODE_SVGA,
	VMODE_XGA,
	VMODE_SXGA,
	VMODE_WSXGA,
	VMODE_FHDVGA,
	VMODE_720FP50HZ, /* Extra VMODE for 3D Frame Packing */
	VMODE_720FP60HZ,
	VMODE_1080FP24HZ,
	VMODE_1080FP25HZ,
	VMODE_1080FP30HZ,
	VMODE_1080FP50HZ,
	VMODE_1080FP60HZ,
	VMODE_LCD,
	VMODE_PAL_M,
	VMODE_PAL_N,
	VMODE_NTSC_M,
	VMODE_3440_1440_50HZ,
	VMODE_3440_1440_60HZ,
	VMODE_NULL, /* null mode is used as temporary witch mode state */
	VMODE_MAX,
	VMODE_INIT_NULL,
	VMODE_MASK = 0xFF,
};

enum tvmode_e {
	TVMODE_480I = 0,
	TVMODE_480I_RPT,
	TVMODE_480CVBS,
	TVMODE_480P,
	TVMODE_480P_RPT,
	TVMODE_576I,
	TVMODE_576I_RPT,
	TVMODE_576CVBS,
	TVMODE_576P,
	TVMODE_576P_RPT,
	TVMODE_720P,
	TVMODE_1024x768P,
	TVMODE_1440x900P,
	TVMODE_640x480P,
	TVMODE_1280x1024P,
	TVMODE_800x600P,
	TVMODE_1680x1050P,
	TVMODE_1024x600P,
	TVMODE_2560x1600P,
	TVMODE_2560x1440P,
	TVMODE_2560x1080P,
	TVMODE_1920x1200P,
	TVMODE_1600x1200P,
	TVMODE_1600x900P,
	TVMODE_1360x768P,
	TVMODE_1280x800P,
	TVMODE_480x320P,
	TVMODE_800x480P,
	TVMODE_1280x480P,
	TVMODE_720P_50HZ,
	TVMODE_768P,
	TVMODE_768P_50HZ,
	TVMODE_1080I,
	TVMODE_1080I_50HZ,
	TVMODE_1080P,
	TVMODE_1080P_30HZ,
	TVMODE_1080P_50HZ,
	TVMODE_1080P_25HZ,
	TVMODE_1080P_24HZ,
	TVMODE_4K2K_30HZ,
	TVMODE_4K2K_25HZ,
	TVMODE_4K2K_24HZ,
	TVMODE_4K2K_SMPTE,
	TVMODE_4K2K_SMPTE_25HZ,
	TVMODE_4K2K_SMPTE_30HZ,
	TVMODE_4K2K_SMPTE_50HZ,
	TVMODE_4K2K_SMPTE_50HZ_Y420,
	TVMODE_4K2K_SMPTE_60HZ,
	TVMODE_4K2K_SMPTE_60HZ_Y420,
	TVMODE_4K2K_FAKE_5G,
	TVMODE_4K2K_60HZ,
	TVMODE_4K2K_60HZ_Y420,
	TVMODE_4K2K_50HZ,
	TVMODE_4K2K_50HZ_Y420,
	TVMODE_4K2K_5G,
	TVMODE_4K1K_120HZ,
	TVMODE_4K1K_120HZ_Y420,
	TVMODE_4K1K_100HZ,
	TVMODE_4K1K_100HZ_Y420,
	TVMODE_4K05K_240HZ,
	TVMODE_4K05K_240HZ_Y420,
	TVMODE_4K05K_200HZ,
	TVMODE_4K05K_200HZ_Y420,
	TVMODE_VGA ,
	TVMODE_SVGA,
	TVMODE_XGA,
	TVMODE_SXGA,
	TVMODE_WSXGA,
	TVMODE_FHDVGA,
	TVMODE_720FP50HZ, /* Extra TVMODE for 3D Frame Packing */
	TVMODE_720FP60HZ,
	TVMODE_1080FP24HZ,
	TVMODE_1080FP25HZ,
	TVMODE_1080FP30HZ,
	TVMODE_1080FP50HZ,
	TVMODE_1080FP60HZ,
	TVMODE_NULL, /* null mode is used as temporary witch mode state */
	TVMODE_MAX,
};

enum tvin_color_fmt_e {
	TVIN_RGB444 = 0,
	TVIN_YUV422,		/* 1 */
	TVIN_YUV444,		/* 2 */
	TVIN_YUYV422,		/* 3 */
	TVIN_YVYU422,		/* 4 */
	TVIN_UYVY422,		/* 5 */
	TVIN_VYUY422,		/* 6 */
	TVIN_NV12,		/* 7 */
	TVIN_NV21,		/* 8 */
	TVIN_BGGR,		/* 9  raw data */
	TVIN_RGGB,		/* 10 raw data */
	TVIN_GBRG,		/* 11 raw data */
	TVIN_GRBG,		/* 12 raw data */
	TVIN_COLOR_FMT_MAX,
};

enum tvin_color_fmt_range_e {
	TVIN_FMT_RANGE_NULL = 0,  /* depend on vedio fromat */
	TVIN_RGB_FULL,  /* 1 */
	TVIN_RGB_LIMIT, /* 2 */
	TVIN_YUV_FULL,  /* 3 */
	TVIN_YUV_LIMIT, /* 4 */
	TVIN_COLOR_FMT_RANGE_MAX,
};

typedef uint32_t u32;

struct hdr_info {
    u32 hdr_support; /* RX EDID hdr support types */
    /*bit7:BT2020RGB    bit6:BT2020YCC bit5:BT2020cYCC bit4:adobeRGB*/
    /*bit3:adobeYCC601 bit2:sYCC601     bit1:xvYCC709    bit0:xvYCC601*/
    u8 colorimetry_support; /* RX EDID colorimetry support types */
    u32 lumi_max; /* RX EDID Lumi Max value */
    u32 lumi_avg; /* RX EDID Lumi Avg value */
    u32 lumi_min; /* RX EDID Lumi Min value */
};

struct vinfo_base_s {
	enum vmode_e mode;
	u32 width;
	u32 height;
	u32 field_height;
	u32 aspect_ratio_num;
	u32 aspect_ratio_den;
	u32 sync_duration_num;
	u32 sync_duration_den;
	u32 screen_real_width;
	u32 screen_real_height;
	u32 video_clk;
	enum tvin_color_fmt_e viu_color_fmt;
	struct hdr_info hdr_info;
};

struct vinfo_s {
	const char *name;
	enum vmode_e mode;
	u32 width;
	u32 height;
	u32 field_height;
	u32 aspect_ratio_num;
	u32 aspect_ratio_den;
	u32 sync_duration_num;
	u32 sync_duration_den;
	u32 screen_real_width;
	u32 screen_real_height;
	u32 video_clk;
	enum tvin_color_fmt_e viu_color_fmt;

	struct hdr_info hdr_info;
//	struct master_display_info_s
//		master_display_info;
//	const struct dv_info *dv_info;
	/* update hdmitx hdr packet, if data is NULL, disalbe packet */
//	void (*fresh_tx_hdr_pkt)(struct master_display_info_s *data);
	/* tunnel_mode: 1: tunneling mode, RGB 8bit  0: YCbCr422 12bit mode */
//	void (*fresh_tx_vsif_pkt)(enum eotf_type type, uint8_t tunnel_mode);
};


enum vmode_e vmode_name_to_mode(const char *str);
const struct vinfo_s *get_tv_info(enum vmode_e mode);
int want_hdmi_mode(enum vmode_e mode);
const struct vinfo_s * findMatchedMode(u32 width, u32 height, u32 refreshrate);
int read_vout_info(int idx, struct vinfo_base_s * info);


#endif //AML_VOUT_H_
