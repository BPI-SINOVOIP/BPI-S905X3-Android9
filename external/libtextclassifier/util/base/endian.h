/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_UTIL_BASE_ENDIAN_H_
#define LIBTEXTCLASSIFIER_UTIL_BASE_ENDIAN_H_

#include "util/base/integral_types.h"

namespace libtextclassifier2 {

#if defined OS_LINUX || defined OS_CYGWIN || defined OS_ANDROID || \
    defined(__ANDROID__)
#include <endian.h>
#elif defined(__APPLE__)
#include <machine/endian.h>
// Add linux style defines.
#ifndef __BYTE_ORDER
#define __BYTE_ORDER BYTE_ORDER
#endif  // __BYTE_ORDER
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif  // __LITTLE_ENDIAN
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN
#endif  // __BIG_ENDIAN
#endif

// The following guarantees declaration of the byte swap functions, and
// defines __BYTE_ORDER for MSVC
#if defined(__GLIBC__) || defined(__BIONIC__) || defined(__CYGWIN__)
#include <byteswap.h>  // IWYU pragma: export
// The following section defines the byte swap functions for OS X / iOS,
// which does not ship with byteswap.h.
#elif defined(__APPLE__)
// Make sure that byte swap functions are not already defined.
#if !defined(bswap_16)
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#endif  // !defined(bswap_16)
#else
#define GG_LONGLONG(x) x##LL
#define GG_ULONGLONG(x) x##ULL
static inline uint16 bswap_16(uint16 x) {
  return (uint16)(((x & 0xFF) << 8) | ((x & 0xFF00) >> 8));  // NOLINT
}
#define bswap_16(x) bswap_16(x)
static inline uint32 bswap_32(uint32 x) {
  return (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) |
          ((x & 0xFF000000) >> 24));
}
#define bswap_32(x) bswap_32(x)
static inline uint64 bswap_64(uint64 x) {
  return (((x & GG_ULONGLONG(0xFF)) << 56) |
          ((x & GG_ULONGLONG(0xFF00)) << 40) |
          ((x & GG_ULONGLONG(0xFF0000)) << 24) |
          ((x & GG_ULONGLONG(0xFF000000)) << 8) |
          ((x & GG_ULONGLONG(0xFF00000000)) >> 8) |
          ((x & GG_ULONGLONG(0xFF0000000000)) >> 24) |
          ((x & GG_ULONGLONG(0xFF000000000000)) >> 40) |
          ((x & GG_ULONGLONG(0xFF00000000000000)) >> 56));
}
#define bswap_64(x) bswap_64(x)
#endif

// define the macros IS_LITTLE_ENDIAN or IS_BIG_ENDIAN
// using the above endian definitions from endian.h if
// endian.h was included
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define IS_BIG_ENDIAN
#endif

#else

#if defined(__LITTLE_ENDIAN__)
#define IS_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN__)
#define IS_BIG_ENDIAN
#endif

// there is also PDP endian ...

#endif  // __BYTE_ORDER

class LittleEndian {
 public:
// Conversion functions.
#ifdef IS_LITTLE_ENDIAN

  static uint16 FromHost16(uint16 x) { return x; }
  static uint16 ToHost16(uint16 x) { return x; }

  static uint32 FromHost32(uint32 x) { return x; }
  static uint32 ToHost32(uint32 x) { return x; }

  static uint64 FromHost64(uint64 x) { return x; }
  static uint64 ToHost64(uint64 x) { return x; }

  static bool IsLittleEndian() { return true; }

#elif defined IS_BIG_ENDIAN

  static uint16 FromHost16(uint16 x) { return gbswap_16(x); }
  static uint16 ToHost16(uint16 x) { return gbswap_16(x); }

  static uint32 FromHost32(uint32 x) { return gbswap_32(x); }
  static uint32 ToHost32(uint32 x) { return gbswap_32(x); }

  static uint64 FromHost64(uint64 x) { return gbswap_64(x); }
  static uint64 ToHost64(uint64 x) { return gbswap_64(x); }

  static bool IsLittleEndian() { return false; }

#endif /* ENDIAN */
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_BASE_ENDIAN_H_
