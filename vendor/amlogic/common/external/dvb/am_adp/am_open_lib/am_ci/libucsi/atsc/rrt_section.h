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

#ifndef _UCSI_ATSC_RRT_SECTION_H
#define _UCSI_ATSC_RRT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>

/**
 * atsc_rrt_section structure.
 */
struct atsc_rrt_section {
	struct atsc_section_psip head;

	uint8_t rating_region_name_length;
	/* struct atsc_text rating_region_name_text */
	/* struct atsc_rrt_section_part2 part2 */
} __ucsi_packed;

struct atsc_rrt_section_part2 {
	uint8_t dimensions_defined;
	/* struct atsc_rrt_dimension dimensions[] */
	/* struct atsc_rrt_section_part3 part3 */
} __ucsi_packed;

struct atsc_rrt_dimension {
	uint8_t dimension_name_length;
	/* struct atsc_text dimension_name_text */
	/* struct atsc_rrt_dimension_part2 part2 */
} __ucsi_packed;

struct atsc_rrt_dimension_part2 {
  EBIT3(uint8_t reserved			: 3; ,
	uint8_t graduated_scale			: 1; ,
	uint8_t values_defined			: 4; );
	/* struct atsc_rrt_dimension_value values[] */
} __ucsi_packed;

struct atsc_rrt_dimension_value {
	uint8_t abbrev_rating_value_length;
	/* struct atsc_text abbrev_rating_value_text */
	/* struct atsc_rrt_dimension_value_part2 */
} __ucsi_packed;

struct atsc_rrt_dimension_value_part2 {
	uint8_t rating_value_length;
	/* struct atsc_text rating_value_text */
} __ucsi_packed;

struct atsc_rrt_section_part3 {
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;


static inline struct atsc_rrt_dimension *
		atsc_rrt_section_dimensions_first(struct atsc_rrt_section_part2 *part2);
static inline struct atsc_rrt_dimension *
		atsc_rrt_section_dimensions_next(struct atsc_rrt_section_part2 *part2,
		struct atsc_rrt_dimension *pos,
		int idx);
static inline struct atsc_rrt_dimension_value *
		atsc_rrt_dimension_part2_values_first(struct atsc_rrt_dimension_part2 *part2);
static inline struct atsc_rrt_dimension_value *
		atsc_rrt_dimension_part2_values_next(struct atsc_rrt_dimension_part2 *part2,
		struct atsc_rrt_dimension_value *pos,
		int idx);

/**
 * Process a atsc_rrt_section.
 *
 * @param section Pointer to anj atsc_section_psip structure.
 * @return atsc_rrt_section pointer, or NULL on error.
 */
struct atsc_rrt_section *atsc_rrt_section_codec(struct atsc_section_psip *section);

/**
 * Accessor for the rating_region field of an RRT.
 *
 * @param rrt RRT pointer.
 * @return The transport_stream_id.
 */
static inline uint8_t atsc_rrt_section_rating_region(struct atsc_rrt_section *rrt)
{
	return rrt->head.ext_head.table_id_ext & 0xff;
}

/**
 * Accessor for the rating_region_name_text field of an RRT.
 *
 * @param rrt RRT pointer.
 * @return struct atsc_text pointer, or NULL.
 */
static inline struct atsc_text *atsc_rrt_section_rating_region_name_text(struct atsc_rrt_section *rrt)
{
	if (rrt->rating_region_name_length == 0)
		return NULL;

	return (struct atsc_text*)(((uint8_t*) rrt) + sizeof(struct atsc_rrt_section));
}

/**
 * Accessor for the part2 field of an RRT.
 *
 * @param rrt RRT pointer.
 * @return struct atsc_rrt_section_part2 pointer.
 */
static inline struct atsc_rrt_section_part2 *atsc_rrt_section_part2(struct atsc_rrt_section *rrt)
{
	return (struct atsc_rrt_section_part2 *)
		(((uint8_t*) rrt) + sizeof(struct atsc_rrt_section) +
			rrt->rating_region_name_length);
}

/**
 * Iterator for the dimensions field in an atsc_rrt_section_part2.
 *
 * @param rrt atsc_rrt_section pointer.
 * @param pos Variable containing a pointer to the current atsc_rrt_dimension.
 * @param idx Integer used to count which dimension we are in.
 */
#define atsc_rrt_section_dimensions_for_each(rrt, pos, idx) \
	for ((pos) = atsc_rrt_section_dimensions_first(rrt), idx=0; \
	     (pos); \
	     (pos) = atsc_rrt_section_dimensions_next(rrt, pos, ++idx))

/**
 * Accessor for the dimension_name_text field of an atsc_rrt_dimension.
 *
 * @param dimension atsc_rrt_dimension pointer.
 * @return struct atsc_text pointer, or NULL on error.
 */
static inline struct atsc_text *atsc_rrt_dimension_name_text(struct atsc_rrt_dimension *dimension)
{
	if (dimension->dimension_name_length == 0)
		return NULL;

	return (struct atsc_text*)(((uint8_t*) dimension) + sizeof(struct atsc_rrt_dimension));
}

/**
 * Accessor for the part2 field of an atsc_rrt_dimension.
 *
 * @param dimension atsc_rrt_dimension pointer.
 * @return struct atsc_rrt_dimension_part2 pointer.
 */
static inline struct atsc_rrt_dimension_part2 *atsc_rrt_dimension_part2(struct atsc_rrt_dimension *dimension)
{
	return (struct atsc_rrt_dimension_part2 *)
			(((uint8_t*) dimension) +
			sizeof(struct atsc_rrt_dimension) +
			dimension->dimension_name_length);
}

/**
 * Iterator for the values field in a atsc_rrt_dimension_part2 structure.
 *
 * @param part2 atsc_rrt_dimension_part2 pointer.
 * @param pos Variable containing a pointer to the current value.
 * @param idx Integer used to count which value we are in
 */
#define atsc_rrt_dimension_part2_values_for_each(part2, pos, idx) \
	for ((pos) = atsc_rrt_dimension_part2_values_first(part2), idx=0; \
	     (pos); \
	     (pos) = atsc_rrt_dimension_part2_values_next(part2, pos, ++idx))

/**
 * Accessor for the dimension_name_text field of an atsc_rrt_dimension.
 *
 * @param dimension atsc_rrt_dimension pointer.
 * @return struct atsc_text pointer.
 */
static inline struct atsc_text *
	atsc_rrt_dimension_value_abbrev_rating_value_text(struct atsc_rrt_dimension_value *value)
{
	if (value->abbrev_rating_value_length == 0)
		return NULL;

	return (struct atsc_text*)(((uint8_t*) value) + sizeof(struct atsc_rrt_dimension_value));
}

/**
 * Accessor for the part2 field of an atsc_rrt_dimension_value.
 *
 * @param value atsc_rrt_dimension_value pointer.
 * @return struct atsc_rrt_dimension_value_part2 pointer.
 */
static inline struct atsc_rrt_dimension_value_part2 *atsc_rrt_dimension_value_part2(struct atsc_rrt_dimension_value *value)
{
	return (struct atsc_rrt_dimension_value_part2 *)
		(((uint8_t*) value) +
		sizeof(struct atsc_rrt_dimension_value) +
		value->abbrev_rating_value_length);
}

/**
 * Accessor for the rating_value_text field of an atsc_rrt_dimension_value_part2.
 *
 * @param part2 atsc_rrt_dimension_value_part2 pointer.
 * @return struct atsc_text pointer.
 */
static inline struct atsc_text *atsc_rrt_dimension_value_part2_rating_value_text(struct atsc_rrt_dimension_value_part2 *part2)
{
	if (part2->rating_value_length == 0)
		return NULL;

	return (struct atsc_text*)(((uint8_t*) part2) + sizeof(struct atsc_rrt_dimension_value_part2));
}

/**
 * Accessor for the third part of an atsc_rrt_section.
 *
 * @param part2 atsc_rrt_section_part2 pointer.
 * @return atsc_rrt_section_part3 pointer.
 */
static inline struct atsc_rrt_section_part3 *
	atsc_rrt_section_part3(struct atsc_rrt_section_part2 *part2)
{
	int pos = sizeof(struct atsc_rrt_section_part2);

	struct atsc_rrt_dimension *cur_dimension;
	int idx;
	atsc_rrt_section_dimensions_for_each(part2, cur_dimension, idx) {
		pos += sizeof(struct atsc_rrt_dimension);
		pos += cur_dimension->dimension_name_length;
		pos += sizeof(struct atsc_rrt_dimension_part2);

		// now we need to iterate over the values. yuck
		struct atsc_rrt_dimension_part2 *dpart2 = atsc_rrt_dimension_part2(cur_dimension);
		struct atsc_rrt_dimension_value *cur_value;
		int vidx;
		atsc_rrt_dimension_part2_values_for_each(dpart2, cur_value, vidx) {
			pos += sizeof(struct atsc_rrt_dimension_value);
			pos += cur_value->abbrev_rating_value_length;

			struct atsc_rrt_dimension_value_part2 *vpart2 = atsc_rrt_dimension_value_part2(cur_value);
			pos += sizeof(struct atsc_rrt_dimension_value_part2);
			pos += vpart2->rating_value_length;
		}
	}

	return (struct atsc_rrt_section_part3 *) (((uint8_t*) part2) + pos);
}

/**
 * Iterator for the descriptors field in a atsc_rrt_section structure.
 *
 * @param part3 atsc_rrt_section_part3 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_rrt_section_part3_descriptors_for_each(part3, pos) \
	for ((pos) = atsc_rrt_section_part3_descriptors_first(part3); \
	     (pos); \
	     (pos) = atsc_rrt_section_part3_descriptors_next(part3, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct atsc_rrt_dimension *
	atsc_rrt_section_dimensions_first(struct atsc_rrt_section_part2 *part2)
{
	size_t pos = sizeof(struct atsc_rrt_section_part2);

	if (part2->dimensions_defined == 0)
		return NULL;

	return (struct atsc_rrt_dimension*) (((uint8_t *) part2) + pos);
}

static inline struct atsc_rrt_dimension *
	atsc_rrt_section_dimensions_next(struct atsc_rrt_section_part2 *part2,
					 struct atsc_rrt_dimension *pos,
					 int idx)
{
	if (idx >= part2->dimensions_defined)
		return NULL;

	struct atsc_rrt_dimension_part2 *dpart2 = atsc_rrt_dimension_part2(pos);
	int len = sizeof(struct atsc_rrt_dimension_part2);

	// now we need to iterate over the values. yuck
	struct atsc_rrt_dimension_value *cur_value;
	int vidx;
	atsc_rrt_dimension_part2_values_for_each(dpart2, cur_value, vidx) {
		len += sizeof(struct atsc_rrt_dimension_value);
		len += cur_value->abbrev_rating_value_length;

		struct atsc_rrt_dimension_value_part2 *vpart2 = atsc_rrt_dimension_value_part2(cur_value);
		len += sizeof(struct atsc_rrt_dimension_value_part2);
		len += vpart2->rating_value_length;
	}

	return (struct atsc_rrt_dimension *) (((uint8_t*) dpart2) + len);
}

static inline struct atsc_rrt_dimension_value *
	atsc_rrt_dimension_part2_values_first(struct atsc_rrt_dimension_part2 *part2)
{
	size_t pos = sizeof(struct atsc_rrt_dimension_part2);

	if (part2->values_defined == 0)
		return NULL;

	return (struct atsc_rrt_dimension_value*) (((uint8_t *) part2) + pos);
}

static inline struct atsc_rrt_dimension_value *
	atsc_rrt_dimension_part2_values_next(struct atsc_rrt_dimension_part2 *part2,
					     struct atsc_rrt_dimension_value *pos,
					     int idx)
{
	if (idx >= part2->values_defined)
		return NULL;

	struct atsc_rrt_dimension_value_part2 *vpart2 = atsc_rrt_dimension_value_part2(pos);
	int len = sizeof(struct atsc_rrt_dimension_value_part2);
	len += vpart2->rating_value_length;

	return (struct atsc_rrt_dimension_value *) (((uint8_t*) vpart2) + len);
}

static inline struct descriptor *
	atsc_rrt_section_part3_descriptors_first(struct atsc_rrt_section_part3 *part3)
{
	size_t pos = sizeof(struct atsc_rrt_section_part3);

	if (part3->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part3) + pos);
}

static inline struct descriptor *
	atsc_rrt_section_part3_descriptors_next(struct atsc_rrt_section_part3 *part3,
						struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part3 + sizeof(struct atsc_rrt_section_part3),
				part3->descriptors_length,
				pos);
}

#ifdef __cplusplus
}
#endif

#endif
