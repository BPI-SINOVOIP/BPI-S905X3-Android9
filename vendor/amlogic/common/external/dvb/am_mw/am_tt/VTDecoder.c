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
 *  Description: TELETEXT FILE
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

#include "VTDecoder.h"
#include "VTTopText.h"
#include "VTCommon.h"
#include "string.h"
#include <stdio.h>
#include <pthread.h>
#include <am_tt.h>
#include <android/log.h>

#ifndef AM_SUCCESS
#define AM_SUCCESS			(0)
#endif

#ifndef AM_FAILURE
#define AM_FAILURE			(-1)
#endif

#define MAX_PAGELIST         (64)

static INT8U			m_LastMagazine;
static INT32U                   m_LastPageCode;
static TMagazineState      	m_MagazineState[8];
static BOOLEAN			m_bMagazineSerial;

static INT8U               	m_CommonHeader[32];
static INT32U			m_ReceivedPages;

static struct TVTPage*      m_NonVisiblePageList;
static struct TVTPage*      m_VisiblePageList[800];

static TServiceData        	m_BroadcastServiceData;
static BOOLEAN             	m_bTopTextForComment = FALSE;
static INT8U               	m_CachingControl;
static INT32U 				m_CurrPageCode;
static BOOLEAN             	m_bHighGranularityCaching;
static BOOLEAN             	m_bSubstituteSpacesForError;
static BOOLEAN             	m_bTopTextForComment;
static Teletext_RegFunc 	m_DecoderEventProc = NULL;

static INT32S 				uTeletextMemSize;
static INT16U 				uTtxMaxPageNum;
static INT16U 				uTtxCurrPageIndex;

static BOOLEAN 		bWaitingPage;
static INT16U 		uWaitingPageCode;

static pthread_mutex_t m_CommonHeaderMutex ;
static pthread_mutex_t m_MagazineStateMutex ;
static pthread_mutex_t m_PageStoreMutex ;
static pthread_mutex_t m_ServiceDataStoreMutex ;


static void VTCompleteMagazine(TMagazineState* magazineState);
static void InitializePage(struct TVTPage* pPage);
static struct TVTPage* FindReceivedPage(struct TVTPage* pPageList);
static struct TVTPage* FindSubPage(struct TVTPage* pPageList, INT32U dwPageCode);
static struct TVTPage* FindNextSubPage(struct TVTPage* pPageList, INT32U dwPageCode, BOOLEAN bReverse);
static void UnsetUpdatedStates(struct TVTPage* pPage);
static void NotifyDecoderEvent(INT32U uEvent, INT32U uPageCode);
static void FreePageStore(void);
static void CopyPageForDisplay(struct TVTPage* pBuffer, struct TVTPage* pPage);

INT32S VTInitDecoderData(INT32U pageIndex)
{
    INT8U retcode;
    INT16U i;
    m_NonVisiblePageList = NULL;
    for (i = 0; i < 800; i++)
        m_VisiblePageList[i] = NULL;

    m_CachingControl = DECODERCACHE_NORMAL;
    m_bHighGranularityCaching = FALSE;
    m_bSubstituteSpacesForError = FALSE;
    uTeletextMemSize = 0;
    uTtxMaxPageNum = 100;
    uTtxCurrPageIndex = pageIndex;
    m_CurrPageCode = pageIndex;
    pthread_mutex_init(&m_CommonHeaderMutex,0);
    pthread_mutex_init(&m_MagazineStateMutex,0);
    pthread_mutex_init(&m_PageStoreMutex,0);
    pthread_mutex_init(&m_ServiceDataStoreMutex,0);
    ResetDecoder();

    return AM_SUCCESS;

fail:

    pthread_mutex_destroy(&m_CommonHeaderMutex);

    pthread_mutex_destroy(&m_MagazineStateMutex);

    pthread_mutex_destroy(&m_PageStoreMutex);

    pthread_mutex_destroy(&m_ServiceDataStoreMutex);

    return AM_FAILURE;
}


void VTFreeDecoderData(void)
{
    INT8U retcode;

    //ResetDecoder();
    pthread_mutex_lock(&m_PageStoreMutex);
    FreePageStore();
    pthread_mutex_unlock(&m_PageStoreMutex);

    pthread_mutex_destroy(&m_CommonHeaderMutex);

    pthread_mutex_destroy(&m_MagazineStateMutex);

    pthread_mutex_destroy(&m_PageStoreMutex);

    pthread_mutex_destroy(&m_ServiceDataStoreMutex);

    return;
}

void ResetDecoder(void)
{
    int i;
    INT8U retcode;
    pthread_mutex_lock(&m_CommonHeaderMutex);
    {
        memset(m_CommonHeader, 0x20, 32);
    }
    pthread_mutex_unlock(&m_CommonHeaderMutex);
    pthread_mutex_lock(&m_MagazineStateMutex);
    {
        for (i = 0; i < 8; i++)
        {
            // Doing this is enough for zeroing the state
            m_MagazineState[i].bReceiving = FALSE;
            m_MagazineState[i].bNewData = FALSE;
        }
        m_LastMagazine = 0;
        m_LastPageCode = 0;
    }
    pthread_mutex_unlock(&m_MagazineStateMutex);

    pthread_mutex_lock(&m_PageStoreMutex);
    {
        ResetPageStore();
        m_ReceivedPages = 0;
    }
    pthread_mutex_unlock(&m_PageStoreMutex);

    VTTopTextReset();

    pthread_mutex_lock(&m_ServiceDataStoreMutex);
    m_BroadcastServiceData.InitialPage = 0;

    for (i = 0; i < 2; i++)
    {
        m_BroadcastServiceData.NetworkIDCode[i] = 0;
    }
    m_BroadcastServiceData.TimeOffset = 0;
    m_BroadcastServiceData.ModifiedJulianDate = 45000;
    m_BroadcastServiceData.UTCHours = 0;
    m_BroadcastServiceData.UTCMinutes = 0;
    m_BroadcastServiceData.UTCSeconds = 0;
    memset(m_BroadcastServiceData.StatusDisplay, 0x20, 20);
    pthread_mutex_unlock(&m_ServiceDataStoreMutex);

    m_bMagazineSerial = FALSE;
    bWaitingPage = FALSE;
    uWaitingPageCode = 0;

    return;
}


void VTDecodeLine(INT8U *data)
{
    INT8U 		n;
    BOOLEAN 	bError = FALSE;
    INT8U 		uMagazine;
    INT8U 		uPacketNumber;
    INT8U 		uDesignationCode = 0;
    INT8U 		uS1, uS2, uS3, uS4;
    INT8U 		uC7_14;
    static INT16U 		uPageHex;
    static INT16U 		uPageSubCode;
    INT16U 		uControlBits;
    INT32U 		uPageCode;
    INT8U 		uProcessMagazine = 0;
    INT32U 		uTTUpdate = 0;
    INT8U 		uLine;
    INT8U 		uLinkMagazine;
    INT8U 		uLinkControlByte;
    INT8U 		uInitialMagazine;
    INT32S 		sTimeOffset;
    INT32U 		uModifiedJulianDate;
    INT16U 		uNetworkIDCode;
    INT8U 		uHours;
    INT8U 		uMinutes;
    INT8U 		uSeconds;
    INT8U 		retcode;
    INT32U nTemp;
    int i;
    // Caution: Remember that these offsets are zero based while indexes
    // in the ETS Teletext specification (ETS 300 706) are one based.
    extern unsigned char invtab[];

    for (i = 0; i < 42; i++)
    {
        data[i] = invtab[data[i]];
    }

    uPacketNumber = UnhamTwo84_LSBF(data, &bError);

    if (bError != FALSE)
    {
        return;
    }
    uMagazine = (uPacketNumber & 0x07);
    uPacketNumber >>= 3;
    // A page header terminates the last page reception
    if (uPacketNumber == 0)
    {
        // Check if there is a finished page

        uProcessMagazine = m_bMagazineSerial ? m_LastMagazine : uMagazine;
        if (m_MagazineState[uProcessMagazine].bReceiving || m_MagazineState[uProcessMagazine].bNewData)
        {
            m_MagazineState[uProcessMagazine].bReceiving = FALSE;
            m_MagazineState[uProcessMagazine].bNewData = FALSE;
            VTCompleteMagazine(&m_MagazineState[uProcessMagazine]);
        }

    }

    // A packets 26 or higher uses a designation code
    if (uPacketNumber >= 26)
    {
        uDesignationCode = Unham84(data[2], &bError);

        if (bError != FALSE)
        {
            // We can't continue without this
            return;
        }
    }
    
    switch (uPacketNumber)
    {
        case 0: // page header

            // Initialize the magazine
            
            // Work out the page number
            uPageHex = UnhamTwo84_LSBF(data + 2, &bError);
            uPageHex |= (uMagazine == 0 ? 0x800 : uMagazine * 0x100);

            // Caution: Remember that these offsets are zero based while indexes
            // in the ETS Teletext specification (ETS 300 706) are one based.
            uS1 = Unham84(data[4], &bError);
            uS2 = Unham84(data[5], &bError);
            uS3 = Unham84(data[6], &bError);
            uS4 = Unham84(data[7], &bError);

            uC7_14 = UnhamTwo84_LSBF(data + 8, &bError);

            if (bError != FALSE)
            {
                //M_TELETEXT_DIAG(("Get The uC7_14 failure!\n"));
                return;
            }
            // Work out the page sub-code
            uPageSubCode = (uS1 | ((uS2 & 0x7) << 4) | (uS3 << 8) | ((uS4 & 0x3) << 12));

            if(m_LastPageCode && (m_LastMagazine == uMagazine) && (m_LastPageCode != MAKELONG(uPageHex, uPageSubCode)) && m_MagazineState[m_LastMagazine].bNewData){
                m_MagazineState[m_LastMagazine].bReceiving = FALSE;
                m_MagazineState[m_LastMagazine].bNewData = FALSE;
                VTCompleteMagazine(&m_MagazineState[m_LastMagazine]);
            }

            pthread_mutex_lock(&m_MagazineStateMutex);
            m_MagazineState[uMagazine].bReceiving = FALSE;
            m_MagazineState[uMagazine].bNewData = FALSE;
            pthread_mutex_unlock(&m_MagazineStateMutex);

            if (CheckTeletextPageInRange(uPageHex) != TRUE)
            {
                return;
            }

            //M_TELETEXT_DIAG(("start received The magzine[%d] and uPageHex[0x%x] subpagecode [0x%x]\n",uMagazine,uPageHex,uPageSubCode));

            // Get the page control bits
            uControlBits = (uMagazine | (uS2 & 0x8) | ((uS4 & 0xC) << 2) | (uC7_14 << 6));

            m_bMagazineSerial = (uControlBits & VTCONTROL_MAGSERIAL) != 0;

            // Update the rolling header if this header is usable
            //if ((uControlBits & VTCONTROL_SUPRESSHEADER) == 0 && (uControlBits & VTCONTROL_INTERRUPTED) == 0)
            if (((uControlBits & VTCONTROL_SUPRESSHEADER) == 0 && (uControlBits & VTCONTROL_INTERRUPTED) == 0) || (uControlBits & VTCONTROL_SUBTITLE|VTCONTROL_NEWSFLASH))
            {
                // Don't use non-visible page headers because
                // some on them are missing the clock.
                if (IsNonVisiblePage(uPageHex) == FALSE)
                {
                    if (CheckParity(data + 10, 32, FALSE) != FALSE)
                    {
                        pthread_mutex_lock(&m_CommonHeaderMutex);
                        memcpy(m_CommonHeader, data + 10, 32);
                        pthread_mutex_unlock(&m_CommonHeaderMutex);
                        m_CurrPageCode = AM_TT_GetCurrentPageCode();
                        if (uPageHex == LOWWORD(m_CurrPageCode))
                        {
                            NotifyDecoderEvent(DECODEREVENT_HEADERUPDATE, MAKELONG(uPageHex, uPageSubCode));
                        }
                    }
                }
            }

            // See if this is an invalid page
            if ((uPageHex & 0xFF) == 0xFF)
            {
                NotifyDecoderEvent(DECODEREVENT_HEADERUPDATE, 0xFF);
                return;
            }

            pthread_mutex_lock(&m_MagazineStateMutex);

            m_MagazineState[uMagazine].bReceiving = TRUE;
            m_MagazineState[uMagazine].bNewData = FALSE;
            m_MagazineState[uMagazine].wPageHex = uPageHex;
            m_MagazineState[uMagazine].wPageSubCode = uPageSubCode;
            m_MagazineState[uMagazine].wControlBits = uControlBits;
            m_MagazineState[uMagazine].uCharacterSubset = (uControlBits & VTCONTROL_CHARSUBSET) >> 11;

            memcpy(m_MagazineState[uMagazine].Header, data + 10, 32);
            memset(m_MagazineState[uMagazine].bLineReceived, 0, 25);
            m_MagazineState[uMagazine].LinkReceived = 0x00;

            m_LastMagazine = uMagazine;
            m_LastPageCode = MAKELONG(uPageHex, uPageSubCode);
            pthread_mutex_unlock(&m_MagazineStateMutex);

            break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            {
                pthread_mutex_lock(&m_MagazineStateMutex);
                //if (m_MagazineState[uMagazine].bReceiving != FALSE)
                {
                    uLine = uPacketNumber - 1;
                    
                    // do not override an existing line
                    if (m_MagazineState[uMagazine].bLineReceived[uLine] != FALSE)
                    {
                        // Assume that a new header was missed and close the magazine
                        m_MagazineState[uMagazine].bReceiving = FALSE;
                        if (m_MagazineState[uMagazine].bReceiving != FALSE)
                        {
                        	VTCompleteMagazine(&m_MagazineState[uProcessMagazine]);
                        }
                    }
                    else
                    {
                        memcpy(m_MagazineState[uMagazine].Line[uLine], data + 2, 40);
                        m_MagazineState[uMagazine].bLineReceived[uLine] = TRUE;
#if 0
                        uPageCode = m_MagazineState[uMagazine].dwPageCode;
                        if (IsTopTextPage(uPageCode))
                        {
                            uTTUpdate = DecodePageRow(uPageCode, uPacketNumber, m_MagazineState[uMagazine].Line[uLine]);
                        }
#endif
                    }

                    if (m_MagazineState[uMagazine].bReceiving == FALSE)
                    {
                    	m_MagazineState[uMagazine].bNewData = TRUE;
                    }
                }

                pthread_mutex_unlock(&m_MagazineStateMutex);
#if 0
                if (uTTUpdate != 0)
                {
                    if (m_bTopTextForComment != FALSE)
                    {
                        NotifyDecoderEvent(DECODEREVENT_COMMENTUPDATE, uTTUpdate);
                        AM_TRACE("DECODEREVENT_COMMENTUPDATE event send :%x \n", uTTUpdate);
                    }
                    else
                    {
                        NotifyDecoderEvent(DECODEREVENT_PAGEUPDATE, uTTUpdate);
                        AM_TRACE("DECODEREVENT_PAGEUPDATE1 event send :%x \n", uTTUpdate);
                    }
                }
#endif
            }
            break;

        case 26:
            break;

        case 27:
            pthread_mutex_lock(&m_MagazineStateMutex);
            //if (m_MagazineState[uMagazine].bReceiving != FALSE)
            {
                // Packet X/27/0 for FLOF Editorial Linking
                if (uDesignationCode == 0)
                {
                    //M_TELETEXT_DIAG(("VTDecodeLine received uPageHex:%x line:%d \n",uPageHex,uPacketNumber));
                    // 2004/05/10 atnak: do not override existing information
                    if (m_MagazineState[uMagazine].LinkReceived != 0x00)
                    {
                        // Assume that a new header was missed and close the magazine
                        m_MagazineState[uMagazine].bReceiving = FALSE;

                        if (m_MagazineState[uMagazine].bReceiving != FALSE)
                        {
                            VTCompleteMagazine(&m_MagazineState[uProcessMagazine]);
                        }
                    }
                    else
                    {
                        for (n = 0; n < 6; n++)
                        {
                            bError = FALSE;
                            uS1 = Unham84(data[6*n + 5], &bError);
                            uS2 = Unham84(data[6*n + 6], &bError);
                            uS3 = Unham84(data[6*n + 7], &bError);
                            uS4 = Unham84(data[6*n + 8], &bError);

                            uPageHex = UnhamTwo84_LSBF(data + 6 * n + 3, &bError);

                            if (bError != FALSE)
                            {
                                continue;
                            }

                            uLinkMagazine = uMagazine ^ (uS2 >> 3 | (uS4 & 0xC) >> 1);

                            uPageHex |= (uLinkMagazine == 0 ? 0x800 : uLinkMagazine * 0x100);
                            uPageSubCode = (uS1 | ((uS2 & 0x7) << 4));
                            uPageSubCode |= ((uS3 << 8) | ((uS4 & 0x3) << 12));

                            uPageCode = MAKELONG(uPageHex, uPageSubCode);

                            m_MagazineState[uMagazine].EditorialLink[n] = uPageCode;
                            m_MagazineState[uMagazine].LinkReceived |= (1 << n);
                        }

                        uLinkControlByte = Unham84(data[39], &bError);

                        if (bError == FALSE)
                        {
                            if ((uLinkControlByte & 0x08) != 0)
                            {
                                m_MagazineState[uMagazine].LinkReceived |= LINKRECV_SHOW24;
                            }
                            else
                            {
                                m_MagazineState[uMagazine].LinkReceived |= LINKRECV_HIDE24;
                            }
                        }
                    }
                }

                if (m_MagazineState[uMagazine].bReceiving == FALSE)
                {
      	            m_MagazineState[uMagazine].bNewData = TRUE;
                }
            }
            pthread_mutex_unlock(&m_MagazineStateMutex);

            break;

        case 28:
        case 29:
            if ((uDesignationCode == 0x00) || (uDesignationCode == 0x04))
            {
                nTemp = module_hamming_24_18(data + 3);

                if (nTemp != 0xfffff)
                {
                    m_MagazineState[uMagazine].uCharacterRegion = (nTemp >> 10) & 0x0f;
                }
            }
            break;

        case 30:
            pthread_mutex_lock(&m_ServiceDataStoreMutex);

            // 8/30/0,1 is Format 1 Broadcast Service Data
            // 8/30/2,3 is Format 2 Broadcast Service Data
            if (uMagazine == 0 && (uDesignationCode & 0xC) == 0x0)
            {
                //M_TELETEXT_DIAG(("VTDecodeLine received uPageHex:%x line:%d \n",uPageHex,uPacketNumber));
                // Caution: Remember that these offsets are zero based while indexes
                // in the ETS Teletext specification (ETS 300 706) are one based.
                uS1 = Unham84(data[5], &bError);
                uS2 = Unham84(data[6], &bError);
                uS3 = Unham84(data[7], &bError);
                uS4 = Unham84(data[8], &bError);

                uPageHex = UnhamTwo84_LSBF(data + 3, &bError);

                if (bError == FALSE)
                {
                    uInitialMagazine = (uS2 >> 3 | (uS4 & 0xC) >> 1);

                    uPageHex |= (uInitialMagazine == 0 ? 0x800 : uInitialMagazine * 0x100);
                    uPageSubCode = (uS1 | ((uS2 & 0x7) << 4));
                    uPageSubCode |= ((uS3 << 8) | ((uS4 & 0x3) << 12));

                    uPageCode = (INT32U)MAKELONG(uPageHex, uPageSubCode);

                    m_BroadcastServiceData.InitialPage = uPageCode;
                }

                // Format 1 and Format 2 are different from here
                if ((uDesignationCode & 0x2) == 0)
                {
                    // The Network Identification Code has bits in reverse
                    uNetworkIDCode = ReverseBits(data[10]) | ReverseBits(data[9]) << 8;
                    m_BroadcastServiceData.NetworkIDCode[1] = m_BroadcastServiceData.NetworkIDCode[0];
                    m_BroadcastServiceData.NetworkIDCode[0] = uNetworkIDCode;
                    /*
                    LOG(1, "m_BroadcastServiceData.NetworkIDCode[0] %x", m_BroadcastServiceData.NetworkIDCode[0]);
                    */

                    // Time offset from UTC in half hour units
                    sTimeOffset = (data[11] >> 1) * ((data[11] & 0x40) ? -1 : 1);
                    m_BroadcastServiceData.TimeOffset = sTimeOffset;

                    // Modified Julian Date 45000 is 31 January 1982
                    uModifiedJulianDate = 0;

                    uModifiedJulianDate += ((data[12] & 0x0F) >> 0) * 10000;
                    uModifiedJulianDate += ((data[13] & 0xF0) >> 4) * 1000;
                    uModifiedJulianDate += ((data[13] & 0x0F) >> 0) * 100;
                    uModifiedJulianDate += ((data[14] & 0xF0) >> 4) * 10;
                    uModifiedJulianDate += ((data[14] & 0x0F) >> 0) * 1;

                    m_BroadcastServiceData.ModifiedJulianDate = uModifiedJulianDate;

                    uHours   = (data[15] & 0x0F) + ((data[15] & 0xF0) >> 4) * 10;
                    uMinutes = (data[16] & 0x0F) + ((data[16] & 0xF0) >> 4) * 10;
                    uSeconds = (data[17] & 0x0F) + ((data[17] & 0xF0) >> 4) * 10;

                    m_BroadcastServiceData.UTCHours = uHours;
                    m_BroadcastServiceData.UTCMinutes = uMinutes;
                    m_BroadcastServiceData.UTCSeconds = uSeconds;
                }

                memcpy(m_BroadcastServiceData.StatusDisplay, data + 22, 20);
            }

            pthread_mutex_unlock(&m_ServiceDataStoreMutex);
            break;

        case 31:  // Independent data services
            break;

        default:
            break;
    }

    return;
}

void VTCompleteMagazine(TMagazineState* magazineState)
{
    INT8U i;
    INT16U wPageHex = magazineState->wPageHex;
    INT16U wPageSubCode = magazineState->wPageSubCode;
    INT16U wControlBits = magazineState->wControlBits;

    BOOLEAN bErasePage = (wControlBits & VTCONTROL_ERASEPAGE) != 0;
    BOOLEAN bPageUpdate = (wControlBits & VTCONTROL_UPDATE) != 0;
    BOOLEAN bFlushLines = FALSE;

    BOOLEAN bLinesAdded = FALSE;
    BOOLEAN bPageChanged = FALSE;

    magazineState->dwPageCode = MAKELONG(wPageHex, wPageSubCode);
    struct TVTPage *pPage = GetPageStore(magazineState->dwPageCode, TRUE);
    if (pPage == NULL)
        return;

    // Erase the storage if this is the first receive
    if (pPage->bReceived == FALSE)
    {
        bErasePage = TRUE;
    }

    // Erase the page if inhibit display is set
    if ((wControlBits & VTCONTROL_INHIBITDISP) != 0)
    {
        bErasePage = TRUE;
    }

    // Make sure old lines aren't retained if erase page
    if (bErasePage != FALSE)
    {
        bPageUpdate = TRUE;
    }

    if (bPageUpdate != FALSE)
    {
        bFlushLines = TRUE;
    }

    // Copy the header if it's not suppressed
    if ((wControlBits & VTCONTROL_SUPRESSHEADER) != 0)
    {
        if ((pPage->LineState[0] & CACHESTATE_HASDATA) != 0)
        {
            memset(&pPage->Frame[0][8], 32, 0x20);//X/0
            pPage->LineState[0] = CACHESTATE_UPDATED;
            bPageChanged = TRUE;
        }
    }
    else
    {
        // Only copy the header if it is an update or
        // if the previous reception had an error
        if (bFlushLines != FALSE ||
            (pPage->LineState[0] & CACHESTATE_HASDATA) == 0 ||
            (pPage->LineState[0] & CACHESTATE_HASERROR) != 0)
        {
            pPage->LineState[0] = CACHESTATE_HASDATA | CACHESTATE_UPDATED;

            if (CheckParity(magazineState->Header, 32, FALSE) == FALSE)
            {
                pPage->LineState[0] |= CACHESTATE_HASERROR;
            }

            memcpy(&pPage->Frame[0][8], magazineState->Header, 32);
            bPageChanged = TRUE;
        }
        else if (m_CachingControl == DECODERCACHE_SECONDCHANCE)
        {
            // If second chance error correction is on, copy the
            // line regardless, as long as the new line is error
            // free.
            if (CheckParity(magazineState->Header, 32, FALSE) != FALSE)
            {
                pPage->LineState[0] = CACHESTATE_HASDATA | CACHESTATE_UPDATED;
                memcpy(&pPage->Frame[0][8], magazineState->Header, 32);
                bPageChanged = TRUE;
            }
        }
    }

    // Some broadcasters may not explicitly set update for
    // subtitles but it needs to be set for subtitles to change
    if ((wControlBits & VTCONTROL_SUBTITLE) != 0)
    {
        bFlushLines = TRUE;
    }

    if (m_CachingControl == DECODERCACHE_ALWAYSUPDATE)
    {
        bFlushLines = TRUE;
    }

    // Copy the display lines
    for (i = 1; i < 26; i++)
    {
        if (magazineState->bLineReceived[i-1] != FALSE &&
            (wControlBits & VTCONTROL_INHIBITDISP) == 0)
        {
            // Only copy the line if it is an update or
            // if the previous reception had an error
            if (bFlushLines != FALSE ||
                (pPage->LineState[i] & CACHESTATE_HASDATA) == 0 ||
                (pPage->LineState[i] & CACHESTATE_HASERROR) != 0)
            {
                pPage->LineState[i] = CACHESTATE_HASDATA | CACHESTATE_UPDATED;

                if (CheckParity(magazineState->Line[i-1], 40, FALSE) == FALSE)
                {
                    pPage->LineState[i] |= CACHESTATE_HASERROR;
                }

                memcpy(pPage->Frame[i], magazineState->Line[i-1], 40);
                bLinesAdded = TRUE;

            }
            else if (m_CachingControl == DECODERCACHE_SECONDCHANCE)
            {
                // If second chance error correction is on, copy the
                // line regardless, as long as the new line is error
                // free.

                if (CheckParity(magazineState->Line[i-1], 40, FALSE) != FALSE)
                {
                    pPage->LineState[i] = CACHESTATE_HASDATA | CACHESTATE_UPDATED;
                    memcpy(pPage->Frame[i], magazineState->Line[i-1], 40);
                    bLinesAdded = TRUE;
                }
            }
        }
        else if (bErasePage != FALSE)
        {
            if ((pPage->LineState[i] & CACHESTATE_HASDATA) != 0)
            {
                memset(pPage->Frame[i], 0x20, 40);
                pPage->LineState[i] = CACHESTATE_UPDATED;
                bPageChanged = TRUE;
            }
        }
    }

    // Copy packet X/27/0 (FLOF) stuff
    for (i = 0; i < 6; i++)
    {
        if (magazineState->LinkReceived & (1 << i))
        {
            pPage->EditorialLink[i] = magazineState->EditorialLink[i];
        }
    }

    if (magazineState->LinkReceived & LINKRECV_SHOW24)
    {
        pPage->bShowRow24 = TRUE;
    }
    else if (magazineState->LinkReceived & LINKRECV_HIDE24)
    {
        pPage->bShowRow24 = FALSE;
    }


    // Mark all lines as updated if the subtitle or newsflash status changed
    if ((wControlBits & (VTCONTROL_NEWSFLASH | VTCONTROL_SUBTITLE)) !=
        (pPage->wControlBits & (VTCONTROL_NEWSFLASH | VTCONTROL_SUBTITLE)))
    {
        for (i = 0; i < 25; i++)
        {
            pPage->LineState[i] |= CACHESTATE_UPDATED;
        }
    }

    // Copy the page control bits
    pPage->wControlBits = wControlBits;
    pPage->uCharacterSubset = magazineState->uCharacterSubset;
    pPage->uCharacterRegion = magazineState->uCharacterRegion;


    // Update the cache count
    if (pPage->bReceived == FALSE)
    {
        // There's no point keeping the page
        // if it came with no display lines.
        if (bLinesAdded == FALSE)
        {

            //M_TELETEXT_DIAG(("pPage Received failure!\n"));
            return;
        }

        pPage->bReceived = TRUE;
        m_ReceivedPages++;
    }

#if 0
    {
        for (i = 0; i < 26; i++)
        {
            M_TELETEXT_DIAG(("line[%d] state is %d\n", i, pPage->LineState[i]));
        }
    }
#endif

    //M_TELETEXT_DIAG(("received page [0x%x] \n",wPageHex));

    m_CurrPageCode = AM_TT_GetCurrentPageCode();
    if (bLinesAdded != FALSE || bPageChanged != FALSE)
    {
        if (LOWWORD(m_CurrPageCode) == LOWWORD(magazineState->dwPageCode))
        {
            NotifyDecoderEvent(DECODEREVENT_PAGEUPDATE, magazineState->dwPageCode);
        }
    }
    else
    {
        if (LOWWORD(m_CurrPageCode) == LOWWORD(magazineState->dwPageCode))
        {
            NotifyDecoderEvent(DECODEREVENT_PAGEREFRESH, magazineState->dwPageCode);
        }
    }

    if (bWaitingPage == TRUE)
    {
        if (LOWWORD(magazineState->dwPageCode) == uWaitingPageCode)
        {
            NotifyDecoderEvent(DECODEREVENT_GETWAITINGPAGE, magazineState->dwPageCode);
            bWaitingPage = FALSE;
        }

    }

    return;
}

static void InitializePage(struct TVTPage* pPage)
{
    int i;

    pPage->dwPageCode   = MAKELONG(0x1FF, 0x3F7F);
    pPage->wControlBits = 0x0000;
    pPage->uCharacterSubset = 0;
    pPage->uCharacterRegion = 0;

    memset(pPage->LineState, CACHESTATE_HASDATA | CACHESTATE_HASERROR, 26);

    for (i = 0; i < 6; i++)
    {
        pPage->EditorialLink[i] = 0;
    }

    // The first eight bytes of the header never changes
    memset(&pPage->Frame[0][0], 0x20, 8);

    pPage->bReceived    = FALSE;
    pPage->bShowRow24   = FALSE;
    // pNextPage must not be set NULL here

    return;
}

void ResetPageStore(void)
{
    INT16U i = 0;
    struct TVTPage* pPage = NULL;

    for (pPage = m_NonVisiblePageList; pPage != NULL; pPage = pPage->pNextPage)
    {
        pPage->bReceived = FALSE;
        pPage->bBufferReserved = FALSE;
    }

    for (i = 0; i < 800; i++)
    {
        pPage = m_VisiblePageList[i];
        for ( ; pPage != NULL; pPage = pPage->pNextPage)
        {
            pPage->bReceived = FALSE;
            pPage->bBufferReserved = FALSE;
        }
    }

    return;
}


void FreePageStore(void)
{
    struct TVTPage* pPage = NULL;
    int i;

    while ((pPage = m_NonVisiblePageList) != NULL)
    {
        m_NonVisiblePageList = pPage->pNextPage;
        free(pPage);
    }

    for (i = 0; i < 800; i++)
    {
        while ((pPage = m_VisiblePageList[i]) != NULL)
        {
            m_VisiblePageList[i] = pPage->pNextPage;
            free(pPage);
        }
    }

    return;
}

void FreeNoNeedPageStore(INT16U uPageIndex)
{
    struct TVTPage* pPage = NULL;
    INT16U uCurrPageDec;
    INT16U uMaxPageNum;
    INT8U retcode;
    INT16U i;

    uCurrPageDec = PageHex2ArrayIndex(uPageIndex);
    if (uCurrPageDec == 0xFFFF)
        return;

    uMaxPageNum = uTtxMaxPageNum / 2;

    pthread_mutex_lock(&m_PageStoreMutex);

    if (((uCurrPageDec - uMaxPageNum) > 0) && ((uCurrPageDec + uMaxPageNum) < 800))
    {
        for (i = 0; i <= (uCurrPageDec - uMaxPageNum); i++)
        {
            while ((pPage = m_VisiblePageList[i]) != NULL)
            {
                m_VisiblePageList[i] = pPage->pNextPage;
                free(pPage);
                //uTeletextMemSize -= sizeof(struct TVTPage);
            }
        }
        for (i = (uCurrPageDec + uMaxPageNum); i < 800; i++)
        {
            while ((pPage = m_VisiblePageList[i]) != NULL)
            {
                m_VisiblePageList[i] = pPage->pNextPage;
                free(pPage);
                //uTeletextMemSize -= sizeof(struct TVTPage);
            }
        }
    }
    else if ((uCurrPageDec - uMaxPageNum) < 0)
    {
        for (i = uCurrPageDec + uMaxPageNum; i <= 800 - (uMaxPageNum - uCurrPageDec); i++)
        {
            while ((pPage = m_VisiblePageList[i]) != NULL)
            {
                m_VisiblePageList[i] = pPage->pNextPage;
                free(pPage);
                //uTeletextMemSize -= sizeof(struct TVTPage);
            }
        }
    }
    else if ((uCurrPageDec + uMaxPageNum) >= 800)
    {
        for (i = uMaxPageNum - (800 - uCurrPageDec); i <= uCurrPageDec - uMaxPageNum; i++)
        {
            while ((pPage = m_VisiblePageList[i]) != NULL)
            {
                m_VisiblePageList[i] = pPage->pNextPage;
                free(pPage);
                //uTeletextMemSize -= sizeof(struct TVTPage);
            }
        }
    }

    pthread_mutex_unlock(&m_PageStoreMutex);
    return;
}

struct TVTPage* GetPageStore(INT32U dwPageCode, BOOLEAN bUpdate)
{
    INT16U nCount = 0;
    INT16U wPageIndex;
    INT16U wPageHex = LOWWORD(dwPageCode);

    struct TVTPage* pPage = NULL;
    struct TVTPage** hPage = NULL;
    struct TVTPage** hList = NULL;

    if (IsNonVisiblePage(wPageHex))
    {
        return 	NULL;
    }
    else
    {
        wPageIndex = PageHex2ArrayIndex(wPageHex);

        if (wPageIndex == 0xFFFF)
        {
            return NULL;
        }

        hList = &m_VisiblePageList[wPageIndex];
    }

    // Look for an available page buffer
    for (hPage = hList; *hPage != NULL; hPage = &(*hPage)->pNextPage)
    {
        if ((*hPage)->bBufferReserved == FALSE)
        {
            break;
        }
        else if ((*hPage)->dwPageCode == dwPageCode)
        {
            if (bUpdate != FALSE)
            {
                // Move the element to the front
                if (hPage != hList)
                {
                    pPage = *hPage;
                    *hPage = pPage->pNextPage;
                    pPage->pNextPage = *hList;
                    *hList = pPage;
                }
                return *hList;
            }

            return *hPage;
        }
        else if (++nCount >= MAX_PAGELIST)
        {
            break;
        }
    }

    if (bUpdate == FALSE)
    {
        return NULL;
    }

    // Create a new buffer if there isn't a spare
    if (*hPage == NULL)
    {
        *hPage = (struct TVTPage*)malloc(sizeof(struct TVTPage));
        if (*hPage == NULL)
        {
            printf(("malloc memory failure!\n"));
            return NULL;
        }
        //uTeletextMemSize += sizeof(struct TVTPage);
        (*hPage)->pNextPage = NULL;
        (*hPage)->bReceived = FALSE;
    }

    pPage = *hPage;

    // Move the new buffer to the front
    if (hPage != hList)
    {
        *hPage = pPage->pNextPage;
        pPage->pNextPage = *hList;
        *hList = pPage;
    }

    pPage->bBufferReserved = TRUE;

    InitializePage(pPage);
    pPage->dwPageCode = dwPageCode;

    return pPage;
}

INT16U PageHex2ArrayIndex(INT16U wPageHex)
{
    INT16U wArrayIndex;

    if ((wPageHex & 0xFF00) < 0x0100 ||
        (wPageHex & 0xFF00) > 0x0800 ||
        (wPageHex & 0x00F0) > 0x0090 ||
        (wPageHex & 0x000F) > 0x0009)
    {
        return 0xFFFF;
    }


    wArrayIndex = (((wPageHex & 0xF00) >> 8) * 100) - 100;
    wArrayIndex += ((wPageHex & 0x0F0) >> 4) * 10;
    wArrayIndex += ((wPageHex & 0x00F));

    return wArrayIndex;
}

INT32U GetProcessingPageCode(void)
{
    INT32U 		dwPageCode = 0;
    INT8U 		retcode;

    pthread_mutex_lock(&m_MagazineStateMutex);
    if (m_MagazineState[m_LastMagazine].bReceiving != FALSE)
    {
        dwPageCode = m_MagazineState[m_LastMagazine].dwPageCode;
    }
    pthread_mutex_unlock(&m_MagazineStateMutex);

    return dwPageCode;
}


INT32U GetReceivedPagesCount(void)
{
    return m_ReceivedPages;
}

INT16U GetVisiblePageNumbers(INT16U* lpNumberList, INT16U nListSize)
{
    INT16U 		i;
    INT16U 		nPagesCount = 0;
    struct TVTPage* 	pPage = NULL;
    INT8U 		retcode;

    pthread_mutex_lock(&m_PageStoreMutex);

    for (i = 0; i < 800 && nPagesCount < nListSize; i++)
    {
        pPage = FindReceivedPage(m_VisiblePageList[i]);
        if (pPage != NULL)
        {
            lpNumberList[nPagesCount] = LOWWORD(pPage->dwPageCode);
            nPagesCount++;
        }
    }

    pthread_mutex_unlock(&m_PageStoreMutex);

    return nPagesCount;
}


INT16U GetNonVisiblePageNumbers(INT16U* lpNumberList, INT16U nListSize)
{
    INT16U 		i;
    INT16U 		nPagesCount = 0;
    struct TVTPage* 	pPage = NULL;
    INT8U 		retcode;

    pthread_mutex_lock(&m_PageStoreMutex);

    for (pPage = FindReceivedPage(m_NonVisiblePageList);
         pPage != NULL && nPagesCount < nListSize;
         pPage = FindReceivedPage(pPage->pNextPage))
    {
        for (i = 0; i < nPagesCount; i++)
        {
            if (lpNumberList[i] == LOWWORD(pPage->dwPageCode))
            {
                break;
            }
        }

        if (i == nPagesCount)
        {
            lpNumberList[nPagesCount] = LOWWORD(pPage->dwPageCode);
            nPagesCount++;
        }
    }

    pthread_mutex_unlock(&m_PageStoreMutex);

    return nPagesCount;
}


void GetDisplayHeader(struct TVTPage* pBuffer, BOOLEAN bClockOnly)
{
    INT8U 		retcode;

    if (bClockOnly != FALSE)
    {
        pthread_mutex_lock(&m_CommonHeaderMutex);
        memcpy(&pBuffer->Frame[0][32], &m_CommonHeader[24], 8);
        CheckParity(&pBuffer->Frame[0][32], 8, TRUE);
        pthread_mutex_unlock(&m_CommonHeaderMutex);
    }
    else
    {
        pthread_mutex_lock(&m_CommonHeaderMutex);
        memcpy(&pBuffer->Frame[0][8], m_CommonHeader, 32);
        CheckParity(&pBuffer->Frame[0][8], 32, TRUE);
        pthread_mutex_unlock(&m_CommonHeaderMutex);
    }
    pBuffer->LineState[0] |= CACHESTATE_HASDATA | CACHESTATE_UPDATED;

    return;
}


INT32U GetDisplayPage(INT32U dwPageCode, struct TVTPage* pBuffer)
{
    INT16U 		wPageHex = LOWWORD(dwPageCode);
    struct TVTPage** 	hPageList = NULL;
    struct TVTPage* 	pPage = NULL;
    INT16U 		wPageIndex;
    INT8U 		retcode;

    if (IsNonVisiblePage(wPageHex))
    {
        if ((wPageHex & 0xFF) == 0xFF)
        {
            return 0;
        }

        hPageList = &m_NonVisiblePageList;
    }
    else
    {
        wPageIndex = PageHex2ArrayIndex(wPageHex);

        if (wPageIndex == 0xFFFF)
        {
            return 0;
        }

        hPageList = &m_VisiblePageList[wPageIndex];
    }

    pthread_mutex_lock(&m_PageStoreMutex);

    if (HIGHWORD(dwPageCode) >= 0x3F7F)
    {
        pPage = FindReceivedPage(*hPageList);

        while (pPage != NULL)
        {
            if (LOWWORD(pPage->dwPageCode) == wPageHex)
            {
                break;
            }
            pPage = FindReceivedPage(pPage->pNextPage);
        }
    }
    else
    {
        pPage = FindSubPage(*hPageList, dwPageCode);
    }

    if (pPage != NULL)
    {
        CopyPageForDisplay(pBuffer, pPage);
        UnsetUpdatedStates(pPage);
    }

    pthread_mutex_unlock(&m_PageStoreMutex);

    return pPage != NULL ? pPage->dwPageCode : 0;
}

INT32U GetNextDisplayPage(INT32U dwPageCode, struct TVTPage* pBuffer, BOOLEAN bReverse)
{
    INT16U 		wPageHex = LOWWORD(dwPageCode);
    INT16U 		wPageIndex = PageHex2ArrayIndex(wPageHex);
    struct TVTPage* 	pPage = NULL;
    INT32U 		dwNextPageCode = 0;
    INT32S 		delta;
    INT32S 		i;
    INT8U 		retcode;

    pthread_mutex_lock(&m_PageStoreMutex);
    if (wPageIndex == 0xFFFF)
    {
        // Find the closest visible page
        if (IsNonVisiblePage(wPageHex))
        {
            wPageIndex = ((wPageHex & 0xF00) >> 8) * 100;
        }
        else
        {
            wPageIndex = 800;
        }

        if (bReverse != FALSE)
        {
            wPageIndex--;
        }

        wPageIndex %= 800;

        pPage = FindReceivedPage(m_VisiblePageList[wPageIndex]);
    }
    if (pPage == NULL)
    {
        delta = (bReverse==TRUE) ? -1 : 1;

        // Loop around the available pages to find the next or previous page
        for (i = 800 + wPageIndex + delta; (i % 800) != wPageIndex; i += delta)
        {
            pPage = FindReceivedPage(m_VisiblePageList[i % 800]);
            if (pPage != NULL)
            {
                break;
            }
        }
    }
    if (pPage != NULL)
    {
        CopyPageForDisplay(pBuffer, pPage);
        UnsetUpdatedStates(pPage);

        dwNextPageCode = pPage->dwPageCode;
    }
    pthread_mutex_unlock(&m_PageStoreMutex);

    return dwNextPageCode;
}


INT32U GetNextDisplaySubPage(INT32U dwPageCode, struct TVTPage* pBuffer, BOOLEAN bReverse)
{
    INT16U 		wPageHex = LOWWORD(dwPageCode);
    INT16U 		wPageIndex = PageHex2ArrayIndex(wPageHex);
    struct TVTPage* 	pSubPageList = NULL;
    INT32U 		dwNextPageCode = 0;
    struct TVTPage* 	pPage = NULL;
    INT8U 		retcode;

    if (wPageIndex == 0xFFFF)
    {
        if (!IsNonVisiblePage(wPageHex))
        {
            return 0;
        }

        pSubPageList = m_NonVisiblePageList;
    }
    else
    {
        pSubPageList = m_VisiblePageList[wPageIndex];
    }

    pthread_mutex_lock(&m_PageStoreMutex);
    if (pSubPageList != NULL)
    {
        pPage = FindNextSubPage(pSubPageList, dwPageCode, bReverse);

        if (pPage == NULL)
        {
            int delta = (bReverse==TRUE) ? -1 : 1;
            int i;
            
            // Loop around the available pages to find the next or previous page
            for (i = 800 + wPageIndex + delta; (i % 800) != wPageIndex; i += delta)
            {
	        pPage = FindReceivedPage(m_VisiblePageList[i % 800]);
                if (pPage != NULL)
                {
                    wPageHex = LOWWORD(pPage->dwPageCode);
                    wPageIndex = PageHex2ArrayIndex(wPageHex);
                    pSubPageList = m_VisiblePageList[i % 800];
                    break;
                }
            }

            if (pPage != NULL)
            {
                struct TVTPage* pSub = NULL;

            	pSub = FindNextSubPage(pSubPageList, MAKELONG(wPageHex, bReverse == FALSE ? 0 : 0xFFFF), bReverse);
            	if(pSub != NULL) {
            	    pPage = pSub;
            	}
            }
            
            if (pPage != NULL)
            {
                if (pPage->dwPageCode == dwPageCode)
                {
                    pPage = NULL;
                }
            }

#if 0
            if (bReverse == FALSE)
            {
                pPage = FindSubPage(pSubPageList, MAKELONG(wPageHex, 0));
            }

            if (pPage == NULL)
            {
                pPage = FindNextSubPage(pSubPageList, MAKELONG(wPageHex, bReverse == FALSE ? 0 : 0xFFFF), bReverse);
            }

            if (pPage != NULL)
            {
                if (pPage->dwPageCode == dwPageCode)
                {
                    pPage = NULL;
                }
            }
#endif
        }
    }

    if (pPage != NULL)
    {
        if (pBuffer != NULL)
        {
            CopyPageForDisplay(pBuffer, pPage);
            UnsetUpdatedStates(pPage);
        }
        dwNextPageCode = pPage->dwPageCode;
    }
    pthread_mutex_unlock(&m_PageStoreMutex);

    return dwNextPageCode;
}


BOOLEAN GetDisplayComment(INT32U dwPageCode, struct TVTPage* pBuffer)
{
    m_bTopTextForComment = TRUE;

    if (GetTopTextDetails(dwPageCode, pBuffer, TRUE))
    {
        return TRUE;
    }

    return FALSE;
}



void CreateTestDisplayPage(struct TVTPage* pBuffer)
{
    pBuffer->dwPageCode = MAKELONG(0x900, 0x0000);
    pBuffer->wControlBits = VTCONTROL_INTERRUPTED;

    INT8U nCol;
    INT8U nRow;

    for (nRow = 0; nRow < 26; nRow++)
    {
        if (nRow == 0)
        {
            memset(pBuffer->Frame[nRow], 0x20, 8);
            memcpy(&pBuffer->Frame[nRow][8], " DScaler Charset Test  \x03", 24);
            memset(&pBuffer->Frame[nRow][32], 0x20, 8);
        }
        else if (nRow == 2)
        {
            memcpy(pBuffer->Frame[nRow], "\x17 \x1es\x13\x10\x16\x10\x1f\x18\x04\x0d\x1d\x03""ENGINEERING\x1a\x12\x1c\x0c\x1es\x15\x0e\x11\x10\x14\x10\x07", 38);

            pBuffer->Frame[nRow][38] = '0' + (nRow / 10);
            pBuffer->Frame[nRow][39] = '0' + (nRow % 10);
        }
        else if (nRow == 5)
        {
            memcpy(pBuffer->Frame[nRow], "\x14\x1a\x1es\x11\x19\x15\x00\x15\x01\x01\x15\x0d\x1d\x02Test Page  \x1c\x0c\x1e\x12s\x16\x18\x13\x00\x17\x18\x01", 38);

            pBuffer->Frame[nRow][38] = '0' + (nRow / 10);
            pBuffer->Frame[nRow][39] = '0' + (nRow % 10);
        }
        else if (nRow == 7)
        {
            memcpy(pBuffer->Frame[nRow], "\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x17\x1e,\x13\x10\x16\x10\x12\x1e,\x15\x10\x11\x10\x14\x10\x1f\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00", 38);

            pBuffer->Frame[nRow][38] = '0' + (nRow / 10);
            pBuffer->Frame[nRow][39] = '0' + (nRow % 10);
        }
        else if (nRow <= 16)
        {
            for (nCol = 0; nCol < 38; nCol++)
            {
                if (nRow & 0x1)
                {
                    pBuffer->Frame[nRow][nCol] = (nCol & 0x1) ? 0x00 : 0x01;
                }
                else
                {
                    pBuffer->Frame[nRow][nCol] = (nCol & 0x1) ? 0x7f : 0x7e;
                }
            }

            pBuffer->Frame[nRow][38] = '0' + (nRow / 10);
            pBuffer->Frame[nRow][39] = '0' + (nRow % 10);
        }
        else if (nRow == 17)
        {
            memcpy(pBuffer->Frame[nRow], "White\x03""Yellow\x06""Cyan\x02""Green\x05""Magenta\x01""Red\x04""Blue", 40);
        }
        else if (nRow == 18)
        {
            for (nCol = 0; nCol < 40; nCol++)
            {
                if ((nCol % 5) == 0)
                {
                    switch (nCol)
                    {
                        case 0:
                            pBuffer->Frame[nRow][nCol] = 0x17;
                            break;
                        case 5:
                            pBuffer->Frame[nRow][nCol] = 0x13;
                            break;
                        case 10:
                            pBuffer->Frame[nRow][nCol] = 0x16;
                            break;
                        case 15:
                            pBuffer->Frame[nRow][nCol] = 0x12;
                            break;
                        case 20:
                            pBuffer->Frame[nRow][nCol] = 0x19;
                            break;
                        case 25:
                            pBuffer->Frame[nRow][nCol] = 0x15;
                            break;
                        case 30:
                            pBuffer->Frame[nRow][nCol] = 0x11;
                            break;
                        case 35:
                            pBuffer->Frame[nRow][nCol] = 0x14;
                            break;
                    }
                }
                else if (nCol == 1)
                {
                    pBuffer->Frame[nRow][nCol] = 0x1a;
                }
                else
                {
                    pBuffer->Frame[nRow][nCol] = 0x1F + nCol - (nCol / 5);
                }
            }
        }
        else if (nRow >= 19 && nRow <= 21)
        {
            for (nCol = 0; nCol < 40; nCol++)
            {
                if ((nCol % 5) == 0)
                {
                    pBuffer->Frame[nRow][nCol] = 0x20;
                }
                else
                {
                    pBuffer->Frame[nRow][nCol] = (0x1F | ((nRow - 19) << 5)) + nCol - (nCol / 5);
                }
            }
        }
        else if (nRow == 22)
        {
            for (nCol = 0; nCol < 40; nCol++)
            {
                if ((nCol % 5) == 0)
                {
                    switch (nCol)
                    {
                        case 0:
                            pBuffer->Frame[nRow][nCol] = 0x14;
                            break;
                        case 5:
                            pBuffer->Frame[nRow][nCol] = 0x11;
                            break;
                        case 10:
                            pBuffer->Frame[nRow][nCol] = 0x15;
                            break;
                        case 15:
                            pBuffer->Frame[nRow][nCol] = 0x12;
                            break;
                        case 20:
                            pBuffer->Frame[nRow][nCol] = 0x1a;
                            break;
                        case 25:
                            pBuffer->Frame[nRow][nCol] = 0x16;
                            break;
                        case 30:
                            pBuffer->Frame[nRow][nCol] = 0x13;
                            break;
                        case 35:
                            pBuffer->Frame[nRow][nCol] = 0x17;
                            break;
                    }
                }
                else
                {
                    pBuffer->Frame[nRow][nCol] = 0x5F + nCol - (nCol / 5);
                }
            }
        }
        else if (nRow == 23)
        {
            memcpy(pBuffer->Frame[nRow], "\x03\x18""Conceal\x08""Flash\x03\x2a\x0b\x0b""Box\x09""Steady\x18""Gone\x0a\x0a?\x16^\x7f", 40);
        }
        else if (nRow == 24 || nRow == 25)
        {
            memset(pBuffer->Frame[nRow], 0x20, 40);
        }

        pBuffer->LineState[nRow] = CACHESTATE_HASDATA | CACHESTATE_UPDATED;
    }

    memset(pBuffer->EditorialLink, 0, sizeof(INT32U) * 6);

    pBuffer->bShowRow24 = FALSE;
    pBuffer->bBufferReserved = TRUE;
    pBuffer->bReceived = TRUE;
    pBuffer->pNextPage = NULL;

    return;
}


void GetStatusDisplay(INT8S* lpBuffer, INT32U nLength)
{
    INT8U 	retcode;

    if (nLength > 21)
    {
        nLength = 21;
    }

    lpBuffer[--nLength] = '\0';

    pthread_mutex_lock(&m_ServiceDataStoreMutex);
    memcpy(lpBuffer, m_BroadcastServiceData.StatusDisplay, nLength);
    pthread_mutex_unlock(&m_ServiceDataStoreMutex);

    CheckParity((INT8U*)lpBuffer, nLength, TRUE);

    while (nLength-- > 0 && lpBuffer[nLength] == 0x20)
    {
        lpBuffer[nLength] = '\0';
    }

    return;
}

static void CopyPageForDisplay(struct TVTPage* pBuffer, struct TVTPage* pPage)
{
    int i;
    int j;

    pBuffer->bShowRow24 = pPage->bShowRow24;
    pBuffer->dwPageCode = pPage->dwPageCode;
    pBuffer->wControlBits = pPage->wControlBits;
    pBuffer->uCharacterSubset = pPage->uCharacterSubset;
    pBuffer->uCharacterRegion = pPage->uCharacterRegion;

    for (i = 0; i < 26; i++)
    {
        for (j = 0; j < 40; j++)
        {
            if (CheckParity(&pPage->Frame[i][j], 1, FALSE) == FALSE)
            {
                if (pPage->Frame[i][j] < 0x20)
                {
                    pPage->Frame[i][j] = 0x20;
                }
                else if (m_bSubstituteSpacesForError != FALSE)
                {
                    pBuffer->Frame[i][j] = 0x20;
                }
                else
                {
                    pBuffer->Frame[i][j] = pPage->Frame[i][j] & 0x7F;
                }
            }
            else
            {
                pBuffer->Frame[i][j] = pPage->Frame[i][j] & 0x7F;
            }
        }
        pBuffer->LineState[i] = pPage->LineState[i];
    }

    memcpy(pBuffer->EditorialLink, pPage->EditorialLink, sizeof(INT32U) * 6);
#if 0
    if (pBuffer->bShowRow24 == FALSE)
    {
        m_bTopTextForComment = FALSE;
        GetTopTextDetails(pBuffer->dwPageCode, pBuffer, FALSE);
    }
#endif
    pBuffer->bBufferReserved = TRUE;
    pBuffer->bReceived = TRUE;
    pBuffer->pNextPage = NULL;

    return;
}

static struct TVTPage* FindReceivedPage(struct TVTPage* pPageList)
{
    if (pPageList == NULL || pPageList->bBufferReserved == FALSE)
    {
        return NULL;
    }
    if (pPageList->bReceived != FALSE)
    {
        return pPageList;
    }

    return FindReceivedPage(pPageList->pNextPage);
}

static struct TVTPage* FindSubPage(struct TVTPage* pPageList, INT32U dwPageCode)
{
    if (pPageList == NULL || pPageList->bBufferReserved == FALSE)
    {
        return NULL;
    }

    struct TVTPage* pPage = pPageList;

    while (pPage != NULL && pPage->bBufferReserved != FALSE)
    {
        if (pPage->bReceived != FALSE && pPage->dwPageCode == dwPageCode)
        {
            return pPage;
        }

        pPage = pPage->pNextPage;
    }

    return NULL;
}

static struct TVTPage* FindNextSubPage(struct TVTPage* pPageList, INT32U dwPageCode, BOOLEAN bReverse)
{
    if (pPageList == NULL || pPageList->bBufferReserved == FALSE)
    {
        return NULL;
    }

    struct TVTPage* pPage = pPageList;
    struct TVTPage* pNextPage = NULL;

    INT16U wPageSubCode = HIGHWORD(dwPageCode);

    while (pPage != NULL && pPage->bBufferReserved != FALSE)
    {
        if (pPage->bReceived != FALSE && LOWWORD(pPage->dwPageCode) == LOWWORD(dwPageCode))
        {
            if (bReverse == FALSE && HIGHWORD(pPage->dwPageCode) > wPageSubCode)
            {
                if (pNextPage == NULL || HIGHWORD(pPage->dwPageCode) < HIGHWORD(pNextPage->dwPageCode))
                {
                    pNextPage = pPage;
                }
            }
            else if (bReverse != FALSE && HIGHWORD(pPage->dwPageCode) < wPageSubCode)
            {
                if (pNextPage == NULL || HIGHWORD(pPage->dwPageCode) > HIGHWORD(pNextPage->dwPageCode))
                {
                    pNextPage = pPage;
                }
            }
        }

        pPage = pPage->pNextPage;
    }

    return pNextPage;
}

static void UnsetUpdatedStates(struct TVTPage* pPage)
{
    int i;

    for (i = 0; i < 26; i++)
    {
        pPage->LineState[i] &= ~CACHESTATE_UPDATED;
    }

    return;
}

void SetCachingControl(INT8U uCachingControl)
{
    m_CachingControl = uCachingControl;

    return;
}

void SetHighGranularityCaching(BOOLEAN bEnable)
{
    m_bHighGranularityCaching = bEnable;

    return;
}

void SetSubstituteSpacesForError(BOOLEAN bEnable)
{
    m_bSubstituteSpacesForError = bEnable;
}

void TeletextRegisterfunc(Teletext_RegFunc RegFunc)
{
    m_DecoderEventProc = RegFunc;

    return;
}

static void NotifyDecoderEvent(INT32U uEvent, INT32U uPageCode)
{
    if (m_DecoderEventProc != NULL)
    {
        m_DecoderEventProc(uEvent, uPageCode);
    }

    return;
}

void  SetTeletextMaxPageNum(INT16U uMaxPageNum)
{
    uTtxMaxPageNum = uMaxPageNum;

    return;
}

void SetTeletextCurrPageIndex(INT16U uPageIndex)
{
    INT16U PageIndex;

    PageIndex = PageHex2ArrayIndex(uPageIndex);
    if (uPageIndex == 0xFFFF)
    {
        return;
    }

    if (uTtxCurrPageIndex != uPageIndex)
    {
        uTtxCurrPageIndex = uPageIndex;
        FreeNoNeedPageStore(uPageIndex);
        if (bWaitingPage == TRUE)
            bWaitingPage = FALSE;
    }

    return;
}

BOOLEAN CheckTeletextPageInRange(INT16U uPageCode)
{
    INT16U uCurrPageDec;
    INT16U uCheckPageDec;
    INT32U uCurrPagecode;
    INT16U uMaxPageNum;
    INT16S nTemp;

    uCurrPagecode = AM_TT_GetCurrentPageCode();
    uCurrPageDec = PageHex2ArrayIndex(LOWWORD(uCurrPagecode));
    uCheckPageDec = PageHex2ArrayIndex(uPageCode);
    uMaxPageNum = uTtxMaxPageNum / 2;

    if (uCurrPageDec == 0xFFFF)
        return FALSE;

    if ((uCurrPageDec - uMaxPageNum > 0) && (uCurrPageDec + uMaxPageNum < 800))
    {
        nTemp = uCheckPageDec - uCurrPageDec;
        if (nTemp < 0)
            nTemp = -nTemp;

        if (nTemp < uMaxPageNum)
            return TRUE;
        else
            return FALSE;
    }
    else if (uCurrPageDec - uMaxPageNum < 0)
    {
        nTemp = uCheckPageDec - uCurrPageDec;
        if (nTemp >= 0)
        {
            if (nTemp < uMaxPageNum)
                return TRUE;
            else if ((799 - uCheckPageDec + uCurrPageDec) < uMaxPageNum)
                return TRUE;
            else
                return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else if (uCurrPageDec + uMaxPageNum > 799)
    {
        nTemp = uCheckPageDec - uCurrPageDec;
        if (nTemp <= 0)
        {
            if (nTemp + uMaxPageNum > 0)
                return TRUE;
            else if ((799 - uCurrPageDec + uCheckPageDec) < uMaxPageNum)
                return TRUE;
            else
                return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    return FALSE;
}

void SetWaitingPage(INT16U uPageCode, BOOLEAN bStatus)
{
    uWaitingPageCode = uPageCode;

    bWaitingPage = bStatus;

    return;
}

INT32S GetTeletextMemSize(void)
{
    return uTeletextMemSize;
}

