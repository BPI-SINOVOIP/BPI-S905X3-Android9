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

#ifndef _UCSI_DVB_MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR
#define _UCSI_DVB_MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * dvb_multilingual_bouquet_name_descriptor structure.
 */
struct dvb_multilingual_bouquet_name_descriptor {
	struct descriptor d;

	/* struct dvb_multilingual_bouquet_name names[]*/
} __ucsi_packed;

/**
 * An entry in the names field of a dvb_multilingual_bouquet_name_descriptor.
 */
struct dvb_multilingual_bouquet_name {
	iso639lang_t language_code;
	uint8_t bouquet_name_length;
	/* uint8_t name[] */
} __ucsi_packed;

/**
 * Process a dvb_multilingual_bouquet_name_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_multilingual_bouquet_name_descriptor pointer, or NULL on error.
 */
static inline struct dvb_multilingual_bouquet_name_descriptor*
	dvb_multilingual_bouquet_name_descriptor_codec(struct descriptor* d)
{
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t pos = 0;
	uint32_t len = d->len;

	while(pos < len) {
		struct dvb_multilingual_bouquet_name *e =
			(struct dvb_multilingual_bouquet_name*) (buf+pos);

		pos += sizeof(struct dvb_multilingual_bouquet_name);

		if (pos > len)
			return NULL;

		pos += e->bouquet_name_length;

		if (pos > len)
			return NULL;
	}

	return (struct dvb_multilingual_bouquet_name_descriptor*) d;
}

/**
 * Iterator for entries in the names field of a dvb_multilingual_bouquet_name_descriptor.
 *
 * @param d dvb_multilingual_bouquet_name_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_multilingual_bouquet_name.
 */
#define dvb_multilingual_bouquet_name_descriptor_names_for_each(d, pos) \
	for ((pos) = dvb_multilingual_bouquet_name_descriptor_names_first(d); \
	     (pos); \
	     (pos) = dvb_multilingual_bouquet_name_descriptor_names_next(d, pos))

/**
 * Accessor for the name field of a dvb_multilingual_bouquet_name.
 *
 * @param e dvb_multilingual_bouquet_name pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_multilingual_bouquet_name_name(struct dvb_multilingual_bouquet_name *e)
{
	return (uint8_t *) e + sizeof(struct dvb_multilingual_bouquet_name);
}










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_multilingual_bouquet_name*
	dvb_multilingual_bouquet_name_descriptor_names_first(struct dvb_multilingual_bouquet_name_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_multilingual_bouquet_name *)
		((uint8_t*) d + sizeof(struct dvb_multilingual_bouquet_name_descriptor));
}

static inline struct dvb_multilingual_bouquet_name*
	dvb_multilingual_bouquet_name_descriptor_names_next(struct dvb_multilingual_bouquet_name_descriptor *d,
							    struct dvb_multilingual_bouquet_name *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos +
			sizeof(struct dvb_multilingual_bouquet_name) +
			pos->bouquet_name_length;

	if (next >= end)
		return NULL;

	return (struct dvb_multilingual_bouquet_name *) next;
}

#ifdef __cplusplus
}
#endif

#endif
