/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef IONVDEC_PRIV_HEADER__
#define IONVDEC_PRIV_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <videodev2.h>
#include <android/log.h>

#define  TAG        "ionvdec"
#define  XLOG(V,T,...)  __android_log_print(V,T,__VA_ARGS__)
#define  LOGI(...)   XLOG(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define  LOGE(...)   XLOG(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

#endif
