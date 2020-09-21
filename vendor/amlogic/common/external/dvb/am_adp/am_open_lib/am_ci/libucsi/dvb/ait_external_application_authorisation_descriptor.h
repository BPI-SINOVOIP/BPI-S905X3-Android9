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

#ifndef _UCSI_DVB_AIT_EXTERNAL_APPLICATION_AUTHORISATION_DESCRIPTOR
#define _UCSI_DVB_AIT_EXTERNAL_APPLICATION_AUTHORISATION_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * dvb_ait_external_application_authorisation_descriptor structure.
 */
struct dvb_ait_external_application_authorisation_descriptor {
	struct descriptor d;

	/* struct dvb_ait_external_application_authorisation auths[] */
} __ucsi_packed;

/**
 * An entry in the auths field of a dvb_ait_external_application_authorisation_descriptor.
 */
struct dvb_ait_external_application_authorisation {
	uint32_t organization_id;
	uint16_t application_id;
	uint8_t application_priority;
} __ucsi_packed;

/**
 * Process a dvb_ait_external_application_authorisation_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_ait_external_application_authorisation_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ait_external_application_authorisation_descriptor*
	dvb_ait_external_application_authorisation_descriptor_codec(struct descriptor* d)
{
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t pos = 0;
	uint32_t len = d->len;

	if (len % sizeof(struct dvb_ait_external_application_authorisation))
		return NULL;

	while(pos < len) {
		bswap32(buf + pos);
		bswap32(buf + pos + 4);
		pos += sizeof(struct dvb_ait_external_application_authorisation);
	}

	return (struct dvb_ait_external_application_authorisation_descriptor*) d;
}

/**
 * Iterator for entries in the auths field of a dvb_ait_external_application_authorisation_descriptor.
 *
 * @param d dvb_ait_external_application_authorisation_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_ait_external_application_authorisation.
 */
#define dvb_ait_external_application_authorisation_descriptor_auths_for_each(d, pos) \
	for ((pos) = dvb_ait_external_application_authorisation_descriptor_auths_first(d); \
	     (pos); \
	     (pos) = dvb_ait_external_application_authorisation_descriptor_auths_next(d, pos))









/******************************** PRIVATE CODE ********************************/
static inline struct dvb_ait_external_application_authorisation*
	dvb_ait_external_application_authorisation_descriptor_auths_first(struct dvb_ait_external_application_authorisation_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_ait_external_application_authorisation *)
		((uint8_t*) d + sizeof(struct dvb_ait_external_application_authorisation_descriptor));
}

static inline struct dvb_ait_external_application_authorisation*
	dvb_ait_external_application_authorisation_descriptor_auths_next(struct dvb_ait_external_application_authorisation_descriptor *d,
							    struct dvb_ait_external_application_authorisation *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos +
			sizeof(struct dvb_ait_external_application_authorisation);

	if (next >= end)
		return NULL;

	return (struct dvb_ait_external_application_authorisation *) next;
}

#ifdef __cplusplus
}
#endif

#endif
