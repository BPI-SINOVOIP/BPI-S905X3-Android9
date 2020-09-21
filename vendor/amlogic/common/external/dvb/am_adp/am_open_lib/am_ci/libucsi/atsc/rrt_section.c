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

#include <libucsi/atsc/rrt_section.h>

struct atsc_rrt_section *atsc_rrt_section_codec(struct atsc_section_psip *psip)
{
	uint8_t * buf = (uint8_t *) psip;
	size_t pos = 0;
	size_t len = section_ext_length(&(psip->ext_head));
	int idx;
	int vidx;
	struct atsc_rrt_section *rrt = (struct atsc_rrt_section *) psip;

	if (len < sizeof(struct atsc_rrt_section))
		return NULL;
	pos += sizeof(struct atsc_rrt_section);

	if (len < (pos + rrt->rating_region_name_length))
		return NULL;
	if (atsc_text_validate(buf+pos, rrt->rating_region_name_length))
		return NULL;

	pos += rrt->rating_region_name_length;
	if (len < (pos + sizeof(struct atsc_rrt_section_part2)))
		return NULL;
	struct atsc_rrt_section_part2 *rrtpart2 = (struct atsc_rrt_section_part2 *) (buf+pos);

	pos += sizeof(struct atsc_rrt_section_part2);
	for (idx =0; idx < rrtpart2->dimensions_defined; idx++) {
		if (len < (pos + sizeof(struct atsc_rrt_dimension)))
			return NULL;
		struct atsc_rrt_dimension *dimension = (struct atsc_rrt_dimension *) (buf+pos);

		pos += sizeof(struct atsc_rrt_dimension);
		if (len < (pos + dimension->dimension_name_length))
			return NULL;
		if (atsc_text_validate(buf+pos, dimension->dimension_name_length))
			return NULL;

		pos += dimension->dimension_name_length;
		if (len < (pos + sizeof(struct atsc_rrt_dimension_part2)))
			return NULL;
		struct atsc_rrt_dimension_part2 *dpart2 = (struct atsc_rrt_dimension_part2 *) (buf+pos);

		pos += sizeof(struct atsc_rrt_dimension_part2);
		for (vidx =0; vidx < dpart2->values_defined; vidx++) {
			if (len < (pos + sizeof(struct atsc_rrt_dimension_value)))
				return NULL;
			struct atsc_rrt_dimension_value *value = (struct atsc_rrt_dimension_value *) (buf+pos);

			pos += sizeof(struct atsc_rrt_dimension_value);
			if (len < (pos + value->abbrev_rating_value_length))
				return NULL;
			if (atsc_text_validate(buf+pos, value->abbrev_rating_value_length))
				return NULL;

			pos += value->abbrev_rating_value_length;
			if (len < (pos + sizeof(struct atsc_rrt_dimension_value_part2)))
				return NULL;
			struct atsc_rrt_dimension_value_part2 *vpart2 =
				(struct atsc_rrt_dimension_value_part2 *) (buf+pos);

			pos += sizeof(struct atsc_rrt_dimension_value_part2);
			if (len < (pos + vpart2->rating_value_length))
				return NULL;
			if (atsc_text_validate(buf+pos, vpart2->rating_value_length))
				return NULL;

			pos+= vpart2->rating_value_length;
		}
	}

	if (len < (pos + sizeof(struct atsc_rrt_section_part3)))
		return NULL;
	struct atsc_rrt_section_part3 *part3 = (struct atsc_rrt_section_part3 *) (buf+pos);

	pos += sizeof(struct atsc_rrt_section_part3);
	if (len < (pos + part3->descriptors_length))
		return NULL;

	if (verify_descriptors(buf + pos, part3->descriptors_length))
		return NULL;

	pos += part3->descriptors_length;
	if (pos != len)
		return NULL;

	return (struct atsc_rrt_section *) psip;
}
