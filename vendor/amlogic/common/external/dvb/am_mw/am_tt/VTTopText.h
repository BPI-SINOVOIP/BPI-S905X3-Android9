/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/


/**
 * @file vttoptext.h vttoptext Header file
 */

#ifndef __VTTOPTEXT_H__
#define __VTTOPTEXT_H__

#include "VTCommon.h"


typedef struct
{
    INT8U	uType;
    INT32U  uPageCode;
} TExtraTopPage;

// Variable constants
enum
{
    TOPMAX_EXTRAPAGES   = 10,
};

// Bits used in m_BTTable[800]
enum
{
    TOP_VALUEMASK       = 0x0F,
    TOP_UNRECEIVED      = 1 << 4,
};

enum eTopLevel
{
    TOPLEVEL_SUBTITLE   = 0x01,
    TOPLEVEL_PROGRAM    = 0x02,
    TOPLEVEL_BLOCK      = 0x04,
    TOPLEVEL_GROUP      = 0x06,
    TOPLEVEL_NORMAL     = 0x08,
    TOPLEVEL_LASTLEVEL  = 0x0B,
};

enum
{
    TOPTYPE_MPT         = 0x01,
    TOPTYPE_AIT         = 0x02,
};

enum
{
    TOPWAIT_GREEN       = 0,
    TOPWAIT_YELLOW,
    TOPWAIT_BLUE,
    TOPWAIT_YELLOW_AIT,
    TOPWAIT_BLUE_AIT,
    TOPWAIT_MPT,
    TOPWAIT_LASTONE
};


INT32S 		VTInitTopTextData(void);
void 		VTFreeTopTextData(void);
void 		VTTopTextReset(void);
INT32U 		DecodePageRow(INT32U uPageCode, INT8U nRow, INT8U* pData);
BOOLEAN 	IsTopTextPage(INT32U uPageCode);
BOOLEAN 	GetTopTextDetails(INT32U uPageCode, struct TVTPage* pBuffer, BOOLEAN bWaitMessage);
BOOLEAN 	DecodeBTTPageRow(INT8U nRow, INT8U* pData);
BOOLEAN 	DecodeMPTPageRow(INT8U nRow, INT8U* pData);
BOOLEAN 	DecodeAITPageRow(INT8U nRow, INT8U* pData);

#endif
