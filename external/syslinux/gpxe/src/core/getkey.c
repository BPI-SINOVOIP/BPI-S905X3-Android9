/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <ctype.h>
#include <console.h>
#include <gpxe/process.h>
#include <gpxe/keys.h>
#include <gpxe/timer.h>

/** @file
 *
 * Special key interpretation
 *
 */

#define GETKEY_TIMEOUT ( TICKS_PER_SEC / 4 )

/**
 * Read character from console if available within timeout period
 *
 * @v timeout		Timeout period, in ticks
 * @ret character	Character read from console
 */
static int getchar_timeout ( unsigned long timeout ) {
	unsigned long expiry = ( currticks() + timeout );

	while ( currticks() < expiry ) {
		step();
		if ( iskey() )
			return getchar();
	}

	return -1;
}

/**
 * Get single keypress
 *
 * @ret key		Key pressed
 *
 * The returned key will be an ASCII value or a KEY_XXX special
 * constant.  This function differs from getchar() in that getchar()
 * will return "special" keys (e.g. cursor keys) as a series of
 * characters forming an ANSI escape sequence.
 */
int getkey ( void ) {
	int character;
	unsigned int n = 0;

	character = getchar();
	if ( character != ESC )
		return character;

	while ( ( character = getchar_timeout ( GETKEY_TIMEOUT ) ) >= 0 ) {
		if ( character == '[' )
			continue;
		if ( isdigit ( character ) ) {
			n = ( ( n * 10 ) + ( character - '0' ) );
			continue;
		}
		if ( character >= 0x40 )
			return KEY_ANSI ( n, character );
	}

	return ESC;
}
