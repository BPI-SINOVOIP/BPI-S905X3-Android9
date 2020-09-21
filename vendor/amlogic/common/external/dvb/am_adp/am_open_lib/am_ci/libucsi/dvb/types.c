#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <string.h>
#include "types.h"

time_t dvbdate_to_unixtime(dvbdate_t dvbdate)
{
	int k = 0;
	struct tm tm;
	double mjd;

	/* check for the undefined value */
	if ((dvbdate[0] == 0xff) &&
	    (dvbdate[1] == 0xff) &&
	    (dvbdate[2] == 0xff) &&
	    (dvbdate[3] == 0xff) &&
	    (dvbdate[4] == 0xff)) {
		return -1;
	}

	memset(&tm, 0, sizeof(tm));
	mjd = (dvbdate[0] << 8) | dvbdate[1];

	tm.tm_year = (int) ((mjd - 15078.2) / 365.25);
	tm.tm_mon = (int) (((mjd - 14956.1) - (int) (tm.tm_year * 365.25)) / 30.6001);
	tm.tm_mday = (int) mjd - 14956 - (int) (tm.tm_year * 365.25) - (int) (tm.tm_mon * 30.6001);
	if ((tm.tm_mon == 14) || (tm.tm_mon == 15)) k = 1;
	tm.tm_year += k;
	tm.tm_mon = tm.tm_mon - 2 - k * 12;
	tm.tm_sec = bcd_to_integer(dvbdate[4]);
	tm.tm_min = bcd_to_integer(dvbdate[3]);
	tm.tm_hour = bcd_to_integer(dvbdate[2]);

	return mktime(&tm);
}

void unixtime_to_dvbdate(time_t unixtime, dvbdate_t dvbdate)
{
	struct tm tm;
	double l = 0;
	int mjd;

	/* the undefined value */
	if (unixtime == -1) {
		memset(dvbdate, 0xff, 5);
		return;
	}

	gmtime_r(&unixtime, &tm);
	tm.tm_mon++;
	if ((tm.tm_mon == 1) || (tm.tm_mon == 2)) l = 1;
	mjd = 14956 + tm.tm_mday + (int) ((tm.tm_year - l) * 365.25) + (int) ((tm.tm_mon + 1 + l * 12) * 30.6001);

	dvbdate[0] = (mjd & 0xff00) >> 8;
	dvbdate[1] = mjd & 0xff;
	dvbdate[2] = integer_to_bcd(tm.tm_hour);
	dvbdate[3] = integer_to_bcd(tm.tm_min);
	dvbdate[4] = integer_to_bcd(tm.tm_sec);
}

int dvbduration_to_seconds(dvbduration_t dvbduration)
{
	int seconds = 0;

	seconds += (bcd_to_integer(dvbduration[0]) * 60 * 60);
	seconds += (bcd_to_integer(dvbduration[1]) * 60);
	seconds += bcd_to_integer(dvbduration[2]);

	return seconds;
}

void seconds_to_dvbduration(int seconds, dvbduration_t dvbduration)
{
	int hours, mins;

	hours = seconds / (60*60);
	seconds -= (hours * 60 * 60);
	mins = seconds / 60;
	seconds -= (mins * 60);

	dvbduration[0] = integer_to_bcd(hours);
	dvbduration[1] = integer_to_bcd(mins);
	dvbduration[2] = integer_to_bcd(seconds);
}

int dvbhhmm_to_seconds(dvbhhmm_t dvbhhmm)
{
	int seconds = 0;

	seconds += (bcd_to_integer(dvbhhmm[0]) * 60 * 60);
	seconds += (bcd_to_integer(dvbhhmm[1]) * 60);

	return seconds;
}

void seconds_to_dvbhhmm(int seconds, dvbhhmm_t dvbhhmm)
{
	int hours, mins;

	hours = seconds / (60*60);
	seconds -= (hours * 60 * 60);
	mins = seconds / 60;

	dvbhhmm[0] = integer_to_bcd(hours);
	dvbhhmm[1] = integer_to_bcd(mins);
}

uint32_t integer_to_bcd(uint32_t intval)
{
	uint32_t val = 0;

	int i;
	for (i=0; i<=28;i+=4) {
		val |= ((intval % 10) << i);
		intval /= 10;
	}

	return val;
}

uint32_t bcd_to_integer(uint32_t bcdval)
{
	uint32_t val = 0;

	int i;
	for (i=28; i>=0;i-=4) {
		val += ((bcdval >> i) & 0x0f);
		if (i != 0) val *= 10;
	}

	return val;
}

const char *dvb_charset(char *dvb_text, int dvb_text_length, int *consumed)
{
	char *charset = "ISO6937";
	int used = 0;

	if (dvb_text_length == 0)
		goto exit;
	if (dvb_text[0] >= 32)
		goto exit;
	if (dvb_text[0] == 0x10) {
		if (dvb_text_length < 3)
			goto exit;

		used = 3;
		uint16_t ext = (dvb_text[1] << 8) | dvb_text[2];
		switch (ext) {
		case 0x01:
			charset = "ISO8859-1";
			break;
		case 0x02:
			charset = "ISO8859-2";
			break;
		case 0x03:
			charset = "ISO8859-3";
			break;
		case 0x04:
			charset = "ISO8859-4";
			break;
		case 0x05:
			charset = "ISO8859-5";
			break;
		case 0x06:
			charset = "ISO8859-6";
			break;
		case 0x07:
			charset = "ISO8859-7";
			break;
		case 0x08:
			charset = "ISO8859-8";
			break;
		case 0x09:
			charset = "ISO8859-9";
			break;
		case 0x0a:
			charset = "ISO8859-10";
			break;
		case 0x0b:
			charset = "ISO8859-11";
			break;
		case 0x0d:
			charset = "ISO8859-13";
			break;
		case 0x0e:
			charset = "ISO8859-14";
			break;
		case 0x0f:
			charset = "ISO8859-15";
			break;
		default:
			used = 0;
			break;
		}
	} else {
		used = 1;
		switch (dvb_text[0]) {
		case 0x01:
			charset = "ISO8859-5";
			break;
		case 0x02:
			charset = "ISO8859-6";
			break;
		case 0x03:
			charset = "ISO8859-7";
			break;
		case 0x04:
			charset = "ISO8859-8";
			break;
		case 0x05:
			charset = "ISO8859-9";
			break;
		case 0x06:
			charset = "ISO8859-10";
			break;
		case 0x07:
			charset = "ISO8859-11";
			break;
		case 0x09:
			charset = "ISO8859-13";
			break;
		case 0x0a:
			charset = "ISO8859-14";
			break;
		case 0x0b:
			charset = "ISO8859-15";
			break;
		case 0x11:
			charset = "UTF16";
			break;
		case 0x12:
			charset = "EUC-KR";
			break;
		case 0x13:
			charset = "GB2312";
			break;
		case 0x14:
			charset = "GBK";
			break;
		case 0x15:
			charset = "UTF8";
			break;
		default:
			used = 0;
			break;
		}
	}
exit:
	*consumed = used;
	return charset;
}
