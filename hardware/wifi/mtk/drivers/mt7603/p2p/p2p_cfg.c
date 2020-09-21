/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
	ap_cfg.c

    Abstract:
    IOCTL related subroutines

    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
*/

#ifdef WFD_SUPPORT
#ifdef OS_ABL_SUPPORT
#ifdef RT_CFG80211_SUPPORT
#include <linux/version.h>
#include <net/cfg80211.h>
#endif /* RT_CFG80211_SUPPORT */
#endif /* OS_ABL_SUPPORT */
#endif /* WFD_SUPPORT */

#include "rt_config.h"
#ifdef WFD_SUPPORT
#ifdef OS_ABL_SUPPORT
#ifdef RT_CFG80211_SUPPORT
#include "cfg80211.h"
#endif /* RT_CFG80211_SUPPORT */
#endif /* OS_ABL_SUPPORT */
#endif /* WFD_SUPPORT */

#define A_BAND_REGION_0				0
#define A_BAND_REGION_1				1
#define A_BAND_REGION_2				2
#define A_BAND_REGION_3				3
#define A_BAND_REGION_4				4
#define A_BAND_REGION_5				5
#define A_BAND_REGION_6				6
#define A_BAND_REGION_7				7
#define A_BAND_REGION_8				8
#define A_BAND_REGION_9				9
#define A_BAND_REGION_10			10

#define G_BAND_REGION_0				0
#define G_BAND_REGION_1				1
#define G_BAND_REGION_2				2
#define G_BAND_REGION_3				3
#define G_BAND_REGION_4				4
#define G_BAND_REGION_5				5
#define G_BAND_REGION_6				6

COUNTRY_CODE_TO_COUNTRY_REGION allCountry[] = {
	/* {Country Number, ISO Name, Country Name, Support 11A, 11A Country Region, Support 11G, 11G Country Region} */
	{0,		"DB",	"Debug",				TRUE,	A_BAND_REGION_7,	TRUE,	G_BAND_REGION_5},
	{8,		"AL",	"ALBANIA",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{12,	"DZ",	"ALGERIA",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{32,	"AR",	"ARGENTINA",			TRUE,	A_BAND_REGION_3,	TRUE,	G_BAND_REGION_1},
	{51,	"AM",	"ARMENIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{36,	"AU",	"AUSTRALIA",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{40,	"AT",	"AUSTRIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{31,	"AZ",	"AZERBAIJAN",			TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{48,	"BH",	"BAHRAIN",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{112,	"BY",	"BELARUS",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{56,	"BE",	"BELGIUM",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{84,	"BZ",	"BELIZE",				TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{68,	"BO",	"BOLIVIA",				TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{76,	"BR",	"BRAZIL",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{96,	"BN",	"BRUNEI DARUSSALAM",	TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{100,	"BG",	"BULGARIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{124,	"CA",	"CANADA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{152,	"CL",	"CHILE",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{156,	"CN",	"CHINA",				TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{170,	"CO",	"COLOMBIA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{188,	"CR",	"COSTA RICA",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{191,	"HR",	"CROATIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{196,	"CY",	"CYPRUS",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{203,	"CZ",	"CZECH REPUBLIC",		TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{208,	"DK",	"DENMARK",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{214,	"DO",	"DOMINICAN REPUBLIC",	TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{218,	"EC",	"ECUADOR",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{818,	"EG",	"EGYPT",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{222,	"SV",	"EL SALVADOR",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{233,	"EE",	"ESTONIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{246,	"FI",	"FINLAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{250,	"FR",	"FRANCE",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{268,	"GE",	"GEORGIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{276,	"DE",	"GERMANY",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{300,	"GR",	"GREECE",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{320,	"GT",	"GUATEMALA",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{340,	"HN",	"HONDURAS",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{344,	"HK",	"HONG KONG",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{348,	"HU",	"HUNGARY",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{352,	"IS",	"ICELAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{356,	"IN",	"INDIA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{360,	"ID",	"INDONESIA",			TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{364,	"IR",	"IRAN",					TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{372,	"IE",	"IRELAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{376,	"IL",	"ISRAEL",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{380,	"IT",	"ITALY",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{392,	"JP",	"JAPAN",				TRUE,	A_BAND_REGION_9,	TRUE,	G_BAND_REGION_1},
	{400,	"JO",	"JORDAN",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{398,	"KZ",	"KAZAKHSTAN",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{408,	"KP",	"KOREA DEMOCRATIC PEOPLE'S REPUBLIC OF",TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{410,	"KR",	"KOREA REPUBLIC OF",	TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{414,	"KW",	"KUWAIT",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{428,	"LV",	"LATVIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{422,	"LB",	"LEBANON",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{438,	"LI",	"LIECHTENSTEIN",		TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{440,	"LT",	"LITHUANIA",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{442,	"LU",	"LUXEMBOURG",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{446,	"MO",	"MACAU",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{807,	"MK",	"MACEDONIA",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{458,	"MY",	"MALAYSIA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{484,	"MX",	"MEXICO",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{492,	"MC",	"MONACO",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{504,	"MA",	"MOROCCO",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{528,	"NL",	"NETHERLANDS",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{554,	"NZ",	"NEW ZEALAND",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{578,	"NO",	"NORWAY",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{512,	"OM",	"OMAN",					TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{586,	"PK",	"PAKISTAN",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{591,	"PA",	"PANAMA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{604,	"PE",	"PERU",					TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{608,	"PH",	"PHILIPPINES",			TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{616,	"PL",	"POLAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{620,	"PT",	"PORTUGAL",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{630,	"PR",	"PUERTO RICO",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{634,	"QA",	"QATAR",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{642,	"RO",	"ROMANIA",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{643,	"RU",	"RUSSIA FEDERATION",	FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{682,	"SA",	"SAUDI ARABIA",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{702,	"SG",	"SINGAPORE",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{703,	"SK",	"SLOVAKIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{705,	"SI",	"SLOVENIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{710,	"ZA",	"SOUTH AFRICA",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{724,	"ES",	"SPAIN",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{752,	"SE",	"SWEDEN",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{756,	"CH",	"SWITZERLAND",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{760,	"SY",	"SYRIAN ARAB REPUBLIC",	FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{158,	"TW",	"TAIWAN",				TRUE,	A_BAND_REGION_3,	TRUE,	G_BAND_REGION_0},
	{764,	"TH",	"THAILAND",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{780,	"TT",	"TRINIDAD AND TOBAGO",	TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{788,	"TN",	"TUNISIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{792,	"TR",	"TURKEY",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{804,	"UA",	"UKRAINE",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{784,	"AE",	"UNITED ARAB EMIRATES",	FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{826,	"GB",	"UNITED KINGDOM",		TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{840,	"US",	"UNITED STATES",		TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{858,	"UY",	"URUGUAY",				TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{860,	"UZ",	"UZBEKISTAN",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_0},
	{862,	"VE",	"VENEZUELA",			TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{704,	"VN",	"VIET NAM",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{887,	"YE",	"YEMEN",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{716,	"ZW",	"ZIMBABWE",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{999,	"",	"",	0,	0,	0,	0}
};

#define NUM_OF_COUNTRIES	(sizeof(allCountry)/sizeof(COUNTRY_CODE_TO_COUNTRY_REGION))


INT Set_CountryString_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_CountryCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef EXT_BUILD_CHANNEL_LIST
INT Set_ChGeography_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* EXT_BUILD_CHANNEL_LIST */

INT Set_AP_SSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxRate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RT305x
INT Set_RfRead_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_RfWrite_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RT305x */

INT	Set_OLBCDetection_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_MaxStaNum_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_IdleTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef IAPP_SUPPORT
INT	Set_IappPID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* IAPP_SUPPORT */

INT Set_AP_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_WpaMixPairCipher_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_RekeyInterval_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_RekeyMethod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_PMKCachePeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_BeaconPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_DtimPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_NoForwarding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_NoForwardingBTNSSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_WmmCapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_HideSSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_VLANID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_VLANPriority_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AccessPolicy_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef RELEASE_EXCLUDE
INT	Set_AccessControlList_Proc(RTMP_ADAPTER *pAd, PUCHAR arg);
#endif /* RELEASE_EXCLUDE  */

INT	Set_ACLAddEntry_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_ACLDelEntry_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_ACLShowAll_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_ACLClearAll_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_RadioOn_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_SiteSurvey_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AutoChannelSel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_BADecline_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Show_StaCount_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Show_StaSecurityInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
	
INT	Show_DriverInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Show_Sat_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RTMP_MAC_PCI
#ifdef DBG_DIAGNOSE
INT Set_DiagOpt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Show_Diag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DBG_DAIGNOSE */
#endif /* RTMP_MAC_PCI */

INT	Show_Sat_Reset_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Show_MATTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DOT1X_SUPPORT
VOID RTMPIoctlQueryRadiusConf(
	IN PRTMP_ADAPTER pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

INT	Set_IEEE8021X_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_PreAuth_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_RADIUS_Server_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_RADIUS_Port_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_RADIUS_Key_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT1X_SUPPORT */

INT	Set_DisConnectSta_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_DisConnectAllSta_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef NINTENDO_AP
int	Set_NintendiCapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

int	Set_NintendoGet_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

int	Set_NintendoSet_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* NINTENDO_AP */

#ifdef APCLI_SUPPORT
INT Set_ApCli_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Ssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Bssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_TxMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_TxMcs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WSC_AP_SUPPORT
INT Set_AP_WscSsid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WSC_AP_SUPPORT */
#endif /* APCLI_SUPPORT */
#ifdef UAPSD_SUPPORT
INT Set_UAPSD_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* UAPSD_SUPPORT */

#ifdef WSC_AP_SUPPORT
INT	Set_WscStatus_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscStop_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

VOID RTMPIoctlWscProfile(
	IN PRTMP_ADAPTER pAdapter, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

VOID RTMPIoctlWscPINCode(
	IN PRTMP_ADAPTER pAdapter, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

VOID RTMPIoctlWscStatus(
	IN PRTMP_ADAPTER pAdapter, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

VOID RTMPIoctlGetWscDynInfo(
	IN PRTMP_ADAPTER pAdapter, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

VOID RTMPIoctlGetWscRegsDynInfo(
	IN PRTMP_ADAPTER pAdapter, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

BOOLEAN WscCheckEnrolleeNonceFromUpnp(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	RTMP_STRING *pData,
	IN  USHORT			Length,
	IN  PWSC_CTRL       pWscControl);

UCHAR	WscRxMsgTypeFromUpnp(
	IN	PRTMP_ADAPTER		pAdapter,
	IN  RTMP_STRING *pData,
	IN	USHORT				Length);

INT	    WscGetConfForUpnp(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWscControl);

INT	Set_AP_WscConfMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_WscConfStatus_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_WscMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_WscGetConf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_WscPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_WscSecurityMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_WscMultiByteCheck_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);


INT	Set_WscVersion_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WSC_V2_SUPPORT
INT	Set_WscV2Support_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscVersion2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscExtraTlvTag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscExtraTlvType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscExtraTlvData_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscSetupLock_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscFragment_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscFragmentSize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WSC_V2_SUPPORT */

#endif /* WSC_AP_SUPPORT */

#ifdef EASY_CONFIG_SETUP 
INT Set_AP_AutoProvisionEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_RssiThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WAC_SUPPORT
INT Set_AP_WACEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WAC_SUPPORT */
#endif /* EASY_CONFIG_SETUP */

#ifdef CONFIG_AP_SUPPORT
#ifdef MCAST_RATE_SPECIFIC
INT Set_McastPhyMode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_McastMcs(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_McastRate(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* MCAST_RATE_SPECIFIC */

#ifdef DOT11N_DRAFT3
INT Set_OBSSScanParam_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11N_DRAFT3 */

INT Set_EntryLifeCheck_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef AP_QLOAD_SUPPORT
INT	Set_QloadClr_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

/* QLOAD ALARM */
INT	Set_QloadAlarmTimeThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_QloadAlarmNumThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* AP_QLOAD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

INT	Set_MemDebug_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef CONFIG_AP_SUPPORT
INT	Set_PowerSaveLifeTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* CONFIG_AP_SUPPORT */

#ifdef P2P_SUPPORT
INT Set_P2p_OpMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Ssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Bssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_TxMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2pCli_TxMcs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
	
#ifdef WSC_AP_SUPPORT
INT Set_P2pCli_WscSsid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WSC_AP_SUPPORT */

INT Set_P2P_ProvAccept_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_P2p_WscManufacturer_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2p_WscModelName_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2p_WscModelNumber_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_P2p_WscSerialNumber_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* P2P_SUPPORT */

static struct {
	RTMP_STRING *name;
	INT (*set_proc)(PRTMP_ADAPTER pAdapter, RTMP_STRING *arg);
} *PRTMP_PRIVATE_SET_PROC, RTMP_PRIVATE_SUPPORT_PROC[] = {
	{"SSID",						Set_AP_SSID_Proc},
	{"Channel",					Set_Channel_Proc},
	{"AuthMode",					Set_AP_AuthMode_Proc},
	{"EncrypType",				Set_AP_EncrypType_Proc},
	{"WpaMixPairCipher", 			Set_AP_WpaMixPairCipher_Proc},
	{"RekeyInterval",				Set_AP_RekeyInterval_Proc},
	{"RekeyMethod", 				Set_AP_RekeyMethod_Proc}, 
	{"DefaultKeyID",				Set_AP_DefaultKeyID_Proc},
	{"WPAPSK",					Set_AP_WPAPSK_Proc},
	{"ResetCounter",				Set_ResetStatCounter_Proc},
	{"MaxStaNum",					Set_AP_MaxStaNum_Proc},
#ifdef DBG	
	{"Debug",					Set_Debug_Proc},
#endif /* DBG */

	{"CountryString",				Set_CountryString_Proc},
	{"CountryCode",				Set_CountryCode_Proc},
	{"NoForwarding",				Set_NoForwarding_Proc},
	
#ifdef WSC_AP_SUPPORT
	{"WscConfMode",				Set_AP_WscConfMode_Proc},
	{"WscConfStatus",				Set_AP_WscConfStatus_Proc},
	{"WscMode",					Set_AP_WscMode_Proc},
	{"WscStatus",					Set_WscStatus_Proc},
	{"WscGetConf",				Set_AP_WscGetConf_Proc},
	{"WscPinCode",				Set_AP_WscPinCode_Proc},
	{"WscStop",                     		Set_WscStop_Proc},
	{"WscGenPinCode",               		Set_WscGenPinCode_Proc},
	{"WscVendorPinCode",            Set_WscVendorPinCode_Proc},
	{"WscSecurityMode",				Set_AP_WscSecurityMode_Proc},
	{"WscMultiByteCheck",			Set_AP_WscMultiByteCheck_Proc},
	{"WscVersion", 					Set_WscVersion_Proc},
#ifdef WSC_V2_SUPPORT
	{"WscV2Support", 				Set_WscV2Support_Proc},
	{"WscVersion2", 				Set_WscVersion2_Proc},
	{"WscExtraTlvTag", 				Set_WscExtraTlvTag_Proc},
	{"WscExtraTlvType",				Set_WscExtraTlvType_Proc},
	{"WscExtraTlvData", 			Set_WscExtraTlvData_Proc},
	{"WscSetupLock", 				Set_WscSetupLock_Proc},
	{"WscFragment",					Set_WscFragment_Proc},
	{"WscFragmentSize", 			Set_WscFragmentSize_Proc},
#endif /* WSC_V2_SUPPORT */
#endif /* WSC_AP_SUPPORT */

#ifdef P2P_SUPPORT
	{"P2pCliEnable",				Set_P2pCli_Enable_Proc},
	{"P2pCliSsid",					Set_P2pCli_Ssid_Proc},
	{"P2pCliBssid",				Set_P2pCli_Bssid_Proc},
	{"P2pCliAuthMode",				Set_P2pCli_AuthMode_Proc},
	{"P2pCliEncrypType",			Set_P2pCli_EncrypType_Proc},
	{"P2pCliDefaultKeyID",			Set_P2pCli_DefaultKeyID_Proc},	
	{"P2pCliWPAPSK",				Set_P2pCli_WPAPSK_Proc},
	{"P2pCliKey1",					Set_P2pCli_Key1_Proc},
	{"P2pCliKey2",					Set_P2pCli_Key2_Proc},
	{"P2pCliKey3",					Set_P2pCli_Key3_Proc},
	{"P2pCliKey4",					Set_P2pCli_Key4_Proc},
	{"P2pCliTxMode",				Set_P2pCli_TxMode_Proc},
	{"P2pCliTxMcs",				Set_P2pCli_TxMcs_Proc},	
#ifdef WSC_AP_SUPPORT	
	{"P2pCliWscSsid",				Set_P2pCli_WscSsid_Proc},
#endif /* WSC_AP_SUPPORT */
	{"P2pOpMode",				Set_P2p_OpMode_Proc},
	{"p2pEnable",						Set_P2P_Enable},
	{"p2pLisCh",						Set_P2P_Listen_Channel},
	{"p2pOpCh", 					Set_P2P_Operation_Channel},
	{"p2pGoInt",						Set_P2P_GO_Intent},
	{"p2pWscManufacturer",              Set_P2p_WscManufacturer_Proc},
	{"p2pWscModelNumber",              Set_P2p_WscModelNumber_Proc},
	{"p2pWscSerialNumber",              Set_P2p_WscSerialNumber_Proc},
	{"p2pWscModelName",              Set_P2p_WscModelName_Proc},
	{"p2pDevName",					Set_P2P_Device_Name},
	{"p2pWscMode",					Set_P2P_WSC_Mode},
	{"p2pWscConf",					Set_P2P_WSC_ConfMethod},
#ifdef RELEASE_EXCLUDE
	{"p2pNoACnt",					Set_P2P_NoA_Count},
	{"p2pNoADuration",				Set_P2P_NoA_Duration},
	{"p2pNoAInv",					Set_P2P_NoA_Interval},
	{"p2pExtLst",					Set_P2P_Extend_Listen},
	{"p2pExtLstPrd",				Set_P2P_Extend_Listen_Periodic},
	{"p2pExtLstInv",				Set_P2P_Extend_Listen_Interval},
	{"p2pIntraBss", 				Set_P2P_Intra_Bss},
#endif /* RELEASE_EXCLUDE */
	{"p2pScan", 					Set_P2P_Scan},
	{"p2pInv",						Set_P2P_Invite_Proc},
	{"p2pDevDisc",					Set_P2P_Device_Discoverability_Proc},
	{"p2pLink", 					Set_P2P_Connect_GoIndex_Proc},
	{"p2pConnDevAddr", 					Set_P2P_Connect_Dev_Addr_Proc},
	{"p2pCfg",						Set_P2P_Print_Cfg},
	{"p2pTab",						Set_P2P_Print_GroupTable_Proc},
	{"p2pProv", 					Set_P2P_Provision_Proc},
	{"p2pStat", 					Set_P2P_State_Proc},
	{"p2pReset", 					Set_P2P_Reset_Proc},
	{"p2pPerTab",						Set_P2P_Print_PersistentTable_Proc},
	{"p2pDefConfMthd", 					Set_P2P_Default_Config_Method_Proc},
	{"p2pLinkDown", 					Set_P2P_Link_Down_Proc},
	{"p2pSigmaEnable", 					Set_P2P_Sigma_Enable_Proc},
	{"p2pQoSLegacyRate", 					Set_P2P_QoS_NULL_Legacy_Rate_Proc},
	{"p2pClientPmEnable", 					Set_P2P_CLIENT_PM_Enable_Proc},
	{"p2pEnterPIN", 					Set_P2P_Enter_WSC_PIN_Proc},
	{"p2pPerEnable", 					Set_P2P_Persistent_Enable_Proc},
	{"p2pDevDiscEnable", 					Set_P2P_Dev_Discoverability_Enable_Proc},
	{"p2pProvAccept", 					Set_P2P_ProvAccept_Proc},
	{"p2pProvByAddr", 					Set_P2P_ProvisionByAddr_Proc},
	{"p2pDelDevByAddr",					Set_P2P_DelDevByAddr_Proc},
	{"p2pDevDiscPeriod", 				Set_P2P_DevDiscPeriod_Proc},
	{"p2pPriDeviceType", 				Set_P2P_PriDeviceType_Proc},
	{"p2p2ndDevTypeList", 				Set_P2P_SecDevTypeList_Proc},
	{"p2pCancel", 						Set_P2P_Cancel_Proc},
	{"p2pConfirmByUI", 					Set_P2P_ConfirmByUI_Proc},
	{"p2pDiscoverable", 					Set_P2P_Discoverable_Proc},
	{"p2pSerDiscEnable", 						Set_P2P_Service_Discovery_Proc},
	{"p2pSerDiscCapable", 						Set_P2P_Service_Discovery_Capable_Proc},
	{"p2pSendSerDiscInit", 						Set_P2P_Send_Service_Discovery_Init_Proc},
	{"p2pSendSerDiscCmb", 						Set_P2P_Send_Service_Discovery_Comeback_Proc},
	{"p2pCleanTable", 				Set_P2P_DelPerstTable_Proc},
	{"p2pCheckPeerCh", 				Set_P2P_CheckPeerChannel_Proc},
	{"p2pAutoAccept", 				Set_P2P_AutoAccept_Proc},
	{"p2pAutoChannel", 			Set_P2P_AutoChannelCtrl},
	{"p2pGoAcceptInvReq", 				Set_P2P_Go_Accept_Invitation_Request},
#endif /* P2P_SUPPORT */
	{NULL,}
};


static struct {
	RTMP_STRING *name;
	INT (*set_proc)(PRTMP_ADAPTER pAdapter, RTMP_STRING *arg);
} *PRTMP_PRIVATE_SHOW_PROC, RTMP_PRIVATE_SHOW_SUPPORT_PROC[] = {
	{"stainfo",			Show_MacTable_Proc},
	{"stacountinfo",			Show_StaCount_Proc},
	{"stasecinfo", 			Show_StaSecurityInfo_Proc},	
	{"descinfo",			Show_DescInfo_Proc},
	{"driverinfo", 			Show_DriverInfo_Proc},
#ifdef DOT11_N_SUPPORT
	{"bainfo",				Show_BaTable_Proc},
#endif /* DOT11_N_SUPPORT */
	{"stat",				Show_Sat_Proc}, 
#ifdef MAT_SUPPORT
	{"matinfo",			Show_MATTable_Proc},
#endif /* MAT_SUPPORT */
#ifdef RELEASE_EXCLUDE
#ifdef RTMP_MAC_USB
	{"rxbulk",	 			Show_RxBulk_Proc},
	{"txbulk",				Show_TxBulk_Proc},
#endif /* RTMP_MAC_USB */
#endif /* RELEASE_EXCLUDE */

#ifdef WSC_AP_SUPPORT
	{"WscPeerList", 		WscApShowPeerList},
#endif /* WSC_AP_SUPPORT */
	{NULL,}
};


INT RTMPAPPrivIoctlSet(
	IN RTMP_ADAPTER *pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *pIoctlCmdStr)
{
	RTMP_STRING *this_char;
	RTMP_STRING *value;
	INT Status = NDIS_STATUS_SUCCESS;

	while ((this_char = strsep((char **)&pIoctlCmdStr->u.data.pointer, ",")) != NULL) 
	{
		if (!*this_char)
			 continue;

		if ((value = strchr(this_char, '=')) != NULL)
			*value++ = 0;

		if (!value 
#ifdef WSC_AP_SUPPORT                        
            && (
                 (strcmp(this_char, "WscStop") != 0) &&
                 (strcmp(this_char, "WscGenPinCode")!= 0)
               )
#endif /* WSC_AP_SUPPORT */
            )
			continue;  							

		for (PRTMP_PRIVATE_SET_PROC = RTMP_PRIVATE_SUPPORT_PROC; PRTMP_PRIVATE_SET_PROC->name; PRTMP_PRIVATE_SET_PROC++)
		{
			if (!strcmp(this_char, PRTMP_PRIVATE_SET_PROC->name)) 
			{
				if(!PRTMP_PRIVATE_SET_PROC->set_proc(pAd, value))
				{   /*FALSE:Set private failed then return Invalid argument */
					Status = -EINVAL;							
				}
				if (Status == NDIS_STATUS_SUCCESS)
				{
					if (strcmp(this_char, "P2pOpMode") == 0)
					{
						if (P2P_GO_ON(pAd))
							pAd->flg_p2p_OpStatusFlags |= P2P_FIXED_MODE;
						else
							pAd->flg_p2p_OpStatusFlags &= (~P2P_FIXED_MODE);
					}					
				}
				break;  /*Exit for loop. */
			}
		}

		if(PRTMP_PRIVATE_SET_PROC->name == NULL)
		{  /*Not found argument */
			Status = -EINVAL;
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::(iwpriv) Command not Support [%s=%s]\n", this_char, value));
			break;
		}	
	}

	return Status;
}


INT RTMPAPPrivIoctlShow(
	IN RTMP_ADAPTER *pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *pIoctlCmdStr)
{
	RTMP_STRING *this_char;
	RTMP_STRING *value = NULL;
	INT Status = NDIS_STATUS_SUCCESS;
	
	while ((this_char = strsep((char **)&pIoctlCmdStr->u.data.pointer, ",")) != NULL) 
	{
		if (!*this_char)
			continue;

		for (PRTMP_PRIVATE_SHOW_PROC = RTMP_PRIVATE_SHOW_SUPPORT_PROC; PRTMP_PRIVATE_SHOW_PROC->name; PRTMP_PRIVATE_SHOW_PROC++)
		{
			if (!strcmp(this_char, PRTMP_PRIVATE_SHOW_PROC->name)) 
			{						
				if(!PRTMP_PRIVATE_SHOW_PROC->set_proc(pAd, value))
				{   /*FALSE:Set private failed then return Invalid argument */
					Status = -EINVAL;							
				}
				break;  /*Exit for loop. */
			}
		}

		if(PRTMP_PRIVATE_SHOW_PROC->name == NULL)
		{  /*Not found argument */
			Status = -EINVAL;
#ifdef RTMP_RBUS_SUPPORT
			if (pAd->infType == RTMP_DEV_INF_RBUS)
			{
				for (PRTMP_PRIVATE_SHOW_PROC = RTMP_PRIVATE_SHOW_SUPPORT_PROC; 
					PRTMP_PRIVATE_SHOW_PROC->name; 
					PRTMP_PRIVATE_SHOW_PROC++)
				DBGPRINT(RT_DEBUG_TRACE, ("%s\n", PRTMP_PRIVATE_SHOW_PROC->name));
			}
#endif /* RTMP_RBUS_SUPPORT */
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::(iwpriv) Command not Support [%s=%s]\n", this_char, value));
			break;
		}	
	}

	return Status;
	
}

#ifdef INF_AR9
#ifdef AR9_MAPI_SUPPORT
INT RTMPAPPrivIoctlAR9Show(
	IN RTMP_ADAPTER *pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *pIoctlCmdStr)
{
	INT Status = NDIS_STATUS_SUCCESS;

		if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_mac_table"))
		{
			RTMPAR9IoctlGetMacTable(pAd,pIoctlCmdStr);
		}
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_stat2"))
		{
			RTMPIoctlGetSTAT2(pAd,pIoctlCmdStr);
		}		
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_radio_dyn_info"))
		{
			RTMPIoctlGetRadioDynInfo(pAd,pIoctlCmdStr);
		}
#ifdef WSC_AP_SUPPORT
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_wsc_profile"))
		{
			RTMPAR9IoctlWscProfile(pAd,pIoctlCmdStr);		
		}
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_wsc_pincode"))
		{
			RTMPIoctlWscPINCode(pAd,pIoctlCmdStr);
		}
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_wsc_status"))
		{
			RTMPIoctlWscStatus(pAd,pIoctlCmdStr);
		}
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_wps_dyn_info"))
		{
			RTMPIoctlGetWscDynInfo(pAd,pIoctlCmdStr);
		}
		else if(!strcmp(pIoctlCmdStr->u.data.pointer, "get_wps_regs_dyn_info"))
		{
			RTMPIoctlGetWscRegsDynInfo(pAd,pIoctlCmdStr);
		}
#endif
	return Status;
}
#endif /*AR9_MAPI_SUPPORT*/
#endif/*AR9_INF*/

INT RTMPAPSetInformation(
	IN RTMP_ADAPTER *pAd,
	INOUT RTMP_IOCTL_INPUT_STRUCT *rq,
	IN INT cmd)
{
	RTMP_IOCTL_INPUT_STRUCT *wrq = (RTMP_IOCTL_INPUT_STRUCT *) rq;
	UCHAR Addr[MAC_ADDR_LEN];
	INT Status = NDIS_STATUS_SUCCESS;

#ifdef SNMP_SUPPORT	
	/*snmp */
    UINT						KeyIdx = 0;
    PNDIS_AP_802_11_KEY			pKey = NULL;
	TX_RTY_CFG_STRUC			tx_rty_cfg;
	ULONG						ShortRetryLimit, LongRetryLimit;
	UCHAR						ctmp;
#endif /* SNMP_SUPPORT */

#ifdef MESH_SUPPORT
	BOOLEAN								RestartMeshIsRequired = FALSE;
#endif /* MESH_SUPPORT */
	
#ifdef HOSTAPD_SUPPORT
 	NDIS_802_11_WEP_STATUS              WepStatus;
 	NDIS_802_11_AUTHENTICATION_MODE     AuthMode = Ndis802_11AuthModeMax;
	NDIS_802_11_SSID                    Ssid;
	MAC_TABLE_ENTRY						*pEntry;
	struct ieee80211req_mlme			mlme;

	struct ieee80211req_key				Key;
	struct ieee80211req_del_key			delkey;
	ULONG								KeyIdx;
	UINT8		Wcid;
	BSS_STRUCT *pMbss ;
	WSC_LV_INFO            WscIEBeacon;
   	WSC_LV_INFO            WscIEProbeResp;
	int i;
#endif /*HOSTAPD_SUPPORT*/

	
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;

	switch(cmd & 0x7FFF)
    {
#ifdef P2P_SUPPORT
		case OID_802_11_P2P_MODE:

			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_MODE \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				UCHAR OpMode=1;
				Status = copy_from_user(&OpMode, wrq->u.data.pointer, wrq->u.data.length);
				if (OpMode == OPMODE_AP)
				{
					if (P2P_CLI_ON(pAd))
						P2P_CliStop(pAd);

					if ((!P2P_GO_ON(pAd)) || (P2P_GO_ON(pAd)))
					{
						P2PCfgInit(pAd);
						P2P_GoStartUp(pAd, MAIN_MBSSID);
					}
				}
				else if (OpMode == OPMODE_APSTA)
				{
					if (P2P_GO_ON(pAd))
						P2P_GoStop(pAd);

					if ((!P2P_CLI_ON(pAd)) || (P2P_CLI_ON(pAd)))
					{
						P2PCfgInit(pAd);
						P2P_CliStartUp(pAd);
						AsicEnableBssSync(pAd, pAd->CommonCfg.BeaconPeriod);
					}
				}
				else
				{
					if (P2P_CLI_ON(pAd))
						P2P_CliStop(pAd);
					else if (P2P_GO_ON(pAd))
					{
						P2P_GoStop(pAd);
						if (INFRA_ON(pAd))
							AsicEnableBssSync(pAd, pAd->CommonCfg.BeaconPeriod);
					}
					P2PCfgInit(pAd);
				}
				if (P2P_GO_ON(pAd))
					pAd->flg_p2p_OpStatusFlags |= P2P_FIXED_MODE;
				else
					pAd->flg_p2p_OpStatusFlags &= (~P2P_FIXED_MODE);

				DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2p_OpMode_Proc::(OpMode = %d)\n", pObj->ioctl_if, OpMode));

					
			}
			break;
/*
		case OID_802_11_P2P_CLEAN_TABLE:

			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_CLEAN_TABLE \n"));
			{
				P2pGroupTabInit(pAd);
			}
			break;
*/

		case OID_802_11_P2P_SSID:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_SSID \n"));
			if (wrq->u.data.length > 32)
				Status = -EINVAL;
			else
			{
				UCHAR Ssid[MAX_LEN_OF_SSID] = {0};

				NdisZeroMemory(Ssid, sizeof(Ssid));
				Status = copy_from_user(&Ssid, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_SSID Ssid=%s\n",Ssid));
				{
					Set_AP_SSID_Proc(pAd, Ssid);
#if 0
					pAd->P2pCfg.DeviceNameLen = wrq->u.data.length;
					NdisZeroMemory(pAd->P2pCfg.DeviceName, 32);
					NdisMoveMemory(pAd->P2pCfg.DeviceName, DeviceName, pAd->P2pCfg.DeviceNameLen);
#endif
					DBGPRINT(RT_DEBUG_TRACE, ("%s:: SSID = %s.\n", __FUNCTION__, Ssid));
				}

			}
			break;

		case OID_802_11_P2P_DEVICE_NAME:			
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_DEVICE_NAME \n"));
			if (wrq->u.data.length > 32)
				Status = -EINVAL;
			else
			{
				UCHAR DeviceName[MAX_LEN_OF_SSID + 1] = {0};

				NdisZeroMemory(DeviceName, sizeof(DeviceName));
				Status = copy_from_user(&DeviceName, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_DEVICE_NAME DeviceName=%s\n",DeviceName));
				{

					pAd->P2pCfg.DeviceNameLen = wrq->u.data.length;
					NdisZeroMemory(pAd->P2pCfg.DeviceName, 32);
					NdisMoveMemory(pAd->P2pCfg.DeviceName, DeviceName, pAd->P2pCfg.DeviceNameLen);
					P2pGetRandomSSID(pAd, pAd->ApCfg.MBSSID[MAIN_MBSSID].Ssid, &(pAd->ApCfg.MBSSID[MAIN_MBSSID].SsidLen));
					DBGPRINT(RT_DEBUG_ERROR, ("%s:: Device Name = %s.\n", __FUNCTION__, pAd->P2pCfg.DeviceName));
				}

			}
			break;

		case OID_802_11_P2P_LISTEN_CHANNEL:			
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_LISTEN_CHANNEL \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				UCHAR listen_ch;
				Status = copy_from_user(&listen_ch, wrq->u.data.pointer, wrq->u.data.length);

				/* check if this channel is valid */
				if (ChannelSanity(pAd, listen_ch) == TRUE)
				{
					pAd->P2pCfg.ListenChannel = listen_ch;
				}
				else
				{
					DBGPRINT(RT_DEBUG_TRACE, ("Listen Channel out of range, using default.\n"));
					pAd->P2pCfg.ListenChannel = 1;
				}

				DBGPRINT(RT_DEBUG_TRACE, ("%s:: Listen Channel = %d.\n", __FUNCTION__, pAd->P2pCfg.ListenChannel));
			}
			break;

		case OID_802_11_P2P_OPERATION_CHANNEL:			
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_OPERATION_CHANNEL \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				UCHAR op_ch;
				Status = copy_from_user(&op_ch, wrq->u.data.pointer, wrq->u.data.length);

					/* check if this channel is valid */
					if (ChannelSanity(pAd, op_ch) == TRUE)
					{
						pAd->P2pCfg.GroupChannel = op_ch;
					}
					else
					{
						DBGPRINT(RT_DEBUG_ERROR, ("Opertation Channel out of range, using default.\n"));
						pAd->P2pCfg.GroupChannel = 1;
					}

					DBGPRINT(RT_DEBUG_ERROR, ("%s:: Op Channel = %d.\n", __FUNCTION__, pAd->P2pCfg.GroupChannel));
					
			}
			break;

		case OID_802_11_P2P_GO_INT:			
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_GO_INT \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				UCHAR intent;
				Status = copy_from_user(&intent, wrq->u.data.pointer, wrq->u.data.length);

					/* check if this channel is valid */
			
				if (intent <= 15)
					pAd->P2pCfg.GoIntentIdx = intent;		
				else
				{
					DBGPRINT(RT_DEBUG_ERROR, ("GO Intent out of range 0 ~ 15, using default.\n"));
					pAd->P2pCfg.GoIntentIdx = 0;
				}

				DBGPRINT(RT_DEBUG_ERROR, ("%s:: GO Intent = %d.\n", __FUNCTION__, pAd->P2pCfg.GoIntentIdx));
		
			}
			break;

		case OID_802_11_P2P_SCAN:		
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_SCAN \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{

				UCHAR ScanType;
				PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;
				Status = copy_from_user(&ScanType, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Scan type is %d\n", ScanType));
	
				switch(ScanType)
				{
					case P2P_STOP_SCAN:
						/* Stop scan and stop to response peer P2P scanning */
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
					pAd->StaCfg.bAutoReconnect = FALSE;
#else
					pAd->StaCfg.bAutoReconnect = TRUE;
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */
					pP2PCtrl->bSentProbeRSP = FALSE;
					P2pStopScan(pAd);
						break;
					case P2P_SCANNING:
						/* Do P2P scanning */
						pAd->StaCfg.bAutoReconnect = FALSE;
						pP2PCtrl->bSentProbeRSP = TRUE;
						P2pGroupTabInit(pAd);
						P2pScan(pAd);
						break;
					case P2P_STOP_SCAN_AND_LISTEN:
						/* Stop scan and lock at Listen Channel to response peer P2P scanning */
						pAd->P2pCfg.bSentProbeRSP = TRUE;
						P2pStopScan(pAd);
						break;
					default:
						DBGPRINT(RT_DEBUG_ERROR, ("Incorrect scan type:%d\n", ScanType));
				}
				
			}
			break;			

		case OID_P2P_WSC_PIN_CODE:	
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_P2P_WSC_PIN_CODE wrq->u.data.length=%d\n",wrq->u.data.length));
            if (wrq->u.data.length != 8) /* PIN Code Length is 8 */
                Status = -EINVAL;
            else
            {
                CHAR PinCode[9] = {0};
                Status = copy_from_user(&PinCode[0], wrq->u.data.pointer, wrq->u.data.length);
                if (Status == 0)
                {
					if (Set_WscVendorPinCode_Proc(pAd, (RTMP_STRING *) &PinCode[0]) == FALSE)
                        Status = -EINVAL;
                }
            }
            break;

		case OID_802_11_P2P_WscMode:			
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_WscMode \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				UCHAR p2pWscMode;
				Status = copy_from_user(&p2pWscMode, wrq->u.data.pointer, wrq->u.data.length);

					/* check if this channel is valid */
			
				if (p2pWscMode <= 2 && p2pWscMode >= 1)
					pAd->P2pCfg.WscMode= p2pWscMode;
				else
				{
					DBGPRINT(RT_DEBUG_ERROR, ("WscMode is invalid, using default.\n"));
					pAd->P2pCfg.WscMode = WSC_PIN_MODE; /* PIN */
				}

				DBGPRINT(RT_DEBUG_ERROR, ("%s:: WscMode = %s.\n", __FUNCTION__, (p2pWscMode == 1) ? "PIN" : "PBC"));
			}
			break;

		case OID_802_11_P2P_WscConf:			
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_WscConf \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				UCHAR method;
				Status = copy_from_user(&method, wrq->u.data.pointer, wrq->u.data.length);

				if (pAd->P2pCfg.WscMode == WSC_PIN_MODE)
				{
					if (method == 1)
					{
						/* Display PIN */
						pAd->P2pCfg.Dpid = DEV_PASS_ID_REG;
						pAd->P2pCfg.ConfigMethod =  WSC_CONFMET_DISPLAY;

					}
					else if (method == 2)
					{
						/* Enter PIN */
						pAd->P2pCfg.Dpid = DEV_PASS_ID_USER;
						pAd->P2pCfg.ConfigMethod =  WSC_CONFMET_KEYPAD;
					}
				}
				else if (pAd->P2pCfg.WscMode == WSC_PBC_MODE)
				{
					if (method == 3)
					{
						pAd->P2pCfg.Dpid = DEV_PASS_ID_PBC;
						pAd->P2pCfg.ConfigMethod = WSC_CONFMET_PBC;
					}
				}

				DBGPRINT(RT_DEBUG_ERROR, ("%s:: Config Method = %s.\n", __FUNCTION__, decodeConfigMethod(pAd->P2pCfg.ConfigMethod)));
			}
			break;

		case OID_802_11_P2P_Link:		
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_Link \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{

				UCHAR p2pindex;
				PUCHAR	pAddr;
				Status = copy_from_user(&p2pindex, wrq->u.data.pointer, wrq->u.data.length);

				DBGPRINT(RT_DEBUG_ERROR, ("%s:: TabIdx[%d]\n", __FUNCTION__, p2pindex));
				if (p2pindex < pAd->P2pTable.ClientNumber)
				{
					/*P2PPrintP2PEntry(pAd, P2pTabIdx); */
					/*pAd->P2pCfg.ConnectingIndex = 0; */
					/*if (pAd->P2pTable.Client[P2pTabIdx].P2pClientState == P2PSTATE_DISCOVERY) */
					/*	pAd->P2pTable.Client[P2pTabIdx].P2pClientState = P2PSTATE_CONNECT_COMMAND; */
					/*COPY_MAC_ADDR(pAd->P2pCfg.ConnectingMAC, pAd->P2pTable.Client[P2pTabIdx].addr); */
					/*pAd->P2pTable.Client[P2pTabIdx].StateCount = 10; */
					/*pAd->P2pTable.Client[P2pTabIdx].bValid = TRUE; */
					/*P2pConnect(pAd); */
					pAddr = &pAd->P2pTable.Client[p2pindex].addr[0];
					P2pConnectPrepare(pAd, pAddr, P2PSTATE_CONNECT_COMMAND);
			    }
			    else
			        DBGPRINT(RT_DEBUG_ERROR, ("Table Idx out of range!\n"));
			}
			break;

		case OID_802_11_P2P_RESET:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::RT_OID_802_11_P2P_RESET \n"));
			Set_P2P_Reset_Proc(pAd, "1");
			break;

		case OID_802_11_P2P_SIGMA_ENABLE:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_SIGMA_ENABLE \n"));
			Set_P2P_Sigma_Enable_Proc(pAd, "1");
			break;

		case OID_802_11_P2P_CONNECT_ADDR:
		{
			UCHAR	Addr[18] = {0};
			Status = copy_from_user(&Addr, wrq->u.data.pointer, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_CONNECT_ADDR \n"));
			Set_P2P_Connect_Dev_Addr_Proc(pAd, &Addr);
		}
			break;

		case OID_802_11_P2P_INVITE:
		{
			UCHAR	Addr[18] = {0};
			Status = copy_from_user(&Addr, wrq->u.data.pointer, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_INVITE \n"));
			Set_P2P_Send_Invite_Proc(pAd, &Addr);
		}
			break;

		case OID_DELETE_PERSISTENT_TABLE:
            			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_DELETE_PERSISTENT_TABLE \n"));
            			Set_P2P_DelPerstTable_Proc(pAd, "1");
			break;
            
		case OID_DELETE_PERSISTENT_ENTRY:
		{
			UCHAR	Addr[18] = {0};
			Status = copy_from_user(&Addr, wrq->u.data.pointer, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_DELETE_PERSISTENT_ENTRY \n"));
			Set_P2P_DelPerstEntry_Proc(pAd, &Addr);
		}
			break;
            
    		case OID_802_11_P2P_PERSISTENT_TABLE:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_PERSISTENT_TABLE \n"));
			if (wrq->u.data.length != (sizeof(RT_P2P_PERSISTENT_ENTRY)*MAX_P2P_TABLE_SIZE))
			{
				Status = -EINVAL;
			} 
			else
			{
				if (copy_from_user(&pAd->P2pTable.PerstEntry, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
				else
				{
					UCHAR j;
					pAd->P2pTable.PerstNumber = 0;					
					for (j = 0; j < MAX_P2P_TABLE_SIZE; j++)
					{
						if (pAd->P2pTable.PerstEntry[j].bValid)
						{
							pAd->P2pTable.PerstNumber++;
						}
					}
				}
			}
			break;

        case OID_802_11_P2P_PERSISTENT_ENABLE:
            {
                if (wrq->u.data.length > sizeof(INT32))
                    Status  = -EINVAL;
                else
                {
                    INT32 EnablePersistent = 0;
                
                    Status = copy_from_user(&EnablePersistent, wrq->u.data.pointer, wrq->u.data.length);
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        if (EnablePersistent <= 0)
                            Set_P2P_Persistent_Enable_Proc(pAd, "0");
                        else
                            Set_P2P_Persistent_Enable_Proc(pAd, "1");
                        DBGPRINT(RT_DEBUG_TRACE, ("%s:: OID_802_11_P2P_PERSISTENT_ENABLE = %d.\n", __FUNCTION__, EnablePersistent));
                        
                    }
                    else
                        DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_PERSISTENT_ENABLE error!!\n"));
                }
            }
            break;
            
		case OID_802_11_P2P_ENTER_PIN:
			{
				UCHAR	PIN[10] = {0};
				Status = copy_from_user(&PIN, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_ENTER_PIN , pin = %s \n", PIN));
				Set_P2P_Enter_WSC_PIN_Proc(pAd, &PIN);
			}
			break;
		case OID_802_11_P2P_TRIGGER_WSC:
			{
				//Set_P2P_ConfirmByUI_Proc(pAd, "1");//We need to make ConfirmByUI to true, to make the further operation go ahead.
				UCHAR wsc_trigger[2] = {0};
				Status = copy_from_user(&wsc_trigger, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("##### Set::OID_802_11_P2P_TRIGGER_WSC \n"));
				Set_AP_WscGetConf_Proc(pAd, &wsc_trigger);
				//Set_P2P_ConfirmByUI_Proc(pAd, "0");//Disable it here. to block the next opration.
			}
			break;
		case OID_802_11_P2P_WSC_CONF_MODE:
			{
				UCHAR wsc_conf_mode[2] = {0};
				Status = copy_from_user(&wsc_conf_mode, wrq->u.data.pointer, wrq->u.data.length);
				//DBGPRINT(RT_DEBUG_TRACE, ("##### Set::OID_802_11_P2P_WSC_CONF_MODE = %s \n", decodeConfigMethod(wsc_conf_mode)));
				DBGPRINT(RT_DEBUG_TRACE, ("##### Set::OID_802_11_P2P_WSC_CONF_MODE = %s \n", wsc_conf_mode));
				Set_AP_WscConfMode_Proc(pAd, &wsc_conf_mode);
			}
			break;
		case OID_802_11_P2P_WSC_MODE:
			{
				UCHAR wsc_mode[2] = {0};
				Status = copy_from_user(&wsc_mode, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("##### Set::OID_802_11_P2P_WSC_MODE = %s \n", wsc_mode));
				Set_AP_WscMode_Proc(pAd, &wsc_mode);
			}
			break;
		case OID_802_11_P2P_PIN_CODE:
			{
				UCHAR	PIN[10] = {0};
				Status = copy_from_user(&PIN, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_PIN_CODE, pin = %s \n", PIN));
				Set_AP_WscPinCode_Proc(pAd, &PIN);
			}
			break;
		case OID_802_11_P2P_PROVISION:
			{
				UCHAR p2pindex;
				PUCHAR pAddr;
				Status = copy_from_user(&p2pindex, wrq->u.data.pointer, wrq->u.data.length);

				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_PROVISION(%d)\n", p2pindex));
				if (p2pindex < pAd->P2pTable.ClientNumber)
				{
					pAddr = &pAd->P2pTable.Client[p2pindex].addr[0];
					P2pConnectPrepare(pAd, pAddr, P2PSTATE_PROVISION_COMMAND);
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_PROVISION out of range!\n"));
			}
			break;

		case OID_802_11_P2P_PROVISION_MAC:
		{
			UCHAR	Addr[18] = {0};
			Status = copy_from_user(&Addr, wrq->u.data.pointer, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_CONNECT_ADDR \n"));
			Set_P2P_Provision_Dev_Addr_Proc(pAd, &Addr);
		}
			break;

		case OID_802_11_P2P_PASSPHRASE:
			{
				BSS_STRUCT *pMBSSStruct;
				INT retval;
				UCHAR pass_phrase[65];

				NdisZeroMemory(&pass_phrase, sizeof(pass_phrase));
				Status = copy_from_user(&pass_phrase, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_PASSPHRASE(%s)\n", pass_phrase));
				retval = Set_AP_WPAPSK_Proc(pAd, pass_phrase);

				//pMBSSStruct = &pAd->ApCfg.MBSSID[0];
				//retval = RT_CfgSetWPAPSKKey(pAd, &pass_phrase, strlen(&pass_phrase), (PUCHAR)pMBSSStruct->Ssid, pMBSSStruct->SsidLen, pMBSSStruct->PMK);
				if (retval == FALSE)
					DBGPRINT(RT_DEBUG_ERROR, ("P2P PassPhrase Generate Fail\n"));

#if 0 //def WSC_AP_SUPPORT
				NdisZeroMemory(pMBSSStruct->WscControl.WpaPsk, 64);
				pMBSSStruct->WscControl.WpaPskLen = 0;
				pMBSSStruct->WscControl.WpaPskLen = strlen(pass_phrase);
				NdisMoveMemory(pMBSSStruct->WscControl.WpaPsk, pass_phrase, pMBSSStruct->WscControl.WpaPskLen);

#endif /* WSC_AP_SUPPORT */
			}
			break;
		case OID_802_11_P2P_LINK_DOWN:
			{
				CHAR conn_stat;
				Status = copy_from_user(&conn_stat, wrq->u.data.pointer, wrq->u.data.length);
				if( conn_stat == 1)
					P2pLinkDown(pAd, P2P_DISCONNECTED);
				else
					P2pLinkDown(pAd, P2P_CONNECT_FAIL);
			}
			break;

		case OID_802_11_P2P_PRI_DEVICE_TYPE:
			{
				UCHAR pri_dev_type[8];
				PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;
				if ( wrq->u.data.length == 8 )
					Status = copy_from_user(&pri_dev_type, wrq->u.data.pointer, wrq->u.data.length);

				if ( Status == 0 )
				{
					
					NdisMoveMemory(pP2PCtrl->DevInfo.PriDeviceType, pri_dev_type, 8);
					DBGPRINT(RT_DEBUG_TRACE, ("%s::OID_802_11_P2P_PRI_DEVICE_TYPE = %02X%02X-%02X%02X%02X%02X-%02X%02X.\n", __FUNCTION__, 
						pP2PCtrl->DevInfo.PriDeviceType[0], pP2PCtrl->DevInfo.PriDeviceType[1],
						pP2PCtrl->DevInfo.PriDeviceType[2], pP2PCtrl->DevInfo.PriDeviceType[3],
						pP2PCtrl->DevInfo.PriDeviceType[4], pP2PCtrl->DevInfo.PriDeviceType[5],
						pP2PCtrl->DevInfo.PriDeviceType[6], pP2PCtrl->DevInfo.PriDeviceType[7]));
				}
			}
			break;

		case OID_802_11_P2P_DEL_CLIENT:
		{
			char macstr[32];
			memset(macstr, 0, sizeof(macstr));
			if (wrq->u.data.length < sizeof(macstr))
			{
				Status = copy_from_user(macstr, wrq->u.data.pointer, wrq->u.data.length);
				if (Status == NDIS_STATUS_SUCCESS)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("%s::OID_802_11_P2P_DEL_CLIENT(macstr=%s)\n", __FUNCTION__, macstr));
					Set_P2P_DelDevByAddr_Proc(pAd, macstr);
				}
			}
		}
		break;

        case OID_802_11_P2P_AUTO_ACCEPT:
            {
                if (wrq->u.data.length > sizeof(INT32))
                    Status  = -EINVAL;
                else
                {
                    INT32 p2pAutoAccept = 0;
                
                    Status = copy_from_user(&p2pAutoAccept, wrq->u.data.pointer, wrq->u.data.length);
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        if (p2pAutoAccept <= 0)
                            Set_P2P_AutoAccept_Proc(pAd, "0");
                        else
                            Set_P2P_AutoAccept_Proc(pAd, "1");
                        DBGPRINT(RT_DEBUG_TRACE, ("%s:: OID_802_11_P2P_AUTO_ACCEPT = %d.\n", __FUNCTION__, p2pAutoAccept));
                        
                    }
                    else
                        DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_AUTO_ACCEPT error!!\n"));
                }
            }
            break;

		case OID_802_11_P2P_WSC_CANCEL:
		{
		    DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_WSC_CANCEL \n"));
        	    Set_WscStop_Proc(pAd,"1");
		}
		break;

        case OID_802_11_P2P_CHECK_PEER_CHANNEL:
        {
            if (wrq->u.data.length > sizeof(INT32))
                Status  = -EINVAL;
            else
            {
                INT32 PolicyMode = 0;
            
                Status = copy_from_user(&PolicyMode, wrq->u.data.pointer, wrq->u.data.length);
                if (Status == NDIS_STATUS_SUCCESS)
                {
					PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;

					if ((PolicyMode >= CHECK_PEER_CHANNEL_DISABLE) &&
						(PolicyMode <= CHECK_PEER_CHANNEL_EVEN_IF_CONNECTION))
						pP2PCtrl->CheckPeerChannelPolicy = PolicyMode;
					else
						pP2PCtrl->CheckPeerChannelPolicy = CHECK_PEER_CHANNEL_IF_NO_CONNECTION;

                     DBGPRINT(RT_DEBUG_TRACE, ("%s:: OID_802_11_P2P_CHECK_PEER_CHANNEL = %d.\n", __FUNCTION__, pP2PCtrl->CheckPeerChannelPolicy));
                    
                }
                else
                    DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_CHECK_PEER_CHANNEL error!!\n"));
            }
        }
        break;

#ifdef WFD_SUPPORT
		/*
		 * Enable/Disable driver WFD functionality
		 * Value:
		 * 	0:	Disable 	
		 * 	1:	Enable		
		*/
		case OID_802_11_WFD_ENABLE:
		{
			if (wrq->u.data.length > sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				BOOLEAN bEnable;
				
				Status = copy_from_user(&bEnable, wrq->u.data.pointer, wrq->u.data.length);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					if (bEnable == TRUE)
					{
						pAd->StaCfg.WfdCfg.bWfdEnable= TRUE;
						DBGPRINT(RT_DEBUG_TRACE, ("%s:: Enable WFD Support!\n", __FUNCTION__));
					}
					else
					{
						pAd->StaCfg.WfdCfg.bWfdEnable= FALSE;
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: Disable WFD Support!\n", __FUNCTION__));
					}
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_ENABLE error!!\n"));
			}
		}
			break;

#ifdef RT_CFG80211_SUPPORT
		/*
		 * Enable/Disable insert WFD IE to management frames from wpa_supplicant
		 * Value:
		 *	0:	Disable 	
		 *	1:	Enable		
		*/
		case OID_802_11_WFD_IE_INSERT:
		{
			if (wrq->u.data.length > sizeof(BOOLEAN))
				Status	= -EINVAL;
			else
			{
				BOOLEAN bEnable;
				CFG80211_CB *pCfg80211_CB = NULL;

				pAd->StaCfg.WfdCfg.bSuppInsertWfdIe = FALSE;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				if (pAd->pCfg80211_CB != NULL)
				{
					pCfg80211_CB = (CFG80211_CB *)pAd->pCfg80211_CB;
					if (pCfg80211_CB->pCfg80211_Wdev != NULL)
					{
						if ((pCfg80211_CB->pCfg80211_Wdev->wiphy->interface_modes & BIT(NL80211_IFTYPE_P2P_CLIENT)) ||
							(pCfg80211_CB->pCfg80211_Wdev->wiphy->interface_modes & BIT(NL80211_IFTYPE_P2P_GO)))
						{
							Status = copy_from_user(&bEnable, wrq->u.data.pointer, wrq->u.data.length);
							
							if (Status == NDIS_STATUS_SUCCESS)
							{
								if (bEnable == TRUE)
								{
									pAd->StaCfg.WfdCfg.bSuppInsertWfdIe = TRUE;
									pAd->StaCfg.WfdCfg.bWfdEnable= TRUE;
									DBGPRINT(RT_DEBUG_TRACE, ("%s:: Enable Insert WFD IE Support for wpa_supplicant!\n", __FUNCTION__));
								}
								else
									DBGPRINT(RT_DEBUG_ERROR, ("%s:: Disable Insert WFD IE Support for wpa_supplicant!\n", __FUNCTION__));
							}
							else
								DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_IE_INSERT error!!\n"));
						}
						else
							DBGPRINT(RT_DEBUG_ERROR, ("%s:: Interface mode not support Insert WFD IE for wpa_supplicant!\n", __FUNCTION__));
				
					}
					else
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: pCfg80211_CB->pCfg80211_Wdev is NULL. Not Support Insert WFD IE for wpa_supplicant!\n", __FUNCTION__));
				
				}
				else
#endif
					DBGPRINT(RT_DEBUG_ERROR, ("%s:: pAd->pCfg80211_CB is NULL. Not Support Insert WFD IE for wpa_supplicant!\n", __FUNCTION__));
			}
		}
			break;
#endif /* RT_CFG80211_SUPPORT */

		/*
		 * Set WFD device type
		 * Value:
		 * 	0:	WFD Source		
		 * 	1:	WFD Primary Sink
		 * 	2:	WFD Secondary Sink		
		 * 	3:	WFD Source/Primary Sink
		*/
		case OID_802_11_WFD_DEVICE_TYPE:
		{
			if (wrq->u.data.length > sizeof(UCHAR))
				Status  = -EINVAL;
			else
			{
				UCHAR DeviceType;
				
				Status = copy_from_user(&DeviceType, wrq->u.data.pointer, wrq->u.data.length);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					if (DeviceType <= WFD_SOURCE_PRIMARY_SINK)
					{
						pAd->StaCfg.WfdCfg.DeviceType = DeviceType;
						DBGPRINT(RT_DEBUG_TRACE, ("%s:: WFD Device Type = %d.\n", __FUNCTION__, pAd->StaCfg.WfdCfg.DeviceType));
					}
					else
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: RTSP Port out of range.\n"));
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_DEVICE_TYPE error!!\n"));
			}
		}
			break;

		/* 
		 * Set WFD device to support couple source/sink or not.
		 * Value:
		 * 	0:	WFD device not support couple sink/source		
		 * 	1:	WFD device support couple sink/source
		 */
		case OID_802_11_WFD_SOURCE_COUPLED:
		case OID_802_11_WFD_SINK_COUPLED:
		{
			if (wrq->u.data.length > sizeof(UCHAR))
				Status  = -EINVAL;
			else
			{
				UCHAR Coupled;
				
				Status = copy_from_user(&Coupled, wrq->u.data.pointer, wrq->u.data.length);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					if ((Coupled == WFD_COUPLED_NOT_SUPPORT) 
						|| (Coupled == WFD_COUPLED_SUPPORT))
					{
						if (pAd->StaCfg.WfdCfg.DeviceType == WFD_SOURCE)
						{
							pAd->StaCfg.WfdCfg.SourceCoupled = Coupled;
						}
						else if ((pAd->StaCfg.WfdCfg.DeviceType == WFD_PRIMARY_SINK) ||
								(pAd->StaCfg.WfdCfg.DeviceType == WFD_SECONDARY_SINK))
						{
							pAd->StaCfg.WfdCfg.SinkCoupled = Coupled;
						}
						else if (pAd->StaCfg.WfdCfg.DeviceType == WFD_SOURCE_PRIMARY_SINK)
						{
							pAd->StaCfg.WfdCfg.SourceCoupled = Coupled;
							pAd->StaCfg.WfdCfg.SinkCoupled = Coupled;
						}
						DBGPRINT(RT_DEBUG_TRACE, ("%s:: WFD SOURCE / SINK COUPLED = %d / %d.\n", 
								__FUNCTION__, pAd->StaCfg.WfdCfg.SourceCoupled, pAd->StaCfg.WfdCfg.SinkCoupled));
					}
					else
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: WFD SOURCE / SINK COUPLED out of range.\n"));
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_SOURCE/SINK_COUPLED error!!\n"));
			}
		}
			break;	

		/* 
		 * Set available for WFD session or not.
		 * Value:
		 * 	0:	Not available for WFD Session		
		 * 	1:	Available for WFD Session
		*/
		case OID_802_11_WFD_SESSION_AVAILABLE:
		{
			if (wrq->u.data.length > sizeof(UCHAR))
				Status  = -EINVAL;
			else
			{
				UCHAR SessionAvail;
				
				Status = copy_from_user(&SessionAvail, wrq->u.data.pointer, wrq->u.data.length);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					if ((SessionAvail == WFD_SESSION_NOT_AVAILABLE)
						|| (SessionAvail == WFD_SESSION_AVAILABLE))
					{
						pAd->StaCfg.WfdCfg.SessionAvail = SessionAvail;
						DBGPRINT(RT_DEBUG_TRACE, ("%s:: WFD SESSION AVAILABLE = %d.\n", __FUNCTION__, pAd->StaCfg.WfdCfg.SessionAvail));
					}
					else
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: WFD SESSION AVAILABLE out of range.\n"));
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_SESSION_AVAILABLE error!!\n"));
			}
		}
			break;

		/* 
		 * Set TCP port at which the WFD device listens for RTSP messages.
		 * Value: 
		 * 	0~65535 (Default 7236)
		*/
		case OID_802_11_WFD_RTSP_PORT:
		{
			if (wrq->u.data.length > sizeof(INT32))
				Status  = -EINVAL;
			else
			{
				INT32 RtspPort;
				
				Status = copy_from_user(&RtspPort, wrq->u.data.pointer, wrq->u.data.length);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					if (RtspPort < 0 || 65535 < RtspPort)
					{
						pAd->StaCfg.WfdCfg.RtspPort = WFD_RTSP_DEFAULT_PORT;
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: RTSP Port out of range, using default\n", __FUNCTION__, pAd->StaCfg.WfdCfg.RtspPort));
					}
					else
						pAd->StaCfg.WfdCfg.RtspPort = RtspPort;
					
					DBGPRINT(RT_DEBUG_TRACE, ("%s:: WFD RTSP PORT = %d.\n", __FUNCTION__, pAd->StaCfg.WfdCfg.RtspPort));
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_RTSP_PORT error!!\n"));
			}
		}
			break;

		/* 
		 * Set Maximum average throughput capability of the WFD Device represented in multiples of 1Mbps.
		 * Value: 
		 * 	1~65535 Mbps
		*/
		case OID_802_11_WFD_MAX_THROUGHPUT:
		{
			if (wrq->u.data.length > sizeof(INT32))
				Status  = -EINVAL;
			else
			{
				INT32 MaxThroughput;
				
				Status = copy_from_user(&MaxThroughput, wrq->u.data.pointer, wrq->u.data.length);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					if (MaxThroughput <= 0)
					{
						pAd->StaCfg.WfdCfg.MaxThroughput = WFD_MAX_THROUGHPUT_DEFAULT;
						DBGPRINT(RT_DEBUG_ERROR, ("%s:: Max Throughput out of range, using default\n", __FUNCTION__, pAd->StaCfg.WfdCfg.MaxThroughput));
					}
					else
						pAd->StaCfg.WfdCfg.MaxThroughput = MaxThroughput;

					DBGPRINT(RT_DEBUG_TRACE, ("%s:: WFD MAX THROUGHPUT = %d.\n", __FUNCTION__, pAd->StaCfg.WfdCfg.MaxThroughput));
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_MAX_THROUGHPUT error!!\n"));
				}
			}
			break;

		/* 
 		 * Set Content Protect capability of the WFD Device
		 * 	0:	Not supported		
		 * 	1:	Supported
		 */
        case OID_802_11_WFD_CONTENT_PROTECT:
        {
            if (wrq->u.data.length > sizeof(INT32))
				Status = -EINVAL;
			else
			{
                INT32 ContentProtect = 0;

                Status = copy_from_user(&ContentProtect, wrq->u.data.pointer, wrq->u.data.length);
                if (Status == NDIS_STATUS_SUCCESS)
                {
                    if (ContentProtect < 0)
				{
                        pAd->StaCfg.WfdCfg.CP= WFD_CONTENT_PROTECT_DEFAULT;
                        DBGPRINT(RT_DEBUG_ERROR, ("%s:: Content Protect out of range, using default\n", __FUNCTION__, pAd->StaCfg.WfdCfg.CP));
                    }
                    else
                        pAd->StaCfg.WfdCfg.CP = ContentProtect;

                    DBGPRINT(RT_DEBUG_TRACE, ("%s:: WFD CONTENT PROTECT = %d.\n", __FUNCTION__, pAd->StaCfg.WfdCfg.CP));
                }
                else
                    DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_WFD_CONTENT_PROTECT error!!\n"));
				}
			}
			break;
#endif /* WFD_SUPPORT */

#ifdef WIDI_SUPPORT
#ifdef WFA_WFD_SUPPORT
		case OID_WFD_IE_IN_BEACON:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_WFD_IE_IN_BEACON \n"));
			if (pAd->pWfdIeInBeacon)
			{
				os_free_mem(NULL, pAd->pWfdIeInBeacon);
				pAd->pWfdIeInBeacon = NULL;
			}
			if (wrq->u.data.length <= 0xFF)
				pAd->WfdIeInBeaconLen = wrq->u.data.length;
			else
				pAd->WfdIeInBeaconLen = 0;
			if (pAd->WfdIeInBeaconLen != 0)
			{
				os_alloc_mem(NULL, &pAd->pWfdIeInBeacon, pAd->WfdIeInBeaconLen);
			}
			if (pAd->pWfdIeInBeacon)
			{
				if (copy_from_user(pAd->pWfdIeInBeacon, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
			}
			break;

		case OID_WFD_IE_IN_PROBE_REQ:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_WFD_IE_IN_PROBE_REQ \n"));
			if (pAd->pWfdIeInProbeReq)
			{
				os_free_mem(NULL, pAd->pWfdIeInProbeReq);
				pAd->pWfdIeInProbeReq = NULL;
			}
			if (wrq->u.data.length < 0xFF)
				pAd->WfdIeInProbeReqLen = wrq->u.data.length;
			else
				pAd->WfdIeInProbeReqLen = 0;
			if (pAd->WfdIeInProbeReqLen != 0)
			{
				os_alloc_mem(NULL, &pAd->pWfdIeInProbeReq, pAd->WfdIeInProbeReqLen);
			}
			if (pAd->pWfdIeInProbeReq)
			{
				if (copy_from_user(pAd->pWfdIeInProbeReq, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
			}
			break;

		case OID_WFD_IE_IN_PROBE_RSP:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_WFD_IE_IN_PROBE_RSP \n"));
			if (pAd->pWfdIeInProbeRsp)
			{
				os_free_mem(NULL, pAd->pWfdIeInProbeRsp);
				pAd->pWfdIeInProbeRsp = NULL;
			}
			if (wrq->u.data.length < 0xFF)
				pAd->WfdIeInProbeRspLen = wrq->u.data.length;
			else
				pAd->WfdIeInProbeRspLen = 0;
			if (pAd->WfdIeInProbeRspLen != 0)
			{
				os_alloc_mem(NULL, &pAd->pWfdIeInProbeRsp, pAd->WfdIeInProbeRspLen);
			}
			if (pAd->pWfdIeInProbeRsp)
			{
				if (copy_from_user(pAd->pWfdIeInProbeRsp, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
			}
			break;
#endif /* WFA_WFD_SUPPORT */
#endif /* WIDI_SUPPORT */
#endif /* P2P_SUPPORT */

    	case OID_802_11_DEAUTHENTICATION:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_DEAUTHENTICATION\n"));
			if (wrq->u.data.length != sizeof(MLME_DEAUTH_REQ_STRUCT))
				Status  = -EINVAL;
			else
			{                								
				MAC_TABLE_ENTRY 		*pEntry = NULL;
				MLME_DEAUTH_REQ_STRUCT  *pInfo;
				MLME_QUEUE_ELEM 		*Elem; /* = (MLME_QUEUE_ELEM *) kmalloc(sizeof(MLME_QUEUE_ELEM), MEM_ALLOC_FLAG); */

				os_alloc_mem(pAd, (UCHAR **)&Elem, sizeof(MLME_QUEUE_ELEM));
				if (Elem)
				{
					pInfo = (MLME_DEAUTH_REQ_STRUCT *) Elem->Msg;
					Status = copy_from_user(pInfo, wrq->u.data.pointer, wrq->u.data.length);

					if ((pEntry = MacTableLookup(pAd, pInfo->Addr)) != NULL)
					{					
						Elem->Wcid = pEntry->wcid;
						MlmeEnqueue(pAd, AP_AUTH_STATE_MACHINE, APMT2_MLME_DEAUTH_REQ,
										sizeof(MLME_DEAUTH_REQ_STRUCT), Elem, 0);
						DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_DEAUTHENTICATION (Reason=%d)\n", pInfo->Reason));
					}
/*					kfree(Elem); */
					os_free_mem(NULL, Elem);
				}
				else
					Status = -EFAULT;
			}
			
			break;
#ifdef IAPP_SUPPORT
    	case RT_SET_IAPP_PID:
			{
				unsigned long IappPid;
				if (copy_from_user(&IappPid, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
    			else
    			{
					RTMP_GET_OS_PID(pObj->IappPid, IappPid);
					pObj->IappPid_nr = IappPid;
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_APD_PID::(IappPid=%lu(0x%x))\n", IappPid, pObj->IappPid));
				}
    		}
			break;
#endif /* IAPP_SUPPORT */

#ifdef DOT11R_FT_SUPPORT
		case RT_SET_FT_STATION_NOTIFY:
		case RT_SET_FT_KEY_REQ:
		case RT_SET_FT_KEY_RSP:
		case RT_FT_KEY_SET:
		case RT_FT_NEIGHBOR_REPORT:
		case RT_FT_NEIGHBOR_REQUEST:
		case RT_FT_NEIGHBOR_RESPONSE:
		case RT_FT_ACTION_FORWARD:
		{
			UCHAR *pBuffer;

			FT_MEM_ALLOC(pAd, &pBuffer, wrq->u.data.length+1);
			if (pBuffer == NULL)
				break;

			if (copy_from_user(pBuffer, wrq->u.data.pointer, wrq->u.data.length))
			{
				Status = -EFAULT;
				FT_MEM_FREE(pAd, pBuffer);
				break;
			}

			switch(cmd & 0x7FFF)
			{
				case RT_SET_FT_STATION_NOTIFY:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_FT_STATION_NOTIFY\n"));
					FT_KDP_StationInform(pAd, pBuffer, wrq->u.data.length);
					break;
				case RT_SET_FT_KEY_REQ:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_FT_KEY_REQ\n"));
					FT_KDP_IOCTL_KEY_REQ(pAd, pBuffer, wrq->u.data.length);
					break;
				case RT_SET_FT_KEY_RSP:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_FT_KEY_RSP\n"));
					FT_KDP_KeyResponseToUs(pAd, pBuffer, wrq->u.data.length);
					break;
				case RT_FT_KEY_SET:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_FT_KEY_SET\n"));
					/* Note: the key must be ended by 0x00 */
					pBuffer[wrq->u.data.length] = 0x00;
					FT_KDP_CryptKeySet(pAd, pBuffer, wrq->u.data.length);
					break;
				case RT_FT_NEIGHBOR_REPORT:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_FT_NEIGHBOR_REPORT\n"));
#ifdef FT_KDP_FUNC_INFO_BROADCAST
					FT_KDP_NeighborReportHandle(pAd, pBuffer, wrq->u.data.length);
#endif /* FT_KDP_FUNC_INFO_BROADCAST */
					break;
				case RT_FT_NEIGHBOR_REQUEST:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_FT_NEIGHBOR_REPORT\n"));
					FT_KDP_NeighborRequestHandle(pAd, pBuffer, wrq->u.data.length);
					break;
				case RT_FT_NEIGHBOR_RESPONSE:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_FT_NEIGHBOR_RESPONSE\n"));
					FT_KDP_NeighborResponseHandle(pAd, pBuffer, wrq->u.data.length);
					break;
				case RT_FT_ACTION_FORWARD:
					DBGPRINT(RT_DEBUG_TRACE, ("RT_FT_ACTION_FORWARD\n"));
					FT_RRB_ActionHandle(pAd, pBuffer, wrq->u.data.length);
					break;
			}

			FT_MEM_FREE(pAd, pBuffer);
		}
			break;

		case OID_802_11R_SUPPORT:
			if (wrq->u.data.length != sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				UCHAR apidx = pObj->ioctl_if;
				ULONG value;
				Status = copy_from_user(&value, wrq->u.data.pointer, wrq->u.data.length);
				pAd->ApCfg.MBSSID[apidx].FtCfg.FtCapFlag.Dot11rFtEnable = (value == 0 ? FALSE : TRUE);
				DBGPRINT(RT_DEBUG_TRACE,("Set::OID_802_11R_SUPPORT(=%d) \n",
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtCapFlag.Dot11rFtEnable));
			}
			break;

		case OID_802_11R_MDID:
			if (wrq->u.data.length != FT_MDID_LEN)
				Status  = -EINVAL;
			else
			{
				UCHAR apidx = pObj->ioctl_if;
				Status = copy_from_user(pAd->ApCfg.MBSSID[apidx].FtCfg.FtMdId, wrq->u.data.pointer, wrq->u.data.length);				
				DBGPRINT(RT_DEBUG_TRACE,("Set::OID_802_11R_MDID(=%c%c) \n",
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtMdId[0],
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtMdId[0]));
			}

			break;


		case OID_802_11R_R0KHID:
			if (wrq->u.data.length <= FT_ROKH_ID_LEN)
				Status  = -EINVAL;
			else
			{
				UCHAR apidx = pObj->ioctl_if;
				Status = copy_from_user(pAd->ApCfg.MBSSID[apidx].FtCfg.FtR0khId, wrq->u.data.pointer, wrq->u.data.length);
				pAd->ApCfg.MBSSID[apidx].FtCfg.FtR0khIdLen = wrq->u.data.length;
				DBGPRINT(RT_DEBUG_TRACE,("Set::OID_802_11R_OID_802_11R_R0KHID(=%s) Len=%d\n",
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtR0khId,
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtR0khIdLen));
			}
			break;

		case OID_802_11R_RIC:
			if (wrq->u.data.length != sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				UCHAR apidx = pObj->ioctl_if;
				ULONG value;
				Status = copy_from_user(&value, wrq->u.data.pointer, wrq->u.data.length);
				pAd->ApCfg.MBSSID[apidx].FtCfg.FtCapFlag.RsrReqCap = (value == 0 ? FALSE : TRUE);
				DBGPRINT(RT_DEBUG_TRACE,("Set::OID_802_11R_RIC(=%d) \n",
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtCapFlag.Dot11rFtEnable));
			}
			break;

		case OID_802_11R_OTD:
			if (wrq->u.data.length != sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				UCHAR apidx = pObj->ioctl_if;
				ULONG value;
				Status = copy_from_user(&value, wrq->u.data.pointer, wrq->u.data.length);
				pAd->ApCfg.MBSSID[apidx].FtCfg.FtCapFlag.FtOverDs = (value == 0 ? FALSE : TRUE);
				DBGPRINT(RT_DEBUG_TRACE,("Set::OID_802_11R_OTD(=%d) \n",
							pAd->ApCfg.MBSSID[apidx].FtCfg.FtCapFlag.Dot11rFtEnable));
			}
			break;
#endif /* DOT11R_FT_SUPPORT */
    	case RT_SET_APD_PID:
			{
				unsigned long apd_pid;
				if (copy_from_user(&apd_pid, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
    			else
    			{
					RTMP_GET_OS_PID(pObj->apd_pid, apd_pid);
					pObj->apd_pid_nr = apd_pid;
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_APD_PID::(ApdPid=%lu(0x%x))\n", apd_pid, pObj->apd_pid));
				}
    		}
			break;
		case RT_SET_DEL_MAC_ENTRY:
    		if (copy_from_user(Addr, wrq->u.data.pointer, wrq->u.data.length))
		{
				Status = -EFAULT; 	
		}
    		else
    		{
			UCHAR HashIdx;
			MAC_TABLE_ENTRY *pEntry = NULL;
			
			DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_DEL_MAC_ENTRY::(%02x:%02x:%02x:%02x:%02x:%02x)\n", Addr[0],Addr[1],Addr[2],Addr[3],Addr[4],Addr[5]));

			HashIdx = MAC_ADDR_HASH_INDEX(Addr);
			pEntry = pAd->MacTab.Hash[HashIdx];
			
			if (pEntry)
			{
				MlmeDeAuthAction(pAd, pEntry, REASON_DISASSOC_STA_LEAVING, FALSE);
/*					MacTableDeleteEntry(pAd, pEntry->wcid, Addr); */
			}
    		}
			break;
#ifdef WSC_AP_SUPPORT
		case RT_OID_WSC_SET_SELECTED_REGISTRAR:
			{	
				PUCHAR      upnpInfo;
				UCHAR	    apidx = pObj->ioctl_if;
				
#ifdef HOSTAPD_SUPPORT
				if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
					{
						DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
						Status = -EINVAL;
					}
				else
				{
#endif /*HOSTAPD_SUPPORT*/

				
				DBGPRINT(RT_DEBUG_TRACE, ("WSC::RT_OID_WSC_SET_SELECTED_REGISTRAR, wrq->u.data.length=%d!\n", wrq->u.data.length));
/*				upnpInfo = kmalloc(wrq->u.data.length, GFP_KERNEL); */
				os_alloc_mem(pAd, (UCHAR **)&upnpInfo, wrq->u.data.length);
				if(upnpInfo)
				{
					int len, Status;
					
					Status = copy_from_user(upnpInfo, wrq->u.data.pointer, wrq->u.data.length);
					if (Status == NDIS_STATUS_SUCCESS)
					{
						len = wrq->u.data.length;
						
						if((pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode & WSC_PROXY))
						{
							WscSelectedRegistrar(pAd, upnpInfo, len, apidx);
							if (pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimerRunning == TRUE)
							{
								BOOLEAN Cancelled;
								RTMPCancelTimer(&pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimer, &Cancelled);
							}
							/* 2mins time-out timer */
							RTMPSetTimer(&pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
							pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimerRunning = TRUE;
						}
					}
/*					kfree(upnpInfo); */
					os_free_mem(NULL, upnpInfo);
				} 
				else 
				{
					Status = -EINVAL;
				}
#ifdef HOSTAPD_SUPPORT
					}
#endif /*HOSTAPD_SUPPORT*/

			}
			break;
		case RT_OID_WSC_EAPMSG:
			{
				RTMP_WSC_U2KMSG_HDR *msgHdr = NULL;
				PUCHAR pUPnPMsg = NULL;
				UINT msgLen = 0, Machine = 0, msgType = 0;
				int retVal, senderID = 0;
#ifdef HOSTAPD_SUPPORT
				UCHAR	    apidx = pObj->ioctl_if;

				if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
					{
						DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
						Status = -EINVAL;
					}
				else
				{
#endif /*HOSTAPD_SUPPORT*/


				DBGPRINT(RT_DEBUG_TRACE, ("WSC::RT_OID_WSC_EAPMSG, wrq->u.data.length=%d, ioctl_if=%d\n", wrq->u.data.length, pObj->ioctl_if));
			
				msgLen = wrq->u.data.length;				
				os_alloc_mem(pAd, (UCHAR **)&pUPnPMsg, msgLen);
/*				if((pUPnPMsg = kmalloc(msgLen, GFP_KERNEL)) == NULL) */
				if (pUPnPMsg == NULL)
					Status = -EINVAL;
				else
				{
					int HeaderLen;
					RTMP_STRING *pWpsMsg;
					UINT WpsMsgLen;
					PWSC_CTRL pWscControl;
					BOOLEAN	bGetDeviceInfo = FALSE;

					NdisZeroMemory(pUPnPMsg, msgLen);
					retVal = copy_from_user(pUPnPMsg, wrq->u.data.pointer, msgLen);

					msgHdr = (RTMP_WSC_U2KMSG_HDR *)pUPnPMsg;
					senderID = get_unaligned((INT32 *)(&msgHdr->Addr2[0]));
					/*senderID = *((int *)&msgHdr->Addr2); */

					DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_EAPMSG++++++++\n\n"));
					hex_dump("MAC::", &msgHdr->Addr3[0], MAC_ADDR_LEN);
					DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_EAPMSG++++++++\n\n"));					

					HeaderLen = LENGTH_802_11 + LENGTH_802_1_H + sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME);
					pWpsMsg = (RTMP_STRING *) &pUPnPMsg[HeaderLen];
					WpsMsgLen = msgLen - HeaderLen;

					/*assign the STATE_MACHINE type */
					Machine = WSC_STATE_MACHINE;
					msgType = WSC_EAPOL_UPNP_MSG;

					pWscControl = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl;
					/* If AP is unconfigured, WPS state machine will be triggered after received M2. */
					if ((pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED)
#ifdef WSC_V2_SUPPORT
						&& (pWscControl->WscV2Info.bWpsEnable || (pWscControl->WscV2Info.bEnableWpsV2 == FALSE))
#endif /* WSC_V2_SUPPORT */
						)
					{
						if (strstr(pWpsMsg, "SimpleConfig") &&
							!pWscControl->EapMsgRunning &&
							!pWscControl->WscUPnPNodeInfo.bUPnPInProgress)
						{
							/* GetDeviceInfo */
							WscInit(pAd, FALSE, pObj->ioctl_if & MIN_NET_DEVICE_FOR_P2P_GO);
							/* trigger wsc re-generate public key */
							pWscControl->RegData.ReComputePke = 1;
							bGetDeviceInfo = TRUE;
						}
						else if (WscRxMsgTypeFromUpnp(pAd, pWpsMsg, WpsMsgLen) == WSC_MSG_M2 &&
								!pWscControl->EapMsgRunning &&
								!pWscControl->WscUPnPNodeInfo.bUPnPInProgress)
						{
							/* Check Enrollee Nonce of M2 */
							if (WscCheckEnrolleeNonceFromUpnp(pAd, pWpsMsg, WpsMsgLen, pWscControl))
							{
								WscGetConfWithoutTrigger(pAd, pWscControl, TRUE);
								pWscControl->WscState = WSC_STATE_SENT_M1;
							}
						}
					}


					retVal = MlmeEnqueueForWsc(pAd, msgHdr->envID, senderID, Machine, msgType, msgLen, pUPnPMsg);
					if((retVal == FALSE) && (msgHdr->envID != 0))
					{
						DBGPRINT(RT_DEBUG_TRACE, ("MlmeEnqueuForWsc return False and envID=0x%x!\n", msgHdr->envID));
						Status = -EINVAL;
					}

					os_free_mem(NULL, pUPnPMsg);
				}
				DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_EAPMSG finished!\n"));
#ifdef HOSTAPD_SUPPORT
					}
#endif /*HOSTAPD_SUPPORT*/
			}
			break;

		case RT_OID_WSC_READ_UFD_FILE:
			if (wrq->u.data.length > 0)
			{
				RTMP_STRING *pWscUfdFileName = NULL;
				UCHAR 		apIdx = pObj->ioctl_if;
				PWSC_CTRL	pWscCtrl = &pAd->ApCfg.MBSSID[apIdx].WscControl;
/*				pWscUfdFileName = (RTMP_STRING *)kmalloc(wrq->u.data.length+1, MEM_ALLOC_FLAG); */
				os_alloc_mem(pAd, (UCHAR **)&pWscUfdFileName, wrq->u.data.length+1);
				if (pWscUfdFileName)
				{
					RTMPZeroMemory(pWscUfdFileName, wrq->u.data.length+1);
					if (copy_from_user(pWscUfdFileName, wrq->u.data.pointer, wrq->u.data.length))
						Status = -EFAULT;
					else
					{
						DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_READ_UFD_FILE (WscUfdFileName=%s)\n", pWscUfdFileName));
						if (pWscCtrl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED)
						{
							if (WscReadProfileFromUfdFile(pAd, apIdx, pWscUfdFileName))
							{
								pWscCtrl->WscConfStatus = WSC_SCSTATE_CONFIGURED;
								APStop(pAd);
								APStartUp(pAd);
							}
						}
						else
						{
							DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_READ_UFD_FILE: AP is configured.\n"));
							Status = -EINVAL;
						}
					}
/*					kfree(pWscUfdFileName); */
					os_free_mem(NULL, pWscUfdFileName);
				}
				else
					Status = -ENOMEM;
			}
			else
				Status = -EINVAL;
			break;
			
		case RT_OID_WSC_WRITE_UFD_FILE:
			if (wrq->u.data.length > 0)
			{
				RTMP_STRING *pWscUfdFileName = NULL;
				UCHAR 		apIdx = pObj->ioctl_if;
				PWSC_CTRL	pWscCtrl = &pAd->ApCfg.MBSSID[apIdx].WscControl;
/*				pWscUfdFileName = (RTMP_STRING *)kmalloc(wrq->u.data.length+1, MEM_ALLOC_FLAG); */
				os_alloc_mem(pAd, (UCHAR **)&pWscUfdFileName, wrq->u.data.length+1);
				if (pWscUfdFileName)
				{
					RTMPZeroMemory(pWscUfdFileName, wrq->u.data.length+1);
					if (copy_from_user(pWscUfdFileName, wrq->u.data.pointer, wrq->u.data.length))
						Status = -EFAULT;
					else
					{
						DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_WRITE_UFD_FILE (WscUfdFileName=%s)\n", pWscUfdFileName));
						if (pWscCtrl->WscConfStatus == WSC_SCSTATE_CONFIGURED)
						{
							WscWriteProfileToUfdFile(pAd, apIdx, pWscUfdFileName);
						}
						else
						{
							DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_WRITE_UFD_FILE: AP is un-configured.\n"));
							Status = -EINVAL;
						}
					}
/*					kfree(pWscUfdFileName); */
					os_free_mem(NULL, pWscUfdFileName);
				}
				else
					Status = -ENOMEM;
			}
			else
				Status = -EINVAL;
			break;

		case RT_OID_WSC_UUID:
			if (wrq->u.data.length == (UUID_LEN_STR-1))
			{
				UCHAR 		apIdx = pObj->ioctl_if;
				pAd->ApCfg.MBSSID[apIdx].WscControl.Wsc_Uuid_Str[0] = '\0';
				Status = copy_from_user(&pAd->ApCfg.MBSSID[apIdx].WscControl.Wsc_Uuid_Str[0],
										wrq->u.data.pointer, 
										wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("UUID ASCII string: %s\n", 
										pAd->ApCfg.MBSSID[apIdx].WscControl.Wsc_Uuid_Str));
			}
			else if (wrq->u.data.length == UUID_LEN_HEX)
			{
				UCHAR 		apIdx = pObj->ioctl_if, ii;				
				Status = copy_from_user(&pAd->ApCfg.MBSSID[apIdx].WscControl.Wsc_Uuid_E[0],
										wrq->u.data.pointer, 
										wrq->u.data.length);

				for (ii=0; ii< 16; ii++)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("%02x", (pAd->ApCfg.MBSSID[apIdx].WscControl.Wsc_Uuid_E[ii] & 0xff)));
				}
			}
			else
				Status = -EINVAL;
			break;
#endif /* WSC_AP_SUPPORT */

#ifdef NINTENDO_AP
		case RT_OID_802_11_NINTENDO_SET_TABLE:
		    RTMPIoctlNintendoSetTable(pAd, wrq);
			break;
		case RT_OID_802_11_NINTENDO_CAPABLE:
		    RTMPIoctlNintendoCapable(pAd, wrq);
			break;
#endif /* NINTENDO_AP */

#ifdef SNMP_SUPPORT
		case OID_802_11_SHORTRETRYLIMIT:
			if (wrq->u.data.length != sizeof(ULONG))
				Status = -EINVAL;
			else
			{
				Status = copy_from_user(&ShortRetryLimit, wrq->u.data.pointer, wrq->u.data.length);
				RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
				tx_rty_cfg.field.ShortRtyLimit = ShortRetryLimit;
				RTMP_IO_WRITE32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_SHORTRETRYLIMIT (tx_rty_cfg.field.ShortRetryLimit=%d, ShortRetryLimit=%ld)\n", tx_rty_cfg.field.ShortRtyLimit, ShortRetryLimit));
			}
			break;

		case OID_802_11_LONGRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_LONGRETRYLIMIT \n"));
			if (wrq->u.data.length != sizeof(ULONG))
				Status = -EINVAL;
			else
			{
				Status = copy_from_user(&LongRetryLimit, wrq->u.data.pointer, wrq->u.data.length);
				RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
				tx_rty_cfg.field.LongRtyLimit = LongRetryLimit;
				RTMP_IO_WRITE32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_LONGRETRYLIMIT (tx_rty_cfg.field.LongRetryLimit= %d,LongRetryLimit=%ld)\n", tx_rty_cfg.field.LongRtyLimit, LongRetryLimit));
			}
			break;

		case OID_802_11_WEPDEFAULTKEYVALUE:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYVALUE\n"));
/*			pKey = kmalloc(wrq->u.data.length, GFP_KERNEL); */
			os_alloc_mem(pAd, (UCHAR **)&pKey, wrq->u.data.length);
			if (pKey == NULL)
			{
				Status= -EINVAL;
				break;
			}
			Status = copy_from_user(pKey, wrq->u.data.pointer, wrq->u.data.length);
			/*pKey = &WepKey; */
			
			if ( pKey->Length != wrq->u.data.length)
			{
				Status = -EINVAL;
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYVALUE, Failed!!\n"));
			}
			KeyIdx = pKey->KeyIndex & 0x0fffffff;
			DBGPRINT(RT_DEBUG_TRACE,("pKey->KeyIndex =%d, pKey->KeyLength=%d\n", pKey->KeyIndex, pKey->KeyLength));

			/* it is a shared key */
			if (KeyIdx > 4)
				Status = -EINVAL;
			else
			{
				pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen = (UCHAR) pKey->KeyLength;
				NdisMoveMemory(&pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].Key, &pKey->KeyMaterial, pKey->KeyLength);
				if (pKey->KeyIndex & 0x80000000)
				{
					/* Default key for tx (shared key) */
					pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId = (UCHAR) KeyIdx;
				}
				/*RestartAPIsRequired = TRUE; */
			}
			os_free_mem(NULL, pKey);
			break;


		case OID_802_11_WEPDEFAULTKEYID:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYID \n"));

			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
				Status = copy_from_user(&pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId, wrq->u.data.pointer, wrq->u.data.length);

			break;


		case OID_802_11_CURRENTCHANNEL:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_CURRENTCHANNEL \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				RTMP_STRING ChStr[5] = {0};
				Status = copy_from_user(&ctmp, wrq->u.data.pointer, wrq->u.data.length);
				snprintf(ChStr, sizeof(ChStr), "%d", ctmp);
				Set_Channel_Proc(pAd, ChStr);
			}
			break;
#endif /* SNMP_SUPPORT */

#ifdef MESH_SUPPORT
		case OID_802_11_MESH_SECURITY_INFO:			
			Status = RTMPIoctlSetMeshSecurityInfo(pAd, wrq);		
			if (Status == NDIS_STATUS_SUCCESS)
				RestartMeshIsRequired = TRUE;
			break;

		case OID_802_11_MESH_ID:
			if (wrq->u.data.length >= MAX_MESH_ID_LEN)
			{
				Status = -EINVAL;
				break;
			}

			{
				UCHAR MeshID[MAX_MESH_ID_LEN];

				NdisZeroMemory(pAd->MeshTab.MeshId, MAX_MESH_ID_LEN);

				Status = copy_from_user(&MeshID,wrq->u.data.pointer, wrq->u.data.length);
				NdisMoveMemory(&pAd->MeshTab.MeshId, MeshID, wrq->u.data.length);
				pAd->MeshTab.MeshIdLen = (UCHAR)wrq->u.data.length;
				DBGPRINT(RT_DEBUG_TRACE, ("Set::RT_OID_802_11_MESH_ID (=%s)\n", pAd->MeshTab.MeshId));
				RestartMeshIsRequired = TRUE;
			}
			break;

		case OID_802_11_MESH_AUTO_LINK:
			if (wrq->u.data.length != 1)
			{
				Status = -EINVAL;
				break;
			}

			{
				UCHAR Enable = TRUE;

				Status = copy_from_user(&Enable, wrq->u.data.pointer, wrq->u.data.length);
				pAd->MeshTab.MeshAutoLink = (Enable > 0) ? TRUE : FALSE;
				DBGPRINT(RT_DEBUG_TRACE, ("Set::RT_OID_802_11_MESH_AUTO_LINK (=%s)\n",
					pAd->MeshTab.MeshAutoLink == TRUE ? "Enable" : "Disable"));
			}
			break;

		case OID_802_11_MESH_ADD_LINK:
			if (wrq->u.data.length != MAC_ADDR_LEN)
			{
				Status = -EINVAL;
				break;
			}

			{
				UCHAR MacAddr[MAC_ADDR_LEN];
				UINT LinkIdx;

				Status = copy_from_user(&MacAddr, wrq->u.data.pointer, MAC_ADDR_LEN);
				LinkIdx = GetMeshLinkId(pAd, (PCHAR)MacAddr);
				if (LinkIdx == BSS_NOT_FOUND)
				{
					LinkIdx = MeshLinkAlloc(pAd, MacAddr, MESH_LINK_STATIC);
					if (LinkIdx == BSS_NOT_FOUND)
					{
						DBGPRINT(RT_DEBUG_TRACE, ("%s() All Mesh-Links been occupied.\n", __FUNCTION__));
						return FALSE;
					}
				}

				if (!VALID_MESH_LINK_ID(LinkIdx))
					break;

				MlmeEnqueue(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_ACTOPN, 0, NULL, LinkIdx);
				DBGPRINT(RT_DEBUG_TRACE, ("%s::(LinkIdx = %d)\n", __FUNCTION__, LinkIdx));
			}
			break;

		case OID_802_11_MESH_DEL_LINK:
			if (wrq->u.data.length != MAC_ADDR_LEN)
			{
				Status = -EINVAL;
				break;
			}

			{
				UCHAR MacAddr[MAC_ADDR_LEN];
				UINT LinkIdx;

				Status = copy_from_user(&MacAddr, wrq->u.data.pointer, MAC_ADDR_LEN);
				LinkIdx = GetMeshLinkId(pAd, (PCHAR)MacAddr);
				if (!VALID_MESH_LINK_ID(LinkIdx))
					break;

				pAd->MeshTab.MeshLink[LinkIdx].Entry.LinkType = MESH_LINK_DYNAMIC;
				MlmeEnqueue(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_CNCL, 0, NULL, LinkIdx);
				DBGPRINT(RT_DEBUG_TRACE, ("%s::(LinkIdx = %d)\n", __FUNCTION__, LinkIdx));
			}
			break;

		case OID_802_11_MESH_MAX_TX_RATE:
			if (wrq->u.data.length != 1)
			{
				Status = -EINVAL;
				break;
			}

			{
				UCHAR TxRate;

				Status = copy_from_user(&TxRate, wrq->u.data.pointer, wrq->u.data.length);
				if (TxRate <= 12)
					pAd->MeshTab.MeshMaxTxRate = TxRate;
				else
					pAd->MeshTab.MeshMaxTxRate = 0;

				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_MESH_MAX_TX_RATE (=%ld)\n",
					pAd->MeshTab.MeshMaxTxRate));
			}
			break;

		case OID_802_11_MESH_CHANNEL:
			if (wrq->u.data.length != 1)
			{
				Status = -EINVAL;
				break;
			}
			{
				UCHAR Channel;

				if (INFRA_ON(pAd) || ADHOC_ON(pAd))
					break;

				Status = copy_from_user(&Channel, wrq->u.data.pointer, wrq->u.data.length);
				if (pAd->MeshTab.UCGEnable == TRUE)
				{
					pAd->MeshTab.MeshChannel = Channel;
					MlmeEnqueue(pAd, MESH_CTRL_STATE_MACHINE, MESH_CTRL_UCG_EVT, 0, NULL, 0);
				}
				else
				{
					pAd->MeshTab.MeshChannel = pAd->CommonCfg.Channel = Channel;
#ifdef DOT11_N_SUPPORT
					N_ChannelCheck(pAd);
					if (WMODE_CAP_N(pAd->CommonCfg.PhyMode)
						&& pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40)
					{
						N_SetCenCh(pAd, pAd->CommonCfg.Channel);
						AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
						AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
						DBGPRINT(RT_DEBUG_TRACE, ("BW_40, control_channel(%d), CentralChannel(%d) \n",
							pAd->CommonCfg.Channel, pAd->CommonCfg.CentralChannel));
					}
					else
#endif /* DOT11_N_SUPPORT */
					{
						AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
						AsicLockChannel(pAd, pAd->CommonCfg.Channel);
						DBGPRINT(RT_DEBUG_TRACE, ("BW_20, Channel(%d)\n", pAd->CommonCfg.Channel));
					}
				}

				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_MESH_CHANNEL (=%d)\n", (int)Channel));
			}
			break;

		case OID_802_11_MESH_HOSTNAME:
			if (wrq->u.data.length >= MAX_HOST_NAME_LEN)
			{
				Status = -EINVAL;
				break;
			}

			{
				NdisZeroMemory(pAd->MeshTab.HostName, MAX_HOST_NAME_LEN);
				Status = copy_from_user(pAd->MeshTab.HostName, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::RT_OID_802_11_HOSTNAME_ID (=%s)\n", pAd->MeshTab.HostName));
			}
			break;

		case OID_802_11_MESH_ONLY_MODE:
			if (wrq->u.data.length != 1)
			{
				Status = -EINVAL;
				break;
			}
			{
				UCHAR Enable;

				Status = copy_from_user(&Enable, wrq->u.data.pointer, wrq->u.data.length);

				pAd->MeshTab.MeshOnly = (Enable == 1 ? TRUE : FALSE);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_MESH_ONLY_MODE (=%s)\n", pAd->MeshTab.MeshOnly == TRUE ? "Enable" : "Disable"));
			}
			break;

		case OID_802_11_MESH_FORWARD:
			if (wrq->u.data.length != 1)
			{
				Status = -EINVAL;
				break;
			}

			{
				UCHAR Enable = TRUE;

				Status = copy_from_user(&Enable, wrq->u.data.pointer, wrq->u.data.length);
				pAd->MeshTab.MeshCapability.field.Forwarding = (Enable > 0) ? (1) : (0);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::RT_OID_802_11_MESH_FORWARD (=%s)\n",
					pAd->MeshTab.MeshCapability.field.Forwarding == 1 ? "Enable" : "Disable"));
			}
			break;
#endif /* MESH_SUPPORT */

#ifdef WAPI_SUPPORT
		case OID_802_11_WAPI_PID:
			{
				unsigned long wapi_pid;
    			if (copy_from_user(&wapi_pid, wrq->u.data.pointer, wrq->u.data.length))
				{
					Status = -EFAULT; 	
				}
    			else
    			{
					RTMP_GET_OS_PID(pObj->wapi_pid, wapi_pid);
					pObj->wapi_pid_nr = wapi_pid;
					DBGPRINT(RT_DEBUG_TRACE, ("OID_802_11_WAPI_PID::(WapiPid=%lu(0x%x))\n", wapi_pid, pObj->wapi_pid));
				}
    		}
			break;

		case OID_802_11_PORT_SECURE_STATE:
#ifdef RELEASE_EXCLUDE
			DBGPRINT(RT_DEBUG_INFO, ("Set::OID_802_11_PORT_SECURE_STATE\n"));
#endif /* RELEASE_EXCLUDE */
			if (wrq->u.data.length != sizeof(WAPI_PORT_SECURE_STRUCT))
                Status  = -EINVAL;
            else
            {                								
				MAC_TABLE_ENTRY 		*pEntry = NULL;
				WAPI_PORT_SECURE_STRUCT  wapi_port;

				Status = copy_from_user(&wapi_port, wrq->u.data.pointer, wrq->u.data.length);
                if (Status == NDIS_STATUS_SUCCESS)
                {
					if ((pEntry = MacTableLookup(pAd, wapi_port.Addr)) != NULL)
					{
						switch (wapi_port.state)
						{
							case WAPI_PORT_SECURED:
								pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
								pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
								break;
							
							default:
								pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
								pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
								break;								
						}											
					}
					DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_PORT_SECURE_STATE (state=%d)\n", wapi_port.state));
				}				
            }
			break;
			
		case OID_802_11_UCAST_KEY_INFO:
#ifdef RELEASE_EXCLUDE
			DBGPRINT(RT_DEBUG_INFO, ("Set::OID_802_11_UCAST_KEY_INFO\n"));
#endif /* RELEASE_EXCLUDE */
			if (wrq->u.data.length != sizeof(WAPI_UCAST_KEY_STRUCT))
                Status  = -EINVAL;
            else
            {                								
				MAC_TABLE_ENTRY 		*pEntry = NULL;
				WAPI_UCAST_KEY_STRUCT   wapi_ukey;

				Status = copy_from_user(&wapi_ukey, wrq->u.data.pointer, wrq->u.data.length);
                if (Status == NDIS_STATUS_SUCCESS)
                {
					if ((pEntry = MacTableLookup(pAd, wapi_ukey.Addr)) != NULL)
					{
						pEntry->usk_id = wapi_ukey.key_id;
						NdisMoveMemory(pEntry->PTK, wapi_ukey.PTK, 64);								

						/* Install pairwise key */
						WAPIInstallPairwiseKey(pAd, pEntry, TRUE);

						/* Start or re-start USK rekey mechanism, if necessary. */
						RTMPCancelWapiRekeyTimerAction(pAd, pEntry);
						RTMPStartWapiRekeyTimerAction(pAd, pEntry);
					}
					DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_UCAST_KEY_INFO complete\n"));
#if 1
					hex_dump("WAPI UCAST KEY", pEntry->PTK, 64);
#endif
				}				
            }
			break;	
			
#if 0			
		case OID_802_11_MCAST_KEY_INFO:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_MCAST_KEY_INFO\n"));
			if (wrq->u.data.length != sizeof(WAPI_MCAST_KEY_STRUCT))
                Status  = -EINVAL;
            else
            {                												
				WAPI_MCAST_KEY_STRUCT   wapi_mkey;

				Status = copy_from_user(&wapi_mkey, wrq->u.data.pointer, wrq->u.data.length);
                if (Status == NDIS_STATUS_SUCCESS)
                {
                	NdisMoveMemory(pAd->ApCfg.MBSSID[pObj->ioctl_if].GTK, wapi_mkey.GTK, 32);
														
					DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_MCAST_KEY_INFO complete\n"));
				}				
            }
			break;		
#endif			
#endif /* WAPI_SUPPORT */

#ifdef DOT1X_SUPPORT
		case OID_802_DOT1X_PMKID_CACHE:
			RTMPIoctlAddPMKIDCache(pAd, wrq);
			break;

		case OID_802_DOT1X_RADIUS_DATA:
			RTMPIoctlRadiusData(pAd, wrq);
			break;

		case OID_802_DOT1X_WPA_KEY:
			RTMPIoctlAddWPAKey(pAd, wrq);
			break;

		case OID_802_DOT1X_STATIC_WEP_COPY:
			RTMPIoctlStaticWepCopy(pAd, wrq);
			break;

		case OID_802_DOT1X_IDLE_TIMEOUT:
			RTMPIoctlSetIdleTimeout(pAd, wrq);
			break;
#endif /* DOT1X_SUPPORT */

#ifdef HOSTAPD_SUPPORT
        case OID_802_11_AUTHENTICATION_MODE:
            if (wrq->u.data.length != sizeof(NDIS_802_11_AUTHENTICATION_MODE)) 
                Status  = -EINVAL;
            else
            {
                Status = copy_from_user(&AuthMode, wrq->u.data.pointer, wrq->u.data.length);
                if (AuthMode > Ndis802_11AuthModeMax)
                {
                    Status  = -EINVAL;
                    break;
                }
                else
                {
                    if (pAd->ApCfg.MBSSID[pObj->ioctl_if].AuthMode != AuthMode)
                    {
                        /* Config has changed */
                        pAd->bConfigChanged = TRUE;
                    }
                    pAd->ApCfg.MBSSID[pObj->ioctl_if].AuthMode = AuthMode;
                }
                pAd->ApCfg.MBSSID[pObj->ioctl_if].PortSecured = WPA_802_1X_PORT_NOT_SECURED;
                DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_AUTHENTICATION_MODE (=%d) \n",pAd->ApCfg.MBSSID[0].AuthMode));
            }
		APStop(pAd);
		APStartUp(pAd);
            break;

        case OID_802_11_WEP_STATUS:
            if (wrq->u.data.length != sizeof(NDIS_802_11_WEP_STATUS)) 
                Status  = -EINVAL;
            else
            {
                Status = copy_from_user(&WepStatus, wrq->u.data.pointer, wrq->u.data.length);
                /* Since TKIP, AES, WEP are all supported. It should not have any invalid setting */
                if (WepStatus <= Ndis802_11GroupWEP104Enabled)
                {
                    if (pAd->ApCfg.MBSSID[pObj->ioctl_if].WepStatus != WepStatus)
                    {
                        /* Config has changed */
                        pAd->bConfigChanged = TRUE;
                    }
                    pAd->ApCfg.MBSSID[pObj->ioctl_if].WepStatus     = WepStatus;

#if 1
                    if (pAd->ApCfg.MBSSID[pObj->ioctl_if].WepStatus == Ndis802_11TKIPAESMix){
						pAd->ApCfg.MBSSID[pObj->ioctl_if].GroupKeyWepStatus = Ndis802_11TKIPEnable;
					}
					else{
                    	pAd->ApCfg.MBSSID[pObj->ioctl_if].GroupKeyWepStatus = WepStatus;
                    }
#else
					pAd->ApCfg.MBSSID[pObj->ioctl_if].GroupKeyWepStatus = WepStatus;
#endif
                }
                else
                {
                    Status  = -EINVAL;
                    break;
                }
			APStop(pAd);
			APStartUp(pAd);
                	DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEP_STATUS (=%d)\n",WepStatus));
            }
            break;
			
        case OID_802_11_SSID:
            if (wrq->u.data.length != sizeof(NDIS_802_11_SSID))
                Status = -EINVAL;
            else
            {
            		RTMP_STRING *pSsidString = NULL;
                	Status = copy_from_user(&Ssid, wrq->u.data.pointer, wrq->u.data.length);

                	if (Ssid.SsidLength > MAX_LEN_OF_SSID)
                    		Status = -EINVAL;
                	else
                	{
                		if (Ssid.SsidLength == 0)
		    		{
                			Status = -EINVAL;
		    		}
				else
                    		{
	                		//pSsidString = (RTMP_STRING *)kmalloc(MAX_LEN_OF_SSID+1, MEM_ALLOC_FLAG);
					os_alloc_mem(pAd, (UCHAR **)&pSsidString, MAX_LEN_OF_SSID+1);
					if (pSsidString)
					{
						NdisZeroMemory(pSsidString, MAX_LEN_OF_SSID+1);
						NdisMoveMemory(pSsidString, Ssid.Ssid, Ssid.SsidLength);
						NdisZeroMemory((PCHAR)pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,MAX_LEN_OF_SSID);
						strcpy((PCHAR)pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,pSsidString);
						pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen=strlen(pSsidString);
						os_free_mem(pAd, pSsidString);
					}
					else
						Status = -ENOMEM;
                    }
                }
            }
            break;


	case HOSTAPD_OID_SET_802_1X:/*pure 1x is enabled. */
			Set_IEEE8021X_Proc(pAd,"1");
			break;

	case HOSTAPD_OID_SET_KEY:
			Status  = -EINVAL;
			pObj = (POS_COOKIE) pAd->OS_Cookie;
			UINT apidx = pObj->ioctl_if;
			if(wrq->u.data.length != sizeof(struct ieee80211req_key) || !wrq->u.data.pointer)
				break;
			Status = copy_from_user(&Key, wrq->u.data.pointer, wrq->u.data.length);
			pEntry = MacTableLookup(pAd, Key.ik_macaddr);


			if((Key.ik_type == CIPHER_WEP64) ||(Key.ik_type == CIPHER_WEP128))/*dynamic wep with 1x */
			{
				if (pEntry)/*pairwise key */
				{
					pEntry->PairwiseKey.KeyLen = Key.ik_keylen;
					NdisMoveMemory(pEntry->PairwiseKey.Key, Key.ik_keydata, Key.ik_keylen);
					pEntry->PairwiseKey.CipherAlg = Key.ik_type;
					KeyIdx=pAd->ApCfg.MBSSID[pEntry->apidx].DefaultKeyId;
                                  AsicAddPairwiseKeyEntry(
                                      pAd, 
                                      (UCHAR)pEntry->wcid, 
                                      &pEntry->PairwiseKey);

					RTMPAddWcidAttributeEntry(
						pAd, 
						pEntry->apidx, 
						KeyIdx, /* The value may be not zero */
						pEntry->PairwiseKey.CipherAlg, 
						pEntry);
				}
				else/*group key */
				{
					pMbss = &pAd->ApCfg.MBSSID[apidx];
					KeyIdx = Key.ik_keyix& 0x0fff;
					/* it is a shared key */
					if (KeyIdx < 4)
					{
						pAd->SharedKey[apidx][KeyIdx].KeyLen = (UCHAR) Key.ik_keylen;
						NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].Key, &Key.ik_keydata, Key.ik_keylen);
						if (Key.ik_keyix & 0x8000)
						{
							/* Default key for tx (shared key) */
							pMbss->DefaultKeyId = (UCHAR) KeyIdx;
						}
						/*pMbss->DefaultKeyId=1; */
						
						pAd->SharedKey[apidx][KeyIdx].CipherAlg = Key.ik_type;
						AsicAddSharedKeyEntry(
							pAd,
							apidx,
							KeyIdx,
						  	&pAd->SharedKey[apidx][KeyIdx]
						  	);

						RTMPAddWcidAttributeEntry(
							pAd, 
							apidx, 
							KeyIdx, 
							pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg,
							NULL);
					}
				}
			}
			else if (pEntry)
			{
				KeyIdx = Key.ik_keyix& 0x0fff;
				if (pEntry->WepStatus == Ndis802_11TKIPEnable)
				{
					pEntry->PairwiseKey.KeyLen = LEN_TK;
					NdisMoveMemory(&pEntry->PTK[OFFSET_OF_PTK_TK], Key.ik_keydata, Key.ik_keylen);
					NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[OFFSET_OF_PTK_TK], Key.ik_keylen);
				}

				if(pEntry->WepStatus == Ndis802_11AESEnable)
				{
					pEntry->PairwiseKey.KeyLen = LEN_TK;
					NdisMoveMemory(&pEntry->PTK[OFFSET_OF_PTK_TK], Key.ik_keydata, OFFSET_OF_PTK_TK);
					NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[OFFSET_OF_PTK_TK], Key.ik_keylen);
				}


    				pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
    				if (pEntry->WepStatus == Ndis802_11TKIPEnable)
        				pEntry->PairwiseKey.CipherAlg = CIPHER_TKIP;
    				else if (pEntry->WepStatus == Ndis802_11AESEnable)
        				pEntry->PairwiseKey.CipherAlg = CIPHER_AES;
				
				pEntry->PairwiseKey.CipherAlg = Key.ik_type;
				 
                            AsicAddPairwiseKeyEntry(
                                pAd, 
                                (UCHAR)pEntry->wcid, 
                                &pEntry->PairwiseKey);	

				RTMPSetWcidSecurityInfo(pAd, 
					pEntry->apidx, 
					(UINT8)KeyIdx, 										 
					pEntry->PairwiseKey.CipherAlg, 
					pEntry->wcid,
					PAIRWISEKEYTABLE);
			} 
			else
			{
				pMbss = &pAd->ApCfg.MBSSID[apidx];
				KeyIdx = Key.ik_keyix& 0x0fff;
				
				/*if (Key.ik_keyix & 0x8000) */
				{
					pMbss->DefaultKeyId = (UCHAR) KeyIdx;
				}

				if (pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11TKIPEnable)
				{
					pAd->SharedKey[apidx][pMbss->DefaultKeyId].KeyLen= LEN_TK;
					NdisMoveMemory(pAd->SharedKey[apidx][pMbss->DefaultKeyId].Key, Key.ik_keydata, 16);
					NdisMoveMemory(pAd->SharedKey[apidx][pMbss->DefaultKeyId].RxMic, (Key.ik_keydata+16+8), 8);
					NdisMoveMemory(pAd->SharedKey[apidx][pMbss->DefaultKeyId].TxMic, (Key.ik_keydata+16), 8);
				}

				if(pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11AESEnable)
				{
					pAd->SharedKey[apidx][pMbss->DefaultKeyId].KeyLen= LEN_TK;	
					NdisMoveMemory(pAd->SharedKey[apidx][pMbss->DefaultKeyId].Key, Key.ik_keydata, 16);
					NdisMoveMemory(pAd->SharedKey[apidx][pMbss->DefaultKeyId].RxMic, (Key.ik_keydata+16+8), 8);
					NdisMoveMemory(pAd->SharedKey[apidx][pMbss->DefaultKeyId].TxMic, (Key.ik_keydata+16), 8);
				}

    				pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg  = CIPHER_NONE;
    				if (pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11TKIPEnable)
        				pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg = CIPHER_TKIP;
    				else if (pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11AESEnable)
        				pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg = CIPHER_AES;

				hex_dump("Key.ik_keydata,", (unsigned char*) Key.ik_keydata, 32);

				AsicAddSharedKeyEntry(
					pAd,
					apidx,
					pMbss->DefaultKeyId,
					&pAd->SharedKey[apidx][pMbss->DefaultKeyId]
					);
				GET_GroupKey_WCID(pAd, Wcid, apidx);

				RTMPSetWcidSecurityInfo(pAd, apidx,(UINT8)KeyIdx, 
									pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg, Wcid, SHAREDKEYTABLE);
				
				/*RTMPAddWcidAttributeEntry(
				pAd, 
				apidx,
				pMbss->DefaultKeyId, 
				pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg,
				NULL);*/
			}
			break;


		case HOSTAPD_OID_DEL_KEY:

			Status  = -EINVAL;
			if(wrq->u.data.length != sizeof(struct ieee80211req_del_key) || !wrq->u.data.pointer)
				break;
			Status = copy_from_user(&delkey, wrq->u.data.pointer, wrq->u.data.length);
			pEntry = MacTableLookup(pAd, delkey.idk_macaddr);
			if (pEntry){
				/* clear the previous Pairwise key table */
				if(pEntry->wcid != 0)
				{
					NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));
					AsicRemovePairwiseKeyEntry(pAd,(UCHAR)pEntry->wcid);
				}
			}
			else if((delkey.idk_macaddr == NULL) && (delkey.idk_keyix < 4))
				/* remove group key */
				AsicRemoveSharedKeyEntry(pAd, pEntry->apidx, delkey.idk_keyix);
			break;

		case HOSTAPD_OID_SET_STA_AUTHORIZED:/*for portsecured flag. */

			if (wrq->u.data.length != sizeof(struct ieee80211req_mlme))
			{
				Status  = -EINVAL;
			}
			else
			{
				Status = copy_from_user(&mlme, wrq->u.data.pointer, wrq->u.data.length);
				pEntry = MacTableLookup(pAd, mlme.im_macaddr);
				if (!pEntry){
					Status = -EINVAL;
				}
				else
				{
					switch (mlme.im_op)
					{
						case IEEE80211_MLME_AUTHORIZE:
							pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
							pEntry->WpaState = AS_PTKINITDONE;/*wpa state machine is not in use. */
							/*pAd->StaCfg.PortSecured= WPA_802_1X_PORT_SECURED; */
							pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
							break;
						case IEEE80211_MLME_UNAUTHORIZE:
							pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
							pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
						break;
						default:
							Status = -EINVAL;
					}
				}
			}
			break;

		case HOSTAPD_OID_STATIC_WEP_COPY:
			
			if (wrq->u.data.length != sizeof(struct ieee80211req_mlme))
			{
				Status  = -EINVAL;
			}
			else
			{
				Status = copy_from_user(&mlme, wrq->u.data.pointer, wrq->u.data.length);
				pEntry = MacTableLookup(pAd, mlme.im_macaddr);
				if (!pEntry)
				{
					Status = -EINVAL;
				}
				else
				{
					/*Status  = -EINVAL; */
					if (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X != TRUE) 
						break;
					if (pEntry->WepStatus != Ndis802_11WEPEnabled) 
						break;

					apidx = pObj->ioctl_if;
					pMbss = &pAd->ApCfg.MBSSID[apidx];
					KeyIdx=pMbss->DefaultKeyId;
					pEntry->AuthMode=pAd->ApCfg.MBSSID[apidx].AuthMode;
					pEntry->PairwiseKey.KeyLen = pAd->SharedKey[apidx][KeyIdx].KeyLen;
					NdisMoveMemory(pEntry->PairwiseKey.Key, pAd->SharedKey[apidx][KeyIdx].Key, pAd->SharedKey[apidx][KeyIdx].KeyLen);
					pEntry->PairwiseKey.CipherAlg = pAd->SharedKey[apidx][KeyIdx].CipherAlg;
					
                  			AsicAddPairwiseKeyEntry(
							pAd, 
							(UCHAR)pEntry->wcid, 
							&pEntry->PairwiseKey);
							
					RTMPAddWcidAttributeEntry(
							pAd, 
							pEntry->apidx, 
							KeyIdx, /* The value may be not zero */
							pEntry->PairwiseKey.CipherAlg, 
							pEntry);
				}
			}
			break;

		case HOSTAPD_OID_SET_STA_DEAUTH:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::HOSTAPD_OID_SET_STA_DEAUTH\n"));
			MLME_DEAUTH_REQ_STRUCT  *pInfo;
			MLME_QUEUE_ELEM 		*Elem; /* = (MLME_QUEUE_ELEM *) kmalloc(sizeof(MLME_QUEUE_ELEM), MEM_ALLOC_FLAG); */
			os_alloc_mem(NULL, (UCHAR **)&Elem, sizeof(MLME_QUEUE_ELEM));
			if(Elem)
			{
				pInfo = (MLME_DEAUTH_REQ_STRUCT *) Elem->Msg;

				if (wrq->u.data.length != sizeof(struct ieee80211req_mlme))
				{
					Status  = -EINVAL;
				}
				else
				{
					Status = copy_from_user(&mlme, wrq->u.data.pointer, wrq->u.data.length);
					NdisMoveMemory(pInfo->Addr, mlme.im_macaddr, MAC_ADDR_LEN);
					if ((pEntry = MacTableLookup(pAd, pInfo->Addr)) != NULL)
					{
						pInfo->Reason = mlme.im_reason;
						Elem->Wcid = pEntry->wcid;
						MlmeEnqueue(pAd, AP_AUTH_STATE_MACHINE, APMT2_MLME_DEAUTH_REQ, sizeof(MLME_DEAUTH_REQ_STRUCT), Elem,0);
					}
				}
				os_free_mem(NULL, Elem);
			}
			break;

		case HOSTAPD_OID_SET_STA_DISASSOC:/*hostapd request to disassoc the station. */
			DBGPRINT(RT_DEBUG_TRACE, ("Set::HOSTAPD_OID_SET_STA_DISASSOC\n"));
			MLME_DISASSOC_REQ_STRUCT DisassocReq;
			if (wrq->u.data.length != sizeof(struct ieee80211req_mlme))
			{
				Status  = -EINVAL;
			}
			else
			{
				Status = copy_from_user(&mlme, wrq->u.data.pointer, wrq->u.data.length);
				NdisMoveMemory(DisassocReq.Addr, mlme.im_macaddr, MAC_ADDR_LEN);
				DisassocReq.Reason = mlme.im_reason;
				MlmeEnqueue(pAd, AP_ASSOC_STATE_MACHINE, APMT2_MLME_DISASSOC_REQ, sizeof(MLME_DISASSOC_REQ_STRUCT), &DisassocReq,0);
			}
			break;

		case OID_HOSTAPD_SUPPORT:/*notify the driver to support hostapd. */
			apidx = pObj->ioctl_if;
			
			if (wrq->u.data.length != sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				BOOLEAN hostapd_enable;
				int v;

				Status = copy_from_user(&hostapd_enable, wrq->u.data.pointer, wrq->u.data.length);
				pAd->ApCfg.MBSSID[apidx].Hostapd = hostapd_enable;
				BSS_STRUCT *pMBSSStruct;

				for (v=0;v<MAX_MBSSID_NUM(pAd);v++)
				{
				       if (pAd->ApCfg.MBSSID[v].Hostapd == Hostapd_EXT)
					   	printk("ApCfg->MBSSID[%d].Hostapd == TURE\n",v);
					else
					   	printk("ApCfg->MBSSID[%d].Hostapd == FALSE\n",v);
					pMBSSStruct = &pAd->ApCfg.MBSSID[v];
					pMBSSStruct->WPAREKEY.ReKeyInterval = 0;
					pMBSSStruct->WPAREKEY.ReKeyMethod = DISABLE_REKEY;
				}
			}
			break;

		case HOSTAPD_OID_COUNTERMEASURES:/*report txtsc to hostapd. */
			
			if (wrq->u.data.length != sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				BOOLEAN countermeasures_enable;
				Status = copy_from_user(&countermeasures_enable, wrq->u.data.pointer, wrq->u.data.length);

				if(countermeasures_enable)
				{

    						{
        						DBGPRINT(RT_DEBUG_ERROR, ("Receive CM Attack Twice within 60 seconds ====>>> \n"));
        
							/* send wireless event - for counter measures */
							pAd->ApCfg.CMTimerRunning = FALSE;

						        for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
						        {
						        	MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];
						            /* happened twice within 60 sec,  AP SENDS disaccociate all associated STAs.  All STA's transition to State 2 */
						            if (IS_ENTRY_CLIENT(pEntry))
						            {
						                MlmeDeAuthAction(pAd, &pAd->MacTab.Content[i], REASON_MIC_FAILURE,FALSE);
						            }
						        }
	        
						        /* Further,  ban all Class 3 DATA transportation for  a period 0f 60 sec */
						        /* disallow new association , too */
						        pAd->ApCfg.BANClass3Data = TRUE;

						    }


				}
				else
				{        
    					    pAd->ApCfg.BANClass3Data = FALSE;
				}
			}
			break;

	case HOSTAPD_OID_SET_WPS_BEACON_IE:/*pure 1x is enabled. */
				apidx = pObj->ioctl_if;
				DBGPRINT(RT_DEBUG_TRACE,("HOSTAPD_OID_SET_WPS_BEACON_IE\n"));
				if (wrq->u.data.length != sizeof(WSC_LV_INFO))
				{
					Status  = -EINVAL;
				}
				else
				{	
					pAd->ApCfg.MBSSID[apidx].HostapdWPS = TRUE;
					BSS_STRUCT *pMBSSStruct;
					NdisZeroMemory(&WscIEBeacon,sizeof(WSC_LV_INFO));
					Status = copy_from_user(&WscIEBeacon, wrq->u.data.pointer, wrq->u.data.length);
					pMBSSStruct = &pAd->ApCfg.MBSSID[apidx];
					NdisMoveMemory(pMBSSStruct->WscIEBeacon.Value,WscIEBeacon.Value, WscIEBeacon.ValueLen);
					pMBSSStruct->WscIEBeacon.ValueLen=WscIEBeacon.ValueLen;
					APUpdateBeaconFrame(pAd, apidx);							
				}

			break;

	case HOSTAPD_OID_SET_WPS_PROBE_RESP_IE:/*pure 1x is enabled. */

				apidx = pObj->ioctl_if;
				DBGPRINT(RT_DEBUG_TRACE,("HOSTAPD_OID_SET_WPS_PROBE_RESP_IE\n"));
				if (wrq->u.data.length != sizeof(WSC_LV_INFO))
				{
					DBGPRINT(RT_DEBUG_TRACE,("HOSTAPD_OID_SET_WPS_PROBE_RESP_IE failed\n"));
					Status  = -EINVAL;
				}
				else
				{	
					pAd->ApCfg.MBSSID[apidx].HostapdWPS = TRUE;
					BSS_STRUCT *pMBSSStruct;
					NdisZeroMemory(&WscIEProbeResp,sizeof(WSC_LV_INFO));
					Status = copy_from_user(&WscIEProbeResp, wrq->u.data.pointer, wrq->u.data.length);
					pMBSSStruct = &pAd->ApCfg.MBSSID[apidx];
					NdisMoveMemory(pMBSSStruct->WscIEProbeResp.Value,WscIEProbeResp.Value, WscIEProbeResp.ValueLen);
					pMBSSStruct->WscIEProbeResp.ValueLen=WscIEProbeResp.ValueLen;
					APUpdateBeaconFrame(pAd, apidx);
				}
			break;
#endif /*HOSTAPD_SUPPORT*/


   		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::unknown IOCTL's subcmd = 0x%08x\n", cmd));
			Status = -EOPNOTSUPP;
			break;
    }
	
#ifdef MESH_SUPPORT
	/* important parameter is changed, restart mesh */
	if (RestartMeshIsRequired) 
	{					
		MeshDown(pAd, TRUE);
		MeshUp(pAd);		
	}
#endif /* MESH_SUPPORT */

	return Status;
}


INT RTMPAPQueryInformation(
	IN	PRTMP_ADAPTER       pAd,
	IN	OUT	RTMP_IOCTL_INPUT_STRUCT    *rq,
	IN	INT                 cmd)
{
	RTMP_IOCTL_INPUT_STRUCT	*wrq = (RTMP_IOCTL_INPUT_STRUCT *) rq;
    INT	Status = NDIS_STATUS_SUCCESS;
    POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
    RTMP_STRING driverVersion[8];

#if defined(DBG) || defined(WSC_AP_SUPPORT)
	UCHAR	apidx = pObj->ioctl_if;
#endif
#ifdef WSC_AP_SUPPORT
	UINT	WscPinCode = 0;
	PWSC_PROFILE	pProfile;
	PWSC_CTRL		pWscControl;
#endif /* WSC_AP_SUPPORT */

#ifdef SNMP_SUPPORT	
	/*for snmp */
	ULONG ulInfo;
	DefaultKeyIdxValue			*pKeyIdxValue;
	INT							valueLen;
	TX_RTY_CFG_STRUC			tx_rty_cfg;
	ULONG						ShortRetryLimit, LongRetryLimit;
	UCHAR						snmp_tmp[64];
#endif /* SNMP_SUPPORT */

#ifdef HOSTAPD_SUPPORT
	struct default_group_key			group_key;
	struct ieee80211req_key			ik;
	unsigned char						*p;
	MAC_TABLE_ENTRY				*pEntry=(MAC_TABLE_ENTRY *)NULL;
	struct ieee80211req_wpaie			wpaie;
	BSS_STRUCT *pMbss ;
#endif /*HOSTAPD_SUPPORT*/

	NDIS_802_11_STATISTICS	*pStatistics;
	ULONG ulInfo;

#ifdef EASY_CONFIG_SETUP
	PUCHAR						pStaMacAddr = NULL;
#endif /* EASY_CONFIG_SETUP */

#ifdef P2P_SUPPORT
	/*RT_P2P_UI_TABLE UI_table;*/
	PRT_P2P_UI_TABLE pUI_table;
	PRT_P2P_TABLE			pP2pTable;
	PRT_P2P_CLIENT_ENTRY   pPAdCli, pUICli;
	PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;
	PRT_P2P_PERSISTENT_ENTRY pP2PPersistent = NULL;
	UCHAR tmp[24];
	UCHAR i;
#endif /* P2P_SUPPORT */


    switch(cmd)
    {
#ifdef P2P_SUPPORT
		case OID_802_11_P2P_Connected_MAC:			
			wrq->u.data.length = MAC_ADDR_LEN;
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.ConnectingMAC, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_Connected_MAC\n"));
			break;
			
		case OID_802_11_P2P_MODE:			
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.Rule, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%d, Rule=%d)\n", sizeof(char), pAd->P2pCfg.Rule));
			break;

		case OID_802_11_P2P_DEVICE_NAME:			
			wrq->u.data.length = pAd->P2pCfg.DeviceNameLen;
			Status = copy_to_user(wrq->u.data.pointer, pAd->P2pCfg.DeviceName, pAd->P2pCfg.DeviceNameLen);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_DEVICE_NAME (Len=%ld, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen, pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_LISTEN_CHANNEL:			
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.ListenChannel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_LISTEN_CHANNEL (Len=%d, Listen_Ch=%d)\n", sizeof(char),pAd->P2pCfg.ListenChannel));
			break;

		case OID_802_11_P2P_OPERATION_CHANNEL:			
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.GroupOpChannel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_OPERATION_CHANNEL (Len=%d, Op_Ch=%d)\n", sizeof(char),pAd->P2pCfg.GroupOpChannel));
			break;


		case OID_802_11_P2P_DEV_ADDR:			
			wrq->u.data.length = 6;
			Status = copy_to_user(wrq->u.data.pointer, pAd->P2pCfg.CurrentAddress, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_DEV_ADDR (Len=%d, Addr=%02x:%02x:%02x:%02x:%02x:%02x)\n", wrq->u.data.length, PRINT_MAC(pAd->P2pCfg.CurrentAddress)));
			break;

		case OID_802_11_P2P_CTRL_STATUS:			
			wrq->u.data.length = 24;
			pP2PCtrl = &pAd->P2pCfg;
			NdisZeroMemory(tmp, 24);
			sprintf(tmp,"%s",decodeCtrlState(pP2PCtrl->CtrlCurrentState));
			Status = copy_to_user(wrq->u.data.pointer, tmp, 24);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%ld, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_DISC_STATUS:			
			wrq->u.data.length = 24;
			pP2PCtrl = &pAd->P2pCfg;
			NdisZeroMemory(tmp, 24);
			sprintf(tmp,"%s",decodeDiscoveryState(pP2PCtrl->DiscCurrentState));
			Status = copy_to_user(wrq->u.data.pointer, tmp, 24);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%ld, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_GOFORM_STATUS:			
			wrq->u.data.length = 24;
			pP2PCtrl = &pAd->P2pCfg;
			NdisZeroMemory(tmp, 24);
			sprintf(tmp,"%s",decodeGroupFormationState(pP2PCtrl->GoFormCurrentState));
			Status = copy_to_user(wrq->u.data.pointer, tmp, 24);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%ld, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;
			
		case OID_802_11_P2P_SCAN_LIST:			

			os_alloc_mem(NULL, (UCHAR **)&pUI_table, sizeof(RT_P2P_UI_TABLE));
			pP2pTable = &pAd->P2pTable;
			if (pUI_table)
			{
				NdisZeroMemory(pUI_table, sizeof(RT_P2P_UI_TABLE));
				pUI_table->ClientNumber = pAd->P2pTable.ClientNumber;
				for (i=0; i < pAd->P2pTable.ClientNumber; i++)
				{
					pPAdCli = &pP2pTable->Client[i];
					pUICli = &pUI_table->Client[i];
	 				NdisMoveMemory(pUICli, pPAdCli, sizeof(RT_P2P_CLIENT_ENTRY));
				}
				wrq->u.data.length = sizeof(RT_P2P_UI_TABLE);
				Status = copy_to_user(wrq->u.data.pointer, pUI_table, sizeof(RT_P2P_UI_TABLE));
				os_free_mem(NULL, pUI_table);
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_SCAN_LIST (Len=%d)\n", sizeof(RT_P2P_UI_TABLE)));
			break;
			
		case OID_P2P_WSC_PIN_CODE:
			wrq->u.data.length = sizeof(UINT);
			WscPinCode = pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode;
			
			if (copy_to_user(wrq->u.data.pointer, &WscPinCode, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_PIN_CODE (=%d)\n", WscPinCode));
			break;

		/*
		 * Query WFD P2P connection status
		 * Value:
		 * 	0: P2P connecting or no link
		 * 	1: P2P Go
		 * 	2: P2P Client
		 * 	5: WFD session not avalible
		*/
		case OID_802_11_P2P_CONNECT_STATUS:
		{
			UCHAR stat = CONNECTING_OR_NO_LINK;

			wrq->u.data.length = sizeof(UCHAR);
			if (P2P_GO_ON(pAd))
			{
				UINT32 i;
				MAC_TABLE_ENTRY *pEntry;

				pWscControl = &pAd->ApCfg.MBSSID[0].WscControl;
				for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
				{
					pEntry = &pAd->MacTab.Content[i];
					if (IS_P2P_GO_ENTRY(pEntry) && (pEntry->WpaState == AS_PTKINITDONE))
					{
						stat = P2P_GO;
						break;
					}
				}
			}
			else if (P2P_CLI_ON(pAd))
			{
				UINT32 i;
				MAC_TABLE_ENTRY *pEntry;
				
				pWscControl = &pAd->ApCfg.ApCliTab[0].WscControl;
				for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
				{
					pEntry = &pAd->MacTab.Content[i];
					if (IS_P2P_CLI_ENTRY(pEntry) && (pEntry->WpaState == AS_PTKINITDONE))
					{
						stat = P2P_CLIENT;
						break;
					}
				}
			}
#ifdef WFD_SUPPORT
			if ((pAd->StaCfg.WfdCfg.bWfdEnable) &&
				(pAd->StaCfg.WfdCfg.PeerSessionAvail == WFD_SESSION_NOT_AVAILABLE))
			{
				stat = WFD_SESSION_UNAVALIBLE;
				pAd->StaCfg.WfdCfg.PeerSessionAvail = WFD_SESSION_AVAILABLE;
			}
#endif /* WFD_SUPPORT */

			if (copy_to_user(wrq->u.data.pointer, &stat, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_CONNECT_STATUS (=%d)\n", stat));
		}
			break;

		case OID_802_11_P2P_PEER_GROUP_ID:
		{
			UCHAR GroupId[128] = {0};
			wrq->u.data.length = sizeof(GroupId);


			if (P2P_GO_ON(pAd))
			{
				printk("OID_802_11_P2P_PEER_GROUP_ID bssid: %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pP2PCtrl->CurrentAddress));
				printk("OID_802_11_P2P_PEER_GROUP_ID ssid: %s\n", pAd->ApCfg.MBSSID[MAIN_MBSSID].Ssid);
				sprintf(GroupId, "%02x:%02x:%02x:%02x:%02x:%02x %s", pP2PCtrl->CurrentAddress[0], pP2PCtrl->CurrentAddress[1], pP2PCtrl->CurrentAddress[2],
					pP2PCtrl->CurrentAddress[3], pP2PCtrl->CurrentAddress[4], pP2PCtrl->CurrentAddress[5], pAd->ApCfg.MBSSID[MAIN_MBSSID].Ssid);
			}
			if (P2P_CLI_ON(pAd))
			{
				printk("OID_802_11_P2P_PEER_GROUP_ID bssid: %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pP2PCtrl->Bssid));
				printk("OID_802_11_P2P_PEER_GROUP_ID ssid: %s\n", pP2PCtrl->SSID);
				sprintf(GroupId, "%02x:%02x:%02x:%02x:%02x:%02x %s", pP2PCtrl->Bssid[0], pP2PCtrl->Bssid[1], pP2PCtrl->Bssid[2],
					pP2PCtrl->Bssid[3], pP2PCtrl->Bssid[4], pP2PCtrl->Bssid[5], pP2PCtrl->SSID);
			}
			DBGPRINT(RT_DEBUG_TRACE, ("GROUP ID :: %s\n", GroupId));
			if (copy_to_user(wrq->u.data.pointer, &GroupId, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_PEER_GROUP_ID (=%s)\n", GroupId));
		}
			break;
		case OID_802_11_P2P_GO_INT:
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.GoIntentIdx, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_GO_INT (GoIntentIdx=%d)\n", pAd->P2pCfg.GoIntentIdx));
			break;

		case OID_802_11_P2P_SSID:
			if (wrq->u.data.length >= pAd->P2pCfg.SSIDLen)
			{
				wrq->u.data.length = pAd->P2pCfg.SSIDLen;
				Status = copy_to_user(wrq->u.data.pointer, pAd->P2pCfg.SSID, wrq->u.data.length);

			}
			else
			{
				Status = -EFAULT;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_SSID (SSIDLen=%d)\n", pAd->P2pCfg.SSIDLen));
			break;

		case OID_802_11_P2P_PASSPHRASE:
			{
				BSS_STRUCT *pMBSSStruct;
				pMBSSStruct = &pAd->ApCfg.MBSSID[0];
				wrq->u.data.length = pMBSSStruct->WscControl.WpaPskLen;
				Status = copy_to_user(wrq->u.data.pointer, pMBSSStruct->WscControl.WpaPsk, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_PASSPHRASE(%s)\n", pMBSSStruct->WscControl.WpaPsk));
			}
			break;

		case OID_802_11_P2P_ASSOCIATE_TAB:
#ifdef HE_BD_SUPPORT
			{
				P2P_STA_ASSOC_LIST AssocList;
				INT			p2pIdx;

				UCHAR p2pindex = P2P_NOT_FOUND;
				RTMPZeroMemory(&AssocList, sizeof(P2P_STA_ASSOC_LIST));
				if (P2P_CLI_ON(pAd))
				{
					NdisMoveMemory(&AssocList, &pP2PCtrl->AssocList, sizeof(P2P_STA_ASSOC_LIST));
					AssocList.maclist_count = 1;
					AssocList.is_p2p[0] = 1;

					AssocList.rssi[0] = (pAd->ApCfg.RssiSample.AvgRssi0 - pAd->BbpRssiToDbmDelta);
#ifdef WFD_SUPPORT
					p2pIdx = P2pGroupTabSearch(pAd, AssocList.maclist[0].octet);
					AssocList.is_wfd[0] = pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.bWfdClient;

					if (pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.bWfdClient)
					{
						AssocList.rtsp_port[0] = pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.rtsp_port;
					}

#endif /* CONFIG_WFD */
					DBGPRINT(RT_DEBUG_TRACE, ("Rule = %s.  Bssid = %0x2:%02x:%0x2:%02x:%0x2:%02x\n", 
								decodeMyRule(pP2PCtrl->Rule), PRINT_MAC(pP2PCtrl->Bssid)));

					DBGPRINT(RT_DEBUG_TRACE,("addr = %02x:%02x:%02x:%02x:%02x:%02x.\n",
							AssocList.maclist[0].octet[0], AssocList.maclist[0].octet[1],
							AssocList.maclist[0].octet[2], AssocList.maclist[0].octet[3],
							AssocList.maclist[0].octet[4], AssocList.maclist[0].octet[5]));
					DBGPRINT(RT_DEBUG_TRACE,("name = %s.\n", AssocList.device_name[0].ssid));
					DBGPRINT(RT_DEBUG_TRACE,("type = %d.\n", AssocList.device_type[0].dev_type));

				}
				else if (P2P_GO_ON(pAd))
				{
					UINT32 i, p2pEntryCnt=0;
					MAC_TABLE_ENTRY *pEntry;
					PWSC_CTRL			pWscControl = &pAd->ApCfg.MBSSID[0].WscControl;
					/* Alan_LG */
					UCHAR           AllZero[MAC_ADDR_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

					NdisMoveMemory(&AssocList, &pP2PCtrl->AssocList, sizeof(P2P_STA_ASSOC_LIST));
					/* Alan_LG */
					if ( pP2PCtrl->AssocList.maclist_count != 0 )
					{
						for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
						{
							pEntry = &pAd->MacTab.Content[i];
							if (IS_P2P_GO_ENTRY(pEntry))
							{
								if ( (!NdisEqualMemory(AllZero, pP2PCtrl->AssocList.maclist[p2pEntryCnt].octet, MAC_ADDR_LEN)) &&
									(pP2PCtrl->AssocList.device_name[p2pEntryCnt].ssid[0] != 0 )) 
								{
									//NdisMoveMemory(&AssocList.maclist[p2pEntryCnt].octet , &pEntry->Addr, MAC_ADDR_LEN);
									AssocList.rssi[p2pEntryCnt] = pEntry->RssiSample.AvgRssi0;
									AssocList.is_p2p[p2pEntryCnt] = 1;
#ifdef WFD_SUPPORT									
									AssocList.is_wfd[p2pEntryCnt] = pEntry->bWfdClient;
									AssocList.is_wfd[p2pEntryCnt] = pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.bWfdClient;
									if (pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.bWfdClient)
									{
										
										AssocList.rtsp_port[p2pEntryCnt] = pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.rtsp_port;
									}

#endif /* CONFIG_WFD */
								DBGPRINT(RT_DEBUG_TRACE,("name = %s.\n", AssocList.device_name[p2pEntryCnt].ssid));
								DBGPRINT(RT_DEBUG_TRACE,("type = %s.\n", AssocList.device_type[p2pEntryCnt].dev_type));
								DBGPRINT(RT_DEBUG_TRACE,("1. %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(AssocList.maclist[p2pEntryCnt].octet)));
									p2pEntryCnt++;
								} 
								DBGPRINT(RT_DEBUG_TRACE,("1. %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(pEntry->Addr)));

							}
						}
					}
					else
					{
						for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
						{
							pEntry = &pAd->MacTab.Content[i];
							if ((IS_ENTRY_CLIENT(pEntry) || IS_ENTRY_APCLI(pEntry))
								&& (pEntry->Sst == SST_ASSOC))
							{
								if (p2pEntryCnt < 3 )
								{
									DBGPRINT(RT_DEBUG_TRACE,("2. %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(pEntry->Addr)));
									NdisMoveMemory(&AssocList.maclist[p2pEntryCnt].octet , &pEntry->Addr, MAC_ADDR_LEN);
									AssocList.is_p2p[p2pEntryCnt] = TRUE;
									AssocList.rssi[p2pEntryCnt] = pEntry->RssiSample.AvgRssi0;
#ifdef WFD_SUPPORT									

									AssocList.is_wfd[p2pEntryCnt] = pEntry->bWfdClient;
									p2pIdx = P2pGroupTabSearch(pAd, AssocList.maclist[p2pEntryCnt].octet);
									if (pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.bWfdClient)
									{
										AssocList.rtsp_port[p2pEntryCnt] = pAd->P2pTable.Client[p2pIdx].WfdEntryInfo.rtsp_port;
									}

#endif /* CONFIG_WFD */
									p2pEntryCnt++;
								}
							}
						}
					}
					AssocList.maclist_count = p2pEntryCnt;
				}


				wrq->u.data.length = sizeof(AssocList);
				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_ASSOCIATE_TAB(Len = %d).\n", sizeof(AssocList)));
				if (copy_to_user(wrq->u.data.pointer, &AssocList, wrq->u.data.length))
					Status = -EFAULT;
			}


#endif /* HE_BD_SUPPORT */
			break;

		case OID_802_11_P2P_PERSISTENT_TABLE:
			if (wrq->u.data.length != (sizeof(RT_P2P_PERSISTENT_ENTRY)*MAX_P2P_TABLE_SIZE))
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ERROR: OID_802_11_P2P_PERSISTENT_TABLE size not match (sent=%d, required=%d)\n",
				wrq->u.data.length, sizeof(RT_P2P_PERSISTENT_ENTRY)*MAX_P2P_TABLE_SIZE));
				Status = -EFAULT;
				break;
			}
			os_alloc_mem(NULL, (UCHAR **)&pP2PPersistent, sizeof(RT_P2P_PERSISTENT_ENTRY)*MAX_P2P_TABLE_SIZE);
			if (pP2PPersistent)
			{
				UINT idx = 0;
				PUCHAR pPtr;
				PRT_P2P_TABLE P2pTab = &pAd->P2pTable;
				
				wrq->u.data.length = sizeof(RT_P2P_PERSISTENT_ENTRY)*MAX_P2P_TABLE_SIZE;
				NdisZeroMemory(pP2PPersistent, wrq->u.data.length);
				pPtr = pP2PPersistent;
				for (idx = 0; idx < MAX_P2P_TABLE_SIZE; idx++)
				{
					NdisMoveMemory(pPtr, &P2pTab->PerstEntry[idx], sizeof(RT_P2P_PERSISTENT_ENTRY));
					pPtr += sizeof(RT_P2P_PERSISTENT_ENTRY);
				}
				if (copy_to_user(wrq->u.data.pointer, pP2PPersistent, wrq->u.data.length))
				{
					Status = -EFAULT;
				}
				os_free_mem(NULL, pP2PPersistent);
				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_PERSISTENT_TABLE (=%d)\n", wrq->u.data.length));
			}
			break;
#ifdef WFD_SUPPORT
		case OID_802_11_WFD_PEER_RTSP_PORT:
			{
				MAC_TABLE_ENTRY *pEntry;
				USHORT RtspPort = WFD_RTSP_DEFAULT_PORT;
				UCHAR P2pIdx = P2P_NOT_FOUND;
				wrq->u.data.length = sizeof(RtspPort);

#ifdef DOT11Z_TDLS_SUPPORT
				i = -1;
				if (pAd->StaCfg.TdlsInfo.bTDLSCapable && pAd->StaCfg.WfdCfg.PC == WFD_PC_TDLS)
				{
					PRT_802_11_TDLS pTDLS = NULL;
					
					DBGPRINT(RT_DEBUG_TRACE, ("%s - TDLS peer rtsp port get...\n", __FUNCTION__));
					for (i = MAX_NUM_OF_TDLS_ENTRY - 1; i >= 0; i--)
					{
						if ((pAd->StaCfg.TdlsInfo.TDLSEntry[i].Valid) && (pAd->StaCfg.TdlsInfo.TDLSEntry[i].Status == TDLS_MODE_CONNECTED))
						{
							pTDLS = &pAd->StaCfg.TdlsInfo.TDLSEntry[i];
							RtspPort = pTDLS->WfdEntryInfo.rtsp_port;
							DBGPRINT(RT_DEBUG_TRACE, ("TDLS Entry[%d][%02x:%02x:%02x:%02x:%02x:%02x]\n", i, PRINT_MAC(pTDLS->MacAddr)));
							DBGPRINT(RT_DEBUG_TRACE, ("RTSP_PORT = %d.\n", pTDLS->WfdEntryInfo.rtsp_port));
							break;
						}
					}
				
					if ((RtspPort == 0) && (pTDLS != NULL))
					{
						DBGPRINT(RT_DEBUG_ERROR, ("TDLS peer rtsp port is zero, search P2P Entry!\n", RtspPort));
				
						P2pIdx = P2pGroupTabSearch(pAd, pTDLS->MacAddr);
						if (P2pIdx != P2P_NOT_FOUND)
						{
							RtspPort = pAd->P2pTable.Client[P2pIdx].WfdEntryInfo.rtsp_port; 
							DBGPRINT(RT_DEBUG_TRACE, ("P2P Entry[%d][%02x:%02x:%02x:%02x:%02x:%02x]\n", P2pIdx, PRINT_MAC(pTDLS->MacAddr)));
							DBGPRINT(RT_DEBUG_TRACE, ("RTSP_PORT = %d.\n", pAd->P2pTable.Client[P2pIdx].WfdEntryInfo.rtsp_port));
							if (RtspPort == 0)
								RtspPort = WFD_RTSP_DEFAULT_PORT;
						}
						else
						{
							RtspPort = WFD_RTSP_DEFAULT_PORT;
							DBGPRINT(RT_DEBUG_ERROR, ("OID_802_11_P2P_PEER_RTSP_PORT::P2P not found, use default RTSP port\n"));
						}
					}
				}
				if (i < 0)
#endif /* DOT11Z_TDLS_SUPPORT */
				{
					DBGPRINT(RT_DEBUG_TRACE, ("%s - P2P peer rtsp port get...\n", __FUNCTION__));
					if (P2P_GO_ON(pAd) || P2P_CLI_ON(pAd))
					{
						for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
						{
							pEntry = &pAd->MacTab.Content[i];
							if (IS_P2P_GO_ENTRY(pEntry) || IS_P2P_CLI_ENTRY(pEntry))
							{
								P2pIdx = P2pGroupTabSearch(pAd, pEntry->Addr);
								DBGPRINT(RT_DEBUG_TRACE, ("P2P Entry[%d][%02x:%02x:%02x:%02x:%02x:%02x]\n", pEntry->P2pInfo.p2pIndex, PRINT_MAC(pEntry->Addr)));
								DBGPRINT(RT_DEBUG_TRACE, ("RTSP_PORT = %d.\n", pAd->P2pTable.Client[pEntry->P2pInfo.p2pIndex].WfdEntryInfo.rtsp_port));
								if (P2pIdx != P2P_NOT_FOUND)
									RtspPort = pAd->P2pTable.Client[P2pIdx].WfdEntryInfo.rtsp_port;	
								else
								{
									RtspPort = WFD_RTSP_DEFAULT_PORT;
									DBGPRINT(RT_DEBUG_ERROR, ("OID_802_11_P2P_PEER_RTSP_PORT::P2P not found, use default RTSP port\n"));
								}
								if (pEntry->P2pInfo.p2pIndex < MAX_P2P_GROUP_SIZE)
									P2PPrintP2PEntry(pAd, pEntry->P2pInfo.p2pIndex);
								break;
							}
						}
						DBGPRINT(RT_DEBUG_TRACE, ("OID_802_11_P2P_PEER_RTSP_PORT bssid: %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(pP2PCtrl->CurrentAddress)));
					}
				}
				if (copy_to_user(wrq->u.data.pointer, &RtspPort, wrq->u.data.length))
				{
					Status = -EFAULT;
				}
				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WFD_PEER_RTSP_PORT (=%d)\n", RtspPort));
			}
			break;

		/*
		 * Query discovered MAC address list of WFD devices (used by WFD sigma tool)
		 * Value:
		 * 	MAC address list (max. number: 7)
		*/
		case OID_802_11_WFD_DEV_LIST:
			{
				CHAR DevList[128];
				pP2pTable = &pAd->P2pTable;
				INT	ListNum = 0;
				INT	ListLen = 0;

				RTMPZeroMemory(DevList, sizeof(DevList));
#ifdef DOT11Z_TDLS_SUPPORT
				if (pAd->StaCfg.TdlsInfo.bTDLSCapable && pAd->StaCfg.WfdCfg.PC == WFD_PC_TDLS)
				{
					PRT_802_11_TDLS pTDLS = NULL;

					for (i = MAX_NUM_OF_TDLS_ENTRY - 1; i >= 0; i--)
					{
						pTDLS = &pAd->StaCfg.TdlsInfo.TDLSEntry[i];
						
						if (pTDLS->Valid)
						{
							if (ListNum > 0)
							{
								*(DevList + ListLen) = 0x20;
								ListLen += 1;
							}
							
							sprintf(DevList + ListLen, "%02x:%02x:%02x:%02x:%02x:%02x", pTDLS->MacAddr[0], pTDLS->MacAddr[1], pTDLS->MacAddr[2],
							pTDLS->MacAddr[3], pTDLS->MacAddr[4], pTDLS->MacAddr[5]);

							ListLen += 17;
							ListNum++;
							if (ListNum >= 7)
								break;
						}
					}
				}
#endif /* DOT11Z_TDLS_SUPPORT */
				for (i=0; i < pAd->P2pTable.ClientNumber; i++)
				{
					if (ListNum >= 7)
						break;

					if (ListNum > 0)
					{
						*(DevList + ListLen) = 0x20;
						ListLen += 1;
					}
					sprintf(DevList + ListLen, "%02x:%02x:%02x:%02x:%02x:%02x", pP2pTable->Client[i].addr[0], pP2pTable->Client[i].addr[1], pP2pTable->Client[i].addr[2],
					pP2pTable->Client[i].addr[3], pP2pTable->Client[i].addr[4], pP2pTable->Client[i].addr[5]);
					
					ListLen += 17;
					ListNum++;
				}
				
				wrq->u.data.length = sizeof(DevList);
				Status = copy_to_user(wrq->u.data.pointer, DevList, sizeof(DevList));

				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WFD_DEV_LIST (Len=%d)\n", sizeof(DevList)));
			}
			break;
#endif /* WFD_SUPPORT */
#endif /* P2P_SUPPORT */

		case RT_OID_VERSION_INFO:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_VERSION_INFO \n"));
			wrq->u.data.length = 8*sizeof(CHAR);
			snprintf(&driverVersion[0], sizeof(driverVersion), "%s", AP_DRIVER_VERSION);
			driverVersion[7] = '\0';
			if (copy_to_user(wrq->u.data.pointer, &driverVersion, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;

		case OID_802_11_NETWORK_TYPES_SUPPORTED:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_NETWORK_TYPES_SUPPORTED \n"));
			wrq->u.data.length = sizeof(UCHAR);
			if (copy_to_user(wrq->u.data.pointer, &pAd->RfIcType, wrq->u.data.length))
			{
				Status = -EFAULT; 	
			}
			break;

#ifdef IAPP_SUPPORT
		case RT_QUERY_SIGNAL_CONTEXT:
		{
			BOOLEAN FlgIs11rSup = FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_QUERY_SIGNAL_CONTEXT \n"));

#ifdef DOT11R_FT_SUPPORT
			FlgIs11rSup = TRUE;
#endif /* DOT11R_FT_SUPPORT */

			if (FlgIs11rSup == FALSE)
			{
#if 0
			wrq->u.data.length = sizeof(RT_SIGNAL_STRUC);
			if (copy_to_user(wrq->u.data.pointer, &pObj->RTSignal, wrq->u.data.length))
#endif
			{
				Status = -EFAULT;
			}
		}
#ifdef DOT11R_FT_SUPPORT
			else
			{
				FT_KDP_SIGNAL *pFtKdp;
				FT_KDP_EVT_HEADER *pEvtHdr;


				/* query signal content for 11r */
				DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_QUERY_FT_KDP_CONTEXT \n"));

				FT_KDP_EventGet(pAd, &pFtKdp);
				if (pFtKdp != NULL)
					pEvtHdr = (FT_KDP_EVT_HEADER *)pFtKdp->Content;

				if ((pFtKdp != NULL) &&
					((RT_SIGNAL_STRUC_HDR_SIZE + pEvtHdr->EventLen) <=
														wrq->u.data.length))
				{
					/* copy the event */
					if (copy_to_user(
								wrq->u.data.pointer,
								pFtKdp,
								RT_SIGNAL_STRUC_HDR_SIZE + pEvtHdr->EventLen))
					{
						wrq->u.data.length = 0;
						Status = -EFAULT;
					}
					else
					{
						wrq->u.data.length = RT_SIGNAL_STRUC_HDR_SIZE;
						wrq->u.data.length += pEvtHdr->EventLen;
					}

					FT_MEM_FREE(pAd, pFtKdp);
				}
				else
				{
					/* no event is queued */
					DBGPRINT(RT_DEBUG_TRACE, ("ft_kdp> no event is queued!\n"));
					wrq->u.data.length = 0;
				}
			}
#endif /* DOT11R_FT_SUPPORT */
		}
			break;

#ifdef DOT11R_FT_SUPPORT
		case RT_FT_DATA_ENCRYPT:
		case RT_FT_DATA_DECRYPT:
		{
			UCHAR *pBuffer;
			UINT32 DataLen;

			DataLen = wrq->u.data.length;

			/*
				Make sure the data length is multiple of 8
				due to AES_KEY_WRAP() limitation.
			*/
			if (DataLen & 0x07)
				DataLen += 8 - (DataLen & 0x07);

			FT_MEM_ALLOC(pAd, &pBuffer, DataLen+FT_KDP_KEY_ENCRYPTION_EXTEND);
			if (pBuffer == NULL)
				break;
			NdisZeroMemory(pBuffer, DataLen+FT_KDP_KEY_ENCRYPTION_EXTEND);

			if (copy_from_user(pBuffer, wrq->u.data.pointer, wrq->u.data.length))
			{
				Status = -EFAULT;
				FT_MEM_FREE(pAd, pBuffer);
				break;
			}

			switch(cmd)
			{
				case RT_FT_DATA_ENCRYPT:
					DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_FT_DATA_ENCRYPT \n"));
					FT_KDP_DataEncrypt(pAd, (UCHAR *)pBuffer, &DataLen);
					break;

				case RT_FT_DATA_DECRYPT:
					DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_FT_DATA_DECRYPT \n"));
					FT_KDP_DataDecrypt(pAd, (UCHAR *)pBuffer, &DataLen);
					break;
			}

			wrq->u.data.length = DataLen;
			if (copy_to_user(wrq->u.data.pointer, pBuffer, wrq->u.data.length))
				Status = -EFAULT;
			FT_MEM_FREE(pAd, pBuffer);
		}
		break;

		case RT_OID_802_11R_INFO:
			{
				UCHAR apidx = pObj->ioctl_if;
				PFT_CONFIG_INFO pFtConfig;
				PFT_CFG pFtCfg;

/*				pFtConfig = kmalloc(sizeof(FT_CONFIG_INFO), GFP_ATOMIC); */
				os_alloc_mem(pAd, (UCHAR **)&pFtConfig, sizeof(FT_CONFIG_INFO));
				if (pFtConfig == NULL)
					break;

				pFtCfg = &pAd->ApCfg.MBSSID[apidx].FtCfg;
				NdisZeroMemory(pFtConfig, sizeof(FT_CONFIG_INFO));

				pFtConfig->FtSupport = pFtCfg->FtCapFlag.Dot11rFtEnable;
				pFtConfig->FtRicSupport = pFtCfg->FtCapFlag.RsrReqCap;
				pFtConfig->FtOtdSupport = pFtCfg->FtCapFlag.FtOverDs;
				NdisMoveMemory(pFtConfig->MdId, pFtCfg->FtMdId, FT_MDID_LEN);
				pFtConfig->R0KHIdLen = pFtCfg->FtR0khIdLen;
				NdisMoveMemory(pFtConfig->R0KHId, pFtCfg->FtR0khId, pFtCfg->FtR0khIdLen);

				wrq->u.data.length = sizeof(FT_CONFIG_INFO);
				Status = copy_to_user(wrq->u.data.pointer, pFtConfig, wrq->u.data.length);
				os_free_mem(NULL, pFtConfig);
			}
			break;
#endif /* DOT11R_FT_SUPPORT */

#endif /* IAPP_SUPPORT */

#ifdef WMM_ACM_SUPPORT
		case RT_OID_WMM_ACM_TSPEC:
		{
			UINT32 NumStream;
			CHAR *pMac;
			UINT32 *pNumOfTspec;
			BOOLEAN FlgIsOk;

			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WMM_ACM_TSPEC \n"));

			FlgIsOk = FALSE;
			pMac = wrq->u.data.pointer; /* from user */
			pNumOfTspec = (UINT32 *)wrq->u.data.pointer; /* to user */
			NumStream = ACMP_StreamNumGet(pAd, ACM_SM_CATEGORY_PEER, 1, (PUCHAR)pMac);

			if (NumStream > 0)
			{
				/* at least one TSPEC */
				if (wrq->u.data.length >= \
						(sizeof(UINT32)+sizeof(ACM_STREAM_INFO)*NumStream))
				{
					/* user buffer is enough to fill all TSPECs */
					if (ACMP_StreamsGet(
							pAd, ACM_SM_CATEGORY_PEER, 1,
							&NumStream, (PUCHAR)pMac,
							(ACM_STREAM_INFO *)(wrq->u.data.pointer+sizeof(UINT32))) == ACM_RTN_OK)
					{
						/* fill the actual number of TSPEC */
						*pNumOfTspec = NumStream;
						FlgIsOk = TRUE;
					}
				}
			}

			if (FlgIsOk == FALSE)
				*pNumOfTspec = 0; /* get fail */
		}
			break;
#endif /* WMM_ACM_SUPPORT */

#ifdef WSC_AP_SUPPORT
		case RT_OID_WSC_QUERY_STATUS:
		{
			INT WscStatus;
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_QUERY_STATUS \n"));
#ifdef APCLI_SUPPORT
			if (pObj->ioctl_if_type == INT_APCLI)
			{
				INT ApCliIdx = pObj->ioctl_if;
				APCLI_MR_APIDX_SANITY_CHECK(ApCliIdx);
				WscStatus = pAd->ApCfg.ApCliTab[ApCliIdx].WscControl.WscStatus;
			}
			else
#endif /* APCLI_SUPPORT */
#ifdef P2P_SUPPORT
		    if (pObj->ioctl_if_type == INT_P2P)
		    {
		    	if (P2P_CLI_ON(pAd))
		    	{
				INT ApCliIdx = pObj->ioctl_if;
				APCLI_MR_APIDX_SANITY_CHECK(ApCliIdx);
				WscStatus = pAd->ApCfg.ApCliTab[ApCliIdx].WscControl.WscStatus;
		    	}
				else
				{
				WscStatus = pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus;
				}
		        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfMode_Proc:: This command is from p2p interface now.\n", apidx));
		    }
			else
#endif /* P2P_SUPPORT */
			{
				WscStatus = pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus;
			}

			wrq->u.data.length = sizeof(INT);
			if (copy_to_user(wrq->u.data.pointer, &WscStatus, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
		}
		case RT_OID_WSC_PIN_CODE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_PIN_CODE \n"));
			wrq->u.data.length = sizeof(UINT);
			/*WscPinCode = GenerateWpsPinCode(pAd, FALSE, apidx); */
#ifdef P2P_SUPPORT
		    if (pObj->ioctl_if_type == INT_P2P)
		    {
#ifdef WIDI_SUPPORT
				if (pAd->P2pCfg.bWIDI)
					pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
				else
#endif  /* WIDI_SUPPORT */
				{
			    	if (P2P_CLI_ON(pAd))
			    	{
						pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
			    	}
					else
					{
						pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
					}
				}
		        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfMode_Proc:: This command is from p2p interface now.\n", apidx));
		    }
			else
#endif /* P2P_SUPPORT */
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
			
			WscPinCode = pWscControl->WscEnrolleePinCode;
			if (copy_to_user(wrq->u.data.pointer, &WscPinCode, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
#ifdef APCLI_SUPPORT
        case RT_OID_APCLI_WSC_PIN_CODE:
            DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_APCLI_WSC_PIN_CODE \n"));
			wrq->u.data.length = sizeof(UINT);
			/*WscPinCode = GenerateWpsPinCode(pAd, TRUE, apidx); */
			WscPinCode = pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode;
			
			if (copy_to_user(wrq->u.data.pointer, &WscPinCode, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
            break;
#endif /* APCLI_SUPPORT */
		case RT_OID_WSC_UUID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_QUERY_UUID \n"));
			wrq->u.data.length = UUID_LEN_STR;
#ifdef P2P_SUPPORT
		    if (pObj->ioctl_if_type == INT_P2P)
		    {
		    	if (P2P_CLI_ON(pAd))
		    	{
					pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
		    	}
				else
				{
					pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
				}
		        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfMode_Proc:: This command is from p2p interface now.\n", apidx));
		    }
			else
#endif /* P2P_SUPPORT */
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
			if (copy_to_user(wrq->u.data.pointer, &pWscControl->Wsc_Uuid_Str[0], UUID_LEN_STR))
			{
				Status = -EFAULT;
			}
			break;
		case RT_OID_WSC_MAC_ADDRESS:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_MAC_ADDRESS \n"));
			wrq->u.data.length = MAC_ADDR_LEN;
//			if (copy_to_user(wrq->u.data.pointer, pAd->ApCfg.MBSSID[apidx].Bssid, wrq->u.data.length))
			if (copy_to_user(wrq->u.data.pointer, pAd->ApCfg.MBSSID[apidx].wdev.bssid, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
		case RT_OID_WSC_CONFIG_STATUS:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_CONFIG_STATUS \n"));
			wrq->u.data.length = sizeof(UCHAR);
			if (copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;

		case RT_OID_WSC_QUERY_PEER_INFO_ON_RUNNING:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_QUERY_PEER_INFO_ON_RUNNING \n"));

			if (pAd->ApCfg.MBSSID[apidx].WscControl.WscState > WSC_STATE_WAIT_M2)
			{
				wrq->u.data.length = sizeof(WSC_PEER_DEV_INFO);
				if (copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].WscControl.WscPeerInfo, wrq->u.data.length))
				{
					Status = -EFAULT;
				}
			}
			else
			{
				Status = -EFAULT;
			}
			break;

		case RT_OID_802_11_WSC_QUERY_PROFILE:
			wrq->u.data.length = sizeof(WSC_PROFILE);
			os_alloc_mem(pAd, (UCHAR **)&pProfile, sizeof(WSC_PROFILE));
			if (pProfile == NULL)
			{
				Status = -EFAULT;
				DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_802_11_WSC_QUERY_PROFILE fail!\n"));
				break;
			}
#ifdef P2P_SUPPORT
		    if (pObj->ioctl_if_type == INT_P2P)
		    {
		    	if (P2P_CLI_ON(pAd))
		    	{
					pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
		    	}
				else
				{
					pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
				}
		        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfMode_Proc:: This command is from p2p interface now.\n", apidx));
		    }
			else
#endif /* P2P_SUPPORT */
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;

			RTMPZeroMemory(pProfile, sizeof(WSC_PROFILE));
			NdisMoveMemory(pProfile, &pWscControl->WscProfile, sizeof(WSC_PROFILE));
            if ((pProfile->Profile[0].AuthType == WSC_AUTHTYPE_OPEN) && (pProfile->Profile[0].EncrType == WSC_ENCRTYPE_NONE))
            {
                pProfile->Profile[0].KeyLength = 0;
                NdisZeroMemory(pProfile->Profile[0].Key, 64);
            }
			if (copy_to_user(wrq->u.data.pointer, pProfile, wrq->u.data.length))
			{
				Status = -EFAULT;
			}			

			os_free_mem(NULL, pProfile);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_WSC_QUERY_PROFILE \n"));
			break;
#ifdef WSC_V2_SUPPORT
		case RT_OID_WSC_V2_SUPPORT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_V2_SUPPORT (=%d)\n", pAd->ApCfg.MBSSID[apidx].WscControl.WscV2Info.bEnableWpsV2));
			wrq->u.data.length = sizeof(BOOLEAN);
			if (copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].WscControl.WscV2Info.bEnableWpsV2, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
		case RT_OID_WSC_FRAGMENT_SIZE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_FRAGMENT_SIZE (=%d)\n", pAd->ApCfg.MBSSID[apidx].WscControl.WscFragSize));
			wrq->u.data.length = sizeof(USHORT);
			if (copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].WscControl.WscFragSize, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
#endif /* WSC_V2_SUPPORT */
#endif /* WSC_AP_SUPPORT */
#ifdef LLTD_SUPPORT
#if 0 /* os abl move to os/linux/ap_ioctl.c */
        case RT_OID_GET_PHY_MODE:
            DBGPRINT(RT_DEBUG_TRACE, ("Query::Get phy mode (%02X) \n", pAd->CommonCfg.PhyMode));
            wrq->u.mode = (UINT32)pAd->CommonCfg.PhyMode;
            break;
#endif /* 0 */

        case RT_OID_GET_LLTD_ASSO_TABLE:
            DBGPRINT(RT_DEBUG_TRACE, ("Query::Get LLTD association table\n"));
            if ((wrq->u.data.pointer == NULL) || (apidx != MAIN_MBSSID))
            {
                Status = -EFAULT;
            }
            else
            {
                INT						    i;
                RT_LLTD_ASSOICATION_TABLE	AssocTab;

            	AssocTab.Num = 0;
            	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
            	{
            		if (IS_ENTRY_CLIENT(&pAd->MacTab.Content[i]) && (pAd->MacTab.Content[i].Sst == SST_ASSOC))
            		{
            			COPY_MAC_ADDR(AssocTab.Entry[AssocTab.Num].Addr, &pAd->MacTab.Content[i].Addr);
                        AssocTab.Entry[AssocTab.Num].phyMode = pAd->CommonCfg.PhyMode;
                        AssocTab.Entry[AssocTab.Num].MOR = RateIdToMbps[pAd->ApCfg.MBSSID[apidx].MaxTxRate] * 2;
            			AssocTab.Num += 1;
            		}
            	}            
                wrq->u.data.length = sizeof(RT_LLTD_ASSOICATION_TABLE);
            	if (copy_to_user(wrq->u.data.pointer, &AssocTab, wrq->u.data.length))
            	{
            		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
                    Status = -EFAULT;
            	}
                DBGPRINT(RT_DEBUG_TRACE, ("AssocTab.Num = %d \n", AssocTab.Num));
            }
            break;
#ifdef APCLI_SUPPORT
		case RT_OID_GET_REPEATER_AP_LINEAGE:
#if 0			
			DBGPRINT(RT_DEBUG_TRACE, ("Query::Get repeater AP lineage.\n"));
			if (wrq->u.data.pointer == NULL)
			{
				Status = -EFAULT;
				break;
			}

			if (pAd->ApCfg.ApCliTab[apidx].Valid)
			{
				wrq->u.data.length = 6;
				if (copy_to_user(wrq->u.data.pointer,
					APCLI_GET_ROOT_BSSID(pAd, pAd->ApCfg.ApCliTab[apidx].MacTabWCID), wrq->u.data.length))
				{
					DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
					Status = -EFAULT;
				}
				DBGPRINT(RT_DEBUG_TRACE, ("%s: Root AP BSSID: \n", __FUNCTION__));
			}
			else
				wrq->u.data.length = 0;
#else
			DBGPRINT(RT_DEBUG_TRACE, ("Not Support : Get repeater AP lineage.\n"));
#endif
			break;
#endif /* APCLI_SUPPORT */

#endif /* LLTD_SUPPORT */
#ifdef NINTENDO_AP
		case RT_OID_802_11_NINTENDO_GET_TABLE:
		    RTMPIoctlNintendoGetTable(pAd, wrq);
			break;
#endif /* NINTENDO_AP */
#ifdef DOT1X_SUPPORT
		case OID_802_DOT1X_CONFIGURATION:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::Get Radius setting(%d)\n", sizeof(DOT1X_CMM_CONF)));
				RTMPIoctlQueryRadiusConf(pAd, wrq);	
			break;
#endif /* DOT1X_SUPPORT */			
#ifdef SNMP_SUPPORT
		case RT_OID_802_11_MAC_ADDRESS:
            wrq->u.data.length = MAC_ADDR_LEN;
            Status = copy_to_user(wrq->u.data.pointer, &pAd->CurrentAddress, wrq->u.data.length);
#ifdef RELEASE_EXCLUDE
            DBGPRINT(RT_DEBUG_INFO, ("Query::RT_OID_802_11_MAC_ADDRESS \n"));
#endif /* RELEASE_EXCLUDE */
			break;

		case RT_OID_802_11_MANUFACTUREROUI:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MANUFACTUREROUI \n"));
			wrq->u.data.length = ManufacturerOUI_LEN;
			Status = copy_to_user(wrq->u.data.pointer, &pAd->CurrentAddress, wrq->u.data.length);
			break;

		case RT_OID_802_11_MANUFACTURERNAME:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MANUFACTURERNAME \n"));
			wrq->u.data.length = strlen(ManufacturerNAME);
			Status = copy_to_user(wrq->u.data.pointer, ManufacturerNAME, wrq->u.data.length);
			break;

		case RT_OID_802_11_RESOURCETYPEIDNAME:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_RESOURCETYPEIDNAME \n"));
			wrq->u.data.length = strlen(ResourceTypeIdName);
			Status = copy_to_user(wrq->u.data.pointer, ResourceTypeIdName, wrq->u.data.length);
			break;

		case RT_OID_802_11_PRIVACYOPTIONIMPLEMENTED:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_PRIVACYOPTIONIMPLEMENTED \n"));
			ulInfo = 1; /* 1 is support wep else 2 is not support. */
			wrq->u.data.length = sizeof(ulInfo);
			Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
			break;

		case RT_OID_802_11_POWERMANAGEMENTMODE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_POWERMANAGEMENTMODE \n"));
			ulInfo = 1; /* 1 is power active else 2 is power save. */
			wrq->u.data.length = sizeof(ulInfo);
			Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
			break;

		case OID_802_11_WEPDEFAULTKEYVALUE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WEPDEFAULTKEYVALUE \n"));
			/*KeyIdxValue.KeyIdx = pAd->PortCfg.MBSSID[pObj->ioctl_if].DefaultKeyId; */
			pKeyIdxValue = wrq->u.data.pointer;
			DBGPRINT(RT_DEBUG_TRACE,("KeyIdxValue.KeyIdx = %d, \n",pKeyIdxValue->KeyIdx));

			valueLen = pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen;
			NdisMoveMemory(pKeyIdxValue->Value,
						   &pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].Key,
						   valueLen);
			pKeyIdxValue->Value[valueLen]='\0';

			wrq->u.data.length = sizeof(DefaultKeyIdxValue);

			Status = copy_to_user(wrq->u.data.pointer, pKeyIdxValue, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE,("DefaultKeyId = %d, total len = %d, str len=%d, KeyValue= %02x %02x %02x %02x \n", pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId, wrq->u.data.length, pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen,
			pAd->SharedKey[pObj->ioctl_if][0].Key[0],
			pAd->SharedKey[pObj->ioctl_if][1].Key[0],
			pAd->SharedKey[pObj->ioctl_if][2].Key[0],
			pAd->SharedKey[pObj->ioctl_if][3].Key[0]));
			break;

		case OID_802_11_WEPDEFAULTKEYID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_WEPDEFAULTKEYID \n"));
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("DefaultKeyId =%d \n", pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId));
			break;

		case RT_OID_802_11_WEPKEYMAPPINGLENGTH:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_WEPKEYMAPPINGLENGTH \n"));
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer,
									&pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen,
									wrq->u.data.length);
			break;

		case OID_802_11_SHORTRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_SHORTRETRYLIMIT \n"));
			wrq->u.data.length = sizeof(ULONG);
			RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
			ShortRetryLimit = tx_rty_cfg.field.ShortRtyLimit;
			DBGPRINT(RT_DEBUG_TRACE, ("ShortRetryLimit =%ld,  tx_rty_cfg.field.ShortRetryLimit=%d\n", ShortRetryLimit, tx_rty_cfg.field.ShortRtyLimit));
			Status = copy_to_user(wrq->u.data.pointer, &ShortRetryLimit, wrq->u.data.length);
			break;

		case OID_802_11_LONGRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_LONGRETRYLIMIT \n"));
			wrq->u.data.length = sizeof(ULONG);
			RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
			LongRetryLimit = tx_rty_cfg.field.LongRtyLimit;
			DBGPRINT(RT_DEBUG_TRACE, ("LongRetryLimit =%ld,  tx_rty_cfg.field.LongRtyLimit=%d\n", LongRetryLimit, tx_rty_cfg.field.LongRtyLimit));
			Status = copy_to_user(wrq->u.data.pointer, &LongRetryLimit, wrq->u.data.length);
			break;
			
		case RT_OID_802_11_PRODUCTID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_PRODUCTID \n"));
		
#ifdef RTMP_MAC_PCI
			{
			
				USHORT  device_id;
				if (((POS_COOKIE)pAd->OS_Cookie)->pci_dev != NULL)
			    	pci_read_config_word(((POS_COOKIE)pAd->OS_Cookie)->pci_dev, PCI_DEVICE_ID, &device_id);
				else 
					DBGPRINT(RT_DEBUG_TRACE, (" pci_dev = NULL\n"));
				snprintf((RTMP_STRING *)snmp_tmp, sizeof(snmp_tmp), "%04x %04x\n", NIC_PCI_VENDOR_ID, device_id);
			}
#endif /* RTMP_MAC_PCI */
#ifdef RTMP_MAC_USB
			snprintf((RTMP_STRING *)snmp_tmp, sizeof(snmp_tmp), "%04x %04x\n", 
						RtmpOsGetUsbDevVendorID(((POS_COOKIE)pAd->OS_Cookie)->pUsb_Dev),
						RtmpOsGetUsbDevProductID(((POS_COOKIE)pAd->OS_Cookie)->pUsb_Dev));
#endif /* RTMP_MAC_USB */
			wrq->u.data.length = strlen((RTMP_STRING *) snmp_tmp);
			Status = copy_to_user(wrq->u.data.pointer, snmp_tmp, wrq->u.data.length);
			break;

		case RT_OID_802_11_MANUFACTUREID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MANUFACTUREID \n"));
			wrq->u.data.length = strlen(ManufacturerNAME);
			Status = copy_to_user(wrq->u.data.pointer, ManufacturerNAME, wrq->u.data.length);
			break;

		case OID_802_11_CURRENTCHANNEL:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_CURRENTCHANNEL \n"));
			wrq->u.data.length = sizeof(UCHAR);
			DBGPRINT(RT_DEBUG_TRACE, ("sizeof UCHAR=%d, channel=%d \n", sizeof(UCHAR), pAd->CommonCfg.Channel));
			Status = copy_to_user(wrq->u.data.pointer, &pAd->CommonCfg.Channel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Status=%d\n", Status));
			break;
#endif /* SNMP_SUPPORT */

        case OID_802_11_STATISTICS:
		os_alloc_mem(pAd, (UCHAR **)&pStatistics, sizeof(NDIS_802_11_STATISTICS));
            if (pStatistics)
            {
                DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_STATISTICS \n"));
                /* add the most up-to-date h/w raw counters into software counters */
			    NICUpdateRawCounters(pAd);
                
		  pStatistics->TransmittedFragmentCount.QuadPart = pAd->WlanCounters.TransmittedFragmentCount.QuadPart + pAd->WlanCounters.MulticastTransmittedFrameCount.QuadPart;
                pStatistics->MulticastTransmittedFrameCount.QuadPart = pAd->WlanCounters.MulticastTransmittedFrameCount.QuadPart;
                pStatistics->FailedCount.QuadPart = pAd->WlanCounters.FailedCount.QuadPart;
                pStatistics->RetryCount.QuadPart = pAd->WlanCounters.RetryCount.QuadPart;
                pStatistics->MultipleRetryCount.QuadPart = pAd->WlanCounters.MultipleRetryCount.QuadPart;
                pStatistics->RTSSuccessCount.QuadPart = pAd->WlanCounters.RTSSuccessCount.QuadPart;
                pStatistics->RTSFailureCount.QuadPart = pAd->WlanCounters.RTSFailureCount.QuadPart;
                pStatistics->ACKFailureCount.QuadPart = pAd->WlanCounters.ACKFailureCount.QuadPart;
                pStatistics->FrameDuplicateCount.QuadPart = pAd->WlanCounters.FrameDuplicateCount.QuadPart;
                pStatistics->ReceivedFragmentCount.QuadPart = pAd->WlanCounters.ReceivedFragmentCount.QuadPart;
                pStatistics->MulticastReceivedFrameCount.QuadPart = pAd->WlanCounters.MulticastReceivedFrameCount.QuadPart;
#ifdef DBG	
                pStatistics->FCSErrorCount = pAd->RalinkCounters.RealFcsErrCount;
#else
                pStatistics->FCSErrorCount.QuadPart = pAd->WlanCounters.FCSErrorCount.QuadPart;
                pStatistics->FrameDuplicateCount.u.LowPart = pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100;
#endif
			pStatistics->TransmittedFrameCount.QuadPart = pAd->WlanCounters.TransmittedFragmentCount.QuadPart;
			pStatistics->WEPUndecryptableCount.QuadPart = pAd->WlanCounters.WEPUndecryptableCount.QuadPart;
                wrq->u.data.length = sizeof(NDIS_802_11_STATISTICS);
                Status = copy_to_user(wrq->u.data.pointer, pStatistics, wrq->u.data.length);
/*                kfree(pStatistics); */
				os_free_mem(NULL, pStatistics);
            }
            else
            {
                DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_STATISTICS(kmalloc failed)\n"));
                Status = -EFAULT;
            }
            break;

	case RT_OID_802_11_PER_BSS_STATISTICS:
		{
			PMBSS_STATISTICS pMbssStat;
			INT apidx = pObj->ioctl_if;
			BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[apidx];

			os_alloc_mem(pAd, (UCHAR * *) &pMbssStat, sizeof(MBSS_STATISTICS));
			NdisZeroMemory(pMbssStat, sizeof(MBSS_STATISTICS));

			pMbssStat->TransmittedByteCount = pMbss->TransmittedByteCount;
			pMbssStat->ReceivedByteCount =  pMbss->ReceivedByteCount;
			pMbssStat->TxCount =  pMbss->TxCount;
                        pMbssStat->RxCount =  pMbss->RxCount;
			pMbssStat->RxErrorCount =  pMbss->RxErrorCount;
			pMbssStat->RxDropCount =  pMbss->RxDropCount;
			pMbssStat->TxErrorCount =  pMbss->TxErrorCount;
			pMbssStat->TxDropCount =  pMbss->TxDropCount;
			pMbssStat->ucPktsTx =  pMbss->ucPktsTx;
			pMbssStat->ucPktsRx =  pMbss->ucPktsRx;
			pMbssStat->mcPktsTx =  pMbss->mcPktsTx;
			pMbssStat->mcPktsRx =  pMbss->mcPktsRx;
			pMbssStat->bcPktsTx=  pMbss->bcPktsTx;
			pMbssStat->bcPktsRx=  pMbss->bcPktsRx;
			wrq->u.data.length = sizeof(MBSS_STATISTICS);
			copy_to_user(wrq->u.data.pointer, pMbssStat, wrq->u.data.length);
			os_free_mem(pAd, pMbssStat);			
		}
		break;
			

#ifdef DOT11_N_SUPPORT
#ifdef TXBF_SUPPORT
		case RT_OID_802_11_QUERY_TXBF_TABLE:
			if (pAd->chipCap.FlgHwTxBfCap)
			{
				INT i;
				RT_802_11_TXBF_TABLE MacTab;

				MacTab.Num = 0;
				for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
				{
					if (IS_ENTRY_CLIENT(&pAd->MacTab.Content[i]) && (pAd->MacTab.Content[i].Sst == SST_ASSOC))
					{
						memcpy(&MacTab.Entry[MacTab.Num], &pAd->MacTab.Content[i].TxBFCounters, sizeof(RT_COUNTER_TXBF));
						MacTab.Num++;
					}
				}

				wrq->u.data.length = sizeof(RT_802_11_TXBF_TABLE);
				Status = copy_to_user(wrq->u.data.pointer, &MacTab, wrq->u.data.length);
			}
			else
			{
				 DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_QUERY_TXBF_TABLE(chip not support)\n"));
				Status= -EFAULT;
			}
			break;
#endif /* TXBF_SUPPORT */
#endif /* DOT11_N_SUPPORT */

#ifdef MESH_SUPPORT
		case OID_802_11_MESH_DEVICENAME:
			if (pAd->MeshTab.dev)
			{
				wrq->u.data.length = strlen(RtmpOsGetNetDevName(pAd->MeshTab.dev)) + 1;
				Status = copy_to_user(wrq->u.data.pointer, RtmpOsGetNetDevName(pAd->MeshTab.dev), strlen(RtmpOsGetNetDevName(pAd->MeshTab.dev)));
				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_DEVICENAME (Len=%d, Name=%s)\n", wrq->u.data.length, RtmpOsGetNetDevName(pAd->MeshTab.dev)));
			}
			break;

		case OID_802_11_MESH_SECURITY_INFO:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_SECURITY_INFO \n"));
			if (wrq->u.data.length	!= sizeof(MESH_SECURITY_INFO))
			{
				DBGPRINT(RT_DEBUG_TRACE, ("Len Diff %d/%d \n", wrq->u.data.length, sizeof(MESH_SECURITY_INFO)));
				Status = -EINVAL;
			}
			else				
				RTMPIoctlQueryMeshSecurityInfo(pAd, wrq);	
			break;

		case OID_802_11_MESH_ID:
			wrq->u.data.length = pAd->MeshTab.MeshIdLen;
			Status = copy_to_user(wrq->u.data.pointer, &pAd->MeshTab.MeshId, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_ID (Len=%d, MeshID=%s)\n", pAd->MeshTab.MeshIdLen,
																						pAd->MeshTab.MeshId));
			break;

		case OID_802_11_MESH_AUTO_LINK:
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->MeshTab.MeshAutoLink, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_AUTO_LINK (=%d)\n", pAd->MeshTab.MeshAutoLink));
			break;

		case OID_802_11_MESH_LINK_STATUS:
			{
				MESH_LINK_INFO	MeshLinkInfo;
				UCHAR i;

				NdisZeroMemory(&MeshLinkInfo, sizeof(MESH_LINK_INFO));

				for (i=0; i<MAX_MESH_LINKS; i++)
				{
					PMESH_LINK_ENTRY pEntry = &pAd->MeshTab.MeshLink[i].Entry;

					COPY_MAC_ADDR(MeshLinkInfo.Entry[i].PeerMacAddr, pEntry->PeerMacAddr);
					MeshLinkInfo.Entry[i].LinkType = pEntry->LinkType;
					MeshLinkInfo.Entry[i].Status = PeerLinkValidCheck(pAd, i);
					MeshLinkInfo.Entry[i].Rssi =
						RTMPMaxRssi(pAd, pAd->MacTab.Content[pEntry->MacTabMatchWCID].RssiSample.AvgRssi0,
							pAd->MacTab.Content[pEntry->MacTabMatchWCID].RssiSample.AvgRssi1,
							pAd->MacTab.Content[pEntry->MacTabMatchWCID].RssiSample.AvgRssi2);
					MeshLinkInfo.Entry[i].CurTxRate = pAd->MacTab.Content[pEntry->MacTabMatchWCID].HTPhyMode;
				}

				wrq->u.data.length = sizeof(MESH_LINK_INFO);
				Status = copy_to_user(wrq->u.data.pointer, &MeshLinkInfo, wrq->u.data.length);
			}
			break;

		case OID_802_11_MESH_LIST:
			{
				PMESH_NEIGHBOR_INFO	pMeshNeighborInfo; /* = kmalloc(sizeof(MESH_NEIGHBOR_INFO), GFP_ATOMIC); */
				UCHAR i, idx = 0;

				os_alloc_mem(pAd, (UCHAR **)&pMeshNeighborInfo, sizeof(MESH_NEIGHBOR_INFO));
				if (pMeshNeighborInfo == NULL)
					break;

				NdisZeroMemory(pMeshNeighborInfo, sizeof(MESH_NEIGHBOR_INFO));
				for (i=0; i<MAX_NEIGHBOR_MP; i++)
				{
					PMESH_NEIGHBOR_ENTRY pEntry = &pAd->MeshTab.pMeshNeighborTab->NeighborMP[i];

					if (pEntry->Valid)
					{
						strcpy((RTMP_STRING *) pMeshNeighborInfo->Entry[idx].HostName, (RTMP_STRING *) pEntry->HostName);
						COPY_MAC_ADDR(pMeshNeighborInfo->Entry[idx].MacAddr, pEntry->PeerMac);
						strcpy((RTMP_STRING *) pMeshNeighborInfo->Entry[idx].MeshId, (RTMP_STRING *) pEntry->MeshId);
						pMeshNeighborInfo->Entry[idx].Channel = pEntry->Channel;
						pMeshNeighborInfo->Entry[idx].Rssi = pEntry->RealRssi;
						pMeshNeighborInfo->Entry[idx].Status = pEntry->State != LINK_AVAILABLE ? 0 : 1;
						pMeshNeighborInfo->Entry[idx].MeshEncrypType =
							MeshCheckPeerMpCipher(pEntry->CapabilityInfo, pEntry->RSNIE, pEntry->RSNIE_Len);;
						idx++;
					}
				}
				pMeshNeighborInfo->num = idx;
				wrq->u.data.length = sizeof(MESH_NEIGHBOR_INFO);
				Status = copy_to_user(wrq->u.data.pointer, pMeshNeighborInfo, wrq->u.data.length);

				if (pMeshNeighborInfo)
/*					kfree(pMeshNeighborInfo); */
					os_free_mem(NULL, pMeshNeighborInfo);
			}
			break;

		case OID_802_11_MESH_ROUTE_LIST:
			{
				PRT_MESH_ROUTE_TABLE	rt_table; /* = kmalloc(sizeof(RT_MESH_ROUTE_TABLE), GFP_ATOMIC); */
				UCHAR i;

				os_alloc_mem(pAd, (UCHAR **)&rt_table, sizeof(RT_MESH_ROUTE_TABLE));
				if (rt_table == NULL)
					break;

				NdisZeroMemory(rt_table, sizeof(RT_MESH_ROUTE_TABLE));

				for (i=0; i<MESH_MAX_LEN_OF_FORWARD_TABLE; i++)
				{
					PMESH_ROUTING_ENTRY pEntry = &pAd->MeshTab.pMeshRouteTab->Content[i];

					if (pEntry->Valid)
					{
						COPY_MAC_ADDR(rt_table->Entry[rt_table->Num].MeshDA, pEntry->MeshDA);
						rt_table->Entry[rt_table->Num].Dsn = pEntry->Dsn;
						COPY_MAC_ADDR(rt_table->Entry[rt_table->Num].NextHop, pEntry->NextHop);
						rt_table->Entry[rt_table->Num].Metric = pEntry->PathMetric;
						rt_table->Num++;
					}
				} 

				wrq->u.data.length = sizeof(RT_MESH_ROUTE_TABLE);
				Status = copy_to_user(wrq->u.data.pointer, rt_table, wrq->u.data.length);

				if (rt_table)
/*					kfree(rt_table); */
					os_free_mem(NULL, rt_table);

				DBGPRINT(RT_DEBUG_OFF, ("Query::OID_802_11_MESH_ROUTE_LIST \n"));
			}
			break;

		case OID_802_11_MESH_MAX_TX_RATE:
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->MeshTab.MeshMaxTxRate, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_CHANNEL (=%d)\n", pAd->MeshTab.MeshChannel));
			break;

		case OID_802_11_MESH_CHANNEL:
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->MeshTab.MeshChannel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_CHANNEL (=%d)\n", pAd->MeshTab.MeshChannel));
			break;

		case OID_802_11_MESH_HOSTNAME:
			wrq->u.data.length = strlen((RTMP_STRING *) pAd->MeshTab.HostName) + 1;
			Status = copy_to_user(wrq->u.data.pointer, &pAd->MeshTab.HostName, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_HOSTNAME (MeshHostName=%s)\n", pAd->MeshTab.HostName));
			break;
		case OID_802_11_MESH_ONLY_MODE:
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->MeshTab.MeshOnly, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_ONLY_MODE (=%s)\n", pAd->MeshTab.MeshOnly == TRUE ? "Enable" : "Disable"));
			break;
#endif /* MESH_SUPPORT */

#ifdef WAPI_SUPPORT
		case OID_802_11_MCAST_TXIV:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MCAST_TXIV \n"));
#if 0
			wrq->u.data.length = LEN_WAPI_TSC; 
			Status = copy_to_user(wrq->u.data.pointer, 0/*pAd->ApCfg.MBSSID[pObj->ioctl_if].tx_iv*/, wrq->u.data.length); 
#endif
			Status  = -EINVAL;
			break;			
		case OID_802_11_WAPI_CONFIGURATION:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::Get WAPI Configuration(%d)\n", sizeof(WAPI_CONF)));
			RTMPIoctlQueryWapiConf(pAd, wrq);	
			break;			
		case OID_802_11_WAPI_IE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WAPI_IE\n"));
			if (wrq->u.data.length != sizeof(WAPI_WIE_STRUCT))
                Status  = -EINVAL;
            else
            {                												
				WAPI_WIE_STRUCT   	wapi_ie;
				MAC_TABLE_ENTRY		*pWapiEntry;

				NdisZeroMemory(&wapi_ie, sizeof(WAPI_WIE_STRUCT));
				NdisMoveMemory(wapi_ie.addr, wrq->u.data.pointer, MAC_ADDR_LEN);
					
				pWapiEntry = MacTableLookup(pAd, wapi_ie.addr);
						
				if (pWapiEntry && IS_ENTRY_CLIENT(pWapiEntry) && (pWapiEntry->RSNIE_Len > 0))
				{										
					wapi_ie.wie_len = pWapiEntry->RSNIE_Len;
					NdisMoveMemory(wapi_ie.wie, pWapiEntry->RSN_IE, pWapiEntry->RSNIE_Len);						
				}
								
				if (copy_to_user(wrq->u.data.pointer, &wapi_ie, wrq->u.data.length))
				{
					DBGPRINT(RT_DEBUG_ERROR, ("%s: copy_to_user() fail\n", __FUNCTION__));
				}								
            }
			break;		

		case OID_802_11_MCAST_KEY_INFO:
			{
				BSS_STRUCT *pMbss;
				WAPI_MCAST_KEY_STRUCT   wapi_mkey;

				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MCAST_KEY_INFO\n"));
							
				pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];					
				NdisZeroMemory(&wapi_mkey, sizeof(WAPI_MCAST_KEY_STRUCT));

				if (pMbss->sw_wpi_encrypt)
				{					
					NdisMoveMemory(wapi_mkey.m_tx_iv, 
								   pAd->SharedKey[pObj->ioctl_if][pMbss->DefaultKeyId].TxTsc,
								   LEN_WAPI_TSC);
				}
				else
				{
					INT	m_wcid;	
				
					GET_GroupKey_WCID(pAd, m_wcid, apidx);
					RTMPGetWapiTxTscFromAsic(pAd, m_wcid, wapi_mkey.m_tx_iv);
				}				
				wapi_mkey.key_id = pMbss->DefaultKeyId;													
				NdisMoveMemory(wapi_mkey.key_announce, pMbss->key_announce_flag, LEN_WAPI_TSC);
				NdisMoveMemory(wapi_mkey.NMK, pMbss->NMK, 16);

				wrq->u.data.length = sizeof(WAPI_MCAST_KEY_STRUCT);
				Status = copy_to_user(wrq->u.data.pointer, &wapi_mkey, wrq->u.data.length);
			}
			break;		
			
#endif /* WAPI_SUPPORT */

#ifdef HOSTAPD_SUPPORT

		case HOSTAPD_OID_GETWPAIE:/*report wpa ie of the new station to hostapd. */
			
			if (wrq->u.data.length != sizeof(wpaie))
			{
				Status = -EINVAL;
			}
			else if (copy_from_user(&wpaie, wrq->u.data.pointer, IEEE80211_ADDR_LEN))
			{
				Status = -EFAULT;
			}
			else
			{
				pEntry = MacTableLookup(pAd, wpaie.wpa_macaddr);
				if (!pEntry){
					Status = -EINVAL;
					break;
				}
				NdisZeroMemory(wpaie.rsn_ie,sizeof(wpaie.rsn_ie));
				/* For WPA1, RSN_IE=221 */
				if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) 
					||(pEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK)
					|| (pEntry->AuthMode == Ndis802_11AuthModeWPA1WPA2) ||(pEntry->AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
					)
				{
					int ielen = pEntry->RSNIE_Len;
					DBGPRINT(RT_DEBUG_TRACE, ("pEntry->RSNIE_Len=%d\n",pEntry->RSNIE_Len));
					if (ielen > sizeof(wpaie.rsn_ie))
						ielen = sizeof(wpaie.rsn_ie)-1;
					p = wpaie.rsn_ie;
					hex_dump("HOSTAPD_OID_GETWPAIE woody==>pEntry->RSN_IE", (unsigned char*)pEntry->RSN_IE,ielen);
					NdisMoveMemory(p, pEntry->RSN_IE, ielen);
				}
			}
			if(copy_to_user(wrq->u.data.pointer, &wpaie, sizeof(wpaie)))
				Status = -EFAULT;
			break;

			
		case HOSTAPD_OID_GET_SEQ:/*report txtsc to hostapd. */

			pMbss = &pAd->ApCfg.MBSSID[apidx];
			if (wrq->u.data.length != sizeof(ik))
			{
				Status = -EINVAL;
			}
			else if (copy_from_user(&ik, wrq->u.data.pointer, IEEE80211_ADDR_LEN))
			{
				Status = -EFAULT;
			}
			else
			{
				NdisZeroMemory(&ik.ik_keytsc, sizeof(ik.ik_keytsc));
				p = (unsigned char *)&ik.ik_keytsc;
				NdisMoveMemory(p+2, pAd->SharedKey[apidx][ pMbss->DefaultKeyId].TxTsc, 6);
				if(copy_to_user(wrq->u.data.pointer, &ik, sizeof(ik)))
					Status = -EFAULT;
			}
			break;

			
		case HOSTAPD_OID_GET_1X_GROUP_KEY:/*report default group key to hostapd. */

			pMbss = &pAd->ApCfg.MBSSID[apidx];
			if (wrq->u.data.length != sizeof(group_key))
			{
				Status = -EINVAL;
			}
			else
			{
				if(pAd->SharedKey[apidx][ pMbss->DefaultKeyId].KeyLen!=0 && pAd->SharedKey[apidx][ pMbss->DefaultKeyId].Key!=NULL)
				{
					group_key.ik_keyix = pMbss->DefaultKeyId;
					group_key.ik_keylen = pAd->SharedKey[apidx][ pMbss->DefaultKeyId].KeyLen;
					NdisMoveMemory(group_key.ik_keydata, pAd->SharedKey[apidx][ pMbss->DefaultKeyId].Key,pAd->SharedKey[apidx][ pMbss->DefaultKeyId].KeyLen);
					if(copy_to_user(wrq->u.data.pointer, &group_key, sizeof(group_key)))
						Status = -EFAULT;
				}
			}
			break;

#endif/*HOSTAPD_SUPPORT*/

#ifdef APCLI_SUPPORT
		case OID_GEN_MEDIA_CONNECT_STATUS:
			{
				ULONG ApCliIdx = pObj->ioctl_if;

				NDIS_MEDIA_STATE MediaState;
				PMAC_TABLE_ENTRY pEntry;
				PAPCLI_STRUCT pApCliEntry;

				if ((pObj->ioctl_if_type != INT_APCLI)
#ifdef P2P_SUPPORT
					&& (pObj->ioctl_if_type != INT_P2P)
#endif /* P2P_SUPPORT */
					)
				{
					Status = -EOPNOTSUPP;
					break;
				}
				else
				{
					APCLI_MR_APIDX_SANITY_CHECK(ApCliIdx);
					pApCliEntry = &pAd->ApCfg.ApCliTab[ApCliIdx];
					pEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID]; 

					if (!IS_ENTRY_APCLI(pEntry)
#ifdef P2P_SUPPORT
						&& !IS_ENTRY_P2PCLI(pEntry)
#endif /* P2P_SUPPORT */
						)
					{
						MediaState = NdisMediaStateDisconnected;
					}
					else
					{
						if ((pAd->ApCfg.ApCliTab[ApCliIdx].Valid == TRUE)
							&& (pEntry->PortSecured == WPA_802_1X_PORT_SECURED))
							MediaState = NdisMediaStateConnected;
						else
							MediaState = NdisMediaStateDisconnected;
					}

					wrq->u.data.length = sizeof(NDIS_MEDIA_STATE);
					Status = copy_to_user(wrq->u.data.pointer, &MediaState, wrq->u.data.length);
				}
			}
			break;
#endif /* APCLI_SUPPORT */

#ifdef RTMP_RBUS_SUPPORT
		case RT_OID_802_11_SNR_0:
			if ((pAd->ApCfg.LastSNR0 > 0))
			{
				ulInfo = ConvertToSnr(pAd, pAd->ApCfg.LastSNR0);

				wrq->u.data.length = sizeof(ulInfo);
				Status = copy_to_user(wrq->u.data.pointer, &ulInfo,	wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_SNR_0(0x=%lx)\n", ulInfo));
			}
            else
			    Status = -EFAULT;
			break;
		case RT_OID_802_11_SNR_1:
			if ((pAd->Antenna.field.RxPath	> 1) && 
                (pAd->ApCfg.LastSNR1 > 0))
			{
				ulInfo = ConvertToSnr(pAd, pAd->ApCfg.LastSNR1);
				wrq->u.data.length = sizeof(ulInfo);
				Status = copy_to_user(wrq->u.data.pointer, &ulInfo,	wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_1(0x=%lx)\n",ulInfo));
			}
			else
				Status = -EFAULT;
            DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_1(pAd->ApCfg.LastSNR1=%d)\n",pAd->ApCfg.LastSNR1));
			break;
#ifdef DOT11N_SS3_SUPPORT
		case RT_OID_802_11_SNR_2:
			if ((pAd->Antenna.field.RxPath	> 2) && 
				(pAd->ApCfg.LastSNR2 > 0))
			{
				ulInfo = ConvertToSnr(pAd, pAd->ApCfg.LastSNR2);
				wrq->u.data.length = sizeof(ulInfo);
				Status = copy_to_user(wrq->u.data.pointer, &ulInfo,	wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_2(0x=%lx)\n",ulInfo));
			}
			else
				Status = -EFAULT;
            DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_2(pAd->ApCfg.LastSNR2=%d)\n",pAd->ApCfg.LastSNR2));
			break;
#endif /* DOT11N_SS3_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

#ifdef EASY_CONFIG_SETUP
		case OID_PIN_OF_ENROLLEE:
			os_alloc_mem(NULL, &pStaMacAddr, MAC_ADDR_LEN);
			if (pStaMacAddr)
			{
	            Status = copy_from_user(pStaMacAddr, wrq->u.data.pointer, MAC_ADDR_LEN);
				if (Status == NDIS_STATUS_SUCCESS)
				{
					PMAC_TABLE_ENTRY	pEntry = NULL;
					PEASY_CONFIG_INFO	pEasyConf = &pAd->ApCfg.MBSSID[pObj->ioctl_if].EasyConfigInfo;
					
					DBGPRINT(RT_DEBUG_TRACE,("Query::OID_PIN_OF_ENROLLEE(STA - %02X:%02X:%02X:%02X:%02X:%02X)\n",
												pStaMacAddr[0],
												pStaMacAddr[1],
												pStaMacAddr[2],
												pStaMacAddr[3],
												pStaMacAddr[4],
												pStaMacAddr[5]));
					pEntry = MacTableLookup(pAd, pStaMacAddr);
					if (pEntry && pEntry->bRaAutoWpsCapable)
					{
						wrq->u.data.length = 8;
						Status = copy_to_user(wrq->u.data.pointer, pEasyConf->WpsPinCode, wrq->u.data.length);
					}
					else
						wrq->u.data.length = 0;
				}
				os_free_mem(NULL, pStaMacAddr);
			}
			break;
#endif /* EASY_CONFIG_SETUP */

#ifdef WAC_SUPPORT
		case RT_OID_WAC_REQ:
			if (wrq->u.data.length < sizeof(WAC_REQUEST))
			{
				Status = -EINVAL;
			}
			else
			{
				WAC_IoctlReq(pAd, pObj->ioctl_if, wrq);
			}
			break;
#endif /* WAC_SUPPORT */

   		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::unknown IOCTL's subcmd = 0x%08x, apidx=%d\n", cmd, apidx));
			Status = -EOPNOTSUPP;
			break;
    }

	return Status;
}


#if 0 /* os abl move to common/rt_os_util.c */
INT32 ralinkrate[256] =
	{2,  4, 11, 22, 12, 18,   24,  36, 48, 72, 96, 108, 109, 110, 111, 112,
	13, 26,   39,  52,  78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260,
	39, 78,  117, 156, 234, 312, 351, 390,
	27, 54,   81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540,
	81, 162, 243, 324, 486, 648, 729, 810,
	14, 29,   43,  57,  87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288,
	43, 87,  130, 173, 260, 317, 390, 433,
	30, 60,   90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600,
	90, 180, 270, 360, 540, 720, 810, 900,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
	20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
	40,41,42,43,44,45,46,47}; /* 3*3 */

VOID RtmpDrvMaxRateGet(
	IN	RTMP_ADAPTER			*pAd,
	IN	PHTTRANSMIT_SETTING		pHtPhyMode,
	OUT	UINT32					*pRate)
{
	int rate_index = 0;

#ifdef DOT11_N_SUPPORT
	if (pHtPhyMode->field.MODE >= MODE_HTMIX)
	{
	/*                	rate_index = 16 + ((UCHAR)pHtPhyMode->field.BW *16) + ((UCHAR)pHtPhyMode->field.ShortGI *32) + ((UCHAR)pHtPhyMode->field.MCS); */
		rate_index = 16 + ((UCHAR)pHtPhyMode->field.BW *24) + ((UCHAR)pHtPhyMode->field.ShortGI *48) + ((UCHAR)pHtPhyMode->field.MCS);
	}
	else 
#endif /* DOT11_N_SUPPORT */
		if (pHtPhyMode->field.MODE == MODE_OFDM)
			rate_index = (UCHAR)(pHtPhyMode->field.MCS) + 4;
		else 
			rate_index = (UCHAR)(pHtPhyMode->field.MCS);

	if (rate_index < 0)
		rate_index = 0;

	if (rate_index > 255)
		rate_index = 255;
    
	*pRate = ralinkrate[rate_index] * 500000;
}
#endif /* 0 */


/* 
    ==========================================================================
    Description:
        Set Country Code.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_CountryCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{

	if(strlen(arg) == 2)
	{
		NdisMoveMemory(pAd->CommonCfg.CountryCode, arg, 2);
		pAd->CommonCfg.bCountryFlag = TRUE;
	}
	else
	{
		NdisZeroMemory(pAd->CommonCfg.CountryCode, sizeof(pAd->CommonCfg.CountryCode));
		pAd->CommonCfg.bCountryFlag = FALSE;
	}	
		
	DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryCode_Proc::(bCountryFlag=%d, CountryCode=%s)\n", pAd->CommonCfg.bCountryFlag, pAd->CommonCfg.CountryCode));

	return TRUE;
}

#ifdef EXT_BUILD_CHANNEL_LIST
INT Set_ChGeography_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Geography;
		
	Geography = simple_strtol(arg, 0, 10);
	if (Geography <= BOTH)
		pAd->CommonCfg.Geography = Geography;
	else
		DBGPRINT(RT_DEBUG_ERROR, ("Set_ChannelGeography_Proc::(wrong setting. 0: Out-door, 1: in-door, 2: both)\n"));

	pAd->CommonCfg.CountryCode[2] =
		(pAd->CommonCfg.Geography == BOTH) ? ' ' : ((pAd->CommonCfg.Geography == IDOR) ? 'I' : 'O');

	DBGPRINT(RT_DEBUG_ERROR, ("Set_ChannelGeography_Proc:: Geography = %s\n", pAd->CommonCfg.Geography == ODOR ? "out-door" : (pAd->CommonCfg.Geography == IDOR ? "in-door" : "both")));
	
	/* After Set ChGeography need invoke SSID change procedural again for Beacon update. */
	/* it's no longer necessary since APStartUp will rebuild channel again. */
	/*BuildChannelListEx(pAd); */

	return TRUE;			
}
#endif /* EXT_BUILD_CHANNEL_LIST */


/*
    ==========================================================================
    Description:
        Set Country String.
        This command will not work, if the field of CountryRegion in eeprom is programmed.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_CountryString_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT   index = 0;
	INT   success = TRUE;
	RTMP_STRING name_buffer[40] = {0};

#ifdef EXT_BUILD_CHANNEL_LIST
	return -EOPNOTSUPP;
#endif /* EXT_BUILD_CHANNEL_LIST */

	if(strlen(arg) <= 38)
	{
		if (strlen(arg) < 4)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::Parameter of CountryString are too short !\n"));
			return FALSE;
		}
		
		for (index = 0; index < strlen(arg); index++)
		{
			if ((arg[index] >= 'a') && (arg[index] <= 'z'))
				arg[index] = toupper(arg[index]);
		}

		for (index = 0; index < NUM_OF_COUNTRIES; index++)
		{
			NdisZeroMemory(name_buffer, 40);
			snprintf(name_buffer, sizeof(name_buffer), "\"%s\"", (RTMP_STRING *) allCountry[index].pCountryName);

			if (strncmp((RTMP_STRING *) allCountry[index].pCountryName, arg, strlen(arg)) == 0)
				break;
			else if (strncmp(name_buffer, arg, strlen(arg)) == 0)
				break;
		}

		if (index == NUM_OF_COUNTRIES)
			success = FALSE;
	}
	else
	{
		success = FALSE;
	}			

	if (success == TRUE)
	{
		if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode))
		{
				if (pAd->CommonCfg.CountryRegionForABand & 0x80)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::parameter of CountryRegion in eeprom is programmed \n"));
					success = FALSE;
				}
				else
				{
					if (allCountry[index].SupportGBand == TRUE)
					{
						NdisZeroMemory(pAd->CommonCfg.CountryCode, sizeof(pAd->CommonCfg.CountryCode));
						NdisMoveMemory(pAd->CommonCfg.CountryCode, allCountry[index].IsoName, 2);
						pAd->CommonCfg.CountryCode[2] = ' ';

						pAd->CommonCfg.bCountryFlag = TRUE;

						pAd->CommonCfg.CountryRegion = (UCHAR) allCountry[index].RegDomainNum11G;

						/* After Set ChGeography need invoke SSID change procedural again for Beacon update. */
						/* it's no longer necessary since APStartUp will rebuild channel again. */
						/*BuildChannelList(pAd); */

						success = TRUE;
					}
					else
					{
						success = FALSE;
						DBGPRINT(RT_DEBUG_TRACE, ("The Country are not Support G Band Channel\n"));
					}
				}
		}
		else if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
		{
				if (pAd->CommonCfg.CountryRegion & 0x80)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::parameter of CountryRegion in eeprom is programmed \n"));
					success = FALSE;
				}
				else
				{
					if (allCountry[index].SupportABand == TRUE)
					{
						NdisZeroMemory(pAd->CommonCfg.CountryCode, sizeof(pAd->CommonCfg.CountryCode));
						NdisMoveMemory(pAd->CommonCfg.CountryCode, allCountry[index].IsoName, 2);
						pAd->CommonCfg.CountryCode[2] = ' ';

						pAd->CommonCfg.bCountryFlag = TRUE;

						pAd->CommonCfg.CountryRegionForABand = (UCHAR) allCountry[index].RegDomainNum11A;

						/* After Set ChGeography need invoke SSID change procedural again for Beacon update. */
						/* it's no longer necessary since APStartUp will rebuild channel again. */
						/*BuildChannelList(pAd); */

						success = TRUE;
					}
					else
					{
						success = FALSE;
						DBGPRINT(RT_DEBUG_TRACE, ("The Country are not Support A Band Channel\n"));
					}
				}
		}
		else
				success = FALSE;
	}

	if (success == TRUE)
	{
		/* if set country string, driver needs to be reset */
		DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryString_Proc::(CountryString=%s CountryRegin=%d CountryCode=%s)\n", 
							allCountry[index].pCountryName, pAd->CommonCfg.CountryRegion, pAd->CommonCfg.CountryCode));
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::Parameters out of range\n"));
	}

	return success;
}


/* 
    ==========================================================================
    Description:
        Set SSID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_SSID_Proc(RTMP_ADAPTER *pAdapter, RTMP_STRING *arg)
{
	INT   success = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAdapter->OS_Cookie;
	BSS_STRUCT *pMBSSStruct;

	if(strlen(arg) <= MAX_LEN_OF_SSID)
	{
		pMBSSStruct = &pAdapter->ApCfg.MBSSID[pObj->ioctl_if];
		NdisZeroMemory(pMBSSStruct->Ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pMBSSStruct->Ssid, arg, strlen(arg));
		pMBSSStruct->SsidLen = (UCHAR)strlen(arg);
		success = TRUE;


#ifdef P2P_SUPPORT
		if (pObj->ioctl_if_type == INT_P2P)
		{
			RT_CfgSetWPAPSKKey( pAdapter, 
								pAdapter->P2pCfg.PhraseKey, 
								pAdapter->P2pCfg.PhraseKeyLen, 
								(PUCHAR)pMBSSStruct->Ssid, 
								pMBSSStruct->SsidLen, 
								pMBSSStruct->PMK);
			
			NdisZeroMemory(pAdapter->P2pCfg.SSID, MAX_LEN_OF_SSID);
			NdisMoveMemory(pAdapter->P2pCfg.SSID, arg, strlen(arg));
			pAdapter->P2pCfg.SSIDLen = (UCHAR)strlen(arg);
			
			if (P2P_GO_ON(pAdapter))
			{
				P2P_GoStop(pAdapter);
				P2P_GoStartUp(pAdapter, MAIN_MBSSID);
				DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_SSID_Proc::(Len=%d,Ssid=%s)\n", pObj->ioctl_if,
					pMBSSStruct->SsidLen, pMBSSStruct->Ssid));
			}
		}
		else
#endif /* P2P_SUPPORT */
		{
			/* If in detection mode, need to stop detect first. */
			if (pAdapter->CommonCfg.bIEEE80211H == FALSE)
			{
				APStop(pAdapter);
				APStartUp(pAdapter);
			}
			else
			{
				/* each mode has different restart method */
				if (pAdapter->Dot11_H.RDMode == RD_SILENCE_MODE)
				{
					APStop(pAdapter);
					APStartUp(pAdapter);
				}
				else if (pAdapter->Dot11_H.RDMode == RD_SWITCHING_MODE)
				{
				}
				else if (pAdapter->Dot11_H.RDMode == RD_NORMAL_MODE)
				{
					APStop(pAdapter);
					APStartUp(pAdapter);
					AsicEnableBssSync(pAdapter, pAd->CommonCfg.BeaconPeriod);
				}
			}

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_SSID_Proc::(Len=%d,Ssid=%s)\n", pObj->ioctl_if,
				pAdapter->ApCfg.MBSSID[pObj->ioctl_if].SsidLen, pAdapter->ApCfg.MBSSID[pObj->ioctl_if].Ssid));
		}
	}
	else
		success = FALSE;

	return success;
}


/* 
    ==========================================================================
    Description:
        Set TxRate
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_TxRate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	NdisZeroMemory(pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRates, MAX_LEN_OF_SUPPORTED_RATES);

	pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRatesIndex = simple_strtol(arg, 0, 10);
	/* todo RTMPBuildDesireRate(pAd, pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRatesIndex); */
	
	/*todo MlmeUpdateTxRates(pAd); */

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set BasicRate
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_BasicRate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	ULONG BasicRateBitmap;

    BasicRateBitmap = (ULONG) simple_strtol(arg, 0, 10);

    if (BasicRateBitmap > 4095) /* (2 ^ MAX_LEN_OF_SUPPORTED_RATES) -1 */
        return FALSE;

    pAd->CommonCfg.BasicRateBitmap = BasicRateBitmap;
    pAd->CommonCfg.BasicRateBitmapOld = BasicRateBitmap;

    MlmeUpdateTxRates(pAd, FALSE, (UCHAR)pObj->ioctl_if);

    DBGPRINT(RT_DEBUG_TRACE, ("Set_BasicRate_Proc::(BasicRateBitmap=0x%08lx)\n", pAd->CommonCfg.BasicRateBitmap));
    
    return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set Beacon Period
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_BeaconPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	USHORT BeaconPeriod;
	INT   success = FALSE;

	BeaconPeriod = (USHORT) simple_strtol(arg, 0, 10);
	if((BeaconPeriod >= 20) && (BeaconPeriod < 1024))
	{
		pAd->CommonCfg.BeaconPeriod = BeaconPeriod;
		success = TRUE;

#ifdef AP_QLOAD_SUPPORT
		/* re-calculate QloadBusyTimeThreshold */
		QBSS_LoadAlarmReset(pAd);
#endif /* AP_QLOAD_SUPPORT */
	}
	else
		success = FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BeaconPeriod_Proc::(BeaconPeriod=%d)\n", pAd->CommonCfg.BeaconPeriod));

	return success;
}

/* 
    ==========================================================================
    Description:
        Set Dtim Period
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_DtimPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	USHORT DtimPeriod;
	INT   success = FALSE;

	DtimPeriod = (USHORT) simple_strtol(arg, 0, 10);
	if((DtimPeriod >= 1) && (DtimPeriod <= 255))
	{
		pAd->ApCfg.DtimPeriod = DtimPeriod;
		success = TRUE;
	}
	else
		success = FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_DtimPeriod_Proc::(DtimPeriod=%d)\n", pAd->ApCfg.DtimPeriod));

	return success;
}

#ifdef RT305x
INT Set_RfRead_Proc(RTMP_ADAPTER *pAdapter, RTMP_STRING *arg)
{
	int i;
	UCHAR Value;
	
	for (i = 0; i < 32; i++)
	{
		RT30xxReadRFRegister(pAdapter, i, &Value);
		printk("%02x ", Value);
		if (((i + 1) % 4) == 0)
			printk("\n");
	}
	return TRUE;
}

INT Set_RfWrite_Proc(RTMP_ADAPTER *pAdapter, RTMP_STRING *arg)
{
	ULONG offset = 0;
	ULONG value = 0;
	PUCHAR p2 = (PUCHAR)arg;
	
	while((*p2 != ':') && (*p2 != '\0'))
	{
		p2++;
	}
	
	if (*p2 == ':')
	{
		A2Hex(offset, arg);
		A2Hex(value, p2+ 1);
	}
	else
	{
		A2Hex(value, arg);
	}
	
	if (offset >= 32)
	{
		return FALSE;
	}
	
	RT30xxWriteRFRegister(pAdapter, offset, value);

	return TRUE;
}
#endif /* RT305x */


/* 
    ==========================================================================
    Description:
        Disable/enable OLBC detection manually
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_OLBCDetection_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	switch (simple_strtol(arg, 0, 10))
	{
		case 0: /*enable OLBC detect */
			pAd->CommonCfg.DisableOLBCDetect = 0;
			break;
		case 1: /*disable OLBC detect */
			pAd->CommonCfg.DisableOLBCDetect = 1;
			break;
		default:  /*Invalid argument */
			return FALSE;
	}

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WmmCapable Enable or Disable
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_WmmCapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN	bWmmCapable;
	POS_COOKIE	pObj= (POS_COOKIE)pAd->OS_Cookie;

	bWmmCapable = simple_strtol(arg, 0, 10);

	if (bWmmCapable == 1)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable = TRUE;
	else if (bWmmCapable == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable = FALSE;
	else
		return FALSE;  /*Invalid argument */

	pAd->ApCfg.MBSSID[pObj->ioctl_if].bWmmCapableOrg = \
								pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable;
	
#ifdef RTL865X_FAST_PATH	
	if (!isFastPathCapable(pAd)) {
		rtlairgo_fast_tx_unregister();
		rtl865x_extDev_unregisterUcastTxDev(pAd->net_dev);		
	}
#endif

#ifdef DOT11_N_SUPPORT
	/*Sync with the HT relate info. In N mode, we should re-enable it */
	SetCommonHT(pAd);
#endif /* DOT11_N_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WmmCapable_Proc::(bWmmCapable=%d)\n", 
		pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable));

	return TRUE;
}


INT	Set_AP_MaxStaNum_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT			apidx = pObj->ioctl_if;

	return ApCfg_Set_MaxStaNum_Proc(pAd, apidx, arg);
}

/* 
    ==========================================================================
    Description:
        Set session idle timeout
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_IdleTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	return ApCfg_Set_IdleTimeout_Proc(pAd, arg);
}
/* 
    ==========================================================================
    Description:
        Set No Forwarding Enable or Disable
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_NoForwarding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG NoForwarding;

	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	NoForwarding = simple_strtol(arg, 0, 10);

	if (NoForwarding == 1)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].IsolateInterStaTraffic = TRUE;
	else if (NoForwarding == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].IsolateInterStaTraffic = FALSE;
	else
		return FALSE;  /*Invalid argument */
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_NoForwarding_Proc::(NoForwarding=%ld)\n", 
		pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].IsolateInterStaTraffic));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set No Forwarding between each SSID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_NoForwardingBTNSSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG NoForwarding;

	NoForwarding = simple_strtol(arg, 0, 10);

	if (NoForwarding == 1)
		pAd->ApCfg.IsolateInterStaTrafficBTNBSSID = TRUE;
	else if (NoForwarding == 0)
		pAd->ApCfg.IsolateInterStaTrafficBTNBSSID = FALSE;
	else
		return FALSE;  /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_NoForwardingBTNSSID_Proc::(NoForwarding=%ld)\n", pAd->ApCfg.IsolateInterStaTrafficBTNBSSID));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set Hide SSID Enable or Disable
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_HideSSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN bHideSsid;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	bHideSsid = simple_strtol(arg, 0, 10);

	if (bHideSsid == 1)
		bHideSsid = TRUE;
	else if (bHideSsid == 0)
		bHideSsid = FALSE;
	else
		return FALSE;  /*Invalid argument */
	
	if (pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid != bHideSsid)
	{
		pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid = bHideSsid;
	}

#ifdef WSC_V2_SUPPORT
	if (pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.bEnableWpsV2)
		WscOnOff(pAd, pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid);
#endif /* WSC_V2_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_HideSSID_Proc::(HideSSID=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set VLAN's ID field
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_VLANID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.VLAN_VID = simple_strtol(arg, 0, 10);
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLANID_Proc::(VLAN_VID=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.VLAN_VID));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set VLAN's priority field
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_VLANPriority_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.VLAN_Priority = simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLANPriority_Proc::(VLAN_Priority=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.VLAN_Priority));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set Authentication mode
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG       i;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;

	if (apidx >= pAd->ApCfg.BssidNum)
		return FALSE;  

	/* Set Authentication mode */
	ApCfg_Set_AuthMode_Proc(pAd, apidx, arg);

	/* reset the portSecure for all entries */
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (IS_ENTRY_CLIENT(&pAd->MacTab.Content[i]))
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}

	/* reset the PortSecure this BSS */
	pAd->ApCfg.MBSSID[apidx].wdev.PortSecured = WPA_802_1X_PORT_NOT_SECURED;

	/* Default key index is always 2 in WPA mode */	
	if(pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
		pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId = 1;

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set Encryption Type
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	
	if ((strcmp(arg, "NONE") == 0) || (strcmp(arg, "none") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WepStatus = Ndis802_11WEPDisabled;
	else if ((strcmp(arg, "WEP") == 0) || (strcmp(arg, "wep") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WepStatus = Ndis802_11WEPEnabled;
	else if ((strcmp(arg, "TKIP") == 0) || (strcmp(arg, "tkip") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WepStatus = Ndis802_11TKIPEnable;
	else if ((strcmp(arg, "AES") == 0) || (strcmp(arg, "aes") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WepStatus = Ndis802_11AESEnable;
	else if ((strcmp(arg, "TKIPAES") == 0) || (strcmp(arg, "tkipaes") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WepStatus = Ndis802_11TKIPAESMix;
#ifdef WAPI_SUPPORT
	else if ((strcmp(arg, "SMS4") == 0) || (strcmp(arg, "sms4") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WepStatus = Ndis802_11EncryptionSMS4Enabled;
#endif /* WAPI_SUPPORT */		
	else
		return FALSE;

	if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus >= Ndis802_11TKIPEnable)
		pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId = 1;

	/* decide the group key encryption type */
	if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11TKIPAESMix)	
		pAd->ApCfg.MBSSID[apidx].wdev.GroupKeyWepStatus = Ndis802_11TKIPEnable;		
	else
		pAd->ApCfg.MBSSID[apidx].wdev.GroupKeyWepStatus = pAd->ApCfg.MBSSID[apidx].wdev.WepStatus;

	/* move to ap.c::APStartUp to process */
    /*RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx); */
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_EncrypType_Proc::(EncrypType=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].wdev.WepStatus));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WPA pairwise mix-cipher combination
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_WpaMixPairCipher_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;

	/*
		In WPA-WPA2 mix mode, it provides a more flexible cipher combination. 
		-	WPA-AES and WPA2-TKIP
		-	WPA-AES and WPA2-TKIPAES
		-	WPA-TKIP and WPA2-AES
		-	WPA-TKIP and WPA2-TKIPAES
		-	WPA-TKIPAES and WPA2-AES
		-	WPA-TKIPAES and WPA2-TKIP
		-	WPA-TKIPAES and WPA2-TKIPAES (default)																 																	
	 */									
	if ((strncmp(arg, "WPA_AES_WPA2_TKIPAES", 20) == 0) || (strncmp(arg, "wpa_aes_wpa2_tkipaes", 20) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_AES_WPA2_TKIPAES;																			
	else if ((strncmp(arg, "WPA_AES_WPA2_TKIP", 17) == 0) || (strncmp(arg, "wpa_aes_wpa2_tkip", 17) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_AES_WPA2_TKIP;								 						
	else if ((strncmp(arg, "WPA_TKIP_WPA2_AES", 17) == 0) || (strncmp(arg, "wpa_tkip_wpa2_aes", 17) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_TKIP_WPA2_AES;								
	else if ((strncmp(arg, "WPA_TKIP_WPA2_TKIPAES", 21) == 0) || (strncmp(arg, "wpa_tkip_wpa2_tkipaes", 21) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_TKIP_WPA2_TKIPAES;
	else if ((strncmp(arg, "WPA_TKIPAES_WPA2_AES", 20) == 0) || (strncmp(arg, "wpa_tkipaes_wpa2_aes", 20) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_TKIPAES_WPA2_AES;
	else if ((strncmp(arg, "WPA_TKIPAES_WPA2_TKIPAES", 24) == 0) || (strncmp(arg, "wpa_tkipaes_wpa2_tkipaes", 24) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_TKIPAES_WPA2_TKIPAES;
	else if ((strncmp(arg, "WPA_TKIPAES_WPA2_TKIP", 21) == 0) || (strncmp(arg, "wpa_tkipaes_wpa2_tkip", 21) == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher = WPA_TKIPAES_WPA2_TKIP;
	else
		return FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_WpaMixPairCipher_Proc=0x%02x\n", apidx, pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher));
	
	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WPA rekey interval value
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_RekeyInterval_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	INT32	val;

	val = simple_strtol(arg, 0, 10);

	if((val >= 10) && (val < MAX_REKEY_INTER))
		pAd->ApCfg.MBSSID[apidx].WPAREKEY.ReKeyInterval = val;
	else /*Default */
		pAd->ApCfg.MBSSID[apidx].WPAREKEY.ReKeyInterval = 3600;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_RekeyInterval_Proc=%ld\n", 
								apidx, pAd->ApCfg.MBSSID[apidx].WPAREKEY.ReKeyInterval));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WPA rekey method
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_RekeyMethod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	PRT_WPA_REKEY	pInfo = &pAd->ApCfg.MBSSID[apidx].WPAREKEY;
	
	if ((strcmp(arg, "TIME") == 0) || (strcmp(arg, "time") == 0))
		pInfo->ReKeyMethod = TIME_REKEY;
	else if ((strcmp(arg, "PKT") == 0) || (strcmp(arg, "pkt") == 0))
		pInfo->ReKeyMethod = PKT_REKEY;
	else if ((strcmp(arg, "DISABLE") == 0) || (strcmp(arg, "disable") == 0))
		pInfo->ReKeyMethod = DISABLE_REKEY;
	else
		pInfo->ReKeyMethod = DISABLE_REKEY;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_RekeyMethod_Proc=%ld\n", 
								apidx, pInfo->ReKeyMethod));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set PMK-cache period
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_PMKCachePeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	UINT32 val = simple_strtol(arg, 0, 10);

	pAd->ApCfg.MBSSID[apidx].PMKCachePeriod = val * 60 * OS_HZ;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_PMKCachePeriod_Proc=%ld\n", 
									apidx, pAd->ApCfg.MBSSID[apidx].PMKCachePeriod));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Default Key ID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG KeyIdx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  /*Invalid argument */
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId));

	return TRUE;
}


#if 0
/* 
    ==========================================================================
    Description:
        Set WEP KEY1
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: /*wep 40 Ascii type */
			pAd->SharedKey[apidx][0].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][0].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: /*wep 40 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][0].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][0].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: /*wep 104 Ascii type */
			pAd->SharedKey[apidx][0].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][0].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: /*wep 104 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][0].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][0].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: /*Invalid argument */
			pAd->SharedKey[apidx][0].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][0].CipherAlg = CipherAlg;

    /* Set keys (into ASIC) */
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   /* not support */
    else    /* Old WEP stuff */
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              0, 
                              pAd->SharedKey[apidx][0].CipherAlg, 
                              pAd->SharedKey[apidx][0].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY2
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: /*wep 40 Ascii type */
			pAd->SharedKey[apidx][1].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][1].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: /*wep 40 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][1].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][1].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: /*wep 104 Ascii type */
			pAd->SharedKey[apidx][1].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][1].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: /*wep 104 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][1].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][1].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: /*Invalid argument */
			pAd->SharedKey[apidx][1].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][1].CipherAlg = CipherAlg;

	/* Set keys (into ASIC) */
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   /* not support */
    else    /* Old WEP stuff */
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              1, 
                              pAd->SharedKey[apidx][1].CipherAlg, 
                              pAd->SharedKey[apidx][1].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY3
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: /*wep 40 Ascii type */
			pAd->SharedKey[apidx][2].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][2].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: /*wep 40 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][2].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][2].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: /*wep 104 Ascii type */
			pAd->SharedKey[apidx][2].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][2].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: /*wep 104 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][2].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][2].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: /*Invalid argument */
			pAd->SharedKey[apidx][2].KeyLen = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("IF(ra%d) Set_Key3_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][2].CipherAlg = CipherAlg;

	/* Set keys (into ASIC) */
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   /* not support */
    else    /* Old WEP stuff */
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              2, 
                              pAd->SharedKey[apidx][2].CipherAlg, 
                              pAd->SharedKey[apidx][2].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY4
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: /*wep 40 Ascii type */
			pAd->SharedKey[apidx][3].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][3].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: /*wep 40 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][3].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][3].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: /*wep 104 Ascii type */
			pAd->SharedKey[apidx][3].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][3].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: /*wep 104 Hex type */
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  /*Not Hex value; */
			}
			pAd->SharedKey[apidx][3].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][3].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: /*Invalid argument */
			pAd->SharedKey[apidx][3].KeyLen = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("IF(ra%d) Set_Key4_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][3].CipherAlg = CipherAlg;

	/* Set keys (into ASIC) */
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   /* not support */
    else    /* Old WEP stuff */
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              3, 
                              pAd->SharedKey[apidx][3].CipherAlg, 
                              pAd->SharedKey[apidx][3].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}
#else
/* 
    ==========================================================================
    Description:
        Set WEP KEY1
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR	apidx;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;		

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][0];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, 0);
	if (retVal == TRUE)
	{		
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, 0, pSharedKey);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s) success!\n", apidx, arg));
	}
	
	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY2
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR	apidx;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;	

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][1];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, 1);
	if (retVal == TRUE)
	{		
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, 1, pSharedKey);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s) success!\n", apidx, arg));
	}

	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY3
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR	apidx;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;	

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][2];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, 2);
	if (retVal == TRUE)
	{		
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, 2, pSharedKey);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s) success!\n", apidx, arg));
	}

	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY4
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{

	POS_COOKIE pObj;
	UCHAR	apidx;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;	

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][3];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, 3);
	if (retVal == TRUE)
	{		
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, 3, pSharedKey);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s) success!\n", apidx, arg));
	}

	return retVal;
}

#endif

/* 
    ==========================================================================
    Description:
        Set Access ctrol policy
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AccessPolicy_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	switch (simple_strtol(arg, 0, 10))
	{
		case 0: /*Disable */
			pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = 0;
			break;
		case 1: /* Allow All, and ACL is positive. */
			pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = 1;
			break;
		case 2: /* Reject All, and ACL is negative. */
			pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = 2;
			break;
		default: /*Invalid argument */
			DBGPRINT(RT_DEBUG_ERROR, ("Set_AccessPolicy_Proc::Invalid argument (=%s)\n", arg));		
			return FALSE;
	}

	/* check if the change in ACL affects any existent association */
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AccessPolicy_Proc::(AccessPolicy=%ld)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy));

	return TRUE;	
}


/* Replaced by Set_ACLAddEntry_Proc() and Set_ACLClearAll_Proc() */
#ifdef RELEASE_EXCLUDE
/* 
    ==========================================================================
    Description:
        Set Access control mac table list
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AccessControlList_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
	RT_802_11_ACL			acl;
	CHAR					*this_char;
	CHAR					*value;
	INT						i, j;
	BOOLEAN					isDuplicate=FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	NdisZeroMemory(&acl, sizeof(RT_802_11_ACL));
	
	this_char = (CHAR *)strsep((char **)&arg, ";");
	if (this_char == NULL)
		return FALSE;

	if (*this_char == '\0')
	{
		DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList%d entered is NULL ===> Make the list empty!\n", pObj->ioctl_if));
	}
	else
	{
		do
		{
			if (strlen((char *)this_char) != 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
			{
				return FALSE;
			}
	        for (i=0, value = (CHAR *)rstrtok((char *)this_char,":"); value; value = (CHAR *)rstrtok((char *)NULL,":")) 
			{
				if((strlen((char *)value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				{
					return FALSE;  /*Invalid */
				}
				AtoH((char *)value, &macAddr[i++], 1);
			}

			if (i != 6)
			{
				return FALSE;  /*Invalid */
			}

			/*Check if this entry is duplicate. */
			isDuplicate = FALSE;
			for (j=0; j<acl.Num; j++)
			{
				if (memcmp(acl.Entry[j].Addr, &macAddr, 6) == 0)
				{
					isDuplicate = TRUE;
				}
			}

			if (!isDuplicate)
			{
				NdisMoveMemory(acl.Entry[acl.Num++].Addr, &macAddr, 6);
			}

			if (acl.Num == MAX_NUM_OF_ACL_LIST)
		    {
				DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList is full, and no more entry can join the list!\n"));
	        	DBGPRINT(RT_DEBUG_WARN, ("The last entry of ACL is %02x:%02x:%02x:%02x:%02x:%02x\n",
	        		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]));
				break;
			}
		}while ((this_char = (CHAR *)strsep((char **)&arg, ";")) != NULL);
	}
	ASSERT(acl.Num <= MAX_NUM_OF_ACL_LIST);
	acl.Policy = pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy;
	NdisMoveMemory(&pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, &acl, sizeof(RT_802_11_ACL));

#if 0
	if (pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num > MAX_NUM_OF_ACL_LIST)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num = MAX_NUM_OF_ACL_LIST;
	else
		pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num = acl.Num;
#endif

	/* Check if the change in ACL affects any existent association. */
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::Set_AccessControlList_Proc(Policy=%ld, Entry#=%ld)\n",
        pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		printk("Entry #%02d: ", i+1);
		for (j=0; j<6; j++)
		   printk("%02X ", pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]);
		printk("\n");
	}
#endif
	return TRUE;
}
#endif /* RELEASE_EXCLUDE  */

/* 
    ==========================================================================
    Description:
        Add one entry or several entries(if allowed to)
        	into Access control mac table list
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ACLAddEntry_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
/*	RT_802_11_ACL			acl; */
	RT_802_11_ACL			*pacl = NULL;
	RTMP_STRING *this_char;
	RTMP_STRING *value;
	INT						i, j;
	BOOLEAN					isDuplicate=FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num >= (MAX_NUM_OF_ACL_LIST - 1))
    {
		DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList is full, and no more entry can join the list!\n"));		
		return FALSE;
	}

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&pacl, sizeof(RT_802_11_ACL));
	if (pacl == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return FALSE;
	}

	NdisZeroMemory(pacl, sizeof(RT_802_11_ACL));
	NdisMoveMemory(pacl, &pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
	
	while ((this_char = strsep((char **)&arg, ";")) != NULL)
	{
		if (*this_char == '\0')
		{
			DBGPRINT(RT_DEBUG_WARN, ("An unnecessary delimiter entered!\n"));
			continue;
		}
		if (strlen(this_char) != 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("illegal MAC address length!\n"));
			continue;
		}
        for (i=0, value = rstrtok(this_char,":"); value; value = rstrtok(NULL,":")) 
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
			{
				DBGPRINT(RT_DEBUG_ERROR, ("illegal MAC address format or octet!\n"));
				/* Do not use "continue" to replace "break" */
				break;
			}
			AtoH(value, &macAddr[i++], 1);
		}

		if (i != MAC_ADDR_LEN)
		{
			continue;
		}

		/* Check if this entry is duplicate. */
		isDuplicate = FALSE;
		for (j=0; j<pacl->Num; j++)
		{
			if (memcmp(pacl->Entry[j].Addr, &macAddr, 6) == 0)
			{
				isDuplicate = TRUE;
				DBGPRINT(RT_DEBUG_WARN, ("You have added an entry before :\n"));
	        	DBGPRINT(RT_DEBUG_WARN, ("The duplicate entry is %02x:%02x:%02x:%02x:%02x:%02x\n",
	        		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]));
			}
		}

		if (!isDuplicate)
		{
			NdisMoveMemory(pacl->Entry[pacl->Num++].Addr, &macAddr, MAC_ADDR_LEN);
		}

		if (pacl->Num == MAX_NUM_OF_ACL_LIST)
	    {
			DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList is full, and no more entry can join the list!\n"));
        	DBGPRINT(RT_DEBUG_WARN, ("The last entry of ACL is %02x:%02x:%02x:%02x:%02x:%02x\n",
        		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]));
			break;
		}
	}
	
	ASSERT(pacl->Num < MAX_NUM_OF_ACL_LIST);

	NdisZeroMemory(&pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
	NdisMoveMemory(&pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, pacl, sizeof(RT_802_11_ACL));

	/* check if the change in ACL affects any existent association */
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::%s(Policy=%ld, Entry#=%ld)\n",
		__FUNCTION__ , pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		printk("Entry #%02d: ", i+1);
		for (j=0; j<MAC_ADDR_LEN; j++)
		   printk("%02X ", pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]);
		printk("\n");
	}
#endif

	if (pacl != NULL)
		os_free_mem(NULL, pacl);

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Delete one entry or several entries(if allowed to)
        	from Access control mac table list
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ACLDelEntry_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
	UCHAR					nullAddr[MAC_ADDR_LEN];
	RT_802_11_ACL			acl;
	RTMP_STRING *this_char;
	RTMP_STRING *value;
	INT						i, j;
	BOOLEAN					isFound=FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	NdisZeroMemory(&acl, sizeof(RT_802_11_ACL));
	NdisMoveMemory(&acl, &pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
	NdisZeroMemory(nullAddr, MAC_ADDR_LEN);
	
	while ((this_char = strsep((char **)&arg, ";")) != NULL)
	{
		if (*this_char == '\0')
		{
			DBGPRINT(RT_DEBUG_WARN, ("An unnecessary delimiter entered!\n"));
			continue;
		}
		if (strlen(this_char) != 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("illegal MAC address length!\n"));
			continue;
		}

		for (i=0, value = rstrtok(this_char,":"); value; value = rstrtok(NULL,":")) 
		{
			if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
			{
				DBGPRINT(RT_DEBUG_ERROR, ("illegal MAC address format or octet!\n"));
				/* Do not use "continue" to replace "break" */
				break;
			}
			AtoH(value, &macAddr[i++], 1);
		}

		if (i != MAC_ADDR_LEN)
		{
			continue;
		}

		/* Check if this entry existed. */
		isFound = FALSE;
		for (j=0; j<acl.Num; j++)
		{
			if (memcmp(acl.Entry[j].Addr, &macAddr, MAC_ADDR_LEN) == 0)
			{
				isFound = TRUE;
				NdisZeroMemory(acl.Entry[j].Addr, MAC_ADDR_LEN);
				DBGPRINT(RT_DEBUG_TRACE, ("The entry %02x:%02x:%02x:%02x:%02x:%02x founded will be deleted!\n",
	        		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]));
			}
		}

		if (!isFound)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("The entry %02x:%02x:%02x:%02x:%02x:%02x is not in the list!\n",
        		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]));
		}
	}
	
	NdisZeroMemory(&pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = acl.Policy;
	ASSERT(pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num == 0);
	i = 0;

	for (j=0; j<acl.Num; j++)
	{
		if (memcmp(acl.Entry[j].Addr, &nullAddr, MAC_ADDR_LEN) == 0)
		{
			continue;			
		}
		else
		{
			NdisMoveMemory(&(pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i++]), acl.Entry[j].Addr, MAC_ADDR_LEN);
		}
	}

	pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num = i;
	ASSERT(acl.Num >= pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num);

	/* check if the change in ACL affects any existent association */
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::%s(Policy=%ld, Entry#=%ld)\n",
		__FUNCTION__ , pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		printk("Entry #%02d: ", i+1);
		for (j=0; j<MAC_ADDR_LEN; j++)
		   printk("%02X ", pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]);
		printk("\n");
	}
#endif
	return TRUE;
}


/* for ACL policy message */
#define ACL_POLICY_TYPE_NUM	3
char const *pACL_PolicyMessage[ACL_POLICY_TYPE_NUM] = {   	
	"the Access Control feature is disabled",						/* 0 : Disable */
	"only the following entries are allowed to join this BSS",			/* 1 : Allow */
	"all the following entries are rejected to join this BSS",			/* 2 : Reject */
};


/* 
    ==========================================================================
    Description:
        Dump all the entries in the Access control 
        	mac table list of a specified BSS
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ACLShowAll_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	RT_802_11_ACL			acl;
	BOOLEAN					bDumpAll=FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT						i, j;
	
	bDumpAll = simple_strtol(arg, 0, 10);

	if (bDumpAll == 1)
	{
		bDumpAll = TRUE;
	}
	else if (bDumpAll == 0)
	{
		bDumpAll = FALSE;
		DBGPRINT(RT_DEBUG_WARN, ("Your input is 0!\n"));
		DBGPRINT(RT_DEBUG_WARN, ("The Access Control List will not be dumped!\n"));
		return TRUE;
	}
	else
	{
		return FALSE;  /* Invalid argument */
	}

	NdisZeroMemory(&acl, sizeof(RT_802_11_ACL));
	NdisMoveMemory(&acl, &pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
	
	/* Check if the list is already empty. */
	if (acl.Num == 0)
	{
		DBGPRINT(RT_DEBUG_WARN, ("The Access Control List is empty!\n"));
		return TRUE;
	}

	ASSERT(((bDumpAll == 1) && (acl.Num > 0)));

	/* Show the corresponding policy first. */
	printk("=============== Access Control Policy ===============\n");
	printk("Policy is %ld : ", acl.Policy);
	printk("%s\n", pACL_PolicyMessage[acl.Policy]);

	/* Dump the entry in the list one by one */
	printk("===============  Access Control List  ===============\n");
	for (i=0; i<acl.Num; i++)
	{
		printk("Entry #%02d: ", i+1);
		for (j=0; j<MAC_ADDR_LEN; j++)
		   printk("%02X ", acl.Entry[i].Addr[j]);
		printk("\n");
	}
	
	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Clear all the entries in the Access control 
        	mac table list of a specified BSS
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ACLClearAll_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
/*	RT_802_11_ACL			acl; */
	RT_802_11_ACL			*pacl = NULL;
	BOOLEAN					bClearAll=FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	bClearAll = simple_strtol(arg, 0, 10);

	if (bClearAll == 1)
	{
		bClearAll = TRUE;
	}
	else if (bClearAll == 0)
	{
		bClearAll = FALSE;
		DBGPRINT(RT_DEBUG_WARN, ("Your input is 0!\n"));
		DBGPRINT(RT_DEBUG_WARN, ("The Access Control List will be kept unchanged!\n"));
		return TRUE;
	}
	else
	{
		return FALSE;  /* Invalid argument */
	}

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&pacl, sizeof(RT_802_11_ACL));
	if (pacl == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return FALSE;
	}

	NdisZeroMemory(pacl, sizeof(RT_802_11_ACL));
	NdisMoveMemory(pacl, &pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
	
	/* Check if the list is already empty. */
	if (pacl->Num == 0)
	{
		DBGPRINT(RT_DEBUG_WARN, ("The Access Control List is empty!\n"));
		DBGPRINT(RT_DEBUG_WARN, ("No need to clear the Access Control List!\n"));
		return TRUE;
	}

	ASSERT(((bClearAll == 1) && (pacl->Num > 0)));

	/* Clear the entry in the list one by one */
	/* Keep the corresponding policy unchanged. */
	do
	{
		NdisZeroMemory(pacl->Entry[pacl->Num - 1].Addr, MAC_ADDR_LEN);
		pacl->Num -= 1;
	}while (pacl->Num > 0);
	
	ASSERT(pacl->Num == 0);

	NdisZeroMemory(&(pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList), sizeof(RT_802_11_ACL));
	NdisMoveMemory(&(pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList), pacl, sizeof(RT_802_11_ACL));

	/* check if the change in ACL affects any existent association */
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);

	if (pacl != NULL)
		os_free_mem(NULL, pacl);

	DBGPRINT(RT_DEBUG_TRACE, ("Set::%s(Policy=%ld, Entry#=%ld)\n",
		__FUNCTION__, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

	return TRUE;
}

#ifdef DBG
static void _rtmp_hexdump(int level, const char *title, const UINT8 *buf,
			 size_t len, int show)
{
	size_t i;
	if (level < RTDebugLevel)
		return;
	printk("%s - hexdump(len=%lu):", title, (unsigned long) len);
	if (show) {
		for (i = 0; i < len; i++)
			printk(" %02x", buf[i]);
	} else {
		printk(" [REMOVED]");
	}
	printk("\n");
}

void rtmp_hexdump(int level, const char *title, const UINT8 *buf, size_t len)
{
	_rtmp_hexdump(level, title, buf, len, 1);
}
#endif



#if 0
/* 
    ==========================================================================
    Description:
        Set WPA PSK key

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 WPA pre-shared key string

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_WPAPSK_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	UCHAR       keyMaterial[40];
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WPAPSK_Proc::(WPAPSK=%s)\n", arg));

	if ((strlen(arg) < 8) || (strlen(arg) > 64))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set failed!!(WPAPSK=%s), WPAPSK key-string required 8 ~ 64 characters \n", arg));
		return FALSE;
	}

	if (strlen(arg) == 64)
	{
	    AtoH(arg, pAd->ApCfg.MBSSID[apidx].PMK, 32);
	}
	else
	{
	    RtmpPasswordHash(arg, (PUCHAR)pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen, keyMaterial);
	    NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].PMK, keyMaterial, 32);		
	}

#ifdef WSC_AP_SUPPORT
    NdisZeroMemory(pAd->ApCfg.MBSSID[apidx].WscControl.WpaPsk, 64);
    pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen = 0;
    pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen = strlen(arg);
    NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].WscControl.WpaPsk, arg, pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen);    
#endif /* WSC_AP_SUPPORT */    

	return TRUE;
}
#else
/* 
    ==========================================================================
    Description:
        Set WPA PSK key

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 WPA pre-shared key string

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_WPAPSK_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;
	INT	retval;
	BSS_STRUCT *pMBSSStruct;
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_WPAPSK_Proc::(WPAPSK=%s)\n", arg));

	pMBSSStruct = &pAd->ApCfg.MBSSID[apidx];
	retval = RT_CfgSetWPAPSKKey(pAd, arg, strlen(arg), (PUCHAR)pMBSSStruct->Ssid, pMBSSStruct->SsidLen, pMBSSStruct->PMK);
	if (retval == FALSE)
		return FALSE;

#ifdef WSC_AP_SUPPORT
    NdisZeroMemory(pMBSSStruct->WscControl.WpaPsk, 64);
    pMBSSStruct->WscControl.WpaPskLen = 0;
    pMBSSStruct->WscControl.WpaPskLen = strlen(arg);
	if (P2P_GO_ON(pAd))
	pMBSSStruct->WscControl.WscConfStatus = WSC_SCSTATE_CONFIGURED;
    NdisMoveMemory(pMBSSStruct->WscControl.WpaPsk, arg, pMBSSStruct->WscControl.WpaPskLen);    
#endif /* WSC_AP_SUPPORT */    

#ifdef P2P_SUPPORT
	if (pObj->ioctl_if_type == INT_P2P)
	{
		PRT_P2P_CONFIG pP2pCfg = &pAd->P2pCfg;
		NdisZeroMemory(pP2pCfg->PhraseKey, 64);
		pP2pCfg->PhraseKeyLen = 0;
		pP2pCfg->PhraseKeyLen = strlen(arg);
		NdisMoveMemory(pP2pCfg->PhraseKey, arg, pP2pCfg->PhraseKeyLen);
	}
#endif /* P2P_SUPPORT */

	return TRUE;
}
#endif

/* 
    ==========================================================================
    Description:
        Reset statistics counter

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/

INT	Set_RadioOn_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	UCHAR radio;

	radio = simple_strtol(arg, 0, 10);

	if (radio)
	{
		MlmeRadioOn(pAd);
		DBGPRINT(RT_DEBUG_TRACE, ("==>Set_RadioOn_Proc (ON)\n"));
	}
	else
	{
		MlmeRadioOff(pAd);
		DBGPRINT(RT_DEBUG_TRACE, ("==>Set_RadioOn_Proc (OFF)\n"));
	}
	
	return TRUE;
}

#ifdef AP_SCAN_SUPPORT
/* 
    ==========================================================================
    Description:
        Issue a site survey command to driver
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 set site_survey
    ==========================================================================
*/
#if 0
INT Set_SiteSurvey_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	NDIS_802_11_SSID Ssid;
    NdisZeroMemory(&Ssid, sizeof(NDIS_802_11_SSID));
	if ((strlen(arg) != 0) && (strlen(arg) <= MAX_LEN_OF_SSID))
    {
        NdisMoveMemory(Ssid.Ssid, arg, strlen(arg));
        Ssid.SsidLength = strlen(arg);
	}

	if (Ssid.SsidLength == 0)
		ApSiteSurvey(pAd, &Ssid, SCAN_PASSIVE, FALSE);
	else
		ApSiteSurvey(pAd, &Ssid, SCAN_ACTIVE, FALSE);
    
    DBGPRINT(RT_DEBUG_TRACE, ("Set_SiteSurvey_Proc\n"));

    return TRUE;
}
#endif

/* 
    ==========================================================================
    Description:
        Issue a Auto-Channel Selection command to driver
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None
    
    Note:
        Usage: 
               1.) iwpriv ra0 set AutoChannelSel=1
                   Ues the number of AP to choose
               2.) iwpriv ra0 set AutoChannelSel=2
                   Ues the False CCA count to choose
    ==========================================================================
*/
INT Set_AutoChannelSel_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	NDIS_802_11_SSID Ssid;


	NdisZeroMemory(&Ssid, sizeof(NDIS_802_11_SSID));
	if (strlen(arg) <= MAX_LEN_OF_SSID)
	{
		if (strlen(arg) != 0)
		{
			NdisMoveMemory(Ssid.Ssid, arg, strlen(arg));
			Ssid.SsidLength = strlen(arg);
		}
		else   /*ANY ssid */
		{
			Ssid.SsidLength = 0; 
			memcpy(Ssid.Ssid, "", 0);
		}
	}
	if (strcmp(arg,"1") == 0)
		pAd->ApCfg.AutoChannelAlg = ChannelAlgApCnt;
	else if (strcmp(arg,"2") == 0)
		pAd->ApCfg.AutoChannelAlg = ChannelAlgCCA;
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Set_AutoChannelSel_Proc Alg isn't defined\n"));
		return FALSE;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("Set_AutoChannelSel_Proc Alg=%d \n", pAd->ApCfg.AutoChannelAlg));
	if (Ssid.SsidLength == 0)
		ApSiteSurvey(pAd, &Ssid, SCAN_PASSIVE, TRUE);
	else
		ApSiteSurvey(pAd, &Ssid, SCAN_ACTIVE, TRUE);
    
    return TRUE;

}

#endif /* AP_SCAN_SUPPORT */

INT Show_DriverInfo_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
    printk("Driver version: %s\n", AP_DRIVER_VERSION);
    
    return TRUE;
}

#if 0
INT	Show_MacTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	INT i;
    	UINT32 RegValue;
	ULONG DataRate=0;
#ifdef DPA_S //yiwei
	UCHAR index;
	BOOLEAN bFound;
#endif  /* DPA_S */
	
	printk("\n");
	RTMP_IO_READ32(pAd, BKOFF_SLOT_CFG, &RegValue);
	printk("BackOff Slot      : %s slot time, BKOFF_SLOT_CFG(0x1104) = 0x%08x\n", 
			OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED) ? "short" : "long",
 			RegValue);

#ifdef DOT11_N_SUPPORT
	printk("HT Operating Mode : %d\n", pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode);
	printk("\n");
#endif /* DOT11_N_SUPPORT */
	
#ifdef DPA_S //yiwei
	printk("\n%-19s%-4s%-4s%-4s%-4s%-8s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s%-7s%-7s%-32s\n",
		   "MAC", "AID", "BSS", "PSM", "WMM", "MIMOPS", "RSSI0", "RSSI1", 
		   "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate", "DevName");
#else /* DPA_S */
	printk("\n%-19s%-4s%-4s%-4s%-4s%-8s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s%-7s%-7s\n",
		   "MAC", "AID", "BSS", "PSM", "WMM", "MIMOPS", "RSSI0", "RSSI1", 
		   "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate");
#endif /* !DPA_S */
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

#ifdef DPA_S
		if ((IS_P2P_GO_ENTRY(pEntry) || IS_P2P_CLI_ENTRY(pEntry)) 
			&& (pEntry->PortSecured == WPA_802_1X_PORT_SECURED))
#else /* DPA_S */
		if ((IS_ENTRY_CLIENT(pEntry) || IS_ENTRY_APCLI(pEntry))
			&& (pEntry->Sst == SST_ASSOC))
#endif /* !DPA_S */
		{
			DataRate=0;
			getRate(pEntry->HTPhyMode, &DataRate);

#ifdef DPA_S //yiwei
			if (IS_P2P_GO_ENTRY(pEntry))
			{
				index = P2pGroupTabSearch(pAd, pEntry->Addr);
				if(index != P2P_NOT_FOUND)
					bFound = TRUE;
				else
					bFound = FALSE;
			}
			else
			{
				bFound = FALSE;
			}
#endif

			printk("%02X:%02X:%02X:%02X:%02X:%02X  ", PRINT_MAC(pEntry->Addr));
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->apidx);
			printk("%-4d", (int)pEntry->PsMode);
			printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
#ifdef DOT11_N_SUPPORT
			printk("%-8d", (int)pEntry->MmpsMode);
#endif /* DOT11_N_SUPPORT */
			printk("%-7d", pEntry->RssiSample.AvgRssi0);
			printk("%-7d", pEntry->RssiSample.AvgRssi1);
			printk("%-7d", pEntry->RssiSample.AvgRssi2);
			printk("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE));
			printk("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW));
			printk("%-6d", pEntry->HTPhyMode.field.MCS);
			printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-6d", pEntry->HTPhyMode.field.STBC);
			printk("%-7d", (int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount));
			printk("%-7d", (int)DataRate);
			printk("%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount, 
						(pEntry->DebugTxCount) ? ((pEntry->DebugTxCount-pEntry->DebugFIFOCount)*100/pEntry->DebugTxCount) : 0);

			printk("\t\t\t\t%-10s", get_phymode_str(pEntry->MaxHTPhyMode.field.MODE));
			printk("%-6s", get_bw_str(pEntry->MaxHTPhyMode.field.BW));
			printk("%-6d", pEntry->MaxHTPhyMode.field.MCS);
			printk("%-6d", pEntry->MaxHTPhyMode.field.ShortGI);
			printk("%-6d\n", pEntry->MaxHTPhyMode.field.STBC);
			
#ifdef DPA_S //yiwei
			printk("%-32s%\n", bFound ? &pAd->P2pTable.Client[index].DeviceName[0]:"N/A");
#endif  /* DPA_S */
			printk("\n");
		}
	} 

	return TRUE;
}
#endif

INT	Show_StaCount_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	INT i;/*, QueIdx=0; */
    	UINT32 RegValue;
	
	printk("\n");
	RTMP_IO_READ32(pAd, BKOFF_SLOT_CFG, &RegValue);
	printk("BackOff Slot      : %s slot time, BKOFF_SLOT_CFG(0x1104) = 0x%08x\n", 
			OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED) ? "short" : "long",
 			RegValue);

#ifdef DOT11_N_SUPPORT
	printk("HT Operating Mode : %d\n", pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode);
	printk("\n");
#endif /* DOT11_N_SUPPORT */
	
	printk("\n%-19s%-4s%-12s%-12s%-12s%-12s\n",
		   "MAC", "AID","TxPackets","RxPackets","TxBytes","RxBytes");
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((IS_ENTRY_CLIENT(pEntry) || IS_ENTRY_APCLI(pEntry))
			&& (pEntry->Sst == SST_ASSOC))
		{
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d", (int)pEntry->Aid);
			printk("%-12ld",(ULONG)pEntry->TxPackets.QuadPart);
			printk("%-12ld", (ULONG)pEntry->RxPackets.QuadPart);
			printk("%-12ld", (ULONG)pEntry->TxBytes);
			printk("%-12ld", (ULONG)pEntry->RxBytes);
			printk("\n");
		}
	} 

	return TRUE;
}

INT	Show_StaSecurityInfo_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	INT i;
	UCHAR	apidx;
    	
	printk("\n");
	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		printk(" BSS(%d) AuthMode(%d)=%s, WepStatus(%d)=%s, GroupWepStatus(%d)=%s, WPAMixPairCipher(0x%02X)\n", 
							apidx, 
							pAd->ApCfg.MBSSID[apidx].wdev.AuthMode, 
							GetAuthMode(pAd->ApCfg.MBSSID[apidx].wdev.AuthMode), 
							pAd->ApCfg.MBSSID[apidx].wdev.WepStatus, 
							GetEncryptType(pAd->ApCfg.MBSSID[apidx].wdev.WepStatus), 
							pAd->ApCfg.MBSSID[apidx].wdev.GroupKeyWepStatus, 
							GetEncryptType(pAd->ApCfg.MBSSID[apidx].wdev.GroupKeyWepStatus),
							pAd->ApCfg.MBSSID[apidx].wdev.WpaMixPairCipher);		
	}
	printk("\n");
	
	printk("\n%-19s%-4s%-4s%-15s%-12s\n",
		   "MAC", "AID", "BSS", "Auth", "Encrypt");
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (pEntry && IS_ENTRY_CLIENT(pEntry) && pEntry->Sst == SST_ASSOC)
		{
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->apidx);
			printk("%-15s", GetAuthMode(pEntry->AuthMode));
			printk("%-12s", GetEncryptType(pEntry->WepStatus));						
			printk("\n");
		}
	} 

	return TRUE;
}


INT	Show_Sat_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	/* Sanity check for calculation of sucessful count */

	printk("TransmittedFragmentCount = %d\n", pAd->WlanCounters.TransmittedFragmentCount.u.LowPart + pAd->WlanCounters.MulticastTransmittedFrameCount.QuadPart);
	printk("MulticastTransmittedFrameCount = %d\n", pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart);
	printk("FailedCount = %d\n", pAd->WlanCounters.FailedCount.u.LowPart);
	printk("RetryCount = %d\n", pAd->WlanCounters.RetryCount.u.LowPart);
	printk("MultipleRetryCount = %d\n", pAd->WlanCounters.MultipleRetryCount.u.LowPart);
	printk("RTSSuccessCount = %d\n", pAd->WlanCounters.RTSSuccessCount.u.LowPart);
	printk("RTSFailureCount = %d\n", pAd->WlanCounters.RTSFailureCount.u.LowPart);
	printk("ACKFailureCount = %d\n", pAd->WlanCounters.ACKFailureCount.u.LowPart);
	printk("FrameDuplicateCount = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart);
	printk("ReceivedFragmentCount = %d\n", pAd->WlanCounters.ReceivedFragmentCount.u.LowPart);
	printk("MulticastReceivedFrameCount = %d\n", pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart);
#ifdef DBG 		
	printk("RealFcsErrCount = %d\n", pAd->RalinkCounters.RealFcsErrCount.u.LowPart);
#else
	printk("FCSErrorCount = %d\n", pAd->WlanCounters.FCSErrorCount.u.LowPart);
	printk("FrameDuplicateCount.LowPart = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100);
#endif
	printk("TransmittedFrameCount = %d\n", pAd->WlanCounters.TransmittedFragmentCount.u.LowPart);
	printk("WEPUndecryptableCount = %d\n", pAd->WlanCounters.WEPUndecryptableCount.u.LowPart);

#ifdef DOT11_N_SUPPORT
	printk("\n===Some 11n statistics variables: \n");
	/* Some 11n statistics variables */
	printk("TransmittedAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedAMSDUCount.u.LowPart);
	printk("TransmittedOctetsInAMSDU = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedOctetsInAMSDU.QuadPart);
	printk("ReceivedAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.ReceivedAMSDUCount.u.LowPart);	
	printk("ReceivedOctesInAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.ReceivedOctesInAMSDUCount.QuadPart);	
	printk("TransmittedAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart);
	printk("TransmittedMPDUsInAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart);
	printk("TransmittedOctetsInAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.u.LowPart);
	printk("MPDUInReceivedAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.MPDUInReceivedAMPDUCount.u.LowPart);
#ifdef DOT11N_DRAFT3
	printk("fAnyStaFortyIntolerant=%d\n", pAd->MacTab.fAnyStaFortyIntolerant);
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

{
	int apidx;
		
	for (apidx=0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		printk("-- IF-ra%d -- \n", apidx);
		printk("Packets Received = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].RxCount);
		printk("Packets Sent = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].TxCount);
		printk("Bytes Received = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].ReceivedByteCount);
		printk("Byte Sent = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].TransmittedByteCount);
		printk("Error Packets Received = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].RxErrorCount);
		printk("Drop Received Packets = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].RxDropCount);
		
#ifdef WSC_INCLUDED
		if (pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode != WSC_DISABLE)
		{
			WSC_CTRL *pWscCtrl;

			pWscCtrl = &pAd->ApCfg.MBSSID[apidx].WscControl;
			printk("WscInfo:\n"
					"\tWscConfMode=%d\n"
					"\tWscMode=%s\n"
					"\tWscConfStatus=%d\n"
					"\tWscPinCode=%d\n"
					"\tWscState=0x%x\n"
					"\tWscStatus=0x%x\n",
					pWscCtrl->WscConfMode, 
					((pWscCtrl->WscMode == WSC_PIN_MODE) ? "PIN" : "PBC"),
					pWscCtrl->WscConfStatus, pWscCtrl->WscEnrolleePinCode, 
					pWscCtrl->WscState, pWscCtrl->WscStatus);	
		}
#endif /* WSC_INCLUDED */

		printk("-- IF-ra%d end -- \n", apidx);
	}
}

{
	int i, j, k, maxMcs = MAX_MCS_SET - 1;
	PMAC_TABLE_ENTRY pEntry;


	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &pAd->MacTab.Content[i];
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{

			printk("\n%02X:%02X:%02X:%02X:%02X:%02X - ",
				   PRINT_MAC(pEntry->Addr));
			printk("%-4d\n", (int)pEntry->Aid);

			for (j=maxMcs; j>=0; j--)
			{
				if ((pEntry->TXMCSExpected[j] != 0) || (pEntry->TXMCSFailed[j] !=0))
				{
					printk("MCS[%02d]: Expected %u, Successful %u (%d%%), Failed %u\n",
						   j, pEntry->TXMCSExpected[j], pEntry->TXMCSSuccessful[j], 
						   pEntry->TXMCSExpected[j] ? (100*pEntry->TXMCSSuccessful[j])/pEntry->TXMCSExpected[j] : 0,
						   pEntry->TXMCSFailed[j]);
					for(k=maxMcs; k>=0; k--)
					{
						if (pEntry->TXMCSAutoFallBack[j][k] != 0)
						{
							printk("\t\t\tAutoMCS[%02d]: %u (%d%%)\n", k, pEntry->TXMCSAutoFallBack[j][k],
								   (100*pEntry->TXMCSAutoFallBack[j][k])/pEntry->TXMCSExpected[j]);
						}
					}
				}
			}
		}
	}

}

#ifdef DOT11_N_SUPPORT
{
	TX_NAG_AGG_CNT_STRUC	TxAggCnt;
	TX_AGG_CNT0_STRUC	TxAggCnt0;
	TX_AGG_CNT1_STRUC	TxAggCnt1;
	TX_AGG_CNT2_STRUC	TxAggCnt2;
	TX_AGG_CNT3_STRUC	TxAggCnt3;	
	UINT32				totalCount;
	
	RTMP_IO_READ32(pAd, TX_AGG_CNT, &TxAggCnt.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT0, &TxAggCnt0.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT1, &TxAggCnt1.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT2, &TxAggCnt2.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT3, &TxAggCnt3.word);

	totalCount = TxAggCnt.field.NonAggTxCount + TxAggCnt.field.AggTxCount;
	printk("Tx_Agg_Cnt->NonAggTxCount=%d!,  AggTxCount=%d!\n", TxAggCnt.field.NonAggTxCount, TxAggCnt.field.AggTxCount);
	printk("\tTx_Agg_Cnt 1 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize1Count, TxAggCnt0.field.AggSize1Count ? (TxAggCnt0.field.AggSize1Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 2 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize2Count, TxAggCnt0.field.AggSize2Count ? (TxAggCnt0.field.AggSize2Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 3 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize3Count, TxAggCnt1.field.AggSize3Count ? (TxAggCnt1.field.AggSize3Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 4 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize4Count, TxAggCnt1.field.AggSize4Count ? (TxAggCnt1.field.AggSize4Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 5 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize5Count, TxAggCnt2.field.AggSize5Count ? (TxAggCnt2.field.AggSize5Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 6 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize6Count, TxAggCnt2.field.AggSize6Count ? (TxAggCnt2.field.AggSize6Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 7 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize7Count, TxAggCnt3.field.AggSize7Count ? (TxAggCnt3.field.AggSize7Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 8 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize8Count, (TxAggCnt3.field.AggSize8Count ? (TxAggCnt3.field.AggSize8Count * 100 / totalCount) : 0));
	printk("====================\n");
	
}
#endif /* DOT11_N_SUPPORT */

	return TRUE;
}



INT	Show_Sat_Reset_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	/* Sanity check for calculation of sucessful count */

	printk("TransmittedFragmentCount = %d\n", pAd->WlanCounters.TransmittedFragmentCount.u.LowPart + pAd->WlanCounters.MulticastTransmittedFrameCount.QuadPart);
	printk("MulticastTransmittedFrameCount = %d\n", pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart);
	printk("FailedCount = %d\n", pAd->WlanCounters.FailedCount.u.LowPart);
	printk("RetryCount = %d\n", pAd->WlanCounters.RetryCount.u.LowPart);
	printk("MultipleRetryCount = %d\n", pAd->WlanCounters.MultipleRetryCount.u.LowPart);
	printk("RTSSuccessCount = %d\n", pAd->WlanCounters.RTSSuccessCount.u.LowPart);
	printk("RTSFailureCount = %d\n", pAd->WlanCounters.RTSFailureCount.u.LowPart);
	printk("ACKFailureCount = %d\n", pAd->WlanCounters.ACKFailureCount.u.LowPart);
	printk("FrameDuplicateCount = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart);
	printk("ReceivedFragmentCount = %d\n", pAd->WlanCounters.ReceivedFragmentCount.u.LowPart);
	printk("MulticastReceivedFrameCount = %d\n", pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart);
#ifdef DBG 		
	printk("RealFcsErrCount = %d\n", pAd->RalinkCounters.RealFcsErrCount.u.LowPart);
#else
	printk("FCSErrorCount = %d\n", pAd->WlanCounters.FCSErrorCount.u.LowPart);
	printk("FrameDuplicateCount.LowPart = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100);
#endif
	printk("TransmittedFrameCount = %d\n", pAd->WlanCounters.TransmittedFrameCount.u.LowPart);
	printk("WEPUndecryptableCount = %d\n", pAd->WlanCounters.WEPUndecryptableCount.u.LowPart);

	pAd->WlanCounters.TransmittedFragmentCount.u.LowPart = 0;
	pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart = 0;
	pAd->WlanCounters.FailedCount.u.LowPart = 0;
	pAd->WlanCounters.RetryCount.u.LowPart = 0;
	pAd->WlanCounters.MultipleRetryCount.u.LowPart = 0;
	pAd->WlanCounters.RTSSuccessCount.u.LowPart = 0;
	pAd->WlanCounters.RTSFailureCount.u.LowPart = 0;
	pAd->WlanCounters.ACKFailureCount.u.LowPart = 0;
	pAd->WlanCounters.FrameDuplicateCount.u.LowPart = 0;
	pAd->WlanCounters.ReceivedFragmentCount.u.LowPart = 0;
	pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart = 0;
#ifdef DBG 		
	pAd->RalinkCounters.RealFcsErrCount.u.LowPart = 0;
#else
	pAd->WlanCounters.FCSErrorCount.u.LowPart = 0;
	pAd->WlanCounters.FrameDuplicateCount.u.LowPart = 0;
#endif
	pAd->WlanCounters.TransmittedFrameCount.u.LowPart = 0;
	pAd->WlanCounters.WEPUndecryptableCount.u.LowPart = 0;


{
	int i, j, k;
	PMAC_TABLE_ENTRY pEntry;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &pAd->MacTab.Content[i];
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{

			printk("\n%02X:%02X:%02X:%02X:%02X:%02X - ",
				   PRINT_MAC(pEntry->Addr));
			printk("%-4d\n", (int)pEntry->Aid);

			for (j=15; j>=0; j--)
			{
				if ((pEntry->TXMCSExpected[j] != 0) || (pEntry->TXMCSFailed[j] !=0))
				{
					printk("MCS[%02d]: Expected %u, Successful %u (%d%%), Failed %u\n",
						   j, pEntry->TXMCSExpected[j], pEntry->TXMCSSuccessful[j], 
						   pEntry->TXMCSExpected[j] ? (100*pEntry->TXMCSSuccessful[j])/pEntry->TXMCSExpected[j] : 0,
						   pEntry->TXMCSFailed[j]
						   );
					for(k=15; k>=0; k--)
					{
						if (pEntry->TXMCSAutoFallBack[j][k] != 0)
						{
							printk("\t\t\tAutoMCS[%02d]: %u (%d%%)\n", k, pEntry->TXMCSAutoFallBack[j][k],
								   (100*pEntry->TXMCSAutoFallBack[j][k])/pEntry->TXMCSExpected[j]);
						}
					}
				}
			}
		}
		for (j=0; j<16; j++)
		{
			pEntry->TXMCSExpected[j] = 0;
			pEntry->TXMCSSuccessful[j] = 0;
			pEntry->TXMCSFailed[j] = 0;
			for(k=15; k>=0; k--)
			{
				pEntry->TXMCSAutoFallBack[j][k] = 0;
			}
		}
	}

#ifdef DOT11_N_SUPPORT
{
	TX_NAG_AGG_CNT_STRUC	TxAggCnt;
	TX_AGG_CNT0_STRUC	TxAggCnt0;
	TX_AGG_CNT1_STRUC	TxAggCnt1;
	TX_AGG_CNT2_STRUC	TxAggCnt2;
	TX_AGG_CNT3_STRUC	TxAggCnt3;	
	UINT32				totalCount, ratio1, ratio2, ratio3, ratio4, ratio5, ratio6, ratio7, ratio8;
	
	RTMP_IO_READ32(pAd, TX_AGG_CNT, &TxAggCnt.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT0, &TxAggCnt0.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT1, &TxAggCnt1.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT2, &TxAggCnt2.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT3, &TxAggCnt3.word);

	totalCount = TxAggCnt.field.NonAggTxCount + TxAggCnt.field.AggTxCount;
	ratio1 = TxAggCnt0.field.AggSize1Count ? (TxAggCnt0.field.AggSize1Count * 100 / totalCount) : 0;
	ratio2 = TxAggCnt0.field.AggSize2Count ? (TxAggCnt0.field.AggSize2Count * 100 / totalCount) : 0;
	ratio3 = TxAggCnt1.field.AggSize3Count ? (TxAggCnt1.field.AggSize3Count * 100 / totalCount) : 0;
	ratio4 = TxAggCnt1.field.AggSize4Count ? (TxAggCnt1.field.AggSize4Count * 100 / totalCount) : 0;
	ratio5 = TxAggCnt2.field.AggSize5Count ? (TxAggCnt2.field.AggSize5Count * 100 / totalCount) : 0;
	ratio6 = TxAggCnt2.field.AggSize6Count ? (TxAggCnt2.field.AggSize6Count * 100 / totalCount) : 0;
	ratio7 = TxAggCnt3.field.AggSize7Count ? (TxAggCnt3.field.AggSize7Count * 100 / totalCount) : 0;
	ratio8 = TxAggCnt3.field.AggSize8Count ? (TxAggCnt3.field.AggSize8Count * 100 / totalCount) : 0;

	printk("Tx_Agg_Cnt->NonAggTxCount=%d!,  AggTxCount=%d!\n", TxAggCnt.field.NonAggTxCount, TxAggCnt.field.AggTxCount);
	printk("\tTx_Agg_Cnt 1 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize1Count, ratio1);
	printk("\tTx_Agg_Cnt 2 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize2Count, ratio2);
	printk("\tTx_Agg_Cnt 3 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize3Count, ratio3);
	printk("\tTx_Agg_Cnt 4 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize4Count, ratio4);
	printk("\tTx_Agg_Cnt 5 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize5Count, ratio5);
	printk("\tTx_Agg_Cnt 6 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize6Count, ratio6);
	printk("\tTx_Agg_Cnt 7 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize7Count, ratio7);
	printk("\tTx_Agg_Cnt 8 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize8Count, ratio8);
	printk("\tRatio: 1(%d%%), 2(%d%%), 3(%d%%), 4(%d%%), 5(%d%%), 6(%d%%), 7(%d%%), 8(%d%%)!\n",
			ratio1+ratio2+ratio3+ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio2+ratio3+ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio3+ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio5+ratio6+ratio7+ratio8,
			ratio6+ratio7+ratio8,
			ratio7+ratio8,
			ratio8);
	printk("====================\n");
	
}
#endif /* DOT11_N_SUPPORT */

}


	return TRUE;
}


#ifdef MAT_SUPPORT
INT	Show_MATTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_STRING *arg)
{
	extern VOID dumpIPMacTb(MAT_STRUCT *pMatCfg, int index);
	extern NDIS_STATUS dumpSesMacTb(MAT_STRUCT *pMatCfg, int hashIdx);
	extern NDIS_STATUS dumpUidMacTb(MAT_STRUCT *pMatCfg, int hashIdx);
	extern NDIS_STATUS dumpIPv6MacTb(MAT_STRUCT *pMatCfg, int hashIdx);

	dumpIPMacTb(&pAd->MatCfg, -1);
	dumpSesMacTb(&pAd->MatCfg, -1);
	dumpUidMacTb(&pAd->MatCfg, -1);
	dumpIPv6MacTb(&pAd->MatCfg, -1);

	printk("Default BroadCast Address=%02x:%02x:%02x:%02x:%02x:%02x!\n", PRINT_MAC(BROADCAST_ADDR));
	return TRUE;
}
#endif /* MAT_SUPPORT */


#ifdef DOT1X_SUPPORT
/* 
    ==========================================================================
    Description:
        It only shall be queried by 802.1x daemon for querying radius configuration.        
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlQueryRadiusConf(
	IN PRTMP_ADAPTER pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	UCHAR	apidx, srv_idx, keyidx, KeyLen = 0;
	UCHAR	*mpool;
	PDOT1X_CMM_CONF	pConf;

	DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlQueryRadiusConf==>\n"));
	
	/* Allocate memory */
	os_alloc_mem(NULL, (PUCHAR *)&mpool, sizeof(DOT1X_CMM_CONF));	
    if (mpool == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("!!!%s: out of resource!!!\n", __FUNCTION__));
        return;
    }
	NdisZeroMemory(mpool, sizeof(DOT1X_CMM_CONF));

	pConf = (PDOT1X_CMM_CONF)mpool;

	/* get MBSS number */
	pConf->mbss_num = pAd->ApCfg.BssidNum;

	/* get own ip address */
	pConf->own_ip_addr = pAd->ApCfg.own_ip_addr;

	/* get retry interval */
	pConf->retry_interval = pAd->ApCfg.retry_interval;

	/* get session timeout interval */
	pConf->session_timeout_interval = pAd->ApCfg.session_timeout_interval;

	/* Get the quiet interval */
	pConf->quiet_interval = pAd->ApCfg.quiet_interval;

	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[apidx];
		PDOT1X_BSS_INFO  	p1xBssInfo = &pConf->Dot1xBssInfo[apidx];
	
		p1xBssInfo->radius_srv_num = pMbss->radius_srv_num;
	
		/* prepare radius ip, port and key */
		for (srv_idx = 0; srv_idx < pMbss->radius_srv_num; srv_idx++)
		{
			if (pMbss->radius_srv_info[srv_idx].radius_ip != 0)
			{
				p1xBssInfo->radius_srv_info[srv_idx].radius_ip = pMbss->radius_srv_info[srv_idx].radius_ip;
				p1xBssInfo->radius_srv_info[srv_idx].radius_port = pMbss->radius_srv_info[srv_idx].radius_port;
				p1xBssInfo->radius_srv_info[srv_idx].radius_key_len = pMbss->radius_srv_info[srv_idx].radius_key_len;
				if (pMbss->radius_srv_info[srv_idx].radius_key_len > 0)
				{
					NdisMoveMemory(p1xBssInfo->radius_srv_info[srv_idx].radius_key, 
									pMbss->radius_srv_info[srv_idx].radius_key, 
									pMbss->radius_srv_info[srv_idx].radius_key_len);
				}
			}
		}
		
		p1xBssInfo->ieee8021xWEP = (pMbss->wdev.IEEE8021X) ? 1 : 0;
		
		if (p1xBssInfo->ieee8021xWEP)
		{
			/* Default Key index, length and material */
			keyidx = pMbss->wdev.DefaultKeyId;
			p1xBssInfo->key_index = keyidx;

			/* Determine if the key is valid. */
			KeyLen = pAd->SharedKey[apidx][keyidx].KeyLen;
			if (KeyLen == 5 || KeyLen == 13)
			{
				p1xBssInfo->key_length = KeyLen;
				NdisMoveMemory(p1xBssInfo->key_material, pAd->SharedKey[apidx][keyidx].Key, KeyLen);
			}
		}

		/* Get NAS-ID per BSS */
		if (pMbss->NasIdLen > 0)
		{
			p1xBssInfo->nasId_len = pMbss->NasIdLen;
			NdisMoveMemory(p1xBssInfo->nasId, pMbss->NasId, pMbss->NasIdLen);
		}

		/* get EAPifname */
		if (pAd->ApCfg.EAPifname_len[apidx] > 0)
		{
			pConf->EAPifname_len[apidx] = pAd->ApCfg.EAPifname_len[apidx];
			NdisMoveMemory(pConf->EAPifname[apidx], pAd->ApCfg.EAPifname[apidx], pAd->ApCfg.EAPifname_len[apidx]);
		}	

		/* get PreAuthifname */
		if (pAd->ApCfg.PreAuthifname_len[apidx] > 0)
		{
			pConf->PreAuthifname_len[apidx] = pAd->ApCfg.PreAuthifname_len[apidx];
			NdisMoveMemory(pConf->PreAuthifname[apidx], pAd->ApCfg.PreAuthifname[apidx], pAd->ApCfg.PreAuthifname_len[apidx]);
		}	

	}
				
	wrq->u.data.length = sizeof(DOT1X_CMM_CONF);
	if (copy_to_user(wrq->u.data.pointer, pConf, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	os_free_mem(NULL, mpool);
	
}


/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlRadiusData(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("RTMPIoctlRadiusData, IF(ra%d)\n", pObj->ioctl_if));
#endif /* RELEASE_EXCLUDE */
	if ((pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode == Ndis802_11AuthModeWPA) 
    	|| (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode == Ndis802_11AuthModeWPA2)
    	|| (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode == Ndis802_11AuthModeWPA1WPA2) 
    	|| (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X == TRUE))
    	WpaSend(pAd, (PUCHAR)wrq->u.data.pointer, wrq->u.data.length);
}

/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlAddWPAKey(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	NDIS_AP_802_11_KEY 	*pKey;
	ULONG				KeyIdx;
	MAC_TABLE_ENTRY  	*pEntry;
	UCHAR				apidx;

	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx =	(UCHAR) pObj->ioctl_if;
		
#ifdef RELEASE_EXCLUDE		
    DBGPRINT(RT_DEBUG_INFO, ("RTMPIoctlAddWPAKey-IF(ra%d)\n", apidx));
#endif /* RELEASE_EXCLUDE */

	pKey = (PNDIS_AP_802_11_KEY) wrq->u.data.pointer;

	if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
	{
		if ((pKey->KeyLength == 32) || (pKey->KeyLength == 64))
		{
			if ((pEntry = MacTableLookup(pAd, pKey->addr)) != NULL)
			{
				INT	k_offset = 0;
			
#ifdef DOT11R_FT_SUPPORT
				/* The key shall be the second 256 bits of the MSK. */
				if (IS_FT_RSN_STA(pEntry) && pKey->KeyLength == 64)
					k_offset = 32;					
#endif /* DOT11R_FT_SUPPORT */				
		
				NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].PMK, pKey->KeyMaterial + k_offset, 32);				
    	        DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Add PMK=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x....\n", apidx,
            	pAd->ApCfg.MBSSID[apidx].PMK[0],pAd->ApCfg.MBSSID[apidx].PMK[1],pAd->ApCfg.MBSSID[apidx].PMK[2],pAd->ApCfg.MBSSID[apidx].PMK[3],
            	pAd->ApCfg.MBSSID[apidx].PMK[4],pAd->ApCfg.MBSSID[apidx].PMK[5],pAd->ApCfg.MBSSID[apidx].PMK[6],pAd->ApCfg.MBSSID[apidx].PMK[7]));
			}
		}
	}
	else	/* Old WEP stuff */
	{
		UCHAR	CipherAlg;
    	PUCHAR	Key;

		if(pKey->KeyLength > 16)
			return;
		
		KeyIdx = pKey->KeyIndex & 0x0fffffff;

		if (KeyIdx < 4)
		{
			/* it is a shared key */
			if (pKey->KeyIndex & 0x80000000)
			{
				UINT8	Wcid;
							
				DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Set Group Key\n", apidx));

				/* Default key for tx (shared key) */
				pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId = (UCHAR) KeyIdx;								
                     
				/* set key material and key length */
				if (pKey->KeyLength > 16)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Key length too long %d\n", apidx, pKey->KeyLength));
					pKey->KeyLength = 16;
				}
				pAd->SharedKey[apidx][KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;
				NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].Key, &pKey->KeyMaterial, pKey->KeyLength);
				
				/* Set Ciper type */
				if (pKey->KeyLength == 5)
					pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_WEP64;
				else
					pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_WEP128;
			
    			CipherAlg = pAd->SharedKey[apidx][KeyIdx].CipherAlg;
    			Key = pAd->SharedKey[apidx][KeyIdx].Key;

				/* Set Group key material to Asic */
				AsicAddSharedKeyEntry(pAd, apidx, (UINT8)KeyIdx, &pAd->SharedKey[apidx][KeyIdx]);
		
				/* Get a specific WCID to record this MBSS key attribute */
				GET_GroupKey_WCID(pAd, Wcid, apidx);
												
				RTMPSetWcidSecurityInfo(pAd, apidx,(UINT8)KeyIdx, 
									CipherAlg, Wcid, SHAREDKEYTABLE);												
			}
			else	/* For Pairwise key setting */
			{
				pEntry = MacTableLookup(pAd, pKey->addr);

				if (pEntry)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Set Pair-wise Key\n", apidx));
		
					/* set key material and key length */
 					pEntry->PairwiseKey.KeyLen = (UCHAR)pKey->KeyLength;
					NdisMoveMemory(pEntry->PairwiseKey.Key, &pKey->KeyMaterial, pKey->KeyLength);
					
					/* set Cipher type */
					if (pKey->KeyLength == 5)
						pEntry->PairwiseKey.CipherAlg = CIPHER_WEP64;
					else
						pEntry->PairwiseKey.CipherAlg = CIPHER_WEP128;
						
					/* Add Pair-wise key to Asic */
					AsicAddPairwiseKeyEntry(
						pAd, 
						(UCHAR)pEntry->wcid,
                		&pEntry->PairwiseKey);

					/* update WCID attribute table and IVEIV table for this entry */
					RTMPSetWcidSecurityInfo(pAd, 
										pEntry->apidx, 
										(UINT8)KeyIdx, 										 
										pEntry->PairwiseKey.CipherAlg, 
										pEntry->wcid,
										PAIRWISEKEYTABLE);

				}	
			}
		}
	}
}


/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlAddPMKIDCache(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	UCHAR				apidx;
	NDIS_AP_802_11_KEY 	*pKey;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	apidx =	(UCHAR) pObj->ioctl_if;

	pKey = (PNDIS_AP_802_11_KEY) wrq->u.data.pointer;
    
    if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA2)
	{
		if(pKey->KeyLength == 32)
		{
			UCHAR	digest[80], PMK_key[20], macaddr[MAC_ADDR_LEN];
			
			/* Calculate PMKID */
#if 1			
			NdisMoveMemory(&PMK_key[0], "PMK Name", 8);
			NdisMoveMemory(&PMK_key[8], pAd->ApCfg.MBSSID[apidx].wdev.bssid, MAC_ADDR_LEN);
			NdisMoveMemory(&PMK_key[14], pKey->addr, MAC_ADDR_LEN);
			RT_HMAC_SHA1(pKey->KeyMaterial, PMK_LEN, PMK_key, 20, digest, SHA1_DIGEST_SIZE);
#else
			/* Todo */
			RTMPDerivePMKID(pAd->ApCfg.MBSSID[apidx].Bssid, 
							pKey->addr, 
							pKey->KeyMaterial, IN PUINT8 pAkm_oui, OUT PUINT8 pPMKID);
#endif

			NdisMoveMemory(macaddr, pKey->addr, MAC_ADDR_LEN);
			RTMPAddPMKIDCache(pAd, apidx, macaddr, digest, pKey->KeyMaterial);
			
			DBGPRINT(RT_DEBUG_TRACE, ("WPA2(pre-auth):(%02x:%02x:%02x:%02x:%02x:%02x)Calc PMKID=%02x:%02x:%02x:%02x:%02x:%02x\n", 
				pKey->addr[0],pKey->addr[1],pKey->addr[2],pKey->addr[3],pKey->addr[4],pKey->addr[5],digest[0],digest[1],digest[2],digest[3],digest[4],digest[5]));
			DBGPRINT(RT_DEBUG_TRACE, ("PMK =%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x\n",pKey->KeyMaterial[0],pKey->KeyMaterial[1],
				pKey->KeyMaterial[2],pKey->KeyMaterial[3],pKey->KeyMaterial[4],pKey->KeyMaterial[5],pKey->KeyMaterial[6],pKey->KeyMaterial[7]));
		}
		else
            DBGPRINT(RT_DEBUG_ERROR, ("Set::RT_OID_802_11_WPA2_ADD_PMKID_CACHE ERROR or is wep key \n"));
	}
    
    DBGPRINT(RT_DEBUG_TRACE, ("<== RTMPIoctlAddPMKIDCache\n"));
}


/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlStaticWepCopy(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	MAC_TABLE_ENTRY  *pEntry;
	UCHAR	MacAddr[MAC_ADDR_LEN];
	UCHAR			apidx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	apidx =	(UCHAR) pObj->ioctl_if;
	
    DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlStaticWepCopy-IF(ra%d)\n", apidx));

    if (wrq->u.data.length != sizeof(MacAddr))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("RTMPIoctlStaticWepCopy: the length isn't match (%d)\n", wrq->u.data.length));
        return;
    }
    else
    {
    	UINT32 len;
		
        len = copy_from_user(&MacAddr, wrq->u.data.pointer, wrq->u.data.length);    
        pEntry = MacTableLookup(pAd, MacAddr);
        if (!pEntry)
        {
            DBGPRINT(RT_DEBUG_ERROR, ("RTMPIoctlStaticWepCopy: the mac address isn't match\n"));
            return;
        }
        else
        {
            UCHAR	KeyIdx;
            
            KeyIdx = pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId;
            
            /*need to copy the default shared-key to pairwise key table for this entry in 802.1x mode */
			if (pAd->SharedKey[apidx][KeyIdx].KeyLen == 0)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ERROR: Can not get Default shared-key (index-%d)\n", KeyIdx));
				return;
			}
			else
        	{
            	pEntry->PairwiseKey.KeyLen = pAd->SharedKey[apidx][KeyIdx].KeyLen;
            	NdisMoveMemory(pEntry->PairwiseKey.Key, pAd->SharedKey[apidx][KeyIdx].Key, pEntry->PairwiseKey.KeyLen);
            	pEntry->PairwiseKey.CipherAlg = pAd->SharedKey[apidx][KeyIdx].CipherAlg;

				/* Add Pair-wise key to Asic */
            	AsicAddPairwiseKeyEntry(
                		pAd, 
                		(UCHAR)pEntry->wcid,
                		&pEntry->PairwiseKey);

				/* update WCID attribute table and IVEIV table for this entry */
				RTMPSetWcidSecurityInfo(pAd, 
										pEntry->apidx, 
										(UINT8)KeyIdx, 
                						pEntry->PairwiseKey.CipherAlg, 
										pEntry->wcid, 
										PAIRWISEKEYTABLE);
        	}
			
        }
	}
    return;
}

/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlSetIdleTimeout(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	MAC_TABLE_ENTRY  		*pEntry;	
	PDOT1X_IDLE_TIMEOUT		pIdleTime;
		
	if (wrq->u.data.length != sizeof(DOT1X_IDLE_TIMEOUT))
	{
        DBGPRINT(RT_DEBUG_ERROR, ("%s : the length is mis-match\n", __FUNCTION__));
        return;
    }

	pIdleTime = (PDOT1X_IDLE_TIMEOUT)wrq->u.data.pointer;

	if ((pEntry = MacTableLookup(pAd, pIdleTime->StaAddr)) == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("%s : the entry is empty\n", __FUNCTION__));
        return;
    }
	else
	{
		pEntry->NoDataIdleCount = 0;
		pEntry->StaIdleTimeout = pIdleTime->idle_timeout;
		DBGPRINT(RT_DEBUG_TRACE, ("%s : Update Idle-Timeout(%d) from dot1x daemon\n",
									__FUNCTION__, pEntry->StaIdleTimeout));
	}
	
	return;
}
#endif /* DOT1X_SUPPORT */

#ifdef DBG
/* 
    ==========================================================================
    Description:
        Read / Write BBP
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 bbp               ==> read all BBP
               2.) iwpriv ra0 bbp 1             ==> read BBP where RegID=1
               3.) iwpriv ra0 bbp 1=10		    ==> write BBP R1=0x10
    ==========================================================================
*/
VOID RTMPAPIoctlBBP(
	IN RTMP_ADAPTER *pAdapter,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
#ifdef RTMP_BBP
	RTMP_STRING *this_char;
	RTMP_STRING *value;
	UCHAR regBBP = 0;
	RTMP_STRING *mpool, *msg; /*msg[2048]; */
	RTMP_STRING *arg; /*arg[255]; */
	RTMP_STRING *ptr;
	INT bbpId;
	LONG bbpValue;
	BOOLEAN bIsPrintAllBBP = FALSE, bAllowDump, bCopyMsg;
	INT argLen;
#endif /* RTMP_BBP */

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlBBP\n"));
#endif /* RELEASE_EXCLUDE */


#ifdef RTMP_BBP
	os_alloc_mem(NULL, (UCHAR **)&mpool, sizeof(CHAR)*(MAX_BBP_MSG_SIZE+256+12));
	if (mpool == NULL) {
		return;
	}

	NdisZeroMemory(mpool, MAX_BBP_MSG_SIZE+256+12);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+MAX_BBP_MSG_SIZE+3) & (ULONG)~0x03);

	bAllowDump = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_NODUMPMSG) == RTPRIV_IOCTL_FLAG_NODUMPMSG) ? FALSE : TRUE;
	bCopyMsg = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_NOSPACE) == RTPRIV_IOCTL_FLAG_NOSPACE) ? FALSE : TRUE;
	argLen = strlen((char *)(wrq->u.data.pointer));

#if 0
	if ((wrq->u.data.length > 1) /*No parameters. */
#ifdef VXWORKS 
		&& (!(wrq->u.data.length == 4096 && bAllowDump))
#endif /* VXWORKS */
	)
#endif
//iversonnote
	if (argLen > 1)
	{
		NdisMoveMemory(arg, wrq->u.data.pointer, (argLen > 255) ? 255 : argLen);
		ptr = arg;
		sprintf(msg, "\n");
		/* Parsing Read or Write */
		while ((this_char = strsep((char **)&ptr, ",")) != NULL)
		{
#ifdef RELEASE_EXCLUDE
			DBGPRINT(RT_DEBUG_INFO, ("this_char=%s\n", this_char));
#endif /* RELEASE_EXCLUDE */
			if (!*this_char)
				continue;

			if ((value = strchr(this_char, '=')) != NULL)
				*value++ = 0;

			if (!value || !*value)
			{ /*Read */
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("this_char=%s, value=%s\n", this_char, value));
#endif /* RELEASE_EXCLUDE */
				if (sscanf(this_char, "%d", &(bbpId)) == 1)
				{
					if (bbpId <= pAdapter->chipCap.MaxNumOfBbpId)
					{
#ifdef CONFIG_ATE
						/*
							In RT2860 ATE mode, we do not load 8051 firmware.
							We must access BBP directly.
							For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
						*/
						if (ATE_ON(pAdapter))
						{
							ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							/* Sync with QA for comparation */
							sprintf(msg+strlen(msg), "%03d = %02X\n", bbpId, regBBP);
						}
						else
#endif /* CONFIG_ATE */
						{
							/* according to Andy, Gary, David require. */
							/* the command bbp shall read BBP register directly for dubug. */
							BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							sprintf(msg+strlen(msg), "R%02d[0x%02x]:%02X  ", bbpId, bbpId, regBBP);
						}
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
					}
					else
					{
						/*Invalid parametes, so default printk all bbp */
						bIsPrintAllBBP = TRUE;
						break;
					}
				}
				else
				{
					/*Invalid parametes, so default printk all bbp */
					bIsPrintAllBBP = TRUE;
					break;
				}
			}
			else
			{ /* Write */
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("this_char=%s, value=%s\n", this_char, value));
#endif /* RELEASE_EXCLUDE */
				if ((sscanf(this_char, "%d", &(bbpId)) == 1) && (sscanf(value, "%lx", &(bbpValue)) == 1))
				{
#ifdef RELEASE_EXCLUDE
					DBGPRINT(RT_DEBUG_INFO, ("bbpID=%02d, value=0x%lx\n", bbpId, bbpValue));
#endif /* RELEASE_EXCLUDE */
					if (bbpId <= pAdapter->chipCap.MaxNumOfBbpId)
					{
#ifdef CONFIG_ATE
						/*
							In RT2860 ATE mode, we do not load 8051 firmware.
							We must access BBP directly.
							For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
						*/
						if (ATE_ON(pAdapter))
						{
							ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							ATE_BBP_IO_WRITE8_BY_REG_ID(pAdapter, (UCHAR)bbpId,(UCHAR) bbpValue);

							/*Read it back for showing */
							ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							/* Sync with QA for comparation */
							sprintf(msg+strlen(msg), "%03d = %02X\n", bbpId, regBBP);
#ifdef RELEASE_EXCLUDE
							DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
						}
						else
#endif /* CONFIG_ATE */
						{
							/* according to Andy, Gary, David require. */
							/* the command bbp shall read/write BBP register directly for dubug. */
							BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							BBP_IO_WRITE8_BY_REG_ID(pAdapter, (UCHAR)bbpId,(UCHAR) bbpValue);
							/*Read it back for showing */
							BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X\n", bbpId, bbpId, regBBP);
						}
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
					}
					else
					{	
						/* Invalid parametes, so default printk all bbp */
						bIsPrintAllBBP = TRUE;
						break;
					}
				}
				else
				{
					/* Invalid parametes, so default printk all bbp */
					bIsPrintAllBBP = TRUE;
					break;
				}
			}
		}
	}
	else
		bIsPrintAllBBP = TRUE;

	if (bIsPrintAllBBP)
	{
		memset(msg, 0x00, MAX_BBP_MSG_SIZE);
		sprintf(msg, "\n");
		for (bbpId = 0; bbpId <= pAdapter->chipCap.MaxNumOfBbpId; bbpId++)
		{
#ifdef CONFIG_ATE
			/*
				In RT2860 ATE mode, we do not load 8051 firmware.
				We must access BBP directly.
				For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
			*/
			if (ATE_ON(pAdapter))
			{
				ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
				/* Sync with QA for comparation */
				sprintf(msg+strlen(msg), "%03d = %02X\n", bbpId, regBBP);
			}
			else
#endif /* CONFIG_ATE */
			{
#ifdef __ECOS
				BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
				printk("R%02d[0x%02X]:%02X    ", bbpId, bbpId, regBBP);
				if ((bbpId%5 == 4) || (bbpId == pAdapter->chipCap.MaxNumOfBbpId))
					printk("\n");			
#else
				/* according to Andy, Gary, David require. */
				/* the command bbp shall read/write BBP register directly for dubug. */
				BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
				sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X    ", bbpId, bbpId, regBBP);
				if (bbpId%5 == 4)
					sprintf(msg+strlen(msg), "\n");
#endif /* __ECOS */
			}
		}
	}
#ifdef RELEASE_EXCLUDE
	else
	{
		DBGPRINT(RT_DEBUG_INFO, ("copy to user:msg[len=%d]=%s\n", strlen(msg), msg));
	}
#endif /* RELEASE_EXCLUDE */

#if 0
	if (bCopyMsg)
	{
		/* Copy the information into the user buffer */
                if (wrq->u.data.length <= strlen(msg)) {		
			wrq->u.data.length = strlen(msg);
			Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
                }
	}
#endif
#ifdef LINUX
	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length)) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
	}
#endif /* LINUX */

	if (!bAllowDump)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Dump BBP msg[%d]=\n", (UINT32)strlen(msg)));
		DBGPRINT(RT_DEBUG_OFF, ("%s\n", msg));
	}

	os_free_mem(NULL, mpool);
	if (!bAllowDump)
		DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlBBP\n\n"));
#endif /* RTMP_BBP */
}


#ifdef RTMP_RF_RW_SUPPORT
/* 
    ==========================================================================
    Description:
        Read / Write RF register
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 rf                ==> read all RF registers
               2.) iwpriv ra0 rf 1              ==> read RF where RegID=1
               3.) iwpriv ra0 rf 1=10		    ==> write RF R1=0x10
    ==========================================================================
*/
VOID RTMPAPIoctlRF(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	RTMP_STRING *this_char;
	RTMP_STRING *value;
	UCHAR				regRF = 0;
	RTMP_STRING *mpool, *msg; /*msg[2048]; */
	RTMP_STRING *arg; /*arg[255]; */
	RTMP_STRING *ptr;
	INT					rfId, maxRFIdx;
	LONG				rfValue;
	BOOLEAN				bIsPrintAllRF = FALSE, bFromUI;
	INT					memLen = sizeof(CHAR) * (2048+256+12);
	
	maxRFIdx = pAdapter->chipCap.MaxNumOfRfId;
		
#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlRF\n"));
#endif /* RELEASE_EXCLUDE */
/*	mpool = (RTMP_STRING *)kmalloc(memLen, MEM_ALLOC_FLAG); */
	os_alloc_mem(NULL, (UCHAR **)&mpool, memLen);
	if (mpool == NULL) {
		return;
	}

	bFromUI = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_UI) == RTPRIV_IOCTL_FLAG_UI) ? TRUE : FALSE;
	
	NdisZeroMemory(mpool, memLen);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+2048+3) & (ULONG)~0x03);
	
	if ((wrq->u.data.length > 1) /*No parameters. */
#ifdef VXWORKS 
     && (!(wrq->u.data.length == 4096 && bFromUI))
#endif
		)
	{
		NdisMoveMemory(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
		ptr = arg;
		sprintf(msg, "\n");
		/*Parsing Read or Write */
		while ((this_char = strsep((char **)&ptr, ",")) != NULL)
		{
#ifdef RELEASE_EXCLUDE
			DBGPRINT(RT_DEBUG_INFO, ("this_char=%s\n", this_char));
#endif /* RELEASE_EXCLUDE */
			if (!*this_char)
				continue;

			if ((value = strchr(this_char, '=')) != NULL)
				*value++ = 0;

			if (!value || !*value)
			{ /*Read */
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("this_char=%s, value=%s\n", this_char, value));
#endif /* RELEASE_EXCLUDE */
				if (sscanf(this_char, "%d", &(rfId)) == 1)
				{
					if (rfId <= pAdapter->chipCap.MaxNumOfRfId)
					{
#ifdef CONFIG_ATE
						/*
							In RT2860 ATE mode, we do not load 8051 firmware.
							We must access BBP directly.
							For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
						*/
						if (ATE_ON(pAdapter))
						{
#ifdef RT6352
							if (IS_RT6352(pAdapter)) {
								ATE_RF_IO_READ8_BY_REG_ID(pAdapter, RF_BANK0, rfId, &regRF);
							}
							else
#endif /* RT6352 */
							{
								ATE_RF_IO_READ8_BY_REG_ID(pAdapter, rfId, &regRF);
							}

							/* Sync with QA for comparation */
							sprintf(msg+strlen(msg), "%03d = %02X\n", rfId, regRF);
						}
						else
#endif /* CONFIG_ATE */
						{
							/* according to Andy, Gary, David require. */
							/* the command rf shall read rf register directly for dubug. */
							/* BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP); */
							RT30xxReadRFRegister(pAdapter, rfId, &regRF);
							sprintf(msg+strlen(msg), "R%02d[0x%02x]:%02X  ", rfId, rfId, regRF);
						}
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
					}
					else
					{
						/*Invalid parametes, so default printk all RF */
						bIsPrintAllRF = TRUE;
						break;
					}
				}
				else
				{
					/*Invalid parametes, so default printk all RF */
					bIsPrintAllRF = TRUE;
					break;
				}
			}
			else
			{ /*Write */
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("this_char=%s, value=%s\n", this_char, value));
#endif /* RELEASE_EXCLUDE */
				if ((sscanf(this_char, "%d", &(rfId)) == 1) && (sscanf(value, "%lx", &(rfValue)) == 1))
				{
#ifdef RELEASE_EXCLUDE
					DBGPRINT(RT_DEBUG_INFO, ("rfID=%02d, value=0x%lx\n", rfId, rfValue));
#endif /* RELEASE_EXCLUDE */
					if (rfId <= pAdapter->chipCap.MaxNumOfRfId)
					{
#ifdef CONFIG_ATE
						/*
							In RT2860 ATE mode, we do not load 8051 firmware.
							We must access BBP directly.
							For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
						*/
						if (ATE_ON(pAdapter))
						{
							ATE_RF_IO_READ8_BY_REG_ID(pAdapter, rfId, &regRF);
							ATE_RF_IO_WRITE8_BY_REG_ID(pAdapter, (UCHAR)rfId,(UCHAR) rfValue);

							/* Read it back for showing. */
							ATE_RF_IO_READ8_BY_REG_ID(pAdapter, rfId, &regRF);
							/* Sync with QA for comparation */
							sprintf(msg+strlen(msg), "%03d = %02X\n", rfId, regRF);
#ifdef RELEASE_EXCLUDE
							DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
						}
						else
#endif /* CONFIG_ATE */
						{
							/* according to Andy, Gary, David require. */
							/* the command RF shall read/write RF register directly for dubug. */
							/*BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP); */
							/*BBP_IO_WRITE8_BY_REG_ID(pAdapter, (UCHAR)bbpId,(UCHAR) bbpValue); */
							RT30xxReadRFRegister(pAdapter, rfId, &regRF);
								RT30xxWriteRFRegister(pAdapter, (UCHAR)rfId,(UCHAR) rfValue);
							/*Read it back for showing */
							RT30xxReadRFRegister(pAdapter, rfId, &regRF);
							sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X\n", rfId, rfId, regRF);
#ifdef RELEASE_EXCLUDE
							DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
						}
					}
					else
					{	/*Invalid parametes, so default printk all RF */
						bIsPrintAllRF = TRUE;
						break;
					}
				}
				else
				{	/*Invalid parametes, so default printk all RF */
					bIsPrintAllRF = TRUE;
					break;
				}
			}
		}
	}
	else
		bIsPrintAllRF = TRUE;

	if (bIsPrintAllRF)
	{
		memset(msg, 0x00, 2048);
		sprintf(msg, "\n");
		for (rfId = 0; rfId <= maxRFIdx; rfId++)
		{
#ifdef CONFIG_ATE
			/*
				In RT2860 ATE mode, we do not load 8051 firmware.
				We must access RF registers directly.
				For RT2870 ATE mode, ATE_RF_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
			*/
			if (ATE_ON(pAdapter))
			{
				ATE_RF_IO_READ8_BY_REG_ID(pAdapter, rfId, &regRF);
				/* Sync with QA for comparation */
				sprintf(msg+strlen(msg), "%03d = %02X\n", rfId, regRF);
			}
			else
#endif /* CONFIG_ATE */
			{
				/* according to Andy, Gary, David require. */
				/* the command RF shall read/write RF register directly for dubug. */
				RT30xxReadRFRegister(pAdapter, rfId, &regRF);
				sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X    ", rfId, rfId*2, regRF);
				if (rfId%5 == 4)
					sprintf(msg+strlen(msg), "\n");
			}
		}
		/* Copy the information into the user buffer */
#ifdef RELEASE_EXCLUDE
		if (!bFromUI)
			DBGPRINT(RT_DEBUG_TRACE, ("strlen(msg)=%d\n", (UINT32)strlen(msg)));
#endif /* RELEASE_EXCLUDE */

#ifdef VXWORKS
		if (bFromUI && (wrq->u.data.length >= strlen(msg)))
		{
		wrq->u.data.length = strlen(msg);
		copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
		}
		else
		{
		DBGPRINT(RT_DEBUG_OFF, ("%s\n", msg));	
	}
#endif /* VXWORKS */
#ifdef LINUX
		wrq->u.data.length = strlen(msg);
		if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length)) 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
		}
#endif /* LINUX */
	}
	else
	{
#ifdef VXWORKS
	if (bFromUI && (wrq->u.data.length >= strlen(msg))) 
	{
		wrq->u.data.length = strlen(msg);
		copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
	} 
#endif /* VXWORKS */
#ifdef LINUX
		/* Copy the information into the user buffer */
		wrq->u.data.length = strlen(msg);
		if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
		}
#endif /* LINUX */
	}
	
	if (!bFromUI)
		DBGPRINT(RT_DEBUG_TRACE, ("Dump RF msg[%d]=%s]\n", strlen(msg), msg));
	
/*	kfree(mpool); */
	os_free_mem(NULL, mpool);
	if (!bFromUI)
		DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlRF\n\n"));
	
}
#endif /* RTMP_RF_RW_SUPPORT */
#endif /*#ifdef DBG */

/* 
    ==========================================================================
    Description:
        Read / Write E2PROM
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 e2p 0     	==> read E2PROM where Addr=0x0
               2.) iwpriv ra0 e2p 0=1234    ==> write E2PROM where Addr=0x0, value=1234
    ==========================================================================
*/
VOID RTMPAPIoctlE2PROM(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	RTMP_STRING *this_char;
	RTMP_STRING *value;
	INT					j = 0, k = 0;
	RTMP_STRING *mpool, *msg;/*msg[1024]; */
	RTMP_STRING *arg; /*arg[255]; */
	RTMP_STRING *ptr;
	USHORT				eepAddr = 0;
	UCHAR				temp[16];
	RTMP_STRING temp2[16];
	USHORT				eepValue;
	BOOLEAN				bIsPrintAllE2PROM = FALSE;

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlE2PROM\n"));
#endif /* RELEASE_EXCLUDE */
/*	mpool = (RTMP_STRING *)kmalloc(sizeof(CHAR)*(4096+256+12), MEM_ALLOC_FLAG); */
	os_alloc_mem(NULL, (UCHAR **)&mpool, sizeof(CHAR)*(4096+256+12));

	if (mpool == NULL) {
		return;
	}

	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+4096+3) & (ULONG)~0x03);


	memset(msg, 0x00, 4096);
	memset(arg, 0x00, 256);		
	if ((wrq->u.data.length > 1) /*No parameters. */
#ifdef VXWORKS 
     && (!(wrq->u.data.length == 4096 && wrq->u.data.flags== RT_OID_802_11_HARDWARE_REGISTER))
#endif
		)
	{
		NdisMoveMemory(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
		ptr = arg;
		sprintf(msg, "\n");
		/*Parsing Read or Write */
		while ((this_char = strsep((char **)&ptr, ",")) != NULL)
		{
#ifdef RELEASE_EXCLUDE
			DBGPRINT(RT_DEBUG_INFO, ("this_char=%s\n", this_char));
#endif /* RELEASE_EXCLUDE */
			if (!*this_char)
				continue;

			if ((value = strchr(this_char, '=')) != NULL)
				*value++ = 0;

			if (!value || !*value)
			{ /*Read */
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("Read: this_char=%s, strlen=%d\n", this_char, (UINT32)strlen(this_char)));
#endif /* RELEASE_EXCLUDE */

				/* Sanity check */
				if(strlen(this_char) > 4)
					break;

				j = strlen(this_char);
				while(j-- > 0)
				{
					if(this_char[j] > 'f' || this_char[j] < '0')
						goto done; /*return; */
				}

				/* E2PROM addr */
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}
				
				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				if(strlen(this_char) == 4)
				{
					AtoH(this_char, temp, 2);
					eepAddr = *temp*256 + temp[1];					
					if (eepAddr < 0xFFFF)
					{
						RT28xx_EEPROM_READ16(pAdapter, eepAddr, eepValue);
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("eepAddr=%x, eepValue=0x%x\n", eepAddr, eepValue));
#endif /* RELEASE_EXCLUDE */
						sprintf(msg+strlen(msg), "[0x%04X]:0x%04X  ", eepAddr , eepValue);
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
					}
					else
					{/*Invalid parametes, so default printk all bbp */
						break;
					}
				}
			}
			else
			{ /*Write */
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("Write: this_char=%s, strlen(value)=%d, value=%s\n", this_char, (UINT32)strlen(value), value));
#endif /* RELEASE_EXCLUDE */
				NdisMoveMemory(&temp2, value, strlen(value));
				temp2[strlen(value)] = '\0';

				/* Sanity check */
				if((strlen(this_char) > 4) || strlen(temp2) > 8)
					break;

				j = strlen(this_char);
				while(j-- > 0)
				{
					if(this_char[j] > 'f' || this_char[j] < '0')
						goto done; /*return; */
				}
				j = strlen(temp2);
				while(j-- > 0)
				{
					if(temp2[j] > 'f' || temp2[j] < '0')
						goto done; /*return; */
				}

				/*MAC Addr */
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}

				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				/*MAC value */
				k = j = strlen(temp2);
				while(j-- > 0)
				{
					temp2[4-k+j] = temp2[j];
				}
				
				while(k < 4)
					temp2[3-k++]='0';
				temp2[4]='\0';

				AtoH(this_char, temp, 2);
				eepAddr = *temp*256 + temp[1];

				AtoH(temp2, temp, 2);
				eepValue = *temp*256 + temp[1];

#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("eepAddr=%02x, eepValue=0x%x\n", eepAddr, eepValue));
#endif /* RELEASE_EXCLUDE */
				RT28xx_EEPROM_WRITE16(pAdapter, eepAddr, eepValue);
				sprintf(msg+strlen(msg), "[0x%02X]:%02X  ", eepAddr, eepValue);
#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
			}
		}
	} else {
		bIsPrintAllE2PROM = TRUE;
	}

	if (bIsPrintAllE2PROM)
	{
		sprintf(msg, "\n");
		
		/* E2PROM Registers */
		for (eepAddr = 0x00; eepAddr < 0x200; eepAddr += 2)
		{
			RT28xx_EEPROM_READ16(pAdapter, eepAddr, eepValue);
			sprintf(msg+strlen(msg), "[0x%04X]:%04X  ", eepAddr , eepValue);
			if ((eepAddr & 0x7) == 0x7)
				sprintf(msg+strlen(msg), "\n");
		}
	}

	if(strlen(msg) == 1)
		sprintf(msg+strlen(msg), "===>Error command format!");

	/* Copy the information into the user buffer */
#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
#endif /* RELEASE_EXCLUDE */

	AP_E2PROM_IOCTL_PostCtrl(wrq, msg);

done:	
/*	kfree(mpool); */
	os_free_mem(NULL, mpool);
    if (wrq->u.data.flags != RT_OID_802_11_HARDWARE_REGISTER)	
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlE2PROM\n"));
}


/* 
    ==========================================================================
    Description:
        Read statistics counter
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 stat 0     	==> Read statistics counter
    ==========================================================================
*/
VOID RTMPIoctlStatistics(
	IN PRTMP_ADAPTER pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	INT					Status;
	RTMP_STRING *msg;
#ifdef WSC_AP_SUPPORT
    UCHAR               idx = 0;
#endif /* WSC_AP_SUPPORT */
#ifdef P2P_SUPPORT
	POS_COOKIE  		pObj = (POS_COOKIE) pAd->OS_Cookie;
#endif /* P2P_SUPPORT */

/*	msg = (RTMP_STRING *)kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG); */
	os_alloc_mem(pAd, (UCHAR **)&msg, sizeof(CHAR)*(2048));
	if (msg == NULL) {
		return;
	}


    memset(msg, 0x00, 1600);
    sprintf(msg, "\n");

#ifdef CONFIG_ATE
	if(ATE_ON(pAd))
	{
	    sprintf(msg+strlen(msg), "Tx success                    = %ld\n", (ULONG)pAd->ate.TxDoneCount);
	    /*sprintf(msg+strlen(msg), "Tx success without retry        = %ld\n", (ULONG)pAd->ate.TxDoneCount); */
	}
	else
#endif /* CONFIG_ATE */
	{
    sprintf(msg+strlen(msg), "Tx success                        = %ld\n", (ULONG)pAd->WlanCounters.TransmittedFragmentCount.u.LowPart);
    /*sprintf(msg+strlen(msg), "Tx success without retry        = %ld\n", (ULONG)pAd->WlanCounters.TransmittedFragmentCount.u.LowPart - (ULONG)pAd->WlanCounters.RetryCount.u.LowPart); */
	}
    sprintf(msg+strlen(msg), "Tx retry count                  = %ld\n", (ULONG)pAd->WlanCounters.RetryCount.u.LowPart);
    sprintf(msg+strlen(msg), "Tx fail to Rcv ACK after retry  = %ld\n", (ULONG)pAd->WlanCounters.FailedCount.u.LowPart);
    sprintf(msg+strlen(msg), "RTS Success Rcv CTS             = %ld\n", (ULONG)pAd->WlanCounters.RTSSuccessCount.u.LowPart);
    sprintf(msg+strlen(msg), "RTS Fail Rcv CTS                = %ld\n", (ULONG)pAd->WlanCounters.RTSFailureCount.u.LowPart);

    sprintf(msg+strlen(msg), "Rx success                      = %ld\n", (ULONG)pAd->WlanCounters.ReceivedFragmentCount.QuadPart);
    sprintf(msg+strlen(msg), "Rx with CRC                     = %ld\n", (ULONG)pAd->WlanCounters.FCSErrorCount.u.LowPart);
    sprintf(msg+strlen(msg), "Rx drop due to out of resource  = %ld\n", (ULONG)pAd->Counters8023.RxNoBuffer);
    sprintf(msg+strlen(msg), "Rx duplicate frame              = %ld\n", (ULONG)pAd->WlanCounters.FrameDuplicateCount.u.LowPart);

    sprintf(msg+strlen(msg), "False CCA (one second)          = %ld\n", (ULONG)pAd->RalinkCounters.OneSecFalseCCACnt);

#ifdef CONFIG_ATE
	if(ATE_ON(pAd))
	{
		if (pAd->ate.RxAntennaSel == 0)
		{
    		sprintf(msg+strlen(msg), "RSSI-A                          = %ld\n", (LONG)(pAd->ate.LastRssi0 - pAd->BbpRssiToDbmDelta));
			sprintf(msg+strlen(msg), "RSSI-B (if available)           = %ld\n", (LONG)(pAd->ate.LastRssi1 - pAd->BbpRssiToDbmDelta));
			sprintf(msg+strlen(msg), "RSSI-C (if available)           = %ld\n\n", (LONG)(pAd->ate.LastRssi2 - pAd->BbpRssiToDbmDelta));
		}
		else
		{
    		sprintf(msg+strlen(msg), "RSSI                            = %ld\n", (LONG)(pAd->ate.LastRssi0 - pAd->BbpRssiToDbmDelta));
		}
	}
	else
#endif /* CONFIG_ATE */
	{
    	sprintf(msg+strlen(msg), "RSSI-A                          = %ld\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi0 - pAd->BbpRssiToDbmDelta));
		sprintf(msg+strlen(msg), "RSSI-B (if available)           = %ld\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi1 - pAd->BbpRssiToDbmDelta));
		sprintf(msg+strlen(msg), "RSSI-C (if available)           = %ld\n\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi2 - pAd->BbpRssiToDbmDelta));
	}

#ifdef WSC_AP_SUPPORT
	sprintf(msg+strlen(msg), "WPS Information:\n");
#ifdef P2P_SUPPORT
	if (pObj->ioctl_if_type == INT_P2P)
	{
		if (pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCodeLen == 8)
			sprintf(msg+strlen(msg), "P2P Client PinCode               %08u\n", 
								pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode);
		else
			sprintf(msg+strlen(msg), "P2P Client PinCode               %04u\n", 
								pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode);
		sprintf(msg+strlen(msg), "P2P Client WPS Profile Count     = %d\n", 
								pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.ProfileCnt);
	}
	else
#endif /* P2P_SUPPORT */
	{
		for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++)
		{
			/* display pin code */
			if (pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCodeLen == 8)
				sprintf(msg+strlen(msg), "Enrollee PinCode(ra%d)           %08u\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCode);
			else
				sprintf(msg+strlen(msg), "Enrollee PinCode(ra%d)           %04u\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCode);
		}
	}
#ifdef APCLI_SUPPORT
    sprintf(msg+strlen(msg), "\n");
	if (pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCodeLen == 8)
		sprintf(msg+strlen(msg), "Enrollee PinCode(ApCli0)        %08u\n", pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode);
	else
		sprintf(msg+strlen(msg), "Enrollee PinCode(ApCli0)        %04u\n", pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode);
    sprintf(msg+strlen(msg), "Ap Client WPS Profile Count     = %d\n", pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.ProfileCnt);
    for (idx = 0; idx < pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.ProfileCnt ; idx++)
    {
        PWSC_CREDENTIAL pCredential = &pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.Profile[idx];
        sprintf(msg+strlen(msg), "Profile[%d]:\n", idx);        
        sprintf(msg+strlen(msg), "SSID                            = %s\n", pCredential->SSID.Ssid);
        sprintf(msg+strlen(msg), "AuthType                        = %s\n", WscGetAuthTypeStr(pCredential->AuthType));
        sprintf(msg+strlen(msg), "EncrypType                      = %s\n", WscGetEncryTypeStr(pCredential->EncrType)); 
        sprintf(msg+strlen(msg), "KeyIndex                        = %d\n", pCredential->KeyIndex);
        if (pCredential->KeyLength != 0)
        {
            sprintf(msg+strlen(msg), "Key                             = %s\n", pCredential->Key);
        }
    }
    sprintf(msg+strlen(msg), "\n");
#endif /* APCLI_SUPPORT */
#endif /* WSC_AP_SUPPORT */
	sprintf(msg+strlen(msg), "P2P Information:\n");
	sprintf(msg+strlen(msg), "My Rule                         = %s\n", decodeMyRule(pAd->P2pCfg.Rule));
	sprintf(msg+strlen(msg), "Listen Channel                  = %d\n", pAd->P2pCfg.ListenChannel);
	sprintf(msg+strlen(msg), "WPS Config Method               = %s\n", decodeConfigMethod(pAd->P2pCfg.ConfigMethod));
	sprintf(msg+strlen(msg), "Peer PinCode                    = %c%c%c%c%c%c%c%c\n", pAd->P2pCfg.PinCode[0], pAd->P2pCfg.PinCode[1], pAd->P2pCfg.PinCode[2], pAd->P2pCfg.PinCode[3],
				pAd->P2pCfg.PinCode[4], pAd->P2pCfg.PinCode[5], pAd->P2pCfg.PinCode[6], pAd->P2pCfg.PinCode[7]);

    /* Copy the information into the user buffer */
#ifdef RELEASE_EXCLUDE
    DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
#endif /* RELEASE_EXCLUDE */
    wrq->u.data.length = strlen(msg);
    Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);

	os_free_mem(NULL, msg);
/*	kfree(msg); */
    DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlStatistics\n"));
}


#ifdef DOT11_N_SUPPORT
/* 
    ==========================================================================
    Description:
        Get Block ACK Table
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) iwpriv ra0 get_ba_table
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
VOID RTMPIoctlQueryBaTable(
	IN	PRTMP_ADAPTER	pAd, 
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	/*char *msg; */
	UCHAR	TotalEntry, i, j, index;
	QUERYBA_TABLE		*BAT;

	BAT = vmalloc(sizeof(QUERYBA_TABLE));

	RTMPZeroMemory(BAT, sizeof(QUERYBA_TABLE));

	TotalEntry = pAd->MacTab.Size;
	index = 0;
	for (i=0; ((i < MAX_LEN_OF_MAC_TABLE) && (TotalEntry > 0)); i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC) && (pEntry->TXBAbitmap))
		{
			NdisMoveMemory(BAT->BAOriEntry[index].MACAddr, pEntry->Addr, 6);
			for (j=0;j<8;j++)
			{
				if (pEntry->BAOriWcidArray[j] != 0)
					BAT->BAOriEntry[index].BufSize[j] = pAd->BATable.BAOriEntry[pEntry->BAOriWcidArray[j]].BAWinSize;
				else
					BAT->BAOriEntry[index].BufSize[j] = 0;
			}

			TotalEntry--;
			index++;
			BAT->OriNum++;
		}
	}

	TotalEntry = pAd->MacTab.Size;
	index = 0;
	for (i=0; ((i < MAX_LEN_OF_MAC_TABLE) && (TotalEntry > 0)); i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC) && (pEntry->RXBAbitmap))
		{
			NdisMoveMemory(BAT->BARecEntry[index].MACAddr, pEntry->Addr, 6);
			BAT->BARecEntry[index].BaBitmap = (UCHAR)pEntry->RXBAbitmap;
			for (j = 0; j < 8; j++)
			{
				if (pEntry->BARecWcidArray[j] != 0)
					BAT->BARecEntry[index].BufSize[j] = pAd->BATable.BARecEntry[pEntry->BARecWcidArray[j]].BAWinSize;
				else
					BAT->BARecEntry[index].BufSize[j] = 0;
			}

			TotalEntry--;
			index++;
			BAT->RecNum++;
		}
	}

	wrq->u.data.length = sizeof(QUERYBA_TABLE);

	if (copy_to_user(wrq->u.data.pointer, BAT, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	vfree(BAT);

#if 0
/*	msg = (CHAR *) kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG); */
	os_alloc_mem(pAd, (UCHAR **)&msg, sizeof(CHAR)*(2048));
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 2048);
	sprintf(msg,"%s","\n");

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s\n", "Configured", "SSID", "AuthMode", "EncrypType");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-13s%-26s\n", "Configured", "SSID", "AuthMode", "EncrypType", "DefaultKeyID", "Key");
	}
	else
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-64s\n", "Configured", "SSID", "AuthMode", "EncrypType", "Key");
	}

	if (Profile.WscConfigured == 1)
		sprintf(msg+strlen(msg),"%-12s", "No");
	else
		sprintf(msg+strlen(msg),"%-12s", "Yes");
	sprintf(msg+strlen(msg), "%-33s", Profile.WscSsid);
	sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	sprintf(msg+strlen(msg), "%-12s", WscGetEncryTypeStr(Profile.WscEncrypType));

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg), "%s\n", "");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg), "%-13d",Profile.DefaultKeyIdx);
		sprintf(msg+strlen(msg), "%-26s\n",Profile.WscWPAKey);
	}
	else if (Profile.WscEncrypType >= 4)
	{
	    sprintf(msg+strlen(msg), "%-64s\n",Profile.WscWPAKey);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));

/*	kfree(msg); */
	os_free_mem(NULL, msg);
#endif
}
#endif /* DOT11_N_SUPPORT */


#ifdef APCLI_SUPPORT
INT Set_ApCli_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT Enable;
	POS_COOKIE pObj;
	UCHAR ifIndex;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	Enable = simple_strtol(arg, 0, 16);

	pAd->ApCfg.ApCliTab[ifIndex].Enable = (Enable > 0) ? TRUE : FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_Enable_Proc::(enable = %d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].Enable));
	
	ApCliIfDown(pAd);

	return TRUE;
}

INT Set_ApCli_Ssid_Proc(
	IN  PRTMP_ADAPTER pAd, 
	IN  RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	INT success = FALSE;
	/*UCHAR keyMaterial[40]; */

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	if(strlen(arg) <= MAX_LEN_OF_SSID)
	{
		apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

		/* bring apcli interface down first */
		if(apcliEn == TRUE )
		{
			pAd->ApCfg.ApCliTab[ifIndex].Enable = FALSE;
			ApCliIfDown(pAd);
		}

		NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, arg, strlen(arg));
		pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen = (UCHAR)strlen(arg);
		success = TRUE;

		/* Upadte PMK and restart WPAPSK state machine for ApCli link */
		if (((pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode == Ndis802_11AuthModeWPAPSK) ||
				(pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode == Ndis802_11AuthModeWPA2PSK)) && 
					pAd->ApCfg.ApCliTab[ifIndex].PSKLen > 0)
		{
			RT_CfgSetWPAPSKKey(pAd, (RTMP_STRING *)pAd->ApCfg.ApCliTab[ifIndex].PSK, 
									pAd->ApCfg.ApCliTab[ifIndex].PSKLen,
									(PUCHAR)pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, 
									pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, 
									pAd->ApCfg.ApCliTab[ifIndex].PMK);
		}

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_Ssid_Proc::(Len=%d,Ssid=%s)\n", ifIndex,
			pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, pAd->ApCfg.ApCliTab[ifIndex].CfgSsid));

		pAd->ApCfg.ApCliTab[ifIndex].Enable = apcliEn;
	}
	else
		success = FALSE;

	return success;
}


INT Set_ApCli_Bssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	RTMP_STRING *value;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

	/* bring apcli interface down first */
	if(apcliEn == TRUE )
	{
		pAd->ApCfg.ApCliTab[ifIndex].Enable = FALSE;
		ApCliIfDown(pAd);
	}

	NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, MAC_ADDR_LEN);

	if(strlen(arg) == 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
	{
		for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"), i++) 
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				return FALSE;  /*Invalid */

			AtoH(value, &pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[i], 1);
		}

		if(i != 6)
			return FALSE;  /*Invalid */
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ApCli_Bssid_Proc (%2X:%2X:%2X:%2X:%2X:%2X)\n",
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[0],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[1],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[2],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[3],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[4],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[5]));

	pAd->ApCfg.ApCliTab[ifIndex].Enable = apcliEn;

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set ApCli-IF Authentication mode
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG       i;
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	if ((strcmp(arg, "WEPAUTO") == 0) || (strcmp(arg, "wepauto") == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if ((strcmp(arg, "SHARED") == 0) || (strcmp(arg, "shared") == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeShared;
	else if ((strcmp(arg, "WPAPSK") == 0) || (strcmp(arg, "wpapsk") == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((strcmp(arg, "WPA2PSK") == 0) || (strcmp(arg, "wpa2psk") == 0) || (strcmp(arg, "WPAPSKWPA2PSK") == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeWPA2PSK;
	else
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeOpen;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (IS_ENTRY_APCLI(&pAd->MacTab.Content[i]))
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}
		
    RTMPMakeRSNIE(pAd, pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode, pAd->ApCfg.ApCliTab[ifIndex].wdev.WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));

	pAd->ApCfg.ApCliTab[ifIndex].wdev.DefaultKeyId  = 0;

	if(pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode >= Ndis802_11AuthModeWPA)
		pAd->ApCfg.ApCliTab[ifIndex].wdev.DefaultKeyId = 1;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_AuthMode_Proc::(AuthMode=%d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode));		
	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set ApCli-IF Encryption Type
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;
	struct wifi_dev *wdev;


	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	wdev = &pApCliEntry->wdev;
	
	wdev->WepStatus = Ndis802_11WEPDisabled; 
	if (rtstrcasecmp(arg, "WEP") == TRUE)
    {
		if (wdev->AuthMode < Ndis802_11AuthModeWPA)
			wdev->WepStatus = Ndis802_11WEPEnabled;				  
	}
	else if (rtstrcasecmp(arg, "TKIP") == TRUE)
	{
		if (wdev->AuthMode >= Ndis802_11AuthModeWPA)
			wdev->WepStatus = Ndis802_11TKIPEnable;                       
    }
	else if ((rtstrcasecmp(arg, "AES") == TRUE) || (rtstrcasecmp(arg, "TKIPAES") == TRUE))
	{
		if (wdev->AuthMode >= Ndis802_11AuthModeWPA)
			wdev->WepStatus = Ndis802_11AESEnable;                            
	}    
	else
	{
		wdev->WepStatus = Ndis802_11WEPDisabled;                 
	}

	pApCliEntry->PairCipher     = pApCliEntry->wdev.WepStatus;
	pApCliEntry->GroupCipher    = pApCliEntry->wdev.WepStatus;
	pApCliEntry->bMixCipher		= FALSE;

	if (wdev->WepStatus >= Ndis802_11TKIPEnable)
		wdev->DefaultKeyId = 1;

	RTMPMakeRSNIE(pAd, wdev->AuthMode, wdev->WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));
	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_EncrypType_Proc::(EncrypType=%d)\n", ifIndex, wdev->WepStatus));

	return TRUE;
}



/* 
    ==========================================================================
    Description:
        Set Default Key ID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG 			KeyIdx;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pApCliEntry->wdev.DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  /*Invalid argument */
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", ifIndex, pApCliEntry->wdev.DefaultKeyId));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WPA PSK key for ApCli link

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 WPA pre-shared key string

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR ifIndex;
	POS_COOKIE pObj;
	PAPCLI_STRUCT   pApCliEntry = NULL;
	INT retval;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_ApCli_WPAPSK_Proc::(WPAPSK=%s)\n", arg));

	retval = RT_CfgSetWPAPSKKey(pAd, arg, strlen(arg), (PUCHAR)pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen, pApCliEntry->PMK);
	if (retval == FALSE)
		return FALSE;
	
	NdisMoveMemory(pApCliEntry->PSK, arg, strlen(arg));
	pApCliEntry->PSKLen = strlen(arg);

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY1 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;
	
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[0], 0);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key1_Proc::(Key1=%s) success!\n", ifIndex, arg));
	
	return retVal;
	
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY2 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[1], 1);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key2_Proc::(Key2=%s) success!\n", ifIndex, arg));
	
	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY3 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[2], 2);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key3_Proc::(Key3=%s) success!\n", ifIndex, arg));
	
	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY4 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[3], 3);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key4_Proc::(Key4=%s) success!\n", ifIndex, arg));
	
	return retVal;
}

INT Set_ApCli_TxMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{	
	POS_COOKIE 		pObj;	
	UCHAR 			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->wdev.DesiredTransmitSetting.field.FixedTxMode = 
								RT_CfgSetFixedTxPhyMode(arg);
	DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_TxMode_Proc = %d\n", ifIndex,
									pApCliEntry->wdev.DesiredTransmitSetting.field.FixedTxMode));

	return TRUE;
}

INT Set_ApCli_TxMcs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->wdev.DesiredTransmitSetting.field.MCS = 
			RT_CfgSetTxMCSProc(arg, &pApCliEntry->wdev.bAutoTxRateSwitch);

	if (pApCliEntry->wdev.DesiredTransmitSetting.field.MCS == MCS_AUTO)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_TxMcs_Proc = AUTO\n", ifIndex));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_TxMcs_Proc = %d\n", ifIndex, 
									pApCliEntry->wdev.DesiredTransmitSetting.field.MCS));
	}	

	return TRUE;
}

#ifdef WSC_AP_SUPPORT
INT Set_AP_WscSsid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR			ifIndex = pObj->ioctl_if;
	PWSC_CTRL	    pWscControl = &pAd->ApCfg.ApCliTab[ifIndex].WscControl;

    if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;
    
	NdisZeroMemory(&pWscControl->WscSsid, sizeof(NDIS_802_11_SSID));

	if( (strlen(arg) > 0) && (strlen(arg) <= MAX_LEN_OF_SSID))
    {
		NdisMoveMemory(pWscControl->WscSsid.Ssid, arg, strlen(arg));
		pWscControl->WscSsid.SsidLength = strlen(arg);

		NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, arg, strlen(arg));
		pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen = (UCHAR)strlen(arg);

		DBGPRINT(RT_DEBUG_TRACE, ("Set_WscSsid_Proc:: (Select SsidLen=%d,Ssid=%s)\n", 
				pWscControl->WscSsid.SsidLength, pWscControl->WscSsid.Ssid));
	}
	else
		return FALSE;	/*Invalid argument */

	return TRUE;	

}
#endif /* WSC_AP_SUPPORT */
#endif /* APCLI_SUPPORT */


#ifdef WSC_AP_SUPPORT
INT	 Set_AP_WscConfMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT         ConfModeIdx;
	/*INT         IsAPConfigured; */
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    BOOLEAN     bFromApCli = FALSE;
    PWSC_CTRL   pWscControl;
    APCLI_STRUCT *pApCliEntry;
    struct wifi_dev *wdev;


	ConfModeIdx = simple_strtol(arg, 0, 10);

#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/


#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
#ifdef P2P_SUPPORT
    if (pObj->ioctl_if_type == INT_P2P)
    {
		if (P2P_CLI_ON(pAd))
		{
			bFromApCli = TRUE;
			pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
		}
		else
		{
			bFromApCli = FALSE;
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
		}
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfMode_Proc:: This command is from p2p interface now.\n", apidx));
    }
    else
#endif /* P2P_SUPPORT */
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfMode_Proc:: This command is from ra interface now.\n", apidx));
    }
        
    pWscControl->bWscTrigger = FALSE;
    if ((ConfModeIdx & WSC_ENROLLEE_PROXY_REGISTRAR) == WSC_DISABLE)
    {
        pWscControl->WscConfMode = WSC_DISABLE;
		pWscControl->WscStatus = STATUS_WSC_NOTUSED;
        if (bFromApCli)
        {
            DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: WPS is disabled.\n", apidx));
        }
        else
        {
            DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfMode_Proc:: WPS is disabled.\n", apidx));
            /* Clear WPS IE in Beacon and ProbeResp */
            pAd->ApCfg.MBSSID[apidx].WscIEBeacon.ValueLen = 0;
        	pAd->ApCfg.MBSSID[apidx].WscIEProbeResp.ValueLen = 0;
			APUpdateBeaconFrame(pAd, apidx);
        }        
    }
    else
    {
#ifdef APCLI_SUPPORT
        if (bFromApCli)
        {
            if (ConfModeIdx == WSC_ENROLLEE)
            {
                pWscControl->WscConfMode = WSC_ENROLLEE;
                WscInit(pAd, TRUE, apidx);
            }
            else
            {
                DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: Ap Client only supports Enrollee mode.(ConfModeIdx=%d)\n", apidx, ConfModeIdx));
                return FALSE;
            }
        }
        else
#endif /* APCLI_SUPPORT */
        {
        	pWscControl->WscConfMode = (ConfModeIdx & WSC_ENROLLEE_PROXY_REGISTRAR);
            WscInit(pAd, FALSE, apidx | MIN_NET_DEVICE_FOR_P2P_GO);
        }
        pWscControl->WscStatus = STATUS_WSC_IDLE;
    }

#ifdef APCLI_SUPPORT

    if (bFromApCli)
    {
        memcpy(mac_addr, &pAd->ApCfg.ApCliTab[apidx].wdev.if_addr,MAC_ADDR_LEN);
    }
    else
#endif /* APCLI_SUPPORT */        
    {
        memcpy(mac_addr, &pAd->ApCfg.MBSSID[apidx].wdev.bssid[0], MAC_ADDR_LEN);
    }

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscConfMode_Proc::(WscConfMode(0~7)=%d)\n", 
                            mac_addr[0], 
                            mac_addr[1], 
                            mac_addr[2], 
                            mac_addr[3], 
                            mac_addr[4], 
                            mac_addr[5], 
                            pWscControl->WscConfMode));
	return TRUE;
}

INT	Set_AP_WscConfStatus_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR       IsAPConfigured = 0;
	INT         IsSelectedRegistrar;
	USHORT      WscMode;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;

#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/


#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfStatus_Proc:: Ap Client doesn't need this command.\n", apidx));
        return FALSE;
    }
#endif /* APCLI_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
	if ((pAd->ApCfg.MBSSID[apidx].WscControl.WscV2Info.bWpsEnable == FALSE) && 
		(pAd->ApCfg.MBSSID[apidx].WscControl.WscV2Info.bEnableWpsV2))
	{
		pAd->ApCfg.MBSSID[apidx].WscIEBeacon.ValueLen = 0;
       	pAd->ApCfg.MBSSID[apidx].WscIEProbeResp.ValueLen = 0;
		return FALSE;
	}
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef P2P_SUPPORT
    if (pObj->ioctl_if_type == INT_P2P)
    {
    	if (P2P_CLI_ON(pAd))
    	{
    		DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfStatus_Proc:: P2P Client doesn't need this command.\n", apidx));
			return FALSE;
    	}
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscConfStatus_Proc:: This command is from p2p interface now.\n", apidx));
    }
#endif /* P2P_SUPPORT */

	IsAPConfigured = (UCHAR)simple_strtol(arg, 0, 10);
	IsSelectedRegistrar = pAd->ApCfg.MBSSID[apidx].WscControl.WscSelReg;
    if (pAd->ApCfg.MBSSID[apidx].WscControl.WscMode == 1)
		WscMode = DEV_PASS_ID_PIN;
	else
		WscMode = DEV_PASS_ID_PBC;

	if ((IsAPConfigured  > 0) && (IsAPConfigured  <= 2))
    {   
        pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus = IsAPConfigured;
        /* Change SC State of WPS IE in Beacon and ProbeResp */
        WscBuildBeaconIE(pAd, IsAPConfigured, IsSelectedRegistrar, WscMode, 0, apidx, NULL, 0, AP_MODE);
    	WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, IsSelectedRegistrar, WscMode, 0, apidx | MIN_NET_DEVICE_FOR_P2P_GO, NULL, 0, AP_MODE);
		APUpdateBeaconFrame(pAd, apidx);
    }
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: Set failed!!(WscConfStatus=%s), WscConfStatus is 1 or 2 \n", apidx, arg));
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: WscConfStatus is not changed (%d) \n", apidx, pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus));
		return FALSE;  /*Invalid argument */
	}

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscConfStatus_Proc::(WscConfStatus=%d)\n", 
                               pAd->ApCfg.MBSSID[apidx].wdev.bssid[0],
                               pAd->ApCfg.MBSSID[apidx].wdev.bssid[1],
                               pAd->ApCfg.MBSSID[apidx].wdev.bssid[2],
                               pAd->ApCfg.MBSSID[apidx].wdev.bssid[3],
                               pAd->ApCfg.MBSSID[apidx].wdev.bssid[4],
                               pAd->ApCfg.MBSSID[apidx].wdev.bssid[5],
                               pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus));

	return TRUE;
}

INT	Set_AP_WscMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT         WscMode;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    PWSC_CTRL   pWscControl;
    BOOLEAN     bFromApCli = FALSE;
    APCLI_STRUCT *pApCliEntry;
    struct wifi_dev *wdev;

   
#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/
    
#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
#ifdef P2P_SUPPORT
    if (pObj->ioctl_if_type == INT_P2P)
    {
    	if (P2P_CLI_ON(pAd))
    	{
			bFromApCli = TRUE;
			pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
    	}
		else
		{
			bFromApCli = FALSE;
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
		}
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscMode_Proc:: This command is from p2p interface now.\n", apidx));
    }
    else
#endif /* P2P_SUPPORT */
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscMode_Proc:: This command is from ra interface now.\n", apidx));
    }

	WscMode = simple_strtol(arg, 0, 10);
    
    if ((WscMode  > 0) && (WscMode  <= 2))
    {
        pWscControl->WscMode = WscMode;
        if (WscMode == WSC_PBC_MODE)
        {
	        WscGetRegDataPIN(pAd, pWscControl->WscPinCode, pWscControl);
        }
    }
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set_WscMode_Proc:: Set failed!!(Set_WscMode_Proc=%s), WscConfStatus is 1 or 2 \n", arg));
        DBGPRINT(RT_DEBUG_TRACE, ("Set_WscMode_Proc:: WscMode is not changed (%d) \n", pWscControl->WscMode));
		return FALSE;  /*Invalid argument */
	}

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, MAC_ADDR_LEN);
    }
    else
#endif /* APCLI_SUPPORT */        
    {
        memcpy(mac_addr, pAd->ApCfg.MBSSID[apidx].wdev.bssid, MAC_ADDR_LEN);
    }
	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscMode_Proc::(WscMode=%d)\n", 
                                mac_addr[0],
                                mac_addr[1],
                                mac_addr[2],
                                mac_addr[3],
                                mac_addr[4],
                                mac_addr[5],
                                pWscControl->WscMode));

	return TRUE;
}

INT	Set_WscStatus_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;
    
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscStatus_Proc::(WscStatus=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus));
	return TRUE;
}

#define WSC_GET_CONF_MODE_EAP	1
#define WSC_GET_CONF_MODE_UPNP	2
INT	Set_AP_WscGetConf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT                 WscMode, wscGetConfMode = 0;
	INT                 IsAPConfigured;
	PWSC_CTRL           pWscControl;
	PWSC_UPNP_NODE_INFO pWscUPnPNodeInfo;
    INT	                idx;
    POS_COOKIE          pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	            apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    BOOLEAN             bFromApCli = FALSE;
#ifdef APCLI_SUPPORT
	BOOLEAN 			apcliEn = pAd->ApCfg.ApCliTab[apidx].Enable;
#endif /* APCLI_SUPPORT */
#ifdef WSC_V2_SUPPORT
	PWSC_V2_INFO		pWscV2Info = NULL;
#endif /* WSC_V2_SUPPORT */
#ifdef WSC_LED_SUPPORT
	UCHAR WPSLEDStatus;
#endif /* WSC_LED_SUPPORT */

#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
    	if (apcliEn == FALSE)
    	{
    		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscMode_Proc:: ApCli is disabled.\n", apidx));
    		return FALSE;
    	}
        bFromApCli = TRUE;
		apidx &= (~MIN_NET_DEVICE_FOR_APCLI);
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
#ifdef P2P_SUPPORT
    if (pObj->ioctl_if_type == INT_P2P)
    {
    	if (P2P_CLI_ON(pAd))
    	{
        	bFromApCli = TRUE;
			pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
    	}
		else
		{
			bFromApCli = FALSE;
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
		}
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_AP_WscGetConf_Proc:: This command is from p2p interface now.\n", apidx & 0x0F));
    }
    else
#endif /* P2P_SUPPORT */
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscMode_Proc:: This command is from ra interface now.\n", apidx));
    }

	NdisZeroMemory(mac_addr, MAC_ADDR_LEN);
#ifdef WSC_V2_SUPPORT
	pWscV2Info = &pWscControl->WscV2Info;
#endif /* WSC_V2_SUPPORT */
	wscGetConfMode = simple_strtol(arg, 0, 10);

    IsAPConfigured = pWscControl->WscConfStatus;
    pWscUPnPNodeInfo = &pWscControl->WscUPnPNodeInfo;

    if ((pWscControl->WscConfMode == WSC_DISABLE)
#ifdef WSC_V2_SUPPORT
		|| ((pWscV2Info->bWpsEnable) == FALSE && (pWscV2Info->bEnableWpsV2))
#endif /* WSC_V2_SUPPORT */
		)
    {
        pWscControl->bWscTrigger = FALSE;
        DBGPRINT(RT_DEBUG_TRACE, ("Set_WscGetConf_Proc: WPS is disabled.\n"));
		return FALSE;
    }

	WscStop(pAd, bFromApCli, pWscControl);
    
	/* trigger wsc re-generate public key */
    pWscControl->RegData.ReComputePke = 1;

	if (pWscControl->WscMode == 1)
		WscMode = DEV_PASS_ID_PIN;
	else
	{
		WscMode = DEV_PASS_ID_PBC;
	}
	WscInitRegistrarPair(pAd, pWscControl, apidx);
    /* Enrollee 192 random bytes for DH key generation */
	for (idx = 0; idx < 192; idx++)
		pWscControl->RegData.EnrolleeRandom[idx] = RandomByte(pAd);
    
#ifdef APCLI_SUPPORT
	if (bFromApCli)
    {
    	/* bring apcli interface down first */
		pAd->ApCfg.ApCliTab[apidx].Enable = FALSE;
		ApCliIfDown(pAd);
			
#if 0	
 		if (WscMode == DEV_PASS_ID_PIN)
    	{
			NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr,
	                       pAd->ApCfg.ApCliTab[apidx].CurrentAddress, 
	                       6);
	        
	        pAd->ApCfg.ApCliTab[apidx].Enable = apcliEn;
			NdisMoveMemory(mac_addr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, MAC_ADDR_LEN);
    	}
		else
		{
			pWscControl->WscSsid.SsidLength = 0;
			NdisZeroMemory(&pWscControl->WscSsid, sizeof(NDIS_802_11_SSID));
			pWscControl->WscPBCBssCount = 0;
			/* WPS - SW PBC */
			WscPushPBCAction(pAd, pWscControl);
		}
#else
		NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr,
	                       pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, 
	                       6);
	        
        pAd->ApCfg.ApCliTab[apidx].Enable = apcliEn;
		NdisMoveMemory(mac_addr, pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, MAC_ADDR_LEN);
#endif
    }
	else
#endif /* APCLI_SUPPORT */
	{
		INT WscDpid;
#ifdef P2P_SUPPORT
		/* As become P2P GO, update the DPID in Beacon and Probe Rsp. */
		if (pAd->P2pCfg.Dpid != DEV_PASS_ID_NOSPEC)
			WscDpid = pAd->P2pCfg.Dpid;
		else
#endif /* P2P_SUPPORT */
			WscDpid = WscMode;

		WscBuildBeaconIE(pAd, IsAPConfigured, TRUE, WscDpid, pWscControl->WscConfigMethods, apidx, NULL, 0, AP_MODE);
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, TRUE, WscDpid, pWscControl->WscConfigMethods, (apidx | MIN_NET_DEVICE_FOR_P2P_GO), NULL, 0, AP_MODE);
		APUpdateBeaconFrame(pAd, apidx);
		NdisMoveMemory(mac_addr, pAd->ApCfg.MBSSID[apidx].wdev.bssid, MAC_ADDR_LEN);
	}

#if 0
#ifdef APCLI_SUPPORT
	if (bFromApCli && (WscMode == DEV_PASS_ID_PBC))
		;
	else
#endif /* APCLI_SUPPORT */
#endif
	{
	    /* 2mins time-out timer */
	    RTMPSetTimer(&pWscControl->Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
	    pWscControl->Wsc2MinsTimerRunning = TRUE;
	    pWscControl->WscStatus = STATUS_WSC_LINK_UP;
		pWscControl->bWscTrigger = TRUE;
	}
	pWscControl->bWscAutoTigeer = FALSE;

    if (!bFromApCli)
    {
    	WscAssignEntryMAC(pAd, pWscControl);
		WscSendUPnPConfReqMsg(pAd, pWscControl->EntryIfIdx, 
	    							(PUCHAR)pAd->ApCfg.MBSSID[pWscControl->EntryIfIdx].Ssid, 
	    							pAd->ApCfg.MBSSID[apidx].wdev.bssid, 3, 0, AP_MODE);
    }

#ifdef WSC_LED_SUPPORT
#ifdef CONFIG_WIFI_LED_SHARE
	/* Change FW default mode to HUAWEI WPS mode*/
	pAd->LedCntl.MCULedCntl.word &= 0x80;
	pAd->LedCntl.MCULedCntl.word |= WPS_LED_MODE_SHARE;

	if (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
	{
		WPSLEDStatus = LED_WPS_PRE_STAGE;
		RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_PREPOST_WIFI_LED_TIMEOUT);
	}
	else
#endif /* CONFIG_WIFI_LED_SHARE */
		WPSLEDStatus = LED_WPS_IN_PROCESS;
		RTMPSetLED(pAd, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscGetConf_Proc trigger WSC state machine, wscGetConfMode=%d\n", 
                                mac_addr[0],
                                mac_addr[1],
                                mac_addr[2],
                                mac_addr[3],
                                mac_addr[4],
                                mac_addr[5],
                                wscGetConfMode));

	return TRUE;
}

INT	Set_AP_WscPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT        PinCode = 0;
	BOOLEAN     validatePin, bFromApCli = FALSE;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR       apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    PWSC_CTRL   pWscControl;
#define IsZero(c) ('0' == (c) ? TRUE:FALSE)
	PinCode = simple_strtol(arg, 0, 10); /* When PinCode is 03571361, return value is 3571361. */

#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscPinCode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
#ifdef P2P_SUPPORT
    if (pObj->ioctl_if_type == INT_P2P)
    {
			bFromApCli = FALSE;
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscMode_Proc:: This command is from p2p interface now.\n", apidx));
    }
#endif /* P2P_SUPPORT */
    
	if (strlen(arg) == 4)
		validatePin = TRUE;
	else
	validatePin = ValidateChecksum(PinCode);

	if ( validatePin )
	{
	    if (pWscControl->WscRejectSamePinFromEnrollee && 
            (PinCode == pWscControl->WscLastPinFromEnrollee))
        {
            DBGPRINT(RT_DEBUG_TRACE, ("PIN authentication or communication error occurs!!\n"
                                      "Registrar does NOT accept the same PIN again!(PIN:%s)\n", arg));
            return FALSE;
        }
        else
        {
    		pWscControl->WscPinCode = PinCode;
            pWscControl->WscLastPinFromEnrollee = pWscControl->WscPinCode;
            pWscControl->WscRejectSamePinFromEnrollee = FALSE;
            /* PIN Code */
			if (strlen(arg) == 4)
			{
				pWscControl->WscPinCodeLen = 4;
				pWscControl->RegData.PinCodeLen = 4;
				NdisMoveMemory(pWscControl->RegData.PIN, arg, 4);
			}
			else
			{
				pWscControl->WscPinCodeLen = 8;

            if (IsZero(*arg))
				{
					pWscControl->RegData.PinCodeLen = 8;
                NdisMoveMemory(pWscControl->RegData.PIN, arg, 8);
				}
            else
    	        WscGetRegDataPIN(pAd, pWscControl->WscPinCode, pWscControl);
        }        
	}
#ifdef P2P_SUPPORT
	    if (pObj->ioctl_if_type == INT_P2P)
	    {
	    	PWSC_CTRL   pP2pCliWscCtrl;
			pP2pCliWscCtrl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
			pP2pCliWscCtrl->WscPinCodeLen = pWscControl->WscPinCodeLen;
			pP2pCliWscCtrl->WscPinCode = pWscControl->WscPinCode;
			WscGetRegDataPIN(pAd, pP2pCliWscCtrl->WscPinCode, pP2pCliWscCtrl);
	    }
#endif /* P2P_SUPPORT */
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set failed!!(Set_WscPinCode_Proc=%s), PinCode Checksum invalid \n", arg));
		return FALSE;  /*Invalid argument */
	}

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, MAC_ADDR_LEN);
    }
    else
#endif /* APCLI_SUPPORT */        
    {
        memcpy(mac_addr, pAd->ApCfg.MBSSID[apidx].wdev.bssid, MAC_ADDR_LEN);
    }
	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscPinCode_Proc::(PinCode=%d)\n", 
                                mac_addr[0],
                                mac_addr[1],
                                mac_addr[2],
                                mac_addr[3],
                                mac_addr[4],
                                mac_addr[5],
                                pWscControl->WscPinCode));

	return TRUE;
}

INT	Set_WscOOB_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    char        *pTempSsid = NULL;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR       apidx = pObj->ioctl_if;
    
#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscPinCode_Proc:: Ap Client doesn't need this command.\n", apidx));
        return FALSE;
    }
#endif /* APCLI_SUPPORT */

    Set_AP_WscConfStatus_Proc(pAd, "1");
    Set_AP_AuthMode_Proc(pAd, "WPAPSK");
    Set_AP_EncrypType_Proc(pAd, "TKIP");
    pTempSsid = vmalloc(33);
    if (pTempSsid)
    {
        memset(pTempSsid, 0, 33);
        snprintf(pTempSsid, 33,"RalinkInitialAP%02X%02X%02X", pAd->ApCfg.MBSSID[apidx].wdev.bssid[3],
                                                          pAd->ApCfg.MBSSID[apidx].wdev.bssid[4],
                                                          pAd->ApCfg.MBSSID[apidx].wdev.bssid[5]);
        Set_AP_SSID_Proc(pAd, pTempSsid);
        vfree(pTempSsid);
    }
	Set_AP_WPAPSK_Proc(pAd, "RalinkInitialAPxx1234");
    
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscOOB_Proc\n", apidx));
	return TRUE;
}

INT	Set_WscStop_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;
    PWSC_CTRL   pWscControl;
    BOOLEAN     bFromApCli = FALSE;
    
#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return FALSE;
	}
#endif /*HOSTAPD_SUPPORT*/

    
#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscStop_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscStop_Proc:: This command is from ra interface now.\n", apidx));
    }

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
	    WscStop(pAd, TRUE, pWscControl);
		pWscControl->WscConfMode = WSC_DISABLE;
    }
    else
#endif /* APCLI_SUPPORT */
    {
        INT	 IsAPConfigured = pWscControl->WscConfStatus;
        WscBuildBeaconIE(pAd, IsAPConfigured, FALSE, 0, 0, apidx, NULL, 0, AP_MODE);
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0, apidx | MIN_NET_DEVICE_FOR_P2P_GO, NULL, 0, AP_MODE);
		APUpdateBeaconFrame(pAd, apidx);
        WscStop(pAd, FALSE, pWscControl);
    }

    pWscControl->bWscTrigger = FALSE;
    DBGPRINT(RT_DEBUG_TRACE, ("<===== Set_WscStop_Proc"));
    return TRUE;
}

/* 
    ==========================================================================
    Description:
        Get WSC Profile
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) iwpriv ra0 get_wsc_profile
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
VOID RTMPIoctlWscProfile(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	WSC_CONFIGURED_VALUE Profile;
	RTMP_STRING *msg;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;

#ifdef HOSTAPD_SUPPORT
	if (pAd->ApCfg.MBSSID[apidx].Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return;
	}
#endif /*HOSTAPD_SUPPORT*/


	memset(&Profile, 0x00, sizeof(WSC_CONFIGURED_VALUE));
	Profile.WscConfigured = pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus;
	NdisZeroMemory(Profile.WscSsid, 32 + 1);
	NdisMoveMemory(Profile.WscSsid, pAd->ApCfg.MBSSID[apidx].Ssid, 
								    pAd->ApCfg.MBSSID[apidx].SsidLen);
	Profile.WscSsid[pAd->ApCfg.MBSSID[apidx].SsidLen] = '\0';
	if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		Profile.WscAuthMode = WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK;
	else
		Profile.WscAuthMode = WscGetAuthType(pAd->ApCfg.MBSSID[apidx].wdev.AuthMode);
	if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11TKIPAESMix)
		Profile.WscEncrypType = WSC_ENCRTYPE_TKIP |WSC_ENCRTYPE_AES;
	else
		Profile.WscEncrypType = WscGetEncryType(pAd->ApCfg.MBSSID[apidx].wdev.WepStatus);
	NdisZeroMemory(Profile.WscWPAKey, 64 + 1);

	if (Profile.WscEncrypType == 2)
	{
		Profile.DefaultKeyIdx = pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId + 1;
#if 0
		if (pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].WepKeyType == WEP_ASCII_TYPE)
		{
			int i;
			for (i=0; i<pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].KeyLen; i++)
			{
				sprintf(Profile.WscWPAKey,
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].KeyLen)*2] = '\0';
	}
		else /* Hex WEP Key */
#endif /* if 0 */
		{
			int i;
			for (i=0; i<pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId].KeyLen; i++)
			{
				snprintf((RTMP_STRING *) Profile.WscWPAKey, sizeof(Profile.WscWPAKey),
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId].KeyLen)*2] = '\0';
		}
	}
	else if (Profile.WscEncrypType >= 4)
	{
		Profile.DefaultKeyIdx = 2;
		NdisMoveMemory(Profile.WscWPAKey, pAd->ApCfg.MBSSID[apidx].WscControl.WpaPsk, 
						pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen);
		Profile.WscWPAKey[pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen] = '\0';
	}
	else
	{
		Profile.DefaultKeyIdx = 1;
	}
	
	wrq->u.data.length = sizeof(Profile);

	if (copy_to_user(wrq->u.data.pointer, &Profile, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

/*	msg = (RTMP_STRING *)kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG); */
	os_alloc_mem(pAd, (UCHAR **)&msg, sizeof(CHAR)*(2048));
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 2048);
	sprintf(msg,"%s","\n");

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s\n", "Configured", "SSID", "AuthMode", "EncrypType");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-13s%-26s\n", "Configured", "SSID", "AuthMode", "EncrypType", "DefaultKeyID", "Key");
	}
	else
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-64s\n", "Configured", "SSID", "AuthMode", "EncrypType", "Key");
	}

	if (Profile.WscConfigured == 1)
		sprintf(msg+strlen(msg),"%-12s", "No");
	else
		sprintf(msg+strlen(msg),"%-12s", "Yes");
	sprintf(msg+strlen(msg), "%-33s", Profile.WscSsid);
	if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		sprintf(msg+strlen(msg), "%-12s", "WPAPSKWPA2PSK");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11TKIPAESMix)
		sprintf(msg+strlen(msg), "%-12s", "TKIPAES");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetEncryTypeStr(Profile.WscEncrypType));

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg), "%s\n", "");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg), "%-13d",Profile.DefaultKeyIdx);
		sprintf(msg+strlen(msg), "%-26s\n",Profile.WscWPAKey);
	}
	else if (Profile.WscEncrypType >= 4)
	{
	    sprintf(msg+strlen(msg), "%-64s\n",Profile.WscWPAKey);
	}
#ifdef INF_AR9
	wrq->u.data.length = strlen(msg);
	copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
#endif/* INF_AR9 */

	DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
/*	kfree(msg); */
	os_free_mem(NULL, msg);
}

#ifdef INF_AR9
#ifdef AR9_MAPI_SUPPORT

/* 
    ==========================================================================
    Description:
        Get WSC Profile
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) iwpriv ra0 ar9_show get_wsc_profile
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
VOID RTMPAR9IoctlWscProfile(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	WSC_CONFIGURED_VALUE Profile;
	RTMP_STRING *msg;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;

	memset(&Profile, 0x00, sizeof(WSC_CONFIGURED_VALUE));
	Profile.WscConfigured = pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus;
	NdisZeroMemory(Profile.WscSsid, 32 + 1);
	NdisMoveMemory(Profile.WscSsid, pAd->ApCfg.MBSSID[apidx].Ssid, 
								    pAd->ApCfg.MBSSID[apidx].SsidLen);
	Profile.WscSsid[pAd->ApCfg.MBSSID[apidx].SsidLen] = '\0';
	if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		Profile.WscAuthMode = WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK;
	else
		Profile.WscAuthMode = WscGetAuthType(pAd->ApCfg.MBSSID[apidx].AuthMode);
	if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11TKIPAESMix)
		Profile.WscEncrypType = WSC_ENCRTYPE_TKIP |WSC_ENCRTYPE_AES;
	else
		Profile.WscEncrypType = WscGetEncryType(pAd->ApCfg.MBSSID[apidx].WepStatus);
	NdisZeroMemory(Profile.WscWPAKey, 64 + 1);

	if (Profile.WscEncrypType == 2)
	{
		Profile.DefaultKeyIdx = pAd->ApCfg.MBSSID[apidx].DefaultKeyId + 1;
		{
			int i;
			for (i=0; i<pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].KeyLen; i++)
			{
				snprintf((RTMP_STRING *) Profile.WscWPAKey, sizeof(Profile.WscWPAKey),
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].KeyLen)*2] = '\0';
		}
	}
	else if (Profile.WscEncrypType >= 4)
	{
		Profile.DefaultKeyIdx = 2;
		NdisMoveMemory(Profile.WscWPAKey, pAd->ApCfg.MBSSID[apidx].WscControl.WpaPsk, 
						pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen);
		Profile.WscWPAKey[pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen] = '\0';
	}
	else
	{
		Profile.DefaultKeyIdx = 1;
	}


/*	msg = (RTMP_STRING *)kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG); */
	os_alloc_mem(pAd, (UCHAR **)&msg, sizeof(CHAR)*(2048));
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 2048);
	sprintf(msg,"%s","\n");

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s\n", "Configured", "SSID", "AuthMode", "EncrypType");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-13s%-26s\n", "Configured", "SSID", "AuthMode", "EncrypType", "DefaultKeyID", "Key");
	}
	else
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-64s\n", "Configured", "SSID", "AuthMode", "EncrypType", "Key");
	}

	if (Profile.WscConfigured == 1)
		sprintf(msg+strlen(msg),"%-12s", "No");
	else
		sprintf(msg+strlen(msg),"%-12s", "Yes");
	sprintf(msg+strlen(msg), "%-33s", Profile.WscSsid);
	if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		sprintf(msg+strlen(msg), "%-12s", "WPAPSKWPA2PSK");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11TKIPAESMix)
		sprintf(msg+strlen(msg), "%-12s", "TKIPAES");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetEncryTypeStr(Profile.WscEncrypType));

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg), "%s\n", "");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg), "%-13d",Profile.DefaultKeyIdx);
		sprintf(msg+strlen(msg), "%-26s\n",Profile.WscWPAKey);
	}
	else if (Profile.WscEncrypType >= 4)
	{
	    sprintf(msg+strlen(msg), "%-64s\n",Profile.WscWPAKey);
	}

	wrq->u.data.length = strlen(msg);
	copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
	}
/*	kfree(msg); */
	os_free_mem(NULL, msg);
}

VOID RTMPIoctlWscPINCode(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	RTMP_STRING *msg;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;
	UCHAR        tempPIN[9]={0};

/*	msg = (RTMP_STRING *)kmalloc(sizeof(CHAR)*(128), MEM_ALLOC_FLAG); */
	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR)*(128));
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 128);
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"WSC_PINCode=");
	if(pAd->ApCfg.MBSSID[apidx].WscControl.WscEnrolleePinCode)
	{
		if (pAd->ApCfg.MBSSID[apidx].WscControl.WscEnrolleePinCodeLen == 8)
			sprintf((RTMP_STRING *) tempPIN, "%08u", pAd->ApCfg.MBSSID[apidx].WscControl.WscEnrolleePinCode);
		else
			sprintf((RTMP_STRING *) tempPIN, "%04u", pAd->ApCfg.MBSSID[apidx].WscControl.WscEnrolleePinCode);
		sprintf(msg,"%s%s\n",msg,tempPIN);
	}
	wrq->u.data.length = strlen(msg);
	copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
	}
/*	kfree(msg); */
	os_free_mem(NULL, msg);
}

VOID RTMPIoctlWscStatus(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	RTMP_STRING *msg;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;

/*	msg = (RTMP_STRING *)kmalloc(sizeof(CHAR)*(128), MEM_ALLOC_FLAG); */
	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR)*(128));
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 128);
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"WSC_Status=");
	sprintf(msg,"%s%d\n",msg,pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus);
	wrq->u.data.length = strlen(msg);
	copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
	}
/*	kfree(msg); */
	os_free_mem(NULL, msg);
}

VOID RTMPIoctlGetWscDynInfo(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	char *msg;
	BSS_STRUCT *pMbss;
	INT apidx,configstate;


/*	msg = kmalloc(sizeof(CHAR)*(pAd->ApCfg.BssidNum*(14*128)), MEM_ALLOC_FLAG); */
	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR)*(pAd->ApCfg.BssidNum*(14*128)));
	if (msg == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Alloc memory failed\n", __FUNCTION__));
		return;
	}
	memset(msg, 0 ,pAd->ApCfg.BssidNum*(14*128));
	sprintf(msg,"%s","\n");
	
	for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
	{
		pMbss=&pAd->ApCfg.MBSSID[apidx];

		if(pMbss->WscControl.WscConfStatus == WSC_SCSTATE_UNCONFIGURED)
			configstate = 0;
		else
			configstate = 1;
		
		sprintf(msg+strlen(msg),"ra%d\n",apidx);
		sprintf(msg+strlen(msg),"UUID = %s\n",(pMbss->WscControl.Wsc_Uuid_Str));
		sprintf(msg+strlen(msg),"wpsVersion = 0x%x\n",WSC_VERSION);
		sprintf(msg+strlen(msg),"setuoLockedState = %d\n",0);
		sprintf(msg+strlen(msg),"configstate = %d\n",configstate);
		sprintf(msg+strlen(msg),"lastConfigError = %d\n",0);
		
	}

	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s", msg));
	}

/*	kfree(msg); */
	os_free_mem(NULL, msg);
}

VOID RTMPIoctlGetWscRegsDynInfo(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	char *msg;
	BSS_STRUCT *pMbss;
	INT apidx;


/*	msg = kmalloc(sizeof(CHAR)*(pAd->ApCfg.BssidNum*(14*128)), MEM_ALLOC_FLAG); */
	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR)*(pAd->ApCfg.BssidNum*(14*128)));
	if (msg == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Alloc memory failed\n", __FUNCTION__));
		return;
	}
	memset(msg, 0 ,pAd->ApCfg.BssidNum*(14*128));
	sprintf(msg,"%s","\n");
	
	for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
	{
		pMbss=&pAd->ApCfg.MBSSID[apidx];
		sprintf(msg+strlen(msg),"ra%d\n",apidx);
		sprintf(msg+strlen(msg),"UUID_R = %s\n",(pMbss->WscControl.RegData.PeerInfo.Uuid));	
	}

	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s", msg));
	}

/*	kfree(msg); */
	os_free_mem(NULL, msg);
}
#endif /*AR9_MAPI_SUPPORT*/
#endif/* INF_AR9 */
BOOLEAN WscCheckEnrolleeNonceFromUpnp(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	RTMP_STRING *pData,
	IN  USHORT			Length,
	IN  PWSC_CTRL       pWscControl) 
{
	USHORT	WscType, WscLen;
    USHORT  WscId = WSC_ID_ENROLLEE_NONCE;

    DBGPRINT(RT_DEBUG_TRACE, ("check Enrollee Nonce\n"));
   
    /* We have to look for WSC_IE_MSG_TYPE to classify M2 ~ M8, the remain size must large than 4 */
	while (Length > 4)
	{
		WSC_IE	TLV_Recv;
        char ZeroNonce[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        
		memcpy((UINT8 *)&TLV_Recv, pData, 4);
		WscType = be2cpu16(TLV_Recv.Type);
		WscLen  = be2cpu16(TLV_Recv.Length);
		pData  += 4;
		Length -= 4;
        
		if (WscType == WscId)
		{
			if (RTMPCompareMemory(pWscControl->RegData.SelfNonce, pData, 16) == 0)
			{
			    DBGPRINT(RT_DEBUG_TRACE, ("Nonce match!!\n"));
                DBGPRINT(RT_DEBUG_TRACE, ("<----- WscCheckNonce\n"));
				return TRUE;
			}
            else if (NdisEqualMemory(pData, ZeroNonce, 16))
            {
                /* Intel external registrar will send WSC_NACK with enrollee nonce */
                /* "10 1A 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" */
                /* when AP is configured and user selects not to configure AP. */
                DBGPRINT(RT_DEBUG_TRACE, ("Zero Enrollee Nonce!!\n"));
                DBGPRINT(RT_DEBUG_TRACE, ("<----- WscCheckNonce\n"));
                return TRUE;
            }
		}
        
		/* Offset to net WSC Ie */
		pData  += WscLen;
		Length -= WscLen;
	}

    DBGPRINT(RT_DEBUG_TRACE, ("Nonce mismatch!!\n"));
    return FALSE;
}

UCHAR WscRxMsgTypeFromUpnp(
	IN	PRTMP_ADAPTER		pAdapter,
	IN  RTMP_STRING *pData,
	IN	USHORT				Length) 
{
	
	USHORT WscType, WscLen;
    
    {   /* Eap-Esp(Messages) */
        /* the first TLV item in EAP Messages must be WSC_IE_VERSION */
        NdisMoveMemory(&WscType, pData, 2);
        if (ntohs(WscType) != WSC_ID_VERSION)
            goto out;

        /* Not Wsc Start, We have to look for WSC_IE_MSG_TYPE to classify M2 ~ M8, the remain size must large than 4 */
		while (Length > 4)
		{
			/* arm-cpu has packet alignment issue, it's better to use memcpy to retrieve data */
			NdisMoveMemory(&WscType, pData, 2);
			NdisMoveMemory(&WscLen,  pData + 2, 2);
			WscLen = ntohs(WscLen);
			if (ntohs(WscType) == WSC_ID_MSG_TYPE)
			{
				return(*(pData + 4));	/* Found the message type */
			}
			else
			{
				pData  += (WscLen + 4);
				Length -= (WscLen + 4);
			}
		}
    }

out:
	return  WSC_MSG_UNKNOWN;
}

VOID RTMPIoctlSetWSCOOB(
	IN PRTMP_ADAPTER pAd)
{
    char        *pTempSsid = NULL;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR       apidx = pObj->ioctl_if;

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscPinCode_Proc:: Ap Client doesn't need this command.\n", apidx));
        return;
    }
#endif /* APCLI_SUPPORT */

    Set_AP_WscConfStatus_Proc(pAd, "1");
    Set_AP_AuthMode_Proc(pAd, "WPAPSK");
    Set_AP_EncrypType_Proc(pAd, "TKIP");
    pTempSsid = vmalloc(33);
    if (pTempSsid)
    {
        memset(pTempSsid, 0, 33);
        snprintf(pTempSsid, 33, "RalinkInitialAP%02X%02X%02X", pAd->ApCfg.MBSSID[apidx].wdev.bssid[3],
                                                          pAd->ApCfg.MBSSID[apidx].wdev.bssid[4],
                                                          pAd->ApCfg.MBSSID[apidx].wdev.bssid[5]);
        Set_AP_SSID_Proc(pAd, pTempSsid);
        vfree(pTempSsid);
    }
	Set_AP_WPAPSK_Proc(pAd, "RalinkInitialAPxx1234");
    
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscOOB_Proc\n", apidx));
	return;
}

/*     
	==========================================================================    
	Description:       	
	Set Wsc Security Mode        
	0 : WPA2PSK AES         
	1 : WPA2PSK TKIP         
	2 : WPAPSK AES        
	3 : WPAPSK TKIP
	Return:        
	TRUE if all parameters are OK, FALSE otherwise    
	==========================================================================
*/
INT Set_AP_WscSecurityMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{	
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;	
	UCHAR		apidx = pObj->ioctl_if;	
	
	if (strcmp(arg, "0") == 0)		
		pAd->ApCfg.MBSSID[apidx].WscSecurityMode = WPA2PSKAES;	
	else if (strcmp(arg, "1") == 0)		
		pAd->ApCfg.MBSSID[apidx].WscSecurityMode = WPA2PSKTKIP;	
	else if (strcmp(arg, "2") == 0)		
		pAd->ApCfg.MBSSID[apidx].WscSecurityMode = WPAPSKAES;	
	else if (strcmp(arg, "3") == 0)		
		pAd->ApCfg.MBSSID[apidx].WscSecurityMode = WPAPSKTKIP;	
	else		
		return FALSE;	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscSecurityMode_Proc::(WscSecurityMode=%d)\n", 
		apidx, pAd->ApCfg.MBSSID[apidx].WscSecurityMode ));	
	
	return TRUE;
}

INT Set_AP_WscMultiByteCheck_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;	
	UCHAR			apidx = pObj->ioctl_if;	
	BOOLEAN			bEnable = FALSE;
	PWSC_CTRL		pWpsCtrl = NULL;
	BOOLEAN     	bFromApCli = FALSE;

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWpsCtrl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
    {
        bFromApCli = FALSE;
        pWpsCtrl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfMode_Proc:: This command is from ra interface now.\n", apidx));
    }

	if (strcmp(arg, "0") == 0)		
		bEnable = FALSE;
	else if (strcmp(arg, "1") == 0)		
		bEnable = TRUE;
	else		
		return FALSE;	

	if (pWpsCtrl->bCheckMultiByte != bEnable)
	{
		pWpsCtrl->bCheckMultiByte = bEnable;
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AP_WscMultiByteCheck_Proc::(bCheckMultiByte=%d)\n", 
		apidx, pWpsCtrl->bCheckMultiByte));
	
	return TRUE;
}

INT Set_WscVersion_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	version = (UCHAR)simple_strtol(arg, 0, 16);	

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscVersion_Proc::(version=%x)\n",version));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.RegData.SelfInfo.Version = version;
	return TRUE;
}

#ifdef WSC_V2_SUPPORT
INT Set_WscFragment_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	bool_flag = (UCHAR)simple_strtol(arg, 0, 16);	

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscFragment_Proc::(bool_flag=%d)\n",bool_flag));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.bWscFragment = bool_flag;
	return TRUE;
}

INT Set_WscFragmentSize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	USHORT		WscFragSize = (USHORT)simple_strtol(arg, 0, 10);	

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscFragmentSize_Proc::(WscFragSize=%d)\n", WscFragSize));
	if ((WscFragSize >=128) && (WscFragSize <=300))
		pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscFragSize = WscFragSize;
	return TRUE;
}

INT Set_WscSetupLock_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		bEnable = (UCHAR)simple_strtol(arg, 0, 10);
	PWSC_CTRL	pWscControl = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl;

	if (bEnable == 0)
		pWscControl->bSetupLock = FALSE;
	else
		pWscControl->bSetupLock = TRUE;
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscSetupLock_Proc::(bSetupLock=%d)\n",
								pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.bSetupLock));
	return TRUE;
}

INT Set_WscV2Support_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		bEnable = (UCHAR)simple_strtol(arg, 0, 10);
	PWSC_CTRL	pWscControl = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl;
	INT 		IsAPConfigured = pWscControl->WscConfStatus;		

	if (bEnable == 0)
		pWscControl->WscV2Info.bEnableWpsV2 = FALSE;
	else
		pWscControl->WscV2Info.bEnableWpsV2 = TRUE;

	if (pWscControl->WscV2Info.bEnableWpsV2)
	{
		/*
			WPS V2 doesn't support WEP and WPA/WPAPSK-TKIP.
		*/
		if ((pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.WepStatus == Ndis802_11WEPEnabled) || 
			(pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.WepStatus == Ndis802_11TKIPEnable) ||
			(pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid))
			WscOnOff(pAd, pObj->ioctl_if | MIN_NET_DEVICE_FOR_P2P_GO, TRUE);
		else
			WscOnOff(pAd, pObj->ioctl_if | MIN_NET_DEVICE_FOR_P2P_GO, FALSE);
		APUpdateBeaconFrame(pAd, pObj->ioctl_if);
	}
	else
	{
		WscInit(pAd, FALSE, (pObj->ioctl_if | MIN_NET_DEVICE_FOR_P2P_GO));
	}	
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscV2Support_Proc::(bEnableWpsV2=%d)\n",
								pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.bEnableWpsV2));
	return TRUE;
}

INT Set_WscVersion2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	version = (UCHAR)simple_strtol(arg, 0, 16);	

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscVersion2_Proc::(version=%x)\n",version));
	if (version >= 0x20)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.RegData.SelfInfo.Version2 = version;
	else
		return FALSE;
	return TRUE;
}

INT Set_WscExtraTlvTag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	USHORT		new_tag = (USHORT)simple_strtol(arg, 0, 16);
	
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.ExtraTlv.TlvTag = new_tag;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscExtraTlvTag_Proc::(new_tag=0x%04X)\n",new_tag));
	return TRUE;
}

INT Set_WscExtraTlvType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		type = (UCHAR)simple_strtol(arg, 0, 10);
	
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.ExtraTlv.TlvType = type;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscExtraTlvType_Proc::(type=%d)\n",type));
	return TRUE;
}

INT Set_WscExtraTlvData_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UINT			DataLen = (UINT)strlen(arg);
	PWSC_TLV		pWscTLV = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.ExtraTlv;
	INT				i;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscExtraTlvData_Proc::(DataLen = %d)\n", DataLen));

	if ((DataLen != 0) && (pWscTLV->TlvType == TLV_HEX))
	{
		for(i=0; i < DataLen; i++)
		{
			if( !isxdigit(*(arg+i)) )
				return FALSE;  /*Not Hex value; */
		}
	}
	
	if (pWscTLV->pTlvData)
	{
		os_free_mem(NULL, pWscTLV->pTlvData);
		pWscTLV->pTlvData = NULL;
	}

	if (DataLen == 0)
		return TRUE;		

	pWscTLV->TlvLen = 0;
	os_alloc_mem(NULL, &pWscTLV->pTlvData, DataLen);
	if (pWscTLV->pTlvData)
	{
		if (pWscTLV->TlvType == TLV_ASCII)
		{
			NdisMoveMemory(pWscTLV->pTlvData, arg, DataLen);
			pWscTLV->TlvLen = DataLen;
		}
		else
		{				
			pWscTLV->TlvLen = DataLen/2;
			AtoH(arg, pWscTLV->pTlvData, pWscTLV->TlvLen);
		}
		return TRUE;
	}
	else
		DBGPRINT(RT_DEBUG_TRACE, ("Set_WscExtraTlvData_Proc::os_alloc_mem fail\n"));

	return FALSE;
}
#endif /* WSC_V2_SUPPORT */
#endif /* WSC_AP_SUPPORT */

#ifdef EASY_CONFIG_SETUP 
INT Set_AP_AutoProvisionEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 			pObj = (POS_COOKIE) pAd->OS_Cookie;	
	UCHAR		apidx = pObj->ioctl_if;	
	PEASY_CONFIG_INFO	pEasyConfig = &pAd->ApCfg.MBSSID[apidx].EasyConfigInfo;
	BOOLEAN				bEnable = FALSE;
	
	if (strcmp(arg, "0") == 0)		
		bEnable = FALSE;
	else if (strcmp(arg, "1") == 0)		
		bEnable = TRUE;
	else		
		return FALSE;	

	if (pEasyConfig->bEnable != bEnable)
	{
		pEasyConfig->bEnable = bEnable;
#ifdef WSC_AP_SUPPORT
		WscInit(pAd, FALSE, apidx);
#endif /* WSC_AP_SUPPORT */
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AP_AutoProvisionEnable_Proc::(AutoProvisionEnable=%d)\n", 
		apidx, pEasyConfig->bEnable));
	
	return TRUE;
}

INT Set_RssiThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR 				Value;
	PEASY_CONFIG_INFO 	pEasyConfig = NULL;
	POS_COOKIE 			pObj = (POS_COOKIE) pAd->OS_Cookie;	
	UCHAR				apidx = pObj->ioctl_if;	

	if (apidx != MAIN_MBSSID)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_RssiThreshold_Proc:: Only support ra0\n",apidx));
		return FALSE;
	}
	Value = (CHAR)simple_strtol(arg, 0, 10);

	pEasyConfig = &pAd->ApCfg.MBSSID[apidx].EasyConfigInfo; 

	if (Value > 70 || Value < 40)
		pEasyConfig->RssiThreshold = -50;
	else    
		pEasyConfig->RssiThreshold = (CHAR)(-1)*Value;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_RssiThreshold_Proc::(RssiThreshold=%d)\n",pEasyConfig->RssiThreshold));

	return TRUE;																																	
}		

#endif /* EASY_CONFIG_SETUP */

#ifdef WAC_SUPPORT
INT Set_AP_WACEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 			pObj = (POS_COOKIE) pAd->OS_Cookie;	
	UCHAR				apidx = pObj->ioctl_if;	
	PEASY_CONFIG_INFO	pEasyConfig = &pAd->ApCfg.MBSSID[apidx].EasyConfigInfo;
	BOOLEAN				bEnable = FALSE;
	
	if (strcmp(arg, "0") == 0)		
		bEnable = FALSE;
	else if (strcmp(arg, "1") == 0)		
		bEnable = TRUE;
	else		
		return FALSE;

	pEasyConfig->bEnableWAC = bEnable;
	APUpdateBeaconFrame(pAd, pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AP_WACEnable_Proc::(bEnableWAC=%d)\n", 
		apidx, pEasyConfig->bEnableWAC));
	
	return TRUE;
}
#endif /* WAC_SUPPORT */

#ifdef IAPP_SUPPORT
INT Set_IappPID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	unsigned long IappPid;
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;

	IappPid = simple_strtol(arg, 0, 10);
	RTMP_GET_OS_PID(pObj->IappPid, IappPid);
	pObj->IappPid_nr = IappPid;

/*	DBGPRINT(RT_DEBUG_TRACE, ("pObj->IappPid = %d", GET_PID_NUMBER(pObj->IappPid))); */
	return TRUE;
}
#endif /* IAPP_SUPPORT */


INT Set_DisConnectSta_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
	RTMP_STRING *value;
	INT						i;
	UCHAR HashIdx;
	MAC_TABLE_ENTRY *pEntry = NULL;

	if(strlen(arg) != 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
		return FALSE;

	for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) 
	{
		if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
			return FALSE;  /*Invalid */

		AtoH(value, &macAddr[i++], 1);
	}

	HashIdx = MAC_ADDR_HASH_INDEX(macAddr);
	pEntry = pAd->MacTab.Hash[HashIdx];

	if (pEntry)
	{
		MlmeDeAuthAction(pAd, pEntry, REASON_DISASSOC_STA_LEAVING, FALSE);
/*		MacTableDeleteEntry(pAd, pEntry->wcid, Addr); */
	}

	return TRUE;
}

INT Set_DisConnectAllSta_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	MacTableReset(pAd, 1);

	return TRUE;
}


#ifdef DOT1X_SUPPORT
/* 
    ==========================================================================
    Description:
        Set IEEE8021X.
        This parameter is 1 when 802.1x-wep turn on, otherwise 0
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_IEEE8021X_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    ULONG ieee8021x;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	ieee8021x = simple_strtol(arg, 0, 10);

	if (ieee8021x == 1)
        pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X = TRUE;
	else if (ieee8021x == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X = FALSE;
	else
		return FALSE;  /*Invalid argument */
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_IEEE8021X_Proc::(IEEE8021X=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set pre-authentication enable or disable when WPA/WPA2 turn on
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_PreAuth_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    ULONG PreAuth;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	PreAuth = simple_strtol(arg, 0, 10);

	if (PreAuth == 1)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].PreAuth = TRUE;
	else if (PreAuth == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].PreAuth = FALSE;
	else
		return FALSE;  /*Invalid argument */
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_PreAuth_Proc::(PreAuth=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].PreAuth));

	return TRUE;
}

INT Set_OwnIPAddr_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32		ip_addr;

	if (rtinet_aton(arg, &ip_addr))
 	{
        pAd->ApCfg.own_ip_addr = ip_addr;  
		DBGPRINT(RT_DEBUG_TRACE, ("own_ip_addr=%s(%x)\n", arg, pAd->ApCfg.own_ip_addr));
	}	 
	return TRUE;
}

INT Set_EAPIfName_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT			i;
	RTMP_STRING *macptr;	

	for (i=0, macptr = rstrtok(arg,";"); (macptr && i < MAX_MBSSID_NUM(pAd)); macptr = rstrtok(NULL,";"), i++) 
	{
		if (strlen(macptr) > 0)
		{
			pAd->ApCfg.EAPifname_len[i] = strlen(macptr); 
			NdisMoveMemory(pAd->ApCfg.EAPifname[i], macptr, strlen(macptr));
			DBGPRINT(RT_DEBUG_TRACE, ("NO.%d EAPifname=%s, len=%d\n", i, 
														pAd->ApCfg.EAPifname[i], 
														pAd->ApCfg.EAPifname_len[i]));
		}
	}
	return TRUE;
}

INT Set_PreAuthIfName_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT			i;
	RTMP_STRING *macptr;	

	for (i=0, macptr = rstrtok(arg,";"); (macptr && i < MAX_MBSSID_NUM(pAd)); macptr = rstrtok(NULL,";"), i++) 
	{
		if (strlen(macptr) > 0)
		{
			pAd->ApCfg.PreAuthifname_len[i] = strlen(macptr); 
			NdisMoveMemory(pAd->ApCfg.PreAuthifname[i], macptr, strlen(macptr));
			DBGPRINT(RT_DEBUG_TRACE, ("NO.%d PreAuthifname=%s, len=%d\n", i,
														pAd->ApCfg.PreAuthifname[i], 
														pAd->ApCfg.PreAuthifname_len[i]));
		}
	}
	return TRUE;

}

INT Set_RADIUS_Server_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;
	RTMP_STRING *macptr;	
	INT			count;
	UINT32		ip_addr;
	INT			srv_cnt = 0;

	for (count = 0, macptr = rstrtok(arg,";"); (macptr && count < MAX_RADIUS_SRV_NUM); macptr = rstrtok(NULL,";"), count++) 
	{
		if (rtinet_aton(macptr, &ip_addr))
		{
			PRADIUS_SRV_INFO pSrvInfo = &pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_cnt];
		
			pSrvInfo->radius_ip = ip_addr;
			srv_cnt++;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), radius_ip(seq-%d)=%s(%x)\n", 
										apidx, srv_cnt, macptr, 
										pSrvInfo->radius_ip));			
		}	    
	}

	if (srv_cnt > 0)
		pAd->ApCfg.MBSSID[apidx].radius_srv_num = srv_cnt;

	return TRUE;
}

INT Set_RADIUS_Port_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;
	RTMP_STRING *macptr;	
	INT			count;
	INT			srv_cnt = 0;

	for (count = 0, macptr = rstrtok(arg,";"); (macptr && count < MAX_RADIUS_SRV_NUM); macptr = rstrtok(NULL,";"), count++) 
	{	  
		if (srv_cnt < pAd->ApCfg.MBSSID[apidx].radius_srv_num)
		{		
			PRADIUS_SRV_INFO pSrvInfo = &pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_cnt];
			
        	pSrvInfo->radius_port = (UINT32) simple_strtol(macptr, 0, 10); 
			srv_cnt ++;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), radius_port(seq-%d)=%d\n", 
									  apidx, srv_cnt, pSrvInfo->radius_port));
		}
	}

	return TRUE;
}

INT Set_RADIUS_Key_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;
	RTMP_STRING *macptr;	
	INT			count;
	INT			srv_cnt = 0;

	for (count = 0, macptr = rstrtok(arg,";"); (macptr && count < MAX_RADIUS_SRV_NUM); macptr = rstrtok(NULL,";"), count++) 
	{	  
		if (strlen(macptr) > 0 && srv_cnt < pAd->ApCfg.MBSSID[apidx].radius_srv_num)
		{		
			PRADIUS_SRV_INFO pSrvInfo = &pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_cnt];

			pSrvInfo->radius_key_len = strlen(macptr); 
			NdisMoveMemory(pSrvInfo->radius_key, macptr, pSrvInfo->radius_key_len);	
			srv_cnt ++;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), radius_key(seq-%d)=%s, len=%d\n", 
										apidx, srv_cnt,
										pSrvInfo->radius_key, 
										pSrvInfo->radius_key_len));			
		}
	}				
	return TRUE;
}
#endif /* DOT1X_SUPPORT */

#ifdef UAPSD_SUPPORT
INT Set_UAPSD_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	if (simple_strtol(arg, 0, 10) != 0)
		pAd->ApCfg.MBSSID[0].UapsdInfo.bAPSDCapable = TRUE;
	else
		pAd->ApCfg.MBSSID[0].UapsdInfo.bAPSDCapable = FALSE;

	return TRUE;
}
#endif /* UAPSD_SUPPORT */



#ifdef MCAST_RATE_SPECIFIC
INT Set_McastPhyMode(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR PhyMode = simple_strtol(arg, 0, 10);

	pAd->CommonCfg.MCastPhyMode.field.BW = pAd->CommonCfg.RegTransmitSetting.field.BW;
	switch (PhyMode)
	{
		case MCAST_DISABLE: /* disable */
			NdisMoveMemory(&pAd->CommonCfg.MCastPhyMode, &pAd->MacTab.Content[MCAST_WCID].HTPhyMode, sizeof(HTTRANSMIT_SETTING));
			break;

		case MCAST_CCK:	/* CCK */
			pAd->CommonCfg.MCastPhyMode.field.MODE = MODE_CCK;
			pAd->CommonCfg.MCastPhyMode.field.BW =  BW_20;
			break;

		case MCAST_OFDM:	/* OFDM */
			pAd->CommonCfg.MCastPhyMode.field.MODE = MODE_OFDM;
			break;
#ifdef DOT11_N_SUPPORT
		case MCAST_HTMIX:	/* HTMIX */
			pAd->CommonCfg.MCastPhyMode.field.MODE = MODE_HTMIX;
			break;
#endif /* DOT11_N_SUPPORT */
		default:
			printk("unknow Muticast PhyMode %d.\n", PhyMode);
			printk("0:Disable 1:CCK, 2:OFDM, 3:HTMIX.\n");
			break;
	}

	return TRUE;
}

INT Set_McastMcs(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR Mcs = simple_strtol(arg, 0, 10);

	if (Mcs > 15)
		printk("Mcs must in range of 0 to 15\n");

	switch(pAd->CommonCfg.MCastPhyMode.field.MODE)
	{
		case MODE_CCK:
			if ((Mcs <= 3) || (Mcs >= 8 && Mcs <= 11))
				pAd->CommonCfg.MCastPhyMode.field.MCS = Mcs;
			else
				printk("MCS must in range of 0 ~ 3 and 8 ~ 11 for CCK Mode.\n");
			break;

		case MODE_OFDM:
			if (Mcs > 7)
				printk("MCS must in range from 0 to 7 for CCK Mode.\n");
			else
				pAd->CommonCfg.MCastPhyMode.field.MCS = Mcs;
			break;

		default:
			pAd->CommonCfg.MCastPhyMode.field.MCS = Mcs;
			break;
	}

	return TRUE;
}

INT Show_McastRate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	printk("Mcast PhyMode =%d\n", pAd->CommonCfg.MCastPhyMode.field.MODE);
	printk("Mcast Mcs =%d\n", pAd->CommonCfg.MCastPhyMode.field.MCS);
	return TRUE;
}
#endif /* MCAST_RATE_SPECIFIC */

#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
INT Set_OBSSScanParam_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	
	INT ObssScanValue;
	UINT Idx;
	RTMP_STRING *thisChar;
	
	Idx = 0;
	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		ObssScanValue = (INT) simple_strtol(thisChar, 0, 10);
		switch (Idx)
		{
			case 0:
				if (ObssScanValue < 5 || ObssScanValue > 1000)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanPassiveDwell(%d), should in range 5~1000\n", ObssScanValue));
				}
				else
				{
					pAd->CommonCfg.Dot11OBssScanPassiveDwell = ObssScanValue;	/* Unit : TU. 5~1000 */
					DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanPassiveDwell=%d\n", ObssScanValue));
				}
				break;
			case 1:
				if (ObssScanValue < 10 || ObssScanValue > 1000)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanActiveDwell(%d), should in range 10~1000\n", ObssScanValue));
				}
				else
				{
					pAd->CommonCfg.Dot11OBssScanActiveDwell = ObssScanValue;	/* Unit : TU. 10~1000 */
					DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanActiveDwell=%d\n", ObssScanValue));
				}
				break;
			case 2:
				pAd->CommonCfg.Dot11BssWidthTriggerScanInt = ObssScanValue;	/* Unit : Second */
				DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthTriggerScanInt=%d\n", ObssScanValue));
				break;
			case 3:
				if (ObssScanValue < 200 || ObssScanValue > 10000)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanPassiveTotalPerChannel(%d), should in range 200~10000\n", ObssScanValue));
				}
				else
				{
					pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel = ObssScanValue;	/* Unit : TU. 200~10000 */
					DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanPassiveTotalPerChannel=%d\n", ObssScanValue));
				}
				break;
			case 4:
				if (ObssScanValue < 20 || ObssScanValue > 10000)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanActiveTotalPerChannel(%d), should in range 20~10000\n", ObssScanValue));
				}
				else
				{
					pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel = ObssScanValue;	/* Unit : TU. 20~10000 */
					DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanActiveTotalPerChannel=%d\n", ObssScanValue));
				}
				break;
			case 5:
				pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor = ObssScanValue;
				DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthChanTranDelayFactor=%d\n", ObssScanValue));
				break;
			case 6:
				pAd->CommonCfg.Dot11OBssScanActivityThre = ObssScanValue;	/* Unit : percentage */
				DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthChanTranDelayFactor=%d\n", ObssScanValue));
				break;
		}
		Idx++;
	}

	if (Idx != 7)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Wrong OBSSScanParamtetrs format in ioctl cmd!!!!! Use default value\n"));
		
		pAd->CommonCfg.Dot11OBssScanPassiveDwell = dot11OBSSScanPassiveDwell;	/* Unit : TU. 5~1000 */
		pAd->CommonCfg.Dot11OBssScanActiveDwell = dot11OBSSScanActiveDwell;	/* Unit : TU. 10~1000 */
		pAd->CommonCfg.Dot11BssWidthTriggerScanInt = dot11BSSWidthTriggerScanInterval;	/* Unit : Second */
		pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel = dot11OBSSScanPassiveTotalPerChannel;	/* Unit : TU. 200~10000 */
		pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel = dot11OBSSScanActiveTotalPerChannel;	/* Unit : TU. 20~10000 */
		pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor = dot11BSSWidthChannelTransactionDelayFactor;
		pAd->CommonCfg.Dot11OBssScanActivityThre = dot11BSSScanActivityThreshold;	/* Unit : percentage */
	}
	pAd->CommonCfg.Dot11BssWidthChanTranDelay = (pAd->CommonCfg.Dot11BssWidthTriggerScanInt * pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor);
	DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthChanTranDelay=%ld\n", pAd->CommonCfg.Dot11BssWidthChanTranDelay));
	
	return TRUE;
}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

INT Set_EntryLifeCheck_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG LifeCheckCnt = (ULONG) simple_strtol(arg, 0, 10);

	if (LifeCheckCnt <= 65535)
		pAd->ApCfg.EntryLifeCheck = LifeCheckCnt;
	else
		printk("LifeCheckCnt must in range of 0 to 65535\n");

	printk("EntryLifeCheck Cnt = %ld.\n", pAd->ApCfg.EntryLifeCheck);
	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Authentication mode
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT ApCfg_Set_AuthMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	INT				apidx,
	IN	RTMP_STRING *arg)
{
	if ((strcmp(arg, "WEPAUTO") == 0) || (strcmp(arg, "wepauto") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if ((strcmp(arg, "OPEN") == 0) || (strcmp(arg, "open") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeOpen;
	else if ((strcmp(arg, "SHARED") == 0) || (strcmp(arg, "shared") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeShared;
	else if ((strcmp(arg, "WPAPSK") == 0) || (strcmp(arg, "wpapsk") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((strcmp(arg, "WPA2PSK") == 0) || (strcmp(arg, "wpa2psk") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWPA2PSK;
	else if ((strcmp(arg, "WPAPSKWPA2PSK") == 0) || (strcmp(arg, "wpapskwpa2psk") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
#ifdef DOT1X_SUPPORT
	else if ((strcmp(arg, "WPA") == 0) || (strcmp(arg, "wpa") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWPA;
	else if ((strcmp(arg, "WPA2") == 0) || (strcmp(arg, "wpa2") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWPA2;
	else if ((strcmp(arg, "WPA1WPA2") == 0) || (strcmp(arg, "wpa1wpa2") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWPA1WPA2;
#endif /* DOT1X_SUPPORT */
#ifdef WAPI_SUPPORT
	else if ((strcmp(arg, "WAICERT") == 0) || (strcmp(arg, "waicert") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWAICERT;
	else if ((strcmp(arg, "WAIPSK") == 0) || (strcmp(arg, "waipsk") == 0))
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeWAIPSK;
#endif /* WAPI_SUPPORT */	
	else
		pAd->ApCfg.MBSSID[apidx].wdev.AuthMode = Ndis802_11AuthModeOpen;  

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d)::AuthMode=%d\n", apidx, pAd->ApCfg.MBSSID[apidx].wdev.AuthMode));		

	return TRUE;
}

INT ApCfg_Set_MaxStaNum_Proc(
	IN PRTMP_ADAPTER 	pAd,
	IN INT				apidx,
	IN RTMP_STRING *arg)
{
	pAd->ApCfg.MBSSID[apidx].MaxStaNum = (UCHAR)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) %s::(MaxStaNum=%d)\n", 
					apidx, __FUNCTION__, pAd->ApCfg.MBSSID[apidx].MaxStaNum));
	return TRUE;
}

INT ApCfg_Set_IdleTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LONG idle_time;

	idle_time = simple_strtol(arg, 0, 10);

	if (idle_time < MAC_TABLE_MIN_AGEOUT_TIME)
		pAd->ApCfg.StaIdleTimeout = MAC_TABLE_MIN_AGEOUT_TIME;
	else
		pAd->ApCfg.StaIdleTimeout = idle_time;

	DBGPRINT(RT_DEBUG_TRACE, ("%s : IdleTimeout=%d\n", __FUNCTION__, pAd->ApCfg.StaIdleTimeout));

	return TRUE;
}


INT Set_MemDebug_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef VENDOR_FEATURE2_SUPPORT
	printk("Number of Packet Allocated = %lu\n", OS_NumOfPktAlloc);
	printk("Number of Packet Freed = %lu\n", OS_NumOfPktFree);
	printk("Offset of Packet Allocated/Freed = %lu\n", OS_NumOfPktAlloc - OS_NumOfPktFree);
#endif /* VENDOR_FEATURE2_SUPPORT */
	return TRUE;
}


#ifdef CONFIG_AP_SUPPORT
/*
========================================================================
Routine Description:
	Set power save life time.

Arguments:
	pAd					- WLAN control block pointer
	Arg					- Input arguments

Return Value:
	None

Note:
========================================================================
*/
INT Set_PowerSaveLifeTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->MacTab.MsduLifeTime = simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("Set new life time = %d\n", pAd->MacTab.MsduLifeTime));
	return TRUE;
}
#endif /* CONFIG_AP_SUPPORT */


#ifdef MBSS_SUPPORT
/*
========================================================================
Routine Description:
	Show MBSS information.

Arguments:
	pAd					- WLAN control block pointer
	Arg					- Input arguments

Return Value:
	None

Note:
========================================================================
*/
INT Show_MbssInfo_Display_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 IdBss;
	UCHAR PhyMode;


	DBGPRINT(RT_DEBUG_ERROR, ("\n\tBSS Idx\t\tPhy Mode\n"));

	for(IdBss=0; IdBss<=pAd->ApCfg.BssidNum; IdBss++)
	{
		if (IdBss == 0)
		{
			PhyMode = pAd->CommonCfg.PhyMode;
			DBGPRINT(RT_DEBUG_ERROR, ("\tMAX\t\t"));
		}
		else
		{
			PhyMode = pAd->ApCfg.MBSSID[IdBss-1].wdev.PhyMode;
			DBGPRINT(RT_DEBUG_ERROR, ("\t%d\t\t", IdBss-1));
		}

		switch(PhyMode)
		{
			case PHY_11BG_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("BG Mixed\n"));
				break;

			case PHY_11B:
				DBGPRINT(RT_DEBUG_ERROR, ("B Only\n"));
				break;

			case PHY_11A:
				DBGPRINT(RT_DEBUG_ERROR, ("A Only\n"));
				break;

			case PHY_11ABG_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("ABG Mixed ==> BG Mixed\n"));
				break;

			case PHY_11G:
				DBGPRINT(RT_DEBUG_ERROR, ("G Only\n"));
				break;

			case PHY_11ABGN_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("ABGN Mixed ==> BGN Mixed\n"));
				break;

			case PHY_11N_2_4G:
				DBGPRINT(RT_DEBUG_ERROR, ("2.4G N Only\n"));
				break;

			case PHY_11GN_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("GN Mixed\n"));
				break;

			case PHY_11AN_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("AN Mixed\n"));
				break;

			case PHY_11BGN_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("BGN Mixed\n"));
				break;

			case PHY_11AGN_MIXED:
				DBGPRINT(RT_DEBUG_ERROR, ("AGN Mixed\n"));
				break;

			case PHY_11N_5G:
				DBGPRINT(RT_DEBUG_ERROR, ("5G N Only\n"));
				break;
		}
	}

	DBGPRINT(RT_DEBUG_ERROR, ("\n"));

	return TRUE;
}
#endif /* MBSS_SUPPORT */


#ifdef HOSTAPD_SUPPORT
VOID RtmpHostapdSecuritySet(
	IN	RTMP_ADAPTER			*pAd,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrqin)
{
	if(wrqin->u.data.length > 20 && MAX_LEN_OF_RSNIE > wrqin->u.data.length && wrqin->u.data.pointer)
	{
		UCHAR RSNIE_Len[2];
		UCHAR RSNIe[2];
		int offset_next_ie=0;

		DBGPRINT(RT_DEBUG_TRACE,("ioctl SIOCSIWGENIE pAd->IoctlIF=%d\n",apidx));

		RSNIe[0]=*(UINT8 *)wrqin->u.data.pointer;
		if(IE_WPA != RSNIe[0] && IE_RSN != RSNIe[0] )
		{
			DBGPRINT(RT_DEBUG_TRACE,("IE %02x != 0x30/0xdd\n",RSNIe[0]));
			Status = -EINVAL;
			break;
		}
		RSNIE_Len[0]=*((UINT8 *)wrqin->u.data.pointer + 1);
		if(wrqin->u.data.length != RSNIE_Len[0]+2)
		{
			DBGPRINT(RT_DEBUG_TRACE,("IE use WPA1 WPA2\n"));
			NdisZeroMemory(pAd->ApCfg.MBSSID[apidx].RSN_IE[1], MAX_LEN_OF_RSNIE);
			RSNIe[1]=*(UINT8 *)wrqin->u.data.pointer;
			RSNIE_Len[1]=*((UINT8 *)wrqin->u.data.pointer + 1);
			DBGPRINT(RT_DEBUG_TRACE,( "IE1 %02x %02x\n",RSNIe[1],RSNIE_Len[1]));
			pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1] = RSNIE_Len[1];
			NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].RSN_IE[1], (UCHAR *)(wrqin->u.data.pointer)+2, RSNIE_Len[1]);
			offset_next_ie=RSNIE_Len[1]+2;
		}
		else
			DBGPRINT(RT_DEBUG_TRACE,("IE use only %02x\n",RSNIe[0]));

		NdisZeroMemory(pAd->ApCfg.MBSSID[apidx].RSN_IE[0], MAX_LEN_OF_RSNIE);
		RSNIe[0]=*(((UINT8 *)wrqin->u.data.pointer)+offset_next_ie);
		RSNIE_Len[0]=*(((UINT8 *)wrqin->u.data.pointer) + offset_next_ie + 1);
		if(IE_WPA != RSNIe[0] && IE_RSN != RSNIe[0] )
		{
			Status = -EINVAL;
			break;
		}
		pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0] = RSNIE_Len[0];
		NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].RSN_IE[0], ((UCHAR *)(wrqin->u.data.pointer))+2+offset_next_ie, RSNIE_Len[0]);
		APMakeAllBssBeacon(pAd);
		APUpdateAllBeaconFrame(pAd);
	}
}
#endif /* HOSTAPD_SUPPORT */


/*
========================================================================
Routine Description:
	Driver Ioctl for AP.

Arguments:
	pAdSrc			- WLAN control block pointer
	wrq				- the IOCTL parameters
	cmd				- the command ID
	subcmd			- the sub-command ID
	pData			- the IOCTL private data pointer
	Data			- the IOCTL private data

Return Value:
	NDIS_STATUS_SUCCESS	- IOCTL OK
	Otherwise			- IOCTL fail

Note:
========================================================================
*/
INT RTMP_AP_IoctlHandle(
	IN	VOID					*pAdSrc,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq,
	IN	INT						cmd,
	IN	USHORT					subcmd,
	IN	VOID					*pData,
	IN	ULONG					Data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT Status = NDIS_STATUS_SUCCESS;


	switch(cmd)
	{
		case CMD_RTPRIV_IOCTL_SET:
			Status = RTMPAPPrivIoctlSet(pAd, wrq);
			break;

		case CMD_RT_PRIV_IOCTL:
			if (subcmd & OID_GET_SET_TOGGLE)
				Status = RTMPAPSetInformation(pAd, wrq,  (INT)subcmd);
			else
			{
#ifdef LLTD_SUPPORT
				if (subcmd == RT_OID_GET_PHY_MODE)
				{
					if(pData != NULL)
					{
						UINT modetmp = 0;
						DBGPRINT(RT_DEBUG_TRACE, ("Query::Get phy mode (%02X) \n", pAd->CommonCfg.PhyMode));
						modetmp = (UINT) pAd->CommonCfg.PhyMode;
						wrq->u.data.length = 1;
						/**(ULONG *)pData = (ULONG)pAd->CommonCfg.PhyMode; */
						if (copy_to_user(pData, &modetmp, wrq->u.data.length))
							Status = -EFAULT;							
					}
					else
						Status = -EFAULT;
				}
				else
#endif /* LLTD_SUPPORT */
					Status = RTMPAPQueryInformation(pAd, wrq, (INT)subcmd);
			}
			break;

		case CMD_RTPRIV_IOCTL_SHOW:
			Status = RTMPAPPrivIoctlShow(pAd, wrq);
			break;

#ifdef WSC_AP_SUPPORT
		case CMD_RTPRIV_IOCTL_SET_WSCOOB:
			RTMPIoctlSetWSCOOB(pAd);
		    break;
#endif/*WSC_AP_SUPPORT*/

		case CMD_RTPRIV_IOCTL_GET_MAC_TABLE:
			RTMPIoctlGetMacTable(pAd,wrq);
		    break;

#if defined (AP_SCAN_SUPPORT) || defined (CONFIG_STA_SUPPORT)
		case CMD_RTPRIV_IOCTL_GSITESURVEY:
			RTMPIoctlGetSiteSurvey(pAd,wrq);
			break;
#endif /* AP_SCAN_SUPPORT */

		case CMD_RTPRIV_IOCTL_STATISTICS:
			RTMPIoctlStatistics(pAd, wrq);
			break;

#ifdef WSC_AP_SUPPORT
		case CMD_RTPRIV_IOCTL_WSC_PROFILE:
		    RTMPIoctlWscProfile(pAd, wrq);
		    break;
#endif /* WSC_AP_SUPPORT */

#ifdef DOT11_N_SUPPORT
		case CMD_RTPRIV_IOCTL_QUERY_BATABLE:
		    RTMPIoctlQueryBaTable(pAd, wrq);
		    break;
#endif /* DOT11_N_SUPPORT */

		case CMD_RTPRIV_IOCTL_E2P:
			RTMPAPIoctlE2PROM(pAd, wrq);
			break;

#ifdef DBG
		case CMD_RTPRIV_IOCTL_BBP:
			RTMPAPIoctlBBP(pAd, wrq);
			break;
			
		case CMD_RTPRIV_IOCTL_MAC:
			RTMPIoctlMAC(pAd, wrq);
			break;
            
#ifdef RTMP_RF_RW_SUPPORT
		case CMD_RTPRIV_IOCTL_RF:
			RTMPAPIoctlRF(pAd, wrq);
			break;
#endif /* RTMP_RF_RW_SUPPORT */
#endif /* DBG */

#ifdef INF_AR9
#ifdef AR9_MAPI_SUPPORT
		case CMD_RTPRIV_IOCTL_GET_AR9_SHOW:
			Status = RTMPAPPrivIoctlAR9Show(pAd, wrq);
			break;
#endif /*AR9_MAPI_SUPPORT*/
#endif/* INF_AR9 */

		case CMD_RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT:
#ifdef RTMP_RBUS_SUPPORT
			RTMPIoctlGetMacTableStaInfo(pAd, wrq);
#endif /* RTMP_RBUS_SUPPORT */
			break;

		case CMD_RTPRIV_IOCTL_AP_SIOCGIFHWADDR:
            if (pObj->ioctl_if < MAX_MBSSID_NUM(pAd))
    			NdisCopyMemory((RTMP_STRING *) wrq->u.name, (RTMP_STRING *) pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bssid, 6);
			break;

		case CMD_RTPRIV_IOCTL_AP_SIOCGIWESSID:
		{
			RT_CMD_AP_IOCTL_SSID *pSSID = (RT_CMD_AP_IOCTL_SSID *)pData;

#ifdef APCLI_SUPPORT
			if (pSSID->priv_flags == INT_APCLI)
			{
				if (pAd->ApCfg.ApCliTab[pObj->ioctl_if].Valid == TRUE)
				{
					pSSID->length = pAd->ApCfg.ApCliTab[pObj->ioctl_if].SsidLen;
					pSSID->pSsidStr = (char *)&pAd->ApCfg.ApCliTab[pObj->ioctl_if].Ssid;
				}
				else {
					pSSID->length = 0;
					pSSID->pSsidStr = NULL;
				}
			}
			else
#endif /* APCLI_SUPPORT */
			{
				if (P2P_CLI_ON(pAd))
				{
					pSSID->length = pAd->ApCfg.ApCliTab[0].MlmeAux.SsidLen;
					pSSID->pSsidStr = (char *)pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid;
				}
				else if (P2P_GO_ON(pAd))
				{
					pSSID->length = pAd->ApCfg.MBSSID[pSSID->apidx].SsidLen;
					pSSID->pSsidStr = (char *)pAd->ApCfg.MBSSID[pSSID->apidx].Ssid;
				}
				else
				{
					pSSID->length = 0;
					pSSID->pSsidStr = NULL;
				}
			}
		}
			break;

#ifdef MBSS_SUPPORT
		case CMD_RTPRIV_IOCTL_MBSS_BEACON_UPDATE:
			APMakeAllBssBeacon(pAd);
			APUpdateAllBeaconFrame(pAd);
			break;

		case CMD_RTPRIV_IOCTL_MBSS_OPEN:
			if (MBSS_Open(pData) != 0)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_MBSS_CLOSE:
			MBSS_Close(pData);
			break;

		case CMD_RTPRIV_IOCTL_MBSS_INIT:
			MBSS_Init(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_MBSS_REMOVE:
			MBSS_Remove(pAd);
			break;
#endif /* MBSS_SUPPORT */

		case CMD_RTPRIV_IOCTL_WSC_INIT:
		{
#ifdef APCLI_SUPPORT
#ifdef WSC_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
			PWSC_V2_INFO	pWscV2Info;
#endif /* WSC_V2_SUPPORT */
			APCLI_STRUCT *pApCliEntry = (APCLI_STRUCT *)pData;
			WscGenerateUUID(pAd, &pApCliEntry->WscControl.Wsc_Uuid_E[0],
						&pApCliEntry->WscControl.Wsc_Uuid_Str[0], 0, FALSE);
			pApCliEntry->WscControl.bWscFragment = FALSE;
			pApCliEntry->WscControl.WscFragSize = 128;
			pApCliEntry->WscControl.WscRxBufLen = 0;
			pApCliEntry->WscControl.pWscRxBuf = NULL;
			os_alloc_mem(pAd, &pApCliEntry->WscControl.pWscRxBuf, MGMT_DMA_BUFFER_SIZE);
			if (pApCliEntry->WscControl.pWscRxBuf)
				NdisZeroMemory(pApCliEntry->WscControl.pWscRxBuf, MGMT_DMA_BUFFER_SIZE);
			pApCliEntry->WscControl.WscTxBufLen = 0;
			pApCliEntry->WscControl.pWscTxBuf = NULL;
			os_alloc_mem(pAd, &pApCliEntry->WscControl.pWscTxBuf, MGMT_DMA_BUFFER_SIZE);
			if (pApCliEntry->WscControl.pWscTxBuf)
				NdisZeroMemory(pApCliEntry->WscControl.pWscTxBuf, MGMT_DMA_BUFFER_SIZE);
			initList(&pApCliEntry->WscControl.WscPeerList);
			NdisAllocateSpinLock(pAd, &pApCliEntry->WscControl.WscPeerListSemLock);
			pApCliEntry->WscControl.PinAttackCount = 0;
			pApCliEntry->WscControl.bSetupLock = FALSE;
#ifdef WSC_V2_SUPPORT
			pWscV2Info = &pApCliEntry->WscControl.WscV2Info;
			pWscV2Info->bWpsEnable = TRUE;
			pWscV2Info->ExtraTlv.TlvLen = 0;
			pWscV2Info->ExtraTlv.TlvTag = 0;
			pWscV2Info->ExtraTlv.pTlvData = NULL;
			pWscV2Info->ExtraTlv.TlvType = TLV_ASCII;
			pWscV2Info->bEnableWpsV2 = TRUE;
#endif /* WSC_V2_SUPPORT */
#endif /* APCLI_SUPPORT */
			WscInit(pAd, TRUE, Data);
#endif /* WSC_AP_SUPPORT */
		}
			break;

#ifdef APCLI_SUPPORT
		case CMD_RTPRIV_IOCTL_APC_UP:
			ApCliIfUp(pAd);
			break;

		case CMD_RTPRIV_IOCTL_APC_DISCONNECT:
			MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_DISCONNECT_REQ, 0, NULL, Data);
			RTMP_MLME_HANDLER(pAd);
			break;
#if 0 /* This CMD OID is not used in STA/P2P driver */
		case CMD_RTPRIV_IOCTL_APC_INIT:
			APCli_Init(pAd, pData);
			break;
#endif
		case CMD_RTPRIV_IOCTL_APC_OPEN:
			if (ApCli_Open(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_APC_CLOSE:
			if (ApCli_Close(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_APC_REMOVE:
			ApCli_Remove(pAd);
			break;
#endif /* APCLI_SUPPORT */

		case CMD_RTPRIV_IOCTL_MAIN_OPEN:
			pAd->ApCfg.MBSSID[MAIN_MBSSID].bcn_buf.bBcnSntReq = TRUE;
			break;

		case CMD_RTPRIV_IOCTL_PREPARE:
		{
			RT_CMD_AP_IOCTL_CONFIG *pConfig = (RT_CMD_AP_IOCTL_CONFIG *)pData;
			pConfig->Status = RTMP_AP_IoctlPrepare(pAd, pData);
			if (pConfig->Status != 0)
				return NDIS_STATUS_FAILURE;
		}
			break;

		case CMD_RTPRIV_IOCTL_AP_SIOCGIWAP:
		{
			UCHAR *pBssidDest = (UCHAR *)pData;
			PCHAR pBssidStr;

#ifdef APCLI_SUPPORT
			if (Data == INT_APCLI)
			{
				if (pAd->ApCfg.ApCliTab[pObj->ioctl_if].Valid == TRUE)
					pBssidStr = (PCHAR)&APCLI_ROOT_BSSID_GET(pAd, pAd->ApCfg.ApCliTab[pObj->ioctl_if].MacTabWCID);
				else
					pBssidStr = NULL;
			}
			else
#endif /* APCLI_SUPPORT */
			{
				pBssidStr = (PCHAR) &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bssid[0];
			}

			if (pBssidStr != NULL)
			{
				memcpy(pBssidDest, pBssidStr, ETH_ALEN);
				DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWAP(=%02x:%02x:%02x:%02x:%02x:%02x)\n",
					pBssidStr[0],pBssidStr[1],pBssidStr[2], pBssidStr[3],pBssidStr[4],pBssidStr[5]));
			}
			else
			{
				memset(pBssidDest, 0, ETH_ALEN);
			}
		}
			break;

		case CMD_RTPRIV_IOCTL_AP_SIOCGIWRATEQ:
		/* handle for SIOCGIWRATEQ */
		{
			RT_CMD_IOCTL_RATE *pRate = (RT_CMD_IOCTL_RATE *)pData;
			HTTRANSMIT_SETTING HtPhyMode;

#ifdef MESH_SUPPORT
			if (pRate->priv_flags == INT_MESH)
				HtPhyMode = pAd->MeshTab.HTPhyMode;
			else
#endif /* MESH_SUPPORT */
#ifdef APCLI_SUPPORT
			if (pRate->priv_flags == INT_APCLI)
				HtPhyMode = pAd->ApCfg.ApCliTab[pObj->ioctl_if].wdev.HTPhyMode;
			else
#endif /* APCLI_SUPPORT */
#ifdef WDS_SUPPORT
			if (pRate->priv_flags == INT_WDS)
				HtPhyMode = pAd->WdsTab.WdsEntry[pObj->ioctl_if].wdev.HTPhyMode;
			else
#endif /* WDS_SUPPORT */
			{
				HtPhyMode = pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.HTPhyMode;

#ifdef MBSS_SUPPORT
				/* reset phy mode for MBSS */
				MBSS_PHY_MODE_RESET(pObj->ioctl_if, HtPhyMode);
#endif /* MBSS_SUPPORT */
			}
			RtmpDrvMaxRateGet(pAd, HtPhyMode.field.MODE, HtPhyMode.field.ShortGI,
							HtPhyMode.field.BW, HtPhyMode.field.MCS,
							(UINT32 *)&pRate->BitRate);
		}
			break;

#ifdef HOSTAPD_SUPPORT
		case CMD_RTPRIV_IOCTL_AP_SIOCGIWRATEQ:
			RtmpHostapdSecuritySet(pAd, wrq);
			break;
#endif /* HOSTAPD_SUPPORT */

		default:
			Status = RTMP_COM_IoctlHandle(pAd, wrq, cmd, subcmd, pData, Data);
			break;
	}

	return Status;
}

#ifdef P2P_SUPPORT
INT Set_P2p_OpMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT OpMode;
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;
	
	OpMode = simple_strtol(arg, 0, 16);

	
	if (OpMode == OPMODE_AP)
	{
#ifdef DPA_S
		RtmpOSWrielessEventSend(pAd->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_SOFTAP_START, NULL, NULL, 0);
#endif /* DPA_S */
		if (P2P_CLI_ON(pAd))
			P2P_CliStop(pAd);

		if ((!P2P_GO_ON(pAd)) || (P2P_GO_ON(pAd)))
		{
			P2PCfgInit(pAd);
			P2P_GoStartUp(pAd, MAIN_MBSSID);
		}
	}
	else if (OpMode == OPMODE_APSTA)
	{
		if (P2P_GO_ON(pAd))
			P2P_GoStop(pAd);

		if ((!P2P_CLI_ON(pAd)) || (P2P_CLI_ON(pAd)))
		{
			P2PCfgInit(pAd);
			P2P_CliStartUp(pAd);
			AsicEnableBssSync(pAd, pAd->CommonCfg.BeaconPeriod);
		}
	}
	else
	{
		if (P2P_CLI_ON(pAd))
			P2P_CliStop(pAd);
		else if (P2P_GO_ON(pAd))
		{
			P2P_GoStop(pAd);
			if (INFRA_ON(pAd))
				AsicEnableBssSync(pAd, pAd->CommonCfg.BeaconPeriod);
		}
		P2PCfgInit(pAd);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2p_OpMode_Proc::(OpMode = %d)\n", pObj->ioctl_if, OpMode));

	return TRUE;
}

INT Set_P2pCli_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT Enable;
	POS_COOKIE pObj;
	UCHAR ifIndex;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	Enable = simple_strtol(arg, 0, 16);

	pAd->ApCfg.ApCliTab[ifIndex].Enable = (Enable > 0) ? TRUE : FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_Enable_Proc::(enable = %d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].Enable));
	
	ApCliIfDown(pAd);

	return TRUE;
}


INT Set_P2pCli_Ssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	INT success = FALSE;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	if(strlen(arg) <= MAX_LEN_OF_SSID)
	{
		apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

		/* bring apcli interface down first */
		if(apcliEn == TRUE )
		{
			pAd->ApCfg.ApCliTab[ifIndex].Enable = FALSE;
			ApCliIfDown(pAd);
		}

		NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, arg, strlen(arg));
		pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen = (UCHAR)strlen(arg);

		NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.Ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.Ssid, arg, strlen(arg));
		pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.SsidLength = (UCHAR)strlen(arg);

		success = TRUE;

		/* Upadte PMK and restart WPAPSK state machine for ApCli link */
		if (((pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode == Ndis802_11AuthModeWPAPSK) ||
				(pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode == Ndis802_11AuthModeWPA2PSK)) && 
					pAd->ApCfg.ApCliTab[ifIndex].PSKLen > 0)
		{
			RT_CfgSetWPAPSKKey(pAd, (RTMP_STRING *)pAd->ApCfg.ApCliTab[ifIndex].PSK, pAd->ApCfg.ApCliTab[ifIndex].PSKLen, 
									(PUCHAR)pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, 
									pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, 
									pAd->ApCfg.ApCliTab[ifIndex].PMK);
		}

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_Ssid_Proc::(Len=%d,Ssid=%s)\n", ifIndex,
			pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, pAd->ApCfg.ApCliTab[ifIndex].CfgSsid));

		pAd->ApCfg.ApCliTab[ifIndex].Enable = apcliEn;
	}
	else
		success = FALSE;

	return success;
}


INT Set_P2pCli_Bssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	RTMP_STRING *value;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

	/* bring apcli interface down first */
	if(apcliEn == TRUE )
	{
		pAd->ApCfg.ApCliTab[ifIndex].Enable = FALSE;
		ApCliIfDown(pAd);
	}

	NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, MAC_ADDR_LEN);

	if(strlen(arg) == 17)  /* Mac address acceptable format 01:02:03:04:05:06 length 17 */
	{
		for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"), i++) 
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				return FALSE;  /* Invalid */

			AtoH(value, &pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[i], 1);
		}

		if(i != 6)
			return FALSE;  /* Invalid */
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_P2pCli_Bssid_Proc (%2X:%2X:%2X:%2X:%2X:%2X)\n",
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[0],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[1],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[2],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[3],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[4],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[5]));

		NdisMoveMemory(pAd->P2pCfg.Bssid, pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, MAC_ADDR_LEN);

	pAd->ApCfg.ApCliTab[ifIndex].Enable = apcliEn;

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set ApCli-IF Authentication mode
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG       i;
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;

	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	if ((strncmp(arg, "WEPAUTO", 7) == 0) || (strncmp(arg, "wepauto", 7) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if ((strncmp(arg, "SHARED", 6) == 0) || (strncmp(arg, "shared", 6) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeShared;
	else if ((strncmp(arg, "WPAPSK", 6) == 0) || (strncmp(arg, "wpapsk", 6) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((strncmp(arg, "WPA2PSK", 7) == 0) || (strncmp(arg, "wpa2psk", 7) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeWPA2PSK;
	else
		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = Ndis802_11AuthModeOpen;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (IS_ENTRY_APCLI(&pAd->MacTab.Content[i]))
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}
		
    RTMPMakeRSNIE(pAd, pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode, pAd->ApCfg.ApCliTab[ifIndex].wdev.WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));

	pAd->ApCfg.ApCliTab[ifIndex].wdev.DefaultKeyId  = 0;

	if(pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode >= Ndis802_11AuthModeWPA)
		pAd->ApCfg.ApCliTab[ifIndex].wdev.DefaultKeyId = 1;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_AuthMode_Proc::(AuthMode=%d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode));		
	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set ApCli-IF Encryption Type
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->wdev.WepStatus = Ndis802_11WEPDisabled; 
	if ((strncmp(arg, "WEP", 3) == 0) || (strncmp(arg, "wep", 3) == 0))
    {
		if (pApCliEntry->wdev.AuthMode < Ndis802_11AuthModeWPA)
			pApCliEntry->wdev.WepStatus = Ndis802_11WEPEnabled;				  
	}
	else if ((strncmp(arg, "TKIP", 4) == 0) || (strncmp(arg, "tkip", 4) == 0))
	{
		if (pApCliEntry->wdev.AuthMode >= Ndis802_11AuthModeWPA)
			pApCliEntry->wdev.WepStatus = Ndis802_11TKIPEnable;                       
    }
	else if ((strncmp(arg, "AES", 3) == 0) || (strncmp(arg, "aes", 3) == 0))
	{
		if (pApCliEntry->wdev.AuthMode >= Ndis802_11AuthModeWPA)
			pApCliEntry->wdev.WepStatus = Ndis802_11AESEnable;                            
	}    
	else
	{
		pApCliEntry->wdev.WepStatus = Ndis802_11WEPDisabled;                 
	}

	pApCliEntry->PairCipher     = pApCliEntry->wdev.WepStatus;
	pApCliEntry->GroupCipher    = pApCliEntry->wdev.WepStatus;
	pApCliEntry->bMixCipher		= FALSE;

	if (pApCliEntry->wdev.WepStatus >= Ndis802_11TKIPEnable)
		pApCliEntry->wdev.DefaultKeyId = 1;

	RTMPMakeRSNIE(pAd, pApCliEntry->wdev.AuthMode, pApCliEntry->wdev.WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));
	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_EncrypType_Proc::(EncrypType=%d)\n", ifIndex, pApCliEntry->wdev.WepStatus));

	return TRUE;
}



/* 
    ==========================================================================
    Description:
        Set Default Key ID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG 			KeyIdx;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pApCliEntry->wdev.DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  /* Invalid argument  */
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", ifIndex, pApCliEntry->wdev.DefaultKeyId));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WPA PSK key for ApCli link

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 WPA pre-shared key string

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR ifIndex;
	POS_COOKIE pObj;
	PAPCLI_STRUCT   pApCliEntry = NULL;
	INT retval;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_WPAPSK_Proc::(WPAPSK=%s)\n", ifIndex, arg));

	retval = RT_CfgSetWPAPSKKey(pAd, arg, strlen(arg), (PUCHAR)pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen, pApCliEntry->PMK);
	if (retval == FALSE)
		return FALSE;
	
	NdisMoveMemory(pApCliEntry->PSK, arg, strlen(arg));
	pApCliEntry->PSKLen = strlen(arg);

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY1 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;
	
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[0], 0);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_Key1_Proc::(Key1=%s) success!\n", ifIndex, arg));
	
	return retVal;
	
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY2 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[1], 1);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_Key2_Proc::(Key2=%s) success!\n", ifIndex, arg));
	
	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY3 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_P2pCli_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[2], 2);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_Key3_Proc::(Key3=%s) success!\n", ifIndex, arg));
	
	return retVal;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY4 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_P2pCli_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	UCHAR			ifIndex;
	INT				retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	retVal = RT_CfgSetWepKey(pAd, arg, &pApCliEntry->SharedKey[3], 3);
	if(retVal == TRUE)
		DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_Key4_Proc::(Key4=%s) success!\n", ifIndex, arg));
	
	return retVal;
}

INT Set_P2pCli_TxMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{	
	POS_COOKIE 		pObj;	
	UCHAR 			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->wdev.DesiredTransmitSetting.field.FixedTxMode = 
								RT_CfgSetFixedTxPhyMode(arg);
	DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_TxMode_Proc = %d\n", ifIndex,
									pApCliEntry->wdev.DesiredTransmitSetting.field.FixedTxMode));

	return TRUE;
}

INT Set_P2pCli_TxMcs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->wdev.DesiredTransmitSetting.field.MCS = 
			RT_CfgSetTxMCSProc(arg, &pApCliEntry->wdev.bAutoTxRateSwitch);

	if (pApCliEntry->wdev.DesiredTransmitSetting.field.MCS == MCS_AUTO)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_TxMcs_Proc = AUTO\n", ifIndex));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_TxMcs_Proc = %d\n", ifIndex, 
									pApCliEntry->wdev.DesiredTransmitSetting.field.MCS));
	}	

	return TRUE;
}

#ifdef WSC_AP_SUPPORT
INT Set_P2pCli_WscSsid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR			ifIndex = pObj->ioctl_if;
	PWSC_CTRL	    pWscControl = &pAd->ApCfg.ApCliTab[ifIndex].WscControl;

    if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;
    
	NdisZeroMemory(&pWscControl->WscSsid, sizeof(NDIS_802_11_SSID));

	if ( (strlen(arg) > 0) && (strlen(arg) <= MAX_LEN_OF_SSID))
	{
		NdisMoveMemory(pWscControl->WscSsid.Ssid, arg, strlen(arg));
		pWscControl->WscSsid.SsidLength = strlen(arg);

		NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, arg, strlen(arg));
		pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen = (UCHAR)strlen(arg);

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_WscSsid_Proc:: (Select SsidLen=%d,Ssid=%s)\n", ifIndex, 
				pWscControl->WscSsid.SsidLength, pWscControl->WscSsid.Ssid));
	}
	else
		return FALSE;	/* Invalid argument  */

	return TRUE;	

}
#endif /* WSC_AP_SUPPORT */


INT Set_P2P_ProvAccept_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;
	PRT_P2P_CLIENT_ENTRY		pP2pEntry;
	ULONG	FrameLen;
	UCHAR	p2pindex;
	UINT bAccept;
	USHORT	PeerWscMethod;

	bAccept = simple_strtol(arg, 0, 10);
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_P2P_ProvAccept_Proc = %d\n", bAccept ));

	if ( pP2PCtrl->bProvAutoRsp == TRUE )
	{
		DBGPRINT(RT_DEBUG_TRACE, ("pP2PCtrl->bProvAutoRsp == TRUE\n" ));
		return TRUE;
	}

	p2pindex = pP2PCtrl->P2pProvIndex;
	if (p2pindex == P2P_NOT_FOUND )
	{
		DBGPRINT(RT_DEBUG_TRACE, ("P2P_NOT_FOUND\n" ));
		return TRUE;
	}
	else
		pP2pEntry = &pAd->P2pTable.Client[p2pindex];

	pP2pEntry->P2pClientState = P2PSTATE_SENT_PROVISION_RSP;

	if (bAccept)
	{
		/* Update My WPS Configuration. */
		P2P_SetWscRule(pAd, p2pindex, &PeerWscMethod);
		/* Update Sigma.ConfigMethod after finished provision procedure. */
		P2P_SET_FLAG(&pAd->P2pTable.Client[p2pindex], P2PFLAG_PROVISIONED);
		pP2pEntry->ReTransmitCnt = 0;
		pP2PCtrl->P2pProvUserNotify = 1;
		P2PSendProvisionRsp(pAd, pP2PCtrl->P2pProvConfigMethod, pP2PCtrl->P2pProvToken, pP2PCtrl->ConnectingMAC, &FrameLen);
		if (P2P_GO_ON(pAd))
			P2pSetWps(pAd, pP2pEntry);
		DBGPRINT(RT_DEBUG_ERROR, ("Accept Provision Req ==>ConfigMethod = %s \n", decodeConfigMethod(pP2PCtrl->P2pProvConfigMethod)));
		
	}
	else
	{
		pP2PCtrl->P2pProvIndex = P2P_NOT_FOUND;
		pP2pEntry->ReTransmitCnt = 0;
		pAd->P2pCfg.P2pCounter.UserAccept = 0;
		pP2PCtrl->P2pProvUserNotify = 2;
		P2PSendProvisionRsp(pAd, 0, pP2PCtrl->P2pProvToken, pP2PCtrl->ConnectingMAC, &FrameLen);
	}
	return TRUE;
}

INT Set_P2p_WscManufacturer_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex = pObj->ioctl_if;
	PRT_P2P_CONFIG	pP2PCtrl;
	
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	pP2PCtrl = &pAd->P2pCfg;

	if ( (strlen(arg) > 0) && (strlen(arg) <= WSC_MANUFACTURE_LEN))
	{
		NdisMoveMemory(pP2PCtrl->Manufacturer, arg, strlen(arg));
		pP2PCtrl->ManufacturerLen = strlen(arg);
	}
	else
		return FALSE;	/* Invalid argument  */

	return TRUE;	
}

INT Set_P2p_WscModelName_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex = pObj->ioctl_if;
	PRT_P2P_CONFIG	pP2PCtrl;
	
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	pP2PCtrl = &pAd->P2pCfg;
    
	if ( (strlen(arg) > 0) && (strlen(arg) <= WSC_MODELNAME_LEN))
	{
		NdisMoveMemory(pP2PCtrl->ModelName, arg, strlen(arg));
		pP2PCtrl->ModelNameLen = strlen(arg);
	}
	else
		return FALSE;	/* Invalid argument  */

	return TRUE;	
}

INT Set_P2p_WscModelNumber_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex = pObj->ioctl_if;
	PRT_P2P_CONFIG	pP2PCtrl;
	
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	pP2PCtrl = &pAd->P2pCfg;
    
	if ( (strlen(arg) > 0) && (strlen(arg) <= WSC_MODELNUNBER_LEN))
	{
		NdisMoveMemory(pP2PCtrl->ModelNumber, arg, strlen(arg));
		pP2PCtrl->ModelNumberLen = strlen(arg);
	}
	else
		return FALSE;	/* Invalid argument  */

	return TRUE;	
}

INT Set_P2p_WscSerialNumber_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex = pObj->ioctl_if;
	PRT_P2P_CONFIG	pP2PCtrl;
	
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	pP2PCtrl = &pAd->P2pCfg;
    
	if ( (strlen(arg) > 0) && (strlen(arg) <= WSC_SERIALNUNBER_LEN))
	{
		NdisMoveMemory(pP2PCtrl->SerialNumber, arg, strlen(arg));
		pP2PCtrl->SerialNumberLen =  strlen(arg);
	}
	else
		return FALSE;	/* Invalid argument  */

	return TRUE;	
}

#endif /* P2P_SUPPORT */
