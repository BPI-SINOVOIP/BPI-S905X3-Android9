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
/**\file am_net.c
 * \brief 网络管理模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-29: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <assert.h>
#include <am_net.h>
#include "am_net_internal.h"
#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define NET_DEV_COUNT      (1)

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef EMU_NET
extern const AM_NET_Driver_t emu_net_drv;
#else
extern const AM_NET_Driver_t aml_net_drv;
#endif

static AM_NET_Device_t net_devices[NET_DEV_COUNT] =
{
#ifdef EMU_NET
{
.drv = &emu_net_drv
}
#else
{
.drv = &aml_net_drv
}
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t net_get_dev(int dev_no, AM_NET_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=NET_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid NET device number %d, must in(%d~%d)", dev_no, 0, NET_DEV_COUNT-1);
		return AM_NET_ERR_INVALID_DEV_NO;
	}
	
	*dev = &net_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t net_get_openned_dev(int dev_no, AM_NET_Device_t **dev)
{
	AM_TRY(net_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "NET device %d has not been openned", dev_no);
		return AM_NET_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/
/**\brief 打开网络设备
 * \param dev_no 网络设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_Open(int dev_no)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(net_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "NET device %d has already been openned", dev_no);
		ret = AM_NET_ERR_BUSY;
		goto final;
	}

	if(dev->drv->open)
	{
		ret = dev->drv->open(dev);
		if(ret!=AM_SUCCESS)
			goto final;
	}
	
	dev->dev_no  = dev_no;
	dev->openned = AM_TRUE;
final:	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 获取网络连接状态
 * \param dev_no 网络设备号
 * \param [out] status 连接状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_GetStatus(int dev_no, AM_NET_Status_t *status)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret;
	
	assert(status);
	
	AM_TRY(net_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->get_status(dev, status);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}
/**\brief 获取MAC地址
 * \param dev_no 网络设备号
 * \param [out] addr 返回的MAC地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_GetHWAddr(int dev_no, AM_NET_HWAddr_t *addr)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret;

	assert(addr);
	
	AM_TRY(net_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->get_hw_addr(dev, addr);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设置MAC地址
 * \param dev_no 网络设备号
 * \param [in] addr MAC地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_SetHWAddr(int dev_no, const AM_NET_HWAddr_t *addr)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret;

	assert(addr);
	
	AM_TRY(net_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->set_hw_addr(dev, addr);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 获取网络参数
 * \param dev_no 网络设备号
 * \param [out] para 返回的网络参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_GetPara(int dev_no, AM_NET_Para_t *para)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret;

	assert(para);
	
	AM_TRY(net_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->get_para(dev, para);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设置网络参数
 * \param dev_no 网络设备号
 * \param mask 设置掩码，见AM_NET_SetParaMask
 * \param [in] para 网络参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_SetPara(int dev_no, int mask, AM_NET_Para_t *para)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret;

	assert(para);
	
	AM_TRY(net_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->set_para(dev, mask, para);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 关闭网络设备
 * \param dev_no 网络设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
AM_ErrorCode_t AM_NET_Close(int dev_no)
{
	AM_NET_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(net_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);

	if(dev->drv->close)
		dev->drv->close(dev);
	
	/*释放资源*/
	pthread_mutex_destroy(&dev->lock);
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}
	
