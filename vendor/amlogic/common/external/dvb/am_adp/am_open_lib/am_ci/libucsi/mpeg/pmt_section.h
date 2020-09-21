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

#ifndef _UCSI_MPEG_PMT_SECTION_H
#define _UCSI_MPEG_PMT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * mpeg_pmt_section structure.
 */
struct mpeg_pmt_section {
	struct section_ext head;

  EBIT2(uint16_t reserved_1		: 3; ,
	uint16_t pcr_pid		:13; );
  EBIT2(uint16_t reserved_2		: 4; ,
	uint16_t program_info_length	:12; );
	/* struct descriptor descriptors[] */
	/* struct mpeg_pmt_stream streams[] */
} __ucsi_packed;

/**
 * A stream within an mpeg_pmt_section.
 */
struct mpeg_pmt_stream {
	uint8_t stream_type;
  EBIT2(uint16_t reserved_1		: 3; ,
	uint16_t pid			:13; );
  EBIT2(uint16_t reserved_2		: 4; ,
	uint16_t es_info_length		:12; );

	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process an mpeg_pmt_section section.
 *
 * @param section Pointer to the generic section header.
 * @return Pointer to the mpeg_pmt_section structure, or NULL on error.
 */
extern struct mpeg_pmt_section *mpeg_pmt_section_codec(struct section_ext *section);

/**
 * Accessor for program_number field of a PMT.
 *
 * @param pmt PMT pointer.
 * @return The program_number.
 */
static inline uint16_t mpeg_pmt_section_program_number(struct mpeg_pmt_section *pmt)
{
	return pmt->head.table_id_ext;
}

/**
 * Convenience iterator for the descriptors field of the mpeg_pmt_section structure.
 *
 * @param pmt Pointer to the mpeg_pmt_section structure.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define mpeg_pmt_section_descriptors_for_each(pmt, pos) \
	for ((pos) = mpeg_pmt_section_descriptors_first(pmt); \
	     (pos); \
	     (pos) = mpeg_pmt_section_descriptors_next(pmt, pos))

/**
 * Convenience iterator for the streams field of the mpeg_pmt_section structure.
 *
 * @param pmt Pointer to the mpeg_pmt_section structure.
 * @param pos Variable holding a pointer to the current mpeg_pmt_stream.
 */
#define mpeg_pmt_section_streams_for_each(pmt, pos) \
	for ((pos) = mpeg_pmt_section_streams_first(pmt); \
	     (pos); \
	     (pos) = mpeg_pmt_section_streams_next(pmt, pos))

/**
 * Convenience iterator for the descriptors field of an mpeg_pmt_stream structure.
 *
 * @param stream Pointer to the mpeg_pmt_stream structure.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define mpeg_pmt_stream_descriptors_for_each(stream, pos) \
	for ((pos) = mpeg_pmt_stream_descriptors_first(stream); \
	     (pos); \
	     (pos) = mpeg_pmt_stream_descriptors_next(stream, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	mpeg_pmt_section_descriptors_first(struct mpeg_pmt_section * pmt)
{
	if (pmt->program_info_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) pmt + sizeof(struct mpeg_pmt_section));
}

static inline struct descriptor *
	mpeg_pmt_section_descriptors_next(struct mpeg_pmt_section *pmt,
					  struct descriptor* pos)
{
	return next_descriptor((uint8_t *) pmt + sizeof(struct mpeg_pmt_section),
			       pmt->program_info_length,
			       pos);
}

static inline struct mpeg_pmt_stream *
	mpeg_pmt_section_streams_first(struct mpeg_pmt_section * pmt)
{
	size_t pos = sizeof(struct mpeg_pmt_section) + pmt->program_info_length;

	if (pos >= section_ext_length(&pmt->head))
		return NULL;

	return (struct mpeg_pmt_stream *)((uint8_t *)pmt + pos);
}

static inline struct mpeg_pmt_stream *
	mpeg_pmt_section_streams_next(struct mpeg_pmt_section * pmt,
				      struct mpeg_pmt_stream * pos)
{
	uint8_t *end = (uint8_t*) pmt + section_ext_length(&pmt->head);
	uint8_t *next =	(uint8_t *) pos + sizeof(struct mpeg_pmt_stream) +
			pos->es_info_length;

	if (next >= end)
		return NULL;

	return (struct mpeg_pmt_stream *) next;
}

static inline struct descriptor *
	mpeg_pmt_stream_descriptors_first(struct mpeg_pmt_stream *stream)
{
	if (stream->es_info_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t*) stream + sizeof(struct mpeg_pmt_stream));
}

static inline struct descriptor *
	mpeg_pmt_stream_descriptors_next(struct mpeg_pmt_stream *stream,
					 struct descriptor* pos)
{
	return next_descriptor((uint8_t *) stream + sizeof(struct mpeg_pmt_stream),
			       stream->es_info_length,
			       pos);
}

#ifdef __cplusplus
}
#endif

#endif
