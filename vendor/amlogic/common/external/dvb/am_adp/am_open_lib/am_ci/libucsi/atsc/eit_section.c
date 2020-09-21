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

#include <libucsi/atsc/eit_section.h>

struct atsc_eit_section *atsc_eit_section_codec(struct atsc_section_psip *psip)
{
	uint8_t * buf = (uint8_t *) psip;
	size_t pos = 0;
	size_t len = section_ext_length(&(psip->ext_head));
	int idx;

	if (len < sizeof(struct atsc_eit_section))
		return NULL;
	struct atsc_eit_section *eit = (struct atsc_eit_section *) psip;

	pos += sizeof(struct atsc_eit_section);
	for (idx =0; idx < eit->num_events_in_section; idx++) {
		if (len < (pos + sizeof(struct atsc_eit_event)))
			return NULL;
		struct atsc_eit_event *event = (struct atsc_eit_event *) (buf+pos);

		bswap16(buf+pos);
		bswap32(buf+pos+2);
		bswap32(buf+pos+6);

		pos += sizeof(struct atsc_eit_event);
		if (len < (pos + event->title_length))
			return NULL;
		if (atsc_text_validate(buf+pos, event->title_length))
			return NULL;

		pos += event->title_length;
		if (len < (pos + sizeof(struct atsc_eit_event_part2)))
			return NULL;
		struct atsc_eit_event_part2 *part2 = (struct atsc_eit_event_part2 *) (buf+pos);

		bswap16(buf+pos);

		pos += sizeof(struct atsc_eit_event_part2);
		if (len < (pos + part2->descriptors_length))
			return NULL;

		if (verify_descriptors(buf + pos, part2->descriptors_length))
			return NULL;
		pos += part2->descriptors_length;
	}

	if (pos != len)
		return NULL;

	return (struct atsc_eit_section *) psip;
}
