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

#ifndef _UCSI_DVB_SIT_SECTION_H
#define _UCSI_DVB_SIT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_sit_section structure.
 */
struct dvb_sit_section {
	struct section_ext head;

  EBIT2(uint16_t reserved			: 4; ,
	uint16_t transmission_info_loop_length  :12; );
	/* struct descriptor descriptors[] */
	/* struct dvb_sit_service services[] */
};

/**
 * An entry in the services field of a dvb_sit_section.
 */
struct dvb_sit_service {
	uint16_t service_id;
  EBIT3(uint16_t reserved		: 1; ,
	uint16_t running_status		: 3; ,
	uint16_t service_loop_length    :12; );
	/* struct descriptor descriptors[] */
};

/**
 * Process a dvb_sit_section.
 *
 * @param section Generic section_ext structure.
 * @return dvb_sit_section pointer, or NULL on error.
 */
struct dvb_sit_section * dvb_sit_section_codec(struct section_ext *section);

/**
 * Iterator for descriptors field in a dvb_sit_section.
 *
 * @param sit dvb_sit_section Pointer.
 * @param pos Variable holding pointer to current descriptor.
 */
#define dvb_sit_section_descriptors_for_each(sit, pos) \
	for ((pos) = dvb_sit_section_descriptors_first(sit); \
	     (pos); \
	     (pos) = dvb_sit_section_descriptors_first(sit))

/**
 * Iterator for services field in a dvb_sit_section.
 *
 * @param sit dvb_sit_section Pointer.
 * @param pos Variable holding pointer to current dvb_sit_service.
 */
#define dvb_sit_section_services_for_each(sit, pos) \
	for ((pos) = dvb_sit_section_services_first(sit); \
	     (pos); \
	     (pos) = dvb_sit_section_services_next(sit, pos))

/**
 * Iterator for descriptors field in a dvb_sit_service.
 *
 * @param service dvb_sit_service Pointer.
 * @param pos Variable holding pointer to current descriptor.
 */
#define dvb_sit_service_descriptors_for_each(service, pos) \
	for ((pos) = dvb_sit_service_descriptors_first(service); \
	     (pos); \
	     (pos) = dvb_sit_service_descriptors_next(service, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	dvb_sit_section_descriptors_first(struct dvb_sit_section *sit)
{
	if (sit->transmission_info_loop_length == 0)
		return NULL;

	return (struct descriptor *)
			((uint8_t *) sit + sizeof(struct dvb_sit_section));
}

static inline struct descriptor *
	dvb_sit_section_descriptors_next(struct dvb_sit_section *sit,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t*) sit + sizeof(struct dvb_sit_section),
			       sit->transmission_info_loop_length,
			       pos);
}

static inline struct dvb_sit_service *
	dvb_sit_section_services_first(struct dvb_sit_section *sit)
{
	size_t pos = sizeof(struct dvb_sit_section) + sit->transmission_info_loop_length;

	if (pos >= section_ext_length(&sit->head))
		return NULL;

	return (struct dvb_sit_service*) ((uint8_t *) sit + pos);
}

static inline struct dvb_sit_service *
	dvb_sit_section_services_next(struct dvb_sit_section *sit,
				      struct dvb_sit_service *pos)
{
	uint8_t *end = (uint8_t*) sit + section_ext_length(&sit->head);
	uint8_t *next =	(uint8_t *) pos + sizeof(struct dvb_sit_service) +
			pos->service_loop_length;

	if (next >= end)
		return NULL;

	return (struct dvb_sit_service *) next;
}

static inline struct descriptor *
	dvb_sit_service_descriptors_first(struct dvb_sit_service * t)
{
	if (t->service_loop_length == 0)
		return NULL;

	return (struct descriptor *)
			((uint8_t *) t + sizeof(struct dvb_sit_service));
}

static inline struct descriptor *
	dvb_sit_service_descriptors_next(struct dvb_sit_service *t,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t*) t + sizeof(struct dvb_sit_service),
			       t->service_loop_length,
			       pos);
}

#ifdef __cplusplus
}
#endif

#endif
