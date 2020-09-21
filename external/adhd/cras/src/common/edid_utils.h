// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __EDID_UTILS_H__
#define __EDID_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* These match the EDID encoding for Standard Timing block */
#define ASPECT_16_10 0
#define ASPECT_4_3   1
#define ASPECT_5_4   2
#define ASPECT_16_9  3
#define N_ASPECTS    4

/* Defines based on EDID and CEA-861D descriptions */
#define EDID_HDR         0
#define EDID_MFG_EID     0x8
#define EDID_MFG_PROD_LO 0x0A
#define EDID_MFG_PROD_HI 0x0B
#define EDID_MFG_SERIAL  0x0C
#define EDID_MFG_WEEK    0x10
#define EDID_MFG_YEAR    0x11
#define EDID_VERSION     0x12
#define EDID_REVISION    0x13
#define EDID_VIDEO_IN    0x14
#define EDID_MAX_HSIZE   0x15
#define EDID_MAX_VSIZE   0x16
#define EDID_GAMMA       0x17
#define EDID_FEATURES    0x18

#define EDID_ESTTIME1    0x23
#define EDID_ESTTIME2    0x24
#define EDID_MFGTIME     0x25
/* Next two repeat 8 times for standard timings 1-8 */
#define EDID_STDTIMEH    0x26
#define EDID_STDTIMEV    0x27
#define EDID_N_STDTIME   8

/* There are 4 DTD blocks in the EDID */
#define EDID_DTD_BASE 0x36
#define EDID_N_DTDS 4

#define EDID_EXT_FLAG 0x7E
#define EDID_CSUM 0x7F
#define EDID_SIZE 0x80

#define EEXT_TAG 0
#define EEXT_REV 1
#define EEXT_SIZE 0x80

#define EEDID_SIZE (EDID_SIZE + EEXT_SIZE)

/* 2 byte standard timing structure */
#define STDTIME_HBASE 248
#define STDTIME_HMULT 8
#define STDTIME_VASPECT_SHIFT 6
#define STDTIME_VREFMINUS60_MASK 0x3f
#define STDTIME_SIZE 2

/* 18 byte DTD structure */
#define DTD_PCLK_LO 0
#define DTD_PCLK_HI 1
#define DTD_HA_LO 2
#define DTD_HBL_LO 3
#define DTD_HABL_HI 4
#define DTD_VA_LO 5
#define DTD_VBL_LO 6
#define DTD_VABL_HI 7
#define DTD_HSO_LO 8
#define DTD_HSW_LO 9
#define DTD_VSX_LO 10
#define DTD_HVSX_HI 11
#define DTD_HSIZE_LO 12
#define DTD_VSIZE_LO 13
#define DTD_HVSIZE_HI 14
#define DTD_HBORDER 15
#define DTD_VBORDER 16
#define DTD_FLAGS 17
#define DTD_SIZE 18

/* These apply when PCLK is zero */
#define DTD_TYPETAG 3
#define DTD_STRING 5
#define DTD_MINV_HZ 5
#define DTD_MAXV_HZ 6
#define DTD_MINH_kHZ 7
#define DTD_MAXH_kHZ 8
#define DTD_MAXCLK_100kHZ 9

/* Types in the TYPETAG field */
#define DTDTYPE_MANUF 0x0f
#define DTDTYPE_STDTIME 0xfa
#define DTDTYPE_COLPOINT 0xfb
#define DTDTYPE_NAME 0xfc
#define DTDTYPE_LIMITS 0xfd
#define DTDTYPE_STRING 0xfe
#define DTDTYPE_SERIAL 0xff

/* This is the CEA extension version 3 */
#define CEA_TAG 0
#define CEA_REV 1
#define CEA_DTD_OFFSET 2
/* Next two are low nibble, high nibble of same byte */
#define CEA_NATIVE_DTDS 3
#define CEA_SUPPORT 3
#define CEA_DBC_START 4
/* Last DBC is at [CEA_DTD_OFFSET]-1, first DTD is at [CEA_DTD_OFFSET] */
/* Padding needs min of two (gives PCLK=00 in DTD) */
#define CEA_LAST_PAD 125
#define CEA_END_PAD 126
#define CEA_CHECKSUM 127

/* Data Block Collections */
/* Same byte: upper 3 bits tag, low five length */
#define DBC_TAG_LENGTH 0
#define DBC_LEN_MASK 0x1f
#define DBC_TAG_SHIFT 5
#define DBC_ETAG 1

#define DBCA_FORMAT 0
#define DBCA_RATE 1
#define DBCA_INFO 2
#define DBCA_SIZE 3

#define DBCA_FMT_LPCM 1

#define DBCV_CODE 0
#define DBCV_SIZE 1

#define DBCVND_IEEE_LO 0
#define DBCVND_IEEE_MID 1
#define DBCVND_IEEE_HI 2

#define DBCVHDMI_CEC_LO 3
#define DBCVHDMI_CEC_HI 4
#define DBCVHDMI_SUPPORT 5
#define DBCVHDMI_MAXTMDS_5MHz 6
#define DBCVHDMI_LATFLAGS 7
#define DBCVHDMI_VLAT 8
#define DBCVHDMI_ALAT 9
#define DBCVHDMI_IVLAT 10
#define DBCVHDMI_IALAT 11


#define DBCSP_ALLOC 0
#define DBCSP_SIZE 3

#define DBC_TAG_AUDIO 1
#define DBC_TAG_VIDEO 2
#define DBC_TAG_VENDOR 3
#define DBC_TAG_SPEAKER 4
#define DBC_TAG_VESA 5
#define DBC_TAG_EXTENDED 7

#define DBC_ETAG_VCDB 0
#define DBC_ETAG_VENDOR_VDB 1
#define DBC_ETAG_COL 5

#define VCDB_TAG 0
#define VCDB_ETAG 1
#define VCDB_FLAGS 2
#define VCDB_S_PT(x) (((x) & 0x30) >> 4)
#define VCDB_S_IT(x) (((x) & 0x0C) >> 2)
#define VCDB_S_CE(x) (((x) & 0x03))

#define COL_TAG 0
#define COL_ETAG 1
#define COL_FLAGS 2
#define COL_META 3

/* Number of test EDID arrays available to get/show_test_edid */
#define N_TEST_EDIDS 6

int edid_valid(const unsigned char *edid_data);
int edid_has_hdmi_info(const unsigned char *edid_data, int ext);
int edid_lpcm_support(const unsigned char *edid_data, int ext);
void show_edid_data(FILE *outfile, unsigned char *edid_data,
		    int items, int base);
void show_edid(FILE *outfile, unsigned char *edid_data, int ext);
int find_aspect(int h, int v);
int find_aspect_fromisize(unsigned char *edid_data);
extern char *aspect_to_str[];
int get_test_edid(int n, unsigned char *dst);
int show_test_edid(FILE *outfile, int n);

/* Gets monitor name from EDID.
 * Args:
 *    edid_data - EDID data.
 *    buf - buffer to store monitor name.
 *    buf_size - buffer size.
 */
int edid_get_monitor_name(const unsigned char *edid_data,
			  char *buf,
			  unsigned int buf_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
