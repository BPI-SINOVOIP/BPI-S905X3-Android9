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

#ifndef _UCSI_DVB_ST_SECTION_H
#define _UCSI_DVB_ST_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_st_section structure.
 */
struct dvb_st_section {
	struct section head;

	/* uint8_t data[] */
};

/**
 * Process a dvb_st_section.
 *
 * @param section Generic section header.
 * @return dvb_st_section pointer, or NULL on error.
 */
struct dvb_st_section *dvb_st_section_codec(struct section *section);

/**
 * Accessor for data field of dvb_st_section.
 *
 * @param st dvb_st_section Pointer.
 * @return Pointer to field.
 */
static inline uint8_t*
	dvb_st_section_data(struct dvb_st_section* st)
{
	return (uint8_t*) st + sizeof(struct dvb_st_section);
}

/**
 * Calculate length of data field of dvb_st_section.
 *
 * @param st dvb_st_section Pointer.
 * @return Length in bytes.
 */
static inline int
	dvb_st_section_data_length(struct dvb_st_section* st)
{
	return st->head.length;
}

#ifdef __cplusplus
}
#endif

#endif
