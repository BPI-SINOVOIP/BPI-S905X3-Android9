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

#ifndef _UCSI_DVB_DATA_BROADCAST_ID_DESCRIPTOR
#define _UCSI_DVB_DATA_BROADCAST_ID_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for data_broadcast_id.
 */
enum {
	DVB_BROADCAST_ID_DATA_PIPE			= 0X0001,
	DVB_BROADCAST_ID_ASYNCHRONOUS_DATA_STREAM	= 0X0002,
	DVB_BROADCAST_ID_SYNCHRONOUS_DATA_STREAM	= 0X0003,
	DVB_BROADCAST_ID_SYNCHRONISED_DATA_STREAM	= 0X0004,
	DVB_BROADCAST_ID_MULTI_PROTOCOL_ENCAPSULATION	= 0X0005,
	DVB_BROADCAST_ID_DATA_CAROUSEL			= 0X0006,
	DVB_BROADCAST_ID_OBJECT_CAROUSEL		= 0X0007,
	DVB_BROADCAST_ID_DVB_ATM_STREAMS		= 0X0008,
	DVB_BROADCAST_ID_HIGHER_PROTOCOLS		= 0X0009,
	DVB_BROADCAST_ID_SOFTWARE_UPDATE		= 0x000A,
	DVB_BROADCAST_ID_IP_MAC_NOTIFICATION_TABLE	= 0x000B,
};

/**
 * dvb_data_broadcast_id_descriptor structure.
 */
struct dvb_data_broadcast_id_descriptor {
	struct descriptor d;

	uint16_t data_broadcast_id;
	/* uint8_t id_selector_byte[] */
} __ucsi_packed;

/**
 * id_selector_byte for 0x000b data_broadcast_id (IP/MAC Notification Table).
 */
struct dvb_id_selector_byte_000b {
	uint8_t platform_id_data_length;
	/* struct dvb_ip_mac_notification_info infos[] */
	/* uint8_t private_data[] */
} __ucsi_packed;

/**
 * Entries in the infos field of a dvb_id_selector_byte_0b.
 */
struct dvb_ip_mac_notification_info {
  EBIT2(uint32_t platform_id		: 24; ,
	uint8_t action_type		: 8;  );
  EBIT3(uint8_t reserved		: 2;  ,
	uint8_t INT_versioning_flag	: 1;  ,
	uint8_t INT_version		: 5;  );
} __ucsi_packed;

/**
 * Process a dvb_data_broadcast_id_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_data_broadcast_id_descriptor pointer, or NULL on error.
 */
static inline struct dvb_data_broadcast_id_descriptor*
	dvb_data_broadcast_id_descriptor_codec(struct descriptor* d)
{
	if (d->len < (sizeof(struct dvb_data_broadcast_id_descriptor) - 2))
		return NULL;

	bswap16((uint8_t*) d + 2);

	return (struct dvb_data_broadcast_id_descriptor*) d;
}

/**
 * Accessor for the selector_byte field of a dvb_data_broadcast_id_descriptor.
 *
 * @param d dvb_data_broadcast_id_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_data_broadcast_id_descriptor_id_selector_byte(struct dvb_data_broadcast_id_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_data_broadcast_id_descriptor);
}

/**
 * Determine the length of the selector_byte field of a dvb_data_broadcast_id_descriptor.
 *
 * @param d dvb_data_broadcast_id_descriptor pointer.
 * @return Length of the field in bytes.
 */
static inline int
	dvb_data_broadcast_id_descriptor_id_selector_byte_length(struct dvb_data_broadcast_id_descriptor *d)
{
	return d->d.len - 2;
}

/**
 * Accessor for a dvb_id_selector_byte_000b pointer.
 *
 * @param d dvb_data_broadcast_id_descriptor pointer.
 * @return Pointer to the data field.
 */
static inline struct dvb_id_selector_byte_000b *
	dvb_id_selector_byte_000b(struct dvb_data_broadcast_id_descriptor *d)
{
	if (d->data_broadcast_id != DVB_BROADCAST_ID_IP_MAC_NOTIFICATION_TABLE)
		return NULL;
	return (struct dvb_id_selector_byte_000b *) dvb_data_broadcast_id_descriptor_id_selector_byte(d);
}

/**
 * Iterator for the dvb_ip_mac_notification_info field of a dvb_id_selector_byte_000b.
 *
 * @param id_selector_byte dvb_id_selector_byte_000b pointer.
 * @param pos Variable containing a pointer to the current dvb_ip_mac_notification_info.
 */
#define dvb_id_selector_byte_000b_ip_mac_notification_info_for_each(id_selector_byte, pos) \
	for ((pos) = dvb_ip_mac_notification_info_first(id_selector_byte); \
	     (pos); \
	     (pos) = dvb_ip_mac_notification_info_next(id_selector_byte, pos))

/**
 * Length of the private_data field of a dvb_id_selector_byte_000b.
 *
 * @param d descriptor pointer.
 * @param i dvb_id_selector_byte_000b pointer.
 * @return Length of the field.
 */
static inline uint8_t
	dvb_id_selector_byte_000b_private_data_length(struct descriptor *d,
			    struct dvb_id_selector_byte_000b *i)
{
	return (uint8_t) (d->len -
	     sizeof(struct descriptor) -
	     i->platform_id_data_length -
	     sizeof(struct dvb_id_selector_byte_000b));
}

/**
 * Accessor for the private_data field of a dvb_id_selector_byte_000b.
 *
 * @param d descriptor pointer.
 * @param i dvb_id_selector_byte_000b pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_id_selector_byte_000b_private_data(struct descriptor *d,
			    struct dvb_id_selector_byte_000b *i)
{
	if (dvb_id_selector_byte_000b_private_data_length(d, i) <= 0)
		return NULL;

	return (uint8_t *) i + i->platform_id_data_length + sizeof(struct dvb_id_selector_byte_000b);
}







/******************************** PRIVATE CODE ********************************/
static inline struct dvb_ip_mac_notification_info *
	dvb_ip_mac_notification_info_first(struct dvb_id_selector_byte_000b *d)
{
	if (d->platform_id_data_length == 0)
		return NULL;

	bswap32((uint8_t *) d + sizeof(struct dvb_id_selector_byte_000b));

	return (struct dvb_ip_mac_notification_info *) ((uint8_t *) d + sizeof(struct dvb_id_selector_byte_000b));
}

static inline struct dvb_ip_mac_notification_info *
	dvb_ip_mac_notification_info_next(struct dvb_id_selector_byte_000b *d,
				    struct dvb_ip_mac_notification_info *pos)
{
	uint8_t *end = (uint8_t *) d + d->platform_id_data_length;
	uint8_t *next =	(uint8_t *) pos +
			sizeof(struct dvb_id_selector_byte_000b) +
			sizeof(struct dvb_ip_mac_notification_info);

	if (next >= end)
		return NULL;

	bswap32(next);

	return (struct dvb_ip_mac_notification_info *) next;
}

#ifdef __cplusplus
}
#endif

#endif

#include <libucsi/dvb/mhp_data_broadcast_id_descriptor.h>
