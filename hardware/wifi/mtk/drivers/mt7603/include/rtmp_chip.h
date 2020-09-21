/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rtmp_chip.h

	Abstract:
	Ralink Wireless Chip related definition & structures

	Revision History:
	Who			When		  What
	--------	----------	  ----------------------------------------------
*/

#ifndef	__RTMP_CHIP_H__
#define	__RTMP_CHIP_H__

#include "rtmp_type.h"
#include "eeprom.h"
#include "tx_power.h"

#ifdef RTMP_MAC_PCI
#include "iface/rtmp_reg_pcirbs.h"
#endif /* RTMP_MAC_PCI */

#include "iface/iface.h"

#include "mac/mac.h"
#include "mcu/mcu.h"

#ifdef RT2860
#include "chip/rt2860.h"
#include "chip/rt28xx.h"
#endif /* RT2860 */

#ifdef RT3090
#include "chip/rt3090.h"
#endif /* RT3090 */

#ifdef RT2870
#include "chip/rt2870.h"
#include "chip/rt28xx.h"
#endif /* RT2870 */

#ifdef RT3070
#include "chip/rt3070.h"
#endif /* RT3070 */

#ifdef RT2880
#include "chip/rt2880.h"
#include "chip/rt28xx.h"
#endif /* RT2880 */

#ifdef RT2883
#include "chip/rt2883.h"
#endif /* RT2883 */

#ifdef RT3883
#include "chip/rt3883.h"
#endif /* RT3883 */

#ifdef RT35xx
#include "chip/rt35xx.h"
#endif /* RT35xx */

#ifdef RT305x
#include "chip/rt305x.h"
#endif /* RT305x */

#ifdef RT3370
#include "chip/rt3370.h"
#endif /* RT3370 */

#ifdef RT3390
#include "chip/rt3390.h"
#endif /* RT3390 */

#ifdef RT3593
#include "chip/rt3593.h"
#include "chip/rt28xx.h"
#endif /* RT3593 */

#ifdef RT3352
#include "chip/rt3352.h"
#endif /* RT3352 */

#ifdef RT5350
#include "chip/rt5350.h"
#endif /* RT5350 */

#ifdef RT3572
#include "chip/rt28xx.h"
#endif /* RT3572 */

#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392)
#include "chip/rt5390.h"
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) */

#ifdef RT5592
#include "chip/rt5592.h"
#endif

#ifdef RT3290
#include "chip/rt3290.h"
#endif /* RT3290 */

#ifdef RT6352
#include "chip/rt6352.h"
#endif /* RT6352 */

#ifdef MT7628
#include "chip/mt7628.h"
#endif /* MT7628 */

#ifdef MT7601
#include "chip/mt7601.h"
#endif /* MT7601 */

#ifdef RT65xx
#include "chip/rt65xx.h"

#ifdef MT76x0
#include "chip/mt76x0.h"
#endif /* MT76x0 */

#ifdef MT76x2
#include "chip/mt76x2.h"
#endif /* MT76x2 */

#ifdef RT8592
#include "chip/rt8592.h"
#endif /* RT8592 */
#endif /* RT65xx */

#ifdef MT7603
#include "chip/mt7603.h"
#endif /* MT7603 */

#ifdef MT7636
#include "chip/mt7636.h"
#endif /* mt7636 */

#ifdef RELEASE_EXCLUDE
/* We will have a cost down version which mac version is 0x3090xxxx */
/* */
/* RT3090A facts */
/* */
/* a) 2.4 GHz */
/* b) Replacement for RT3090 */
/* c) Internal LNA */
/* d) Interference over channel #14 */
/* e) New BBP features (e.g., SIG re-modulation) */
/* */

/*
	IS_RT30xx: 307x 3090 3390
	IS_RT3390: 3390 3370
	IS_RT3071:3090 3071 3072 3091 3092
*/
#endif /* RELEASE_EXCLUDE */
#define IS_RT3090A(_pAd)    ((((_pAd)->MACVersion & 0xffff0000) == 0x30900000))

/* We will have a cost down version which mac version is 0x3090xxxx */
#define IS_RT3090(_pAd)     ((((_pAd)->MACVersion & 0xffff0000) == 0x30710000) || (IS_RT3090A(_pAd)))

#define IS_RT3070(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x30700000)
#define IS_RT3071(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x30710000)
#define IS_RT2070(_pAd)		(((_pAd)->RfIcType == RFIC_2020) || ((_pAd)->EFuseTag == 0x27))

#define IS_RT2860(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x28600000)
#define IS_RT2870(_pAd)		(IS_RT2860(_pAd) && IS_USB_INF(_pAd))
#define IS_RT2872(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x28720000)
#define IS_RT2880(_pAd)		(IS_RT2860(_pAd) && IS_RBUS_INF(_pAd))

#define IS_RT30xx(_pAd)		(((_pAd)->MACVersion & 0xfff00000) == 0x30700000||IS_RT3090A(_pAd)||IS_RT3390(_pAd))

#ifdef RELEASE_EXCLUDE
/* This chip needs high power patch. */
#endif /* RELEASE_EXCLUDE */
#define IS_RT3052B(_pAd)	(((_pAd)->CommonCfg.CID == 0x102) && (((_pAd)->CommonCfg.CN >> 16) == 0x3033))
#define IS_RT3052(_pAd)		(((_pAd)->MACVersion == 0x28720200) && (_pAd->Antenna.field.TxPath == 2))
#define IS_RT3050(_pAd)		(((_pAd)->MACVersion == 0x28720200) && ((_pAd)->RfIcType == RFIC_3020))
#define IS_RT3350(_pAd)		(((_pAd)->MACVersion == 0x28720200) && ((_pAd)->RfIcType == RFIC_3320))
#define IS_RT3352(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x33520000)
#define IS_RT5350(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x53500000)
#define IS_RT305x(_pAd)		(IS_RT3050(_pAd) || IS_RT3052(_pAd) || IS_RT3350(_pAd) || IS_RT3352(_pAd) || IS_RT5350(_pAd))
#define IS_RT3050_3052_3350(_pAd) (\
	((_pAd)->MACVersion == 0x28720200) && \
	((((_pAd)->CommonCfg.CN >> 16) == 0x3333) || (((_pAd)->CommonCfg.CN >> 16) == 0x3033)) \
)
#define IS_RT6352(_pAd)		((((_pAd)->MACVersion & 0xFFFF0000) == 0x53900000) && ((_pAd)->infType == RTMP_DEV_INF_RBUS))

#ifdef RELEASE_EXCLUDE
/*
	RT3050: MAC_CSR0  [ Ver:Rev=0x28720200]
			RF IC Type: 5
			pAd->Antenna.field.TxPath = 1
			pAd->CommonCfg.CN: 33335452
			pAd->CommonCfg.CID = 102

	RT3350: MAC_CSR0  [ Ver:Rev=0x28720200]
			RF IC Type: 11
			pAd->Antenna.field.TxPath = 1
			pAd->CommonCfg.CN: 33335452
			pAd->CommonCfg.CID = 102
*/
#endif // RELEASE_EXCLUDE //

/* RT3572, 3592, 3562, 3062 share the same MAC version */
#define IS_RT3572(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x35720000)

/* Check if it is RT3xxx, or Specified ID in registry for debug */
#define IS_DEV_RT3xxx(_pAd)( \
	(_pAd->DeviceID == NIC3090_PCIe_DEVICE_ID) || \
	(_pAd->DeviceID == NIC3091_PCIe_DEVICE_ID) || \
	(_pAd->DeviceID == NIC3092_PCIe_DEVICE_ID) || \
	(_pAd->DeviceID == NIC3592_PCIe_DEVICE_ID) || \
	((_pAd->DeviceID == NIC3593_PCI_OR_PCIe_DEVICE_ID) && (RT3593OverPCIe(_pAd))) \
)

#ifdef RT3593
#define RT3593_DEVICE_ID_CHECK(__DevId)			\
	(__DevId == NIC3593_PCI_OR_PCIe_DEVICE_ID)
#else
#define RT3593_DEVICE_ID_CHECK(__DevId)			\
	(0)
#endif /* RT3593 */

#define RT3592_DEVICE_ID_CHECK(__DevId)			\
	(__DevId == NIC3592_PCIe_DEVICE_ID)

#ifdef RT2883
#define IS_RT2883(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x28830000)
#else
#define IS_RT2883(_pAd)		(0)
#endif /* RT2883 */

#ifdef RT3883
#define IS_RT3883(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x38830000)
#else
#define IS_RT3883(_pAd)		(0)
#endif /* RT3883 */

#define IS_VERSION_BEFORE_F(_pAd)			(((_pAd)->MACVersion&0xffff) <= 0x0211)
/* F version is 0x0212, E version is 0x0211. 309x can save more power after F version. */
#define IS_VERSION_AFTER_F(_pAd)			((((_pAd)->MACVersion&0xffff) >= 0x0212) || (((_pAd)->b3090ESpecialChip == TRUE)))

#define IS_RT3290(_pAd)	(((_pAd)->MACVersion & 0xffff0000) == 0x32900000)
#define IS_RT3290LE(_pAd)   ((((_pAd)->MACVersion & 0xffffffff) >= 0x32900011))

/* 3593 */
#define IS_RT3593(_pAd) (((_pAd)->MACVersion & 0xFFFF0000) == 0x35930000)

/* RT5392 */
#define IS_RT5392(_pAd)   ((_pAd->MACVersion & 0xFFFF0000) == 0x53920000) /* Include RT5392, RT5372 and RT5362 */

/* RT5390 */
#define IS_RT5390(_pAd)   (((((_pAd)->MACVersion & 0xFFFF0000) == 0x53900000) || IS_RT5390H(_pAd)) && ((_pAd)->infType != RTMP_DEV_INF_RBUS)) /* Include RT5390, RT5370 and RT5360 */

/* RT5390F */
#define IS_RT5390F(_pAd)	((IS_RT5390(_pAd)) && (((_pAd)->MACVersion & 0x0000FFFF) >= 0x0502))

/* RT5370G */
#define IS_RT5370G(_pAd)	((IS_RT5390(_pAd)) && (((_pAd)->MACVersion & 0x0000FFFF) >= 0x0503)) /* support HW PPAD ( the hardware rx antenna diversity ) */

/* RT5390R */
#define IS_RT5390R(_pAd)   ((IS_RT5390(_pAd)) && (((_pAd)->MACVersion & 0x0000FFFF) == 0x1502)) /* support HW PPAD ( the hardware rx antenna diversity ) */

/* PCIe interface NIC */
#define IS_MINI_CARD(_pAd) ((_pAd)->Antenna.field.BoardType == BOARD_TYPE_MINI_CARD)

/* 5390U (5370 using PCIe interface) */
#define IS_RT5390U(_pAd)   (IS_MINI_CARD(_pAd) && (((_pAd)->MACVersion & 0xFFFF0000) == 0x53900000) && ((_pAd)->infType != RTMP_DEV_INF_RBUS))

/* RT5390H */
#define IS_RT5390H(_pAd)   (((_pAd->MACVersion & 0xFFFF0000) == 0x53910000) && (((_pAd)->MACVersion & 0x0000FFFF) >= 0x1500))

/* RT5390BC8 (WiFi + BT) */

#ifdef BT_COEXISTENCE_SUPPORT
#define IS_RT5390BC8(_pAd)   ((IS_RT5390(_pAd)) && ((_pAd)->bHWCoexistenceInit == TRUE))
#endif /* BT_COEXISTENCE_SUPPORT */

/* RT5390D */
#define IS_RT5390D(_pAd)	((IS_RT5390(_pAd)) && (((_pAd)->MACVersion & 0x0000FFFF) >= 0x0502))

/* RT5392C */
#define IS_RT5392C(_pAd)	((IS_RT5392(_pAd)) && (((_pAd)->MACVersion & 0x0000FFFF) >= 0x0222)) /* Include RT5392, RT5372 and RT5362 */

#define IS_RT5592(_pAd)		(((_pAd)->MACVersion & 0xFFFF0000) == 0x55920000)
#define REV_RT5592C			0x0221

#define IS_RT8592(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x85590000)

#define IS_MT7601(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x76010000)
#define IS_MT7601U(_pAd)	(IS_MT7601(_pAd) && (IS_USB_INF(_pAd)))

#define IS_MT7650(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76500000)
#define IS_MT7650E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76500000) && (IS_PCIE_INF(_pAd)))
#define IS_MT7650U(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76500000) && (IS_USB_INF(_pAd)))
#define IS_MT7630(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76300000)
#define IS_MT7630E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76300000) && (IS_PCIE_INF(_pAd)))
#define IS_MT7630U(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76300000) && (IS_USB_INF(_pAd)))
#define IS_MT7610(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76100000)
#define IS_MT7610E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76100000) && (IS_PCIE_INF(_pAd)))
#define IS_MT7610U(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76100000) && (IS_USB_INF(_pAd)))
#define IS_MT76x0(_pAd)		(IS_MT7610(_pAd) || IS_MT7630(_pAd) || IS_MT7650(_pAd))
#define IS_MT76x0E(_pAd)	(IS_MT7650E(_pAd) || IS_MT7630E(_pAd) || IS_MT7610E(_pAd))
#define IS_MT76x0U(_pAd)	(IS_MT7650U(_pAd) || IS_MT7630U(_pAd) || IS_MT7610U(_pAd))

#define IS_MT7662(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76620000)
#define IS_MT7662E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76620000) && (IS_PCIE_INF(_pAd)))
#define IS_MT7662U(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76620000) && (IS_USB_INF(_pAd)))
#define IS_MT7632(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76320000)
#define IS_MT7632E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76320000) && (IS_PCIE_INF(_pAd)))
#define IS_MT7632U(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76320000) && (IS_USB_INF(_pAd)))
#define IS_MT7612(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76120000)
#define IS_MT7612E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76120000) && (IS_PCIE_INF(_pAd)))
#define IS_MT7612U(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76120000) && (IS_USB_INF(_pAd)))
#define IS_MT7602(_pAd)		(((_pAd)->ChipID & 0xffff0000) == 0x76020000)
#define IS_MT7602E(_pAd)	((((_pAd)->ChipID & 0xffff0000) == 0x76020000) && (IS_PCIE_INF(_pAd)))
#define IS_MT76x2(_pAd)		(IS_MT7662(_pAd) || IS_MT7632(_pAd) || IS_MT7612(_pAd) || IS_MT7602(_pAd))
#define IS_MT76x2E(_pAd)	(IS_MT7662E(_pAd) || IS_MT7632E(_pAd) || IS_MT7612E(_pAd) || IS_MT7602E(_pAd))
#define IS_MT76x2U(_pAd)	(IS_MT7662U(_pAd) || IS_MT7632U(_pAd) || IS_MT7612U(_pAd))
#define REV_MT76x2E3        0x0022

#define IS_MT76xx(_pAd)		(IS_MT76x0(_pAd) || IS_MT76x2(_pAd))

#define IS_RT65XX(_pAd)		((((_pAd)->MACVersion & 0xFFFF0000) == 0x65900000) ||\
							 (IS_RT8592(_pAd))||\
							 (IS_MT76x0(_pAd)) ||\
							 (IS_MT76x2(_pAd)))


#define IS_MT7603(_pAd)		(((_pAd)->ChipID & 0x0000ffff) == 0x00007603)
#define IS_MT7603E(_pAd)	((((_pAd)->ChipID & 0x0000ffff) == 0x00007603) && IS_PCIE_INF(_pAd))
#define IS_MT7603U(_pAd)	((((_pAd)->ChipID & 0x0000ffff) == 0x00007603) && IS_USB_INF(_pAd) && (_pAd->AntMode == 0))
#define IS_MT76031U(_pAd)	((((_pAd)->ChipID & 0x0000ffff) == 0x00007603) && IS_USB_INF(_pAd) && (_pAd->AntMode == 1))

#define IS_MT7636(_pAd)     ((((_pAd)->ChipID & 0x0000ffff) == 0x00007606)||(((_pAd)->ChipID & 0x0000ffff) == 0x00007636))
#define IS_MT7636U(_pAd)	((((_pAd)->ChipID & 0x0000ffff) == 0x00007606) && IS_USB_INF(_pAd)

#define MT7603E1 0x0000
#define MT7603E2 0x0010

#define IS_MT7628(_pAd)		(((_pAd)->MACVersion & 0xffff0000) == 0x76280000)

#define MT7628E1 0x0000
#define MT7628E2 0x0010

/* RT3592BC8 (WiFi + BT) */
#ifdef BT_COEXISTENCE_SUPPORT
#define IS_RT3592BC8(_pAd)   ((IS_RT3572(_pAd)) && ((_pAd)->bHWCoexistenceInit == TRUE))
#endif /* BT_COEXISTENCE_SUPPORT */

#define IS_USB_INF(_pAd)		((_pAd)->infType == RTMP_DEV_INF_USB)
#define IS_USB3_INF(_pAd)		((IS_USB_INF(_pAd)) && ((_pAd)->BulkOutMaxPacketSize == 1024))
#define IS_PCIE_INF(_pAd)		((_pAd)->infType == RTMP_DEV_INF_PCIE)
#define IS_PCI_INF(_pAd)		(((_pAd)->infType == RTMP_DEV_INF_PCI) || IS_PCIE_INF(_pAd))
#define IS_PCI_ONLY_INF(_pAd)	((_pAd)->infType == RTMP_DEV_INF_PCI)
#define IS_RBUS_INF(_pAd) ((_pAd)->infType == RTMP_DEV_INF_RBUS)

#define RT_REV_LT(_pAd, _chip, _rev)\
	IS_##_chip(_pAd) && (((_pAd)->MACVersion & 0x0000FFFF) < (_rev))

#define RT_REV_GTE(_pAd, _chip, _rev)\
	IS_##_chip(_pAd) && (((_pAd)->MACVersion & 0x0000FFFF) >= (_rev))

#define MT_REV_LT(_pAd, _chip, _rev)\
	IS_##_chip(_pAd) && (((_pAd)->ChipID & 0x0000FFFF) < (_rev))

#define MT_REV_GTE(_pAd, _chip, _rev)\
	IS_##_chip(_pAd) && (((_pAd)->ChipID & 0x0000FFFF) >= (_rev))

#define MTK_REV_LT(_pAd, _chip, _rev)\
	IS_##_chip(_pAd) && (((_pAd)->HWVersion & 0x000000ff) < (_rev))

#define MTK_REV_GTE(_pAd, _chip, _rev)\
	IS_##_chip(_pAd) && (((_pAd)->HWVersion & 0x000000ff) >= (_rev)) 

/* Dual-band NIC (RF/BBP/MAC are in the same chip.) */

#define IS_RT_NEW_DUAL_BAND_NIC(_pAd) ((FALSE))


/* Is the NIC dual-band NIC? */

#define IS_DUAL_BAND_NIC(_pAd) (((_pAd->RfIcType == RFIC_2850) || (_pAd->RfIcType == RFIC_2750) || (_pAd->RfIcType == RFIC_3052)		\
								|| (_pAd->RfIcType == RFIC_3053) || (_pAd->RfIcType == RFIC_2853) || (_pAd->RfIcType == RFIC_3853) 	\
								|| IS_RT_NEW_DUAL_BAND_NIC(_pAd)) && !IS_RT5390(_pAd))


/* RT3593 over PCIe bus */
#define RT3593OverPCIe(_pAd) (IS_RT3593(_pAd) && (_pAd->CommonCfg.bPCIeBus == TRUE))

/* RT3593 over PCI bus */
#define RT3593OverPCI(_pAd) (IS_RT3593(_pAd) && (_pAd->CommonCfg.bPCIeBus == FALSE))

#ifdef RELEASE_EXCLUDE
/* */
/* RT3390 facts */
/* */
/* a) Base on RT3090 (RF IC: RT3020) */
/* b) 2.4 GHz */
/* c) 1x1 */
/* d) Single chip */
/* e) Internal components: PA and LNA */
/* */
#endif /* RELEASE_EXCLUDE */
/*RT3390,RT3370 */
#define IS_RT3390(_pAd)				(((_pAd)->MACVersion & 0xFFFF0000) == 0x33900000)

#define CCA_AVG_MAX_COUNT	5

/* ------------------------------------------------------ */
/* PCI registers - base address 0x0000 */
/* ------------------------------------------------------ */
#define CHIP_PCI_CFG		0x0000
#define CHIP_PCI_EECTRL		0x0004
#define CHIP_PCI_MCUCTRL	0x0008

#define OPT_14			0x114

#define RETRY_LIMIT		10

/* ------------------------------------------------------ */
/* BBP & RF	definition */
/* ------------------------------------------------------ */
#define	BUSY		                1
#define	IDLE		                0

/*------------------------------------------------------------------------- */
/* EEPROM definition */
/*------------------------------------------------------------------------- */
#define EEDO                        0x08
#define EEDI                        0x04
#define EECS                        0x02
#define EESK                        0x01
#define EERL                        0x80

#define EEPROM_WRITE_OPCODE         0x05
#define EEPROM_READ_OPCODE          0x06
#define EEPROM_EWDS_OPCODE          0x10
#define EEPROM_EWEN_OPCODE          0x13

#define NUM_EEPROM_BBP_PARMS		19	/* Include NIC Config 0, 1, CR, TX ALC step, BBPs */
#define NUM_EEPROM_TX_G_PARMS		7

#define VALID_EEPROM_VERSION        1
#define EEPROM_VERSION_OFFSET       0x02
#define EEPROM_NIC1_OFFSET          0x34	/* The address is from NIC config 0, not BBP register ID */
#define EEPROM_NIC2_OFFSET          0x36	/* The address is from NIC config 1, not BBP register ID */

#if defined(RT2883) || defined(RT3883)
#define	EEPROM_NIC3_OFFSET          0x38	/* The address is from NIC config 2, not BBP register ID */
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883)
#define EEPROM_COUNTRY_REGION		0x3e
#else
#define EEPROM_COUNTRY_REGION			0x38
#define COUNTRY_REGION_A_BAND_MASK (0xff)
#define COUNTRY_REGION_G_BAND (0xff << 8)
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883)
#define EEPROM_DEFINE_MAX_TXPWR		0x40
#else
#define EEPROM_DEFINE_MAX_TXPWR			0x4e
#define MAX_EIRP_TX_PWR_G_BAND_MASK (0xff)
#define MAX_EIRP_TX_PWR_A_BAND_MASK (0xff << 8)
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883)
#ifdef RT2883
#define	EEPROM_FREQ_OFFSET			0x42
#define EEPROM_LEDAG_CONF_OFFSET	0x44
#define EEPROM_LEDACT_CONF_OFFSET	0x46
#define EEPROM_LED_POLARITY_OFFSET	0x48
#endif /* RT2883 */
#ifdef RT3883
#define	EEPROM_FREQ_OFFSET			0x44
#define EEPROM_LEDAG_CONF_OFFSET	0x46
#define EEPROM_LEDACT_CONF_OFFSET	0x48
#define EEPROM_LED_POLARITY_OFFSET	0x4a
#endif /* RT3883 */
#else
#define EEPROM_FREQ_OFFSET			0x3a
#define FREQ_OFFSET_MASK (0x7f)
#define FREQ_OFFSET_DIP (1 << 7)
#define LED_MODE_MASK (0xff << 8)

#define EEPROM_LEDAG_CONF_OFFSET	0x3c
#define EEPROM_LEDACT_CONF_OFFSET	0x3e
#define EEPROM_LED_POLARITY_OFFSET	0x40
#if defined(BT_COEXISTENCE_SUPPORT) || defined(RT3290) || defined(RT8592) || defined(RT6352)
#define	EEPROM_NIC3_OFFSET			0x42
#endif /* defined(BT_COEXISTENCE_SUPPORT) || defined(RT3290) || defined(RT8592) || defined(RT6352) */
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883)
#define EEPROM_LNA_OFFSET			0x4c
#define EEPROM_LNA_OFFSET2			0x4e
#else
#define EEPROM_LNA_OFFSET			0x44
#define LNA_GAIN_G_BAND_MASK (0x7f)
#define LNA_GAIN_G_BAND_EN (1 << 7)
#define LNA_GAIN_A_BAND_CH36_64_MASK (0x7f << 8)
#define LNA_GAIN_A_BAND_CH36_64_EN (1 << 15)
#endif /* defined (RT2883) || defined (RT3883) */

#if defined(RT2883) || defined(RT3883)
#define EEPROM_RSSI_BG_OFFSET		0x50
#define EEPROM_RSSI_A_OFFSET		0x54
#else
#define EEPROM_RSSI_BG_OFFSET			0x46
#define RSSI0_OFFSET_G_BAND_MASK (0x3f)
#define RSSI0_OFFSET_G_BAND_SIGN (1 << 6)
#define RSSI0_OFFSET_G_BAND_EN (1 << 7)
#define RSSI1_OFFSET_G_BAND_MASK (0x3f << 8)
#define RSSI1_OFFSET_G_BAND_SIGN (1 << 14)
#define RSSI1_OFFSET_G_BAND_EN (1 << 15)

#define EEPROM_TXMIXER_GAIN_2_4G		0x48
#define LNA_GAIN_A_BAND_CH100_128_MASK (0x7f << 8)
#define LNA_GAIN_A_BAND_CH100_128_EN (1 << 15)

#define EEPROM_RSSI_A_OFFSET			0x4a
#define RSSI0_OFFSET_A_BAND_MASK (0x3f)
#define RSSI0_OFFSET_A_BAND_SIGN (1 << 6)
#define RSSI0_OFFSET_A_BANE_EN (1 << 7)
#define RSSI1_OFFSET_A_BAND_MASK (0x3f << 8)
#define RSSI1_OFFSET_A_BAND_SIGN (1 << 14)
#define RSSI1_OFFSET_A_BAND_EN (1 << 15)

#define EEPROM_TXMIXER_GAIN_5G			0x4c
#define LNA_GAIN_A_BAND_CH132_165_MASK (0x7f << 8)
#define LNA_GAIN_A_BAND_CH132_165_EN (1 << 15)
#endif /* defined(RT2883) || defined(RT3883) */

#define EEPROM_TXPOWER_DELTA			0x50	/* 20MHZ AND 40 MHZ use different power. This is delta in 40MHZ. */

#if defined(RT2883) || defined(RT3883)
#define	EEPROM_G_TX_PWR_OFFSET		0x60
#define	EEPROM_G_TX2_PWR_OFFSET		0x6e
#define	EEPROM_G_TX3_PWR_OFFSET		0x7c
#else
#define EEPROM_G_TX_PWR_OFFSET			0x52
#define EEPROM_G_TX2_PWR_OFFSET			0x60
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883)
#define EEPROM_G_TSSI_BOUND1		0x8a
#define EEPROM_G_TSSI_BOUND2		0x8c
#define EEPROM_G_TSSI_BOUND3		0x8e
#define EEPROM_G_TSSI_BOUND4		0x90
#define EEPROM_G_TSSI_BOUND5		0x92
#else
#define EEPROM_G_TSSI_BOUND1			0x6e
#define EEPROM_G_TSSI_BOUND2			0x70
#define EEPROM_G_TSSI_BOUND3			0x72
#define EEPROM_G_TSSI_BOUND4			0x74
#define EEPROM_G_TSSI_BOUND5			0x76
#endif /* defined(RT2883) || defined(RT3883) */

#ifdef MT7601
#define EEPROM_TX0_TSSI_SLOPE				0x6e
#define EEPROM_TX0_TSSI_OFFSET_GROUP1	0x70
#define EEPROM_TX0_TSSI_OFFSET			0x76
#define EEPROM_G_TARGET_POWER			0xD0
#define EEPROM_FREQ_OFFSET_COMPERSATION		0xDA
#endif /* MT7601 */


#if defined(RT2883) || defined(RT3883)
#define EEPROM_A_TX_PWR_OFFSET      0x96
#define EEPROM_A_TX2_PWR_OFFSET     0xca
#define EEPROM_A_TX3_PWR_OFFSET     0xfe
#else
#define EEPROM_A_TX_PWR_OFFSET      		0x78
#define EEPROM_A_TX2_PWR_OFFSET			0xa6
#endif /* defined (RT2883) || defined (RT3883) */

#define MBSSID_MODE0 	0
#define MBSSID_MODE1 	1	/* Enhance NEW MBSSID MODE mapping to mode 0 */
#ifdef ENHANCE_NEW_MBSSID_MODE
#define MBSSID_MODE2	2	/* Enhance NEW MBSSID MODE mapping to mode 1 */
#define MBSSID_MODE3	3	/* Enhance NEW MBSSID MODE mapping to mode 2 */
#define MBSSID_MODE4	4	/* Enhance NEW MBSSID MODE mapping to mode 3 */
#define MBSSID_MODE5	5	/* Enhance NEW MBSSID MODE mapping to mode 4 */
#define MBSSID_MODE6	6	/* Enhance NEW MBSSID MODE mapping to mode 5 */
#endif /* ENHANCE_NEW_MBSSID_MODE */

enum FREQ_CAL_INIT_MODE {
	FREQ_CAL_INIT_MODE0,
	FREQ_CAL_INIT_MODE1,
	FREQ_CAL_INIT_MODE2,
	FREQ_CAL_INIT_UNKNOW,
};

enum FREQ_CAL_MODE {
	FREQ_CAL_MODE0,
	FREQ_CAL_MODE1,
	FREQ_CAL_MODE2,
};

enum RXWI_FRQ_OFFSET_FIELD {
	RXWI_FRQ_OFFSET_FIELD0, /* SNR1 */
	RXWI_FRQ_OFFSET_FIELD1, /* Frequency Offset */
};

#ifdef IQ_CAL_SUPPORT
/* IQ Calibration */
enum IQ_CAL_CHANNEL_INDEX {
	IQ_CAL_2G,
#ifdef A_BAND_SUPPORT
	IQ_CAL_GROUP1_5G, /* Ch36~Ch64 */
	IQ_CAL_GROUP2_5G, /* Ch100~Ch138 */
	IQ_CAL_GROUP3_5G, /* Ch140~Ch165 */
#endif /* A_BAND_SUPPORT */
	IQ_CAL_CHANNEL_GROUP_NUM,
};

enum IQ_CAL_TXRX_CHAIN {
	IQ_CAL_TX0,
	IQ_CAL_TX1,
	IQ_CAL_CHAIN_NUM,
};

enum IQ_CAL_TYPE {
	IQ_CAL_GAIN,
	IQ_CAL_PHASE,
	IQ_CAL_TYPE_NUM,
};

#define EEPROM_IQ_GLOBAL_BBP_ACCESS_BASE					0xF0
#define EEPROM_IQ_GAIN_CAL_TX0_2G							0x130
#define EEPROM_IQ_PHASE_CAL_TX0_2G							0x131
#define EEPROM_IQ_GROUPDELAY_CAL_TX0_2G						0x132
#define EEPROM_IQ_GAIN_CAL_TX1_2G							0x133
#define EEPROM_IQ_PHASE_CAL_TX1_2G							0x134
#define EEPROM_IQ_GROUPDELAY_CAL_TX1_2G						0x135
#define EEPROM_IQ_GAIN_CAL_RX0_2G							0x136
#define EEPROM_IQ_PHASE_CAL_RX0_2G							0x137
#define EEPROM_IQ_GROUPDELAY_CAL_RX0_2G					0x138
#define EEPROM_IQ_GAIN_CAL_RX1_2G							0x139
#define EEPROM_IQ_PHASE_CAL_RX1_2G							0x13A
#define EEPROM_IQ_GROUPDELAY_CAL_RX1_2G					0x13B
#define EEPROM_RF_IQ_COMPENSATION_CONTROL 					0x13C
#define EEPROM_RF_IQ_IMBALANCE_COMPENSATION_CONTROL 		0x13D
#define EEPROM_IQ_GAIN_CAL_TX0_CH36_TO_CH64_5G				0x144
#define EEPROM_IQ_PHASE_CAL_TX0_CH36_TO_CH64_5G			0x145
#define EEPROM_IQ_GAIN_CAL_TX0_CH100_TO_CH138_5G			0X146
#define EEPROM_IQ_PHASE_CAL_TX0_CH100_TO_CH138_5G			0x147
#define EEPROM_IQ_GAIN_CAL_TX0_CH140_TO_CH165_5G			0x148
#define EEPROM_IQ_PHASE_CAL_TX0_CH140_TO_CH165_5G			0x149
#define EEPROM_IQ_GAIN_CAL_TX1_CH36_TO_CH64_5G				0x14A
#define EEPROM_IQ_PHASE_CAL_TX1_CH36_TO_CH64_5G			0x14B
#define EEPROM_IQ_GAIN_CAL_TX1_CH100_TO_CH138_5G			0X14C
#define EEPROM_IQ_PHASE_CAL_TX1_CH100_TO_CH138_5G			0x14D
#define EEPROM_IQ_GAIN_CAL_TX1_CH140_TO_CH165_5G			0x14E
#define EEPROM_IQ_PHASE_CAL_TX1_CH140_TO_CH165_5G			0x14F
#define EEPROM_IQ_GROUPDELAY_CAL_TX0_CH36_TO_CH64_5G		0x150
#define EEPROM_IQ_GROUPDELAY_CAL_TX1_CH36_TO_CH64_5G		0x151
#define EEPROM_IQ_GROUPDELAY_CAL_TX0_CH100_TO_CH138_5G	0x152
#define EEPROM_IQ_GROUPDELAY_CAL_TX1_CH100_TO_CH138_5G	0x153
#define EEPROM_IQ_GROUPDELAY_CAL_TX0_CH140_TO_CH165_5G	0x154
#define EEPROM_IQ_GROUPDELAY_CAL_TX1_CH140_TO_CH165_5G	0x155
#define EEPROM_IQ_GAIN_CAL_RX0_CH36_TO_CH64_5G				0x156
#define EEPROM_IQ_PHASE_CAL_RX0_CH36_TO_CH64_5G			0x157
#define EEPROM_IQ_GAIN_CAL_RX0_CH100_TO_CH138_5G			0X158
#define EEPROM_IQ_PHASE_CAL_RX0_CH100_TO_CH138_5G			0x159
#define EEPROM_IQ_GAIN_CAL_RX0_CH140_TO_CH165_5G			0x15A
#define EEPROM_IQ_PHASE_CAL_RX0_CH140_TO_CH165_5G			0x15B
#define EEPROM_IQ_GAIN_CAL_RX1_CH36_TO_CH64_5G				0x15C
#define EEPROM_IQ_PHASE_CAL_RX1_CH36_TO_CH64_5G			0x15D
#define EEPROM_IQ_GAIN_CAL_RX1_CH100_TO_CH138_5G			0X15E
#define EEPROM_IQ_PHASE_CAL_RX1_CH100_TO_CH138_5G			0x15F
#define EEPROM_IQ_GAIN_CAL_RX1_CH140_TO_CH165_5G			0x160
#define EEPROM_IQ_PHASE_CAL_RX1_CH140_TO_CH165_5G			0x161
#define EEPROM_IQ_GROUPDELAY_CAL_RX0_CH36_TO_CH64_5G		0x162
#define EEPROM_IQ_GROUPDELAY_CAL_RX1_CH36_TO_CH64_5G		0x163
#define EEPROM_IQ_GROUPDELAY_CAL_RX0_CH100_TO_CH138_5G	0x164
#define EEPROM_IQ_GROUPDELAY_CAL_RX1_CH100_TO_CH138_5G	0x165
#define EEPROM_IQ_GROUPDELAY_CAL_RX0_CH140_TO_CH165_5G	0x166
#define EEPROM_IQ_GROUPDELAY_CAL_RX1_CH140_TO_CH165_5G	0x167
#endif /* IQ_CAL_SUPPORT */

#ifdef MT76x0
#define EEPROM_MT76x0_TEMPERATURE_OFFSET	0xd1 /* signed value */
#define EEPROM_MT76x0_A_BAND_MB			0xdc
#define EEPROM_MT76x0_A_BAND_HB			0xdd

#ifdef MT76x0_TSSI_CAL_COMPENSATION
#define EEPROM_MT76x0_2G_TARGET_POWER	0xd0
#define EEPROM_MT76x0_5G_TARGET_POWER	0xd2
#define EEPROM_MT76x0_2G_SLOPE_OFFSET	0x6E
#define EEPROM_MT76x0_5G_SLOPE_OFFSET	0xf0
#define EEPROM_MT76x0_5G_CHANNEL_BOUNDARY 0xd4
#endif /* MT76x0_TSSI_CAL_COMPENSATION */
#endif /* MT76x0 */

#if defined(RT2883) || defined(RT3883)
#define EEPROM_A_TSSI_BOUND1				0x134
#define EEPROM_A_TSSI_BOUND2				0x136
#define EEPROM_A_TSSI_BOUND3				0x138
#define EEPROM_A_TSSI_BOUND4				0x13a
#define EEPROM_A_TSSI_BOUND5				0x13c
#else
#define EEPROM_A_TSSI_BOUND1		0xd4
#define EEPROM_A_TSSI_BOUND2		0xd6
#define EEPROM_A_TSSI_BOUND3		0xd8
#define EEPROM_A_TSSI_BOUND4		0xda
#define EEPROM_A_TSSI_BOUND5		0xdc
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883) || defined(RT3593)
/* ITxBF calibration values EEPROM locations 0x1a0 to 0x1ab */
#define EEPROM0_ITXBF_CAL				0x1a0
#endif

#ifdef MT76x2
/* ITxBF calibration values EEPROM locations 0xC0 to 0xF1 */
#define EEPROM1_ITXBF_CAL				0xc0
#endif

#if defined(RT2883) || defined(RT3883)
#ifdef RT2883
#define EEPROM_TXPOWER_BYRATE_CCK_OFDM		0x140
#define EEPROM_TXPOWER_BYRATE_20MHZ_2_4G	0x146	/* 20MHZ 2.4G tx power. */
#define EEPROM_TXPOWER_BYRATE_40MHZ_2_4G	0x156	/* 40MHZ 2.4G tx power. */
#define EEPROM_TXPOWER_BYRATE_20MHZ_5G		0x166	/* 20MHZ 5G tx power. */
#define EEPROM_TXPOWER_BYRATE_40MHZ_5G		0x176	/* 40MHZ 5G tx power. */
#endif /* RT2883 */
#ifdef RT3883
#define EEPROM_TXPOWER_BYRATE_CCK_OFDM		0x140
#define EEPROM_TXPOWER_BYRATE_40MHZ_OFDM_2_4G	0x150
#define EEPROM_TXPOWER_BYRATE_20MHZ_OFDM_5G	0x160
#define EEPROM_TXPOWER_BYRATE_40MHZ_OFDM_5G	0x170
#define EEPROM_TXPOWER_BYRATE_20MHZ_2_4G	0x144	/* 20MHZ 2.4G tx power. */
#define EEPROM_TXPOWER_BYRATE_40MHZ_2_4G	0x154	/* 40MHZ 2.4G tx power. */
#define EEPROM_TXPOWER_BYRATE_20MHZ_5G		0x164	/* 20MHZ 5G tx power. */
#define EEPROM_TXPOWER_BYRATE_40MHZ_5G		0x174	/* 40MHZ 5G tx power. */
#endif /* RT3883 */
#else
#define EEPROM_TXPOWER_BYRATE 			0xde	/* 20MHZ power. */
#define EEPROM_TXPOWER_BYRATE_20MHZ_2_4G	0xde	/* 20MHZ 2.4G tx power. */
#define EEPROM_TXPOWER_BYRATE_40MHZ_2_4G	0xee	/* 40MHZ 2.4G tx power. */
#define EEPROM_TXPOWER_BYRATE_20MHZ_5G		0xfa	/* 20MHZ 5G tx power. */
#define EEPROM_TXPOWER_BYRATE_40MHZ_5G		0x10a	/* 40MHZ 5G tx power. */
#endif /* defined(RT2883) || defined(RT3883) */

#if defined(RT2883) || defined(RT3883)
#define	EEPROM_BBP_BASE_OFFSET			0x186
#else
#define EEPROM_BBP_BASE_OFFSET			0xf0	/* The address is from NIC config 0, not BBP register ID */
#endif /* defined(RT2883) || defined(RT3883) */

/* */
/* Bit mask for the Tx ALC and the Tx fine power control */
/* */
#define GET_TX_ALC_BIT_MASK					0x1F	/* Valid: 0~31, and in 0.5dB step */
#define GET_TX_FINE_POWER_CTRL_BIT_MASK	0xE0	/* Valid: 0~4, and in 0.1dB step */
#define NUMBER_OF_BITS_FOR_TX_ALC			5	/* The length, in bit, of the Tx ALC field */


/* TSSI gain and TSSI attenuation */

#define EEPROM_TSSI_GAIN_AND_ATTENUATION	0x76

/*#define EEPROM_Japan_TX_PWR_OFFSET      0x90 // 802.11j */
/*#define EEPROM_Japan_TX2_PWR_OFFSET      0xbe */
/*#define EEPROM_TSSI_REF_OFFSET	0x54 */
/*#define EEPROM_TSSI_DELTA_OFFSET	0x24 */
/*#define EEPROM_CCK_TX_PWR_OFFSET  0x62 */
/*#define EEPROM_CALIBRATE_OFFSET	0x7c */

#define EEPROM_NIC_CFG1_OFFSET		0
#define EEPROM_NIC_CFG2_OFFSET		1
#define EEPROM_NIC_CFG3_OFFSET		2
#define EEPROM_COUNTRY_REG_OFFSET	3
#define EEPROM_BBP_ARRAY_OFFSET		4

#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
/* */
/* The TSSI over OFDM 54Mbps */
/* */
#define EEPROM_TSSI_OVER_OFDM_54		0x6E

/* */
/* The TSSI value/step (0.5 dB/unit) */
/* */
#define EEPROM_TSSI_STEP_OVER_2DOT4G	0x77
#define EEPROM_TSSI_STEP_OVER_5DOT5G	0xDD
#define TSSI_READ_SAMPLE_NUM			3

/* */
/* Per-channel Tx power offset (for the extended TSSI mode) */
/* */
#define EEPROM_TX_POWER_OFFSET_OVER_CH_1	0x6F
#define EEPROM_TX_POWER_OFFSET_OVER_CH_3	0x70
#define EEPROM_TX_POWER_OFFSET_OVER_CH_5	0x71
#define EEPROM_TX_POWER_OFFSET_OVER_CH_7	0x72
#define EEPROM_TX_POWER_OFFSET_OVER_CH_9	0x73
#define EEPROM_TX_POWER_OFFSET_OVER_CH_11	0x74
#define EEPROM_TX_POWER_OFFSET_OVER_CH_13	0x75

/* */
/* Tx power configuration (bit3:0 for Tx0 power setting and bit7:4 for Tx1 power setting) */
/* */
#define EEPROM_CCK_MCS0_MCS1				0xDE
#define EEPROM_CCK_MCS2_MCS3				0xDF
#define EEPROM_OFDM_MCS0_MCS1			0xE0
#define EEPROM_OFDM_MCS2_MCS3			0xE1
#define EEPROM_OFDM_MCS4_MCS5			0xE2
#define EEPROM_OFDM_MCS6_MCS7			0xE3
#define EEPROM_HT_MCS0_MCS1				0xE4
#define EEPROM_HT_MCS2_MCS3				0xE5
#define EEPROM_HT_MCS4_MCS5				0xE6
#define EEPROM_HT_MCS6_MCS7				0xE7
#define EEPROM_HT_MCS8_MCS9                     	0xE8
#define EEPROM_HT_MCS10_MCS11                   	0xE9
#define EEPROM_HT_MCS12_MCS13                   	0xEA
#define EEPROM_HT_MCS14_MCS15                   	0xEB
#define EEPROM_HT_USING_STBC_MCS0_MCS1	0xEC
#define EEPROM_HT_USING_STBC_MCS2_MCS3	0xED
#define EEPROM_HT_USING_STBC_MCS4_MCS5	0xEE
#define EEPROM_HT_USING_STBC_MCS6_MCS7	0xEF

/* */
/* Bit mask for the Tx ALC and the Tx fine power control */
/* */
#if 0
#define GET_TX_ALC_BIT_MASK				0x1F	/* Valid: 0~31, and in 0.5dB step */
#define GET_TX_FINE_POWER_CTRL_BIT_MASK	0xE0	/* Valid: 0~4, and in 0.1dB step */
#define NUMBER_OF_BITS_FOR_TX_ALC			5	/* The length, in bit, of the Tx ALC field */
#endif

#define DEFAULT_BBP_TX_FINE_POWER_CTRL 	0

#endif /* RTMP_INTERNAL_TX_ALC || RTMP_TEMPERATURE_COMPENSATION */

#ifdef RT33xx
#define EEPROM_EVM_RF09  0x120
#define EEPROM_EVM_RF19  0x122
#define EEPROM_EVM_RF21  0x124
#define EEPROM_EVM_RF29  0x128
#endif /* RT33xx */

#ifdef RT_BIG_ENDIAN
typedef union _EEPROM_ANTENNA_STRUC {
	struct {
		USHORT RssiIndicationMode:1; 	/* RSSI indication mode */
		USHORT Rsv:1;
		USHORT BoardType:2; 		/* 0: mini card; 1: USB pen */
		USHORT RfIcType:4;			/* see E2PROM document */
		USHORT TxPath:4;			/* 1: 1T, 2: 2T, 3: 3T */
		USHORT RxPath:4;			/* 1: 1R, 2: 2R, 3: 3R */
	} field;
	USHORT word;
} EEPROM_ANTENNA_STRUC, *PEEPROM_ANTENNA_STRUC;
#else
typedef union _EEPROM_ANTENNA_STRUC {
	struct {
		USHORT RxPath:4;			/* 1: 1R, 2: 2R, 3: 3R */
		USHORT TxPath:4;			/* 1: 1T, 2: 2T, 3: 3T */
		USHORT RfIcType:4;			/* see E2PROM document */
		USHORT BoardType:2; 		/* 0: mini card; 1: USB pen */
		USHORT Rsv:1;
		USHORT RssiIndicationMode:1; 	/* RSSI indication mode */
	} field;
	USHORT word;
} EEPROM_ANTENNA_STRUC, *PEEPROM_ANTENNA_STRUC;
#endif


/*
  *   EEPROM operation related marcos
  */
#define RT28xx_EEPROM_READ16(_pAd, _offset, _val)   rt28xx_eeprom_read16(_pAd, _offset, &(_val))

#define RT28xx_EEPROM_WRITE16(_pAd, _offset, _val)		\
	do {\
		if ((_pAd)->chipOps.eewrite) \
			(_pAd)->chipOps.eewrite(_pAd, (_offset), (USHORT)(_val));\
	} while(0)

#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
/* The Tx power tuning entry */
typedef struct _TX_POWER_TUNING_ENTRY_STRUCT {
	CHAR	RF_TX_ALC; 		/* 3390: RF R12[4:0]: Tx0 ALC, 5390: RF R49[5:0]: Tx0 ALC */
	CHAR 	MAC_PowerDelta;	/* Tx power control over MAC 0x1314~0x1324 */
} TX_POWER_TUNING_ENTRY_STRUCT, *PTX_POWER_TUNING_ENTRY_STRUCT;
#endif /* defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION) */

struct RF_BANK_OFFSET {
	UINT8 RFBankIndex;
	UINT16 RFStart;
	UINT16 RFEnd;
};

struct RF_INDEX_OFFSET {
	UINT8 RFIndex;
	UINT16 RFStart;
	UINT16 RFEnd;
};

/*
	2860: 28xx
	2870: 28xx

	30xx:
		3090
		3070
		2070 3070

	33xx:	30xx
		3390 3090
		3370 3070

	35xx:	30xx
		3572, 2870, 28xx
		3062, 2860, 28xx
		3562, 2860, 28xx

	3593, 28xx, 30xx, 35xx

	< Note: 3050, 3052, 3350 can not be compiled simultaneously. >
	305x:
		3052
		3050
		3350, 3050

	3352: 305x

	2880: 28xx
	2883:
	3883:
*/

struct _RTMP_CHIP_CAP {
	/* ------------------------ packet --------------------- */
	UINT8 TXWISize;	// TxWI or LMAC TxD max size
	UINT8 RXWISize; // RxWI or LMAC RxD max size
	UINT8 tx_hw_hdr_len;	// Tx Hw meta info size which including all hw info fields
	UINT8 rx_hw_hdr_len;	// Rx Hw meta info size

	enum ASIC_CAP asic_caps;
	enum PHY_CAP phy_caps;
	enum HIF_TYPE hif_type;
	enum MAC_TYPE mac_type;
	enum BBP_TYPE bbp_type;
	enum MCU_TYPE MCUType;
	enum RF_TYPE rf_type;

	/* register */
	REG_PAIR *pRFRegTable;
	REG_PAIR *pBBPRegTable;
	UCHAR bbpRegTbSize;

	UINT32 MaxNumOfRfId;
	UINT32 MaxNumOfBbpId;

#define RF_REG_WT_METHOD_NONE			0
#define RF_REG_WT_METHOD_STEP_ON		1
	UCHAR RfReg17WtMethod;

	/* beacon */
	BOOLEAN FlgIsSupSpecBcnBuf;	/* SPECIFIC_BCN_BUF_SUPPORT */
	UINT8 BcnMaxNum;	/* software use */
	UINT8 BcnMaxHwNum;	/* hardware limitation */
	UINT8 WcidHwRsvNum;	/* hardware available WCID number */
	UINT16 BcnMaxHwSize;	/* hardware maximum beacon size */
	UINT16 BcnBase[HW_BEACON_MAX_NUM];	/* hardware beacon base address */
	
	/* function */
	/* use UINT8, not bit-or to speed up driver */
	BOOLEAN FlgIsHwWapiSup;

	/* VCO calibration mode */
	UINT8 VcoPeriod; /* default 10s */
#define VCO_CAL_DISABLE		0	/* not support */
#define VCO_CAL_MODE_1		1	/* toggle RF7[0] */
#define VCO_CAL_MODE_2		2	/* toggle RF3[7] */
#define VCO_CAL_MODE_3		3	/* toggle RF4[7] or RF5[7] */
	UINT8	FlgIsVcoReCalMode;

	BOOLEAN FlgIsHwAntennaDiversitySup;
	BOOLEAN Flg7662ChipCap;
#ifdef STREAM_MODE_SUPPORT
	BOOLEAN FlgHwStreamMode;
#endif /* STREAM_MODE_SUPPORT */
#ifdef TXBF_SUPPORT
	BOOLEAN FlgHwTxBfCap;
	BOOLEAN FlgITxBfBinWrite;
#endif /* TXBF_SUPPORT */
#ifdef FIFO_EXT_SUPPORT
	BOOLEAN FlgHwFifoExtCap;
#endif /* FIFO_EXT_SUPPORT */

	UCHAR ba_max_cnt;

#ifdef RTMP_MAC_PCI
#ifdef CONFIG_STA_SUPPORT
	BOOLEAN HW_PCIE_PS_SUPPORT;
	BOOLEAN HW_PCIE_PS_L3_ENABLE;
#endif /* CONFIG_STA_SUPPORT */
#endif /* RTMP_MAC_PCI */

#ifdef TXRX_SW_ANTDIV_SUPPORT
	BOOLEAN bTxRxSwAntDiv;
#endif /* TXRX_SW_ANTDIV_SUPPORT */

#ifdef DYNAMIC_VGA_SUPPORT
	BOOLEAN dynamic_vga_support;
#endif /* DYNAMIC_VGA_SUPPORT */

	/* ---------------------------- signal ---------------------------------- */
#define SNR_FORMULA1		0	/* ((0xeb     - pAd->StaCfg.wdev.LastSNR0) * 3) / 16; */
#define SNR_FORMULA2		1	/* (pAd->StaCfg.wdev.LastSNR0 * 3 + 8) >> 4; */
#define SNR_FORMULA3		2	/* (pAd->StaCfg.wdev.LastSNR0) * 3) / 16; */
#define SNR_FORMULA4		3	/* for MT7603 */
	UINT8 SnrFormula;

	UINT8 max_nss;			/* maximum Nss, 3 for 3883 or 3593 */
	UINT8 max_vht_mcs;		/* Maximum Vht MCS */

	BOOLEAN bTempCompTxALC;
	BOOLEAN rx_temp_comp;

	BOOLEAN bLimitPowerRange; /* TSSI compensation range limit */

#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
	UINT8 TxAlcTxPowerUpperBound_2G;
	const TX_POWER_TUNING_ENTRY_STRUCT *TxPowerTuningTable_2G;
#ifdef RT305x
#if defined(RT3352) || defined(RT5350)
	UINT8 TxPowerMaxCompenStep;
	UINT8 TxPowerTableMaxIdx;
#endif /* defined(RT3352) || defined(RT5350) */
#endif /* RT305x */
#ifdef A_BAND_SUPPORT
	UINT8 TxAlcTxPowerUpperBound_5G;
	const TX_POWER_TUNING_ENTRY_STRUCT *TxPowerTuningTable_5G;
#endif /* A_BAND_SUPPORT */

#ifdef MT7601
	MT7601_TX_ALC_DATA TxALCData;
#endif /* MT7601 */
#endif /* defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION) */

#if defined(RTMP_INTERNAL_TX_ALC) || defined(SINGLE_SKU_V2)
	INT16	PAModeCCK[4];
	INT16	PAModeOFDM[8];
	INT16	PAModeHT[16];
#ifdef DOT11_VHT_AC
	INT16	PAModeVHT[10];
#endif /* DOT11_VHT_AC */
#endif /* defined(RTMP_INTERNAL_TX_ALC) || defined(SINGLE_SKU_V2) */

#ifdef MT7601
	CHAR	TemperatureRef25C;
	UCHAR	TemperatureMode;
	BOOLEAN	bPllLockProtect;
	CHAR	CurrentTemperBbpR49;
	INT32	TemperatureDPD;					// temperature when do DPD calibration
	INT32	CurrentTemperature;					// (BBP_R49 - Ref25C) * offset
#endif /* MT7601 */

	/* ---------------------------- others ---------------------------------- */
#ifdef RTMP_EFUSE_SUPPORT
	UINT16 EFUSE_USAGE_MAP_START;
	UINT16 EFUSE_USAGE_MAP_END;
	UINT8 EFUSE_USAGE_MAP_SIZE;
	UINT8 EFUSE_RESERVED_SIZE;
#endif /* RTMP_EFUSE_SUPPORT */

	UCHAR *EEPROM_DEFAULT_BIN;
	UINT16 EEPROM_DEFAULT_BIN_SIZE;

#ifdef RTMP_FLASH_SUPPORT
	BOOLEAN ee_inited;
#endif /* RTMP_FLASH_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	UCHAR carrier_func;
#endif /* CARRIER_DETECTION_SUPPORT */
#ifdef DFS_SUPPORT
	UINT8 DfsEngineNum;
#endif /* DFS_SUPPORT */

	/*
		Define the burst size of WPDMA of PCI
		0 : 4 DWORD (16bytes)
		1 : 8 DWORD (32 bytes)
		2 : 16 DWORD (64 bytes)
		3 : 32 DWORD (128 bytes)
	*/
	UINT8 WPDMABurstSIZE;

	/*
 	 * 0: MBSSID_MODE0
 	 * (The multiple MAC_ADDR/BSSID are distinguished by [bit2:bit0] of byte5)
 	 * 1: MBSSID_MODE1
 	 * (The multiple MAC_ADDR/BSSID are distinguished by [bit4:bit2] of byte0)
 	 */
	UINT8 MBSSIDMode;

#ifdef DOT11W_PMF_SUPPORT
#define PMF_ENCRYPT_MODE_0      0	/* All packets must software encryption. */
#define PMF_ENCRYPT_MODE_1      1	/* Data packets do hardware encryption, management packet do software encryption. */
#define PMF_ENCRYPT_MODE_2      2	/* Data and management packets do hardware encryption. */
	UINT8	FlgPMFEncrtptMode;
#endif /* DOT11W_PMF_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
	UINT8 FreqCalibrationSupport;
	UINT8 FreqCalInitMode;
	UINT8 FreqCalMode;
	UINT8 RxWIFrqOffset;
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef RT5592EP_SUPPORT
	UINT32 Priv; /* Flag for RT5592 EP */
#endif /* RT5592EP_SUPPORT */

#ifdef RT6352
	INT16 DPDCalPassThres;
	INT16 DPDCalPassLowThresTX0;
	INT16 DPDCalPassLowThresTX1;
	INT16 DPDCalPassHighThresTX0;
	INT16 DPDCalPassHighThresTX1;
#endif /* RT6352 */
#ifdef RT65xx
	UINT8 PAType; /* b'00: 2.4G+5G external PA, b'01: 5G external PA, b'10: 2.4G external PA, b'11: Internal PA */
#endif /* RT65xx */

#ifdef CONFIG_ANDES_SUPPORT
	UINT32 WlanMemmapOffset;
	UINT32 InbandPacketMaxLen; /* must be 48 multible */
	UINT8 CmdRspRxRing;
	BOOLEAN IsComboChip;
	BOOLEAN need_load_fw;
	BOOLEAN need_load_rom_patch;
	UINT8 ram_code_protect;
	UINT8 rom_code_protect;
	UINT8 load_iv;
	UINT32 ilm_offset;
	UINT32 dlm_offset;
	UINT32 rom_patch_offset;
	UINT8 DownLoadType;
#endif

	UINT8 cmd_header_len;
	UINT8 cmd_padding_len;

#ifdef SINGLE_SKU_V2
	CHAR	Apwrdelta;
	CHAR	Gpwrdelta;
#endif /* SINGLE_SKU_V2 */

#ifdef RTMP_USB_SUPPORT
	UINT8 DataBulkInAddr;
	UINT8 CommandRspBulkInAddr;
	UINT8 WMM0ACBulkOutAddr[5];//4 --> 5 for USE_BMC
	UINT8 WMM1ACBulkOutAddr;
	UINT8 CommandBulkOutAddr;
#endif

#ifdef CONFIG_SWITCH_CHANNEL_OFFLOAD
	UINT16 ChannelParamsSize;
	UCHAR *ChannelParam;
	INT XtalType;
#endif

	UCHAR load_code_method;
	UCHAR *FWImageName;
	UCHAR *fw_header_image;
	UCHAR *fw_bin_file_name;
	UCHAR *rom_patch;
	UCHAR *rom_patch_header_image;
	UCHAR *rom_patch_bin_file_name;
	UINT32 rom_patch_len;
	UINT32 fw_len;
	UCHAR *MACRegisterVer;
	UCHAR *BBPRegisterVer;
	UCHAR *RFRegisterVer;

#ifdef CONFIG_WIFI_TEST
	UINT32 MemMapStart;
	UINT32 MemMapEnd;
	UINT32 BBPMemMapOffset;
	UINT16 BBPStart;
	UINT16 BBPEnd;
	UINT16 RFStart;
	UINT16 RFEnd;
#ifdef RLT_RF
	UINT8 RFBankNum;
	struct RF_BANK_OFFSET *RFBankOffset;
#endif
#ifdef MT_RF
	UINT8 RFIndexNum;
	struct RF_INDEX_OFFSET *RFIndexOffset;
#endif
	UINT32 MacMemMapOffset;
	UINT32 MacStart;
	UINT32 MacEnd;
	UINT16 E2PStart;
	UINT16 E2PEnd;
	BOOLEAN pbf_loopback;
	BOOLEAN pbf_rx_drop;
#endif /* CONFIG_WIFI_TEST */

#if defined(MT76x2)
#define TSSI_TRIGGER_STAGE 0
#define TSSI_COMP_STAGE 1
#define TSSI_CAL_STAGE 2
	UINT8 tssi_stage;
#define TSSI_0_SLOPE_G_BAND_DEFAULT_VALUE 0x84
#define TSSI_1_SLOPE_G_BAND_DEFAULT_VALUE 0x83
	UINT8 tssi_0_slope_g_band;
	UINT8 tssi_1_slope_g_band;
#define TSSI_0_OFFSET_G_BAND_DEFAULT_VALUE 0x0A
#define TSSI_1_OFFSET_G_BAND_DEFAULT_VALUE 0x0B
	UINT8 tssi_0_offset_g_band;
	UINT8 tssi_1_offset_g_band;
#define A_BAND_GRP0_CHL 0
#define A_BAND_GRP1_CHL 1
#define A_BAND_GRP2_CHL 2
#define A_BAND_GRP3_CHL 3
#define A_BAND_GRP4_CHL 4
#define A_BAND_GRP5_CHL 5
#define A_BAND_GRP_NUM 6
#define TSSI_0_SLOPE_A_BAND_GRP0_DEFAULT_VALUE 0x7A
#define TSSI_1_SLOPE_A_BAND_GRP0_DEFAULT_VALUE 0x79
#define TSSI_0_SLOPE_A_BAND_GRP1_DEFAULT_VALUE 0x7A
#define TSSI_1_SLOPE_A_BAND_GRP1_DEFAULT_VALUE 0x79
#define TSSI_0_SLOPE_A_BAND_GRP2_DEFAULT_VALUE 0x78
#define TSSI_1_SLOPE_A_BAND_GRP2_DEFAULT_VALUE 0x7B
#define TSSI_0_SLOPE_A_BAND_GRP3_DEFAULT_VALUE 0x7E
#define TSSI_1_SLOPE_A_BAND_GRP3_DEFAULT_VALUE 0x77
#define TSSI_0_SLOPE_A_BAND_GRP4_DEFAULT_VALUE 0x76
#define TSSI_1_SLOPE_A_BAND_GRP4_DEFAULT_VALUE 0x7E
#define TSSI_0_SLOPE_A_BAND_GRP5_DEFAULT_VALUE 0x75
#define TSSI_1_SLOPE_A_BAND_GRP5_DEFAULT_VALUE 0x79
	UINT8 tssi_0_slope_a_band[A_BAND_GRP_NUM];
	UINT8 tssi_1_slope_a_band[A_BAND_GRP_NUM];
#define TSSI_0_OFFSET_A_BAND_GRP0_DEFAULT_VALUE 0x06
#define TSSI_1_OFFSET_A_BAND_GRP0_DEFAULT_VALUE 0x0A
#define TSSI_0_OFFSET_A_BAND_GRP1_DEFAULT_VALUE 0x06
#define TSSI_1_OFFSET_A_BAND_GRP1_DEFAULT_VALUE 0x0A
#define TSSI_0_OFFSET_A_BAND_GRP2_DEFAULT_VALUE 0x08
#define TSSI_1_OFFSET_A_BAND_GRP2_DEFAULT_VALUE 0x06
#define TSSI_0_OFFSET_A_BAND_GRP3_DEFAULT_VALUE 0x02
#define TSSI_1_OFFSET_A_BAND_GRP3_DEFAULT_VALUE 0x14
#define TSSI_0_OFFSET_A_BAND_GRP4_DEFAULT_VALUE 0x16
#define TSSI_1_OFFSET_A_BAND_GRP4_DEFAULT_VALUE 0x05
#define TSSI_0_OFFSET_A_BAND_GRP5_DEFAULT_VALUE 0x1C
#define TSSI_1_OFFSET_A_BAND_GRP5_DEFAULT_VALUE 0x11
	UINT8 tssi_0_offset_a_band[A_BAND_GRP_NUM];
	UINT8 tssi_1_offset_a_band[A_BAND_GRP_NUM];
#define TX_TARGET_PWR_DEFAULT_VALUE 0x14
	CHAR tx_0_target_pwr_g_band;
	CHAR tx_1_target_pwr_g_band;
	CHAR tx_0_target_pwr_a_band[A_BAND_GRP_NUM];
	CHAR tx_1_target_pwr_a_band[A_BAND_GRP_NUM];
	CHAR tx_0_chl_pwr_delta_g_band[3];
	CHAR tx_1_chl_pwr_delta_g_band[3];
	CHAR tx_0_chl_pwr_delta_a_band[A_BAND_GRP_NUM][2];
	CHAR tx_1_chl_pwr_delta_a_band[A_BAND_GRP_NUM][2];
	CHAR delta_tx_pwr_bw40_g_band;
	CHAR delta_tx_pwr_bw40_a_band;
	CHAR delta_tx_pwr_bw80;

	CHAR tx_pwr_cck_1_2;
	CHAR tx_pwr_cck_5_11;
	CHAR tx_pwr_g_band_ofdm_6_9;
	CHAR tx_pwr_g_band_ofdm_12_18;
	CHAR tx_pwr_g_band_ofdm_24_36;
	CHAR tx_pwr_g_band_ofdm_48_54;
	CHAR tx_pwr_ht_mcs_0_1;
	CHAR tx_pwr_ht_mcs_2_3;
	CHAR tx_pwr_ht_mcs_4_5;
	CHAR tx_pwr_ht_mcs_6_7;
	CHAR tx_pwr_ht_mcs_8_9;
	CHAR tx_pwr_ht_mcs_10_11;
	CHAR tx_pwr_ht_mcs_12_13;
	CHAR tx_pwr_ht_mcs_14_15;
	CHAR tx_pwr_a_band_ofdm_6_9;
	CHAR tx_pwr_a_band_ofdm_12_18;
	CHAR tx_pwr_a_band_ofdm_24_36;
	CHAR tx_pwr_a_band_ofdm_48_54;
	CHAR tx_pwr_vht_mcs_0_1;
	CHAR tx_pwr_vht_mcs_2_3;
	CHAR tx_pwr_vht_mcs_4_5;
	CHAR tx_pwr_vht_mcs_6_7;
	CHAR tx_pwr_2g_vht_mcs_8_9;
	CHAR tx_pwr_5g_vht_mcs_8_9;

	CHAR rf0_2g_rx_high_gain;
	CHAR rf1_2g_rx_high_gain;
	CHAR rf0_5g_grp0_rx_high_gain;
	CHAR rf1_5g_grp0_rx_high_gain;
	CHAR rf0_5g_grp1_rx_high_gain;
	CHAR rf1_5g_grp1_rx_high_gain;
	CHAR rf0_5g_grp2_rx_high_gain;
	CHAR rf1_5g_grp2_rx_high_gain;
	CHAR rf0_5g_grp3_rx_high_gain;
	CHAR rf1_5g_grp3_rx_high_gain;
	CHAR rf0_5g_grp4_rx_high_gain;
	CHAR rf1_5g_grp4_rx_high_gain;
	CHAR rf0_5g_grp5_rx_high_gain;
	CHAR rf1_5g_grp5_rx_high_gain;
	BOOLEAN chl_smth_enable;
#endif
	BOOLEAN tssi_enable;

#ifdef MT_MAC
	struct MT_TX_PWR_CAP MTTxPwrCap;
	UCHAR TmrEnable;
#endif

#ifdef MT76x0
	BOOLEAN temp_sensor_enable;
	SHORT temp_offset;
	SHORT last_vco_temp;
	SHORT last_full_cal_temp;
	SHORT current_temp;
	UCHAR a_band_mid_ch;
	UCHAR a_band_high_ch;
	UCHAR ext_pa_current_setting;
	MT76x0_RATE_PWR_Table rate_pwr_table;
	UCHAR delta_tw_pwr_bw40_5G;
	UCHAR delta_tw_pwr_bw40_2G;
	UCHAR delta_tw_pwr_bw80;
	BOOLEAN bInternalTxALC; /* Internal Tx ALC */
#ifdef MT76x0_TSSI_CAL_COMPENSATION
	UCHAR tssi_info_1;
	UCHAR tssi_info_2;
	UCHAR tssi_info_3;
	UCHAR tssi_2G_target_power;
	UCHAR tssi_5G_target_power;
	UCHAR efuse_2G_54M_tx_power;
	UCHAR efuse_5G_54M_tx_power;
	CHAR tssi_slope_2G;
	CHAR tssi_offset_2G;
	UCHAR tssi_slope_5G[8];
	UCHAR tssi_offset_5G[8];
	UCHAR tssi_5G_channel_boundary[7];
	CHAR tssi_current_DC;
	INT tssi_pre_delta_pwr;
#endif /* MT76x0_TSSI_CAL_COMPENSATION */
#endif /* MT76x0 */

	UINT8 TxBAWinSize;
	UINT8 RxBAWinSize;
	UINT8 AMPDUFactor;
    UINT8 BiTxOpOn;

    UINT32  CurrentTxOP;
};

#ifdef RT305x
typedef VOID (*CHIP_SPEC_FUNC)(VOID *pAd, VOID *pData, ULONG Data);

/* The chip specific function ID */
typedef enum _CHIP_SPEC_ID
{
	RT305x_WLAN_MODE_CHANGE,
	RT305x_INITIALIZATION,
	RT305x_HT_MODE_CHANGE,
	CHIP_SPEC_RESV_FUNC
} CHIP_SPEC_ID;

#define CHIP_SPEC_ID_NUM 	CHIP_SPEC_RESV_FUNC
#endif /* RT305x */

struct _RSSI_SAMPLE;

struct _RTMP_CHIP_OP {
	int (*sys_onoff)(struct _RTMP_ADAPTER *pAd, BOOLEAN on, BOOLEAN reser);

	/*  Calibration access related callback functions */
	int (*eeinit)(struct _RTMP_ADAPTER *pAd);
	BOOLEAN (*eeread)(struct _RTMP_ADAPTER *pAd, UINT16 offset, USHORT *pValue);
	int (*eewrite)(struct _RTMP_ADAPTER *pAd, USHORT offset, USHORT value);

	/* ITxBf calibration */
	int (*fITxBfDividerCalibration)(struct _RTMP_ADAPTER *pAd, int calFunction, int calMethod, UCHAR *divPhase);
	void (*fITxBfLNAPhaseCompensate)(struct _RTMP_ADAPTER *pAd);
	int (*fITxBfCal)(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg);
	int (*fITxBfLNACalibration)(struct _RTMP_ADAPTER *pAd, int calFunction, int calMethod, BOOLEAN gBand);

	/* MCU related callback functions */
	int (*load_rom_patch)(struct _RTMP_ADAPTER *ad);
	int (*erase_rom_patch)(struct _RTMP_ADAPTER *ad);
	int (*loadFirmware)(struct _RTMP_ADAPTER *pAd);
	int (*eraseFirmware)(struct _RTMP_ADAPTER *pAd);
	int (*sendCommandToMcu)(struct _RTMP_ADAPTER *pAd, UCHAR cmd, UCHAR token, UCHAR arg0, UCHAR arg1, BOOLEAN FlgIsNeedLocked);	/* int (*sendCommandToMcu)(RTMP_ADAPTER *pAd, UCHAR cmd, UCHAR token, UCHAR arg0, UCHAR arg1); */
#ifdef CONFIG_ANDES_SUPPORT
	int (*sendCommandToAndesMcu)(struct _RTMP_ADAPTER *pAd, UCHAR QueIdx, UCHAR cmd, UCHAR *pData, USHORT DataLen, BOOLEAN FlgIsNeedLocked);
#endif

	void (*AsicRfInit)(struct _RTMP_ADAPTER *pAd);
	void (*AsicBbpInit)(struct _RTMP_ADAPTER *pAd);
	void (*AsicMacInit)(struct _RTMP_ADAPTER *pAd);

	void (*AsicRfTurnOn)(struct _RTMP_ADAPTER *pAd);
	void (*AsicRfTurnOff)(struct _RTMP_ADAPTER *pAd);
	void (*AsicReverseRfFromSleepMode)(struct _RTMP_ADAPTER *pAd, BOOLEAN FlgIsInitState);
	void (*AsicHaltAction)(struct _RTMP_ADAPTER *pAd);

	/* Power save */
	void (*EnableAPMIMOPS)(struct _RTMP_ADAPTER *pAd, IN BOOLEAN ReduceCorePower);
	void (*DisableAPMIMOPS)(struct _RTMP_ADAPTER *pAd);
	INT (*PwrSavingOP)(struct _RTMP_ADAPTER *pAd, UINT32 PwrOP, UINT32 PwrLevel,
							UINT32 ListenInterval, UINT32 PreTBTTLeadTime,
							UINT8 TIMByteOffset, UINT8 TIMBytePattern);

	/* Chip tuning */
	VOID (*RxSensitivityTuning)(IN struct _RTMP_ADAPTER *pAd);

	/* MAC */
	VOID (*BeaconUpdate)(struct _RTMP_ADAPTER *pAd, USHORT Offset, UINT32 Value, UINT8 Unit);

	/* BBP adjust */
	VOID (*ChipBBPAdjust)(IN struct _RTMP_ADAPTER *pAd);

	/* AGC */
	VOID (*ChipAGCInit)(struct _RTMP_ADAPTER *pAd, UCHAR bw);
	UCHAR (*ChipAGCAdjust)(struct _RTMP_ADAPTER *pAd, CHAR Rssi, UCHAR OrigR66Value);
	
	/* Channel */
	VOID (*ChipSwitchChannel)(struct _RTMP_ADAPTER *pAd, UCHAR ch, BOOLEAN bScan);

	/* EDCCA */
	VOID (*ChipSetEDCCA)(struct _RTMP_ADAPTER *pAd, BOOLEAN bOn);

	/* IQ Calibration */
	VOID (*ChipIQCalibration)(struct _RTMP_ADAPTER *pAd, UCHAR Channel);

	/* TX ALC */
	UINT32 (*TSSIRatio)(INT32 delta_power);
	VOID (*InitDesiredTSSITable)(IN struct _RTMP_ADAPTER *pAd);
	int (*ATETssiCalibration)(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg);
	int (*ATETssiCalibrationExtend)(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg);
	int (*ATEReadExternalTSSI)(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg);

	VOID (*AsicTxAlcGetAutoAgcOffset)(
				IN struct _RTMP_ADAPTER	*pAd,
				IN PCHAR				pDeltaPwr,
				IN PCHAR				pTotalDeltaPwr,
				IN PCHAR				pAgcCompensate,
				IN PCHAR 				pDeltaPowerByBbpR1);
	
	VOID (*AsicGetTxPowerOffset)(struct _RTMP_ADAPTER *pAd, ULONG *TxPwr);
	VOID (*AsicExtraPowerOverMAC)(struct _RTMP_ADAPTER *pAd);

	VOID (*AsicAdjustTxPower)(struct _RTMP_ADAPTER *pAd);

	/* Antenna */
	VOID (*AsicAntennaDefaultReset)(struct _RTMP_ADAPTER *pAd, union _EEPROM_ANTENNA_STRUC *pAntenna);
	VOID (*SetRxAnt)(struct _RTMP_ADAPTER *pAd, UCHAR Ant);

	/* EEPROM */
	VOID (*NICInitAsicFromEEPROM)(IN struct _RTMP_ADAPTER *pAd);

	/* Temperature Compensation */
	VOID (*InitTemperCompensation)(IN struct _RTMP_ADAPTER *pAd);
	VOID (*TemperCompensation)(IN struct _RTMP_ADAPTER *pAd);
	
	/* high power tuning */
	VOID (*HighPowerTuning)(struct _RTMP_ADAPTER *pAd, struct _RSSI_SAMPLE *pRssi);
	
	/* Others */
	VOID (*NetDevNickNameInit)(IN struct _RTMP_ADAPTER *pAd);
#ifdef CAL_FREE_IC_SUPPORT
	BOOLEAN (*is_cal_free_ic)(IN struct _RTMP_ADAPTER *pAd);
	VOID (*cal_free_data_get)(IN struct _RTMP_ADAPTER *pAd);
#endif /* CAL_FREE_IC_SUPPORT */

	/* The chip specific function list */
#ifdef RT305x
	// TODO: shiang-usw, need to rewrite this function definition becasue "CHIP_SPEC_ID_NUM" is declared in enum!
	CHIP_SPEC_FUNC ChipSpecFunc[CHIP_SPEC_ID_NUM];
#endif /* RT305x */
	
	VOID (*AsicResetBbpAgent)(IN struct _RTMP_ADAPTER *pAd);

#ifdef ANT_DIVERSITY_SUPPORT
	VOID (*HwAntEnable)(struct _RTMP_ADAPTER *pAd);
#endif /* ANT_DIVERSITY_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	VOID (*ToneRadarProgram)(struct _RTMP_ADAPTER *pAd, ULONG  threshold);
#endif /* CARRIER_DETECTION_SUPPORT */
	VOID (*CckMrcStatusCtrl)(struct _RTMP_ADAPTER *pAd);
	VOID (*RadarGLRTCompensate)(struct _RTMP_ADAPTER *pAd);
	VOID (*SecondCCADetection)(struct _RTMP_ADAPTER *pAd);

	/* MCU */
	void (*MCUCtrlInit)(struct _RTMP_ADAPTER *ad);
	void (*MCUCtrlExit)(struct _RTMP_ADAPTER *ad);
	VOID (*FwInit)(struct _RTMP_ADAPTER *pAd);
	VOID (*FwExit)(struct _RTMP_ADAPTER *pAd);
	int (*BurstWrite)(struct _RTMP_ADAPTER *ad, UINT32 Offset, UINT32 *Data, UINT32 Cnt);
	int (*BurstRead)(struct _RTMP_ADAPTER *ad, UINT32 Offset, UINT32 Cnt, UINT32 *Data);
	int (*RandomRead)(struct _RTMP_ADAPTER *ad, RTMP_REG_PAIR *RegPair, UINT32 Num);
	int (*RFRandomRead)(struct _RTMP_ADAPTER *ad, BANK_RF_REG_PAIR *RegPair, UINT32 Num);
	int (*ReadModifyWrite)(struct _RTMP_ADAPTER *ad, R_M_W_REG *RegPair, UINT32 Num);
	int (*RFReadModifyWrite)(struct _RTMP_ADAPTER *ad, RF_R_M_W_REG *RegPair, UINT32 Num);
	int (*RandomWrite)(struct _RTMP_ADAPTER *ad, RTMP_REG_PAIR *RegPair, UINT32 Num);
	int (*RFRandomWrite)(struct _RTMP_ADAPTER *ad, BANK_RF_REG_PAIR *RegPair, UINT32 Num);
#ifdef CONFIG_ANDES_BBP_RANDOM_WRITE_SUPPORT
	int (*BBPRandomWrite)(struct _RTMP_ADAPTER *ad, RTMP_REG_PAIR *RegPair, UINT32 Num);
#endif /* CONFIG_ANDES_BBP_RANDOM_WRITE_SUPPORT */
	int (*sc_random_write)(struct _RTMP_ADAPTER *ad, CR_REG *table, UINT32 num, UINT32 flags);
	int (*sc_rf_random_write)(struct _RTMP_ADAPTER *ad, BANK_RF_CR_REG *table, UINT32 num, UINT32 flags);
	int (*DisableTxRx)(struct _RTMP_ADAPTER *ad, UCHAR Level);
	void (*AsicRadioOn)(struct _RTMP_ADAPTER *ad, UCHAR Stage);
	void (*AsicRadioOff)(struct _RTMP_ADAPTER *ad, UINT8 Stage);
#ifdef RLT_MAC
	// TODO: shiang-usw, currently the "ANDES_CALIBRATION_PARAM" is defined in andes_rlt.h
	int (*Calibration)(struct _RTMP_ADAPTER *pAd, UINT32 CalibrationID, ANDES_CALIBRATION_PARAM *param);
#endif /* RLT_MAC */
#ifdef CONFIG_ANDES_SUPPORT
#ifdef RTMP_PCI_SUPPORT
	int (*pci_kick_out_cmd_msg)(struct _RTMP_ADAPTER *ad, struct cmd_msg *msg);
#endif
#ifdef RTMP_USB_SUPPORT
	int (*usb_kick_out_cmd_msg)(struct _RTMP_ADAPTER *ad, struct cmd_msg *msg);
#endif
#ifdef RTMP_SDIO_SUPPORT
	int (*sdio_kick_out_cmd_msg)(struct _RTMP_ADAPTER *ad, struct cmd_msg *msg);
#endif

	void (*andes_fill_cmd_header)(struct cmd_msg *msg, PNDIS_PACKET net_pkt);
	INT32 (*AndesMTChkCrc)(struct _RTMP_ADAPTER *pAd, UINT32 checksum_len);
	UINT16 (*AndesMTGetCrc)(struct _RTMP_ADAPTER *pAd);
#endif /* CONFIG_ANDES_SUPPORT */
	void (*rx_event_handler)(struct _RTMP_ADAPTER *ad, UCHAR *data);

#ifdef MICROWAVE_OVEN_SUPPORT
	VOID (*AsicMeasureFalseCCA)(IN struct _RTMP_ADAPTER *pAd);

	VOID (*AsicMitigateMicrowave)(IN struct _RTMP_ADAPTER *pAd);
#endif /* MICROWAVE_OVEN_SUPPORT */

#if (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT)
	VOID (*AsicWOWEnable)(struct _RTMP_ADAPTER *ad);
	VOID (*AsicWOWDisable)(struct _RTMP_ADAPTER *ad);
	VOID (*AsicWOWInit)(struct _RTMP_ADAPTER *ad);
#endif /* (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT) */

	void (*usb_cfg_read)(struct _RTMP_ADAPTER *ad, UINT32 *value);
	void (*usb_cfg_write)(struct _RTMP_ADAPTER *ad, UINT32 value);
	void (*show_pwr_info)(struct _RTMP_ADAPTER *ad);
	void (*cal_test)(struct _RTMP_ADAPTER *ad, UINT32 type);
};

#define RTMP_CHIP_ENABLE_AP_MIMOPS(__pAd, __ReduceCorePower)	\
do {	\
		if (__pAd->chipOps.EnableAPMIMOPS != NULL)	\
			__pAd->chipOps.EnableAPMIMOPS(__pAd, __ReduceCorePower);	\
} while (0)

#define RTMP_CHIP_DISABLE_AP_MIMOPS(__pAd)	\
do {	\
		if (__pAd->chipOps.DisableAPMIMOPS != NULL)	\
			__pAd->chipOps.DisableAPMIMOPS(__pAd);	\
} while (0)
	
#define PWR_SAVING_OP(__pAd, __PwrOP, __PwrLevel, __ListenInterval, \
						__PreTBTTLeadTime, __TIMByteOffset, __TIMBytePattern)	\
do {	\
		if (__pAd->chipOps.PwrSavingOP != NULL)	\
			__pAd->chipOps.PwrSavingOP(__pAd, __PwrOP, __PwrLevel,	\
										__ListenInterval,__PreTBTTLeadTime, \
										__TIMByteOffset, __TIMBytePattern);	\
} while (0)

#define RTMP_CHIP_RX_SENSITIVITY_TUNING(__pAd)	\
do {	\
		if (__pAd->chipOps.RxSensitivityTuning != NULL)	\
			__pAd->chipOps.RxSensitivityTuning(__pAd);	\
} while (0)

#define RTMP_CHIP_ASIC_AGC_ADJUST(__pAd, __Rssi, __R66)	\
do {	\
		if (__pAd->chipOps.ChipAGCAdjust != NULL)	\
			__R66 = __pAd->chipOps.ChipAGCAdjust(__pAd, __Rssi, __R66);	\
} while (0)

#define RTMP_CHIP_ASIC_TSSI_TABLE_INIT(__pAd)	\
do {	\
		if (__pAd->chipOps.InitDesiredTSSITable != NULL)	\
			__pAd->chipOps.InitDesiredTSSITable(__pAd);	\
} while (0)

#define RTMP_CHIP_ATE_TSSI_CALIBRATION(__pAd, __pData)	\
do {	\
		if (__pAd->chipOps.ATETssiCalibration != NULL)	\
			__pAd->chipOps.ATETssiCalibration(__pAd, __pData);	\
} while (0)

#define RTMP_CHIP_ATE_TSSI_CALIBRATION_EXTEND(__pAd, __pData)	\
do {	\
		if (__pAd->chipOps.ATETssiCalibrationExtend != NULL)	\
			__pAd->chipOps.ATETssiCalibrationExtend(__pAd, __pData);	\
} while (0)

#define RTMP_CHIP_ATE_READ_EXTERNAL_TSSI(__pAd, __pData)	\
do {	\
		if (__pAd->chipOps.ATEReadExternalTSSI != NULL)	\
			__pAd->chipOps.ATEReadExternalTSSI(__pAd, __pData);	\
} while (0)

#define RTMP_CHIP_ASIC_TX_POWER_OFFSET_GET(__pAd, __pCfgOfTxPwrCtrlOverMAC)	\
do {	\
		if (__pAd->chipOps.AsicGetTxPowerOffset != NULL)	\
			__pAd->chipOps.AsicGetTxPowerOffset(__pAd, __pCfgOfTxPwrCtrlOverMAC);	\
} while (0)
		
#define RTMP_CHIP_ASIC_AUTO_AGC_OFFSET_GET(	\
		__pAd, __pDeltaPwr, __pTotalDeltaPwr, __pAgcCompensate, __pDeltaPowerByBbpR1)	\
do {	\
		if (__pAd->chipOps.AsicTxAlcGetAutoAgcOffset != NULL)	\
			__pAd->chipOps.AsicTxAlcGetAutoAgcOffset(	\
		__pAd, __pDeltaPwr, __pTotalDeltaPwr, __pAgcCompensate, __pDeltaPowerByBbpR1);	\
} while (0)

#define RTMP_CHIP_ASIC_EXTRA_POWER_OVER_MAC(__pAd)	\
do {	\
		if (__pAd->chipOps.AsicExtraPowerOverMAC != NULL)	\
			__pAd->chipOps.AsicExtraPowerOverMAC(__pAd);	\
} while (0)

#define RTMP_CHIP_ASIC_ADJUST_TX_POWER(__pAd)					\
do {	\
		if (__pAd->chipOps.AsicAdjustTxPower != NULL)					\
			__pAd->chipOps.AsicAdjustTxPower(__pAd);	\
} while (0)

#define RTMP_CHIP_ASIC_GET_TSSI_RATIO(__pAd, __DeltaPwr)	\
do {	\
		if (__pAd->chipOps.AsicFreqCalStop != NULL)	\
			__pAd->chipOps.TSSIRatio(__DeltaPwr);	\
} while (0)

#define RTMP_CHIP_ASIC_FREQ_CAL_STOP(__pAd)	\
do {	\
		if (__pAd->chipOps.AsicFreqCalStop != NULL)	\
			__pAd->chipOps.AsicFreqCalStop(__pAd);	\
} while (0)

#define RTMP_CHIP_IQ_CAL(__pAd, __pChannel)	\
do {	\
		if (__pAd->chipOps.ChipIQCalibration != NULL)	\
			 __pAd->chipOps.ChipIQCalibration(__pAd, __pChannel);	\
} while (0)

#define RTMP_CHIP_HIGH_POWER_TUNING(__pAd, __pRssi)	\
do {	\
		if (__pAd->chipOps.HighPowerTuning != NULL)	\
			__pAd->chipOps.HighPowerTuning(__pAd, __pRssi);	\
} while (0)

#define RTMP_CHIP_ANTENNA_INFO_DEFAULT_RESET(__pAd, __pAntenna)	\
do {	\
		if (__pAd->chipOps.AsicAntennaDefaultReset != NULL)	\
			__pAd->chipOps.AsicAntennaDefaultReset(__pAd, __pAntenna);	\
} while (0)

#define RTMP_NET_DEV_NICKNAME_INIT(__pAd)	\
do {	\
		if (__pAd->chipOps.NetDevNickNameInit != NULL)	\
			__pAd->chipOps.NetDevNickNameInit(__pAd);	\
} while (0)

#ifdef CAL_FREE_IC_SUPPORT
#define RTMP_CAL_FREE_IC_CHECK(__pAd, __is_cal_free)	\
do {	\
		if (__pAd->chipOps.is_cal_free_ic != NULL)	\
			__is_cal_free = __pAd->chipOps.is_cal_free_ic(__pAd);	\
		else		\
			__is_cal_free = FALSE;	\
} while (0)

#define RTMP_CAL_FREE_DATA_GET(__pAd)	\
do {	\
		if (__pAd->chipOps.cal_free_data_get != NULL)	\
			__pAd->chipOps.cal_free_data_get(__pAd);	\
} while (0)
#endif /* CAL_FREE_IC_SUPPORT */

#define RTMP_EEPROM_ASIC_INIT(__pAd)	\
do {	\
		if (__pAd->chipOps.NICInitAsicFromEEPROM != NULL)	\
			__pAd->chipOps.NICInitAsicFromEEPROM(__pAd);	\
} while (0)

#define RTMP_CHIP_ASIC_INIT_TEMPERATURE_COMPENSATION(__pAd)								\
do {	\
		if (__pAd->chipOps.InitTemperCompensation != NULL)					\
			__pAd->chipOps.InitTemperCompensation(__pAd);	\
} while (0)

#define RTMP_CHIP_ASIC_TEMPERATURE_COMPENSATION(__pAd)						\
do {	\
		if (__pAd->chipOps.TemperCompensation != NULL)					\
			__pAd->chipOps.TemperCompensation(__pAd);	\
} while (0)

#define RTMP_CHIP_ASIC_SET_EDCCA(__pAd,__bOn)			\
do {	\
		if (__pAd->chipOps.ChipSetEDCCA != NULL)						\
			__pAd->chipOps.ChipSetEDCCA(__pAd, __bOn);	\
} while (0)


#ifdef RT305x
#define RTMP_CHIP_SPECIFIC(__pAd, __FuncId, __pData, __Data)	\
do {	\
		if ((__FuncId >= 0) && (__FuncId < CHIP_SPEC_RESV_FUNC))	\
		{	\
			if (__pAd->chipOps.ChipSpecFunc[__FuncId] != NULL)	\
				__pAd->chipOps.ChipSpecFunc[__FuncId](__pAd, __pData, __Data);	\
		}	\
} while (0)
#endif /* RT305x */

#define RTMP_CHIP_UPDATE_BEACON(__pAd, Offset, Value, Unit)	\
do {	\
		if (__pAd->chipOps.BeaconUpdate != NULL)	\
			__pAd->chipOps.BeaconUpdate(__pAd, Offset, Value, Unit);	\
} while (0)

#ifdef CARRIER_DETECTION_SUPPORT
#define	RTMP_CHIP_CARRIER_PROGRAM(__pAd, threshold)	\
do {	\
		if(__pAd->chipOps.ToneRadarProgram != NULL)	\
			__pAd->chipOps.ToneRadarProgram(__pAd, threshold);	\
} while (0)
#endif /* CARRIER_DETECTION_SUPPORT */

#define RTMP_CHIP_CCK_MRC_STATUS_CTRL(__pAd)	\
do {	\
		if(__pAd->chipOps.CckMrcStatusCtrl != NULL)	\
			__pAd->chipOps.CckMrcStatusCtrl(__pAd);	\
} while (0)

#define RTMP_CHIP_RADAR_GLRT_COMPENSATE(__pAd) \
do {	\
		if(__pAd->chipOps.RadarGLRTCompensate != NULL)	\
			__pAd->chipOps.RadarGLRTCompensate(__pAd);	\
} while (0)


#define CHIP_CALIBRATION(__pAd, __CalibrationID, param) \
do {	\
	ANDES_CALIBRATION_PARAM calibration_param; \
	calibration_param.generic = param; \
	if(__pAd->chipOps.Calibration != NULL) \
		__pAd->chipOps.Calibration(__pAd, __CalibrationID, &calibration_param); \
} while (0)

#define BURST_WRITE(_pAd, _Offset, _pData, _Cnt)	\
do {												\
		if (_pAd->chipOps.BurstWrite != NULL)		\
			_pAd->chipOps.BurstWrite(_pAd, _Offset, _pData, _Cnt);\
} while (0)

#define BURST_READ(_pAd, _Offset, _Cnt, _pData)	\
do {											\
		if (_pAd->chipOps.BurstRead != NULL)	\
			_pAd->chipOps.BurstRead(_pAd, _Offset, _Cnt, _pData);	\
} while (0)

#define RANDOM_READ(_pAd, _RegPair, _Num)	\
do {										\
		if (_pAd->chipOps.RandomRead != NULL)	\
			_pAd->chipOps.RandomRead(_pAd, _RegPair, _Num);	\
} while (0)

#define RF_RANDOM_READ(_pAd, _RegPair, _Num)	\
do {											\
		if (_pAd->chipOps.RFRandomRead != NULL)	\
			_pAd->chipOps.RFRandomRead(_pAd, _RegPair, _Num); \
} while (0)

#define READ_MODIFY_WRITE(_pAd, _RegPair, _Num)	\
do {	\
		if (_pAd->chipOps.ReadModifyWrite != NULL)	\
			_pAd->chipOps.ReadModifyWrite(_pAd, _RegPair, _Num);	\
} while (0)

#define RF_READ_MODIFY_WRITE(_pAd, _RegPair, _Num)	\
do {	\
		if (_pAd->chipOps.RFReadModifyWrite != NULL)	\
			_pAd->chipOps.RFReadModifyWrite(_pAd, _RegPair, _Num);	\
} while (0)

#define RANDOM_WRITE(_pAd, _RegPair, _Num)	\
do {	\
		if (_pAd->chipOps.RandomWrite != NULL)	\
			_pAd->chipOps.RandomWrite(_pAd, _RegPair, _Num);	\
} while (0)

#define RF_RANDOM_WRITE(_pAd, _RegPair, _Num)	\
do {	\
		if (_pAd->chipOps.RFRandomWrite != NULL)	\
			_pAd->chipOps.RFRandomWrite(_pAd, _RegPair, _Num);	\
} while (0)

#ifdef CONFIG_ANDES_BBP_RANDOM_WRITE_SUPPORT
#define BBP_RANDOM_WRITE(_pAd, _RegPair, _Num)	\
do {	\
		if (_pAd->chipOps.BBPRandomWrite != NULL)	\
			_pAd->chipOps.BBPRandomWrite(_pAd, _RegPair, _Num);	\
} while (0)
#endif /* CONFIG_ANDES_BBP_RANDOM_WRITE_SUPPORT */

#if 0
	/* MT7650 E2 doesn't need SW to do 2nd CCA dectection. */
#endif
#define RTMP_SECOND_CCA_DETECTION(__pAd) \
do {	\
	if (__pAd->chipOps.SecondCCADetection != NULL)	\
	{	\
		__pAd->chipOps.SecondCCADetection(__pAd);	\
	}	\
} while (0)

#define SC_RANDOM_WRITE(_ad, _table, _num, _flags)	\
do {	\
		if (_ad->chipOps.sc_random_write != NULL)	\
			_ad->chipOps.sc_random_write(_ad, _table, _num, _flags);	\
} while (0)

#define SC_RF_RANDOM_WRITE(_ad, _table, _num, _flags)	\
do {	\
		if (_ad->chipOps.sc_rf_random_write != NULL)	\
			_ad->chipOps.sc_rf_random_write(_ad, _table, _num, _flags);	\
} while (0)

#define DISABLE_TX_RX(_pAd, _Level)	\
do {	\
	if (_pAd->chipOps.DisableTxRx != NULL)	\
		_pAd->chipOps.DisableTxRx(_pAd, _Level);	\
} while (0)

#define ASIC_RADIO_ON(_pAd, _Stage)	\
do {	\
	if (_pAd->chipOps.AsicRadioOn != NULL)	\
		_pAd->chipOps.AsicRadioOn(_pAd, _Stage);	\
} while (0)

#define ASIC_RADIO_OFF(_pAd, _Stage)	\
do {	\
	if (_pAd->chipOps.AsicRadioOff != NULL)	\
		_pAd->chipOps.AsicRadioOff(_pAd, _Stage);	\
} while (0)

#ifdef MICROWAVE_OVEN_SUPPORT
#define ASIC_MEASURE_FALSE_CCA(_pAd)	\
do {	\
	if (_pAd->chipOps.AsicMeasureFalseCCA != NULL)	\
		_pAd->chipOps.AsicMeasureFalseCCA(_pAd);	\
} while (0)

#define ASIC_MITIGATE_MICROWAVE(_pAd)	\
do {	\
	if (_pAd->chipOps.AsicMitigateMicrowave != NULL)	\
		_pAd->chipOps.AsicMitigateMicrowave(_pAd);	\
} while (0)
#endif /* MICROWAVE_OVEN_SUPPORT */

#if (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB))|| defined(MT_WOW_SUPPORT)
#define ASIC_WOW_ENABLE(_pAd)	\
do {	\
	if (_pAd->chipOps.AsicWOWEnable != NULL)	\
		_pAd->chipOps.AsicWOWEnable(_pAd);	\
} while (0)

#define ASIC_WOW_DISABLE(_pAd)	\
do {	\
	if (_pAd->chipOps.AsicWOWDisable != NULL)	\
		_pAd->chipOps.AsicWOWDisable(_pAd);	\
} while(0)

#define ASIC_WOW_INIT(_pAd) \
do {	\
	if (_pAd->chipOps.AsicWOWInit != NULL)	\
		_pAd->chipOps.AsicWOWInit(_pAd);	\
} while(0)

#endif /* (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT) */

#define MCU_CTRL_INIT(_pAd)	\
do {	\
	if (_pAd->chipOps.MCUCtrlInit != NULL)	\
		_pAd->chipOps.MCUCtrlInit(_pAd);	\
} while (0)

#define MCU_CTRL_EXIT(_pAd)	\
do {	\
	if (_pAd->chipOps.MCUCtrlExit != NULL)	\
		_pAd->chipOps.MCUCtrlExit(_pAd);	\
} while (0)

#define USB_CFG_READ(_ad, _pvalue)	\
do {	\
	if (_ad->chipOps.usb_cfg_read != NULL)	\
		_ad->chipOps.usb_cfg_read(_ad, _pvalue);	\
	else {\
		DBGPRINT(RT_DEBUG_ERROR, ("%s(): usb_cfg_read not inited!\n", __FUNCTION__));\
	}\
} while (0)

#define USB_CFG_WRITE(_ad, _value)	\
do {	\
	if (_ad->chipOps.usb_cfg_write != NULL)	\
		_ad->chipOps.usb_cfg_write(_ad, _value);	\
	else {\
		DBGPRINT(RT_DEBUG_ERROR, ("%s(): usb_cfg_write not inited!\n", __FUNCTION__));\
	}\
} while (0)

VOID AsicInitTxRxRing(struct _RTMP_ADAPTER *pAd);

int RtmpChipOpsHook(VOID *pCB);
VOID RtmpChipBcnInit(struct _RTMP_ADAPTER *pAd);
VOID RtmpChipBcnSpecInit(struct _RTMP_ADAPTER *pAd);
#ifdef RLT_MAC
VOID rlt_bcn_buf_init(struct _RTMP_ADAPTER *pAd);
#endif /* RLT_MAC */
#ifdef MT_MAC
VOID mt_bcn_buf_init(struct _RTMP_ADAPTER *pAd);
#endif /* MT_MAC */

UINT8 NICGetBandSupported(struct _RTMP_ADAPTER *pAd);

VOID RtmpChipWriteHighMemory(
	IN	struct _RTMP_ADAPTER *pAd,
	IN	USHORT			Offset,
	IN	UINT32			Value,
	IN	UINT8			Unit);

VOID RtmpChipWriteMemory(
	IN	struct _RTMP_ADAPTER *pAd,
	IN	USHORT			Offset,
	IN	UINT32			Value,
	IN	UINT8			Unit);

VOID RTMPReadChannelPwr(struct _RTMP_ADAPTER *pAd);
VOID RTMPReadTxPwrPerRate(struct _RTMP_ADAPTER *pAd);

INT tx_pwr_comp_init(struct _RTMP_ADAPTER *pAd);

#ifdef IQ_CAL_SUPPORT
VOID GetIQCalibration(struct _RTMP_ADAPTER *pAd);
VOID IQCalibrationViaBBPAccessSpace(struct _RTMP_ADAPTER *pAd, UCHAR Channel);
UCHAR IQCal(
	IN enum IQ_CAL_CHANNEL_INDEX ChannelIndex,
	IN enum IQ_CAL_TXRX_CHAIN TxRxChain,
	IN enum IQ_CAL_TYPE IQCalType);
#endif /* IQ_CAL_SUPPORT */

VOID NetDevNickNameInit(IN struct _RTMP_ADAPTER *pAd);


#ifdef ANT_DIVERSITY_SUPPORT
VOID HWAntennaDiversityEnable(struct _RTMP_ADAPTER *pAd);
#endif /* ANT_DIVERSITY_SUPPORT */

#ifdef GREENAP_SUPPORT
VOID EnableAPMIMOPSv2(struct _RTMP_ADAPTER *pAd, BOOLEAN ReduceCorePower);
VOID DisableAPMIMOPSv2(struct _RTMP_ADAPTER *pAd);
VOID EnableAPMIMOPSv1(struct _RTMP_ADAPTER *pAd, BOOLEAN ReduceCorePower);
VOID DisableAPMIMOPSv1(struct _RTMP_ADAPTER *pAd);
#endif /* GREENAP_SUPPORT */

#ifdef RTMP_MAC
VOID RTxx_default_Init(struct _RTMP_ADAPTER *pAd);
#endif /* RTMP_MAC */


/* global variable */
extern FREQUENCY_ITEM RtmpFreqItems3020[];
extern FREQUENCY_ITEM FreqItems3020_Xtal20M[];
extern UCHAR NUM_OF_3020_CHNL;
extern FREQUENCY_ITEM *FreqItems3020;
extern RTMP_RF_REGS RF2850RegTable[];
extern UCHAR NUM_OF_2850_CHNL;

INT AsicGetMacVersion(struct _RTMP_ADAPTER *pAd);

INT WaitForAsicReady(struct _RTMP_ADAPTER *pAd);

INT AsicWaitMacTxRxIdle(struct _RTMP_ADAPTER *pAd);

#define ASIC_MAC_TX			1
#define ASIC_MAC_RX			2
#define ASIC_MAC_TXRX		3
#define ASIC_MAC_TXRX_RXV	4
#define ASIC_MAC_RXV		5
#define ASIC_MAC_RX_RXV		6

INT AsicSetMacTxRx(struct _RTMP_ADAPTER *pAd, INT32 txrx, BOOLEAN enable);

enum {
	PDMA_TX,
	PDMA_RX,
	PDMA_TX_RX,
};

INT AsicSetWPDMA(struct _RTMP_ADAPTER *pAd, INT32 TxRx, BOOLEAN enable);
BOOLEAN AsicWaitPDMAIdle(struct _RTMP_ADAPTER *pAd, INT round, INT wait_us);
INT AsicSetMacWD(struct _RTMP_ADAPTER *pAd);

INT AsicReadAggCnt(struct _RTMP_ADAPTER *pAd, ULONG *aggCnt, int cnt_len);

INT rtmp_asic_top_init(struct _RTMP_ADAPTER *pAd);

#ifdef MT_MAC
INT mt_asic_top_init(struct _RTMP_ADAPTER *pAd);
INT mt_hif_sys_init(struct _RTMP_ADAPTER *pAd);
INT rt28xx_read16(struct _RTMP_ADAPTER *pAd, USHORT , USHORT *);
VOID AsicSetRxGroup(struct _RTMP_ADAPTER *pAd, UINT32 Port, UINT32 Group, BOOLEAN Enable);
#if defined(RTMP_MAC_PCI) || defined(RTMP_MAC_USB)
INT32 AsicDMASchedulerInit(struct _RTMP_ADAPTER *pAd, INT mode);
#endif
VOID AsicSetBARTxCntLimit(struct _RTMP_ADAPTER *pAd, BOOLEAN Enable, UINT32 Count);
VOID AsicSetRTSTxCntLimit(struct _RTMP_ADAPTER *pAd, BOOLEAN Enable, UINT32 Count);
VOID AsicSetTxSClassifyFilter(struct _RTMP_ADAPTER *pAd, UINT32 Port, UINT8 DestQ, 
										UINT32 AggNums, UINT32 Filter);

BOOLEAN AsicSetBmcQCR(
        IN struct _RTMP_ADAPTER *pAd,
        IN UCHAR Operation,
        IN UCHAR CrReadWrite,
        IN UINT32 apidx,
        INOUT UINT32    *pcr_val);

#define CR_READ         1
#define CR_WRITE        2

#define BMC_FLUSH       1
#define BMC_ENABLE      2
#define BMC_CNT_UPDATE  3

#define AC_QUEUE_STOP 0
#define AC_QUEUE_FLUSH 1
#define AC_QUEUE_START 2


#endif /* MT_MAC */

INT StopDmaTx(struct _RTMP_ADAPTER *pAd, UCHAR Level);
INT StopDmaRx(struct _RTMP_ADAPTER *pAd, UCHAR Level);

#ifdef RT65xx
BOOLEAN isExternalPAMode(struct _RTMP_ADAPTER *ad, INT channel);
#endif /* RT65xx */

VOID MtAsicSetRxPspollFilter(struct _RTMP_ADAPTER *pAd, CHAR enable);

#if defined(MT7603) || defined(MT7628)
INT32 MtAsicGetThemalSensor(struct _RTMP_ADAPTER *pAd, CHAR type);
VOID MtAsicACQueue(struct _RTMP_ADAPTER *pAd, UINT8 ucation, UINT8 BssidIdx, UINT32 u4AcQueueMap);
#endif /* MT7603 || MT7628 */
#endif /* __RTMP_CHIP_H__ */

