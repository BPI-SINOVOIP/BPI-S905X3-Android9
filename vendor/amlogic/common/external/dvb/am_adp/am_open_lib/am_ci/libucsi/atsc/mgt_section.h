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

#ifndef _UCSI_ATSC_MGT_SECTION_H
#define _UCSI_ATSC_MGT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>

enum atsc_mgt_section_table_type {
	ATSC_MGT_TABLE_TYPE_TVCT_CURRENT = 0,
	ATSC_MGT_TABLE_TYPE_TVCT_NEXT = 1,
	ATSC_MGT_TABLE_TYPE_CVCT_CURRENT = 2,
	ATSC_MGT_TABLE_TYPE_CVCT_NEXT = 3,
	ATSC_MGT_TABLE_TYPE_CHANNEL_ETT = 4,
	ATSC_MGT_TABLE_TYPE_DCCSCT = 5,
};

/**
 * atsc_mgt_section structure.
 */
struct atsc_mgt_section {
	struct atsc_section_psip head;

	uint16_t tables_defined;
	/* struct atsc_mgt_table tables[] */
	/* struct atsc_mgt_section_part2 part2 */
} __ucsi_packed;

struct atsc_mgt_table {
	uint16_t table_type;
  EBIT2(uint16_t reserved			: 3; ,
	uint16_t table_type_PID			:13; );
  EBIT2(uint8_t reserved1			: 3; ,
	uint8_t table_type_version_number	: 5; );
	uint32_t number_bytes;
  EBIT2(uint16_t reserved2			: 4; ,
	uint16_t table_type_descriptors_length	:12; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

struct atsc_mgt_section_part2 {
  EBIT2(uint16_t reserved			: 4; ,
	uint16_t descriptors_length		:12; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

static inline struct atsc_mgt_table * atsc_mgt_section_tables_first(struct atsc_mgt_section *mgt);
static inline struct atsc_mgt_table *
	atsc_mgt_section_tables_next(struct atsc_mgt_section *mgt, struct atsc_mgt_table *pos, int idx);

/**
 * Process a atsc_mgt_section.
 *
 * @param section Pointer to an atsc_section_psip structure.
 * @return atsc_mgt_section pointer, or NULL on error.
 */
struct atsc_mgt_section *atsc_mgt_section_codec(struct atsc_section_psip *section);

/**
 * Iterator for the tables field in an atsc_mgt_section.
 *
 * @param mgt atsc_mgt_section pointer.
 * @param pos Variable containing a pointer to the current atsc_mgt_table.
 * @param idx Integer used to count which table we in.
 */
#define atsc_mgt_section_tables_for_each(mgt, pos, idx) \
	for ((pos) = atsc_mgt_section_tables_first(mgt), idx=0; \
	     (pos); \
	     (pos) = atsc_mgt_section_tables_next(mgt, pos, ++idx))

/**
 * Iterator for the descriptors field in a atsc_mgt_table structure.
 *
 * @param table atsc_mgt_table pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_mgt_table_descriptors_for_each(table, pos) \
	for ((pos) = atsc_mgt_table_descriptors_first(table); \
	     (pos); \
	     (pos) = atsc_mgt_table_descriptors_next(table, pos))

/**
 * Accessor for the second part of an atsc_mgt_section.
 *
 * @param mgt atsc_mgt_section pointer.
 * @return atsc_mgt_section_part2 pointer.
 */
static inline struct atsc_mgt_section_part2 *
	atsc_mgt_section_part2(struct atsc_mgt_section *mgt)
{
	int pos = sizeof(struct atsc_mgt_section);

	struct atsc_mgt_table *cur_table;
	int idx;
	atsc_mgt_section_tables_for_each(mgt, cur_table, idx) {
		pos += sizeof(struct atsc_mgt_table);
		pos += cur_table->table_type_descriptors_length;
	}

	return (struct atsc_mgt_section_part2 *) (((uint8_t*) mgt) + pos);
}

/**
 * Iterator for the descriptors field in a atsc_mgt_section structure.
 *
 * @param part2 atsc_mgt_section_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_mgt_section_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_mgt_section_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_mgt_section_part2_descriptors_next(part2, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct atsc_mgt_table *
	atsc_mgt_section_tables_first(struct atsc_mgt_section *mgt)
{
	size_t pos = sizeof(struct atsc_mgt_section);

	if (mgt->tables_defined == 0)
		return NULL;

	return (struct atsc_mgt_table*) (((uint8_t *) mgt) + pos);
}

static inline struct atsc_mgt_table *
	atsc_mgt_section_tables_next(struct atsc_mgt_section *mgt,
				     struct atsc_mgt_table *pos,
				     int idx)
{
	if (idx >= mgt->tables_defined)
		return NULL;

	return (struct atsc_mgt_table *)
		(((uint8_t*) pos) + sizeof(struct atsc_mgt_table) + pos->table_type_descriptors_length);
}

static inline struct descriptor *
	atsc_mgt_table_descriptors_first(struct atsc_mgt_table *table)
{
	size_t pos = sizeof(struct atsc_mgt_table);

	if (table->table_type_descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) table) + pos);
}

static inline struct descriptor *
	atsc_mgt_table_descriptors_next(struct atsc_mgt_table *table,
					struct descriptor *pos)
{
	return next_descriptor((uint8_t*) table + sizeof(struct atsc_mgt_table),
				table->table_type_descriptors_length,
				pos);
}

static inline struct descriptor *
	atsc_mgt_section_part2_descriptors_first(struct atsc_mgt_section_part2 *part2)
{
	size_t pos = sizeof(struct atsc_mgt_section_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_mgt_section_part2_descriptors_next(struct atsc_mgt_section_part2 *part2,
						struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_mgt_section_part2),
				part2->descriptors_length,
				pos);
}

#ifdef __cplusplus
}
#endif

#endif
