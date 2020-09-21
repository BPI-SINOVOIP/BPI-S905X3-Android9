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

#ifndef _UCSI_DVB_TARGET_IP_ADDRESS_DESCRIPTOR
#define _UCSI_DVB_TARGET_IP_ADDRESS_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/types.h>

/**
 * dvb_target_ip_address_descriptor structure.
 */
struct dvb_target_ip_address_descriptor {
	struct descriptor d;

	uint8_t ipv4_addr_mask[4];
	/* struct dvb_ipv4_addr ipv4_addr[] */
} __ucsi_packed;

/**
 * An entry in the ipv4_addr field of a dvb_target_ip_address_descriptor.
 */
struct dvb_ipv4_addr {
	uint8_t ipv4_addr[4];
} __ucsi_packed;

/**
 * Process a dvb_target_ip_address_descriptor.
 *
 * @param d Generic descriptor structure pointer.
 * @return dvb_target_ip_address_descriptor pointer, or NULL on error.
 */
static inline struct dvb_target_ip_address_descriptor*
	dvb_target_ip_address_descriptor_codec(struct descriptor* d)
{
	uint32_t len = d->len - 4;

	if (len % sizeof(struct dvb_ipv4_addr))
		return NULL;

	return (struct dvb_target_ip_address_descriptor*) d;
}

/**
 * Iterator for entries in the ipv4_addr field of a dvb_target_ip_address_descriptor.
 *
 * @param d dvb_target_ip_address_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_ipv4_addr.
 */
#define dvb_target_ip_address_descriptor_ipv4_addr_for_each(d, pos) \
	for ((pos) = dvb_target_ip_address_descriptor_ipv4_addr_first(d); \
	     (pos); \
	     (pos) = dvb_target_ip_address_descriptor_ipv4_addr_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_ipv4_addr*
	dvb_target_ip_address_descriptor_ipv4_addr_first(struct dvb_target_ip_address_descriptor *d)
{
	if (d->d.len == 4)
		return NULL;

	return (struct dvb_ipv4_addr *)
		((uint8_t*) d + sizeof(struct dvb_target_ip_address_descriptor));
}

static inline struct dvb_ipv4_addr*
	dvb_target_ip_address_descriptor_ipv4_addr_next(struct dvb_target_ip_address_descriptor *d,
						    struct dvb_ipv4_addr *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len - 4;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_ipv4_addr);

	if (next >= end)
		return NULL;

	return (struct dvb_ipv4_addr *) next;
}

#ifdef __cplusplus
}
#endif

#endif
