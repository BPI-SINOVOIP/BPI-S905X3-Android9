/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002-2010, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
	rt_led.c
 
    Abstract:
 
    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
 */

#include	"rt_config.h"

#ifdef LED_CONTROL_SUPPORT


#ifdef CONFIG_ANDES_SUPPORT
INT LED_Array[16][12]={
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{ 0,   2,     1,    0,   -1,  -1,  0, -1,   5, -1, -1, 17},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{  3,  2,   -1,   -1,   -1, -1, 16,   1,  5,  -1, -1, 17},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1,   -1,   -1,   -1, -1, -1, -1, -1, -1, -1, -1},
	{ 1,   2,     1,   -1,   -1, -1,  3, -1,   6, -1, -1,   0},
	{ 1,   2,     1,   -1,   -1, -1, -1,  1,   4, -1, -1, 18}
};
#endif /* CONFIG_ANDES_SUPPORT */
	

#ifdef CONFIG_SWMCU_SUPPORT
INT Show_LedCfg_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LED_CFG_T macLedCfg;
	ULED_PARAMETER *pLedParameter;
	
	pLedParameter = &pAd->LedCntl.SWMCULedCntl.LedParameter;

	printk("LedAMode_RadioOnLinkA=%d\n", pLedParameter->LedAgCfg.field.LedAMode_RadioOnLinkA);
	printk("LedGMode_RadioOnLinkA=%d\n", pLedParameter->LedAgCfg.field.LedGMode_RadioOnLinkA);
	printk("LedAMode_RadioOnLinkG=%d\n", pLedParameter->LedAgCfg.field.LedAMode_RadioOnLinkG);
	printk("LedGMode_RadioOnLinkG=%d\n", pLedParameter->LedAgCfg.field.LedGMode_RadioOnLinkG);
	printk("LedAMode_RadioOnLinkDown=%d\n", pLedParameter->LedAgCfg.field.LedAMode_RadioOnLinkDown);
	printk("LedGMode_RadioOnLinkDown=%d\n", pLedParameter->LedAgCfg.field.LedGMode_RadioOnLinkDown);
	printk("LedAMode_RadioOff=%d\n", pLedParameter->LedAgCfg.field.LedAMode_RadioOff);
	printk("LedGMode_RadioOff=%d\n", pLedParameter->LedAgCfg.field.LedGMode_RadioOff);

	printk("LedActModeNoTx_RadioOnLinkA=%d\n", pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOnLinkA);
	printk("LedActMode_RadioOnLinkA=%d\n", pLedParameter->LedActCfg.field.LedActMode_RadioOnLinkA);
	printk("LedActModeNoTx_RadioOnLinkG=%d\n", pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOnLinkG);
	printk("LedActMode_RadioOnLinkG=%d\n", pLedParameter->LedActCfg.field.LedActMode_RadioOnLinkG);
	printk("LedActModeNoTx_RadioOnLinkDown=%d\n", pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOnLinkDown);
	printk("LedActMode_RadioOnLinkDown=%d\n", pLedParameter->LedActCfg.field.LedActMode_RadioOnLinkDown);
	printk("LedActModeNoTx_RadioOff=%d\n", pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOff);
	printk("LedActMode_RadioOff=%d\n", pLedParameter->LedActCfg.field.LedActMode_RadioOff);

	RTMP_IO_READ32(pAd, MAC_LED_CFG, &macLedCfg.word);
	
	printk("LED_CFG = %x\n\n", macLedCfg.word);
	printk("LED_ON_TIME = %d\n", macLedCfg.field.LED_ON_TIME);
	printk("LED_OFF_TIME = %d\n", macLedCfg.field.LED_OFF_TIME);
	printk("SLOW_BLK_TIME = %d\n", macLedCfg.field.SLOW_BLK_TIME);
	printk("R_LED_MODE (A) = %d\n", macLedCfg.field.R_LED_MODE);
	printk("G_LED_MODE (ACT) = %d\n", macLedCfg.field.G_LED_MODE);
	printk("Y_LED_MODE (A) = %d\n", macLedCfg.field.Y_LED_MODE);
	printk("LED_POL = %d\n", macLedCfg.field.LED_POL);

	return TRUE;
}


INT Set_LedCfg_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT loop;
	PCHAR thisChar;
	long polarity=0;
	long ledOnTime=0;
	long ledOffTime=0;
	long slowBlkTime=0;
	
	loop = 0;

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		switch(loop)
		{
			case 0:
				polarity = simple_strtol(thisChar, 0, 10);
				break;

			case 1:
				ledOnTime = simple_strtol(thisChar, 0, 10);
				break;

			case 2:
				ledOffTime = simple_strtol(thisChar, 0, 10);
				break;

			case 3:
				slowBlkTime = simple_strtol(thisChar, 0, 10);
				break;

			default:
				break;
		}
		loop ++;
	}

	SetLedCfg(pAd, (INT)polarity, (INT)ledOnTime, (INT)ledOffTime, (INT)slowBlkTime);

	return TRUE;
}

INT Set_LedCheck_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT loop;
	PCHAR thisChar;
	long ledSel=0;
	long ledMode=0;

	loop = 0;
	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		switch(loop)
		{
			case 0:
				ledSel = simple_strtol(thisChar, 0, 10);
				break;

			case 1:
				ledMode = simple_strtol(thisChar, 0, 10);
				break;

			default:
				break;
		}
		loop ++;
	}

	SetLedMode(pAd, (INT)ledSel, (INT)ledMode);

	return TRUE;
}

VOID SetLedCfg(
	IN PRTMP_ADAPTER pAd,
	IN INT ledPolarity,
	IN INT ledOnTime,
	IN INT ledOffTime,
	IN INT slowBlkTime)
{
	LED_CFG_T macLedCfg;
	RTMP_IO_READ32(pAd, MAC_LED_CFG, &macLedCfg.word);

	macLedCfg.field.LED_POL = ledPolarity;
	macLedCfg.field.LED_ON_TIME = ledOnTime;
	macLedCfg.field.LED_OFF_TIME = ledOffTime;
	macLedCfg.field.SLOW_BLK_TIME = slowBlkTime;

	RTMP_IO_WRITE32(pAd, MAC_LED_CFG, macLedCfg.word);

	return;
}

VOID SetLedMode(
	IN PRTMP_ADAPTER pAd,
	IN INT LedSel,
	IN INT LedMode)
{
	LED_CFG_T macLedCfg;
	RTMP_IO_READ32(pAd, MAC_LED_CFG, &macLedCfg.word);

	switch(LedSel)
	{
		case LED_G: /* LED G. */
			macLedCfg.field.Y_LED_MODE = LedMode;
			break;

		case LED_A: /* LED A. */
			macLedCfg.field.G_LED_MODE = LedMode;
			break;

		case LED_ACT: /* LED ACT. */
			macLedCfg.field.R_LED_MODE = LedMode;
			break;

		default:
			break;
	}

	RTMP_IO_WRITE32(pAd, MAC_LED_CFG, macLedCfg.word);

	return;
}


/*
 * WPS Type : time of LED on,
 *     		  time of LED off,
 *		  	  time of on/off cycles that LED blink,
 *		  	  time of on/off cycles that LED rest
 * InProgress: 0.2s, 0.1s, infinite, 0s
 * Error: 0.1s, 0.1s, infinite, 0s
 * Session Overlap Detected: 0.1s, 0.1s, 1s, 0.5s
 * Success: 300s, infinite, infinite, 0s
 * Pre, Post stage: 0.5s, 0.5s, infinite, 0s
 */
void SetWPSLinkStatus(IN PRTMP_ADAPTER pAd)
{
	PLED_OPERATION_MODE pCurrentLedCfg = &pAd->LedCntl.SWMCULedCntl.CurrentLedCfg;
	PWPS_LED_TIME_UNIT pWPSLedTimeUnit = &pAd->LedCntl.SWMCULedCntl.WPSLedTimeUnit;
	
	switch(pAd->LedCntl.SWMCULedCntl.LinkStatus)
	{
#ifdef CONFIG_WIFI_LED_SHARE
		case LINK_STATUS_WPS_PRE_STAGE:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_PRE_STAGE;
			pWPSLedTimeUnit->OnPeriod = 0.5 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = 0.5 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_WPS_POST_STAGE:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_POST_STAGE;
			pWPSLedTimeUnit->OnPeriod = 0.5 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = 0.5 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
#endif /* CONFIG_WIFI_LED_SHARE */
#ifdef WSC_INCLUDED
#ifdef WSC_LED_SUPPORT
		case LINK_STATUS_WPS_IN_PROCESS:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_IN_PROCESS;
			pWPSLedTimeUnit->OnPeriod = 0.2 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = 0.1 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_WPS_ERROR:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_ERROR;
			pWPSLedTimeUnit->OnPeriod = 0.1 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = 0.1 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_WPS_SESSION_OVERLAP_DETECTED:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_SESSION_OVERLAP_DETECTED;
			pWPSLedTimeUnit->OnPeriod = 0.1 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = 0.1 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->BlinkPeriod = 1 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->RestPeriod = 0.5 * 1000 / LED_CHECK_INTERVAL;
			break;
		case LINK_STATUS_WPS_SUCCESS_WITH_SECURITY:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_SUCCESS_WITH_SECURITY;
			pWPSLedTimeUnit->OnPeriod = 300 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_WPS_SUCCESS_WITHOUT_SECURITY:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_SUCCESS_WITHOUT_SECURITY;
			pWPSLedTimeUnit->OnPeriod = 300 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_WPS_TURN_LED_OFF:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_TURN_LED_OFF;
			pWPSLedTimeUnit->OnPeriod = 0;
			pWPSLedTimeUnit->OffPeriod = 0;
			pWPSLedTimeUnit->BlinkPeriod = 0;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_NORMAL_CONNECTION_WITHOUT_SECURITY:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_NORMAL_CONNECTION_WITHOUT_SECURITY;
			pWPSLedTimeUnit->OnPeriod = 300 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
		case LINK_STATUS_NORMAL_CONNECTION_WITH_SECURITY:
			pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_WPS_NORMAL_CONNECTION_WITH_SECURITY;
			pWPSLedTimeUnit->OnPeriod = 300 * 1000 / LED_CHECK_INTERVAL;
			pWPSLedTimeUnit->OffPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->BlinkPeriod = INFINITE_PERIOD;
			pWPSLedTimeUnit->RestPeriod = 0;
			break;
#endif /* WSC_INCLUDED */
#endif /* WSC_LED_SUPPORT */
		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s:(Unknown LinkStatus)%d", __FUNCTION__, pAd->LedCntl.SWMCULedCntl.LinkStatus));
			break;
		}

		pWPSLedTimeUnit->LeftOnPeriod = pWPSLedTimeUnit->OnPeriod;
		pWPSLedTimeUnit->LeftOffPeriod = pWPSLedTimeUnit->OffPeriod;
		pWPSLedTimeUnit->LeftBlinkPeriod = pWPSLedTimeUnit->BlinkPeriod;
		pWPSLedTimeUnit->LeftRestPeriod = pWPSLedTimeUnit->RestPeriod;
}


void SetLedLinkStatus(IN PRTMP_ADAPTER pAd)
{
	PSWMCU_LED_CONTROL pSWMCULedCntl = &pAd->LedCntl.SWMCULedCntl;
	ULED_PARAMETER *pLedParameter = &pAd->LedCntl.SWMCULedCntl.LedParameter;
	PLED_OPERATION_MODE pCurrentLedCfg  = &pAd->LedCntl.SWMCULedCntl.CurrentLedCfg;
	
	pAd->LedCntl.SWMCULedCntl.BlinkFor8sTimer = 0;

	switch(pSWMCULedCntl->LinkStatus)
	{
		case LINK_STATUS_RADIO_OFF:			/* Radio off */
			pCurrentLedCfg->field.LedGMode = pLedParameter->LedAgCfg.field.LedGMode_RadioOff;
			pCurrentLedCfg->field.LedAMode = pLedParameter->LedAgCfg.field.LedAMode_RadioOff;
			pCurrentLedCfg->field.LedActMode = pLedParameter->LedActCfg.field.LedActMode_RadioOff;
			pCurrentLedCfg->field.LedActModeNoTx = pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOff;
			pCurrentLedCfg->field.LedGPolarity = pLedParameter->LedPolarityCfg.field.LedGPolarity_RadioOff;
			pCurrentLedCfg->field.LedAPolarity = pLedParameter->LedPolarityCfg.field.LedAPolarity_RadioOff;
			pCurrentLedCfg->field.LedActPolarity = pLedParameter->LedPolarityCfg.field.LedActPolarity_RadioOff;
			break;

		case LINK_STATUS_RADIO_ON:			/* Radio on but not link up */
			pCurrentLedCfg->field.LedGMode = pLedParameter->LedAgCfg.field.LedGMode_RadioOnLinkDown;
			pCurrentLedCfg->field.LedAMode = pLedParameter->LedAgCfg.field.LedAMode_RadioOnLinkDown;
			pCurrentLedCfg->field.LedActMode = pLedParameter->LedActCfg.field.LedActMode_RadioOnLinkDown;
			pCurrentLedCfg->field.LedActModeNoTx = pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOnLinkDown;
			pCurrentLedCfg->field.LedGPolarity = pLedParameter->LedPolarityCfg.field.LedGPolarity_RadioOnLinkDown;
			pCurrentLedCfg->field.LedAPolarity = pLedParameter->LedPolarityCfg.field.LedAPolarity_RadioOnLinkDown;
			pCurrentLedCfg->field.LedActPolarity = pLedParameter->LedPolarityCfg.field.LedActPolarity_RadioOnLinkDown;
  			break;

		case LINK_STATUS_GBAND_LINK_UP:		/* Radio on and link to G */
			pCurrentLedCfg->field.LedGMode = pLedParameter->LedAgCfg.field.LedGMode_RadioOnLinkG;
			pCurrentLedCfg->field.LedAMode = pLedParameter->LedAgCfg.field.LedAMode_RadioOnLinkG;
			pCurrentLedCfg->field.LedActMode = pLedParameter->LedActCfg.field.LedActMode_RadioOnLinkG;
			pCurrentLedCfg->field.LedActModeNoTx = pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOnLinkG;
			pCurrentLedCfg->field.LedGPolarity = pLedParameter->LedPolarityCfg.field.LedGPolarity_RadioOnLinkG;
			pCurrentLedCfg->field.LedAPolarity = pLedParameter->LedPolarityCfg.field.LedAPolarity_RadioOnLinkG;
			pCurrentLedCfg->field.LedActPolarity = pLedParameter->LedPolarityCfg.field.LedActPolarity_RadioOnLinkG;
  			break;

		case LINK_STATUS_ABAND_LINK_UP:		/* Radio on and link to A */
			pCurrentLedCfg->field.LedGMode = pLedParameter->LedAgCfg.field.LedGMode_RadioOnLinkA;
			pCurrentLedCfg->field.LedAMode = pLedParameter->LedAgCfg.field.LedAMode_RadioOnLinkA;
			pCurrentLedCfg->field.LedActMode = pLedParameter->LedActCfg.field.LedActMode_RadioOnLinkA;
			pCurrentLedCfg->field.LedActModeNoTx = pLedParameter->LedActCfg.field.LedActModeNoTx_RadioOnLinkA;
			pCurrentLedCfg->field.LedGPolarity = pLedParameter->LedPolarityCfg.field.LedGPolarity_RadioOnLinkA;
			pCurrentLedCfg->field.LedAPolarity = pLedParameter->LedPolarityCfg.field.LedAPolarity_RadioOnLinkA;
			pCurrentLedCfg->field.LedActPolarity = pLedParameter->LedPolarityCfg.field.LedActPolarity_RadioOnLinkA;
  			break;

		case LINK_STATUS_WPS:		/* WPS */
			if(pLedParameter->LedMode == LED_MODE_WPS_LOW_POLARITY)
			{
				pCurrentLedCfg->field.LedActPolarity = ACTIVE_LOW;
				pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_SOLID_ON;	/* Force ACT to solid on/off */
				pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_OFF;			/* Force ACT to solid on/off */
			}
			if(pLedParameter->LedMode == LED_MODE_WPS_HIGH_POLARITY)
			{
				pCurrentLedCfg->field.LedActPolarity = ACTIVE_HIGH;
				pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_SOLID_ON;	/* Force ACT to solid on/off */
				pCurrentLedCfg->field.LedActMode = MCU_LED_ACT_OFF;			/* Force ACT to solid on/off */
			}
			break;

		case LINK_STATUS_ON_SITE_SURVEY:
			if (pLedParameter->LedMode == LED_MODE_8SEC_SCAN)
				pSWMCULedCntl->BlinkFor8sTimer = (8000 / LED_CHECK_INTERVAL);
			break;
	}
}


static void ChgSignalStrengthLed(RTMP_ADAPTER *pAd)
{
	RTMP_IO_WRITE32(pAd, GPIO_DIR, 0x00); /* set GPIO to output. */
	RTMP_IO_WRITE32(pAd, GPIO_DAT, (pAd->LedCntl.SWMCULedCntl.GPIOPolarity ? pAd->LedCntl.SWMCULedCntl.SignalStrength : ~pAd->LedCntl.SWMCULedCntl.SignalStrength));
}


static inline int TX_TRAFFIC_EXIST(RTMP_ADAPTER *pAd)
{
	UINT RegValue;

	RTMP_IO_READ32(pAd, MCU_INT_STATUS, &RegValue);
	return (RegValue & 0x0e);
}

static inline int BEN_TC_ACT(RTMP_ADAPTER *pAd)
{
	UINT RegValue;

	RTMP_IO_READ32(pAd, MCU_INT_STATUS, &RegValue);
	return (RegValue & 0x10);
}

#ifdef __ECOS
#ifdef WSC_LED_SUPPORT
#ifdef RT5350
#define WPS_LED_GPIO 14
void __WPS_LED_ON()
{
        int value;

        value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET);
        value |= (1<<WPS_LED_GPIO);
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET) = value;

        value = (1<<WPS_LED_GPIO);
#if RALINK_GPIO_LED_LOW_ACT
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_CLEAR_OFFSET) = value;   
#else
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_SET_OFFSET) = value;   
#endif
}

void __WPS_LED_OFF()
{
        int value;

        value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET);
        value |= (1<<WPS_LED_GPIO);
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET) = value;

        value = (1<<WPS_LED_GPIO);
#if RALINK_GPIO_LED_LOW_ACT        
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_SET_OFFSET) = value;       
#else
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_CLEAR_OFFSET) = value;   
#endif
}
#endif /* RT5350 */

#ifdef RT6352
#define WPS_LED_GPIO 15
void __WPS_LED_ON()
{
        int value;

        value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_DIR_OFFSET);
        value |= (1<<WPS_LED_GPIO);
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_DIR_OFFSET) = value;

        value = (1<<WPS_LED_GPIO);
#if RALINK_GPIO_LED_LOW_ACT
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_CLEAR_OFFSET) = value;   
#else
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_SET_OFFSET) = value;   
#endif
}

void __WPS_LED_OFF()
{
        int value;

        value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_DIR_OFFSET);
        value |= (1<<WPS_LED_GPIO);
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_DIR_OFFSET) = value;

        value = (1<<WPS_LED_GPIO);
#if RALINK_GPIO_LED_LOW_ACT        
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_SET_OFFSET) = value;       
#else
        HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO3924_CLEAR_OFFSET) = value;   
#endif
}
#endif /* RT6352 */

static void ChgMacWPSLedCfg(RTMP_ADAPTER *pAd)
{
	PWPS_LED_TIME_UNIT pWPSLedTimeUnit = &pAd->LedCntl.SWMCULedCntl.WPSLedTimeUnit;

        /* Always LED Off */
        if (pWPSLedTimeUnit->OnPeriod == 0)
                __WPS_LED_OFF();

        /* Always LED On */
        if ((pWPSLedTimeUnit->OnPeriod == INFINITE_PERIOD) 
                || ((pWPSLedTimeUnit->OnPeriod != 0) 
                && (pWPSLedTimeUnit->OffPeriod == 0) 
                && (pWPSLedTimeUnit->BlinkPeriod == INFINITE_PERIOD)))
                __WPS_LED_ON();


        if ((pWPSLedTimeUnit->LeftBlinkPeriod > 0) 
                || (pWPSLedTimeUnit->BlinkPeriod == INFINITE_PERIOD)) {

                if (pWPSLedTimeUnit->BlinkPeriod != INFINITE_PERIOD)
                        pWPSLedTimeUnit->LeftBlinkPeriod--;
                
                if ((pWPSLedTimeUnit->LeftOnPeriod == 0) 
                        && (pWPSLedTimeUnit->LeftOffPeriod == 0)) {
                        pWPSLedTimeUnit->LeftOnPeriod = pWPSLedTimeUnit->OnPeriod;
                        pWPSLedTimeUnit->LeftOffPeriod = pWPSLedTimeUnit->OffPeriod;
                }

                if (pWPSLedTimeUnit->LeftOnPeriod > 0) {
                        /* Set LED On until LeftOnPeriod == 0 */
                        __WPS_LED_ON();
                        pWPSLedTimeUnit->LeftOnPeriod--;
                } else if (pWPSLedTimeUnit->LeftOffPeriod > 0) {
                        /* Set LED Off until LeftOffPeriod == 0 */
                        __WPS_LED_OFF();
                        pWPSLedTimeUnit->LeftOffPeriod--;
                        return;
                } else if (pWPSLedTimeUnit->OffPeriod == INFINITE_PERIOD) {
                        __WPS_LED_OFF();
                }                
        } else if (pWPSLedTimeUnit->LeftRestPeriod > 0) {
                __WPS_LED_OFF();
                pWPSLedTimeUnit->LeftRestPeriod--;
                if (pWPSLedTimeUnit->LeftRestPeriod == 0) {
                        pWPSLedTimeUnit->LeftOnPeriod = pWPSLedTimeUnit->OnPeriod;
                        pWPSLedTimeUnit->LeftOffPeriod = pWPSLedTimeUnit->OffPeriod;
                        pWPSLedTimeUnit->LeftBlinkPeriod = pWPSLedTimeUnit->BlinkPeriod;
                        pWPSLedTimeUnit->LeftRestPeriod = pWPSLedTimeUnit->RestPeriod;
                }
        } else
                __WPS_LED_OFF();
}
#endif /* WSC_LED_SUPPORT */
#endif /* __ECOS */


/* Change LED State according to CurrentLedPolarity, CurrentLedCfg, and Tx activity */
/* The result is stored in AbstractMacLedCfg (Abstract MacLedCfg). */
/* AbstractMacLedCfg is an abstract layer. Since MAC's LED_CFG doesn't match firmware requirement totally, */
/* this abstract layer is to hide these difference. */
static void ChgMacLedCfg(RTMP_ADAPTER *pAd)
{
	LED_CFG_T LedCfgBuf;
	BYTE DataTxActivity, BeaconTxActivity;
	PSWMCU_LED_CONTROL pSWMCULedCntl = &pAd->LedCntl.SWMCULedCntl;
	PLED_OPERATION_MODE pCurrentLedCfg = &pAd->LedCntl.SWMCULedCntl.CurrentLedCfg;

	/* 
	** MCU_INT_STA (offset: 0x0414)
	** bit 1: MTX2_INT, TX2Q to MAC frame transfer complete interrupt.
	** bit 2: MTX1_INT, TX1Q to MAC frame transfer complete interrupt.
	** bit 3: MTX0_INT, TX0Q to MAC frame transfer complete interrupt.
	*/
	if (TX_TRAFFIC_EXIST(pAd)) /* Check if there is Tx Traffic */
		DataTxActivity = 1;
	else
		DataTxActivity = 0;

	/* Check if there are beacon */
	BeaconTxActivity = BEN_TC_ACT(pAd);

	/* Clear Tx and beacon Tx complete interrupt */
	RTMP_IO_WRITE32(pAd, MCU_INT_STATUS, 0x1e);
	
	RTMP_IO_READ32(pAd, MAC_LED_CFG, &LedCfgBuf.word);

	/* For backward compatible issue,
         * LedActMode: 0: None, 1: Solid ON, 2: Blink (data/mgr), 3: Blink (data,mgr,beacon)
	 * =>Solid off = solid on + high polarity
         */
//#ifdef RT5350
#if defined(RT5350) || defined(RT6352)
	LedCfgBuf.field.LED_POL = !pCurrentLedCfg->field.LedActPolarity;
#else
	LedCfgBuf.field.LED_POL = pCurrentLedCfg->field.LedActPolarity;
#endif
	if(pSWMCULedCntl->LedBlinkTimer!=0)
		pSWMCULedCntl->LedBlinkTimer--;
	else
		pSWMCULedCntl->LedBlinkTimer = 0xff;

	/* LED Act. connect to G_LED. */
	if (pSWMCULedCntl->LedParameter.LedMode == LED_MODE_8SEC_SCAN && pSWMCULedCntl->BlinkFor8sTimer)
	{
		UINT8 LedPolarity = pCurrentLedCfg->field.LedActPolarity ? 0 : 3;
		LedCfgBuf.field.G_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 0) ? LedPolarity : ~LedPolarity;
		pSWMCULedCntl->BlinkFor8sTimer--;
	}
	else if (pCurrentLedCfg->field.LedActMode == MCU_LED_ACT_OFF)
	{
		LedCfgBuf.field.G_LED_MODE =
				pCurrentLedCfg->field.LedActPolarity ? MAC_LED_ON : MAC_LED_OFF;
	}
	else if (pCurrentLedCfg->field.LedActMode == MCU_LED_ACT_SOLID_ON)
	{
		LedCfgBuf.field.G_LED_MODE =
				pCurrentLedCfg->field.LedActPolarity ? MAC_LED_OFF : MAC_LED_ON;
	}
	else if ((DataTxActivity && pCurrentLedCfg->field.LedActMode > MCU_LED_ACT_SOLID_ON) /* Data packet transmited. */
		|| (BeaconTxActivity && pCurrentLedCfg->field.LedActMode == MCU_LED_ACT_BLINK_UPON_TX_DATA_MNG_BEN)) /* Beacon frame transmited. */
	{
		LedCfgBuf.field.G_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 0) ? MAC_LED_ON : MAC_LED_OFF;
	}
	else if (!DataTxActivity && !BeaconTxActivity)
	{
		if (pCurrentLedCfg->field.LedActModeNoTx == 0) /* solid on when no tx. */
			LedCfgBuf.field.G_LED_MODE =
				pCurrentLedCfg->field.LedActPolarity ? MAC_LED_OFF : MAC_LED_ON;
		else /* slow blink. */
			LedCfgBuf.field.G_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 4) ? MAC_LED_ON : MAC_LED_OFF;
	}

	/* LED G. connect to Y_LED. */
	if (pCurrentLedCfg->field.LedGMode == MCU_LED_G_FAST_BLINK)
		LedCfgBuf.field.Y_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 0) ? MAC_LED_ON : MAC_LED_OFF;
	else if (pCurrentLedCfg->field.LedGMode == MCU_LED_G_SLOW_BLINK)
		LedCfgBuf.field.Y_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 4) ? MAC_LED_ON : MAC_LED_OFF;
	else if (pCurrentLedCfg->field.LedGMode == MCU_LED_G_SOLID_ON)
		LedCfgBuf.field.Y_LED_MODE =
			pCurrentLedCfg->field.LedGPolarity ? MAC_LED_OFF : MAC_LED_ON;
	else /* dark */
		LedCfgBuf.field.Y_LED_MODE =
			pCurrentLedCfg->field.LedGPolarity ? MAC_LED_ON : MAC_LED_OFF;

	/* LED A. connect to R_LED. */
	if (pCurrentLedCfg->field.LedAMode == MCU_LED_A_FAST_BLINK)
		LedCfgBuf.field.R_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 0) ? MAC_LED_ON : MAC_LED_OFF;
	else if (pCurrentLedCfg->field.LedAMode ==  MCU_LED_A_SLOW_BLINK)
		LedCfgBuf.field.R_LED_MODE = GetBitX(pSWMCULedCntl->LedBlinkTimer, 4) ? MAC_LED_ON : MAC_LED_OFF;
	else if (pCurrentLedCfg->field.LedAMode == MCU_LED_A_SOLID_ON)
		LedCfgBuf.field.R_LED_MODE =
			pCurrentLedCfg->field.LedAPolarity ? MAC_LED_OFF : MAC_LED_ON;
	else
		LedCfgBuf.field.R_LED_MODE =
			pCurrentLedCfg->field.LedAPolarity ? MAC_LED_ON : MAC_LED_OFF;

	if ((pSWMCULedCntl->bWlanLed) && !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
	{
		RTMP_IO_WRITE32(pAd, MAC_LED_CFG, LedCfgBuf.word);
	}
	else
	{
#if defined(RT5350) || defined(RT6352)
		/* dont't look e2p 0x40 alway set down the led*/
		if (IS_RT5350(pAd) || IS_RT6352(pAd))
		{
			RTMP_IO_WRITE32(pAd, MAC_LED_CFG, 1 << 30);
		}
		else
#endif /* defined(RT5350) || defined(RT6352) */
		{
			RTMP_IO_WRITE32(pAd, MAC_LED_CFG, 0);
		}		
	}

	return;
}


#ifdef CONFIG_WIFI_LED_SHARE
static void WPSChgMacLedCfg(RTMP_ADAPTER *pAd)
{
	LED_CFG_T LedCfgBuf;
	PWPS_LED_TIME_UNIT pWPSLedTimeUnit = &pAd->LedCntl.SWMCULedCntl.WPSLedTimeUnit;

	if (pWPSLedTimeUnit->LeftBlinkPeriod)
	{
		if (pWPSLedTimeUnit->LeftOnPeriod)
		{
			LedCfgBuf.field.G_LED_MODE = MAC_LED_ON; /* Solid on */
			pWPSLedTimeUnit->LeftOnPeriod--;

			if(!pWPSLedTimeUnit->LeftOnPeriod)
			{
				pWPSLedTimeUnit->LeftOffPeriod = pWPSLedTimeUnit->OffPeriod;
				if (!pWPSLedTimeUnit->LeftOffPeriod)
					pWPSLedTimeUnit->LeftOnPeriod = pWPSLedTimeUnit->OnPeriod;
			}
		}
		else if (pWPSLedTimeUnit->LeftOffPeriod)
		{
			LedCfgBuf.field.G_LED_MODE = MAC_LED_OFF; /* Off */
			pWPSLedTimeUnit->LeftOffPeriod--;

			if (!pWPSLedTimeUnit->LeftOffPeriod)
			{
				pWPSLedTimeUnit->LeftOnPeriod = pWPSLedTimeUnit->OnPeriod;
				if (!pWPSLedTimeUnit->LeftOnPeriod)
					pWPSLedTimeUnit->LeftOffPeriod = pWPSLedTimeUnit->OffPeriod;
			}
		}

		pWPSLedTimeUnit->LeftBlinkPeriod--;

		if(!pWPSLedTimeUnit->LeftBlinkPeriod)
		{
			pWPSLedTimeUnit->LeftRestPeriod = pWPSLedTimeUnit->RestPeriod;
			if (!pWPSLedTimeUnit->LeftRestPeriod)
				pWPSLedTimeUnit->LeftBlinkPeriod = pWPSLedTimeUnit->BlinkPeriod;
		}
	}
	else if (pWPSLedTimeUnit->LeftRestPeriod)
	{
		LedCfgBuf.field.G_LED_MODE = MAC_LED_ON; /* Off */
		pWPSLedTimeUnit->LeftRestPeriod--;
		if (!pWPSLedTimeUnit->LeftRestPeriod)
		{
			pWPSLedTimeUnit->LeftBlinkPeriod = pWPSLedTimeUnit->BlinkPeriod;
			if (!pWPSLedTimeUnit->LeftBlinkPeriod)
				pWPSLedTimeUnit->LeftRestPeriod = pWPSLedTimeUnit->RestPeriod;
		}
	}

	RTMP_IO_WRITE32(pAd, MAC_LED_CFG, LedCfgBuf.word);
}
#endif /* CONFIG_WIFI_LED_SHARE */


/* Entry for SWMCU Led control.  Triggered by timer per LED_CHECK_INTERVAL */
void SWMCULedCtrlMain(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;
	PSWMCU_LED_CONTROL pSWMCULedCntl = &pAd->LedCntl.SWMCULedCntl;

	switch(pSWMCULedCntl->LedParameter.LedMode)
	{
		case 0:		/* Hardware controlled mode. Just ignore it. */
			return;
		case LED_MODE_SIGNAL_STREGTH:	/* In addition to mode 0, set signal strength LED */
			ChgSignalStrengthLed(pAd);
			break;
#ifdef CONFIG_WIFI_LED_SHARE
		case WPS_LED_MODE_SHARE:
			WPSChgMacLedCfg(pAd);
			break;
#endif /* CONFIG_WIFI_LED_SHARE */
		default:
#ifdef __ECOS
#if defined(RT5350) || defined(RT6352)
#ifdef WSC_LED_SUPPORT
			ChgMacWPSLedCfg(pAd);
#endif /* WSC_LED_SUPPORT */
#endif /* RT5350 || RT6352 */
#endif /* __ECOS */		
			ChgMacLedCfg(pAd);
			break;
	}
}
BUILD_TIMER_FUNCTION(SWMCULedCtrlMain);

#endif /* CONFIG_SWMCU_SUPPORT */


/*
	========================================================================
	
	Routine Description:
		Set LED Status

	Arguments:
		pAd						Pointer to our adapter
		Status					LED Status

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
VOID RTMPSetLEDStatus(RTMP_ADAPTER *pAd, UCHAR Status)
{
	/*ULONG			data; */
	UCHAR			LinkStatus = 0;
	UCHAR			LedMode;
	UCHAR			MCUCmd = 0;
	BOOLEAN 		bIgnored = FALSE;
#ifdef MT7603
	INT 				LED_CMD = -1;
#endif /*MT7603*/
#ifdef WSC_INCLUDED
#ifdef WSC_LED_SUPPORT
	PWSC_CTRL		pWscControl = NULL;

#ifdef CONFIG_AP_SUPPORT
	pWscControl = &pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	pWscControl = &pAd->StaCfg.WscControl;
#endif /* CONFIG_STA_SUPPORT */
#endif /* WSC_LED_SUPPORT */
#endif /* WSC_INCLUDED */

#ifdef CONFIG_ATE
	/*
		In ATE mode of RT2860 AP/STA, we have erased 8051 firmware.
		So LED mode is not supported when ATE is running.
	*/
	if (!IS_RT3572(pAd))
	{
		if (ATE_ON(pAd))
			return;
	}
#endif /* CONFIG_ATE */

#ifdef RTMP_MAC_USB
#ifdef STATS_COUNT_SUPPORT
	if(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_IDLE_RADIO_OFF))
		return;
#endif /* STATS_COUNT_SUPPORT */
#endif /* RTMP_MAC_USB */

#ifdef CONFIG_ZTE_RADIO_ONOFF 
	if (!pAd->CommonCfg.bRadioEnable)
	{
		return;
	}
#endif /* CONFIG_ZTE_RADIO_ONOFF */

	LedMode = 1;//LED_MODE(pAd);
#ifdef MT7603
	if (IS_MT7603(pAd))
	{
		if (LedMode < 0 || Status <0 || LedMode > 15 || Status > 11)
			return;
		else
			LED_CMD = LED_Array[LedMode][Status];
	}
#endif /*MT7603*/
	switch (Status)
	{
		case LED_LINK_DOWN:
			LinkStatus = LINK_STATUS_LINK_DOWN;
			pAd->LedCntl.LedIndicatorStrength = 0;
			MCUCmd = MCU_SET_LED_MODE;
			break;
		case LED_LINK_UP:
			if (pAd->CommonCfg.Channel > 14)
				LinkStatus = LINK_STATUS_ABAND_LINK_UP;
			else
				LinkStatus = LINK_STATUS_GBAND_LINK_UP;

			MCUCmd = MCU_SET_LED_MODE;
			break;
		case LED_RADIO_ON:
			LinkStatus = LINK_STATUS_RADIO_ON;
			MCUCmd = MCU_SET_LED_MODE;
			break;
		case LED_HALT: 
			LedMode = 0; /* Driver sets MAC register and MAC controls LED */
		case LED_RADIO_OFF:
			LinkStatus = LINK_STATUS_RADIO_OFF;
			MCUCmd = MCU_SET_LED_MODE;
			break;
		case LED_WPS:
			LinkStatus = LINK_STATUS_WPS;
			MCUCmd = MCU_SET_LED_MODE;
			break;
		case LED_ON_SITE_SURVEY:
			LinkStatus = LINK_STATUS_ON_SITE_SURVEY;
			MCUCmd = MCU_SET_LED_MODE;
			break;
		case LED_POWER_UP:
			LinkStatus = LINK_STATUS_POWER_UP;
			MCUCmd = MCU_SET_LED_MODE;
			break;
#ifdef CONFIG_ATE
#ifdef RT35xx
		case LED_HW_CONTROL:
			LedMode = 0;
			LinkStatus = LINK_STATUS_HW_CONTROL;
			MCUCmd = MCU_SET_LED_MODE;
			break;	
#endif /* RT35xx */
#endif /* CONFIG_ATE */
#ifdef WSC_INCLUDED
#ifdef WSC_LED_SUPPORT
#ifdef CONFIG_WIFI_LED_SHARE
		case LED_WPS_PRE_STAGE:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_PRE_STAGE;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_PRE_STAGE;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_POST_STAGE:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_PRE_STAGE;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_POST_STAGE;
			}
			else
				bIgnored = TRUE;
			break;
#endif /* CONFIG_WIFI_LED_SHARE */
		case LED_WPS_IN_PROCESS:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_IN_PROCESS;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_IN_PROCESS;
				DBGPRINT(RT_DEBUG_TRACE, ("%s: LED_WPS_IN_PROCESS\n", __FUNCTION__));				
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_ERROR:
			if (WscSupportWPSLEDMode(pAd))
			{
				/* In the case of LED mode 9, the error LED should be turned on only after WPS walk time expiration. */
				if ((pWscControl->bWPSWalkTimeExpiration == FALSE) && 
					 (LED_MODE(pAd) == WPS_LED_MODE_9))
				{
					/* do nothing. */
				}
				else
				{
					LinkStatus = LINK_STATUS_WPS_ERROR;
					MCUCmd = MCU_SET_WPS_LED_MODE;
				}
		
				pWscControl->WscLEDMode = LED_WPS_ERROR;
				pWscControl->WscLastWarningLEDMode = LED_WPS_ERROR;
				
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_SESSION_OVERLAP_DETECTED:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_SESSION_OVERLAP_DETECTED;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_SESSION_OVERLAP_DETECTED;
				pWscControl->WscLastWarningLEDMode = LED_WPS_SESSION_OVERLAP_DETECTED;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_SUCCESS:
			if (WscSupportWPSLEDMode(pAd))
			{
				if ((LED_MODE(pAd) == WPS_LED_MODE_7) || 
					(LED_MODE(pAd) == WPS_LED_MODE_11) || 
					(LED_MODE(pAd) == WPS_LED_MODE_12)
#ifdef CONFIG_WIFI_LED_SHARE
					||(LED_MODE(pAd) == WPS_LED_MODE_SHARE)
#endif /* CONFIG_WIFI_LED_SHARE */
					)
				{
					/* In the WPS LED mode 7, 11 and 12, the blue LED would last 300 seconds regardless of the AP's security settings. */
					LinkStatus = LINK_STATUS_WPS_SUCCESS_WITH_SECURITY;
					MCUCmd = MCU_SET_WPS_LED_MODE;
					pWscControl->WscLEDMode = LED_WPS_SUCCESS;
		
					/* Turn off the WPS successful LED pattern after 300 seconds. */
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_SUCCESSFUL_LED_PATTERN_TIMEOUT);
				}
				else if (LED_MODE(pAd) == WPS_LED_MODE_8) /* The WPS LED mode 8 */
				{
					if (WscAPHasSecuritySetting(pAd, pWscControl)) /* The WPS AP has the security setting. */
					{
						LinkStatus = LINK_STATUS_WPS_SUCCESS_WITH_SECURITY;
						MCUCmd = MCU_SET_WPS_LED_MODE;
						pWscControl->WscLEDMode = LED_WPS_SUCCESS;
		
						/* Turn off the WPS successful LED pattern after 300 seconds. */
						RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_SUCCESSFUL_LED_PATTERN_TIMEOUT);
					}
					else /* The WPS AP does not have the secuirty setting. */
					{
						LinkStatus = LINK_STATUS_WPS_SUCCESS_WITHOUT_SECURITY;
						MCUCmd = MCU_SET_WPS_LED_MODE;
						pWscControl->WscLEDMode = LED_WPS_SUCCESS;
		
						/* Turn off the WPS successful LED pattern after 300 seconds. */
						RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_SUCCESSFUL_LED_PATTERN_TIMEOUT);
					}
				}
				else if (LED_MODE(pAd) == WPS_LED_MODE_9) /* The WPS LED mode 9. */
				{
					/* Always turn on the WPS blue LED for 300 seconds. */
					LinkStatus = LINK_STATUS_WPS_BLUE_LED;
					MCUCmd = MCU_SET_WPS_LED_MODE;
					pWscControl->WscLEDMode = LED_WPS_SUCCESS;
		
					/* Turn off the WPS successful LED pattern after 300 seconds. */
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_SUCCESSFUL_LED_PATTERN_TIMEOUT);
					
				}
				else
				{
					DBGPRINT(RT_DEBUG_TRACE, ("%s: LED_WPS_SUCCESS (Incorrect LED mode = %d)\n", 
						__FUNCTION__, LED_MODE(pAd)));
					ASSERT(FALSE);
				}
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_TURN_LED_OFF:
			if (WscSupportWPSLEDMode(pAd))
			{
#ifdef CONFIG_WIFI_LED_SHARE
				USHORT value;
				RT28xx_EEPROM_READ16(pAd, EEPROM_FREQ_OFFSET, value);
				pAd->LedCntl.MCULedCntl.word = (value >> 8);
#endif /* CONFIG_WIFI_LED_SHARE */
				LinkStatus = LINK_STATUS_WPS_TURN_LED_OFF;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_TURN_LED_OFF;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_TURN_ON_BLUE_LED:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_BLUE_LED;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_SUCCESS;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_NORMAL_CONNECTION_WITHOUT_SECURITY:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_NORMAL_CONNECTION_WITHOUT_SECURITY;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_SUCCESS;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_NORMAL_CONNECTION_WITH_SECURITY:
			if (WscSupportWPSLEDMode(pAd))
			{
				LinkStatus = LINK_STATUS_NORMAL_CONNECTION_WITH_SECURITY;
				MCUCmd = MCU_SET_WPS_LED_MODE;
				pWscControl->WscLEDMode = LED_WPS_SUCCESS;
			}
			else
				bIgnored = TRUE;
			break;
		/*WPS LED Mode 10 */
		case LED_WPS_MODE10_TURN_ON:
			if(WscSupportWPSLEDMode10(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_MODE10_TURN_ON;
				MCUCmd = MCU_SET_WPS_LED_MODE;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_MODE10_FLASH:
			if(WscSupportWPSLEDMode10(pAd))
			{	
				LinkStatus = LINK_STATUS_WPS_MODE10_FLASH;
				MCUCmd = MCU_SET_WPS_LED_MODE;
			}
			else
				bIgnored = TRUE;
			break;
		case LED_WPS_MODE10_TURN_OFF:
			if(WscSupportWPSLEDMode10(pAd))
			{
				LinkStatus = LINK_STATUS_WPS_MODE10_TURN_OFF;
				MCUCmd = MCU_SET_WPS_LED_MODE;;
			}
			else
				bIgnored = TRUE;
			break;
#endif /* WSC_LED_SUPPORT */
#endif /* WSC_INCLUDED */
		default:
			DBGPRINT(RT_DEBUG_WARN, ("RTMPSetLED::Unknown Status 0x%x\n", Status));
			break;
	}

#ifdef MT7603
	if (IS_MT7603(pAd))
	{
		AndesLedOP(pAd, 0, LED_CMD);
		DBGPRINT(RT_DEBUG_TRACE, ("%s: LED_CMD:0x%x, LED Mode:0x%x, LinkStatus:0x%x\n", __FUNCTION__, LED_CMD, LedMode, LinkStatus)); 
	} else
#endif /*MT7603*/
	if (MCUCmd)
	{
		AsicSendCommandToMcu(pAd, MCUCmd, 0xff, LedMode, LinkStatus, FALSE);
		DBGPRINT(RT_DEBUG_TRACE, ("%s: MCUCmd:0x%x, LED Mode:0x%x, LinkStatus:0x%x\n", __FUNCTION__, MCUCmd, LedMode, LinkStatus)); 
	}
	
    /* */
	/* Keep LED status for LED SiteSurvey mode. */
	/* After SiteSurvey, we will set the LED mode to previous status. */
	/* */
	if ((Status != LED_ON_SITE_SURVEY) && (Status != LED_POWER_UP) && (bIgnored == FALSE))
		pAd->LedCntl.LedStatus = Status;
    
}


/*
	========================================================================
	
	Routine Description:
		Set LED Signal Stregth 

	Arguments:
		pAd						Pointer to our adapter
		Dbm						Signal Stregth

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	
	Note:
		Can be run on any IRQL level. 

		According to Microsoft Zero Config Wireless Signal Stregth definition as belows.
		<= -90  No Signal
		<= -81  Very Low
		<= -71  Low
		<= -67  Good
		<= -57  Very Good
		 > -57  Excellent		
	========================================================================
*/
VOID RTMPSetSignalLED(RTMP_ADAPTER *pAd, NDIS_802_11_RSSI Dbm)
{
	UCHAR		nLed = 0;


#ifdef RTMP_MAC_USB
#ifdef STATS_COUNT_SUPPORT
	if(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_IDLE_RADIO_OFF))
		return;
#endif /* STATS_COUNT_SUPPORT */
#endif /* RTMP_MAC_USB */


	if (pAd->LedCntl.MCULedCntl.field.LedMode == LED_MODE_SIGNAL_STREGTH)
	{
		if (Dbm <= -90)
			nLed = 0;
		else if (Dbm <= -81)
			nLed = 1;
		else if (Dbm <= -71)
			nLed = 3;
		else if (Dbm <= -67)
			nLed = 7;
		else if (Dbm <= -57)
			nLed = 15;
		else 
			nLed = 31;

		/* */
		/* Update Signal Stregth to firmware if changed. */
		/* */
		if (pAd->LedCntl.LedIndicatorStrength != nLed)
		{
			AsicSendCommandToMcu(pAd, MCU_SET_LED_GPIO_SIGNAL_CFG, 0xff, nLed, pAd->LedCntl.MCULedCntl.field.Polarity, FALSE);
			pAd->LedCntl.LedIndicatorStrength = nLed;
		}
	}
}


#ifdef WSC_STA_SUPPORT
#ifdef WSC_LED_SUPPORT
/*
	LED indication for normal connection start.
*/
VOID LEDConnectionStart(RTMP_ADAPTER *pAd)
{
	/* LED indication. */
	/*if (pAd->StaCfg.WscControl.bWPSSession == FALSE) */
	/*if (pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE && pAd->StaCfg.WscControl.bWscTrigger) */
	if (pAd->StaCfg.WscControl.WscConfMode == WSC_DISABLE)
	{
		if (LED_MODE(pAd) == WPS_LED_MODE_9) /* LED mode 9. */
		{
			UCHAR WPSLEDStatus = 0;
			
			/* The AP uses OPEN-NONE. */
			if ((pAd->StaCfg.AuthMode == Ndis802_11AuthModeOpen) 
					&& (pAd->StaCfg.WepStatus == Ndis802_11WEPDisabled))
				WPSLEDStatus = LED_WPS_TURN_LED_OFF;
			else /* The AP uses an encryption algorithm. */
				WPSLEDStatus = LED_WPS_IN_PROCESS;
			RTMPSetLED(pAd, WPSLEDStatus);
			DBGPRINT(RT_DEBUG_TRACE, ("%s: %d\n", __FUNCTION__, WPSLEDStatus));
		}
	}
}


/*
	LED indication for normal connection completion.
*/
VOID LEDConnectionCompletion(RTMP_ADAPTER *pAd, BOOLEAN bSuccess)
{
	/* LED indication. */
	/*if (pAd->StaCfg.WscControl.bWPSSession == FALSE) */
	if (pAd->StaCfg.WscControl.WscConfMode == WSC_DISABLE)
	{
		if (LED_MODE(pAd) == WPS_LED_MODE_9) /* LED mode 9. */
		{
			UCHAR WPSLEDStatus = 0;
			
			if (bSuccess == TRUE) /* Successful connenction. */
			{				
				/* The AP uses OPEN-NONE. */
				if ((pAd->StaCfg.AuthMode == Ndis802_11AuthModeOpen) && (pAd->StaCfg.WepStatus == Ndis802_11WEPDisabled))
					WPSLEDStatus = LED_NORMAL_CONNECTION_WITHOUT_SECURITY;
				else /* The AP uses an encryption algorithm. */
					WPSLEDStatus = LED_NORMAL_CONNECTION_WITH_SECURITY;
			}
			else /* Connection failure. */
				WPSLEDStatus = LED_WPS_TURN_LED_OFF;

			RTMPSetLED(pAd, WPSLEDStatus);
			DBGPRINT(RT_DEBUG_TRACE, ("%s: %d\n", __FUNCTION__, WPSLEDStatus));
		}
	}
}
#endif /* WSC_LED_SUPPORT */
#endif /* WSC_STA_SUPPORT */


void rtmp_read_led_setting_from_eeprom(RTMP_ADAPTER *pAd)
{
	USHORT Value;
	PLED_CONTROL pLedCntl = &pAd->LedCntl;
#ifdef RT3593
	if (IS_RT3593(pAd))
	{
		RT28xx_EEPROM_READ16(pAd, EEPROM_EXT_LED_MODE_SETTING - 1, Value);
		pLedCntl->MCULedCntl.word = ((Value & 0xFF00) >> 8);
		RT28xx_EEPROM_READ16(pAd, EEPROM_EXT_LED_AG_CONFIGURATION, Value);
		pLedCntl->LedAGCfg = Value;
		RT28xx_EEPROM_READ16(pAd, EEPROM_EXT_LED_ACT_CONFIGURATION, Value);
		pLedCntl->LedACTCfg = Value;
		RT28xx_EEPROM_READ16(pAd, EEPROM_EXT_LED_AG_ACT_POLARITY, Value);
		pLedCntl->LedPolarity = Value;
	}
	else
#endif /* RT3593 */
#ifdef RT65xx
	if (IS_MT76x0(pAd))
	{
		//; // TODO: wait TC6008 EEPROM format
		RT28xx_EEPROM_READ16(pAd, EEPROM_FREQ_OFFSET, Value);
		pLedCntl->MCULedCntl.word = (Value >> 8);
		RT28xx_EEPROM_READ16(pAd, EEPROM_LEDAG_CONF_OFFSET, Value);
		pLedCntl->LedAGCfg= Value;
		RT28xx_EEPROM_READ16(pAd, EEPROM_LEDACT_CONF_OFFSET, Value);
		pLedCntl->LedACTCfg = Value;
		RT28xx_EEPROM_READ16(pAd, EEPROM_LED_POLARITY_OFFSET, Value);
		pLedCntl->LedPolarity = Value;
	}
	else
#endif /* RT65xx */
#ifdef MT7601
	if (IS_MT7601(pAd))
	{
		; // TODO: wait TC6008 EEPROM format
	}
	else
#endif /* MT7601 */
	{
		RT28xx_EEPROM_READ16(pAd, EEPROM_FREQ_OFFSET, Value);
		pLedCntl->MCULedCntl.word = (Value >> 8);
		RT28xx_EEPROM_READ16(pAd, EEPROM_LEDAG_CONF_OFFSET, Value);
		pLedCntl->LedAGCfg= Value;
		RT28xx_EEPROM_READ16(pAd, EEPROM_LEDACT_CONF_OFFSET, Value);
		pLedCntl->LedACTCfg = Value;
		RT28xx_EEPROM_READ16(pAd, EEPROM_LED_POLARITY_OFFSET, Value);
		pLedCntl->LedPolarity = Value;
	}
}


void RTMPStartLEDMode(IN RTMP_ADAPTER *pAd)
{
#ifdef CONFIG_SWMCU_SUPPORT
	PSWMCU_LED_CONTROL pSWMCULedCntl = &pAd->LedCntl.SWMCULedCntl;
	pSWMCULedCntl->bWlanLed = TRUE;
	RTMPInitTimer(pAd, &pSWMCULedCntl->LedCheckTimer, GET_TIMER_FUNCTION(SWMCULedCtrlMain), pAd, TRUE);
	RTMPSetTimer(&pSWMCULedCntl->LedCheckTimer, LED_CHECK_INTERVAL);
#endif /* CONFIG_SWMCU_SUPPORT */
}


void RTMPInitLEDMode(IN RTMP_ADAPTER *pAd)
{
	PLED_CONTROL pLedCntl = &pAd->LedCntl;

	if (pLedCntl->MCULedCntl.word == 0xFF)
	{
		pLedCntl->MCULedCntl.word = 0x01;
		pLedCntl->LedAGCfg = 0x5555;
		pLedCntl->LedACTCfg= 0x2221;

#ifdef RTMP_MAC_PCI
		pLedCntl->LedPolarity = 0xA9F8;
#endif /* RTMP_MAC_PCI */
#ifdef RTMP_MAC_USB
		pLedCntl->LedPolarity = 0x5627; 
#endif /* RTMP_MAC_USB */
	}
	
	AsicSendCommandToMcu(pAd, MCU_SET_LED_AG_CFG, 0xff, (UCHAR)pLedCntl->LedAGCfg, (UCHAR)(pLedCntl->LedAGCfg >> 8), FALSE);
	AsicSendCommandToMcu(pAd, MCU_SET_LED_ACT_CFG, 0xff, (UCHAR)pLedCntl->LedACTCfg, (UCHAR)(pLedCntl->LedACTCfg >> 8), FALSE);
	AsicSendCommandToMcu(pAd, MCU_SET_LED_POLARITY, 0xff, (UCHAR)pLedCntl->LedPolarity, (UCHAR)(pLedCntl->LedPolarity >> 8), FALSE);
	AsicSendCommandToMcu(pAd, MCU_SET_LED_GPIO_SIGNAL_CFG, 0xff, 0, pLedCntl->MCULedCntl.field.Polarity, FALSE);

	pAd->LedCntl.LedIndicatorStrength = 0xFF;
	RTMPSetSignalLED(pAd, -100);	/* Force signal strength Led to be turned off, before link up */

	RTMPStartLEDMode(pAd);
}


inline void RTMPExitLEDMode(IN RTMP_ADAPTER *pAd)
{
#ifdef CONFIG_SWMCU_SUPPORT
	BOOLEAN Cancelled;
	PSWMCU_LED_CONTROL pSWMCULedCntl = &pAd->LedCntl.SWMCULedCntl;
#endif /* CONFIG_SWMCU_SUPPORT */

	RTMPSetLED(pAd, LED_RADIO_OFF);

#ifdef CONFIG_SWMCU_SUPPORT
	pSWMCULedCntl->bWlanLed = FALSE;
	OS_WAIT(500);
	RTMPCancelTimer(&pSWMCULedCntl->LedCheckTimer, &Cancelled);
#endif /* CONFIG_SWMCU_SUPPORT */
	return;
}

#endif /* LED_CONTROL_SUPPORT */


INT RTMPSetLED(RTMP_ADAPTER *pAd, UCHAR Status)
{
	// TODO: shiang-7603
#if 0	
	if (pAd->chipCap.hif_type == HIF_MT) {
		DBGPRINT(RT_DEBUG_OFF, ("%s(%d): Not support for HIF_MT yet!\n",
							__FUNCTION__, __LINE__));
		return TRUE;
	}
#endif

#ifdef LED_CONTROL_METHOD_1
	if (pAd->LedCntl.LedConfigMethod == 1)
		return TRUE;
#endif /* LED_CONTROL_Method1 */

#ifdef RTMP_MAC_USB
	if (IS_USB_INF(pAd))
	{
		UCHAR LEDStatus;
		LEDStatus = Status;
		if (Status == LED_RADIO_OFF)
			RTMPSetLEDStatus(pAd, Status);
		else
			RTEnqueueInternalCmd(pAd, CMDTHREAD_SET_LED_STATUS, &LEDStatus, sizeof(LEDStatus));
	}	
#endif /* RTMP_MAC_USB */

#ifdef RTMP_MAC_PCI
	if (IS_PCI_INF(pAd) || IS_RBUS_INF(pAd))
	{
#ifdef LED_CONTROL_SUPPORT
		RTMPSetLEDStatus(pAd, Status);
#endif /* LED_CONTROL_SUPPORT */
	}
#endif /* RTMP_MAC_PCI */

	return TRUE;
}


