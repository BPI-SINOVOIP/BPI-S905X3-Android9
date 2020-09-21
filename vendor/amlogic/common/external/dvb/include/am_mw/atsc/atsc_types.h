/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef	_ATSC_TYPES_H
#define	_ATSC_TYPES_H

#include <endian.h>
#include <am_types.h>
#include <am_mem.h>
#include <am_debug.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************
* Global Definitions
*****************************************************************************/
typedef int8_t		INT8S;
typedef uint8_t		INT8U;
typedef int16_t	    INT16S;
typedef uint16_t	INT16U;
typedef int32_t		INT32S;
typedef uint32_t	INT32U;

#ifndef __FUNC__
#define __FUNC__	__func__
#endif

#define AM_TRACE(_fmt...) AM_DEBUG(1, _fmt);

#ifndef BYTE_ORDER
#define BYTE_ORDER _BYTE_ORDER
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN _BIG_ENDIAN
#endif

#define AMMem_malloc malloc
#define AMMem_free free

#define DVB_INVALID_PID                 0x1fff
#define DVB_INVALID_ID                  0xffff
#define DVB_INVALID_VERSION             0xff

/* atsc table */
#define ATSC_BASE_PID           0x1ffb
#define ATSC_PSIP_MGT_TID       0xc7
#define ATSC_PSIP_TVCT_TID      0xc8
#define ATSC_PSIP_CVCT_TID      0xc9
#define ATSC_PSIP_RRT_TID       0xca
#define ATSC_PSIP_EIT_TID       0xcb
#define ATSC_PSIP_ETT_TID       0xcc
#define ATSC_PSIP_STT_TID       0xcd
#define ATSC_PSIP_DCCT_TID      0xd3
#define ATSC_PSIP_DCCSCT_TID    0xd4
#define ATSC_PSIP_CEA_TID       0xd8

#ifdef __cplusplus
}
#endif

#endif












