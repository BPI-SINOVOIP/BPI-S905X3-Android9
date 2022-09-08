/*
 * Copyright (c) 2012 Nicolas George
 *
 *
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: h file
 */

#ifndef AVUTIL_BPRINT_H
#define AVUTIL_BPRINT_H

#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

//#include <attributes.h>
//#include <avstring.h>

#ifndef UINT_MAX
#define UINT_MAX 0xFFFFFFFFFFFFFFFLL
#endif
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))

/**
 * @ingroup lavc_decoding
 * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
 * This is mainly needed because some optimized bitstream readers read
 * 32 or 64 bit at once and could read over the end.<br>
 * Note: If the first 23 bits of the additional bytes are not 0, then damaged
 * MPEG bitstreams could cause overread and segfault.
 */
#define AV_INPUT_BUFFER_PADDING_SIZE 32



void av_fast_padded_malloc(void *ptr, unsigned int *size, size_t min_size);

 #define FF_PAD_STRUCTURE(size, ...) \
     __VA_ARGS__ \
     char reserved_padding[size - sizeof(struct { __VA_ARGS__ })];

/**
 * Buffer to print data progressively
 *
 * The string buffer grows as necessary and is always 0-terminated.
 * The content of the string is never accessed, and thus is
 * encoding-agnostic and can even hold binary data.
 *
 * Small buffers are kept in the structure itself, and thus require no
 * memory allocation at all (unless the contents of the buffer is needed
 * after the structure goes out of scope). This is almost as lightweight as
 * declaring a local "char buf[512]".
 *
 * The length of the string can go beyond the allocated size: the buffer is
 * then truncated, but the functions still keep account of the actual total
 * length.
 *
 * In other words, buf->len can be greater than buf->size and records the
 * total length of what would have been to the buffer if there had been
 * enough memory.
 *
 * Append operations do not need to be tested for failure: if a memory
 * allocation fails, data stop being appended to the buffer, but the length
 * is still updated. This situation can be tested with
 * av_bprint_is_complete().
 *
 * The size_max field determines several possible behaviours:
 *
 * size_max = -1 (= UINT_MAX) or any large value will let the buffer be
 * reallocated as necessary, with an amortized linear cost.
 *
 * size_max = 0 prevents writing anything to the buffer: only the total
 * length is computed. The write operations can then possibly be repeated in
 * a buffer with exactly the necessary size
 * (using size_init = size_max = len + 1).
 *
 * size_max = 1 is automatically replaced by the exact size available in the
 * structure itself, thus ensuring no dynamic memory allocation. The
 * internal buffer is large enough to hold a reasonable paragraph of text,
 * such as the current paragraph.
 */
 typedef struct AVBPrint {
    //FF_PAD_STRUCTURE(1024,
    //char *str;         /**< string so far */
    //unsigned len;      /**< length so far */
    //unsigned size;     /**< allocated memory */
    //unsigned size_max; /**< maximum allocated memory */
    //char reserved_internal_buffer[1];
    //)         //for compile success
    char *str;         /**< string so far */
    unsigned len;      /**< length so far */
    unsigned size;     /**< allocated memory */
    unsigned size_max; /**< maximum allocated memory */
    char reserved_internal_buffer[1];
} AVBPrint;


/**
 * Convenience macros for special values for av_bprint_init() size_max
 * parameter.
 */
#define AV_BPRINT_SIZE_UNLIMITED  ((unsigned)-1)
#define AV_BPRINT_SIZE_AUTOMATIC  1
#define AV_BPRINT_SIZE_COUNT_ONLY 0

/**
 * Init a print buffer.
 *
 * @param buf        buffer to init
 * @param size_init  initial size (including the final 0)
 * @param size_max   maximum size;
 *                   0 means do not write anything, just count the length;
 *                   1 is replaced by the maximum value for automatic storage;
 *                   any large value means that the internal buffer will be
 *                   reallocated as needed up to that limit; -1 is converted to
 *                   UINT_MAX, the largest limit possible.
 *                   Check also AV_BPRINT_SIZE_* macros.
 */
void av_bprint_init(AVBPrint *buf, unsigned size_init, unsigned size_max);

#ifdef __GNUC__
#    define av_printf_format(fmtpos, attrpos) __attribute__((__format__(__printf__, fmtpos, attrpos)))
#else
#    define av_printf_format(fmtpos, attrpos)
#endif


/**
 * Append a formatted string to a print buffer.
 */
void av_bprintf(AVBPrint *buf, const char *fmt, ...) av_printf_format(2, 3);

/**
 * Reset the string to "" but keep internal allocated data.
*/
void av_bprint_clear(AVBPrint *buf);

/**
 * Test if the print buffer is complete (not truncated).
 *
 * It may have been truncated due to a memory allocation failure
 * or the size_max limit (compare size and size_max if necessary).
 */
static inline int av_bprint_is_complete(const AVBPrint *buf)
{
    return buf->len < buf->size;
}

/**
 * Finalize a print buffer.
 *
 * The print buffer can no longer be used afterwards,
 * but the len and size fields are still valid.
 *
 * @arg[out] ret_str  if not NULL, used to return a permanent copy of the
 *                    buffer contents, or NULL if memory allocation fails;
 *                    if NULL, the buffer is discarded and freed
 * @return  0 for success or error code (probably AVERROR(ENOMEM))
 */
int av_bprint_finalize(AVBPrint *buf, char **ret_str);

#if 1
void *av_malloc(unsigned int size);

void *av_realloc(void *ptr, unsigned int size);

void av_freep(void **arg);
void av_free(void *arg);
void *av_mallocz(unsigned int size);
#endif

#ifdef __cplusplus
}
#endif

#endif /* AVUTIL_BPRINT_H */