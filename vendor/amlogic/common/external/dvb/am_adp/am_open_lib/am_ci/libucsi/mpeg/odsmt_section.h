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

#ifndef _UCSI_MPEG_ODSMT_SECTION_H
#define _UCSI_MPEG_ODSMT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * mpeg_odsmt_section structure.
 */
struct mpeg_odsmt_section {
	struct section_ext head;

	uint8_t stream_count;
	/* stream_count==0 => struct mpeg_odsmt_stream_single streams
	   stream_count>0  => struct mpeg_odsmt_stream_multi streams[] */
	/* uint8_t object_descriptors[] */
} __ucsi_packed;

struct mpeg_odsmt_stream_single
{
	uint16_t esid;
	uint8_t es_info_length;
	/* struct descriptor descriptors[] */
} __ucsi_packed;

struct mpeg_odsmt_stream_multi
{
	uint16_t esid;
	uint8_t fmc;
	uint8_t es_info_length;
	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Structure describing the stream information held in an mpeg_odsmt_section.
 */
struct mpeg_odsmt_stream {
	union {
		struct mpeg_odsmt_stream_single single;
		struct mpeg_odsmt_stream_multi multi;
	} u;
} __ucsi_packed;

/**
 * Process an mpeg_odsmt_section.
 *
 * @param section Pointer to the generic section_ext structure.
 * @return Pointer to a mpeg_odsmt_section structure, or NULL on error.
 */
extern struct mpeg_odsmt_section *mpeg_odsmt_section_codec(struct section_ext *section);

/**
 * Accessor for the PID field of an ODSMT.
 *
 * @param odsmt odsmt pointer.
 * @return The pid.
 */
static inline uint16_t mpeg_odsmt_section_pid(struct mpeg_odsmt_section *odsmt)
{
	return odsmt->head.table_id_ext & 0x1fff;
}

/**
 * Convenience iterator for the streams field of an mpeg_odsmt_section.
 *
 * @param osdmt Pointer to the mpeg_odsmt_section structure.
 * @param pos Variable holding pointer to the current mpeg_odsmt_stream structure.
 * @param index Variable holding the stream index.
 */
#define mpeg_odsmt_section_streams_for_each(osdmt, pos, index) \
	for (index=0, (pos) = mpeg_odsmt_section_streams_first(odsmt); \
	     (pos); \
	     (pos) = mpeg_odsmt_section_streams_next(odsmt, pos, ++index))

/**
 * Convenience iterator for the descriptors field of an mpeg_odsmt_stream.
 *
 * @param osdmt Pointer to the mpeg_odsmt_section structure.
 * @param stream Pointer to the mpeg_odsmt_stream structure.
 * @param pos Variable holding pointer to the current descriptor structure.
 */
#define mpeg_odsmt_stream_descriptors_for_each(osdmt, stream, pos) \
	for ((pos) = mpeg_odsmt_stream_descriptors_first(odsmt, stream); \
	     (pos); \
	     (pos) = mpeg_odsmt_stream_descriptors_next(odsmt, stream, pos))

/**
 * Retrieve a pointer to the object_descriptors field of an mpeg_odsmt_section.
 *
 * @param osdmt Pointer to the mpeg_odsmt_section structure.
 * @param len On return, will contain the number of bytes in the object descriptors field.
 * @return Pointer to the object_descriptors field, or NULL on error.
 */
static inline uint8_t*
	mpeg_odsmt_section_object_descriptors(struct mpeg_odsmt_section * odsmt,
					      size_t* len);










/******************************** PRIVATE CODE ********************************/
static inline struct mpeg_odsmt_stream *
	mpeg_odsmt_section_streams_first(struct mpeg_odsmt_section *odsmt)
{
	size_t pos = sizeof(struct mpeg_odsmt_section);

	if (pos >= section_ext_length(&odsmt->head))
		return NULL;

	return (struct mpeg_odsmt_stream *) ((uint8_t *) odsmt + pos);
}

static inline struct mpeg_odsmt_stream *
	mpeg_odsmt_section_streams_next(struct mpeg_odsmt_section *odsmt,
					struct mpeg_odsmt_stream *pos,
				        int _index)
{
	uint8_t *end = (uint8_t*) odsmt + section_ext_length(&odsmt->head);
	uint8_t *next;

	if (_index > odsmt->stream_count)
		return NULL;

	next = (uint8_t *) pos + sizeof(struct mpeg_odsmt_stream_multi) +
		pos->u.multi.es_info_length;

	if (next >= end)
		return NULL;

	return (struct mpeg_odsmt_stream *) next;
}

static inline struct descriptor *
	mpeg_odsmt_stream_descriptors_first(struct mpeg_odsmt_section *odsmt,
							struct mpeg_odsmt_stream *stream)
{
	if (odsmt->stream_count == 0) {
		if (stream->u.single.es_info_length == 0)
			return NULL;

		return (struct descriptor *)
			((uint8_t*) stream + sizeof(struct mpeg_odsmt_stream_single));
	} else {
		if (stream->u.multi.es_info_length == 0)
			return NULL;

		return (struct descriptor *)
			((uint8_t*) stream + sizeof(struct mpeg_odsmt_stream_multi));
	}
}

static inline struct descriptor *
	mpeg_odsmt_stream_descriptors_next(struct mpeg_odsmt_section *odsmt,
					   struct mpeg_odsmt_stream *stream,
					   struct descriptor* pos)
{
	if (odsmt->stream_count == 0) {
		return next_descriptor((uint8_t *) stream + sizeof(struct mpeg_odsmt_stream_single),
				       stream->u.single.es_info_length,
				       pos);
	} else {
		return next_descriptor((uint8_t *) stream + sizeof(struct mpeg_odsmt_stream_multi),
				       stream->u.multi.es_info_length,
				       pos);
	}
}

static inline uint8_t*
	mpeg_odsmt_section_object_descriptors(struct mpeg_odsmt_section * odsmt,
					      size_t* len)
{
	struct mpeg_odsmt_stream* pos;
	size_t size = sizeof(struct mpeg_odsmt_section);
	int _index;

	mpeg_odsmt_section_streams_for_each(odsmt, pos, _index) {
		if (odsmt->stream_count == 0)
			size += sizeof(struct mpeg_odsmt_stream_single) +
					pos->u.single.es_info_length;
		else
			size += sizeof(struct mpeg_odsmt_stream_multi) +
					pos->u.multi.es_info_length;
	}

	*len = section_ext_length(&odsmt->head) - size;
	return (uint8_t*) odsmt + size;
}

#ifdef __cplusplus
}
#endif

#endif
