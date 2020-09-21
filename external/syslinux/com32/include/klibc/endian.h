/*
 * klibc/endian.h
 *
 * Like <endian.h>, but export only double-underscore symbols
 */

#ifndef _KLIBC_ENDIAN_H
#define _KLIBC_ENDIAN_H

#define __LITTLE_ENDIAN		/* we're on i386, littleendian */

/* Linux' asm/byteorder.h defines either __LITTLE_ENDIAN or
   __BIG_ENDIAN, but the glibc/BSD-ish macros expect both to be
   defined with __BYTE_ORDER defining which is actually used... */

#if defined(__LITTLE_ENDIAN)
# undef  __LITTLE_ENDIAN
# define __LITTLE_ENDIAN 1234
# define __BIG_ENDIAN    4321
# define __PDP_ENDIAN    3412
# define __BYTE_ORDER    __LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN)
# undef  __BIG_ENDIAN
# define __LITTLE_ENDIAN 1234
# define __BIG_ENDIAN    4321
# define __PDP_ENDIAN    3412
# define __BYTE_ORDER    __BIG_ENDIAN
#elif defined(__PDP_ENDIAN)
# undef  __PDP_ENDIAN
# define __LITTLE_ENDIAN 1234
# define __BIG_ENDIAN    4321
# define __PDP_ENDIAN    3412
# define __BYTE_ORDER    __PDP_ENDIAN
#else
# error "Unknown byte order!"
#endif

#endif /* _KLIBC_ENDIAN_H */
