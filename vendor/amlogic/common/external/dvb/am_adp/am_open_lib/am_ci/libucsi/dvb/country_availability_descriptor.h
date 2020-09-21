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

#ifndef _UCSI_DVB_COUNTRY_AVAILABILITY_DESCRIPTOR
#define _UCSI_DVB_COUNTRY_AVAILABILITY_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * dvb_country_availability_descriptor structure.
 */
struct dvb_country_availability_descriptor {
	struct descriptor d;

  EBIT2(uint8_t country_availability_flag	: 1; ,
	uint8_t reserved			: 7; );
	/* struct dvb_country_availability_entry countries[] */
} __ucsi_packed;

/**
 * An entry in the countries field of a dvb_country_availability_descriptor.
 */
struct dvb_country_availability_entry {
	iso639country_t country_code;
} __ucsi_packed;

/**
 * Process a dvb_country_availability_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_country_availability_descriptor pointer, or NULL on error.
 */
static inline struct dvb_country_availability_descriptor*
	dvb_country_availability_descriptor_codec(struct descriptor* d)
{
	uint32_t len = d->len;

	if (len < (sizeof(struct dvb_country_availability_descriptor) - 2))
		return NULL;

	if ((len - 1) % sizeof(struct dvb_country_availability_entry))
		return NULL;

	return (struct dvb_country_availability_descriptor*) d;
}

/**
 * Iterator for the countries field of a dvb_country_availability_descriptor.
 *
 * @param d dvb_country_availability_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_country_availability_entry.
 */
#define dvb_country_availability_descriptor_countries_for_each(d, pos) \
	for ((pos) = dvb_country_availability_descriptor_countries_first(d); \
	     (pos); \
	     (pos) = dvb_country_availability_descriptor_countries_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_country_availability_entry*
	dvb_country_availability_descriptor_countries_first(struct dvb_country_availability_descriptor *d)
{
	if (d->d.len == 1)
		return NULL;

	return (struct dvb_country_availability_entry *)
		((uint8_t*) d + sizeof(struct dvb_country_availability_descriptor));
}

static inline struct dvb_country_availability_entry*
	dvb_country_availability_descriptor_countries_next(struct dvb_country_availability_descriptor *d,
							   struct dvb_country_availability_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_country_availability_entry);

	if (next >= end)
		return NULL;

	return (struct dvb_country_availability_entry *) next;
}

#ifdef __cplusplus
}
#endif

#endif
