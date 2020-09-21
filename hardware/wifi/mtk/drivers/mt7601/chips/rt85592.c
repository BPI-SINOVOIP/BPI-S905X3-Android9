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
	rt85592.c

	Abstract:
	Specific funcitons and configurations for RT85592

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
*/

#ifdef RT8592

#include	"rt_config.h"

static const RT8592_FREQ_ITEM RT85592_FreqPlan_Xtal40M[] =
{
	/* Channel, N, K, mod, R */
	{1, 80, 12, 30, 0},
	{2, 80, 17, 30, 0}, 
	{3, 80, 22, 30, 0},
	{4, 80, 27, 30, 0},
	{5, 81, 2, 30, 0},
	{6, 81, 7, 30, 0},
	{7, 81, 12, 30, 0},
	{8, 81, 17, 30, 0},
	{9, 81, 22, 30, 0},
	{10, 81, 27, 30, 0},
	{11, 82, 2, 30, 0},
	{12, 82, 7, 30, 0},
	{13, 82, 12, 30, 0},
	{14, 82, 24, 30, 0},

	{36, 86, 4, 12, 0},
	{38, 86, 6, 12, 0},
	{40, 86, 8, 12, 0},
	{42, 86, 10, 12, 0},
	{44, 87, 0, 12, 0},
	{46, 87, 2, 12, 0},
	{48, 87, 4, 12, 0},
	{50, 87, 6, 12, 0},
	{52, 87, 8, 12, 0},
	{54, 87, 10, 12, 0},
	{56, 88, 0, 12, 0},
	{58, 88, 2, 12, 0},
	{60, 88, 4, 12, 0},
	{62, 88, 6, 12, 0},
	{64, 88, 8, 12, 0},
	{100, 91, 8, 12, 0},
	{102, 91, 10, 12, 0},
	{104, 92, 0, 12, 0},
	{106, 92, 2, 12, 0},
	{108, 92, 4, 12, 0},
	{110, 92, 6, 12, 0},
	{112, 92, 8, 12, 0},
	{114, 92, 10, 12, 0},
	{116, 93, 0, 12, 0},
	{118, 93, 2, 12, 0},
	{120, 93, 4, 12, 0},
	{122, 93, 6, 12, 0},
	{124, 93, 8, 12, 0},
	{126, 93, 10, 12, 0},
	{128, 94, 0, 12, 0},
	{130, 94, 2, 12, 0},
	{132, 94, 4, 12, 0},
	{134, 94, 6, 12, 0},
	{136, 94, 8, 12, 0},
	{138, 94, 10, 12, 0},
	{140, 95, 0, 12, 0},
	{149, 95, 9, 12, 0},
	{151, 95, 11, 12, 0},
	{153, 96, 1, 12, 0},
	{155, 96, 3, 12, 0},
	{157, 96, 5, 12, 0},
	{159, 96, 7, 12, 0},
	{161, 96, 9, 12, 0},
	{165, 97, 1, 12, 0},
	{184, 82, 0, 12, 0},
	{188, 82, 4, 12, 0},
	{192, 82, 8, 12, 0},
	{196, 83, 0, 12, 0},
};
static UCHAR RT85592_FreqPlan_Sz = (sizeof(RT85592_FreqPlan_Xtal40M) / sizeof(RT8592_FREQ_ITEM));


//+++ temporary functions get from windows team

#define BIT0		(1 << 0)
#define BIT1		(1 << 1)
#define BIT2		(1 << 2)
#define BIT3		(1 << 3)
#define BIT4		(1 << 4)
#define BIT5		(1 << 5)
#define BIT6		(1 << 6)
#define BIT7		(1 << 7)
#define BIT8		(1 << 8)
#define BIT9		(1 << 9)
#define BIT10	(1 << 10)
#define BIT11	(1 << 11)
#define BIT12	(1 << 12)
#define BIT13	(1 << 13)
#define BIT14	(1 << 14)
#define BIT15	(1 << 15)
#define BIT16	(1 << 16)
#define BIT17	(1 << 17)
#define BIT18	(1 << 18)
#define BIT19	(1 << 19)
#define BIT20	(1 << 20)
#define BIT21	(1 << 21)
#define BIT22	(1 << 22)
#define BIT23	(1 << 23)
#define BIT24	(1 << 24)
#define BIT25	(1 << 25)
#define BIT26	(1 << 26)
#define BIT27	(1 << 27)
#define BIT28	(1 << 28)
#define BIT29	(1 << 29)
#define BIT30	(1 << 30)
#define BIT31	(1 << 31)


#define BB_GET_BYTE0(_BbReg) \
	((_BbReg) & 0x000000FF)

#define BB_SET_BYTE0(_BbReg, _BbValueByte) \
	((_BbReg) = (((_BbReg) & ~0x000000FF) | (_BbValueByte)))

#define BB_SET_BYTE1(_BbReg, _BbValueByte) \
	((_BbReg) = (((_BbReg) & ~0x0000FF00) | (_BbValueByte << 8)))

#define BB_SET_BYTE2(_BbReg, _BbValueByte) \
	((_BbReg) = (((_BbReg) & ~0x00FF0000) | (_BbValueByte << 16)))

#define BB_SET_BYTE3(_BbReg, _BbValueByte) \
	((_BbReg) = (((_BbReg) & ~0xFF000000) | (_BbValueByte << 24)))

#define BB_BITWISE_WRITE(_BbReg, _BitLocation, _BitValue) \
	(_BbReg = (((_BbReg) & ~(_BitLocation)) | (_BitValue)))
#define BB_BITMASK_READ(_BbReg, _BitLocation) \
	((_BbReg) & (_BitLocation))

#define  PCIE_PHY_TX_ATTENUATION_CTRL	0x05C8

#ifdef BIG_ENDIAN
typedef union _TX_ATTENUATION_CTRL_STRUC
{
	struct
	{
		UINT32	Reserve1:20;
		UINT32	PCIE_PHY_TX_ATTEN_EN:1;
		UINT32	PCIE_PHY_TX_ATTEN_VALUE:3;
		UINT32	Reserve2:7;
		UINT32	RF_ISOLATION_ENABLE:1;
	} field;
	
	UINT32	word;
} TX_ATTENUATION_CTRL_STRUC, *PTX_ATTENUATION_CTRL_STRUC;
#else
typedef union _TX_ATTENUATION_CTRL_STRUC {
	struct
	{
		UINT32	RF_ISOLATION_ENABLE:1;
		UINT32	Reserve2:7;
		UINT32	PCIE_PHY_TX_ATTEN_VALUE:3;
		UINT32	PCIE_PHY_TX_ATTEN_EN:1;
		UINT32	Reserve1:20;		
	} field;
	
	UINT32	word;
} TX_ATTENUATION_CTRL_STRUC, *PTX_ATTENUATION_CTRL_STRUC;
#endif


//
// VHT BW80 delta power control (+4~-4dBm) for per-rate Tx power control
//
#define EEPROM_VHT_BW80_TX_POWER_DELTA	(0x11E)

//
// Read per-rate Tx power
//
VOID RT85592ReadTxPwrPerRate(
	IN PRTMP_ADAPTER pAd)
{
	UINT32 data, DataBw40ABand, DataBw80ABand, DataBw40GBand;
	USHORT i, value, value2;
	INT TxPwrBw40ABand, TxPwrBw80ABand, TxPwrBw40GBand;
	UCHAR t1,t2,t3,t4;
	BOOLEAN bMinusBw40ABand = FALSE, bMinusBw80ABand = FALSE,bMinusBw40GBand = FALSE;

    DBGPRINT(RT_DEBUG_TRACE, ("%s() -->\n", __FUNCTION__));
	
	//
	// Get power delta for BW40
	//
	RT28xx_EEPROM_READ16(pAd, EEPROM_TXPOWER_DELTA, value2);
	TxPwrBw40ABand = 0;
	TxPwrBw40GBand = 0;

	if ((value2 & 0xFF) != 0xFF)
	{
		if (value2 & 0x80)
			TxPwrBw40GBand = (value2 & 0xF);
	
		if (value2 & 0x40)
			bMinusBw40GBand = FALSE;
		else
			bMinusBw40GBand = TRUE;
	}
	
	if ((value2 & 0xFF00) != 0xFF00)
	{
		if (value2 & 0x8000)
			TxPwrBw40ABand = ((value2&0x0F00) >> 8);

		if (value2 & 0x4000)
			bMinusBw40ABand = FALSE;
		else
			bMinusBw40ABand = TRUE;
	}

	//
	// Get power delta for BW80
	//
	RT28xx_EEPROM_READ16(pAd, EEPROM_VHT_BW80_TX_POWER_DELTA, value2);
	TxPwrBw80ABand = 0;

	if ((value2 & 0xFF) != 0xFF)
	{
		if (value2 & 0x80)
			TxPwrBw80ABand = (value2 & 0xF);
	
		if (value2 & 0x40)
			bMinusBw80ABand = FALSE;
		else
			bMinusBw80ABand = TRUE;
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("%s: bMinusBw40ABand = %d, bMinusBw80ABand = %d, bMinusBw40GBand = %d\n", 
		__FUNCTION__, 
		bMinusBw40ABand, 
		bMinusBw80ABand, 
		bMinusBw40GBand));

	DBGPRINT(RT_DEBUG_TRACE, ("%s: TxPwrBw40ABand = %d, TxPwrBw80ABand = %d, TxPwrBw40GBand = %d\n", 
		__FUNCTION__, 
		TxPwrBw40ABand, 
		TxPwrBw80ABand, 
		TxPwrBw40GBand));

	for (i = 0; i < 5; i++)
	{
		RT28xx_EEPROM_READ16(pAd, (EEPROM_TXPOWER_BYRATE_20MHZ_2_4G + (i * 4)), value);
		data = value;
		
		if (bMinusBw40ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40ABand);
			if (t1 > 0xF)
				t1 = 0xF;
			
			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40ABand);
			if (t2 > 0xF)
				t2 = 0xF;
			
			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40ABand);
			if (t3 > 0xF)
				t3 = 0xF;
			
			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40ABand)
				t1 = (value & 0xF) - (TxPwrBw40ABand);
			else
				t1 = 0;
			
			if (((value & 0xF0) >> 4) > TxPwrBw40ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40ABand);
			else
				t2 = 0;
			
			if (((value & 0xF00) >> 8) > TxPwrBw40ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40ABand);
			else
				t3 = 0;
			
			if (((value & 0xF000) >> 12) > TxPwrBw40ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40ABand);
			else
				t4 = 0;
		}
		
		DataBw40ABand = t1 + (t2 << 4) + (t3 << 8) + (t4 << 12);

		if (bMinusBw80ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw80ABand);
			if (t1 > 0xF)
				t1 = 0xF;
			
			t2 = ((value & 0xF0) >> 4) + (TxPwrBw80ABand);
			if (t2 > 0xF)
				t2 = 0xF;
			
			t3 = ((value & 0xF00) >> 8) + (TxPwrBw80ABand);
			if (t3 > 0xF)
				t3 = 0xF;
			
			t4 = ((value & 0xF000) >> 12) + (TxPwrBw80ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw80ABand)
				t1 = (value & 0xF) - (TxPwrBw80ABand);
			else
				t1 = 0;
			
			if (((value & 0xF0) >> 4) > TxPwrBw80ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw80ABand);
			else
				t2 = 0;
			
			if (((value & 0xF00) >> 8) > TxPwrBw80ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw80ABand);
			else
				t3 = 0;
			
			if (((value & 0xF000) >> 12) > TxPwrBw80ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw80ABand);
			else
				t4 = 0;
		}
		
		DataBw80ABand = t1 + (t2 << 4) + (t3 << 8) + (t4 << 12);
		
		if (bMinusBw40GBand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40GBand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40GBand);
			if (t2 > 0xF)
				t2 = 0xF;

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40GBand);
			if (t3 > 0xF)
				t3 = 0xF;

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40GBand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40GBand)
				t1 = (value & 0xF) - (TxPwrBw40GBand);
			else
				t1 = 0;
			
			if (((value & 0xF0) >> 4) > TxPwrBw40GBand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40GBand);
			else
				t2 = 0;

			if (((value & 0xF00) >> 8) > TxPwrBw40GBand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40GBand);
			else
				t3 = 0;

			if (((value & 0xF000) >> 12) > TxPwrBw40GBand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40GBand);
			else
				t4 = 0;
		}
		
		DataBw40GBand = t1 + (t2 << 4) + (t3 << 8) + (t4 << 12);
	
		RT28xx_EEPROM_READ16(pAd, (EEPROM_TXPOWER_BYRATE_20MHZ_2_4G + (i * 4) + 2), value);
		if (bMinusBw40ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40ABand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40ABand);
			if (t2 > 0xF)
				t2 = 0xF;

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40ABand);
			if (t3 > 0xF)
				t3 = 0xF;

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40ABand)
				t1 = (value & 0xF) - (TxPwrBw40ABand);
			else
				t1 = 0;

			if (((value & 0xF0) >> 4) > TxPwrBw40ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40ABand);
			else
				t2 = 0;

			if (((value & 0xF00) >> 8) > TxPwrBw40ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40ABand);
			else
				t3 = 0;

			if (((value & 0xF000) >> 12) > TxPwrBw40ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40ABand);
			else
				t4 = 0;
		}
		
		DataBw40ABand |= ((t1 << 16) + (t2 << 20) + (t3 << 24) + (t4 << 28));

		if (bMinusBw80ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw80ABand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw80ABand);
			if (t2 > 0xF)
				t2 = 0xF;

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw80ABand);
			if (t3 > 0xF)
				t3 = 0xF;

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw80ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw80ABand)
				t1 = (value & 0xF) - (TxPwrBw80ABand);
			else
				t1 = 0;

			if (((value & 0xF0) >> 4) > TxPwrBw80ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw80ABand);
			else
				t2 = 0;

			if (((value & 0xF00) >> 8) > TxPwrBw80ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw80ABand);
			else
				t3 = 0;

			if (((value & 0xF000) >> 12) > TxPwrBw80ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw80ABand);
			else
				t4 = 0;
		}
		
		DataBw80ABand |= ((t1 << 16) + (t2 << 20) + (t3 << 24) + (t4 << 28));
		
		if (bMinusBw40GBand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40GBand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40GBand);
			if (t2 > 0xF)
			{
				t2 = 0xF;
			}

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40GBand);
			if (t3 > 0xF)
			{
				t3 = 0xF;
			}

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40GBand);
			if (t4 > 0xF)
			{
				t4 = 0xF;
			}
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40GBand)
			{
				t1 = (value & 0xF) - (TxPwrBw40GBand);
			}
			else
			{
				t1 = 0;
			}

			if (((value & 0xF0) >> 4) > TxPwrBw40GBand)
			{
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40GBand);
			}
			else
			{
				t2 = 0;
			}

			if (((value & 0xF00) >> 8) > TxPwrBw40GBand)
			{
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40GBand);
			}
			else
			{
				t3 = 0;
			}

			if (((value & 0xF000) >> 12) > TxPwrBw40GBand)
			{
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40GBand);
			}
			else
			{
				t4 = 0;
			}
		}
		
		DataBw40GBand |= ((t1 << 16) + (t2 << 20) + (t3 << 24) + (t4 << 28));
		
		data |= (value << 16);
	
		pAd->Tx20MPwrCfgABand[i] = data;
		pAd->Tx20MPwrCfgGBand[i] = data;
		
		pAd->Tx40MPwrCfgABand[i] = DataBw40ABand;
		pAd->Tx80MPwrCfgABand[i] = DataBw80ABand;
		pAd->Tx40MPwrCfgGBand[i] = DataBw40GBand;
	
		if (data != 0xFFFFFFFF)
		{
			RTMP_IO_WRITE32(pAd, (TX_PWR_CFG_0 + (i * 4)), data);
		}
		
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%s: data = 0x%X,  DataBw40ABand = 0x%X,  DataBw80ABand = 0x%X, DataBw40GBand = 0x%x\n", 
			__FUNCTION__, data, DataBw40ABand, DataBw80ABand, DataBw40GBand));
	}

    DBGPRINT(RT_DEBUG_TRACE, ("%s: <--\n", __FUNCTION__));
}




/*
 * Switch channel for RT 85592.
 * Some section is copy from the section of 5592
 *
 */

//-----------------------------------------------------------------------------------


UCHAR RT85592_EeBuffer[EEPROM_SIZE] = {
	0x83, 0x38, 0x01, 0x00, 0x00, 0x0c, 0x43, 0x28, 0x83, 0x00, 0x83, 0x28, 0x14, 0x18, 0xff, 0xff,
	0xff, 0xff, 0x83, 0x28, 0x14, 0x18, 0x00, 0x00, 0x01, 0x00, 0x6a, 0xff, 0x00, 0x02, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0c, 0x43, 0x28, 0x83, 0x01, 0x00, 0x0c,
	0x43, 0x28, 0x83, 0x02, 0x33, 0x0a, 0xec, 0x00, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x20, 0x01, 0x55, 0x77, 0xa8, 0xaa, 0x8c, 0x88, 0xff, 0xff, 0x0a, 0x08, 0x08, 0x06,
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x66, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa,
	0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xaa, 0xaa,
	0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa,
	0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xaa, 0xaa,
	0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	} ;


static RTMP_REG_PAIR	RT85592_MACRegTable[] = {
//+++Add by shiang for testing mgmt ring
	/* {0x230, 0x41041}, */
//---Add by shiang for testing mgmt ring
	{PBF_SYS_CTRL,			0x80c00},
	{PBF_CFG,				0x1f},
	{FCE_CTRL,				0x1},
	{AMPDU_MAX_LEN_20M1S,	0xAA842211},
	{TX_SW_CFG0,			0x604},	/* rising time gap */
// TODO: following register patch from windows
	{TX_SW_CFG1,		0x0},
	{TX_SW_CFG2,		0x0},
// TODO: ----patch end
// TODO: shiang-6590, check what tx report will send to us when following default value set as 2
	{0xa44,					0x0}, /* disable Tx info report */
// TODO: shiang-6590, following regiser used to patch for bbp hang issue, will make the RIFS rx failed!
	{TXOP_HLDR_ET,			0x3},
// ---
};
static UCHAR RT85592_NUM_MAC_REG_PARMS = (sizeof(RT85592_MACRegTable) / sizeof(RTMP_REG_PAIR));


static RTMP_REG_PAIR RT85592_BBPRegTb[]={
	{CORE_R1,	0x40},
	{CORE_R4,	0x0},
	{CORE_R24,	0x0},
	{CORE_R32,	0x4003000a},
	{CORE_R42,	0x0},
	{CORE_R44,	0x0},

	{IBI_R0,		0x209a2098},
	{IBI_R1,		0x29252924},
	{IBI_R2,		0x29272926},
	{IBI_R3,		0x23512350},
	{IBI_R4,		0x29292928},
	{IBI_R5,		0x20ff20ff},
	{IBI_R6,		0x20ff20ff},
	{IBI_R7,		0x20ff20ff},
	{IBI_R9,		0xffffff3f},
	{IBI_R11,	0x0},

	{AGC1_R0,	0x00021400},
	{AGC1_R1,	0x00000001},
	{AGC1_R2,	0x003A6464},
	{AGC1_R8,	0x132C40C0},
	{AGC1_R9,	0x132C40C0},
	{AGC1_R12,	0x703028F9},

	{AGC1_R22,	0x00001E21},
	{AGC1_R23,	0x0000272C},
	{AGC1_R24,	0x00002F3A},
	{AGC1_R25,	0x8000005a},
	{AGC1_R27,	0x000000C8},
	{AGC1_R28,	0x00060806},

	{AGC1_R32,	0x00002828},
	{AGC1_R33,	0x00003218},
	{AGC1_R34,	0x000A0008},
	{AGC1_R37,	0x2121262C},
	{AGC1_R39,	0x2A2A3036},

	{AGC1_R41,	0x38383E45},
	{AGC1_R49,	0x3333393E},

	{AGC1_R57,	0x00001010},

	{AGC1_R61,	0x404C362C},
	{AGC1_R62,	0x00362E2C},
	{AGC1_R63,	0x00006E30},

	{TXC_R1, 	0x0},
	
	{RXC_R1,	0x00000012},
	{RXC_R2,	0x00000011},
	{RXC_R3,	0x00000005},
	{RXC_R4,	0x0},

	{TXO_R8,	0x0},

	{TXBE_R0,	0x0},
	{TXBE_R2,	0x0},
	{TXBE_R3,	0x0},
	{TXBE_R4,	0x00000008},
	{TXBE_R6,	0x0},
	{TXBE_R8,	0x00000014},
	{TXBE_R9,	0x20000000},
	{TXBE_R10,	0x0},
	{TXBE_R12,	0x0},
	{TXBE_R13,	0x0},
	{TXBE_R14,	0x0},
	{TXBE_R15,	0x0},
	{TXBE_R16,	0x0},
	{TXBE_R17,	0x0},

	{RXFE_R0,	0x006000e0},
	{RXFE_R3,	0x0},
	{RXFE_R4,	0x0},

	{RXO_R13,	0x00000012},
	{RXO_R14,	0x00060612},
	{RXO_R15,	0xC8321918 /*0xC8321B1C*/},	/* disable REMOD and MLD for E1 */
	{RXO_R16,	0x0000001E},
	{RXO_R17,	0x0},
	{RXO_R21,	0x00000001},
	{RXO_R24,	0x00000006},
	{RXO_R28,	0x0000003F},
};


static RTMP_REG_PAIR RT85592_BBPRegTb_GBand[]={
	{AGC1_R4,	0x1FEA9C48},
	{AGC1_R5,	0x1FEA9C48},
	{AGC1_R13,	0x3D120412},
	{AGC1_R14,	0x00003202},

	{AGC1_R26,	0x00602004},

	{AGC1_R30,	0x00000006},
	{AGC1_R31,	0x00000FC3},
	{AGC1_R35,	0x11111116},

	{AGC1_R43,	0x27273438},
	{AGC1_R45,	0x26263034},
	{AGC1_R47,	0x26263438},
	
	{AGC1_R51,	0x17171C1C},
	{AGC1_R53,	0x26262A2F},
	{AGC1_R55,	0x40404E58},
	{AGC1_R58,	0x00001010},
	{AGC1_R59,	0x004B28FF},

	{AGC1_R60,	0x0000FFFF},
};
static UCHAR RT85592_BBP_GBand_Tb_Size = (sizeof(RT85592_BBPRegTb_GBand) / sizeof(RTMP_REG_PAIR));


static RTMP_REG_PAIR RT85592_BBPRegTb_ABand[]={
	{AGC1_R4,	0x1FEA9445},
	{AGC1_R5,	0x1FEA9445},
	{AGC1_R13,	0x2C3A0412},
	{AGC1_R14,	0x00002A02},

	{AGC1_R26,	0x005a2004},

	{AGC1_R30,	0x0},
	{AGC1_R31,	0x00000FD7},
	{AGC1_R35,	0x11111F16},

	{AGC1_R43,	0x27272B30},
	{AGC1_R45,	0x26262C31},
	{AGC1_R47,	0x26262B30},
	
	{AGC1_R51,	0xFFFFFFFF},
	{AGC1_R53,	0xFFFFFFFF},
	{AGC1_R55,	0xFFFFFFFF},
	{AGC1_R58,	0x0},
	{AGC1_R59,	0x004B287D},

	{AGC1_R60,	0x00003214},
};
static UCHAR RT85592_BBP_ABand_Tb_Size = (sizeof(RT85592_BBPRegTb_ABand) / sizeof(RTMP_REG_PAIR));


static REG_PAIR rf_init_tb[]={
	{RF_R00,		0x80},
	{RF_R01,		0x3f},
	{RF_R02,		0x80},
	{RF_R03,		0x8},
	{RF_R05,		0x0},
	{RF_R07,		0x1b},
	{RF_R10,		0x98},

	{RF_R13,		0x41},
	{RF_R14,		0x08},
	{RF_R15,		0x40},
	{RF_R16,		0x07},
/*	{RF_R17,		0x26},*/
	{RF_R18,		0x03},
	{RF_R19,		0x08},

	{RF_R21,		0x8D},
	{RF_R26,		0x82},
	{RF_R27,		0x42},
	{RF_R28,		0x00},
	{RF_R29,		0x10},

	{RF_R30,		0x0C},
	{RF_R31,		0x38},
	{RF_R33,		0x00},
	{RF_R34,		0x07},
	{RF_R35,		0x12},

	{RF_R37,		0x84},

	{RF_R47,		0x0C},

	{RF_R53,		0x44},

	{RF_R63,		0x07},
};
static UCHAR rf_init_tb_sz = (sizeof(rf_init_tb) / sizeof(REG_PAIR));


static REG_PAIR_PHY rf_adapt_tb[]={
/*     reg, 	s_ch,e_ch, phy, bw, 	val */
	{RF_R06,	   1,	 14,	0xff, 0xff, 0x70},
	{RF_R06,	 36, 196,	0xff, 0xff, 0xF0},	

	{RF_R11,	   1,	 14, 0xff, 0xff, 0x48},
	{RF_R11,	 36, 196, 0xff, 0xff, 0x40},

	{RF_R12,	   1,	 14, 0xff, 0xff, 0x78},
	{RF_R12,	 36,  64, 0xff, 0xff, 0xEC},
	{RF_R12,	100,165, 0xff, 0xff, 0xEC},

	{RF_R20,	   1,	 14, 0xff, 0xff, 0x81},
	{RF_R20,	 36, 165, 0xff, 0xff, 0x01},

	{RF_R22,	   1,	 14, 0xff, RF_BW_20 | RF_BW_10, 0x70},
	{RF_R22,	   1,  14, 0xff, RF_BW_40, 0x75},
	{RF_R22,	   1,  14, 0xff, RF_BW_80, 0x7A},
	{RF_R22,	 36,165, 0xff, RF_BW_20 | RF_BW_10, 0xE0},
	{RF_R22,	 36,165, 0xff, RF_BW_40, 0xE5},
	{RF_R22,	 36,165, 0xff, RF_BW_80, 0xEA},

	{RF_R23,	   1,	 10, 0xff, 0xff, 0x08},
	{RF_R23,	 11,	 14, 0xff, 0xff, 0x07},
	{RF_R23,	 36,  64, 0xff, 0xff, 0xFB},
	{RF_R23,	100,112, 0xff, 0xff, 0xFC},
	{RF_R23,	114,130, 0xff, 0xff, 0xFA},
	{RF_R23,	132,140, 0xff, 0xff, 0xF7},
	{RF_R23,	149,165, 0xff, 0xff, 0xF2},

	{RF_R24,	   1,	 14, 0xff, 0xff, 0x4A},
	{RF_R24,	 36,  64, 0xff, 0xff, 0x07},
	{RF_R24,	100,130, 0xff, 0xff, 0x02},
	{RF_R24,	132,140, 0xff, 0xff, 0x01},
	{RF_R24,	149,165, 0xff, 0xff, 0x00},

	{RF_R25,	   1,	 14, 0xff, 0xff, 0x80},
//+++Add by shiang for testing
	{RF_R25,	 36, 165, 0xff, 0xff, 0xB8},
//---Add by shiang for testing
	{RF_R32,	   1, 165, RF_MODE_CCK, 0xff, 0x80},
	{RF_R32,	   1, 165, RF_MODE_OFDM, 0xff, 0x40},

	{RF_R36,	   1,	 14, 0xff, 0xff, 0x80},
	{RF_R36,	 36, 165, 0xff, 0xff, 0x00},

	{RF_R38,	   1,	 14, 0xff, 0xff, 0x89},
	{RF_R38,	 36, 165, 0xff, 0xff, 0x85},
	
	{RF_R39,	   1,	 14, 0xff, 0xff, 0x1B},
	{RF_R39,	 36,  64, 0xff, 0xff, 0x1C},
	{RF_R39,	100,138, 0xff, 0xff, 0x1A},
	{RF_R39,	140,165, 0xff, 0xff, 0x18},

	{RF_R40,	   1,	 14, 0xff, 0xff, 0x0D},
	{RF_R40,	 36, 165, 0xff, 0xff, 0x42},

	{RF_R41,	   1,	 14, 0xff, 0xff, 0x9B},
	{RF_R41,	 36, 165, 0xff, 0xff, 0xAB},

	{RF_R42,	   1,	 14, 0xff, 0xff, 0xD5},
	{RF_R42,	 36, 165, 0xff, 0xff, 0xD7},
	
	{RF_R43,	   1,	 14, 0xff, 0xff, 0x72},
	{RF_R43,	 36,  64, 0xff, 0xff, 0x5B},
	{RF_R43,	100,138, 0xff, 0xff, 0x3B},
	{RF_R43,	140,165, 0xff, 0xff, 0x1B},

	{RF_R44,	   1,	 14, 0xff, 0xff, 0x1E},
	{RF_R44,	 36,  64, 0xff, 0xff, 0x42},
	{RF_R44,	100,112, 0xff, 0xff, 0x22},
	{RF_R44,	114,130, 0xff, 0xff, 0x1A},
	{RF_R44,	132,140, 0xff, 0xff, 0x22},
	{RF_R44,	149,165, 0xff, 0xff, 0x12},

	{RF_R45,	   1,	 14, 0xff, 0xff, 0x6A},
	{RF_R45,	 36, 138, 0xff, 0xff, 0x19},
	{RF_R45,	140,165, 0xff, 0xff, 0x11},

	{RF_R46,	   1,	 14, 0xff, 0xff, 0x6B},
	{RF_R46,	 36,  64, 0xff, 0xff, 0x88},
	{RF_R46,	100,138, 0xff, 0xff, 0x90},
	{RF_R46,	140,165, 0xff, 0xff, 0x88},

	{RF_R48,	   1,	 14, 0xff, 0xff, 0x10},
	{RF_R48,	 36, 165, 0xff, 0xff, 0x00},

	{RF_R51,	   1,	 14, 0xff, 0xff, 0x3E},
	{RF_R51,	 36,  64, 0xff, 0xff, 0x9E},
	{RF_R51,	100,130, 0xff, 0xff, 0x9D},
	{RF_R51,	132,165, 0xff, 0xff, 0x9C},

	{RF_R52,	   1,	 14, 0xff, 0xff, 0x48},
	{RF_R52,	 36,  64, 0xff, 0xff, 0x0C},
	{RF_R52,	100,165, 0xff, 0xff, 0x04},
	
	{RF_R54,	   1,	 14, 0xff, 0xff, 0x48},
	{RF_R54,	 36, 130, 0xff, 0xff, 0x98},
	{RF_R54,	132,140, 0xff, 0xff, 0x9B},
	{RF_R54,	149,165, 0xff, 0xff, 0x99},

	{RF_R55,	   1,	 14, RF_MODE_CCK, 0xff, 0x47},
	{RF_R55,	   1,	 14, RF_MODE_OFDM, 0xff, 0x43},
	{RF_R55,	 36, 112, 0xff, 0xff, 0x04},
	{RF_R55,	114,130, 0xff, 0xff, 0x03},
	{RF_R55,	132,165, 0xff, 0xff, 0x02},

	{RF_R56,	   1,	 14, 0xff, 0xff, 0xA1},
	{RF_R56,	 36,  64, 0xff, 0xff, 0xC4},
	{RF_R56,	100,130, 0xff, 0xff, 0x94},
	{RF_R56,	132,140, 0xff, 0xff, 0x8C},
	{RF_R56,	149,165, 0xff, 0xff, 0x94},
	
	{RF_R57,	   1,	 14, 0xff, 0xff, 0x00},
	{RF_R57,	 36, 165, 0xff, 0xff, 0xFF},

	{RF_R58,	   1,	 14, 0xff, 0xff, 0x39},
	{RF_R58,	 36,  64, 0xff, 0xff, 0x11},
	{RF_R58,	100,140, 0xff, 0xff, 0x19},
	{RF_R58,	149,165, 0xff, 0xff, 0x11},

	{RF_R59,	   1,	  9, 0xff, 0xff, 0x06},
	{RF_R59,	  10,	 14, 0xff, 0xff, 0x04},
	{RF_R59,	 36,  64, 0xff, 0xff, 0xFA},
	{RF_R59,	100,112, 0xff, 0xff, 0xFC},
	{RF_R59,	114,140, 0xff, 0xff, 0xFA},
	{RF_R59,	149,165, 0xff, 0xff, 0xF8},
	
	{RF_R60,	   1,	 14, 0xff, 0xff, 0x45},
	{RF_R60,	 36, 165, 0xff, 0xff, 0x05},
	
	{RF_R61,	   1,	 14, 0xff, 0xff, 0x49},
	{RF_R61,	 36, 165, 0xff, 0xff, 0x01},

	{RF_R62,	   1,	 14, 0xff, 0xff, 0x39},
	{RF_R62,	 36, 165, 0xff, 0xff, 0x11},
};
static INT rf_adapt_tb_sz = (sizeof(rf_adapt_tb) / sizeof(REG_PAIR_PHY));

/*
	NOTE: MAX_NUM_OF_CHANNELS shall  equal sizeof(txpwr_chlist))
*/
static UCHAR txpwr_chlist[] = {
	1, 2,3,4,5,6,7,8,9,10,11,12,13,14,
	36,38,40,44,46,48,52,54,56,60,62,64,
	100,102,104,108,110,112,116,118,120,124,126,128,132,134,136,140,
	149,151,153,157,159,161,165,167,169,171,173,
	42, 58, 106, 122, 155,
};

INT RT85592_ReadChannelPwr(RTMP_ADAPTER *pAd)
{
	UINT32 i, choffset, idx, ss_idx, ss_offset_g[2], ss_num;
	EEPROM_TX_PWR_STRUC Power;
	EEPROM_TX_PWR_STRUC Power2;
	CHAR tx_pwr1, tx_pwr2;

	/* Read Tx power value for all channels*/
	/* Value from 1 - 0x7f. Default value is 24.*/
	/* Power value : 2.4G 0x00 (0) ~ 0x1F (31)*/
	/*             : 5.5G 0xF9 (-7) ~ 0x0F (15)*/

	DBGPRINT(RT_DEBUG_TRACE, ("%s()--->\n", __FUNCTION__));
	choffset = 0;
	ss_num = 2;
	for (i = 0; i < sizeof(txpwr_chlist); i++)
	{
		pAd->TxPower[i].Channel = txpwr_chlist[i];
		pAd->TxPower[i].Power = DEFAULT_RF_TX_POWER;
		pAd->TxPower[i].Power2 = DEFAULT_RF_TX_POWER;
	}

	/* 0. 11b/g, ch1 - ch 14, 2SS */
	ss_offset_g[0] = EEPROM_G_TX_PWR_OFFSET;
	ss_offset_g[1] = EEPROM_G_TX2_PWR_OFFSET;
	for (ss_idx = 0; ss_idx < 2; ss_idx++)
	{
		for (i = 0; i < 7; i++)
		{
			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, ss_offset_g[ss_idx] + idx, Power.word);

			tx_pwr1 = tx_pwr2 = DEFAULT_RF_TX_POWER;
			if ((Power.field.Byte0 <= 0x27) && (Power.field.Byte0 >= 0))
				tx_pwr1 = Power.field.Byte0;
			if ((Power.field.Byte1 <= 0x27) || (Power.field.Byte1 >= 0))
				tx_pwr2 = Power.field.Byte1;

			if (ss_idx == 0) {
				pAd->TxPower[idx].Power = tx_pwr1;
				pAd->TxPower[idx + 1].Power = tx_pwr2;
			}
			else {
				pAd->TxPower[idx].Power2 = tx_pwr1;
				pAd->TxPower[idx + 1].Power2 = tx_pwr2;
			}
			choffset++;
		}
	}

	{
		/* 1. U-NII lower/middle band: 36, 38, 40; 44, 46, 48; 52, 54, 56; 60, 62, 64 (including central frequency in BW 40MHz)*/
		ASSERT((pAd->TxPower[choffset].Channel == 36));
		choffset = 14;
		ASSERT((pAd->TxPower[choffset].Channel == 36));
		for (i = 0; i < 6; i++)
		{
			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + idx, Power.word);
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + idx, Power2.word);

			if ((Power.field.Byte0 < 0x20) && (Power.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power = Power.field.Byte0;

			if ((Power.field.Byte1 < 0x20) && (Power.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power = Power.field.Byte1;			

			if ((Power2.field.Byte0 < 0x20) && (Power2.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power2 = Power2.field.Byte0;

			if ((Power2.field.Byte1 < 0x20) && (Power2.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power2 = Power2.field.Byte1;
		}

		/* 2. HipperLAN 2 100, 102 ,104; 108, 110, 112; 116, 118, 120; 124, 126, 128; 132, 134, 136; 140 (including central frequency in BW 40MHz)*/
		choffset = 14 + 12;
		ASSERT((pAd->TxPower[choffset].Channel == 100));
		for (i = 0; i < 8; i++)
		{
			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + idx, Power.word);
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + (choffset - 14) + idx, Power2.word);

			if ((Power.field.Byte0 < 0x20) && (Power.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power = Power.field.Byte0;

			if ((Power.field.Byte1 < 0x20) && (Power.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power = Power.field.Byte1;

			if ((Power2.field.Byte0 < 0x20) && (Power2.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power2 = Power2.field.Byte0;

			if ((Power2.field.Byte1 < 0x20) && (Power2.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power2 = Power2.field.Byte1;
		}

		/* 3. U-NII upper band: 149, 151, 153; 157, 159, 161; 165, 167, 169; 171, 173 (including central frequency in BW 40MHz)*/
		choffset = 14 + 12 + 16;
		ASSERT((pAd->TxPower[choffset].Channel == 149));
		for (i = 0; i < 6; i++)
		{
			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + idx, Power.word);
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + (choffset - 14) + idx, Power2.word);

			if ((Power.field.Byte0 < 0x20) && (Power.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power = Power.field.Byte0;

			if ((Power.field.Byte1 < 0x20) && (Power.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power = Power.field.Byte1;

			if ((Power2.field.Byte0 < 0x20) && (Power2.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power2 = Power2.field.Byte0;

			if ((Power2.field.Byte1 < 0x20) && (Power2.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power2 = Power2.field.Byte1;
		}

		/* choffset = 14 + 12 + 16 + 7; */
		choffset = 14 + 12 + 16 + 11;

#ifdef DOT11_VHT_AC
		ASSERT((pAd->TxPower[choffset].Channel == 42));

		// TODO: shiang-6590, fix me for the TxPower setting code here!
		/* For VHT80MHz, we need assign tx power for central channel 42, 58, 106, 122, and 155 */
		DBGPRINT(RT_DEBUG_TRACE, ("%s: Update Tx power control of the central channel (42, 58, 106, 122 and 155) for VHT BW80\n", __FUNCTION__));
		
		NdisMoveMemory(&pAd->TxPower[53], &pAd->TxPower[16], sizeof(CHANNEL_TX_POWER)); // channel 42 = channel 40
		NdisMoveMemory(&pAd->TxPower[54], &pAd->TxPower[22], sizeof(CHANNEL_TX_POWER)); // channel 58 = channel 56
		NdisMoveMemory(&pAd->TxPower[55], &pAd->TxPower[28], sizeof(CHANNEL_TX_POWER)); // channel 106 = channel 104
		NdisMoveMemory(&pAd->TxPower[56], &pAd->TxPower[34], sizeof(CHANNEL_TX_POWER)); // channel 122 = channel 120
		NdisMoveMemory(&pAd->TxPower[57], &pAd->TxPower[44], sizeof(CHANNEL_TX_POWER)); // channel 155 = channel 153

		pAd->TxPower[choffset].Channel = 42;
		pAd->TxPower[choffset+1].Channel = 58;
		pAd->TxPower[choffset+2].Channel = 106;
		pAd->TxPower[choffset+3].Channel = 122;
		pAd->TxPower[choffset+4].Channel = 155;

		choffset += 5;		/* the central channel of VHT80 */
		choffset = (MAX_NUM_OF_CHANNELS - 1);
#endif /* DOT11_VHT_AC */

		/* 4. Print and Debug*/
		for (i = 0; i < choffset; i++)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("E2PROM: TxPower[%03d], Channel=%d, Power[Tx0:%d, Tx1:%d]\n",
						i, pAd->TxPower[i].Channel, pAd->TxPower[i].Power, pAd->TxPower[i].Power2 ));
		}
	}

	return TRUE;
}


/*
	==========================================================================
	Description:

	Reverse RF sleep-mode setup

	==========================================================================
 */
static VOID RT85592ReverseRFSleepModeSetup(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN FlgIsInitState)
{
	UCHAR	RFValue;

	RT30xxReadRFRegister(pAd, RF_R01, &RFValue);
	RFValue = ((RFValue & ~0x3F) | 0x3F);
	RT30xxWriteRFRegister(pAd, RF_R01, RFValue);

	RT30xxReadRFRegister(pAd, RF_R06, &RFValue);
	//RFValue = 0xE4;
	RFValue = ((RFValue & ~0xC0) | 0xC0); /* vco_ic (VCO bias current control, 11: high) */
	RT30xxWriteRFRegister(pAd, RF_R06, RFValue);

	RT30xxReadRFRegister(pAd, RF_R02, &RFValue);
	RFValue = ((RFValue & ~0x80) | 0x80); /* rescal_en (initiate calibration) */
	RT30xxWriteRFRegister(pAd, RF_R02, RFValue);
	
	RT30xxReadRFRegister(pAd, RF_R22, &RFValue);
	RFValue = ((RFValue & ~0xE0) | 0x20); /* cp_ic (reference current control, 001: 0.33 mA) */
	RT30xxWriteRFRegister(pAd, RF_R22, RFValue);

	RT30xxReadRFRegister(pAd, RF_R42, &RFValue);
	RFValue = ((RFValue & ~0x40) | 0x40); /* rx_ctb_en */
	RT30xxWriteRFRegister(pAd, RF_R42, RFValue);

	RT30xxReadRFRegister(pAd, RF_R20, &RFValue);
	RFValue = ((RFValue & ~0x77) | 0x10); /* ldo_rf_vc(0) and ldo_pll_vc(111: +0.05) */
	RT30xxWriteRFRegister(pAd, RF_R20, RFValue);

	RT30xxReadRFRegister(pAd, RF_R03, &RFValue);
	RFValue = ((RFValue & ~0x80) | 0x80); /* vcocal_en (initiate VCO calibration (reset after completion)) */
	RT30xxWriteRFRegister(pAd, RF_R03, RFValue);
}


/*
	==========================================================================
	Description:

	Load RF sleep-mode setup

	==========================================================================
 */
static VOID RT85592LoadRFSleepModeSetup(RTMP_ADAPTER *pAd)
{
	UCHAR	rfreg;

	// TODO: shiang-6590, what should I do for this??
	return;
	
	/* Disabe rf_block */
	RT30xxReadRFRegister(pAd, RF_R01, &rfreg);
	if ((pAd->Antenna.field.TxPath == 2) && (pAd->Antenna.field.RxPath == 2))
		rfreg = ((rfreg & ~0x3F) | 0x3F);
	else
		rfreg = ((rfreg & ~0x0F) | 0x0F); // Enable rf_block_en, pll_en, rx0_en and tx0_en
	RT30xxWriteRFRegister(pAd, RF_R01, rfreg);

	RT30xxReadRFRegister(pAd, RF_R06, &rfreg);
	rfreg = ((rfreg & ~0xC0) | 0xC0); /* vco_ic (VCO bias current control, 00: off) */
	RT30xxWriteRFRegister(pAd, RF_R06, rfreg);

	RT30xxReadRFRegister(pAd, RF_R22, &rfreg);
	rfreg = 0xEA; 	/* Set 0xEA for A band 80BW */
	RT30xxWriteRFRegister(pAd, RF_R22, rfreg);

	RT30xxReadRFRegister(pAd, RF_R42, &rfreg);
	rfreg = 0xD7; /* rx_ctb_en */
	RT30xxWriteRFRegister(pAd, RF_R42, rfreg);

	RT30xxReadRFRegister(pAd, RF_R20, &rfreg);
	rfreg = 0x01; /* ldo_pll_vc and ldo_rf_vc (111: -0.15) */
	RT30xxWriteRFRegister(pAd, RF_R20, rfreg);

	RT30xxReadRFRegister(pAd, RF_R03, &rfreg);
	rfreg = ((rfreg & ~0x80) | 0x80); // vcocal_en (initiate VCO calibration (reset after completion))
	RT30xxWriteRFRegister(pAd, RF_R03, rfreg);
}


VOID RT85592LoadRFNormalModeSetup(RTMP_ADAPTER *pAd)
{
	UCHAR RFValue;
	UINT32 bbp_val;

	RT30xxReadRFRegister(pAd, RF_R38, &RFValue);
	RFValue = ((RFValue & ~0x20) | 0x00); /* rx_lo1_en (enable RX LO1, 0: LO1 follows TR switch) */
	RT30xxWriteRFRegister(pAd, RF_R38, RFValue);

	RT30xxReadRFRegister(pAd, RF_R39, &RFValue);
	RFValue = ((RFValue & ~0x80) | 0x00); /* rx_lo2_en (enable RX LO2, 0: LO2 follows TR switch) */
	RT30xxWriteRFRegister(pAd, RF_R39, RFValue);

	RTMP_BBP_IO_READ32(pAd, CORE_R1, &bbp_val);
	bbp_val |= 0x40;
	RTMP_BBP_IO_WRITE32(pAd, CORE_R1, bbp_val);

}


static VOID NICInitRT85592RFRegisters(RTMP_ADAPTER *pAd)
{
	INT IdReg;
	UCHAR rf_val;

// TODO: shiang-8592, clear RF_CSR_REG busy bit first and try to work-around it in RT6856 platform
	{
		RF_CSR_CFG_STRUC reg_val = {.word = 0};

		RTMP_IO_READ32(pAd, RF_CSR_CFG, &reg_val.word);
		if (reg_val.word != 0)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s(): Before Init RF, RF_CSR_CFG.word = 0x%x, RF_CSR_CFG.Kick=%d\n",
										__FUNCTION__, reg_val.word, reg_val.field.RF_CSR_KICK));
			RTMP_IO_WRITE32(pAd, RF_CSR_CFG, 0);
			RTMP_IO_READ32(pAd, RF_CSR_CFG, &reg_val.word);
			DBGPRINT(RT_DEBUG_ERROR, ("%s(): After write RF_CSR_CFG, read value back=0x%x\n",
										__FUNCTION__, reg_val.word));
		}
	}
// TODO: ---End

	/* Init RF calibration, toggle bit 7 before init RF registers */
	RT30xxReadRFRegister(pAd, RF_R02, (PUCHAR)&rf_val);
	rf_val = ((rf_val & ~0x80) | 0x80);
	RT30xxWriteRFRegister(pAd, RF_R02, (UCHAR)rf_val);
	RTMPusecDelay(1000);
	rf_val = ((rf_val & ~0x80) | 0x00);
	RT30xxWriteRFRegister(pAd, RF_R02, (UCHAR)rf_val);

	DBGPRINT(RT_DEBUG_TRACE, ("%s(): rf_init_tb_sz=%d\n", __FUNCTION__, rf_init_tb_sz));
	for(IdReg=0; IdReg<rf_init_tb_sz; IdReg++)
	{
		RT30xxWriteRFRegister(pAd, rf_init_tb[IdReg].Register,
				rf_init_tb[IdReg].Value);
		DBGPRINT(RT_DEBUG_TRACE, ("%s(): Write RF_R%02d=%02x\n",
					__FUNCTION__, rf_init_tb[IdReg].Register,
					rf_init_tb[IdReg].Value));
	}
	RT85592LoadRFNormalModeSetup(pAd);

	return;
}


/*
========================================================================
Routine Description:
	Initialize specific MAC registers.

Arguments:
	pAd					- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
static VOID NICInitRT85592MacRegisters(RTMP_ADAPTER *pAd)
{
	INT IdReg;

	for(IdReg=0; IdReg<RT85592_NUM_MAC_REG_PARMS; IdReg++)
	{
		RTMP_IO_WRITE32(pAd, RT85592_MACRegTable[IdReg].Register,
				RT85592_MACRegTable[IdReg].Value);
	}
}


/*
========================================================================
Routine Description:
	Initialize specific BBP registers.

Arguments:
	pAd					- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
static VOID NICInitRT85592BbpRegisters(RTMP_ADAPTER *pAd)
{
	INT IdReg;

	for(IdReg=0; IdReg < pAd->chipCap.bbpRegTbSize; IdReg++)
	{
		RTMP_BBP_IO_WRITE32(pAd, RT85592_BBPRegTb[IdReg].Register,
				RT85592_BBPRegTb[IdReg].Value);
	}
	
	return;
}


static VOID RT85592_AsicAntennaDefaultReset(
	IN struct _RTMP_ADAPTER	*pAd,
	IN EEPROM_ANTENNA_STRUC *pAntenna)
{	
	pAntenna->word = 0;
	pAntenna->field.RfIcType = 0xf;
	pAntenna->field.TxPath = 2;
	pAntenna->field.RxPath = 2;
}


static VOID RT85592_ChipBBPAdjust(RTMP_ADAPTER *pAd)
{
	static char *ext_str[]={"extNone", "extAbove", "", "extBelow"};
	UCHAR rf_bw, ext_ch;


#ifdef DOT11_N_SUPPORT
	if (get_ht_cent_ch(pAd, &rf_bw, &ext_ch) == FALSE)
#endif /* DOT11_N_SUPPORT */
	{
		rf_bw = BW_20;
		ext_ch = EXTCHA_NONE;
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel;
	}

#ifdef DOT11_VHT_AC
	if (WMODE_CAP(pAd->CommonCfg.PhyMode, WMODE_AC) &&
		(pAd->CommonCfg.Channel > 14) &&
		(rf_bw == BW_40) &&
		(pAd->CommonCfg.vht_bw == VHT_BW_80) &&
		(pAd->CommonCfg.vht_cent_ch != pAd->CommonCfg.CentralChannel))
	{
		rf_bw = BW_80;
		pAd->CommonCfg.vht_cent_ch = vht_cent_ch_freq(pAd, pAd->CommonCfg.Channel);
	}

//+++Add by shiang for debug
	DBGPRINT(RT_DEBUG_OFF, ("%s():rf_bw=%d, ext_ch=%d, PrimCh=%d, HT-CentCh=%d, VHT-CentCh=%d\n",
				__FUNCTION__, rf_bw, ext_ch, pAd->CommonCfg.Channel,
				pAd->CommonCfg.CentralChannel, pAd->CommonCfg.vht_cent_ch));
//---Add by shiang for debug
#endif /* DOT11_VHT_AC */

	rtmp_bbp_set_bw(pAd, rf_bw);

	/* TX/Rx : control channel setting */
	rtmp_mac_set_ctrlch(pAd, ext_ch);
	rtmp_bbp_set_ctrlch(pAd, ext_ch);

#ifdef DOT11_N_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE, ("%s() : %s, ChannelWidth=%d, Channel=%d, ExtChanOffset=%d(%d) \n",
									__FUNCTION__, ext_str[ext_ch],
									pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth,
									pAd->CommonCfg.Channel,
									pAd->CommonCfg.RegTransmitSetting.field.EXTCHA,
									pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset));
#endif /* DOT11_N_SUPPORT */
}


INT set_freq_plan(RTMP_ADAPTER *pAd, UCHAR Channel)
{
	INT idx;
	UCHAR RFValue;

	for (idx = 0; idx < RT85592_FreqPlan_Sz; idx++)
	{
		if (Channel == RT85592_FreqPlan_Xtal40M[idx].Channel)
		{
			/* Frequeny plan setting */
			/* 
 			 * R setting 
 			 * R11[1:0]
 			 */
			RT30xxReadRFRegister(pAd, RF_R11, &RFValue);
			RFValue &= (~0x03);
			RFValue |= RT85592_FreqPlan_Xtal40M[idx].R;
			RT30xxWriteRFRegister(pAd, RF_R11, RFValue);

			/*
 			 * N setting
 			 * R9[4], R8[7:0] (RF PLL freq selection) 
 			 */
			//RT30xxReadRFRegister(pAd, RF_R08, &RFValue);
			RFValue = (RT85592_FreqPlan_Xtal40M[idx].N & 0x00ff);
			RT30xxWriteRFRegister(pAd, RF_R08, RFValue);

			RT30xxReadRFRegister(pAd, RF_R09, &RFValue);
			RFValue &= (~0x10);
			RFValue |= ((RT85592_FreqPlan_Xtal40M[idx].N & 0x100) >> 8) << 4;
			RT30xxWriteRFRegister(pAd, RF_R09, RFValue);
			
			/* 
 			 * K setting 
 			 * R7[7], R9[3:0] (RF PLL freq selection)
 			 */
			RT30xxReadRFRegister(pAd, RF_R09, &RFValue);
			RFValue &= (~0x0f);
			RFValue |= (RT85592_FreqPlan_Xtal40M[idx].K & 0x0f);
			RT30xxWriteRFRegister(pAd, RF_R09, RFValue);
			RT30xxReadRFRegister(pAd, RF_R07, &RFValue);
			RFValue &= (~0x80);
			RFValue |= ((RT85592_FreqPlan_Xtal40M[idx].K & 0x10) >> 4) << 7;
			RT30xxWriteRFRegister(pAd, RF_R07, RFValue);


			/* 
 			 * mode setting 
 			 * R9[7:5] (RF PLL freq selection)
 			 * R11[3:2] (RF PLL)
 			 */
			RT30xxReadRFRegister(pAd, RF_R11, &RFValue);
			RFValue &= (~0x0c);
			RFValue |= (RT85592_FreqPlan_Xtal40M[idx].mod & 0x3) << 2;
			RT30xxWriteRFRegister(pAd, RF_R11, RFValue);

			RT30xxReadRFRegister(pAd, RF_R09, &RFValue);
			RFValue &= (~0xe0);
			RFValue |= (RT85592_FreqPlan_Xtal40M[idx].mod >> 2) << 5;
			RT30xxWriteRFRegister(pAd, RF_R09, RFValue);
			break;
		}
	}

	return idx;
}


#ifdef CONFIG_STA_SUPPORT
static UCHAR RT85592_ChipAGCAdjust(RTMP_ADAPTER *pAd, CHAR Rssi, UCHAR old_val)
{
	// TODO: shiang-6590, not implement yet!!!

	return 0;
}
#endif // CONFIG_STA_SUPPORT //


static VOID RT85592_ChipAGCInit(RTMP_ADAPTER *pAd, UCHAR BandWidth)
{
	// TODO: shiang-6590, not implement yet!!!
	/* rtmp_bbp_set_agc(pAd, R66, RX_CHAIN_ALL); */
}


#define WINDOWS 0

VOID RT85592_ChipSwitchChannel(
	struct _RTMP_ADAPTER *pAd,
	UCHAR Channel,
	BOOLEAN	 bScan)
{
	CHAR TxPwer = 0, TxPwer2 = DEFAULT_RF_TX_POWER;
	UINT idx;
	UCHAR RFValue = 0, PreRFValue;
	UINT32 TxPinCfg;
	UCHAR rf_phy_mode, rf_bw = RF_BW_20;


	if (WMODE_EQUAL(pAd->CommonCfg.PhyMode, WMODE_B))
		rf_phy_mode = RF_MODE_CCK;
	else
		rf_phy_mode = RF_MODE_OFDM;

	if (pAd->CommonCfg.BBPCurrentBW == BW_80)
		rf_bw = RF_BW_80;
	else if (pAd->CommonCfg.BBPCurrentBW == BW_40)
		rf_bw = RF_BW_40;
	else
		rf_bw = RF_BW_20;
	
#ifdef CONFIG_AP_SUPPORT
#ifdef AP_QLOAD_SUPPORT
	/* clear all statistics count for QBSS Load */
	QBSS_LoadStatusClear(pAd);
#endif /* AP_QLOAD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	/*
		We can't use ChannelList to search channel, since some central channel's txpowr doesn't list
		in ChannelList, so use TxPower array instead.
	*/
	for (idx = 0; idx < MAX_NUM_OF_CHANNELS; idx++)
	{
		if (Channel == pAd->TxPower[idx].Channel)
		{
			TxPwer = pAd->TxPower[idx].Power;
			TxPwer2 = pAd->TxPower[idx].Power2;
			break;
		}
	}

	if (idx == MAX_NUM_OF_CHANNELS)
		DBGPRINT(RT_DEBUG_ERROR, ("%s(): Can't find the Channel#%d \n",
					__FUNCTION__, Channel));

	idx = set_freq_plan(pAd, Channel);
	if (idx < RT85592_FreqPlan_Sz)
		DBGPRINT(RT_DEBUG_TRACE,
				("%s(): Switch to Ch#%d(RF=%d, Pwr0=%d, Pwr1=%d, %dT), "
				"N=0x%02X, K=0x%02X, R=0x%02X, BBP_BW=%d\n",
				__FUNCTION__,
				Channel, pAd->RfIcType,
				TxPwer, TxPwer2,
				pAd->Antenna.field.TxPath,
				RT85592_FreqPlan_Xtal40M[idx].N,
				RT85592_FreqPlan_Xtal40M[idx].K,
				RT85592_FreqPlan_Xtal40M[idx].R,
				pAd->CommonCfg.BBPCurrentBW));

	/* Enable RF block */
	RT30xxReadRFRegister(pAd, RF_R01, &RFValue);
	/* Enable rf_block_en, pll_en */
	RFValue |= 0x3;
	RFValue &= (~0x28);
	if (pAd->Antenna.field.TxPath == 2)
		RFValue |= 0x28;	/* Enable tx0_en, tx1_en */
	else if (pAd->Antenna.field.TxPath == 1)
		RFValue |= 0x08;	/* Enable tx0_en */

	RFValue &= (~0x14);
	if (pAd->Antenna.field.RxPath == 2)
		RFValue |= 0x14;	/* Enable rx0_en, rx1_en */
	else if (pAd->Antenna.field.RxPath == 1)
		RFValue |= 0x04;	/* Enable rx0_en */
	RT30xxWriteRFRegister(pAd, RF_R01, RFValue);

	/* RF setting */
	for (idx = 0; idx < rf_adapt_tb_sz; idx++)
	{
		if ((Channel >= rf_adapt_tb[idx].s_ch) &&
			(Channel <= rf_adapt_tb[idx].e_ch) &&
			(rf_phy_mode & rf_adapt_tb[idx].phy) &&
			(rf_bw & rf_adapt_tb[idx].bw)
		)
		{
			RT30xxWriteRFRegister(pAd, rf_adapt_tb[idx].reg, 
					                   rf_adapt_tb[idx].val);
		}
	}


	if (Channel <= 14)
	{
		/* 
		 * R49 CH0 TX power ALC code(RF DAC value) 
		 * G-band bit<7:6>=1:0, bit<5:0> range from 0x0~0x27
		 */
		if (TxPwer > 0x27)
			TxPwer = 0x27;
		RFValue = 0x80 | TxPwer;
		RT30xxWriteRFRegister(pAd, RF_R49, RFValue);

		/* 
	 	 * R50 CH0 TX power ALC code(RF DAC value) 
	 	 * G-band bit<7:6>=1:0, bit<5:0> range from 0x0~0x27
	 	 */
		if (TxPwer2 > 0x27)
			TxPwer2 = 0x27;
		RFValue = 0x80 | TxPwer2;
		RT30xxWriteRFRegister(pAd, RF_R50, RFValue);
	}
	else
	{
		/* 
		 * R49 CH0 TX power ALC code(RF DAC value) 
		 * A-band bit<7:6>=0:1, bit<5:0> range from 0x0~0x2B
		 */
		if (TxPwer > 0x2B)
			TxPwer = 0x2B;
		RFValue = 0x40 | TxPwer;				
		RT30xxWriteRFRegister(pAd, RF_R49, RFValue);

		/* 
		 * R50 CH1 TX power ALC code(RF DAC value) 
		 * A-band bit<7:6>=0:1, bit<5:0> range from 0x0~0x2B
		 */
		if (TxPwer2 > 0x2B)
			TxPwer2 = 0x2B;
		RFValue = 0x40 | TxPwer2;
		RT30xxWriteRFRegister(pAd, RF_R50, RFValue);
	}

	
	RT30xxReadRFRegister(pAd, RF_R17, &RFValue);
	PreRFValue = RFValue;
#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
	if( (pAd->FreqCalibrationCtrl.bEnableFrequencyCalibration == TRUE) && INFRA_ON(pAd))
		RFValue = ((RFValue & ~0x7F) | (pAd->FreqCalibrationCtrl.AdaptiveFreqOffset & 0x7F));
	else
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
		RFValue = ((RFValue & ~0x7F) | (pAd->RfFreqOffset & 0x7F)); 
	RFValue = min((INT)RFValue, 0x5F);
	if (PreRFValue != RFValue)
		RT30xxWriteRFRegister(pAd, RF_R17, (UCHAR)RFValue);

	RT30xxReadRFRegister(pAd, RF_R03, (PUCHAR)&RFValue);
	RFValue = ((RFValue & ~0x80) | 0x80); /* vcocal_en (initiate VCO calibration (reset after completion)) - It should be at the end of RF configuration. */
	RT30xxWriteRFRegister(pAd, RF_R03, (UCHAR)RFValue);
	pAd->LatchRfRegs.Channel = Channel; /* Channel latch */


// TODO: End---

	if (Channel <= 14)
	{
		for (idx = 0; idx < RT85592_BBP_GBand_Tb_Size; idx++)
			RTMP_BBP_IO_WRITE32(pAd, RT85592_BBPRegTb_GBand[idx].Register,
									RT85592_BBPRegTb_GBand[idx].Value);

		/* 5G band selection PIN, bit1 and bit2 are complement */
		rtmp_mac_set_band(pAd, BAND_24G);

		/* Turn off unused PA or LNA when only 1T or 1R */
		TxPinCfg = 0x00050F0A; /* Gary 2007/08/09 0x050A0A */
		if (pAd->Antenna.field.TxPath == 1)
			TxPinCfg &= 0xFFFFFFF3;
		if (pAd->Antenna.field.RxPath == 1)
			TxPinCfg &= 0xFFFFF3FF;

		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, TxPinCfg);

		if (WINDOWS)
		{
			TX_ATTENUATION_CTRL_STRUC TxAttenuationCtrl = {.word=0};

			// PCIe PHY Transmit attenuation adjustment
			RTMP_IO_READ32(pAd, PCIE_PHY_TX_ATTENUATION_CTRL, &TxAttenuationCtrl.word);
			if (Channel == 14) // Channel #14
			{
				TxAttenuationCtrl.field.PCIE_PHY_TX_ATTEN_EN = 1; // Enable PCIe PHY Tx attenuation
				TxAttenuationCtrl.field.PCIE_PHY_TX_ATTEN_VALUE = 4; // 9/16 full drive level
			}
			else // Channel #1~#13
			{
				TxAttenuationCtrl.field.PCIE_PHY_TX_ATTEN_EN = 0; // Disable PCIe PHY Tx attenuation
				TxAttenuationCtrl.field.PCIE_PHY_TX_ATTEN_VALUE = 0; // n/a
			}

			RTMP_IO_WRITE32(pAd, PCIE_PHY_TX_ATTENUATION_CTRL, TxAttenuationCtrl.word);
		}
	}
	else
	{
		for (idx = 0; idx < RT85592_BBP_ABand_Tb_Size; idx++)
			RTMP_BBP_IO_WRITE32(pAd, RT85592_BBPRegTb_ABand[idx].Register,
									RT85592_BBPRegTb_ABand[idx].Value);

		if (rf_bw == RF_BW_40)
		{
			RTMP_BBP_IO_WRITE32(pAd, AGC1_R35, 0x11111116);
		}

		// 5G band selection PIN, bit1 and bit2 are complement */
		rtmp_mac_set_band(pAd, BAND_5G);

		/* Turn off unused PA or LNA when only 1T or 1R */
		TxPinCfg = 0x00050F05;/* Gary 2007/8/9 0x050505 */
		if (pAd->Antenna.field.TxPath == 1)
			TxPinCfg &= 0xFFFFFFF3;

		if (pAd->Antenna.field.RxPath == 1)
			TxPinCfg &= 0xFFFFF3FF;

		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, TxPinCfg);
	}


	/*
	  On 11A, We should delay and wait RF/BBP to be stable
	  and the appropriate time should be 1000 micro seconds
	  2005/06/05 - On 11G, We also need this delay time. Otherwise it's difficult to pass the WHQL.
	*/
	RTMPusecDelay(2000);

	//dump_bw_info(pAd);
	
}


#ifdef CONFIG_STA_SUPPORT
static VOID RT85592_NetDevNickNameInit(
	IN struct _RTMP_ADAPTER *pAd)
{
	snprintf((PSTRING) pAd->nickname, sizeof(pAd->nickname), "RT85592STA");
}
#endif /* CONFIG_STA_SUPPORT */


/*
========================================================================
Routine Description:
	Initialize RT85592.

Arguments:
	pAd					- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RT85592_Init(RTMP_ADAPTER *pAd)
{
	RTMP_CHIP_OP *pChipOps = &pAd->chipOps;
	RTMP_CHIP_CAP *pChipCap = &pAd->chipCap;


	DBGPRINT(RT_DEBUG_TRACE, ("-->%s():\n", __FUNCTION__));
	
	pAd->RfIcType = RFIC_UNKNOWN;
	/* 
		Init chip capabilities
	*/
	pChipCap->MaxNss = 1;
	pChipCap->TXWISize = 20;
	pChipCap->RXWISize = 28;
	pChipCap->WPDMABurstSIZE = 3;

	pChipCap->FlgIsHwWapiSup = TRUE;
	// TODO: ++++++following parameters are not finialized yet!!

#ifdef RTMP_FLASH_SUPPORT
	pChipCap->eebuf = RT85592_EeBuffer;
#endif /* RTMP_FLASH_SUPPORT */
	pChipCap->SnrFormula = SNR_FORMULA2;
	pChipCap->VcoPeriod = 10;
	pChipCap->FlgIsVcoReCalMode = VCO_CAL_MODE_2;
	pChipCap->FlgIsHwAntennaDiversitySup = FALSE;
#ifdef STREAM_MODE_SUPPORT
	pChipCap->FlgHwStreamMode = FALSE;
#endif /* STREAM_MODE_SUPPORT */
#ifdef TXBF_SUPPORT
	pChipCap->FlgHwTxBfCap = FALSE;
#endif /* TXBF_SUPPORT */
#ifdef FIFO_EXT_SUPPORT
	pChipCap->FlgHwFifoExtCap = TRUE;
#endif /* FIFO_EXT_SUPPORT */


	pChipCap->asic_caps |= (fASIC_CAP_PMF_ENC);

	pChipCap->phy_caps = (fPHY_CAP_24G | fPHY_CAP_5G);
	pChipCap->phy_caps |= (fPHY_CAP_HT | fPHY_CAP_VHT);

	pChipCap->RfReg17WtMethod = RF_REG_WT_METHOD_STEP_ON;

	pChipCap->MaxNumOfRfId = 63;
	pChipCap->pRFRegTable = rf_init_tb;

	pChipCap->MaxNumOfBbpId = 200;	
	pChipCap->pBBPRegTable = (REG_PAIR *)RT85592_BBPRegTb;
	pChipCap->bbpRegTbSize = (sizeof(RT85592_BBPRegTb) / sizeof(RTMP_REG_PAIR));

#ifdef NEW_MBSSID_MODE
	pChipCap->MBSSIDMode = MBSSID_MODE1;
#else
	pChipCap->MBSSIDMode = MBSSID_MODE0;
#endif /* NEW_MBSSID_MODE */


#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
	// TODO: shiang-6590, fix me for FrqCal in STA mode!!
	/* Frequence Calibration */
	pChipCap->FreqCalibrationSupport = FALSE;
	pChipCap->FreqCalInitMode = 0;
	pChipCap->FreqCalMode = 0;
	pChipCap->RxWIFrqOffset = 0;
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

	RTMP_DRS_ALG_INIT(pAd, RATE_ALG_GRP);

	/*
		Following function configure beacon related parameters in pChipCap
			FlgIsSupSpecBcnBuf / BcnMaxHwNum / 
			WcidHwRsvNum / BcnMaxHwSize / BcnBase[]
	*/
	rlt_bcn_buf_init(pAd);

	// TODO: ------Upper parameters are not finialized yet!!



	/*
		init operator
	*/

	// TODO: ++++++following parameters are not finialized yet!!	
	/* BBP adjust */
	pChipOps->ChipBBPAdjust = RT85592_ChipBBPAdjust;
#ifdef CONFIG_STA_SUPPORT
	pChipOps->ChipAGCAdjust = NULL; /* RT85592_ChipAGCAdjust; */
#endif /* CONFIG_STA_SUPPORT */

	/* Channel */
	pChipOps->ChipSwitchChannel = RT85592_ChipSwitchChannel;
	pChipOps->ChipAGCInit = RT85592_ChipAGCInit;

	pChipOps->AsicMacInit = NICInitRT85592MacRegisters;
	pChipOps->AsicBbpInit = NICInitRT85592BbpRegisters;
	pChipOps->AsicRfInit = NICInitRT85592RFRegisters;
	pChipOps->AsicRfTurnOn = NULL;

	pChipOps->AsicHaltAction = NULL;
	pChipOps->AsicRfTurnOff = RT85592LoadRFSleepModeSetup;
	pChipOps->AsicReverseRfFromSleepMode = RT85592ReverseRFSleepModeSetup;
	
	pChipOps->AsicResetBbpAgent = NULL;
	
	/* MAC */

	/* EEPROM */
	pChipOps->NICInitAsicFromEEPROM = NULL;
	
	/* Antenna */
	pChipOps->AsicAntennaDefaultReset = RT85592_AsicAntennaDefaultReset;

	/* TX ALC */
	pChipOps->InitDesiredTSSITable = NULL;
 	pChipOps->ATETssiCalibration = NULL;
	pChipOps->ATETssiCalibrationExtend = NULL;
	pChipOps->AsicTxAlcGetAutoAgcOffset = NULL;
	pChipOps->ATEReadExternalTSSI = NULL;
	pChipOps->TSSIRatio = NULL;
	
	/* Others */
#ifdef CONFIG_STA_SUPPORT
	pChipOps->NetDevNickNameInit = RT85592_NetDevNickNameInit;
#endif /* CONFIG_STA_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	pAd->chipCap.carrier_func = TONE_RADAR_V2;
	pChipOps->ToneRadarProgram = ToneRadarProgram_v2;
#endif /* CARRIER_DETECTION_SUPPORT */

	/* Chip tuning */
	pChipOps->RxSensitivityTuning = NULL;
	pChipOps->AsicTxAlcGetAutoAgcOffset = NULL;
	pChipOps->AsicGetTxPowerOffset = AsicGetTxPowerOffset;
	
	
	
/* 
	Following callback functions already initiailized in RtmpChipOpsHook() 
	1. Power save related
*/
#ifdef GREENAP_SUPPORT
	pChipOps->EnableAPMIMOPS = EnableAPMIMOPSv2;
	pChipOps->DisableAPMIMOPS = DisableAPMIMOPSv2;
#endif /* GREENAP_SUPPORT */
	// TODO: ------Upper parameters are not finialized yet!!
	pChipCap->MCUType = ANDES;
}

#endif /* RT8592 */

