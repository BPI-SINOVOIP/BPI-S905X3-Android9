/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 * Copyright (C) 2008 Patrick Boettcher (pb@linuxtv.org)
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

#ifndef _UCSI_MPEG_DATAGRAM_SECTION_H
#define _UCSI_MPEG_DATAGRAM_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * datagram_section structure.
 */
struct datagram_section {
	struct section head;

	uint8_t MAC_address_6;
	uint8_t MAC_address_5;
 EBIT5(uint8_t reserved                   : 2; ,
	uint8_t payload_scrambling_control : 2; ,
	uint8_t address_scrambling_control : 2; ,
	uint8_t LLC_SNAP_flag              : 1; ,
	uint8_t current_next_indicator     : 1; );
	uint8_t section_number;
	uint8_t last_section_number;
	uint8_t MAC_address_4;
	uint8_t MAC_address_3;
	uint8_t MAC_address_2;
	uint8_t MAC_address_1;

	/* LLC_SNAP or IP-data */
	/* if last section stuffing */
	/* CRC */
} __ucsi_packed;

/**
 */
static inline struct datagram_section *datagram_section_codec(struct section *section)
{
	/* something to do here ? */
	return (struct datagram_section *) section;
}

static inline uint8_t *datagram_section_ip_data(struct datagram_section *d)
{
	return (uint8_t *) d + sizeof(struct section) + 2 + 1 + 1 + 1 + 4;
}

static inline size_t datagram_section_ip_data_length(struct datagram_section *d)
{
	return section_length(&d->head) - (sizeof(struct section) + 2 + 1 + 1 + 1 + 4) - CRC_SIZE;
}


#ifdef __cplusplus
}
#endif

#endif
