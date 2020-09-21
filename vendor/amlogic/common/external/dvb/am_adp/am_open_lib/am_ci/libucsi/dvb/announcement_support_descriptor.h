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

#ifndef _UCSI_DVB_ANNOUNCEMENT_SUPPORT_DESCRIPTOR
#define _UCSI_DVB_ANNOUNCEMENT_SUPPORT_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for announcement_support_indicator.
 */
enum {
	DVB_ANNOUNCEMENT_SUPPORT_EMERGENCY		= 0x01,
	DVB_ANNOUNCEMENT_SUPPORT_ROAD_TRAFFIC_FLASH	= 0x02,
	DVB_ANNOUNCEMENT_SUPPORT_PUBLIC_TRANSPORT_FLASH	= 0x04,
	DVB_ANNOUNCEMENT_SUPPORT_WARNING_MESSAGE	= 0x08,
	DVB_ANNOUNCEMENT_SUPPORT_NEWS_FLASH		= 0x10,
	DVB_ANNOUNCEMENT_SUPPORT_WEATHER_FLASH		= 0x20,
	DVB_ANNOUNCEMENT_SUPPORT_EVENT_ANNOUNCEMENT	= 0x40,
	DVB_ANNOUNCEMENT_SUPPORT_PERSONAL_CALL		= 0x80,
};

/**
 * Possible values for announcement_type.
 */
enum {
	DVB_ANNOUNCEMENT_TYPE_EMERGENCY			= 0x00,
	DVB_ANNOUNCEMENT_TYPE_ROAD_TRAFFIC_FLASH	= 0x01,
	DVB_ANNOUNCEMENT_TYPE_PUBLIC_TRANSPORT_FLASH	= 0x02,
	DVB_ANNOUNCEMENT_TYPE_WARNING_MESSAGE		= 0x03,
	DVB_ANNOUNCEMENT_TYPE_NEWS_FLASH		= 0x04,
	DVB_ANNOUNCEMENT_TYPE_WEATHER_FLASH		= 0x05,
	DVB_ANNOUNCEMENT_TYPE_EVENT_ANNOUNCEMENT	= 0x06,
	DVB_ANNOUNCEMENT_TYPE_PERSONAL_CALL		= 0x07,
};

/**
 * Possible values for reference_type.
 */
enum {
	DVB_REFERENCE_TYPE_AUDIO			= 0x00,
	DVB_REFERENCE_TYPE_OTHER_AUDIO			= 0x01,
	DVB_REFERENCE_TYPE_OTHER_SERVICE		= 0x02,
	DVB_REFERENCE_TYPE_OTHER_TS			= 0x03,
};

/**
 * dvb_announcement_support_descriptor structure.
 */
struct dvb_announcement_support_descriptor {
	struct descriptor d;
	uint16_t announcement_support_indicator;
	/* struct dvb_announcement_support_entry entries[] */
} __ucsi_packed;

/**
 * An entry in the entries field of a dvb_announcement_support_descriptor.
 */
struct dvb_announcement_support_entry {
  EBIT3(uint8_t announcement_type		: 4; ,
	uint8_t reserved			: 1; ,
	uint8_t reference_type			: 3; );
	/* Only if reference_type == 1, 2 or 3:
	 * struct dvb_announcement_support_reference reference */
} __ucsi_packed;

/**
 * The optional reference field only present in a dvb_announcement_support_descriptor if
 * its reference_type field is 1,2 or 3.
 */
struct dvb_announcement_support_reference {
	uint16_t original_network_id;
	uint16_t transport_stream_id;
	uint16_t service_id;
	uint8_t component_tag;
} __ucsi_packed;

/**
 * Process a dvb_announcement_support_descriptor.
 *
 * @param d Generic descriptor pointer.
 * @return dvb_announcement_support_descriptor pointer, or NULL on error.
 */
static inline struct dvb_announcement_support_descriptor*
	dvb_announcement_support_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 0;
	uint8_t* buf = (uint8_t*) d + 2;
	uint32_t len = d->len;

	if (len < (sizeof(struct dvb_announcement_support_descriptor) - 2))
		return NULL;

	bswap16(buf+pos);

	pos += 2;

	while(pos < len) {
		struct dvb_announcement_support_entry *e =
			(struct dvb_announcement_support_entry*) (buf+pos);

		pos += sizeof(struct dvb_announcement_support_entry);

		if (pos > len)
			return NULL;

		if ((e->reference_type == 1) ||
		    (e->reference_type == 2) ||
		    (e->reference_type == 3)) {
			if ((pos + sizeof(struct dvb_announcement_support_reference)) > len)
				return NULL;

			bswap16(buf+pos);
			bswap16(buf+pos+2);
			bswap16(buf+pos+4);

			pos += sizeof(struct dvb_announcement_support_reference);
		}
	}

	return (struct dvb_announcement_support_descriptor*) d;
}

/**
 * Iterator for the entries field of a dvb_announcement_support_descriptor.
 *
 * @param d dvb_announcement_support_descriptor pointer.
 * @param pod Variable holding a pointer to the current dvb_announcement_support_entry.
 */
#define dvb_announcement_support_descriptor_entries_for_each(d, pos) \
	for ((pos) = dvb_announcement_support_descriptor_entries_first(d); \
	     (pos); \
	     (pos) = dvb_announcement_support_descriptor_entries_next(d, pos))

/**
 * Accessor for the reference field of a dvb_announcement_support_entry if present.
 *
 * @param entry dvb_announcement_support_entry pointer.
 * @return dvb_announcement_support_reference pointer, or NULL on error.
 */
static inline struct dvb_announcement_support_reference*
	dvb_announcement_support_entry_reference(struct dvb_announcement_support_entry* entry)
{
	if ((entry->reference_type != 0x01) &&
	    (entry->reference_type != 0x02) &&
	    (entry->reference_type != 0x03))
		return NULL;

	return (struct dvb_announcement_support_reference*)
		((uint8_t*) entry + sizeof(struct dvb_announcement_support_entry));
}










/******************************** PRIVATE CODE ********************************/
static inline struct dvb_announcement_support_entry*
	dvb_announcement_support_descriptor_entries_first(struct dvb_announcement_support_descriptor *d)
{
	if (d->d.len == 2)
		return NULL;

	return (struct dvb_announcement_support_entry *)
		((uint8_t*) d + sizeof(struct dvb_announcement_support_descriptor));
}

static inline struct dvb_announcement_support_entry*
	dvb_announcement_support_descriptor_entries_next(struct dvb_announcement_support_descriptor *d,
							 struct dvb_announcement_support_entry *pos)
{
	uint8_t *end = (uint8_t*) d + 2 + d->d.len;
	uint8_t* next = (uint8_t*) pos + sizeof(struct dvb_announcement_support_entry);
	struct dvb_announcement_support_reference* reference =
		dvb_announcement_support_entry_reference(pos);

	if (reference)
		next += sizeof(struct dvb_announcement_support_reference);

	if (next >= end)
		return NULL;

	return (struct dvb_announcement_support_entry *) next;
}

#ifdef __cplusplus
}
#endif

#endif
