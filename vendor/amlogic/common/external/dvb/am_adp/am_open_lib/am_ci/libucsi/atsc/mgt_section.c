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

#include <libucsi/atsc/mgt_section.h>

struct atsc_mgt_section *atsc_mgt_section_codec(struct atsc_section_psip *psip)
{
	uint8_t * buf = (uint8_t *) psip;
	size_t pos = sizeof(struct atsc_section_psip);
	size_t len = section_ext_length(&(psip->ext_head));
	struct atsc_mgt_section *mgt = (struct atsc_mgt_section *) psip;
	int i;

	if (len < sizeof(struct atsc_mgt_section))
		return NULL;

	bswap16(buf + pos);
	pos += 2;

	// we cannot use the tables_defined value here because of the braindead ATSC spec!
	for (i=0; i < mgt->tables_defined; i++) {
		// we think we're still in the tables - process as normal
		if ((pos + sizeof(struct atsc_mgt_table)) > len)
			return NULL;
		struct atsc_mgt_table *table = (struct atsc_mgt_table *) (buf+pos);

		bswap16(buf+pos);
		bswap16(buf+pos+2);
		bswap32(buf+pos+5);
		bswap16(buf+pos+9);

		pos += sizeof(struct atsc_mgt_table);
		if ((pos + table->table_type_descriptors_length) > len)
			return NULL;
		if (verify_descriptors(buf + pos, table->table_type_descriptors_length))
			return NULL;

		pos += table->table_type_descriptors_length;
	}

	if ((pos + sizeof(struct atsc_mgt_section_part2)) > len)
		return NULL;
	struct atsc_mgt_section_part2 *part2 = (struct atsc_mgt_section_part2 *) (buf+pos);

	bswap16(buf+pos);

	pos += sizeof(struct atsc_mgt_section_part2);
	if ((pos + part2->descriptors_length) > len)
		return NULL;
	if (verify_descriptors(buf + pos, part2->descriptors_length))
		return NULL;
	pos += part2->descriptors_length;

	if (pos != len)
		return NULL;

	return (struct atsc_mgt_section *) psip;
}
