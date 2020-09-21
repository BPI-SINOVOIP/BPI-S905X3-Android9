/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
/*
 * @file vtdrawer.h vtdrawer Header file
 */

#ifndef __VTDRAWER_H__
#define __VTDRAWER_H__

#include "VTCommon.h"

enum
{
    VTDF_HIDDEN             = 0x0001,
    VTDF_HIDDENONLY         = 0x0002,
    VTDF_FLASH              = 0x0004,
    VTDF_FLASHONLY          = 0x0008,
    VTDF_MIXEDMODE          = 0x0010,
    VTDF_UPDATEDONLY        = 0x0020,
    VTDF_FORCEHEADER        = 0x0040,
    VTDF_HEADERONLY         = 0x0080,
    VTDF_CLOCKONLY          = 0x0100,
    VTDF_ROW24ONLY          = 0x0200,
};


INT32S VTInitDrawerData(void);
/// Sets the text codepage to use when drawing
void SetCodepage(INT8S Codepage);
void SetBounds(INT16U uWidth, INT16U uHeight);

void DrawPage(struct TVTPage* pPage, INT16U uFlags, INT8U pDoubleProfile[25]);

INT32S Draw_RegisterCallback(INT32S (*fill_rectangle)(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color),
                             INT32S (*draw_text)(INT32S x, INT32S y, INT32U width, INT32U height, INT16U *text, INT32S len, INT32U color, INT32U w_scale, INT32U h_scale),
                             INT32U (*convert_color)(INT32U index, INT32U type),
                             INT32U (*get_font_height)(void),
                             INT32U (*get_font_max_width)(void),
			     void (*clean_osd)(void));

INT32S Vtdrawer_init(void);
INT32S Vtdrawer_deinit(void);



#endif
