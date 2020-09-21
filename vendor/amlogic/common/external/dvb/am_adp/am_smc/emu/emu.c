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
 * \brief 模拟智能卡驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-05: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "../am_smc_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static functions declaration
 ***************************************************************************/
 
static AM_ErrorCode_t emu_open (AM_SMC_Device_t *dev, const AM_SMC_OpenPara_t *para);
static AM_ErrorCode_t emu_close (AM_SMC_Device_t *dev);
static AM_ErrorCode_t emu_get_status (AM_SMC_Device_t *dev, AM_SMC_CardStatus_t *status);
static AM_ErrorCode_t emu_reset (AM_SMC_Device_t *dev, uint8_t *atr, int *len);
static AM_ErrorCode_t emu_read (AM_SMC_Device_t *dev, uint8_t *data, int *len, int timeout);
static AM_ErrorCode_t emu_write (AM_SMC_Device_t *dev, const uint8_t *data, int *len, int timeout);

/**\brief 模拟智能卡设备驱动*/
const AM_SMC_Driver_t emu_drv =
{
.open  = emu_open,
.close = emu_close,
.get_status =  emu_get_status,
.reset = emu_reset,
.read  = emu_read,
.write = emu_write
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

AM_ErrorCode_t emu_open (AM_SMC_Device_t *dev, const AM_SMC_OpenPara_t *para)
{
	return AM_SUCCESS;
}

AM_ErrorCode_t emu_close (AM_SMC_Device_t *dev)
{
	return AM_SUCCESS;
}

AM_ErrorCode_t emu_get_status (AM_SMC_Device_t *dev, AM_SMC_CardStatus_t *status)
{
	*status = AM_SMC_CARD_OUT;
	return AM_SUCCESS;
}

AM_ErrorCode_t emu_reset (AM_SMC_Device_t *dev, uint8_t *atr, int *len)
{
	return AM_SMC_ERR_NO_CARD;
}

AM_ErrorCode_t emu_read (AM_SMC_Device_t *dev, uint8_t *data, int *len, int timeout)
{
	return AM_SMC_ERR_NO_CARD;
}

AM_ErrorCode_t emu_write (AM_SMC_Device_t *dev, const uint8_t *data, int *len, int timeout)
{
	return AM_SMC_ERR_NO_CARD;
}

