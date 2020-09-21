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

#ifndef _UCSI_DVB_AIT_APPLICATION_DESCRIPTOR
#define _UCSI_DVB_AIT_APPLICATION_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for the visibility field.
 */
enum {
	AVB_AIT_APPLICATION_VISIBILITY_HIDDEN		= 0x00,
	AVB_AIT_APPLICATION_VISIBILITY_APPSONLY		= 0x01,
	AVB_AIT_APPLICATION_VISIBILITY_VISIBLE		= 0x03,
};

/**
 * dvb_ait_application_descriptor structure.
 */
struct dvb_ait_application_descriptor {
	struct descriptor d;

	uint8_t application_profiles_length;
	/* struct dvb_ait_application_profile profiles [] */
	/* struct dvb_ait_application_descriptor_part2 part2 */
} __ucsi_packed;

/**
 * An entry in the profiles field of a dvb_ait_application_descriptor.
 */
struct dvb_ait_application_profile {
	uint16_t application_profile;
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t version_micro;
} __ucsi_packed;

/**
 * Second part of a dvb_ait_application_descriptor structure.
 */
struct dvb_ait_application_descriptor_part2 {
  EBIT3(uint8_t service_bound_flag		: 1; ,
	uint8_t visibility			: 2; ,
	uint8_t reserved			: 5; );
	uint8_t application_priority;
	/* uint8_t transport_protocol_label[] */
} __ucsi_packed;

/**
 * Process a dvb_ait_application_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_ait_application_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ait_application_descriptor*
	dvb_ait_application_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint32_t pos2 = 0;
	uint32_t len = d->len + 2;
	uint8_t* buf = (uint8_t*) d;
	struct dvb_ait_application_descriptor *ret =
		(struct dvb_ait_application_descriptor*) d;

	if (len < sizeof(struct dvb_ait_application_descriptor))
		return NULL;

	if (len < (sizeof(struct dvb_ait_application_descriptor) + ret->application_profiles_length))
		return NULL;

	if (ret->application_profiles_length % sizeof(struct dvb_ait_application_profile))
		return NULL;

	pos += sizeof(struct dvb_ait_application_descriptor);
	pos2 = 0;
	while(pos2 < ret->application_profiles_length) {
		bswap16(buf + pos + pos2);
		pos2 += sizeof(struct dvb_ait_application_descriptor);
	}
	pos += pos2;

	if (len < (pos + sizeof(struct dvb_ait_application_descriptor_part2)))
		return NULL;

	return ret;
}

/**
 * Iterator for the profiles field of a dvb_ait_application_descriptor.
 *
 * @param d dvb_ait_application_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_ait_application_profile.
 */
#define dvb_ait_application_descriptor_profiles_for_each(d, pos) \
	for ((pos) = dvb_ait_application_descriptor_profiles_first(d); \
	     (pos); \
	     (pos) = dvb_ait_application_descriptor_profiles_next(d, pos))

/**
 * Accessor for the part2 field of a dvb_ait_application_descriptor.
 *
 * @param d dvb_ait_application_descriptor pointer.
 * @return dvb_ait_application_descriptor_part2 pointer.
 */
static inline struct dvb_ait_application_descriptor_part2*
	dvb_ait_application_descriptor_part2(struct dvb_ait_application_descriptor* d)
{
	return (struct dvb_ait_application_descriptor_part2*)
		((uint8_t*) d +
		 sizeof(struct dvb_ait_application_descriptor) +
		 d->application_profiles_length);
}

/**
 * Accessor for the transport_protocol_label field of a dvb_ait_application_descriptor_part2.
 *
 * @param d dvb_ait_application_descriptor_part2 pointer.
 * @return Pointer to the field.
 */
static inline uint8_t*
	dvb_ait_application_descriptor_part2_transport_protocol_label(struct dvb_ait_application_descriptor_part2* d)
{
	return (uint8_t*) d +
			sizeof(struct dvb_ait_application_descriptor_part2);
}

/**
 * Calculate the number of bytes in the transport_protocol_label field of a dvb_ait_application_descriptor_part2.
 *
 * @param d dvb_ait_application_descriptor pointer.
 * @param part2 dvb_ait_application_descriptor_part2 pointer.
 * @return Number of bytes.
 */
static inline int
	dvb_ait_application_descriptor_part2_transport_protocol_label_length(struct dvb_ait_application_descriptor *d,
									     struct dvb_ait_application_descriptor_part2* part2)
{
	uint8_t *ptr = (uint8_t*) part2 + sizeof(struct dvb_ait_application_descriptor_part2);
	uint8_t *end = (uint8_t*) d + d->d.len + 2;

	return (int) (end - ptr);
}






/******************************** PRIVATE CODE ********************************/
static inline struct dvb_ait_application_profile*
	dvb_ait_application_descriptor_profiles_first(struct dvb_ait_application_descriptor *d)
{
	if (d->application_profiles_length == 0)
		return NULL;

	return (struct dvb_ait_application_profile *)
		((uint8_t*) d + sizeof(struct dvb_ait_application_descriptor));
}

static inline struct dvb_ait_application_profile*
	dvb_ait_application_descriptor_profiles_next(struct dvb_ait_application_descriptor *d,
						     struct dvb_ait_application_profile *pos)
{
	uint8_t *end = (uint8_t*) d +
			sizeof(struct dvb_ait_application_descriptor) +
			d->application_profiles_length;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_ait_application_profile);

	if (next >= end)
		return NULL;

	return (struct dvb_ait_application_profile *) next;
}

#ifdef __cplusplus
}
#endif

#endif
