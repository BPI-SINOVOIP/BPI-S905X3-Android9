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

#ifndef _UCSI_ATSC_TIME_SHIFTED_SERVICE_DESCRIPTOR
#define _UCSI_ATSC_TIME_SHIFTED_SERVICE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * atsc_time_shifted_service_descriptor structure.
 */
struct atsc_time_shifted_service_descriptor {
	struct descriptor d;

  EBIT2(uint8_t reserved		: 3; ,
	uint8_t number_of_services	: 5; );
	/* struct atsc_time_shifted_service services[] */
} __ucsi_packed;

/**
 * An entry in the services field of an atsc_time_shifted_service_descriptor.
 */
struct atsc_time_shifted_service {
  EBIT2(uint16_t reserved 		: 6; ,
	uint16_t time_shift		:10; );
  EBIT3(uint32_t reserved2 		: 4; ,
	uint32_t major_channel_number	:10; ,
	uint32_t minor_channel_number	:10; );
} __ucsi_packed;

/**
 * Process an atsc_time_shifted_service_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return atsc_time_shifted_service_descriptor pointer, or NULL on error.
 */
static inline struct atsc_time_shifted_service_descriptor*
	atsc_time_shifted_service_descriptor_codec(struct descriptor* d)
{
	struct atsc_time_shifted_service_descriptor *ret =
		(struct atsc_time_shifted_service_descriptor *) d;
	uint8_t *buf = (uint8_t*) d + 2;
	int pos = 0;
	int idx;

	if (d->len < 1)
		return NULL;
	pos++;

	for(idx = 0; idx < ret->number_of_services; idx++) {
		if (d->len < (pos + sizeof(struct atsc_time_shifted_service)))
			return NULL;

		bswap16(buf+pos);
		bswap24(buf+pos+2);

		pos += sizeof(struct atsc_time_shifted_service);
	}

	return (struct atsc_time_shifted_service_descriptor*) d;
}

/**
 * Iterator for services field of a atsc_time_shifted_service_descriptor.
 *
 * @param d atsc_time_shifted_service_descriptor pointer.
 * @param pos Variable holding a pointer to the current atsc_service_location_element.
 * @param idx Integer used to count which service we are in.
 */
#define atsc_time_shifted_service_descriptor_services_for_each(d, pos, idx) \
	for ((pos) = atsc_time_shifted_service_descriptor_services_first(d), idx=0; \
	     (pos); \
	     (pos) = atsc_time_shifted_service_descriptor_services_next(d, pos, ++idx))










/******************************** PRIVATE CODE ********************************/
static inline struct atsc_time_shifted_service*
	atsc_time_shifted_service_descriptor_services_first(struct atsc_time_shifted_service_descriptor *d)
{
	if (d->number_of_services == 0)
		return NULL;

	return (struct atsc_time_shifted_service *)
		((uint8_t*) d + sizeof(struct atsc_time_shifted_service_descriptor));
}

static inline struct atsc_time_shifted_service*
	atsc_time_shifted_service_descriptor_services_next(struct atsc_time_shifted_service_descriptor *d,
							   struct atsc_time_shifted_service *pos,
							   int idx)
{
	uint8_t *next =	(uint8_t *) pos + sizeof(struct atsc_time_shifted_service);

	if (idx >= d->number_of_services)
		return NULL;
	return (struct atsc_time_shifted_service *) next;
}

#ifdef __cplusplus
}
#endif

#endif
