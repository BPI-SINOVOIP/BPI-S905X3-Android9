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

#ifndef _UCSI_MPEG_CAT_SECTION_H
#define _UCSI_MPEG_CAT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * mpeg_cat_section structure.
 */
struct mpeg_cat_section {
	struct section_ext head;

	/* struct descriptor descriptors[] */
} __ucsi_packed;

/**
 * Process an mpeg_cat_section.
 *
 * @param section The generic section_ext structure.
 * @return Pointer to an mpeg_cat_section structure, or NULL on error.
 */
extern struct mpeg_cat_section *mpeg_cat_section_codec(struct section_ext *section);

/**
 * Convenience iterator for descriptors field of an mpeg_cat_section.
 *
 * @param cat The mpeg_cat_section pointer.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define mpeg_cat_section_descriptors_for_each(cat, pos) \
	for ((pos) = mpeg_cat_section_descriptors_first(cat); \
	     (pos); \
	     (pos) = mpeg_cat_section_descriptors_next(cat, pos))









/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	mpeg_cat_section_descriptors_first(struct mpeg_cat_section *cat)
{
	size_t pos = sizeof(struct mpeg_cat_section);

	if (pos >= section_ext_length(&cat->head))
		return NULL;

	return (struct descriptor*)((uint8_t *) cat + pos);
}


static inline struct descriptor *
	mpeg_cat_section_descriptors_next(struct mpeg_cat_section *cat,
					  struct descriptor* pos)
{
	return next_descriptor((uint8_t *) cat + sizeof(struct mpeg_cat_section),
			       section_ext_length(&cat->head) - sizeof(struct mpeg_cat_section),
			       pos);
}

#ifdef __cplusplus
}
#endif

#endif
