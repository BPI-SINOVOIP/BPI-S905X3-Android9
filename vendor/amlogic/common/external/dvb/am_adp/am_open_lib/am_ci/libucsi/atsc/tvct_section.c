#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
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

#include <libucsi/atsc/tvct_section.h>

struct atsc_tvct_section *atsc_tvct_section_codec(struct atsc_section_psip *psip)
{
	uint8_t * buf = (uint8_t *) psip;
	size_t pos = sizeof(struct atsc_section_psip);
	size_t len = section_ext_length(&(psip->ext_head));
	int idx;
	struct atsc_tvct_section *tvct = (struct atsc_tvct_section *) psip;

	if (len < sizeof(struct atsc_tvct_section))
		return NULL;

	pos++;

	for (idx =0; idx < tvct->num_channels_in_section; idx++) {

		if ((pos + sizeof(struct atsc_tvct_channel)) > len)
			return NULL;
		struct atsc_tvct_channel *channel = (struct atsc_tvct_channel *) (buf+pos);

		pos += 7*2;

		bswap32(buf+pos);
		bswap32(buf+pos+4);
		bswap16(buf+pos+8);
		bswap16(buf+pos+10);
		bswap16(buf+pos+12);
		bswap16(buf+pos+14);
		bswap16(buf+pos+16);
		pos+=18;

		if ((pos + channel->descriptors_length) > len)
			return NULL;
		if (verify_descriptors(buf + pos, channel->descriptors_length))
			return NULL;

		pos += channel->descriptors_length;
	}

	if ((pos + sizeof(struct atsc_tvct_section_part2)) > len)
		return NULL;
	struct atsc_tvct_section_part2 *part2 = (struct atsc_tvct_section_part2 *) (buf+pos);

	bswap16(buf+pos);
	pos+=2;

	if ((pos + part2->descriptors_length) > len)
		return NULL;

	if (verify_descriptors(buf + pos, part2->descriptors_length))
		return NULL;

	pos += part2->descriptors_length;

	if (pos != len)
		return NULL;

	return (struct atsc_tvct_section *) psip;
}
