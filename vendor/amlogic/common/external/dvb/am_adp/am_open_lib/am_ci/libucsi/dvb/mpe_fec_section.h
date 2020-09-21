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

#ifndef _UCSI_DVB_MPE_FEC_SECTION_H
#define _UCSI_DVB_MPE_FEC_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/mpeg/section.h>

/**
 * mpe_fec_section structure. TODO
 */
struct mpe_fec_section {
	struct section head;
};


/**
 * real_time_paramters
 * can also be found in datagram_section in MAC4-1-bytes */
struct real_time_parameters {
  EBIT4(uint32_t delta_t         : 12; ,
	uint32_t table_boundary  : 1;  ,
	uint32_t frame_boundary  : 1;  ,
	uint32_t address         : 18; )
};


static inline struct real_time_parameters * datagram_section_real_time_parameters_codec(struct datagram_section *d)
{
	struct real_time_parameters *rt = (struct real_time_parameters *) &d->MAC_address_4;
	uint8_t b[4];
	b[0] = d->MAC_address_4;
	b[1] = d->MAC_address_3;
	b[2] = d->MAC_address_2;
	b[3] = d->MAC_address_1;

	rt->delta_t = (b[0] << 4) | ((b[1] >> 4) & 0x0f);
	rt->table_boundary = (b[1] >> 3) & 0x1;
	rt->frame_boundary = (b[1] >> 2) & 0x1;
	rt->address        = ((b[1] & 0x3) << 16) | (b[2] << 8) | b[3];

	return rt;
}

#ifdef __cplusplus
}
#endif

#endif
