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

#ifndef _UCSI_MPEG_MUXCODE_DESCRIPTOR
#define _UCSI_MPEG_MUXCODE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * mpeg_muxcode_descriptor structure
 */
struct mpeg_muxcode_descriptor {
	struct descriptor d;

	/* uint8_t entries[] */
} __ucsi_packed;

/**
 * Process an mpeg_muxcode_descriptor.
 *
 * @param d Pointer to a generic descriptor structure.
 * @return Pointer to an mpeg_muxcode_descriptor structure, or NULL on error.
 */
static inline struct mpeg_muxcode_descriptor*
	mpeg_muxcode_descriptor_codec(struct descriptor* d)
{
	return (struct mpeg_muxcode_descriptor*) d;
}

/**
 * Retrieve pointer to entries field of an mpeg_muxcode_descriptor structure.
 *
 * @param d Generic descriptor structure.
 * @return Pointer to the entries field.
 */
static inline uint8_t *
	mpeg_muxcode_descriptor_entries(struct mpeg_muxcode_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct mpeg_muxcode_descriptor);
}

/**
 * Determine length of entries field of an mpeg_muxcode_descriptor structure.
 *
 * @param d Generic descriptor structure.
 * @return Number of bytes in the entries field.
 */
static inline int
	mpeg_muxcode_descriptor_entries_length(struct mpeg_muxcode_descriptor *d)
{
	return d->d.len;
}

#ifdef __cplusplus
}
#endif

#endif
