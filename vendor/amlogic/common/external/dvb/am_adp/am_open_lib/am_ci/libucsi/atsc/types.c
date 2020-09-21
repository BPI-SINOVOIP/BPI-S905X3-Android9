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
#include "libucsi/atsc/types.h"

/* GPS epoch == unix time_t at 06/Jan/1980 */
#define GPS_EPOCH 315964800


int atsc_text_validate(uint8_t *buf, int len)
{
	int i;
	int j;
	int number_strings;
	int number_segments;
	int number_bytes;
	int pos = 0;

	if (len == 0)
		return 0;
	number_strings = buf[pos];
	pos++;

	for (i=0; i< number_strings; i++) {
		if (len < (pos+4))
			return -1;
		number_segments = buf[pos+3];
		pos+=4;

		for (j=0; j < number_segments; j++) {
			if (len < (pos+3))
				return -1;
			number_bytes = buf[pos+2];
			pos+=3;

			if (len < (pos + number_bytes))
				return -1;
			pos += number_bytes;
		}
	}

	return 0;
}

time_t atsctime_to_unixtime(atsctime_t atsc)
{
	return atsc + GPS_EPOCH;
}

atsctime_t unixtime_to_atsctime(time_t t)
{
	return t - GPS_EPOCH;
}
