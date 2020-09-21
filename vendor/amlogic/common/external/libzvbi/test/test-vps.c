/*
 *  libzvbi - VPS low level functions unit test
 *
 *  Copyright (C) 2006 Michael H. Schimek
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

/* $Id: test-vps.c,v 1.7 2008/03/01 07:35:44 mschimek Exp $ */

#undef NDEBUG

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "src/misc.h"
#include "src/vps.h"

int
main				(void)
{
	const unsigned int cnis[] = { 0x000, 0x001, 0x5A5, 0xA5A, 0xFFF };
	uint8_t buffer1[13];
	uint8_t buffer2[13];
	unsigned int cni2;
	unsigned int i;

	for (i = 0; i < N_ELEMENTS (buffer2); ++i)
		buffer2[i] = rand ();

	memcpy (buffer1, buffer2, sizeof (buffer1));

	vbi_decode_vps_cni (&cni2, buffer2);

	for (i = 0; i < N_ELEMENTS (cnis); ++i) {
		unsigned int cni;

		assert (vbi_encode_vps_cni (buffer1, cnis[i]));

		assert (vbi_decode_vps_cni (&cni, buffer1));
		assert (cni == cnis[i]);

		assert (vbi_encode_vps_cni (buffer1, cni2));
		assert (0 == memcmp (buffer1, buffer2, sizeof (buffer1)));
	}

	assert (!vbi_encode_vps_cni (buffer1, -1));
	assert (!vbi_encode_vps_cni (buffer1, 0x1000));
	assert (!vbi_encode_vps_cni (buffer1, INT_MAX));

	assert (0 == memcmp (buffer1, buffer2, sizeof (buffer1)));

	return 0;
}
