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

#ifndef _UCSI_DVB_SERVICE_MOVE_DESCRIPTOR
#define _UCSI_DVB_SERVICE_MOVE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_service_move_descriptor structure.
 */
struct dvb_service_move_descriptor {
	struct descriptor d;

	uint16_t new_original_network_id;
	uint16_t new_transport_stream_id;
	uint16_t new_service_id;
} __ucsi_packed;

/**
 * Process a dvb_service_move_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return Pointer to dvb_service_move_descriptor, or NULL on error.
 */
static inline struct dvb_service_move_descriptor*
	dvb_service_move_descriptor_codec(struct descriptor* d)
{
	if (d->len != (sizeof(struct dvb_service_move_descriptor) - 2))
		return NULL;

	bswap16((uint8_t*) d + 2);
	bswap16((uint8_t*) d + 4);
	bswap16((uint8_t*) d + 6);

	return (struct dvb_service_move_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
