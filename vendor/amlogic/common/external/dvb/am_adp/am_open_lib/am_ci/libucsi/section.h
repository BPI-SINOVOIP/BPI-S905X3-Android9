/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
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

#ifndef _UCSI_SECTION_H
#define _UCSI_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/endianops.h>
#include <libucsi/descriptor.h>
#include <libucsi/crc32.h>
#include <stdint.h>
#include <string.h>

#define CRC_SIZE 4


/**
 * Generic section header.
 */
struct section {
	uint8_t	table_id;
  EBIT4(uint16_t syntax_indicator	: 1; ,
	uint16_t private_indicator	: 1; , /* 2.4.4.10 */
	uint16_t reserved 		: 2; ,
	uint16_t length			:12; );
} __ucsi_packed;

/**
 * Generic extended section header structure.
 */
struct section_ext {
	uint8_t	table_id;
  EBIT4(uint16_t syntax_indicator	: 1; ,
	uint16_t private_indicator	: 1; , /* 2.4.4.10 */
	uint16_t reserved		: 2; ,
	uint16_t length			:12; );

	uint16_t table_id_ext;
  EBIT3(uint8_t reserved1		: 2; ,
	uint8_t version_number		: 5; ,
	uint8_t current_next_indicator	: 1; );
	uint8_t section_number;
	uint8_t last_section_number;
} __ucsi_packed;

/**
 * Structure for keeping track of sections of a PSI table.
 */
struct psi_table_state {
	uint8_t version_number;
	uint16_t next_section_number;
	uint8_t complete:1;
	uint8_t new_table:1;
} __ucsi_packed;


/**
 * Determine the total length of a section, including the header.
 *
 * @param section The parsed section structure.
 * @return The length.
 */
static inline size_t section_length(struct section *section)
{
	return section->length + sizeof(struct section);
}

/**
 * Determine the total length of an extended section, including the header,
 * but omitting the CRC.
 *
 * @param section The parsed section_ext structure.
 * @return The length.
 */
static inline size_t section_ext_length(struct section_ext * section)
{
	return section->length + sizeof(struct section) - CRC_SIZE;
}

/**
 * Process a section structure in-place.
 *
 * @param buf Pointer to the data.
 * @param len Length of data.
 * @return Pointer to the section structure, or NULL if invalid.
 */
static inline struct section * section_codec(uint8_t * buf, size_t len)
{
	struct section * ret = (struct section *)buf;

	if (len < 3)
		return NULL;

	bswap16(buf+1);

	if (len != ret->length + 3U)
		return NULL;

	return ret;
}

/**
 * Some sections have a CRC even though they are not section_exts.
 * This function is to allow checking of them.
 *
 * @param section Pointer to the processed section structure.
 * @return Nonzero on error, or 0 if the CRC was correct.
 */
static inline int section_check_crc(struct section *section)
{
	uint8_t * buf = (uint8_t *) section;
	size_t len = section_length(section);
	uint32_t crc;

	/* the crc check has to be performed on the unswapped data */
	bswap16(buf+1);
	crc = crc32(CRC32_INIT, buf, len);
	bswap16(buf+1);

	/* the crc check includes the crc value,
	 * the result should therefore be zero.
	 */
	if (crc)
		return -1;
	return 0;
}


/**
 * Decode an extended section structure.
 *
 * @param section Pointer to the processed section structure.
 * @param check_crc If 1, the CRC of the section will also be checked.
 * @return Pointer to the parsed section_ext structure, or NULL if invalid.
 */
static inline struct section_ext * section_ext_decode(struct section * section,
						      int check_crc)
{
	if (section->syntax_indicator == 0)
		return NULL;

	if (check_crc) {
		if (section_check_crc(section))
			return NULL;
	}

	bswap16((uint8_t *)section + sizeof(struct section));

	return (struct section_ext *)section;
}

/**
 * Encode an extended section structure for transmission.
 *
 * @param section Pointer to the section_ext structure.
 * @param update_crc If 1, the CRC of the section will also be updated.
 * @return Pointer to the encoded section_ext structure, or NULL if invalid.
 */
static inline struct section_ext * section_ext_encode(struct section_ext* section,
						      int update_crc)
{
	if (section->syntax_indicator == 0)
		return NULL;

	bswap16((uint8_t *)section + sizeof(struct section));

	if (update_crc) {
		uint8_t * buf = (uint8_t *) section;
		int len = sizeof(struct section) + section->length;
		uint32_t crc;

		/* the crc has to be performed on the swapped data */
		bswap16(buf+1);
		crc = crc32(CRC32_INIT, buf, len-4);
		bswap16(buf+1);

		/* update the CRC */
		*((uint32_t*) (buf+len-4)) = crc;
		bswap32(buf+len-4);
	}

	return (struct section_ext *)section;
}

/**
 * Reset a psi_table_state structure.
 *
 * @param tstate The structure to reset.
 */
static inline void psi_table_state_reset(struct psi_table_state *tstate)
{
	tstate->version_number = 0xff;
}

/**
 * Check if a supplied section_ext is something we want to process.
 *
 * @param section The parsed section_ext structure.
 * @param tstate The state structure for this PSI table.
 * @return 0=> not useful. nonzero => useful.
 */
static inline int section_ext_useful(struct section_ext *section, struct psi_table_state *tstate)
{
	if ((section->version_number == tstate->version_number) && tstate->complete)
		return 0;
	if (section->version_number != tstate->version_number) {
	        if (section->section_number != 0)
	                return 0;

		tstate->next_section_number = 0;
		tstate->complete = 0;
		tstate->version_number = section->version_number;
		tstate->new_table = 1;
	} else if (section->section_number == tstate->next_section_number) {
		tstate->new_table = 0;
	} else {
		return 0;
	}

	tstate->next_section_number++;
	if (section->last_section_number < tstate->next_section_number) {
		tstate->complete = 1;
	}

	return 1;
}

#ifdef __cplusplus
}
#endif

#endif
