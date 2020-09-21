/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/types.h>

/* Checks if a string is valid UTF-8.
 *
 * Supports 1 to 4 character UTF-8 sequences. Passes tests here:
 *    https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
 *
 * Exceptions: The following unicode non-characters are allowed:
 *    U+FFFE, U+FFFF, U+FDD0 - U+FDEF, U+nFFFE (n = 1 - 10),
 *    U+nFFFD (n = 1 - 10).
 *
 * Args:
 *    string[in] - a string.
 *    bad_pos[out] - position of the first bad character.
 *
 * Returns:
 *    1 if it is a vlid utf-8 string. 0 otherwise.
 *    bad_pos contains the strlen() of the string if it is
 *    valid.
 */
int valid_utf8_string(const char *string, size_t *bad_pos);

/* Checks if a string is a valid utf-8 string.
 *
 * Args:
 *    string[in] - a string.
 *
 * Returns:
 *    1 if it is a valid utf-8 string. 0 otherwise.
 */
int is_utf8_string(const char* string);
