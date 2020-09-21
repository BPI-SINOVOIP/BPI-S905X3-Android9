/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gpxe/uaccess.h>
#include <gpxe/smbios.h>

/** @file
 *
 * System Management BIOS
 *
 */

/** SMBIOS entry point descriptor */
static struct smbios smbios = {
	.address = UNULL,
};

/**
 * Find SMBIOS strings terminator
 *
 * @v offset		Offset to start of strings
 * @ret offset		Offset to strings terminator, or 0 if not found
 */
static size_t find_strings_terminator ( size_t offset ) {
	size_t max_offset = ( smbios.len - 2 );
	uint16_t nulnul;

	for ( ; offset <= max_offset ; offset++ ) {
		copy_from_user ( &nulnul, smbios.address, offset, 2 );
		if ( nulnul == 0 )
			return ( offset + 1 );
	}
	return 0;
}

/**
 * Find specific structure type within SMBIOS
 *
 * @v type		Structure type to search for
 * @v structure		SMBIOS structure descriptor to fill in
 * @ret rc		Return status code
 */
int find_smbios_structure ( unsigned int type,
			    struct smbios_structure *structure ) {
	unsigned int count = 0;
	size_t offset = 0;
	size_t strings_offset;
	size_t terminator_offset;
	int rc;

	/* Find SMBIOS */
	if ( ( smbios.address == UNULL ) &&
	     ( ( rc = find_smbios ( &smbios ) ) != 0 ) )
		return rc;
	assert ( smbios.address != UNULL );

	/* Scan through list of structures */
	while ( ( ( offset + sizeof ( structure->header ) ) < smbios.len )
		&& ( count < smbios.count ) ) {

		/* Read next SMBIOS structure header */
		copy_from_user ( &structure->header, smbios.address, offset,
				 sizeof ( structure->header ) );

		/* Determine start and extent of strings block */
		strings_offset = ( offset + structure->header.len );
		if ( strings_offset > smbios.len ) {
			DBG ( "SMBIOS structure at offset %zx with length "
			      "%x extends beyond SMBIOS\n", offset,
			      structure->header.len );
			return -ENOENT;
		}
		terminator_offset = find_strings_terminator ( strings_offset );
		if ( ! terminator_offset ) {
			DBG ( "SMBIOS structure at offset %zx has "
			      "unterminated strings section\n", offset );
			return -ENOENT;
		}
		structure->strings_len = ( terminator_offset - strings_offset);

		DBG ( "SMBIOS structure at offset %zx has type %d, length %x, "
		      "strings length %zx\n", offset, structure->header.type,
		      structure->header.len, structure->strings_len );

		/* If this is the structure we want, return */
		if ( structure->header.type == type ) {
			structure->offset = offset;
			return 0;
		}

		/* Move to next SMBIOS structure */
		offset = ( terminator_offset + 1 );
		count++;
	}

	DBG ( "SMBIOS structure type %d not found\n", type );
	return -ENOENT;
}

/**
 * Copy SMBIOS structure
 *
 * @v structure		SMBIOS structure descriptor
 * @v data		Buffer to hold SMBIOS structure
 * @v len		Length of buffer
 * @ret rc		Return status code
 */
int read_smbios_structure ( struct smbios_structure *structure,
			    void *data, size_t len ) {

	assert ( smbios.address != UNULL );

	if ( len > structure->header.len )
		len = structure->header.len;
	copy_from_user ( data, smbios.address, structure->offset, len );
	return 0;
}

/**
 * Find indexed string within SMBIOS structure
 *
 * @v structure		SMBIOS structure descriptor
 * @v index		String index
 * @v data		Buffer for string
 * @v len		Length of string buffer
 * @ret rc		Length of string, or negative error
 */
int read_smbios_string ( struct smbios_structure *structure,
			 unsigned int index, void *data, size_t len ) {
	size_t strings_start = ( structure->offset + structure->header.len );
	size_t strings_end = ( strings_start + structure->strings_len );
	size_t offset;
	size_t string_len;

	assert ( smbios.address != UNULL );

	/* String numbers start at 1 (0 is used to indicate "no string") */
	if ( ! index )
		return -ENOENT;

	for ( offset = strings_start ; offset < strings_end ;
	      offset += ( string_len + 1 ) ) {
		/* Get string length.  This is known safe, since the
		 * smbios_strings struct is constructed so as to
		 * always end on a string boundary.
		 */
		string_len = strlen_user ( smbios.address, offset );
		if ( --index == 0 ) {
			/* Copy string, truncating as necessary. */
			if ( len > string_len )
				len = string_len;
			copy_from_user ( data, smbios.address, offset, len );
			return string_len;
		}
	}

	DBG ( "SMBIOS string index %d not found\n", index );
	return -ENOENT;
}
