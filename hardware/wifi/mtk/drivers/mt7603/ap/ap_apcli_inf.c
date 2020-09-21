/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    ap_apcli.c

    Abstract:
    Support AP-Client function.

    Note:
    1. Call RT28xx_ApCli_Init() in init function and
       call RT28xx_ApCli_Remove() in close function

    2. MAC of ApCli-interface is initialized in RT28xx_ApCli_Init()

    3. ApCli index (0) of different rx packet is got in

    4. ApCli index (0) of different tx packet is assigned in

    5. ApCli index (0) of different interface is got in APHardTransmit() by using

    6. ApCli index (0) of IOCTL command is put in pAd->OS_Cookie->ioctl_if

    8. The number of ApCli only can be 1

	9. apcli convert engine subroutines, we should just take care data packet.
    Revision History:
    Who             When            What
    --------------  ----------      ----------------------------------------------
    Shiang, Fonchi  02-13-2007      created
*/
#define RTMP_MODULE_OS

#ifdef APCLI_SUPPORT

/*#include "rt_config.h" */
#include "rtmp_comm.h"
#include "rt_os_util.h"
#include "rt_os_net.h"


/*
========================================================================
Routine Description:
    Init AP-Client function.

Arguments:
    pAd            points to our adapter
    main_dev_p      points to the main BSS network interface

Return Value:
    None

Note:
	1. Only create and initialize virtual network interfaces.
	2. No main network interface here.
========================================================================
*/
VOID RT28xx_ApCli_Init(VOID *pAd, PNET_DEV main_dev_p)
{
	RTMP_OS_NETDEV_OP_HOOK netDevOpHook;

	/* init operation functions */
	NdisZeroMemory(&netDevOpHook, sizeof(RTMP_OS_NETDEV_OP_HOOK));
	netDevOpHook.open = ApCli_VirtualIF_Open;
	netDevOpHook.stop = ApCli_VirtualIF_Close;
	netDevOpHook.xmit = rt28xx_send_packets;
	netDevOpHook.ioctl = rt28xx_ioctl;
	RTMP_AP_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_APC_INIT,
						0, &netDevOpHook, 0);
}

/*
========================================================================
Routine Description:
    Open a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: open successfully
    otherwise: open fail

Note:
========================================================================
*/
INT ApCli_VirtualIF_Open(PNET_DEV dev_p)
{
	VOID *pAd;

	pAd = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

	if (VIRTUAL_IF_UP(pAd) != 0)
		return -1;

	/* increase MODULE use count */
	RT_MOD_INC_USE_COUNT();

#if 0 /* os abl move */
	for (ifIndex = 0; ifIndex < MAX_APCLI_NUM; ifIndex++)
	{
		if (pAd->ApCfg.ApCliTab[ifIndex].dev == dev_p)
		{
			RTMP_OS_NETDEV_START_QUEUE(dev_p);
			ApCliIfUp(pAd);
		}
	}
#endif

	RTMP_AP_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_APC_OPEN, 0, dev_p, 0);

	return 0;
}


/*
========================================================================
Routine Description:
    Close a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: close successfully
    otherwise: close fail

Note:
========================================================================
*/
INT ApCli_VirtualIF_Close(PNET_DEV dev_p)
{
	VOID *pAd;

	pAd = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

#if 0 /* os abl move */
	for (ifIndex = 0; ifIndex < MAX_APCLI_NUM; ifIndex++)
	{
		if (pAd->ApCfg.ApCliTab[ifIndex].dev == dev_p)
		{
			RTMP_OS_NETDEV_STOP_QUEUE(dev_p);
			
			/* send disconnect-req to sta State Machine. */
			if (pAd->ApCfg.ApCliTab[ifIndex].Enable)
			{
				MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_DISCONNECT_REQ, 0, NULL, ifIndex);
				RTMP_MLME_HANDLER(pAd);
				DBGPRINT(RT_DEBUG_TRACE, ("(%s) ApCli interface[%d] startdown.\n", __FUNCTION__, ifIndex));
			}
			break;
		}
	}
#endif /* 0 */

	RTMP_AP_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_APC_CLOSE, 0, dev_p, 0);

	VIRTUAL_IF_DOWN(pAd);

	RT_MOD_DEC_USE_COUNT();

	return 0;
}


/*
========================================================================
Routine Description:
    Remove ApCli-BSS network interface.

Arguments:
    pAd            points to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID RT28xx_ApCli_Remove(VOID *pAd)
{
#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */

	RTMP_AP_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_APC_REMOVE, 0, NULL, 0);

#if 0 /* os abl move */
	for(index = 0; index < MAX_APCLI_NUM; index++)
	{
		if (pAd->ApCfg.ApCliTab[index].dev)
		{
			RtmpOSNetDevDetach(pAd->ApCfg.ApCliTab[index].dev);
			rtmp_wdev_idx_unreg(pAd, wdev);
			RtmpOSNetDevFree(pAd->ApCfg.ApCliTab[index].dev);

			/* Clear it as NULL to prevent latter access error. */
			pAd->flg_apcli_init = FALSE;
			pAd->ApCfg.ApCliTab[index].dev = NULL;
		}
	}
#endif /* 0 */

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */
}

#endif /* APCLI_SUPPORT */

