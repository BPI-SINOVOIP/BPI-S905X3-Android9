/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rtmp_mcu.c

	Abstract:

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
*/


#include	"rt_config.h"

INT MCUBurstWrite(PRTMP_ADAPTER pAd, UINT32 Offset, UINT32 *Data, UINT32 Cnt)
{
	RTUSBMultiWrite_nBytes(pAd, Offset, (PUCHAR)Data, Cnt * 4, 64); 
	return 0;
}

INT MCURandomWrite(PRTMP_ADAPTER pAd, UINT32 Num,... /*RTMP_REG_PAIR *RegPair*/)
{
	UINT32 Index;
	va_list arg_ptr;
	RTMP_REG_PAIR* RegPair = NULL;
	va_start(arg_ptr, Num);
	RegPair = va_arg(arg_ptr, PRTMP_REG_PAIR);
	for (Index = 0; Index < Num; Index++)
		RTMP_IO_WRITE32(pAd, RegPair->Register, RegPair->Value);
	va_end(arg_ptr);
	return 0;
}

VOID ChipOpsMCUHook(PRTMP_ADAPTER pAd, enum MCU_TYPE MCUType)
{

	RTMP_CHIP_OP *pChipOps = &pAd->chipOps;


	if (MCUType == M8051) 
	{
		pChipOps->sendCommandToMcu = RtmpAsicSendCommandToMcu;
		pChipOps->BurstWrite = MCUBurstWrite;
		pChipOps->RandomWrite = MCURandomWrite;
	}

#ifdef CONFIG_ANDES_SUPPORT
	if (MCUType == ANDES) 
	{

#ifdef RTMP_USB_SUPPORT
		pChipOps->loadFirmware = USBLoadFirmwareToAndes;
#endif
		//pChipOps->sendCommandToMcu = AsicSendCmdToAndes;
		pChipOps->Calibration = AndesCalibrationOP;
		pChipOps->BurstWrite =  AndesBurstWrite;
		pChipOps->BurstRead = AndesBurstRead;
		pChipOps->RandomRead = AndesRandomRead;
		pChipOps->RFRandomRead = AndesRFRandomRead;
		pChipOps->ReadModifyWrite = AndesReadModifyWrite;
		pChipOps->RFReadModifyWrite = AndesRFReadModifyWrite;
		pChipOps->RandomWrite = AndesRandomWrite;
		pChipOps->RFRandomWrite = AndesRFRandomWrite;
		pChipOps->PwrSavingOP = AndesPwrSavingOP;
	}
#endif
}

NDIS_STATUS isMCUNeedToLoadFIrmware(
        IN PRTMP_ADAPTER pAd)
{
        NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
        ULONG                   Index;
        UINT32                  MacReg;

        Index = 0;

#ifdef CONFIG_ANDES_SUPPORT
	if (WaitForAsicReady(pAd) == TRUE)
	{
		DBGPRINT(RT_DEBUG_TRACE,("%s WaitForAsicReady ====> TRUE\n", __FUNCTION__));	
		#if 1 // don't need to reset MAC in normal cases.
		MT7601_WLAN_ChipOnOff(pAd, FALSE, FALSE);
		#else
		MT7601_WLAN_ChipOnOff(pAd, TRUE, TRUE);
		#endif
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("%s ====> \n", __FUNCTION__));
	}
#endif
        do {
                if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
                        return NDIS_STATUS_FAILURE;

#ifdef CONFIG_ANDES_SUPPORT
                RTMP_IO_READ32(pAd, COM_REG0, &MacReg);
                                
                if (MacReg == 0x1) 
                        break;

                RtmpOsMsDelay(10);
#else
                RTMP_IO_READ32(pAd, PBF_SYS_CTRL, &MacReg);

                if (MacReg & 0x100) /* check bit 8*/
                        break;

                RTMPusecDelay(1000);
#endif
        } while (Index++ < 100);

        if (Index >= 100)
                Status = NDIS_STATUS_FAILURE;
        return Status;
}

