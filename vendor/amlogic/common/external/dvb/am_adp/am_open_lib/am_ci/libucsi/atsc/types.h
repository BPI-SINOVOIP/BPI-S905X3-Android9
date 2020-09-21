	/*
 * section and descriptor parser
 *
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

#ifndef _UCSI_ATSC_TYPES_H
#define _UCSI_ATSC_TYPES_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <time.h>
#include <libucsi/types.h>

enum atsc_vct_modulation {
	ATSC_VCT_MODULATION_ANALOG 	= 0x01,
	ATSC_VCT_MODULATION_SCTE_MODE1 	= 0x02,
	ATSC_VCT_MODULATION_SCTE_MODE2 	= 0x03,
	ATSC_VCT_MODULATION_8VSB 	= 0x04,
	ATSC_VCT_MODULATION_16VSB 	= 0x05,
};

enum atsc_vct_service_type {
	ATSC_VCT_SERVICE_TYPE_ANALOG 	= 0x01,
	ATSC_VCT_SERVICE_TYPE_TV 	= 0x02,
	ATSC_VCT_SERVICE_TYPE_AUDIO 	= 0x03,
	ATSC_VCT_SERVICE_TYPE_DATA 	= 0x04,
};

enum atsc_etm_location {
	ATSC_VCT_ETM_NONE	 		= 0x00,
	ATSC_VCT_ETM_IN_THIS_PTC 		= 0x01,
	ATSC_VCT_ETM_IN_CHANNEL_TSID 		= 0x02,
};

enum atsc_text_compress_type {
	ATSC_TEXT_COMPRESS_NONE			= 0x00,
	ATSC_TEXT_COMPRESS_PROGRAM_TITLE	= 0x01,
	ATSC_TEXT_COMPRESS_PROGRAM_DESCRIPTION	= 0x02,
};

enum atsc_text_segment_mode {
	ATSC_TEXT_SEGMENT_MODE_UNICODE_RANGE_MIN	= 0x00,
	ATSC_TEXT_SEGMENT_MODE_UNICODE_RANGE_MAX	= 0x33,
	ATSC_TEXT_SEGMENT_MODE_SCSU			= 0x3e,
	ATSC_TEXT_SEGMENT_MODE_UTF16			= 0x3f,
	ATSC_TEXT_SEGMENT_MODE_TAIWAN_BITMAP		= 0x40,
	ATSC_TEXT_SEGMENT_MODE_TAIWAN_CODEWORD_BITMAP	= 0x41,
};

typedef uint32_t atsctime_t;

struct atsc_text {
	uint8_t number_strings;
	/* struct atsc_text_string strings[] */
};

struct atsc_text_string {
	iso639lang_t language_code;
	uint8_t number_segments;
	/* struct atsc_text_string_segment segments[] */
};

struct atsc_text_string_segment {
	uint8_t compression_type;
	uint8_t mode;
	uint8_t number_bytes;
	/* uint8_t bytes[] */
};

/**
 * Iterator for strings field of an atsc_text structure.
 *
 * @param txt atsc_text pointer.
 * @param pos Variable holding a pointer to the current atsc_text_string.
 * @param idx Iterator variable.
 */
#define atsc_text_strings_for_each(txt, pos, idx) \
	for ((pos) = atsc_text_strings_first(txt), idx=0; \
	     (pos); \
	     (pos) = atsc_text_strings_next(txt, pos, ++idx))

/**
 * Iterator for segments field of an atsc_text_string structure.
 *
 * @param str atsc_text_string pointer.
 * @param pos Variable holding a pointer to the current atsc_text_string_segment.
 * @param idx Iterator variable.
 */
#define atsc_text_string_segments_for_each(str, pos, idx) \
	for ((pos) = atsc_text_string_segments_first(str), idx=0; \
	     (pos); \
	     (pos) = atsc_text_string_segments_next(str, pos, ++idx))

/**
 * Accessor for the bytes field of an atsc_text_string_segment.
 *
 * @param seg atsc_text_string_segment pointer.
 * @return Pointer to the bytes.
 */
static inline uint8_t*
	atsc_text_string_segment_bytes(struct atsc_text_string_segment *d)
{
	return ((uint8_t*) d) + sizeof(struct atsc_text_string_segment);
}

/**
 * Validate a buffer containing an atsc_text structure.
 *
 * @param buf Start of the atsc_text structure.
 * @param len Length in bytes of the buffer.
 * @return 0 if valid, nonzero if not.
 */
extern int atsc_text_validate(uint8_t *buf, int len);

/**
 * Decodes an atsc_text_segment with mode < 0x3e. Decompression of the ATSC text encoding IS
 * supported. The output text will be in the UTF-8 encoding.
 *
 * @param segment Pointer to the segment to decode.
 * @param destbuf Pointer to the malloc()ed buffer to append text to (pass NULL if none).
 * @param destbufsize Size of destbuf in bytes.
 * @param destbufpos Position within destbuf. This will be updated to point after the end of the
 * string on exit.
 * @return New value of destbufpos, or < 0 on error.
 */
extern int atsc_text_segment_decode(struct atsc_text_string_segment *segment,
				    uint8_t **destbuf, size_t *destbufsize, size_t *destbufpos);

/**
 * Convert from ATSC time to unix time_t.
 *
 * @param atsc ATSC time.
 * @return The time value.
 */
extern time_t atsctime_to_unixtime(atsctime_t atsc);

/**
 * Convert from unix time_t to atsc time.
 *
 * @param t unix time_t.
 * @return The atsc time value.
 */
extern atsctime_t unixtime_to_atsctime(time_t t);







/******************************** PRIVATE CODE ********************************/
static inline struct atsc_text_string*
	atsc_text_strings_first(struct atsc_text *txt)
{
	if (txt->number_strings == 0)
		return NULL;

	return (struct atsc_text_string *)
		((uint8_t*) txt + sizeof(struct atsc_text));
}

static inline struct atsc_text_string*
	atsc_text_strings_next(struct atsc_text *txt, struct atsc_text_string *pos, int idx)
{
	int i;
	uint8_t *buf;

	if (idx >= txt->number_strings)
		return NULL;

	buf = ((uint8_t*) pos) + sizeof(struct atsc_text_string);
	for(i=0; i < pos->number_segments; i++) {
		struct atsc_text_string_segment *seg =
			(struct atsc_text_string_segment *) buf;

		buf += sizeof(struct atsc_text_string_segment);
		buf += seg->number_bytes;
	}

	return (struct atsc_text_string *) buf;
}

static inline struct atsc_text_string_segment*
	atsc_text_string_segments_first(struct atsc_text_string *str)
{
	if (str->number_segments == 0)
		return NULL;

	return (struct atsc_text_string_segment *)
		((uint8_t*) str + sizeof(struct atsc_text_string));
}

static inline struct atsc_text_string_segment*
	atsc_text_string_segments_next(struct atsc_text_string *str,
				       struct atsc_text_string_segment *pos, int idx)
{
	if (idx >= str->number_segments)
		return NULL;

	return (struct atsc_text_string_segment *)
		(((uint8_t*) pos) + sizeof(struct atsc_text_string_segment) + pos->number_bytes);
}

#ifdef __cplusplus
}
#endif

#endif
