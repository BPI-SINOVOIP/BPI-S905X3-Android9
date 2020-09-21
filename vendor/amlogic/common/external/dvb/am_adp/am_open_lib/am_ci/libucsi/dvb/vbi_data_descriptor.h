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

#ifndef _UCSI_DVB_VBI_DATA_DESCRIPTOR
#define _UCSI_DVB_VBI_DATA_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for the data_service_id field.
 */
enum {
	DVB_VBI_DATA_SERVICE_ID_EBU		= 0x01,
	DVB_VBI_DATA_SERVICE_ID_INVERTED	= 0x02,
	DVB_VBI_DATA_SERVICE_ID_VPS		= 0x04,
	DVB_VBI_DATA_SERVICE_ID_WSS		= 0x05,
	DVB_VBI_DATA_SERVICE_ID_CC		= 0x06,
	DVB_VBI_DATA_SERVICE_ID_MONO_422	= 0x07,
};

/**
 * dvb_vbi_data_descriptor structure
 */
struct dvb_vbi_data_descriptor {
	struct descriptor d;

	/* struct dvb_vbi_data_entry entries[] */
} __ucsi_packed;

/**
 * An entry in the dvb_vbi_data_descriptor entries field.
 */
struct dvb_vbi_data_entry {
	uint8_t data_service_id;
	uint8_t data_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * Format of the dvb_vbi_data_entry data field, if data_service_id == 1,2,4,5,6,7.
 */
struct dvb_vbi_data_x {
  EBIT3(uint8_t reserved 	: 2; ,
	uint8_t field_parity 	: 1; ,
	uint8_t line_offset	: 5; );
} __ucsi_packed;

/**
 * Process a dvb_vbi_data_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_vbi_data_descriptor pointer, or NULL on error.
 */
static inline struct dvb_vbi_data_descriptor*
	dvb_vbi_data_descriptor_codec(struct descriptor* d)
{
	uint8_t* p = (uint8_t*) d + 2;
	uint32_t pos = 0;
	uint32_t len = d->len;

	while(pos < len) {
		struct dvb_vbi_data_entry *e =
			(struct dvb_vbi_data_entry*) (p+pos);

		pos += sizeof(struct dvb_vbi_data_entry);

		if (pos > len)
			return NULL;

		pos += e->data_length;

		if (pos > len)
			return NULL;
	}

	return (struct dvb_vbi_data_descriptor*) d;
}

/**
 * Iterator for entries field in a dvb_vbi_data_descriptor structure.
 *
 * @param d Pointer to dvb_vbi_data_descriptor structure.
 * @param pos Variable holding pointer to the current dvb_vbi_data_entry structure.
 */
#define dvb_vbi_data_descriptor_entries_for_each(d, pos) \
	for ((pos) = dvb_vbi_data_descriptor_entries_first(d); \
	     (pos); \
	     (pos) = dvb_vbi_data_descriptor_entries_next(d, pos))

/**
 * Get a pointer to the data field of a dvb_vbi_data_entry.
 *
 * @param d dvb_vbi_data_entry structure.
 * @return Pointer to the data field.
 */
static inline uint8_t *
	dvb_vbi_data_entry_data(struct dvb_vbi_data_entry *d)
{
	return (uint8_t *) d + sizeof(struct dvb_vbi_data_entry);
}

/**
 * Get a pointer to the data field of a dvb_vbi_data_x for id 1,2,4,5,6,7.
 *
 * @param d dvb_vbi_data_entry structure.
 * @return Pointer to the data field, or NULL if invalid
 */
static inline struct dvb_vbi_data_x*
	dvb_vbi_data_entry_data_x(struct dvb_vbi_data_entry *d)
{
	switch(d->data_service_id) {
	case 1:
	case 2:
	case 4:
	case 5:
	case 6:
	case 7:
		return (struct dvb_vbi_data_x*) ((uint8_t *) d + sizeof(struct dvb_vbi_data_entry));
	}

	return NULL;
}










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_vbi_data_entry*
	dvb_vbi_data_descriptor_entries_first(struct dvb_vbi_data_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_vbi_data_entry *)
		((uint8_t*) d + sizeof(struct dvb_vbi_data_descriptor));
}

static inline struct dvb_vbi_data_entry*
	dvb_vbi_data_descriptor_entries_next(struct dvb_vbi_data_descriptor *d,
					     struct dvb_vbi_data_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_vbi_data_entry) +
			pos->data_length;

	if (next >= end)
		return NULL;

	return (struct dvb_vbi_data_entry *) next;
}

#ifdef __cplusplus
}
#endif

#endif
