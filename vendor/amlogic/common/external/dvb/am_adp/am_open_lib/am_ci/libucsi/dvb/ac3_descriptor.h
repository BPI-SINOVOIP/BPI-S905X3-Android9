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

#ifndef _UCSI_DVB_AC3_DESCRIPTOR
#define _UCSI_DVB_AC3_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_ac3_descriptor structure.
 */
struct dvb_ac3_descriptor {
	struct descriptor d;

  EBIT5(uint8_t ac3_type_flag		: 1; ,
	uint8_t bsid_flag		: 1; ,
	uint8_t mainid_flag		: 1; ,
	uint8_t asvc_flag		: 1; ,
	uint8_t reserved		: 4; );
	/* uint8_t additional_info[] */
} __ucsi_packed;

/**
 * Process a dvb_ac3_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_ac3_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ac3_descriptor*
	dvb_ac3_descriptor_codec(struct descriptor* d)
{
	if (d->len < (sizeof(struct dvb_ac3_descriptor) - 2))
		return NULL;

	return (struct dvb_ac3_descriptor*) d;
}

/**
 * Retrieve pointer to additional_info field of a dvb_ac3_descriptor.
 *
 * @param d dvb_ac3_descriptor pointer.
 * @return Pointer to additional_info field.
 */
static inline uint8_t *dvb_ac3_descriptor_additional_info(struct dvb_ac3_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_ac3_descriptor);
}

/**
 * Determine length of additional_info field of a dvb_ac3_descriptor.
 *
 * @param d dvb_ac3_descriptor pointer.
 * @return Length of field in bytes.
 */
static inline int dvb_ac3_descriptor_additional_info_length(struct dvb_ac3_descriptor *d)
{
	return d->d.len - 1;
}

#ifdef __cplusplus
}
#endif

#endif
