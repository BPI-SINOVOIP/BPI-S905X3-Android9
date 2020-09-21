/*******************************************************************
 *
 *  Copyright (C) 2010 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description: decode is define
 *
 *  Author: kui zhang
 *
 *******************************************************************/
 
#ifndef _DECODE_H_
#define _DECODE_H_




#ifdef  __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <unistd.h> 


#include <unistd.h>
#include "misc.h"
#include "trigger.h"
#include "format.h"
#include "lang.h"
#include "hamm.h"
#include "tables.h"
#include "vbi.h"

#define NTSC_XDS_CB_STATUS 0x1000
#define NTSC_XDS_CB_STATUS_STARTED 0x1001
#define NTSC_XDS_CB_STATUS_STOPPED 0x1002

#define NTSC_XDS_CB_ERROR 0x2000
#define NTSC_XDS_CB_ERROR_MEMORY 0x2002
#define NTSC_XDS_CB_ERROR_TIMEOUT 0x2003
#define NTSC_XDS_CB_ERROR_aaa 0x2010

typedef void (* NTSC_XDS_status_cb)(int what, int arg);

int start_demux_ntsc();


void decode_ntsc_xds_set_callback(NTSC_XDS_status_cb cb_ptr);

vbi_bool decode_vbi_test	(int dev_no, int fid, const uint8_t *data, int len, void *user_data);

#endif
