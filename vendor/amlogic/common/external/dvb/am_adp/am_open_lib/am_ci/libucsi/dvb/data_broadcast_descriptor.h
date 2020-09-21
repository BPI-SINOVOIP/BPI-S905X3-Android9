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

#ifndef _UCSI_DVB_DATA_BROADCAST_DESCRIPTOR
#define _UCSI_DVB_DATA_BROADCAST_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * dvb_data_broadcast_descriptor structure.
 */
struct dvb_data_broadcast_descriptor {
	struct descriptor d;

	uint16_t data_broadcast_id;
	uint8_t component_tag;
	uint8_t selector_length;
	/* uint8_t selector[] */
	/* struct dvb_data_broadcast_descriptor_part2 part2 */
} __ucsi_packed;

/**
 * Second part of a dvb_data_broadcast_descriptor following the variable length selector field.
 */
struct dvb_data_broadcast_descriptor_part2 {
	iso639lang_t language_code;
	uint8_t text_length;
	/* uint8_t text[] */
} __ucsi_packed;

/**
 * Process a dvb_data_broadcast_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_data_broadcast_descriptor pointer, or NULL on error.
 */
static inline struct dvb_data_broadcast_descriptor*
	dvb_data_broadcast_descriptor_codec(struct descriptor* d)
{
	struct dvb_data_broadcast_descriptor *p =
		(struct dvb_data_broadcast_descriptor *) d;
	struct dvb_data_broadcast_descriptor_part2 *p2;
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t pos = sizeof(struct dvb_data_broadcast_descriptor) - 2;
	uint32_t len = d->len;

	if (pos > len)
		return NULL;

	bswap16(buf + 2);

	pos += p->selector_length;

	if (pos > len)
		return NULL;

	p2 = (struct dvb_data_broadcast_descriptor_part2*) (buf + 2 + pos);

	pos += sizeof(struct dvb_data_broadcast_descriptor_part2);

	if (pos > len)
		return NULL;

	pos += p2->text_length;

	if (pos != len)
		return NULL;

	return p;
}

/**
 * Accessor for the selector field of a dvb_data_broadcast_descriptor.
 *
 * @param d dvb_data_broadcast_descriptor pointer.
 * @return pointer to the field.
 */
static inline uint8_t *
	dvb_data_broadcast_descriptor_selector(struct dvb_data_broadcast_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_data_broadcast_descriptor);
}

/**
 * Accessor for the second part of a dvb_data_broadcast_descriptor.
 *
 * @param d dvb_data_broadcast_descriptor pointer.
 * @return dvb_data_broadcast_descriptor_part2 pointer.
 */
static inline struct dvb_data_broadcast_descriptor_part2 *
	dvb_data_broadcast_descriptor_part2(struct dvb_data_broadcast_descriptor *d)
{
	return (struct dvb_data_broadcast_descriptor_part2*)
		((uint8_t*) d + sizeof(struct dvb_data_broadcast_descriptor) +
		 d->selector_length);
}

/**
 * Accessor for the text field in a dvb_data_broadcast_descriptor_part2.
 *
 * @param d dvb_data_broadcast_descriptor_part2 pointer.
 * @return pointer to the field.
 */
static inline uint8_t *
	dvb_data_broadcast_descriptor_part2_text(struct dvb_data_broadcast_descriptor_part2 *d)
{
	return (uint8_t *) d + sizeof(struct dvb_data_broadcast_descriptor_part2);
}

#ifdef __cplusplus
}
#endif

#endif
