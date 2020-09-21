/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/


/**
 * @file vtmosaicgraphics.h vtmosaicgraphics Header file
 */

#ifndef __VTMOSAICGRAPHICS_H___
#define __VTMOSAICGRAPHICS_H___

#include "VTCommon.h"

void DrawG1Mosaic(LPRECT lpRect, INT8U uChar, INT8U uColor, BOOLEAN bSeparated);

INT32S Mosaic_RegisterCallback(INT32S (*fill_rectangle)(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color),
                               INT32U (*convert_color)(INT32U index, INT32U type));


INT32S Mosaic_init(void);
INT32S Mosaic_deinit(void);



#endif
