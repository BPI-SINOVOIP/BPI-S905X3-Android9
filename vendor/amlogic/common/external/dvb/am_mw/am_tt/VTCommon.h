/****************************************************************************
/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
/*
**
** File Name:   TELETEXT FILE
**
** Revision:    1.0
** Date:        2008.2.20
**
** Description: Header file for smc api.
**
****************************************************************************/

#ifndef __VTCOMMON_H_
#define __VTCOMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "includes.h"


#ifndef __ROM_
#define M_TELETEXT_DIAG(x)                         \
{                                               \
    printf x;                                   \
}
#else
#define M_TELETEXT_DIAG(x)
#endif


#ifndef AM_SUCCESS
#define AM_SUCCESS			(0)
#endif

#ifndef AM_FAILURE
#define AM_FAILURE			(-1)
#endif

#ifndef TRUE
#define TRUE                (1)
#endif

#ifndef FALSE
#define FALSE               (0)
#endif

#ifndef MAKEWORD
#define MAKEWORD(x,y)       ((INT16U)((y << 8)|x))
#endif

#ifndef MAKELONG
#define MAKELONG(x,y)       ((INT32U)((y << 16)|x))
#endif

#ifndef HIGHBYTE
#define HIGHBYTE(x)         ((INT8U)((x >> 8)&0xff))
#endif

#ifndef LOWBYTE
#define LOWBYTE(x)          ((INT8U)((x)&0xff))
#endif

#ifndef HIGHWORD
#define HIGHWORD(x)         ((INT16U)((x >> 16)&0xffff))
#endif

#ifndef LOWWORD
#define LOWWORD(x)          ((INT16U)((x)&0xffff))
#endif

typedef struct
{
    INT32S          left;
    INT32S          top;
    INT32U          width;
    INT32U          height;
} RECT;

#ifndef LPRECT
typedef RECT*       LPRECT;
#endif

/// The description of a VideoText page
struct TVTPage
{
    INT32U			dwPageCode;
    INT16U  		wControlBits;
    INT8U 			uCharacterSubset;
    INT8U 			uCharacterRegion;
    INT8U   		Frame[26][40];
    INT8U   		LineState[26];
    INT32U  		EditorialLink[6];
    BOOLEAN			bBufferReserved;
    BOOLEAN    		bReceived;
    BOOLEAN    		bShowRow24;
    struct TVTPage 	*pNextPage;
};


/// The callback function used by ParsePageElements and ParseLineElements
typedef INT8U (*TParserCallback)(struct TVTPage *pPage, INT16U uPoint, INT16U *lpFlags, INT16U uColour, INT8U uChar, INT8U uMode, void *draw_para);

/// Control bits in a page header
enum
{
    VTCONTROL_MAGAZINE      = 7 << 0,
    VTCONTROL_ERASEPAGE     = 1 << 3,
    VTCONTROL_NEWSFLASH     = 1 << 4,
    VTCONTROL_SUBTITLE      = 1 << 5,
    VTCONTROL_SUPRESSHEADER = 1 << 6,
    VTCONTROL_UPDATE        = 1 << 7,
    VTCONTROL_INTERRUPTED   = 1 << 8,
    VTCONTROL_INHIBITDISP   = 1 << 9,
    VTCONTROL_MAGSERIAL     = 1 << 10,
    VTCONTROL_CHARSUBSET    = 7 << 11,
};


/// VideoText display modes
enum
{
    VTMODE_GRAPHICS     = 1 << 0,
    VTMODE_SEPARATED    = 1 << 1,
    VTMODE_FLASH        = 1 << 2,
    VTMODE_CONCEAL      = 1 << 3,
    VTMODE_BOXED        = 1 << 4,
    VTMODE_DOUBLE       = 1 << 5,
    VTMODE_HOLD         = 1 << 6,
};


/// VideoText display colours
enum
{
    VTCOLOR_BLACK   = 0,
    VTCOLOR_RED     = 1,
    VTCOLOR_GREEN   = 2,
    VTCOLOR_YELLOW  = 3,
    VTCOLOR_BLUE    = 4,
    VTCOLOR_MAGENTA = 5,
    VTCOLOR_CYAN    = 6,
    VTCOLOR_WHITE   = 7,

    // These one is irregular
    VTCOLOR_NONE    = 8,
};


/// Coloured buttons on the remote
enum
{
    VTFLOF_RED      = 0,
    VTFLOF_GREEN    = 1,
    VTFLOF_YELLOW   = 2,
    VTFLOF_BLUE     = 3,
    VTFLOF_INDEX    = 4,
    VTFLOF_UNKN1    = 5
};


/// Videotext codepages
enum
{
    VTCODEPAGE_SAME             = -2,   // Special internal use
    VTCODEPAGE_NONE             = -1,   // Special internal use
    VTCODEPAGE_ENGLISH          = 0,
    VTCODEPAGE_FRENCH,
    VTCODEPAGE_SWEDISH,                 // Also Finnish/Hungarian
    VTCODEPAGE_CZECH,                   // Also Slovak
    VTCODEPAGE_GERMAN,
    VTCODEPAGE_PORTUGUESE,              // Also Spanish
    VTCODEPAGE_ITALIAN,
    VTCODEPAGE_POLISH,
    VTCODEPAGE_TURKISH,
    VTCODEPAGE_SLOVENIAN,               // Also Serbian/Croation (Latin)
    VTCODEPAGE_RUMANIAN,
    VTCODEPAGE_SERBIAN,                 // Also Croatian (Cyrillic-1)
    VTCODEPAGE_RUSSIAN,                 // Also Bulgarian
    VTCODEPAGE_ESTONIAN,
    VTCODEPAGE_UKRAINIAN,
    VTCODEPAGE_LETTISH,                 // Also Lithuanian
    VTCODEPAGE_GREEK,
    VTCODEPAGE_ENGLISHA,                // English with Arabic G2
    VTCODEPAGE_FRENCHA,                 // French with Arabic G2
    VTCODEPAGE_ARABIC,
    VTCODEPAGE_HEBREW,
    VTCODEPAGE_DANISH,
    VTCODEPAGE_LASTONE
};

/// Videotext codepages
/// These names are guesses --AtNak
enum
{
    VTREGION_DEFAULT            = 0,
    VTREGION_CENTRALEUROPE,
    VTREGION_SOUTHERNEUROPE,
    VTREGION_BALKANS,
    VTREGION_EASTERNEUROPE,
    VTREGION_RESERVED1,
    VTREGION_MEDITERRANEAN,
    VTREGION_RESERVED2,
    VTREGION_NORTHAFRICA,
    VTREGION_RESERVED3,
    VTREGION_MIDDLEEAST,
    VTREGION_LASTONE
};

/// Special page and subpage values
enum
{
    /*
     * These values are local use and not from
     * any VT standards.  Where these are used
     * need to be under strict control because
     * not all path look out for these special
     * values.
     */

    VTPAGE_ERROR                = 0x000,

    VTPAGE_FLOFRED              = 0x010,
    VTPAGE_FLOFGREEN            = 0x011,
    VTPAGE_FLOFYELLOW           = 0x012,
    VTPAGE_FLOFBLUE             = 0x013,
    VTPAGE_PREVIOUS             = 0x020,

    VTPAGE_NULLMASK             = 0x0FF,

    VTSUBPAGE_NULL              = 0x3F7F,
    VTSUBPAGE_UNSET             = 0xFFFF,
};

/// Bit vector for LineState
enum
{
    CACHESTATE_HASDATA       = 1 << 0,
    CACHESTATE_UPDATED       = 1 << 1,
    CACHESTATE_HASERROR      = 1 << 2,
};

/// ParsePageElements and ParseLineElements flags
enum
{
    // ParsePageElements input
    PARSE_HASDATAONLY           = 0x0001,
    PARSE_UPDATEDONLY           = 0x0002,
    PARSE_FORCEHEADER           = 0x0004,
    PARSE_REPEATDOUBLE          = 0x0008,
    PARSE_ALSOKEYWORDS          = 0x0010,

    // Parse line input
    PARSE_SKIPDOUBLECHECK       = 0x0020,

    // ParseLineElements output
    PARSE_DOUBLEHEIGHT          = 0x0100,

    // Callback input
    PARSE_DOUBLEREPEAT          = 0x0040,
    PARSE_EMPTYLINE             = 0x0080,

    // Callback return values
    PARSE_CONTINUE              = 0,
    PARSE_STOPLINE              = 1,
    PARSE_STOPPAGE              = 2,

    // ParseLineElements return values

    PARSE_NODATA                = 3,
    PARSE_SUPPRESSHEADER        = 4,
    PARSE_INHIBITDISP           = 5,
    PARSE_ROW24HIDDEN           = 6,
    PARSE_SKIPKEYWORDS          = 7,
    PARSE_NOTUPDATED            = 8,
    PARSE_DOUBLESKIP            = 9,
    PARSE_CONFIGERROR           = 10,
};


INT32S VTInitCommonData(void);
INT8U Parity(INT8U Byte);
void InitializeDecodeTable(void);
// Reverses the bits in an 8-bit byte
INT8U ReverseBits(INT8U Byte);
// Checks if the data is odd parity.  The parity bit is the 8th
BOOLEAN CheckParity(INT8U* Data, INT32U Length , BOOLEAN bStripParityBits);
// Unhams a Hamming 8/4 coded byte
INT8U Unham84(INT8U pByte, BOOLEAN* Error);
// Unhams two Hamming 8/4 coded bytes.  The first byte becomes the MSB
INT8U UnhamTwo84_MSBF(INT8U Byte[2], BOOLEAN* Error);
// Unhams two Hamming 8/4 coded bytes.  The first byte becomes the LSB
INT8U UnhamTwo84_LSBF(INT8U Byte[2], BOOLEAN* Error);
// Unhams a Hamming 24/18 coded triplet
INT32U module_hamming_24_18(INT8U *p);
INT8S GetRegionCodepage(INT8S VTRegion, INT8U uCharacterSubsetCode, BOOLEAN bCorrectMissing);
INT8U ParsePageElements(struct TVTPage* pPage, INT16U* lpFlags, TParserCallback fnParseProc, void* draw_para);
INT8U ParseLineElements(struct TVTPage* pPage, INT8U nRow, INT16U* lpFlags, TParserCallback fnParseProc, void* draw_para);
BOOLEAN HasDoubleElement(struct TVTPage* pPage, INT8U nRow);
BOOLEAN	IsNonVisiblePage(INT16U wPageHex);


#ifdef __cplusplus
}
#endif

#endif

