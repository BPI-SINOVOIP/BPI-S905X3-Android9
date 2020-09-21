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

#ifndef _UCSI_DVB_TOT_SECTION_H
#define _UCSI_DVB_TOT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>
#include <libucsi/dvb/types.h>

/**
 * dvb_tot_section structure.
 */
struct dvb_tot_section {
	struct section head;

	dvbdate_t utc_time;
  EBIT2(uint16_t reserved		: 4; ,
	uint16_t descriptors_loop_length:12; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process a dvb_tot_section.
 *
 * @param section Pointer to generic section structure.
 * @return dvb_tot_section pointer, or NULL on error.
 */
struct dvb_tot_section * dvb_tot_section_codec(struct section *section);

/**
 * Iterator for descriptors field of dvb_tot_section.
 *
 * @param tot dvb_tot_section pointer.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define dvb_tot_section_descriptors_for_each(tot, pos) \
	for ((pos) = dvb_tot_section_descriptors_first(tot); \
	     (pos); \
	     (pos) = dvb_tot_section_descriptors_next(tot, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	dvb_tot_section_descriptors_first(struct dvb_tot_section * tot)
{
	if (tot->descriptors_loop_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) tot + sizeof(struct dvb_tot_section));
}

static inline struct descriptor *
	dvb_tot_section_descriptors_next(struct dvb_tot_section *tot,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t *) tot + sizeof(struct dvb_tot_section),
			       tot->descriptors_loop_length,
			       pos);
}

#ifdef __cplusplus
}
#endif

#endif
