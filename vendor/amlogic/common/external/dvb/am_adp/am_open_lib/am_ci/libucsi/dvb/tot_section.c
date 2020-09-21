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

#include <libucsi/dvb/tot_section.h>

struct dvb_tot_section *dvb_tot_section_codec(struct section *section)
{
	uint8_t * buf = (uint8_t *)section;
	size_t pos = sizeof(struct section);
	size_t len = section_length(section) - CRC_SIZE;
	struct dvb_tot_section * ret = (struct dvb_tot_section *)section;

	if (len < sizeof(struct dvb_tot_section))
		return NULL;

	pos += 5;
	bswap16(buf + pos);
	pos += 2;

	if ((pos + ret->descriptors_loop_length) > len)
		return NULL;

	if (verify_descriptors(buf + pos, ret->descriptors_loop_length))
		return NULL;

	pos += ret->descriptors_loop_length;

	if (pos != len)
		return NULL;

	return ret;
}
