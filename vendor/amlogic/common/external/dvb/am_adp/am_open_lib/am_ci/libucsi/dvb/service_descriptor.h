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

#ifndef _UCSI_DVB_SERVICE_DESCRIPTOR
#define _UCSI_DVB_SERVICE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for service_type.
 */
enum {
	DVB_SERVICE_TYPE_DIGITAL_TV		= 0x01,
	DVB_SERVICE_TYPE_DIGITAL_RADIO		= 0x02,
	DVB_SERVICE_TYPE_TELETEXT		= 0x03,
	DVB_SERVICE_TYPE_NVOD_REF		= 0x04,
	DVB_SERVICE_TYPE_NVOD_TIMESHIFT		= 0x05,
	DVB_SERVICE_TYPE_MOSAIC			= 0x06,
	DVB_SERVICE_TYPE_PAL			= 0x07,
	DVB_SERVICE_TYPE_SECAM			= 0x08,
	DVB_SERVICE_TYPE_D_D2_MAC		= 0x09,
	DVB_SERVICE_TYPE_FM_RADIO		= 0x0a,
	DVB_SERVICE_TYPE_NTSC			= 0x0b,
	DVB_SERVICE_TYPE_DATA_BCAST		= 0x0c,
	DVB_SERVICE_TYPE_EN50221		= 0x0d,
	DVB_SERVICE_TYPE_RCS_MAP		= 0x0e,
	DVB_SERVICE_TYPE_RCS_FLS		= 0x0f,
	DVB_SERVICE_TYPE_MHP			= 0x10,
	DVB_SERVICE_TYPE_MPEG2_HD_DIGITAL_TV	= 0x11,
	DVB_SERVICE_TYPE_ADVANCED_CODEC_SD_DIGITAL_TV = 0x16,
	DVB_SERVICE_TYPE_ADVANCED_CODEC_SD_NVOD_TIMESHIFT = 0x17,
	DVB_SERVICE_TYPE_ADVANCED_CODEC_SD_NVOD_REF = 0x18,
	DVB_SERVICE_TYPE_ADVANCED_CODEC_HD_DIGITAL_TV = 0x19,
	DVB_SERVICE_TYPE_ADVANCED_CODEC_HD_NVOD_TIMESHIFT = 0x1a,
	DVB_SERVICE_TYPE_ADVANCED_CODEC_HD_NVOD_REF = 0x1b,
};

/**
 * dvb_service_descriptor structure.
 */
struct dvb_service_descriptor {
	struct descriptor d;

	uint8_t service_type;
	uint8_t service_provider_name_length;
	/* uint8_t service_provider_name[] */
	/* struct dvb_service_descriptor_part2 part2 */
} __ucsi_packed;

/**
 * Second part of a dvb_service_descriptor following the variable length
 * service_provider_name field.
 */
struct dvb_service_descriptor_part2 {
	uint8_t service_name_length;
	/* uint8_t service_name[] */
} __ucsi_packed;

/**
 * Process a dvb_service_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_service_descriptor pointer, or NULL on error.
 */
static inline struct dvb_service_descriptor*
	dvb_service_descriptor_codec(struct descriptor* d)
{
	struct dvb_service_descriptor *p =
		(struct dvb_service_descriptor *) d;
	struct dvb_service_descriptor_part2 *p2;
	uint32_t pos = sizeof(struct dvb_service_descriptor) - 2;
	uint32_t len = d->len;

	if (pos > len)
		return NULL;

	pos += p->service_provider_name_length;

	if (pos > len)
		return NULL;

	p2 = (struct dvb_service_descriptor_part2*) ((uint8_t*) d + 2 + pos);

	pos += sizeof(struct dvb_service_descriptor_part2);

	if (pos > len)
		return NULL;

	pos += p2->service_name_length;

	if (pos != len)
		return NULL;

	return p;
}

/**
 * Accessor for the service_provider_name field of a dvb_service_descriptor.
 *
 * @param d dvb_service_descriptor pointer.
 * @return Pointer to the service_provider_name field.
 */
static inline uint8_t *
	dvb_service_descriptor_service_provider_name(struct dvb_service_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_service_descriptor);
}

/**
 * Accessor for the second part of a dvb_service_descriptor.
 *
 * @param d dvb_service_descriptor pointer.
 * @return dvb_service_descriptor_part2 pointer.
 */
static inline struct dvb_service_descriptor_part2 *
	dvb_service_descriptor_part2(struct dvb_service_descriptor *d)
{
	return (struct dvb_service_descriptor_part2 *)
		((uint8_t*) d + sizeof(struct dvb_service_descriptor) +
		 d->service_provider_name_length);
}

/**
 * Accessor for the service_name field of a dvb_service_descriptor_part2.
 *
 * @param d dvb_service_descriptor_part2 pointer.
 * @return Pointer to the service_name field.
 */
static inline uint8_t *
	dvb_service_descriptor_service_name(struct dvb_service_descriptor_part2 *d)
{
	return (uint8_t *) d + sizeof(struct dvb_service_descriptor_part2);
}

#ifdef __cplusplus
}
#endif

#endif
