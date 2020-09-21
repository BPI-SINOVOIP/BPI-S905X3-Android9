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

#ifndef _UCSI_DVB_SHORT_SMOOTHING_BUFFER_DESCRIPTOR
#define _UCSI_DVB_SHORT_SMOOTHING_BUFFER_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_short_smoothing_buffer_descriptor structure.
 */
struct dvb_short_smoothing_buffer_descriptor {
	struct descriptor d;

  EBIT2(uint8_t sb_size		: 2; ,
	uint8_t sb_leak_rate	: 6; );
	/* uint8_t reserved [] */
} __ucsi_packed;

/**
 * Process a dvb_short_smoothing_buffer_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_short_smoothing_buffer_descriptor pointer, or NULL on error.
 */
static inline struct dvb_short_smoothing_buffer_descriptor*
	dvb_short_smoothing_buffer_descriptor_codec(struct descriptor* d)
{
	if (d->len < (sizeof(struct dvb_short_smoothing_buffer_descriptor) - 2))
		return NULL;

	return (struct dvb_short_smoothing_buffer_descriptor*) d;
}

/**
 * Accessor for reserved field in a dvb_short_smoothing_buffer_descriptor.
 *
 * @param d dvb_short_smoothing_buffer_descriptor pointer.
 * @return Pointer to reserved field.
 */
static inline uint8_t *
	dvb_short_smoothing_buffer_descriptor_reserved(struct dvb_short_smoothing_buffer_descriptor *d)
{
	return (uint8_t*) d + sizeof(struct dvb_short_smoothing_buffer_descriptor);
}

/**
 * Calculate length of reserved field in a dvb_short_smoothing_buffer_descriptor.
 *
 * @param d dvb_short_smoothing_buffer_descriptor pointer.
 * @return Length of the field in bytes.
 */
static inline int
	dvb_short_smoothing_buffer_descriptor_reserved_length(struct dvb_short_smoothing_buffer_descriptor *d)
{
	return d->d.len - 1;
}

#ifdef __cplusplus
}
#endif

#endif
