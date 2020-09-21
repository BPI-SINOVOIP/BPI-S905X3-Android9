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

#ifndef _UCSI_DVB_TVA_CONTAINER_SECTION_H
#define _UCSI_DVB_TVA_CONTAINER_SECTION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>
#include <libucsi/dvb/types.h>

/**
 * dvb_tva_container_section structure.
 */
struct dvb_tva_container_section {
	struct section_ext head;

	/* uint8_t data[] */
} __ucsi_packed;

/**
 * Process a dvb_tva_container_section.
 *
 * @param section Generic section header.
 * @return dvb_tdt_section pointer, or NULL on error.
 */
struct dvb_tva_container_section *dvb_tva_container_section_codec(struct section_ext *ext);

/**
 * Accessor for the container_id field of a tva container section.
 *
 * @param container dvb_tva_container_section pointer.
 * @return The container_id.
 */
static inline uint16_t dvb_tva_container_section_container_id(struct dvb_tva_container_section *container)
{
	return container->head.table_id_ext;
}

/**
 * Accessor for the data field of a dvb_data_broadcast_id_descriptor.
 *
 * @param d dvb_data_broadcast_id_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
dvb_tva_container_section_data(struct dvb_tva_container_section *s)
{
	return (uint8_t *) s + sizeof(struct dvb_tva_container_section);
}

/**
 * Determine the number of bytes in the data field of a dvb_tva_container_section.
 *
 * @param d dvb_tva_container_section pointer.
 * @return Length of the field in bytes.
 */
static inline int
dvb_tva_container_section_data_length(struct dvb_tva_container_section *s)
{
	return section_ext_length(&s->head) - sizeof(struct dvb_tva_container_section);
}


#ifdef __cplusplus
}
#endif

#endif
