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

#ifndef _UCSI_DVB_AIT_APPLICATION_ICONS_DESCRIPTOR
#define _UCSI_DVB_AIT_APPLICATION_ICONS_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * Possible values for the icon_flags field.
 */
enum {
	AIT_APPLICATION_ICON_FLAG_32_32		= 0x001,
	AIT_APPLICATION_ICON_FLAG_32_32_43	= 0x002,
	AIT_APPLICATION_ICON_FLAG_24_32_169	= 0x004,

	AIT_APPLICATION_ICON_FLAG_64_64		= 0x008,
	AIT_APPLICATION_ICON_FLAG_64_64_43	= 0x010,
	AIT_APPLICATION_ICON_FLAG_48_64_169	= 0x020,

	AIT_APPLICATION_ICON_FLAG_128_128	= 0x040,
	AIT_APPLICATION_ICON_FLAG_128_128_43	= 0x080,
	AIT_APPLICATION_ICON_FLAG_96_128_169	= 0x100,
};

/**
 * dvb_ait_application_icons_descriptor structure.
 */
struct dvb_ait_application_icons_descriptor {
	struct descriptor d;

	uint8_t icon_locator_length;
	/* uint8_t icon_locator[] */
	/* struct dvb_ait_application_icons_descriptor_part2 */
} __ucsi_packed;

/**
 * Second part of a dvb_ait_application_icons_descriptor.
 */
struct dvb_ait_application_icons_descriptor_part2 {
	uint16_t icon_flags;
	/* uint8_t reserved[] */
} __ucsi_packed;

/**
 * Process a dvb_ait_application_icons_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_ait_application_icons_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ait_application_icons_descriptor*
	dvb_ait_application_icons_descriptor_codec(struct descriptor* d)
{
	uint8_t* buf = (uint8_t*) d;
	uint32_t pos = 0;
	uint32_t len = d->len + 2;
	struct dvb_ait_application_icons_descriptor *ret =
		(struct dvb_ait_application_icons_descriptor *) d;

	if (len < sizeof(struct dvb_ait_application_icons_descriptor))
		return NULL;
	if (len < (sizeof(struct dvb_ait_application_icons_descriptor) + ret->icon_locator_length))
		return NULL;

	pos += sizeof(struct dvb_ait_application_icons_descriptor) + ret->icon_locator_length;

	if ((len - pos) < sizeof(struct dvb_ait_application_icons_descriptor_part2))
		return NULL;
	bswap16(buf + pos);

	return ret;
}
/**
 * Accessor for the icon_locator field of a dvb_ait_application_icons_descriptor.
 *
 * @param e dvb_ait_application_icons_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_ait_application_icons_descriptor_icon_locator(struct dvb_ait_application_icons_descriptor *e)
{
	return (uint8_t *) e + sizeof(struct dvb_ait_application_icons_descriptor);
}

/**
 * Accessor for the part2 field of a dvb_ait_application_icons_descriptor.
 *
 * @param e dvb_ait_application_icons_descriptor Pointer.
 * @return dvb_ait_application_icons_descriptor_part2 pointer.
 */
static inline struct dvb_ait_application_icons_descriptor_part2 *
	dvb_ait_application_icons_descriptor_part2(struct dvb_ait_application_icons_descriptor *e)
{
	return (struct dvb_ait_application_icons_descriptor_part2 *)
		((uint8_t *) e + sizeof(struct dvb_ait_application_icons_descriptor) +
		e->icon_locator_length);
}

/**
 * Accessor for the reserved field of a dvb_ait_application_icons_descriptor_part2.
 *
 * @param e dvb_ait_application_icons_part2 pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_ait_application_icons_descriptor_part2_reserved(struct dvb_ait_application_icons_descriptor_part2 *e)
{
	return (uint8_t *) e + sizeof(struct dvb_ait_application_icons_descriptor_part2);
}

/**
 * Calculate the number of bytes in the reserved field of a dvb_ait_application_icons_descriptor_part2.
 *
 * @param d dvb_ait_application_icons_descriptorpointer.
 * @param part2 dvb_ait_application_icons_descriptor_part2 pointer.
 * @return Number of bytes.
 */
static inline int
	dvb_ait_application_icons_descriptor_part2_reserved_length(struct dvb_ait_application_icons_descriptor *d,
		struct dvb_ait_application_icons_descriptor_part2* part2)
{
	uint8_t *ptr = (uint8_t*) part2 + sizeof(struct dvb_ait_application_icons_descriptor_part2);
	uint8_t *end = (uint8_t*) d + d->d.len + 2;

	return (int) (end - ptr);
}

#ifdef __cplusplus
}
#endif

#endif
