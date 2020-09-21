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

#ifndef _UCSI_MPEG_IOD_DESCRIPTOR
#define _UCSI_MPEG_IOD_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * mpeg_iod_descriptor structure.
 */
struct mpeg_iod_descriptor {
	struct descriptor d;

	uint8_t scope_of_iod_label;
	uint8_t iod_label;
	/* uint8_t iod[] */
} __ucsi_packed;

/**
 * Process an mpeg_iod_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return Pointer to an mpeg_iod_descriptor structure, or NULL on error.
 */
static inline struct mpeg_iod_descriptor*
	mpeg_iod_descriptor_codec(struct descriptor* d)
{
	if (d->len < (sizeof(struct mpeg_iod_descriptor) - 2))
		return NULL;

	return (struct mpeg_iod_descriptor*) d;
}

/**
 * Retrieve pointer to iod field of an mpeg_iod_descriptor structure.
 *
 * @param d Pointer to mpeg_iod_descriptor structure.
 * @return Pointer to the iod field.
 */
static inline uint8_t *
	mpeg_iod_descriptor_iod(struct mpeg_iod_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct mpeg_iod_descriptor);
}

/**
 * Calculate the length of the iod field of an mpeg_iod_descriptor structure.
 *
 * @param d Pointer to mpeg_iod_descriptor structure.
 * @return The number of bytes.
 */
static inline int
	mpeg_iod_descriptor_iod_length(struct mpeg_iod_descriptor *d)
{
	return d->d.len - 2;
}

#ifdef __cplusplus
}
#endif

#endif
