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

#ifndef _UCSI_DVB_CONTENT_DESCRIPTOR
#define _UCSI_DVB_CONTENT_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

// FIXME: the nibbles

/**
 * dvb_content_descriptor structure.
 */
struct dvb_content_descriptor {
	struct descriptor d;

	/* struct dvb_content_nibble nibbles[] */
} __ucsi_packed;

/**
 * An entry in the nibbles field of a dvb_content_descriptor.
 */
struct dvb_content_nibble {
  EBIT2(uint8_t content_nibble_level_1	: 4; ,
	uint8_t content_nibble_level_2	: 4; );
  EBIT2(uint8_t user_nibble_1		: 4; ,
	uint8_t user_nibble_2		: 4; );
} __ucsi_packed;

/**
 * Process a dvb_content_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_content_descriptor pointer, or NULL on error.
 */
static inline struct dvb_content_descriptor*
	dvb_content_descriptor_codec(struct descriptor* d)
{
	if (d->len % sizeof(struct dvb_content_nibble))
		return NULL;

	return (struct dvb_content_descriptor*) d;
}

/**
 * Iterator for the nibbles field of a dvb_content_descriptor.
 *
 * @param d dvb_content_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_content_nibble.
 */
#define dvb_content_descriptor_nibbles_for_each(d, pos) \
	for ((pos) = dvb_content_descriptor_nibbles_first(d); \
	     (pos); \
	     (pos) = dvb_content_descriptor_nibbles_next(d, pos))









/******************************** PRIVATE CODE ********************************/
static inline struct dvb_content_nibble*
	dvb_content_descriptor_nibbles_first(struct dvb_content_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_content_nibble *)
		((uint8_t*) d + sizeof(struct dvb_content_descriptor));
}

static inline struct dvb_content_nibble*
	dvb_content_descriptor_nibbles_next(struct dvb_content_descriptor *d,
					    struct dvb_content_nibble *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_content_nibble);

	if (next >= end)
		return NULL;

	return (struct dvb_content_nibble *) next;
}

#ifdef __cplusplus
}
#endif

#endif
