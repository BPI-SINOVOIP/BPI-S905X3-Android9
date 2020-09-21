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


#include "VTMosaicGraphics.h"
#include "VTCommon.h"

#include "string.h"
#include <stdio.h>

#define MOSAIC_DEBUG                      (0)

#if MOSAIC_DEBUG
#define mosaic_debug                      AM_TRACE
#else
#define mosaic_debug(...)                 {do{}while(0);}
#endif


struct mosaic_ctx_s
{
    INT32S (*fill_rectangle)(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color);
    INT32U (*convert_color)(INT32U index);
};

struct mosaic_ctx_s* mosaic_ctx = NULL;


static INT8U m_G1MosaicSet[64] =
{
    0x00, 0x20, 0x10, 0x30, 0x08, 0x28, 0x18, 0x38, 0x04, 0x24, 0x14, 0x34, 0x0C, 0x2C, 0x1C, 0x3C,
    0x02, 0x22, 0x12, 0x32, 0x0A, 0x2A, 0x1A, 0x3A, 0x06, 0x26, 0x16, 0x36, 0x0E, 0x2E, 0x1E, 0x3E,
    0x01, 0x21, 0x11, 0x31, 0x09, 0x29, 0x19, 0x39, 0x05, 0x25, 0x15, 0x35, 0x0D, 0x2D, 0x1D, 0x3D,
    0x03, 0x23, 0x13, 0x33, 0x0B, 0x2B, 0x1B, 0x3B, 0x07, 0x27, 0x17, 0x37, 0x0F, 0x2F, 0x1F, 0x3F,
};


static INT8U m_xmgraf[2][2] =
{
	{0, 0},
	{0, 1}
};


static INT8U m_ymgraf[3][3] =
{
    0, 0, 0,
    0, 0, 1,
    0, 1, 1,
};


static INT8U m_BitTable[3][2] =
{
    0x20, 0x10,
    0x08, 0x04,
    0x02, 0x01,
};

INT32S Mosaic_init(void)
{
    INT32S error_code = AM_SUCCESS;

    if (mosaic_ctx == NULL)
    {
        mosaic_ctx = malloc(sizeof(struct mosaic_ctx_s));
        if (mosaic_ctx == NULL)
        {
            mosaic_debug("[%s] out of memory malloc mosaic_ctx!\r\n", __func__);
            return AM_FAILURE;
        }
        else
        {
            mosaic_ctx->fill_rectangle = NULL;
            mosaic_ctx->convert_color   = NULL;
        }
    }

    return error_code;
}

INT32S Mosaic_deinit(void)
{
    INT32S error_code = AM_SUCCESS;

    if (mosaic_ctx != NULL)
    {
        free(mosaic_ctx);
        mosaic_ctx = NULL;
    }

    return error_code;
}


INT32S Mosaic_RegisterCallback(INT32S (*fill_rectangle)(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color),
                               INT32U (*convert_color)(INT32U index, INT32U type))
{
    INT32S error_code = AM_SUCCESS;

    if (fill_rectangle == NULL || convert_color == NULL)
    {
        return AM_FAILURE;
    }

    if (mosaic_ctx == NULL)
    {
        return AM_FAILURE;
    }

    mosaic_ctx->fill_rectangle = fill_rectangle;
    mosaic_ctx->convert_color  = convert_color;

    return error_code;
}


void DrawG1Mosaic(LPRECT lpRect, INT8U uChar, INT8U uColor, BOOLEAN bSeparated)
{
    INT32S x_m = lpRect->width % 2;
    INT32S y_m = lpRect->height % 3;
    INT32S x_d = lpRect->width / 2;
    INT32S y_d = lpRect->height / 3;
    INT32S w, h;
    RECT rect;
    int x;
    int y;

    uChar = (uChar & 0x1f) | ((uChar & 0x40) >> 1);

    for (x = 0; x < 2; x++)
    {
        for (y = 0; y < 3; y++)
        {
            if (m_G1MosaicSet[uChar] & m_BitTable[y][x])
            {
                rect.left = lpRect->left + (x ? (x_d * x + m_xmgraf[x_m][x-1]) : 0);
                rect.top = lpRect->top + (y ? (y_d * y + m_ymgraf[y_m][y-1]) : 0);

                w = x_d + m_xmgraf[x_m][x];
                h = y_d + m_ymgraf[y_m][y];

                if (bSeparated)
                {
                    w = w * 3 / 4;
                    h = h * 3 / 4;
                }

                rect.width = w;
                rect.height = h;
                mosaic_ctx->fill_rectangle(rect.left, rect.top, rect.width, rect.height, mosaic_ctx->convert_color(uColor));
            }
        }
    }
}
