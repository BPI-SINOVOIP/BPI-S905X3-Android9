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

#ifndef _UCSI_ATSC_DCCT_SECTION_H
#define _UCSI_ATSC_DCCT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>

enum atsc_dcc_context {
	ATSC_DCC_CONTEXT_TEMPORARY_RETUNE = 0,
	ATSC_DCC_CONTEXT_CHANNEL_REDIRECT = 1,
};

enum atsc_dcc_selection_type {
	ATSC_DCC_SELECTION_UNCONDITIONAL_CHANNEL_CHANGE = 0x00,
	ATSC_DCC_SELECTION_NUMERIC_POSTAL_CODE_INCLUSION = 0x01,
	ATSC_DCC_SELECTION_ALPHANUMERIC_POSTAL_CODE_INCLUSION = 0x02,
	ATSC_DCC_SELECTION_DEMOGRAPHIC_CATEGORY_ONE_OR_MORE = 0x05,
	ATSC_DCC_SELECTION_DEMOGRAPHIC_CATEGORY_ALL = 0x06,
	ATSC_DCC_SELECTION_GENRE_CATEGORY_ONE_OR_MORE = 0x07,
	ATSC_DCC_SELECTION_GENRE_CATEGORY_ALL = 0x08,
	ATSC_DCC_SELECTION_CANNOT_BE_AUTHORIZED = 0x09,
	ATSC_DCC_SELECTION_GEOGRAPHIC_LOCATION_INCLUSION = 0x0c,
	ATSC_DCC_SELECTION_RATING_BLOCKED = 0x0d,
	ATSC_DCC_SELECTION_RETURN_TO_ORIGINAL_CHANNEL = 0x0f,
	ATSC_DCC_SELECTION_NUMERIC_POSTAL_CODE_EXCLUSION = 0x11,
	ATSC_DCC_SELECTION_ALPHANUMERIC_POSTAL_CODE_EXCLUSION = 0x12,
	ATSC_DCC_SELECTION_DEMOGRAPHIC_CATEGORY_NOT_ONE_OR_MORE = 0x15,
	ATSC_DCC_SELECTION_DEMOGRAPHIC_CATEGORY_NOT_ALL = 0x16,
	ATSC_DCC_SELECTION_GENRE_CATEGORY_NOT_ONE_OR_MORE = 0x17,
	ATSC_DCC_SELECTION_GENRE_CATEGORY_NOT_ALL = 0x18,
	ATSC_DCC_SELECTION_GEOGRAPHIC_LOCATION_EXCLUSION = 0x1c,
	ATSC_DCC_SELECTION_VIEWER_DIRECT_SELECT_A = 0x20,
	ATSC_DCC_SELECTION_VIEWER_DIRECT_SELECT_B = 0x21,
	ATSC_DCC_SELECTION_VIEWER_DIRECT_SELECT_C = 0x22,
	ATSC_DCC_SELECTION_VIEWER_DIRECT_SELECT_D = 0x23,
};

/**
 * atsc_dcct_section structure.
 */
struct atsc_dcct_section {
	struct atsc_section_psip head;

	uint8_t dcc_test_count;
	/* struct atsc_dcct_test tests */
	/* struct atsc_dcct_section_part2 part2 */
} __ucsi_packed;

struct atsc_dcct_test {
  EBIT4(uint32_t dcc_context			: 1; ,
	uint32_t reserved			: 3; ,
	uint32_t dcc_from_major_channel_number	:10; ,
	uint32_t dcc_from_minor_channel_number	:10; );
  EBIT3(uint32_t reserved1			: 4; ,
	uint32_t dcc_to_major_channel_number	:10; ,
	uint32_t dcc_to_minor_channel_number	:10; );
	atsctime_t start_time;
	atsctime_t end_time;
	uint8_t dcc_term_count;
	/* struct atsc_dcct_term terms */
	/* struct atsc_dcct_test_part2 part2 */
} __ucsi_packed;

struct atsc_dcct_term {
	uint8_t dcc_selection_type;
	uint64_t dcc_selection_id;
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

struct atsc_dcct_test_part2 {
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

struct atsc_dcct_section_part2 {
  EBIT2(uint16_t reserved			: 6; ,
	uint16_t descriptors_length		:10; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;

static inline struct atsc_dcct_test *
	atsc_dcct_section_tests_first(struct atsc_dcct_section *dcct);
static inline struct atsc_dcct_test *
	atsc_dcct_section_tests_next(struct atsc_dcct_section *dcct,
				     struct atsc_dcct_test *pos,
				     int idx);
static inline struct atsc_dcct_term *
	atsc_dcct_test_terms_first(struct atsc_dcct_test *test);
static inline struct atsc_dcct_term *
	atsc_dcct_test_terms_next(struct atsc_dcct_test *test,
				  struct atsc_dcct_term *pos,
				  int idx);


/**
 * Process an atsc_dcct_section.
 *
 * @param section Pointer to an atsc_section_psip structure.
 * @return atsc_dcct_section pointer, or NULL on error.
 */
struct atsc_dcct_section *atsc_dcct_section_codec(struct atsc_section_psip *section);

/**
 * Accessor for the dcc_subtype field of a dcct.
 *
 * @param dcct dcct pointer.
 * @return The dcc_subtype.
 */
static inline uint8_t atsc_dcct_section_dcc_subtype(struct atsc_dcct_section *dcct)
{
	return dcct->head.ext_head.table_id_ext >> 8;
}

/**
 * Accessor for the dcc_id field of a dcct.
 *
 * @param dcct dcct pointer.
 * @return The dcc_id.
 */
static inline uint8_t atsc_dcct_section_dcc_id(struct atsc_dcct_section *dcct)
{
	return dcct->head.ext_head.table_id_ext & 0xff;
}

/**
 * Iterator for the tests field in an atsc_dcct_section.
 *
 * @param dcct atsc_dcct_section pointer.
 * @param pos Variable containing a pointer to the current atsc_dcct_test.
 * @param idx Integer used to count which test we are in.
 */
#define atsc_dcct_section_tests_for_each(dcct, pos, idx) \
	for ((pos) = atsc_dcct_section_tests_first(dcct), idx=0; \
	     (pos); \
	     (pos) = atsc_dcct_section_tests_next(dcct, pos, ++idx))

/**
 * Iterator for the terms field in an atsc_dcct_test.
 *
 * @param test atsc_dcct_test pointer.
 * @param pos Variable containing a pointer to the current atsc_dcct_term.
 * @param idx Integer used to count which test we are in.
 */
#define atsc_dcct_test_terms_for_each(test, pos, idx) \
	for ((pos) = atsc_dcct_test_terms_first(test), idx=0; \
	     (pos); \
	     (pos) = atsc_dcct_test_terms_next(test, pos, ++idx))

/**
 * Iterator for the descriptors field in a atsc_dcct_term structure.
 *
 * @param term atsc_dcct_term pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_dcct_term_descriptors_for_each(term, pos) \
	for ((pos) = atsc_dcct_term_descriptors_first(term); \
	     (pos); \
	     (pos) = atsc_dcct_term_descriptors_next(term, pos))

/**
 * Accessor for the part2 field of an atsc_dcct_test.
 *
 * @param test atsc_dcct_test pointer.
 * @return struct atsc_dcct_test_part2 pointer.
 */
static inline struct atsc_dcct_test_part2 *atsc_dcct_test_part2(struct atsc_dcct_test *test)
{
	int pos = sizeof(struct atsc_dcct_test);

	struct atsc_dcct_term *cur_term;
	int idx;
	atsc_dcct_test_terms_for_each(test, cur_term, idx) {
		pos += sizeof(struct atsc_dcct_term);
		pos += cur_term->descriptors_length;
	}

	return (struct atsc_dcct_test_part2 *) (((uint8_t*) test) + pos);
}

/**
 * Iterator for the descriptors field in a atsc_dcct_test_part2 structure.
 *
 * @param term atsc_dcct_test_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_dcct_test_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_dcct_test_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_dcct_test_part2_descriptors_next(part2, pos))

/**
 * Accessor for the part2 field of an atsc_dcct_section.
 *
 * @param dcct atsc_dcct_section pointer.
 * @return struct atsc_dcct_section_part2 pointer.
 */
static inline struct atsc_dcct_section_part2 *atsc_dcct_section_part2(struct atsc_dcct_section *dcct)
{
	int pos = sizeof(struct atsc_dcct_section);

	struct atsc_dcct_test *cur_test;
	int testidx;
	atsc_dcct_section_tests_for_each(dcct, cur_test, testidx) {
		struct atsc_dcct_test_part2 *part2 = atsc_dcct_test_part2(cur_test);
		pos += ((uint8_t*) part2 - (uint8_t*) cur_test);

		pos += sizeof(struct atsc_dcct_test_part2);
		pos += part2->descriptors_length;
	}

	return (struct atsc_dcct_section_part2 *) (((uint8_t*) dcct) + pos);
}

/**
 * Iterator for the descriptors field in a atsc_dcct_section_part2 structure.
 *
 * @param part2 atsc_dcct_section_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_dcct_section_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_dcct_section_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_dcct_section_part2_descriptors_next(part2, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct atsc_dcct_test *
	atsc_dcct_section_tests_first(struct atsc_dcct_section *dcct)
{
	size_t pos = sizeof(struct atsc_dcct_section);

	if (dcct->dcc_test_count == 0)
		return NULL;

	return (struct atsc_dcct_test*) (((uint8_t *) dcct) + pos);
}

static inline struct atsc_dcct_test *
	atsc_dcct_section_tests_next(struct atsc_dcct_section *dcct,
				     struct atsc_dcct_test *pos,
				     int idx)
{
	if (idx >= dcct->dcc_test_count)
		return NULL;

	struct atsc_dcct_test_part2 *part2 = atsc_dcct_test_part2(pos);
	int len = sizeof(struct atsc_dcct_test_part2);
	len += part2->descriptors_length;

	return (struct atsc_dcct_test *) (((uint8_t*) part2) + len);
}

static inline struct atsc_dcct_term *
	atsc_dcct_test_terms_first(struct atsc_dcct_test *test)
{
	size_t pos = sizeof(struct atsc_dcct_test);

	if (test->dcc_term_count == 0)
		return NULL;

	return (struct atsc_dcct_term*) (((uint8_t *) test) + pos);
}

static inline struct atsc_dcct_term *
	atsc_dcct_test_terms_next(struct atsc_dcct_test *test,
				  struct atsc_dcct_term *pos,
				  int idx)
{
	if (idx >= test->dcc_term_count)
		return NULL;

	int len = sizeof(struct atsc_dcct_term);
	len += pos->descriptors_length;

	return (struct atsc_dcct_term *) (((uint8_t*) pos) + len);
}

static inline struct descriptor *
	atsc_dcct_term_descriptors_first(struct atsc_dcct_term *term)
{
	size_t pos = sizeof(struct atsc_dcct_term);

	if (term->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) term) + pos);
}

static inline struct descriptor *
	atsc_dcct_term_descriptors_next(struct atsc_dcct_term *term,
					struct descriptor *pos)
{
	return next_descriptor((uint8_t*) term + sizeof(struct atsc_dcct_term),
				term->descriptors_length,
				pos);
}

static inline struct descriptor *
	atsc_dcct_test_part2_descriptors_first(struct atsc_dcct_test_part2 *part2)
{
	size_t pos = sizeof(struct atsc_dcct_test_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_dcct_test_part2_descriptors_next(struct atsc_dcct_test_part2 *part2,
					      struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_dcct_test_part2),
				part2->descriptors_length,
				pos);
}

static inline struct descriptor *
	atsc_dcct_section_part2_descriptors_first(struct atsc_dcct_section_part2 *part2)
{
	size_t pos = sizeof(struct atsc_dcct_section_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_dcct_section_part2_descriptors_next(struct atsc_dcct_section_part2 *part2,
						 struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_dcct_section_part2),
				part2->descriptors_length,
				pos);
}


#ifdef __cplusplus
}
#endif

#endif
