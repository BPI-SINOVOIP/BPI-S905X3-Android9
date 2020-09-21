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

#ifndef _UCSI_MPEG_CONTENT_LABELLING_DESCRIPTOR
#define _UCSI_MPEG_CONTENT_LABELLING_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Possible values for content_time_base_indicator.
 */
enum {
	MPEG_CONTENT_TIME_BASE_STC			= 0x01,
	MPEG_CONTENT_TIME_BASE_NPT			= 0x02,
};

/**
 * mpeg_content_labelling_descriptor structure.
 */
struct mpeg_content_labelling_descriptor {
	struct descriptor d;

	uint16_t metadata_application_format;
	/* struct mpeg_content_labelling_descriptor_application_format_identifier id */
	/* struct mpeg_content_labelling_descriptor_flags flags */
	/* struct mpeg_content_labelling_descriptor_reference_id reference_id */
	/* struct mpeg_content_labelling_descriptor_time_base time_base */
	/* struct mpeg_content_labelling_descriptor_content_id content_id */
	/* struct mpeg_content_labelling_descriptor_time_base_association time_base_assoc */
	/* uint8_t private_data[] */
} __ucsi_packed;

/**
 * id field of a content_labelling_descriptor.
 */
struct mpeg_content_labelling_descriptor_application_format_identifier {
	uint32_t id;
} __ucsi_packed;

/**
 * Flags field of a content_labelling_descriptor
 */
struct mpeg_content_labelling_descriptor_flags {
  EBIT3(uint8_t content_reference_id_record_flag		: 1;  ,
	uint8_t content_time_base_indicator			: 4;  ,
	uint8_t reserved					: 3;  );
} __ucsi_packed;

/**
 * Reference_id field of a content_labelling_descriptor.
 */
struct mpeg_content_labelling_descriptor_reference_id {
	uint8_t content_reference_id_record_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * time_base field of a content_labelling_descriptor.
 */
struct mpeg_content_labelling_descriptor_time_base {
  EBIT2(uint64_t reserved_1					: 7;  ,
	uint64_t content_time_base_value			:33;  );
  EBIT2(uint64_t reserved_2					: 7;  ,
	uint64_t metadata_time_base_value			:33;  );
} __ucsi_packed;

/**
 * content_id field of a content_labelling_descriptor.
 */
struct mpeg_content_labelling_descriptor_content_id {
  EBIT2(uint8_t reserved					: 1;  ,
	uint8_t contentId					: 7;  );
} __ucsi_packed;

/**
 * time_base_assoc field of a content_labelling_descriptor.
 */
struct mpeg_content_labelling_descriptor_time_base_association {
	uint8_t time_base_association_data_length;
	/* uint8_t data[] */
} __ucsi_packed;



/**
 * Process an mpeg_content_labelling_descriptor.
 *
 * @param d Generic descriptor.
 * @return Pointer to an mpeg_content_labelling_descriptor, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor*
	mpeg_content_labelling_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 2;
	uint8_t *buf = (uint8_t*) d;
	uint32_t len = d->len + 2;
	struct mpeg_content_labelling_descriptor_flags *flags;
	int id;

	if (len < sizeof(struct mpeg_content_labelling_descriptor))
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

	if (len < (pos + sizeof(struct mpeg_content_labelling_descriptor_flags)))
		return NULL;
	flags = (struct mpeg_content_labelling_descriptor_flags*) (buf+pos);
	pos += sizeof(struct mpeg_content_labelling_descriptor_flags);

	if (flags->content_reference_id_record_flag == 1) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if ((flags->content_time_base_indicator == 1) ||
	    (flags->content_time_base_indicator == 2)) {
		if (len < (pos + sizeof(struct mpeg_content_labelling_descriptor_time_base)))
			return NULL;
		bswap40(buf+pos);
		bswap40(buf+pos+5);
		pos += sizeof(struct mpeg_content_labelling_descriptor_time_base);
	}

	if (flags->content_time_base_indicator == 2) {
		if (len < (pos + sizeof(struct mpeg_content_labelling_descriptor_content_id)))
			return NULL;
		pos += sizeof(struct mpeg_content_labelling_descriptor_content_id);
	}

	if (flags->content_time_base_indicator > 2) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if (len < pos)
		return NULL;

	return (struct mpeg_content_labelling_descriptor*) d;
}

/**
 * Accessor for pointer to id field of an mpeg_content_labelling_descriptor.
 *
 * @param d The mpeg_content_labelling_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor_application_format_identifier*
	mpeg_content_labelling_descriptor_id(struct mpeg_content_labelling_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d;

	if (d->metadata_application_format != 0xffff)
		return NULL;
	return (struct mpeg_content_labelling_descriptor_application_format_identifier*)
		(buf + sizeof(struct mpeg_content_labelling_descriptor));
}

/**
 * Accessor for pointer to flags field of an mpeg_content_labelling_descriptor.
 *
 * @param d The mpeg_content_labelling_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor_flags*
        mpeg_content_labelling_descriptor_flags(struct mpeg_content_labelling_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_content_labelling_descriptor);

	if (d->metadata_application_format != 0xffff)
		buf += 4;

	return (struct mpeg_content_labelling_descriptor_flags *) buf;
}

/**
 * Accessor for reference_id field of an mpeg_content_labelling_descriptor.
 *
 * @param flags Pointer to the mpeg_content_labelling_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor_reference_id*
	mpeg_content_labelling_descriptor_reference_id(struct mpeg_content_labelling_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_content_labelling_descriptor_flags);

	if (flags->content_reference_id_record_flag != 1)
		return NULL;

	return (struct mpeg_content_labelling_descriptor_reference_id *) buf;
}

/**
 * Accessor for data field of an mpeg_content_reference_id.
 *
 * @param d The mpeg_content_reference_id structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_content_reference_id_data(struct mpeg_content_labelling_descriptor_reference_id *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_content_labelling_descriptor_reference_id);
}

/**
 * Accessor for time_base field of an mpeg_content_labelling_descriptor.
 *
 * @param flags Pointer to the mpeg_content_labelling_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor_time_base*
	mpeg_content_labelling_descriptor_time_base(struct mpeg_content_labelling_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_content_labelling_descriptor_flags);

	if ((flags->content_time_base_indicator!=1) && (flags->content_time_base_indicator!=2))
		return NULL;

	if (flags->content_reference_id_record_flag == 1)
		buf += 1 + buf[1];

	return (struct mpeg_content_labelling_descriptor_time_base *) buf;
}

/**
 * Accessor for content_id field of an mpeg_content_labelling_descriptor.
 *
 * @param flags Pointer to the mpeg_content_labelling_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor_content_id*
	mpeg_content_labelling_descriptor_content_id(struct mpeg_content_labelling_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_content_labelling_descriptor_flags);

	if (flags->content_time_base_indicator!=2)
		return NULL;

	if (flags->content_reference_id_record_flag == 1)
		buf += 1 + buf[1];
	if ((flags->content_time_base_indicator==1) || (flags->content_time_base_indicator==2))
		buf += sizeof(struct mpeg_content_labelling_descriptor_time_base);

	return (struct mpeg_content_labelling_descriptor_content_id *) buf;
}

/**
 * Accessor for time_base_association field of an mpeg_content_labelling_descriptor.
 *
 * @param flags Pointer to the mpeg_content_labelling_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_content_labelling_descriptor_time_base_association*
	mpeg_content_labelling_descriptor_time_base_assoc(struct mpeg_content_labelling_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_content_labelling_descriptor_flags);

	if (flags->content_time_base_indicator<3)
		return NULL;

	if (flags->content_reference_id_record_flag == 1)
		buf += 1 + buf[1];
	if ((flags->content_time_base_indicator==1) || (flags->content_time_base_indicator==2))
		buf += sizeof(struct mpeg_content_labelling_descriptor_time_base);
	if (flags->content_time_base_indicator==2)
		buf += sizeof(struct mpeg_content_labelling_descriptor_content_id);

	return (struct mpeg_content_labelling_descriptor_time_base_association *) buf;
}

/**
 * Accessor for data field of an mpeg_time_base_association.
 *
 * @param d The mpeg_time_base_association structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_time_base_association_data(struct mpeg_content_labelling_descriptor_time_base_association *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_content_labelling_descriptor_time_base_association);
}


/**
 * Accessor for private_data field of an mpeg_content_labelling_descriptor.
 *
 * @param d The mpeg_content_labelling_descriptor structure.
 * @param flags Pointer to the mpeg_content_labelling_descriptor_flags.
 * @param length Where the number of bytes in the field should be stored.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_content_labelling_descriptor_data(struct mpeg_content_labelling_descriptor *d,
					       struct mpeg_content_labelling_descriptor_flags *flags,
					       int *length)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_content_labelling_descriptor_flags);
	uint8_t *end = (uint8_t*) d + d->d.len + 2;

	if (flags->content_reference_id_record_flag == 1)
		buf += 1 + buf[1];
	if ((flags->content_time_base_indicator==1) || (flags->content_time_base_indicator==2))
		buf += sizeof(struct mpeg_content_labelling_descriptor_time_base);
	if (flags->content_time_base_indicator==2)
		buf += sizeof(struct mpeg_content_labelling_descriptor_content_id);
	if (flags->content_time_base_indicator<3)
		buf += 1 + buf[1];

	*length = end - buf;

	return buf;
}

#ifdef __cplusplus
}
#endif

#endif
