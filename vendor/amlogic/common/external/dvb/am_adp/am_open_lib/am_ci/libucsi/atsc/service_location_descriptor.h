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

#ifndef _UCSI_ATSC_SERVICE_LOCATION_DESCRIPTOR
#define _UCSI_ATSC_SERVICE_LOCATION_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

enum atsc_stream_types {
	ATSC_STREAM_TYPE_VIDEO			= 0x02,
	ATSC_STREAM_TYPE_AUDIO 			= 0x81,
};

/**
 * atsc_service_location_descriptor structure.
 */
struct atsc_service_location_descriptor {
	struct descriptor d;

  EBIT2(uint16_t reserved		: 3; ,
	uint16_t PCR_PID		:13; );
	uint8_t number_elements;
	/* struct atsc_service_location_element elements[] */
} __ucsi_packed;

/**
 * An entry in the elements field of an atsc_service_location_descriptor.
 */
struct atsc_caption_service_location_element {
	uint8_t stream_type;
  EBIT2(uint16_t reserved 		: 3; ,
	uint16_t elementary_PID		:13; );
	iso639lang_t language_code;
} __ucsi_packed;

/**
 * Process an atsc_service_location_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return atsc_service_location_descriptor pointer, or NULL on error.
 */
static inline struct atsc_service_location_descriptor*
	atsc_service_location_descriptor_codec(struct descriptor* d)
{
	struct atsc_service_location_descriptor *ret =
		(struct atsc_service_location_descriptor *) d;
	uint8_t *buf = (uint8_t*) d + 2;
	int pos = 0;
	int idx;

	if (d->len < 3)
		return NULL;
	bswap16(buf + pos);
	pos+=3;

	for(idx = 0; idx < ret->number_elements; idx++) {
		if (d->len < (pos + sizeof(struct atsc_caption_service_entry)))
			return NULL;

		bswap16(buf+pos+1);

		pos += sizeof(struct atsc_caption_service_entry);
	}

	return (struct atsc_service_location_descriptor*) d;
}

/**
 * Iterator for elements field of a atsc_service_location_descriptor.
 *
 * @param d atsc_service_location_descriptor pointer.
 * @param pos Variable holding a pointer to the current atsc_service_location_element.
 * @param idx Integer used to count which dimension we are in.
 */
#define atsc_service_location_descriptor_elements_for_each(d, pos, idx) \
	for ((pos) = atsc_service_location_descriptor_elements_first(d), idx=0; \
	     (pos); \
	     (pos) = atsc_service_location_descriptor_elements_next(d, pos, ++idx))










/******************************** PRIVATE CODE ********************************/
static inline struct atsc_caption_service_location_element*
	atsc_service_location_descriptor_elements_first(struct atsc_service_location_descriptor *d)
{
	if (d->number_elements == 0)
		return NULL;

	return (struct atsc_caption_service_location_element *)
		((uint8_t*) d + sizeof(struct atsc_service_location_descriptor));
}

static inline struct atsc_caption_service_location_element*
	atsc_service_location_descriptor_elements_next(struct atsc_service_location_descriptor *d,
						       struct atsc_caption_service_location_element *pos,
							int idx)
{
	uint8_t *next =	(uint8_t *) pos + sizeof(struct atsc_caption_service_location_element);

	if (idx >= d->number_elements)
		return NULL;
	return (struct atsc_caption_service_location_element *) next;
}

#ifdef __cplusplus
}
#endif

#endif
