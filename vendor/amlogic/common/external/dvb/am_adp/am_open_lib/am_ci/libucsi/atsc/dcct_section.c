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

#include <libucsi/atsc/dcct_section.h>

struct atsc_dcct_section *atsc_dcct_section_codec(struct atsc_section_psip *psip)
{
	uint8_t * buf = (uint8_t *) psip;
	size_t pos = 0;
	size_t len = section_ext_length(&(psip->ext_head));
	int testidx;
	int termidx;

	if (len < sizeof(struct atsc_dcct_section))
		return NULL;
	struct atsc_dcct_section *dcct = (struct atsc_dcct_section *) psip;

	pos += sizeof(struct atsc_dcct_section);
	for (testidx =0; testidx < dcct->dcc_test_count; testidx++) {
		if (len < (pos + sizeof(struct atsc_dcct_test)))
			return NULL;
		struct atsc_dcct_test *test = (struct atsc_dcct_test *) (buf+pos);

		bswap24(buf+pos);
		bswap24(buf+pos+3);
		bswap32(buf+pos+6);
		bswap32(buf+pos+10);

		pos += sizeof(struct atsc_dcct_test);
		for (termidx =0; termidx < test->dcc_term_count; termidx++) {
			if (len < (pos + sizeof(struct atsc_dcct_term)))
				return NULL;
			struct atsc_dcct_term *term = (struct atsc_dcct_term *) (buf+pos);

			bswap64(buf+pos+1);
			bswap16(buf+pos+9);

			pos += sizeof(struct atsc_dcct_term);
			if (len < (pos + term->descriptors_length))
				return NULL;
			if (verify_descriptors(buf + pos, term->descriptors_length))
				return NULL;

			pos += term->descriptors_length;
		}

		if (len < (pos + sizeof(struct atsc_dcct_test_part2)))
			return NULL;
		struct atsc_dcct_test_part2 *part2 = (struct atsc_dcct_test_part2 *) (buf+pos);

		bswap16(buf+pos);

		pos += sizeof(struct atsc_dcct_test_part2);
		if (len < (pos + part2->descriptors_length))
			return NULL;
		if (verify_descriptors(buf + pos, part2->descriptors_length))
			return NULL;
		pos += part2->descriptors_length;
	}

	if (len < (pos + sizeof(struct atsc_dcct_section_part2)))
		return NULL;
	struct atsc_dcct_section_part2 *part2 = (struct atsc_dcct_section_part2 *) (buf+pos);

	bswap16(buf+pos);

	pos += sizeof(struct atsc_dcct_section_part2);
	if (len < (pos + part2->descriptors_length))
		return NULL;
	if (verify_descriptors(buf + pos, part2->descriptors_length))
		return NULL;

	pos += part2->descriptors_length;
	if (pos != len)
		return NULL;

	return (struct atsc_dcct_section *) psip;
}
