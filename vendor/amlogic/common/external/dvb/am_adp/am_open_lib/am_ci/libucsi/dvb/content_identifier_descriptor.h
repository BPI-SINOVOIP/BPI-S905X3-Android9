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

#ifndef _UCSI_DVB_CONTENT_IDENTIFIER_DESCRIPTOR
#define _UCSI_DVB_CONTENT_IDENTIFIER_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>


/**
 * Possible values for the crid_type.
 */
enum {
	DVB_CRID_TYPE_NONE		= 0x00,
	DVB_CRID_TYPE_ITEM		= 0x01,
	DVB_CRID_TYPE_SERIES		= 0x02,
	DVB_CRID_TYPE_RECOMMENDATION	= 0x03,
};

/**
 * Possible values for the crid_location.
 */
enum {
	DVB_CRID_LOCATION_THIS_DESCRIPTOR	= 0x00,
	DVB_CRID_LOCATION_CIT			= 0x01,
};

/**
 * dvb_content_identifier_descriptor structure.
 */
struct dvb_content_identifier_descriptor {
	struct descriptor d;

	/* struct dvb_content_identifier_entry entries[] */
} __ucsi_packed;

/**
 * An entry in the entries field of a dvb_content_identifier_descriptor.
 */
struct dvb_content_identifier_entry {
  EBIT2(uint8_t crid_type		: 6; ,
	uint8_t crid_location		: 2; );
	/* struct dvb_content_identifier_data_00 data0 */
	/* struct dvb_content_identifier_data_01 data1 */
} __ucsi_packed;

/**
 * The data if crid_location == 0
 */
struct dvb_content_identifier_entry_data_0 {
	uint8_t crid_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * The data if crid_location == 1
 */
struct dvb_content_identifier_entry_data_1 {
	uint16_t crid_ref;
} __ucsi_packed;


/**
 * Process a dvb_content_identifier_descriptor.
 *
 * @param d Generic descriptor.
 * @return dvb_content_identifier_descriptor pointer, or NULL on error.
 */
static inline struct dvb_content_identifier_descriptor*
	dvb_content_identifier_descriptor_codec(struct descriptor* d)
{
	uint32_t len = d->len + 2;
	uint32_t pos = 2;
	uint8_t *buf = (uint8_t*) d;

	while(pos < len) {
		struct dvb_content_identifier_entry *e =
			(struct dvb_content_identifier_entry*) (buf + pos);

		if (len < (pos+1))
			return NULL;
		pos++;

		switch(e->crid_location) {
		case 0:
			if (len < (pos + 1))
				return NULL;
			if (len < (pos + 1 + buf[pos]))
				return NULL;
			pos += 1 + buf[pos];
			break;

		case 1:
			if (len < (pos+2))
				return NULL;
			bswap16(buf+pos);
			break;
		}
	}

	if (pos != len)
		return NULL;

	return (struct dvb_content_identifier_descriptor*) d;
}

/**
 * Iterator for entries field of a dvb_content_identifier_descriptor.
 *
 * @param d dvb_content_identifier_descriptor pointer.
 * @param pos Variable holding a pointer to the current dvb_content_identifier_entry.
 */
#define dvb_content_identifier_descriptor_entries_for_each(d, pos) \
	for ((pos) = dvb_content_identifier_descriptor_entries_first(d); \
	     (pos); \
	     (pos) = dvb_content_identifier_descriptor_entries_next(d, pos))

/**
 * Accessor for the data0 field of a dvb_content_identifier_entry.
 *
 * @param d dvb_content_identifier_entry pointer.
 * @return Pointer, or NULL on error.
 */
static inline struct dvb_content_identifier_entry_data_0*
	dvb_content_identifier_entry_data_0(struct dvb_content_identifier_entry *d)
{
	if (d->crid_location != 0)
		return NULL;
	return (struct dvb_content_identifier_entry_data_0*)
		((uint8_t*) d + sizeof(struct dvb_content_identifier_entry));
}
/**
 * Accessor for the data field of a dvb_content_identifier_entry_data_0.
 *
 * @param d dvb_content_identifier_entry_data_0 pointer.
 * @return Pointer, or NULL on error.
 */
static inline uint8_t*
	dvb_content_identifier_entry_data_0_data(struct dvb_content_identifier_entry_data_0 *d)
{
	return ((uint8_t*) d + sizeof(struct dvb_content_identifier_entry_data_0));
}

/**
 * Accessor for the data1 field of a dvb_content_identifier_entry.
 *
 * @param d dvb_content_identifier_entry pointer.
 * @return Pointer, or NULL on error.
 */
static inline struct dvb_content_identifier_entry_data_1*
	dvb_content_identifier_entry_data_1(struct dvb_content_identifier_entry *d)
{
	if (d->crid_location != 1)
		return NULL;
	return (struct dvb_content_identifier_entry_data_1*)
		((uint8_t*) d + sizeof(struct dvb_content_identifier_entry));
}








/******************************** PRIVATE CODE ********************************/
static inline struct dvb_content_identifier_entry*
	dvb_content_identifier_descriptor_entries_first(struct dvb_content_identifier_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_content_identifier_entry *)
		((uint8_t*) d + sizeof(struct dvb_content_identifier_descriptor));
}

static inline struct dvb_content_identifier_entry*
	dvb_content_identifier_descriptor_entries_next(struct dvb_content_identifier_descriptor *d,
					     struct dvb_content_identifier_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_content_identifier_entry);

	if (next >= end)
		return NULL;

	switch(pos->crid_location) {
	case 0:
		if ((next+2) >= end)
			return NULL;
		if ((next+2+next[1]) >= end)
			return NULL;
		break;

	case 1:
		if ((next+3) >= end)
			return NULL;
		break;
	}

	return (struct dvb_content_identifier_entry*) next;
}

#ifdef __cplusplus
}
#endif

#endif
