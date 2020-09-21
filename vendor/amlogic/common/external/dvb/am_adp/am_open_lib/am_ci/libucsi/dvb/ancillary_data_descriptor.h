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

#ifndef _UCSI_DVB_ANCILLARY_DATA_DESCRIPTOR
#define _UCSI_DVB_ANCILLARY_DATA_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_ancillary_data_descriptor structure.
 */
struct dvb_ancillary_data_descriptor {
	struct descriptor d;
  EBIT8(uint8_t reserved			: 1; ,
	uint8_t rds_via_udcp			: 1; ,
	uint8_t mpeg4_ancillary_data		: 1; ,
	uint8_t scale_factor_error_check	: 1; ,
	uint8_t dab_ancillary_data		: 1; ,
	uint8_t announcement_switching_data	: 1; ,
	uint8_t extended_ancillary_data		: 1; ,
	uint8_t dvd_video_ancillary_data	: 1; );
} __ucsi_packed;

/**
 * Process a dvb_ancillary_data_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_ancillary_data_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ancillary_data_descriptor*
	dvb_ancillary_data_descriptor_codec(struct descriptor* d)
{
	if (d->len != (sizeof(struct dvb_ancillary_data_descriptor) - 2))
		return NULL;

	return (struct dvb_ancillary_data_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
