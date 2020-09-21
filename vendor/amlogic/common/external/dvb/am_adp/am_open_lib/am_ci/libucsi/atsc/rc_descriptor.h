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

#ifndef _UCSI_ATSC_RC_DESCRIPTOR
#define _UCSI_ATSC_RC_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * atsc_rc_descriptor structure.
 */
struct atsc_rc_descriptor {
	struct descriptor d;

	/* uint8_t info[] */
} __ucsi_packed;

/**
 * Process an atsc_rc_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return atsc_rc_descriptor pointer, or NULL on error.
 */
static inline struct atsc_rc_descriptor*
	atsc_rc_descriptor_codec(struct descriptor* d)
{
	return (struct atsc_rc_descriptor*) d;
}

/**
 * Accessor for the info field of an atsc_rc_descriptor.
 *
 * @param d atsc_rc_descriptor pointer.
 * @return Pointer to the atsc_text data.
 */
static inline uint8_t*
	atsc_rc_descriptor_info(struct atsc_rc_descriptor *d)
{
	return ((uint8_t*) d) + sizeof(struct atsc_rc_descriptor);
}

/**
 * Accessor for the length of the info field of an atsc_rc_descriptor.
 *
 * @param d atsc_rc_descriptor pointer.
 * @return The length
 */
static inline int
	atsc_rc_descriptor_info_length(struct atsc_rc_descriptor *d)
{
	return d->d.len;
}

#ifdef __cplusplus
}
#endif

#endif
