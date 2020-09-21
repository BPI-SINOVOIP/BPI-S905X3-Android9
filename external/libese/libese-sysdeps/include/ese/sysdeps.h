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
#ifndef ESE_SYSDEPS_H__
#define ESE_SYSDEPS_H__ 1

#include <stdbool.h> /* bool */
#include <stdint.h> /* uint*_t */

#define ESE_UINT32_MAX (UINT32_MAX)

#ifndef NULL
#define NULL ((void *)(0))
#endif

/* Set visibility for exported functions. */
#ifndef ESE_API
#define ESE_API __attribute__ ((visibility("default")))
#endif  /* ESE_API */

/* Mimic C11 _Static_assert behavior for a C99 world. */
#ifndef _static_assert
#define _static_assert(what, why) { while (!(1 / (!!(what)))); }
#endif

extern void *ese_memcpy(void *__dest, const void *__src, uint64_t __n);
extern void *ese_memset(void *__s, int __c, uint64_t __n);

extern uint32_t ese_be32toh(uint32_t big_endian_32bits);
extern uint32_t ese_htobe32(uint32_t host_32bits);
extern uint32_t ese_le32toh(uint32_t little_endian_32bits);
extern uint32_t ese_htole32(uint32_t host_32bits);

#endif  /* ESE_SYSDEPS_H__ */
