/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef ATAP_SYSDEPS_H_
#define ATAP_SYSDEPS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Change these includes to match your platform to bring in the
 * equivalent types available in a normal C runtime. At least things
 * like uint8_t, uint64_t, and bool (with |false|, |true| keywords)
 * must be present.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* If you don't have gcc or clang, these attribute macros may need to
 * be adjusted.
 */
#define ATAP_ATTR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define ATAP_ATTR_PACKED __attribute__((packed))
#define ATAP_ATTR_NO_RETURN __attribute__((noreturn))
#define ATAP_ATTR_SENTINEL __attribute__((__sentinel__))

/* Copy |n| bytes from |src| to |dest|. */
void* atap_memcpy(void* dest, const void* src, size_t n);

/* Set |n| bytes starting at |s| to |c|.  Returns |dest|. */
void* atap_memset(void* dest, const int c, size_t n);

/* Aborts the program or reboots the device. */
void atap_abort(void) ATAP_ATTR_NO_RETURN;

/* Prints out a message. The string passed must be a NUL-terminated
 * UTF-8 string.
 */
void atap_print(const char* message);

/* Prints out a vector of strings. Each argument must point to a
 * NUL-terminated UTF-8 string and NULL should be the last argument.
 */
void atap_printv(const char* message, ...) ATAP_ATTR_SENTINEL;

/* Allocates |size| bytes. Returns NULL if no memory is available,
 * otherwise a pointer to the allocated memory.
 *
 * The memory is not initialized.
 *
 * The pointer returned is guaranteed to be word-aligned.
 *
 * The memory should be freed with atap_free() when you are done with it.
 */
void* atap_malloc(size_t size) ATAP_ATTR_WARN_UNUSED_RESULT;

/* Frees memory previously allocated with atap_malloc(). */
void atap_free(void* ptr);

/* Returns the length of |str|, excluding the terminating NUL-byte. */
size_t atap_strlen(const char* str) ATAP_ATTR_WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif /* ATAP_SYSDEPS_H_ */
