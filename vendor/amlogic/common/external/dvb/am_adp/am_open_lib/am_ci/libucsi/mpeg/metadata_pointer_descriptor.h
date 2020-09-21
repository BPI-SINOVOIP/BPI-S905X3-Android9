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

#ifndef _UCSI_MPEG_METADATA_POINTER_DESCRIPTOR
#define _UCSI_MPEG_METADATA_POINTER_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for the mpeg_carriage_flags field.
 */
enum {
	MPEG_CARRIAGE_SAME_TS					= 0x00,
	MPEG_CARRIAGE_DIFFERENT_TS				= 0x01,
	MPEG_CARRIAGE_PS					= 0x02,
	MPEG_CARRIAGE_OTHER					= 0x03,
};

/**
 * mpeg_metadata_pointer_descriptor structure.
 */
struct mpeg_metadata_pointer_descriptor {
	struct descriptor d;

	uint16_t metadata_application_format;
	/* struct mpeg_metadata_pointer_descriptor_application_format_identifier appid */
	/* uint8_t metadata_format */
	/* struct mpeg_metadata_pointer_descriptor_format_identifier formid */
	/* struct mpeg_metadata_pointer_descriptor_flags flags */
	/* struct mpeg_metadata_pointer_descriptor_locator locator */
	/* struct mpeg_metadata_pointer_descriptor_program_number program_number */
	/* struct mpeg_metadata_pointer_descriptor_carriage carriage */
	/* uint8_t private_data[] */
} __ucsi_packed;

/**
 * appid field of a metadata_pointer_descriptor.
 */
struct mpeg_metadata_pointer_descriptor_application_format_identifier {
	uint32_t id;
} __ucsi_packed;

/**
 * formid field of a metadata_pointer_descriptor.
 */
struct mpeg_metadata_pointer_descriptor_format_identifier {
	uint32_t id;
} __ucsi_packed;

/**
 * Flags field of a metadata_pointer_descriptor
 */
struct mpeg_metadata_pointer_descriptor_flags {
	uint8_t metadata_service_id;
  EBIT3(uint8_t metadata_locator_record_flag			: 1;  ,
	uint8_t mpeg_carriage_flags				: 2;  ,
	uint8_t reserved					: 5;  );
} __ucsi_packed;

/**
 * Reference_id field of a metadata_pointer_descriptor.
 */
struct mpeg_metadata_pointer_descriptor_locator {
	uint8_t metadata_locator_record_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * program_number field of a metadata_pointer_descriptor.
 */
struct mpeg_metadata_pointer_descriptor_program_number {
	uint16_t number;
} __ucsi_packed;

/**
 * carriage field of a metadata_pointer_descriptor.
 */
struct mpeg_metadata_pointer_descriptor_carriage {
	uint16_t transport_stream_location;
	uint16_t transport_stream_id;
} __ucsi_packed;




/**
 * Process an mpeg_metadata_pointer_descriptor.
 *
 * @param d Generic descriptor.
 * @return Pointer to an mpeg_metadata_pointer_descriptor, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor*
	mpeg_metadata_pointer_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 2;
	uint8_t *buf = (uint8_t*) d;
	uint32_t len = d->len + 2;
	struct mpeg_metadata_pointer_descriptor_flags *flags;
	int id;

	if (len < sizeof(struct mpeg_metadata_pointer_descriptor))
		return NULL;

	bswap16(buf + pos);
	id = *((uint16_t*) (buf+pos));
	pos += 2;

	if (id == 0xffff) {
		if (len < (pos+4))
			return NULL;
		bswap32(buf+pos);
		pos += 4;
	}

	if (len < (pos+1))
		return NULL;

	id = buf[pos];
	pos++;
	if (id == 0xff) {
		if (len < (pos+4))
			return NULL;
		bswap32(buf+pos);
		pos += 4;
	}

	if (len < (pos + sizeof(struct mpeg_metadata_pointer_descriptor_flags)))
		return NULL;
	flags = (struct mpeg_metadata_pointer_descriptor_flags*) (buf+pos);
	pos += sizeof(struct mpeg_metadata_pointer_descriptor_flags);

	if (flags->metadata_locator_record_flag == 1) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if (flags->mpeg_carriage_flags < 3) {
		if (len < (pos + 2))
			return NULL;
		bswap16(buf+pos);
		pos += 2;
	}

	if (flags->mpeg_carriage_flags == 1) {
		if (len < (pos + 4))
			return NULL;
		bswap16(buf+pos);
		bswap16(buf+pos+2);
		pos += 4;
	}

	if (len < pos)
		return NULL;

	return (struct mpeg_metadata_pointer_descriptor*) d;
}

/**
 * Accessor for pointer to appid field of an mpeg_metadata_pointer_descriptor.
 *
 * @param d The mpeg_metadata_pointer_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor_application_format_identifier*
	mpeg_metadata_pointer_descriptor_appid(struct mpeg_metadata_pointer_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_pointer_descriptor);

	if (d->metadata_application_format != 0xffff)
		return NULL;
	return (struct mpeg_metadata_pointer_descriptor_application_format_identifier*) buf;
}

/**
 * Accessor for metadata_format field of an mpeg_metadata_pointer_descriptor.
 *
 * @param d The mpeg_metadata_pointer_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline uint8_t
	mpeg_metadata_pointer_descriptor_metadata_format(struct mpeg_metadata_pointer_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_pointer_descriptor);

	if (d->metadata_application_format == 0xffff)
		buf+=4;
	return *buf;
}

/**
 * Accessor for pointer to formid field of an mpeg_metadata_pointer_descriptor.
 *
 * @param d The mpeg_metadata_pointer_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor_format_identifier*
        mpeg_metadata_pointer_descriptor_formid(struct mpeg_metadata_pointer_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_pointer_descriptor);

	if (d->metadata_application_format == 0xffff)
		buf+=4;
	if (*buf != 0xff)
		return NULL;

	return (struct mpeg_metadata_pointer_descriptor_format_identifier*) (buf+1);
}

/**
 * Accessor for flags field of an mpeg_metadata_pointer_descriptor.
 *
 * @param d The mpeg_metadata_pointer_descriptor structure.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor_flags*
	mpeg_metadata_pointer_descriptor_flags(struct mpeg_metadata_pointer_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_pointer_descriptor);

	if (d->metadata_application_format == 0xffff)
		buf+=4;
	if (*buf == 0xff)
		buf+=4;

	return (struct mpeg_metadata_pointer_descriptor_flags*) buf;
}


/**
 * Accessor for locator field of an mpeg_metadata_pointer_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_pointer_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor_locator*
	mpeg_metadata_pointer_descriptor_locator(struct mpeg_metadata_pointer_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_pointer_descriptor_flags);

	if (flags->metadata_locator_record_flag!=1)
		return NULL;

	return (struct mpeg_metadata_pointer_descriptor_locator *) buf;
}

/**
 * Accessor for data field of an mpeg_metadata_pointer_descriptor_locator.
 *
 * @param d The mpeg_metadata_pointer_descriptor_locator structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_pointer_descriptor_locator_data(struct mpeg_metadata_pointer_descriptor_locator *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_metadata_pointer_descriptor_locator);
}


/**
 * Accessor for program_number field of an mpeg_metadata_pointer_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_pointer_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor_program_number*
	mpeg_metadata_pointer_descriptor_program_number(struct mpeg_metadata_pointer_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_pointer_descriptor_flags);

	if (flags->mpeg_carriage_flags < 3)
		return NULL;

	if (flags->metadata_locator_record_flag==1)
		buf += 1 + buf[1];

	return (struct mpeg_metadata_pointer_descriptor_program_number*) buf;
}

/**
 * Accessor for carriage field of an mpeg_metadata_pointer_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_pointer_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_pointer_descriptor_carriage*
	mpeg_metadata_pointer_descriptor_carriage(struct mpeg_metadata_pointer_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_pointer_descriptor_flags);

	if (flags->mpeg_carriage_flags != 1)
		return NULL;

	if (flags->metadata_locator_record_flag==1)
		buf += 1 + buf[1];
	if (flags->mpeg_carriage_flags < 3)
		buf += sizeof(struct mpeg_metadata_pointer_descriptor_program_number);

	return (struct mpeg_metadata_pointer_descriptor_carriage *) buf;
}

/**
 * Accessor for private_data field of an mpeg_metadata_pointer_descriptor.
 *
 * @param d The mpeg_metadata_pointer_descriptor structure.
 * @param flags Pointer to the mpeg_metadata_pointer_descriptor_flags.
 * @param length Where the number of bytes in the field should be stored.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_pointer_descriptor_private_data(struct mpeg_metadata_pointer_descriptor *d,
								struct mpeg_metadata_pointer_descriptor_flags *flags,
								int *length)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_pointer_descriptor_flags);
	uint8_t *end = (uint8_t*) d + d->d.len + 2;


	if (flags->metadata_locator_record_flag==1)
		buf += 1 + buf[1];
	if (flags->mpeg_carriage_flags < 3)
		buf += sizeof(struct mpeg_metadata_pointer_descriptor_program_number);
	if (flags->mpeg_carriage_flags != 1)
		buf += sizeof(struct mpeg_metadata_pointer_descriptor_carriage);

	*length = end - buf;
	return buf;
}

#ifdef __cplusplus
}
#endif

#endif
