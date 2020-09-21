/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef SUB_BITS_H
#define SUB_BITS_H

#include "includes.h"

#ifndef inline
#define inline __inline
#endif

typedef struct bs_s
{
    unsigned char   *p_start;
    unsigned char   *p;
    unsigned char   *p_end;

    int             left;    /* number of available bits */
} bs_t;

static const unsigned int mask[33] =
{
    0x00,
    0x01,      0x03,      0x07,      0x0f,
    0x1f,      0x3f,      0x7f,      0xff,
    0x1ff,     0x3ff,     0x7ff,     0xfff,
    0x1fff,    0x3fff,    0x7fff,    0xffff,
    0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
    0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
    0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

static inline void bs_init( bs_t *s, const void *buffer, unsigned int size )
{
    s->p_start = (unsigned char *)buffer;
    s->p       = s->p_start;
    s->p_end   = s->p_start + size;
    s->left  = 8;
}

static inline int bs_pos( const bs_t *s )
{
    return( 8 * ( s->p - s->p_start ) + 8 - s->left );
}

static inline int bs_eof( const bs_t *s )
{
    return( s->p >= s->p_end ? 1 : 0 );
}

static inline unsigned int bs_read( bs_t *s, int count )
{
    int shr = 0;
    unsigned int result = 0;

    while ( count > 0 )
    {
        if ( s->p >= s->p_end )
        {
            break;
        }

        if ( ( shr = s->left - count ) >= 0 )
        {
            /* more in the buffer than requested */
            result |= ( *s->p >> shr )&mask[count];
            s->left -= count;
            if ( s->left == 0 )
            {
                s->p++;
                s->left = 8;
            }
            return( result );
        }
        else
        {
            /* less in the buffer than requested */
            result |= (*s->p & mask[s->left]) << -shr;
            count  -= s->left;
            s->p++;
            s->left = 8;
        }
    }

    return( result );
}

static inline unsigned int bs_read1( bs_t *s )
{
    if ( s->p < s->p_end )
    {
        unsigned int result;

        s->left--;
        result = ( *s->p >> s->left ) & 0x01;
        if ( s->left == 0 )
        {
            s->p++;
            s->left = 8;
        }
        return result;
    }

    return 0;
}

static inline unsigned int bs_show( bs_t *s, int count )
{
    bs_t     s_tmp = *s;
    return bs_read( &s_tmp, count );
}

static inline void bs_skip( bs_t *s, int count )
{
    s->left -= count;

    if ( s->left <= 0 )
    {
        const int i_bytes = ( -s->left + 8 ) / 8;

        s->p += i_bytes;
        s->left += 8 * i_bytes;
    }
}

static inline void bs_write( bs_t *s, int count, unsigned int bits )
{
    while ( count > 0 )
    {
        if ( s->p >= s->p_end )
        {
            break;
        }

        count--;

        if ( ( bits >> count ) & 0x01 )
        {
            *s->p |= 1 << ( s->left - 1 );
        }
        else
        {
            *s->p &= ~( 1 << ( s->left - 1 ) );
        }
        s->left--;
        if ( s->left == 0 )
        {
            s->p++;
            s->left = 8;
        }
    }
}

static inline void bs_align( bs_t *s )
{
    if ( s->left != 8 )
    {
        s->left = 8;
        s->p++;
    }
}

static inline void bs_align_0( bs_t *s )
{
    if ( s->left != 8 )
    {
        bs_write( s, s->left, 0 );
    }
}

static inline void bs_align_1( bs_t *s )
{
    while ( s->left != 8 )
    {
        bs_write( s, 1, 1 );
    }
}

#endif

