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

#ifndef _UCSI_DVB_S2_SATELLITE_DELIVERY_DESCRIPTOR
#define _UCSI_DVB_S2_SATELLITE_DELIVERY_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_s2_satellite_delivery_descriptor structure.
 */
struct dvb_s2_satellite_delivery_descriptor {
	struct descriptor d;

  EBIT4(uint8_t scrambling_sequence_selector	: 1; ,
	uint8_t multiple_input_stream		: 1; ,
	uint8_t backwards_compatability		: 1; ,
	uint8_t reserved			: 5; );
	/* uint32_t scrambling_sequence_index if scrambling_sequence_selector = 1 */
	/* uint8_t input_stream_id if multiple_input_stream = 1 */
} __ucsi_packed;

/**
 * Process a dvb_s2_satellite_delivery_descriptor.
 *
 * @param d Pointer to a generic descriptor structure.
 * @return dvb_s2_satellite_delivery_descriptor pointer, or NULL on error.
 */
static inline struct dvb_s2_satellite_delivery_descriptor*
	dvb_s2_satellite_delivery_descriptor_codec(struct descriptor* d)
{
	struct dvb_s2_satellite_delivery_descriptor *s2 =
			(struct dvb_s2_satellite_delivery_descriptor*) d;

	if (d->len < (sizeof(struct dvb_s2_satellite_delivery_descriptor) - 2))
		return NULL;

	int len = sizeof(struct dvb_s2_satellite_delivery_descriptor);
	if (s2->scrambling_sequence_selector) {
		len += 3;
	}
	if (s2->multiple_input_stream) {
		len += 1;
	}

	if (d->len < len)
		return NULL;

	return s2;
}

/**
 * Accessor for the scrambling_sequence_index field of a dvb_s2_satellite_delivery_descriptor.
 *
 * @param s2 dvb_s2_satellite_delivery_descriptor pointer.
 * @return The scrambling_sequence_index.
 */
static inline uint32_t dvb_s2_satellite_delivery_descriptor_scrambling_sequence_index(struct dvb_s2_satellite_delivery_descriptor *s2)
{
	uint8_t *tmp = (uint8_t*) s2;

	if (s2->scrambling_sequence_selector) {
		return ((tmp[4] & 0x03) << 16) | (tmp[5] << 8) | tmp[6];
	}
	return 0;
}

/**
 * Accessor for the input_stream_id field of a dvb_s2_satellite_delivery_descriptor.
 *
 * @param s2 dvb_s2_satellite_delivery_descriptor pointer.
 * @return The input_stream_id.
 */
static inline uint8_t dvb_s2_satellite_delivery_descriptor_input_stream_id(struct dvb_s2_satellite_delivery_descriptor *s2)
{
	uint8_t *tmp = (uint8_t*) s2;

	if (!s2->multiple_input_stream)
		return 0;

	int off = 3;
	if (s2->scrambling_sequence_selector) {
		off += 3;
	}
	return tmp[off];
}

#ifdef __cplusplus
}
#endif

#endif
