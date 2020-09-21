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

#ifndef _UCSI_DVB_EXTENDED_EVENT_DESCRIPTOR
#define _UCSI_DVB_EXTENDED_EVENT_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * dvb_extended_event_descriptor structure.
 */
struct dvb_extended_event_descriptor {
	struct descriptor d;

  EBIT2(uint8_t descriptor_number	: 4; ,
	uint8_t last_descriptor_number	: 4; );
	iso639lang_t language_code;
	uint8_t length_of_items;
	/* struct dvb_extended_event_item items[] */
	/* struct dvb_extended_event_descriptor_part2 part2 */
} __ucsi_packed;

/**
 * An entry in the items field of a dvb_extended_event_descriptor.
 */
struct dvb_extended_event_item {
	uint8_t item_description_length;
	/* uint8_t item_description[] */
	/* struct dvb_extended_event_item_part2 part2 */
} __ucsi_packed;

/**
 * The second part of a dvb_extended_event_item, following the variable length
 * description field.
 */
struct dvb_extended_event_item_part2 {
	uint8_t item_length;
	/* uint8_t item[] */
} __ucsi_packed;

/**
 * The second part of a dvb_extended_event_descriptor, following the variable
 * length items field.
 */
struct dvb_extended_event_descriptor_part2 {
	uint8_t text_length;
	/* uint8_t text[] */
} __ucsi_packed;

/**
 * Process a dvb_extended_event_descriptor.
 *
 * @param d Generic descriptor structure.
 * @return dvb_extended_event_descriptor pointer, or NULL on error.
 */
static inline struct dvb_extended_event_descriptor*
	dvb_extended_event_descriptor_codec(struct descriptor* d)
{
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t pos = 0;
	uint32_t len = d->len;
	struct dvb_extended_event_descriptor * p =
		(struct dvb_extended_event_descriptor *) d;
	struct dvb_extended_event_descriptor_part2 *p2;

	pos += sizeof(struct dvb_extended_event_descriptor) - 2;

	if (pos > len)
		return NULL;

	pos += p->length_of_items;

	if (pos > len)
		return NULL;

	p2 = (struct dvb_extended_event_descriptor_part2*) (buf+pos);

	pos += sizeof(struct dvb_extended_event_descriptor_part2);

	if (pos > len)
		return NULL;

	pos += p2->text_length;

	if (pos != len)
		return NULL;

	return p;
}

/**
 * Iterator for the items field of a dvb_extended_event_descriptor.
 *
 * @param d dvb_extended_event_descriptor pointer.
 * @param pos Variable containing a pointer to the current dvb_extended_event_item.
 */
#define dvb_extended_event_descriptor_items_for_each(d, pos) \
	for ((pos) = dvb_extended_event_descriptor_items_first(d); \
	     (pos); \
	     (pos) = dvb_extended_event_descriptor_items_next(d, pos))

/**
 * Accessor for the description field of a dvb_extended_event_item.
 *
 * @param d dvb_extended_event_item pointer.
 * @return Pointer to the field.
 */
static inline uint8_t*
	dvb_extended_event_item_description(struct dvb_extended_event_item *d)
{
	return (uint8_t*) d + sizeof(struct dvb_extended_event_item);
}

/**
 * Accessor for the second part of a dvb_extended_event_item.
 *
 * @param dvb_extended_event_item pointer.
 * @return dvb_extended_event_item_part2 pointer.
 */
static inline struct dvb_extended_event_item_part2*
	dvb_extended_event_item_part2(struct dvb_extended_event_item *d)
{
	return (struct dvb_extended_event_item_part2*)
		((uint8_t*) d + sizeof(struct dvb_extended_event_item) +
		 d->item_description_length);
}

/**
 * Accessor for the item field of a dvb_extended_event_item_part2.
 *
 * @param d dvb_extended_event_item_part2 pointer.
 * @return Pointer to the item field.
 */
static inline uint8_t*
	dvb_extended_event_item_part2_item(struct dvb_extended_event_item_part2 *d)
{
	return (uint8_t*) d + sizeof(struct dvb_extended_event_item_part2);
}

/**
 * Accessor for the second part of a dvb_extended_event_descriptor.
 *
 * @param d dvb_extended_event_descriptor pointer.
 * @return dvb_extended_event_descriptor_part2 pointer.
 */
static inline struct dvb_extended_event_descriptor_part2*
	dvb_extended_event_descriptor_part2(struct dvb_extended_event_descriptor *d)
{
	return (struct dvb_extended_event_descriptor_part2*)
		((uint8_t*) d + sizeof(struct dvb_extended_event_descriptor) +
		 d->length_of_items);
}

/**
 * Accessor for the text field of an dvb_extended_event_descriptor_part2.
 *
 * @param d dvb_extended_event_descriptor_part2 pointer.
 * @return Pointer to the text field.
 */
static inline uint8_t*
	dvb_extended_event_descriptor_part2_text(struct dvb_extended_event_descriptor_part2 *d)
{
	return (uint8_t*)
		((uint8_t*) d + sizeof(struct dvb_extended_event_descriptor_part2));
}









/******************************** PRIVATE CODE ********************************/
static inline struct dvb_extended_event_item*
	dvb_extended_event_descriptor_items_first(struct dvb_extended_event_descriptor *d)
{
	if (d->length_of_items == 0)
		return NULL;

	return (struct dvb_extended_event_item *)
		((uint8_t*) d + sizeof(struct dvb_extended_event_descriptor));
}

static inline struct dvb_extended_event_item*
	dvb_extended_event_descriptor_items_next(struct dvb_extended_event_descriptor *d,
						 struct dvb_extended_event_item *pos)
{
	struct dvb_extended_event_item_part2* part2 =
		dvb_extended_event_item_part2(pos);
	uint8_t *end = (uint8_t*) d + sizeof(struct dvb_extended_event_descriptor) + d->length_of_items;
	uint8_t *next =	(uint8_t *) part2 +
			sizeof(struct dvb_extended_event_item_part2) +
			part2->item_length;

	if (next >= end)
		return NULL;

	return (struct dvb_extended_event_item *) next;
}

#ifdef __cplusplus
}
#endif

#endif
