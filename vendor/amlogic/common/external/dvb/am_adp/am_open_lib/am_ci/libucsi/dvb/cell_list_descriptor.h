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

#ifndef _UCSI_DVB_CELL_LIST_DESCRIPTOR
#define _UCSI_DVB_CELL_LIST_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * dvb_cell_list_descriptor structure.
 */
struct dvb_cell_list_descriptor {
	struct descriptor d;

	/* struct dvb_cell_list_entry cells[] */
} __ucsi_packed;

/**
 * An entry in the cells field of a dvb_cell_list_descriptor.
 */
struct dvb_cell_list_entry {
	uint16_t cell_id;
	uint16_t cell_latitude;
	uint16_t cell_longitude;
  EBIT3(uint32_t cell_extend_of_latitude	:12; ,
	uint32_t cell_extend_of_longitude	:12; ,
	uint32_t subcell_info_loop_length	: 8; );
	/* struct dvb_subcell_list_entry subcells[] */
} __ucsi_packed;

/**
 * An entry in the subcells field of a dvb_cell_list_entry.
 */
struct dvb_subcell_list_entry {
	uint8_t cell_id_extension;
	uint16_t subcell_latitude;
	uint16_t subcell_longitude;
  EBIT2(uint32_t subcell_extend_of_latitude	:12; ,
	uint32_t subcell_extend_of_longitude	:12; );
} __ucsi_packed;

/**
 * Process a dvb_cell_list_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_cell_list_descriptor pointer, or NULL on error.
 */
static inline struct dvb_cell_list_descriptor*
	dvb_cell_list_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint32_t pos2 = 0;
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t len = d->len;

	while(pos < len) {
		struct dvb_cell_list_entry *e =
			(struct dvb_cell_list_entry*) (buf+pos);

		if ((pos + sizeof(struct dvb_cell_list_entry)) > len)
			return NULL;

		bswap16(buf+pos);
		bswap16(buf+pos+2);
		bswap16(buf+pos+4);
		bswap32(buf+pos+6);

		pos += sizeof(struct dvb_cell_list_entry);

		if ((pos + e->subcell_info_loop_length) > len)
			return NULL;

		if (e->subcell_info_loop_length % sizeof(struct dvb_subcell_list_entry))
			return NULL;

		pos2 = 0;
		while(pos2 < e->subcell_info_loop_length) {
			bswap16(buf+pos+pos2+1);
			bswap16(buf+pos+pos2+3);
			bswap24(buf+pos+pos2+5);

			pos2 += sizeof(struct dvb_subcell_list_entry);
		}

		pos += e->subcell_info_loop_length;
	}

	return (struct dvb_cell_list_descriptor*) d;
}

/**
 * Iterator for the cells field of a dvb_cell_list_descriptor.
 *
 * @param d dvb_cell_list_descriptor pointer.
 * @param pos Variable holding a pointer to the current dvb_cell_list_entry.
 */
#define dvb_cell_list_descriptor_cells_for_each(d, pos) \
	for ((pos) = dvb_cell_list_descriptor_cells_first(d); \
	     (pos); \
	     (pos) = dvb_cell_list_descriptor_cells_next(d, pos))

/**
 * Iterator for the subcells field of a dvb_cell_list_entry.
 *
 * @param cell dvb_cell_list_entry pointer.
 * @param pos Variable holding a pointer to the current dvb_subcell_list_entry.
 */
#define dvb_cell_list_entry_subcells_for_each(cell, pos) \
	for ((pos) = dvb_cell_list_entry_subcells_first(cell); \
	     (pos); \
	     (pos) = dvb_cell_list_entry_subcells_next(cell, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_cell_list_entry*
	dvb_cell_list_descriptor_cells_first(struct dvb_cell_list_descriptor *d)
{
	if (d->d.len == 0)
		return NULL;

	return (struct dvb_cell_list_entry *)
		((uint8_t*) d + sizeof(struct dvb_cell_list_descriptor));
}

static inline struct dvb_cell_list_entry*
	dvb_cell_list_descriptor_cells_next(struct dvb_cell_list_descriptor *d,
					    struct dvb_cell_list_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos +
			sizeof(struct dvb_cell_list_entry) +
			pos->subcell_info_loop_length;

	if (next >= end)
		return NULL;

	return (struct dvb_cell_list_entry *) next;
}

static inline struct dvb_subcell_list_entry*
	dvb_cell_list_entry_subcells_first(struct dvb_cell_list_entry *d)
{
	if (d->subcell_info_loop_length == 0)
		return NULL;

	return (struct dvb_subcell_list_entry*)
		((uint8_t*) d + sizeof(struct dvb_cell_list_entry));
}

static inline struct dvb_subcell_list_entry*
	dvb_cell_list_entry_subcells_next(struct dvb_cell_list_entry *d,
					  struct dvb_subcell_list_entry *pos)
{
	uint8_t *next = (uint8_t*) pos + sizeof(struct dvb_subcell_list_entry);
	uint8_t *end = (uint8_t*) d +
			sizeof(struct dvb_cell_list_entry) +
			d->subcell_info_loop_length;

	if (next >= end)
		return NULL;

	return (struct dvb_subcell_list_entry *) next;
}

#ifdef __cplusplus
}
#endif

#endif
