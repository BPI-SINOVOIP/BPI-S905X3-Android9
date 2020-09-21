/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 网络管理模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-19: create the document
 ***************************************************************************/

#ifndef _AM_NET_H
#define _AM_NET_H

#include "am_types.h"
#include <arpa/inet.h>

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

/**\brief 网络管理模块错误代码*/
enum AM_NET_ErrorCode
{
	AM_NET_ERROR_BASE=AM_ERROR_BASE(AM_MOD_NET),
	AM_NET_ERR_INVALID_DEV_NO,          /**< 设备号无效*/
	AM_NET_ERR_BUSY,                    /**< 设备已经被打开*/
	AM_NET_ERR_INVAL_ARG,               /**< 无效的参数*/
	AM_NET_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_NET_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_NET_ERR_END
};


/**\brief IP地址*/
typedef struct in_addr AM_NET_IPAddr_t;

/**\brief 网络参数设置选项, 对应AM_NET_Para_t中的参数，为1时表示设置对应参数*/
enum AM_NET_SetParaMask
{
	AM_NET_SP_DHCP	= 0x1,
	AM_NET_SP_IP	= 0x2,
	AM_NET_SP_MASK	= 0x4,
	AM_NET_SP_GW	= 0x8,
	AM_NET_SP_DNS1	= 0x10,
	AM_NET_SP_DNS2	= 0x20,
	AM_NET_SP_DNS3	= 0x40,
	AM_NET_SP_DNS4	= 0x80,
	AM_NET_SP_ALL 	= AM_NET_SP_DHCP | AM_NET_SP_IP | AM_NET_SP_MASK | AM_NET_SP_GW | 
					  AM_NET_SP_DNS1 | AM_NET_SP_DNS2 | AM_NET_SP_DNS3 | AM_NET_SP_DNS4
};

/**\brief 网卡状态*/
typedef enum
{
	AM_NET_STATUS_UNLINKED,    /**< 网络断开*/
	AM_NET_STATUS_LINKED       /**< 已经连接到网络*/
} AM_NET_Status_t;

/**\brief MAC地址*/
typedef struct
{
	uint8_t      mac[6];       /**< MAC地址*/
} AM_NET_HWAddr_t;

/**\brief 网络参数*/
typedef struct
{
	AM_Bool_t        dynamic;  /**< 是否使用DHCP*/
	AM_NET_IPAddr_t  ip;       /**< IP地址*/
	AM_NET_IPAddr_t  mask;     /**< 子网掩码*/
	AM_NET_IPAddr_t  gw;       /**< 网关地址*/
	AM_NET_IPAddr_t  dns1;     /**< DNS服务器1*/
	AM_NET_IPAddr_t  dns2;     /**< DNS服务器2*/
	AM_NET_IPAddr_t  dns3;     /**< DNS服务器1*/
	AM_NET_IPAddr_t  dns4;     /**< DNS服务器2*/
} AM_NET_Para_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开网络设备
 * \param dev_no 网络设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_Open(int dev_no);

/**\brief 获取网络连接状态
 * \param dev_no 网络设备号
 * \param [out] status 连接状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_GetStatus(int dev_no, AM_NET_Status_t *status);

/**\brief 获取MAC地址
 * \param dev_no 网络设备号
 * \param [out] addr 返回的MAC地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_GetHWAddr(int dev_no, AM_NET_HWAddr_t *addr);

/**\brief 设置MAC地址
 * \param dev_no 网络设备号
 * \param [in] addr MAC地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_SetHWAddr(int dev_no, const AM_NET_HWAddr_t *addr);

/**\brief 获取网络参数
 * \param dev_no 网络设备号
 * \param [out] para 返回的网络参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_GetPara(int dev_no, AM_NET_Para_t *para);

/**\brief 设置网络参数
 * \param dev_no 网络设备号
 * \param mask 设置掩码，见AM_NET_SetParaMask
 * \param [in] para 网络参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_SetPara(int dev_no, int mask, AM_NET_Para_t *para);


/**\brief 关闭网络设备
 * \param dev_no 网络设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_net.h)
 */
extern AM_ErrorCode_t AM_NET_Close(int dev_no);


#ifdef __cplusplus
}
#endif

#endif

