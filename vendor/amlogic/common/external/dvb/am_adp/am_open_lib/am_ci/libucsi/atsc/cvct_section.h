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

#ifndef _UCSI_ATSC_CVCT_SECTION_H
#define _UCSI_ATSC_CVCT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>

/**
 * atsc_cvct_section structure.
 */
struct atsc_cvct_section {
	struct atsc_section_psip head;

	uint8_t num_channels_in_section;
	/* struct atsc_cvct_channel channels[] */
	/* struct atsc_cvct_channel_part2 part2 */
} __ucsi_packed;

struct atsc_cvct_channel {
	uint16_t short_name[7]; // UTF-16 network ordered
  EBIT4(uint32_t reserved			: 4; ,
	uint32_t major_channel_number		:10; ,
	uint32_t minor_channel_number		:10; ,
	uint32_t modulation_mode		: 8; );
	uint32_t carrier_frequency;
	uint16_t channel_TSID;
	uint16_t program_number;
  EBIT8(uint16_t ETM_location			: 2; ,
	uint16_t access_controlled		: 1; ,
	uint16_t hidden				: 1; ,
	uint16_t path_select			: 1; ,
	uint16_t out_of_band			: 1; ,
	uint16_t hide_guide			: 1; ,
	uint16_t reserved2			: 3; ,
	uint16_t service_type			: 6; );
	uint16_t source_id;
  EBIT2(uint16_t reserved3			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

struct atsc_cvct_section_part2 {
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

static inline struct atsc_cvct_channel *atsc_cvct_section_channels_first(struct atsc_cvct_section *cvct);
static inline struct atsc_cvct_channel *
	atsc_cvct_section_channels_next(struct atsc_cvct_section *cvct, struct atsc_cvct_channel *pos, int idx);

/**
 * Process a atsc_cvct_section.
 *
 * @param section Pointer to anj atsc_section_psip structure.
 * @return atsc_cvct_section pointer, or NULL on error.
 */
struct atsc_cvct_section *atsc_cvct_section_codec(struct atsc_section_psip *section);

/**
 * Accessor for the transport_stream_id field of a CVCT.
 *
 * @param cvdt CVDT pointer.
 * @return The transport_stream_id.
 */
static inline uint16_t atsc_cvct_section_transport_stream_id(struct atsc_cvct_section *cvct)
{
	return cvct->head.ext_head.table_id_ext;
}

/**
 * Iterator for the tables field in an atsc_cvct_section.
 *
 * @param mgt atsc_cvct_section pointer.
 * @param pos Variable containing a pointer to the current atsc_cvct_channel.
 * @param idx Integer used to count which table we in.
 */
#define atsc_cvct_section_channels_for_each(mgt, pos, idx) \
	for ((pos) = atsc_cvct_section_channels_first(mgt), idx=0; \
	     (pos); \
	     (pos) = atsc_cvct_section_channels_next(mgt, pos, ++idx))

/**
 * Iterator for the descriptors field in a atsc_cvct_channel structure.
 *
 * @param table atsc_cvct_channel pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_cvct_channel_descriptors_for_each(table, pos) \
	for ((pos) = atsc_cvct_channel_descriptors_first(table); \
	     (pos); \
	     (pos) = atsc_cvct_channel_descriptors_next(table, pos))

/**
 * Accessor for the second part of an atsc_cvct_section.
 *
 * @param mgt atsc_cvct_section pointer.
 * @return atsc_cvct_section_part2 pointer.
 */
static inline struct atsc_cvct_section_part2 *
	atsc_cvct_section_part2(struct atsc_cvct_section *mgt)
{
	int pos = sizeof(struct atsc_cvct_section);

	struct atsc_cvct_channel *cur_table;
	int idx;
	atsc_cvct_section_channels_for_each(mgt, cur_table, idx) {
		pos += sizeof(struct atsc_cvct_channel);
		pos += cur_table->descriptors_length;
	}

	return (struct atsc_cvct_section_part2 *) (((uint8_t*) mgt) + pos);
}

/**
 * Iterator for the descriptors field in a atsc_cvct_section structure.
 *
 * @param part2 atsc_cvct_section_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_cvct_section_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_cvct_section_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_cvct_section_part2_descriptors_next(part2, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct atsc_cvct_channel *
	atsc_cvct_section_channels_first(struct atsc_cvct_section *cvct)
{
	size_t pos = sizeof(struct atsc_cvct_section);

	if (cvct->num_channels_in_section == 0)
		return NULL;

	return (struct atsc_cvct_channel*) (((uint8_t *) cvct) + pos);
}

static inline struct atsc_cvct_channel *
	atsc_cvct_section_channels_next(struct atsc_cvct_section *cvct,
				     struct atsc_cvct_channel *pos,
				     int idx)
{
	if (idx >= cvct->num_channels_in_section)
		return NULL;

	return (struct atsc_cvct_channel *)
		(((uint8_t*) pos) + sizeof(struct atsc_cvct_channel) + pos->descriptors_length);
}

static inline struct descriptor *
	atsc_cvct_channel_descriptors_first(struct atsc_cvct_channel *table)
{
	size_t pos = sizeof(struct atsc_cvct_channel);

	if (table->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) table) + pos);
}

static inline struct descriptor *
	atsc_cvct_channel_descriptors_next(struct atsc_cvct_channel *table,
					struct descriptor *pos)
{
	return next_descriptor((uint8_t*) table + sizeof(struct atsc_cvct_channel),
				table->descriptors_length,
				pos);
}

static inline struct descriptor *
	atsc_cvct_section_part2_descriptors_first(struct atsc_cvct_section_part2 *part2)
{
	size_t pos = sizeof(struct atsc_cvct_section_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_cvct_section_part2_descriptors_next(struct atsc_cvct_section_part2 *part2,
						 struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_cvct_section_part2),
				part2->descriptors_length,
				pos);
}

#ifdef __cplusplus
}
#endif

#endif
