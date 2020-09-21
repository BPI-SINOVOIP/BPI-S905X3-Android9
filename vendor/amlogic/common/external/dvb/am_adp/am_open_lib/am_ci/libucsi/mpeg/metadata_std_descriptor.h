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

#ifndef _UCSI_MPEG_METADATA_STD_DESCRIPTOR
#define _UCSI_MPEG_METADATA_STD_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * mpeg_metadata_std_descriptor structure.
 */
struct mpeg_metadata_std_descriptor {
	struct descriptor d;

  EBIT2(uint32_t reserved_1			: 2; ,
	uint32_t metadata_input_leak_rate	:22; );
  EBIT2(uint32_t reserved_2			: 2; ,
	uint32_t metadata_buffer_size		:22; );
  EBIT2(uint32_t reserved_3			: 2; ,
	uint32_t metadata_output_leak_rate	:22; );
} __ucsi_packed;

/**
 * Process an mpeg_metadata_std_descriptor.
 *
 * @param d Pointer to the generic descriptor structure.
 * @return Pointer to the mpeg_metadata_std_descriptor, or NULL on error.
 */
static inline struct mpeg_metadata_std_descriptor*
	mpeg_metadata_std_descriptor_codec(struct descriptor* d)
{
	uint8_t *buf = (uint8_t*) d;

	if (d->len != (sizeof(struct mpeg_metadata_std_descriptor) - 2))
		return NULL;

	bswap24(buf + 2);
	bswap24(buf + 5);
	bswap24(buf + 8);

	return (struct mpeg_metadata_std_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
