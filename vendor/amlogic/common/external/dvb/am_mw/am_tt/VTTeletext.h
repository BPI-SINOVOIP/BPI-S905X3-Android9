/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

/**
 * @file vbi_videotext.h vbi_videotext Header file
 */

#ifndef __VTTELETEXT_H___
#define __VTTELETEXT_H___


typedef enum
{
    VT_OFF,
    VT_BLACK,
    VT_MIXED
} eVTState;


INT32S AM_VT_Init(INT32U pageIndex);
INT32S AM_VT_Exit(void);

#endif
