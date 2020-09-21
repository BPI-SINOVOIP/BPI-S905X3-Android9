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

#ifndef _UCSI_DVB_NIT_SECTION_H
#define _UCSI_DVB_NIT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_nit_section structure.
 */
struct dvb_nit_section {
	struct section_ext head;

  EBIT2(uint16_t reserved_1			: 4; ,
	uint16_t network_descriptors_length	:12; );
	/* struct descriptor descriptors[] */
	/* struct dvb_nit_section_part2 part2 */
};

/**
 * Second part of a dvb_nit_section, following the variable length descriptors field.
 */
struct dvb_nit_section_part2 {
  EBIT2(uint16_t reserved_2			: 4; ,
	uint16_t transport_stream_loop_length	:12; );
	/* struct dvb_nit_transport transports[] */
} __ucsi_packed;

/**
 * An entry in the transports field of a dvb_nit_section_part2
 */
struct dvb_nit_transport {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
  EBIT2(uint16_t reserved			: 4; ,
	uint16_t transport_descriptors_length	:12; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process a dvb_nit_section.
 *
 * @param section Generic section_ext pointer.
 * @return dvb_nit_section pointer, or NULL on error.
 */
struct dvb_nit_section * dvb_nit_section_codec(struct section_ext *section);

/**
 * Accessor for the network_id field of a NIT.
 *
 * @param nit NIT pointer.
 * @return The network_id.
 */
static inline uint16_t dvb_nit_section_network_id(struct dvb_nit_section *nit)
{
	return nit->head.table_id_ext;
}

/**
 * Iterator over the descriptors field in a dvb_nit_section.
 *
 * @param nit dvb_nit_section pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define dvb_nit_section_descriptors_for_each(nit, pos) \
	for ((pos) = dvb_nit_section_descriptors_first(nit); \
	     (pos); \
	     (pos) = dvb_nit_section_descriptors_next(nit, pos))

/**
 * Accessor for a pointer to the dvb_nit_section_part2 structure.
 *
 * @param nit dvb_nit_section pointer.
 * @return dvb_nit_section_part2 pointer.
 */
static inline struct dvb_nit_section_part2 *dvb_nit_section_part2(struct dvb_nit_section * nit)
{
	return (struct dvb_nit_section_part2 *)
		((uint8_t*) nit + sizeof(struct dvb_nit_section) +
		 nit->network_descriptors_length);
}

/**
 * Iterator over the transports field in a dvb_nit_section_part2.
 *
 * @param nit dvb_nit_section pointer.
 * @param part2 dvb_nit_section_part2 pointer.
 * @param pos Pointer to the current dvb_nit_transport.
 */
#define dvb_nit_section_transports_for_each(nit, part2, pos) \
	for ((pos) = dvb_nit_section_transports_first(part2); \
	     (pos); \
	     (pos) = dvb_nit_section_transports_next(part2, pos))

/**
 * Iterator over the descriptors field in a dvb_nit_transport.
 *
 * @param transport dvb_nit_transport pointer.
 * @param pos Pointer to the current descriptor.
 */
#define dvb_nit_transport_descriptors_for_each(transport, pos) \
	for ((pos) = dvb_nit_transport_descriptors_first(transport); \
	     (pos); \
	     (pos) = dvb_nit_transport_descriptors_next(transport, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	dvb_nit_section_descriptors_first(struct dvb_nit_section * nit)
{
	if (nit->network_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) nit + sizeof(struct dvb_nit_section));
}

static inline struct descriptor *
	dvb_nit_section_descriptors_next(struct dvb_nit_section * nit,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t*) nit + sizeof(struct dvb_nit_section),
			      nit->network_descriptors_length,
			      pos);
}

static inline struct dvb_nit_transport *
	dvb_nit_section_transports_first(struct dvb_nit_section_part2 *part2)
{
	if (part2->transport_stream_loop_length == 0)
		return NULL;

	return (struct dvb_nit_transport *)
		((uint8_t *)part2 + sizeof(struct dvb_nit_section_part2));
}

static inline struct dvb_nit_transport *
	dvb_nit_section_transports_next(struct dvb_nit_section_part2 *part2,
					struct dvb_nit_transport *pos)
{
	uint8_t *end = (uint8_t*) part2 + sizeof(struct dvb_nit_section_part2) +
		part2->transport_stream_loop_length;
	uint8_t *next = (uint8_t*) pos + sizeof(struct dvb_nit_transport) +
		pos->transport_descriptors_length;

	if (next >= end)
		return NULL;

	return (struct dvb_nit_transport *) next;
}

static inline struct descriptor *
	dvb_nit_transport_descriptors_first(struct dvb_nit_transport *t)
{
	if (t->transport_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t*) t + sizeof(struct dvb_nit_transport));
}

static inline struct descriptor *
	dvb_nit_transport_descriptors_next(struct dvb_nit_transport *t,
					   struct descriptor* pos)
{
	return next_descriptor((uint8_t*) t + sizeof(struct dvb_nit_transport),
			      t->transport_descriptors_length,
			      pos);
}

#ifdef __cplusplus
}
#endif

#endif
