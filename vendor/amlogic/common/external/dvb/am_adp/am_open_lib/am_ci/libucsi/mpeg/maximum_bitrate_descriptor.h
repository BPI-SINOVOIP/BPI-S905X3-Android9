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

#ifndef _UCSI_MPEG_MAXIMUM_BITRATE_DESCRIPTOR
#define _UCSI_MPEG_MAXIMUM_BITRATE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * mpeg_maximum_bitrate_descriptor structure.
 */
struct mpeg_maximum_bitrate_descriptor {
	struct descriptor d;

  EBIT2(uint32_t reserved		: 2;  ,
	uint32_t maximum_bitrate	: 22; );
} __ucsi_packed;

/**
 * Process an mpeg_maximum_bitrate_descriptor.
 *
 * @param d Pointer to generic descriptor structure.
 * @return Pointer to mpeg_maximum_bitrate_descriptor, or NULL on error.
 */
static inline struct mpeg_maximum_bitrate_descriptor*
	mpeg_maximum_bitrate_descriptor_codec(struct descriptor* d)
{
	if (d->len != (sizeof(struct mpeg_maximum_bitrate_descriptor) - 2))
		return NULL;

	bswap24((uint8_t*) d + 2);

	return (struct mpeg_maximum_bitrate_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
