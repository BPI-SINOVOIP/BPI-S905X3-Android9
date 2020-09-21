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

#ifndef _UCSI_DVB_RNT_RAR_OVER_IP_DESCRIPTOR
#define _UCSI_DVB_RNT_RAR_OVER_IP_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_rnt_rar_over_ip_descriptor structure.
 */
struct dvb_rnt_rar_over_ip_descriptor {
	struct descriptor d;

	dvbdate_t first_valid_date;
	dvbdate_t last_valid_date;
  EBIT3(uint8_t weighting		: 6; ,
	uint8_t complete_flag		: 1; ,
	uint8_t reserved		: 1; );
	uint8_t url_length;
	/* uint8_t url[] */
} __ucsi_packed;

/**
 * Process a dvb_rnt_rar_over_ip_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_rnt_rar_over_ip_descriptor pointer, or NULL on error.
 */
static inline struct dvb_rnt_rar_over_ip_descriptor*
	dvb_rnt_rar_over_ip_descriptor_codec(struct descriptor* d)
{
	uint8_t *buf = (uint8_t*) d;
	uint32_t len = d->len + 2;
	struct dvb_rnt_rar_over_ip_descriptor *ret =
		(struct dvb_rnt_rar_over_ip_descriptor *) buf;

	if (len < sizeof(struct dvb_rnt_rar_over_ip_descriptor))
		return NULL;
	if (len < (sizeof(struct dvb_rnt_rar_over_ip_descriptor) + buf[13]))
		return NULL;

	return ret;
}

/**
 * Accessor for the url field of a dvb_rnt_rar_over_ip_descriptor.
 *
 * @param d dvb_rnt_rar_over_ip_descriptor pointer.
 * @return Pointer.
 */
static inline uint8_t*
	dvb_rnt_rar_over_ip_descriptor_url(struct dvb_rnt_rar_over_ip_descriptor *d)
{
	return (uint8_t*)
		((uint8_t*) d + sizeof(struct dvb_rnt_rar_over_ip_descriptor));
}

#ifdef __cplusplus
}
#endif

#endif
