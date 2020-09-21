/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvDmx"

#include "CTvDmx.h"
CTvDmx::CTvDmx(int id)
{
    mDmxDevId = id;
}

CTvDmx::~CTvDmx()
{
}

#ifdef SUPPORT_ADTV
int CTvDmx::Open(AM_DMX_OpenPara_t &para)
{
    return AM_DMX_Open ( mDmxDevId, &para );
}

int CTvDmx::Close()
{
    return     AM_DMX_Close ( mDmxDevId );
}

int CTvDmx::SetSource(AM_DMX_Source_t source)
{
    return AM_DMX_SetSource ( mDmxDevId, source );
}
#endif

