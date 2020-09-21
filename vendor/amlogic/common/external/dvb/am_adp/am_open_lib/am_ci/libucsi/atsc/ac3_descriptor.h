/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_atsc@lidskialf.net)
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

#ifndef _UCSI_ATSC_AC3_DESCRIPTOR
#define _UCSI_ATSC_AC3_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

enum atsc_ac3_channels {
	ATSC_AC3_CHANNELS_1_PLUS_1	= 0x0,
	ATSC_AC3_CHANNELS_1_0		= 0x1,
	ATSC_AC3_CHANNELS_2_0		= 0x2,
	ATSC_AC3_CHANNELS_3_0		= 0x3,
	ATSC_AC3_CHANNELS_2_1		= 0x4,
	ATSC_AC3_CHANNELS_3_1 		= 0x5,
	ATSC_AC3_CHANNELS_2_2		= 0x6,
	ATSC_AC3_CHANNELS_3_2		= 0x7,
	ATSC_AC3_CHANNELS_1		= 0x8,
	ATSC_AC3_CHANNELS_LTEQ_2	= 0x9,
	ATSC_AC3_CHANNELS_LTEQ_3	= 0xa,
	ATSC_AC3_CHANNELS_LTEQ_4	= 0xb,
	ATSC_AC3_CHANNELS_LTEQ_5	= 0xc,
	ATSC_AC3_CHANNELS_LTEQ_6	= 0xd,
};

/**
 * atsc_ac3_descriptor structure.
 */
struct atsc_ac3_descriptor {
	struct descriptor d;

  EBIT2(uint8_t sample_rate_code        : 3; ,
	uint8_t bsid			: 5; );
  EBIT2(uint8_t bit_rate_code	        : 6; ,
	uint8_t surround_mode		: 2; );
  EBIT3(uint8_t bsmod		        : 3; ,
	uint8_t num_channels		: 4; ,
	uint8_t full_svc		: 1; );
	/* uint8_t additional_info[] */
} __ucsi_packed;

/**
 * Process an atsc_ac3_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return atsc_ac3_descriptor pointer, or NULL on error.
 */
static inline struct atsc_ac3_descriptor*
	atsc_ac3_descriptor_codec(struct descriptor* d)
{
	int pos = 0;

	if (d->len < (pos+4))
		return NULL;
	pos += 4;

	return (struct atsc_ac3_descriptor*) d;
}

/**
 * Retrieve pointer to additional_info field of a atsc_ac3_descriptor.
 *
 * @param d atsc_ac3_descriptor pointer.
 * @return Pointer to additional_info field.
 */
static inline uint8_t *atsc_ac3_descriptor_additional_info(struct atsc_ac3_descriptor *d)
{
	int pos = sizeof(struct atsc_ac3_descriptor);

	return ((uint8_t *) d) + pos;
}

/**
 * Determine length of additional_info field of a atsc_ac3_descriptor.
 *
 * @param d atsc_ac3_descriptor pointer.
 * @return Length of field in bytes.
 */
static inline int atsc_ac3_descriptor_additional_info_length(struct atsc_ac3_descriptor* d)
{
	return d->d.len - 3;
}

#ifdef __cplusplus
}
#endif

#endif
