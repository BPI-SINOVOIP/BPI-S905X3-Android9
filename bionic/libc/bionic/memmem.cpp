/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>

void* memmem(const void* void_haystack, size_t n, const void* void_needle, size_t m) {
  const unsigned char* haystack = reinterpret_cast<const unsigned char*>(void_haystack);
  const unsigned char* needle = reinterpret_cast<const unsigned char*>(void_needle);

  if (n < m) return nullptr;

  if (m == 0) return const_cast<void*>(void_haystack);
  if (m == 1) return const_cast<void*>(memchr(haystack, needle[0], n));

  // This uses the "Not So Naive" algorithm, a very simple but usually effective algorithm.
  // http://www-igm.univ-mlv.fr/~lecroq/string/
  const unsigned char* y = haystack;
  const unsigned char* x = needle;
  size_t j = 0;
  size_t k = 1, l = 2;

  if (x[0] == x[1]) {
    k = 2;
    l = 1;
  }
  while (j <= n-m) {
    if (x[1] != y[j+1]) {
      j += k;
    } else {
      if (!memcmp(x+2, y+j+2, m-2) && x[0] == y[j]) return const_cast<unsigned char*>(&y[j]);
      j += l;
    }
  }
  return nullptr;
}
