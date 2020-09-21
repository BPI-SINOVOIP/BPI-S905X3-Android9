#ifndef _GPXE_ROTATE_H
#define _GPXE_ROTATE_H

/** @file
 *
 * Bit operations
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>

static inline uint32_t rol32 ( uint32_t data, unsigned int rotation ) {
        return ( ( data << rotation ) | ( data >> ( 32 - rotation ) ) );
}

static inline uint32_t ror32 ( uint32_t data, unsigned int rotation ) {
        return ( ( data >> rotation ) | ( data << ( 32 - rotation ) ) );
}

static inline uint64_t rol64 ( uint64_t data, unsigned int rotation ) {
        return ( ( data << rotation ) | ( data >> ( 64 - rotation ) ) );
}

static inline uint64_t ror64 ( uint64_t data, unsigned int rotation ) {
        return ( ( data >> rotation ) | ( data << ( 64 - rotation ) ) );
}

#endif /* _GPXE_ROTATE_H */
