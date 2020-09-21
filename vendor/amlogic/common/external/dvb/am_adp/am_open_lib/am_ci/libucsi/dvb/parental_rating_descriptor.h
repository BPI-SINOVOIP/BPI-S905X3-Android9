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

#ifndef _UCSI_DVB_PARENTAL_RATING_DESCRIPTOR
#define _UCSI_DVB_PARENTAL_RATING_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * Defined values for the rating field.
 */
enum {
	DVB_PARENTAL_RATING_MIN_3YEARS		= 0x01,
	DVB_PARENTAL_RATING_MIN_4YEARS		= 0x02,
	DVB_PARENTAL_RATING_MIN_5YEARS		= 0x03,
	DVB_PARENTAL_RATING_MIN_6YEARS		= 0x04,
	DVB_PARENTAL_RATING_MIN_7YEARS		= 0x05,
	DVB_PARENTAL_RATING_MIN_8YEARS		= 0x06,
	DVB_PARENTAL_RATING_MIN_9YEARS		= 0x07,
	DVB_PARENTAL_RATING_MIN_10YEARS		= 0x08,
	DVB_PARENTAL_RATING_MIN_11YEARS		= 0x09,
	DVB_PARENTAL_RATING_MIN_12YEARS		= 0x0a,
	DVB_PARENTAL_RATING_MIN_13YEARS		= 0x0b,
	DVB_PARENTAL_RATING_MIN_14YEARS		= 0x0c,
	DVB_PARENTAL_RATING_MIN_15YEARS		= 0x0d,
	DVB_PARENTAL_RATING_MIN_16YEARS		= 0x0e,
	DVB_PARENTAL_RATING_MIN_17YEARS		= 0x0f,
};

/**
 * dvb_parental_rating_descriptor structure.
 */
struct dvb_parental_rating_descriptor {
	struct descriptor d;

	/* struct dvb_parental_rating ratings[] */
} __ucsi_packed;

/**
 * An entry in the ratings field of a dvb_parental_rating_descriptor.
 */
struct dvb_parental_rating {
	iso639country_t country_code;
	uint8_t rating;
} __ucsi_packed;

/**
 * Process a dvb_parental_rating_descriptor.
 *
 * @param d Generic descriptor structure pointer.
 * @return dvb_parental_rating_descriptor pointer, or NULL on error.
 */
static inline struct dvb_parental_rating_descriptor*
	dvb_parental_rating_descriptor_codec(struct descriptor* d)
{
	if (d->len % sizeof(struct dvb_parental_rating))
		return NULL;

	return (struct dvb_parental_rating_descriptor*) d;
}

/**
 * Iterator for entries in the ratings field of a dvb_parental_rating_descriptor.
 *
 * @param d dvb_parental_rating_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_parental_rating.
 */
#define dvb_parental_rating_descriptor_ratings_for_each(d, pos) \
	for ((pos) = dvb_parental_rating_descriptor_ratings_first(d); \
	     (pos); \
	     (pos) = dvb_parental_rating_descriptor_ratings_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_parental_rating*
	dvb_parental_rating_descriptor_ratings_first(struct dvb_parental_rating_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_parental_rating *)
		((uint8_t*) d + sizeof(struct dvb_parental_rating_descriptor));
}

static inline struct dvb_parental_rating*
	dvb_parental_rating_descriptor_ratings_next(struct dvb_parental_rating_descriptor *d,
						    struct dvb_parental_rating *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_parental_rating);

	if (next >= end)
		return NULL;

	return (struct dvb_parental_rating *) next;
}

#ifdef __cplusplus
}
#endif

#endif
