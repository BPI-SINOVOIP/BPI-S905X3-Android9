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

#ifndef _UCSI_ATSC_COMPONENT_NAME_DESCRIPTOR
#define _UCSI_ATSC_COMPONENT_NAME_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>
#include <libucsi/atsc/types.h>

/**
 * atsc_component_name_descriptor structure.
 */
struct atsc_component_name_descriptor {
	struct descriptor d;

	/* struct atsc_text text[] */
} __ucsi_packed;

/**
 * Process an atsc_component_name_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return atsc_component_name_descriptor pointer, or NULL on error.
 */
static inline struct atsc_component_name_descriptor*
	atsc_component_name_descriptor_codec(struct descriptor* d)
{
	uint8_t *txt = ((uint8_t*) d) + sizeof(struct atsc_component_name_descriptor);

	if (atsc_text_validate(txt, d->len))
		return NULL;

	return (struct atsc_component_name_descriptor*) d;
}

/**
 * Accessor for the text field of an atsc_component_name_descriptor.
 *
 * @param d atsc_component_name_descriptor pointer.
 * @return Pointer to the atsc_text data, or NULL on error.
 */
static inline struct atsc_text*
	atsc_component_name_descriptor_text(struct atsc_component_name_descriptor *d)
{
	uint8_t *txt = ((uint8_t*) d) + sizeof(struct atsc_component_name_descriptor);

	return (struct atsc_text*) txt;
}

/**
 * Accessor for the length of the text field of an atsc_component_name_descriptor_text.
 *
 * @param d atsc_component_name_descriptor pointer.
 * @return The length in bytes.
 */
static inline int
	atsc_component_name_descriptor_text_length(struct atsc_component_name_descriptor *d)
{
	return d->d.len;
}


#ifdef __cplusplus
}
#endif

#endif
