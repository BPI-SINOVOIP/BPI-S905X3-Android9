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

#ifndef _UCSI_DVB_PARTIAL_TRANSPORT_STREAM_DESCRIPTOR
#define _UCSI_DVB_PARTIAL_TRANSPORT_STREAM_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_partial_transport_stream_descriptor structure.
 */
struct dvb_partial_transport_stream_descriptor {
	struct descriptor d;

  EBIT6(uint64_t reserved				: 2;  ,
	uint64_t peak_rate				:22;  ,
	uint64_t reserved_2				: 2;  ,
	uint64_t minimum_overall_smoothing_rate		:22;  ,
	uint64_t reserved_3				: 2;  ,
	uint64_t maximum_overall_smoothing_rate		:14;  );
} __ucsi_packed;

/**
 * Process a dvb_partial_transport_stream_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_partial_transport_stream_descriptor pointer, or NULL on error.
 */
static inline struct dvb_partial_transport_stream_descriptor*
	dvb_partial_transport_stream_descriptor_codec(struct descriptor* d)
{
	if (d->len != (sizeof(struct dvb_partial_transport_stream_descriptor) - 2))
		return NULL;

	bswap64((uint8_t*) d + 2);

	return (struct dvb_partial_transport_stream_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
