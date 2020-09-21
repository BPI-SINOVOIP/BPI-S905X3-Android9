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

#ifndef _UCSI_DVB_TARGET_IPV6_SOURCE_SLASH_DESCRIPTOR
#define _UCSI_DVB_TARGET_IPV6_SOURCE_SLASH_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/types.h>

/**
 * dvb_target_ipv6_source_slash_descriptor structure.
 */
struct dvb_target_ipv6_source_slash_descriptor {
	struct descriptor d;

	/* struct dvb_ipv6_source_slash ipv6_source_slash[] */
} __ucsi_packed;

/**
 * An entry in the ipv6_source_slash field of a dvb_target_ipv6_source_slash_descriptor.
 */
struct dvb_ipv6_source_slash {
	uint8_t ipv6_source_addr[16];
	uint8_t ipv6_source_slash;
	uint8_t ipv6_dest_addr[16];
	uint8_t ipv6_dest_slash;
} __ucsi_packed;

/**
 * Process a dvb_target_ipv6_source_slash_descriptor.
 *
 * @param d Generic descriptor structure pointer.
 * @return dvb_target_ipv6_source_slash_descriptor pointer, or NULL on error.
 */
static inline struct dvb_target_ipv6_source_slash_descriptor*
	dvb_target_ipv6_source_slash_descriptor_codec(struct descriptor* d)
{
	uint32_t len = d->len;

	if (len % sizeof(struct dvb_ipv6_source_slash))
		return NULL;

	return (struct dvb_target_ipv6_source_slash_descriptor*) d;
}

/**
 * Iterator for entries in the ipv6_source_slash field of a dvb_target_ipv6_source_slash_descriptor.
 *
 * @param d dvb_target_ipv6_source_slash_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_ipv6_source_slash.
 */
#define dvb_target_ipv6_source_slash_descriptor_ipv6_source_slash_for_each(d, pos) \
	for ((pos) = dvb_target_ipv6_source_slash_descriptor_ipv6_source_slash_first(d); \
	     (pos); \
	     (pos) = dvb_target_ipv6_source_slash_descriptor_ipv6_source_slash_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_ipv6_source_slash*
	dvb_target_ipv6_source_slash_descriptor_ipv6_source_slash_first(struct dvb_target_ipv6_source_slash_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_ipv6_source_slash *)
		((uint8_t*) d + sizeof(struct dvb_target_ipv6_source_slash_descriptor));
}

static inline struct dvb_ipv6_source_slash*
	dvb_target_ipv6_source_slash_descriptor_ipv6_source_slash_next(struct dvb_target_ipv6_source_slash_descriptor *d,
						    struct dvb_ipv6_source_slash *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_ipv6_source_slash);

	if (next >= end)
		return NULL;

	return (struct dvb_ipv6_source_slash *) next;
}

#ifdef __cplusplus
}
#endif

#endif
