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

#ifndef _UCSI_DVB_MHP_DATA_BROADCAST_ID_DESCRIPTOR
#define _UCSI_DVB_MHP_DATA_BROADCAST_ID_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _UCSI_DVB_DATA_BROADCAST_ID_DESCRIPTOR
#error Must include dvb/data_broadcast_id_descriptor.h first
#endif

/**
 * Broadcast IDs for MHP.
 */
enum {
	DVB_BROADCAST_ID_MHP_OBJECT_CAROUSEL = 0x00f0,
	DVB_BROADCAST_ID_MHP_MPE = 0x00f1,
};

/**
 * dvb_mhp_data_broadcast_id_descriptor structure.
 */
struct dvb_mhp_data_broadcast_id_descriptor {
	struct dvb_data_broadcast_id_descriptor d;
	/* uint16_t application_type[] */
} __ucsi_packed;

/**
 * Process a dvb_mhp_data_broadcast_id_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_mhp_data_broadcast_id_descriptor pointer, or NULL on error.
 */
static inline struct dvb_mhp_data_broadcast_id_descriptor*
	dvb_mhp_data_broadcast_id_descriptor_codec(struct dvb_data_broadcast_id_descriptor* d)
{
	uint8_t * buf;
	int len;
	int pos = 0;
	struct dvb_mhp_data_broadcast_id_descriptor *res =
		(struct dvb_mhp_data_broadcast_id_descriptor *) d;

	if ((res->d.data_broadcast_id < 0xf0) || (res->d.data_broadcast_id > 0xfe))
		return NULL;

	buf = dvb_data_broadcast_id_descriptor_id_selector_byte(d);
	len = dvb_data_broadcast_id_descriptor_id_selector_byte_length(d);

	if (len % 2)
		return NULL;

	while(pos < len) {
		bswap16(buf+pos);
		pos+=2;
	}

	return res;
}

/**
 * Accessor for the application_type field of a dvb_mhp_data_broadcast_id_descriptor.
 *
 * @param d dvb_mhp_data_broadcast_id_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint16_t *
	dvb_mhp_data_broadcast_id_descriptor_id_application_type(struct dvb_mhp_data_broadcast_id_descriptor *d)
{
	return (uint16_t *) dvb_data_broadcast_id_descriptor_id_selector_byte((struct dvb_data_broadcast_id_descriptor*) d);
}

/**
 * Determine the number of entries in the application_type field of a dvb_mhp_data_broadcast_id_descriptor.
 *
 * @param d dvb_data_broadcast_id_descriptor pointer.
 * @return Length of the field in bytes.
 */
static inline int
	dvb_mhp_data_broadcast_id_descriptor_id_application_type_count(struct dvb_mhp_data_broadcast_id_descriptor *d)
{
	return dvb_data_broadcast_id_descriptor_id_selector_byte_length((struct dvb_data_broadcast_id_descriptor*) d) >> 1;
}

#ifdef __cplusplus
}
#endif

#endif
