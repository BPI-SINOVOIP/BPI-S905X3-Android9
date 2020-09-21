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

#ifndef _UCSI_DVB_LOCAL_TIME_OFFSET_DESCRIPTOR
#define _UCSI_DVB_LOCAL_TIME_OFFSET_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>
#include <libucsi/dvb/types.h>

/**
 * dvb_local_time_offset_descriptor parameter.
 */
struct dvb_local_time_offset_descriptor {
	struct descriptor d;

	/* struct dvb_local_time_offset offsets[] */
} __ucsi_packed;

/**
 * Entry in the offsets field of dvb_local_time_offset_descriptor.
 */
struct dvb_local_time_offset {
	iso639country_t country_code;
  EBIT3(uint8_t country_region_id		: 6; ,
	uint8_t reserved			: 1; ,
	uint8_t local_time_offset_polarity	: 1; );
	dvbhhmm_t local_time_offset;
	dvbdate_t time_of_change;
	dvbhhmm_t next_time_offset;
} __ucsi_packed;

/**
 * Process a dvb_local_time_offset_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_local_time_offset_descriptor pointer, or NULL on error.
 */
static inline struct dvb_local_time_offset_descriptor*
	dvb_local_time_offset_descriptor_codec(struct descriptor* d)
{
	uint32_t len = d->len;
	uint32_t pos = 0;

	if (len % sizeof(struct dvb_local_time_offset))
		return NULL;

	while(pos < len) {
		pos += sizeof(struct dvb_local_time_offset);
	}

	return (struct dvb_local_time_offset_descriptor*) d;
}

/**
 * Iterator for the offsets field of a dvb_local_time_offset_descriptor.
 *
 * @param d dvb_local_time_offset_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_local_time_offset.
 */
#define dvb_local_time_offset_descriptor_offsets_for_each(d, pos) \
	for ((pos) = dvb_local_time_offset_descriptor_offsets_first(d); \
	     (pos); \
	     (pos) = dvb_local_time_offset_descriptor_offsets_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_local_time_offset*
	dvb_local_time_offset_descriptor_offsets_first(struct dvb_local_time_offset_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_local_time_offset *)
		((uint8_t*) d + sizeof(struct dvb_local_time_offset_descriptor));
}

static inline struct dvb_local_time_offset*
	dvb_local_time_offset_descriptor_offsets_next(struct dvb_local_time_offset_descriptor *d,
						      struct dvb_local_time_offset *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_local_time_offset);

	if (next >= end)
		return NULL;

	return (struct dvb_local_time_offset *) next;
}

#ifdef __cplusplus
}
#endif

#endif
