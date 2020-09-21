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
 * \brief 网络管理模块aml驱动
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-29: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <am_types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>
#include <am_misc.h>
#include "../am_net_internal.h"


/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define NET_SOCKET_FILE	   "/tmp/netman_socket"

struct mii_data 
{
	__u16	phy_id;
	__u16	reg_num;
	__u16	val_in;
	__u16	val_out;
}; 

static AM_ErrorCode_t aml_net_open(AM_NET_Device_t *dev);
static AM_ErrorCode_t aml_net_get_status(AM_NET_Device_t *dev, AM_NET_Status_t *status);
static AM_ErrorCode_t aml_net_get_hw_addr(AM_NET_Device_t *dev, AM_NET_HWAddr_t *addr);
static AM_ErrorCode_t aml_net_set_hw_addr(AM_NET_Device_t *dev, const AM_NET_HWAddr_t *addr);
static AM_ErrorCode_t aml_net_get_para(AM_NET_Device_t *dev, AM_NET_Para_t *para);
static AM_ErrorCode_t aml_net_set_para(AM_NET_Device_t *dev, int mask, AM_NET_Para_t *para);
static AM_ErrorCode_t aml_net_close(AM_NET_Device_t *dev);

const AM_NET_Driver_t aml_net_drv =
{
.open        = aml_net_open,
.close       = aml_net_close,
.get_status  = aml_net_get_status,
.get_hw_addr = aml_net_get_hw_addr,
.set_hw_addr = aml_net_set_hw_addr,
.get_para 	 = aml_net_get_para,
.set_para 	 = aml_net_set_para
};



/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief 执行一个命令并等待回应*/
static AM_ErrorCode_t aml_net_do(const char *cmd, char *resp, int resp_len)
{
	int fd;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(cmd);

	AM_DEBUG(1, "connect...");
	AM_TRY(AM_LocalConnect(NET_SOCKET_FILE, &fd));
	AM_DEBUG(1, "send cmd: %s", cmd);
	AM_TRY_FINAL(AM_LocalSendCmd(fd, cmd));

	/*需要等待回应*/
	if (resp)
	{
		AM_DEBUG(1, "wait resp");
		AM_TRY_FINAL(AM_LocalGetResp(fd, resp, resp_len));
		AM_DEBUG(1, "get resp %s", resp);
	}
final:
	close(fd);
	return ret;
}



/**\brief 打开网络设备*/
static AM_ErrorCode_t aml_net_open(AM_NET_Device_t *dev)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	return ret;
}

/**\brief 获取网络连接状态*/
static AM_ErrorCode_t aml_net_get_status(AM_NET_Device_t *dev, AM_NET_Status_t *status)
{
	static struct ifreq ifr;
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
	int s;
	
	if ((s = socket(AF_INET, SOCK_DGRAM,0)) == -1)
	{
		return AM_NET_STATUS_UNLINKED;
	}

	snprintf(ifr.ifr_name, IFNAMSIZ, "eth%d", dev->dev_no);
	
	if (ioctl(s, SIOCGMIIPHY, &ifr)==-1)
	{
		return AM_NET_STATUS_UNLINKED;
	}
	
	mii->reg_num = 0x01;
	if (ioctl(s, SIOCGMIIREG, &ifr) < 0)
	{
		return AM_NET_STATUS_UNLINKED;
	}
	
	close(s);

	*status = (mii->val_out&0x04)?AM_NET_STATUS_LINKED:AM_NET_STATUS_UNLINKED;
	
	return AM_SUCCESS;
}
/**\brief 获取MAC地址*/
static AM_ErrorCode_t aml_net_get_hw_addr(AM_NET_Device_t *dev, AM_NET_HWAddr_t *addr)
{
	char resp[30];
	int adr[6];
	int i;
	
	AM_TRY(aml_net_do("gethw", resp, sizeof(resp)));
	sscanf(resp, "%02x:%02x:%02x:%02x:%02x:%02x", &adr[0], &adr[1], &adr[2], 
												  &adr[3], &adr[4], &adr[5]);
	for (i=0; i<6; i++)
	{
		addr->mac[i] = (uint8_t)adr[i];
		AM_DEBUG(1, "%02x", addr->mac[i]);
	}
	
	return AM_SUCCESS;
}

/**\brief 设置MAC地址*/
static AM_ErrorCode_t aml_net_set_hw_addr(AM_NET_Device_t *dev, const AM_NET_HWAddr_t *addr)
{
	char cmd[30];

	snprintf(cmd, sizeof(cmd), "sethw %02x:%02x:%02x:%02x:%02x:%02x", addr->mac[0], addr->mac[1], addr->mac[2], 
												 					  addr->mac[3], addr->mac[4], addr->mac[5]);
	AM_TRY(aml_net_do(cmd, NULL, 0));
	
	return AM_SUCCESS;
}

/**\brief 获取网络参数*/
static AM_ErrorCode_t aml_net_get_para(AM_NET_Device_t *dev, AM_NET_Para_t *para)
{
	char resp[256];
	char dhcp[5], ip[20], mask[20], gw[20];
	char dns1[20], dns2[20], dns3[20], dns4[20];
	
	/*获取信息*/
	AM_TRY(aml_net_do("getinfo", resp, sizeof(resp)));

	sscanf(resp, "%s %s %s %s %s %s %s %s", dhcp, ip, mask, gw, dns1, dns2, dns3, dns4);

	/*DHCP?*/
	if (!strcmp(dhcp, "yes"))
		para->dynamic = AM_TRUE;
	else
		para->dynamic = AM_FALSE;
		
	/*IP地址*/
	if (! strcmp(ip, "none"))
		para->ip.s_addr = 0;
	else
		inet_pton(AF_INET, ip, &para->ip);

	/*子网掩码*/
	if (! strcmp(mask, "none"))
		para->mask.s_addr = 0;
	else
		inet_pton(AF_INET, mask, &para->mask);

	/*网关地址*/
	if (! strcmp(gw, "none"))
		para->gw.s_addr = 0;
	else
		inet_pton(AF_INET, gw, &para->gw);

	/*DNS服务器1*/
	if (! strcmp(dns1, "none"))
		para->dns1.s_addr = 0;
	else
		inet_pton(AF_INET, dns1, &para->dns1);

	/*DNS服务器2*/
	if (! strcmp(dns2, "none"))
		para->dns2.s_addr = 0;
	else
		inet_pton(AF_INET, dns2, &para->dns2);

	/*DNS服务器3*/
	if (! strcmp(dns3, "none"))
		para->dns3.s_addr = 0;
	else
		inet_pton(AF_INET, dns3, &para->dns3);

	/*DNS服务器4*/
	if (! strcmp(dns4, "none"))
		para->dns4.s_addr = 0;
	else
		inet_pton(AF_INET, dns4, &para->dns4);

	return AM_SUCCESS;
}

/**\brief 设置网络参数*/
static AM_ErrorCode_t aml_net_set_para(AM_NET_Device_t *dev, int mask, AM_NET_Para_t *para)
{
	char cmd[40];
	char buf[INET_ADDRSTRLEN];
	
	/*DHCP?*/
	if (mask & AM_NET_SP_DHCP)
	{
		if (para->dynamic)
		{
			AM_TRY(aml_net_do("setdhcp on", NULL, 0));
			return AM_SUCCESS;
		}
		else
		{
			AM_TRY(aml_net_do("setdhcp off", NULL, 0));
		}
	}
	
	/*IP地址*/
	if (mask & AM_NET_SP_IP)
	{
		inet_ntop(AF_INET, &para->ip, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setip %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}

	/*子网掩码*/
	if (mask & AM_NET_SP_MASK)
	{
		inet_ntop(AF_INET, &para->mask, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setmask %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}

	/*网关地址*/
	if (mask & AM_NET_SP_GW)
	{
		inet_ntop(AF_INET, &para->gw, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setgw %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}

	/*DNS服务器1*/
	if (mask & AM_NET_SP_DNS1)
	{
		inet_ntop(AF_INET, &para->dns1, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setdns1 %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}

	/*DNS服务器2*/
	if (mask & AM_NET_SP_DNS2)
	{
		inet_ntop(AF_INET, &para->dns2, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setdns2 %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}

	/*DNS服务器3*/
	if (mask & AM_NET_SP_DNS3)
	{
		inet_ntop(AF_INET, &para->dns3, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setdns3 %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}
	
	/*DNS服务器4*/
	if (mask & AM_NET_SP_DNS4)
	{
		inet_ntop(AF_INET, &para->dns4, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "setdns4 %s", buf);
		AM_TRY(aml_net_do(cmd, NULL, 0));
	}
	
	return AM_SUCCESS;
}

/**\brief 关闭网络设备 */
static AM_ErrorCode_t aml_net_close(AM_NET_Device_t *dev)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	return ret;
}
	
