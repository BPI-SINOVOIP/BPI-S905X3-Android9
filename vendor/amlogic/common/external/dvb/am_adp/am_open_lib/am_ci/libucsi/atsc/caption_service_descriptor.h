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

#ifndef _UCSI_ATSC_CAPTION_SERVICE_DESCRIPTOR
#define _UCSI_ATSC_CAPTION_SERVICE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * atsc_caption_service_descriptor structure.
 */
struct atsc_caption_service_descriptor {
	struct descriptor d;

  EBIT2(uint8_t reserved		: 3; ,
	uint8_t number_of_services	: 5; );
	/* struct atsc_caption_service_entry entries[] */
} __ucsi_packed;

/**
 * An entry in the entries field of a atsc_caption_service_descriptor.
 */
struct atsc_caption_service_entry {
	iso639lang_t language_code;
  EBIT3(uint8_t digital_cc 		: 1; ,
	uint8_t reserved		: 1; ,
	uint8_t value			: 6; );
  EBIT3(uint16_t easy_reader 		: 1; ,
	uint16_t wide_aspect_ratio	: 1; ,
	uint16_t reserved1		:14; );
} __ucsi_packed;

/**
 * Process an atsc_caption_service_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return atsc_caption_service_descriptor pointer, or NULL on error.
 */
static inline struct atsc_caption_service_descriptor*
	atsc_caption_service_descriptor_codec(struct descriptor* d)
{
	struct atsc_caption_service_descriptor *ret =
		(struct atsc_caption_service_descriptor *) d;
	uint8_t *buf = (uint8_t*) d + 2;
	int pos = 0;
	int idx;

	if (d->len < 1)
		return NULL;
	pos++;

	for(idx = 0; idx < ret->number_of_services; idx++) {
		if (d->len < (pos + sizeof(struct atsc_caption_service_entry)))
			return NULL;

		bswap16(buf+pos+4);

		pos += sizeof(struct atsc_caption_service_entry);
	}

	return (struct atsc_caption_service_descriptor*) d;
}

/**
 * Iterator for entries field of a atsc_caption_service_descriptor.
 *
 * @param d atsc_caption_service_descriptor pointer.
 * @param pos Variable holding a pointer to the current atsc_caption_service_entry.
 * @param idx Field iterator integer.
 */
#define atsc_caption_service_descriptor_entries_for_each(d, pos, idx) \
	for ((pos) = atsc_caption_service_descriptor_entries_first(d), idx=0; \
	     (pos); \
	     (pos) = atsc_caption_service_descriptor_entries_next(d, pos, ++idx))










/******************************** PRIVATE CODE ********************************/
static inline struct atsc_caption_service_entry*
	atsc_caption_service_descriptor_entries_first(struct atsc_caption_service_descriptor *d)
{
	if (d->number_of_services == 0)
		return NULL;

	return (struct atsc_caption_service_entry *)
		((uint8_t*) d + sizeof(struct atsc_caption_service_descriptor));
}

static inline struct atsc_caption_service_entry*
	atsc_caption_service_descriptor_entries_next(struct atsc_caption_service_descriptor *d,
						     struct atsc_caption_service_entry *pos,
						     int idx)
{
	if (idx >= d->number_of_services)
		return NULL;

	return (struct atsc_caption_service_entry *)
		((uint8_t *) pos + sizeof(struct atsc_caption_service_entry));
}

#ifdef __cplusplus
}
#endif

#endif
