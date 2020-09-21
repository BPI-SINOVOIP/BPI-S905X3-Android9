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

#ifndef _UCSI_DVB_SDT_SECTION_H
#define _UCSI_DVB_SDT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_sdt_section structure.
 */
struct dvb_sdt_section {
	struct section_ext head;

	uint16_t original_network_id;
	uint8_t reserved;
	/* struct dvb_sdt_service services[] */
} __ucsi_packed;

/**
 * An entry in the services field of a dvb_sdt_section.
 */
struct dvb_sdt_service {
	uint16_t service_id;
  EBIT3(uint8_t	reserved			: 6; ,
	uint8_t eit_schedule_flag		: 1; ,
	uint8_t eit_present_following_flag	: 1; );
  EBIT3(uint16_t running_status			: 3; ,
	uint16_t free_ca_mode			: 1; ,
	uint16_t descriptors_loop_length	:12; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process a dvb_sdt_section.
 *
 * @param section Pointer to a generic section_ext structure.
 * @return dvb_sdt_section pointer, or NULL on error.
 */
struct dvb_sdt_section * dvb_sdt_section_codec(struct section_ext *section);

/**
 * Accessor for the transport_stream_id field of an SDT.
 *
 * @param sdt SDT pointer.
 * @return The transport_stream_id.
 */
static inline uint16_t dvb_sdt_section_transport_stream_id(struct dvb_sdt_section *sdt)
{
	return sdt->head.table_id_ext;
}

/**
 * Iterator for the services field in a dvb_sdt_section.
 *
 * @param sdt dvb_sdt_section pointer.
 * @param pos Variable containing a pointer to the current dvb_sdt_service.
 */
#define dvb_sdt_section_services_for_each(sdt, pos) \
	for ((pos) = dvb_sdt_section_services_first(sdt); \
	     (pos); \
	     (pos) = dvb_sdt_section_services_next(sdt, pos))

/**
 * Iterator for the descriptors field in a dvb_sdt_service.
 *
 * @param service dvb_sdt_service pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define dvb_sdt_service_descriptors_for_each(service, pos) \
	for ((pos) = dvb_sdt_service_descriptors_first(service); \
	     (pos); \
	     (pos) = dvb_sdt_service_descriptors_next(service, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct dvb_sdt_service *
	dvb_sdt_section_services_first(struct dvb_sdt_section * sdt)
{
	size_t pos = sizeof(struct dvb_sdt_section);

	if (pos >= section_ext_length(&sdt->head))
		return NULL;

	return (struct dvb_sdt_service*) ((uint8_t *) sdt + pos);
}

static inline struct dvb_sdt_service *
	dvb_sdt_section_services_next(struct dvb_sdt_section * sdt,
				      struct dvb_sdt_service * pos)
{
	uint8_t *end = (uint8_t*) sdt + section_ext_length(&sdt->head);
	uint8_t *next = (uint8_t*) pos + sizeof(struct dvb_sdt_service) +
			pos->descriptors_loop_length;

	if (next >= end)
		return NULL;

	return (struct dvb_sdt_service *) next;
}

static inline struct descriptor *
	dvb_sdt_service_descriptors_first(struct dvb_sdt_service *svc)
{
	if (svc->descriptors_loop_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t*) svc + sizeof(struct dvb_sdt_service));
}

static inline struct descriptor *
	dvb_sdt_service_descriptors_next(struct dvb_sdt_service *svc,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t*) svc + sizeof(struct dvb_sdt_service),
			       svc->descriptors_loop_length,
			       pos);
}

#ifdef __cplusplus
}
#endif

#endif
