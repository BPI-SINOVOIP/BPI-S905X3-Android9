/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "includes.h"

#include "VTTopText.h"
#include "VTCommon.h"
#include <string.h>
#include <stdio.h>

#include <pthread.h>


static INT8U                   m_BTTable[800];         // Basic TOP
static INT8U                   m_MPTable[800];         // Multi-Page
static INT8U*                  m_AITable[800];         // Additional Information
static INT8U                   m_AITReceived[800];
static TExtraTopPage           m_ExtraTopPages[TOPMAX_EXTRAPAGES];
static INT8U                   m_ExtraPagesHead;
static INT16U                  m_WaitingPage[TOPWAIT_LASTONE];
static INT32U                  m_LastPageCode;
static INT8U                   m_LastBuffer[40];
static BOOLEAN 				bWaitMessage = FALSE;
static pthread_mutex_t  m_IntegrityMutex ;

static char* 	m_WaitMessage     = "Please wait";
static char* 	m_NoneMessage     = "Page not included";
static char* 	m_MultiMessage    = "Multi-page with %d sub-pages";
static char* 	m_SubtitleMessage = "Subtitle page";
static char* 	m_EmptyTitleText  = "    ????    ";


static INT8U 	GetExtraPageType(INT32U uPageCode);
static BOOLEAN 	IsWaitingBTTPage(INT16U nPage);
static BOOLEAN 	IsWaitingMPTPage(INT16U nPage);
static BOOLEAN 	IsWaitingAITPage(INT16U nPage);
static void 	ResetWaitingPages();
static BOOLEAN 	IsMultiPage(INT16U nPage);
static INT16S 	GetFirstInBlock(INT16U nPage, INT16U* pMissingPage);
static INT16S 	GetFirstInGroup(INT16U nPage, INT16U* pMissingPage);
static INT16S 	GetNextBlock(INT16U nPage, INT16U* pMissingPage);
static INT16S 	GetNextGroup(INT16U nPage, INT16U* pMissingPage);
static INT16S 	GetNextGroupInBlock(INT16U nPage, INT16U* pMissingPage);
static INT16S 	GetNextPage(INT16U nPage, INT16U* pMissingPage);
static INT16S 	GetNextPageInGroup(INT16U nPage, INT16U* pMissingPage);
static INT16S 	PageHex2Page(INT16U uPageHex);
static INT16U 	Page2PageHex(INT16S nPage);


INT32S VTInitTopTextData(void)
{
    INT8U retcode;
    int i = 0;

    
    pthread_mutex_init(&m_IntegrityMutex,0);

    pthread_mutex_unlock(&m_IntegrityMutex);

    for (i = 0; i < 800; i++)
    {
        m_AITable[i] = NULL;
    }

    VTTopTextReset();

    return AM_SUCCESS;

fail:
    pthread_mutex_destroy(&m_IntegrityMutex);

    return AM_FAILURE;
}


void VTFreeTopTextData(void)
{
    int i = 0;
    INT8U retcode;

    for (i = 0; i < 800; i++)
    {
        if (m_AITable[i] != NULL)
        {
            free(m_AITable[i]);
        }
    }

    pthread_mutex_destroy(&m_IntegrityMutex);

    return;
}


void VTTopTextReset(void)
{
    int i = 0;
    INT8U retcode;
    pthread_mutex_lock(&m_IntegrityMutex);

    m_ExtraPagesHead = 1;
    m_ExtraTopPages[0].uType = 0xFF;
    ResetWaitingPages();

    memset(m_BTTable, TOP_UNRECEIVED, sizeof(INT8U) * 800);
    memset(m_MPTable, 0x00, sizeof(INT8U) * 800);

    for (i = 0; i < 800; i++)
    {
        if (m_AITable[i] != NULL)
        {
            free(m_AITable[i]);
        }
        m_AITable[i] = NULL;
        m_AITReceived[i] = FALSE;
    }

    m_LastPageCode = 0;
    memset(m_LastBuffer, 0x00, sizeof(INT8U) * 40);

    pthread_mutex_unlock(&m_IntegrityMutex);
}


void ResetWaitingPages(void)
{
    int i;
    for (i = 0; i < TOPWAIT_LASTONE; i++)
    {
        m_WaitingPage[i] = 0xffff;
    }
}


BOOLEAN IsWaitingBTTPage(INT16U nPage)
{
    INT16U i;
    for (i = TOPWAIT_GREEN; i <= TOPWAIT_BLUE; i++)
    {
        if (m_WaitingPage[i] == nPage)
        {
            return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN IsWaitingAITPage(INT16U nPage)
{
    INT16U i;
    for (i = TOPWAIT_YELLOW_AIT; i <= TOPWAIT_BLUE_AIT; i++)
    {
        if (m_WaitingPage[i] == nPage)
        {
            return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN IsWaitingMPTPage(INT16U nPage)
{
    if (m_WaitingPage[TOPWAIT_MPT] == nPage)
    {
        return TRUE;
    }
    return FALSE;
}


INT32U DecodePageRow(INT32U dwPageCode, INT8U nRow, INT8U* pData)
{
    INT8U Type;

    // 0x1F0 is always the Basic TOP page
    if (LOWWORD(dwPageCode) == 0x1F0)
    {
        if (DecodeBTTPageRow(nRow, pData))
        {
            return m_LastPageCode;
        }
        return 0;
    }

    // Determine if this page is a TOP page
    Type = GetExtraPageType(dwPageCode);

    if (Type == 0xFF)
    {
        return 0;
    }

    switch (Type)
    {
        case TOPTYPE_MPT:
            if (DecodeMPTPageRow(nRow, pData))
            {
                return m_LastPageCode;
            }
            break;

        case TOPTYPE_AIT:
            if (DecodeAITPageRow(nRow, pData))
            {
                return m_LastPageCode;
            }
            break;
    }

    return 0;
}


BOOLEAN DecodeBTTPageRow(INT8U nRow, INT8U* pData)
{
    INT16U i;
    INT16U n;
    BOOLEAN    	bError = FALSE;
    BOOLEAN    	bLastPageUpdated = FALSE;
    INT16U 		nPage;
    INT8U 		uLevel;
    INT8U    	uType;
    INT8U    	uMagazine;
    INT16U    	uPageHex;
    INT16U    	uPageSubCode;
    INT8U    	s1, s2, s3, s4;
    INT32U 		uPageCode;
    INT8U 		retcode;

    pthread_mutex_lock(&m_IntegrityMutex);

    if (nRow >= 1 && nRow <= 20)
    {
        for (i = 0, nPage = ((nRow - 1) * 40); i < 40; i++, nPage++)
        {
            uLevel = Unham84(pData[i], &bError);

            if (bError != FALSE)
            {
                continue;
            }

            m_BTTable[nPage] = uLevel;
            // See if a missing page was received
            if (IsWaitingBTTPage(nPage))
            {
                ResetWaitingPages();
                bLastPageUpdated = TRUE;
            }
        }
    }
    else if (nRow >= 21 && nRow <= 22)
    {
        for (n = 0; n < 5; n++)
        {
            uMagazine = Unham84(pData[n*8], &bError);

            if (bError != FALSE || uMagazine == 0x0F)
            {
                break;
            }
            if (uMagazine >= 0x08)
            {
                continue;
            }

            uType = Unham84(pData[n*8 + 7], &bError);

            if (bError != FALSE || (uType != TOPTYPE_MPT && uType != TOPTYPE_AIT))
            {
                continue;
            }

            // These units are MSB first
            uPageHex = UnhamTwo84_MSBF(&pData[n*8 + 1], &bError);

            s1 = Unham84(pData[n*8 + 3], &bError);
            s2 = Unham84(pData[n*8 + 4], &bError);
            s3 = Unham84(pData[n*8 + 5], &bError);
            s4 = Unham84(pData[n*8 + 6], &bError);

            if (bError != FALSE)
            {
                continue;
            }

            uPageHex |= (uMagazine == 0 ? 0x800 : uMagazine * 0x100);
            uPageSubCode = (s4 | ((s3 & 0x7) << 4));
            uPageSubCode |= ((s2 << 7) | ((s1 & 0x3) << 11));

            uPageCode = MAKELONG(uPageHex, uPageSubCode);

            if (!IsTopTextPage(uPageCode))
            {
                // Multi-thread protection
                m_ExtraTopPages[m_ExtraPagesHead].uType      = 0xFF;

                m_ExtraTopPages[m_ExtraPagesHead].uPageCode = uPageCode;
                m_ExtraTopPages[m_ExtraPagesHead].uType      = uType;

                m_ExtraPagesHead = (m_ExtraPagesHead + 1) % TOPMAX_EXTRAPAGES;
            }
        }
    }

    pthread_mutex_unlock(&m_IntegrityMutex);

    return bLastPageUpdated;
}


BOOLEAN DecodeMPTPageRow(INT8U nRow, INT8U* pData)
{
    INT16U 		i;
    INT16U 		Page;
    INT8U 		uPages;
    BOOLEAN    	bError = FALSE;
    BOOLEAN    	bLastPageUpdated = FALSE;
    INT8U 		retcode;

    pthread_mutex_lock(&m_IntegrityMutex);

    if (nRow >= 1 && nRow <= 20)
    {
        for (i = 0, Page = (nRow - 1) * 40; i < 40; i++, Page++)
        {
            uPages = Unham84(pData[i], &bError);

            if (bError != FALSE)
            {
                continue;
            }

            m_MPTable[Page] = uPages;

            if (IsWaitingMPTPage(Page))
            {
                ResetWaitingPages();
                bLastPageUpdated = TRUE;
            }
        }
    }

    pthread_mutex_unlock(&m_IntegrityMutex);

    return bLastPageUpdated;
}


BOOLEAN DecodeAITPageRow(INT8U nRow, INT8U* pData)
{
    INT16U n;
    INT16U i;
    INT8U 		retcode;
    BOOLEAN    	bError = FALSE;
    BOOLEAN    	bLastPageUpdated = FALSE;
    BOOLEAN    	bTitleChanged;
    INT8U    	uChar;
    INT8U    	Magazine;
    INT16U    	wPageHex;
    INT16S		Page;


    pthread_mutex_lock(&m_IntegrityMutex);

    if (nRow >= 1 && nRow <= 22)
    {
        for (n = 0; n < 2; n++)
        {
            Magazine = Unham84(pData[n*20], &bError);

            if (bError != FALSE || Magazine == 0x0F)
            {
                break;
            }
            if (Magazine >= 0x08)
            {
                continue;
            }

            wPageHex = UnhamTwo84_MSBF(&pData[n*20 + 1], &bError);
            wPageHex |= (Magazine == 0 ? 0x800 : Magazine * 0x100);

            if (bError != FALSE || (Page = PageHex2Page(wPageHex)) == -1)
            {
                continue;
            }

            if (m_AITable[Page] == NULL)
            {
                m_AITable[Page] = (INT8U*)malloc(sizeof(INT8U) * 12);
            }

            bTitleChanged = FALSE;

            for (i = 0; i < 12; i++)
            {
                uChar = pData[n*20 + 8 + i];

                if (m_AITReceived[Page] == FALSE)
                {
                    m_AITable[Page][i] = (uChar & 0x7F) < 0x20 ? 0x20 : (uChar & 0x7F);
                    bTitleChanged = TRUE;
                }
                else
                {
                    if (CheckParity(&uChar, 1, TRUE))
                    {
                        if (uChar != m_AITable[Page][i])
                        {
                            m_AITable[Page][i] = uChar;
                            bTitleChanged = TRUE;
                        }
                    }
                }
            }

            m_AITReceived[Page] = TRUE;

            if (bTitleChanged != FALSE)
            {
                if (IsWaitingAITPage(Page))
                {
                    ResetWaitingPages();
                    bLastPageUpdated = TRUE;
                }
            }
        }
    }

    pthread_mutex_unlock(&m_IntegrityMutex);

    return bLastPageUpdated;
}


INT8U GetExtraPageType(INT32U uPageCode)
{
    INT8U i = m_ExtraPagesHead;

    while (1)
    {
        i = (TOPMAX_EXTRAPAGES + i - 1) % TOPMAX_EXTRAPAGES;

        if (m_ExtraTopPages[i].uType == 0xFF)
        {
            break;
        }
        if (m_ExtraTopPages[i].uPageCode == uPageCode)
        {
            break;
        }
        if (i == m_ExtraPagesHead)
        {
            return 0xFF;
        }
    }

    return m_ExtraTopPages[i].uType;
}


BOOLEAN IsTopTextPage(INT32U uPageCode)
{
    if (LOWWORD(uPageCode) == 0x1F0)
    {
        return TRUE;
    }

    return GetExtraPageType(uPageCode) != 0xFF;
}


BOOLEAN IsMultiPage(INT16U nPage)
{
    switch (m_BTTable[nPage])
    {
        case 0x03:
        case 0x05:
        case 0x07:
        case 0x0A:
        case 0x0B:
            return TRUE;
    }
    return FALSE;
}


INT16S GetFirstInBlock(INT16U nPage, INT16U* pMissingPage)
{
    for ( ; nPage > 0; nPage--)
    {
        if (m_BTTable[nPage] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = nPage;
            }
            return -1;
        }

        if (m_BTTable[nPage] >= TOPLEVEL_PROGRAM &&
            m_BTTable[nPage] < TOPLEVEL_GROUP)
        {
            return nPage;
        }
    }

    return 0;
}


INT16S GetFirstInGroup(INT16U nPage, INT16U* pMissingPage)
{
    for ( ; nPage > 0; nPage--)
    {
        if (m_BTTable[nPage] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = nPage;
            }
            return -1;
        }

        if (m_BTTable[nPage] >= TOPLEVEL_GROUP &&
            m_BTTable[nPage] < TOPLEVEL_NORMAL)
        {
            return nPage;
        }
    }

    return 0;
}


INT16S GetNextBlock(INT16U nPage, INT16U* pMissingPage)
{
    INT16U i = nPage;

    while ((i = (i + 1) % 800) != nPage)
    {
        if (m_BTTable[i] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = i;
            }
            return -1;
        }

        if (m_BTTable[i] >= TOPLEVEL_PROGRAM &&
            m_BTTable[i] < TOPLEVEL_GROUP)
        {
            break;
        }
    }

    return i;
}


INT16S GetNextGroup(INT16U nPage, INT16U* pMissingPage)
{
    INT16U i = nPage;

    while ((i = (i + 1) % 800) != nPage)
    {
        if (m_BTTable[i] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = i;
            }
            return -1;
        }

        if (m_BTTable[i] >= TOPLEVEL_PROGRAM &&
            m_BTTable[i] < TOPLEVEL_NORMAL)
        {
            break;
        }
    }

    return i;
}


INT16S GetNextGroupInBlock(INT16U nPage, INT16U* pMissingPage)
{
    INT16U i;

    for (i = nPage + 1; i < 800; i++)
    {
        if (m_BTTable[i] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = i;
            }
            return -1;
        }

        if (m_BTTable[i] >= TOPLEVEL_SUBTITLE &&
            m_BTTable[i] < TOPLEVEL_GROUP)
        {
            break;
        }

        if (m_BTTable[i] >= TOPLEVEL_GROUP &&
            m_BTTable[i] < TOPLEVEL_NORMAL)
        {
            return i;
        }
    }

    return GetFirstInBlock(nPage, pMissingPage);
}


INT16S GetNextPage(INT16U nPage, INT16U* pMissingPage)
{
    INT16U i = nPage;

    while ((i = (i + 1) % 800) != nPage)
    {
        if (m_BTTable[i] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = i;
            }
            return -1;
        }

        if (m_BTTable[i] >= TOPLEVEL_SUBTITLE &&
            m_BTTable[i] <= TOPLEVEL_LASTLEVEL)
        {
            break;
        }
    }

    return i;
}


INT16S GetNextPageInGroup(INT16U nPage, INT16U* pMissingPage)
{
    INT16U i;
    for (i = nPage + 1; i < 800; i++)
    {
        if (m_BTTable[i] & TOP_UNRECEIVED)
        {
            if (pMissingPage != NULL)
            {
                *pMissingPage = i;
            }
            return -1;
        }

        if (m_BTTable[i] >= TOPLEVEL_SUBTITLE &&
            m_BTTable[i] < TOPLEVEL_NORMAL)
        {
            break;
        }

        if (m_BTTable[i] >= TOPLEVEL_NORMAL &&
            m_BTTable[i] <= TOPLEVEL_LASTLEVEL)
        {
            return i;
        }
    }

    return GetFirstInGroup(nPage, pMissingPage);
}


INT16S PageHex2Page(INT16U uPageHex)
{
    if ((uPageHex & 0xF00) < 0x100 ||
        (uPageHex & 0xF00) > 0x800 ||
        (uPageHex & 0x0F0) > 0x090 ||
        (uPageHex & 0x00F) > 0x009)
    {
        return -1;
    }

    INT16U nPage;

    nPage = (((uPageHex & 0xF00) >> 8) * 100) - 100;
    nPage += ((uPageHex & 0x0F0) >> 4) * 10;
    nPage += ((uPageHex & 0x00F));

    return nPage;
}


INT16U Page2PageHex(INT16S nPage)
{
    if (nPage < 0 || nPage > 799)
    {
        return 0;
    }

    INT16U uPageHex = 0;

    uPageHex |= nPage / 100 * 0x100 + 0x100;
    uPageHex |= nPage % 100 / 10 * 0x10;
    uPageHex |= nPage % 10;

    return uPageHex;
}


BOOLEAN GetTopTextDetails(INT32U uPageCode, struct TVTPage* pBuffer, BOOLEAN bWaitMessage)
{
    INT16S	 nPage;
    INT16S	 nLinkPage;
    INT16U   nMissingPage;
    INT8U*   pLineInput;
    char*    pMessage;
    char     szBuffer[40];
    int 	 nLength;
    int 	 nStart;
    int i;


    if ((nPage = PageHex2Page(LOWWORD(uPageCode))) == -1)
    {
        return FALSE;
    }

    ResetWaitingPages();

    m_LastPageCode = uPageCode;

    // We can't do much more if we don't have a Basic TOP
    if (m_BTTable[nPage] == TOP_UNRECEIVED)
    {
        m_WaitingPage[TOPWAIT_GREEN] = nPage;

        // Multi-thread protection
        if (m_BTTable[nPage] == TOP_UNRECEIVED)
        {
            return FALSE;
        }
        else
        {
            m_WaitingPage[TOPWAIT_GREEN] = 0xFFFF;
        }
    }

    // Set the RED Flof key
    pBuffer->EditorialLink[VTFLOF_RED] = VTPAGE_PREVIOUS;

    // Get the GREEN Flof link
    nLinkPage = GetNextPage(nPage, &nMissingPage);

    while (nLinkPage == -1)
    {
        m_WaitingPage[TOPWAIT_GREEN] = nMissingPage;

        // Multi-thread protection
        if (m_BTTable[nMissingPage] == TOP_UNRECEIVED)
        {
            break;
        }

        m_WaitingPage[TOPWAIT_GREEN] = 0xFFFF;
        nLinkPage = GetNextPage(nPage, &nMissingPage);
    }

    // Set the GREEN Flof key
    pBuffer->EditorialLink[VTFLOF_GREEN] = Page2PageHex(nLinkPage);

    pBuffer->bShowRow24 = TRUE;
    pBuffer->LineState[24] = CACHESTATE_HASDATA;

    pLineInput = pBuffer->Frame[24];

    // See if a wait message is required instead
    if (bWaitMessage != FALSE)
    {
        // Invalidate the YELLOW and BLUE Flof keys
        pBuffer->EditorialLink[VTFLOF_YELLOW] = 0;
        pBuffer->EditorialLink[VTFLOF_BLUE] = 0;

        if (m_BTTable[nPage] == 0x00)
        {
            pMessage = m_NoneMessage;
        }
        else if (m_BTTable[nPage] == TOPLEVEL_SUBTITLE)
        {
            pMessage = m_SubtitleMessage;
        }
        else if (IsMultiPage(nPage))
        {
            if (m_MPTable[nPage] == 0)
            {
                m_WaitingPage[TOPWAIT_MPT] = nPage;

                // Multi-thread protection
                if (m_MPTable[nPage] != 0)
                {
                    m_WaitingPage[TOPWAIT_MPT] = 0xFFFF;
                }
            }

            if (m_MPTable[nPage] != 0)
            {
                sprintf(szBuffer, m_MultiMessage, m_MPTable[nPage]);
                pMessage = szBuffer;
            }
            else
            {
                pMessage = m_WaitMessage;
            }
        }
        else
        {
            pMessage = m_WaitMessage;
        }

        nLength = strlen(pMessage);
        nStart = (40 - nLength) / 2;

        memset(pLineInput, 0x20, nStart);
        memcpy(&pLineInput[nStart], pMessage, nLength);
        memset(&pLineInput[nStart + nLength], 0x20, 40 - nStart - nLength);
    }
    else
    {
        // Red section
        *pLineInput++ = 0x11;       // Red Mosaic
        *pLineInput++ = 0x3C;       // Box Mosaic
        *pLineInput++ = 0x01;       // Red text
        *pLineInput++ = 0x2D;       // '-'
        *pLineInput++ = 0x20;       // Space

        // Green section
        if (nLinkPage != -1)
        {
            *pLineInput++ = 0x12;       // Green Mosaic
            *pLineInput++ = 0x3C;       // Box Mosaic
            *pLineInput++ = 0x02;       // Green text
            *pLineInput++ = 0x2B;       // '+'
            *pLineInput++ = 0x20;       // Space
        }
        else
        {
            memset(pLineInput, 0x20, 5);
            pLineInput += 5;
        }

        // Yellow Flof link
        nLinkPage = GetNextGroup(nPage, &nMissingPage);

        while (nLinkPage == -1)
        {
            m_WaitingPage[TOPWAIT_YELLOW] = nMissingPage;

            // Multi-thread protection
            if (m_BTTable[nMissingPage] == TOP_UNRECEIVED)
            {
                break;
            }

            m_WaitingPage[TOPWAIT_YELLOW] = 0xFFFF;
            nLinkPage = GetNextGroup(nPage, &nMissingPage);
        }

        pBuffer->EditorialLink[VTFLOF_YELLOW] = Page2PageHex(nLinkPage);

        if (nLinkPage != -1)
        {
            m_WaitingPage[TOPWAIT_YELLOW_AIT] = nLinkPage;

            if (m_AITReceived[nLinkPage] != FALSE)
            {
                pMessage = (char*)m_AITable[nLinkPage];
            }
            else
            {
                pMessage = m_EmptyTitleText;
            }

            *pLineInput++ = 0x13;       // Yellow Mosaic
            *pLineInput++ = 0x3C;       // Box Mosaic
            *pLineInput++ = 0x03;       // Yellow text

            memcpy(pLineInput, pMessage, 12);
            pLineInput += 12;
        }
        else
        {
            memset(pLineInput, 0x20, 15);
            pLineInput += 15;
        }

        // Blue section
        nLinkPage = GetNextBlock(nPage, &nMissingPage);

        while (nLinkPage == -1)
        {
            m_WaitingPage[TOPWAIT_BLUE] = nMissingPage;

            // Multi-thread protection
            if (m_BTTable[nMissingPage] == TOP_UNRECEIVED)
            {
                break;
            }

            m_WaitingPage[TOPWAIT_BLUE] = 0xFFFF;
            nLinkPage = GetNextBlock(nPage, &nMissingPage);
        }

        pBuffer->EditorialLink[VTFLOF_BLUE] = Page2PageHex(nLinkPage);

        if (nLinkPage != -1)
        {
            m_WaitingPage[TOPWAIT_BLUE_AIT] = nLinkPage;

            if (m_AITReceived[nLinkPage] != FALSE)
            {
                pMessage = (char*)m_AITable[nLinkPage];
            }
            else
            {
                pMessage = m_EmptyTitleText;
            }

            *pLineInput++ = 0x16;       // Cyan Mosaic
            *pLineInput++ = 0x3C;       // Box Mosaic
            *pLineInput++ = 0x06;       // Cyan text

            memcpy(pLineInput, pMessage, 12);
            pLineInput += 12;
        }
        else
        {
            memset(pLineInput, 0x20, 15);
            pLineInput += 15;
        }
    }

    pBuffer->EditorialLink[VTFLOF_INDEX] = 0x0100;

    for (i = VTFLOF_RED; i <= VTFLOF_INDEX; i++)
    {
        // All TOP-Text pages have unspecified subcodes
        pBuffer->EditorialLink[i] |= (0x3F7F << 16);
    }

    if (memcmp(pBuffer->Frame[24], m_LastBuffer, 40) != 0)
    {
        memcpy(m_LastBuffer, pBuffer->Frame[24], 40);
        pBuffer->LineState[24] |= CACHESTATE_UPDATED;
    }

    return TRUE;
}


/*  This creates the traditional red, green, yellow, blue
 *  box background TOP-Text commentary but it doesn't look
 *  very nice.  This code is outdated.

    // Red
    *pLineInput++ = 0x01;       // Red
    *pLineInput++ = 0x1d;       // Red Background
    *pLineInput++ = 0x07;       // White text
    *pLineInput++ = 0x60;       // '-'
    *pLineInput++ = 0x20;       // Space
    *pLineInput++ = 0x02;       // Green

    // Green
    *pLineInput++ = 0x1d;       // Green Background
    *pLineInput++ = 0x00;       // Black text
    *pLineInput++ = 0x2B;        // '+'
    *pLineInput++ = 0x20;       // Space
    *pLineInput++ = 0x03;       // Yellow

    // Yellow
    *pLineInput++ = 0x1d;       // Yellow Background
    *pLineInput++ = 0x00;       // Black text
    for (i = 0; i < 12; i++)
    {
        *pLineInput++ = (m_AITable[Page] == NULL) ? ' ' : m_AITable[Page][i];
    }
    *pLineInput++ = 0x04;       // Blue

    // Cyan
    *pLineInput++ = 0x1d;       // Blue Background
    *pLineInput++ = 0x07;       // White text
    for (i = 0; i < 12; i++)
    {
        *pLineInput++ = (m_AITable[Page] == NULL) ? ' ' : m_AITable[Page][i];
    }
*/

