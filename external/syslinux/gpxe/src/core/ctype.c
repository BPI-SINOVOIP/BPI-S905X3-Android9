/*
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>.
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

/**
 * @file
 *
 * Character types
 *
 */

#include <ctype.h>

/**
 * Check to see if character is a space
 *
 * @v c			Character
 * @ret isspace		Character is a space
 */
int isspace ( int c ) {
	switch ( c ) {
	case ' ' :
	case '\f' :
	case '\n' :
	case '\r' :
	case '\t' :
	case '\v' :
		return 1;
	default:
		return 0;
	}
}
