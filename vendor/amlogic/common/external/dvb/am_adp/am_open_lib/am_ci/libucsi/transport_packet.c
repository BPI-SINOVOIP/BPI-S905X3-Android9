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

#include "transport_packet.h"

#define CONTINUITY_VALID 0x80
#define CONTINUITY_DUPESEEN 0x40

int transport_packet_values_extract(struct transport_packet *pkt,
				    struct transport_values *out,
					enum transport_value extract)
{
	uint8_t *end = (uint8_t*) pkt + TRANSPORT_PACKET_LENGTH;
	uint8_t *adapend;
	uint8_t *pos = (uint8_t*) pkt + sizeof(struct transport_packet);
	enum transport_value extracted = 0;
	enum transport_adaptation_flags adapflags = 0;
	enum transport_adaptation_extension_flags adapextflags = 0;
	int adaplength = 0;
	int adapextlength = 0;

	/* does the packet contain an adaptation field ? */
	if ((pkt->adaptation_field_control & 2) == 0)
		goto extract_payload;

	/* get the adaptation field length and skip the byte */
	adaplength = *pos++;

	/* do we actually have any adaptation data? */
	if (adaplength == 0)
		goto extract_payload;

	/* sanity check */
	adapend = pos + adaplength;
	if (adapend > end)
		return -1;

	/* extract the adaptation flags (we must have at least 1 byte to be here) */
	adapflags = *pos++;

	/* do we actually want anything else? */
	if ((extract & 0xffff) == 0)
		goto extract_payload;

	/* PCR? */
	if (adapflags & transport_adaptation_flag_pcr) {
		if ((pos+6) > adapend)
			return -1;

		if (extract & transport_value_pcr) {
			uint64_t base = ((uint64_t) pos[0] << 25) |
					((uint64_t) pos[1] << 17) |
					((uint64_t) pos[2] << 9) |
					((uint64_t) pos[3] << 1) |
					((uint64_t) pos[4] >> 7);
			uint64_t ext = (((uint64_t) pos[4] & 1) << 8) |
					(uint64_t) pos[5];
			out->pcr= base * 300ULL + ext;
			extracted |= transport_value_pcr;
		}
		pos += 6;
	}

	/* OPCR? */
	if (adapflags & transport_adaptation_flag_opcr) {
		if ((pos+6) > adapend)
			return -1;

		if (extract & transport_value_opcr) {
			uint64_t base = ((uint64_t) pos[0] << 25) |
					((uint64_t) pos[1] << 17) |
					((uint64_t) pos[2] << 9) |
					((uint64_t) pos[3] << 1) |
					((uint64_t) pos[4] >> 7);
			uint64_t ext = (((uint64_t) pos[4] & 1) << 8) |
					(uint64_t) pos[5];
			out->opcr= base * 300ULL + ext;
			extracted |= transport_value_opcr;
		}
		pos += 6;
	}

	/* splice countdown? */
	if (adapflags & transport_adaptation_flag_splicing_point) {
		if ((pos+1) > adapend)
			return -1;

		if (extract & transport_value_splice_countdown) {
			out->splice_countdown = *pos;
			extracted |= transport_value_splice_countdown;
		}
		pos++;
	}

	/* private data? */
	if (adapflags & transport_adaptation_flag_private_data) {
		if ((pos+1) > adapend)
			return -1;
		if ((pos+1+*pos) > adapend)
			return -1;

		if (extract & transport_value_private_data) {
			out->private_data_length = *pos;
			out->private_data = pos + 1;
			extracted |= transport_value_private_data;
		}
		pos += 1 + *pos;
	}

	/* is there an adaptation extension? */
	if (!(adapflags & transport_adaptation_flag_extension))
		goto extract_payload;

	/* get/check the length */
	if (pos >= adapend)
		return -1;
	adapextlength = *pos++;
	if ((pos + adapextlength) > adapend)
		return -1;

	/* do we want/have anything in the adaptation extension? */
	if (((extract & 0xff00) == 0) || (adapextlength == 0))
		goto extract_payload;

	/* extract the adaptation extension flags (we must have at least 1 byte
	 * to be here) */
	adapextflags = *pos++;

	/* LTW? */
	if (adapextflags & transport_adaptation_extension_flag_ltw) {
		if ((pos+2) > adapend)
			return -1;

		if (extract & transport_value_ltw) {
			if (*pos & 0x80) {
				out->ltw_offset = ((pos[0] & 0x7f) << 8) |
						  (pos[1]);
				extracted |= transport_value_ltw;
			}
		}
		pos += 2;
	}

	/* piecewise_rate? */
	if (adapextflags & transport_adaptation_extension_flag_piecewise_rate) {
		if ((pos+3) > adapend)
			return -1;

		if (extract & transport_value_piecewise_rate) {
			out->piecewise_rate = ((pos[0] & 0x3f) << 16) |
					      (pos[1] << 8) |
					      pos[2];
			extracted |= transport_value_piecewise_rate;
		}
		pos += 3;
	}

	/* seamless_splice? */
	if (adapextflags & transport_adaptation_extension_flag_seamless_splice) {
		if ((pos+5) > adapend)
			return -1;

		if (extract & transport_value_piecewise_rate) {
			out->splice_type = pos[0] >> 4;
			out->dts_next_au = ((pos[0] & 0x0e) << 29) |
					   (pos[1] << 22) |
					   ((pos[2] & 0xfe) << 14) |
					   (pos[3] << 7) |
					   ((pos[4] & 0xfe) >> 1);
			extracted |= transport_value_seamless_splice;
		}
		pos += 5;
	}



extract_payload:
	/* does the packet contain a payload? */
	if (pkt->adaptation_field_control & 1) {
		int off = sizeof(struct transport_packet);
		if (pkt->adaptation_field_control & 2)
			off++;
		off += adaplength;

		out->payload = (uint8_t*) pkt + off;
		out->payload_length = TRANSPORT_PACKET_LENGTH - off;
	} else {
		out->payload = NULL;
		out->payload_length = 0;
	}

	out->flags = adapflags;
	return extracted;
}

int transport_packet_continuity_check(struct transport_packet *pkt,
				      int discontinuity_indicator, unsigned char *cstate)
{
	unsigned char pktcontinuity = pkt->continuity_counter;
	unsigned char prevcontinuity = *cstate & 0x0f;
	unsigned char nextcontinuity;

	/* NULL packets have undefined continuity */
	if (transport_packet_pid(pkt) == TRANSPORT_NULL_PID)
		return 0;

	/* is the state valid? */
	if (!(*cstate & CONTINUITY_VALID)) {
		*cstate = pktcontinuity | CONTINUITY_VALID;
		return 0;
	}

	/* check for discontinuity_indicator */
	if (discontinuity_indicator) {
		*cstate = pktcontinuity | CONTINUITY_VALID;
		return 0;
	}

	/* only packets with a payload should increment the counter */
	if (pkt->adaptation_field_control & 1)
		nextcontinuity = (prevcontinuity + 1) & 0xf;
	else
		nextcontinuity = prevcontinuity;

	/* check for a normal continuity progression */
	if (nextcontinuity == pktcontinuity) {
		*cstate = pktcontinuity | CONTINUITY_VALID;
		return 0;
	}

	/* one dupe is allowed */
	if ((prevcontinuity == pktcontinuity) && (!(*cstate & CONTINUITY_DUPESEEN))) {
		*cstate = pktcontinuity | (CONTINUITY_VALID|CONTINUITY_DUPESEEN);
		return 0;
	}

	/* continuity error */
	return -1;
}
