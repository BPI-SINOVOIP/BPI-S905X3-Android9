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

#ifndef _UCSI_DVB_RST_SECTION_H
#define _UCSI_DVB_RST_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_rst_section structure.
 */
struct dvb_rst_section {
	struct section head;

	/* struct dvb_rst_status statuses[] */
};

/**
 * An entry in the statuses field of a dvb_rst_section structure.
 */
struct dvb_rst_status {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
	uint16_t event_id;
  EBIT2(uint8_t reserved	: 5;  ,
	uint8_t running_status	: 3;  );
};

/**
 * Process a dvb_rst_section.
 *
 * @param section Pointer to a generic section strcuture.
 * @return dvb_rst_section pointer, or NULL on error.
 */
struct dvb_rst_section *dvb_rst_section_codec(struct section *section);

/**
 * Iterator for entries in the statuses field of a dvb_rst_section.
 *
 * @param rst dvb_rst_section pointer.
 * @param pos Variable containing a pointer to the current dvb_rst_status.
 */
#define dvb_rst_section_statuses_for_each(rst, pos) \
	for ((pos) = dvb_rst_section_statuses_first(rst); \
	     (pos); \
	     (pos) = dvb_rst_section_statuses_next(rst, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_rst_status *
	dvb_rst_section_statuses_first(struct dvb_rst_section *rst)
{
	size_t pos = sizeof(struct dvb_rst_section);

	if (pos >= section_length(&rst->head))
		return NULL;

	return (struct dvb_rst_status*) ((uint8_t *) rst + pos);
}

static inline struct dvb_rst_status *
	dvb_rst_section_statuses_next(struct dvb_rst_section * rst,
				      struct dvb_rst_status * pos)
{
	uint8_t *end = (uint8_t*) rst + section_length(&rst->head);
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_rst_status);

	if (next >= end)
		return NULL;

	return (struct dvb_rst_status *) next;
}

#ifdef __cplusplus
}
#endif

#endif
