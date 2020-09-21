// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "edid_utils.h"

/* Dump out an EDID block in a simple format */
void show_edid_data(FILE *outfile, unsigned char *edid_data,
		    int items, int base)
{
	int item = 0;

	while (item < items) {
		int i;
		fprintf(outfile, " 0x%04x:  ", item + base);
		for (i = 0; i < 16; i++) {
			fprintf(outfile, "%02x ", edid_data[item++]);
			if (item >= items)
				break;
		}
		fprintf(outfile, "\n");
	}
}


unsigned char test_edid1[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x06, 0xaf, 0x5c, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x12, 0x01, 0x03, 0x80, 0x1a, 0x0e, 0x78,
	0x0a, 0x99, 0x85, 0x95, 0x55, 0x56, 0x92, 0x28,
	0x22, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x96, 0x19,
	0x56, 0x28, 0x50, 0x00, 0x08, 0x30, 0x18, 0x10,
	0x24, 0x00, 0x00, 0x90, 0x10, 0x00, 0x00, 0x18,
	0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x41,
	0x55, 0x4f, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfe,
	0x00, 0x42, 0x31, 0x31, 0x36, 0x58, 0x57, 0x30,
	0x32, 0x20, 0x56, 0x30, 0x20, 0x0a, 0x00, 0xf8
};

unsigned char test_edid2[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x30, 0xe4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x01, 0x03, 0x80, 0x1a, 0x0e, 0x78,
	0x0a, 0xbf, 0x45, 0x95, 0x58, 0x52, 0x8a, 0x28,
	0x25, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x84, 0x1c,
	0x56, 0xa8, 0x50, 0x00, 0x19, 0x30, 0x30, 0x20,
	0x35, 0x00, 0x00, 0x90, 0x10, 0x00, 0x00, 0x1b,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x4c,
	0x47, 0x20, 0x44, 0x69, 0x73, 0x70, 0x6c, 0x61,
	0x79, 0x0a, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
	0x00, 0x4c, 0x50, 0x31, 0x31, 0x36, 0x57, 0x48,
	0x31, 0x2d, 0x54, 0x4c, 0x4e, 0x31, 0x00, 0x4e
};

unsigned char test_edid3[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x4d, 0xd9, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x11, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
	0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27,
	0x12, 0x48, 0x4c, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d,
	0x80, 0xd0, 0x72, 0x1c, 0x16, 0x20, 0x10, 0x2c,
	0x25, 0x80, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e,
	0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20,
	0x58, 0x2c, 0x25, 0x00, 0xc4, 0x8e, 0x21, 0x00,
	0x00, 0x9e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x48,
	0x44, 0x4d, 0x49, 0x20, 0x4c, 0x4c, 0x43, 0x0a,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfd,
	0x00, 0x3b, 0x3d, 0x0f, 0x2d, 0x08, 0x00, 0x0a,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xc0,
	0x02, 0x03, 0x1e, 0x47, 0x4f, 0x94, 0x13, 0x05,
	0x03, 0x04, 0x02, 0x01, 0x16, 0x15, 0x07, 0x06,
	0x11, 0x10, 0x12, 0x1f, 0x23, 0x09, 0x07, 0x01,
	0x65, 0x03, 0x0c, 0x00, 0x10, 0x00, 0x8c, 0x0a,
	0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40,
	0x55, 0x00, 0x13, 0x8e, 0x21, 0x00, 0x00, 0x18,
	0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20,
	0xb8, 0x28, 0x55, 0x40, 0xc4, 0x8e, 0x21, 0x00,
	0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0,
	0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xc4, 0x8e,
	0x21, 0x00, 0x00, 0x18, 0x01, 0x1d, 0x00, 0x72,
	0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00,
	0xc4, 0x8e, 0x21, 0x00, 0x00, 0x1e, 0x8c, 0x0a,
	0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
	0x96, 0x00, 0x13, 0x8e, 0x21, 0x00, 0x00, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb
};

unsigned char test_edid4[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x4c, 0x2d, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x31, 0x0f, 0x01, 0x03, 0x80, 0x10, 0x09, 0x8c,
	0x0a, 0xe2, 0xbd, 0xa1, 0x5b, 0x4a, 0x98, 0x24,
	0x15, 0x47, 0x4a, 0x20, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d,
	0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28,
	0x55, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e,
	0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20,
	0x58, 0x2c, 0x25, 0x00, 0xa0, 0x5a, 0x00, 0x00,
	0x00, 0x9e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x3b,
	0x3d, 0x1e, 0x2e, 0x08, 0x00, 0x0a, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
	0x00, 0x53, 0x41, 0x4d, 0x53, 0x55, 0x4e, 0x47,
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x8d,
	0x02, 0x03, 0x16, 0x71, 0x43, 0x84, 0x05, 0x03,
	0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00,
	0x65, 0x03, 0x0c, 0x00, 0x20, 0x00, 0x8c, 0x0a,
	0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
	0x96, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30
};

unsigned char test_edid5[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x3d, 0xcb, 0x61, 0x07, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x11, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
	0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27,
	0x12, 0x48, 0x4c, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d,
	0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c,
	0x25, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e,
	0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c, 0x16, 0x20,
	0x10, 0x2c, 0x25, 0x80, 0xc4, 0x8e, 0x21, 0x00,
	0x00, 0x9e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x54,
	0x58, 0x2d, 0x53, 0x52, 0x36, 0x30, 0x35, 0x0a,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfd,
	0x00, 0x17, 0xf0, 0x0f, 0x7e, 0x11, 0x00, 0x0a,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x93,
	0x02, 0x03, 0x3b, 0x72, 0x55, 0x85, 0x04, 0x03,
	0x02, 0x0e, 0x0f, 0x07, 0x23, 0x24, 0x10, 0x94,
	0x13, 0x12, 0x11, 0x1d, 0x1e, 0x16, 0x25, 0x26,
	0x01, 0x1f, 0x35, 0x09, 0x7f, 0x07, 0x0f, 0x7f,
	0x07, 0x17, 0x07, 0x50, 0x3f, 0x06, 0xc0, 0x57,
	0x06, 0x00, 0x5f, 0x7e, 0x01, 0x67, 0x5e, 0x00,
	0x83, 0x4f, 0x00, 0x00, 0x66, 0x03, 0x0c, 0x00,
	0x20, 0x00, 0x80, 0x8c, 0x0a, 0xd0, 0x8a, 0x20,
	0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xc4,
	0x8e, 0x21, 0x00, 0x00, 0x18, 0x8c, 0x0a, 0xd0,
	0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40, 0x55,
	0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01,
	0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e,
	0x28, 0x55, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdd
};

/* Has DTD that is too wide */
unsigned char test_edid6[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x10, 0xac, 0x63, 0x40, 0x4c, 0x35, 0x31, 0x33,
	0x0c, 0x15, 0x01, 0x03, 0x80, 0x40, 0x28, 0x78,
	0xea, 0x8d, 0x85, 0xad, 0x4f, 0x35, 0xb1, 0x25,
	0x0e, 0x50, 0x54, 0xa5, 0x4b, 0x00, 0x71, 0x4f,
	0x81, 0x00, 0x81, 0x80, 0xa9, 0x40, 0xd1, 0x00,
	0xd1, 0x40, 0x01, 0x01, 0x01, 0x01, 0xe2, 0x68,
	0x00, 0xa0, 0xa0, 0x40, 0x2e, 0x60, 0x30, 0x20,
	0x36, 0x00, 0x81, 0x91, 0x21, 0x00, 0x00, 0x1a,
	0x00, 0x00, 0x00, 0xff, 0x00, 0x50, 0x48, 0x35,
	0x4e, 0x59, 0x31, 0x33, 0x4d, 0x33, 0x31, 0x35,
	0x4c, 0x0a, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x44,
	0x45, 0x4c, 0x4c, 0x20, 0x55, 0x33, 0x30, 0x31,
	0x31, 0x0a, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfd,
	0x00, 0x31, 0x56, 0x1d, 0x71, 0x1c, 0x00, 0x0a,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0xb0
};

static unsigned char *test_edids[N_TEST_EDIDS] = {
	test_edid1, test_edid2, test_edid3, test_edid4, test_edid5,
	test_edid6
};

int get_test_edid(int n, unsigned char *dst)
{
	if ((n < 1) || (n > N_TEST_EDIDS))
		return -1;
	memcpy(dst, test_edids[n-1], 256);
	return 0;
}

int show_test_edid(FILE *outfile, int n)
{
	if ((n < 1) || (n > N_TEST_EDIDS))
		return -1;
	fprintf(outfile, "Test EDID %d\n", n);
	show_edid(outfile, test_edids[n-1], 1);
	return 0;
}

static void get_dtd_string(const char *str, char *buf, int buf_size)
{
	int stp;
	int len = buf_size < 14 ? buf_size : 14;

	strncpy(buf, str, len - 1);
	for (stp = 0; stp < len - 1; stp++)
		if (buf[stp] == 0x0a)
			buf[stp] = 0;
	buf[stp] = 0;
}

/* Print an edid descriptor block (standard case is at 54 + 18 * i) */
void show_edid_dtd(FILE *outfile, unsigned char *base)
{
	int pelclk = base[DTD_PCLK_LO] + (base[DTD_PCLK_HI]<<8);
	char monstr[DTD_SIZE];

	if (pelclk != 0) {
		int hres = base[DTD_HA_LO] + ((base[DTD_HABL_HI] & 0xf0)<<4);
		int hbl = base[DTD_HBL_LO] + ((base[DTD_HABL_HI] & 0x0f)<<8);
		int vres = base[DTD_VA_LO] + ((base[DTD_VABL_HI] & 0xf0)<<4);
		int vbl = base[DTD_VBL_LO] + ((base[DTD_VABL_HI] & 0x0f)<<8);
		int hso = base[DTD_HSO_LO] + ((base[DTD_HVSX_HI] & 0xc0)<<2);
		int hsw = base[DTD_HSW_LO] + ((base[DTD_HVSX_HI] & 0x30)<<4);
		int vso = (base[DTD_VSX_LO] >> 4) +
			   ((base[DTD_HVSX_HI] & 0x0c) << 2);
		int vsw = (base[DTD_VSX_LO] & 0xf) +
			   ((base[DTD_HVSX_HI] & 0x03) << 4);
		int hsiz = base[DTD_HSIZE_LO] +
			   ((base[DTD_HVSIZE_HI] & 0xf0) << 4);
		int vsiz = base[DTD_VSIZE_LO] +
			   ((base[DTD_HVSIZE_HI] & 0x0f) << 8);
		int hbdr = base[DTD_HBORDER];
		int vbdr = base[DTD_VBORDER];
		int mdflg = base[DTD_FLAGS];

		int refr = (pelclk * 10000)/((hres+hbl)*(vres+vbl));
		int refm = (pelclk * 10000)%((hres+hbl)*(vres+vbl));
		int refd = (refm*100)/((hres+hbl)*(vres+vbl));

		fprintf(outfile,
			"%dx%d%c@%d.%02d, dot clock %d  %cHsync %cVsync\n",
			hres, vres, (mdflg & 0x80) ? 'i' : 'p',
			refr, refd,
			pelclk * 10000,
			(mdflg & 0x2) ? '+' : '-',
			(mdflg & 0x4) ? '+' : '-');
		fprintf(outfile, "H: start %d, end %d, total %d\n",
			hres+hso, hres+hso+hsw, hres+hbl);
		fprintf(outfile, "V: start %d, end %d, total %d\n",
			vres+vso, vres+vso+vsw, vres+vbl);
		fprintf(outfile, "Size %dx%dmm, Border %dx%d pixels\n",
			hsiz, vsiz, hbdr, vbdr);
		return;
	}

	switch (base[DTD_TYPETAG]) {
	case DTDTYPE_SERIAL:
	case DTDTYPE_STRING:
	case DTDTYPE_NAME:
		get_dtd_string((const char *)base + DTD_STRING,
			       monstr, DTD_SIZE);

		if (base[3] != DTDTYPE_STRING)
			fprintf(outfile, "%s: %s\n",
				(base[3] == DTDTYPE_NAME) ?
				"Name" : "Serial",
				monstr);
		else
			fprintf(outfile, "%s\n", monstr);
		break;

	case DTDTYPE_LIMITS:
		fprintf(outfile,
			"V %d - %d Hz, H %d - %d kHz, Pel <= %d MHz\n",
			base[DTD_MINV_HZ], base[DTD_MAXV_HZ],
			base[DTD_MINH_kHZ], base[DTD_MAXH_kHZ],
			base[DTD_MAXCLK_100kHZ]*10);
		break;

	default:
		fprintf(outfile,
			"Undecoded descriptor block type 0x%x\n",
			base[DTD_TYPETAG]);
		break;
	}
}


char *sad_audio_type[16] = {
	"Reserved", "LPCM", "AC-3", "MPEG1 (Layer 1 and 2)",
	"MP3", "MPEG2", "AAC", "DTS",
	"ATRAC", "SACD", "DD+", "DTS-HD",
	"MLP/Dolby TrueHD", "DST Audio", "WMA Pro", "Reserved",
};

char *uscanstr[4] = {
	"not supported",
	"always overscan",
	"always underscan",
	"supports both over- and underscan",
};

static inline void show_audio_dbc(FILE *outfile,
				  const unsigned char *edid_ext,
				  int dbc)
{
	int dbp = dbc + 1;
	int db_len = edid_ext[dbc+DBC_TAG_LENGTH] & DBC_LEN_MASK;

	while (dbp < (dbc + db_len + 1)) {
		int atype =
			(edid_ext[dbp + DBCA_FORMAT]>>3) & 0xf;
		unsigned char dbca_rate = edid_ext[dbp + DBCA_RATE];

		fprintf(outfile, "Audio: %d channels %s: ",
			(edid_ext[dbp + DBCA_FORMAT] & 0x7) + 1,
			sad_audio_type[atype]);

		if (dbca_rate & 0x40)
			fprintf(outfile, "192k ");
		if (dbca_rate & 0x20)
			fprintf(outfile, "176k ");
		if (dbca_rate & 0x10)
			fprintf(outfile, "96k ");
		if (dbca_rate & 0x08)
			fprintf(outfile, "88k ");
		if (dbca_rate & 0x04)
			fprintf(outfile, "48k ");
		if (dbca_rate & 0x02)
			fprintf(outfile, "44k ");
		if (dbca_rate & 0x01)
			fprintf(outfile, "32k ");

		if (atype == 1) {
			unsigned char dbca_info = edid_ext[dbp + DBCA_INFO];
			fprintf(outfile, "%s%s%s\n",
				(dbca_info & 0x4) ? "24-bit " : "",
				(dbca_info & 0x2) ? "20-bit " : "",
				(dbca_info & 0x1) ? "16-bit" : "");
		} else if ((atype >= 2) && (atype <= 8)) {
			fprintf(outfile, "Max %dkHz\n",
				edid_ext[dbp + DBCA_INFO] * 8);
		} else {
			fprintf(outfile, "Codec vendor flags 0x%02x\n",
				edid_ext[dbp + DBCA_INFO]);
		}

		dbp += DBCA_SIZE;
	}
}

static inline void show_vendor_dbc(FILE *outfile,
				   const unsigned char *edid_ext,
				   int dbp)
{
	if ((edid_ext[dbp + DBCVND_IEEE_LO] != 0x03) ||
	    (edid_ext[dbp + DBCVND_IEEE_MID] != 0x0C) ||
	    (edid_ext[dbp + DBCVND_IEEE_HI] != 0x00)) {
		fprintf(outfile, "Vendor block for %02x-%02x-%02x",
			edid_ext[dbp + DBCVND_IEEE_LO],
			edid_ext[dbp + DBCVND_IEEE_MID],
			edid_ext[dbp + DBCVND_IEEE_HI]);
		return;
	}

	fprintf(outfile,
		"HDMI Vendor block (CEC @0x%04x):\n"
		"Support: %s%s%s%s%s%s\n",
		edid_ext[dbp + DBCVHDMI_CEC_LO] +
		(edid_ext[dbp + DBCVHDMI_CEC_HI] << 8),
		(edid_ext[dbp + DBCVHDMI_SUPPORT] & 0x80) ? "AI " : "",
		(edid_ext[dbp + DBCVHDMI_SUPPORT] & 0x40) ? "DC_48bit " : "",
		(edid_ext[dbp + DBCVHDMI_SUPPORT] & 0x20) ? "DC_36bit " : "",
		(edid_ext[dbp + DBCVHDMI_SUPPORT] & 0x10) ? "DC_30bit " : "",
		(edid_ext[dbp + DBCVHDMI_SUPPORT] & 0x08) ? "DC_Y444 " : "",
		(edid_ext[dbp + DBCVHDMI_SUPPORT] & 0x01) ? "DVI_Dual" : "");

	if (edid_ext[dbp + DBCVHDMI_MAXTMDS_5MHz] > 0)
		fprintf(outfile, "Max TMDS Frequency %dMHz\n",
			edid_ext[dbp + DBCVHDMI_MAXTMDS_5MHz]*5);

	if (edid_ext[dbp + DBCVHDMI_LATFLAGS] & 0x80)
		fprintf(outfile, "Video latency %dms, audio latency %dms\n",
			2 * (edid_ext[dbp + DBCVHDMI_VLAT] - 1),
			2 * (edid_ext[dbp + DBCVHDMI_ALAT] - 1));

	if (edid_ext[dbp + 7] & 0x40)
		fprintf(outfile,
			"Interlaced Video latency %dms, audio latency %dms\n",
			2 * (edid_ext[dbp + DBCVHDMI_IVLAT] - 1),
			2 * (edid_ext[dbp + DBCVHDMI_IALAT] - 1));
}

static void show_extended_dbc(FILE *outfile,
			      const unsigned char *edid_ext,
			      int dbc)
{
	int dbp = dbc + 1;
	int db_len = edid_ext[dbc + DBC_TAG_LENGTH] & DBC_LEN_MASK;

	switch (edid_ext[dbp + DBC_ETAG]) {
	case DBC_ETAG_VCDB:
	{
		unsigned char vcdb_flags;

		fprintf(outfile, "Video Capabilities:\n");
		fprintf(outfile,
			"  Quantization range selectable: %s\n",
			(edid_ext[dbp + VCDB_FLAGS] & 0x40) ?
				"unknown" : "via AVI Q");

		/* PT field zero implies no data, just use IT
		 * and CE fields
		 */
		vcdb_flags = edid_ext[dbp + VCDB_FLAGS];
		if (VCDB_S_PT(vcdb_flags))
			fprintf(outfile,
				"  Preferred mode %s\n",
				uscanstr[VCDB_S_PT(vcdb_flags)]);
		fprintf(outfile, "  IT modes %s\n",
			uscanstr[VCDB_S_IT(vcdb_flags)]);
		fprintf(outfile, "  CE modes %s\n",
			uscanstr[VCDB_S_CE(vcdb_flags)]);
		break;
	}

	case DBC_ETAG_COL:
		fprintf(outfile,
			"Colorimetry supports %s%s metadata 0x%x\n",
			(edid_ext[dbp + COL_FLAGS] & 0x02) ? "HD(YCC709) " : "",
			(edid_ext[dbp + COL_FLAGS] & 0x01) ? "SD(YCC601) " : "",
			(edid_ext[dbp + COL_META] & 0x07));
		break;

	default:
		fprintf(outfile,
			"Unknown extended tag data block 0x%x,  length 0x%x\n",
			edid_ext[dbc + DBC_ETAG], db_len);
	}
}

void show_cea_timing(FILE *outfile, unsigned char *edid_ext)
{
	int i, dbc;
	int off_dtd = edid_ext[CEA_DTD_OFFSET];
	int n_dtd;
	fprintf(outfile, "Found CEA EDID Timing Extension rev 3\n");

	if (off_dtd < CEA_DBC_START) {
		fprintf(outfile, "Block is empty (off_dtd = %d)\n", off_dtd);
		return;
	}
	/* Ends with 0 and a checksum, have at least one pad byte */
	n_dtd = (CEA_LAST_PAD - off_dtd)/DTD_SIZE;
	fprintf(outfile,
		"Block has DTDs starting at offset %d (%d bytes of DBCs)\n",
		off_dtd, off_dtd - CEA_DBC_START);
	fprintf(outfile, "There is space for %d DTDs in extension\n", n_dtd);
	fprintf(outfile,
		"There are %d native DTDs (between regular and extensions)\n",
		edid_ext[CEA_NATIVE_DTDS] & 0xf);
	fprintf(outfile, "IT formats %sdefault to underscan\n",
		(edid_ext[CEA_SUPPORT] & 0x80) ? "" : "do not ");
	fprintf(outfile,
		"Support: %sbasic audio, %sYCrCb 4:4:4, %sYCrCb 4:2:2\n",
		(edid_ext[CEA_SUPPORT] & 0x40) ? "" : "no ",
		(edid_ext[CEA_SUPPORT] & 0x20) ? "" : "no ",
		(edid_ext[CEA_SUPPORT] & 0x10) ? "" : "no ");

	/* Between offset 4 and off_dtd is the Data Block Collection */
	/* There may be none, in which case off_dtd == 4             */
	dbc = CEA_DBC_START;
	while (dbc < off_dtd) {
		int db_len = edid_ext[dbc + DBC_TAG_LENGTH] & DBC_LEN_MASK;
		int dbp = dbc + 1;

		switch (edid_ext[dbc+DBC_TAG_LENGTH] >> DBC_TAG_SHIFT) {
		case DBC_TAG_AUDIO:
			/* Audio Data Block */
			show_audio_dbc(outfile, edid_ext, dbc);
			break;

		case DBC_TAG_VIDEO:
			/* Vidio Data Block */
			while (dbp < (dbc + db_len + 1)) {
				int vtype = edid_ext[dbp + DBCV_CODE] & 0x7f;
				fprintf(outfile, "Video: Code %d %s\n", vtype,
					(edid_ext[dbp + DBCV_CODE] & 0x80) ?
						"(native)" : "");
				dbp += DBCV_SIZE;
			}
			break;

		case DBC_TAG_VENDOR:
			/* Vendor Data Block */
			show_vendor_dbc(outfile, edid_ext, dbc + 1);
			break;

		case DBC_TAG_SPEAKER:
		{
			/* Speaker allocation Block */
			unsigned char dbcsp_alloc = edid_ext[dbp + DBCSP_ALLOC];

			fprintf(outfile, "Speakers: %s%s%s%s%s%s%s\n",
				(dbcsp_alloc & 0x40) ? "RearCenter L/R " : "",
				(dbcsp_alloc & 0x20) ? "FrontCenter L/R " : "",
				(dbcsp_alloc & 0x10) ? "Rear Center" : "",
				(dbcsp_alloc & 0x08) ? "Rear L/R " : "",
				(dbcsp_alloc & 0x04) ? "Front Center " : "",
				(dbcsp_alloc & 0x02) ? "LFE " : "",
				(dbcsp_alloc & 0x01) ? "Front L/R " : "");
			break;
		}

		case DBC_TAG_EXTENDED:
			show_extended_dbc(outfile, edid_ext, dbc);
			break;

		default:
			fprintf(outfile,
				"Unknown Data Block type tag 0x%x, len 0x%x\n",
				edid_ext[dbc+DBC_TAG_LENGTH] >> DBC_TAG_SHIFT,
				db_len);
			break;
		}

		dbc += db_len + 1;
	}
	for (i = 0; i < n_dtd; i++) {
		/* Find 0,0 when we hit padding */
		if ((edid_ext[off_dtd + DTD_SIZE * i + DTD_PCLK_LO] == 0) &&
		    (edid_ext[off_dtd + DTD_SIZE * i + DTD_PCLK_HI] == 0)) {
			fprintf(outfile,
				"End of DTD padding after %d DTDs\n", i);
			break;
		}
		show_edid_dtd(outfile, edid_ext + (off_dtd + DTD_SIZE * i));
	}
}


int edid_valid(const unsigned char *edid_data)
{
	return ((edid_data[EDID_HDR + 0] == 0x00) &&
		(edid_data[EDID_HDR + 1] == 0xff) &&
		(edid_data[EDID_HDR + 2] == 0xff) &&
		(edid_data[EDID_HDR + 3] == 0xff) &&
		(edid_data[EDID_HDR + 4] == 0xff) &&
		(edid_data[EDID_HDR + 5] == 0xff) &&
		(edid_data[EDID_HDR + 6] == 0xff) &&
		(edid_data[EDID_HDR + 7] == 0x00));
}

int edid_lpcm_support(const unsigned char *edid_data, int ext)
{
	const unsigned char *edid_ext = edid_data + EDID_SIZE;
	int dbc;
	int off_dtd = edid_ext[CEA_DTD_OFFSET];

	/* No if no extension, which can happen for two reasons */
	/* a) ext < 1 indicates no data was read into the extension area */
	/* b) edid_data[126] < 1 indicates EDID does not use extension area */
	if ((ext < 1) || (edid_data[EDID_EXT_FLAG] < 1))
		return 0;

	/* No if extension is not CEA rev 3 */
	if (!((edid_ext[EEXT_TAG] == 0x02) && (edid_ext[EEXT_REV] == 0x03)))
		return 0;

	/* If DBC block is not empty look for audio info */
	if (off_dtd <= CEA_DBC_START)
		goto done_dtd;

	/* Between offset 4 and off_dtd is the Data Block Collection */
	/* There may be none, in which case off_dtd == 4             */
	dbc = CEA_DBC_START;
	while (dbc < off_dtd) {
		int db_len = edid_ext[dbc + DBC_TAG_LENGTH] & DBC_LEN_MASK;
		int dbp = dbc + 1;
		unsigned char dbc_type;

		/* Audio Data Block, type LPCM, return bitmap of frequencies */
		dbc_type = edid_ext[dbc + DBC_TAG_LENGTH] >> DBC_TAG_SHIFT;
		if ((dbc_type == DBC_TAG_AUDIO) &&
		    (((edid_ext[dbp + DBCA_FORMAT]>>3) & 0xF) == DBCA_FMT_LPCM))
			return edid_ext[dbp + DBCA_RATE];

		dbc += db_len + 1;
	}
	/* Get here if failed to find LPCM info in DBC block */

done_dtd:
	/* Last chance is to look for Basic Audio support. Return bitmap for 32,
	 * 44.1, 48 */
	if (edid_ext[CEA_SUPPORT] & 0x40)
		return 0x7;

	return 0;
}


int edid_has_hdmi_info(const unsigned char *edid_data, int ext)
{
	const unsigned char *edid_ext = edid_data + EDID_SIZE;
	int dbc;
	int off_dtd = edid_ext[CEA_DTD_OFFSET];

	/* No if no extension, which can happen for two reasons */
	/* a) ext < 1 indicates no data was read into the extension area */
	/* b) edid_data[126] < 1 indicates EDID does not use extension area */
	if ((ext < 1) || (edid_data[EDID_EXT_FLAG] < 1))
		return 0;

	/* No if extension is not CEA rev 3 */
	if (!((edid_ext[EEXT_TAG] == 0x02) && (edid_ext[EEXT_REV] == 0x03)))
		return 0;

	/* No if block is empty */
	if (off_dtd < CEA_DBC_START)
		return 0;

	/* Between offset 4 and off_dtd is the Data Block Collection */
	/* There may be none, in which case off_dtd == 4             */
	dbc = CEA_DBC_START;
	while (dbc < off_dtd) {
		int db_len = edid_ext[dbc + DBC_TAG_LENGTH] & DBC_LEN_MASK;
		int dbp = dbc + 1;
		unsigned char dbc_type;

		dbc_type = edid_ext[dbc + DBC_TAG_LENGTH] >> DBC_TAG_SHIFT;
		if (dbc_type == DBC_TAG_VENDOR) {
			/* Vendor Data Block */
			if ((edid_ext[dbp + DBCVND_IEEE_LO] == 0x03) &&
			    (edid_ext[dbp + DBCVND_IEEE_MID] == 0x0C) &&
			    (edid_ext[dbp + DBCVND_IEEE_HI] == 0x00))
				return 1;
		}
		dbc += db_len + 1;
	}
	return 0;
}

/* Print out an EDID */
void show_edid(FILE *outfile, unsigned char *edid_data, int ext)
{
	int i;
	int edidver = edid_data[EDID_VERSION];
	int edidrev = edid_data[EDID_REVISION];
	unsigned char *edid_ext;
	unsigned char edid_features;

	if (!edid_valid(edid_data)) {
		fprintf(outfile, "Block does not contain EDID header\n");
		return;
	}
	/* unsigned edid_data so the right shifts pull in zeros */
	fprintf(outfile, "Manufacturer ID %c%c%c, product ID 0x%x\n",
		'@' + (edid_data[EDID_MFG_EID]>>2),
		'@' + (((edid_data[EDID_MFG_EID] & 3)<<3) +
			(edid_data[EDID_MFG_EID+1]>>5)),
		'@' + (edid_data[EDID_MFG_EID+1] & 0x1f),
		edid_data[EDID_MFG_PROD_LO] + (edid_data[EDID_MFG_PROD_HI]<<8));
	fprintf(outfile,
		"Manufactured wk %d of %d. Edid version %d.%d\n",
		edid_data[EDID_MFG_WEEK], 1990+edid_data[EDID_MFG_YEAR],
		edidver, edidrev);
	fprintf(outfile,
		"Input: %s, vid level %d, %s, %s %s %s %s sync, %dx%dcm, Gamma %f\n",
		(edid_data[EDID_VIDEO_IN] & 0x80) ? "digital" : "analog",
		(edid_data[EDID_VIDEO_IN]>>5) & 3,
		(edid_data[EDID_VIDEO_IN] * 0x10) ? "Blank to black" : "",
		(edid_data[EDID_VIDEO_IN] * 0x08) ? "Separate" : "",
		(edid_data[EDID_VIDEO_IN] * 0x04) ? "Composite" : "",
		(edid_data[EDID_VIDEO_IN] * 0x02) ? "On-green" : "",
		(edid_data[EDID_VIDEO_IN] * 0x01) ? "Serration V" : "",
		edid_data[EDID_MAX_HSIZE], edid_data[EDID_MAX_VSIZE],
		1.0+((float)edid_data[EDID_GAMMA]/100.0));

	edid_features = edid_data[EDID_FEATURES];
	fprintf(outfile, "Features: %s %s %s %s %s %s %s\n",
		(edid_features & 0x80) ? "standby" : "",
		(edid_features & 0x40) ? "suspend" : "",
		(edid_features & 0x20) ? "active-off" : "",
		(edid_features & 0x18) ? "colour" : "monochrome",
		(edid_features & 0x04) ? "std-cspace" : "non-std-cspace",
		(edid_features & 0x02) ? "preferred-timing" : "",
		(edid_features & 0x01) ? "default-GTF" : "");

	fprintf(outfile, "Established Timing:\n");
	if (edid_data[EDID_ESTTIME1] & 0x80)
		fprintf(outfile, "720x400@70\n");
	if (edid_data[EDID_ESTTIME1] & 0x40)
		fprintf(outfile, "720x400@88\n");
	if (edid_data[EDID_ESTTIME1] & 0x20)
		fprintf(outfile, "640x480@60\n");
	if (edid_data[EDID_ESTTIME1] & 0x10)
		fprintf(outfile, "640x480@67\n");
	if (edid_data[EDID_ESTTIME1] & 0x08)
		fprintf(outfile, "640x480@72\n");
	if (edid_data[EDID_ESTTIME1] & 0x04)
		fprintf(outfile, "640x480@75\n");
	if (edid_data[EDID_ESTTIME1] & 0x02)
		fprintf(outfile, "800x600@56\n");
	if (edid_data[EDID_ESTTIME1] & 0x01)
		fprintf(outfile, "800x600@60\n");
	if (edid_data[EDID_ESTTIME2] & 0x80)
		fprintf(outfile, "800x600@72\n");
	if (edid_data[EDID_ESTTIME2] & 0x40)
		fprintf(outfile, "800x600@75\n");
	if (edid_data[EDID_ESTTIME2] & 0x20)
		fprintf(outfile, "832x624@75\n");
	if (edid_data[EDID_ESTTIME2] & 0x10)
		fprintf(outfile, "1024x768i@87\n");
	if (edid_data[EDID_ESTTIME2] & 0x08)
		fprintf(outfile, "1024x768@60\n");
	if (edid_data[EDID_ESTTIME2] & 0x04)
		fprintf(outfile, "1024x768@70\n");
	if (edid_data[EDID_ESTTIME2] & 0x02)
		fprintf(outfile, "1024x768@75\n");
	if (edid_data[EDID_ESTTIME2] & 0x01)
		fprintf(outfile, "1280x1024@75\n");
	if (edid_data[EDID_MFGTIME]  & 0x80)
		fprintf(outfile, "1152x870@75\n");

	fprintf(outfile, "Standard timing:\n");
	for (i = 0; i < EDID_N_STDTIME; i++) {
		int hinfo = edid_data[EDID_STDTIMEH + 2 * i];
		int vinfo = edid_data[EDID_STDTIMEV + 2 * i];
		int hres, vres;

		/* 01 01 is pad by spec, but 00 00 and 20 20 are see in wild */
		if (((hinfo == 0x01) && (vinfo == 0x01)) ||
		    ((hinfo == 0x00) && (vinfo == 0x00)) ||
		    ((hinfo == 0x20) && (vinfo == 0x20)))
			continue;
		hres = (hinfo * 8) + 248;
		switch (vinfo >> 6) {
		case ASPECT_16_10:
			vres = (hres * 10)/16;
			break;
		case ASPECT_4_3:
			vres = (hres * 3)/4;
			break;
		case ASPECT_5_4:
			vres = (hres * 4)/5;
			break;
		case ASPECT_16_9:
			vres = (hres * 9)/16;
			break;
			/* Default only hit if compiler broken */
		default:
			vres = 0;
		}
		fprintf(outfile, "%d: %dx%d@%d\n",
			i, hres, vres, 60 + (vinfo & 0x3f));
	}

	fprintf(outfile, "Descriptor blocks:\n");
	for (i = 0; i < EDID_N_DTDS; i++)
		show_edid_dtd(outfile,
			      edid_data + (EDID_DTD_BASE + i * DTD_SIZE));
	fprintf(outfile,
		"EDID contains %d extensions\n",
		edid_data[EDID_EXT_FLAG]);

	edid_ext = edid_data + EDID_SIZE;

	if ((ext >= 1) && (edid_data[EDID_EXT_FLAG] >= 1)) {
		unsigned char eext_tag = edid_ext[EEXT_TAG];
		if ((eext_tag == 0x02) && (edid_ext[EEXT_REV] == 0x03)) {
			show_cea_timing(outfile, edid_ext);
		} else {
			char *tagtype;
			switch (eext_tag) {
			case 0x01:
				tagtype = "LCD Timings";
				break;
			case 0x02:
				tagtype = "CEA";
				break;
			case 0x20:
				tagtype = "EDID 2.0";
				break;
			case 0x30:
				tagtype = "Color Information";
				break;
			case 0x40:
				tagtype = "DVI Feature";
				break;
			case 0x50:
				tagtype = "Touch Screen Map";
				break;
			case 0xF0:
				tagtype = "Block Map";
				break;
			case 0xFF:
				tagtype = "Manufacturer";
				break;
			default:
				tagtype = "Unknown";
			}
			fprintf(outfile,
				"EDID %s ext tag 0x%02x rev 0x%02x skipped\n",
				tagtype,
				edid_ext[EEXT_TAG],
				edid_ext[EEXT_REV]);
		}
	}
}


/* Pixel counts normally round to 8 */
#define CLOSE_ENOUGH(a, b) (abs((a)-(b)) < 16)

/* These match order of defines ASPECT_x_y in edid_utils.h */
char *aspect_to_str[]={"16:10","4:3","5:4","16:9"};

int find_aspect(int h, int v)
{
	if (CLOSE_ENOUGH((h * 3), (v * 4)))
		return ASPECT_4_3;
	if (CLOSE_ENOUGH((h * 4), (v * 5)))
		return ASPECT_5_4;
	if (CLOSE_ENOUGH((h * 9), (v * 16)))
		return ASPECT_16_9;
	if (CLOSE_ENOUGH((h * 10), (v * 16)))
		return ASPECT_16_10;

	return -1;
}

int find_aspect_fromisize(unsigned char *edid_data)
{
	int hsiz = edid_data[EDID_MAX_HSIZE];
	int vsiz = edid_data[EDID_MAX_VSIZE];
	int res;

	/* Zero size for projector */
	/* Only use this code if there was no preferred resolution */
	/* So assume it is an older 4:3 projector not a video one  */
	if ((hsiz == 0) && (vsiz == 0))
		return ASPECT_4_3;

	res = find_aspect(hsiz, vsiz);

	/* If things didn't work out, assume the old 4:3 case */
	if (res < 0)
		return ASPECT_4_3;
	else
		return res;
}

int edid_get_monitor_name(const unsigned char *edid_data,
			  char *buf,
			  unsigned int buf_size)
{
	int i;
	const unsigned char *dtd;

	for (i = 0; i < EDID_N_DTDS; i++) {
		dtd = edid_data + (EDID_DTD_BASE + i * DTD_SIZE);
		if (dtd[DTD_PCLK_LO] == 0x00 && dtd[DTD_PCLK_HI] == 0x00 &&
		    dtd[DTD_TYPETAG] == DTDTYPE_NAME) {
			get_dtd_string((const char *)dtd + DTD_STRING,
				       buf, buf_size);
			return 0;
		}
	}

	return -1;
}
