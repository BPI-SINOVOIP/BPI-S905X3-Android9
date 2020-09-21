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

#include "VTCommon.h"
#include "VTDecoder.h"
#include "VTTopText.h"
#include "VTTeletext.h"
#include "VTDrawer.h"



INT32S AM_VT_Init(INT32U pageIndex)
{
    if (AM_SUCCESS != VTInitCommonData())
    {
        M_TELETEXT_DIAG(("\n init common data fail! \n"));
        goto fail;
    }
    if (AM_SUCCESS != VTInitTopTextData())
    {
        M_TELETEXT_DIAG(("\n init TopText data fail! \n"));
        goto fail;
    }

    if (AM_SUCCESS != VTInitDecoderData(pageIndex))
    {
        M_TELETEXT_DIAG(("\n init Decoder data fail! \n"));
        goto fail;
    }
    VTInitDrawerData();

    return AM_SUCCESS;

fail:

    return AM_FAILURE;
}


INT32S AM_VT_Exit(void)
{

    VTFreeDecoderData();
    VTFreeTopTextData();

    return AM_SUCCESS;

}








