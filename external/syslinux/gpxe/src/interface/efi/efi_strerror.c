/*
 * Copyright (C) 2008 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <stdio.h>
#include <gpxe/efi/efi.h>

/** @file
 *
 * gPXE error message formatting for EFI
 *
 */

/**
 * Format EFI status code
 *
 * @v efirc		EFI status code
 * @v efi_strerror	EFI status code string
 */
const char * efi_strerror ( EFI_STATUS efirc ) {
	static char errbuf[32];

	if ( ! efirc )
		return "No error";

	snprintf ( errbuf, sizeof ( errbuf ), "Error %lld",
		   ( unsigned long long ) ( efirc ^ MAX_BIT ) );
	return errbuf;
}
