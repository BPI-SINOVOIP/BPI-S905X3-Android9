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

#ifndef _UCSI_MPEG_ISO_639_LANGUAGE_DESCRIPTOR
#define _UCSI_MPEG_ISO_639_LANGUAGE_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * Possible values for audio_type.
 */
enum {
	MPEG_AUDIO_TYPE_CLEAN_EFFECTS 			= 0x01,
	MPEG_AUDIO_TYPE_HEARING_IMPAIRED 		= 0x02,
	MPEG_AUDIO_TYPE_VISUAL_IMPAIRED_COMMENTARY 	= 0x03,
};

/**
 * mpeg_iso_639_language_descriptor structure.
 */
struct mpeg_iso_639_language_descriptor {
	struct descriptor d;

	/* struct mpeg_iso_639_language_code languages[] */
} __ucsi_packed;

/**
 * An entry in the mpeg_iso_639_language_descriptor languages field.
 */
struct mpeg_iso_639_language_code {
	iso639lang_t language_code;
	uint8_t audio_type;
} __ucsi_packed;

/**
 * Process an mpeg_iso_639_language_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return Pointer to an mpeg_iso_639_language_descriptor structure, or NULL
 * on error.
 */
static inline struct mpeg_iso_639_language_descriptor*
	mpeg_iso_639_language_descriptor_codec(struct descriptor* d)
{
	if (d->len % sizeof(struct mpeg_iso_639_language_code))
		return NULL;

	return (struct mpeg_iso_639_language_descriptor*) d;
}

/**
 * Convenience iterator for the languages field of an mpeg_iso_639_language_descriptor
 *
 * @param d Pointer to the mpeg_iso_639_language_descriptor structure.
 * @param pos Variable holding a pointer to the current entry.
 */
#define mpeg_iso_639_language_descriptor_languages_for_each(_d, _pos) \
	for ((_pos) = mpeg_iso_639_language_descriptor_languages_first(_d); \
	     (_pos); \
	     (_pos) = mpeg_iso_639_language_descriptor_languages_next(_d, _pos))










/******************************** PRIVATE CODE ********************************/
static inline struct mpeg_iso_639_language_code*
	mpeg_iso_639_language_descriptor_languages_first(struct mpeg_iso_639_language_descriptor *d)
{
	if (d->d.len < sizeof(struct mpeg_iso_639_language_code))
		return NULL;

	return (struct mpeg_iso_639_language_code *)
		((uint8_t*) d + sizeof(struct mpeg_iso_639_language_descriptor));
}

static inline struct mpeg_iso_639_language_code*
	mpeg_iso_639_language_descriptor_languages_next(struct mpeg_iso_639_language_descriptor *d,
						        struct mpeg_iso_639_language_code *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct mpeg_iso_639_language_code);

	if (next >= end)
		return NULL;

	return (struct mpeg_iso_639_language_code *) next;
}

#ifdef __cplusplus
}
#endif

#endif
