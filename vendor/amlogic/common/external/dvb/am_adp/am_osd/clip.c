#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 剪切区计算
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-19: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <am_mem.h>
#include "am_osd_internal.h"

/****************************************************************************
 * Functions
 ***************************************************************************/

/**\brief 检查一个点是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param x 检测点的X座标
 * \param y 检测点的Y座标
 * \return
 *   - AM_TRUE 点在剪切矩形内
 *   - AM_FALSE 点在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipPoint(AM_OSD_Rect_t *c, int x, int y)
{
	if(x<c->x || x>=c->x+c->w || y<c->y || y>=c->y+c->h)
		return AM_FALSE;
	return AM_TRUE;
}

/**\brief 检查一条水平线是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param x 水平线左顶点X座标
 * \param y 水平线Y座标
 * \param w 水平线宽度
 * \param[out] ox 水平线在剪切矩形内的左顶点X坐标
 * \param[out] ow 水平线在剪切矩形内的宽度
 * \return
 *   - AM_TRUE 水平线在剪切矩形内
 *   - AM_FALSE 水平线在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipHLine(AM_OSD_Rect_t *c, int x, int y, int w, int *ox, int *ow)
{
	int minx, maxx;
	
	if(y<c->y || y>=c->y+c->h)
		return AM_FALSE;
	
	minx = AM_MAX(x, c->x);
	maxx = AM_MIN(x+w, c->x+c->w);
	
	*ox = minx;
	*ow = maxx-minx;
	return AM_TRUE;
}

/**\brief 检查一条垂直线是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param x 垂直线上顶点X座标
 * \param y 垂直线Y座标
 * \param h 垂直线高度
 * \param[out] oy 垂直线在剪切矩形内的上顶点Y坐标
 * \param[out] oh 垂直线在剪切矩形内的高度
 * \return
 *   - AM_TRUE 垂直线在剪切矩形内
 *   - AM_FALSE 垂直线在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipVLine(AM_OSD_Rect_t *c, int x, int y, int h, int *oy, int *oh)
{
	int miny, maxy;
	
	if(x<c->x || x>=c->x+c->w)
		return AM_FALSE;
	
	miny = AM_MAX(y, c->y);
	maxy = AM_MIN(y+h, c->y+c->h);
	
	*oy = miny;
	*oh = maxy-miny;
	return AM_TRUE;
}

/**\brief 检查一个矩形是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param r 检测矩形
 * \param[out] or 矩形在剪切区内的部分
 * \return
 *   - AM_TRUE 矩形在剪切矩形内
 *   - AM_FALSE 矩形在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipRect(AM_OSD_Rect_t *c, AM_OSD_Rect_t *r, AM_OSD_Rect_t *or)
{
	int minx, miny, maxx, maxy;
	
	minx = AM_MAX(c->x, r->x);
	miny = AM_MAX(c->y, r->y);
	maxx = AM_MIN(c->x+c->w, r->x+r->w);
	maxy = AM_MIN(c->y+c->h, r->y+r->h);
	
	if(maxx<=minx || maxy<=miny)
		return AM_FALSE;
	
	or->x = minx;
	or->y = miny;
	or->w = maxx-minx;
	or->h = maxy-miny;
	return AM_TRUE;
}

/**\brief 计算两个矩形的合集矩形
 * \param[in] r1 矩形1
 * \param[in] r2 矩形2
 * \param[out] or 合集矩形
 */
void AM_OSD_CollectRect(AM_OSD_Rect_t *r1, AM_OSD_Rect_t *r2, AM_OSD_Rect_t *or)
{
	int minx, miny, maxx, maxy;
	
	minx = AM_MIN(r1->x, r2->x);
	miny = AM_MIN(r1->y, r2->y);
	maxx = AM_MAX(r1->x+r1->w, r2->x+r2->w);
	maxy = AM_MAX(r1->y+r1->h, r2->y+r2->h);
	
	or->x = minx;
	or->y = miny;
	or->w = maxx-minx;
	or->h = maxy-miny;
}

