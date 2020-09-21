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

#ifndef _UCSI_MPEG_METADATA_SECTION_H
#define _UCSI_MPEG_METADATA_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * mpeg_metadata_section structure.
 */
struct mpeg_metadata_section {
	struct section_ext head;

	/* uint8_t data[] */
} __ucsi_packed;

/**
 * Process an mpeg_metadata_section structure.
 *
 * @param section Pointer to the section_ext structure.
 * @return Pointer to the mpeg_metadata_section structure, or NULL on error.
 */
extern struct mpeg_metadata_section *mpeg_metadata_section_codec(struct section_ext *section);

/**
 * Accessor for the random_access_indicator field of a metadata section.
 *
 * @param metadata metadata section pointer.
 * @return The random_access_indicator.
 */
static inline uint8_t mpeg_metadata_section_random_access_indicator(struct mpeg_metadata_section *metadata)
{
	return metadata->head.reserved >> 1;
}

/**
 * Accessor for the decoder_config_flag field of a metadata section.
 *
 * @param metadata metadata section pointer.
 * @return The decoder_config_flag.
 */
static inline uint8_t mpeg_metadata_section_decoder_config_flag(struct mpeg_metadata_section *metadata)
{
	return metadata->head.reserved & 1;
}

/**
 * Accessor for the fragment_indicator field of a metadata section.
 *
 * @param metadata metadata section pointer.
 * @return The fragment_indicator.
 */
static inline uint8_t mpeg_metadata_section_fragment_indicator(struct mpeg_metadata_section *metadata)
{
	return metadata->head.reserved1;
}

/**
 * Accessor for the service_id field of a metadata section.
 *
 * @param metadata metadata section pointer.
 * @return The service_id.
 */
static inline uint16_t mpeg_metadata_section_service_id(struct mpeg_metadata_section *metadata)
{
	return metadata->head.table_id_ext >> 8;
}

/**
 * Retrieve pointer to data field of an mpeg_metadata_section.
 *
 * @param s mpeg_metadata_section pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	mpeg_metadata_section_data(struct mpeg_metadata_section *s)
{
	return (uint8_t *) s + sizeof(struct mpeg_metadata_section);
}


/**
 * Determine length of the data field of an mpeg_copyright_descriptor.
 *
 * @param s mpeg_metadata_section_data pointer.
 * @return Length of field in bytes.
 */
static inline int
	mpeg_metadata_section_data_length(struct mpeg_metadata_section *s)
{
	return section_ext_length(&s->head) - sizeof(struct mpeg_metadata_section);
}

#ifdef __cplusplus
}
#endif

#endif
