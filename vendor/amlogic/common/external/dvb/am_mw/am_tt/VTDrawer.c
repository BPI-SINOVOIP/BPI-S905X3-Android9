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

#include "VTDrawer.h"
#include "VTCharacterSet.h"
#include "VTMosaicGraphics.h"
#include "VTCommon.h"
#include <am_tt.h>
#include <stdio.h>
#include <string.h>


#define VTDRAW_DEBUG                      (1)
#if VTDRAW_DEBUG
#define vtdraw_debug                      printf
#else
#define vtdraw_debug(...)                 {do{}while(0);}
#endif


typedef struct
{
    INT16U flag;
    INT8U DoubleProfile[25];

} TVTDrawPara;

struct vtdraw_ctx_s
{
    INT32S (*fill_rectangle)(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color);
    INT32S (*draw_text)(INT32S x, INT32S y, INT32U width, INT32U height, INT16U *text, INT32S len, INT32U color, INT32U w_scale, INT32U h_scale);
    INT32U (*convert_color)(INT32U index, INT32U type);
    INT32U (*get_font_height)(void);
    INT32U (*get_font_max_width)(void);
    void (*clean_osd)(void) ;
	

    INT32U width_scale;
    INT32U height_scale;
};

static struct vtdraw_ctx_s* vtdraw_ctx = NULL;

static INT8S		m_CurrentCodepage;
static INT16U		m_CharacterSet[96];
static INT16U		m_uRectWidth = 0xffff;  //18;
static INT16U  		m_uRectHeight = 0xffff; //22;
static INT16U       m_uHMargin = 0xffff;
static INT16U       m_uWMargin = 0xffff;

static INT8U DrawPageProc(struct TVTPage*, INT16U wPoint, INT16U* lpFlags, INT16U wColour, INT8U uChar, INT8U uMode, void *draw_para);
static INT8U GetColorValue(INT8U TxtColor);

INT32S VTInitDrawerData(void)
{
    unsigned char i;

    m_CurrentCodepage = VTCODEPAGE_NONE;

    for ( i = 0; i < 96; i++ )
        m_CharacterSet[i] = i + 0x20;

    return AM_SUCCESS;
}

void SetCodepage(INT8S Codepage)
{
    if (Codepage != m_CurrentCodepage)
    {
        GetCharacterSet(Codepage, m_CharacterSet);
        m_CurrentCodepage = Codepage;
    }
}

void DrawPage(struct TVTPage* pPage, INT16U uFlags, INT8U pDoubleProfile[25])
{
    INT16U wParsePageFlags = PARSE_REPEATDOUBLE|PARSE_ALSOKEYWORDS;
    TVTDrawPara draw_para;

    if (uFlags & VTDF_UPDATEDONLY)
    {
        wParsePageFlags |= PARSE_UPDATEDONLY;
    }
    if (uFlags & VTDF_FORCEHEADER)
    {
        wParsePageFlags |= PARSE_FORCEHEADER;
    }
    if (uFlags & VTDF_CLOCKONLY)
    {
        uFlags |= VTDF_HEADERONLY;
    }

    draw_para.flag = uFlags;
    memcpy(draw_para.DoubleProfile, pDoubleProfile, sizeof(INT8U) * 25);

    if(vtdraw_ctx->clean_osd)
    vtdraw_ctx->clean_osd();
	
    ParsePageElements(pPage, &wParsePageFlags, (TParserCallback)DrawPageProc, (void*)&draw_para);
}

/*
 * REPEATDOUBLE:
 *
 * First row:       dddddddDDDDDDDDDDDDD
 * Repeat row:    -------DDDDDDDDDDDDD
 *                       ^
 *                       |- DOUBLEHEIGHT
 *
 * 1. Do the first row normally until DOUBLEHEIGHT then
 *    double the background height from there on.  The
 *    foreground height is as defined by VTMODE_DOUBLE.
 * 2. Do the repeat row to fill the missing backgrounds
 *    until DOUBLEHEIGHT is encountered.
 *
 */

INT8U DrawPageProc(struct TVTPage* tvp , INT16U wPoint, INT16U* lpFlags, INT16U wColour, INT8U uChar, INT8U uMode, void *draw_para)
{
    TVTDrawPara* pTemp = NULL;
    INT16U    uVTDFlags;
    INT8U    DoubleProfile[25];

    INT8U nRow;
    INT8U nCol;
    RECT DrawRect;
    INT8U FrontColor;
    INT8U BackColor;
    INT8U TxtFgColor;
    INT8U TxtBgColor;
    INT16U UnicodeChar;

    INT16U str_length = 1;
    static BOOLEAN bDoubleFont = FALSE;

    pTemp = (TVTDrawPara*)draw_para;

    if (pTemp == NULL)
    {
        return PARSE_STOPPAGE;
    }

    if(m_uRectWidth == 0xffff || m_uRectHeight == 0xffff)
    {
        m_uRectWidth = vtdraw_ctx->get_font_max_width();
        if(m_uRectWidth > 18)
        {
            m_uRectWidth = 18;
        }

        m_uRectHeight = vtdraw_ctx->get_font_height() + 2;
        if(m_uRectWidth > 22)
        {
            m_uRectWidth = 22;
        }

        m_uHMargin = (576 - (m_uRectHeight * 26)) >> 1;
        m_uWMargin = (720 - (m_uRectWidth * 40)) >> 1;
    }

    uVTDFlags = (INT16U)pTemp->flag;
    memcpy(DoubleProfile, pTemp->DoubleProfile, 25 * sizeof(INT8U));

    if ((uVTDFlags & VTDF_CLOCKONLY) && HIGHBYTE(wPoint) < 32)
    {
        return PARSE_CONTINUE;
    }

    if ((uVTDFlags & VTDF_HEADERONLY) && LOWBYTE(wPoint) > 0)
    {
        return PARSE_STOPPAGE;
    }

    if ((uVTDFlags & VTDF_ROW24ONLY) && LOWBYTE(wPoint) < 24)
    {
        return PARSE_CONTINUE;
    }

    if ((*lpFlags & PARSE_DOUBLEREPEAT) && (*lpFlags & PARSE_DOUBLEHEIGHT))
    {
        return PARSE_STOPLINE;
    }

    if ((uVTDFlags & VTDF_HIDDENONLY) && !(uMode & VTMODE_CONCEAL))
    {
        return PARSE_CONTINUE;
    }

    /*if ((uMode & VTMODE_CONCEAL) && !(uVTDFlags & VTDF_HIDDEN))
    {
        return PARSE_CONTINUE;
    }*/

    nCol = HIGHBYTE(wPoint);
    nRow = LOWBYTE(wPoint);

    DrawRect.left = m_uWMargin + (INT32S)(nCol * m_uRectWidth);
    DrawRect.top = m_uHMargin + m_uRectHeight + (INT32S)(nRow * m_uRectHeight);
    DrawRect.width = m_uRectWidth;
    DrawRect.height = m_uRectHeight;

    //printf("%d %d %d %d\n",DrawRect.left,DrawRect.top,m_uHMargin,m_uRectHeight);

    // Record all the lines where doubles are drawn
    DoubleProfile[nRow] = ((*lpFlags & PARSE_DOUBLEHEIGHT) != 0);


    if (*lpFlags & PARSE_DOUBLEHEIGHT)
    {
        DrawRect.height = m_uRectHeight * 2;
        vtdraw_ctx->height_scale = 2;
        bDoubleFont = TRUE;
    }
    else
    {
        vtdraw_ctx->width_scale = 1;
        vtdraw_ctx->height_scale = 1;
    }

    FrontColor = LOWBYTE(wColour);
    BackColor = HIGHBYTE(wColour);

    TxtFgColor = GetColorValue(FrontColor);
    TxtBgColor = GetColorValue(BackColor);

    if ((uMode & VTMODE_GRAPHICS) && (uChar & 0x20))
    {
        if (TxtBgColor < 16)
        {
            vtdraw_ctx->fill_rectangle(DrawRect.left, DrawRect.top, DrawRect.width, DrawRect.height, vtdraw_ctx->convert_color(TxtBgColor, AM_TT_COLOR_MOSAIC));
        }

        DrawG1Mosaic(&DrawRect, uChar, TxtFgColor, (uMode & VTMODE_SEPARATED) != 0);
    }

    else
    {
        //if(uChar < 0x20 || uChar > 0x7E)
        if (uChar <= 0x20)
        {
            uChar = 0x20;
        }

        UnicodeChar = m_CharacterSet[uChar - 0x20];

        if (TxtBgColor < 16)
        {
            vtdraw_ctx->fill_rectangle(DrawRect.left, DrawRect.top, DrawRect.width, DrawRect.height,
            		vtdraw_ctx->convert_color(TxtBgColor, (uChar==0x20)?AM_TT_COLOR_SPACE:AM_TT_COLOR_TEXT_BG));
        }

        if (TxtFgColor < 16)
        {
	    char tmp_high;
	    char tmp_low;
	    tmp_high=(UnicodeChar>>8)&0xff;
	    tmp_low=UnicodeChar&0xff;
	    UnicodeChar =tmp_low<<8|tmp_high;
		
            vtdraw_ctx->draw_text(  DrawRect.left,
                                    DrawRect.top,
                                    DrawRect.width,
                                    DrawRect.height,
                                    (INT16U*)&UnicodeChar,
                                    str_length*2,
                                    vtdraw_ctx->convert_color(TxtFgColor, AM_TT_COLOR_TEXT_FG),
                                    vtdraw_ctx->width_scale,
                                    vtdraw_ctx->height_scale);
        }
    }

    if (bDoubleFont)
    {
        bDoubleFont = FALSE;
        vtdraw_ctx->width_scale = 1;
        vtdraw_ctx->height_scale = 1;
    }

    return PARSE_CONTINUE;
}

INT32S Vtdrawer_init(void)
{
    INT32S error_code = AM_SUCCESS;

    if (vtdraw_ctx == NULL)
    {
        vtdraw_ctx = malloc(sizeof(struct vtdraw_ctx_s));
        if (vtdraw_ctx == NULL)
        {
            vtdraw_debug("[%s] out of memory malloc vtdraw_ctx!\r\n", __func__);
            return AM_FAILURE;
        }
        else
        {
            vtdraw_ctx->draw_text      = NULL;
            vtdraw_ctx->fill_rectangle = NULL;
            vtdraw_ctx->convert_color  = NULL;
	    vtdraw_ctx->clean_osd=NULL;

            vtdraw_ctx->width_scale    = 1;
            vtdraw_ctx->height_scale   = 1;
        }
    }

    return error_code;
}

INT32S Vtdrawer_deinit(void)
{
    INT32S error_code = AM_SUCCESS;

    if (vtdraw_ctx != NULL)
    {
        free(vtdraw_ctx);
        vtdraw_ctx = NULL;
    }

    return error_code;
}


INT32S Draw_RegisterCallback(INT32S (*fill_rectangle)(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color),
                             INT32S (*draw_text)(INT32S x, INT32S y, INT32U width, INT32U height, INT16U *text, INT32S len, INT32U color, INT32U w_scale, INT32U h_scale),
                             INT32U (*convert_color)(INT32U index, INT32U type),
                             INT32U (*get_font_height)(void),
                             INT32U (*get_font_max_width)(void),
			     void (*clean_osd)(void))
{
    INT32S error_code = AM_SUCCESS;

    if (fill_rectangle == NULL || draw_text == NULL || convert_color == NULL || get_font_height == NULL || get_font_max_width == NULL)
    {
        return AM_FAILURE;
    }

    if (vtdraw_ctx == NULL)
    {
        return AM_FAILURE;
    }

    vtdraw_ctx->fill_rectangle = fill_rectangle;
    vtdraw_ctx->draw_text      = draw_text;
    vtdraw_ctx->convert_color  = convert_color;
    vtdraw_ctx->get_font_height = get_font_height;
    vtdraw_ctx->get_font_max_width = get_font_max_width;
     vtdraw_ctx->clean_osd=clean_osd;

    return error_code;
}


static INT8U GetColorValue(INT8U TxtColor)
{
    return TxtColor;
}

void SetBounds(INT16U uWidth, INT16U uHeight)
{
    m_uRectWidth = uWidth / 40;
    m_uRectHeight = uHeight / 24;
}


/*
 *  ROUNDING theory:
 *
 *  4 pixels:
 *  .__ __ __ __.
 *  | 0| 1| 2| 3|
 *  *--*--*--*--*
 *
 *  Divided into 3 columns:
 *  .___ ___ ___.
 *  | c0| c1| c2|
 *  *---*---*---*
 *
 *  Each would have a width of (4 / 3) = 1.33 pixels.
 *
 *  Column 1: 0.00 - 1.33
 *  Column 2: 1.33 - 2.66
 *  Column 3: 2.66 - 3.99
 *
 *  ROUND the ranges:
 *
 *  Column 1: 0 - 1
 *  Column 2: 1 - 3
 *  Column 3: 3 - 4
 *
 *  The 4 pixels will divided as:
 *  .__ __ __ __.
 *  |c0|c1|c1|c2|
 *  *--*--*--*--*
 * ------------------------------
 *
 *  Obtaining the column from a pixel:
 *  .__ __ __ __.
 *  |  | P|  |  |    P = 1
 *  *--*--*--*--*
 *
 *  DON'T ROUND: Add 1 to P then divide by 1.33.
 *
 *  (P + 1) / 1.33 = 1.50 => 1
 */




