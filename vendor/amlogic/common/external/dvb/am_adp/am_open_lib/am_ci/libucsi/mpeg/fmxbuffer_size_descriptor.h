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

#ifndef _UCSI_MPEG_FMXBUFFER_SIZE_DESCRIPTOR
#define _UCSI_MPEG_FMXBUFFER_SIZE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>


/**
 * mpeg_fmxbuffer_size_descriptor structure.
 */
struct mpeg_fmxbuffer_size_descriptor {
	struct descriptor d;

	/* uint8_t descriptors[] */
} __ucsi_packed;

/**
 * Process an mpeg_fmxbuffer_size_descriptor structure.
 *
 * @param d Pointer to a generic descriptor structure.
 * @return Pointer to an mpeg_fmxbuffer_size_descriptor structure, or NULL on error.
 */
static inline struct mpeg_fmxbuffer_size_descriptor*
	mpeg_fmxbuffer_size_descriptor_codec(struct descriptor* d)
{
	return (struct mpeg_fmxbuffer_size_descriptor*) d;
}

/**
 * Retrieve pointer to descriptors field of mpeg_fmxbuffer_size_descriptor structure.
 *
 * @param d mpeg_fmxbuffer_size_descriptor structure pointer.
 * @return Pointer to the descriptors.
 */
static inline uint8_t *
	mpeg_fmxbuffer_size_descriptor_descriptors(struct mpeg_fmxbuffer_size_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct mpeg_fmxbuffer_size_descriptor);
}

/**
 * Calculate the length of the descriptors field of an mpeg_fmxbuffer_size_descriptor structure.
 *
 * @param d mpeg_fmxbuffer_size_descriptor structure pointer.
 * @return Length of descriptors in bytes.
 */
static inline int
	mpeg_fmxbuffer_size_descriptor_descriptors_length(struct mpeg_fmxbuffer_size_descriptor *d)
{
	return d->d.len;
}

#ifdef __cplusplus
}
#endif

#endif
