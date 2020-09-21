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

#ifndef _UCSI_DVB_TVA_ID_DESCRIPTOR
#define _UCSI_DVB_TVA_ID_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_tva_id_descriptor structure.
 */
struct dvb_tva_id_descriptor {
	struct descriptor d;

	/* struct dvb_tva_id_entry entries[] */
} __ucsi_packed;

/**
 * An entry in the entries field of a dvb_tva_id_descriptor.
 */
struct dvb_tva_id_entry {
	uint16_t tva_id;
  EBIT2(uint8_t reserved		: 5; ,
	uint8_t running_status		: 3; );
} __ucsi_packed;

/**
 * Process a dvb_tva_id_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_tva_id_descriptor pointer, or NULL on error.
 */
static inline struct dvb_tva_id_descriptor*
	dvb_tva_id_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint32_t len = d->len;
	uint8_t* buf = (uint8_t*) d + 2;

	pos += sizeof(struct dvb_tva_id_descriptor) - 2;
	if (len % sizeof(struct dvb_tva_id_entry))
		return NULL;

	while(pos < len) {
		bswap16(buf+pos);
		pos+=3;
	}

	return (struct dvb_tva_id_descriptor*) d;
}

/**
 * Iterator for the entries field of a dvb_tva_id_descriptor.
 *
 * @param d dvb_tva_id_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_tva_id_entry.
 */
#define dvb_tva_id_descriptor_entries_for_each(d, pos) \
	for ((pos) = dvb_tva_id_descriptor_entries_first(d); \
	     (pos); \
	     (pos) = dvb_tva_id_descriptor_entries_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_tva_id_entry*
	dvb_tva_id_descriptor_entries_first(struct dvb_tva_id_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_tva_id_entry *)
		((uint8_t*) d + sizeof(struct dvb_tva_id_descriptor));
}

static inline struct dvb_tva_id_entry*
	dvb_tva_id_descriptor_entries_next(struct dvb_tva_id_descriptor *d,
							     struct dvb_tva_id_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_tva_id_entry);

	if (next >= end)
		return NULL;

	return (struct dvb_tva_id_entry *) next;
}

#ifdef __cplusplus
}
#endif

#endif
