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

#ifndef _UCSI_DVB_FREQUENCY_LIST_DESCRIPTOR
#define _UCSI_DVB_FREQUENCY_LIST_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for coding_type.
 */
enum {
	DVB_CODING_TYPE_SATELLITE		= 0x01,
	DVB_CODING_TYPE_CABLE			= 0x02,
	DVB_CODING_TYPE_TERRESTRIAL		= 0x03,
};

/**
 * dvb_frequency_list_descriptor structure.
 */
struct dvb_frequency_list_descriptor {
	struct descriptor d;

  EBIT2(uint8_t reserved	: 6; ,
	uint8_t coding_type	: 2; );
	/* uint32_t centre_frequencies [] */
} __ucsi_packed;

/**
 * Process a dvb_frequency_list_descriptor.
 *
 * @param d Pointer to a generic descriptor structure.
 * @return dvb_frequency_list_descriptor pointer, or NULL on error.
 */
static inline struct dvb_frequency_list_descriptor*
	dvb_frequency_list_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t len = d->len;

	pos += sizeof(struct dvb_frequency_list_descriptor) - 2;

	if ((len - pos) % 4)
		return NULL;

	while(pos < len) {
		bswap32(buf+pos);
		pos += 4;
	}

	return (struct dvb_frequency_list_descriptor*) d;
}

/**
 * Accessor for the centre_frequencies field of a dvb_frequency_list_descriptor.
 *
 * @param d dvb_frequency_list_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint32_t *
	dvb_frequency_list_descriptor_centre_frequencies(struct dvb_frequency_list_descriptor *d)
{
	return (uint32_t *) ((uint8_t *) d + sizeof(struct dvb_frequency_list_descriptor));
}

/**
 * Determine the number of entries in the centre_frequencies field of a dvb_frequency_list_descriptor.
 *
 * @param d dvb_frequency_list_descriptor pointer.
 * @return The number of entries.
 */
static inline int
	dvb_frequency_list_descriptor_centre_frequencies_count(struct dvb_frequency_list_descriptor *d)
{
	return (d->d.len - 1) >> 2;
}

#ifdef __cplusplus
}
#endif

#endif
