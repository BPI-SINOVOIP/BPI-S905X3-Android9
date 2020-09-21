/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _AM_XDS_H
#define _AM_XDS_H

#include "am_xds_internal.h"
/*
 * AM_Initialize_XDSData :
 *
 * This routines initilaizes the XDS Data Buffer
 */
void AM_Initialize_XDSData(AM_XDSContext_t *context);

/**************************************************************************************
 *
 * Function            : AM_Get_XDSContentAdvisoryInfo
 *
 * Description       : This function returns the content advisory information.
 *
 ***************************************************************************************/

AM_ErrorCode_t AM_Get_XDSContentAdvisoryInfo (AM_ContentAdvisoryInfo_t *info);


/*
 * AM_Initialize_XDSDataServices :
 *
 * This function Initializes all the XDS Services
 * and allocated memory for the XDS Buffer.
 */

AM_ErrorCode_t AM_Initialize_XDSDataServices(void);


/*
 * AM_Terminate_XDSDataServices :
 *
 * This function frees up all the resources allocated for
 * XDS Data
 *
 */
void AM_Terminate_XDSDataServices(void);

/*
 *
 * XDS  Data Parse :
 *
 */
AM_ErrorCode_t AM_Decode_XDSData(unsigned char data1,unsigned char data2);


void AM_Reset_XDSCounters(AM_XDSContext_t *context);
int Am_Get_Vchip_Change_Status();
void Am_Reset_Vchip_Change_Status();
void AM_Clr_Vchip_Event();

/**************************************************************************************
 *
 * Function            :  AM_Get_XDSDataInfo
 *
 * Description       : Returns the Parsed and decoded XDS Data Information as available
 *                     by the video driver.
 *
 ***************************************************************************************/

AM_ErrorCode_t AM_Get_XDSDataInfo(AM_XDSInfo_t *returnInfo);

#endif
