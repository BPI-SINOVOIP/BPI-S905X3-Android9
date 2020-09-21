/*******************************************************************
 *
 *  Copyright (C) 2010 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description: decode is define
 *
 *  Author: kui zhang
 *
 *******************************************************************/


#ifndef _TRANSFER_VOD_H_
#define _TRANSFER_VOD_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include<unistd.h> 


typedef void (* NTSC_XDS_status_cb)(int what, int arg);

int start_demux_ntsc();


void decode_ntsc_xds_set_callback(NTSC_XDS_status_cb cb_ptr);

#endif




