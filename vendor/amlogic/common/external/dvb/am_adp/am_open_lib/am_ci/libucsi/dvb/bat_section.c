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

#include <libucsi/dvb/bat_section.h>

struct dvb_bat_section * dvb_bat_section_codec(struct section_ext * ext)
{
	uint8_t * buf = (uint8_t *) ext;
	size_t pos = sizeof(struct section_ext);
	size_t len = section_ext_length(ext);
	struct dvb_bat_section * ret = (struct dvb_bat_section *) ext;

	if (len < sizeof(struct dvb_bat_section))
		return NULL;

	bswap16(buf + pos);
	pos += 2;

	if ((pos + ret->bouquet_descriptors_length) > len)
		return NULL;

	if (verify_descriptors(buf + pos, ret->bouquet_descriptors_length))
		return NULL;
	pos += ret->bouquet_descriptors_length;

	if ((pos + sizeof(struct dvb_bat_section_part2)) > len)
		return NULL;

	bswap16(buf + pos);
	pos += sizeof(struct dvb_bat_section_part2);

	while (pos < len) {
		struct dvb_bat_transport * transport =
			(struct dvb_bat_transport *) (buf + pos);

		if ((pos + sizeof(struct dvb_bat_transport))  > len)
			return NULL;

		bswap16(buf + pos);
		bswap16(buf + pos + 2);
		bswap16(buf + pos + 4);

		pos += sizeof(struct dvb_bat_transport);

		if ((pos + transport->transport_descriptors_length) > len)
			return NULL;

		if (verify_descriptors(buf + pos,
					transport->transport_descriptors_length))
			return NULL;

		pos += transport->transport_descriptors_length;
	}

	if (pos != len)
		return NULL;

	return ret;
}
