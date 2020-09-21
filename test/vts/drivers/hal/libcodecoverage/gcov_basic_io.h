#ifndef __VTS_SYSFUZZER_LIBMEASUREMENT_GCOV_BASIC_IO_H__
#define __VTS_SYSFUZZER_LIBMEASUREMENT_GCOV_BASIC_IO_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace vts {

struct block_info;
struct source_info;

#define GCOV_DATA_MAGIC ((unsigned)0x67636461) /* "gcda" */

#define GCOV_TAG_FUNCTION ((unsigned int)0x01000000)
#define GCOV_TAG_FUNCTION_LENGTH (3) /* or 2 */
#define GCOV_COUNTER_ARCS 0
#define GCOV_TAG_ARCS ((unsigned)0x01430000)
#define GCOV_TAG_ARCS_LENGTH(NUM) (1 + (NUM)*2)
#define GCOV_TAG_ARCS_NUM(LENGTH) (((LENGTH)-1) / 2)
#define GCOV_TAG_LINES ((unsigned)0x01450000)
#define GCOV_TAG_BLOCKS ((unsigned)0x01410000)
#define GCOV_TAG_BLOCKS_LENGTH(NUM) (NUM)
#define GCOV_TAG_BLOCKS_NUM(LENGTH) (LENGTH)
#define GCOV_TAG_OBJECT_SUMMARY ((unsigned)0xa1000000)
#define GCOV_TAG_COUNTER_NUM(LENGTH) ((LENGTH) / 2)

#define GCOV_HISTOGRAM_SIZE 252
#define GCOV_HISTOGRAM_BITVECTOR_SIZE (GCOV_HISTOGRAM_SIZE + 31) / 32
#define GCOV_COUNTERS_SUMMABLE (GCOV_COUNTER_ARCS + 1)

#define GCOV_TAG_IS_COUNTER(TAG) \
  (!((TAG)&0xFFFF) && GCOV_COUNTER_FOR_TAG(TAG) < GCOV_COUNTERS)

#define GCOV_TAG_MASK(TAG) (((TAG)-1) ^ (TAG))

/* Return nonzero if SUB is an immediate subtag of TAG.  */
#define GCOV_TAG_IS_SUBTAG(TAG, SUB)                \
  (GCOV_TAG_MASK(TAG) >> 8 == GCOV_TAG_MASK(SUB) && \
   !(((SUB) ^ (TAG)) & ~GCOV_TAG_MASK(TAG)))

/* Return nonzero if SUB is at a sublevel to TAG. */
#define GCOV_TAG_IS_SUBLEVEL(TAG, SUB) (GCOV_TAG_MASK(TAG) > GCOV_TAG_MASK(SUB))

typedef long long gcov_type;

enum { GCOV_COUNTERS };

#define GCOV_VERSION ((unsigned)0x34303670) /* 406p */

#define GCOV_TAG_BUILD_INFO ((unsigned)0xa7000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((unsigned)0xa3000000)

#define XCNEWVEC(T, N) ((T *)calloc((N), sizeof(T)))

#define GCOV_TAG_COUNTER_BASE ((unsigned)0x01a10000)
#define GCOV_TAG_COUNTER_LENGTH(NUM) ((NUM)*2)

/* Convert a counter index to a tag. */
#define GCOV_TAG_FOR_COUNTER(COUNT) \
  (GCOV_TAG_COUNTER_BASE + ((unsigned)(COUNT) << 17))

/* Convert a tag to a counter. */
#define GCOV_COUNTER_FOR_TAG(TAG) \
  ((unsigned)(((TAG)-GCOV_TAG_COUNTER_BASE) >> 17))

/* Check whether a tag is a counter tag.  */
#define GCOV_TAG_IS_COUNTER(TAG) \
  (!((TAG)&0xFFFF) && GCOV_COUNTER_FOR_TAG(TAG) < GCOV_COUNTERS)

#define GCOV_UNSIGNED2STRING(ARRAY, VALUE)                                 \
  ((ARRAY)[0] = (char)((VALUE) >> 24), (ARRAY)[1] = (char)((VALUE) >> 16), \
   (ARRAY)[2] = (char)((VALUE) >> 8), (ARRAY)[3] = (char)((VALUE) >> 0))

#define GCOV_BLOCK_SIZE (1 << 10)

typedef struct arc_info {
  /* source and destination blocks.  */
  struct block_info *src;
  struct block_info *dst;

  /* transition counts.  */
  gcov_type count;
  /* used in cycle search, so that we do not clobber original counts.  */
  gcov_type cs_count;

  unsigned int count_valid : 1;
  unsigned int on_tree : 1;
  unsigned int fake : 1;
  unsigned int fall_through : 1;

  /* Arc to a catch handler. */
  unsigned int is_throw : 1;

  /* Arc is for a function that abnormally returns.  */
  unsigned int is_call_non_return : 1;

  /* Arc is for catch/setjmp. */
  unsigned int is_nonlocal_return : 1;

  /* Is an unconditional branch. */
  unsigned int is_unconditional : 1;

  /* Loop making arc.  */
  unsigned int cycle : 1;

  /* Next branch on line.  */
  struct arc_info *line_next;

  /* Links to next arc on src and dst lists.  */
  struct arc_info *succ_next;
  struct arc_info *pred_next;
} arc_t;

struct gcov_var_t {
  FILE* file;
  unsigned start;  /* Position of first byte of block */
  unsigned offset;  /* Read/write position within the block.  */
  unsigned length;  /* Read limit in the block.  */
  unsigned overread;  /* Number of words overread.  */
  int error;  /* < 0 overflow, > 0 disk error.  */
  int mode;  /* < 0 writing, > 0 reading */
  int endian;  /* Swap endianness.  */
  /* Holds a variable length block, as the compiler can write
     strings and needs to backtrack.  */
  size_t alloc;
  unsigned* buffer;
};

}  // namespace vts
}  // namespace android

#endif
