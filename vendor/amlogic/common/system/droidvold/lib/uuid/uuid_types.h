/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC uuid_type
 */
#if (!defined(_LINUX_TYPES_H) && !defined(_BLKID_TYPES_H) && \
	!defined(_EXT2_TYPES_H))
#define _EXT2_TYPES_H

typedef unsigned char __u8;
typedef __signed__ char __s8;
typedef unsigned short __u16;
typedef __signed__ short __s16;
typedef unsigned int __u32;
typedef __signed__ int __s32;
typedef unsigned long long __u64;
typedef __signed__ long long __s64;
#endif

#include <stdint.h> //uintptr_t

/* endian checking stuff */
#ifndef EXT2_ENDIAN_H_
#define EXT2_ENDIAN_H_

#ifdef __CHECKER__
#define __bitwise		__attribute__((bitwise))
#define __force			__attribute__((force))
#else
#define __bitwise
#define __force
#endif

typedef __u16	__bitwise	__le16;
typedef __u32	__bitwise	__le32;
typedef __u64	__bitwise	__le64;
typedef __u16	__bitwise	__be16;
typedef __u32	__bitwise	__be32;
typedef __u64	__bitwise	__be64;

#endif /* EXT2_ENDIAN_H_ */
