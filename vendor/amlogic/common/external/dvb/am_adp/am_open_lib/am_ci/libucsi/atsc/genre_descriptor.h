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

#ifndef _UCSI_ATSC_GENRE_DESCRIPTOR
#define _UCSI_ATSC_GENRE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * atsc_genre_descriptor structure.
 */
struct atsc_genre_descriptor {
	struct descriptor d;

  EBIT2(uint8_t reserved		: 3; ,
	uint8_t attribute_count		: 5; );
	/* uint8_t attributes[] */
} __ucsi_packed;

/**
 * Process an atsc_genre_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return atsc_genre_descriptor pointer, or NULL on error.
 */
static inline struct atsc_genre_descriptor*
	atsc_genre_descriptor_codec(struct descriptor* d)
{
	struct atsc_genre_descriptor *ret =
		(struct atsc_genre_descriptor *) d;

	if (d->len < 1)
		return NULL;

	if (d->len != (1 + ret->attribute_count))
		return NULL;

	return (struct atsc_genre_descriptor*) d;
}

/**
 * Accessor for the attributes field of an atsc_genre_descriptor.
 *
 * @param d atsc_genre_descriptor pointer.
 * @return Pointer to the attributes.
 */
static inline uint8_t*
	atsc_genre_descriptor_attributes(struct atsc_genre_descriptor *d)
{
	return ((uint8_t*) d) + sizeof(struct atsc_genre_descriptor);
}

#ifdef __cplusplus
}
#endif

#endif
