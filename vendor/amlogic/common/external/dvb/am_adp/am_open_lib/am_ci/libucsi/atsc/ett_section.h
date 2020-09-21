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

#ifndef _UCSI_ATSC_ETT_SECTION_H
#define _UCSI_ATSC_ETT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>

enum atsc_etm_type {
	ATSC_ETM_CHANNEL 	= 0x00,
	ATSC_ETM_EVENT 		= 0x02,
};

/**
 * atsc_ett_section structure.
 */
struct atsc_ett_section {
	struct atsc_section_psip head;

  EBIT3(uint32_t ETM_source_id			:16; ,
	uint32_t ETM_sub_id			:14; ,
	uint32_t ETM_type			: 2; );
	/* struct atsc_text extended_text_message */
} __ucsi_packed;

/**
 * Process a atsc_ett_section.
 *
 * @param section Pointer to an atsc_section_psip structure.
 * @return atsc_ett_section pointer, or NULL on error.
 */
struct atsc_ett_section *atsc_ett_section_codec(struct atsc_section_psip *section);

/**
 * Accessor for the extended_text_message part of an atsc_ett_section.
 *
 * @param ett atsc_ett_section pointer.
 * @return atsc_text pointer, or NULL on error.
 */
static inline struct atsc_text*
	atsc_ett_section_extended_text_message(struct atsc_ett_section *ett)
{
	int pos = sizeof(struct atsc_ett_section);
	int len = section_ext_length(&ett->head.ext_head) - sizeof(struct atsc_ett_section);

	if (len == 0)
		return NULL;

	return (struct atsc_text*)(((uint8_t*) ett) + pos);
}

/**
 * Accessor for the extended_text_message part of an atsc_ett_section.
 *
 * @param ett atsc_ett_section pointer.
 * @return The length.
 */
static inline int
	atsc_ett_section_extended_text_message_length(struct atsc_ett_section *ett)
{
	return section_ext_length(&ett->head.ext_head) - sizeof(struct atsc_ett_section);
}

#ifdef __cplusplus
}
#endif

#endif
