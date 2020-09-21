#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
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

#include <errno.h>
#include <string.h>
#include "section_buf.h"

#define SECTION_HDR_SIZE 3
#define SECTION_PAD 0xff

int section_buf_init(struct section_buf *section, int max)
{
	if (max < SECTION_HDR_SIZE)
		return -EINVAL;

	memset(section, 0, sizeof(struct section_buf));
	section->max = max; /* max size of data */
	section->len = SECTION_HDR_SIZE;
	section->wait_pdu = 1;

	return 0;
}

int section_buf_add(struct section_buf *section, uint8_t* frag, int len, int *section_status)
{
	int copy;
	int used = 0;
	uint8_t *data;
	uint8_t *pos = (uint8_t*) section + sizeof(struct section_buf) + section->count;

	/* have we finished? */
	if (section->header && (section->len == section->count)) {
		*section_status = 1;
		return 0;
	}

	/* skip over section padding bytes */
	*section_status = 0;
	if (section->count == 0) {
		while (len && (*frag == SECTION_PAD)) {
			frag++;
			len--;
			used++;
		}

		if (len == 0)
			return used;
	}

	/* grab the header to get the section length */
	if (!section->header) {
		/* copy the header frag */
		copy = SECTION_HDR_SIZE - section->count;
		if (copy > len)
			copy = len;
		memcpy(pos, frag, copy);
		section->count += copy;
		pos += copy;
		frag += copy;
		used += copy;
		len -= copy;

		/* we need 3 bytes for the section header */
		if (section->count != SECTION_HDR_SIZE)
			return used;

		/* work out the length & check it isn't too big */
		data = (uint8_t*) section + sizeof(struct section_buf);
		section->len = SECTION_HDR_SIZE + (((data[1] & 0x0f) << 8) | data[2]);
		if (section->len > section->max) {
			*section_status = -ERANGE;
			return len + used;
		}

		/* update fields */
		section->header = 1;
	}

	/* accumulate frag */
	copy = section->len - section->count;
	if (copy > len)
		copy = len;
	memcpy(pos, frag, copy);
	section->count += copy;
	used += copy;

	/* have we finished? */
	if (section->header && (section->len == section->count))
		*section_status = 1;

	/* return number of bytes used */
	return used;
}

int section_buf_add_transport_payload(struct section_buf *section,
				      uint8_t* payload, int len,
				      int pdu_start, int *section_status)
{
	int used = 0;
	int tmp;

	/* have we finished? */
	if (section->header && (section->len == section->count)) {
		*section_status = 1;
		return 0;
	}

	/* don't bother if we're waiting for a PDU */
	*section_status = 0;
	if (section->wait_pdu && (!pdu_start))
		return len;

	/* if we're at a PDU start, we need extra handling for the extra first
	 * byte giving the offset to the start of the next section. */
	if (pdu_start) {
		/* we have received a pdu */
		section->wait_pdu = 0;

		/* work out the offset to the _next_ payload */
		int offset = payload[0];
		if ((offset+1) > len) {
			section->wait_pdu = 1;
			*section_status = -EINVAL;
			return len;
		}

		/* accumulate the end if we need to */
		if (section->count != 0) {
			/* add the final fragment. */
			tmp = section_buf_add(section, payload + 1, offset, section_status);

			/* the stream said this was the final fragment
			 * (PDU START bit) - check that it really was! */
			if ((tmp != offset) || section_buf_remaining(section) || (*section_status != 1)) {
				*section_status = -ERANGE;
				section->wait_pdu = 1;
				return 1 + tmp;
			}

			/* it is complete - return the number of bytes we used */
			return 1 + tmp;
		}

		/* otherwise, we skip the end of the previous section, and
		 * start accumulating the new data. */
		used = 1 + offset;
	}

	/* ok, just accumulate the data as normal */
	tmp = section_buf_add(section, payload+used, len - used, section_status);
	if (*section_status < 0) {
		section->wait_pdu = 1;
	}

	return used + tmp;
}
