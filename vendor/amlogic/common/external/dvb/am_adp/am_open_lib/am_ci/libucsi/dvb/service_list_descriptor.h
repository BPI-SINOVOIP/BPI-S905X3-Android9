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

#ifndef _UCSI_DVB_SERVICE_LIST_DESCRIPTOR
#define _UCSI_DVB_SERVICE_LIST_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_service_list_descriptor structure.
 */
struct dvb_service_list_descriptor {
	struct descriptor d;

	/* struct dvb_service_list_service services[] */
} __ucsi_packed;

/**
 * An entry in the services field of a dvb_service_list_descriptor.
 */
struct dvb_service_list_service {
	uint16_t service_id;
	uint8_t service_type;
} __ucsi_packed;

/**
 * Process a dvb_service_list_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_service_list_descriptor pointer, or NULL on error.
 */
static inline struct dvb_service_list_descriptor*
	dvb_service_list_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint32_t len = d->len;
	uint8_t *p = (uint8_t*) d + 2;

	if (len % sizeof(struct dvb_service_list_service))
		return NULL;

	while(pos < len) {
		bswap16(p+pos);
		pos += sizeof(struct dvb_service_list_service);
	}

	return (struct dvb_service_list_descriptor*) d;
}

/**
 * Iterator for services field in a dvb_service_list_descriptor.
 *
 * @param d dvb_service_list_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_service_list_service.
 */
#define dvb_service_list_descriptor_services_for_each(d, pos) \
	for ((pos) = dvb_service_list_descriptor_services_first(d); \
	     (pos); \
	     (pos) = dvb_service_list_descriptor_services_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_service_list_service*
	dvb_service_list_descriptor_services_first(struct dvb_service_list_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_service_list_service *)
		((uint8_t*) d + sizeof(struct dvb_service_list_descriptor));
}

static inline struct dvb_service_list_service*
	dvb_service_list_descriptor_services_next(struct dvb_service_list_descriptor *d,
						  struct dvb_service_list_service *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_service_list_service);

	if (next >= end)
		return NULL;

	return (struct dvb_service_list_service *) next;
}

#ifdef __cplusplus
}
#endif

#endif
