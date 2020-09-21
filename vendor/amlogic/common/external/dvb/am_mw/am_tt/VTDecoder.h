/****************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
*/
/*
** File Name:   smc_api.h
**
** Revision:    1.0
** Date:        2008.2.20
**
** Description: Header file for smc api.
**
****************************************************************************/

#ifndef __VTDECODER_H_
#define __VTDECODER_H_

#include "VTCommon.h"


typedef void (*Teletext_RegFunc)(INT32U nEvent, INT32U uPageCode);


/// Decoder Event messages
enum
{
    DECODEREVENT_HEADERUPDATE   = 0x01,    // A new rolling header is ready
    DECODEREVENT_PAGEUPDATE     = 0x02,    // Page update have been received
    DECODEREVENT_PAGEREFRESH    = 0x04,    // Page received with no update
    DECODEREVENT_COMMENTUPDATE  = 0x08,    // Row 24 commentary changed
    DECODEREVENT_GETWAITINGPAGE = 0x10,    //
};

/// Cache control settings
enum
{
    // Controls how internal page lines cache
    // is updated when a new line is received
    DECODERCACHE_NORMAL         = 0,    // Update if the cached line has errors
    DECODERCACHE_SECONDCHANCE   = 1,    // Update if the new line is error free
    DECODERCACHE_ALWAYSUPDATE   = 2,    // Always update the line
};
// Bit vector used by LinkReceived
enum
{
    LINKRECV_RECVMASK            = 0x3F,
    LINKRECV_SHOW24              = 0x40,
    LINKRECV_HIDE24              = 0x80,
};


// The receiving buffer for new pages
typedef struct _MagazineState
{
    INT16U wPageHex;
    INT16U wPageSubCode;
    INT32U dwPageCode;

    // Packet X/0
    BOOLEAN     bReceiving;
    BOOLEAN     bNewData;
    INT16U 	wControlBits;
    INT8U 	uCharacterSubset;
    INT8U 	uCharacterRegion;

    INT8U 	Header[32];

    // Packets X/1 to X/25
    INT8U 	Line[25][40];
    // This must be BYTE to be 1 byte big
    INT8U 	bLineReceived[25];

    // Packet X/27/0
    INT32U 	EditorialLink[6];
    INT8U 	LinkReceived;

} TMagazineState;

// Broadcast service data
typedef struct _ServiceData
{
    INT32U	InitialPage;
    INT16U  NetworkIDCode[2];
    INT8S	TimeOffset;
    INT32U  ModifiedJulianDate;
    INT8U   UTCHours;
    INT8U   UTCMinutes;
    INT8U   UTCSeconds;
    INT8U   StatusDisplay[20];
} TServiceData;

INT32S VTInitDecoderData(INT32U pageIndex);
void VTFreeDecoderData(void);
void VTDecodeLine(INT8U *data);
void ResetDecoder(void);
void ResetPageStore(void);
struct TVTPage* GetPageStore(INT32U dwPageCode, BOOLEAN bUpdate);
INT32U GetProcessingPageCode(void);
INT32U GetReceivedPagesCount(void);
INT16U GetVisiblePageNumbers(INT16U* lpNumberList, INT16U nListSize);
INT16U GetNonVisiblePageNumbers(INT16U* lpNumberList, INT16U nListSize);
void GetDisplayHeader(struct TVTPage* pBuffer, BOOLEAN bClockOnly);
INT32U GetDisplayPage(INT32U dwPageCode, struct TVTPage* pBuffer);
INT32U GetNextDisplayPage(INT32U dwPageCode, struct TVTPage* pBuffer, BOOLEAN bReverse);
INT32U GetNextDisplaySubPage(INT32U dwPageCode, struct TVTPage* pBuffer, BOOLEAN bReverse);
BOOLEAN GetDisplayComment(INT32U dwPageCode, struct TVTPage* pBuffer);
void CreateTestDisplayPage(struct TVTPage* pBuffer);
void GetStatusDisplay(INT8S* lpBuffer, INT32U nLength);
void SetCachingControl(INT8U uCachingControl);
void SetHighGranularityCaching(BOOLEAN bEnable);
void SetSubstituteSpacesForError(BOOLEAN bEnable);
void TeletextRegisterfunc(Teletext_RegFunc RegFunc);
void SetTeletextMaxPageNum(INT16U uMaxPageNum);
void SetTeletextCurrPageIndex(INT16U uPagrIndex);
BOOLEAN CheckTeletextPageInRange(INT16U uPageCode);
INT16U PageHex2ArrayIndex(INT16U wPageHex);
void SetWaitingPage(INT16U uPageCode, BOOLEAN bStatus);
INT32S GetTeletextMemSize(void);
void FreeNoNeedPageStore(INT16U uPageIndex);

/*@}*/
#endif

