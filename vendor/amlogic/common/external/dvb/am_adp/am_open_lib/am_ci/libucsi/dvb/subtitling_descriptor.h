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

#ifndef _UCSI_DVB_SUBTITLING_DESCRIPTOR
#define _UCSI_DVB_SUBTITLING_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * dvb_subtitling_descriptor structure.
 */
struct dvb_subtitling_descriptor {
	struct descriptor d;

	/* struct dvb_subtitling_entry subtitles[] */
} __ucsi_packed;

/**
 * An entry in the subtitles field of the a dvb_subtitling_descriptor.
 */
struct dvb_subtitling_entry {
	iso639lang_t language_code;
	uint8_t subtitling_type;
	uint16_t composition_page_id;
	uint16_t ancillary_page_id;
} __ucsi_packed;

/**
 * Process a dvb_subtitling_descriptor.
 *
 * @param d Generic descriptor.
 * @return dvb_subtitling_descriptor pointer, or NULL on error.
 */
static inline struct dvb_subtitling_descriptor*
	dvb_subtitling_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint8_t* ptr = (uint8_t*) d + 2;
	uint32_t len = d->len;

	if (len % sizeof(struct dvb_subtitling_entry))
		return NULL;

	while(pos < len) {
		bswap16(ptr+pos+4);
		bswap16(ptr+pos+6);
		pos += sizeof(struct dvb_subtitling_entry);
	}

	return (struct dvb_subtitling_descriptor*) d;
}

/**
 * Iterator for subtitles field in dvb_subtitling_descriptor.
 *
 * @param d dvb_subtitling_descriptor pointer.
 * @param pos Variable containing a pointer to current dvb_subtitling_entry.
 */
#define dvb_subtitling_descriptor_subtitles_for_each(d, pos) \
	for ((pos) = dvb_subtitling_descriptor_subtitles_first(d); \
	     (pos); \
	     (pos) = dvb_subtitling_descriptor_subtitles_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_subtitling_entry*
	dvb_subtitling_descriptor_subtitles_first(struct dvb_subtitling_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_subtitling_entry *)
		((uint8_t*) d + sizeof(struct dvb_subtitling_descriptor));
}

static inline struct dvb_subtitling_entry*
	dvb_subtitling_descriptor_subtitles_next(struct dvb_subtitling_descriptor *d,
						 struct dvb_subtitling_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_subtitling_entry);

	if (next >= end)
		return NULL;

	return (struct dvb_subtitling_entry *) next;
}

#ifdef __cplusplus
}
#endif

#endif
