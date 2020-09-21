/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_rec.h
 * \brief Record manager module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2011-3-30: create the document
 ***************************************************************************/

#ifndef _AM_CHECK_SCRAMB_H
#define _AM_CHECK_SCRAMB_H

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************
 * Function prototypes
 ***************************************************************************/
#define AM_DVB_PID_MAXCOUNT 1024
extern AM_ErrorCode_t AM_Check_Scramb_Start(int dvr_dev, int fifo_id, int dmx_dev, int dmx_src);
extern AM_ErrorCode_t AM_Check_Scramb_Stop(void);
extern AM_ErrorCode_t AM_Check_Scramb_GetInfo(int pid, int *pkt, int *scramb_pkt);
extern AM_ErrorCode_t AM_Check_Has_Tspackage(int pid, int *pkt_num);
#ifdef __cplusplus
}
#endif

#endif

