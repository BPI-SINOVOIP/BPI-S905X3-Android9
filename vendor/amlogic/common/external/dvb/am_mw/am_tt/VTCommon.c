#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*******************************************************************
 *
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *
 *  Description: TELETEXT
 *
 *  Author: Amlogic STB Software
 *  Created: 12:00 2008-3-21
 *
 *  Modification History:
 *
 *  Version		Date		   	Author		Log
 *  -------		----	   		------		----------
 *	1.00	  	2008-3-21	    sence		created
 *******************************************************************/

#include "includes.h"



#include "VTCommon.h"

#ifndef AM_SUCCESS
#define AM_SUCCESS			(0)
#endif

#ifndef AM_FAILURE
#define AM_FAILURE			(-1)
#endif



static INT8U m_DecodeTable[256];
static BOOLEAN m_DecodeTableInitialized = FALSE;

enum
{
    DECODE_8_4_RESULT   = 0x0F,
    DECODE_8_4_ERROR    = 0x10,
    DECODE_PARITY_BIT   = 0x20,
};

unsigned char invtab[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};


static INT32U module_hamm24cor[64] =
{
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00001, 0x00000, 0x00002, 0x00004, 0x00008,
    0x00000, 0x00010, 0x00020, 0x00040, 0x00080, 0x00100, 0x00200, 0x00400,
    0x00000, 0x00800, 0x01000, 0x02000, 0x04000, 0x08000, 0x10000, 0x20000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
};
static INT16U module_hamm24err[64] =
{
    0x0000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
    0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
    0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
    0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
    0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
};

static INT8S module_hamm24val[256] =
{
    0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,
    2,  2,  2,  2,  3,  3,  3,  3,  2,  2,  2,  2,  3,  3,  3,  3,
    4,  4,  4,  4,  5,  5,  5,  5,  4,  4,  4,  4,  5,  5,  5,  5,
    6,  6,  6,  6,  7,  7,  7,  7,  6,  6,  6,  6,  7,  7,  7,  7,
    8,  8,  8,  8,  9,  9,  9,  9,  8,  8,  8,  8,  9,  9,  9,  9,
    10, 10, 10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 11, 11, 11, 11,
    12, 12, 12, 12, 13, 13, 13, 13, 12, 12, 12, 12, 13, 13, 13, 13,
    14, 14, 14, 14, 15, 15, 15, 15, 14, 14, 14, 14, 15, 15, 15, 15,
    0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,
    2,  2,  2,  2,  3,  3,  3,  3,  2,  2,  2,  2,  3,  3,  3,  3,
    4,  4,  4,  4,  5,  5,  5,  5,  4,  4,  4,  4,  5,  5,  5,  5,
    6,  6,  6,  6,  7,  7,  7,  7,  6,  6,  6,  6,  7,  7,  7,  7,
    8,  8,  8,  8,  9,  9,  9,  9,  8,  8,  8,  8,  9,  9,  9,  9,
    10, 10, 10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 11, 11, 11, 11,
    12, 12, 12, 12, 13, 13, 13, 13, 12, 12, 12, 12, 13, 13, 13, 13,
    14, 14, 14, 14, 15, 15, 15, 15, 14, 14, 14, 14, 15, 15, 15, 15
};

static INT8U  module_hamm24par[3][256] =
{
    { /* parities of first byte*/
        0, 33, 34,  3, 35,  2,  1, 32, 36,  5,  6, 39,  7, 38, 37,  4,
        37,  4,  7, 38,  6, 39, 36,  5,  1, 32, 35,  2, 34,  3,  0, 33,
        38,  7,  4, 37,  5, 36, 39,  6,  2, 35, 32,  1, 33,  0,  3, 34,
        3, 34, 33,  0, 32,  1,  2, 35, 39,  6,  5, 36,  4, 37, 38,  7,
        39,  6,  5, 36,  4, 37, 38,  7,  3, 34, 33,  0, 32,  1,  2, 35,
        2, 35, 32,  1, 33,  0,  3, 34, 38,  7,  4, 37,  5, 36, 39,  6,
        1, 32, 35,  2, 34,  3,  0, 33, 37,  4,  7, 38,  6, 39, 36,  5,
        36,  5,  6, 39,  7, 38, 37,  4,  0, 33, 34,  3, 35,  2,  1, 32,
        40,  9, 10, 43, 11, 42, 41,  8, 12, 45, 46, 15, 47, 14, 13, 44,
        13, 44, 47, 14, 46, 15, 12, 45, 41,  8, 11, 42, 10, 43, 40,  9,
        14, 47, 44, 13, 45, 12, 15, 46, 42, 11,  8, 41,  9, 40, 43, 10,
        43, 10,  9, 40,  8, 41, 42, 11, 15, 46, 45, 12, 44, 13, 14, 47,
        15, 46, 45, 12, 44, 13, 14, 47, 43, 10,  9, 40,  8, 41, 42, 11,
        42, 11,  8, 41,  9, 40, 43, 10, 14, 47, 44, 13, 45, 12, 15, 46,
        41,  8, 11, 42, 10, 43, 40,  9, 13, 44, 47, 14, 46, 15, 12, 45,
        12, 45, 46, 15, 47, 14, 13, 44, 40,  9, 10, 43, 11, 42, 41,  8
    },
    { /* parities of second byte*/
        0, 41, 42,  3, 43,  2,  1, 40, 44,  5,  6, 47,  7, 46, 45,  4,
        45,  4,  7, 46,  6, 47, 44,  5,  1, 40, 43,  2, 42,  3,  0, 41,
        46,  7,  4, 45,  5, 44, 47,  6,  2, 43, 40,  1, 41,  0,  3, 42,
        3, 42, 41,  0, 40,  1,  2, 43, 47,  6,  5, 44,  4, 45, 46,  7,
        47,  6,  5, 44,  4, 45, 46,  7,  3, 42, 41,  0, 40,  1,  2, 43,
        2, 43, 40,  1, 41,  0,  3, 42, 46,  7,  4, 45,  5, 44, 47,  6,
        1, 40, 43,  2, 42,  3,  0, 41, 45,  4,  7, 46,  6, 47, 44,  5,
        44,  5,  6, 47,  7, 46, 45,  4,  0, 41, 42,  3, 43,  2,  1, 40,
        48, 25, 26, 51, 27, 50, 49, 24, 28, 53, 54, 31, 55, 30, 29, 52,
        29, 52, 55, 30, 54, 31, 28, 53, 49, 24, 27, 50, 26, 51, 48, 25,
        30, 55, 52, 29, 53, 28, 31, 54, 50, 27, 24, 49, 25, 48, 51, 26,
        51, 26, 25, 48, 24, 49, 50, 27, 31, 54, 53, 28, 52, 29, 30, 55,
        31, 54, 53, 28, 52, 29, 30, 55, 51, 26, 25, 48, 24, 49, 50, 27,
        50, 27, 24, 49, 25, 48, 51, 26, 30, 55, 52, 29, 53, 28, 31, 54,
        49, 24, 27, 50, 26, 51, 48, 25, 29, 52, 55, 30, 54, 31, 28, 53,
        28, 53, 54, 31, 55, 30, 29, 52, 48, 25, 26, 51, 27, 50, 49, 24
    },
    { /* parities of third byte*/
        63, 14, 13, 60, 12, 61, 62, 15, 11, 58, 57,  8, 56,  9, 10, 59,
        10, 59, 56,  9, 57,  8, 11, 58, 62, 15, 12, 61, 13, 60, 63, 14,
        9, 56, 59, 10, 58, 11,  8, 57, 61, 12, 15, 62, 14, 63, 60, 13,
        60, 13, 14, 63, 15, 62, 61, 12,  8, 57, 58, 11, 59, 10,  9, 56,
        8, 57, 58, 11, 59, 10,  9, 56, 60, 13, 14, 63, 15, 62, 61, 12,
        61, 12, 15, 62, 14, 63, 60, 13,  9, 56, 59, 10, 58, 11,  8, 57,
        62, 15, 12, 61, 13, 60, 63, 14, 10, 59, 56,  9, 57,  8, 11, 58,
        11, 58, 57,  8, 56,  9, 10, 59, 63, 14, 13, 60, 12, 61, 62, 15,
        31, 46, 45, 28, 44, 29, 30, 47, 43, 26, 25, 40, 24, 41, 42, 27,
        42, 27, 24, 41, 25, 40, 43, 26, 30, 47, 44, 29, 45, 28, 31, 46,
        41, 24, 27, 42, 26, 43, 40, 25, 29, 44, 47, 30, 46, 31, 28, 45,
        28, 45, 46, 31, 47, 30, 29, 44, 40, 25, 26, 43, 27, 42, 41, 24,
        40, 25, 26, 43, 27, 42, 41, 24, 28, 45, 46, 31, 47, 30, 29, 44,
        29, 44, 47, 30, 46, 31, 28, 45, 41, 24, 27, 42, 26, 43, 40, 25,
        30, 47, 44, 29, 45, 28, 31, 46, 42, 27, 24, 41, 25, 40, 43, 26,
        43, 26, 25, 40, 24, 41, 42, 27, 31, 46, 45, 28, 44, 29, 30, 47
    }
};


/// The codepages used in various regions
static INT8S m_RegionCodepage[VTREGION_LASTONE][8] =
{
    // VTREGION_DEFAULT
    { VTCODEPAGE_ENGLISH,   VTCODEPAGE_FRENCH,  VTCODEPAGE_SWEDISH,     VTCODEPAGE_CZECH,   VTCODEPAGE_GERMAN,  VTCODEPAGE_PORTUGUESE,  VTCODEPAGE_ITALIAN,     VTCODEPAGE_NONE     },

    // VTREGION_CENTRALEUROPE
    { VTCODEPAGE_POLISH,    VTCODEPAGE_SAME,    VTCODEPAGE_SAME,        VTCODEPAGE_SAME,    VTCODEPAGE_SAME,    VTCODEPAGE_NONE,        VTCODEPAGE_SAME,        VTCODEPAGE_NONE     },

    // VTREGION_SOUTHERNEUROPE
    { VTCODEPAGE_SAME,      VTCODEPAGE_SAME,    VTCODEPAGE_SAME,        VTCODEPAGE_TURKISH, VTCODEPAGE_SAME,    VTCODEPAGE_SAME,        VTCODEPAGE_SAME,        VTCODEPAGE_NONE     },

    // VTREGION_BALKANS
    { VTCODEPAGE_NONE,      VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,    VTCODEPAGE_NONE,    VTCODEPAGE_SLOVENIAN,   VTCODEPAGE_NONE,        VTCODEPAGE_RUMANIAN },

    // VTREGION_EASTERNEUROPE
    { VTCODEPAGE_SERBIAN,   VTCODEPAGE_RUSSIAN, VTCODEPAGE_ESTONIAN,    VTCODEPAGE_SAME,    VTCODEPAGE_SAME,    VTCODEPAGE_UKRAINIAN,   VTCODEPAGE_LETTISH,     VTCODEPAGE_NONE     },

    // VTREGION_RESERVED1
    { VTCODEPAGE_NONE,      VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,    VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,        VTCODEPAGE_NONE     },

    // VTREGION_MEDITERRANEAN
    { VTCODEPAGE_NONE,      VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_TURKISH, VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,        VTCODEPAGE_GREEK    },

    // VTREGION_RESERVED2
    { VTCODEPAGE_NONE,      VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,    VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,        VTCODEPAGE_NONE     },

    // VTREGION_NORTHAFRICA
    { VTCODEPAGE_ENGLISHA,  VTCODEPAGE_FRENCHA, VTCODEPAGE_NONE,        VTCODEPAGE_NONE,    VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,        VTCODEPAGE_ARABIC   },

    // VTREGION_RESERVED3
    { VTCODEPAGE_NONE,      VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,    VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,        VTCODEPAGE_NONE     },

    // VTREGION_MIDDLEEAST
    { VTCODEPAGE_NONE,      VTCODEPAGE_NONE,    VTCODEPAGE_NONE,        VTCODEPAGE_NONE,    VTCODEPAGE_NONE,    VTCODEPAGE_HEBREW,      VTCODEPAGE_NONE,        VTCODEPAGE_ARABIC   },
};


static INT8U CheckLineParsable(struct TVTPage* pPage, INT8U nRow, INT16U* lpFlags);
static INT8U ParseEmptyLine(struct TVTPage* pPage, INT8U nRow, INT16U* lpFlags, TParserCallback fnParseProc, void* draw_para);


INT32S VTInitCommonData(void)
{
    if (m_DecodeTableInitialized == FALSE)
    {
        InitializeDecodeTable();
    }

    return AM_SUCCESS;
}


void InitializeDecodeTable(void)
{
    INT8U Parity;
    INT8U Data;
    int i;

    // Create the Odd Parity checker
    for (i = 0; i < 256; i++)
    {
        Parity = i ^ (i >> 4);
        Parity ^= Parity >> 2;
        Parity ^= Parity >> 1;

        m_DecodeTable[i] = (Parity & 1) ? 0 : DECODE_PARITY_BIT;
    }

    // Create the Hamming 8/4 decoder
    for (i = 0; i < 256; i++)
    {
        Parity = 0x80;
        Parity >>= ((m_DecodeTable[i & 0xA3] & DECODE_PARITY_BIT) != 0) * 1;
        Parity >>= ((m_DecodeTable[i & 0x8E] & DECODE_PARITY_BIT) != 0) * 2;
        Parity >>= ((m_DecodeTable[i & 0x3A] & DECODE_PARITY_BIT) != 0) * 4;

        if ((Parity != 0x80) && (m_DecodeTable[i] & DECODE_PARITY_BIT) == 0)
        {
            // Double bit error
            m_DecodeTable[i] |= DECODE_8_4_ERROR | DECODE_8_4_RESULT;
            continue;
        }

        if (Parity >> 3)
        {
            Parity &= 0x10;
            Parity >>= 1;
        }

        Data = (i & (1 << 1)) >> 1;
        Data |= (i & (1 << 3)) >> 2;
        Data |= (i & (1 << 5)) >> 3;
        Data |= (i & (1 << 7)) >> 4;

        // Correct an incorrect bit
        Data ^= Parity;

        m_DecodeTable[i] |= Data;
    }

    m_DecodeTableInitialized = TRUE;
}


/// Reverses the bits in a byte
INT8U ReverseBits(INT8U Byte)
{
    return invtab[Byte];
}


/// Returns the bit value required to make Byte Odd Parity
INT8U Parity(INT8U Byte)
{
    return (m_DecodeTable[Byte] & DECODE_PARITY_BIT) != 0;
}


/// Check the odd parity-ness of a string
BOOLEAN CheckParity(INT8U* Data, INT32U Length, BOOLEAN bStripParityBits)
{
    INT32U i = 0;
    BOOLEAN ErrorFree = TRUE;

    for (i = 0; i < Length; i++)
    {
        if (Parity(Data[i]) != 0)
        {
            ErrorFree = FALSE;
        }

        if (bStripParityBits != FALSE)
        {
            Data[i] &= 0x7F;
        }
    }

    return ErrorFree;
}


INT8U Unham84(INT8U Byte, BOOLEAN* Error)
{
    if (m_DecodeTable[Byte] & DECODE_8_4_ERROR)
    {
        *Error = TRUE;
    }

    return m_DecodeTable[Byte] & DECODE_8_4_RESULT;
}


INT8U UnhamTwo84_LSBF(INT8U Byte[2], BOOLEAN* Error)
{
    char Result;

    Result = Unham84(Byte[0], Error);
    Result |= Unham84(Byte[1], Error) << 4;

    return Result;
}


INT8U UnhamTwo84_MSBF(INT8U Byte[2], BOOLEAN* Error)
{
    char Result;

    Result = Unham84(Byte[0], Error) << 4;
    Result |= Unham84(Byte[1], Error);

    return Result;
}

INT32U module_hamming_24_18(INT8U *p)
{
    int err;
    int e = module_hamm24par[0][p[0]] ^ module_hamm24par[1][p[1]] ^ module_hamm24par[2][p[2]];
    int x = module_hamm24val[p[0]] + (p[1] % 128) * 16 + (p[2] % 128) * 2048;

    err = module_hamm24err[e];
    if (err == 0x1000)
        return 0xfffff;

    return x ^ module_hamm24cor[e];
}


BOOLEAN	IsNonVisiblePage(INT16U wPageHex)
{
    if ((wPageHex & 0xFF00) < 0x0100 ||
        (wPageHex & 0xFF00) > 0x0800)
    {
        return FALSE;
    }

    if ((wPageHex & 0x00F0) >= 0x00A0 ||
        (wPageHex & 0x000F) >= 0x000A)
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN HasDoubleElement(struct TVTPage* pPage, INT8U nRow)
{
    INT8U nCol;
    if ((pPage->LineState[nRow] & CACHESTATE_HASDATA) == 0)
    {
        return FALSE;
    }

    for (nCol = 0; nCol < 40; nCol++)
    {
        if ((pPage->Frame[nRow][nCol] & 0x7F) == 0x0D)//0x0D IS DOUBLE HEIGHT FLAG
        {
            return TRUE;
        }
    }
    return FALSE;
}


INT8S GetRegionCodepage(INT8S VTRegion, INT8U uCharacterSubsetCode, BOOLEAN bCorrectMissing)
{
    int i = 0;

    if (uCharacterSubsetCode > 0x07)
    {
        return VTCODEPAGE_ENGLISH;
    }
    if (VTRegion < VTREGION_DEFAULT || VTRegion >= VTREGION_LASTONE)
        VTRegion = VTREGION_DEFAULT;

    INT8S Codepage = m_RegionCodepage[VTRegion][uCharacterSubsetCode];

    // Same indicates it's the same as the first
    if (Codepage == VTCODEPAGE_SAME)
    {
        Codepage = m_RegionCodepage[VTREGION_DEFAULT][uCharacterSubsetCode];
    }

    // If it's none, the region is wrong.  Pick the
    // first valid one we come accross
    if (bCorrectMissing != FALSE && Codepage == VTCODEPAGE_NONE)
    {
        for (i = VTREGION_DEFAULT; i < VTREGION_LASTONE; i++)
        {
            Codepage = m_RegionCodepage[i][uCharacterSubsetCode];

            if (Codepage != VTCODEPAGE_NONE && Codepage != VTCODEPAGE_SAME)
            {
                break;
            }
        }
    }

    return Codepage;
}

/*
 *  ParsePageElements
 *
 *  Parses all the elements in a Teletext page and calls the
 *  supplied callback function for all the characters.  This
 *  function understands the exclusive control characters in
 *  Teletext pages and should be used rather than attempting
 *  to parse pages with normal string manipulation functions.
 *
 *  These are the relavent uFlags:
 *
 *    PARSE_HASDATAONLY    - Only parse lines with HASDATA set
 *    PARSE_UPDATEDONLY    - Only parse lines with UPDATED set
 *    PARSE_REPEATDOUBLE   - Repeat lines with double in it as
 *                           the contents of the following row
 *    PARSE_FORCEDHEADER   - Always perform parsing for header
 *    PARSE_ALSOKEYWORDS   - Include keywords line for parsing
 *
 *  The parser callback function should return one of these:
 *
 *    PARSE_CONTINUE      - Continue normally to the next char
 *    PARSE_STOPLINE      - Stop the currently processing line
 *    PARSE_STOPPAGE      - Stop parsing the rest of this page
 *
 *  The return value of ParsePageElements is the value returned
 *  by the last call of ParseLineElements.  This may or may not
 *  be the same value returned by the last call to the callback
 *  function.
 *
 *  ParseLineElements may return one of these values:
 *
 *    PARSE_STOPLINE        - The callback returned this value
 *    PARSE_STOPPAGE        - The callback returned this value
 *    PARSE_NODATA          - PARSE_HASDATAONLY is set and the
 *                            line does not have received data
 *    PARSE_SUPPRESSHEADER  - PARSE_HASDATAONLY is set and the
 *                            header is suppressed by the page
 *    PARSE_INHIBITDISP     - PARSE_HASDATAONLY is set and the
 *                            display is inhibited by the page
 *    PARSE_ROW24HIDDEN     - PARSE_HASDATAONLY is set and the
 *                            line 24 is not shown by the page
 *    PARSE_SKIPKEYWORDS    - PARSE_ALSOKEYWORDS bit is not on
 *                            and the line was navigation line
 *    PARSE_NOTUPDATED      - PARSE_UPDATEDONLY is set and the
 *                            line state is not set to updated
 *    PARSE_DOUBLESKIP      - PARSE_REPEATDOUBLE bit is not on
 *                            and this line is invalid because
 *                            of a double on the previous line
 *    PARSE_CONFIGERROR     - Bad flags were set with the call
 *
 */


INT8U ParsePageElements(struct TVTPage* pPage, INT16U* lpFlags, TParserCallback fnParseProc, void* draw_para)
{
    INT8U uRow;
    INT8U uLastRow;
    INT8U uResult = PARSE_CONTINUE;
    INT16U uFlags = *lpFlags;

    if ((*lpFlags & PARSE_ALSOKEYWORDS) != 0)
    {
        uLastRow = 25;
    }
    else
    {
        uLastRow = 24;
    }

    *lpFlags &= ~(PARSE_DOUBLEHEIGHT | PARSE_DOUBLEREPEAT);

    // Loop through the rows
    for (uRow = 0; uRow <= uLastRow; uRow++)
    {
        uFlags = *lpFlags | PARSE_SKIPDOUBLECHECK;

        // Call the line elements parser
        if ((uResult = ParseLineElements(pPage, uRow, &uFlags, fnParseProc, draw_para)) == PARSE_STOPPAGE)
        {
            //break;
        }

        if (uResult == PARSE_NOTUPDATED)
        {
            // The line was not updated so the necessary double
            // check was not performed.  Do it here.
            if (uRow >= 1 && uRow < 23 && HasDoubleElement(pPage, uRow))
            {
                uFlags |= PARSE_DOUBLEHEIGHT;
            }
        }

        if ((uFlags & PARSE_DOUBLEHEIGHT) != 0)
        {
            uFlags = *lpFlags | PARSE_DOUBLEREPEAT | PARSE_SKIPDOUBLECHECK;

            // Call the line parser again for the repeat
            if ((uResult = ParseLineElements(pPage, uRow, &uFlags, fnParseProc, draw_para)) == PARSE_STOPPAGE)
            {
                //break;
            }
            uRow++;
        }
    }

    *lpFlags = uFlags;

    return (INT8U)uResult;
}


INT8U ParseLineElements(struct TVTPage* pPage, INT8U nRow, INT16U* lpFlags, TParserCallback fnParseProc, void* draw_para)
{
    INT8U DisplayChar;
    INT8U DisplayModes;
    INT8U DisplayColour;
    INT8U DisplayBkColour;
    INT8U DisplayRow;
    INT8U SetAfterModes;
    INT8U SetAfterColour;
    INT8U Background;
    INT8U nLastBoxPos;
    INT8U nLastUnboxPos;
    INT8U HeldGraphChar;
    INT8U uResult;
    INT8U nConsecutiveDoubles;
    INT8U nCol;
    BOOLEAN bHeldGraphSeparated = FALSE;
    BOOLEAN bBoxedElementsOnly = FALSE;


    if ((*lpFlags & PARSE_DOUBLEREPEAT) != 0)
    {
        if (nRow < 1 || nRow >= 23)
        {
            return PARSE_CONFIGERROR;
        }
    }

    // Check if we are to look for doubles in the previous row
    if ((*lpFlags & PARSE_SKIPDOUBLECHECK) == 0 && nRow > 1 && nRow <= 23)
    {
        // Check if the previous line had a double
        if (HasDoubleElement(pPage, nRow - 1))
        {
            // Check that this isn't already a double repeated
            if ((*lpFlags & PARSE_DOUBLEREPEAT) != 0)
            {
                nConsecutiveDoubles = 1;

                // There are problematic doubles in the page
                for (DisplayRow = nRow - 2; DisplayRow >= 1 && HasDoubleElement(pPage, DisplayRow); DisplayRow--)
                {
                    nConsecutiveDoubles++;
                }

                // Check if this double repeat is valid
                if ((nConsecutiveDoubles % 2) == 1)
                {
                    // This row is invalid, so the double repeat
                    // is also invalid.  Redo the next row line
                    // as a non-repeat
                    *lpFlags &= ~PARSE_DOUBLEREPEAT;
                    nRow++;
                }
                // Ignore the double on the previous line
                // and continue repeating the double on
                // this line as the next row line.
            }
            else
            {
                // Repeat the previous as this line
                *lpFlags |= PARSE_DOUBLEREPEAT;
                return ParseLineElements(pPage, nRow - 1, lpFlags, fnParseProc, draw_para);
            }
        }
    }

    // If this row is a repeat, the actual row is the one below
    if ((*lpFlags & PARSE_DOUBLEREPEAT) != 0)
    {
        if ((*lpFlags & PARSE_REPEATDOUBLE) == 0)
        {
            return PARSE_DOUBLESKIP;
        }

        DisplayRow = nRow + 1;
    }
    else
    {
        DisplayRow = nRow;
    }

    // Check if we need to process this line
    uResult = CheckLineParsable(pPage, nRow, lpFlags);

    if (uResult != PARSE_CONTINUE)
    {
        if ((*lpFlags & PARSE_EMPTYLINE) != 0)
        {
            return ParseEmptyLine(pPage, nRow, lpFlags, fnParseProc, draw_para);
        }
        return (INT8U)uResult;
    }

    if (nRow != 25)
    {
        // If newsflash or subtitle is set, only boxed elements
        // should be displayed
        if ((pPage->wControlBits & VTCONTROL_NEWSFLASH) != 0 ||
            (pPage->wControlBits & VTCONTROL_SUBTITLE) != 0)
        {
            // Ignore if force header is on
            if (nRow != 0 || (*lpFlags & PARSE_FORCEHEADER) == 0)
            {
                bBoxedElementsOnly = TRUE;
            }
        }
    }

    DisplayModes = 0;
    DisplayColour = VTCOLOR_WHITE;
    Background = VTCOLOR_BLACK;
    HeldGraphChar = 0x20;

    nLastBoxPos = 40;
    nLastUnboxPos = 40;

    for (nCol = 0; nCol < 40; nCol++)
    {
        SetAfterModes = 0;
        SetAfterColour = DisplayColour;

        DisplayChar = pPage->Frame[nRow][nCol] & 0x7F;

        // Remember the most recent graphics character
        if ((DisplayModes & VTMODE_GRAPHICS) && (DisplayChar & 0x20))
        {
            HeldGraphChar = DisplayChar;
            bHeldGraphSeparated = (DisplayModes & VTMODE_SEPARATED);
        }

        // Check if the character is a control character
        if (DisplayChar < 0x20)
        {
            switch (DisplayChar)
            {
                case 0x00:  // NUL (reserved) (2.5+: Alpha Black)
                    // Workaround to 1.x vs 2.5+ conflict
                   /* if (Background == 0)
                    {
                        break;
                    }*/
                case 0x01:  // Alpha Red
                case 0x02:  // Alpha Green
                case 0x03:  // Alpha Yellow
                case 0x04:  // Alpha Blue
                case 0x05:  // Alpha Magenta
                case 0x06:  // Alpha Cyan
                case 0x07:  // Alpha White
                    SetAfterColour = DisplayChar;
                    SetAfterModes |= (DisplayModes & VTMODE_GRAPHICS);
                    SetAfterModes |= (DisplayModes & VTMODE_CONCEAL);
                    break;
                case 0x08:  // Flash
                    SetAfterModes |= (DisplayModes & VTMODE_FLASH ^ VTMODE_FLASH);
                    break;
                case 0x09:  // Steady (immediate)
                    DisplayModes &= ~VTMODE_FLASH;
                    break;
                case 0x0a:  // Unboxed (immediate on 2nd)
                    if (nCol == (nLastUnboxPos + 1))
                    {
                        DisplayModes &= ~VTMODE_BOXED;
                    }
                    else
                    {
                        nLastUnboxPos = nCol;
                    }
                    break;
                case 0x0b:  // Boxed (immediate on 2nd)
                    if (nCol == (nLastBoxPos + 1))
                    {
                        DisplayModes |= VTMODE_BOXED;
                    }
                    else
                    {
                        nLastBoxPos = nCol;
                    }
                    break;
                case 0x0c:  // Normal Height (immediate)
                    DisplayModes &= ~VTMODE_DOUBLE;
                    break;
                case 0x0d:  // Double Height
                    // Double on rows 0, 23, 24, 25 not permitted
                    if (nRow > 0 && nRow < 23)
                    {
                        *lpFlags |= PARSE_DOUBLEHEIGHT;
                        SetAfterModes |= (DisplayModes & VTMODE_DOUBLE ^ VTMODE_DOUBLE);
                    }
                    break;
                case 0x0e:  // Shift Out (reserved)
                case 0x0f:  // Shift In (reserved
                    break;
                case 0x10:  // DLE (reserved) (2.5+: Graphics Black)
                    // Workaround to 1.x vs 2.5+ conflict
                    if (Background == 0)
                    {
                        break;
                    }
                case 0x11:  // Graphics Red
                case 0x12:  // Graphics Green
                case 0x13:  // Graphics Yellow
                case 0x14:  // Graphics Blue
                case 0x15:  // Graphics Magenta
                case 0x16:  // Graphics Cyan
                case 0x17:  // Graphics White
                    SetAfterColour = DisplayChar - 0x10;
                    SetAfterModes |= (DisplayModes & VTMODE_GRAPHICS ^ VTMODE_GRAPHICS);
                    SetAfterModes |= (DisplayModes & VTMODE_CONCEAL);
                    break;
                case 0x18:  // Conceal (immediate)
                    DisplayModes |= VTMODE_CONCEAL;
                    break;
                case 0x19:  // Contiguous Graphics (either)
                    DisplayModes &= ~VTMODE_SEPARATED;
                    break;
                case 0x1a:  // Separated Graphics (either)
                    DisplayModes |= VTMODE_SEPARATED;
                    break;
                case 0x1b:  // ESC (reserved)
                    break;
                case 0x1c:  // Black Background (immediate)
                    Background = VTCOLOR_BLACK;
                    break;
                case 0x1d:  // New Background (immediate)
                    Background = DisplayColour;
                    break;
                case 0x1e:  // Hold Graphics (immediate)
                    DisplayModes |= VTMODE_HOLD;
                    break;
                case 0x1f:  // Release Graphics
                    SetAfterModes |= (DisplayModes & VTMODE_HOLD);
                    break;
            }

            DisplayChar = 0x20;

            if ((DisplayModes & VTMODE_HOLD) != 0)
            {
                DisplayChar = HeldGraphChar;
                if (!bHeldGraphSeparated != !(DisplayModes & VTMODE_SEPARATED))
                {
                    DisplayModes ^= VTMODE_SEPARATED;
                    SetAfterModes ^= VTMODE_SEPARATED;
                }
            }
        }

        if ((bBoxedElementsOnly != FALSE) && !(DisplayModes & VTMODE_BOXED))
        {
            DisplayChar = 0x20;
            DisplayColour = VTCOLOR_NONE;
            DisplayBkColour = VTCOLOR_NONE;
        }
        else
        {
            DisplayBkColour = Background;
        }

        //AM_TRACE("[%s] DisplayChar = %d,DisplayColour =%d,DisplayBkColour = %d\r\n",__func__,DisplayChar,DisplayColour,DisplayBkColour);
        // Call the callback function
        uResult = (fnParseProc)(pPage, MAKEWORD(DisplayRow, nCol), lpFlags,
                                MAKEWORD(DisplayColour, DisplayBkColour),
                                DisplayChar, DisplayModes, draw_para);

        if (uResult == PARSE_STOPLINE || uResult == PARSE_STOPPAGE)
        {
            break;
        }

        DisplayModes ^= SetAfterModes;
        DisplayColour = SetAfterColour;

        // Forget the last graph character if one of these changed
        if (SetAfterModes & VTMODE_GRAPHICS || SetAfterModes & VTMODE_DOUBLE)
        {
            HeldGraphChar = 0x20;
        }
    }
    return (INT8U)uResult;
}

static INT8U CheckLineParsable(struct TVTPage* pPage, INT8U nRow, INT16U* lpFlags)
{
    INT8U uCheckResult;

    if ((*lpFlags & PARSE_EMPTYLINE) != 0)
    {
        return PARSE_NODATA;
    }

    if (nRow == 0 && (*lpFlags & PARSE_FORCEHEADER) != 0)
    {
        if ((pPage->LineState[nRow] & CACHESTATE_HASDATA) != 0)
        {
            return PARSE_CONTINUE;
        }
    }

    // Check if the caller wants updated rows only
    if ((*lpFlags & PARSE_UPDATEDONLY) != 0)
    {
        if ((pPage->LineState[nRow] & CACHESTATE_UPDATED) == 0)
        {
            return PARSE_NOTUPDATED;
        }
    }

    uCheckResult = PARSE_CONTINUE;

    // If this row has no data
    if ((pPage->LineState[nRow] & CACHESTATE_HASDATA) == 0)
    {
        uCheckResult = PARSE_NODATA;
    }
    // If this row has data and is the header row
    else if (nRow == 0)
    {
        if ((pPage->wControlBits & VTCONTROL_SUPRESSHEADER) != 0)
        {
            uCheckResult = PARSE_SUPPRESSHEADER;
        }
    }
    // If this row has data and is the commentary row
    else if (nRow == 24)
    {
        if (pPage->bShowRow24 == FALSE)
        {
            uCheckResult = PARSE_ROW24HIDDEN;
        }
    }
    // If this row has data and is the keywords row
    else if (nRow == 25)
    {
        if ((*lpFlags & PARSE_ALSOKEYWORDS) == 0)
        {
            // Has data only flag doesn't affect his row
            return PARSE_SKIPKEYWORDS;
        }
    }
    // For all other rows
    else
    {
        if ((pPage->wControlBits & VTCONTROL_INHIBITDISP) != 0)
        {
            uCheckResult = PARSE_INHIBITDISP;
        }
    }

    if (uCheckResult != PARSE_CONTINUE)
    {
        if ((*lpFlags & PARSE_HASDATAONLY) == 0)
        {
            *lpFlags = PARSE_EMPTYLINE;
        }
    }

    return uCheckResult;
}


static INT8U ParseEmptyLine(struct TVTPage* pPage, INT8U nRow, INT16U* lpFlags, TParserCallback fnParseProc, void* draw_para)
{
    INT8U uResult = PARSE_CONTINUE;
    INT16U wColour = MAKEWORD(VTCOLOR_WHITE, VTCOLOR_BLACK);
    INT8U  nCol;

    if ((pPage->wControlBits & VTCONTROL_NEWSFLASH) != 0 ||
        (pPage->wControlBits & VTCONTROL_SUBTITLE) != 0)
    {
        if (nRow != 0 || (*lpFlags & PARSE_FORCEHEADER) == 0)
        {
            wColour = MAKEWORD(VTCOLOR_WHITE, VTCOLOR_NONE);
        }
    }

    for (nCol = 0; nCol < 40; nCol++)
    {
        uResult = (fnParseProc)(pPage, MAKEWORD(nRow, nCol), lpFlags, wColour, 0x20, 0x00, draw_para);

        if (uResult == PARSE_STOPLINE || uResult == PARSE_STOPPAGE)
        {
            break;
        }
    }

    return (INT8U)uResult;
}





