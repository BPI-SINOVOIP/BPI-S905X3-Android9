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

#ifndef _UCSI_MPEG_PAT_SECTION_H
#define _UCSI_MPEG_PAT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * mpeg_pat_section structure.
 */
struct mpeg_pat_section {
	struct section_ext head; /* table_id_ext == transport_stream_id */

	/* struct mpeg_pat_program programs[] */
} __ucsi_packed;

/**
 * A program within an mpeg_pat_section.
 */
struct mpeg_pat_program {
	uint16_t program_number;
  EBIT2(uint16_t reserved	: 3; ,
	uint16_t pid		:13; );
} __ucsi_packed;

/**
 * Process an mpeg_pat_section.
 *
 * @param section Pointer to the generic section_ext structure.
 * @return Pointer to the mpeg_pat_section structure, or NULL on error.
 */
extern struct mpeg_pat_section *mpeg_pat_section_codec(struct section_ext *section);

/**
 * Accessor for the transport_stream_id field of a PAT.
 *
 * @param pat PAT pointer.
 * @return The transport_stream_id.
 */
static inline uint16_t mpeg_pat_section_transport_stream_id(struct mpeg_pat_section *pat)
{
	return pat->head.table_id_ext;
}

/**
 * Conveience iterator for the programs field of an mpeg_pat_section.
 *
 * @param pat Pointer to the mpeg_pat_section structure.
 * @param pos Variable holding a pointer to the current mpeg_pat_program structure.
 */
#define mpeg_pat_section_programs_for_each(pat, pos) \
	for ((pos) = mpeg_pat_section_programs_first(pat); \
	     (pos); \
	     (pos) = mpeg_pat_section_programs_next(pat, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct mpeg_pat_program *
	mpeg_pat_section_programs_first(struct mpeg_pat_section * pat)
{
	size_t pos = sizeof(struct mpeg_pat_section);

	if (pos >= section_ext_length(&pat->head))
		return NULL;

	return (struct mpeg_pat_program*)((uint8_t *) pat + pos);
}

static inline
	struct mpeg_pat_program *mpeg_pat_section_programs_next(struct mpeg_pat_section * pat,
								struct mpeg_pat_program * pos)
{
	uint8_t *end = (uint8_t*) pat + section_ext_length(&pat->head);
	uint8_t *next= (uint8_t *) pos + sizeof(struct mpeg_pat_program);

	if (next >= end)
		return NULL;

	return (struct mpeg_pat_program *) next;
}

#ifdef __cplusplus
}
#endif

#endif
