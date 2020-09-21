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

#ifndef _UCSI_DVB_NVOD_REFERENCE_DESCRIPTOR
#define _UCSI_DVB_NVOD_REFERENCE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_nvod_reference_descriptor structure.
 */
struct dvb_nvod_reference_descriptor {
	struct descriptor d;

	/* struct dvb_nvod_reference references[] */
} __ucsi_packed;

/**
 * An entry in the references field of a dvb_nvod_reference_descriptor.
 */
struct dvb_nvod_reference {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
} __ucsi_packed;

/**
 * Process a dvb_nvod_reference_descriptor.
 *
 * @param d Pointer to a generic descriptor structure pointer.
 * @return dvb_nvod_reference_descriptor pointer, or NULL on error.
 */
static inline struct dvb_nvod_reference_descriptor*
	dvb_nvod_reference_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t len = d->len;

	if (len % sizeof(struct dvb_nvod_reference))
		return NULL;

	while(pos < len) {
		bswap16(buf+pos);
		bswap16(buf+pos+2);
		bswap16(buf+pos+4);
		pos += sizeof(struct dvb_nvod_reference);
	}

	return (struct dvb_nvod_reference_descriptor*) d;
}

/**
 * Iterator over the references field in a dvb_nvod_reference_descriptor.
 *
 * @param d dvb_nvod_reference_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_nvod_reference.
 */
#define dvb_nvod_reference_descriptor_references_for_each(d, pos) \
	for ((pos) = dvb_nvod_reference_descriptor_references_first(d); \
	     (pos); \
	     (pos) = dvb_nvod_reference_descriptor_references_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_nvod_reference*
	dvb_nvod_reference_descriptor_references_first(struct dvb_nvod_reference_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_nvod_reference *)
		((uint8_t*) d + sizeof(struct dvb_nvod_reference_descriptor));
}

static inline struct dvb_nvod_reference*
	dvb_nvod_reference_descriptor_references_next(struct dvb_nvod_reference_descriptor *d,
						      struct dvb_nvod_reference *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_nvod_reference);

	if (next >= end)
		return NULL;

	return (struct dvb_nvod_reference *) next;
}

#ifdef __cplusplus
}
#endif

#endif
