/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief motor方位角计算库内部头文件
 *
 * \author jiang zhongming <zhongming.jiang@amlogic.com>
 * \date 2010-05-22: create the document
 ***************************************************************************/

#ifndef __AM_ROTOR_CALC_H__
#define __AM_ROTOR_CALC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

double AM_CalcSatHourangle( double SatLon, double SiteLat, double SiteLon );

#ifdef __cplusplus
}
#endif

#endif
