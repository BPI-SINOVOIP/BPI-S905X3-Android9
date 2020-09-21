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

#ifndef _UCSI_MPEG_DATA_STREAM_ALIGNMENT_DESCRIPTOR
#define _UCSI_MPEG_DATA_STREAM_ALIGNMENT_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for alignment_type.
 */
enum {
	MPEG_DATA_STREAM_ALIGNMENT_VIDEO_SLICE_OR_AU		= 0x01,
	MPEG_DATA_STREAM_ALIGNMENT_VIDEO_AU			= 0x02,
	MPEG_DATA_STREAM_ALIGNMENT_VIDEO_GOP_OR_SEQ		= 0x03,
	MPEG_DATA_STREAM_ALIGNMENT_VIDEO_SEQ			= 0x04,

	MPEG_DATA_STREAM_ALIGNMENT_AUDIO_SYNC_WORD		= 0x01,
};

/**
 * mpeg_data_stream_alignment_descriptor structure.
 */
struct mpeg_data_stream_alignment_descriptor {
	struct descriptor d;

	uint8_t alignment_type;
} __ucsi_packed;

/**
 * Process an mpeg_data_stream_alignment_descriptor.
 *
 * @param d Pointer to generic descriptor structure.
 * @return Pointer to mpeg_data_stream_alignment_descriptor, or NULL on error.
 */
static inline struct mpeg_data_stream_alignment_descriptor*
	mpeg_data_stream_alignment_descriptor_codec(struct descriptor* d)
{
	if (d->len != (sizeof(struct mpeg_data_stream_alignment_descriptor) - 2))
		return NULL;

	return (struct mpeg_data_stream_alignment_descriptor*) d;
}

#ifdef __cplusplus
}
#endif

#endif
