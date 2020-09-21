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

#ifndef _UCSI_DVB_LINKAGE_DESCRIPTOR
#define _UCSI_DVB_LINKAGE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * Possible values for linkage_type.
 */
enum {
	DVB_LINKAGE_TYPE_INFORMATION			= 0x01,
	DVB_LINKAGE_TYPE_EPG				= 0x02,
	DVB_LINKAGE_TYPE_CA_REPLACEMENT			= 0x03,
	DVB_LINKAGE_TYPE_TS_WITH_BAT_NIT		= 0x04,
	DVB_LINKAGE_TYPE_SERVICE_REPLACMENT		= 0x05,
	DVB_LINKAGE_TYPE_DATA_BCAST			= 0x06,
	DVB_LINKAGE_TYPE_RCS_MAP			= 0x07,
	DVB_LINKAGE_TYPE_MOBILE_HANDOVER		= 0x08,
	DVB_LINKAGE_TYPE_SOFTWARE_UPDATE		= 0x09,
	DVB_LINKAGE_TYPE_TS_WITH_SSU_BAT_NIT		= 0x0a,
	DVB_LINKAGE_TYPE_IP_MAC_NOTIFICATION		= 0x0b,
	DVB_LINKAGE_TYPE_TS_WITH_INT_BAT_NIT		= 0x0c,
};

/**
 * Possible values for hand_over_type.
 */
enum {
	DVB_HAND_OVER_TYPE_IDENTICAL_NEIGHBOURING_COUNTRY	= 0x01,
	DVB_HAND_OVER_TYPE_LOCAL_VARIATION			= 0x02,
	DVB_HAND_OVER_TYPE_ASSOCIATED_SERVICE			= 0x03,
};

/**
 * Possible values for origin_type.
 */
enum {
	DVB_ORIGIN_TYPE_NIT				= 0x00,
	DVB_ORIGIN_TYPE_SDT				= 0x01,
};

/**
 * dvb_linkage_descriptor structure.
 */
struct dvb_linkage_descriptor {
	struct descriptor d;

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
	uint8_t linkage_type;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * Data for a linkage_type of 0x08.
 */
struct dvb_linkage_data_08 {
  EBIT3(uint8_t hand_over_type		: 4;  ,
	uint8_t reserved		: 3;  ,
	uint8_t origin_type		: 1;  );
	/* uint16_t network_id if hand_over_type == 1,2,3 */
	/* uint16_t initial_service_id if origin_type = 0 */
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * Data for an linkage_type of 0x0b (IP/MAC Notification Table).
 */
struct dvb_linkage_data_0b {
	uint8_t platform_id_data_length;
	/* struct platform_id ids[] */
} __ucsi_packed;

/**
 * Entries in the ids field of a dvb_linkage_data_0b.
 */
struct dvb_platform_id {
  EBIT2(uint32_t platform_id			: 24; ,
	uint8_t platform_name_loop_length	: 8;  );
	/* struct platform_name names[] */
} __ucsi_packed;

/**
 * Entries in the names field of a dvb_platform_id.
 */
struct dvb_platform_name {
	iso639lang_t language_code;
	uint8_t platform_name_length;
	/* uint8_t text[] */
} __ucsi_packed;

/**
 * Data for a linkage_type of 0x0c (IP/MAC Notification Table).
 */
struct dvb_linkage_data_0c {
	uint8_t table_type;
	/* uint16_t bouquet_id if table_type == 0x02 */
} __ucsi_packed;


/**
 * Process a dvb_linkage_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_linkage_descriptor pointer, or NULL on error.
 */
static inline struct dvb_linkage_descriptor*
	dvb_linkage_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t len = d->len;
	struct dvb_linkage_descriptor *p =
		(struct dvb_linkage_descriptor*) d;

	if (len < (sizeof(struct dvb_linkage_descriptor) - 2))
		return NULL;

	bswap16(buf);
	bswap16(buf+2);
	bswap16(buf+4);

	pos += sizeof(struct dvb_linkage_descriptor) - 2;

	if (p->linkage_type == 0x08) {
		struct dvb_linkage_data_08 *d08;

		if ((len - pos) < sizeof(struct dvb_linkage_data_08))
			return NULL;
		d08 = (struct dvb_linkage_data_08 *) (buf+pos);
		pos += sizeof(struct dvb_linkage_data_08);

		switch(d08->hand_over_type) {
		case 1:
		case 2:
		case 3:
			if ((len - pos) < 2)
				return NULL;
			bswap16(buf+pos);
			pos += 2;
			break;
		}
		if (d08->origin_type == 0) {
			if ((len - pos) < 2)
				return NULL;
			bswap16(buf+pos);
			pos+=2;
		}

	} else if (p->linkage_type == 0x0b) {
		uint32_t pos2=0;
		struct dvb_linkage_data_0b *l_0b = (struct dvb_linkage_data_0b *) (buf + pos);

		if ((len - pos) < sizeof(struct dvb_linkage_data_0b))
			return NULL;

		pos += sizeof(struct dvb_linkage_data_0b);
		if ((len - pos) < l_0b->platform_id_data_length)
			return NULL;

		while (pos2 < l_0b->platform_id_data_length) {
			bswap32(buf + pos + pos2);

			struct dvb_platform_id *p_id = (struct dvb_platform_id *) (buf + pos + pos2);
			if ((len - pos - pos2) < p_id->platform_name_loop_length)
				return NULL;

			pos2 += sizeof(struct dvb_platform_id) + p_id->platform_name_loop_length;
		}

		pos += pos2;
	} else if (p->linkage_type == 0x0c) {
		struct dvb_linkage_data_0c *l_0c = (struct dvb_linkage_data_0c *) (buf + pos);

		if ((len - pos) < sizeof(struct dvb_linkage_data_0c))
			return NULL;
		pos += sizeof(struct dvb_linkage_data_0c);

		if (l_0c->table_type == 0x02) {
			if ((len - pos) < 2)
				return NULL;
			bswap16(buf+pos);
		}
	}

	return (struct dvb_linkage_descriptor*) d;
}

/**
 * Accessor for the data field of a dvb_linkage_descriptor.
 *
 * @param d dvb_linkage_descriptor pointer.
 * @return Pointer to the data field.
 */
static inline uint8_t *
	dvb_linkage_descriptor_data(struct dvb_linkage_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_linkage_descriptor);
}

/**
 * Determine the length of the data field of a dvb_linkage_descriptor.
 *
 * @param d dvb_linkage_descriptor pointer.
 * @return Length of the field in bytes.
 */
static inline int
	dvb_linkage_descriptor_data_length(struct dvb_linkage_descriptor *d)
{
	return d->d.len - 7;
}

/**
 * Accessor for a dvb_linkage_data_08 pointer.
 *
 * @param d dvb_linkage_descriptor pointer.
 * @return Pointer to the data field.
 */
static inline struct dvb_linkage_data_08 *
	dvb_linkage_data_08(struct dvb_linkage_descriptor *d)
{
	if (d->linkage_type != 0x08)
		return NULL;
	return (struct dvb_linkage_data_08 *) dvb_linkage_descriptor_data(d);
}

/**
 * Accessor for the network_id field of a dvb_linkage_data_08.
 *
 * @param d dvb_linkage_descriptor pointer
 * @param d08 dvb_linkage_data_08 pointer.
 * @return network_id, or -1 if not present
 */
static inline int
	dvb_linkage_data_08_network_id(struct dvb_linkage_descriptor *d, struct dvb_linkage_data_08 *d08)
{
	if (d->linkage_type != 0x08)
		return -1;

	switch(d08->hand_over_type) {
	case 1:
	case 2:
	case 3:
		return *((uint16_t*) ((uint8_t*) d08 + sizeof(struct dvb_linkage_data_08)));
	}

	return -1;
}

/**
 * Accessor for the initial_service_id field of a dvb_linkage_data_08.
 *
 * @param d dvb_linkage_descriptor pointer
 * @param d08 dvb_linkage_data_08 pointer.
 * @return initial_service_id, or -1 if not present
 */
static inline int
	dvb_linkage_data_08_initial_service_id(struct dvb_linkage_descriptor *d, struct dvb_linkage_data_08 *d08)
{
	uint8_t *pos;

	if (d->linkage_type != 0x08)
		return -1;
	if (d08->origin_type != 0)
		return -1;

	pos = ((uint8_t*) d08) + sizeof(struct dvb_linkage_data_08);
	switch(d08->hand_over_type) {
	case 1:
	case 2:
	case 3:
		pos +=2;
		break;
	}

	return *((uint16_t*) pos);
}

/**
 * Accessor for the data field of a dvb_linkage_data_08.
 *
 * @param d dvb_linkage_descriptor pointer
 * @param d08 dvb_linkage_data_08 pointer.
 * @param length Pointer to int destination for data length.
 * @return Pointer to the data field, or NULL if invalid
 */
static inline uint8_t *
	dvb_linkage_data_08_data(struct dvb_linkage_descriptor *d, struct dvb_linkage_data_08 *d08, int *length)
{
	uint8_t *pos;
	int used = 0;

	if (d->linkage_type != 0x08) {
		*length = 0;
		return NULL;
	}

	pos = ((uint8_t*) d08) + sizeof(struct dvb_linkage_data_08);
	switch(d08->hand_over_type) {
	case 1:
	case 2:
	case 3:
		pos += 2;
		used += 2;
		break;
	}
	if (d08->origin_type == 0) {
		pos+=2;
		used+=2;
	}

	*length = dvb_linkage_descriptor_data_length(d) - (sizeof(struct dvb_linkage_data_08) + used);
	return pos;
}

/**
 * Accessor for a dvb_linkage_data_0b pointer.
 *
 * @param d dvb_linkage_descriptor pointer.
 * @return Pointer to the data field.
 */
static inline struct dvb_linkage_data_0b *
	dvb_linkage_data_0b(struct dvb_linkage_descriptor *d)
{
	if (d->linkage_type != 0x0b)
		return NULL;
	return (struct dvb_linkage_data_0b *) dvb_linkage_descriptor_data(d);
}

/**
 * Iterator for the platform_id field of a dvb_linkage_data_0b.
 *
 * @param linkage dvb_linkage_data_0b pointer.
 * @param pos Variable containing a pointer to the current dvb_platform_id.
 */
#define dvb_linkage_data_0b_platform_id_for_each(linkage, pos) \
	for ((pos) = dvb_platform_id_first(linkage); \
	     (pos); \
	     (pos) = dvb_platform_id_next(linkage, pos))

/**
 * Iterator for the platform_name field of a dvb_platform_id.
 *
 * @param platid dvb_platform_id pointer.
 * @param pos Variable containing a pointer to the current dvb_platform_name.
 */
#define dvb_platform_id_platform_name_for_each(platid, pos) \
	for ((pos) = dvb_platform_name_first(platid); \
	     (pos); \
	     (pos) = dvb_platform_name_next(platid, pos))

/**
 * Accessor for the text field of a dvb_platform_name.
 *
 * @param p dvb_platform_name pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_platform_name_text(struct dvb_platform_name *p)
{
	return (uint8_t *) p + sizeof(struct dvb_platform_name);
}

/**
 * Accessor for a dvb_linkage_data_0c pointer.
 *
 * @param d dvb_linkage_descriptor pointer.
 * @return Pointer to the data field.
 */
static inline struct dvb_linkage_data_0c *
		dvb_linkage_data_0c(struct dvb_linkage_descriptor *d)
{
	if (d->linkage_type != 0x0c)
		return NULL;
	return (struct dvb_linkage_data_0c *) dvb_linkage_descriptor_data(d);
}

/**
 * Accessor for the bouquet_id field of a dvb_linkage_data_0c if table_id == 0x02.
 *
 * @param l_0c dvb_linkage_data_0c pointer.
 * @return The bouquet field, or -1 on error.
 */
static inline int
	dvb_linkage_data_0c_bouquet_id(struct dvb_linkage_data_0c *l_0c)
{
	if (l_0c->table_type != 0x02)
		return -1;

	return *((uint16_t *) ((uint8_t*) l_0c + 1));
}







/******************************** PRIVATE CODE ********************************/
static inline struct dvb_platform_id *
	dvb_platform_id_first(struct dvb_linkage_data_0b *d)
{
	if (d->platform_id_data_length == 0)
		return NULL;

	return (struct dvb_platform_id *) ((uint8_t *) d + sizeof(struct dvb_linkage_data_0b));
}

static inline struct dvb_platform_id *
	dvb_platform_id_next(struct dvb_linkage_data_0b *d,
				    struct dvb_platform_id *pos)
{
	uint8_t *end = (uint8_t *) d + d->platform_id_data_length;
	uint8_t *next =	(uint8_t *) pos +
			sizeof(struct dvb_platform_id) +
			pos->platform_name_loop_length;

	if (next >= end)
		return NULL;

	return (struct dvb_platform_id *) next;
}

static inline struct dvb_platform_name *
	dvb_platform_name_first(struct dvb_platform_id *p)
{
	if (p->platform_name_loop_length == 0)
		return NULL;

	return (struct dvb_platform_name *) ((uint8_t *) p + sizeof(struct dvb_platform_id));
}

static inline struct dvb_platform_name *
	dvb_platform_name_next(struct dvb_platform_id *p,
				    struct dvb_platform_name *pos)
{
	uint8_t *end = (uint8_t *) p + p->platform_name_loop_length;
	uint8_t *next =	(uint8_t *) pos +
			sizeof(struct dvb_platform_name) +
			pos->platform_name_length;

	if (next >= end)
		return NULL;

	return (struct dvb_platform_name *) next;
}

#ifdef __cplusplus
}
#endif

#endif
