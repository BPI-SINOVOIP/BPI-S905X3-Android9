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

/** @file
 *
 * RDTSC timer
 *
 */

#include <assert.h>
#include <gpxe/timer.h>
#include <gpxe/timer2.h>

/**
 * Number of TSC ticks per microsecond
 *
 * This is calibrated on the first use of the timer.
 */
static unsigned long rdtsc_ticks_per_usec;

/**
 * Delay for a fixed number of microseconds
 *
 * @v usecs		Number of microseconds for which to delay
 */
static void rdtsc_udelay ( unsigned long usecs ) {
	unsigned long start;
	unsigned long elapsed;

	/* Sanity guard, since we may divide by this */
	if ( ! usecs )
		usecs = 1;

	start = currticks();
	if ( rdtsc_ticks_per_usec ) {
		/* Already calibrated; busy-wait until done */
		do {
			elapsed = ( currticks() - start );
		} while ( elapsed < ( usecs * rdtsc_ticks_per_usec ) );
	} else {
		/* Not yet calibrated; use timer2 and calibrate
		 * based on result.
		 */
		timer2_udelay ( usecs );
		elapsed = ( currticks() - start );
		rdtsc_ticks_per_usec = ( elapsed / usecs );
		DBG ( "RDTSC timer calibrated: %ld ticks in %ld usecs "
		      "(%ld MHz)\n", elapsed, usecs,
		      ( rdtsc_ticks_per_usec << TSC_SHIFT ) );
	}
}

/**
 * Get number of ticks per second
 *
 * @ret ticks_per_sec	Number of ticks per second
 */
static unsigned long rdtsc_ticks_per_sec ( void ) {

	/* Calibrate timer, if not already done */
	if ( ! rdtsc_ticks_per_usec )
		udelay ( 1 );

	/* Sanity check */
	assert ( rdtsc_ticks_per_usec != 0 );

	return ( rdtsc_ticks_per_usec * 1000 * 1000 );
}

PROVIDE_TIMER ( rdtsc, udelay, rdtsc_udelay );
PROVIDE_TIMER_INLINE ( rdtsc, currticks );
PROVIDE_TIMER ( rdtsc, ticks_per_sec, rdtsc_ticks_per_sec );
