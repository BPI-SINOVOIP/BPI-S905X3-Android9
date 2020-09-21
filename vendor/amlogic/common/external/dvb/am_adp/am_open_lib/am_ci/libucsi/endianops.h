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

#ifndef _UCSI_COMMON_H
#define _UCSI_COMMON_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <byteswap.h>
#include <endian.h>

#define __ucsi_packed __attribute__((packed))




#if __BYTE_ORDER == __BIG_ENDIAN
#define EBIT2(x1,x2) x1 x2
#define EBIT3(x1,x2,x3) x1 x2 x3
#define EBIT4(x1,x2,x3,x4) x1 x2 x3 x4
#define EBIT5(x1,x2,x3,x4,x5) x1 x2 x3 x4 x5
#define EBIT6(x1,x2,x3,x4,x5,x6) x1 x2 x3 x4 x5 x6
#define EBIT7(x1,x2,x3,x4,x5,x6,x7) x1 x2 x3 x4 x5 x6 x7
#define EBIT8(x1,x2,x3,x4,x5,x6,x7,x8) x1 x2 x3 x4 x5 x6 x7 x8

static inline void bswap16(uint8_t *buf) {
	(void) buf;
}

static inline void bswap32(uint8_t *buf) {
	(void) buf;
}

static inline void bswap64(uint8_t *buf) {
	(void) buf;
}

static inline void bswap24(uint8_t *buf) {
	(void) buf;
}

static inline void bswap40(uint8_t *buf) {
	(void) buf;
}

static inline void bswap48(uint8_t *buf) {
	(void) buf;
}

#else
#define EBIT2(x1,x2) x2 x1
#define EBIT3(x1,x2,x3) x3 x2 x1
#define EBIT4(x1,x2,x3,x4) x4 x3 x2 x1
#define EBIT5(x1,x2,x3,x4,x5) x5 x4 x3 x2 x1
#define EBIT6(x1,x2,x3,x4,x5,x6) x6 x5 x4 x3 x2 x1
#define EBIT7(x1,x2,x3,x4,x5,x6,x7) x7 x6 x5 x4 x3 x2 x1
#define EBIT8(x1,x2,x3,x4,x5,x6,x7,x8) x8 x7 x6 x5 x4 x3 x2 x1

static inline void bswap16(uint8_t * buf) {
	*((uint16_t*)buf) = bswap_16((*(uint16_t*)buf));
}

static inline void bswap32(uint8_t * buf) {
	*((uint32_t*)buf) = bswap_32((*(uint32_t*)buf));
}

static inline void bswap64(uint8_t * buf) {
	*((uint64_t*)buf) = bswap_64((*(uint64_t*)buf));
}

static inline void bswap24(uint8_t * buf) {
	uint8_t tmp0 = buf[0];

	buf[0] = buf[2];
	buf[2] = tmp0;
}

static inline void bswap40(uint8_t * buf) {
	uint8_t tmp0 = buf[0];
	uint8_t tmp1 = buf[1];

	buf[0] = buf[4];
	buf[1] = buf[3];
	buf[3] = tmp1;
	buf[4] = tmp0;
}

static inline void bswap48(uint8_t * buf) {
	uint8_t tmp0 = buf[0];
	uint8_t tmp1 = buf[1];
	uint8_t tmp2 = buf[2];

	buf[0] = buf[5];
	buf[1] = buf[4];
	buf[2] = buf[3];
	buf[3] = tmp2;
	buf[4] = tmp1;
	buf[5] = tmp0;
}

#endif // __BYTE_ORDER

#ifdef __cplusplus
}
#endif

#endif
