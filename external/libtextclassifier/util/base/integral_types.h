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

// Basic integer type definitions.

#ifndef LIBTEXTCLASSIFIER_UTIL_BASE_INTEGRAL_TYPES_H_
#define LIBTEXTCLASSIFIER_UTIL_BASE_INTEGRAL_TYPES_H_

#include "util/base/config.h"

namespace libtextclassifier2 {

typedef unsigned int uint32;
typedef unsigned long long uint64;

#ifndef SWIG
typedef int int32;
typedef unsigned char uint8;    // NOLINT
typedef unsigned short uint16;  // NOLINT

// A type to represent a Unicode code-point value. As of Unicode 4.0,
// such values require up to 21 bits.
// (For type-checking on pointers, make this explicitly signed,
// and it should always be the signed version of whatever int32 is.)
typedef signed int char32;
#endif  // SWIG

#ifdef COMPILER_MSVC
typedef __int64 int64;
#else
typedef long long int64;  // NOLINT
#endif  // COMPILER_MSVC

// Some compile-time assertions that our new types have the intended size.
// static_assert exists only since C++11, so we need an ifdef.
#ifdef LANG_CXX11
static_assert(sizeof(int) == 4, "Our typedefs depend on int being 32 bits");
static_assert(sizeof(uint32) == 4, "wrong size");
static_assert(sizeof(int32) == 4, "wrong size");
static_assert(sizeof(uint8) == 1, "wrong size");
static_assert(sizeof(uint16) == 2, "wrong size");
static_assert(sizeof(char32) == 4, "wrong size");
static_assert(sizeof(int64) == 8, "wrong size");
#endif  // LANG_CXX11

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_BASE_INTEGRAL_TYPES_H_
