#ifndef __SUBTITLE_DVB_COMMON_TYPES_H__
#define __SUBTITLE_DVB_COMMON_TYPES_H__


#if defined(ALT_BITSTREAM_READER_LE) && !defined(ALT_BITSTREAM_READER)
#define ALT_BITSTREAM_READER
#error 1
#endif

#if !defined(A32_BITSTREAM_READER) && !defined(ALT_BITSTREAM_READER)
#if defined(ARCH_ARM) && !defined(HAVE_FAST_UNALIGNED)
#define A32_BITSTREAM_READER
#else
#define ALT_BITSTREAM_READER
//#define A32_BITSTREAM_READER
#endif
#endif


#ifndef AV_RB16
#define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
     ((const uint8_t*)(x))[1])
#endif
#ifndef AV_RB32
#define AV_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
     (((const uint8_t*)(x))[1] << 16) |    \
     (((const uint8_t*)(x))[2] <<  8) |    \
     ((const uint8_t*)(x))[3])
#endif


#ifndef NEG_USR32
#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))
#endif

#ifndef sign_extend
static inline int sign_extend(int val, unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) -
            bits);
}
#endif

#ifndef zero_extend
static inline unsigned zero_extend(unsigned val, unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) -
            bits);
}
#endif

#ifndef av_bswap16
static  uint16_t av_bswap16(uint16_t x)
{
    x = (x >> 8) | (x << 8);
    return x;
}
#endif

#ifndef av_bswap32

static uint32_t av_bswap32(uint32_t x)
{
    x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
    x = (x >> 16) | (x << 16);
    return x;
}
#endif

#ifndef av_bswap64
static inline uint64_t av_bswap64(uint64_t x)
{
#if 0
    x = ((x << 8) & 0xFF00FF00FF00FF00ULL) | ((x >> 8) &
            0x00FF00FF00FF00FFULL);
    x = ((x << 16) & 0xFFFF0000FFFF0000ULL) | ((x >> 16) &
            0x0000FFFF0000FFFFULL);
    return (x >> 32) | (x << 32);
#else
    union
    {
        uint64_t ll;
        uint32_t l[2];
    } w, r;
    w.ll = x;
    r.l[0] = av_bswap32(w.l[1]);
    r.l[1] = av_bswap32(w.l[0]);
    return r.ll;
#endif
}
#endif

// be2ne ... big-endian to native-endian
// le2ne ... little-endian to native-endian

#if AV_HAVE_BIGENDIAN
#define av_be2ne16(x) (x)
#define av_be2ne32(x) (x)
#define av_be2ne64(x) (x)
#define av_le2ne16(x) av_bswap16(x)
#define av_le2ne32(x) av_bswap32(x)
#define av_le2ne64(x) av_bswap64(x)
#define AV_BE2NEC(s, x) (x)
#define AV_LE2NEC(s, x) AV_BSWAPC(s, x)
#else
#define av_be2ne16(x) av_bswap16(x)
#define av_be2ne32(x) av_bswap32(x)
#define av_be2ne64(x) av_bswap64(x)
#define av_le2ne16(x) (x)
#define av_le2ne32(x) (x)
#define av_le2ne64(x) (x)
#define AV_BE2NEC(s, x) AV_BSWAPC(s, x)
#define AV_LE2NEC(s, x) (x)
#endif

/* bit input */
/* buffer, buffer_end and size_in_bits must be present and used by every reader */
typedef struct GetBitContext
{
    const uint8_t *buffer, *buffer_end;
#ifdef ALT_BITSTREAM_READER
    int index;
#elif defined A32_BITSTREAM_READER
    uint32_t *buffer_ptr;
    uint32_t cache0;
    uint32_t cache1;
    int bit_count;
#endif
    int size_in_bits;
} GetBitContext;

#ifdef ALT_BITSTREAM_READER
#define MIN_CACHE_BITS 25

#define OPEN_READER(name, gb)                \
    unsigned int name##_index = (gb)->index;    \
    unsigned int name##_cache

#define CLOSE_READER(name, gb) (gb)->index = name##_index

#ifdef ALT_BITSTREAM_READER_LE
#define UPDATE_CACHE(name, gb) \
    name##_cache = AV_RL32(((const uint8_t *)(gb)->buffer)+(name##_index>>3)) >> (name##_index&0x07)

#define SKIP_CACHE(name, gb, num) name##_cache >>= (num)
#else
#define UPDATE_CACHE(name, gb) \
    name##_cache = AV_RB32(((const uint8_t *)(gb)->buffer)+(name##_index>>3)) << (name##_index&0x07)

#define SKIP_CACHE(name, gb, num) name##_cache <<= (num)
#endif

// FIXME name?
#define SKIP_COUNTER(name, gb, num) name##_index += (num)

#define SKIP_BITS(name, gb, num) do {        \
        SKIP_CACHE(name, gb, num);              \
        SKIP_COUNTER(name, gb, num);            \
    } while (0)

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num)

#ifdef ALT_BITSTREAM_READER_LE
#define SHOW_UBITS(name, gb, num) zero_extend(name##_cache, num)

#define SHOW_SBITS(name, gb, num) sign_extend(name##_cache, num)
#else
#define SHOW_UBITS(name, gb, num) NEG_USR32(name##_cache, num)

#define SHOW_SBITS(name, gb, num) NEG_SSR32(name##_cache, num)
#endif

#define GET_CACHE(name, gb) ((uint32_t)name##_cache)

/**
 * Read 1-25 bits.
 */

static inline int get_bits_count(const GetBitContext *s) {
    return s->index;
}

static inline void skip_bits_long(GetBitContext *s, int n) {
    s->index += n;
}

#elif defined A32_BITSTREAM_READER

#define MIN_CACHE_BITS 32

#define OPEN_READER(name, gb)                        \
    int name##_bit_count        = (gb)->bit_count;      \
    uint32_t name##_cache0      = (gb)->cache0;         \
    uint32_t name##_cache1      = (gb)->cache1;         \
    uint32_t *name##_buffer_ptr = (gb)->buffer_ptr

#define CLOSE_READER(name, gb) do {          \
        (gb)->bit_count  = name##_bit_count;    \
        (gb)->cache0     = name##_cache0;       \
        (gb)->cache1     = name##_cache1;       \
        (gb)->buffer_ptr = name##_buffer_ptr;   \
    } while (0)

#define UPDATE_CACHE(name, gb) do {                                  \
        if(name##_bit_count > 0){                                       \
            const uint32_t next = av_be2ne32(*name##_buffer_ptr);       \
            name##_cache0 |= NEG_USR32(next, name##_bit_count);         \
            name##_cache1 |= next << name##_bit_count;                  \
            name##_buffer_ptr++;                                        \
            name##_bit_count -= 32;                                     \
        }                                                               \
    } while (0)

#if ARCH_X86
#define SKIP_CACHE(name, gb, num)                            \
    __asm__("shldl %2, %1, %0          \n\t"                    \
            "shll  %2, %1              \n\t"                    \
            : "+r" (name##_cache0), "+r" (name##_cache1)        \
            : "Ic" ((uint8_t)(num)))
#else
#define SKIP_CACHE(name, gb, num) do {               \
        name##_cache0 <<= (num);                        \
        name##_cache0 |= NEG_USR32(name##_cache1,num);  \
        name##_cache1 <<= (num);                        \
    } while (0)
#endif

#define SKIP_COUNTER(name, gb, num) name##_bit_count += (num)

#define SKIP_BITS(name, gb, num) do {        \
        SKIP_CACHE(name, gb, num);              \
        SKIP_COUNTER(name, gb, num);            \
    } while (0)

#define LAST_SKIP_BITS(name, gb, num)  SKIP_BITS(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num) SKIP_CACHE(name, gb, num)

#define SHOW_UBITS(name, gb, num) NEG_USR32(name##_cache0, num)

#define SHOW_SBITS(name, gb, num) NEG_SSR32(name##_cache0, num)

#define GET_CACHE(name, gb) name##_cache0

static inline int get_bits_count(const GetBitContext *s) {
    return ((uint8_t *) s->buffer_ptr - s->buffer) * 8 - 32 + s->bit_count;
}

static inline void skip_bits_long(GetBitContext *s, int n) {
    OPEN_READER(re, s);
    re_bit_count += n;
    re_buffer_ptr += re_bit_count >> 5;
    re_bit_count &= 31;
    re_cache0 = av_be2ne32(re_buffer_ptr[-1]) << re_bit_count;
    re_cache1 = 0;
    UPDATE_CACHE(re, s);
    CLOSE_READER(re, s);
}

#endif

static inline unsigned int get_bits(GetBitContext *s, int n) {
    int tmp;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return tmp;
}

/**
 * Shows 1-25 bits.
 */
static inline unsigned int show_bits(GetBitContext *s, int n) {
    int tmp;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    return tmp;
}

static inline void skip_bits(GetBitContext *s, int n) {
    //Note gcc seems to optimize this to s->index+=n for the ALT_READER :))
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
}

static inline unsigned int get_bits1(GetBitContext *s) {
#ifdef ALT_BITSTREAM_READER
    unsigned int index = s->index;
    uint8_t result = s->buffer[index >> 3];
#ifdef ALT_BITSTREAM_READER_LE
    result >>= index & 7;
    result &= 1;
#else
    // avoid asan report integer overflow error
    uint32_t tmp = result << (index & 7);
    result = (uint8_t)tmp;
    result >>= 8 - 1;
#endif
    index++;
    s->index = index;
    return result;
#else
    return get_bits(s, 1);
#endif
}

/**
 * init GetBitContext.
 * @param buffer bitstream buffer, must be FF_INPUT_BUFFER_PADDING_SIZE bytes larger then the actual read bits
 * because some optimized bitstream readers read 32 or 64 bit at once and could read over the end
 * @param bit_size the size of the buffer in bits
 *
 * While GetBitContext stores the buffer size, for performance reasons you are
 * responsible for checking for the buffer end yourself (take advantage of the padding)!
 */
static inline void init_get_bits(GetBitContext *s,
                                 const uint8_t *buffer, int bit_size) {
    int buffer_size = (bit_size + 7) >> 3;
    if (buffer_size < 0 || bit_size < 0) {
        buffer_size = bit_size = 0;
        buffer = NULL;
    }
    s->buffer = buffer;
    s->size_in_bits = bit_size;
    s->buffer_end = buffer + buffer_size;
#ifdef ALT_BITSTREAM_READER
    s->index = 0;
#elif defined A32_BITSTREAM_READER
    s->buffer_ptr = (uint32_t *)((intptr_t) buffer & ~3);
    s->bit_count = 32 + 8 * ((intptr_t) buffer & 3);
    skip_bits_long(s, 0);
#endif
}

static inline uint32_t bytestream_get_be32(const uint8_t **ptr) {
    uint32_t tmp;
    tmp = (*ptr)[3] | ((*ptr)[2]<<8) | ((*ptr)[1]<<16) | ((*ptr)[0]<<24);
    *ptr += 4;
    return tmp;
}

static inline uint32_t bytestream_get_be24(const uint8_t **ptr) {
    uint32_t tmp;
    tmp = (*ptr)[2] | ((*ptr)[1]<<8) | ((*ptr)[0]<<16);
    *ptr += 3;
    return tmp;
}

static inline uint32_t bytestream_get_be16(const uint8_t **ptr) {
    uint32_t tmp;
    tmp = (*ptr)[1] | ((*ptr)[0]<<8);
    *ptr += 2;
    return tmp;
}

static inline uint8_t bytestream_get_byte(const uint8_t **ptr) {
    uint8_t tmp;
    tmp = **ptr;
    *ptr += 1;
    return tmp;
}

enum AVSubtitleType
{
    SUBTITLE_NONE,

    SUBTITLE_BITMAP,    ///< A bitmap, pict will be set

    /**
     * Plain text, the text field must be set by the decoder and is
     * authoritative. ass and pict fields may contain approximations.
     */
    SUBTITLE_TEXT,

    /**
     * Formatted text, the ass field must be set by the decoder and is
     * authoritative. pict and text fields may contain approximations.
     */
    SUBTITLE_ASS,
};

/**
 * four components are given, that's all.
 * the last component is alpha
 */
struct AVPicture {
    uint8_t *data[4];
    int  lineSize[4];    ///< number of bytes per line
};

#define AVPALETTE_SIZE 1024

struct AVSubtitleRect {
    int x;          ///< top left corner  of pict, undefined when pict is not set
    int y;          ///< top left corner  of pict, undefined when pict is not set
    int w;          ///< width            of pict, undefined when pict is not set
    int h;          ///< height           of pict, undefined when pict is not set
    int nbColors;      ///< number of colors in pict, undefined when pict is not set

    /**
     * data+linesize for the bitmap of this subtitle.
     * can be set for text/ass as well once they where rendered
     */
    AVPicture pict;
    enum AVSubtitleType type;

    char *text;     ///< 0 terminated plain UTF-8 text

    /**
     * 0 terminated ASS/SSA compatible event line.
     * The pressentation of this is unaffected by the other values in this
     * struct.
     */
    char *ass;
};


struct AVSubtitle {
    uint16_t format;    /* 0 = graphics */
    uint32_t startDisplayTime;    /* relative to packet pts, in ms */
    uint32_t endDisplayTime;  /* relative to packet pts, in ms */
    unsigned numRects;
    AVSubtitleRect **rects;
    int64_t pts;        ///< Same as packet pts, in AV_TIME_BASE
};


#endif


