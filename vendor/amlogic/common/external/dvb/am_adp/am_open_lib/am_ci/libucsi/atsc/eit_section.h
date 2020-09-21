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

#ifndef _UCSI_ATSC_EIT_SECTION_H
#define _UCSI_ATSC_EIT_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>

/**
 * atsc_eit_section structure.
 */
struct atsc_eit_section {
	struct atsc_section_psip head;

	uint8_t num_events_in_section;
	/* struct atsc_eit_event events[] */
} __ucsi_packed;

struct atsc_eit_event {
  EBIT2(uint16_t reserved			: 2; ,
	uint16_t event_id			:14; );
	atsctime_t start_time;
  EBIT4(uint32_t reserved1			: 2; ,
	uint32_t ETM_location			: 2; ,
	uint32_t length_in_seconds		:20; ,
	uint32_t title_length			: 8; );
	/* struct atsc_text title_text */
	/* struct atsc_eit_event_part2 part2 */
} __ucsi_packed;

struct atsc_eit_event_part2 {
  EBIT2(uint16_t reserved			: 4; ,
	uint16_t descriptors_length		:12; );
	/* struct descriptor descriptors[] */
} __ucsi_packed;


/**
 * Process a atsc_eit_section.
 *
 * @param section Pointer to an atsc_section_psip structure.
 * @return atsc_eit_section pointer, or NULL on error.
 */
struct atsc_eit_section *atsc_eit_section_codec(struct atsc_section_psip *section);

/**
 * Accessor for the source_id field of an EIT.
 *
 * @param eit EIT pointer.
 * @return The source_id .
 */
static inline uint16_t atsc_eit_section_source_id(struct atsc_eit_section *eit)
{
	return eit->head.ext_head.table_id_ext;
}

/**
 * Iterator for the events field in an atsc_eit_section.
 *
 * @param eit atsc_eit_section pointer.
 * @param pos Variable containing a pointer to the current atsc_eit_event.
 * @param idx Integer used to count which event we are in.
 */
#define atsc_eit_section_events_for_each(eit, pos, idx) \
	for ((pos) = atsc_eit_section_events_first(eit), idx=0; \
	     (pos); \
	     (pos) = atsc_eit_section_events_next(eit, pos, ++idx))

/**
 * Accessor for the title_text field of an atsc_eit_event.
 *
 * @param event atsc_eit_event pointer.
 * @return struct atsc_text pointer, or NULL on error.
 */
static inline struct atsc_text *atsc_eit_event_name_title_text(struct atsc_eit_event *event)
{
	if (event->title_length == 0)
		return NULL;

	return (struct atsc_text*)(((uint8_t*) event) + sizeof(struct atsc_eit_event));
}

/**
 * Accessor for the part2 field of an atsc_eit_event.
 *
 * @param event atsc_eit_event pointer.
 * @return struct atsc_eit_event_part2 pointer.
 */
static inline struct atsc_eit_event_part2 *atsc_eit_event_part2(struct atsc_eit_event *event)
{
	return (struct atsc_eit_event_part2 *)
		(((uint8_t*) event) + sizeof(struct atsc_eit_event) + event->title_length);
}

/**
 * Iterator for the descriptors field in a atsc_eit_section structure.
 *
 * @param part2 atsc_eit_event_part2 pointer.
 * @param pos Variable containing a pointer to the current descriptor.
 */
#define atsc_eit_event_part2_descriptors_for_each(part2, pos) \
	for ((pos) = atsc_eit_event_part2_descriptors_first(part2); \
	     (pos); \
	     (pos) = atsc_eit_event_part2_descriptors_next(part2, pos))











/******************************** PRIVATE CODE ********************************/
static inline struct atsc_eit_event *
	atsc_eit_section_events_first(struct atsc_eit_section *eit)
{
	size_t pos = sizeof(struct atsc_eit_section);

	if (eit->num_events_in_section == 0)
		return NULL;

	return (struct atsc_eit_event*) (((uint8_t *) eit) + pos);
}

static inline struct atsc_eit_event *
	atsc_eit_section_events_next(struct atsc_eit_section *eit,
				     struct atsc_eit_event *pos,
				     int idx)
{
	if (idx >= eit->num_events_in_section)
		return NULL;

	struct atsc_eit_event_part2 *part2 = atsc_eit_event_part2(pos);
	int len = sizeof(struct atsc_eit_event_part2);
	len += part2->descriptors_length;

	return (struct atsc_eit_event *) (((uint8_t*) part2) + len);
}

static inline struct descriptor *
	atsc_eit_event_part2_descriptors_first(struct atsc_eit_event_part2 *part2)
{
	size_t pos = sizeof(struct atsc_eit_event_part2);

	if (part2->descriptors_length == 0)
		return NULL;

	return (struct descriptor*) (((uint8_t *) part2) + pos);
}

static inline struct descriptor *
	atsc_eit_event_part2_descriptors_next(struct atsc_eit_event_part2 *part2,
					      struct descriptor *pos)
{
	return next_descriptor((uint8_t*) part2 + sizeof(struct atsc_eit_event_part2),
				part2->descriptors_length,
				pos);
}

#ifdef __cplusplus
}
#endif

#endif
