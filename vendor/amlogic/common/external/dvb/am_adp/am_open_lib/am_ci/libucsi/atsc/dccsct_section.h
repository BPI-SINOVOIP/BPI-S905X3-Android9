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

#ifndef _UCSI_ATSC_DCCSCT_SECTION_H
#define _UCSI_ATSC_DCCSCT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>

enum atsc_dccst_update_types {
	ATSC_DCCST_UPDATE_NEW_GENRE 	= 0x01,
	ATSC_DCCST_UPDATE_NEW_STATE 	= 0x02,
	ATSC_DCCST_UPDATE_NEW_COUNTY 	= 0x03,
};

/**
 * atsc_dccsct_section structure.
 */
struct atsc_dccsct_section {
	struct atsc_section_psip head;

	uint8_t updates_defined;
	/* struct atsc_dccsct_update updates */
	/* struct atsc_dccsct_section_part2 part2 */
} __ucsi_packed;

struct atsc_dccsct_update {
	uint8_t update_type;
	uint8_t update_data_length;
	/* struct atsc_dccsct_update_XXX data -- depends on update_type */
	/* struct atsc_dccsct_update_part2 part2 */
} __ucsi_packed;

struct atsc_dccsct_update_new_genre {
	uint8_t genre_category_code;
	/* atsc_text name */
} __ucsi_packed;

struct atsc_dccsct_update_new_state {
	uint8_t dcc_state_location_code;
	/* atsc_text name */
} __ucsi_packed;

struct atsc_dccsct_update_new_county {
	uint8_t state_code;
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t dcc_county_location_code	:10; );
	/* atsc_text name */
} __ucsi_packed;

struct atsc_dccsct_update_part2 {
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

struct atsc_dccsct_section_part2 {
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process an atsc_dccsct_section.
 *
 * @param section Pointer to an atsc_section_psip structure.
 * @return atsc_dccsct_section pointer, or NULL on error.
 */
struct atsc_dccsct_section *atsc_dccsct_section_codec(struct atsc_section_psip *section);

/**
 * Accessor for the dccsct_type field of a dccsct.
 *
 * @param dccsct dccsct pointer.
 * @return The dccsct_type.
 */
static inline uint16_t atsc_dccsct_section_dccsct_type(struct atsc_dccsct_section *dccsct)
{
	return dccsct->head.ext_head.table_id_ext;
}

/**
 * Iterator for the updates field in an atsc_dccsct_section.
 *
 * @param dccsct atsc_dccsct_section pointer.
 * @param pos Variable containing a pointer to the current atsc_dccsct_update.
 * @param idx Integer used to count which test we are in.
 */
#define atsc_dccsct_section_updates_for_each(dccsct, pos, idx) \
	for ((pos) = atsc_dccsct_section_updates_first(dccsct), idx=0; \
	     (pos); \
	     (pos) = atsc_dccsct_section_updates_next(dccsct, pos, ++idx))

/**
 * Accessor for the data field of a new genre atsc_dccsct_update.
 *
 * @param update atsc_dccsct_update pointer.
 * @return struct atsc_dccsct_update_new_genre pointer.
 */
static inline struct atsc_dccsct_update_new_genre *atsc_dccsct_update_new_genre(struct atsc_dccsct_update *update)
{
	if (update->update_type != ATSC_DCCST_UPDATE_NEW_GENRE)
		return NULL;

	return (struct atsc_dccsct_update_new_genre *)
		(((uint8_t*) update) + sizeof(struct atsc_dccsct_update));
}

/**
 * Accessor for the name field of an atsc_dccsct_update_new_genre.
 *
 * @param update atsc_dccsct_update_new_genre pointer.
 * @return text pointer.
 */
static inline struct atsc_text *atsc_dccsct_update_new_genre_name(struct atsc_dccsct_update *update)
{
	if ((update->update_data_length - 1) == 0)
		return NULL;

	return (struct atsc_text *)
		(((uint8_t*) update) + sizeof(struct atsc_dccsct_update_new_genre));
}

/**
 * Accessor for the data field of a new state atsc_dccsct_update.
 *
 * @param update atsc_dccsct_update pointer.
 * @return struct atsc_dccsct_update_new_state pointer.
 */
static inline struct atsc_dccsct_update_new_state *
	atsc_dccsct_update_new_state(struct atsc_dccsct_update *update)
{
	if (update->update_type != ATSC_DCCST_UPDATE_NEW_STATE)
		return NULL;

	return (struct atsc_dccsct_update_new_state *)
		(((uint8_t*) update) + sizeof(struct atsc_dccsct_update));
}

/**
 * Accessor for the name field of an atsc_dccsct_update_new_state.
 *
 * @param update atsc_dccsct_update_new_state pointer.
 * @return text pointer.
 */
static inline struct atsc_text *atsc_dccsct_update_new_state_name(struct atsc_dccsct_update *update)
{
	if ((update->update_data_length - 1) == 0)
		return NULL;

	return (struct atsc_text *)
		(((uint8_t*) update) + sizeof(struct atsc_dccsct_update_new_state));
}

/**
 * Accessor for the data field of a new county atsc_dccsct_update.
 *
 * @param update atsc_dccsct_update pointer.
 * @return struct atsc_dccsct_update_new_county pointer.
 */
static inline struct atsc_dccsct_update_new_county *
	atsc_dccsct_update_new_county(struct atsc_dccsct_update *update)
{
	if (update->update_type != ATSC_DCCST_UPDATE_NEW_COUNTY)
		return NULL;

	return (struct atsc_dccsct_update_new_county *)
		(((uint8_t*) update) + sizeof(struct atsc_dccsct_update));
}

/**
 * Accessor for the name field of an atsc_dccsct_update_new_county.
 *
 * @param update atsc_dccsct_update_new_county pointer.
 * @return text pointer.
 */
static inline struct atsc_text *atsc_dccsct_update_new_county_name(struct atsc_dccsct_update *update)
{
	if ((update->update_data_length - 3) == 0)
		return NULL;

	return (struct atsc_text*)
		(((uint8_t*) update) + sizeof(struct atsc_dccsct_update_new_county));
}

/**
 * Accessor for the part2 field of an atsc_dccsct_update.
 *
 * @param update atsc_dccsct_update pointer.
 * @return struct atsc_dccsct_test_part2 pointer.
 */
static inline struct atsc_dccsct_update_part2 *atsc_dccsct_update_part2(struct atsc_dccsct_update *update)
{
	int pos = sizeof(struct atsc_dccsct_update);
	pos += update->update_data_length;

	return (struct atsc_dccsct_update_part2 *) (((uint8_t*) update) + pos);
}

/**
 * Iterator for the descriptors field in an atsc_dccsct_update_part2 structure.
 *
 * @param part2 atsc_dccsct_update_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_dccsct_update_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_dccsct_update_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_dccsct_update_part2_descriptors_next(part2, pos))

/**
 * Iterator for the descriptors field in a atsc_dccsct_section_part2 structure.
 *
 * @param part2 atsc_dccsct_section_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_dccsct_section_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_dccsct_section_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_dccsct_section_part2_descriptors_next(part2, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct atsc_dccsct_update *
	atsc_dccsct_section_updates_first(struct atsc_dccsct_section *dccsct)
{
	size_t pos = sizeof(struct atsc_dccsct_section);

	if (dccsct->updates_defined == 0)
		return NULL;

	return (struct atsc_dccsct_update*) (((uint8_t *) dccsct) + pos);
}

static inline struct atsc_dccsct_update*
	atsc_dccsct_section_updates_next(struct atsc_dccsct_section *dccsct,
				         struct atsc_dccsct_update *pos,
				         int idx)
{
	if (idx >= dccsct->updates_defined)
		return NULL;

	struct atsc_dccsct_update_part2 *part2 = atsc_dccsct_update_part2(pos);
	int len = sizeof(struct atsc_dccsct_update_part2);
	len += part2->descriptors_length;

	return (struct atsc_dccsct_update *) (((uint8_t*) part2) + len);
}

static inline struct descriptor *
	atsc_dccsct_update_part2_descriptors_first(struct atsc_dccsct_update_part2 *part2)
{
	size_t pos = sizeof(struct atsc_dccsct_update_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_dccsct_update_part2_descriptors_next(struct atsc_dccsct_update_part2 *part2,
						  struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_dccsct_update_part2),
				part2->descriptors_length,
				pos);
}

static inline struct descriptor *
	atsc_dccsct_section_part2_descriptors_first(struct atsc_dccsct_section_part2 *part2)
{
	size_t pos = sizeof(struct atsc_dccsct_section_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_dccsct_section_part2_descriptors_next(struct atsc_dccsct_section_part2 *part2,
						 struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_dccsct_section_part2),
				part2->descriptors_length,
				pos);
}


#ifdef __cplusplus
}
#endif

#endif
