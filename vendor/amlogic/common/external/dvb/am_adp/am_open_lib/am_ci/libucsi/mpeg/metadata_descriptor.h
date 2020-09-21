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

#ifndef _UCSI_MPEG_METADATA_DESCRIPTOR
#define _UCSI_MPEG_METADATA_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>

/**
 * Values for the decoder_config_flags field.
 */
enum {
	MPEG_DECODER_CONFIG_NONE				= 0x00,
	MPEG_DECODER_CONFIG_IN_DECODER_CONFIG			= 0x01,
	MPEG_DECODER_CONFIG_SAME_METADATA_SERVICE		= 0x02,
	MPEG_DECODER_CONFIG_DSMCC				= 0x03,
	MPEG_DECODER_CONFIG_SAME_PROGRAM			= 0x04,
};

/**
 * mpeg_metadata_descriptor structure.
 */
struct mpeg_metadata_descriptor {
	struct descriptor d;

	uint16_t metadata_application_format;
	/* struct mpeg_metadata_descriptor_application_format_identifier appid */
	/* uint8_t metadata_format */
	/* struct mpeg_metadata_descriptor_format_identifier formid */
	/* struct mpeg_metadata_descriptor_flags flags */
	/* struct mpeg_metadata_descriptor_service_identifier service_identifier */
	/* struct mpeg_metadata_descriptor_decoder_config decoder_config */
	/* struct mpeg_metadata_descriptor_decoder_config_id_record decoder_config_id_record */
	/* struct mpeg_metadata_descriptor_decoder_config_service_id decoder_config_service_id */
	/* struct mpeg_metadata_descriptor_decoder_config_reserved decoder_config_reserved */
	/* uint8_t private_data[] */
} __ucsi_packed;

/**
 * appid field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_application_format_identifier {
	uint32_t id;
} __ucsi_packed;

/**
 * formid field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_format_identifier {
	uint32_t id;
} __ucsi_packed;

/**
 * Flags field of a metadata_descriptor
 */
struct mpeg_metadata_descriptor_flags {
	uint8_t metadata_service_id;
  EBIT3(uint8_t decoder_config_flags				: 3;  ,
	uint8_t dsm_cc_flag					: 1;  ,
	uint8_t reserved					: 4;  );
} __ucsi_packed;

/**
 * service_identifier field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_service_identifier {
	uint8_t service_identification_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * decoder_config field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_decoder_config {
	uint8_t decoder_config_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * decoder_config_id_record field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_decoder_config_id_record {
	uint8_t decoder_config_id_record_length;
	/* uint8_t data[] */
} __ucsi_packed;

/**
 * decoder_config_service_id field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_decoder_config_service_id {
	uint8_t decoder_config_metadata_service_id;
} __ucsi_packed;

/**
 * decoder_config_reserved field of a metadata_descriptor.
 */
struct mpeg_metadata_descriptor_decoder_config_reserved {
	uint8_t reserved_data_length;
	/* uint8_t data[] */
} __ucsi_packed;




/**
 * Process an mpeg_metadata_descriptor.
 *
 * @param d Generic descriptor.
 * @return Pointer to an mpeg_metadata_descriptor, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor*
	mpeg_metadata_descriptor_codec(struct descriptor* d)
{
	uint32_t pos = 2;
	uint8_t *buf = (uint8_t*) d;
	uint32_t len = d->len + 2;
	struct mpeg_metadata_descriptor_flags *flags;
	int id;

	if (len < sizeof(struct mpeg_metadata_descriptor))
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

	if (len < (pos + sizeof(struct mpeg_metadata_descriptor_flags)))
		return NULL;
	flags = (struct mpeg_metadata_descriptor_flags*) (buf+pos);
	pos += sizeof(struct mpeg_metadata_descriptor_flags);

	if (flags->dsm_cc_flag == 1) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if (flags->decoder_config_flags == 1) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if (flags->decoder_config_flags == 3) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if (flags->decoder_config_flags == 4) {
		if (len < (pos+1))
			return NULL;
		pos++;
	}

	if ((flags->decoder_config_flags == 5) ||
	    (flags->decoder_config_flags == 6)) {
		if (len < (pos+1))
			return NULL;
		if (len < (pos+1+buf[pos]))
			return NULL;
		pos += 1 + buf[pos];
	}

	if (len < pos)
		return NULL;

	return (struct mpeg_metadata_descriptor*) d;
}

/**
 * Accessor for pointer to appid field of an mpeg_metadata_descriptor.
 *
 * @param d The mpeg_metadata_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_application_format_identifier*
	mpeg_metadata_descriptor_appid(struct mpeg_metadata_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor);

	if (d->metadata_application_format != 0xffff)
		return NULL;
	return (struct mpeg_metadata_descriptor_application_format_identifier*) buf;
}

/**
 * Accessor for metadata_format field of an mpeg_metadata_descriptor.
 *
 * @param d The mpeg_metadata_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline uint8_t
	mpeg_metadata_descriptor_metadata_format(struct mpeg_metadata_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor);

	if (d->metadata_application_format == 0xffff)
		buf+=4;
	return *buf;
}

/**
 * Accessor for pointer to formid field of an mpeg_metadata_descriptor.
 *
 * @param d The mpeg_metadata_descriptor structure.
 * @return The pointer, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_format_identifier*
        mpeg_metadata_descriptor_formid(struct mpeg_metadata_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor);

	if (d->metadata_application_format == 0xffff)
		buf+=4;
	if (*buf != 0xff)
		return NULL;

	return (struct mpeg_metadata_descriptor_format_identifier*) (buf+1);
}

/**
 * Accessor for flags field of an mpeg_metadata_descriptor.
 *
 * @param d The mpeg_metadata_descriptor structure.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_flags*
	mpeg_metadata_descriptor_flags(struct mpeg_metadata_descriptor *d)
{
	uint8_t *buf = (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor);

	if (d->metadata_application_format == 0xffff)
		buf+=4;
	if (*buf == 0xff)
		buf+=4;

	return (struct mpeg_metadata_descriptor_flags*) buf;
}


/**
 * Accessor for service_identifier field of an mpeg_metadata_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_service_identifier*
	mpeg_metadata_descriptor_sevice_identifier(struct mpeg_metadata_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_descriptor_flags);

	if (flags->dsm_cc_flag!=1)
		return NULL;

	return (struct mpeg_metadata_descriptor_service_identifier *) buf;
}

/**
 * Accessor for data field of an mpeg_metadata_descriptor_service_identifier.
 *
 * @param d The mpeg_metadata_descriptor_service_identifier structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_descriptor_service_identifier_data(struct mpeg_metadata_descriptor_service_identifier *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor_service_identifier);
}

/**
 * Accessor for decoder_config field of an mpeg_metadata_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_decoder_config*
	mpeg_metadata_descriptor_decoder_config(struct mpeg_metadata_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_descriptor_flags);

	if (flags->decoder_config_flags != 1)
		return NULL;

	if (flags->dsm_cc_flag==1)
		buf += 1 + buf[1];

	return (struct mpeg_metadata_descriptor_decoder_config*) buf;
}

/**
 * Accessor for data field of an mpeg_metadata_descriptor_service_identifier.
 *
 * @param d The mpeg_metadata_descriptor_service_identifier structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_descriptor_decoder_config_data(struct mpeg_metadata_descriptor_decoder_config *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor_decoder_config);
}

/**
 * Accessor for decoder_config_id_record field of an mpeg_metadata_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_decoder_config_id_record*
	mpeg_metadata_descriptor_decoder_config_id_record(struct mpeg_metadata_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_descriptor_flags);

	if (flags->decoder_config_flags != 3)
		return NULL;

	if (flags->dsm_cc_flag==1)
		buf += 1 + buf[1];

	return (struct mpeg_metadata_descriptor_decoder_config_id_record *) buf;
}

/**
 * Accessor for data field of an mpeg_metadata_descriptor_decoder_config_id_record.
 *
 * @param d The mpeg_metadata_descriptor_decoder_config_id_record structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_descriptor_decoder_config_id_record_data(struct mpeg_metadata_descriptor_decoder_config_id_record *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor_decoder_config_id_record);
}

/**
 * Accessor for decoder_config_service_id field of an mpeg_metadata_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_decoder_config_service_id*
	mpeg_metadata_descriptor_decoder_config_service_id(struct mpeg_metadata_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_descriptor_flags);

	if (flags->decoder_config_flags != 4)
		return NULL;

	if (flags->dsm_cc_flag==1)
		buf += 1 + buf[1];

	return (struct mpeg_metadata_descriptor_decoder_config_service_id *) buf;
}

/**
 * Accessor for decoder_config_reserved field of an mpeg_metadata_descriptor.
 *
 * @param flags Pointer to the mpeg_metadata_descriptor_flags.
 * @return Pointer to the field, or NULL on error.
 */
static inline struct mpeg_metadata_descriptor_decoder_config_reserved*
	mpeg_metadata_descriptor_decoder_config_reserved(struct mpeg_metadata_descriptor_flags *flags)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_descriptor_flags);

	if ((flags->decoder_config_flags != 5) && (flags->decoder_config_flags != 6))
		return NULL;

	if (flags->dsm_cc_flag==1)
		buf += 1 + buf[1];

	return (struct mpeg_metadata_descriptor_decoder_config_reserved *) buf;
}

/**
 * Accessor for data field of an mpeg_metadata_descriptor_decoder_config_reserved.
 *
 * @param d The mpeg_metadata_descriptor_decoder_config_reserved structure.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_descriptor_decoder_config_reserved_data(struct mpeg_metadata_descriptor_decoder_config_reserved *d)
{
	return (uint8_t*) d + sizeof(struct mpeg_metadata_descriptor_decoder_config_reserved);
}

/**
 * Accessor for private_data field of an mpeg_metadata_descriptor.
 *
 * @param d The mpeg_metadata_descriptor structure.
 * @param flags Pointer to the mpeg_metadata_descriptor_flags.
 * @param length Where the number of bytes in the field should be stored.
 * @return Pointer to the field.
 */
static inline uint8_t*
	mpeg_metadata_descriptor_private_data(struct mpeg_metadata_descriptor *d,
					      struct mpeg_metadata_descriptor_flags *flags,
					      int *length)
{
	uint8_t *buf = (uint8_t*) flags + sizeof(struct mpeg_metadata_descriptor_flags);
	uint8_t *end = (uint8_t*) d + d->d.len + 2;


	if (flags->dsm_cc_flag==1)
		buf += 1 + buf[1];
	if (flags->decoder_config_flags==1)
		buf += 1 + buf[1];
	if (flags->decoder_config_flags==3)
		buf += 1 + buf[1];
	if (flags->decoder_config_flags==4)
		buf++;
	if ((flags->decoder_config_flags==5)||(flags->decoder_config_flags==6))
		buf += 1 + buf[1];

	*length = end - buf;
	return buf;
}

#ifdef __cplusplus
}
#endif

#endif
