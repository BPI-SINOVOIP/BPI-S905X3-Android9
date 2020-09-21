/**
 * crc32 calculation routines.
 *
 * Copyright (c) 2005 by Andrew de Quincey <adq_dvb@lidskialf.net>
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

#ifndef _UCSI_CRC32_H
#define _UCSI_CRC32_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CRC32_INIT (~0)

extern uint32_t crc32tbl[];

/**
 * Calculate a CRC32 over a piece of data.
 *
 * @param crc Current CRC value (use CRC32_INIT for first call).
 * @param buf Buffer to calculate over.
 * @param len Number of bytes.
 * @return Calculated CRC.
 */
static inline uint32_t crc32(uint32_t crc, uint8_t* buf, size_t len)
{
	size_t i;

	for (i=0; i< len; i++) {
		crc = (crc << 8) ^ crc32tbl[((crc >> 24) ^ buf[i]) & 0xff];
	}

	return crc;
}

#ifdef __cplusplus
}
#endif

#endif
