/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 * Copyright (C) 2008 Patrick Boettcher (pb@linuxtv.org)
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

#ifndef _UCSI_DVB_TIME_SLICE_FEC_IDENTIFIER_DESCRIPTOR
#define _UCSI_DVB_TIME_SLICE_FEC_IDENTIFIER_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/types.h>

/*
 * dvb_time_slice_fec_identifier_descriptor structure.
 */
struct dvb_time_slice_fec_identifier_descriptor {
	struct descriptor d;

  EBIT4(uint8_t time_slicing		:1;  ,
	uint8_t mpe_fec			:2;  ,
	uint8_t reserved		:2;  ,
	uint8_t frame_size		:3;  );

	uint8_t max_burst_duration;

  EBIT2(uint8_t max_average_rate	:4;  ,
	uint8_t time_slice_fec_id	:4;  );
	/* id_selector_bytes[] */
};

static inline struct dvb_time_slice_fec_identifier_descriptor *
	dvb_time_slice_fec_identifier_descriptor_codec(struct descriptor* d)
{
	if (d->len < 3)
		return NULL;
	return (struct dvb_time_slice_fec_identifier_descriptor *) d;
}

static inline uint8_t dvb_time_slice_fec_identifier_selector_byte_length(struct dvb_time_slice_fec_identifier_descriptor *d)
{
	return d->d.len - 3;
}

static inline uint8_t * dvb_time_slice_fec_identifier_selector_bytes(struct dvb_time_slice_fec_identifier_descriptor *d)
{
	if (d->d.len < 3)
		return NULL;
	else
		return ((uint8_t *) d) + 2 + 3;
}

static inline uint16_t dvb_time_slice_fec_identifier_max_burst_duration_msec(struct dvb_time_slice_fec_identifier_descriptor *d)
{
	return (d->max_burst_duration + 1) * 20;
}

static inline uint16_t dvb_time_slice_fec_identifier_frame_size_kbits(struct dvb_time_slice_fec_identifier_descriptor *d)
{
	if (d->frame_size > 3)
		return 0;
	return (d->frame_size+1) * 512;
}

static inline uint16_t dvb_time_slice_fec_identifier_frame_size_rows(struct dvb_time_slice_fec_identifier_descriptor *d)
{
	return dvb_time_slice_fec_identifier_frame_size_kbits(d) / 2;
}

#ifdef __cplusplus
}
#endif

#endif
