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

#ifndef _UCSI_MPEG_FMC_DESCRIPTOR
#define _UCSI_MPEG_FMC_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * mpeg_fmc_descriptor structure.
 */
struct mpeg_fmc_descriptor {
	struct descriptor d;

	/* struct mpeg_flex_mux muxes[] */
} __ucsi_packed;

/**
 * An entry in the muxes field of an mpeg_fmc_descriptor structure.
 */
struct mpeg_flex_mux {
	uint16_t es_id;
	uint8_t flex_mux_channel;
} __ucsi_packed;

/**
 * Process an mpeg_fmc_descriptor structure.
 *
 * @param d Generic descriptor structure.
 * @return Pointer to an mpeg_fmc_descriptor structure, or NULL on error.
 */
static inline struct mpeg_fmc_descriptor*
	mpeg_fmc_descriptor_codec(struct descriptor* d)
{
	uint8_t* buf = (uint8_t*) d + 2;
	int pos = 0;
	int len = d->len;

	if (len % sizeof(struct mpeg_flex_mux))
		return NULL;

	while(pos < len) {
		bswap16(buf+pos);
		pos += sizeof(struct mpeg_flex_mux);
	}

	return (struct mpeg_fmc_descriptor*) d;
}

/**
 * Convenience iterator for the muxes field of an mpeg_fmc_descriptor structure.
 *
 * @param d Generic descriptor structure.
 * @param pos Variable holding a pointer to the the current entry within the muxes field.
 */
#define mpeg_fmc_descriptor_muxes_for_each(d, pos) \
	for ((pos) = mpeg_fmc_descriptor_muxes_first(d); \
	     (pos); \
	     (pos) = mpeg_fmc_descriptor_muxes_next(d, pos))










/******************************** PRIVATE CODE ********************************/
static inline struct mpeg_flex_mux*
		mpeg_fmc_descriptor_muxes_first(struct mpeg_fmc_descriptor *d)
{
	if (d->d.len < sizeof(struct mpeg_flex_mux))
		return NULL;

	return (struct mpeg_flex_mux *)
			((uint8_t*) d + sizeof(struct mpeg_fmc_descriptor));
}

static inline struct mpeg_flex_mux*
	mpeg_fmc_descriptor_muxes_next(struct mpeg_fmc_descriptor *d,
				       struct mpeg_flex_mux *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t *next =	(uint8_t *) pos + sizeof(struct mpeg_flex_mux);

	if (next >= end)
		return NULL;

	return (struct mpeg_flex_mux *) next;
}

#ifdef __cplusplus
}
#endif

#endif
