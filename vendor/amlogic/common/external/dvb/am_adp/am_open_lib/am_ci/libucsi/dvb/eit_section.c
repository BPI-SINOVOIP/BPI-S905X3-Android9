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

#include <libucsi/dvb/eit_section.h>

struct dvb_eit_section *dvb_eit_section_codec(struct section_ext * ext)
{
	uint8_t * buf = (uint8_t *) ext;
	size_t pos = sizeof(struct section_ext);
	size_t len = section_ext_length(ext);

	if (len < sizeof(struct dvb_eit_section))
		return NULL;

	bswap16(buf + pos);
	pos += 2;
	bswap16(buf + pos);
	pos += 4;

	while (pos < len) {
		struct dvb_eit_event * event =
			(struct dvb_eit_event *) (buf + pos);

		if ((pos + sizeof(struct dvb_eit_event)) > len)
			return NULL;

		bswap16(buf + pos);
		bswap16(buf + pos + 10);

		pos += sizeof(struct dvb_eit_event);

		if ((pos + event->descriptors_loop_length) > len)
			return NULL;

		if (verify_descriptors(buf + pos, event->descriptors_loop_length))
			return NULL;

		pos += event->descriptors_loop_length;
	}

	if (pos != len)
		return NULL;

	return (struct dvb_eit_section *) ext;
}
