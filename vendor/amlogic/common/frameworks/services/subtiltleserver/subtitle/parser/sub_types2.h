/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef _SUB_TYPE2_H
#define _SUB_TYPE2_H
#include <utils/Log.h>

//#include <am_sub2.h>
//#include <am_tt2.h>
#include <am_dmx.h>
#include <am_pes.h>
#include <am_misc.h>
#include <am_cc.h>
#include <am_userdata.h>


//#include <am_isdb.h>
#include <am_scte27.h>

#include <stdlib.h>
#include <pthread.h>
#include <android/log.h>
#include <cutils/properties.h>

#define CC_JSON_BUFFER_SIZE 8192




typedef struct {
    pthread_mutex_t  lock;
    //AM_SUB2_Handle_t sub_handle;
    AM_PES_Handle_t  pes_handle;
    //AM_TT2_Handle_t  tt_handle;
    AM_CC_Handle_t   cc_handle;
    AM_SCTE27_Handle_t scte27_handle;
    int              dmx_id;
    int              filter_handle;
    //jobject        obj;
    //SkBitmap       *bitmap;
	//jobject        obj_bitmap;
    int              bmp_w;
    int              bmp_h;
    int              bmp_pitch;
    uint8_t          *buffer;
    int              sub_w;
    int              sub_h;

    void dump(int fd, const char* prefix) {
        dprintf(fd, "%s   SubtitleDataContext:\n", prefix);
        dprintf(fd, "%s     handle[pes:%p cc:%p scte:%p]\n", prefix, pes_handle, cc_handle, scte27_handle);
        dprintf(fd, "%s     dmx_id:%d\n", prefix, dmx_id);
        dprintf(fd, "%s     filter_handle:%d\n", prefix, filter_handle);
        dprintf(fd, "%s     bitmap size:%dx%d\n", prefix, bmp_w, bmp_h);
        dprintf(fd, "%s     bitmap pitch:%d\n", prefix, bmp_pitch);
        dprintf(fd, "%s     subtitle size:%dx%d\n", prefix, sub_w, sub_h);
        dprintf(fd, "%s     buffer:%p\n", prefix, buffer);
    }
} TVSubtitleData;




#endif
