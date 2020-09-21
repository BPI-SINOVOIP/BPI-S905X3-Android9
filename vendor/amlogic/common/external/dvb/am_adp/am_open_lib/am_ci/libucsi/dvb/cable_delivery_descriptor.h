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

#ifndef _UCSI_DVB_CABLE_DELIVERY_DESCRIPTOR
#define _UCSI_DVB_CABLE_DELIVERY_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_cable_delivery_descriptor structure.
 */
struct dvb_cable_delivery_descriptor {
	struct descriptor d;

	uint32_t frequency;			// BCD, units 100Hz
  EBIT2(uint16_t reserved	: 12; ,
	uint16_t fec_outer	: 4;  );
	uint8_t modulation;
  EBIT2(uint32_t symbol_rate	: 28; ,		// BCD, units 100Hz
	uint32_t fec_inner	: 4;  );
} __ucsi_packed;

/**
 * Process a dvb_cable_delivery_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_cable_delivery_descriptor pointer, or NULL on error.
 */
static inline struct dvb_cable_delivery_descriptor*
	dvb_cable_delivery_descriptor_codec(struct descriptor* d)
{
	if (d->len != (sizeof(struct dvb_cable_delivery_descriptor) - 2))
		return NULL;

	bswap32((uint8_t*) d + 2);
	bswap16((uint8_t*) d + 6);
	bswap32((uint8_t*) d + 9);

	return (struct dvb_cable_delivery_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
