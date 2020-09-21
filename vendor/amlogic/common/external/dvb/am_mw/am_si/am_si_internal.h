/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_si_internal.h
 * \brief SI Decoder 模块内部头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-15: create the document
 ***************************************************************************/

#ifndef _AM_SI_INTERNAL_H
#define _AM_SI_INTERNAL_H

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

/**\brief SI Decoder 数据*/
typedef struct
{
	void 		*prv_data;	/**< 私有数据,用于句柄检查*/
	AM_Bool_t   allocated;	/**< 是否已经分配*/
}SI_Decoder_t;

/**\brief SI Decode Descriptor Flag
   bit definition
   SI_DESCR_x_y the flag for the same tag x with the LSB set will be used
*/
typedef enum
{
	SI_DESCR_87_CA = 0x1,
	SI_DESCR_87_LCN = 0x2,
}SI_Descriptor_Flag_t;
/****************************************************************************
 * Function prototypes  
 ***************************************************************************/
 

#ifdef __cplusplus
}
#endif

#endif

