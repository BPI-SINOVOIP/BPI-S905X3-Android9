#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
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

#include <libucsi/mpeg/odsmt_section.h>

struct mpeg_odsmt_section *mpeg_odsmt_section_codec(struct section_ext * ext)
{
	struct mpeg_odsmt_section * odsmt = (struct mpeg_odsmt_section *)ext;
	uint8_t * buf = (uint8_t *)ext;
	size_t pos = sizeof(struct section_ext);
	size_t len = section_ext_length(ext);
	int i;

	if (len < sizeof(struct mpeg_odsmt_section))
		return NULL;

	pos++;

	if (odsmt->stream_count == 0) {
		struct mpeg_odsmt_stream * stream =
			(struct mpeg_odsmt_stream *) (buf + pos);

		if ((pos + sizeof(struct mpeg_odsmt_stream_single)) > len)
			return NULL;

		bswap16(buf+pos);
		pos+=3;

		if ((pos + stream->u.single.es_info_length) >= len)
			return NULL;

		if (verify_descriptors(buf + pos, stream->u.single.es_info_length))
			return NULL;

		pos += stream->u.single.es_info_length;
	} else {
		for (i=0; i< odsmt->stream_count; i++) {
			struct mpeg_odsmt_stream * stream =
				(struct mpeg_odsmt_stream *)(buf + pos);

			if ((pos + sizeof(struct mpeg_odsmt_stream_multi)) > len)
				return NULL;

			bswap16(buf+pos);
			pos += sizeof(struct mpeg_odsmt_stream_multi);

			if ((pos + stream->u.multi.es_info_length) > len)
				return NULL;

			if (verify_descriptors(buf + pos,
			    stream->u.multi.es_info_length))
				return NULL;

			pos += stream->u.multi.es_info_length;
		}
	}

	if (pos != len)
		return NULL;

	return (struct mpeg_odsmt_section *) ext;
}
