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

#ifndef _UCSI_ATSC_STT_SECTION_H
#define _UCSI_ATSC_STT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>

/**
 * atsc_stt_section structure.
 */
struct atsc_stt_section {
	struct atsc_section_psip head;

	atsctime_t system_time;
	uint8_t gps_utc_offset;
  EBIT4(uint16_t DS_status			: 1; ,
	uint16_t reserved			: 2; ,
	uint16_t DS_day_of_month		: 5; ,
	uint16_t DS_hour			: 8; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process a atsc_stt_section.
 *
 * @param section Pointer to an atsc_section_psip structure.
 * @return atsc_stt_section pointer, or NULL on error.
 */
struct atsc_stt_section *atsc_stt_section_codec(struct atsc_section_psip *section);

/**
 * Iterator for the services field in a atsc_stt_section.
 *
 * @param stt atsc_stt_section pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_stt_section_descriptors_for_each(stt, pos) \
	for ((pos) = atsc_stt_section_descriptors_first(stt); \
	     (pos); \
	     (pos) = atsc_stt_section_descriptors_next(stt, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	atsc_stt_section_descriptors_first(struct atsc_stt_section *stt)
{
	size_t pos = sizeof(struct atsc_stt_section);

	if (pos >= section_ext_length(&stt->head.ext_head))
		return NULL;

	return (struct descriptor*) ((uint8_t *) stt + pos);
}

static inline struct descriptor *
	atsc_stt_section_descriptors_next(struct atsc_stt_section *stt,
				          struct descriptor *pos)
{
	int len = section_ext_length(&stt->head.ext_head);
	len -= sizeof(struct atsc_stt_section);

	return next_descriptor((uint8_t*) stt + sizeof(struct atsc_stt_section),
				len,
				pos);
}

#ifdef __cplusplus
}
#endif

#endif
