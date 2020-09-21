/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 * Copyright (C) 2006 Stephane Este-Gracias (sestegra@free.fr)
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

#ifndef _UCSI_DVB_IP_MAC_STREAM_LOCATION_DESCRIPTOR
#define _UCSI_DVB_IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/types.h>

/**
 * dvb_ip_mac_stream_location_descriptor structure.
 */
struct dvb_ip_mac_stream_location_descriptor {
	struct descriptor d;

	uint16_t network_id;
	uint16_t original_network_id;
	uint16_t transport_stream_id;
	uint16_t service_id;
	uint8_t component_tag;
} __ucsi_packed;

/**
 * Process a dvb_ip_mac_stream_location_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_ip_mac_stream_location_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ip_mac_stream_location_descriptor*
	dvb_ip_mac_stream_location_descriptor_codec(struct descriptor* d)
{
	uint8_t* buf = (uint8_t*) d + 2;

	if (d->len != (sizeof(struct dvb_ip_mac_stream_location_descriptor) - 2))
		return NULL;

	bswap16(buf);
	bswap16(buf+2);
	bswap16(buf+4);
	bswap16(buf+6);

	return (struct dvb_ip_mac_stream_location_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
