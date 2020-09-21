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

#ifndef _UCSI_DVB_BAT_SECTION_H
#define _UCSI_DVB_BAT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_bat_section structure.
 */
struct dvb_bat_section {
	struct section_ext head;

  EBIT2(uint16_t reserved_1			: 4; ,
	uint16_t bouquet_descriptors_length	:12; );
	/* struct descriptor descriptors[] */
	/* struct dvb_bat_section_part2 part2 */
};

/**
 * Second part of a dvb_bat_section, following the variable length descriptors field.
 */
struct dvb_bat_section_part2 {
  EBIT2(uint16_t reserved_2			: 4; ,
	uint16_t transport_stream_loop_length	:12; );
	/* struct dvb_bat_transport transports[] */
} __ucsi_packed;

/**
 * An entry in the transports field of a dvb_bat_section_part2.
 */
struct dvb_bat_transport {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
  EBIT2(uint16_t reserved			: 4; ,
	uint16_t transport_descriptors_length	:12; );
	/* struct descriptor descriptors[] */
};

/**
 * Process a dvb_bat_section.
 *
 * @param section Generic section  pointer.
 * @return dvb_bat_section pointer, or NULL on error.
 */
struct dvb_bat_section *dvb_bat_section_codec(struct section_ext *section);

/**
 * Accessor for the bouquet_id field of a BAT.
 *
 * @param bat BAT pointer.
 * @return The bouquet_id.
 */
static inline uint16_t dvb_bat_section_bouquet_id(struct dvb_bat_section *bat)
{
	return bat->head.table_id_ext;
}

/**
 * Iterator for the descriptors field in a dvb_bat_section.
 *
 * @param bat dvb_bat_section pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define dvb_bat_section_descriptors_for_each(bat, pos) \
	for ((pos) = dvb_bat_section_descriptors_first(bat); \
	     (pos); \
	     (pos) = dvb_bat_section_descriptors_next(bat, pos))

/**
 * Accessor for the second part of a dvb_bat_section.
 *
 * @param bat dvb_bat_section pointer.
 * @return dvb_bat_section_part2 pointer.
 */
static inline struct dvb_bat_section_part2 *
	dvb_bat_section_part2(struct dvb_bat_section *bat)
{
	return (struct dvb_bat_section_part2 *)
		((uint8_t*) bat +
		 sizeof(struct dvb_bat_section) +
		 bat->bouquet_descriptors_length);

}

/**
 * Iterator for the transports field of a dvb_bat_section_part2.
 *
 * @param part2 dvb_bat_section_part2 pointer.
 * @param pos Variable containing a pointer to the current dvb_bat_transport.
 */
#define dvb_bat_section_transports_for_each(part2, pos) \
	for ((pos) = dvb_bat_section_transports_first(part2); \
	     (pos); \
	     (pos) = dvb_bat_section_transports_next(part2, pos))

/**
 * Iterator for the descriptors field of a dvb_bat_transport.
 *
 * @param transport dvb_bat_transport pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define dvb_bat_transport_descriptors_for_each(transport, pos) \
	for ((pos) = dvb_bat_transport_descriptors_first(transport); \
	     (pos); \
	     (pos) = dvb_bat_transport_descriptors_next(transport, pos))












/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	dvb_bat_section_descriptors_first(struct dvb_bat_section *bat)
{
	if (bat->bouquet_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) bat + sizeof(struct dvb_bat_section));
}

static inline struct descriptor *
	dvb_bat_section_descriptors_next(struct dvb_bat_section *bat,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t*) bat + sizeof(struct dvb_bat_section),
			       bat->bouquet_descriptors_length,
			       pos);
}

static inline struct dvb_bat_transport *
	dvb_bat_section_transports_first(struct dvb_bat_section_part2 *part2)
{
	if (part2->transport_stream_loop_length == 0)
		return NULL;

	return (struct dvb_bat_transport *)
		((uint8_t *) part2 + sizeof(struct dvb_bat_section_part2));
}

static inline struct dvb_bat_transport *
	dvb_bat_section_transports_next(struct dvb_bat_section_part2 *part2,
					struct dvb_bat_transport *pos)
{
	uint8_t *end = (uint8_t*) part2 + sizeof(struct dvb_bat_section_part2) +
			part2->transport_stream_loop_length;
	uint8_t *next = (uint8_t*) pos + sizeof(struct dvb_bat_transport) +
			pos->transport_descriptors_length;

	if (next >= end)
		return NULL;

	return (struct dvb_bat_transport *) next;
}

static inline struct descriptor *
	dvb_bat_transport_descriptors_first(struct dvb_bat_transport *t)
{
	if (t->transport_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t*)t + sizeof(struct dvb_bat_transport));
}

static inline struct descriptor *
	dvb_bat_transport_descriptors_next(struct dvb_bat_transport *t,
					   struct descriptor* pos)
{
	return next_descriptor((uint8_t*) t + sizeof(struct dvb_bat_transport),
			       t->transport_descriptors_length,
			       pos);
}

#ifdef __cplusplus
}
#endif

#endif
