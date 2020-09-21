/*
 * Copyright (c) 2017 Imagination Technologies.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with
 *        the distribution.
 *      * Neither the name of Imagination Technologies nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#if !defined(UNALIGNED_INSTR_SUPPORT)
/* does target have unaligned lw/ld/ualw/uald instructions? */
#define UNALIGNED_INSTR_SUPPORT 0
#if __mips_isa_rev < 6 && !__mips1
#undef UNALIGNED_INSTR_SUPPORT
#define UNALIGNED_INSTR_SUPPORT 1
#endif
#endif

#if !defined(HW_UNALIGNED_SUPPORT)
/* Does target have hardware support for unaligned accesses?  */
#define HW_UNALIGNED_SUPPORT 0
#if __mips_isa_rev >= 6
#undef HW_UNALIGNED_SUPPORT
#define HW_UNALIGNED_SUPPORT 1
#endif
#endif

#define ENABLE_PREFETCH     1

#if ENABLE_PREFETCH
#define PREFETCH(addr)  __builtin_prefetch (addr, 0, 1);
#else
#define PREFETCH(addr)
#endif

#if _MIPS_SIM == _ABIO32
typedef unsigned long reg_t;
typedef struct
{
  reg_t B0:8, B1:8, B2:8, B3:8;
} bits_t;
#else
typedef unsigned long long reg_t;
typedef struct
{
  reg_t B0:8, B1:8, B2:8, B3:8, B4:8, B5:8, B6:8, B7:8;
} bits_t;
#endif

typedef union
{
  reg_t v;
  bits_t b;
} bitfields_t;

#define DO_BYTE(a, i)   \
  a[i] = bw.b.B##i;     \
  len--;                \
  if(!len) return ret;  \

/* This code is called when aligning a pointer, there are remaining bytes
   after doing word compares, or architecture does not have some form
   of unaligned support.  */
static inline void * __attribute__ ((always_inline))
do_bytes (void *a, const void *b, unsigned long len, void *ret)
{
  unsigned char *x = (unsigned char *) a;
  unsigned char *y = (unsigned char *) b;
  unsigned long i;

  /* 'len' might be zero here, so preloading the first two values
     before the loop may access unallocated memory.  */
  for (i = 0; i < len; i++)
  {
    *x = *y;
    x++;
    y++;
  }
  return ret;
}

static inline void * __attribute__ ((always_inline))
do_bytes_backward (void *a, const void *b, unsigned long len, void *ret)
{
  unsigned char *x = (unsigned char *) a;
  unsigned char *y = (unsigned char *) b;
  unsigned long i;

  /* 'len' might be zero here, so preloading the first two values
     before the loop may access unallocated memory.  */
  for (i = 0; i < len; i++) {
    *--x = *--y;
  }
  return ret;
}

static inline void * __attribute__ ((always_inline))
do_bytes_aligned (void *a, const void *b, unsigned long len, void *ret)
{
  unsigned char *x = (unsigned char *) a;

  if(len > 0) {
    bitfields_t bw;
    bw.v = *((reg_t*) b);

#if __mips64
    DO_BYTE(x, 0);
    DO_BYTE(x, 1);
    DO_BYTE(x, 2);
    DO_BYTE(x, 3);
    DO_BYTE(x, 4);
    DO_BYTE(x, 5);
    DO_BYTE(x, 6);
    DO_BYTE(x, 7);
#else
    DO_BYTE(x, 0);
    DO_BYTE(x, 1);
    DO_BYTE(x, 2);
    DO_BYTE(x, 3);
#endif
  }

  return ret;
}

#if !HW_UNALIGNED_SUPPORT
#if UNALIGNED_INSTR_SUPPORT
/* for MIPS GCC, there are no unaligned builtins - so this struct forces
   the compiler to treat the pointer access as unaligned.  */
struct ulw
{
  reg_t uli;
} __attribute__ ((packed));

#define STORE_UNALIGNED_8(a, b)                      \
{                                                    \
  reg_t y0 = b[0], y1 = b[1], y2 = b[2], y3 = b[3];  \
  reg_t y4 = b[4], y5 = b[5], y6 = b[6], y7 = b[7];  \
  a[0].uli = y0;                                     \
  a[1].uli = y1;                                     \
  a[2].uli = y2;                                     \
  a[3].uli = y3;                                     \
  a[4].uli = y4;                                     \
  a[5].uli = y5;                                     \
  a[6].uli = y6;                                     \
  a[7].uli = y7;                                     \
}

#define STORE_UNALIGNED_4(a, b)                      \
{                                                    \
  reg_t y0 = b[0], y1 = b[1], y2 = b[2], y3 = b[3];  \
  a[0].uli = y0;                                     \
  a[1].uli = y1;                                     \
  a[2].uli = y2;                                     \
  a[3].uli = y3;                                     \
}

/* first pointer is not aligned while second pointer is.  */
static void *
unaligned_words_forward (struct ulw *a, const reg_t * b,
                         unsigned long words, unsigned long bytes, void *ret)
{
#if ((_MIPS_SIM == _ABIO32) || _MIPS_TUNE_I6400)
  unsigned long i, words_by_8, words_by_1;
  words_by_1 = words % 8;
  words_by_8 = words >> 3;
  for (; words_by_8 > 0; words_by_8--) {
    if(words_by_8 != 1)
      PREFETCH (b + 8);
    STORE_UNALIGNED_8(a, b);
    a += 8;
    b += 8;
  }
#else
  unsigned long i, words_by_4, words_by_1;
  words_by_1 = words % 4;
  words_by_4 = words >> 2;
  for (; words_by_4 > 0; words_by_4--) {
    if(words_by_4 != 1)
      PREFETCH (b + 4);
    STORE_UNALIGNED_4(a, b);
    a += 4;
    b += 4;
  }
#endif

  /* do remaining words.  */
  for (i = 0; i < words_by_1; i++) {
    a->uli = *b;
    a += 1;
    b += 1;
  }

  /* mop up any remaining bytes.  */
  return do_bytes_aligned (a, b, bytes, ret);
}

static void *
unaligned_words_backward (struct ulw *a, const reg_t * b,
                          unsigned long words, unsigned long bytes, void *ret)
{
#if ((_MIPS_SIM == _ABIO32) || _MIPS_TUNE_I6400)
  unsigned long i, words_by_8, words_by_1;
  words_by_1 = words % 8;
  words_by_8 = words >> 3;
  for (; words_by_8 > 0; words_by_8--) {
    if(words_by_8 != 1)
      PREFETCH (b - 16);
    a -= 8;
    b -= 8;
    STORE_UNALIGNED_8(a, b);
  }
#else
  unsigned long i, words_by_4, words_by_1;
  words_by_1 = words % 4;
  words_by_4 = words >> 2;
  for (; words_by_4 > 0; words_by_4--) {
    if(words_by_4 != 1)
      PREFETCH (b - 8);
    a -= 4;
    b -= 4;
    STORE_UNALIGNED_4(a, b);
  }
#endif

  /* do remaining words.  */
  for (i = 0; i < words_by_1; i++) {
    a -= 1;
    b -= 1;
    a->uli = *b;
  }

  /* mop up any remaining bytes.  */
  return do_bytes_backward (a, b, bytes, ret);
}

#else
/* no HW support or unaligned lw/ld/ualw/uald instructions.  */
static void *
unaligned_words_forward (reg_t * a, const reg_t * b,
                         unsigned long words, unsigned long bytes, void *ret)
{
  return do_bytes_aligned (a, b, (sizeof (reg_t) * words) + bytes, ret);
}

static void *
unaligned_words_backward (reg_t * a, const reg_t * b,
                          unsigned long words, unsigned long bytes, void *ret)
{
  return do_bytes_backward (a, b, (sizeof (reg_t) * words) + bytes, ret);
}

#endif /* UNALIGNED_INSTR_SUPPORT */
#endif /* HW_UNALIGNED_SUPPORT */

/* both pointers are aligned, or first isn't and HW support for unaligned.  */

#define STORE_ALIGNED_8(a, b)                        \
{                                                    \
  reg_t x0 = b[0], x1 = b[1], x2 = b[2], x3 = b[3];  \
  reg_t x4 = b[4], x5 = b[5], x6 = b[6], x7 = b[7];  \
  a[0] = x0;                                         \
  a[1] = x1;                                         \
  a[2] = x2;                                         \
  a[3] = x3;                                         \
  a[4] = x4;                                         \
  a[5] = x5;                                         \
  a[6] = x6;                                         \
  a[7] = x7;                                         \
}

#define STORE_ALIGNED_4(a, b)                        \
{                                                    \
  reg_t x0 = b[0], x1 = b[1], x2 = b[2], x3 = b[3];  \
  a[0] = x0;                                         \
  a[1] = x1;                                         \
  a[2] = x2;                                         \
  a[3] = x3;                                         \
}

static void *
aligned_words_forward (reg_t * a, const reg_t * b,
                       unsigned long words, unsigned long bytes, void *ret)
{
#if ((_MIPS_SIM == _ABIO32) || _MIPS_TUNE_I6400)
  unsigned long i, words_by_8, words_by_1;
  words_by_1 = words % 8;
  words_by_8 = words >> 3;
  for (; words_by_8 > 0; words_by_8--) {
    if(words_by_8 != 1)
      PREFETCH (b + 8);
    STORE_ALIGNED_8(a, b);
    a += 8;
    b += 8;
  }
#else
  unsigned long i, words_by_4, words_by_1;
  words_by_1 = words % 4;
  words_by_4 = words >> 2;
  for (; words_by_4 > 0; words_by_4--) {
    if(words_by_4 != 1)
      PREFETCH (b + 4);
    STORE_ALIGNED_4(a, b);
    a += 4;
    b += 4;
  }
#endif

  /* do remaining words.  */
  for (i = 0; i < words_by_1; i++) {
    *a = *b;
    a += 1;
    b += 1;
  }

  /* mop up any remaining bytes.  */
  return do_bytes_aligned (a, b, bytes, ret);
}


static void *
aligned_words_backward (reg_t * a, const reg_t * b,
                        unsigned long words, unsigned long bytes, void *ret)
{
#if ((_MIPS_SIM == _ABIO32) || _MIPS_TUNE_I6400)
  unsigned long i, words_by_8, words_by_1;
  words_by_1 = words % 8;
  words_by_8 = words >> 3;
  for (; words_by_8 > 0; words_by_8--) {
    if(words_by_8 != 1)
      PREFETCH (b - 16);
    a -= 8;
    b -= 8;
    STORE_ALIGNED_8(a, b);
  }
#else
  unsigned long i, words_by_4, words_by_1;
  words_by_1 = words % 4;
  words_by_4 = words >> 2;
  for (; words_by_4 > 0; words_by_4--) {
    if(words_by_4 != 1)
      PREFETCH (b - 8);
    a -= 4;
    b -= 4;
    STORE_ALIGNED_4(a, b);
  }
#endif

  /* do remaining words.  */
  for (i = 0; i < words_by_1; i++) {
    a -= 1;
    b -= 1;
    *a = *b;
  }

  /* mop up any remaining bytes.  */
  return do_bytes_backward (a, b, bytes, ret);
}

void *
memmove (void *dst0, const void *src0, size_t length)
{
  unsigned long bytes, words;
  void *ret = dst0;

  if (length == 0 || dst0 == src0)      /* nothing to do */
    return dst0;

  if ((unsigned long)dst0 < (unsigned long)src0) {
    /* Copy forwards. */
    /* This shouldn't hit that often. */
    if (length < sizeof (reg_t) * 4) {
      return do_bytes (dst0, src0, length, ret);
    }

    /* Align the second pointer to word/dword alignment.
       Note that the pointer is only 32-bits for o32/n32 ABIs. For
       n32, loads are done as 64-bit while address remains 32-bit.   */
    bytes = ((unsigned long) src0) % sizeof (reg_t);
    if (bytes) {
      bytes = sizeof (reg_t) - bytes;
      if (bytes > length)
        bytes = length;
      do_bytes (dst0, src0, bytes, ret);
      if (length == bytes)
        return ret;
      length -= bytes;
      dst0 = (void *) (((unsigned char *) dst0) + bytes);
      src0 = (const void *) (((unsigned char *) src0) + bytes);
    }

    /* Second pointer now aligned.  */
    words = length / sizeof (reg_t);
    bytes = length % sizeof (reg_t);
#if HW_UNALIGNED_SUPPORT
    /* treat possible unaligned first pointer as aligned.  */
    return aligned_words_forward (dst0, src0, words, bytes, ret);
#else
    if (((unsigned long) dst0) % sizeof (reg_t) == 0) {
      return aligned_words_forward (dst0, src0, words, bytes, ret);
    }
    /* need to use unaligned instructions on first pointer.  */
    return unaligned_words_forward (dst0, src0, words, bytes, ret);
#endif
  } else {
    /* Copy backwards. */
    dst0 = (void *) (((unsigned char *) dst0) + length);
    src0 = (const void *) (((unsigned char *) src0) + length);

    /* This shouldn't hit that often. */
    if (length < sizeof (reg_t) * 4) {
      return do_bytes_backward (dst0, src0, length, ret);
    }

    /* Align the second pointer to word/dword alignment.
       Note that the pointer is only 32-bits for o32/n32 ABIs. For
       n32, loads are done as 64-bit while address remains 32-bit.   */
    bytes = ((unsigned long) src0) % sizeof (reg_t);
    if (bytes) {
      if (bytes > length)
        bytes = length;
      do_bytes_backward (dst0, src0, bytes, ret);
      if (length == bytes)
        return ret;
      length -= bytes;
      dst0 = (void *) (((unsigned char *) dst0) - bytes);
      src0 = (const void *) (((unsigned char *) src0) - bytes);
    }

    words = length / sizeof (reg_t);
    bytes = length % sizeof (reg_t);
#if HW_UNALIGNED_SUPPORT
    /* treat possible unaligned first pointer as aligned.  */
    return aligned_words_backward ((void *)dst0, (void *)src0, words, bytes, ret);
#else
    if (((unsigned long) dst0) % sizeof (reg_t) == 0) {
      return aligned_words_backward (dst0, src0, words, bytes, ret);
    }
    /* need to use unaligned instructions on first pointer.  */
    return unaligned_words_backward (dst0, src0, words, bytes, ret);
#endif
  }
}
