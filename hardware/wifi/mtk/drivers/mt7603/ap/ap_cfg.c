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


#include "rt_config.h"


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

#ifdef SPECIFIC_TX_POWER_SUPPORT
INT Set_AP_PKT_PWR(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* SPECIFIC_TX_POWER_SUPPORT */

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

INT Set_AP_ASSOC_REQ_RSSI_THRESHOLD(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_AP_KickStaRssiLow_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AP_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AP_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AP_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AP_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AP_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_BasicRate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_BeaconPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_DtimPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_NoForwarding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_NoForwardingBTNSSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AP_WmmCapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_HideSSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_VLANID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_VLANPriority_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_VLAN_TAG_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AccessPolicy_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef RELEASE_EXCLUDE
INT Set_AccessControlList_Proc(RTMP_ADAPTER *pAd, PUCHAR arg);
#endif /* RELEASE_EXCLUDE  */

INT Set_ACLAddEntry_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_ACLDelEntry_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_ACLShowAll_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_ACLClearAll_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_RadioOn_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_SiteSurvey_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AutoChannelSel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef AP_SCAN_SUPPORT
INT Set_AutoChannelSelCheckTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* AP_SCAN_SUPPORT */

INT Set_BADecline_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Show_StaCount_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_StaSecurityInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_DriverInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_Sat_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_RAInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RTMP_MAC_PCI
#ifdef DBG_DIAGNOSE
INT Set_DiagOpt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_BDInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_diag_cond_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_Diag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DBG_DAIGNOSE */
#endif /* RTMP_MAC_PCI */

INT Show_Sat_Reset_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_MATTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef DOT1X_SUPPORT
VOID RTMPIoctlQueryRadiusConf(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

INT Set_IEEE8021X_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_PreAuth_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_RADIUS_Server_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_RADIUS_Port_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_RADIUS_Key_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT1X_SUPPORT */

INT Set_DisConnectSta_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_DisConnectAllSta_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef NINTENDO_AP
int Set_NintendiCapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

int Set_NintendoGet_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

int Set_NintendoSet_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
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
#ifdef APCLI_AUTO_CONNECT_SUPPORT
INT Set_ApCli_AutoConnect_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* APCLI_AUTO_CONNECT_SUPPORT */

#ifdef WPA_SUPPLICANT_SUPPORT
INT Set_ApCli_Wpa_Support(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ApCli_IEEE8021X_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif/*WPA_SUPPLICANT_SUPPORT*/

#ifdef MAC_REPEATER_SUPPORT
INT Set_ReptMode_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* MAC_REPEATER_SUPPORT */

#ifdef WSC_AP_SUPPORT
INT Set_AP_WscSsid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WSC_AP_SUPPORT */
#endif /* APCLI_SUPPORT */
#ifdef UAPSD_SUPPORT
INT Set_UAPSD_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* UAPSD_SUPPORT */

#ifdef WSC_AP_SUPPORT
INT Set_WscStatus_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef BB_SOC
INT Set_WscOOB_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif

INT Set_WscStop_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

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

INT	Set_WscMaxPinAttack_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WscSetupLockTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WSC_V2_SUPPORT */
INT	Set_WscAutoTriggerDisable_Proc(
	IN	RTMP_ADAPTER *pAd, 
	IN	RTMP_STRING	 *arg);
#endif /* WSC_AP_SUPPORT */

#ifdef EASY_CONFIG_SETUP
INT Set_AP_AutoProvisionEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_RssiThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

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
INT Set_AP2040ReScan_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11N_DRAFT3 */

INT Set_EntryLifeCheck_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RTMP_RBUS_SUPPORT
#ifdef LED_CONTROL_SUPPORT
INT Set_WlanLed_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* LED_CONTROL_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

#ifdef AP_QLOAD_SUPPORT
INT Set_QloadClr_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

/* QLOAD ALARM */
INT Set_QloadAlarmTimeThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_QloadAlarmNumThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* AP_QLOAD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef RT3883
INT Set_CwCTest_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RT3883 */

INT Set_MemDebug_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef CONFIG_AP_SUPPORT
INT Set_PowerSaveLifeTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
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
#endif /* P2P_SUPPORT */

#ifdef DYNAMIC_VGA_SUPPORT
INT	Set_DyncVgaEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT set_false_cca_hi_th(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_false_cca_low_th(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DYNAMIC_VGA_SUPPORT */

#ifdef MT_MAC
INT	Set_AP_TimEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT setApTmrEnableProc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DBG
INT    Set_AP_DumpTime_Proc(
       IN      PRTMP_ADAPTER   pAd,
       IN      RTMP_STRING     *arg);

INT    Set_BcnStateCtrl_Proc(
       IN      PRTMP_ADAPTER   pAd,
       IN      RTMP_STRING     *arg);
#endif

#endif /* MT_MAC */

static struct {
	RTMP_STRING *name;
	INT (*set_proc)(PRTMP_ADAPTER pAdapter, RTMP_STRING *arg);
} *PRTMP_PRIVATE_SET_PROC, RTMP_PRIVATE_SUPPORT_PROC[] = {
	{"RateAlg",						Set_RateAlg_Proc},
#ifdef NEW_RATE_ADAPT_SUPPORT
	{"PerThrdAdj",					Set_PerThrdAdj_Proc},
	{"LowTrafficThrd",				Set_LowTrafficThrd_Proc},
	{"TrainUpRule",					Set_TrainUpRule_Proc},
	{"TrainUpRuleRSSI",				Set_TrainUpRuleRSSI_Proc},
	{"TrainUpLowThrd",				Set_TrainUpLowThrd_Proc},
	{"TrainUpHighThrd",				Set_TrainUpHighThrd_Proc},
	{"RateTable",					Set_RateTable_Proc},
#endif /* NEW_RATE_ADAPT_SUPPORT */
	{"DriverVersion",				Set_DriverVersion_Proc},
	{"CountryRegion",				Set_CountryRegion_Proc},
	{"CountryRegionABand",			Set_CountryRegionABand_Proc},
	{"CountryString",				Set_CountryString_Proc},
	{"CountryCode",				Set_CountryCode_Proc},
#ifdef EXT_BUILD_CHANNEL_LIST
	{"ChGeography",				Set_ChGeography_Proc},
#endif /* EXT_BUILD_CHANNEL_LIST */
	{"SSID",						Set_AP_SSID_Proc},
	{"WirelessMode",				Set_WirelessMode_Proc},
	{"BasicRate",					Set_BasicRate_Proc},
	{"ShortSlot",					Set_ShortSlot_Proc},
	{"Channel",					Set_Channel_Proc},
#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392)
	{"BbpResetFlagCountVale",		Set_BbpResetFlagCountVale},
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) */
	{"BeaconPeriod",				Set_BeaconPeriod_Proc},
	{"DtimPeriod",					Set_DtimPeriod_Proc},
	{"TxPower",					Set_TxPower_Proc},
	{"BGProtection",				Set_BGProtection_Proc},
	{"DisableOLBC", 				Set_OLBCDetection_Proc},
	{"TxPreamble",				Set_TxPreamble_Proc},
	{"RTSThreshold",				Set_RTSThreshold_Proc},
	{"FragThreshold",				Set_FragThreshold_Proc},
	{"TxBurst",					Set_TxBurst_Proc},
	{"MaxStaNum",					Set_AP_MaxStaNum_Proc},
#ifdef RTMP_MAC_PCI
	{"ShowRF",					Set_ShowRF_Proc},
#endif /* RTMP_MAC_PCI */
	{"IdleTimeout",					Set_AP_IdleTimeout_Proc},
#ifdef DOT11_N_SUPPORT
	{"BASetup",					Set_BASetup_Proc},
	{"BADecline",					Set_BADecline_Proc},
	{"SendMIMOPS",				Set_SendSMPSAction_Proc},
	{"BAOriTearDown",				Set_BAOriTearDown_Proc},
	{"BARecTearDown",				Set_BARecTearDown_Proc},
	{"HtBw",						Set_HtBw_Proc},
	{"HtMcs",						Set_HtMcs_Proc},
	{"HtGi",						Set_HtGi_Proc},
	{"HtOpMode",					Set_HtOpMode_Proc},
	{"HtStbc",					Set_HtStbc_Proc},
	{"HtExtcha",					Set_HtExtcha_Proc},
	{"HtMpduDensity",				Set_HtMpduDensity_Proc},
	{"HtBaWinSize",				Set_HtBaWinSize_Proc},
	{"HtMIMOPS",					Set_HtMIMOPSmode_Proc},
	{"HtRdg",						Set_HtRdg_Proc},
	{"HtLinkAdapt",				Set_HtLinkAdapt_Proc},
	{"HtAmsdu",					Set_HtAmsdu_Proc},
	{"HtAutoBa",					Set_HtAutoBa_Proc},
	{"HtProtect",					Set_HtProtect_Proc},
	{"HtMimoPs",					Set_HtMimoPs_Proc},
	{"HtTxStream",				Set_HtTxStream_Proc},
	{"HtRxStream",				Set_HtRxStream_Proc},
	{"ForceShortGI",				Set_ForceShortGI_Proc},
	{"ForceGF",		        		Set_ForceGF_Proc},
	{"HtTxBASize",					Set_HtTxBASize_Proc},
	{"BurstMode",					Set_BurstMode_Proc},
#ifdef GREENAP_SUPPORT
	{"GreenAP",					Set_GreenAP_Proc},
#endif /* GREENAP_SUPPORT */
	{"HtDisallowTKIP",				Set_HtDisallowTKIP_Proc},
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
	{"VhtBw",					Set_VhtBw_Proc},
	{"VhtStbc",					Set_VhtStbc_Proc},
	{"VhtBwSignal",				set_VhtBwSignal_Proc},
	{"VhtDisallowNonVHT",		Set_VhtDisallowNonVHT_Proc},
#endif /* DOT11_VHT_AC */

#ifdef IAPP_SUPPORT
	{"IappPID",					Set_IappPID_Proc},
#endif /* IAPP_SUPPORT */

#ifdef AGGREGATION_SUPPORT
	{"PktAggregate",				Set_PktAggregate_Proc},
#endif /* AGGREGATION_SUPPORT */

#ifdef INF_PPA_SUPPORT
	{"INF_AMAZON_SE_PPA",			Set_INF_AMAZON_SE_PPA_Proc},
#endif /* INF_PPA_SUPPORT */

	{"WmmCapable",				Set_AP_WmmCapable_Proc},

	{"NoForwarding",				Set_NoForwarding_Proc},
	{"NoForwardingBTNBSSID",		Set_NoForwardingBTNSSID_Proc},
	{"HideSSID",					Set_HideSSID_Proc},
	{"IEEE80211H",				Set_IEEE80211H_Proc},
	{"VLANID",					Set_VLANID_Proc},
	{"VLANPriority",				Set_VLANPriority_Proc},
	{"VLANTag",					Set_VLAN_TAG_Proc},
	{"AuthMode",					Set_AP_AuthMode_Proc},
	{"EncrypType",				Set_AP_EncrypType_Proc},
	{"WpaMixPairCipher", 			Set_AP_WpaMixPairCipher_Proc},
	{"RekeyInterval",				Set_AP_RekeyInterval_Proc},
	{"RekeyMethod", 				Set_AP_RekeyMethod_Proc},
	{"DefaultKeyID",				Set_AP_DefaultKeyID_Proc},
	{"Key1",						Set_AP_Key1_Proc},
	{"Key2",						Set_AP_Key2_Proc},
	{"Key3",						Set_AP_Key3_Proc},
	{"Key4",						Set_AP_Key4_Proc},
	{"AccessPolicy",				Set_AccessPolicy_Proc},
	{"ACLAddEntry",					Set_ACLAddEntry_Proc},
	{"ACLDelEntry",					Set_ACLDelEntry_Proc},
	{"ACLShowAll",					Set_ACLShowAll_Proc},
	{"ACLClearAll",					Set_ACLClearAll_Proc},
	{"WPAPSK",					Set_AP_WPAPSK_Proc},
	{"RadioOn",					Set_RadioOn_Proc},
#ifdef SPECIFIC_TX_POWER_SUPPORT
	{"PktPwr",						Set_AP_PKT_PWR},
#endif /* SPECIFIC_TX_POWER_SUPPORT */
	{"AssocReqRssiThres",           Set_AP_ASSOC_REQ_RSSI_THRESHOLD},
	{"KickStaRssiLow",				Set_AP_KickStaRssiLow_Proc},
#ifdef AP_SCAN_SUPPORT
	{"SiteSurvey",					Set_SiteSurvey_Proc},
	{"AutoChannelSel",				Set_AutoChannelSel_Proc},
	{"ACSCheckTime",				Set_AutoChannelSelCheckTime_Proc},
#endif /* AP_SCAN_SUPPORT */
	{"ResetCounter",				Set_ResetStatCounter_Proc},
	{"DisConnectSta",				Set_DisConnectSta_Proc},
	{"DisConnectAllSta",			Set_DisConnectAllSta_Proc},
#ifdef DOT1X_SUPPORT
	{"IEEE8021X",					Set_IEEE8021X_Proc},
	{"PreAuth",						Set_PreAuth_Proc},
	{"PMKCachePeriod", 				Set_AP_PMKCachePeriod_Proc},
	{"own_ip_addr",					Set_OwnIPAddr_Proc},
	{"EAPifname",					Set_EAPIfName_Proc},
	{"PreAuthifname",				Set_PreAuthIfName_Proc},
	{"RADIUS_Server",				Set_RADIUS_Server_Proc},
	{"RADIUS_Port",					Set_RADIUS_Port_Proc},
	{"RADIUS_Key",					Set_RADIUS_Key_Proc},
#endif /* DOT1X_SUPPORT */
#ifdef DBG
	{"Debug",					Set_Debug_Proc},
	{"DebugFunc",					Set_DebugFunc_Proc},
#endif /* DBG */

#if defined(DFS_SUPPORT) || defined(CARRIER_DETECTION_SUPPORT)
	{"RadarShow",					Set_RadarShow_Proc},
#ifdef DFS_SUPPORT
	{"RadarDebug",					Set_RadarDebug_Proc},
	/*{"RadarHit",					Set_RadarHit_Proc},*/
	{"CSPeriod",					Set_CSPeriod_Proc},
	{"ResetRadarHwDetect",					Set_ResetRadarHwDetect_Proc},
	{"DfsSwDisable",				Set_DfsSwDisable_Proc},
	{"DfsEvDropAdjTime",			Set_DfsEnvtDropAdjTime_Proc},
	{"RadarStart",					Set_RadarStart_Proc},
	{"RadarStop",					Set_RadarStop_Proc},
	{"RadarT1",						Set_RadarSetTbl1_Proc},
	{"RadarT2",						Set_RadarSetTbl2_Proc},
	{"PollTime",					Set_PollTime_Proc},
	{"PrintBusyIdle",				Set_PrintBusyIdle_Proc},
	{"BusyIdleRatio",				Set_BusyIdleRatio_Proc},
	{"DfsRssiHigh",				Set_DfsRssiHigh_Proc},
	{"DfsRssiLow",					Set_DfsRssiLow_Proc},
	{"ChMovTime",					Set_ChMovingTime_Proc},
	{"BlockChReset",				Set_BlockChReset_Proc},
#endif /* DFS_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	{"CarrierDetect",				Set_CarrierDetect_Proc},
	{"CarrierCriteria",				Set_CarrierCriteria_Proc},
	{"CarrierReCheck",				Set_CarrierReCheck_Proc},
	{"CarrierGoneThreshold",			Set_CarrierGoneThreshold_Proc},
	{"CarrierDebug",				Set_CarrierDebug_Proc},
	{"Delta",						Set_CarrierDelta_Proc},
	{"DivFlag",						Set_CarrierDivFlag_Proc},
	{"CarrThrd",					Set_CarrierThrd_Proc},
	/* v2 functions */
	{"SymRund",					Set_CarrierSymRund_Proc},
	{"CarrMask",					Set_CarrierMask_Proc},
#endif /* CARRIER_DETECTION_SUPPORT */
#endif /* defined(DFS_SUPPORT) || defined(CARRIER_DETECTION_SUPPORT) */

#ifdef RT305x
	{"RfRead",							Set_RfRead_Proc},
	{"RfWrite",							Set_RfWrite_Proc},
	{"HiPower",							Set_HiPower_Proc},
#if defined(RT3352) || defined(RT5350)
#ifdef RTMP_INTERNAL_TX_ALC
	{"TSSIMaxRange",					Set_TSSIMaxRange_Proc},
#endif /* RTMP_INTERNAL_TX_ALC */
#endif /* defined(RT3352) || defined(RT5350) */
#endif /* RT305x */

#ifdef RT6352
	{"RfBankSel",					Set_RfBankSel_Proc},
#ifdef RTMP_TEMPERATURE_CALIBRATION
	{"TemperatureInit",				Set_TemperatureCAL_Proc},
#endif /* RTMP_TEMPERATURE_CALIBRATION */
#endif /* RT6352 */

#ifdef CONFIG_ATE
	{"ATE",	SetATE},
	{"ATEDA", SetATEDa},
	{"ATESA", SetATESa},
	{"ADCDump", SetADCDump},
	{"ATEBSSID", SetATEBssid},
	{"ATECHANNEL", SetATEChannel},
	{"ATEINITCHAN", SetATEInitChan},
#ifdef RTMP_TEMPERATURE_CALIBRATION
	{"ATETEMPCAL", SetATETempCal},
	{"ATESHOWTSSI",	SetATEShowTssi},
#endif /* RTMP_TEMPERATURE_CALIBRATION */
#ifdef RTMP_INTERNAL_TX_ALC
	{"ATETSSICBA", SetATETssiCalibration},
#if defined(RT3350) || defined(RT3352)
	{"ATETSSICALBRENABLE", RT335x_Set_ATE_TSSI_CALIBRATION_ENABLE_Proc},
#endif /* defined(RT3350) || defined(RT3352) */
#endif /* RTMP_INTERNAL_TX_ALC */
#ifdef IQ_CAL_SUPPORT
	{"ATEIQCOMP", SetATEIqCompensation},
#endif /* IQ_CAL_SUPPORT */
#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
	{"ATETSSICBAEX", SetATETssiCalibrationEx},
#ifdef RT5592
	{"ATETSSICALBRENABLE", RT5592_Set_ATE_TSSI_CALIBRATION_ENABLE_Proc},
#endif /* RT5592 */
#endif /* defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION) */
	{"ATETXPOW0", SetATETxPower0},
	{"ATETXPOW1", SetATETxPower1},
#ifdef DOT11N_SS3_SUPPORT
	{"ATETXPOW2", SetATETxPower2},
#endif /* DOT11N_SS3_SUPPORT */
	{"ATETXPOWEVAL", SetATETxPowerEvaluation},
	{"ATETXANT", SetATETxAntenna},
	{"ATERXANT", SetATERxAntenna},
#ifdef RT3350
	{"ATEPABIAS", SetATEPABias},
#endif /* RT3350 */
	{"ATETXFREQOFFSET", SetATETxFreqOffset},
	{"ATETXBW", SetATETxBw},
	{"ATETXLEN", SetATETxLength},
	{"ATETXCNT", SetATETxCount},
	{"ATETXMCS", SetATETxMcs},
	{"ATETXSTBC", SetATETxStbc},
	{"ATETXMODE", SetATETxMode},
	{"ATETXGI", SetATETxGi},
	{"ATERXFER", SetATERxFer},
	{"ATERRF", SetATEReadRF},
#if (!defined(RTMP_RF_RW_SUPPORT)) && (!defined(RLT_RF))
	{"ATEWRF1",	SetATEWriteRF1},
	{"ATEWRF2", SetATEWriteRF2},
	{"ATEWRF3", SetATEWriteRF3},
	{"ATEWRF4",	SetATEWriteRF4},
#endif /* (!defined(RTMP_RF_RW_SUPPORT)) && (!defined(RLT_RF)) */
	{"ATELDE2P", SetATELoadE2p},
	{"ATERE2P", SetATEReadE2p},
#ifdef LED_CONTROL_SUPPORT
#ifdef RT35xx
	{"ATELED", SetATELed},
	{"ATEHWLED", SetATEHwLed},
#endif /* RT35xx */
#endif /* LED_CONTROL_SUPPORT */
	{"ATEAUTOALC", SetATEAutoAlc},
	{"ATETEMPSENSOR", SetATETempSensor},
#ifdef VCORECAL_SUPPORT
	{"ATEVCOCAL", SetATEVcoCal},
#endif /* VCORECAL_SUPPORT */
	{"ATEIPG", SetATEIpg},
	{"ATEPAYLOAD", SetATEPayload},
	{"ATEFIXEDPAYLOAD", SetATEFixedPayload},
#ifdef HUAWEI_ATE
#ifdef CONFIG_AP_SUPPORT
	{"ATEFILTER", SetATEMacFilter},
	{"ATEFILTERENABLE", SetATEMacFilterEnable},
#endif /* CONFIG_AP_SUPPORT */
#endif /* HUAWEI_ATE */
#ifdef TXBF_SUPPORT
	{"ATETXBF", SetATETxBf},
	{"ATETXSOUNDING", SetATETxSounding},
	{"ATETXBFDIVCAL", SetATETxBfDivcal},
	{"ATETXBFLNACAL", SetATETxBfLnacal},
	{"ATETxBfInit",	SetATETxBfInit},
	{"ATETxBfCal", SetATETxBfCal},
#ifdef MT76x2
	{"ATETxBfNewCal", SetATETxBfNewCal},
	{"ATETxBfNewVerify", SetATENewPhaseVerify},
#endif
	{"ATETxBfGolden", SetATETxBfGolden},
	{"ATETxBfVerify", SetATETXBfVerify},
	{"ATETxBfVerifyN", SetATETxBfVerifyNoComp},
	{"ATEForceBBP", SetATEForceBBP},
#endif /* TXBF_SUPPORT */
	{"ATETTR", SetATETtr},
	{"ATESHOW", SetATEShow},
	{"ATEHELP", SetATEHelp},
#ifdef RT33xx
	{"ATETXEVMSHOW", SetATETxEvmCalibrationShow},
	{"ATETXEVMCAL", SetATETxEvmCalibration},
	{"ATETXEVMFILL", SetATETxEvmCalibrationFill},
#endif /* RT33xx */
#ifdef CONFIG_QA
	{"TxStop", SetTxStop},
	{"RxStop", SetRxStop},
#ifdef DBG
	{"EERead", SetEERead},
	{"EEWrite",	SetEEWrite},
	{"BBPRead", SetBBPRead},
	{"BBPWrite", SetBBPWrite},
#endif /* DBG */
#endif /* CONFIG_QA */
#endif /* CONFIG_ATE */

#ifdef APCLI_SUPPORT
	{"ApCliEnable",				Set_ApCli_Enable_Proc},
	{"ApCliSsid",					Set_ApCli_Ssid_Proc},
	{"ApCliBssid",					Set_ApCli_Bssid_Proc},
	{"ApCliAuthMode",				Set_ApCli_AuthMode_Proc},
	{"ApCliEncrypType",				Set_ApCli_EncrypType_Proc},
	{"ApCliDefaultKeyID",			Set_ApCli_DefaultKeyID_Proc},
	{"ApCliWPAPSK",				Set_ApCli_WPAPSK_Proc},
	{"ApCliKey1",					Set_ApCli_Key1_Proc},
	{"ApCliKey2",					Set_ApCli_Key2_Proc},
	{"ApCliKey3",					Set_ApCli_Key3_Proc},
	{"ApCliKey4",					Set_ApCli_Key4_Proc},
	{"ApCliTxMode",					Set_ApCli_TxMode_Proc},
	{"ApCliTxMcs",					Set_ApCli_TxMcs_Proc},
#ifdef APCLI_AUTO_CONNECT_SUPPORT
	{"ApCliAutoConnect", 			Set_ApCli_AutoConnect_Proc},
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
#ifdef WPA_SUPPLICANT_SUPPORT
	{"ApCliWpaSupport",					Set_ApCli_Wpa_Support},
	{"ApCliIEEE1X",					Set_ApCli_IEEE8021X_Proc},
#endif /* WPA_SUPPLICANT_SUPPORT */

#ifdef MAC_REPEATER_SUPPORT
	{"MACRepeaterEn",			Set_ReptMode_Enable_Proc},
#if 0
	{"ReptCliAddr",				Set_ReptCli_Addr_Proc},
#endif
#endif /* MAC_REPEATER_SUPPORT */

#ifdef WSC_AP_SUPPORT
	{"ApCliWscSsid",				Set_AP_WscSsid_Proc},
#endif /* WSC_AP_SUPPORT */
#endif /* APCLI_SUPPORT */
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
	{"WscMaxPinAttack", 			Set_WscMaxPinAttack_Proc},
	{"WscSetupLockTime", 			Set_WscSetupLockTime_Proc},
#endif /* WSC_V2_SUPPORT */
#ifdef WSC_NFC_SUPPORT
	{"NfcStatus",					Set_NfcStatus_Proc},
	{"NfcPasswdToken",				Set_NfcPasswdToken_Proc},
	{"NfcConfigToken",				Set_NfcConfigurationToken_Proc},
	{"DoWpsByNFC",					Set_DoWpsByNFC_Proc},
	{"NfcRegenPK",					Set_NfcRegenPK_Proc}, /* For NFC negative test */
#endif /* WSC_NFC_SUPPORT */
	{"WscAutoTriggerDisable", 		Set_WscAutoTriggerDisable_Proc},
#endif /* WSC_AP_SUPPORT */
#ifdef EASY_CONFIG_SETUP
	{"AutoProvisionEn",				Set_AP_AutoProvisionEnable_Proc},
	{"RssiThreshold", 				Set_RssiThreshold_Proc},
#ifdef WAC_SUPPORT
	{"WACEnable",					Set_AP_WACEnable_Proc},
#endif /* WAC_SUPPORT */
#endif /* EASY_CONFIG_SETUP */
#ifdef NINTENDO_AP
	{"NintendoCapable",			Set_NintendiCapable_Proc},
	{"NintendoLogin",				Set_NintendoSet_Proc},
/*	{"Nintendoget",				Set_NintendoGet_Proc}, */
#endif /* NINTENDO_AP */
#ifdef UAPSD_SUPPORT
	{"UAPSDCapable",				Set_UAPSD_Proc},
#endif /* UAPSD_SUPPORT */
#ifdef IGMP_SNOOP_SUPPORT
	{"IgmpSnEnable",				Set_IgmpSn_Enable_Proc},
	{"IgmpAdd",					Set_IgmpSn_AddEntry_Proc},
	{"IgmpDel",					Set_IgmpSn_DelEntry_Proc},
#endif /* IGMP_SNOOP_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
#ifdef MCAST_RATE_SPECIFIC
	{"McastPhyMode",				Set_McastPhyMode},
	{"McastMcs",					Set_McastMcs},
#endif /* MCAST_RATE_SPECIFIC */
#endif /* CONFIG_AP_SUPPORT */
	{"FixedTxMode",                 Set_FixedTxMode_Proc},
#ifdef CONFIG_APSTA_MIXED_SUPPORT
	{"OpMode",					Set_OpMode_Proc},
#endif /* CONFIG_APSTA_MIXED_SUPPORT */
#ifdef MESH_SUPPORT
	{"MeshId",						Set_MeshId_Proc},
	{"MeshHostName",				Set_MeshHostName_Proc},
	{"MeshAutoLink",				Set_MeshAutoLink_Proc},
	{"MeshForward",					Set_MeshForward_Proc},
	{"MeshAddLink",					Set_MeshAddLink_Proc},
	{"MeshDelLink",					Set_MeshDelLink_Proc},
	{"MeshMultiCastAgeOut",			Set_MeshMultiCastAgeOut_Proc},
	{"MeshAuthMode",				Set_MeshAuthMode_Proc},
	{"MeshEncrypType",				Set_MeshEncrypType_Proc},
	{"MeshDefaultkey",				Set_MeshDefaultkey_Proc},
	{"MeshWEPKEY",					Set_MeshWEPKEY_Proc},
	{"MeshWPAKEY",					Set_MeshWPAKEY_Proc},
#endif /* MESH_SUPPORT */

#ifdef TXBF_SUPPORT
#ifndef MT76x2
	{"TxBfTag",					Set_TxBfTag_Proc},
	{"ReadITxBf",				Set_ReadITxBf_Proc},
	{"WriteITxBf",				Set_WriteITxBf_Proc},
	{"StatITxBf",				Set_StatITxBf_Proc},
	{"ReadETxBf",				Set_ReadETxBf_Proc},
	{"WriteETxBf",				Set_WriteETxBf_Proc},
	{"StatETxBf",				Set_StatETxBf_Proc},
	{"ITxBfTimeout",			Set_ITxBfTimeout_Proc},
	{"ETxBfTimeout",			Set_ETxBfTimeout_Proc},
	{"InvTxBfTag",				Set_InvTxBfTag_Proc},
	{"ITxBfCal",				Set_ITxBfCal_Proc},
	{"ITxBfDivCal",				Set_ITxBfDivCal_Proc},
	{"ITxBfLnaCal",				Set_ITxBfLnaCal_Proc},
#endif
	{"InvTxBfTag",				Set_InvTxBfTag_Proc},
	{"ITxBfDivCal",				Set_ITxBfDivCal_Proc},

	{"ITxBfEn",					Set_ITxBfEn_Proc},
	{"ETxBfEnCond",				Set_ETxBfEnCond_Proc},
	{"ETxBfCodebook",			Set_ETxBfCodebook_Proc},
	{"ETxBfCoefficient",		Set_ETxBfCoefficient_Proc},
	{"ETxBfGrouping",			Set_ETxBfGrouping_Proc},
	{"ETxBfNoncompress",		Set_ETxBfNoncompress_Proc},
	{"ETxBfIncapable",			Set_ETxBfIncapable_Proc},
	{"NoSndgCntThrd",			Set_NoSndgCntThrd_Proc},
	{"NdpSndgStreams",			Set_NdpSndgStreams_Proc},
	{"TriggerSounding",			Set_Trigger_Sounding_Proc},

#ifdef MT76x2
	{"TxBfProfileTagHelp",     Set_TxBfProfileTag_Help},
    {"TxBfProfileTagValid",    Set_TxBfProfileTagValid},
	{"TxBfProfileTagTimeOut",  Set_TxBfProfileTag_TimeOut},
	{"TxBfProfileTagMatrix",   Set_TxBfProfileTag_Matrix},
    {"TxBfProfileTagSNR",      Set_TxBfProfileTag_SNR},
    {"TxBfProfileTagTxScale",  Set_TxBfProfileTag_TxScale},
    {"TxBfProfileTagMac",      Set_TxBfProfileTag_MAC},
    {"TxBfProfileTagFlg",      Set_TxBfProfileTag_Flg},
    {"TxBfProfileTagRead",     Set_TxBfProfileTagRead},
	{"TxBfProfileTagWrite",    Set_TxBfProfileTagWrite},
    {"TxBfProfileDataRead",    Set_TxBfProfileDataRead},
    {"TxBfProfileDataWrite",   Set_TxBfProfileDataWrite},
	{"TxBfProfileDataReadAll", Set_TxBfProfileDataReadAll},
    {"TxBfProfileDataWriteAll",Set_TxBfProfileDataWriteAll},
#endif

#endif /* TXBF_SUPPORT */
#ifdef VHT_TXBF_SUPPORT
	{"VhtNDPA",					Set_VhtNDPA_Sounding_Proc},
#endif /* VHT_TXBF_SUPPORT */

#if defined(RT2883) || defined(RT3883)
	{"PhyRateLimit",				Set_PhyRateLimit_Proc},
#ifdef DBG
	{"FixedRate",				Set_FixedRate_Proc},
#endif /* DBG */
#endif /* defined (RT2883) || defined (RT3883) */

#ifdef PRE_ANT_SWITCH
	{"PreAntSwitch",		        Set_PreAntSwitch_Proc},
	{"PreAntSwitchRSSI",		    Set_PreAntSwitchRSSI_Proc},
	{"PreAntSwitchTimeout",		    Set_PreAntSwitchTimeout_Proc},
#endif /* PRE_ANT_SWITCH */

#ifdef CFO_TRACK
	{"CFOTrack",				Set_CFOTrack_Proc},
#endif /* CFO_TRACK */

#ifdef STREAM_MODE_SUPPORT
	{"StreamMode",				Set_StreamMode_Proc},
	{"StreamModeMac",			Set_StreamModeMac_Proc},
	{"StreamModeMCS",			Set_StreamModeMCS_Proc},
#endif /* STREAM_MODE_SUPPORT */

#ifdef DBG_CTRL_SUPPORT
	{"DebugFlags",					Set_DebugFlags_Proc},
#ifdef INCLUDE_DEBUG_QUEUE
	{"DebugQueue",				Set_DebugQueue_Proc},
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

	{"LongRetry",	        		Set_LongRetryLimit_Proc},
	{"ShortRetry",	        		Set_ShortRetryLimit_Proc},
	{"AutoFallBack",	        	Set_AutoFallBack_Proc},
#ifdef RTMP_MAC_PCI
#ifdef DBG_DIAGNOSE
	{"DiagOpt",					Set_DiagOpt_Proc},
	{"BDInfo",					Set_BDInfo_Proc},
    {"diag_cond",               Set_diag_cond_Proc},
#endif /* DBG_DIAGNOSE */
#endif /* RTMP_MAC_PCI */

	{"MeasureReq",					Set_MeasureReq_Proc},
	{"TpcReq",						Set_TpcReq_Proc},
	{"PwrConstraint",				Set_PwrConstraint},
#ifdef DOT11K_RRM_SUPPORT
	{"BcnReq",						Set_BeaconReq_Proc},
	{"LinkReq",						Set_LinkMeasureReq_Proc},
	{"RrmEnable",					Set_Dot11kRRM_Enable_Proc},
	{"TxReq",						Set_TxStreamMeasureReq_Proc},

	/* only for voice enterprise power constraint testing. */
	{"vopwrc",						Set_VoPwrConsTest},

	/* only for selftesting and debugging. */
	{"rrm", 						Set_RRM_Selftest_Proc},
#endif /* DOT11K_RRM_SUPPORT */
#ifdef DOT11V_WNM_SUPPORT
/*	{"WnmMaxIdlePeriod",					Set_WnmMaxIdlePeriod_Proc}, */
	{"WNMTransMantREQ",					Set_WNMTransMantREQ_Proc},
	{"APWNMDMSShow",					Set_APWNMDMSShow_Proc},
#endif /* DOT11V_WNM_SUPPORT */
#ifdef DOT11N_DRAFT3
	{"OBSSScanParam",				Set_OBSSScanParam_Proc},
	{"AP2040Rescan",			Set_AP2040ReScan_Proc},
	{"HtBssCoex",						Set_HT_BssCoex_Proc},
	{"HtBssCoexApCntThr",				Set_HT_BssCoexApCntThr_Proc},
#endif /* DOT11N_DRAFT3 */
	{"EntryLifeCheck",				Set_EntryLifeCheck_Proc},
#ifdef WMM_ACM_SUPPORT
	{"acm",							ACM_Ioctl},
#endif /* WMM_ACM_SUPPORT */
#ifdef DOT11R_FT_SUPPORT
	{"ft",							FT_Ioctl},
	{"ftenable",					Set_FT_Enable},
	{"ftmdid",						Set_FT_Mdid},
	{"ftr0khid",					Set_FT_R0khid},
	{"ftric",						Set_FT_RIC},
	{"ftotd",						Set_FT_OTD},
#endif /* DOT11R_FT_SUPPORT */

#ifdef RTMP_EFUSE_SUPPORT
	{"efuseLoadFromBin",			set_eFuseLoadFromBin_Proc}, /* For backward compatible, the usage is the same as bufferLoadFromBin + bufferWriteBack */
	{"efuseFreeNumber",			set_eFuseGetFreeBlockCount_Proc},
	{"efuseDump",				set_eFusedump_Proc},
#ifdef CONFIG_ATE
	{"bufferLoadFromEfuse",		Set_LoadEepromBufferFromEfuse_Proc},
#ifdef BB_SOC
	{"efuseBufferModeWriteBack",	set_BinModeWriteBack_Proc},
#else
	{"efuseBufferModeWriteBack",	set_eFuseBufferModeWriteBack_Proc}, /* For backward compatible, the usage is the same as bufferWriteBack */
#endif
#endif /* CONFIG_ATE */
#endif /* RTMP_EFUSE_SUPPORT */
	{"bufferLoadFromBin", 			Set_LoadEepromBufferFromBin_Proc},
	{"bufferWriteBack", 			Set_EepromBufferWriteBack_Proc},

#ifdef RT30xx
	{"ant",					Set_Antenna_Proc},
#endif /* RT30xx */

#ifdef RT5350
	{"HwAntDiv",				Set_Hw_Antenna_Div_Proc},
#endif /* RT5350 */

#ifdef RTMP_RBUS_SUPPORT
#ifdef LED_CONTROL_SUPPORT
	{"WlanLed",					Set_WlanLed_Proc},
#endif /* LED_CONTROL_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

#ifdef AP_QLOAD_SUPPORT
	{"qloadclr",					Set_QloadClr_Proc},
	{"qloadalarmtimethres",			Set_QloadAlarmTimeThreshold_Proc}, /* QLOAD ALARM */
	{"qloadalarmnumthres",			Set_QloadAlarmNumThreshold_Proc}, /* QLOAD ALARM */
#endif /* AP_QLOAD_SUPPORT */

	{"ra_interval",					Set_RateAdaptInterval},

#ifdef THERMAL_PROTECT_SUPPORT
	{"tpc",						set_thermal_protection_criteria_proc},
#endif /* THERMAL_PROTECT_SUPPORT */

#ifdef SMART_ANTENNA
	{"sa", 						Set_SmartAnt_Proc},
	{"sa_msc", 					Set_McsStableCnt_Proc},
	{"sa_mcs", 					Set_SA_McsBound_Proc},
	{"sa_sta", 					Set_SA_Station_Proc},
	{"sa_starule", 				Set_SA_StationCandRule_Proc},
	{"sa_mode", 					Set_SA_Mode_Proc},
	{"sa_txNss", 					set_SA_txNss_Proc},
	{"sa_ant", 					Set_SA_StaticAntPair_Proc},
	{"sa_agsp", 					Set_SA_AGSP_Proc},
	{"sa_tseq", 					Set_SA_TrainSeq_Proc},
	{"sa_tdelay", 					Set_SA_TrainDelay_Proc},
	{"sa_tcond", 					Set_SA_TrainCond_Proc},
	{"sa_rssivar", 					Set_SA_RssiVariance_Proc},
	{"sa_rssith", 					Set_SA_RssiThreshold_Proc},
	{"sa_skipconf", 				Set_SA_SkipConfirmStage_Proc},
	{"sa_tcand", 					Set_SA_AntCand_Proc},
	{"sa_tp", 					Set_TestPeriod_Proc},
	{"sa_tc", 					Set_MaxAntennaTry_Proc},
	{"sa_dbg", 					Set_DbgLogOn_Proc},
#endif // SMART_ANTENNA //

	{"memdebug",					Set_MemDebug_Proc},

#ifdef CONFIG_AP_SUPPORT
	{"pslifetime",					Set_PowerSaveLifeTime_Proc},

#ifdef MBSS_SUPPORT
	{"MBSSWirelessMode",			Set_MBSS_WirelessMode_Proc},
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

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
	{"p2pDevName",					Set_P2P_Device_Name},
	{"p2pWscMode",					Set_P2P_WSC_Mode},
	{"p2pWscConf",					Set_P2P_WSC_ConfMethod},
	{"p2pNoACnt",					Set_P2P_NoA_Count},
	{"p2pNoADuration",				Set_P2P_NoA_Duration},
	{"p2pNoAInv",					Set_P2P_NoA_Interval},
	{"p2pExtLst",					Set_P2P_Extend_Listen},
	{"p2pExtLstPrd",				Set_P2P_Extend_Listen_Periodic},
	{"p2pExtLstInv",				Set_P2P_Extend_Listen_Interval},
	{"p2pIntraBss", 				Set_P2P_Intra_Bss},
	{"p2pScan", 					Set_P2P_Scan},
	{"p2pProv", 					Set_P2P_Provision_Proc},
	{"p2pInv",						Set_P2P_Invite_Proc},
	{"p2pDevDisc",					Set_P2P_Device_Discoverability_Proc},
	{"p2pLink", 					Set_P2P_Connect_GoIndex_Proc},
	{"p2pCfg",						Set_P2P_Print_Cfg},
	{"p2pTab",						Set_P2P_Print_GroupTable_Proc},
	{"p2pPerTab",						Set_P2P_Print_PersistentTable_Proc},
	{"p2pStat", 					Set_P2P_State_Proc},
	{"p2pReset", 					Set_P2P_Reset_Proc},
	{"p2pDefConfMthd", 					Set_P2P_Default_Config_Method_Proc},
#endif /* P2P_SUPPORT */

	{"VcoPeriod",					Set_VcoPeriod_Proc},
#ifdef CONFIG_SNIFFER_SUPPORT
	{"MonitorMode",					Set_MonitorMode_Proc},
#endif /* CONFIG_SNIFFER_SUPPORT */
#ifdef SINGLE_SKU
	{"ModuleTxpower",				Set_ModuleTxpower_Proc},
#endif /* SINGLE_SKU */

#ifdef RT6352
	{"TestDPDCalibration",		Set_TestDPDCalibration_Proc},
	{"TestDPDCalibrationTX0",	Set_TestDPDCalibrationTX0_Proc},
	{"TestDPDCalibrationTX1",	Set_TestDPDCalibrationTX1_Proc},
#endif /* RT6352 */

#ifdef DOT11W_PMF_SUPPORT
    {"PMFMFPC",                                         Set_PMFMFPC_Proc},
    {"PMFMFPR",                                         Set_PMFMFPR_Proc},
    {"PMFSHA256",                                       Set_PMFSHA256_Proc},
#endif /* DOT11W_PMF_SUPPORT */

#ifdef MICROWAVE_OVEN_SUPPORT
	{"MO_FalseCCATh",				Set_MO_FalseCCATh_Proc},
#endif /* MICROWAVE_OVEN_SUPPORT */

#ifdef CONFIG_FPGA_MODE
	{"fpga_on",					set_fpga_mode},		/* 1 = manual mode, 2 = fix phy/mode/rate/mcs */
	{"dataphy",					set_data_phy_mode},	/* 0 = CCK, 1 = OFDM, 2 = MODE_HTMIX, 3 = HT-GF, 4 = VHT */
	{"databw",					set_data_bw},			/* 0 = 20M, 1 = 40M, 2 = 80M */
	{"datamcs",					set_data_mcs},			/* 0~ 15 */
	{"databasize",				set_data_basize},
	{"datagi",					set_data_gi},
	{"dataldpc",				set_data_ldpc},
	{"txcnt",					set_tx_kickcnt},
	{"stop_tr",					set_tr_stop},
	{"vco_cal",					set_vco_cal},

#ifdef MT_MAC
	{"txs_type",					set_txs_report_type},
	{"no_bcn",					set_no_bcn},
#endif /* MT_MAC */

#ifdef CAPTURE_MODE
	{"cap_start",					set_cap_start},
	{"cap_trigger",				set_cap_trigger},
	{"cap_dump",					set_cap_dump},
#endif /* CAPTURE_MODE */
#endif /* CONFIG_FPGA_MODE */

#if defined(WFA_VHT_PF) || defined(MT7603_FPGA) || defined(MT7628_FPGA)
	{"force_amsdu",				set_force_amsdu},
#endif /* defined(WFA_VHT_PF) || defined(MT7603_FPGA) */

#ifdef WFA_VHT_PF
	{"nss_mcs_opt",				set_vht_nss_mcs_opt},	// SIGMA
	{"opt_md_notif_ie",			set_vht_opmode_notify_ie}, // SIGMA
	{"force_op",					set_force_operating_mode},
	{"force_noack",				set_force_noack},
	{"force_vht_sgi",				set_force_vht_sgi},
	{"force_vht_stbc",				set_force_vht_tx_stbc},
	{"ext_cca",					set_force_ext_cca},
#endif /* WFA_VHT_PF */
	{"rf",						SetRF},
#ifdef MT76x0
	{"AntennaSelect",		Set_AntennaSelect_Proc},
#endif /* MT76x0 */
	{"tssi_enable", set_tssi_enable},
#ifdef RLT_MAC
#ifdef CONFIG_WIFI_TEST
	{"pbf_loopback", set_pbf_loopback},
	{"pbf_rx_drop", set_pbf_rx_drop},
#endif
#endif
#ifdef DYNAMIC_VGA_SUPPORT
	{"DyncVgaEnable", Set_DyncVgaEnable_Proc},
	{"fc_hth", set_false_cca_hi_th},
	{"fc_lth", set_false_cca_low_th},
#endif /* DYNAMIC_VGA_SUPPORT */
#ifdef MT_MAC
	{"TimEnable",				Set_AP_TimEnable_Proc},//only for development purpose!! iwpriv ra0 set
    {"TmrEnable",				setApTmrEnableProc},//0: disalbe, 1: initialiter, 2: responser.
    {"get_fid",     set_get_fid},
	{"fwlog", set_fw_log},
    {"manual_txop", SetManualTxOP},
    {"manual_txop_thld", SetManualTxOPThreshold},
    {"manual_txop_upbound", SetManualTxOPUpBound},
    {"manual_txop_lowbound", SetManualTxOPLowBound},
#ifdef DBG
    {"DumpTime",                            Set_AP_DumpTime_Proc},//only for development purpose!! iwpriv ra0 set
    {"BcnStateCtrl",                           Set_BcnStateCtrl_Proc},//only for development purpose!! iwpriv ra0 set
#endif
#ifdef LED_CONTROL_SUPPORT
	{"MT7603LED",		Set_MT7603LED_Proc},
	{"MT7603LEDEnhance",		Set_MT7603LED_Enhance_Proc},
        {"MT7603LEDBehavior",           Set_MT7603LED_Behavor_Proc},
#endif /* LED_CONTROL_SUPPORT */	
#ifdef RTMP_MAC_PCI
	{"pdma_dbg", Set_PDMAWatchDog_Proc},
	{"pse_dbg", SetPSEWatchDog_Proc},
#endif
	{"get_thermal_sensor", Set_themal_sensor},
	{"rx_pspoll_filter", Set_rx_pspoll_filter_proc},	
#endif
#ifdef RT_CFG80211_SUPPORT
	{"DisableCfg2040Scan",				Set_DisableCfg2040Scan_Proc},
#endif	
#ifdef SINGLE_SKU_V2
	{"SKUEnable", SetSKUEnable_Proc},
#endif /* SINGLE_SKU_V2 */
	{"ed_chk", Set_ed_chk_proc},
#ifdef SMART_CARRIER_SENSE_SUPPORT
	{"SCSEnable", SetSCSEnable_Proc},
	{"SCSCfg", SetSCSCfg_Proc},
#endif /* SMART_CARRIER_SENSE_SUPPORT */
	{NULL,}
};


static struct {
	RTMP_STRING *name;
	INT (*set_proc)(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
} *PRTMP_PRIVATE_SHOW_PROC, RTMP_PRIVATE_SHOW_SUPPORT_PROC[] = {
	{"stainfo",			Show_MacTable_Proc},
#ifdef MT_MAC
	{"psinfo",			Show_PSTable_Proc},
	{"wtbl",				show_wtbl_proc},
	{"mibinfo", show_mib_proc},
	{"tmacinfo", ShowTmacInfo},
	{"agginfo", ShowAggInfo},
	{"manual_txop", ShowManualTxOP},
	{"pseinfo", ShowPseInfo},
	{"psedata", ShowPseData},
#if defined(RTMP_PCI_SUPPORT) || defined(RTMP_USB_SUPPORT)
	{"dschinfo", ShowDMASchInfo},
#endif
#endif /* MT_MAC */
	{"sta_tr",				Show_sta_tr_proc},
	{"peerinfo", 			show_stainfo_proc},
	{"stacountinfo",			Show_StaCount_Proc},
	{"stasecinfo", 			Show_StaSecurityInfo_Proc},
	{"descinfo",			Show_DescInfo_Proc},
	{"driverinfo", 			Show_DriverInfo_Proc},
	{"devinfo",			show_devinfo_proc},
	{"sysinfo",			show_sysinfo_proc},
	{"trinfo",	show_trinfo_proc},
	{"pwrinfo", show_pwr_info},
	{"txqinfo",			show_txqinfo_proc},
#ifdef WDS_SUPPORT
	{"wdsinfo",				Show_WdsTable_Proc},
#endif /* WDS_SUPPORT */
#ifdef DOT11_N_SUPPORT
	{"bainfo",				Show_BaTable_Proc},
#endif /* DOT11_N_SUPPORT */
	{"stat",				Show_Sat_Proc},
#ifdef RTMP_MAC_PCI
#ifdef DBG_DIAGNOSE
	{"diag",				Show_Diag_Proc},
#endif /* DBG_DIAGNOSE */
#endif /* RTMP_MAC_PCI */
	{"stat_reset",			Show_Sat_Reset_Proc},
#ifdef IGMP_SNOOP_SUPPORT
	{"igmpinfo",			Set_IgmpSn_TabDisplay_Proc},
#endif /* IGMP_SNOOP_SUPPORT */
#ifdef MCAST_RATE_SPECIFIC
	{"mcastrate",			Show_McastRate},
#endif /* MCAST_RATE_SPECIFIC */
#ifdef MAT_SUPPORT
	{"matinfo",			Show_MATTable_Proc},
#endif /* MAT_SUPPORT */
#ifdef RELEASE_EXCLUDE
#ifdef RTMP_MAC_USB
	{"rxbulk",	 			Show_RxBulk_Proc},
	{"txbulk",				Show_TxBulk_Proc},
#endif /* RTMP_MAC_USB */
#endif /* RELEASE_EXCLUDE */
#ifdef MESH_SUPPORT
	{"meshinfo",			Set_MeshInfo_Display_Proc},
	{"neighinfo",			Set_NeighborInfo_Display_Proc},
	{"meshrouteinfo",		Set_MeshRouteInfo_Display_Proc},
	{"meshproxyinfo",		Set_MeshProxyInfo_Display_Proc},
	{"meshentryinfo",		Set_MeshEntryInfo_Display_Proc},
	{"multipathinfo",		Set_MultipathInfo_Display_Proc},
	{"multicastageoutinfo",	Set_MultiCastAgeOut_Display_Proc},
	{"pktsiginfo",			Set_PktSig_Display_Proc},
#endif /* MESH_SUPPORT */
#ifdef DFS_SUPPORT
	{"blockch", 			Show_BlockCh_Proc},
#endif /* DFS_SUPPORT */
#ifdef DOT11R_FT_SUPPORT
	{"ftinfo",				Show_FTConfig_Proc},
#endif /* DOT11R_FT_SUPPORT */
#ifdef DOT11K_RRM_SUPPORT
	{"rrminfo",				RRM_InfoDisplay_Proc},
#endif /* DOT11K_RRM_SUPPORT */
#ifdef AP_QLOAD_SUPPORT
	{"qload",				Show_QoSLoad_Proc},
#endif /* AP_QLOAD_SUPPORT */
#ifdef APCLI_SUPPORT
	{"connStatus",			RTMPIoctlConnStatus},
#endif /* APCLI_SUPPORT */
#ifdef MAC_REPEATER_SUPPORT
	{"reptinfo",			Show_Repeater_Cli_Proc},
#endif /* MAC_REPEATER_SUPPORT */
#ifdef SMART_ANTENNA
	{"sainfo",				Show_SA_CfgInfo_Proc},
	{"sadbginfo",			Show_SA_DbgInfo_Proc},
#endif // SMART_ANTENNA //

	{"rainfo",				Show_RAInfo_Proc},

#ifdef MBSS_SUPPORT
	{"mbss",			Show_MbssInfo_Display_Proc},
#endif /* MBSS_SUPPORT */
#ifdef WSC_AP_SUPPORT
	{"WscPeerList", 		WscApShowPeerList},
#ifdef WSC_NFC_SUPPORT
	{"NfcStatus", 			Get_NfcStatus_Proc},
#endif /* WSC_NFC_SUPPORT */
#endif /* WSC_AP_SUPPORT */
	{"rfinfo", ShowRFInfo},
	{"bbpinfo", ShowBBPInfo},
#ifdef SMART_CARRIER_SENSE_SUPPORT
	{"SCSInfo", ShowSCSInfo},
#endif /* SMART_CARRIER_SENSE_SUPPORT */
	{NULL,}
};


INT RTMPAPPrivIoctlSet(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *pIoctlCmdStr)
{
	RTMP_STRING *this_char, *value;
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
#ifdef BB_SOC
		 (strcmp(this_char, "WscResetPinCode") != 0) &&
#endif
                 (strcmp(this_char, "WscGenPinCode")!= 0)
               )
#endif /* WSC_AP_SUPPORT */
#ifdef SMART_ANTENNA
			&& (strcmp(this_char, "sa") != 0)
#endif /* SMART_ANTENNA */
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
	RTMP_STRING *this_char, *value = NULL;
	INT Status = NDIS_STATUS_SUCCESS;

	while ((this_char = strsep((char **)&pIoctlCmdStr->u.data.pointer, ",")) != NULL)
	{
		if (!*this_char)
			continue;

		value = strchr(this_char, '=');
		if (value) {
			if (strlen(value) > 1) {
				*value=0;
				value++;
			} else
				value = NULL;
		}

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
				for (PRTMP_PRIVATE_SHOW_PROC = RTMP_PRIVATE_SHOW_SUPPORT_PROC; PRTMP_PRIVATE_SHOW_PROC->name; PRTMP_PRIVATE_SHOW_PROC++)
					DBGPRINT(RT_DEBUG_TRACE, ("%s\n", PRTMP_PRIVATE_SHOW_PROC->name));
			}
#endif /* RTMP_RBUS_SUPPORT */
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::(iwpriv) Command not Support [%s=%s]\n", this_char, value));
			break;
		}
	}

	return Status;

}

#if defined(INF_AR9) || defined(BB_SOC)
#if defined(AR9_MAPI_SUPPORT) || defined(BB_SOC)
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
	IN PRTMP_ADAPTER pAd,
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

 	NDIS_802_11_WEP_STATUS              WepStatus;
 	NDIS_802_11_AUTHENTICATION_MODE     AuthMode = Ndis802_11AuthModeMax;
	NDIS_802_11_SSID                    Ssid;

#ifdef HOSTAPD_SUPPORT
	MAC_TABLE_ENTRY						*pEntry;
	STA_TR_ENTRY *tr_entry;
	struct ieee80211req_mlme			mlme;

	struct ieee80211req_key				Key;
	struct ieee80211req_del_key			delkey;
	UINT8 Wcid;
	BSS_STRUCT *pMbss ;
	WSC_LV_INFO            WscIEBeacon;
   	WSC_LV_INFO            WscIEProbeResp;
	int i;
#endif /*HOSTAPD_SUPPORT*/


#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	INT ifIndex;
	BOOLEAN apcliEn=FALSE;
	PNDIS_APCLI_802_11_PMKID                  pPmkId = NULL;
    BOOLEAN IEEE8021xState = FALSE;
    BOOLEAN IEEE8021x_required_keys = FALSE;
    UCHAR wpa_supplicant_enable = 0;
	PNDIS_802_11_REMOVE_KEY pRemoveKey = NULL;
	INT BssIdx, i;
	PNDIS_802_11_WEP pWepKey =NULL;
	PAPCLI_STRUCT pApCliEntry=NULL;
	MAC_TABLE_ENTRY *pMacEntry=(MAC_TABLE_ENTRY *)NULL;
	PNDIS_APCLI_802_11_KEY pApCliKey = NULL;
	MLME_DISASSOC_REQ_STRUCT DisassocReq;
	MLME_DEAUTH_REQ_STRUCT	DeAuthFrame;
	PULONG pCurrState;
	STA_TR_ENTRY *tr_entry = NULL;
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/


	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;

	switch(cmd & 0x7FFF)
	{
#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	        case OID_802_11_SET_IEEE8021X:
			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;
			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

	            if (wrq->u.data.length != sizeof(BOOLEAN))
	                Status  = -EINVAL;
	            else
	            {

				if(apcliEn == TRUE )
				{
					Status = copy_from_user(&IEEE8021xState, wrq->u.data.pointer, wrq->u.data.length);
			        pAd->ApCfg.ApCliTab[ifIndex].wdev.IEEE8021X = IEEE8021xState;
	                DBGPRINT(RT_DEBUG_TRACE, ("Set Apcli(%d)::OID_802_11_SET_IEEE8021X (=%d)\n",ifIndex, IEEE8021xState));
				}
				else
					Status  = -EINVAL;
			}
	            break;

	        case OID_802_11_SET_IEEE8021X_REQUIRE_KEY:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (wrq->u.data.length != sizeof(BOOLEAN))
					Status  = -EINVAL;
	            else
	            {
	            		if(apcliEn == TRUE )
				{
	                		Status = copy_from_user(&IEEE8021x_required_keys, wrq->u.data.pointer, wrq->u.data.length);
					pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.IEEE8021x_required_keys = IEEE8021x_required_keys;
					DBGPRINT(RT_DEBUG_TRACE, ("Set Apcli(%d)::OID_802_11_SET_IEEE8021X_REQUIRE_KEY (%d)\n",ifIndex, IEEE8021x_required_keys));
	            		}
				else
					Status  = -EINVAL;
			}
				break;

        case OID_802_11_PMKID:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			os_alloc_mem(NULL, (UCHAR **)&pPmkId, wrq->u.data.length);

	        if(pPmkId == NULL) {
                Status = -ENOMEM;
                break;
            }
            Status = copy_from_user(pPmkId, wrq->u.data.pointer, wrq->u.data.length);

	        /* check the PMKID information */
	        if (pPmkId->BSSIDInfoCount == 0)
                NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].SavedPMK, sizeof(BSSID_INFO)*PMKID_NO);
	        else
	        {
		        PBSSID_INFO	pBssIdInfo;
		        UINT		BssIdx;
		        UINT		CachedIdx;
			UINT tail;

			tail = Offsetof(NDIS_APCLI_802_11_PMKID, BSSIDInfo);
		        for (BssIdx = 0; BssIdx < pPmkId->BSSIDInfoCount; BssIdx++)
		        {
				tail += sizeof(BSSID_INFO);
				if (tail > wrq->u.data.length) {
					DBGPRINT(RT_DEBUG_WARN,
						 ("buf not enough, stop at BssIdx %u\n",
						  BssIdx));
					break;
				}
			        /* point to the indexed BSSID_INFO structure */
			        pBssIdInfo = (PBSSID_INFO) ((PUCHAR) pPmkId + 2 * sizeof(UINT) + BssIdx * sizeof(BSSID_INFO));
			        /* Find the entry in the saved data base. */
			        for (CachedIdx = 0; CachedIdx < pAd->ApCfg.ApCliTab[ifIndex].SavedPMKNum; CachedIdx++)
			        {
				        /* compare the BSSID */
				        if (NdisEqualMemory(pBssIdInfo->BSSID, pAd->ApCfg.ApCliTab[ifIndex].SavedPMK[CachedIdx].BSSID, sizeof(NDIS_802_11_MAC_ADDRESS)))
					        break;
			        }

			        /* Found, replace it */
			        if (CachedIdx < PMKID_NO)
			        {
				        DBGPRINT(RT_DEBUG_OFF, ("Update OID_802_11_PMKID, idx = %d\n", CachedIdx));
				        NdisMoveMemory(&pAd->ApCfg.ApCliTab[ifIndex].SavedPMK[CachedIdx], pBssIdInfo, sizeof(BSSID_INFO));
				        pAd->ApCfg.ApCliTab[ifIndex].SavedPMKNum++;
			        }
			        /* Not found, replace the last one */
			        else
			        {
				        /* Randomly replace one */
				        CachedIdx = (pBssIdInfo->BSSID[5] % PMKID_NO);
				        DBGPRINT(RT_DEBUG_OFF, ("Update OID_802_11_PMKID, idx = %d\n", CachedIdx));
				        NdisMoveMemory(&pAd->ApCfg.ApCliTab[ifIndex].SavedPMK[CachedIdx], pBssIdInfo, sizeof(BSSID_INFO));
			        }
		        }
			}
			if(pPmkId)
				os_free_mem(NULL, pPmkId);
	        break;

	        case RT_OID_WPA_SUPPLICANT_SUPPORT:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			if (wrq->u.data.length != sizeof(UCHAR))
	                Status  = -EINVAL;
	            else
	            {
					Status = copy_from_user(&wpa_supplicant_enable, wrq->u.data.pointer, wrq->u.data.length);
					if (wpa_supplicant_enable & WPA_SUPPLICANT_ENABLE_WPS)
						pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.WpaSupplicantUP |= WPA_SUPPLICANT_ENABLE_WPS;
					else
					{
						pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.WpaSupplicantUP = wpa_supplicant_enable;
						pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.WpaSupplicantUP &= 0x7F;
					}
					DBGPRINT(RT_DEBUG_TRACE, ("APCLI Set::RT_OID_WPA_SUPPLICANT_SUPPORT (=0x%02X)\n",
								pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.WpaSupplicantUP));
				}
	            break;

	        case OID_802_11_REMOVE_KEY:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			os_alloc_mem(NULL, (UCHAR **)&pRemoveKey, wrq->u.data.length);
	            if(pRemoveKey == NULL)
	            {
	                Status = -ENOMEM;
	                break;
	            }

	            Status = copy_from_user(pRemoveKey, wrq->u.data.pointer, wrq->u.data.length);
	            if (pRemoveKey->Length != wrq->u.data.length)
	            {
	                Status  = -EINVAL;
	                DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_REMOVE_KEY, Failed!!\n"));
	            }
	            else
	            {
			if (pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode >= Ndis802_11AuthModeWPA)
	                {
	                    RTMPWPARemoveKeyProc(pAd, pRemoveKey);
	                    DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_REMOVE_KEY, Remove WPA Key!!\n"));
	                }
	                else
	                {
	                	UINT KeyIdx;
	                	BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;
	                    KeyIdx = pRemoveKey->KeyIndex;

	                    if (KeyIdx & 0x80000000)
	                    {
	                        /* Should never set default bit when remove key */
	                        Status  = -EINVAL;
	                        DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_REMOVE_KEY, Failed!!(Should never set default bit when remove key)\n"));
	                    }
	                    else
	                    {
	                        KeyIdx = KeyIdx & 0x0fffffff;
	                        if (KeyIdx > 3)
	                        {
	                            Status  = -EINVAL;
	                            DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_REMOVE_KEY, Failed!!(KeyId[%d] out of range)\n", KeyIdx));
	                        }
	                        else
	                        {
	                            pAd->ApCfg.ApCliTab[ifIndex].SharedKey[KeyIdx].KeyLen = 0;
	                            pAd->ApCfg.ApCliTab[ifIndex].SharedKey[KeyIdx].CipherAlg = CIPHER_NONE;
								AsicRemoveSharedKeyEntry(pAd
								, (UCHAR)BssIdx
								, (UCHAR)KeyIdx);
	                            DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_REMOVE_KEY (id=0x%x, Len=%d-byte)\n", pRemoveKey->KeyIndex, pRemoveKey->Length));
	                        }
	                    }
	                }
	            }
				if (pRemoveKey)
					os_free_mem(NULL, pRemoveKey);
	            break;

		case OID_802_11_ADD_WEP:
			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			os_alloc_mem(NULL, (UCHAR **)&pWepKey, wrq->u.data.length);
			if(pWepKey == NULL)
			{
				Status = -ENOMEM;
				DBGPRINT(RT_DEBUG_TRACE, ("Set Apcli::OID_802_11_ADD_WEP, Failed!!\n"));
				break;
			}
			BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;
			pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
			pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];
			tr_entry = &pAd->MacTab.tr_entry[pApCliEntry->MacTabWCID];
			Status = copy_from_user(pWepKey, wrq->u.data.pointer, wrq->u.data.length);
			if (Status)
			{
				Status  = -EINVAL;
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_ADD_WEP, Failed (length mismatch)!!\n"));
			} else if ((pWepKey->KeyLength != 5) ||
				   (pWepKey->KeyLength != 13)) {
				Status = -EINVAL;
				DBGPRINT(RT_DEBUG_WARN,
					 ("invalid pWepKey->KeyLength %u\n",
					  pWepKey->KeyLength));
			}
			else
			{
				UINT KeyIdx;
				KeyIdx = pWepKey->KeyIndex & 0x0fffffff;
				/* KeyIdx must be 0 ~ 3 */
				if (KeyIdx > 3) {
					Status  = -EINVAL;
					DBGPRINT(RT_DEBUG_TRACE, ("Set ApCli::OID_802_11_ADD_WEP, Failed (KeyIdx must be smaller than 4)!!\n"));
				}
				else
				{
					UCHAR CipherAlg = 0;
					PUCHAR Key;

					/* Zero the specific shared key */
					NdisZeroMemory(&pApCliEntry->SharedKey[KeyIdx], sizeof(CIPHER_KEY));

					/* set key material and key length */
					pApCliEntry->SharedKey[KeyIdx].KeyLen = (UCHAR) pWepKey->KeyLength;
					NdisMoveMemory(pApCliEntry->SharedKey[KeyIdx].Key, &pWepKey->KeyMaterial, pWepKey->KeyLength);

					switch(pWepKey->KeyLength)
					{
						case 5:
							CipherAlg = CIPHER_WEP64;
							break;
						case 13:
							CipherAlg = CIPHER_WEP128;
							break;
						default:
							DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_ADD_WEP, only support CIPHER_WEP64(len:5) & CIPHER_WEP128(len:13)!!\n"));
							Status = -EINVAL;
							break;
					}
					pApCliEntry->SharedKey[KeyIdx].CipherAlg = CipherAlg;

					/* Default key for tx (shared key) */
					if (pWepKey->KeyIndex & 0x80000000)
					{
						NdisZeroMemory(&pApCliEntry->wpa_supplicant_info.DesireSharedKey[KeyIdx], sizeof(CIPHER_KEY));

						/* set key material and key length */
						pApCliEntry->wpa_supplicant_info.DesireSharedKey[KeyIdx].KeyLen = (UCHAR) pWepKey->KeyLength;
						NdisMoveMemory(pApCliEntry->wpa_supplicant_info.DesireSharedKey[KeyIdx].Key, &pWepKey->KeyMaterial, pWepKey->KeyLength);
						pApCliEntry->wpa_supplicant_info.DesireSharedKeyId =
						(UCHAR)KeyIdx;
						pApCliEntry->wpa_supplicant_info.DesireSharedKey[KeyIdx].CipherAlg = CipherAlg;

						pApCliEntry->wdev.DefaultKeyId = (UCHAR) KeyIdx;
					}

					if ((pApCliEntry->wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
						(pApCliEntry->wdev.AuthMode >= Ndis802_11AuthModeWPA))
					{
						Key = pWepKey->KeyMaterial;

						/* Set Group key material to Asic */
						AsicAddSharedKeyEntry(pAd
						, (UCHAR)BssIdx
						, (UCHAR)KeyIdx
						, &pApCliEntry->SharedKey[KeyIdx]);

						if (pWepKey->KeyIndex & 0x80000000) {
							RTMPSetWcidSecurityInfo(pAd, (UCHAR)BssIdx
							, (UINT8)KeyIdx, CipherAlg
							, pApCliEntry->MacTabWCID, SHAREDKEYTABLE);
						}
						NdisAcquireSpinLock(&pAd->MacTabLock);
						// TODO: shiang-usw, need to replace upper setting with tr_entry
						pAd->MacTab.tr_entry[pMacEntry->wcid].PortSecured = WPA_802_1X_PORT_SECURED;
						NdisReleaseSpinLock(&pAd->MacTabLock);
					}
					else if ((pApCliEntry->Valid == TRUE)
						&& (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED))
					{
						Key = pApCliEntry->SharedKey[KeyIdx].Key;

						/* Set key material and cipherAlg to Asic */
						AsicAddSharedKeyEntry(pAd, (UCHAR)BssIdx
						, (UCHAR)KeyIdx, &pApCliEntry->SharedKey[KeyIdx]);

						if (pWepKey->KeyIndex & 0x80000000)
						{
							/* Assign pairwise key info */
							RTMPSetWcidSecurityInfo(pAd,
								(UCHAR)BssIdx,
								(UINT8)KeyIdx,
								CipherAlg,
								pApCliEntry->MacTabWCID,
								SHAREDKEYTABLE);
						}
					}
					DBGPRINT(RT_DEBUG_TRACE, ("Set ApCli::OID_802_11_ADD_WEP (id=0x%x, Len=%d-byte), %s\n",
								pWepKey->KeyIndex, pWepKey->KeyLength,
								(tr_entry->PortSecured == WPA_802_1X_PORT_SECURED ? "Port Secured":"Port NOT Secured")));
				}
			}

			if (pWepKey)
				os_free_mem(NULL, pWepKey);
	            break;

        case OID_802_11_ADD_KEY:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;
			pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
			pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];

				os_alloc_mem(NULL, (UCHAR **)&pApCliKey, wrq->u.data.length);
	            if(pApCliKey == NULL)
	            {
	                Status = -ENOMEM;
	                break;
	            }
	            Status = copy_from_user(pApCliKey, wrq->u.data.pointer, wrq->u.data.length);
		if ((pApCliKey->Length != wrq->u.data.length) ||
		    ((pApCliKey->KeyIndex & 0xFF) >= SHARE_KEY_NUM)) {
			Status  = -EINVAL;
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_ADD_KEY, Failed!!\n"));
	            }
	            else
	            {
	                RTMPApCliAddKey(pAd, ifIndex, pApCliKey);
	                DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_ADD_KEY (id=0x%x, Len=%d-byte)\n", pApCliKey->KeyIndex, pApCliKey->KeyLength));
	            }
				if (pApCliKey)
					os_free_mem(NULL, pApCliKey);
	            break;

		case OID_802_11_DISASSOCIATE:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
			pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
			pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


			if (!apcliEn || ifIndex >= MAX_APCLI_NUM)
				return FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_DISASSOCIATE	\n"));

			DisassocParmFill(pAd, &DisassocReq, pAd->MlmeAux.Bssid, REASON_DISASSOC_STA_LEAVING);

			MlmeEnqueue(pAd, APCLI_ASSOC_STATE_MACHINE, APCLI_MT2_MLME_DISASSOC_REQ,
					sizeof(MLME_DISASSOC_REQ_STRUCT), &DisassocReq, ifIndex);

			if (pApCliEntry->Valid)
				ApCliLinkDown(pAd, (UCHAR)ifIndex);

			/* set the apcli interface be invalid. */
			pApCliEntry->Valid = FALSE;

			/* clear MlmeAux.Ssid and Bssid. */
			NdisZeroMemory(pAd->MlmeAux.Bssid, MAC_ADDR_LEN);
			pAd->MlmeAux.SsidLen = 0;
			NdisZeroMemory(pAd->MlmeAux.Ssid, MAX_LEN_OF_SSID);
			pAd->MlmeAux.Rssi = 0;

			*pCurrState = APCLI_CTRL_DEASSOC;
			break;

		case OID_802_11_DROP_UNENCRYPTED:
			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
			pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
			pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];
			tr_entry = &pAd->MacTab.tr_entry[pApCliEntry->MacTabWCID];
			if (!apcliEn)
				return FALSE;

	            if (wrq->u.data.length != sizeof(int))
	                Status  = -EINVAL;
	            else
	            {
	                int enabled = 0;
	                Status = copy_from_user(&enabled, wrq->u.data.pointer, wrq->u.data.length);

			NdisAcquireSpinLock(&pAd->MacTabLock);
			if (enabled == 1)
				tr_entry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
			else
				tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
			NdisReleaseSpinLock(&pAd->MacTabLock);
	                DBGPRINT(RT_DEBUG_TRACE, ("Set ApCLi::OID_802_11_DROP_UNENCRYPTED (=%d)\n", enabled));
	            }
	            break;

	    case OID_SET_COUNTERMEASURES:
			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
			pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
			pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];

			if (!apcliEn)
				return FALSE;

	            if (wrq->u.data.length != sizeof(int))
	                Status  = -EINVAL;
	            else
	            {
	                int enabled = 0;
	                Status = copy_from_user(&enabled, wrq->u.data.pointer, wrq->u.data.length);
	                if (enabled == 1)
	                    pApCliEntry->bBlockAssoc = TRUE;
	                else
	                    /* WPA MIC error should block association attempt for 60 seconds */
	                    pApCliEntry->bBlockAssoc = FALSE;
	                	DBGPRINT(RT_DEBUG_TRACE, ("Set ApCli::OID_SET_COUNTERMEASURES bBlockAssoc=%s\n", pApCliEntry->bBlockAssoc ? "TRUE":"FALSE"));
	            }
		        break;
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/

    #ifdef P2P_SUPPORT

		case OID_802_11_P2P_MODE:

			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_P2P_MODE \n"));
			DBGPRINT(RT_DEBUG_OFF, ("wrq->u.data.length =%d sizeof(UCHAR)=%d\n"
					, wrq->u.data.length, sizeof(UCHAR)));
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
		case OID_802_11_P2P_DEVICE_NAME:
			DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_DEVICE_NAME \n"));
			if (wrq->u.data.length > 32)
				Status = -EINVAL;
			else
			{
				UCHAR DeviceName[MAX_LEN_OF_SSID] = {0};

				NdisZeroMemory(DeviceName, sizeof(DeviceName));
				Status = copy_from_user(&DeviceName, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_P2P_DEVICE_NAME DeviceName=%s\n",DeviceName));
				{

					pAd->P2pCfg.DeviceNameLen = wrq->u.data.length;
					NdisZeroMemory(pAd->P2pCfg.DeviceName, 32);
					NdisMoveMemory(pAd->P2pCfg.DeviceName, DeviceName, pAd->P2pCfg.DeviceNameLen);

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
				/*UCHAR __buf[4]; */
				UCHAR listen_ch;
				Status = copy_from_user(&listen_ch, wrq->u.data.pointer, wrq->u.data.length);
				/*sprintf(__buf, "%d", listen_ch);*/
				DBGPRINT(RT_DEBUG_ERROR, ("%s:: listen_ch = %d.\n", __FUNCTION__, listen_ch));

				{
					/*
					POS_COOKIE			pObj;
					UINT32 channel;

					pObj = (POS_COOKIE) pAd->OS_Cookie;
					if (pObj->ioctl_if_type != INT_P2P)
						return 0;

					channel = (UCHAR) simple_strtol(arg, 0, 10);
					*/
					/* check if this channel is valid */
					if (ChannelSanity(pAd, listen_ch) == TRUE)
					{
						pAd->P2pCfg.ListenChannel = listen_ch;
					}
					else
					{
						DBGPRINT(RT_DEBUG_ERROR, ("Listen Channel out of range, using default.\n"));
						pAd->P2pCfg.ListenChannel = 1;
					}

					DBGPRINT(RT_DEBUG_ERROR, ("%s:: Listen Channel = %d.\n", __FUNCTION__, pAd->P2pCfg.ListenChannel));
				}
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

				UCHAR bScan;
				Status = copy_from_user(&bScan, wrq->u.data.pointer, wrq->u.data.length);


				if (bScan)
				{
					pAd->StaCfg.bAutoReconnect = FALSE;
					P2pScan(pAd);
				}
				else
				{
					pAd->StaCfg.bAutoReconnect = TRUE;
					P2pStopScan(pAd);
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
                    if (Set_AP_WscPinCode_Proc(pAd, (RTMP_STRING *) &PinCode[0]) == FALSE)
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
						DBGPRINT(RT_DEBUG_ERROR, ("    *************************************************\n"));
						DBGPRINT(RT_DEBUG_ERROR, ("    *       PIN Code = %08u                     *\n", pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode));
						DBGPRINT(RT_DEBUG_ERROR, ("    *************************************************\n"));

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
#endif /* P2P_SUPPORT */

    	case OID_802_11_DEAUTHENTICATION:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_DEAUTHENTICATION\n"));
			if (wrq->u.data.length != sizeof(MLME_DEAUTH_REQ_STRUCT))
				Status  = -EINVAL;
			else
			{
				MAC_TABLE_ENTRY *pEntry = NULL;
				MLME_DEAUTH_REQ_STRUCT  *pInfo = NULL;
				MLME_QUEUE_ELEM *Elem;

				os_alloc_mem(pAd, (UCHAR **)&Elem, sizeof(MLME_QUEUE_ELEM));

				if(Elem == NULL)
				{
					Status = -ENOMEM;
					DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_DEAUTHENTICATION, Failed!!\n"));
					break;
				}

#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
				if (pObj->ioctl_if_type == INT_APCLI)
				{
					ifIndex = pObj->ioctl_if;
					apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
					pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
					pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;
					if (ifIndex >= MAX_APCLI_NUM)
					{
						os_free_mem(NULL, Elem);
						return FALSE;
					}

					os_alloc_mem(pAd, (UCHAR **)&pInfo, sizeof(MLME_DEAUTH_REQ_STRUCT));
					if(!pInfo)
					{
						DBGPRINT(RT_DEBUG_ERROR, ("Set::OID_802_11_DEAUTHENTICATION, alloc pInfo buffer Failed!\n"));
						os_free_mem(NULL, Elem);
						return FALSE;
					}
					Status = copy_from_user(pInfo, wrq->u.data.pointer, wrq->u.data.length);
					/* Fill in the related information */
					DeAuthFrame.Reason = (USHORT)pInfo->Reason;
					COPY_MAC_ADDR(DeAuthFrame.Addr, pInfo->Addr);

					MlmeEnqueue(pAd,
								  APCLI_AUTH_STATE_MACHINE,
								  APCLI_MT2_MLME_DEAUTH_REQ,
								  sizeof(MLME_DEAUTH_REQ_STRUCT),
								  &DeAuthFrame,
								  ifIndex);

					if (pApCliEntry->Valid)
						ApCliLinkDown(pAd, (UCHAR)ifIndex);

					/* set the apcli interface be invalid.*/
					pApCliEntry->Valid = FALSE;

					/* clear MlmeAux.Ssid and Bssid.*/
					NdisZeroMemory(pAd->MlmeAux.Bssid, MAC_ADDR_LEN);
					pAd->MlmeAux.SsidLen = 0;
					NdisZeroMemory(pAd->MlmeAux.Ssid, MAX_LEN_OF_SSID);
					pAd->MlmeAux.Rssi = 0;

					*pCurrState = APCLI_CTRL_DISCONNECTED;
					if(pInfo)
						os_free_mem(NULL, pInfo);

					os_free_mem(NULL, Elem);
				}
				else
#endif /* WPA_SUPPLICANT_SUPPORT */
#endif/*APCLI_SUPPORT*/
				{
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
						os_free_mem(NULL, Elem);
					}
					else
						Status = -EFAULT;
				}
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
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_APD_PID::(IappPid=%lu(0x%lx))\n", IappPid, (ULONG)pObj->IappPid));
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
					DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_APD_PID::(ApdPid=%lu(0x%lx))\n", apd_pid, (ULONG)pObj->apd_pid));
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
			MAC_TABLE_ENTRY *pEntry = NULL;

			DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_DEL_MAC_ENTRY::(%02x:%02x:%02x:%02x:%02x:%02x)\n", Addr[0],Addr[1],Addr[2],Addr[3],Addr[4],Addr[5]));

			pEntry = MacTableLookup(pAd, Addr);
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
				if (pUPnPMsg == NULL)
					Status = -EINVAL;
				else
				{
					int HeaderLen;
					RTMP_STRING *pWpsMsg;
					UINT WpsMsgLen;
					PWSC_CTRL pWscControl;
					//BOOLEAN	bGetDeviceInfo = FALSE;

					NdisZeroMemory(pUPnPMsg, msgLen);
					retVal = copy_from_user(pUPnPMsg, wrq->u.data.pointer, msgLen);

					msgHdr = (RTMP_WSC_U2KMSG_HDR *)pUPnPMsg;
					senderID = get_unaligned((INT32 *)(&msgHdr->Addr2[0]));
					/*senderID = *((int *)&msgHdr->Addr2); */

					DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_EAPMSG++++++++\n\n"));
					hex_dump("MAC::", &msgHdr->Addr3[0], MAC_ADDR_LEN);
					DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_EAPMSG++++++++\n\n"));

					HeaderLen = LENGTH_802_11 + LENGTH_802_1_H + sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME);
					pWpsMsg = (RTMP_STRING *)&pUPnPMsg[HeaderLen];
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
							WscInit(pAd, FALSE, pObj->ioctl_if);
							/* trigger wsc re-generate public key */
							pWscControl->RegData.ReComputePke = 1;
							//bGetDeviceInfo = TRUE;
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
#ifdef WSC_UFD
		case RT_OID_WSC_READ_UFD_FILE:
			if (wrq->u.data.length > 0)
			{
				RTMP_STRING *pWscUfdFileName = NULL;
				UCHAR apIdx = pObj->ioctl_if;
				WSC_CTRL *pWscCtrl = &pAd->ApCfg.MBSSID[apIdx].WscControl;

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
				RTMP_STRING*pWscUfdFileName = NULL;
				UCHAR apIdx = pObj->ioctl_if;
				WSC_CTRL *pWscCtrl = &pAd->ApCfg.MBSSID[apIdx].WscControl;

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
					os_free_mem(NULL, pWscUfdFileName);
				}
				else
					Status = -ENOMEM;
			}
			else
				Status = -EINVAL;
			break;
#endif /* WSC_UFD */
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
				AsicSetRetryLimit(pAd, TX_RTY_CFG_RTY_LIMIT_SHORT, ShortRetryLimit);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_SHORTRETRYLIMIT (ShortRetryLimit=%ld)\n", ShortRetryLimit));
			}
			break;

		case OID_802_11_LONGRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_LONGRETRYLIMIT \n"));
			if (wrq->u.data.length != sizeof(ULONG))
				Status = -EINVAL;
			else
			{
				Status = copy_from_user(&LongRetryLimit, wrq->u.data.pointer, wrq->u.data.length);
				AsicSetRetryLimit(pAd, TX_RTY_CFG_RTY_LIMIT_LONG, LongRetryLimit);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_LONGRETRYLIMIT (,LongRetryLimit=%ld)\n", LongRetryLimit));
			}
			break;

		case OID_802_11_WEPDEFAULTKEYVALUE:
		{
			UINT KeyIdx;
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYVALUE\n"));

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

		}
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
					UCHAR rf_channel;

					pAd->MeshTab.MeshChannel = pAd->CommonCfg.Channel = Channel;
#ifdef DOT11_N_SUPPORT
					N_ChannelCheck(pAd);
					if (WMODE_CAP_N(pAd->CommonCfg.PhyMode)
						&& pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40)
						rf_channel = N_SetCenCh(pAd, pAd->CommonCfg.Channel);
					else
#endif /* DOT11_N_SUPPORT */
						rf_channel = pAd->CommonCfg.Channel;

					AsicSwitchChannel(pAd, rf_channel, FALSE);
					AsicLockChannel(pAd, rf_channel);
					DBGPRINT(RT_DEBUG_TRACE, ("BW_%d, control_channel(%d), CentralChannel(%d) \n",
							pAd->CommonCfg.RegTransmitSetting.field.BW,
							pAd->CommonCfg.Channel, pAd->CommonCfg.CentralChannel));
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
					DBGPRINT(RT_DEBUG_TRACE, ("OID_802_11_WAPI_PID::(WapiPid=%lu(0x%x))\n", wapi_pid, (UINT32)pObj->wapi_pid));
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
				STA_TR_ENTRY *tr_entry;

				Status = copy_from_user(&wapi_port, wrq->u.data.pointer, wrq->u.data.length);
                if (Status == NDIS_STATUS_SUCCESS)
                {
					if ((pEntry = MacTableLookup(pAd, wapi_port.Addr)) != NULL)
					{
						tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];
						switch (wapi_port.state)
						{
							case WAPI_PORT_SECURED:
								tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
								pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
								break;

							default:
								tr_entry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
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

#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
		if (pObj->ioctl_if_type == INT_APCLI)
		{
			ifIndex = pObj->ioctl_if;
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
			if(apcliEn)
			{
                   		 if (pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode != AuthMode)
                   		 {
		                        /* Config has changed */
		                        pAd->ApCfg.ApCliTab[ifIndex].bConfigChanged = TRUE;
		               }
                    		pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode = AuthMode;

				for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
				{
					if (IS_ENTRY_APCLI(&pAd->MacTab.Content[i]))
					{
						pAd->MacTab.tr_entry[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
					}
				}

				RTMPMakeRSNIE(pAd, pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode,
					pAd->ApCfg.ApCliTab[ifIndex].wdev.WepStatus
					, (UCHAR)(ifIndex + MIN_NET_DEVICE_FOR_APCLI));
				pAd->ApCfg.ApCliTab[ifIndex].wdev.DefaultKeyId  = 0;

				if(pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode >= Ndis802_11AuthModeWPA)
					pAd->ApCfg.ApCliTab[ifIndex].wdev.DefaultKeyId = 1;
			}
		}
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/
                else
                {
                    if (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode != AuthMode)
                    {
                        /* Config has changed */
                        pAd->bConfigChanged = TRUE;
                    }
                    pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode = AuthMode;
                }
                pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.PortSecured = WPA_802_1X_PORT_NOT_SECURED;
                DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_AUTHENTICATION_MODE (=%d) \n", pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode));
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

#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
				if (pObj->ioctl_if_type == INT_APCLI)
				{
					ifIndex = pObj->ioctl_if;
					apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
					pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
					pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;
					if (ifIndex >= MAX_APCLI_NUM)
						return FALSE;

					if(apcliEn)
					{
						 if (WepStatus <= Ndis802_11GroupWEP104Enabled)
		                		{
			                   		 if (pAd->ApCfg.ApCliTab[ifIndex].wdev.WepStatus != WepStatus)
			                   		 {
					                        /* Config has changed */
					                        pAd->ApCfg.ApCliTab[ifIndex].bConfigChanged = TRUE;
					               }
			                    		pAd->ApCfg.ApCliTab[ifIndex].wdev.WepStatus = WepStatus;

							for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
							{
								if (IS_ENTRY_APCLI(&pAd->MacTab.Content[i]))
								{
									INT idx = pAd->MacTab.Content[i].wcid;  
									pAd->MacTab.tr_entry[idx].PortSecured = WPA_802_1X_PORT_NOT_SECURED;
								}
							}

							pApCliEntry->PairCipher     = pApCliEntry->wdev.WepStatus;
							pApCliEntry->GroupCipher    = pApCliEntry->wdev.WepStatus;
							pApCliEntry->bMixCipher		= FALSE;

							if (pApCliEntry->wdev.WepStatus >= Ndis802_11TKIPEnable)
								pApCliEntry->wdev.DefaultKeyId = 1;

						RTMPMakeRSNIE(pAd
						, pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode
						, pAd->ApCfg.ApCliTab[ifIndex].wdev.WepStatus
						, (UCHAR)(ifIndex + MIN_NET_DEVICE_FOR_APCLI));

						 }
					}

				}
				else
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/
		{
		/* Since TKIP, AES, WEP are all supported. It should not have any invalid setting */
                if (WepStatus <= Ndis802_11GroupWEP104Enabled)
                {
                	struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
					if (wdev->WepStatus != WepStatus)
					{
						/* Config has changed */
						pAd->bConfigChanged = TRUE;
					}
					wdev->WepStatus = WepStatus;

					if (wdev->WepStatus == Ndis802_11TKIPAESMix)
						wdev->GroupKeyWepStatus = Ndis802_11TKIPEnable;
					else
						wdev->GroupKeyWepStatus = WepStatus;
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
						os_alloc_mem(NULL, (UCHAR **)&pSsidString, MAX_LEN_OF_SSID+1);
					if (pSsidString)
					{
						NdisZeroMemory(pSsidString, MAX_LEN_OF_SSID+1);
						NdisMoveMemory(pSsidString, Ssid.Ssid, Ssid.SsidLength);
#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
						if (pObj->ioctl_if_type == INT_APCLI)
						{
							ifIndex = pObj->ioctl_if;
							apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;
							if(apcliEn)
							{
								Set_ApCli_Ssid_Proc(pAd,pSsidString);
							}
						}
						else
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/
						{
							NdisZeroMemory((PCHAR)pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,MAX_LEN_OF_SSID);
							strcpy((PCHAR)pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,pSsidString);
							pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen =
							(UCHAR)strlen(pSsidString);
						}
								os_free_mem(NULL, pSsidString);
					}
					else
						Status = -ENOMEM;
                    }
                }
            }
            break;

#ifdef HOSTAPD_SUPPORT
	case HOSTAPD_OID_SET_802_1X:/*pure 1x is enabled. */
			Set_IEEE8021X_Proc(pAd,"1");
			break;

	case HOSTAPD_OID_SET_KEY:
	{
			UINT KeyIdx;
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
					KeyIdx=pAd->ApCfg.MBSSID[pEntry->func_tb_idx].DefaultKeyId;
                                  AsicAddPairwiseKeyEntry(
                                      pAd,
                                      (UCHAR)pEntry->wcid,
                                      &pEntry->PairwiseKey);

					RTMPAddWcidAttributeEntry(
						pAd,
						pEntry->func_tb_idx,
						KeyIdx, /* The value may be not zero */
						pEntry->PairwiseKey.CipherAlg,
						pEntry);
				}
				else/*group key */
				{
					pMbss = &pAd->ApCfg.MBSSID[apidx];
					DBGPRINT(RT_DEBUG_OFF, ("Key.ik_keyix=%x\n", Key.ik_keyix));
					KeyIdx = Key.ik_keyix& 0x0fff;
					DBGPRINT(RT_DEBUG_OFF, ("ra%d KeyIdx=%d\n", apidx, KeyIdx));
					DBGPRINT(RT_DEBUG_OFF, ("Key.ik_keyix=%x\n", Key.ik_keyix));
					/* it is a shared key */
					if (KeyIdx < 4)
					{
						pAd->SharedKey[apidx][KeyIdx].KeyLen = (UCHAR) Key.ik_keylen;
						NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].Key, &Key.ik_keydata, Key.ik_keylen);
						if (Key.ik_keyix & 0x8000)
						{
							/* Default key for tx (shared key) */
							DBGPRINT(RT_DEBUG_OFF, ("ra%d DefaultKeyId=%d\n"
									, apidx, KeyIdx));
							pMbss->DefaultKeyId = (UCHAR) KeyIdx;
						}
						/*pMbss->DefaultKeyId=1; */

						pAd->SharedKey[apidx][KeyIdx].CipherAlg = Key.ik_type;
						AsicAddSharedKeyEntry(
							pAd,
							(UCHAR)apidx,
							(UCHAR)KeyIdx,
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
					pEntry->func_tb_idx,
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
					(UCHAR)apidx,
					(UCHAR)pMbss->DefaultKeyId,
					&pAd->SharedKey[apidx][pMbss->DefaultKeyId]
					);
				GET_GroupKey_WCID(pAd, Wcid, apidx);

				RTMPSetWcidSecurityInfo(pAd, (UINT8)apidx, (UINT8)KeyIdx,
									pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg, Wcid, SHAREDKEYTABLE);

				/*RTMPAddWcidAttributeEntry(
				pAd,
				apidx,
				pMbss->DefaultKeyId,
				pAd->SharedKey[apidx][pMbss->DefaultKeyId].CipherAlg,
				NULL);*/
			}
			break;
		}
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
				AsicRemoveSharedKeyEntry(pAd, pEntry->func_tb_idx, delkey.idk_keyix);
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
					tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];
					switch (mlme.im_op)
					{
						case IEEE80211_MLME_AUTHORIZE:
							pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
							pEntry->WpaState = AS_PTKINITDONE;/*wpa state machine is not in use. */
							/*pAd->StaCfg.PortSecured= WPA_802_1X_PORT_SECURED; */
							tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
							break;
						case IEEE80211_MLME_UNAUTHORIZE:
							pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
							tr_entry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
						break;
						default:
							Status = -EINVAL;
					}
				}
			}
			break;

		case HOSTAPD_OID_STATIC_WEP_COPY:
		{
			UINT KeyIdx;
			INT	 apidx;
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
				else{
					/*Status  = -EINVAL; */
					DBGPRINT(RT_DEBUG_OFF, ("HOSTAPD_OID_STATIC_WEP_COPY IEEE8021X=%d WepStatus=%d\n"
							, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X, pEntry->WepStatus));
					if (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X != TRUE)
						break;
					if (pEntry->WepStatus != Ndis802_11WEPEnabled)
						break;

					apidx = pObj->ioctl_if;
					pMbss = &pAd->ApCfg.MBSSID[apidx];

					KeyIdx=pMbss->DefaultKeyId;
					DBGPRINT(RT_DEBUG_OFF, ("HOSTAPD_OID_STATIC_WEP_COPY=%d\n", KeyIdx));
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
							pEntry->func_tb_idx,
							KeyIdx, /* The value may be not zero */
							pEntry->PairwiseKey.CipherAlg,
							pEntry);

				}
			}
			break;
		}
		case HOSTAPD_OID_SET_STA_DEAUTH:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::HOSTAPD_OID_SET_STA_DEAUTH\n"));
			MLME_DEAUTH_REQ_STRUCT  *pInfo;
			MLME_QUEUE_ELEM *Elem;

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
			if (wrq->u.data.length != sizeof(BOOLEAN))
				Status  = -EINVAL;
			else
			{
				BOOLEAN hostapd_enable;
				int v, apidx;
				apidx = pObj->ioctl_if;
				Status = copy_from_user(&hostapd_enable, wrq->u.data.pointer, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_OFF, ("OID_HOSTAPD_SUPPORT apidx=%d\n", apidx));
				pAd->ApCfg.MBSSID[apidx].Hostapd = hostapd_enable;
				BSS_STRUCT *pMBSSStruct;

				for(v=0;v<MAX_MBSSID_NUM(pAd);v++)
				{
					DBGPRINT(RT_DEBUG_OFF, ("ApCfg->MBSSID[%d].Hostapd == %s\n"
							, v, (pAd->ApCfg.MBSSID[v].Hostapd == Hostapd_EXT ? "TRUE" : "FALSE")));
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

						        /* Further,  ban all Class 3 DATA transportation for a period of 60 sec */
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
				DBGPRINT(RT_DEBUG_TRACE,("HOSTAPD_OID_SET_WPS_BEACON_IE\n"));
				if (wrq->u.data.length != sizeof(WSC_LV_INFO))
				{
					Status  = -EINVAL;
				}
				else
				{
					INT apidx;
					apidx = pObj->ioctl_if;
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
					INT apidx;
					apidx = pObj->ioctl_if;
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

#ifdef CONFIG_HOTSPOT
	case OID_802_11_HS_TEST:
		DBGPRINT(RT_DEBUG_TRACE, ("hotspot test\n"));
		break;
	case OID_802_11_HS_IE:
		{
			UCHAR *IE;
			os_alloc_mem(NULL, (UCHAR **)&IE, wrq->u.data.length);
			copy_from_user(IE, wrq->u.data.pointer, wrq->u.data.length);
			Set_AP_IE(pAd, IE, wrq->u.data.length);
			os_free_mem(NULL, IE);
		}
		break;
	case OID_802_11_HS_ANQP_RSP:
		{
			UCHAR *Buf;
			struct anqp_rsp_data *rsp_data;
			os_alloc_mem(NULL, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
			rsp_data = (struct anqp_rsp_data *)Buf;
			Send_ANQP_Rsp(pAd,
				      rsp_data->peer_mac_addr,
				      rsp_data->anqp_rsp,
				      rsp_data->anqp_rsp_len);
			os_free_mem(NULL, Buf);
		}
		break;
	case OID_802_11_HS_ONOFF:
		{
			UCHAR *Buf;
			struct hs_onoff *onoff;
			os_alloc_mem(NULL, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
			onoff = (struct hs_onoff *)Buf;
			Set_HotSpot_OnOff(pAd, onoff->hs_onoff, onoff->event_trigger, onoff->event_type);
			os_free_mem(NULL, Buf);
		}
		break;
	case OID_802_11_HS_PARAM_SETTING:
		{
			UCHAR *Buf;
			struct hs_param_setting *param_setting;
			os_alloc_mem(NULL, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
			param_setting = (struct hs_param_setting *)Buf;
			Set_HotSpot_Param(pAd, param_setting->param, param_setting->value);
			os_free_mem(NULL, Buf);
		}
		break;
	case OID_802_11_HS_RESET_RESOURCE:
		DBGPRINT(RT_DEBUG_TRACE, ("hotspot reset some resource\n"));
		Clear_Hotspot_All_IE(pAd);
		//Clear_All_PROXY_TABLE(pAd);
		break;
#ifdef CONFIG_HOTSPOT_R2	
	case OID_802_11_HS_SASN_ENABLE:
		{
			UCHAR *Buf;
			PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[pObj->ioctl_if].HotSpotCtrl;
			os_alloc_mem(NULL, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
			pHSCtrl->bASANEnable = Buf[0];
			os_free_mem(NULL, Buf);
		}
		break;
	case OID_802_11_BSS_LOAD:
		{
			UCHAR *Buf;
			PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[pObj->ioctl_if].HotSpotCtrl;
			os_alloc_mem(NULL, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
			pHSCtrl->QLoadTestEnable = Buf[0];
			pHSCtrl->QLoadCU = Buf[1];
			memcpy(&pHSCtrl->QLoadStaCnt, &Buf[2], 2);
			os_free_mem(NULL, Buf);
		}
		break;	
#endif		
#endif

#ifdef CONFIG_DOT11V_WNM
#ifdef CONFIG_HOTSPOT_R2
	case OID_802_11_WNM_BTM_REQ:
		{
			UCHAR *Buf;
			MAC_TABLE_ENTRY  *pEntry;
			struct btm_req_data *req_data;
			PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[pObj->ioctl_if].HotSpotCtrl;
			
			os_alloc_mem(Buf, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
			req_data = (struct btm_req_data *)Buf;
			
			if ((pEntry = MacTableLookup(pAd, req_data->peer_mac_addr)) == NULL)
    		{	
    		}
    		else if (((pHSCtrl->HotSpotEnable) && (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode == Ndis802_11AuthModeOpen)) ||
				((pEntry->WpaState == AS_PTKINITDONE) && (pEntry->GTKState = REKEY_ESTABLISHED)))
    		{
				DBGPRINT(RT_DEBUG_OFF, ("btm1\n"));
				Send_BTM_Req(pAd,
				     req_data->peer_mac_addr,
				     req_data->btm_req,
				     req_data->btm_req_len);
			}
			else
			{
				DBGPRINT(RT_DEBUG_OFF, ("btm2\n"));
				pEntry->IsBTMReqValid = TRUE;
				os_alloc_mem(pEntry->ReqbtmData, (UCHAR **)&pEntry->ReqbtmData, sizeof(struct btm_req_data)+req_data->btm_req_len);	
				memcpy(pEntry->ReqbtmData, Buf, sizeof(struct btm_req_data)+req_data->btm_req_len);
			}
			os_free_mem(NULL, Buf); 
		} 
		break;
	case OID_802_11_WNM_NOTIFY_REQ:
		{
			UCHAR *Buf;
			MAC_TABLE_ENTRY  *pEntry;
			struct wnm_req_data *req_data;
			PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[pObj->ioctl_if].HotSpotCtrl;
			
			os_alloc_mem(Buf, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
		
			req_data = (struct wnm_req_data *)Buf;
			//printk("addr=%02x:%02x:%02x:%02x:%02x:%02x\n", req_data->peer_mac_addr[0],req_data->peer_mac_addr[1],req_data->peer_mac_addr[2],req_data->peer_mac_addr[3],req_data->peer_mac_addr[4],req_data->peer_mac_addr[5]);
			//for(k=0;k<req_data->wnm_req_len;k++)
			//	printk("%02x:", *(req_data->wnm_req+k));
			
			//printk("req len=%d\n",req_data->wnm_req_len);
			if ((pEntry = MacTableLookup(pAd, req_data->peer_mac_addr)) == NULL)
    		{	
    		}
    		else if (((pHSCtrl->HotSpotEnable) && (pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode == Ndis802_11AuthModeOpen)) ||
				((pEntry->WpaState == AS_PTKINITDONE) && (pEntry->GTKState = REKEY_ESTABLISHED)))
    		{	
				DBGPRINT(RT_DEBUG_OFF, ("wnm1\n"));
				Send_WNM_Notify_Req(pAd,
				     req_data->peer_mac_addr,
				     req_data->wnm_req,
				     req_data->wnm_req_len,
				     req_data->type);
			}	
			else
			{
				DBGPRINT(RT_DEBUG_OFF, ("wnm2\n"));
				pEntry->IsWNMReqValid = TRUE;
				os_alloc_mem(pEntry->ReqData, (UCHAR **)&pEntry->ReqData, sizeof(struct wnm_req_data)+req_data->wnm_req_len);	
				memcpy(pEntry->ReqData, Buf, sizeof(struct wnm_req_data)+req_data->wnm_req_len);
			}

			os_free_mem(NULL, Buf); 
		} 
		break;	
	case OID_802_11_QOSMAP_CONFIGURE:
		{
			UCHAR *Buf;
			MAC_TABLE_ENTRY  *pEntry;
			struct qosmap_data *req_data;
			unsigned int i;
		
			os_alloc_mem(Buf, (UCHAR **)&Buf, wrq->u.data.length);
			copy_from_user(Buf, wrq->u.data.pointer, wrq->u.data.length);
		
			req_data = (struct qosmap_data *)Buf;
			
			if ((pEntry = MacTableLookup(pAd, req_data->peer_mac_addr)) != NULL)
    		{	
    			//clear previous data
    			pEntry->DscpExceptionCount = 0;
 				memset(pEntry->DscpRange, 0xff, 16);
 				memset(pEntry->DscpException, 0xff, 42);
 			
    			pEntry->DscpExceptionCount = req_data->qosmap_len-16;
 				memcpy((UCHAR *)pEntry->DscpRange, &req_data->qosmap[pEntry->DscpExceptionCount], 16);
 				if (pEntry->DscpExceptionCount != 0)
 					memcpy((UCHAR *)pEntry->DscpException, req_data->qosmap, pEntry->DscpExceptionCount);
 		
				Send_QOSMAP_Configure(pAd,
				     req_data->peer_mac_addr,
				     req_data->qosmap,
				     req_data->qosmap_len,
				     pEntry->apidx);
			}	
			else if ((req_data->peer_mac_addr[0] == 0) 
				&& (req_data->peer_mac_addr[1] == 0)
				&& (req_data->peer_mac_addr[2] == 0)
				&& (req_data->peer_mac_addr[3] == 0)
				&& (req_data->peer_mac_addr[4] == 0)
				&& (req_data->peer_mac_addr[5] == 0) ) {
			/* Special MAC 00:00:00:00:00:00 for HS2 QoS Map Change using. */
				for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++) {
					pEntry = &pAd->MacTab.Content[i];

					if ((IS_ENTRY_CLIENT(pEntry))
							&& (pEntry->Sst == SST_ASSOC)) {
						if (pEntry->QosMapSupport) {	
							pEntry->DscpExceptionCount = 0;
							memset(pEntry->DscpRange, 0xff, 16);
							memset(pEntry->DscpException, 0xff, 42);

							pEntry->DscpExceptionCount = req_data->qosmap_len-16;
							memcpy((UCHAR *)pEntry->DscpRange, 
									&req_data->qosmap[pEntry->DscpExceptionCount], 16);
							if (pEntry->DscpExceptionCount != 0) {
								memcpy((UCHAR *)pEntry->DscpException, 
										req_data->qosmap, pEntry->DscpExceptionCount);
							} 
							DBGPRINT(RT_DEBUG_OFF, ("send QoS map frame: apidx=%d\n", pEntry->apidx));
							Send_QOSMAP_Configure(pAd,
									pEntry->Addr,
									req_data->qosmap,
									req_data->qosmap_len,
									pEntry->apidx);
						}
					}
				}
			}	
			os_free_mem(NULL, Buf);
		}
		break;
#endif
#endif
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
	IN RTMP_ADAPTER *pAd,
	IN OUT RTMP_IOCTL_INPUT_STRUCT *rq,
	IN INT cmd)
{
	RTMP_IOCTL_INPUT_STRUCT	*wrq = (RTMP_IOCTL_INPUT_STRUCT *) rq;
	ULONG Status = NDIS_STATUS_SUCCESS;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	RTMP_STRING driverVersion[16];
	UCHAR apidx = pObj->ioctl_if;
#ifdef WSC_AP_SUPPORT
	UINT WscPinCode = 0;
	PWSC_PROFILE pProfile;
	PWSC_CTRL pWscControl;
#endif /* WSC_AP_SUPPORT */

#ifdef SNMP_SUPPORT
	ULONG ulInfo;
	DefaultKeyIdxValue *pKeyIdxValue;
	INT valueLen;
	TX_RTY_CFG_STRUC tx_rty_cfg;
	ULONG ShortRetryLimit, LongRetryLimit;
	UCHAR snmp_tmp[64];
#endif /* SNMP_SUPPORT */

#ifdef HOSTAPD_SUPPORT
	struct default_group_key group_key;
	struct ieee80211req_key ik;
	unsigned char *p;
	MAC_TABLE_ENTRY *pEntry=(MAC_TABLE_ENTRY *)NULL;
	struct ieee80211req_wpaie wpaie;
	BSS_STRUCT *pMbss;
#endif /*HOSTAPD_SUPPORT*/

#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	INT ifIndex;
	ULONG 						BssBufSize;
	BOOLEAN apcliEn=FALSE;
	INT 								i,Padding = 0;
	PUCHAR                              pBuf = NULL, pPtr=NULL;
	NDIS_802_11_BSSID_LIST_EX           *pBssidList = NULL;
	USHORT                              BssLen = 0;
	PNDIS_WLAN_BSSID_EX                 pBss;
	MAC_TABLE_ENTRY				*pMacEntry=(MAC_TABLE_ENTRY *)NULL;
	STA_TR_ENTRY *tr_entry;
	PAPCLI_STRUCT pApCliEntry=NULL;
	NDIS_802_11_SSID                    Ssid;
	UINT                                we_version_compiled;
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/


	NDIS_802_11_STATISTICS	*pStatistics;

#ifdef EASY_CONFIG_SETUP
	PUCHAR						pStaMacAddr = NULL;
#endif /* EASY_CONFIG_SETUP */

#ifdef P2P_SUPPORT
	/*RT_P2P_UI_TABLE UI_table;*/
	PRT_P2P_UI_TABLE pUI_table;
	PRT_P2P_TABLE			pP2pTable;
	PRT_P2P_CLIENT_ENTRY   pPAdCli, pUICli;
	PRT_P2P_CONFIG	pP2PCtrl; /* = &pAd->P2pCfg; */
	UCHAR tmp[24];
	UCHAR i;
#endif /* P2P_SUPPORT */

#ifdef DOT1X_SUPPORT
	INT IEEE8021X = 0; 
#endif /* DOT1X_SUPPORT */

	INT mBss_staCount = 0;
	NDIS_802_11_AUTHENTICATION_MODE AuthMode = Ndis802_11AuthModeMax;
#ifdef RTMP_RBUS_SUPPORT
	struct wifi_dev *wdev = NULL;
#endif /* RTMP_RBUS_SUPPORT */
	/* For all ioctl to this function, we assume that's query for AP/APCLI/GO device */
	if ((pObj->ioctl_if_type == INT_MBSSID) || (pObj->ioctl_if_type == INT_MAIN))
	{
		if (apidx >= pAd->ApCfg.BssidNum)
			return EFAULT;
#ifdef RTMP_RBUS_SUPPORT
		wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
#endif /* RTMP_RBUS_SUPPORT */
	}

    switch(cmd)
    {
	case OID_802_11_MBSS_GET_STA_COUNT:
		wrq->u.data.length = sizeof(INT);
		mBss_staCount = pAd->ApCfg.MBSSID[apidx].StaCount;
		DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MBSS_GET_STA_COUNT ==> %d \n", mBss_staCount));
		Status = copy_to_user(wrq->u.data.pointer, &mBss_staCount, wrq->u.data.length);
		break;
#ifdef DOT1X_SUPPORT
	case OID_802_11_SET_IEEE8021X:
		DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_SET_IEEE8021X \n"));
                wrq->u.data.length = sizeof(INT);
                if(pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.IEEE8021X == TRUE)
                        IEEE8021X=1;
                else
                        IEEE8021X=0;

                Status = copy_to_user(wrq->u.data.pointer, &IEEE8021X, wrq->u.data.length);
                break;
#endif /* DOT1X_SUPPORT */
	case OID_802_11_AUTHENTICATION_MODE:
                DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_AUTHENTICATION_MODE \n"));
	        wrq->u.data.length = sizeof(NDIS_802_11_AUTHENTICATION_MODE);
	        AuthMode=pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.AuthMode;
	        Status = copy_to_user(wrq->u.data.pointer, &AuthMode, wrq->u.data.length);
                break;

#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	        case RT_OID_NEW_DRIVER:
	            {
	                UCHAR enabled = 1;
	    	        wrq->u.data.length = sizeof(UCHAR);
	    	        Status = copy_to_user(wrq->u.data.pointer, &enabled, wrq->u.data.length);
	                DBGPRINT(RT_DEBUG_TRACE, ("Query apcli::RT_OID_NEW_DRIVER (=%d)\n", enabled));
			   break;
	            }

	    	case RT_OID_WE_VERSION_COMPILED:
		        	wrq->u.data.length = sizeof(UINT);
			        we_version_compiled = RtmpOsWirelessExtVerGet();
		        	Status = copy_to_user(wrq->u.data.pointer, &we_version_compiled, wrq->u.data.length);
#ifdef RELEASE_EXCLUDE
			        DBGPRINT(RT_DEBUG_INFO, ("Query::RT_OID_WE_VERSION_COMPILED (=%d)\n", we_version_compiled));
#endif /* RELEASE_EXCLUDE */
		        	break;

        case OID_802_11_BSSID_LIST:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			pMacEntry = &pAd->MacTab.Content[pAd->ApCfg.ApCliTab[ifIndex].MacTabWCID];
			//tr_entry = &pAd->MacTab.Content[pMacEntry->wcid];
			tr_entry = &pAd->MacTab.tr_entry[pMacEntry->wcid];
            if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
            {
            	/*
            	 * Still scanning, indicate the caller should try again.
            	 */
            	DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_BSSID_LIST (Still scanning)\n"));
				return -EAGAIN;
            }
			if ((pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.WpaSupplicantUP & 0x7F) == WPA_SUPPLICANT_ENABLE)
			{
				pAd->ApCfg.ApCliTab[ifIndex].wpa_supplicant_info.WpaSupplicantScanCount = 0;
			}

            DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_BSSID_LIST (%d BSS returned)\n",pAd->ScanTab.BssNr));
            BssBufSize = sizeof(ULONG);

            for (i = 0; i < pAd->ScanTab.BssNr; i++)
            {
                BssBufSize += (sizeof(NDIS_WLAN_BSSID_EX) - 1 + sizeof(NDIS_802_11_FIXED_IEs) + pAd->ScanTab.BssEntry[i].VarIELen + Padding);
            }

            BssBufSize += 256;
		os_alloc_mem(pAd, (UCHAR **)&pBuf, BssBufSize);
            if(pBuf == NULL)
            {
                Status = -ENOMEM;
                break;
            }
            NdisZeroMemory(pBuf, BssBufSize);
            pBssidList = (PNDIS_802_11_BSSID_LIST_EX) pBuf;
            pBssidList->NumberOfItems = pAd->ScanTab.BssNr;

            BssLen = 4; /* Consist of NumberOfItems */
            pPtr = (PUCHAR) &pBssidList->Bssid[0];
            for (i = 0; i < pAd->ScanTab.BssNr; i++)
            {
                pBss = (PNDIS_WLAN_BSSID_EX) pPtr;
                NdisMoveMemory(&pBss->MacAddress, &pAd->ScanTab.BssEntry[i].Bssid, MAC_ADDR_LEN);
                if ((pAd->ScanTab.BssEntry[i].Hidden == 1))
                {
                    /*
					 We must return this SSID during 4way handshaking, otherwise Aegis will failed to parse WPA infomation
					 and then failed to send EAPOl farame.
			*/
					if ((pAd->ApCfg.ApCliTab[ifIndex].wdev.AuthMode >= Ndis802_11AuthModeWPA) && (tr_entry->PortSecured != WPA_802_1X_PORT_SECURED))
					{
						pBss->Ssid.SsidLength = pAd->ScanTab.BssEntry[i].SsidLen;
						NdisMoveMemory(pBss->Ssid.Ssid, pAd->ScanTab.BssEntry[i].Ssid, pAd->ScanTab.BssEntry[i].SsidLen);
					}
					else
                    	pBss->Ssid.SsidLength = 0;
                }
                else
                {
                    pBss->Ssid.SsidLength = pAd->ScanTab.BssEntry[i].SsidLen;
                    NdisMoveMemory(pBss->Ssid.Ssid, pAd->ScanTab.BssEntry[i].Ssid, pAd->ScanTab.BssEntry[i].SsidLen);
                }
                pBss->Privacy = pAd->ScanTab.BssEntry[i].Privacy;
                pBss->Rssi = pAd->ScanTab.BssEntry[i].Rssi - pAd->BbpRssiToDbmDelta;
                pBss->NetworkTypeInUse = NetworkTypeInUseSanity(&pAd->ScanTab.BssEntry[i]);
                pBss->Configuration.Length = sizeof(NDIS_802_11_CONFIGURATION);
                pBss->Configuration.BeaconPeriod = pAd->ScanTab.BssEntry[i].BeaconPeriod;
                pBss->Configuration.ATIMWindow = pAd->ScanTab.BssEntry[i].AtimWin;

                MAP_CHANNEL_ID_TO_KHZ(pAd->ScanTab.BssEntry[i].Channel, pBss->Configuration.DSConfig);

                if (pAd->ScanTab.BssEntry[i].BssType == BSS_INFRA)
                    pBss->InfrastructureMode = Ndis802_11Infrastructure;
                else
                    pBss->InfrastructureMode = Ndis802_11IBSS;

                NdisMoveMemory(pBss->SupportedRates, pAd->ScanTab.BssEntry[i].SupRate, pAd->ScanTab.BssEntry[i].SupRateLen);
                NdisMoveMemory(pBss->SupportedRates + pAd->ScanTab.BssEntry[i].SupRateLen,
                               pAd->ScanTab.BssEntry[i].ExtRate,
                               pAd->ScanTab.BssEntry[i].ExtRateLen);

                if (pAd->ScanTab.BssEntry[i].VarIELen == 0)
                {
                    pBss->IELength = sizeof(NDIS_802_11_FIXED_IEs);
                    NdisMoveMemory(pBss->IEs, &pAd->ScanTab.BssEntry[i].FixIEs, sizeof(NDIS_802_11_FIXED_IEs));
                    pPtr = pPtr + sizeof(NDIS_WLAN_BSSID_EX) - 1 + sizeof(NDIS_802_11_FIXED_IEs);
                }
                else
                {
                    pBss->IELength = (ULONG)(sizeof(NDIS_802_11_FIXED_IEs) + pAd->ScanTab.BssEntry[i].VarIELen);
                    pPtr = pPtr + sizeof(NDIS_WLAN_BSSID_EX) - 1 + sizeof(NDIS_802_11_FIXED_IEs);
                    NdisMoveMemory(pBss->IEs, &pAd->ScanTab.BssEntry[i].FixIEs, sizeof(NDIS_802_11_FIXED_IEs));
                    NdisMoveMemory(pBss->IEs + sizeof(NDIS_802_11_FIXED_IEs), pAd->ScanTab.BssEntry[i].VarIEs, pAd->ScanTab.BssEntry[i].VarIELen);
                    pPtr += pAd->ScanTab.BssEntry[i].VarIELen;
                }
                pBss->Length = (ULONG)(sizeof(NDIS_WLAN_BSSID_EX) - 1 + sizeof(NDIS_802_11_FIXED_IEs) + pAd->ScanTab.BssEntry[i].VarIELen + Padding);

#if WIRELESS_EXT < 17
                if ((BssLen + pBss->Length) < wrq->u.data.length)
                BssLen += pBss->Length;
                else
                {
                    pBssidList->NumberOfItems = i;
                    break;
                }
#else
                BssLen += pBss->Length;
#endif
            }

#if WIRELESS_EXT < 17
            wrq->u.data.length = BssLen;
#else
            if (BssLen > wrq->u.data.length)
            {
		   os_free_mem(NULL, pBssidList);
                return -E2BIG;
            }
            else
                wrq->u.data.length = BssLen;
#endif
            Status = copy_to_user(wrq->u.data.pointer, pBssidList, BssLen);
		   os_free_mem(NULL, pBssidList);
            break;
        case OID_802_3_CURRENT_ADDRESS:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			pApCliEntry=&pAd->ApCfg.ApCliTab[ifIndex];
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			pMacEntry = &pAd->MacTab.Content[pAd->ApCfg.ApCliTab[ifIndex].MacTabWCID];

            		wrq->u.data.length = MAC_ADDR_LEN;
            		Status = copy_to_user(wrq->u.data.pointer, pApCliEntry->wdev.if_addr, wrq->u.data.length);
            break;

        case OID_802_11_BSSID:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			pApCliEntry=&pAd->ApCfg.ApCliTab[ifIndex];
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

            if (INFRA_ON(pAd) || ADHOC_ON(pAd))
            {
                Status = copy_to_user(wrq->u.data.pointer, pApCliEntry->CfgApCliBssid, sizeof(NDIS_802_11_MAC_ADDRESS));


                DBGPRINT(RT_DEBUG_INFO, ("IOCTL::SIOCGIWAP(=%02x:%02x:%02x:%02x:%02x:%02x)\n",
                       pApCliEntry->CfgApCliBssid[0],pApCliEntry->CfgApCliBssid[1],pApCliEntry->CfgApCliBssid[2],
                        pApCliEntry->CfgApCliBssid[3],pApCliEntry->CfgApCliBssid[4],pApCliEntry->CfgApCliBssid[5]));

            }
            else
            {
                DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_BSSID(=EMPTY)\n"));
                Status = -ENOTCONN;
            }
            break;
        case OID_802_11_SSID:

			if (pObj->ioctl_if_type != INT_APCLI)
				return FALSE;

			ifIndex = pObj->ioctl_if;
			pApCliEntry=&pAd->ApCfg.ApCliTab[ifIndex];
			apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

			if (!apcliEn)
				return FALSE;

			NdisZeroMemory(&Ssid, sizeof(NDIS_802_11_SSID));
			NdisZeroMemory(Ssid.Ssid, MAX_LEN_OF_SSID);
            		Ssid.SsidLength = pApCliEntry->CfgSsidLen;
			NdisMoveMemory(Ssid.Ssid, pApCliEntry->CfgSsid,Ssid.SsidLength);
            		wrq->u.data.length = sizeof(NDIS_802_11_SSID);
            		Status = copy_to_user(wrq->u.data.pointer, &Ssid, wrq->u.data.length);
           		 DBGPRINT(RT_DEBUG_TRACE, ("Query Apcli::OID_802_11_SSID (Len=%d, ssid=%s)\n", Ssid.SsidLength,Ssid.Ssid));
            break;
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/

#ifdef P2P_SUPPORT
		case OID_802_11_P2P_Connected_MAC:
			wrq->u.data.length = MAC_ADDR_LEN;
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.ConnectingMAC, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_Connected_MAC\n"));
			break;

		case OID_802_11_P2P_MODE:
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.Rule, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%d, Rule=%s)\n", sizeof(char),pAd->P2pCfg.Rule));
			break;

		case OID_802_11_P2P_DEVICE_NAME:
			wrq->u.data.length = pAd->P2pCfg.DeviceNameLen;
			Status = copy_to_user(wrq->u.data.pointer, pAd->P2pCfg.DeviceName, pAd->P2pCfg.DeviceNameLen);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_DEVICE_NAME (Len=%d, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_LISTEN_CHANNEL:
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.ListenChannel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_LISTEN_CHANNEL (Len=%d, Listen_Ch=%d)\n", sizeof(char),pAd->P2pCfg.ListenChannel));
			break;

		case OID_802_11_P2P_OPERATION_CHANNEL:
			wrq->u.data.length = sizeof(char);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->P2pCfg.GroupChannel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_OPERATION_CHANNEL (Len=%d, Op_Ch=%d)\n", sizeof(char),pAd->P2pCfg.GroupOpChannel));
			break;


		case OID_802_11_P2P_MAC_ADDR:
			wrq->u.data.length = 6;
			Status = copy_to_user(wrq->u.data.pointer, pAd->P2pCfg.Bssid, wrq->u.data.length);
			/*DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MAC_ADDR (Len=%d, Rule=%s)\n", sizeof(char),pAd->P2pCfg.GroupOpChannel)); */
			break;

		case OID_802_11_P2P_CTRL_STATUS:
			wrq->u.data.length = 24;
			pP2PCtrl = &pAd->P2pCfg;
			NdisZeroMemory(tmp, 24);
			sprintf(tmp,"%s",decodeCtrlState(pP2PCtrl->CtrlCurrentState));
			Status = copy_to_user(wrq->u.data.pointer, tmp, 24);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%d, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_DISC_STATUS:
			wrq->u.data.length = 24;
			pP2PCtrl = &pAd->P2pCfg;
			NdisZeroMemory(tmp, 24);
			sprintf(tmp,"%s",decodeDiscoveryState(pP2PCtrl->DiscCurrentState));
			Status = copy_to_user(wrq->u.data.pointer, tmp, 24);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%d, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_GOFORM_STATUS:
			wrq->u.data.length = 24;
			pP2PCtrl = &pAd->P2pCfg;
			NdisZeroMemory(tmp, 24);
			sprintf(tmp,"%s",decodeGroupFormationState(pP2PCtrl->GoFormCurrentState));
			Status = copy_to_user(wrq->u.data.pointer, tmp, 24);
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_MODE (Len=%d, DeviceName=%s)\n", pAd->P2pCfg.DeviceNameLen,pAd->P2pCfg.DeviceName));
			break;

		case OID_802_11_P2P_SCAN_LIST:

			os_alloc_mem(NULL, (UCHAR **)&pUI_table, sizeof(RT_P2P_UI_TABLE));
			pP2pTable = &pAd->P2pTable;
			/*NdisZeroMemory(&UI_table, sizeof(UI_table));*/
			/*pUI_table = &UI_table;*/
			NdisZeroMemory(pUI_table, sizeof(RT_P2P_UI_TABLE));
			pUI_table->ClientNumber = pAd->P2pTable.ClientNumber;
			for (i=0; i < pAd->P2pTable.ClientNumber; i++)
			{
				pPAdCli = &pP2pTable->Client[i];
				pUICli = &pUI_table->Client[i];
 				NdisMoveMemory(pUICli, pPAdCli, sizeof(RT_P2P_CLIENT_ENTRY));
			}
			DBGPRINT(RT_DEBUG_OFF, ("Query::OID_802_11_P2P_SCAN_LIST\n"));
			Status = copy_to_user(wrq->u.data.pointer, pUI_table, sizeof(RT_P2P_UI_TABLE));
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_P2P_SCAN_LIST (Len=%d, Rule=%s)\n", sizeof(char),pAd->P2pCfg.GroupOpChannel));
			os_free_mem(NULL, pUI_table);
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
#endif /* P2P_SUPPORT */

		case RT_OID_VERSION_INFO:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_VERSION_INFO \n"));
			wrq->u.data.length = (UINT16)strlen(AP_DRIVER_VERSION);
			snprintf(&driverVersion[0], sizeof(driverVersion), "%s", AP_DRIVER_VERSION);
			driverVersion[wrq->u.data.length] = '\0';
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
				/* End of if */

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
			/* End of if */

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

#ifdef WSC_NFC_SUPPORT
		case RT_OID_NFC_STATUS:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_NFC_STATUS (=%d)\n", pAd->ApCfg.MBSSID[apidx].WscControl.NfcStatus));
			wrq->u.data.length = sizeof(UCHAR);
			if (copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].WscControl.NfcStatus, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
#endif /* WSC_NFC_SUPPORT */

#endif /* WSC_AP_SUPPORT */
#ifdef LLTD_SUPPORT
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

		case OID_802_DOT1X_QUERY_STA_AID:
			RTMPIoctlQueryStaAid(pAd, wrq);
            break;

#endif /* DOT1X_SUPPORT */

		case RT_OID_802_11_MAC_ADDRESS:
                        wrq->u.data.length = MAC_ADDR_LEN;
                        Status = copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].wdev.bssid, wrq->u.data.length);
#ifdef RELEASE_EXCLUDE
                        DBGPRINT(RT_DEBUG_INFO, ("Query::RT_OID_802_11_MAC_ADDRESS \n"));
#endif /* RELEASE_EXCLUDE */
			break;

#ifdef SNMP_SUPPORT
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
		{
			ULONG ulInfo;
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_PRIVACYOPTIONIMPLEMENTED \n"));
			ulInfo = 1; /* 1 is support wep else 2 is not support. */
			wrq->u.data.length = sizeof(ulInfo);
			Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
			break;
		}
		case RT_OID_802_11_POWERMANAGEMENTMODE:
		{
			ULONG ulInfo;
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_POWERMANAGEMENTMODE \n"));
			ulInfo = 1; /* 1 is power active else 2 is power save. */
			wrq->u.data.length = sizeof(ulInfo);
			Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
			break;
		}
		case OID_802_11_WEPDEFAULTKEYVALUE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WEPDEFAULTKEYVALUE \n"));
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
			ShortRetryLimit = AsicGetRetryLimit(pAd, TX_RTY_CFG_RTY_LIMIT_SHORT);
			DBGPRINT(RT_DEBUG_TRACE, ("ShortRetryLimit =%ld\n", ShortRetryLimit));
			Status = copy_to_user(wrq->u.data.pointer, &ShortRetryLimit, wrq->u.data.length);
			break;

		case OID_802_11_LONGRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_LONGRETRYLIMIT \n"));
			wrq->u.data.length = sizeof(ULONG);
			LongRetryLimit = AsicGetRetryLimit(pAd, TX_RTY_CFG_RTY_LIMIT_LONG);
			DBGPRINT(RT_DEBUG_TRACE, ("LongRetryLimit =%ld\n", LongRetryLimit));
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
			/*NICUpdateRawCounters(pAd);*/

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
			os_free_mem(NULL, pStatistics);
            }
            else
            {
                DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_STATISTICS(mem alloc failed)\n"));
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
			Status = copy_to_user(wrq->u.data.pointer, pMbssStat, wrq->u.data.length);
			os_free_mem(pAd, pMbssStat);
		}
		break;

#ifdef DOT11_N_SUPPORT
#ifdef TXBF_SUPPORT
		case RT_OID_802_11_QUERY_TXBF_TABLE:
			if (!pAd->chipCap.FlgHwTxBfCap)
				Status = -EFAULT;
			else
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
			break;
#endif /* TXBF_SUPPORT */
#endif /* DOT11_N_SUPPORT */

#ifdef MESH_SUPPORT
		case OID_802_11_MESH_DEVICENAME:
			if (pAd->MeshTab.wdev.if_dev)
			{
				wrq->u.data.length = strlen(RtmpOsGetNetDevName(pAd->MeshTab.wdev.if_dev)) + 1;
				Status = copy_to_user(wrq->u.data.pointer, RtmpOsGetNetDevName(pAd->MeshTab.wdev.if_dev), strlen(RtmpOsGetNetDevName(pAd->MeshTab.wdev.if_dev)));
				DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_MESH_DEVICENAME (Len=%d, Name=%s)\n", wrq->u.data.length, RtmpOsGetNetDevName(pAd->MeshTab.wdev.if_dev)));
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
					PMESH_LINK_ENTRY pMeshEntry = &pAd->MeshTab.MeshLink[i].Entry;
					MAC_TABLE_ENTRY *mac_entry = &pAd->MacTab.Content[pEntry->MacTabMatchWCID];
					COPY_MAC_ADDR(MeshLinkInfo.Entry[i].PeerMacAddr, pMeshEntry->PeerMacAddr);
					MeshLinkInfo.Entry[i].LinkType = pMeshEntry->LinkType;
					MeshLinkInfo.Entry[i].Status = PeerLinkValidCheck(pAd, i);
					MeshLinkInfo.Entry[i].Rssi =
						RTMPMaxRssi(pAd, mac_entry->RssiSample.AvgRssi[0],
							mac_entry->RssiSample.AvgRssi[1],
							mac_entry->RssiSample.AvgRssi[2]);
					MeshLinkInfo.Entry[i].CurTxRate = mac_entry->HTPhyMode;
				}

				wrq->u.data.length = sizeof(MESH_LINK_INFO);
				Status = copy_to_user(wrq->u.data.pointer, &MeshLinkInfo, wrq->u.data.length);
			}
			break;

		case OID_802_11_MESH_LIST:
			{
				PMESH_NEIGHBOR_INFO pMeshNeighborInfo;
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
						strcpy((RTMP_STRING *)pMeshNeighborInfo->Entry[idx].HostName, (RTMP_STRING *)pEntry->HostName);
						COPY_MAC_ADDR(pMeshNeighborInfo->Entry[idx].MacAddr, pEntry->PeerMac);
						strcpy((RTMP_STRING *)pMeshNeighborInfo->Entry[idx].MeshId, (RTMP_STRING *)pEntry->MeshId);
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
					os_free_mem(NULL, pMeshNeighborInfo);
			}
			break;

		case OID_802_11_MESH_ROUTE_LIST:
			{
				PRT_MESH_ROUTE_TABLE rt_table;
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
			wrq->u.data.length = strlen((RTMP_STRING *)pAd->MeshTab.HostName) + 1;
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
								   pAd->SharedKey[pObj->ioctl_if][pMbss->wdev.DefaultKeyId].TxTsc,
								   LEN_WAPI_TSC);
				}
				else
				{
					INT	m_wcid;

					GET_GroupKey_WCID(pAd, m_wcid, apidx);
					RTMPGetWapiTxTscFromAsic(pAd, m_wcid, wapi_mkey.m_tx_iv);
				}
				wapi_mkey.key_id = pMbss->wdev.DefaultKeyId;
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
				STA_TR_ENTRY *tr_entry;
				PAPCLI_STRUCT pApCliEntry;

				if (pObj->ioctl_if_type != INT_APCLI)
				{
					Status = -EOPNOTSUPP;
					break;
				}
				else
				{
					APCLI_MR_APIDX_SANITY_CHECK(ApCliIdx);
					pApCliEntry = &pAd->ApCfg.ApCliTab[ApCliIdx];
					pEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];
					tr_entry = &pAd->MacTab.tr_entry[pApCliEntry->MacTabWCID];
					if (!IS_ENTRY_APCLI(pEntry))
					{
						Status = -EOPNOTSUPP;
						break;
					}

					if ((pAd->ApCfg.ApCliTab[ApCliIdx].Valid == TRUE)
						&& (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED))
						MediaState = NdisMediaStateConnected;
					else
						MediaState = NdisMediaStateDisconnected;

					wrq->u.data.length = sizeof(NDIS_MEDIA_STATE);
					Status = copy_to_user(wrq->u.data.pointer, &MediaState, wrq->u.data.length);
				}
			}
			break;
#endif /* APCLI_SUPPORT */

#ifdef RTMP_RBUS_SUPPORT
		case RT_OID_802_11_SNR_0:
			if (wdev && wdev->LastSNR0 > 0) {
				ULONG ulInfo;
				ulInfo = ConvertToSnr(pAd, wdev->LastSNR0);
				wrq->u.data.length = sizeof(ulInfo);
				Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_SNR_0(0x=%lx)\n", ulInfo));
			}
			else
				Status = -EFAULT;
			break;
		case RT_OID_802_11_SNR_1:
			if (wdev && (pAd->Antenna.field.RxPath > 1) && (wdev->LastSNR1 > 0))
			{
				ULONG ulInfo;
				ulInfo = ConvertToSnr(pAd, wdev->LastSNR1);
				wrq->u.data.length = sizeof(ulInfo);
				Status = copy_to_user(wrq->u.data.pointer, &ulInfo,	wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_1(%lx, LastSNR1=%d)\n",ulInfo, wdev->LastSNR1));
			}
			else
				Status = -EFAULT;
			DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_1, Status=%d\n",Status));
			break;
#ifdef DOT11N_SS3_SUPPORT
		case RT_OID_802_11_SNR_2:
			if (wdev && (pAd->Antenna.field.RxPath > 2) && (wdev->LastSNR2 > 0))
			{
				ULONG ulInfo;
				ulInfo = ConvertToSnr(pAd, wdev->LastSNR2);
				wrq->u.data.length = sizeof(ulInfo);
				Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
				DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_1(%lx, LastSNR2=%d)\n",ulInfo, wdev->LastSNR2));
			}
			else
				Status = -EFAULT;
			DBGPRINT(RT_DEBUG_TRACE,("Query::RT_OID_802_11_SNR_1, Status=%d\n",Status));
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

		case OID_802_11_ACL_LIST:
			if (wrq->u.data.length < sizeof(RT_802_11_ACL))
			{
				Status = -EINVAL;
			}
			else
			{
				Status = copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, sizeof(RT_802_11_ACL));
			}
			break;
#ifdef CONFIG_HOTSPOT
#ifdef CONFIG_DOT11V_WNM
		case OID_802_11_WNM_IPV4_PROXY_ARP_LIST:
			{
				BSS_STRUCT *pMbss;
				PUCHAR pProxyARPTable;
				UINT32 ARPTableLen;
				pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];
				ARPTableLen = IPv4ProxyARPTableLen(pAd, pMbss);
				os_alloc_mem(NULL, &pProxyARPTable, ARPTableLen);
				GetIPv4ProxyARPTable(pAd, pMbss, &pProxyARPTable);
				wrq->u.data.length = ARPTableLen;
				Status = copy_to_user(wrq->u.data.pointer, pProxyARPTable, ARPTableLen);
				os_free_mem(NULL, pProxyARPTable);
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WNM_PROXY_ARP_LIST\n"));
			break;
		case OID_802_11_WNM_IPV6_PROXY_ARP_LIST:
			{
				BSS_STRUCT *pMbss;
				PUCHAR pProxyARPTable;
				UINT32 ARPTableLen;
				pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];
				ARPTableLen = IPv6ProxyARPTableLen(pAd, pMbss);
				os_alloc_mem(NULL, &pProxyARPTable, ARPTableLen);
				GetIPv6ProxyARPTable(pAd, pMbss, &pProxyARPTable);
				wrq->u.data.length = ARPTableLen;
				Status = copy_to_user(wrq->u.data.pointer, pProxyARPTable, ARPTableLen);
				os_free_mem(NULL, pProxyARPTable);
			}
			break;
#endif
		case OID_802_11_SECURITY_TYPE:
			{
				BSS_STRUCT *pMbss;
				PUCHAR pType;
				struct security_type *SecurityType;
				//DBGPRINT(RT_DEBUG_TRACE, ("Query:OID_802_11_SECURITY_TYPE\n"));
				DBGPRINT(RT_DEBUG_OFF, ("Query:OID_802_11_SECURITY_TYPE\n"));
				os_alloc_mem(NULL, &pType, sizeof(*SecurityType));
				SecurityType = (struct security_type *)pType;
				pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];
				SecurityType->ifindex = pObj->ioctl_if;
				SecurityType->auth_mode = pMbss->wdev.AuthMode; //pMbss->AuthMode;
				SecurityType->encryp_type = pMbss->wdev.WepStatus; //pMbss->WepStatus;
				wrq->u.data.length = sizeof(*SecurityType);
				Status = copy_to_user(wrq->u.data.pointer, pType, sizeof(*SecurityType));
				os_free_mem(NULL, pType);
			}
			break;
		case OID_802_11_HS_BSSID:
			{
				BSS_STRUCT *pMbss;
				pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];
				wrq->u.data.length = 6;
				Status = copy_to_user(wrq->u.data.pointer, pMbss->wdev.bssid, 6);
			}
			break;
#ifdef CONFIG_HOTSPOT_R2
		case OID_802_11_HS_OSU_SSID:
			{
				wrq->u.data.length = pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen; //+2;
				Status = copy_to_user(wrq->u.data.pointer, pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid, pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen);
			}	
			break;	
//JERRY for hs test
#endif /* CONFIG_HOTSPOT_R2 */
#endif
#ifdef RLT_MAC
#ifdef CONFIG_WIFI_TEST
		case OID_WIFI_TEST_BBP:

			break;
		case OID_WIFI_TEST_BBP32:
			{
				UINT32 Index;
				UINT32 j = 0;
				struct bbp32_info *Info;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, wrq->u.data.length);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("Memory is not available\n"));
					break;
				}

				Status = copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length);
				Info = (struct bbp32_info *)buf;

				DBGPRINT(RT_DEBUG_OFF, ("Info->bbp_start = %x\n", Info->bbp_start));
				DBGPRINT(RT_DEBUG_OFF, ("Info->bbp_end = %x\n", Info->bbp_end));


				for (Index = Info->bbp_start; Index <= Info->bbp_end; Index += 4)
				{
					UINT32 Value;
					RTMP_BBP_IO_READ32(pAd, Index + pAd->chipCap.BBPMemMapOffset, &Value);
					DBGPRINT(RT_DEBUG_OFF, ("Offset = %x\n", Index + pAd->chipCap.BBPMemMapOffset));
					DBGPRINT(RT_DEBUG_OFF, ("Value = %x\n", Value));
					NdisMoveMemory(Info->bbp_value + j, &Value, 4);
					j++;
				}

				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}


			break;
		case OID_WIFI_TEST_RF:

			break;
#ifdef RLT_RF
		case OID_WIFI_TEST_RF_BANK:
			{
				UINT16 Index;
				UINT16 j = 0;
				struct rf_bank_info *Info;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, wrq->u.data.length);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("memory is not available\n"));
					break;
				}

				Status = copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length);
				Info = (struct rf_bank_info *)buf;

				DBGPRINT(RT_DEBUG_OFF, ("Info->rf_bank = %x\n", Info->rf_bank));
				DBGPRINT(RT_DEBUG_OFF, ("Info->rf_start = %x\n",  Info->rf_start));
				DBGPRINT(RT_DEBUG_OFF, ("Info->rf_end = %x\n",  Info->rf_end));


				for (Index = Info->rf_start; Index <= Info->rf_end; Index ++)
				{
					UINT8 Value;
					rlt_rf_read(pAd, Info->rf_bank, Index, &Value);
					DBGPRINT(RT_DEBUG_OFF, ("Offset = %x\n", Index));
					DBGPRINT(RT_DEBUG_OFF, ("Value = %x\n", Value));
					NdisMoveMemory(Info->rf_value + j, &Value, 1);
					j++;
				}

				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
#endif /* RLT_RF */
#ifdef MT_RF
		case OID_WIFI_TEST_RF_INDEX:
			{
				UINT16 Index;
				UINT16 j = 0;
				struct rf_index_info *Info;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, wrq->u.data.length);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("memory is not available\n"));
					break;
				}

				Status = copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length);
				Info = (struct rf_bank_info *)buf;

				DBGPRINT(RT_DEBUG_OFF, ("Info->rf_index = %x\n", Info->rf_index));
				DBGPRINT(RT_DEBUG_OFF, ("Info->rf_start = %x\n",  Info->rf_start));
				DBGPRINT(RT_DEBUG_OFF, ("Info->rf_end = %x\n",  Info->rf_end));

				for (Index = Info->rf_start; Index <= Info->rf_end; Index += 4)
				{
					UINT32 Value;
					mt76x2_rf_read(pAd, Info->rf_index, Index, &Value);
					DBGPRINT(RT_DEBUG_OFF, ("Offset = %x\n", Index));
					DBGPRINT(RT_DEBUG_OFF, ("Value = %x\n", Value));
					NdisMoveMemory(Info->rf_value + j, &Value, 4);
					j++;
				}

				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
#endif /* MT_RF */
		case OID_WIFI_TEST_MEM_MAP_INFO:
			{
				UINT16 Index;
				UINT16 j = 0;
				struct mem_map_info *Info;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, wrq->u.data.length);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("memory is not available\n"));
					break;
				}

				Status = copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length);
				Info = (struct mem_map_info *)buf;

				DBGPRINT(RT_DEBUG_OFF, ("Info->base = %x\n", Info->base));
				DBGPRINT(RT_DEBUG_OFF, ("Info->mem_map_start = %x\n"
						, Info->mem_map_start));
				DBGPRINT(RT_DEBUG_OFF, ("Info->mem_map_end = %x\n"
						, Info->mem_map_end));


				for (Index = Info->mem_map_start; Index <= Info->mem_map_end; Index += 4)
				{
					UINT32 Value;
					read_reg(pAd, Info->base, Index, &Value);
					NdisMoveMemory(Info->mem_map_value + j, &Value, 4);
					j++;
				}

				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
		case OID_WIFI_TEST_E2P:
			{
				UINT16 Index;
				UINT16 j = 0;
				struct e2p_info *Info;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, wrq->u.data.length);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("Memory is not available\n"));
					break;
				}

				Status = copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length);
				Info = (struct e2p_info *)buf;

				DBGPRINT(RT_DEBUG_OFF, ("Info->e2p_start = %x\n", Info->e2p_start));
				DBGPRINT(RT_DEBUG_OFF, ("Info->e2p_end = %x\n",  Info->e2p_end));


				for (Index = Info->e2p_start; Index <= Info->e2p_end; Index += 2)
				{
					UINT16 Value;
					RT28xx_EEPROM_READ16(pAd, Index, Value);
					NdisMoveMemory(Info->e2p_value + j, &Value, 2);
					j++;
				}

				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
		case OID_WIFI_TEST_MAC:
			{
				UINT32 Index;
				UINT32 j = 0;
				struct mac_info *Info;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, wrq->u.data.length);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("memory is not available\n"));
					break;
				}

				Status = copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length);
				Info = (struct mac_info *)buf;

				DBGPRINT(RT_DEBUG_OFF, ("Info->mac_start = %x\n", Info->mac_start));
				DBGPRINT(RT_DEBUG_OFF, ("Info->mac_end = %x\n",  Info->mac_end));


				for (Index = Info->mac_start; Index <= Info->mac_end; Index += 4)
				{
					UINT32 Value;
					RTMP_IO_READ32(pAd, Index + pAd->chipCap.MacMemMapOffset, &Value);
					DBGPRINT(RT_DEBUG_OFF, ("Offset = %x\n",
							Index + pAd->chipCap.MacMemMapOffset));
					DBGPRINT(RT_DEBUG_OFF, ("Value = %x\n", Value));
					NdisMoveMemory(Info->mac_value + j, &Value, 4);
					j++;
				}

				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
		case OID_WIFI_TEST_BBP_NUM:
			{
				struct bbp32_info Info;
				Info.bbp_start = pAd->chipCap.BBPStart;
				Info.bbp_end = pAd->chipCap.BBPEnd;
				wrq->u.data.length = sizeof(Info);
				Status = copy_to_user(wrq->u.data.pointer, &Info, wrq->u.data.length);
			}
			break;
		case OID_WIFI_TEST_RF_NUM:

			break;

#ifdef RLT_RF
		case OID_WIFI_TEST_RF_BANK_OFFSET:
			{
				struct rf_bank_info *Info;
				struct RF_BANK_OFFSET *Offset;
				UINT8 Index;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, sizeof(*Info) * pAd->chipCap.RFBankNum);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("Memory is not available\n"));
					break;
				}

				Info = (struct rf_bank_info *)buf;
				Offset = pAd->chipCap.RFBankOffset;

				DBGPRINT(RT_DEBUG_OFF, ("pAd->chipCap.RFBankNum = %d\n", pAd->chipCap.RFBankNum));

				for (Index = 0; Index < pAd->chipCap.RFBankNum; Index++)
				{
					Info->rf_bank = Offset->RFBankIndex;
					Info->rf_start = Offset->RFStart;
					Info->rf_end =Offset->RFEnd;
					DBGPRINT(RT_DEBUG_OFF, ("Info->rf_bank = %d\n", Info->rf_bank));
					DBGPRINT(RT_DEBUG_OFF, ("Info->rf_start = %x\n", Info->rf_start));
					DBGPRINT(RT_DEBUG_OFF, ("Info->rf_end = %x\n", Info->rf_end));
					Info++;
					Offset++;
				}

				wrq->u.data.length = sizeof(*Info) * pAd->chipCap.RFBankNum;
				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
#endif /* RLT_RF */

#ifdef MT_RF
		case OID_WIFI_TEST_RF_INDEX_OFFSET:
			{
				struct rf_index_info *Info;
				struct RF_INDEX_OFFSET *Offset;
				UINT8 Index;
				char *buf;

				os_alloc_mem(pAd, (UCHAR **)&buf, sizeof(*Info) * pAd->chipCap.RFIndexNum);

				if (!buf)
				{
					Status = -EINVAL;
					DBGPRINT(RT_DEBUG_ERROR, ("Memory is not available\n"));
					break;
				}

				Info = (struct rf_index_info *)buf;
				Offset = pAd->chipCap.RFIndexOffset;

				DBGPRINT(RT_DEBUG_OFF, ("pAd->chipCap.RFIndexNum = %d\n", pAd->chipCap.RFIndexNum));

				for (Index = 0; Index < pAd->chipCap.RFIndexNum; Index++)
				{
					Info->rf_index = Offset->RFIndex;
					Info->rf_start = Offset->RFStart;
					Info->rf_end =Offset->RFEnd;
					DBGPRINT(RT_DEBUG_OFF, ("Info->rf_index = %d\n", Info->rf_index));
					DBGPRINT(RT_DEBUG_OFF, ("Info->rf_start = %x\n", Info->rf_start));
					DBGPRINT(RT_DEBUG_OFF, ("Info->rf_end = %x\n", Info->rf_end));
					Info++;
					Offset++;
				}

				wrq->u.data.length = sizeof(*Info) * pAd->chipCap.RFIndexNum;
				Status = copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length);

				os_free_mem(NULL, buf);
			}
			break;
#endif
		case OID_WIFI_TEST_MAC_NUM:
			{
				struct mac_info Info;
				Info.mac_start = pAd->chipCap.MacStart;
				Info.mac_end = pAd->chipCap.MacEnd;
				wrq->u.data.length = sizeof(Info);
				Status = copy_to_user(wrq->u.data.pointer, &Info, wrq->u.data.length);
			}
			break;
		case OID_WIFI_TEST_E2P_NUM:
			{
				struct e2p_info Info;
				Info.e2p_start = pAd->chipCap.E2PStart;
				Info.e2p_end = pAd->chipCap.E2PEnd;
				wrq->u.data.length = sizeof(Info);
				Status = copy_to_user(wrq->u.data.pointer, &Info, wrq->u.data.length);
			}
			break;
		case OID_WIFI_TEST_MEM_MAP_NUM:
			{
				struct mem_map_info Info;
				Info.mem_map_start = pAd->chipCap.MemMapStart;
				Info.mem_map_end = pAd->chipCap.MemMapEnd;
				wrq->u.data.length = sizeof(Info);
				Status = copy_to_user(wrq->u.data.pointer, &Info, wrq->u.data.length);
			}
			break;
		case OID_WIFI_TEST_PHY_MODE:
			{
				struct phy_mode_info info;
				info.data_phy = pAd->fpga_ctl.rx_data_phy;
				info.data_bw = pAd->fpga_ctl.rx_data_bw;
				info.data_ldpc = pAd->fpga_ctl.rx_data_ldpc;
				info.data_mcs = pAd->fpga_ctl.rx_data_mcs;
				info.data_gi = pAd->fpga_ctl.rx_data_gi;
				info.data_stbc =  pAd->fpga_ctl.rx_data_stbc;
				wrq->u.data.length = sizeof(info);
				Status = copy_to_user(wrq->u.data.pointer, &info, wrq->u.data.length);
			}
			break;
#endif
#endif
   		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::unknown IOCTL's subcmd = 0x%08x, apidx=%d\n", cmd, apidx));
			Status = -EOPNOTSUPP;
			break;
    }

	return Status;
}

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

#ifdef EXT_BUILD_CHANNEL_LIST
	/* reset temp table status */
	pAd->CommonCfg.pChDesp = NULL;
	pAd->CommonCfg.DfsType = MAX_RD_REGION;
#endif /* EXT_BUILD_CHANNEL_LIST */

	if(strlen(arg) == 2)
	{
		NdisMoveMemory(pAd->CommonCfg.CountryCode, arg, 2);
		pAd->CommonCfg.bCountryFlag = TRUE;
	}
	else
	{
		NdisZeroMemory(pAd->CommonCfg.CountryCode,
				sizeof(pAd->CommonCfg.CountryCode));
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
		if (pAd->CommonCfg.CountryRegion & 0x80)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::parameter of CountryRegion in eeprom is programmed \n"));
			success = FALSE;
		}
		else
		{
			success = FALSE;
			if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode))
			{
				if (allCountry[index].SupportGBand == TRUE)
				{
					pAd->CommonCfg.CountryRegion = (UCHAR) allCountry[index].RegDomainNum11G;
					success = TRUE;
				}
				else
				{
					DBGPRINT(RT_DEBUG_TRACE, ("The Country are not Support G Band Channel\n"));
				}
			}

			if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			{
				if (allCountry[index].SupportABand == TRUE)
				{
					pAd->CommonCfg.CountryRegionForABand = (UCHAR) allCountry[index].RegDomainNum11A;
					success = TRUE;
				}
				else
				{
					DBGPRINT(RT_DEBUG_TRACE, ("The Country are not Support A Band Channel\n"));
				}
			}
		}
	}

	if (success == TRUE)
	{
		NdisZeroMemory(pAd->CommonCfg.CountryCode, sizeof(pAd->CommonCfg.CountryCode));
		NdisMoveMemory(pAd->CommonCfg.CountryCode, allCountry[index].IsoName, 2);
		pAd->CommonCfg.CountryCode[2] = ' ';
		/* After Set ChGeography need invoke SSID change procedural again for Beacon update. */
		/* it's no longer necessary since APStartUp will rebuild channel again. */
		/*BuildChannelList(pAd); */

		pAd->CommonCfg.bCountryFlag = TRUE;

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
INT	Set_AP_SSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT success = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BSS_STRUCT *mbss;

	if(((pObj->ioctl_if < HW_BEACON_MAX_NUM)) && (strlen(arg) <= MAX_LEN_OF_SSID))
	{
		mbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];

		NdisZeroMemory(mbss->Ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(mbss->Ssid, arg, strlen(arg));
		mbss->SsidLen = (UCHAR)strlen(arg);
		success = TRUE;

#ifdef P2P_SUPPORT
		if (pObj->ioctl_if_type == INT_P2P)
		{
			if (P2P_GO_ON(pAd))
			{
				P2P_GoStop(pAd);
				P2P_GoStartUp(pAd, MAIN_MBSSID);
				DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_SSID_Proc::(Len=%d,Ssid=%s)\n", pObj->ioctl_if,
					mbss->SsidLen, mbss->Ssid));
			}
		}
		else
#endif /* P2P_SUPPORT */
		{
			/* If in detection mode, need to stop detect first. */
			if (pAd->CommonCfg.bIEEE80211H == FALSE)
			{
				APStop(pAd);
				APStartUp(pAd);
			}
			else
			{
				/* each mode has different restart method */
				if (pAd->Dot11_H.RDMode == RD_SILENCE_MODE)
				{
					APStop(pAd);
					APStartUp(pAd);
				}
				else if (pAd->Dot11_H.RDMode == RD_SWITCHING_MODE)
				{
				}
				else if (pAd->Dot11_H.RDMode == RD_NORMAL_MODE)
				{
					APStop(pAd);
					APStartUp(pAd);
				}
			}

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_SSID_Proc::(Len=%d,Ssid=%s)\n", pObj->ioctl_if,
				mbss->SsidLen, mbss->Ssid));
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
INT Set_TxRate_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	NdisZeroMemory(pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRates, MAX_LEN_OF_SUPPORTED_RATES);

	pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRatesIndex = (UCHAR)simple_strtol(arg, 0, 10);
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
INT	Set_BeaconPeriod_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_DtimPeriod_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	UCHAR DtimPeriod;
	INT   success = FALSE;

	DtimPeriod = (UCHAR) simple_strtol(arg, 0, 10);
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
INT Set_RfRead_Proc(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  RTMP_STRING *arg)
{
	int i;
	UCHAR Value;

	for (i = 0; i < 32; i++)
	{
		RT30xxReadRFRegister(pAdapter, i, &Value);
		DBGPRINT(RT_DEBUG_OFF, ("%02x ", Value));
		if (((i + 1) % 4) == 0)
			DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}
	return TRUE;
}

INT Set_RfWrite_Proc(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  RTMP_STRING *arg)
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
INT	Set_OLBCDetection_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_WmmCapable_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	BOOLEAN	bWmmCapable;
	POS_COOKIE	pObj= (POS_COOKIE)pAd->OS_Cookie;

	bWmmCapable = (BOOLEAN)simple_strtol(arg, 0, 10);

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


INT	Set_AP_MaxStaNum_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_IdleTimeout_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_NoForwarding_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_NoForwardingBTNSSID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_HideSSID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	BOOLEAN bHideSsid;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	bHideSsid = (BOOLEAN)simple_strtol(arg, 0, 10);

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
INT	Set_VLANID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev;


	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
	wdev->VLAN_VID = (USHORT)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLANID_Proc::(VLAN_VID=%d)\n",
				pObj->ioctl_if, wdev->VLAN_VID));

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
	struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;

	wdev->VLAN_Priority = (USHORT)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLANPriority_Proc::(VLAN_Priority=%d)\n", pObj->ioctl_if, wdev->VLAN_Priority));

	return TRUE;
}


/*
    ==========================================================================
    Description:
        Set enable or disable carry VLAN in the air
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_VLAN_TAG_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN	bVLAN_Tag;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev;


	bVLAN_Tag = (BOOLEAN)simple_strtol(arg, 0, 10);

	if (bVLAN_Tag == 1)
		bVLAN_Tag = TRUE;
	else if (bVLAN_Tag == 0)
		bVLAN_Tag = FALSE;
	else
		return FALSE;  //Invalid argument

	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
	wdev->bVLAN_Tag = bVLAN_Tag;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLAN_TAG_Proc::(VLAN_Tag=%d)\n",
				pObj->ioctl_if, wdev->bVLAN_Tag));

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
INT	Set_AP_AuthMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	ULONG i;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;
	struct wifi_dev *wdev;


	if (apidx >= pAd->ApCfg.BssidNum)
		return FALSE;

	/* Set Authentication mode */
	ApCfg_Set_AuthMode_Proc(pAd, apidx, arg);

	/* reset the portSecure for all entries */
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (IS_ENTRY_CLIENT(&pAd->MacTab.Content[i]))
		{
			pAd->MacTab.tr_entry[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}

	wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	/* reset the PortSecure this BSS */
	wdev->PortSecured = WPA_802_1X_PORT_NOT_SECURED;

	/* Default key index is always 2 in WPA mode */
	if(wdev->AuthMode >= Ndis802_11AuthModeWPA)
		wdev->DefaultKeyId = 1;

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
INT	Set_AP_EncrypType_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	struct wifi_dev *wdev;

	wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	if ((strcmp(arg, "NONE") == 0) || (strcmp(arg, "none") == 0))
		wdev->WepStatus = Ndis802_11WEPDisabled;
	else if ((strcmp(arg, "WEP") == 0) || (strcmp(arg, "wep") == 0))
		wdev->WepStatus = Ndis802_11WEPEnabled;
	else if ((strcmp(arg, "TKIP") == 0) || (strcmp(arg, "tkip") == 0))
		wdev->WepStatus = Ndis802_11TKIPEnable;
	else if ((strcmp(arg, "AES") == 0) || (strcmp(arg, "aes") == 0))
		wdev->WepStatus = Ndis802_11AESEnable;
	else if ((strcmp(arg, "TKIPAES") == 0) || (strcmp(arg, "tkipaes") == 0))
		wdev->WepStatus = Ndis802_11TKIPAESMix;
#ifdef WAPI_SUPPORT
	else if ((strcmp(arg, "SMS4") == 0) || (strcmp(arg, "sms4") == 0))
		wdev->WepStatus = Ndis802_11EncryptionSMS4Enabled;
#endif /* WAPI_SUPPORT */
	else
		return FALSE;

	if (wdev->WepStatus >= Ndis802_11TKIPEnable)
		wdev->DefaultKeyId = 1;

	/* decide the group key encryption type */
	if (wdev->WepStatus == Ndis802_11TKIPAESMix)
		wdev->GroupKeyWepStatus = Ndis802_11TKIPEnable;
	else
		wdev->GroupKeyWepStatus = wdev->WepStatus;

	/* move to ap.c::APStartUp to process */
    /*RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx); */
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_EncrypType_Proc::(EncrypType=%d)\n", apidx, wdev->WepStatus));

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
INT	Set_AP_WpaMixPairCipher_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	struct wifi_dev *wdev;

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
	 wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	if ((strncmp(arg, "WPA_AES_WPA2_TKIPAES", 20) == 0) || (strncmp(arg, "wpa_aes_wpa2_tkipaes", 20) == 0))
		wdev->WpaMixPairCipher = WPA_AES_WPA2_TKIPAES;
	else if ((strncmp(arg, "WPA_AES_WPA2_TKIP", 17) == 0) || (strncmp(arg, "wpa_aes_wpa2_tkip", 17) == 0))
		wdev->WpaMixPairCipher = WPA_AES_WPA2_TKIP;
	else if ((strncmp(arg, "WPA_TKIP_WPA2_AES", 17) == 0) || (strncmp(arg, "wpa_tkip_wpa2_aes", 17) == 0))
		wdev->WpaMixPairCipher = WPA_TKIP_WPA2_AES;
	else if ((strncmp(arg, "WPA_TKIP_WPA2_TKIPAES", 21) == 0) || (strncmp(arg, "wpa_tkip_wpa2_tkipaes", 21) == 0))
		wdev->WpaMixPairCipher = WPA_TKIP_WPA2_TKIPAES;
	else if ((strncmp(arg, "WPA_TKIPAES_WPA2_AES", 20) == 0) || (strncmp(arg, "wpa_tkipaes_wpa2_aes", 20) == 0))
		wdev->WpaMixPairCipher = WPA_TKIPAES_WPA2_AES;
	else if ((strncmp(arg, "WPA_TKIPAES_WPA2_TKIPAES", 24) == 0) || (strncmp(arg, "wpa_tkipaes_wpa2_tkipaes", 24) == 0))
		wdev->WpaMixPairCipher = WPA_TKIPAES_WPA2_TKIPAES;
	else if ((strncmp(arg, "WPA_TKIPAES_WPA2_TKIP", 21) == 0) || (strncmp(arg, "wpa_tkipaes_wpa2_tkip", 21) == 0))
		wdev->WpaMixPairCipher = WPA_TKIPAES_WPA2_TKIP;
	else
		return FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_WpaMixPairCipher_Proc=0x%02x\n", apidx, wdev->WpaMixPairCipher));

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
INT	Set_AP_RekeyInterval_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	INT32	val;

	val = simple_strtol(arg, 0, 10);

	if((val >= 10) && (val < MAX_REKEY_INTER))
		pAd->ApCfg.MBSSID[apidx].WPAREKEY.ReKeyInterval = val;
	else /* Default */
		pAd->ApCfg.MBSSID[apidx].WPAREKEY.ReKeyInterval = 3600;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_RekeyInterval_Proc=%ld\n",
								apidx, pAd->ApCfg.MBSSID[apidx].WPAREKEY.ReKeyInterval));

	return TRUE;
}

#ifdef SPECIFIC_TX_POWER_SUPPORT
INT Set_AP_PKT_PWR(
    IN  PRTMP_ADAPTER    pAd,
    IN  RTMP_STRING *arg)
{
        POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
        UCHAR           apidx = pObj->ioctl_if;
        INT input;
        input = simple_strtol(arg, 0, 10);

	/*
	  Tx_PWR_ADJ[3:0] From 0 to 7 is Positive & add with Tx Power (dB),
          From 8 to 15 is minus with Tx Power mapping to -16 to -2 (step by 2),
          Default value: 0.

	  [0x13BC]TX_ALC_MONITOR, 13:8
		  TX_ALC_REQ_ADJ TX ALC Req Saturated[5:0], unit (0.5dB)
	*/

	if ((input >= 0) && (input <= 15))
        	pAd->ApCfg.MBSSID[apidx].TxPwrAdj = input;
	else
		DBGPRINT(RT_DEBUG_ERROR, ("AP[%d]->PktPwr: Out of Range\n"));

	DBGPRINT(RT_DEBUG_TRACE, ("AP[%d]->PktPwr: %d\n", apidx, pAd->ApCfg.MBSSID[apidx].TxPwrAdj));

	return TRUE;
}
#endif /* SPECIFIC_TX_POWER_SUPPORT */
/*
    ==========================================================================
    Description:
        Set WPA rekey method
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_RekeyMethod_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_PMKCachePeriod_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
        Set AssocReq RSSI Threshold to reject STA with weak signal.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_AP_ASSOC_REQ_RSSI_THRESHOLD(
    IN  PRTMP_ADAPTER    pAd,
    IN  RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR           apidx = pObj->ioctl_if;
	UINT j;
        CHAR rssi;
		rssi = (CHAR)simple_strtol(arg, 0, 10);

        if (rssi == 0)
        {
                DBGPRINT(RT_DEBUG_TRACE, ("Disable AP_ASSOC_REQ_RSSI_THRESHOLD\n"));
        }
        else if (rssi > 0 || rssi < -100)
        {
                DBGPRINT(RT_DEBUG_TRACE, ("Set_AP_ASSOC_REQ_RSSI_THRESHOLD Value Error.\n"));
                return FALSE;
        }


        pAd->ApCfg.MBSSID[apidx].AssocReqRssiThreshold = rssi;

        DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_AP_ASSOC_REQ_RSSI_THRESHOLD=%d\n", apidx,
                                                                        pAd->ApCfg.MBSSID[apidx].AssocReqRssiThreshold ));

	for(j = BSS0; j < pAd->ApCfg.BssidNum; j++)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%d. ==> %d\n", j, pAd->ApCfg.MBSSID[j].AssocReqRssiThreshold ));
	}


        return TRUE;
}

/*
    ==========================================================================
    Description:
        Set lower limit for AP kicking out a STA.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_AP_KickStaRssiLow_Proc(
    IN  PRTMP_ADAPTER    pAd,
    IN  RTMP_STRING *arg)
{
        POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
        UCHAR           apidx = pObj->ioctl_if;
        UINT j;
        CHAR rssi;
		rssi = (CHAR)simple_strtol(arg, 0, 10);

        if (rssi == 0)
        {
                DBGPRINT(RT_DEBUG_TRACE, ("Disable RssiLowForStaKickOut Function\n"));
        }
        else if (rssi > 0 || rssi < -100)
        {
                DBGPRINT(RT_DEBUG_TRACE, ("RssiLowForStaKickOut Value Error.\n"));
                return FALSE;
        }


        pAd->ApCfg.MBSSID[apidx].RssiLowForStaKickOut = rssi;

        DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) RssiLowForStaKickOut=%d\n", apidx,
                                                                        pAd->ApCfg.MBSSID[apidx].RssiLowForStaKickOut ));

        for(j = BSS0; j < pAd->ApCfg.BssidNum; j++)
        {
                DBGPRINT(RT_DEBUG_TRACE, ("%d. ==> %d\n", j, pAd->ApCfg.MBSSID[j].RssiLowForStaKickOut ));
        }


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
INT	Set_AP_DefaultKeyID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	ULONG KeyIdx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  /* Invalid argument */

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
INT	Set_AP_Key1_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_Key2_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_Key3_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_Key4_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	Set_AP_Key1_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR	apidx, keyidx = 0;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = (UCHAR)pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][keyidx];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, keyidx);
	if (retVal == TRUE)
	{
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, keyidx, pSharedKey);
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
INT	Set_AP_Key2_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR	apidx, keyidx = 1;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = (UCHAR)pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][keyidx];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, keyidx);
	if (retVal == TRUE)
	{
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, keyidx, pSharedKey);
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
INT	Set_AP_Key3_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR	apidx, keyidx = 2;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = (UCHAR)pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][keyidx];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, keyidx);
	if (retVal == TRUE)
	{
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, keyidx, pSharedKey);
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
INT	Set_AP_Key4_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{

	POS_COOKIE pObj;
	UCHAR	apidx, keyidx = 3;
	CIPHER_KEY	*pSharedKey;
	INT		retVal;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = (UCHAR)pObj->ioctl_if;
	pSharedKey = &pAd->SharedKey[apidx][keyidx];
	retVal = RT_CfgSetWepKey(pAd, arg, pSharedKey, keyidx);
	if (retVal == TRUE)
	{
		/* Set keys (into ASIC) */
		if (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode >= Ndis802_11AuthModeWPA)
			;   /* not support */
		else    /* Old WEP stuff */
		{
			AsicAddSharedKeyEntry(pAd, apidx, keyidx, pSharedKey);
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
INT	Set_AccessPolicy_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
	ApUpdateAccessControlList(pAd, (UCHAR)pObj->ioctl_if);
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
	ApUpdateAccessControlList(pAd, (UCHAR)pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::Set_AccessControlList_Proc(Policy=%ld, Entry#=%ld)\n",
        pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("Entry #%02d: ", i+1));
		for (j=0; j<6; j++)
			DBGPRINT(RT_DEBUG_OFF, ("%02X ",
					pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]));
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
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
INT	Set_ACLAddEntry_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
	ApUpdateAccessControlList(pAd, (UCHAR)pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::%s(Policy=%ld, Entry#=%ld)\n",
		__FUNCTION__ , pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("Entry #%02d: ", i+1));
		for (j=0; j<MAC_ADDR_LEN; j++)
			DBGPRINT(RT_DEBUG_OFF, ("%02X ",
					pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]));
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
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
INT	Set_ACLDelEntry_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
	ApUpdateAccessControlList(pAd, (UCHAR)pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::%s(Policy=%ld, Entry#=%ld)\n",
		__FUNCTION__ , pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("Entry #%02d: ", i+1));
		for (j=0; j<MAC_ADDR_LEN; j++)
			DBGPRINT(RT_DEBUG_OFF, ("%02X ",
					pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]));
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
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
INT	Set_ACLShowAll_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	RT_802_11_ACL			acl;
	BOOLEAN					bDumpAll = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT						i, j;

	bDumpAll = (BOOLEAN)simple_strtol(arg, 0, 10);

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
	DBGPRINT(RT_DEBUG_OFF, ("=============== Access Control Policy ===============\n"));
	DBGPRINT(RT_DEBUG_OFF, ("Policy is %ld : ", acl.Policy));
	DBGPRINT(RT_DEBUG_OFF, ("%s\n", pACL_PolicyMessage[acl.Policy]));

	/* Dump the entry in the list one by one */
	DBGPRINT(RT_DEBUG_OFF, ("===============  Access Control List  ===============\n"));
	for (i=0; i<acl.Num; i++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("Entry #%02d: ", i+1));
		for (j=0; j<MAC_ADDR_LEN; j++)
			DBGPRINT(RT_DEBUG_OFF, ("%02X ", acl.Entry[i].Addr[j]));
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
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
INT	Set_ACLClearAll_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
/*	RT_802_11_ACL			acl; */
	RT_802_11_ACL			*pacl = NULL;
	BOOLEAN					bClearAll = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	bClearAll = (BOOLEAN)simple_strtol(arg, 0, 10);

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

		if (pacl != NULL)
			os_free_mem(NULL, pacl);

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
	ApUpdateAccessControlList(pAd, (UCHAR)pObj->ioctl_if);

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
	DBGPRINT(RT_DEBUG_OFF, ("%s - hexdump(len=%lu):", title, (unsigned long) len));
	if (show) {
		for (i = 0; i < len; i++)
			DBGPRINT(RT_DEBUG_OFF, (" %02x", buf[i]));
	} else {
		DBGPRINT(RT_DEBUG_OFF, (" [REMOVED]"));
	}
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
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
    NdisMoveMemory(pMBSSStruct->WscControl.WpaPsk, arg, pMBSSStruct->WscControl.WpaPskLen);
#endif /* WSC_AP_SUPPORT */

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

	radio = (UCHAR)simple_strtol(arg, 0, 10);

	pAd->iwpriv_command = FALSE;
#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT) {
		pAd->iwpriv_command = TRUE;		
	}
#endif /* MT_MAC */

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
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Error!!! SsidLength is larger than %d!\n",MAX_LEN_OF_SSID));
		return FALSE;
	}
	
	if (strcmp(arg,"1") == 0)
		pAd->ApCfg.AutoChannelAlg = ChannelAlgApCnt;
	else if (strcmp(arg,"2") == 0)
		pAd->ApCfg.AutoChannelAlg = ChannelAlgCCA;
	else if (strcmp(arg,"3") == 0)
		pAd->ApCfg.AutoChannelAlg =ChannelAlgBusyTime;
#ifdef SUPPORT_ACS_ALL_CHANNEL_RANK
    else if (strcmp(arg,"4") == 0)
		pAd->ApCfg.AutoChannelAlg =ChannelAlgCombined;
#endif    
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


/*
    ==========================================================================
    Description:
        Set a periodic check time for auto channel selection (unit: hour)
   Arguments:
       pAdapter                    Pointer to our adapter

    Return Value:
        TRUE if success, FALSE otherwise

    Note:
        Usage:
               iwpriv ra0 set ACSCheckTime=3  (unit: hour)

    ==========================================================================
*/
INT Set_AutoChannelSelCheckTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
   UINT8 Hour = simple_strtol(arg, 0, 10);

   pAd->ApCfg.ACSCheckTime = Hour*3600; /* Hour to second */
   pAd->ApCfg.ACSCheckCount = 0; /* Reset counter */
   DBGPRINT(RT_DEBUG_TRACE, ("%s(): ACSCheckTime=%u seconds(%u hours)\n",
                               __FUNCTION__, pAd->ApCfg.ACSCheckTime, Hour));
   return TRUE;
}
#endif /* AP_SCAN_SUPPORT */


INT Show_DriverInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	DBGPRINT(RT_DEBUG_OFF, ("driver version: %s.\n", AP_DRIVER_VERSION));

#ifdef CONFIG_ANDES_SUPPORT
	if (pAd->chipCap.MCUType == ANDES) {
		UINT32 loop = 0;
		RTMP_CHIP_CAP *cap = &pAd->chipCap;

		if (pAd->chipCap.need_load_fw) {
			USHORT fw_ver, build_ver;
			fw_ver = (*(cap->FWImageName + 11) << 8) | (*(cap->FWImageName + 10));
			build_ver = (*(cap->FWImageName + 9) << 8) | (*(cap->FWImageName + 8));

			DBGPRINT(RT_DEBUG_OFF, ("fw version:%d.%d.%02d ", (fw_ver & 0xf000) >> 8,
							(fw_ver & 0x0f00) >> 8, fw_ver & 0x00ff));
			DBGPRINT(RT_DEBUG_OFF, ("build:%x\n", build_ver));
			DBGPRINT(RT_DEBUG_OFF, ("build time:"));

			for (loop = 0; loop < 16; loop++)
				DBGPRINT(RT_DEBUG_OFF, ("%c", *(cap->FWImageName + 16 + loop)));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}

		if (pAd->chipCap.need_load_rom_patch) {
			DBGPRINT(RT_DEBUG_OFF, ("rom patch version = \n"));

			for (loop = 0; loop < 4; loop++)
				DBGPRINT(RT_DEBUG_OFF, ("%c", *(cap->rom_patch + 24 + loop)));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));

			DBGPRINT(RT_DEBUG_OFF, ("build time = \n"));

			for (loop = 0; loop < 16; loop++)
				DBGPRINT(RT_DEBUG_OFF, ("%c", *(cap->rom_patch + loop)));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}
#endif

#if defined(MT7603_FPGA) || defined(MT7628_FPGA)
	if ((IS_MT7603(pAd) || IS_MT7628(pAd)) && pAd->chipCap.hif_type == HIF_MT) {
		UINT32 mac_val, ver, date_code, rev;

		RTMP_IO_READ32(pAd, 0x2700, &ver);
		RTMP_IO_READ32(pAd, 0x2704, &rev);
		RTMP_IO_READ32(pAd, 0x2708, &date_code);
		RTMP_IO_READ32(pAd, 0x21f8, &mac_val);
		DBGPRINT(RT_DEBUG_OFF, ("##########################################\n"));
		DBGPRINT(RT_DEBUG_OFF, ("MT7603 FPGA Version:\n"));

		DBGPRINT(RT_DEBUG_OFF, ("\tFGPA1: Code[0x700]:0x%x, [0x704]:0x%x, [0x708]:0x%x\n",
					ver, rev, date_code));
		DBGPRINT(RT_DEBUG_OFF, ("\tFPGA2: Version[0x21f8]:0x%x\n", mac_val));
		DBGPRINT(RT_DEBUG_OFF, ("##########################################\n"));
	}
#endif /* MT7603_FPGA */

    return TRUE;
}


INT Show_StaCount_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i = 0, bss_idx=0;
	BSS_STRUCT *pMbss = NULL;

	DBGPRINT(RT_DEBUG_OFF, ("\n"));
	for (bss_idx=0; bss_idx < pAd->ApCfg.BssidNum; bss_idx++)
	{
		pMbss = &pAd->ApCfg.MBSSID[bss_idx];
		DBGPRINT(RT_DEBUG_OFF, ("BSS[%02d]: %5d\n", bss_idx, pMbss->StaCount));
	}

	DBGPRINT(RT_DEBUG_OFF, ("\n"));
#ifdef DOT11_N_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("HT Operating Mode : %d\n", pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode));
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
#endif /* DOT11_N_SUPPORT */

	DBGPRINT(RT_DEBUG_OFF, ("\n%-19s%-4s%-12s%-12s%-12s%-12s\n"
			, "MAC", "AID", "TxPackets", "RxPackets", "TxBytes", "RxBytes"));

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((IS_ENTRY_CLIENT(pEntry) || IS_ENTRY_APCLI(pEntry))
			&& (pEntry->Sst == SST_ASSOC))
		{
			DBGPRINT(RT_DEBUG_OFF, ("%02X:%02X:%02X:%02X:%02X:%02X  "
					, pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2]
					, pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->Aid));
			DBGPRINT(RT_DEBUG_OFF, ("%-12ld", (ULONG)pEntry->TxPackets.QuadPart));
			DBGPRINT(RT_DEBUG_OFF, ("%-12ld", (ULONG)pEntry->RxPackets.QuadPart));
			DBGPRINT(RT_DEBUG_OFF, ("%-12ld", (ULONG)pEntry->TxBytes));
			DBGPRINT(RT_DEBUG_OFF, ("%-12ld", (ULONG)pEntry->RxBytes));
			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}

	return TRUE;
}


INT Show_StaSecurityInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	UCHAR apidx;
	struct wifi_dev *wdev;

	DBGPRINT(RT_DEBUG_OFF, ("\n"));
	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
		DBGPRINT(RT_DEBUG_OFF, (" BSS(%d) AuthMode(%d)=%s, WepStatus(%d)=%s, GroupWepStatus(%d)=%s, WPAMixPairCipher(0x%02X)\n"
				, apidx, wdev->AuthMode, GetAuthMode((CHAR)wdev->AuthMode)
				, wdev->WepStatus, GetEncryptType((CHAR)wdev->WepStatus)
				, wdev->GroupKeyWepStatus
				, GetEncryptType((CHAR)wdev->GroupKeyWepStatus)
				, wdev->WpaMixPairCipher));
	}
	DBGPRINT(RT_DEBUG_OFF, ("\n"));

	DBGPRINT(RT_DEBUG_OFF, ("\n%-19s%-4s%-4s%-15s%-12s\n",
		   "MAC", "AID", "BSS", "Auth", "Encrypt"));

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (pEntry && IS_ENTRY_CLIENT(pEntry) && pEntry->Sst == SST_ASSOC)
		{
			DBGPRINT(RT_DEBUG_OFF, ("%02X:%02X:%02X:%02X:%02X:%02X  ",
					PRINT_MAC(pEntry->Addr)));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->Aid));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->func_tb_idx));
			DBGPRINT(RT_DEBUG_OFF, ("%-15s", GetAuthMode((CHAR)pEntry->AuthMode)));
			DBGPRINT(RT_DEBUG_OFF, ("%-12s", GetEncryptType((CHAR)pEntry->WepStatus)));
			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}

	return TRUE;
}


INT Show_RAInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef PRE_ANT_SWITCH
	DBGPRINT(RT_DEBUG_OFF, ("PreAntSwitch: %d\n", pAd->CommonCfg.PreAntSwitch));
	DBGPRINT(RT_DEBUG_OFF, ("PreAntSwitchRSSI: %d\n", pAd->CommonCfg.PreAntSwitchRSSI));
#endif /* PRE_ANT_SWITCH */

#ifdef CFO_TRACK
	DBGPRINT(RT_DEBUG_OFF, ("CFOTrack: %d\n", pAd->CommonCfg.CFOTrack));
#endif /* CFO_TRACK */

#if defined (RT2883) || defined (RT3883)
	DBGPRINT(RT_DEBUG_OFF, ("FixedRate: %d\n", pAd->CommonCfg.FixedRate));
#endif /* defined (RT2883) || defined (RT3883) */

#ifdef NEW_RATE_ADAPT_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("LowTrafficThrd: %d\n", pAd->CommonCfg.lowTrafficThrd));
	DBGPRINT(RT_DEBUG_OFF, ("TrainUpRule: %d\n", pAd->CommonCfg.TrainUpRule));
	DBGPRINT(RT_DEBUG_OFF, ("TrainUpRuleRSSI: %d\n", pAd->CommonCfg.TrainUpRuleRSSI));
	DBGPRINT(RT_DEBUG_OFF, ("TrainUpLowThrd: %d\n", pAd->CommonCfg.TrainUpLowThrd));
	DBGPRINT(RT_DEBUG_OFF, ("TrainUpHighThrd: %d\n", pAd->CommonCfg.TrainUpHighThrd));
#endif /* NEW_RATE_ADAPT_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("StreamMode: %d\n", pAd->CommonCfg.StreamMode));
	DBGPRINT(RT_DEBUG_OFF, ("StreamModeMCS: 0x%04x\n", pAd->CommonCfg.StreamModeMCS));
#endif /* STREAM_MODE_SUPPORT */
#ifdef TXBF_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("ITxBfEn: %d\n", pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn));
	DBGPRINT(RT_DEBUG_OFF, ("ITxBfTimeout: %ld\n", pAd->CommonCfg.ITxBfTimeout));
	DBGPRINT(RT_DEBUG_OFF, ("ETxBfTimeout: %ld\n", pAd->CommonCfg.ETxBfTimeout));
	DBGPRINT(RT_DEBUG_OFF, ("ETxBfEnCond: %ld\n", pAd->CommonCfg.ETxBfEnCond));
	DBGPRINT(RT_DEBUG_OFF, ("ETxBfNoncompress: %d\n", pAd->CommonCfg.ETxBfNoncompress));
	DBGPRINT(RT_DEBUG_OFF, ("ETxBfIncapable: %d\n", pAd->CommonCfg.ETxBfIncapable));
#endif /* TXBF_SUPPORT */

#ifdef DBG_CTRL_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("DebugFlags: 0x%lx\n", pAd->CommonCfg.DebugFlags));
#endif /* DBG_CTRL_SUPPORT */

	return TRUE;
}


#ifdef RTMP_MAC_PCI
#ifdef DBG_DIAGNOSE
INT Set_DiagOpt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG diagOpt;
	/*POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie; */

	diagOpt = simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("DiagOpt=%ld!\n", diagOpt));

	return TRUE;
}


INT Set_BDInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT32 i, QueIdx=0;
	UINT32 RegValue;
	RXD_STRUC *pRxD;
	TXD_STRUC *pTxD;
#ifdef RT_BIG_ENDIAN
	RXD_STRUC *pDestRxD, *pDestTxD;
	RXD_STRUC RxD, TxD;
#endif /* RT_BIG_ENDIAN */
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QueIdx];
	RTMP_MGMT_RING *pMgmtRing = &pAd->MgmtRing;
	RTMP_RX_RING *pRxRing = &pAd->RxRing[0];

	DBGPRINT(RT_DEBUG_OFF, ("\n"));

#ifdef DOT11_N_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("HT Operating Mode : %d\n",
	pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode));
#endif /* DOT11_N_SUPPORT */
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
	DBGPRINT(RT_DEBUG_OFF, ("[Tx]: SwFreeIdx=%d, CpuIdx=%d, DmaIdx=%d\n",
		pAd->TxRing[QueIdx].TxSwFreeIdx,
		pAd->TxRing[QueIdx].TxCpuIdx,
		pAd->TxRing[QueIdx].TxDmaIdx));

#ifdef RT_BIG_ENDIAN
	pDestTxD = (TXD_STRUC *) pTxRing->Cell[pAd->TxRing[QueIdx].TxSwFreeIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD = (TXD_STRUC *) pTxRing->Cell[pAd->TxRing[QueIdx].TxSwFreeIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("Tx SwFreeIdx Descriptor", (char *)pTxD, 16);
	DBGPRINT(RT_DEBUG_OFF, ("pTxD->DMADONE = %x\n", pTxD->DMADONE));

#ifdef RT_BIG_ENDIAN
	pDestTxD = (TXD_STRUC *) pTxRing->Cell[pAd->TxRing[QueIdx].TxCpuIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD = (TXD_STRUC *) pTxRing->Cell[pAd->TxRing[QueIdx].TxCpuIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("Tx CpuIdx Descriptor", (char *)pTxD, 16);
	DBGPRINT(RT_DEBUG_OFF, ("pTxD->DMADONE = %x\n", pTxD->DMADONE));

#ifdef RT_BIG_ENDIAN
	pDestTxD = (TXD_STRUC *) pTxRing->Cell[pAd->TxRing[QueIdx].TxDmaIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD = (TXD_STRUC *) pTxRing->Cell[pAd->TxRing[QueIdx].TxDmaIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("Tx DmaIdx Descriptor", (char *)pTxD, 16);
	DBGPRINT(RT_DEBUG_OFF, ("pTxD->DMADONE = %x\n", pTxD->DMADONE));

	DBGPRINT(RT_DEBUG_OFF, ("[Mgmt]: SwFreeIdx=%d, CpuIdx=%d, DmaIdx=%d\n",
		pAd->MgmtRing.TxSwFreeIdx,
		pAd->MgmtRing.TxCpuIdx,
		pAd->MgmtRing.TxDmaIdx));

#ifdef RT_BIG_ENDIAN
	pDestTxD = (TXD_STRUC *) pMgmtRing->Cell[pAd->MgmtRing.TxSwFreeIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD = (TXD_STRUC *) pMgmtRing->Cell[pAd->MgmtRing.TxSwFreeIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("Mgmt SwFreeIdx Descriptor", (char *)pTxD, 16);

#ifdef RT_BIG_ENDIAN
	pDestTxD = (TXD_STRUC *) pMgmtRing->Cell[pAd->MgmtRing.TxCpuIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD = (TXD_STRUC *) pMgmtRing->Cell[pAd->MgmtRing.TxCpuIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("Mgmt CpuIdx Descriptor", (char *)pTxD, 16);

#ifdef RT_BIG_ENDIAN
	pDestTxD = (TXD_STRUC *) pMgmtRing->Cell[pAd->MgmtRing.TxDmaIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD = (TXD_STRUC *) pMgmtRing->Cell[pAd->MgmtRing.TxDmaIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("Mgmt DmaIdx Descriptor", (char *)pTxD, 16);
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
	DBGPRINT(RT_DEBUG_OFF, ("[Rx]:  SwRedIdx=%d, CpuIdx=%d, DmaIdx=%d\n",
		pAd->RxRing[0].RxSwReadIdx,
		pAd->RxRing[0].RxCpuIdx,
		pAd->RxRing[0].RxDmaIdx));

#ifdef RT_BIG_ENDIAN
	pDestRxD = (RXD_STRUC *) pRxRing->Cell[pAd->RxRing[0].RxSwReadIdx].AllocVa;
	RxD = *pDestRxD;
	pRxD = &RxD;
	RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#else
	pRxD = (RXD_STRUC *) pRxRing->Cell[pAd->RxRing[0].RxSwReadIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("RX SwRedIdx Descriptor", (char *)pRxD, 16);

#ifdef RT_BIG_ENDIAN
	pDestRxD = (RXD_STRUC *) pRxRing->Cell[pAd->RxRing[0].RxCpuIdx].AllocVa;
	RxD = *pDestRxD;
	pRxD = &RxD;
	RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#else
	pRxD = (RXD_STRUC *) pRxRing->Cell[pAd->RxRing[0].RxCpuIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("RX RxCupIdx Descriptor", (char *)pRxD, 16);

#ifdef RT_BIG_ENDIAN
	pDestRxD = (RXD_STRUC *) pRxRing->Cell[pAd->RxRing[0].RxDmaIdx].AllocVa;
	RxD = *pDestRxD;
	pRxD = &RxD;
	RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#else
	pRxD = (RXD_STRUC *) pRxRing->Cell[pAd->RxRing[0].RxDmaIdx].AllocVa;
#endif /* RT_BIG_ENDIAN */
	hex_dump("RX RxDmaIdx Descritpro", (char *)pRxD, 16);
	DBGPRINT(RT_DEBUG_OFF, ("\n%-19s%-4s%-4s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s\n"
			, "MAC", "AID", "PSM", "RSSI0", "RSSI1", "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC"));

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			DBGPRINT(RT_DEBUG_OFF, ("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->Aid));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->PsMode));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pEntry->RssiSample.AvgRssi[0]));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pEntry->RssiSample.AvgRssi[1]));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pEntry->RssiSample.AvgRssi[2]));


			DBGPRINT(RT_DEBUG_OFF, ("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE)));
			DBGPRINT(RT_DEBUG_OFF, ("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW)));
			DBGPRINT(RT_DEBUG_OFF, ("%-6d", pEntry->HTPhyMode.field.MCS));
			DBGPRINT(RT_DEBUG_OFF, ("%-6d", pEntry->HTPhyMode.field.ShortGI));
			DBGPRINT(RT_DEBUG_OFF, ("%-6d", pEntry->HTPhyMode.field.STBC));
			DBGPRINT(RT_DEBUG_OFF, ("%-10d, %d, %d\n", pEntry->FIFOCount, pEntry->DebugTxCount
					, pEntry->DebugTxCount-pEntry->FIFOCount));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}

	return TRUE;
}


INT Set_diag_cond_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
    UINT32 cond;

    cond = simple_strtol(arg, 0, 10);
    pAd->DiagStruct.diag_cond = cond;

    return TRUE;
}


INT Show_Diag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	RtmpDiagStruct *pDiag = NULL;
	UCHAR i, start, stop, McsIdx, SwQNumLevel, TxDescNumLevel, que_idx;
	unsigned long irqFlags;
	UCHAR McsMaxIdx = MAX_MCS_SET;
#ifdef DOT11_VHT_AC
	UCHAR vht_mcs_max_idx = MAX_VHT_MCS_SET;
#endif /* DOT11_VHT_AC */

	os_alloc_mem(pAd, (UCHAR **)&pDiag, sizeof(RtmpDiagStruct));
	if (!pDiag) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s():AllocMem failed!\n", __FUNCTION__));
		return FALSE;
	}

	RTMP_IRQ_LOCK(&pAd->irq_lock, irqFlags);
	NdisMoveMemory(pDiag, &pAd->DiagStruct, sizeof(RtmpDiagStruct));
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, irqFlags);

#ifdef DOT11N_SS3_SUPPORT
	if (IS_RT2883(pAd) || IS_RT3883(pAd))
		McsMaxIdx = 24;
#endif /* DOT11N_SS3_SUPPORT */

	if (pDiag->inited == FALSE)
		goto done;

	start = pDiag->ArrayStartIdx;
	stop = pDiag->ArrayCurIdx;
	DBGPRINT(RT_DEBUG_OFF, ("Start=%d, stop=%d!\n\n", start, stop));
	DBGPRINT(RT_DEBUG_OFF, ("    %-12s", "Time(Sec)"));
	for(i=1; i< DIAGNOSE_TIME; i++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%-7d", i));
	}
	DBGPRINT(RT_DEBUG_OFF, ("\n    -------------------------------------------------------------------------------\n"));
	DBGPRINT(RT_DEBUG_OFF, ("Tx Info:\n"));
	DBGPRINT(RT_DEBUG_OFF, ("    %-12s", "TxDataCnt\n"));
	for (que_idx = 0; que_idx < WMM_NUM_OF_AC; que_idx++) {
		DBGPRINT(RT_DEBUG_OFF, ("\tQueue[%d]:", que_idx));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxDataCnt[que_idx]));
		}
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s", "TxFailCnt"));
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxFailCnt));
	}

#ifdef DBG_TX_AGG_CNT
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s", "TxAggCnt"));
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxAggCnt));
	}
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
#endif /* DBG_TX_AGG_CNT */

#ifdef DBG_TXQ_DEPTH
	DBGPRINT(RT_DEBUG_OFF, ("DeQueue Info:\n"));

	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "DeQueueFunc Called Distribution"));
	DBGPRINT(RT_DEBUG_OFF, ("\t"));
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		DBGPRINT(RT_DEBUG_OFF, ("%-8d", pDiag->diag_info[i].deq_called));

	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "DeQueueRound(Per-Call) Distribution"));
	DBGPRINT(RT_DEBUG_OFF, ("\t"));
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		DBGPRINT(RT_DEBUG_OFF, ("%-8d", pDiag->diag_info[i].deq_round));

	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "DeQueueCount(Per-Round) Distribution"));
	for (SwQNumLevel = 0 ; SwQNumLevel < 9; SwQNumLevel++)
	{
		if (SwQNumLevel == 8)
			DBGPRINT(RT_DEBUG_OFF, ("\t>%-5d",  SwQNumLevel));
		else
			DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", SwQNumLevel));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].deq_cnt[SwQNumLevel]);

		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}

	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s%d", "Sw-Queued TxSwQCnt for WCID ", pDiag->wcid));
	for (que_idx = 0; que_idx < WMM_NUM_OF_AC; que_idx++) {
		DBGPRINT(RT_DEBUG_OFF, ("\n    %s[%d]\n", "Queue", que_idx));
		for (SwQNumLevel = 0 ; SwQNumLevel < 9; SwQNumLevel++)
		{
			if (SwQNumLevel == 8)
				DBGPRINT(RT_DEBUG_OFF, ("\t>%-5d",  SwQNumLevel));
			else
				DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", SwQNumLevel));
			for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
				DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxSWQueCnt[que_idx][SwQNumLevel]));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
		DBGPRINT(RT_DEBUG_OFF, ("	%-12s\n", "TxEnQFailCnt"));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			DBGPRINT(RT_DEBUG_OFF, ("\t%-7d", pDiag->diag_info[i].enq_fall_cnt[que_idx]));
		}
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}

	DBGPRINT(RT_DEBUG_OFF, ("\n	  %s\n", "DeQueFailedCnt:Reason NotTxResource"));
	for (que_idx = 0; que_idx < WMM_NUM_OF_AC; que_idx++) {
		DBGPRINT(RT_DEBUG_OFF, ("\n    %s[%d]:", "Queue", que_idx));
		for (i = start; i != stop;	i = (i+1) % DIAGNOSE_TIME)
			DBGPRINT(RT_DEBUG_OFF, ("\t%-7d", pDiag->diag_info[i].deq_fail_no_resource_cnt[que_idx]));
	}
#endif /* DBG_TXQ_DEPTH */

#ifdef DBG_TX_RING_DEPTH
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "DMA-Queued TxDescCnt"));
	for (que_idx = 0; que_idx < WMM_NUM_OF_AC; que_idx++) {
		DBGPRINT(RT_DEBUG_OFF, ("\n Queue[%d]\n", que_idx);
		for(TxDescNumLevel = 0; TxDescNumLevel < 16; TxDescNumLevel++)
		{
			if (TxDescNumLevel == 15)
				DBGPRINT(RT_DEBUG_OFF, ("\t>%-5d",  TxDescNumLevel));
			else
				DBGPRINT(RT_DEBUG_OFF, ("\t%-6d",  TxDescNumLevel));
			for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
				DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxDescCnt[que_idx][TxDescNumLevel]));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}
#endif /* DBG_TX_RING_DEPTH */

#ifdef MT_MAC
#ifdef DBG_PSE_DEPTH
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n",
	"PSE-Queued TxPageCnt in unit of Packets (mod 13)"));
	{
		UCHAR pg_level;
		for(pg_level = 0; pg_level < 50; pg_level++)
		{
			if (pg_level == 49)
				DBGPRINT(RT_DEBUG_OFF, ("\t>%-5d",  pg_level));
			else
				DBGPRINT(RT_DEBUG_OFF, ("\t%-6d",  pg_level));
			for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
				DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].pse_pg_cnt[pg_level]));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}
#endif /* DBG_PSE_DEPTH */
#endif /* MT_MAC */

#ifdef DOT11_N_SUPPORT
#ifdef DBG_TX_AGG_CNT
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "Tx-Agged AMPDUCnt"));
	for (McsIdx =0 ; McsIdx < 16; McsIdx++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", (McsIdx+1)));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			DBGPRINT(RT_DEBUG_OFF, ("%d(%d%%)  ", pDiag->diag_info[i].TxAMPDUCnt[McsIdx],
					pDiag->diag_info[i].TxAMPDUCnt[McsIdx] ? (pDiag->diag_info[i].TxAMPDUCnt[McsIdx] * 100 / pDiag->diag_info[i].TxAggCnt) : 0));
		}
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}
#endif /* DBG_TX_AGG_CNT */
#endif /* DOT11_N_SUPPORT */

#ifdef DBG_TX_MCS
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "TxMcsCnt_HT"));
	for (McsIdx =0 ; McsIdx < McsMaxIdx; McsIdx++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", McsIdx));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxMcsCnt_HT[McsIdx]));
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}

#ifdef DOT11_VHT_AC
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "TxMcsCnt_VHT"));
	for (McsIdx =0 ; McsIdx < vht_mcs_max_idx; McsIdx++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", McsIdx));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].TxMcsCnt_VHT[McsIdx]));
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}
#endif /* DOT11_VHT_AC */
#endif /* DBG_TX_MCS */



	DBGPRINT(RT_DEBUG_OFF, ("Rx Info\n"));
	DBGPRINT(RT_DEBUG_OFF, ("    %-12s", "RxDataCnt"));
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].RxDataCnt));
	}
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s", "RxCrcErrCnt"));
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].RxCrcErrCnt));
	}

#ifdef DBG_RX_MCS
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "RxMcsCnt_HT"));
	for (McsIdx =0 ; McsIdx < McsMaxIdx; McsIdx++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", McsIdx));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].RxMcsCnt_HT[McsIdx]));
		}
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}

#ifdef DOT11_VHT_AC
	DBGPRINT(RT_DEBUG_OFF, ("\n    %-12s\n", "RxMcsCnt_VHT"));
	for (McsIdx =0 ; McsIdx < vht_mcs_max_idx; McsIdx++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("\t%-6d", McsIdx));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pDiag->diag_info[i].RxMcsCnt_VHT[McsIdx]));
		}
		DBGPRINT(RT_DEBUG_OFF, ("\n"));
	}
#endif /* DOT11_VHT_AC */

#endif /* DBG_RX_MCS */

	DBGPRINT(RT_DEBUG_OFF, ("\n-------------\n"));

done:
	os_free_mem(pAd, pDiag);
	return TRUE;
}
#endif /* DBG_DIAGNOSE */
#endif /* RTMP_MAC_PCI */


INT Show_Sat_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	/* Sanity check for calculation of sucessful count */
	DBGPRINT(RT_DEBUG_OFF, ("TransmitCountFromOS = %d\n"
			, pAd->WlanCounters.TransmitCountFrmOs.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("TransmittedFragmentCount = %lld\n"
			, pAd->WlanCounters.TransmittedFragmentCount.u.LowPart + pAd->WlanCounters.MulticastTransmittedFrameCount.QuadPart));
	DBGPRINT(RT_DEBUG_OFF, ("MulticastTransmittedFrameCount = %d\n"
			, pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("FailedCount = %d\n", pAd->WlanCounters.FailedCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RetryCount = %d\n", pAd->WlanCounters.RetryCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("MultipleRetryCount = %d\n"
			, pAd->WlanCounters.MultipleRetryCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RTSSuccessCount = %d\n", pAd->WlanCounters.RTSSuccessCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RTSFailureCount = %d\n", pAd->WlanCounters.RTSFailureCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("ACKFailureCount = %d\n", pAd->WlanCounters.ACKFailureCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("FrameDuplicateCount = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("ReceivedFragmentCount = %d\n", pAd->WlanCounters.ReceivedFragmentCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("MulticastReceivedFrameCount = %d\n", pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("Rx drop due to out of resource  = %ld\n", (ULONG)pAd->Counters8023.RxNoBuffer));
#ifdef DBG
	DBGPRINT(RT_DEBUG_OFF, ("RealFcsErrCount = %d\n", pAd->RalinkCounters.RealFcsErrCount.u.LowPart));
#else
	DBGPRINT(RT_DEBUG_OFF, ("FCSErrorCount = %d\n", pAd->WlanCounters.FCSErrorCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("FrameDuplicateCount.LowPart = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100));
#endif
	DBGPRINT(RT_DEBUG_OFF, ("TransmittedFrameCount = %d\n", pAd->WlanCounters.TransmittedFragmentCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("WEPUndecryptableCount = %d\n", pAd->WlanCounters.WEPUndecryptableCount.u.LowPart));

#ifdef DOT11_N_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("\n===Some 11n statistics variables:\n"));
	/* Some 11n statistics variables */
	DBGPRINT(RT_DEBUG_OFF, ("TxAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TxAMSDUCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RxAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.RxAMSDUCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("TransmittedAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("TransmittedMPDUsInAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("TransmittedOctetsInAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("MPDUInReceivedAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.MPDUInReceivedAMPDUCount.u.LowPart));
#ifdef DOT11N_DRAFT3
	DBGPRINT(RT_DEBUG_OFF, ("fAnyStaFortyIntolerant=%d\n", pAd->MacTab.fAnyStaFortyIntolerant));
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

{
	int apidx;

	for (apidx=0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("-- IF-ra%d --\n", apidx));
		DBGPRINT(RT_DEBUG_OFF, ("Packets Received = %ld\n",
			(ULONG)pAd->ApCfg.MBSSID[apidx].RxCount));
		DBGPRINT(RT_DEBUG_OFF, ("Packets Sent = %ld\n",
			(ULONG)pAd->ApCfg.MBSSID[apidx].TxCount));
		DBGPRINT(RT_DEBUG_OFF, ("Bytes Received = %ld\n",
			(ULONG)pAd->ApCfg.MBSSID[apidx].ReceivedByteCount));
		DBGPRINT(RT_DEBUG_OFF, ("Byte Sent = %ld\n",
			(ULONG)pAd->ApCfg.MBSSID[apidx].TransmittedByteCount));
		DBGPRINT(RT_DEBUG_OFF, ("Error Packets Received = %ld\n",
			(ULONG)pAd->ApCfg.MBSSID[apidx].RxErrorCount));
		DBGPRINT(RT_DEBUG_OFF, ("Drop Received Packets = %ld\n",
			(ULONG)pAd->ApCfg.MBSSID[apidx].RxDropCount));

#ifdef WSC_INCLUDED
		if (pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode != WSC_DISABLE)
		{
			WSC_CTRL *pWscCtrl;

			pWscCtrl = &pAd->ApCfg.MBSSID[apidx].WscControl;
			DBGPRINT(RT_DEBUG_OFF, ("WscInfo:\n"
					"\tWscConfMode=%d\n"
					"\tWscMode=%s\n"
					"\tWscConfStatus=%d\n"
					"\tWscPinCode=%d\n"
					"\tWscState=0x%x\n"
					"\tWscStatus=0x%x\n",
					pWscCtrl->WscConfMode,
					((pWscCtrl->WscMode == WSC_PIN_MODE) ? "PIN" : "PBC"),
					pWscCtrl->WscConfStatus, pWscCtrl->WscEnrolleePinCode,
					pWscCtrl->WscState, pWscCtrl->WscStatus))));
		}
#endif /* WSC_INCLUDED */

		DBGPRINT(RT_DEBUG_OFF, ("-- IF-ra%d end --\n", apidx));
	}
}

{
	int i, j, k, maxMcs = MAX_MCS_SET -1;
	PMAC_TABLE_ENTRY pEntry;

#ifdef DOT11N_SS3_SUPPORT
	if (IS_RT2883(pAd) || IS_RT3883(pAd))
		maxMcs = 23;
#endif /* DOT11N_SS3_SUPPORT */

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &pAd->MacTab.Content[i];
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x - ",
					PRINT_MAC(pEntry->Addr)));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d\n", (int)pEntry->Aid));

			for (j=maxMcs; j>=0; j--)
			{
				if ((pEntry->TXMCSExpected[j] != 0) || (pEntry->TXMCSFailed[j] !=0))
				{
					DBGPRINT(RT_DEBUG_OFF, ("MCS[%02d]: Expected %u, Successful %u (%d%%), Failed %u\n"
							, j, pEntry->TXMCSExpected[j], pEntry->TXMCSSuccessful[j]
							, pEntry->TXMCSExpected[j] ? (100*pEntry->TXMCSSuccessful[j])/pEntry->TXMCSExpected[j] : 0
							, pEntry->TXMCSFailed[j]));
					for(k=maxMcs; k>=0; k--)
					{
						if (pEntry->TXMCSAutoFallBack[j][k] != 0) {
							DBGPRINT(RT_DEBUG_OFF, ("\t\t\tAutoMCS[%02d]: %u (%d%%)\n", k
									, pEntry->TXMCSAutoFallBack[j][k]
									, (100*pEntry->TXMCSAutoFallBack[j][k])/pEntry->TXMCSExpected[j]));
						}
					}
				}
			}
		}
	}

}

#ifdef DOT11_N_SUPPORT
	/* Display Tx Aggregation statistics */
	DisplayTxAgg(pAd);
#endif /* DOT11_N_SUPPORT */

	return TRUE;
}



INT Show_Sat_Reset_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	/* Sanity check for calculation of sucessful count */

	DBGPRINT(RT_DEBUG_OFF, ("TransmittedFragmentCount = %lld\n"
			, pAd->WlanCounters.TransmittedFragmentCount.u.LowPart + pAd->WlanCounters.MulticastTransmittedFrameCount.QuadPart));
	DBGPRINT(RT_DEBUG_OFF, ("MulticastTransmittedFrameCount = %d\n"
			, pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("FailedCount = %d\n", pAd->WlanCounters.FailedCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RetryCount = %d\n", pAd->WlanCounters.RetryCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("MultipleRetryCount = %d\n", pAd->WlanCounters.MultipleRetryCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RTSSuccessCount = %d\n", pAd->WlanCounters.RTSSuccessCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("RTSFailureCount = %d\n", pAd->WlanCounters.RTSFailureCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("ACKFailureCount = %d\n", pAd->WlanCounters.ACKFailureCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("FrameDuplicateCount = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("ReceivedFragmentCount = %d\n", pAd->WlanCounters.ReceivedFragmentCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("MulticastReceivedFrameCount = %d\n", pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("Rx drop due to out of resource  = %ld\n", (ULONG)pAd->Counters8023.RxNoBuffer));
#ifdef DBG
	DBGPRINT(RT_DEBUG_OFF, ("RealFcsErrCount = %d\n", pAd->RalinkCounters.RealFcsErrCount.u.LowPart));
#else
	DBGPRINT(RT_DEBUG_OFF, ("FCSErrorCount = %d\n", pAd->WlanCounters.FCSErrorCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("FrameDuplicateCount.LowPart = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100));
#endif
	DBGPRINT(RT_DEBUG_OFF, ("TransmittedFrameCount = %d\n", pAd->WlanCounters.TransmittedFrameCount.u.LowPart));
	DBGPRINT(RT_DEBUG_OFF, ("WEPUndecryptableCount = %d\n", pAd->WlanCounters.WEPUndecryptableCount.u.LowPart));

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
	pAd->Counters8023.RxNoBuffer = 0;
#ifdef DBG
	pAd->RalinkCounters.RealFcsErrCount.u.LowPart = 0;
#else
	pAd->WlanCounters.FCSErrorCount.u.LowPart = 0;
	pAd->WlanCounters.FrameDuplicateCount.u.LowPart = 0;
#endif

	pAd->WlanCounters.TransmittedFrameCount.u.LowPart = 0;
	pAd->WlanCounters.WEPUndecryptableCount.u.LowPart = 0;

	{
		int i, j, k, maxMcs = 15;
		PMAC_TABLE_ENTRY pEntry;

#ifdef DOT11N_SS3_SUPPORT
		if (IS_RT2883(pAd) || IS_RT3883(pAd))
			maxMcs = 23;
#endif /* DOT11N_SS3_SUPPORT */

		for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
		{
			pEntry = &pAd->MacTab.Content[i];
			if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
			{

				DBGPRINT(RT_DEBUG_OFF, ("\n%02X:%02X:%02X:%02X:%02X:%02X - ",
					   pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
					   pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]));
				DBGPRINT(RT_DEBUG_OFF, ("%-4d\n", (int)pEntry->Aid));

				for (j = maxMcs; j >= 0; j--)
				{
					if ((pEntry->TXMCSExpected[j] != 0) || (pEntry->TXMCSFailed[j] !=0))
					{
						DBGPRINT(RT_DEBUG_OFF, ("MCS[%02d]: Expected %u, Successful %u (%d%%), Failed %u\n"
								, j, pEntry->TXMCSExpected[j], pEntry->TXMCSSuccessful[j]
								, pEntry->TXMCSExpected[j] ? (100*pEntry->TXMCSSuccessful[j])/pEntry->TXMCSExpected[j] : 0
								, pEntry->TXMCSFailed[j]));
						for(k = maxMcs; k >= 0; k--)
						{
							if (pEntry->TXMCSAutoFallBack[j][k] != 0)
							{
								DBGPRINT(RT_DEBUG_OFF, ("\t\t\tAutoMCS[%02d]: %u (%d%%)\n"
										, k, pEntry->TXMCSAutoFallBack[j][k]
										, (100*pEntry->TXMCSAutoFallBack[j][k])/pEntry->TXMCSExpected[j]));
							}
						}
					}
				}
			}
			for (j = 0; j < (maxMcs + 1); j++)
			{
				pEntry->TXMCSExpected[j] = 0;
				pEntry->TXMCSSuccessful[j] = 0;
				pEntry->TXMCSFailed[j] = 0;
				for(k = maxMcs; k >= 0; k--)
				{
					pEntry->TXMCSAutoFallBack[j][k] = 0;
				}
			}
		}
	}
#ifdef DOT11_N_SUPPORT
	/* Display Tx Aggregation statistics */
	DisplayTxAgg(pAd);
#endif /* DOT11_N_SUPPORT */

	return TRUE;
}


#ifdef MAT_SUPPORT
INT Show_MATTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	extern VOID dumpIPMacTb(MAT_STRUCT *pMatCfg, int index);
	extern NDIS_STATUS dumpSesMacTb(MAT_STRUCT *pMatCfg, int hashIdx);
	extern NDIS_STATUS dumpUidMacTb(MAT_STRUCT *pMatCfg, int hashIdx);
	extern NDIS_STATUS dumpIPv6MacTb(MAT_STRUCT *pMatCfg, int hashIdx);

	dumpIPMacTb(&pAd->MatCfg, -1);
	dumpSesMacTb(&pAd->MatCfg, -1);
	dumpUidMacTb(&pAd->MatCfg, -1);
	dumpIPv6MacTb(&pAd->MatCfg, -1);

	DBGPRINT(RT_DEBUG_OFF, ("Default BroadCast Address=%02x:%02x:%02x:%02x:%02x:%02x!\n",
			BROADCAST_ADDR[0], BROADCAST_ADDR[1], BROADCAST_ADDR[2],
			BROADCAST_ADDR[3], BROADCAST_ADDR[4], BROADCAST_ADDR[5]));
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
	struct wifi_dev *wdev;

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("RTMPIoctlRadiusData, IF(ra%d)\n", pObj->ioctl_if));
#endif /* RELEASE_EXCLUDE */

	if (pObj->ioctl_if > pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Invalid MBSSID index(%d)!\n",
						__FUNCTION__, pObj->ioctl_if));
		return;
	}

	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
	if ((wdev->AuthMode == Ndis802_11AuthModeWPA)
    	|| (wdev->AuthMode == Ndis802_11AuthModeWPA2)
    	|| (wdev->AuthMode == Ndis802_11AuthModeWPA1WPA2)
    	|| (wdev->IEEE8021X == TRUE))
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
    	//PUCHAR	Key;

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
    			//Key = pAd->SharedKey[apidx][KeyIdx].Key;

				/* Set Group key material to Asic */
				AsicAddSharedKeyEntry(pAd, apidx, (UINT8)KeyIdx, &pAd->SharedKey[apidx][KeyIdx]);

				/* Get a specific WCID to record this MBSS key attribute */
				GET_GroupKey_WCID(pAd, Wcid, apidx);

				RTMPSetWcidSecurityInfo(pAd, apidx,(UINT8)KeyIdx,
									CipherAlg, Wcid, SHAREDKEYTABLE);
#ifdef MT_MAC
				if (pAd->chipCap.hif_type == HIF_MT)
					CmdProcAddRemoveKey(pAd, 0, apidx, KeyIdx, Wcid, SHAREDKEYTABLE, &pAd->SharedKey[apidx][KeyIdx], BROADCAST_ADDR);
#endif
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
										pEntry->func_tb_idx,
										(UINT8)KeyIdx,
										pEntry->PairwiseKey.CipherAlg,
										pEntry->wcid,
										PAIRWISEKEYTABLE);
#ifdef MT_MAC										
					if (pAd->chipCap.hif_type == HIF_MT)
						CmdProcAddRemoveKey(pAd, 0, pEntry->func_tb_idx, KeyIdx, pEntry->wcid, PAIRWISEKEYTABLE, &pEntry->PairwiseKey, pEntry->Addr);
#endif

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
    	//UINT32 len;

        /*len = */copy_from_user(&MacAddr, wrq->u.data.pointer, wrq->u.data.length);
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

            /* need to copy the default shared-key to pairwise key table for this entry in 802.1x mode */
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
										pEntry->func_tb_idx,
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
		// TODO: shiang-usw,  remove upper setting becasue we need to migrate to tr_entry!
		pAd->MacTab.tr_entry[pEntry->wcid].NoDataIdleCount = 0;

		pEntry->StaIdleTimeout = pIdleTime->idle_timeout;
		DBGPRINT(RT_DEBUG_TRACE, ("%s : Update Idle-Timeout(%d) from dot1x daemon\n",
									__FUNCTION__, pEntry->StaIdleTimeout));
	}

	return;
}


VOID RTMPIoctlQueryStaAid(
        IN      PRTMP_ADAPTER   pAd,
        IN      RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	DOT1X_QUERY_STA_AID macBuf;
	MAC_TABLE_ENTRY *pEntry = NULL;

	if (wrq->u.data.length != sizeof(DOT1X_QUERY_STA_AID))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s : the length is mis-match\n", __FUNCTION__));
        	return;
	}
	else
	{
		copy_from_user(&macBuf, wrq->u.data.pointer, wrq->u.data.length);
		pEntry = MacTableLookup(pAd, macBuf.StaAddr);

		if (pEntry != NULL)
		{
			wrq->u.data.length = sizeof(DOT1X_QUERY_STA_AID);
			macBuf.aid = pEntry->Aid;
			if (copy_to_user(wrq->u.data.pointer, &macBuf, wrq->u.data.length))
			{
				DBGPRINT(RT_DEBUG_ERROR, ("%s: copy_to_user() fail\n", __FUNCTION__));
			}

			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_DOT1X_QUERY_STA_AID(%02x:%02x:%02x:%02x:%02x:%02x, AID=%d)\n",
						PRINT_MAC(macBuf.StaAddr), macBuf.aid));
		}
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_DOT1X_QUERY_STA_AID(%02x:%02x:%02x:%02x:%02x:%02x, Not Found)\n",
					PRINT_MAC(macBuf.StaAddr)));
		}
	}
}
#endif /* DOT1X_SUPPORT */


#if defined(DBG) ||(defined(BB_SOC)&&defined(CONFIG_ATE))

#ifdef RT65xx
VOID RTMPAPIoctlBBP32(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	RTMP_STRING *this_char, *value, *arg, *ptr;
	UINT32				regBBP = 0;
	RTMP_STRING *mpool, *msg;
	INT					bbpId;
	LONG				bbpValue;
	BOOLEAN				bIsPrintAllBBP = FALSE, bAllowDump, bCopyMsg;
	INT					argLen;


#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlBBP32()\n"));
#endif /* RELEASE_EXCLUDE */

	os_alloc_mem(NULL, (UCHAR **)&mpool, sizeof(CHAR)*(MAX_BBP_MSG_SIZE * 2 +256+12));
	if (mpool == NULL) {
		return;
	}

	NdisZeroMemory(mpool, MAX_BBP_MSG_SIZE * 2 +256+12);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+MAX_BBP_MSG_SIZE * 2+3) & (ULONG)~0x03);

	bAllowDump = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_NODUMPMSG) == RTPRIV_IOCTL_FLAG_NODUMPMSG) ? FALSE : TRUE;
	bCopyMsg = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_NOSPACE) == RTPRIV_IOCTL_FLAG_NOSPACE) ? FALSE : TRUE;
	argLen = strlen((char *)(wrq->u.data.pointer));

	if (argLen > 0)
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
			{
				/*Read */
				if (sscanf(this_char, "%x", &(bbpId)) == 1)
				{
					if ((bbpId <= 0x2fff) && (bbpId >= 0x2000))
					{
							/* according to Andy, Gary, David require. */
							/* the command bbp shall read BBP register directly for dubug. */
							RTMP_BBP_IO_READ32(pAd, bbpId, &regBBP);
							sprintf(msg+strlen(msg), "BBP[0x%04x]:%08x \n", bbpId, regBBP);
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
				if ((sscanf(this_char, "%x", &(bbpId)) == 1) && (sscanf(value, "%lx", &(bbpValue)) == 1))
				{
					DBGPRINT(RT_DEBUG_TRACE, ("bbpID=%04x, value=0x%lx\n", bbpId, bbpValue));
					if ((bbpId <= 0x2fff) && (bbpId >= 0x2000))
					{
						/* according to Andy, Gary, David require. */
						/* the command bbp shall read/write BBP register directly for dubug. */
						RTMP_BBP_IO_WRITE32(pAd, bbpId, bbpValue);
						/*Read it back for showing */
						RTMP_BBP_IO_READ32(pAd, bbpId, &regBBP);
						sprintf(msg+strlen(msg), "BBP[0x%04x]:%08x\n", bbpId, regBBP);
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
		static RTMP_REG_PAIR bbp_regs[]={
			{CORE_R0,	CORE_R44},
			{IBI_R0,	IBI_R15},
			{AGC1_R0,	AGC1_R63},
			{TXC_R0, 	TXC_R1},
			{RXC_R0,	RXC_R9},
			{TXO_R0,	TXO_R13},
			{TXBE_R0,	TXBE_R52},
			{RXFE_R0,	RXFE_R17},
			{RXO_R0,	RXO_R63},
			{DFS_R0,	DFS_R37},
			{TR_R0,     TR_R9},
			{CAL_R0,   CAL_R62},
			{DSC_R0,    DSC_R10},
			{PFMU_R0,   PFMU_R57}
		};
		UINT32 reg, i;

		memset(msg, 0x00, MAX_BBP_MSG_SIZE * 2);
		sprintf(msg, "\n");
		for (i = 0; i < sizeof(bbp_regs) / sizeof(RTMP_REG_PAIR); i++)
		{
			for (reg = bbp_regs[i].Register; reg <= bbp_regs[i].Value; reg += 4)
			{
				RTMP_BBP_IO_READ32(pAd, reg, &regBBP);
				if (strlen(msg) >= (MAX_BBP_MSG_SIZE * 2 - 25))
					break;
				sprintf(msg+strlen(msg), "BBP[0x%04x]:%08x\n", reg, regBBP);
				if (bbpId%5 == 4)
					sprintf(msg+strlen(msg), "\n");
			}
		}
	}
#ifdef RELEASE_EXCLUDE
	else
	{
		DBGPRINT(RT_DEBUG_INFO, ("copy to user:msg[len=%d]=%s\n", strlen(msg), msg));
	}
#endif /* RELEASE_EXCLUDE */

#ifdef LINUX
	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}
#endif /* LINUX */

	if (!bAllowDump)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s\n", msg));
	}

	os_free_mem(NULL, mpool);
	if (!bAllowDump)
		DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlBBP\n\n"));
}
#endif /* RT65xx */


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
	IN	PRTMP_ADAPTER	pAdapter,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
#ifdef RTMP_BBP
	RTMP_STRING *this_char, *value;
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

#ifdef RT65xx
	if (IS_RT65XX(pAdapter)) {
		RTMPAPIoctlBBP32(pAdapter, wrq);
		return;
	}
#endif /* RT65xx */

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

	if (argLen > 0)
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
				DBGPRINT(RT_DEBUG_OFF, ("R%02d[0x%02X]:%02X    "
						, bbpId, bbpId, regBBP));
				if ((bbpId%5 == 4) || (bbpId == pAdapter->chipCap.MaxNumOfBbpId))
					DBGPRINT(RT_DEBUG_OFF, ("\n"));
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


#ifdef MT76x2
VOID RTMPAPIoctlRF_mt76x2(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	CHAR				*value;
	UINT				regRF = 0;
	CHAR				*mpool, *msg; /*msg[2048]; */
	BOOLEAN				bIsPrintAllRF = TRUE;
	UINT				rfidx =0, offset = 0;
	INT					memLen = sizeof(CHAR) * 9000; //(2048+256+12);

	DBGPRINT(RT_DEBUG_TRACE, ("==>RTMPIoctlRF\n"));

	os_alloc_mem(NULL, (UCHAR **)&mpool, memLen);
	if (mpool == NULL) {
		return;
	}

	NdisZeroMemory(mpool, memLen);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);

	if (bIsPrintAllRF)
	{
		RTMPZeroMemory(msg, memLen);
		sprintf(msg, "\n");
		for (rfidx = 0; rfidx <= 1; rfidx++)
		{
			for (offset = 0; offset <= 0x3ff; offset+=4)
			{
				mt76x2_rf_read(pAd, rfidx, offset, &regRF);
				sprintf(msg+strlen(msg), "%d %03x = %08X\n", rfidx, offset, regRF);
			}
			offset = 0xfff;
			mt76x2_rf_read(pAd, rfidx, offset, &regRF);
			sprintf(msg+strlen(msg), "%d %03x = %08X\n", rfidx, offset, regRF);
		}
		RtmpDrvAllRFPrint(NULL, msg, strlen(msg));

		DBGPRINT(RT_DEBUG_TRACE, ("strlen(msg)=%d\n", (UINT32)strlen(msg)));
		/* Copy the information into the user buffer */
#ifdef LINUX
		wrq->u.data.length = strlen("Dump to RFDump.txt");
		if (copy_to_user(wrq->u.data.pointer, "Dump to RFDump.txt", wrq->u.data.length))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
		}
#endif /* LINUX */
	}

	os_free_mem(NULL, mpool);
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlRF\n\n"));
}
#endif /* MT76x2 */

//CFG TODO
#ifndef RT_CFG80211_P2P_SUPPORT
#if defined (MT7603) || defined (MT7628)
VOID RTMPAPIoctlRF_mt7603(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	UINT32				regRF = 0;
	CHAR				*mpool, *msg;
	BOOLEAN				bIsPrintAllRF = TRUE;
	UINT				offset = 0;
	INT					memLen = sizeof(CHAR) * 50000;

	DBGPRINT(RT_DEBUG_TRACE, ("==>RTMPIoctlRF\n"));

	os_alloc_mem(NULL, (UCHAR **)&mpool, memLen);
	if (mpool == NULL) {
		return;
	}

	NdisZeroMemory(mpool, memLen);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);

	if (bIsPrintAllRF)
	{
		RTMPZeroMemory(msg, memLen);
		sprintf(msg, "WF0 : \n");
		for (offset = 0; offset < 0x1000; offset+=4)
		{
			CmdRFRegAccessRead(pAd, (UINT32)0, (UINT32)offset, (UINT32 *)&regRF);
			sprintf(msg+strlen(msg), "0x%08X = 0x%08X\n", offset, regRF);
		}

		sprintf(msg+strlen(msg), "\n");
		sprintf(msg+strlen(msg), "WF1 : \n");
		for (offset = 0; offset < 0x1000; offset+=4)
		{
			CmdRFRegAccessRead(pAd, (UINT32)1, (UINT32)offset, (UINT32 *)&regRF);
			sprintf(msg+strlen(msg), "0x%08X = 0x%08X\n", offset, regRF);
		}

		RtmpDrvAllRFPrint(NULL, msg, strlen(msg));

		DBGPRINT(RT_DEBUG_TRACE, ("strlen(msg)=%d\n", (UINT32)strlen(msg)));
		/* Copy the information into the user buffer */
#ifdef LINUX
		wrq->u.data.length = strlen("Dump to RFDump.txt");
		if (copy_to_user(wrq->u.data.pointer, "Dump to RFDump.txt", wrq->u.data.length))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
		}
#endif /* LINUX */
	}

	os_free_mem(NULL, mpool);
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlRF\n\n"));
}
#endif /* MT7603 */
#endif /* RT_CFG80211_P2P_SUPPORT */

#ifdef RLT_RF
VOID RTMPAPIoctlRF_rlt(RTMP_ADAPTER *pAdapter, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	UCHAR				regRF = 0;
	RTMP_STRING *mpool, *msg;
	RTMP_STRING *arg;
	INT					rfId, maxRFIdx, bank_Id;
	BOOLEAN				bIsPrintAllRF = TRUE, bFromUI;
	INT					memLen = sizeof(CHAR) * (2048+256+12);
	INT					argLen;

	maxRFIdx = pAdapter->chipCap.MaxNumOfRfId;

	DBGPRINT(RT_DEBUG_TRACE, ("==>RTMPIoctlRF (maxRFIdx = %d)\n", maxRFIdx));

	memLen = 12*(maxRFIdx+1)*MAC_RF_BANK;
	os_alloc_mem(NULL, (UCHAR **)&mpool, memLen);
	if (mpool == NULL) {
		return;
	}

	bFromUI = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_UI) == RTPRIV_IOCTL_FLAG_UI) ? TRUE : FALSE;

	NdisZeroMemory(mpool, memLen);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+2048+3) & (ULONG)~0x03);
	argLen = strlen((char *)(wrq->u.data.pointer));
	if (bIsPrintAllRF)
	{
		RTMPZeroMemory(msg, memLen);
		sprintf(msg, "\n");
		for (bank_Id = 0; bank_Id <= MAC_RF_BANK; bank_Id++)
		{
			if (IS_MT76x0(pAdapter))
			{
				if ((bank_Id <=4) && (bank_Id >=1))
					continue;
			}
			for (rfId = 0; rfId <= maxRFIdx; rfId++)
			{
				rlt_rf_read(pAdapter, bank_Id, rfId, &regRF);
				sprintf(msg+strlen(msg), "%d %03d = %02X\n", bank_Id, rfId, regRF);
			}
		}
		RtmpDrvAllRFPrint(NULL, msg, strlen(msg));
		/* Copy the information into the user buffer */
#ifdef RELEASE_EXCLUDE
		if (!bFromUI)
			DBGPRINT(RT_DEBUG_TRACE, ("strlen(msg)=%d\n", (UINT32)strlen(msg)));
#endif /* RELEASE_EXCLUDE */

#ifdef LINUX
		wrq->u.data.length = strlen("Dump to RFDump.txt");
		if (copy_to_user(wrq->u.data.pointer, "Dump to RFDump.txt", wrq->u.data.length))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
		}
#endif /* LINUX */
	}

	os_free_mem(NULL, mpool);
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlRF\n"));
}
#endif /* RLT_RF */


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
               1.) iwpriv ra0 rf		==> read all RF registers
               2.) iwpriv ra0 rf 1		==> read RF where RegID=1
               3.) iwpriv ra0 rf 1=10	==> write RF R1=0x10
    ==========================================================================
*/
VOID RTMPAPIoctlRF(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
#ifdef RT_RF
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
#endif /* RT_RF */

#ifdef RLT_RF
	if (IS_MT7601(pAdapter) || IS_MT76x0(pAdapter)) {
		RTMPAPIoctlRF_rlt(pAdapter, wrq);
		return;
	}
#endif /* RLT_RF */

#ifdef MT76x2
	if (IS_MT76x2(pAdapter)) {
		RTMPAPIoctlRF_mt76x2(pAdapter, wrq);
		return;
	}
#endif /* MT76x2 */

#ifndef RT_CFG80211_P2P_SUPPORT
#if defined (MT7603) || defined (MT7628)
	if (IS_MT7603(pAdapter) || IS_MT7628(pAdapter)) {
		RTMPAPIoctlRF_mt7603(pAdapter, wrq);
		return;
	}
#endif /* MT7603 */
#endif /* RT_CFG80211_P2P_SUPPORT */

#ifdef RT_RF
	maxRFIdx = pAdapter->chipCap.MaxNumOfRfId;

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlRF\n"));
#endif /* RELEASE_EXCLUDE */
	os_alloc_mem(NULL, (UCHAR **)&mpool, memLen);
	if (mpool == NULL) {
		return;
	}

	bFromUI = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_UI) == RTPRIV_IOCTL_FLAG_UI) ? TRUE : FALSE;

	NdisZeroMemory(mpool, memLen);
	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+2048+3) & (ULONG)~0x03);

	if ((wrq->u.data.length > 1) /* No parameters. */
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
#ifdef RELEASE_EXCLUDE
						/*
							In RT2860 ATE mode, we do not load 8051 firmware.
							We must access BBP directly.
							For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
						*/
#endif /* RELEASE_EXCLUDE */
						if (ATE_ON(pAdapter))
						{
#ifdef RT6352
							if (IS_RT6352(pAdapter))
								ATE_RF_IO_READ8_BY_REG_ID(pAdapter, pAdapter->RfBank, rfId, &regRF);
							else
#endif /* RT6352 */
								RT30xxReadRFRegister(pAdapter, rfId, &regRF);

							/* Sync with QA for comparation */
							sprintf(msg+strlen(msg), "%03d = %02X\n", rfId, regRF);
						}
						else
#endif /* CONFIG_ATE */
						{
							/* according to Andy, Gary, David require. */
							/* the command rf shall read rf register directly for dubug. */
							/* BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP); */
#ifdef RT6352
							if (IS_RT6352(pAdapter))
							{
								RT635xReadRFRegister(pAdapter, pAdapter->RfBank, rfId, &regRF);
								sprintf(msg+strlen(msg), "Bank_%02d_R%02d[0x%02x]:%02X  ",
														pAdapter->RfBank, rfId, rfId, regRF);
							}
							else
#endif /* RT6352 */
							{
								RT30xxReadRFRegister(pAdapter, rfId, &regRF);
								sprintf(msg+strlen(msg), "R%02d[0x%02x]:%02X  ", rfId, rfId, regRF);
							}
						}
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
					}
					else
					{
						/* Invalid parametes, so default printk all RF */
						bIsPrintAllRF = TRUE;
						break;
					}
				}
				else
				{
					/* Invalid parametes, so default printk all RF */
					bIsPrintAllRF = TRUE;
					break;
				}
			}
			else
			{ /* Write */
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
#ifdef RELEASE_EXCLUDE
						/*
							In RT2860 ATE mode, we do not load 8051 firmware.
							We must access BBP directly.
							For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
						*/
#endif /* RELEASE_EXCLUDE */
						if (ATE_ON(pAdapter))
						{
#ifdef RT6352
							if (IS_RT6352(pAdapter))
							{
								ATE_RF_IO_READ8_BY_REG_ID(pAdapter, pAdapter->RfBank, rfId, &regRF);
								ATE_RF_IO_WRITE8_BY_REG_ID(pAdapter, pAdapter->RfBank, (UCHAR)rfId,(UCHAR) rfValue);
							}
							else
#endif /* RT6352 */
							{
								RT30xxReadRFRegister(pAdapter, rfId, &regRF);
								RT30xxWriteRFRegister(pAdapter, (UCHAR)rfId,(UCHAR) rfValue);
							}


							/* Read it back for showing. */
#ifdef RT6352
							if (IS_RT6352(pAdapter))
								ATE_RF_IO_READ8_BY_REG_ID(pAdapter, pAdapter->RfBank, rfId, &regRF);
							else
#endif /* RT6352 */
								RT30xxReadRFRegister(pAdapter, rfId, &regRF);

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
#ifdef RT6352
							if (IS_RT6352(pAdapter))
							{
								RT635xReadRFRegister(pAdapter, pAdapter->RfBank, rfId, &regRF);
								RT635xWriteRFRegister(pAdapter, pAdapter->RfBank, (UCHAR)rfId, (UCHAR)rfValue);
								/* Read it back for showing */
								RT635xReadRFRegister(pAdapter, pAdapter->RfBank, rfId, &regRF);
								sprintf(msg+strlen(msg), "Bank_%02d_R%02d[0x%02x]:%02X  ",
														pAdapter->RfBank, rfId, rfId, regRF);

							}
							else
#endif /* RT6352 */
							{
								RT30xxReadRFRegister(pAdapter, rfId, &regRF);
								RT30xxWriteRFRegister(pAdapter, (UCHAR)rfId,(UCHAR) rfValue);
								/* Read it back for showing */
								RT30xxReadRFRegister(pAdapter, rfId, &regRF);
								sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X\n", rfId, rfId, regRF);
							}
#ifdef RELEASE_EXCLUDE
							DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
#endif /* RELEASE_EXCLUDE */
						}
					}
					else
					{	/* Invalid parametes, so default printk all RF */
						bIsPrintAllRF = TRUE;
						break;
					}
				}
				else
				{	/* Invalid parametes, so default printk all RF */
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
#ifdef RT6352
				if (IS_RT6352(pAdapter))
					ATE_RF_IO_READ8_BY_REG_ID(pAdapter, pAdapter->RfBank, rfId, &regRF);
				else
#endif /* RT6352 */
					RT30xxReadRFRegister(pAdapter, rfId, &regRF);

				/* Sync with QA for comparation */
				sprintf(msg+strlen(msg), "%03d = %02X\n", rfId, regRF);
			}
			else
#endif /* CONFIG_ATE */
			{
#ifdef __ECOS
				RT30xxReadRFRegister(pAdapter, rfId, &regRF);
				DBGPRINT(RT_DEBUG_OFF, ("R%02d[0x%02X]:%02X    ", rfId, rfId*2, regRF));
				if ((rfId%5 == 4) || (rfId == maxRFIdx))
					DBGPRINT(RT_DEBUG_OFF, ("\n"));
#else
				/* according to Andy, Gary, David require. */
				/* the command RF shall read/write RF register directly for dubug. */
#ifdef RT6352
				if (IS_RT6352(pAdapter))
				{
					RT635xReadRFRegister(pAdapter, pAdapter->RfBank, rfId, &regRF);
					sprintf(msg+strlen(msg), "Bank_%02d_R%02d[0x%02X]:%02X    ", pAdapter->RfBank, rfId, rfId*2, regRF);
					if (rfId%4 == 3)
						sprintf(msg+strlen(msg), "\n");
				}
				else
#endif /* RT6352 */
				{
					RT30xxReadRFRegister(pAdapter, rfId, &regRF);
					sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X    ", rfId, rfId*2, regRF);
					if (rfId%5 == 4)
						sprintf(msg+strlen(msg), "\n");
				}
#endif /* __ECOS */
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
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Dump RF msg[%d]=\n", (UINT32)strlen(msg)));
		DBGPRINT(RT_DEBUG_OFF, ("%s\n", msg));
	}

	os_free_mem(NULL, mpool);
	if (!bFromUI)
		DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlRF\n\n"));
#endif /* RT_RF */

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
	RTMP_STRING *this_char, *value;
	INT					j = 0, k = 0;
	RTMP_STRING *mpool, *msg;/*msg[1024]; */
	RTMP_STRING *arg, *ptr;
	USHORT				eepAddr = 0;
	UCHAR				temp[16];
	RTMP_STRING temp2[16];
	USHORT				eepValue;
	BOOLEAN				bIsPrintAllE2PROM = FALSE;

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlE2PROM\n"));
#endif /* RELEASE_EXCLUDE */
	os_alloc_mem(NULL, (UCHAR **)&mpool, sizeof(CHAR)*(4096+256+12));

	if (mpool == NULL) {
		return;
	}

	msg = (RTMP_STRING *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg+4096+3) & (ULONG)~0x03);


	memset(msg, 0x00, 4096);
	memset(arg, 0x00, 256);

	if (
#ifdef __ECOS
		(wrq->u.data.length > 0) /*No parameters. */
#endif /* __ECOS */
#ifdef LINUX
		(wrq->u.data.length > 1) /* If no parameter, dump all e2p. */
#endif /* LINUX */
#ifdef VXWORKS
		(wrq->u.data.length > 1) && (!(wrq->u.data.length == 4096 && wrq->u.data.flags== RT_OID_802_11_HARDWARE_REGISTER))
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
						goto done; /* return; */
				}
				j = strlen(temp2);
				while(j-- > 0)
				{
					if(temp2[j] > 'f' || temp2[j] < '0')
						goto done; /* return; */
				}

				/* MAC Addr */
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}

				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				/* MAC value */
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
	}
	else
	{
		bIsPrintAllE2PROM = TRUE;
	}

	if (bIsPrintAllE2PROM)
	{
#ifdef __ECOS
		for (eepAddr = 0x00; eepAddr < 0x200; eepAddr += 2)
		{
			RT28xx_EEPROM_READ16(pAdapter, eepAddr, eepValue);
			DBGPRINT(RT_DEBUG_OFF, ("[0x%04X]:%04X  ", eepAddr, eepValue));
			if ((eepAddr & 0x6) == 0x6)
				DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
#else
		sprintf(msg, "\n");

		/* E2PROM Registers */
		for (eepAddr = 0x00; eepAddr < 0x200; eepAddr += 2)
		{
			RT28xx_EEPROM_READ16(pAdapter, eepAddr, eepValue);
			sprintf(msg+strlen(msg), "[0x%04X]:%04X  ", eepAddr , eepValue);
			if ((eepAddr & 0x6) == 0x6)
				sprintf(msg+strlen(msg), "\n");
		}
#endif /* __ECOS */
	}

	if(strlen(msg) == 1)
		sprintf(msg+strlen(msg), "===>Error command format!");

	/* Copy the information into the user buffer */
#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
#endif /* RELEASE_EXCLUDE */

	AP_E2PROM_IOCTL_PostCtrl(wrq, msg);

done:
	os_free_mem(NULL, mpool);
    if (wrq->u.data.flags != RT_OID_802_11_HARDWARE_REGISTER)
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlE2PROM\n"));
}


//#define ENHANCED_STAT_DISPLAY	// Display PER and PLR statistics


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
VOID RTMPIoctlStatistics(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	ULONG Status;
	RTMP_STRING *msg;
#ifdef WSC_AP_SUPPORT
    UCHAR idx = 0;
#endif /* WSC_AP_SUPPORT */
#ifdef P2P_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#endif /* P2P_SUPPORT */
	ULONG txCount = 0;
#ifdef MT_MAC
	//ULONG txFailCount = 0;
#endif /* MT_MAC */
	UINT32 rxCount = 0;
#ifdef ENHANCED_STAT_DISPLAY
	ULONG per, plr;
	INT i;
#endif
#ifdef RTMP_EFUSE_SUPPORT
	UINT efusefreenum=0;
#endif /* RTMP_EFUSE_SUPPORT */

#ifdef BB_SOC
	ULONG txPackets=0, rxPackets=0, txBytes=0, rxBytes=0;
	UCHAR index=0;
#endif

	os_alloc_mem(pAd, (UCHAR **)&msg, sizeof(CHAR)*(2048));
	if (msg == NULL) {
		return;
	}


    memset(msg, 0x00, 1600);
    sprintf(msg, "\n");

#ifdef CONFIG_ATE
	if(ATE_ON(pAd))
	{
#ifdef CONFIG_QA
		txCount = pAd->ATECtrl.TxDoneCount;
		//rxCount = pAd->ATECtrl.U2M + pAd->ATECtrl.OtherData + pAd->ATECtrl.OtherCount;
        rxCount = pAd->ATECtrl.RxTotalCnt;
#else
        txCount = pAd->ATECtrl.TxDoneCount;
		rxCount = pAd->ATECtrl.RxTotalCnt;
#endif
	}
	else
#endif /* CONFIG_ATE */
	{
		txCount = pAd->WlanCounters.TransmittedFragmentCount.u.LowPart;
		rxCount = (UINT32)pAd->WlanCounters.ReceivedFragmentCount.QuadPart;
	}

    sprintf(msg+strlen(msg), "Tx success                      = %ld\n", txCount);
#ifdef ENHANCED_STAT_DISPLAY
	if (pAd->chipCap.hif_type == HIF_MT) {
	per = txCount==0? 0: 1000*(pAd->WlanCounters.FailedCount.u.LowPart)/(pAd->WlanCounters.FailedCount.u.LowPart+txCount);
    sprintf(msg+strlen(msg), "Tx fail count                   = %ld, PER=%ld.%1ld%%\n",
									(ULONG)pAd->WlanCounters.FailedCount.u.LowPart,
									per/10, per % 10);
	} else {
	per = txCount==0? 0: 1000*(pAd->WlanCounters.RetryCount.u.LowPart+pAd->WlanCounters.FailedCount.u.LowPart)/(pAd->WlanCounters.RetryCount.u.LowPart+pAd->WlanCounters.FailedCount.u.LowPart+txCount);
    sprintf(msg+strlen(msg), "Tx retry count                  = %ld, PER=%ld.%1ld%%\n",
									(ULONG)pAd->WlanCounters.RetryCount.u.LowPart,
									per/10, per % 10);
	plr = txCount==0? 0: 10000*pAd->WlanCounters.FailedCount.u.LowPart/(pAd->WlanCounters.FailedCount.u.LowPart+txCount);
    sprintf(msg+strlen(msg), "Tx fail to Rcv ACK after retry  = %ld, PLR=%ld.%02ld%%\n",
									(ULONG)pAd->WlanCounters.FailedCount.u.LowPart, plr/100, plr%100);
	}
    sprintf(msg+strlen(msg), "Rx success                      = %ld\n", (ULONG)rxCount);
#ifdef CONFIG_QA
	if(ATE_ON(pAd))
	per = rxCount==0? 0: 1000*(pAd->WlanCounters.FCSErrorCount.u.LowPart)/(pAd->WlanCounters.FCSErrorCount.u.LowPart+rxCount);
	else
#endif /* CONFIG_QA */
	per = pAd->WlanCounters.ReceivedFragmentCount.u.LowPart==0? 0: 1000*(pAd->WlanCounters.FCSErrorCount.u.LowPart)/(pAd->WlanCounters.FCSErrorCount.u.LowPart+pAd->WlanCounters.ReceivedFragmentCount.u.LowPart);
	sprintf(msg+strlen(msg), "Rx with CRC                     = %ld, PER=%ld.%1ld%%\n",
									(ULONG)pAd->WlanCounters.FCSErrorCount.u.LowPart, per/10, per % 10);
	sprintf(msg+strlen(msg), "Rx with PhyErr                  = %ld\n",
									(ULONG)pAd->RalinkCounters.PhyErrCnt);
	sprintf(msg+strlen(msg), "Rx with PlcpErr                 = %ld\n",
									(ULONG)pAd->RalinkCounters.PlcpErrCnt);
	sprintf(msg+strlen(msg), "Rx drop due to out of resource  = %ld\n", (ULONG)pAd->Counters8023.RxNoBuffer);
	sprintf(msg+strlen(msg), "Rx duplicate frame              = %ld\n", (ULONG)pAd->WlanCounters.FrameDuplicateCount.u.LowPart);

	sprintf(msg+strlen(msg), "False CCA                       = %ld\n", (ULONG)pAd->RalinkCounters.FalseCCACnt);
#else
    sprintf(msg+strlen(msg), "Tx retry count                  = %ld\n", (ULONG)pAd->WlanCounters.RetryCount.u.LowPart);
    sprintf(msg+strlen(msg), "Tx fail to Rcv ACK after retry  = %ld\n", (ULONG)pAd->WlanCounters.FailedCount.u.LowPart);
    sprintf(msg+strlen(msg), "RTS Success Rcv CTS             = %ld\n", (ULONG)pAd->WlanCounters.RTSSuccessCount.u.LowPart);
    sprintf(msg+strlen(msg), "RTS Fail Rcv CTS                = %ld\n", (ULONG)pAd->WlanCounters.RTSFailureCount.u.LowPart);

    sprintf(msg+strlen(msg), "Rx success                      = %ld\n", (ULONG)pAd->WlanCounters.ReceivedFragmentCount.QuadPart);
    sprintf(msg+strlen(msg), "Rx with CRC                     = %ld\n", (ULONG)pAd->WlanCounters.FCSErrorCount.u.LowPart);
    sprintf(msg+strlen(msg), "Rx drop due to out of resource  = %ld\n", (ULONG)pAd->Counters8023.RxNoBuffer);
    sprintf(msg+strlen(msg), "Rx duplicate frame              = %ld\n", (ULONG)pAd->WlanCounters.FrameDuplicateCount.u.LowPart);

    sprintf(msg+strlen(msg), "False CCA (one second)          = %ld\n", (ULONG)pAd->RalinkCounters.OneSecFalseCCACnt);
#endif /* ENHANCED_STAT_DISPLAY */

#ifdef CONFIG_QA
	if(ATE_ON(pAd))
	{
		if (pAd->ATECtrl.RxAntennaSel == 0)
		{
    		sprintf(msg+strlen(msg), "RSSI-A                          = %ld\n", (LONG)(pAd->ATECtrl.LastRssi0 - pAd->BbpRssiToDbmDelta));
			sprintf(msg+strlen(msg), "RSSI-B (if available)           = %ld\n", (LONG)(pAd->ATECtrl.LastRssi1 - pAd->BbpRssiToDbmDelta));
			sprintf(msg+strlen(msg), "RSSI-C (if available)           = %ld\n\n", (LONG)(pAd->ATECtrl.LastRssi2 - pAd->BbpRssiToDbmDelta));
		}
		else
		{
    		sprintf(msg+strlen(msg), "RSSI                            = %ld\n", (LONG)(pAd->ATECtrl.LastRssi0 - pAd->BbpRssiToDbmDelta));
		}
		sprintf(msg+strlen(msg), "Rx U2M                          = %ld\n", (ULONG)pAd->ATECtrl.U2M);
		sprintf(msg+strlen(msg), "Rx other Data                   = %ld\n", (ULONG)pAd->ATECtrl.OtherData);
		sprintf(msg+strlen(msg), "Rx others(Mgmt+Cntl)            = %ld\n", (ULONG)pAd->ATECtrl.OtherCount);
	}
	else
#endif /* CONFIG_QA */
	{
#ifdef ENHANCED_STAT_DISPLAY
	sprintf(msg+strlen(msg), "RSSI                            = %ld %ld %ld\n",
    			(LONG)(pAd->ApCfg.RssiSample.LastRssi[0] - pAd->BbpRssiToDbmDelta),
    			(LONG)(pAd->ApCfg.RssiSample.LastRssi[1] - pAd->BbpRssiToDbmDelta),
    			(LONG)(pAd->ApCfg.RssiSample.LastRssi[2] - pAd->BbpRssiToDbmDelta));

    	/* Display Last Rx Rate and BF SNR of first Associated entry in MAC table */
    	if (pAd->MacTab.Size > 0)
    	{
    		static char *phyMode[5] = {"CCK", "OFDM", "MM", "GF", "VHT"};
#ifdef RT65xx
			static char *bw[3] = {"20M", "40M", "80M"};
			static char *fec_coding[2] = {"bcc", "ldpc"};
#endif /* RT65xx */

#if 0
		// TODO: shiang-usw, these code used for reference only!
#ifdef MT76x0
		if (IS_MT76x0(pAd))
		{
			RXWI_STRUC *pRxWI;
			pEntry->LastRxRate = (ULONG)((pRxWI->RXWI_N.mcs) + (pRxWI->RXWI_N.bw <<7) +
										(pRxWI->RXWI_N.sgi <<9) + (pRxWI->RXWI_N.stbc <<10) +
										(pRxWI->RXWI_N.phy_mode <<14));
		}
		else
#else
		{
			RXWI_STRUC *pRxWI;
			pEntry->LastRxRate = (ULONG)((pRxWI->RXWI_O.mcs) + (pRxWI->RXWI_O.bw <<7) +
									(pRxWI->RXWI_O.sgi<<8) + (pRxWI->RXWI_O.stbc <<9) +
									(pRxWI->RXWI_O.phy_mode<<14));
		}
#endif
#endif
    		for (i=1; i<MAX_LEN_OF_MAC_TABLE; i++)
			{
    			PMAC_TABLE_ENTRY pEntry = &(pAd->MacTab.Content[i]);
    			if (IS_ENTRY_CLIENT(pEntry) && pEntry->Sst==SST_ASSOC)
				{
					//sprintf(msg+strlen(msg), "sta mac: %02x:%02x:%02x:%02x:%02x:%02x\n", pEntry->wdev->if_addr[0], pEntry->wdev->if_addr[1],  pEntry->wdev->if_addr[2],  pEntry->wdev->if_addr[3],  pEntry->wdev->if_addr[4],  pEntry->wdev->if_addr[5]);
					UINT32 lastRxRate = pEntry->LastRxRate;
					UINT32 lastTxRate = pEntry->LastTxRate;
#ifdef RT65xx
					if (IS_RT65XX(pAd)) {
						if (((lastTxRate >> 13) & 0x7) == 0x04) {
							sprintf(msg+strlen(msg), "Last TX Rate                    = MCS%d, %dSS, %s, %s, %cGI, %s%s\n",
								lastTxRate & 0x0F,
								(((lastTxRate >> 4) & 0x3) + 1),
								fec_coding[((lastTxRate >> 6) & 0x1)],
								bw[((lastTxRate >> 7) & 0x3)],
								((lastTxRate >> 9) & 0x1)? 'S': 'L',
								phyMode[(lastTxRate >> 13) & 0x7],
								((lastTxRate >> 10) & 0x3)? ", STBC": " ");
						} else {
							sprintf(msg+strlen(msg), "Last TX Rate                    = MCS%d, %s, %s, %cGI, %s%s\n",
								lastTxRate & 0x3F,
								fec_coding[((lastTxRate >> 6) & 0x1)],
								bw[((lastTxRate >> 7) & 0x3)],
								((lastTxRate >> 9) & 0x1)? 'S': 'L',
								phyMode[(lastTxRate >> 13) & 0x7],
								((lastTxRate >> 10) & 0x3)? ", STBC": " ");
						}

						if (((lastRxRate >> 13) & 0x7) == 0x04) {
							sprintf(msg+strlen(msg), "Last RX Rate                    = MCS%d, %dSS, %s, %s, %cGI, %s%s\n",
								lastRxRate & 0x0F,
								(((lastRxRate >> 4) & 0x3) + 1),
								fec_coding[((lastRxRate >> 6) & 0x1)],
								bw[((lastRxRate >> 7) & 0x3)],
								((lastRxRate >> 9) & 0x1)? 'S': 'L',
								phyMode[(lastRxRate >> 13) & 0x7],
								((lastRxRate >> 10) & 0x3)? ", STBC": " ");
						} else {
							sprintf(msg+strlen(msg), "Last RX Rate                    = MCS%d, %s, %s, %cGI, %s%s\n",
								lastRxRate & 0x3F,
								fec_coding[((lastRxRate >> 6) & 0x1)],
								bw[((lastRxRate >> 7) & 0x3)],
								((lastRxRate >> 9) & 0x1)? 'S': 'L',
								phyMode[(lastRxRate >> 13) & 0x7],
								((lastRxRate >> 10) & 0x3)? ", STBC": " ");
						}
					}
					else
#endif /* RT65xx */
#ifdef MT_MAC
					if (pAd->chipCap.hif_type == HIF_MT)  {
						StatRateToString(pAd, msg, 0, lastTxRate);
						StatRateToString(pAd, msg, 1, lastRxRate);
					}
					else
#endif /* MT_MAC */
					{
						sprintf(msg+strlen(msg), "Last TX Rate                    = MCS%d, %2dM, %cGI, %s%s\n",
							lastTxRate & 0x7F,  ((lastTxRate>>7) & 0x1)? 40: 20,
							((lastTxRate>>8) & 0x1)? 'S': 'L',
							phyMode[(lastTxRate>>14) & 0x3],
							((lastTxRate>>9) & 0x3)? ", STBC": " ");
						sprintf(msg+strlen(msg), "Last RX Rate                    = MCS%d, %2dM, %cGI, %s%s\n",
							lastRxRate & 0x7F,  ((lastRxRate>>7) & 0x1)? 40: 20,
							((lastRxRate>>8) & 0x1)? 'S': 'L',
							phyMode[(lastRxRate>>14) & 0x3],
							((lastRxRate>>9) & 0x3)? ", STBC": " ");
					}

#if defined(RT2883) || defined(RT3883)
					sprintf(msg+strlen(msg), "BF SNR                          = %d.%02d, %d.%02d, %d.%02d  FO:%02X\n",
							pEntry->BF_SNR[0]/4, (pEntry->BF_SNR[0] % 4)*25,
							pEntry->BF_SNR[1]/4, (pEntry->BF_SNR[1] % 4)*25,
							pEntry->BF_SNR[2]/4, (pEntry->BF_SNR[2] % 4)*25,
							pEntry->freqOffset & 0xFF);
#endif /* defined(RT2883) || defined(RT3883) */
					break;
				}
			}
    	}
#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT)
	{
		sprintf(msg+strlen(msg), "TX AGG Range 1 (1)              = %ld\n", (LONG)(pAd->WlanCounters.TxAggRange1Count.u.LowPart));
		sprintf(msg+strlen(msg), "TX AGG Range 2 (2~5)            = %ld\n", (LONG)(pAd->WlanCounters.TxAggRange2Count.u.LowPart));
		sprintf(msg+strlen(msg), "TX AGG Range 3 (6~15)           = %ld\n", (LONG)(pAd->WlanCounters.TxAggRange3Count.u.LowPart));
		sprintf(msg+strlen(msg), "TX AGG Range 4 (>15)            = %ld\n", (LONG)(pAd->WlanCounters.TxAggRange4Count.u.LowPart));
		{
			ULONG mpduTXCount;

			mpduTXCount = pAd->WlanCounters.AmpduSuccessCount.u.LowPart;

	    		sprintf(msg+strlen(msg), "AMPDU Tx success                = %ld\n", mpduTXCount);

			per = mpduTXCount==0? 0: 1000*(pAd->WlanCounters.AmpduFailCount.u.LowPart)/(pAd->WlanCounters.AmpduFailCount.u.LowPart+mpduTXCount);
			sprintf(msg+strlen(msg), "AMPDU Tx fail count             = %ld, PER=%ld.%1ld%%\n",
										(ULONG)pAd->WlanCounters.AmpduFailCount.u.LowPart,
										per/10, per % 10);
		}

        if (pAd->CommonCfg.bTXRX_RXV_ON) {
            sprintf(msg+strlen(msg), "/* Condition Number should enable mode4 of 0x6020_426c */\n");
            sprintf(msg+strlen(msg),
                    "--10 packets Condition Number   = [%d|%d|%d|%d|%d|%d|%d|%d|%d|%d]\n",
                    (UINT8)(pAd->rxv2_cyc3[0] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[1] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[2] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[3] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[4] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[5] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[6] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[7] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[8] & 0xff),
                    (UINT8)(pAd->rxv2_cyc3[9] & 0xff)
                   );
        }
	}
#endif /* MT_MAC */

 #else
    	sprintf(msg+strlen(msg), "RSSI-A                          = %ld\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi[0] - pAd->BbpRssiToDbmDelta));
		sprintf(msg+strlen(msg), "RSSI-B (if available)           = %ld\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi[1] - pAd->BbpRssiToDbmDelta));
		sprintf(msg+strlen(msg), "RSSI-C (if available)           = %ld\n\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi[2] - pAd->BbpRssiToDbmDelta));
#endif /* ENHANCED_STAT_DISPLAY */
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
#ifdef BB_SOC
	for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++){
//		sprintf(msg+strlen(msg), "Enrollee PinCode(ra%d)          = %08u\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCode);
//		sprintf(msg+strlen(msg), "WPS Query Status(ra%d)        = %d\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.WscStatus);
		sprintf(msg+strlen(msg), "WPS Wsc2MinsTimerRunning(ra%d)        = %d\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.Wsc2MinsTimerRunning);
	}
#else
	for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++)
		{
			/* display pin code */
			if (pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCodeLen == 8)
				sprintf(msg+strlen(msg), "Enrollee PinCode(ra%d)           %08u\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCode);
			else
				sprintf(msg+strlen(msg), "Enrollee PinCode(ra%d)           %04u\n", idx, pAd->ApCfg.MBSSID[idx].WscControl.WscEnrolleePinCode);
		}
#endif
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

#if /*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_SCHEDULE)
	if(pAd->Flags & fRTMP_ADAPTER_RADIO_OFF)
		sprintf(msg+strlen(msg), "Enable Wireless LAN		= %s\n", "0");
	else
		sprintf(msg+strlen(msg), "Enable Wireless LAN		= %s\n", "1");
	sprintf(msg+strlen(msg), "\n");
#endif /*TCSUPPORT_COMPILE*/

/*
 * Let "iwpriv ra0 stat" can print out Tx/Rx Packet and Byte count.
 * Therefore, we can parse them out in cfg_manager. --Trey */
#ifdef BB_SOC
	for (index = 0; index < pAd->ApCfg.BssidNum; index++){
		rxPackets += (ULONG)pAd->ApCfg.MBSSID[index].RxCount;
		txPackets += (ULONG)pAd->ApCfg.MBSSID[index].TxCount;
		rxBytes += (ULONG)pAd->ApCfg.MBSSID[index].ReceivedByteCount;
		txBytes += (ULONG)pAd->ApCfg.MBSSID[index].TransmittedByteCount;
	}
	sprintf(msg+strlen(msg), "Packets Received       = %lu\n", rxPackets);
	sprintf(msg+strlen(msg), "Packets Sent           = %lu\n", txPackets);
	sprintf(msg+strlen(msg), "Bytes Received         = %lu\n", rxBytes);
	sprintf(msg+strlen(msg), "Bytes Sent             = %lu\n", txBytes);
	sprintf(msg+strlen(msg), "\n");
#endif

#ifdef RTMP_EFUSE_SUPPORT
	if (pAd->bUseEfuse == TRUE)
	{
		eFuseGetFreeBlockCount(pAd, &efusefreenum);
		sprintf(msg+strlen(msg), "efuseFreeNumber                 = %d\n", efusefreenum);
	}
#endif /* RTMP_EFUSE_SUPPORT */

#ifdef CONFIG_HOTSPOT
	{
		PHOTSPOT_CTRL pHSCtrl;
		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
		UCHAR apidx = pObj->ioctl_if;
		
#ifdef CONFIG_DOT11V_WNM		
		PWNM_CTRL pWNMCtrl = &pAd->ApCfg.MBSSID[apidx].WNMCtrl;
#endif
		pHSCtrl = &pAd->ApCfg.MBSSID[apidx].HotSpotCtrl;
		PGAS_CTRL pGASCtrl = &pAd->ApCfg.MBSSID[apidx].GASCtrl;
    	sprintf(msg+strlen(msg), "\n");
    	sprintf(msg+strlen(msg), "hotspot enable					= %d\n", pHSCtrl->HotSpotEnable);
    	sprintf(msg+strlen(msg), "daemon ready					= %d\n", pHSCtrl->HSDaemonReady);
    	sprintf(msg+strlen(msg), "hotspot DGAFDisable				= %d\n", pHSCtrl->DGAFDisable);
    	sprintf(msg+strlen(msg), "hotspot L2Filter				= %d\n", pHSCtrl->L2Filter);
    	sprintf(msg+strlen(msg), "hotspot ICMPv4Deny				= %d\n", pHSCtrl->ICMPv4Deny);
    	sprintf(msg+strlen(msg), "hotspot QosMapEnable				= %d\n", pHSCtrl->QosMapEnable);
#ifdef CONFIG_DOT11V_WNM    	
    	sprintf(msg+strlen(msg), "proxy arp enable				= %d\n", pWNMCtrl->ProxyARPEnable);
    	sprintf(msg+strlen(msg), "WNMNotify enable				= %d\n", pWNMCtrl->WNMNotifyEnable);
#endif    
		sprintf(msg+strlen(msg), "hotspot OSEN enable              		= %d\n", pHSCtrl->bASANEnable);
		sprintf(msg+strlen(msg), "GAS come back delay   			       	= %d\n", pGASCtrl->cb_delay);
		sprintf(msg+strlen(msg), "\n");
    }
#endif
        
    /* Copy the information into the user buffer */
#ifdef RELEASE_EXCLUDE
    DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
#endif /* RELEASE_EXCLUDE */
	wrq->u.data.length = (UINT16)strlen(msg);
    Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);

	os_free_mem(NULL, msg);

#if defined(TXBF_SUPPORT) && defined(ENHANCED_STAT_DISPLAY)
#ifdef DBG_CTRL_SUPPORT
	/* Debug code to display BF statistics */
	if (pAd->CommonCfg.DebugFlags & DBF_SHOW_BF_STATS)
	{
		for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++) {
			PMAC_TABLE_ENTRY pEntry = &(pAd->MacTab.Content[i]);
			COUNTER_TXBF *pCnt;
			ULONG totalNBF, totalEBF, totalIBF, totalTx, totalRetry, totalSuccess;

			if (!IS_ENTRY_CLIENT(pEntry) || pEntry->Sst!=SST_ASSOC)
				continue;

			pCnt = &pEntry->TxBFCounters;

			totalNBF = pCnt->TxSuccessCount + pCnt->TxFailCount;
			totalEBF = pCnt->ETxSuccessCount + pCnt->ETxFailCount;
			totalIBF = pCnt->ITxSuccessCount + pCnt->ITxFailCount;

			totalTx = totalNBF + totalEBF + totalIBF;
			totalRetry = pCnt->TxRetryCount + pCnt->ETxRetryCount + pCnt->ITxRetryCount;
			totalSuccess = pCnt->TxSuccessCount + pCnt->ETxSuccessCount + pCnt->ITxSuccessCount;

			DBGPRINT(RT_DEBUG_OFF, ("MacTable[%d]     Success    Retry/PER    Fail/PLR\n", i) );
			if (totalTx==0) {
				DBGPRINT(RT_DEBUG_OFF, ("   Total = 0\n") );
				continue;
			}

			if (totalNBF!=0) {
				DBGPRINT(RT_DEBUG_OFF, ("   NonBF (%3lu%%): %7lu  %7lu (%2lu%%) %5lu (%1lu%%)\n",
					100*totalNBF/totalTx, pCnt->TxSuccessCount,
					pCnt->TxRetryCount, 100*pCnt->TxRetryCount/(pCnt->TxSuccessCount+pCnt->TxRetryCount),
					pCnt->TxFailCount, 100*pCnt->TxFailCount/totalNBF) );
			}

			if (totalEBF!=0) {
				DBGPRINT(RT_DEBUG_OFF, ("   ETxBF (%3lu%%): %7lu  %7lu (%2lu%%) %5lu (%1lu%%)\n",
					 100*totalEBF/totalTx, pCnt->ETxSuccessCount,
					pCnt->ETxRetryCount, 100*pCnt->ETxRetryCount/(pCnt->ETxSuccessCount+pCnt->ETxRetryCount),
					pCnt->ETxFailCount, 100*pCnt->ETxFailCount/totalEBF) );
			}

			if (totalIBF!=0) {
				DBGPRINT(RT_DEBUG_OFF, ("   ITxBF (%3lu%%): %7lu  %7lu (%2lu%%) %5lu (%1lu%%)\n",
					100*totalIBF/totalTx, pCnt->ITxSuccessCount,
					pCnt->ITxRetryCount, 100*pCnt->ITxRetryCount/(pCnt->ITxSuccessCount+pCnt->ITxRetryCount),
					pCnt->ITxFailCount, 100*pCnt->ITxFailCount/totalIBF) );
			}

			DBGPRINT(RT_DEBUG_OFF, ("   Total         %7lu  %7lu (%2lu%%) %5lu (%1lu%%)\n",
				totalSuccess, totalRetry, 100*totalRetry/(totalSuccess + totalRetry),
				pCnt->TxFailCount+pCnt->ETxFailCount+pCnt->ITxFailCount,
				100*(pCnt->TxFailCount+pCnt->ETxFailCount+pCnt->ITxFailCount)/totalTx) );
		}
	}
#endif /* DBG_CTRL_SUPPORT */
#endif /* defined(TXBF_SUPPORT) && defined(ENHANCED_STAT_DISPLAY) */

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
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	/*char *msg; */
	UINT16	TotalEntry;
	UCHAR	i, j, index;
	QUERYBA_TABLE		*BAT;

	BAT = vmalloc(sizeof(QUERYBA_TABLE));

	if (BAT == NULL)
		return;

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

	os_free_mem(NULL, msg);
#endif
}
#endif /* DOT11_N_SUPPORT */


#ifdef APCLI_SUPPORT
INT Set_ApCli_Enable_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	UINT Enable;
	POS_COOKIE pObj;
	INT ifIndex;

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


INT Set_ApCli_Ssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	INT ifIndex;
	BOOLEAN apcliEn;
	INT success = FALSE;
	/*UCHAR keyMaterial[40]; */
	UCHAR PskKey[100];
	APCLI_STRUCT *apcli_entry;
	struct wifi_dev *wdev;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	if(strlen(arg) <= MAX_LEN_OF_SSID)
	{
		apcli_entry = &pAd->ApCfg.ApCliTab[ifIndex];
		wdev = &apcli_entry->wdev;

		/* bring apcli interface down first */
		apcliEn = apcli_entry->Enable;
		if(apcliEn == TRUE )
		{
			apcli_entry->Enable = FALSE;
			ApCliIfDown(pAd);
		}

		NdisZeroMemory(apcli_entry->CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(apcli_entry->CfgSsid, arg, strlen(arg));
		apcli_entry->CfgSsidLen = (UCHAR)strlen(arg);
		success = TRUE;

		/* Upadte PMK and restart WPAPSK state machine for ApCli link */
		if (((wdev->AuthMode == Ndis802_11AuthModeWPAPSK) ||
			(wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)) &&
			apcli_entry->PSKLen > 0)
		{
			NdisZeroMemory(PskKey, 100);
			NdisMoveMemory(PskKey, apcli_entry->PSK, apcli_entry->PSKLen);

#if 0
			if ((strlen((RTMP_STRING *) PskKey) >= 8) && (strlen((RTMP_STRING *) PskKey) < 64))
			{
	    		RtmpPasswordHash((char *)PskKey, (PUCHAR)apcli_entry->CfgSsid, apcli_entry->CfgSsidLen, keyMaterial);
	    		NdisMoveMemory(apcli_entry->PMK, keyMaterial, 32);
			}
			else if ((INT) strlen((RTMP_STRING *) PskKey) == 64)
			{
				AtoH((RTMP_STRING *) PskKey, apcli_entry->PMK, 32);
			}
#else
			RT_CfgSetWPAPSKKey(pAd, (RTMP_STRING *)PskKey,
									apcli_entry->PSKLen,
									(PUCHAR)apcli_entry->CfgSsid,
									apcli_entry->CfgSsidLen,
									apcli_entry->PMK);
#endif
		}

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_Ssid_Proc::(Len=%d,Ssid=%s)\n",
				ifIndex, apcli_entry->CfgSsidLen, apcli_entry->CfgSsid));

		apcli_entry->Enable = apcliEn;
	}
	else
		success = FALSE;

	return success;
}


INT Set_ApCli_Bssid_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	INT i;
	RTMP_STRING *value;
	INT ifIndex;
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
	ULONG i;
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex;
	struct wifi_dev *wdev;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = (UCHAR)pObj->ioctl_if;
	wdev = &pAd->ApCfg.ApCliTab[ifIndex].wdev;

	if (rtstrcasecmp(arg, "WEPAUTO") == TRUE)
		wdev->AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if (rtstrcasecmp(arg, "SHARED") == TRUE)
		wdev->AuthMode = Ndis802_11AuthModeShared;
	else if (rtstrcasecmp(arg, "WPAPSK") == TRUE)
		wdev->AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((rtstrcasecmp(arg, "WPA2PSK") == TRUE) || (rtstrcasecmp(arg, "WPAPSKWPA2PSK") == TRUE))
		wdev->AuthMode = Ndis802_11AuthModeWPA2PSK;
#ifdef WPA_SUPPLICANT_SUPPORT
   	else if (rtstrcasecmp(arg, "WPA") == TRUE)
       	wdev->AuthMode = Ndis802_11AuthModeWPA;
    	else if (rtstrcasecmp(arg, "WPA2") == TRUE)
        	wdev->AuthMode = Ndis802_11AuthModeWPA2;
#endif /*WPA_SUPPLICANT_SUPPORT */

	else
		wdev->AuthMode = Ndis802_11AuthModeOpen;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (IS_ENTRY_APCLI(&pAd->MacTab.Content[i]))
		{
			pAd->MacTab.tr_entry[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}

	RTMPMakeRSNIE(pAd, wdev->AuthMode, wdev->WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));

	wdev->DefaultKeyId  = 0;

	if(wdev->AuthMode >= Ndis802_11AuthModeWPA)
		wdev->DefaultKeyId = 1;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_AuthMode_Proc::(AuthMode=%d)\n", ifIndex, wdev->AuthMode));
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
INT	Set_ApCli_EncrypType_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;
	struct wifi_dev *wdev;


	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = (UCHAR)pObj->ioctl_if;

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

	pApCliEntry->PairCipher = wdev->WepStatus;
	pApCliEntry->GroupCipher = wdev->WepStatus;
	pApCliEntry->bMixCipher = FALSE;

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
INT	Set_ApCli_DefaultKeyID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	ULONG 			KeyIdx;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev;


	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	wdev = &pAd->ApCfg.ApCliTab[pObj->ioctl_if].wdev;

	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		wdev->DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  /* Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", pObj->ioctl_if, wdev->DefaultKeyId));

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
INT	Set_ApCli_WPAPSK_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	INT ifIndex;
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
	pApCliEntry->PSKLen = (UCHAR)strlen(arg);

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
INT	Set_ApCli_Key1_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	INT			ifIndex;
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
INT	Set_ApCli_Key2_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	INT			ifIndex;
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
INT	Set_ApCli_Key3_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	INT			ifIndex;
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
INT	Set_ApCli_Key4_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	PAPCLI_STRUCT	pApCliEntry = NULL;
	INT			ifIndex;
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

INT Set_ApCli_TxMode_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	struct wifi_dev *wdev;


	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	wdev = &pAd->ApCfg.ApCliTab[pObj->ioctl_if].wdev;

	wdev->DesiredTransmitSetting.field.FixedTxMode = (USHORT)RT_CfgSetFixedTxPhyMode(arg);
	DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_TxMode_Proc = %d\n",
				pObj->ioctl_if, wdev->DesiredTransmitSetting.field.FixedTxMode));

	return TRUE;
}

INT Set_ApCli_TxMcs_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev;


	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	wdev = &pAd->ApCfg.ApCliTab[pObj->ioctl_if].wdev;

	wdev->DesiredTransmitSetting.field.MCS =
			(USHORT)RT_CfgSetTxMCSProc(arg, &wdev->bAutoTxRateSwitch);

	if (wdev->DesiredTransmitSetting.field.MCS == MCS_AUTO)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_TxMcs_Proc = AUTO\n", pObj->ioctl_if));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_TxMcs_Proc = %d\n",
					pObj->ioctl_if, wdev->DesiredTransmitSetting.field.MCS));
	}

	return TRUE;
}


#ifdef WPA_SUPPLICANT_SUPPORT
INT Set_ApCli_Wpa_Support(
    IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	INT		ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

    if ( simple_strtol(arg, 0, 10) == 0)
        pApCliEntry->wpa_supplicant_info.WpaSupplicantUP = WPA_SUPPLICANT_DISABLE;
    else if ( simple_strtol(arg, 0, 10) == 1)
        pApCliEntry->wpa_supplicant_info.WpaSupplicantUP = WPA_SUPPLICANT_ENABLE;
    else if ( simple_strtol(arg, 0, 10) == 2)
        pApCliEntry->wpa_supplicant_info.WpaSupplicantUP = WPA_SUPPLICANT_ENABLE_WITH_WEB_UI;
    else
        pApCliEntry->wpa_supplicant_info.WpaSupplicantUP = WPA_SUPPLICANT_DISABLE;

    DBGPRINT(RT_DEBUG_TRACE, ("Set_ApCli_Wpa_Support::(WpaSupplicantUP=%d)\n",
				pApCliEntry->wpa_supplicant_info.WpaSupplicantUP));

    return TRUE;
}


INT	Set_ApCli_IEEE8021X_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
    	ULONG ieee8021x;
	POS_COOKIE 		pObj;
	INT			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	ieee8021x = simple_strtol(arg, 0, 10);

	if (ieee8021x == 1)
        pApCliEntry->wdev.IEEE8021X = TRUE;
	else if (ieee8021x == 0)
		pApCliEntry->wdev.IEEE8021X = FALSE;
	else
		return FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_ApCli_IEEE8021X_Proc::(IEEE8021X=%d)\n", pObj->ioctl_if, pApCliEntry->wdev.IEEE8021X));

	return TRUE;
}
#endif /* WPA_SUPPLICANT_SUPPORT */

#ifdef MAC_REPEATER_SUPPORT
INT Set_ReptMode_Enable_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	UCHAR Enable = simple_strtol(arg, 0, 10);

	pAd->ApCfg.bMACRepeaterEn = (Enable ? TRUE : FALSE);

	DBGPRINT(RT_DEBUG_TRACE, ("MACRepeaterEn = %d, Repeater Mode (%s)\n",
				pAd->ApCfg.bMACRepeaterEn, (Enable ? "ON" : "OFF")));

	AsicSetMacAddrExt(pAd, pAd->ApCfg.bMACRepeaterEn);
	return TRUE;
}

#ifdef APCLI_AUTO_CONNECT_SUPPORT
/*
    ==========================================================================
    Description:
        Trigger Apcli Auto connect to find the missed AP.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_ApCli_AutoConnect_Proc(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	POS_COOKIE  		pObj= (POS_COOKIE) pAd->OS_Cookie;
	UCHAR				ifIndex;
	AP_ADMIN_CONFIG *pApCfg;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;
	pApCfg = &pAd->ApCfg;
	ifIndex = pObj->ioctl_if;

	if (pApCfg->ApCliAutoConnectRunning == FALSE)
	{
		Set_ApCli_Enable_Proc(pAd, "0");
		pApCfg->ApCliAutoConnectRunning = TRUE;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set_ApCli_AutoConnect_Proc() is still running\n"));
		return TRUE;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_AutoConnect_Proc::(Len=%d,Ssid=%s)\n",
			ifIndex, pApCfg->ApCliTab[ifIndex].CfgSsidLen, pApCfg->ApCliTab[ifIndex].CfgSsid));


	/*
		use site survey function to trigger auto connecting (when pAd->ApCfg.ApAutoConnectRunning == TRUE)
	*/
	Set_SiteSurvey_Proc(pAd, "");//pApCfg->ApCliTab[ifIndex].CfgSsid);

	return TRUE;
}
#endif  /* APCLI_AUTO_CONNECT_SUPPORT */

#if 0
INT	Set_ReptCli_Addr_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	UCHAR tempMAC[6], apCliIdx;
	RTMP_STRING *token;
	RTMP_STRING sepValue[] = ":", DASH = '-';
	ULONG offset, Addr;
	INT i, CliIdx;
	PREPEATER_CLIENT_ENTRY pReptCliEntry = NULL;

	if(strlen(arg) < 19)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		APCLI_CTRL_MSG_STRUCT ApCliCtrlMsg;

		apCliIdx = (UCHAR) simple_strtol((token+1), 0, 10);

		if (apCliIdx > 7)
			return FALSE;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (&tempMAC[i]), 1);
		}

		if(i != 6)
			return FALSE;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n",
								tempMAC[0], tempMAC[1], tempMAC[2], tempMAC[3], tempMAC[4], tempMAC[5], apCliIdx));

		for (CliIdx = 0; CliIdx < MAX_EXT_MAC_ADDR_SIZE; CliIdx++)
		{
			if (pAd->ApCfg.ApCliTab[apCliIdx].RepeaterCli[CliIdx].bValid == FALSE)
				break;
		}

		if (CliIdx >= MAX_EXT_MAC_ADDR_SIZE)
		{
			DBGPRINT(RT_DEBUG_ERROR, (" Repeate Client Full !!!\n"));
			return FALSE;
		}

		pReptCliEntry = &pAd->ApCfg.ApCliTab[apCliIdx].RepeaterCli[CliIdx];
		COPY_MAC_ADDR(pReptCliEntry->OriginalAddress, tempMAC);
		tempMAC[0] = pAd->ApCfg.ApCliTab[apCliIdx].CurrentAddress[0];
		tempMAC[0] |= 0x2;
		tempMAC[1] = pAd->ApCfg.ApCliTab[apCliIdx].CurrentAddress[1];
		tempMAC[2] = pAd->ApCfg.ApCliTab[apCliIdx].CurrentAddress[2];
		COPY_MAC_ADDR(pReptCliEntry->CurrentAddress, tempMAC);
		pReptCliEntry->bValid = TRUE;
		pReptCliEntry->CliConnectState = 1;

		NdisZeroMemory(&ApCliCtrlMsg, sizeof(APCLI_CTRL_MSG_STRUCT));
		ApCliCtrlMsg.Status = MLME_SUCCESS;
		COPY_MAC_ADDR(&ApCliCtrlMsg.SrcAddr[0], tempMAC);
		ApCliCtrlMsg.BssIdx = apCliIdx;
		ApCliCtrlMsg.CliIdx = CliIdx;

		offset = 0x1480 + (HW_WCID_ENTRY_SIZE * CliIdx);
		Addr = tempMAC[0] + (tempMAC[1] << 8) +(tempMAC[2] << 16) +(tempMAC[3] << 24);
		RTMP_IO_WRITE32(pAd, offset, Addr);
		Addr = tempMAC[4] + (tempMAC[5] << 8);
		RTMP_IO_WRITE32(pAd, offset + 4, Addr);

		MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_MT2_AUTH_REQ,
			sizeof(APCLI_CTRL_MSG_STRUCT), &ApCliCtrlMsg, apCliIdx);

		return TRUE;
	}

	return FALSE;
}
#endif
#endif /* MAC_REPEATER_SUPPORT */

#ifdef WSC_AP_SUPPORT
INT Set_AP_WscSsid_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT	 Set_AP_WscConfMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	INT         ConfModeIdx;
	/*INT         IsAPConfigured; */
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    BOOLEAN     bFromApCli = FALSE;
    PWSC_CTRL   pWscControl;

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
                DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: Ap Client only supports Enrollee mode.(ConfModeIdx=%d)\n",
						apidx, ConfModeIdx));
                return FALSE;
            }
        }
        else
#endif /* APCLI_SUPPORT */
        {
        	pWscControl->WscConfMode = (ConfModeIdx & WSC_ENROLLEE_PROXY_REGISTRAR);
            WscInit(pAd, FALSE, apidx);
        }
        pWscControl->WscStatus = STATUS_WSC_IDLE;
    }

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, &pAd->ApCfg.ApCliTab[apidx].wdev.if_addr[0], MAC_ADDR_LEN);
    }
    else
#endif /* APCLI_SUPPORT */
    {
        memcpy(mac_addr, &pAd->ApCfg.MBSSID[apidx].wdev.bssid[0], MAC_ADDR_LEN);
    }

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscConfMode_Proc::(WscConfMode(0~7)=%d)\n",
                            PRINT_MAC(mac_addr), pWscControl->WscConfMode));
	return TRUE;
}

INT	Set_AP_WscConfStatus_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
    	WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, IsSelectedRegistrar, WscMode, 0, apidx, NULL, 0, AP_MODE);
		APUpdateBeaconFrame(pAd, apidx);
    }
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: Set failed!!(WscConfStatus=%s), WscConfStatus is 1 or 2 \n", apidx, arg));
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: WscConfStatus is not changed (%d) \n", apidx, pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus));
		return FALSE;  /*Invalid argument */
	}

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscConfStatus_Proc::(WscConfStatus=%d)\n",
				PRINT_MAC(pAd->ApCfg.MBSSID[apidx].wdev.bssid),
                               pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus));

	return TRUE;
}

INT	Set_AP_WscMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	INT         WscMode;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    PWSC_CTRL   pWscControl;
#ifdef APCLI_SUPPORT
    BOOLEAN     bFromApCli = FALSE;
#endif /* APCLI_SUPPORT */

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
#ifdef APCLI_SUPPORT
			bFromApCli = TRUE;
#endif /* APCLI_SUPPORT */
			pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
    	}
		else
		{
#ifdef APCLI_SUPPORT
			bFromApCli = FALSE;
#endif /* APCLI_SUPPORT */
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
		}
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscMode_Proc:: This command is from p2p interface now.\n", apidx));
    }
    else
#endif /* P2P_SUPPORT */
    {
#ifdef APCLI_SUPPORT
        bFromApCli = FALSE;
#endif /* APCLI_SUPPORT */
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
					PRINT_MAC(mac_addr), pWscControl->WscMode));

	return TRUE;
}

INT	Set_WscStatus_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscStatus_Proc::(WscStatus=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus));
	return TRUE;
}

#define WSC_GET_CONF_MODE_EAP	1
#define WSC_GET_CONF_MODE_UPNP	2
INT	Set_AP_WscGetConf_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	INT                 WscMode, wscGetConfMode = 0;
	INT                 IsAPConfigured;
	PWSC_CTRL           pWscControl;
	//PWSC_UPNP_NODE_INFO pWscUPnPNodeInfo;
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
    		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_AP_WscGetConf_Proc:: ApCli is disabled.\n", apidx));
    		return FALSE;
    	}
        bFromApCli = TRUE;
		apidx &= (~MIN_NET_DEVICE_FOR_APCLI);
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_AP_WscGetConf_Proc:: This command is from apcli interface now.\n", apidx));
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
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_AP_WscGetConf_Proc:: This command is from p2p interface now.\n", apidx));
    }
    else
#endif /* P2P_SUPPORT */
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AP_WscGetConf_Proc:: This command is from ra interface now.\n", apidx));
    }

	NdisZeroMemory(mac_addr, MAC_ADDR_LEN);

#ifdef WSC_V2_SUPPORT
	pWscV2Info = &pWscControl->WscV2Info;
#endif /* WSC_V2_SUPPORT */
	wscGetConfMode = simple_strtol(arg, 0, 10);

    IsAPConfigured = pWscControl->WscConfStatus;
    //pWscUPnPNodeInfo = &pWscControl->WscUPnPNodeInfo;

    if ((pWscControl->WscConfMode == WSC_DISABLE)
#ifdef WSC_V2_SUPPORT
		|| ((pWscV2Info->bWpsEnable == FALSE) && (pWscV2Info->bEnableWpsV2))
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
#ifdef P2P_SUPPORT
	if (pAd->P2pCfg.Dpid != DEV_PASS_ID_NOSPEC)
		WscMode = pAd->P2pCfg.Dpid;
#endif /* P2P_SUPPORT */
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

 		if (WscMode == DEV_PASS_ID_PIN)
    	{
			NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr,
	                       pAd->ApCfg.ApCliTab[apidx].wdev.if_addr,
	                       6);

	        pAd->ApCfg.ApCliTab[apidx].Enable = apcliEn;
			NdisMoveMemory(mac_addr, pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, MAC_ADDR_LEN);
    	}
		else
		{
			pWscControl->WscSsid.SsidLength = 0;
			NdisZeroMemory(&pWscControl->WscSsid, sizeof(NDIS_802_11_SSID));
			pWscControl->WscPBCBssCount = 0;
			/* WPS - SW PBC */
			WscPushPBCAction(pAd, pWscControl);
		}
    }
	else
#endif /* APCLI_SUPPORT */
	{
		WscBuildBeaconIE(pAd, IsAPConfigured, TRUE, WscMode, pWscControl->WscConfigMethods, apidx, NULL, 0, AP_MODE);
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, TRUE, WscMode, pWscControl->WscConfigMethods, apidx, NULL, 0, AP_MODE);
		APUpdateBeaconFrame(pAd, apidx);
		NdisMoveMemory(mac_addr, pAd->ApCfg.MBSSID[apidx].wdev.bssid, MAC_ADDR_LEN);
	}

#ifdef APCLI_SUPPORT
	if (bFromApCli && (WscMode == DEV_PASS_ID_PBC))
		;
	else
#endif /* APCLI_SUPPORT */
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
    	if (WscMode == DEV_PASS_ID_PIN)
    	{
    		WscAssignEntryMAC(pAd, pWscControl);
			WscSendUPnPConfReqMsg(pAd, pWscControl->EntryIfIdx,
	    							(PUCHAR)pAd->ApCfg.MBSSID[pWscControl->EntryIfIdx].Ssid,
	    							pAd->ApCfg.MBSSID[apidx].wdev.bssid, 3, 0, AP_MODE);
    	}
		else
		{
			RTMP_SEM_LOCK(&pWscControl->WscPeerListSemLock);
			WscClearPeerList(&pWscControl->WscPeerList);
			RTMP_SEM_UNLOCK(&pWscControl->WscPeerListSemLock);
		}
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
				PRINT_MAC(mac_addr), wscGetConfMode));

	return TRUE;
}

INT	Set_AP_WscPinCode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	UINT        PinCode = 0;
	BOOLEAN     validatePin;
#ifdef APCLI_SUPPORT
	BOOLEAN bFromApCli = FALSE;
#endif /* APCLI_SUPPORT */
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
    	if (P2P_CLI_ON(pAd))
    	{
#ifdef APCLI_SUPPORT
			bFromApCli = TRUE;
#endif
			pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
    	}
		else
		{
#ifdef APCLI_SUPPORT
			bFromApCli = FALSE;
#endif
			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
		}
        DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_WscMode_Proc:: This command is from p2p interface now.\n", apidx));
    }
    else
#endif /* P2P_SUPPORT */
    {
#ifdef APCLI_SUPPORT
        bFromApCli = FALSE;
#endif
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscPinCode_Proc:: This command is from ra interface now.\n", apidx));
    }

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
					PRINT_MAC(mac_addr), pWscControl->WscPinCode));

	return TRUE;
}

INT	Set_WscOOB_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
    Set_AP_AuthMode_Proc(pAd, "WPA2PSK");
    Set_AP_EncrypType_Proc(pAd, "AES");
    pTempSsid = vmalloc(33);
    if (pTempSsid)
    {
        memset(pTempSsid, 0, 33);
        snprintf(pTempSsid, 33,"RalinkInitialAP%02X%02X%02X",
						pAd->ApCfg.MBSSID[apidx].wdev.bssid[3],
                                          pAd->ApCfg.MBSSID[apidx].wdev.bssid[4],
                                          pAd->ApCfg.MBSSID[apidx].wdev.bssid[5]);
        Set_AP_SSID_Proc(pAd, pTempSsid);
        vfree(pTempSsid);
    }
	Set_AP_WPAPSK_Proc(pAd, "RalinkInitialAPxx1234");

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscOOB_Proc\n", apidx));
	return TRUE;
}

INT	Set_WscStop_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;
    PWSC_CTRL   pWscControl;
#ifdef APCLI_SUPPORT
    BOOLEAN     bFromApCli = FALSE;
#endif

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
#ifdef APCLI_SUPPORT
        bFromApCli = FALSE;
#endif
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
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0, apidx, NULL, 0, AP_MODE);
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
	BSS_STRUCT *pMbss;
	struct wifi_dev *wdev;


	pMbss = &pAd->ApCfg.MBSSID[apidx];
	wdev = &pMbss->wdev;
#ifdef HOSTAPD_SUPPORT
	if (pMbss->Hostapd == Hostapd_EXT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS is control by hostapd now.\n"));
		return;
	}
#endif /*HOSTAPD_SUPPORT*/


	memset(&Profile, 0x00, sizeof(WSC_CONFIGURED_VALUE));
	Profile.WscConfigured = pMbss->WscControl.WscConfStatus;
	NdisZeroMemory(Profile.WscSsid, 32 + 1);
	NdisMoveMemory(Profile.WscSsid, pMbss->Ssid, pMbss->SsidLen);
	Profile.WscSsid[pMbss->SsidLen] = '\0';
	if (wdev->AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		Profile.WscAuthMode = WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK;
	else
		Profile.WscAuthMode = WscGetAuthType(wdev->AuthMode);
	if (wdev->WepStatus == Ndis802_11TKIPAESMix)
		Profile.WscEncrypType = WSC_ENCRTYPE_TKIP |WSC_ENCRTYPE_AES;
	else
		Profile.WscEncrypType = WscGetEncryType(wdev->WepStatus);
	NdisZeroMemory(Profile.WscWPAKey, 64 + 1);

	if (Profile.WscEncrypType == 2)
	{
		Profile.DefaultKeyIdx = wdev->DefaultKeyId + 1;
#if 0
		if (pAd->SharedKey[apidx][wdev->DefaultKeyId].WepKeyType == WEP_ASCII_TYPE)
		{
			int i;
			for (i=0; i<pAd->SharedKey[apidx][wdev->DefaultKeyId].KeyLen; i++)
			{
				sprintf(Profile.WscWPAKey,
						"%s%02x", Profile.WscWPAKey,
						pAd->SharedKey[apidx][wdev->DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[apidx][wdev->DefaultKeyId].KeyLen)*2] = '\0';
	}
		else /* Hex WEP Key */
#endif /* if 0 */
		{
			int i;
			for (i=0; i<pAd->SharedKey[apidx][wdev->DefaultKeyId].KeyLen; i++)
			{
				snprintf((RTMP_STRING *) Profile.WscWPAKey, sizeof(Profile.WscWPAKey),
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[apidx][wdev->DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[apidx][wdev->DefaultKeyId].KeyLen)*2] = '\0';
		}
	}
	else if (Profile.WscEncrypType >= 4)
	{
		Profile.DefaultKeyIdx = 2;
		NdisMoveMemory(Profile.WscWPAKey, pMbss->WscControl.WpaPsk,
						pMbss->WscControl.WpaPskLen);
		Profile.WscWPAKey[pMbss->WscControl.WpaPskLen] = '\0';
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
	if (wdev->AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		sprintf(msg+strlen(msg), "%-12s", "WPAPSKWPA2PSK");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	if (wdev->WepStatus == Ndis802_11TKIPAESMix)
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
	os_free_mem(NULL, msg);
}

#if defined(INF_AR9) || defined(BB_SOC)
#if defined(AR9_MAPI_SUPPORT) || defined(BB_SOC)

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
	BSS_STRUCT *pMbss;
	struct wifi_dev *wdev;

	pMbss = &pAd->ApCfg.MBSSID[apidx];
	wdev = &pMbss->wdev;

	memset(&Profile, 0x00, sizeof(WSC_CONFIGURED_VALUE));
	Profile.WscConfigured = pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus;
	NdisZeroMemory(Profile.WscSsid, 32 + 1);
	NdisMoveMemory(Profile.WscSsid, pAd->ApCfg.MBSSID[apidx].Ssid,
								    pAd->ApCfg.MBSSID[apidx].SsidLen);
	Profile.WscSsid[pAd->ApCfg.MBSSID[apidx].SsidLen] = '\0';
	if (wdev->AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		Profile.WscAuthMode = WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK;
	else
		Profile.WscAuthMode = WscGetAuthType(wdev->AuthMode);
	if (wdev->WepStatus == Ndis802_11Encryption4Enabled)
		Profile.WscEncrypType = WSC_ENCRTYPE_TKIP |WSC_ENCRTYPE_AES;
	else
		Profile.WscEncrypType = WscGetEncryType(wdev->WepStatus);
	NdisZeroMemory(Profile.WscWPAKey, 64 + 1);

	if (Profile.WscEncrypType == 2)
	{
		Profile.DefaultKeyIdx = wdev->DefaultKeyId + 1;
		{
			int i;
			for (i=0; i<pAd->SharedKey[apidx][wdev->DefaultKeyId].KeyLen; i++)
			{
				snprintf((RTMP_STRING *) Profile.WscWPAKey, sizeof(Profile.WscWPAKey),
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[apidx][wdev->DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[apidx][wdev->DefaultKeyId].KeyLen)*2] = '\0';
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
	if (wdev->AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		sprintf(msg+strlen(msg), "%-12s", "WPAPSKWPA2PSK");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	if (wdev->WepStatus == Ndis802_11Encryption4Enabled)
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

	os_free_mem(NULL, msg);
}

VOID RTMPIoctlWscStatus(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	RTMP_STRING *msg;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;

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

	os_free_mem(NULL, msg);
}

VOID RTMPIoctlGetWscDynInfo(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	char *msg;
	BSS_STRUCT *pMbss;
	INT apidx,configstate;

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
#ifdef BB_SOC
		sprintf(msg+strlen(msg),"DeviceName = %s\n",(pMbss->WscControl.RegData.SelfInfo.DeviceName));
#endif
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

	os_free_mem(NULL, msg);
}

VOID RTMPIoctlGetWscRegsDynInfo(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	char *msg;
	BSS_STRUCT *pMbss;
	INT apidx;

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
#ifdef BB_SOC
		sprintf(msg+strlen(msg),"DeviceName = %s\n",(pMbss->WscControl.RegData.SelfInfo.DeviceName));
#endif
		sprintf(msg+strlen(msg),"UUID_R = %s\n",(pMbss->WscControl.RegData.PeerInfo.Uuid));
	}

	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s", msg));
	}

	os_free_mem(NULL, msg);
}
#endif /* defined(AR9_MAPI_SUPPORT) || defined(BB_SOC) */
#endif /* defined(INF_AR9) || defined(BB_SOC) */

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

UCHAR	WscRxMsgTypeFromUpnp(
	IN	PRTMP_ADAPTER		pAdapter,
	IN  RTMP_STRING *	pData,
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
        snprintf(pTempSsid, 33, "RalinkInitialAP%02X%02X%02X",
				pAd->ApCfg.MBSSID[apidx].wdev.bssid[3],
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
INT	Set_AP_WscSecurityMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT Set_AP_WscMultiByteCheck_Proc(
	IN  PRTMP_ADAPTER   pAd,
	IN  RTMP_STRING *arg)
{
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			apidx = pObj->ioctl_if;
	BOOLEAN			bEnable = FALSE;
	PWSC_CTRL		pWpsCtrl = NULL;
	//BOOLEAN     	bFromApCli = FALSE;

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        //bFromApCli = TRUE;
        pWpsCtrl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif /* APCLI_SUPPORT */
    {
        //bFromApCli = FALSE;
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

INT	Set_WscVersion_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	version = (UCHAR)simple_strtol(arg, 0, 16);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscVersion_Proc::(version=%x)\n",version));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.RegData.SelfInfo.Version = version;
	return TRUE;
}

#ifdef WSC_V2_SUPPORT
INT	Set_WscFragment_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	bool_flag = (UCHAR)simple_strtol(arg, 0, 16);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscFragment_Proc::(bool_flag=%d)\n",bool_flag));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.bWscFragment = bool_flag;
	return TRUE;
}

INT	Set_WscFragmentSize_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	USHORT		WscFragSize = (USHORT)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscFragmentSize_Proc::(WscFragSize=%d)\n", WscFragSize));
	if ((WscFragSize >=128) && (WscFragSize <=300))
		pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscFragSize = WscFragSize;
	return TRUE;
}

INT	Set_WscSetupLock_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		bEnable = (UCHAR)simple_strtol(arg, 0, 10);
	PWSC_CTRL	pWscControl = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl;

	if (bEnable == 0)
	{
		BOOLEAN bCancelled = FALSE;
		pWscControl->PinAttackCount = 0;
		if (pWscControl->WscSetupLockTimerRunning)
		{
			RTMPCancelTimer(&pWscControl->WscSetupLockTimer, &bCancelled);
		}
		WscSetupLockTimeout(NULL, pWscControl, NULL, NULL);
	}
	else
	{
		pWscControl->bSetupLock = TRUE;
		WscBuildBeaconIE(pAd,
						 pWscControl->WscConfStatus,
						 FALSE,
						 0,
						 0,
						 pObj->ioctl_if,
						 NULL,
						 0,
						 AP_MODE);
		WscBuildProbeRespIE(pAd,
							WSC_MSGTYPE_AP_WLAN_MGR,
							pWscControl->WscConfStatus,
							FALSE,
							0,
							0,
							pObj->ioctl_if,
							NULL,
							0,
							AP_MODE);
		APUpdateBeaconFrame(pAd, pObj->ioctl_if);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscSetupLock_Proc::(bSetupLock=%d)\n",
								pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.bSetupLock));
	return TRUE;
}

INT	Set_WscV2Support_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR bEnable = (UCHAR)simple_strtol(arg, 0, 10);
	PWSC_CTRL pWscControl = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl;

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
			WscOnOff(pAd, pObj->ioctl_if, TRUE);
		else
			WscOnOff(pAd, pObj->ioctl_if, FALSE);

		APUpdateBeaconFrame(pAd, pObj->ioctl_if);
	}
	else
	{
		WscInit(pAd, FALSE, pObj->ioctl_if);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscV2Support_Proc::(bEnableWpsV2=%d)\n",
								pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.bEnableWpsV2));
	return TRUE;
}

INT	Set_WscVersion2_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_WscExtraTlvTag_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	USHORT		new_tag = (USHORT)simple_strtol(arg, 0, 16);

	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.ExtraTlv.TlvTag = new_tag;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscExtraTlvTag_Proc::(new_tag=0x%04X)\n",new_tag));
	return TRUE;
}

INT	Set_WscExtraTlvType_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		type = (UCHAR)simple_strtol(arg, 0, 10);

	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.WscV2Info.ExtraTlv.TlvType = type;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscExtraTlvType_Proc::(type=%d)\n",type));
	return TRUE;
}

INT	Set_WscExtraTlvData_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_WscMaxPinAttack_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		MaxPinAttack = (UCHAR)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscMaxPinAttack_Proc::(MaxPinAttack=%d)\n", MaxPinAttack));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.MaxPinAttack = MaxPinAttack;
	return TRUE;
}


INT	Set_WscSetupLockTime_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UINT		SetupLockTime = (UINT)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscSetupLockTime_Proc::(SetupLockTime=%d)\n", SetupLockTime));
	pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl.SetupLockTime = SetupLockTime;
	return TRUE;
}

#endif /* WSC_V2_SUPPORT */

INT	Set_WscAutoTriggerDisable_Proc(
	IN	RTMP_ADAPTER	*pAd, 
	IN	RTMP_STRING		*arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR bEnable = (UCHAR)simple_strtol(arg, 0, 10);
	PWSC_CTRL pWscCtrl = &pAd->ApCfg.MBSSID[pObj->ioctl_if].WscControl;

	if (bEnable == 0)
		pWscCtrl->bWscAutoTriggerDisable = FALSE;
	else
		pWscCtrl->bWscAutoTriggerDisable = TRUE;
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_WscAutoTriggerDisable_Proc::(bWscAutoTriggerDisable=%d)\n",
								pWscCtrl->bWscAutoTriggerDisable));
	return TRUE;
}

#endif /* WSC_AP_SUPPORT */

#ifdef EASY_CONFIG_SETUP
INT Set_AP_AutoProvisionEnable_Proc(
	IN  PRTMP_ADAPTER   pAd,
	IN  RTMP_STRING *arg)
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

INT	Set_RssiThreshold_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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
INT Set_AP_WACEnable_Proc(
	IN  PRTMP_ADAPTER   pAd,
	IN  RTMP_STRING *arg)
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
INT	Set_IappPID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	unsigned long IappPid;
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;

	IappPid = simple_strtol(arg, 0, 10);
	RTMP_GET_OS_PID(pObj->IappPid, IappPid);
	pObj->IappPid_nr = IappPid;

/*	DBGPRINT(RT_DEBUG_TRACE, ("pObj->IappPid = %d", GET_PID_NUMBER(pObj->IappPid))); */
	return TRUE;
} /* End of Set_IappPID_Proc */
#endif /* IAPP_SUPPORT */


INT	Set_DisConnectSta_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
	RTMP_STRING *value;
	INT						i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	if(strlen(arg) != 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
		return FALSE;

	for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"))
	{
		if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid */

		AtoH(value, (UCHAR *)&macAddr[i++], 1);
	}

	pEntry = MacTableLookup(pAd, macAddr);
	if (pEntry)
	{
		MlmeDeAuthAction(pAd, pEntry, REASON_DISASSOC_STA_LEAVING, FALSE);
/*		MacTableDeleteEntry(pAd, pEntry->wcid, Addr); */
	}

	return TRUE;
}

INT Set_DisConnectAllSta_Proc(
        IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
#ifdef DOT11W_PMF_SUPPORT
        CHAR value = simple_strtol(arg, 0, 10);

	if (value == 2)
        {
		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
		INT i;

		DBGPRINT(RT_DEBUG_WARN, ("[PMF]%s:: apidx=%d\n", __FUNCTION__, pObj->ioctl_if));
		APMlmeKickOutAllSta(pAd, pObj->ioctl_if, REASON_DEAUTH_STA_LEAVING);
		for (i=1; i<MAX_LEN_OF_MAC_TABLE; i++)
		{
			MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];
			if (IS_ENTRY_CLIENT(pEntry)) {
				DBGPRINT(RT_DEBUG_ERROR, ("[PMF]%s: MacTableDeleteEntry %x:%x:%x:%x:%x:%x\n",
						__FUNCTION__, PRINT_MAC(pEntry->Addr)));
				MacTableDeleteEntry(pAd, pEntry->wcid, pEntry->Addr);
			}
		}
        } else
#endif /* DOT11W_PMF_SUPPORT */
	{
		MacTableReset(pAd, 1);
	}

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
INT	Set_IEEE8021X_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	ULONG ieee8021x;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev;

	ieee8021x = simple_strtol(arg, 0, 10);

	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
	if (ieee8021x == 1)
		wdev->IEEE8021X = TRUE;
	else if (ieee8021x == 0)
		wdev->IEEE8021X = FALSE;
	else
		return FALSE;  /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_IEEE8021X_Proc::(IEEE8021X=%d)\n", pObj->ioctl_if, wdev->IEEE8021X));

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
INT	Set_PreAuth_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_OwnIPAddr_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	UINT32		ip_addr;

	if (rtinet_aton(arg, &ip_addr))
 	{
        pAd->ApCfg.own_ip_addr = ip_addr;
		DBGPRINT(RT_DEBUG_TRACE, ("own_ip_addr=%s(%x)\n", arg, pAd->ApCfg.own_ip_addr));
	}
	return TRUE;
}

INT	Set_EAPIfName_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_PreAuthIfName_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_RADIUS_Server_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_RADIUS_Port_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
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

INT	Set_RADIUS_Key_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR IdMbss = pObj->ioctl_if;

	if (simple_strtol(arg, 0, 10) != 0)
		pAd->ApCfg.MBSSID[IdMbss].wdev.UapsdInfo.bAPSDCapable = TRUE;
	else
		pAd->ApCfg.MBSSID[IdMbss].wdev.UapsdInfo.bAPSDCapable = FALSE;

	return TRUE;
} /* End of Set_UAPSD_Proc */
#endif /* UAPSD_SUPPORT */



#ifdef MCAST_RATE_SPECIFIC
INT Set_McastPhyMode(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
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
			DBGPRINT(RT_DEBUG_OFF, ("unknown Multicast PhyMode %d.\n", PhyMode));
			DBGPRINT(RT_DEBUG_OFF, ("0:Disable 1:CCK, 2:OFDM, 3:HTMIX.\n"));
			break;
	}

	return TRUE;
}

INT Set_McastMcs(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	UCHAR Mcs = simple_strtol(arg, 0, 10);

	if (Mcs > 15)
		DBGPRINT(RT_DEBUG_OFF, ("Mcs must in range of 0 to 15\n"));

	switch(pAd->CommonCfg.MCastPhyMode.field.MODE)
	{
		case MODE_CCK:
			if ((Mcs <= 3) || (Mcs >= 8 && Mcs <= 11))
				pAd->CommonCfg.MCastPhyMode.field.MCS = Mcs;
			else
				DBGPRINT(RT_DEBUG_OFF, ("MCS must in range of 0 ~ 3 and 8 ~ 11 for CCK Mode.\n"));
			break;

		case MODE_OFDM:
			if (Mcs > 7)
				DBGPRINT(RT_DEBUG_OFF, ("MCS must in range from 0 to 7 for CCK Mode.\n"));
			else
				pAd->CommonCfg.MCastPhyMode.field.MCS = Mcs;
			break;

		default:
			pAd->CommonCfg.MCastPhyMode.field.MCS = Mcs;
			break;
	}

	return TRUE;
}

INT Show_McastRate(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	DBGPRINT(RT_DEBUG_OFF, ("Mcast PhyMode =%d\n", pAd->CommonCfg.MCastPhyMode.field.MODE));
	DBGPRINT(RT_DEBUG_OFF, ("Mcast Mcs =%d\n", pAd->CommonCfg.MCastPhyMode.field.MCS));
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


INT	Set_AP2040ReScan_Proc(
	IN	PRTMP_ADAPTER pAd,
	IN	RTMP_STRING *arg)
{
	APOverlappingBSSScan(pAd);

	/* apply setting */
	SetCommonHT(pAd);
	AsicBBPAdjust(pAd);
	AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
	AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_AP2040ReScan_Proc() Trigger AP ReScan !!!\n"));

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
		DBGPRINT(RT_DEBUG_OFF, ("LifeCheckCnt must in range of 0 to 65535\n"));

	DBGPRINT(RT_DEBUG_OFF, ("EntryLifeCheck Cnt = %ld.\n", pAd->ApCfg.EntryLifeCheck));
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
INT	ApCfg_Set_AuthMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	INT				apidx,
	IN	RTMP_STRING *arg)
{
	struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[apidx].wdev;

	if ((strcmp(arg, "WEPAUTO") == 0) || (strcmp(arg, "wepauto") == 0))
		wdev->AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if ((strcmp(arg, "OPEN") == 0) || (strcmp(arg, "open") == 0))
		wdev->AuthMode = Ndis802_11AuthModeOpen;
	else if ((strcmp(arg, "SHARED") == 0) || (strcmp(arg, "shared") == 0))
		wdev->AuthMode = Ndis802_11AuthModeShared;
	else if ((strcmp(arg, "WPAPSK") == 0) || (strcmp(arg, "wpapsk") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((strcmp(arg, "WPA2PSK") == 0) || (strcmp(arg, "wpa2psk") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWPA2PSK;
	else if ((strcmp(arg, "WPAPSKWPA2PSK") == 0) || (strcmp(arg, "wpapskwpa2psk") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
#ifdef DOT1X_SUPPORT
	else if ((strcmp(arg, "WPA") == 0) || (strcmp(arg, "wpa") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWPA;
	else if ((strcmp(arg, "WPA2") == 0) || (strcmp(arg, "wpa2") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWPA2;
	else if ((strcmp(arg, "WPA1WPA2") == 0) || (strcmp(arg, "wpa1wpa2") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWPA1WPA2;
#endif /* DOT1X_SUPPORT */
#ifdef WAPI_SUPPORT
	else if ((strcmp(arg, "WAICERT") == 0) || (strcmp(arg, "waicert") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWAICERT;
	else if ((strcmp(arg, "WAIPSK") == 0) || (strcmp(arg, "waipsk") == 0))
		wdev->AuthMode = Ndis802_11AuthModeWAIPSK;
#endif /* WAPI_SUPPORT */
	else
		wdev->AuthMode = Ndis802_11AuthModeOpen;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d)::AuthMode=%d\n", apidx, wdev->AuthMode));

	return TRUE;
}

INT	ApCfg_Set_MaxStaNum_Proc(
	IN PRTMP_ADAPTER 	pAd,
	IN INT				apidx,
	IN RTMP_STRING *arg)
{
	pAd->ApCfg.MBSSID[apidx].MaxStaNum = (UCHAR)simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) %s::(MaxStaNum=%d)\n",
					apidx, __FUNCTION__, pAd->ApCfg.MBSSID[apidx].MaxStaNum));
	return TRUE;
}

INT	ApCfg_Set_IdleTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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

#ifdef RTMP_RBUS_SUPPORT
#ifdef LED_CONTROL_SUPPORT
INT Set_WlanLed_Proc(
	IN PRTMP_ADAPTER 	pAd,
	IN RTMP_STRING *arg)
{
	BOOLEAN bWlanLed;
#ifdef CONFIG_SWMCU_SUPPORT
	PSWMCU_LED_CONTROL pSWMCULedCntl = &pAd->LedCntl.SWMCULedCntl;
#endif /* CONFIG_SWMCU_SUPPORT */

	bWlanLed = (BOOLEAN) simple_strtol(arg, 0, 10);

#ifdef CONFIG_SWMCU_SUPPORT
	if (bWlanLed != pSWMCULedCntl->bWlanLed)
#endif /* CONFIG_SWMCU_SUPPORT */
	{
		if (bWlanLed)
			RTMPStartLEDMode(pAd);
		else
			RTMPExitLEDMode(pAd);
	};

	return TRUE;
}
#endif /* LED_CONTROL_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */



#ifdef RT3883
INT Set_CwCTest_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	char *Cwc_mode_str[] = {"OFF", "ON", "AUTO"};
	long x = simple_strtol(arg, 0, 10);

	if ((x >= 0) && (x <= 2))
	{
		pAd->FlgCWC = x;
		if (x == 0 || x == 1)
			RT3883_CWC_AsicSet(pAd, x);
		DBGPRINT(RT_DEBUG_OFF, ("CwC %s\n", Cwc_mode_str[x]));
	}
	else
	{
		DBGPRINT(RT_DEBUG_OFF, ("current FlgCWC = %d\n", pAd->FlgCWC));
	}

	return TRUE;
}
#endif /* RT3883 */


INT	Set_MemDebug_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef VENDOR_FEATURE2_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("Number of Packet Allocated = %lu\n", OS_NumOfPktAlloc));
	DBGPRINT(RT_DEBUG_OFF, ("Number of Packet Freed = %lu\n", OS_NumOfPktFree));
	DBGPRINT(RT_DEBUG_OFF, ("Offset of Packet Allocated/Freed = %lu\n",
			OS_NumOfPktAlloc - OS_NumOfPktFree));
#endif /* VENDOR_FEATURE2_SUPPORT */
	return TRUE;
}


#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
VOID RTMPApCliAddKey(
	IN	PRTMP_ADAPTER	    pAd,
	IN 	INT				apidx,
	IN	PNDIS_APCLI_802_11_KEY    pKey)
{
	ULONG				KeyIdx;
	MAC_TABLE_ENTRY  	*pEntry;
	STA_TR_ENTRY *tr_entry;
	UCHAR	ifIndex, BssIdx;
	PAPCLI_STRUCT pApCliEntry;
	struct wifi_dev *wdev;
	MAC_TABLE_ENTRY				*pMacEntry=(MAC_TABLE_ENTRY *)NULL;

    DBGPRINT(RT_DEBUG_TRACE, ("RTMPApCliAddKey ------>\n"));

	ifIndex = (UCHAR)apidx;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	wdev = &pApCliEntry->wdev;
	pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];
	tr_entry = &pAd->MacTab.tr_entry[pApCliEntry->MacTabWCID];
	BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;

	if (wdev->AuthMode >= Ndis802_11AuthModeWPA)
	{
		if (pKey->KeyIndex & 0x80000000)
		{
			    if (wdev->AuthMode == Ndis802_11AuthModeWPANone)
	            		{
		                NdisZeroMemory(pApCliEntry->PMK, 32);
		                NdisMoveMemory(pApCliEntry->PMK, pKey->KeyMaterial, pKey->KeyLength);
	                		goto end;
	            		}
			    /* Update PTK */
			    NdisZeroMemory(&pMacEntry->PairwiseKey, sizeof(CIPHER_KEY));
	            		pMacEntry->PairwiseKey.KeyLen = LEN_TK;
	            		NdisMoveMemory(pMacEntry->PairwiseKey.Key, pKey->KeyMaterial, LEN_TK);

		            if (pApCliEntry->PairCipher == Ndis802_11TKIPEnable)
		            {
		                NdisMoveMemory(pMacEntry->PairwiseKey.RxMic, pKey->KeyMaterial + LEN_TK, LEN_TKIP_MIC);
		                NdisMoveMemory(pMacEntry->PairwiseKey.TxMic, pKey->KeyMaterial + LEN_TK + LEN_TKIP_MIC, LEN_TKIP_MIC);
		            }
		            else

		            {
		            	NdisMoveMemory(pMacEntry->PairwiseKey.TxMic, pKey->KeyMaterial + LEN_TK, LEN_TKIP_MIC);
		                NdisMoveMemory(pMacEntry->PairwiseKey.RxMic, pKey->KeyMaterial + LEN_TK + LEN_TKIP_MIC, LEN_TKIP_MIC);
		            }

            /* Decide its ChiperAlg */
		        	if (pApCliEntry->PairCipher == Ndis802_11TKIPEnable)
		        		pMacEntry->PairwiseKey.CipherAlg = CIPHER_TKIP;
		        	else if (pApCliEntry->PairCipher == Ndis802_11AESEnable)
		        		pMacEntry->PairwiseKey.CipherAlg = CIPHER_AES;
		        	else
		        		pMacEntry->PairwiseKey.CipherAlg = CIPHER_NONE;

		    AsicAddPairwiseKeyEntry(
		        pAd,
		        (UCHAR)pMacEntry->Aid,
		        &pMacEntry->PairwiseKey);

			RTMPSetWcidSecurityInfo(pAd,
									BssIdx,
									0,
									pMacEntry->PairwiseKey.CipherAlg,
									(UCHAR)pMacEntry->Aid,
									PAIRWISEKEYTABLE);

	            if (pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
	            {
	                /* set 802.1x port control */
	                tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
			  pMacEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
	            }
		}
        else
        {
            /* Update GTK  */
            wdev->DefaultKeyId = (pKey->KeyIndex & 0xFF);
            NdisZeroMemory(&pApCliEntry->SharedKey[wdev->DefaultKeyId], sizeof(CIPHER_KEY));
            pApCliEntry->SharedKey[wdev->DefaultKeyId].KeyLen = LEN_TK;
            NdisMoveMemory(pApCliEntry->SharedKey[wdev->DefaultKeyId].Key, pKey->KeyMaterial, LEN_TK);

            if (pApCliEntry->GroupCipher == Ndis802_11TKIPEnable)
            {
                NdisMoveMemory(pApCliEntry->SharedKey[wdev->DefaultKeyId].RxMic, pKey->KeyMaterial + LEN_TK, LEN_TKIP_MIC);
                NdisMoveMemory(pApCliEntry->SharedKey[wdev->DefaultKeyId].TxMic, pKey->KeyMaterial + LEN_TK + LEN_TKIP_MIC, LEN_TKIP_MIC);
            }
            else

            {
            	NdisMoveMemory(pApCliEntry->SharedKey[wdev->DefaultKeyId].TxMic, pKey->KeyMaterial + LEN_TK, LEN_TKIP_MIC);
                NdisMoveMemory(pApCliEntry->SharedKey[wdev->DefaultKeyId].RxMic, pKey->KeyMaterial + LEN_TK + LEN_TKIP_MIC, LEN_TKIP_MIC);
            }

            /* Update Shared Key CipherAlg */
    		pApCliEntry->SharedKey[wdev->DefaultKeyId].CipherAlg = CIPHER_NONE;
    		if (pApCliEntry->GroupCipher == Ndis802_11TKIPEnable)
    			pApCliEntry->SharedKey[wdev->DefaultKeyId].CipherAlg = CIPHER_TKIP;
    		else if (pApCliEntry->GroupCipher == Ndis802_11AESEnable)
    			pApCliEntry->SharedKey[wdev->DefaultKeyId].CipherAlg = CIPHER_AES;

            /* Update group key information to ASIC Shared Key Table */
        	AsicAddSharedKeyEntry(pAd,
        						  BssIdx,
								(UCHAR)wdev->DefaultKeyId,
        						  &pApCliEntry->SharedKey[wdev->DefaultKeyId]);



        	/* Update ASIC WCID attribute table and IVEIV table */
        	RTMPAddWcidAttributeEntry(pAd,
        							  BssIdx,
        							  wdev->DefaultKeyId,
        							  pApCliEntry->SharedKey[wdev->DefaultKeyId].CipherAlg,
        							  NULL);


            /* set 802.1x port control */
             if (pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
            {
                /* set 802.1x port control */
                tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
		  pMacEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
            }
        }
	}
	else	/* dynamic WEP from wpa_supplicant */
	{
		UCHAR	CipherAlg;
    	PUCHAR	Key;

		if(pKey->KeyLength == 32)
			goto end;

		KeyIdx = pKey->KeyIndex & 0x0fffffff;

		if (KeyIdx < 4)
		{
			/* it is a default shared key, for Pairwise key setting */
			if (pKey->KeyIndex & 0x80000000)
			{
				pEntry = MacTableLookup(pAd, pKey->BSSID);

				if (pEntry && IS_ENTRY_APCLI(pEntry))
				{
					DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddKey: Set Pair-wise Key\n"));

					/* set key material and key length */
 					pEntry->PairwiseKey.KeyLen = (UCHAR )pKey->KeyLength;
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
								BssIdx,
								(UINT8)KeyIdx,
								pEntry->PairwiseKey.CipherAlg,
								pEntry->wcid,
								PAIRWISEKEYTABLE);
				}
			}
			else
            		{
				/* Default key for tx (shared key) */
				wdev->DefaultKeyId = (UCHAR) KeyIdx;

				/*/ set key material and key length */
				pApCliEntry->SharedKey[KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;
				NdisMoveMemory(pApCliEntry->SharedKey[KeyIdx].Key, &pKey->KeyMaterial, pKey->KeyLength);

				/* Set Ciper type */
				if (pKey->KeyLength == 5)
					pApCliEntry->SharedKey[KeyIdx].CipherAlg = CIPHER_WEP64;
				else
					pApCliEntry->SharedKey[KeyIdx].CipherAlg = CIPHER_WEP128;

	    			CipherAlg = pApCliEntry->SharedKey[KeyIdx].CipherAlg;
	    			Key = pApCliEntry->SharedKey[KeyIdx].Key;

					/* Set Group key material to Asic */
				AsicAddSharedKeyEntry(pAd, BssIdx, (UCHAR)KeyIdx
				, &pApCliEntry->SharedKey[KeyIdx]);

				/* STA doesn't need to set WCID attribute for group key */

				/* Update WCID attribute table and IVEIV table for this group key table */
				RTMPAddWcidAttributeEntry(pAd, BssIdx
				, (UCHAR)KeyIdx, CipherAlg, NULL);

			}
		}
	}
end:

    DBGPRINT(RT_DEBUG_INFO, ("<------ RTMPApCliAddKey\n"));

	return;
}
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/

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
INT	Set_PowerSaveLifeTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->MacTab.MsduLifeTime = (USHORT)simple_strtol(arg, 0, 10);
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
INT	Show_MbssInfo_Display_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 IdBss;
	UINT PhyMode;
	CHAR *mod_str = NULL;

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
		} /* End of if */

		switch(PhyMode)
		{
			case (WMODE_B | WMODE_G):
				mod_str = "BG Mixed";
				break;

			case (WMODE_B):
				mod_str = "B Only";
				break;

			case (WMODE_A):
				mod_str = "A Only";
				break;

			case (WMODE_A | WMODE_B | WMODE_G):
				mod_str = "ABG Mixed ==> BG Mixed";
				break;

			case (WMODE_G):
				mod_str = "G Only";
				break;

#ifdef DOT11_N_SUPPORT
			case (WMODE_A | WMODE_B | WMODE_G | WMODE_AN | WMODE_GN):
				mod_str = "ABGN Mixed ==> BGN Mixed";
				break;

			case (WMODE_GN):
				mod_str = "2.4G N Only";
				break;

			case (WMODE_G | WMODE_GN):
				mod_str = "GN Mixed";
				break;

			case (WMODE_A | WMODE_AN):
				mod_str = "AN Mixed";
				break;

			case (WMODE_B | WMODE_G | WMODE_GN):
				mod_str = "BGN Mixed";
				break;

			case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
				mod_str = "AGN Mixed";
				break;

			case (WMODE_AN):
				mod_str = "5G N Only";
				break;
#endif /* DOT11_N_SUPPORT */
		}
	}

	if (mod_str)
		DBGPRINT(RT_DEBUG_ERROR, ("%s\n", mod_str));
	DBGPRINT(RT_DEBUG_ERROR, ("\n"));

	return TRUE;
} /* End of Show_MbssInfo_Display_Proc */
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

#ifdef MT_MAC
/* Only for TimReq frame generating */
//TODO: Tim service establish.
INT	Set_AP_TimEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LONG interval;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	UCHAR APIndex = pObj->ioctl_if;
	BSS_STRUCT *pMbss;
	UINT32 Value = 0;

	interval = simple_strtol(arg, 0, 10);

	if ((interval < 0) || (interval > 255)) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s: interval is incorrect!!\n", __func__));
		return FALSE;
	}

	pMbss = &pAd->ApCfg.MBSSID[APIndex];

	ASSERT(pMbss);
	if (pMbss)
		wdev_tim_buf_init(pAd, &pMbss->tim_buf);

	if (interval > 0) //valid interval --> enable
	{
		RTMP_IO_WRITE32(pAd, LPON_PTTISR, 0x50);//set PreTTTT interval.

		RTMP_IO_READ32(pAd, LPON_TT0TPCR, &Value);
		Value = 0;
		//Carter, test 10ms after TBTT//Carter, I make TTTT always before than TBTT 10ms.
		Value &= ~TTTTn_OFFSET_OF_TBTTn_MASK;
		Value |= TTTTn_OFFSET_OF_TBTTn(0x2800);
		Value |= TTTTn_CAL_EN;
		RTMP_IO_WRITE32(pAd, LPON_TT0TPCR, Value);//enable TTTT0 calculation.

		RTMP_IO_READ32(pAd, LPON_TT4TPCR, &Value);
		Value = 0;
		Value &= ~TIM_PERIODn_MASK;
		Value |= TIM_PERIODn(interval * pAd->CommonCfg.BeaconPeriod);
		RTMP_IO_WRITE32(pAd, LPON_TT4TPCR, Value);//enable TTTT0 tim interval.

		RTMP_IO_READ32(pAd, LPON_MPTCR4, &Value);
		Value = 0;
		Value |= TTTT0_PERIODTIMER_EN;
		Value |= TTTT0_TIMEUP_EN;
		Value |= PRETTTT0TIMEUP_EN;
		Value |= PRETTTT0_TRIG_EN;
		Value |= PRETTTT0INT_EN;
		RTMP_IO_WRITE32(pAd, LPON_MPTCR4, Value);

		//enable dual_btim_enable
		RTMP_IO_READ32(pAd, AGG_ARCR, &Value);
		Value = Value | (1 << 31);
		RTMP_IO_WRITE32(pAd, AGG_ARCR, Value);

		RTMP_IO_WRITE32(pAd, 0x21280, 0x4f);//setting rate OFDM 9M

		Value = 0x00000000;

		if (APIndex == 0)
			Value |= 0x1;
		else if ((APIndex >= 1) && (APIndex <= 15))
			Value |= (0x1 << (APIndex + 15));

		RTMP_IO_WRITE32(pAd, ARB_BTIMCR0, Value);//enable BTIM_EN bit in ARB_BTIMCR0
		RTMP_IO_WRITE32(pAd, ARB_BTIMCR1, Value);//enable 2 TIM broadcast

		RTMP_IO_WRITE32(pAd, HWIER4, 0x80008000);//enable TTTT0/PRETTTT0 interrupt.
	}
	else //interval 0 --> disable
	{
		if (APIndex == 0)
			Value = Value & 0x0;
		else if ((APIndex >= 1) && (APIndex <= 15))
			Value = Value & (0x0 << (APIndex + 15));

		RTMP_IO_WRITE32(pAd, ARB_BTIMCR0, Value);//disable BTIM_EN bit in ARB_BTIMCR0
	}

	return TRUE;
}

#ifdef DBG
INT Set_AP_DumpTime_Proc(
    IN      PRTMP_ADAPTER   pAd,
    IN      RTMP_STRING     *arg)
{
    int apidx = 0, i = 0;
    BSS_STRUCT *pMbss;

	DBGPRINT(RT_DEBUG_OFF, ("\n\t%-10s\t%-10s\n", "PreTBTTTime", "TBTTTime"));
	DBGPRINT(RT_DEBUG_OFF, ("Idx[%d]:%-10lu\t%-10lu\n", i, pAd->HandlePreInterruptTime,
			pAd->HandleInterruptTime));

    for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
        pMbss = &pAd->ApCfg.MBSSID[apidx];

        if(!BeaconTransmitRequired(pAd, apidx, pMbss))
            continue;

		DBGPRINT(RT_DEBUG_OFF, ("\n%s:%d\tBcn_State:%d\t%-10s: %d\n", "Apidx"
				, apidx, pMbss->bcn_buf.bcn_state, "recover", pMbss->bcn_recovery_num));
		DBGPRINT(RT_DEBUG_OFF, ("\t%-10s\t%-10s\t%-10s\t%-10s\n", "WriteBcnRing", "BcnDmaDone", "TXS_TSF", "TXS_SN"));
		for (i = 0; i < MAX_TIME_RECORD; i++) {
			DBGPRINT(RT_DEBUG_OFF, ("Idx[%d]:\t%-10lu\t%-10lu\t%-10lu\t%-10lu\n"
					, i, pMbss->WriteBcnDoneTime[i], pMbss->BcnDmaDoneTime[i], pMbss->TXS_TSF[i], pMbss->TXS_SN[i]));
        }
    }

    return TRUE;
}

INT Set_BcnStateCtrl_Proc(
    IN  PRTMP_ADAPTER   pAd,
    IN  RTMP_STRING *arg)
{
    UCHAR bcn_state;
    POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
    UCHAR apIndex = pObj->ioctl_if;
    BSS_STRUCT *pMbss = NULL;

	bcn_state = (UCHAR)simple_strtol(arg, 0, 10);

    if (pAd->chipCap.hif_type != HIF_MT) {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: chipCap is not HIF_MT\n", __func__));
        return FALSE;
    }

    if ((bcn_state < BCN_TX_IDLE) || (bcn_state > BCN_TX_STOP)) {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: bcn_state is incorrect!!\n", __func__));
        return FALSE;
    }

    pMbss = &pAd->ApCfg.MBSSID[apIndex];

    ASSERT(pMbss);
    if (pMbss == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: pMbss == NULL!!\n", __func__));
        return FALSE;
    }

	if (pMbss->bcn_buf.bcn_state != BCN_TX_IDLE) {
		DBGPRINT(RT_DEBUG_OFF, ("MBSSID[%d], bcn_buf.bcn_state = %d, will change to %d\n"
				, apIndex, pMbss->bcn_buf.bcn_state, bcn_state));
        RTMP_SEM_LOCK(&pAd->BcnRingLock);
        pMbss->bcn_buf.bcn_state = bcn_state;
        RTMP_SEM_UNLOCK(&pAd->BcnRingLock);
    }

        return TRUE;
}
#endif

INT setApTmrEnableProc(
    IN  PRTMP_ADAPTER   pAd,
    IN  RTMP_STRING *arg)
{
	LONG enable;
    POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
    UCHAR apIndex = pObj->ioctl_if;
    BSS_STRUCT *pMbss = NULL;
    UINT32  value = 0;

    enable = simple_strtol(arg, 0, 10);

    if (pAd->chipCap.hif_type != HIF_MT) {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: chipCap is not HIF_MT\n", __func__));
        return FALSE;
    }

    if ((enable < 0) || (enable > 2)) {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: enable is incorrect!!\n", __func__));
        return FALSE;
    }

    pMbss = &pAd->ApCfg.MBSSID[apIndex];

    ASSERT(pMbss);
    if (pMbss == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: pMbss == NULL!!\n", __func__));
        return FALSE;
    }

    switch (enable)
    {
        case 1://initialiter
        {
            DBGPRINT(RT_DEBUG_OFF, ("%s: enable TMR report, as Initialiter\n", __func__));
            RTMP_IO_READ32(pAd, RMAC_TMR_PA, &value);
            value = value | BIT31;
            value = value & ~BIT30;
            value = value | 0x34;//Action frame register
            RTMP_IO_WRITE32(pAd, RMAC_TMR_PA, value);
            pAd->chipCap.TmrEnable = 1;
        }
            break;
        case 2://responder
        {
            DBGPRINT(RT_DEBUG_OFF, ("%s: enable TMR report, as Responser\n", __func__));
            RTMP_IO_READ32(pAd, RMAC_TMR_PA, &value);
            value = value | BIT31;
            value = value | BIT30;
            value = value | 0x34;//Action frame register
            RTMP_IO_WRITE32(pAd, RMAC_TMR_PA, value);
            pAd->chipCap.TmrEnable = 2;
        }
            break;

        case 0://disable
	default:
        {
            DBGPRINT(RT_DEBUG_OFF, ("%s: disable TMR report\n", __func__));
            RTMP_IO_READ32(pAd, RMAC_TMR_PA, &value);
            value = value & ~BIT31;
            RTMP_IO_WRITE32(pAd, RMAC_TMR_PA, value);
            pAd->chipCap.TmrEnable = FALSE;
        }
    }
	return TRUE;
}
#endif /* MT_MAC */


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
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_SET\n"));
				Status = -EINVAL;
				break;
			}

			Status = RTMPAPPrivIoctlSet(pAd, wrq);
			break;

		case CMD_RT_PRIV_IOCTL:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RT_PRIV_IOCTL\n"));
				Status = -EINVAL;
				break;
			}

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
						wrq->u.data.length = 4;
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
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_SHOW\n"));
				Status = -EINVAL;
				break;
			}

			Status = RTMPAPPrivIoctlShow(pAd, wrq);
			break;

#ifdef WSC_AP_SUPPORT
		case CMD_RTPRIV_IOCTL_SET_WSCOOB:
			RTMPIoctlSetWSCOOB(pAd);
		    break;
#endif/*WSC_AP_SUPPORT*/

		case CMD_RTPRIV_IOCTL_GET_MAC_TABLE:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_GET_MAC_TABLE\n"));
				Status = -EINVAL;
				break;
			}

			RTMPIoctlGetMacTable(pAd,wrq);
			break;

#if defined (AP_SCAN_SUPPORT) || defined (CONFIG_STA_SUPPORT)
		case CMD_RTPRIV_IOCTL_GSITESURVEY:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_GSITESURVEY\n"));
				Status = -EINVAL;
				break;
			}

			RTMPIoctlGetSiteSurvey(pAd,wrq);
			break;
#endif /* AP_SCAN_SUPPORT */

		case CMD_RTPRIV_IOCTL_STATISTICS:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_STATISTICS\n"));
				Status = -EINVAL;
				break;
			}

			RTMPIoctlStatistics(pAd, wrq);
			break;

#ifdef WSC_AP_SUPPORT
		case CMD_RTPRIV_IOCTL_WSC_PROFILE:
		    RTMPIoctlWscProfile(pAd, wrq);
		    break;
#endif /* WSC_AP_SUPPORT */

#ifdef DOT11_N_SUPPORT
		case CMD_RTPRIV_IOCTL_QUERY_BATABLE:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_QUERY_BATABLE\n"));
				Status = -EINVAL;
				break;
			}

			RTMPIoctlQueryBaTable(pAd, wrq);
			break;
#endif /* DOT11_N_SUPPORT */

		case CMD_RTPRIV_IOCTL_E2P:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_E2P\n"));
				Status = -EINVAL;
				break;
			}

			RTMPAPIoctlE2PROM(pAd, wrq);
			break;

#ifdef DBG
		case CMD_RTPRIV_IOCTL_BBP:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_BBP\n"));
				Status = -EINVAL;
				break;
			}

			RTMPAPIoctlBBP(pAd, wrq);
			break;

		case CMD_RTPRIV_IOCTL_MAC:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_MAC\n"));
				Status = -EINVAL;
				break;
			}

			RTMPIoctlMAC(pAd, wrq);
			break;

#ifdef RTMP_RF_RW_SUPPORT
		case CMD_RTPRIV_IOCTL_RF:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_RF\n"));
				Status = -EINVAL;
				break;
			}

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
			 if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT\n"));
				Status = -EINVAL;
				break;
			}

			RTMPIoctlGetMacTableStaInfo(pAd, wrq);
			break;

		case CMD_RTPRIV_IOCTL_AP_SIOCGIFHWADDR:
			if (!wrq) {
				DBGPRINT(RT_DEBUG_WARN,
					 ("null wrq in CMD_RTPRIV_IOCTL_AP_SIOCGIFHWADDR\n"));
				Status = -EINVAL;
				break;
			}

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
				pSSID->length = pAd->ApCfg.MBSSID[pSSID->apidx].SsidLen;
				pSSID->pSsidStr = (char *)pAd->ApCfg.MBSSID[pSSID->apidx].Ssid;
			}
		}
			break;

#ifdef MBSS_SUPPORT
		case CMD_RTPRIV_IOCTL_MBSS_BEACON_UPDATE:
			//CFG TODO	
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

#ifdef MT_MAC
        case CMD_RTPRIV_IOCTL_MBSS_CR_ENABLE:
			if (mbss_cr_enable(pData) != 0)
				return NDIS_STATUS_FAILURE;
			break;

        case CMD_RTPRIV_IOCTL_MBSS_CR_DISABLE:
			if (mbss_cr_disable(pData) != 0)
				return NDIS_STATUS_FAILURE;
			break;
#endif /* MT_MAC */
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
			WscInit(pAd, TRUE, Data);
#endif /* WSC_AP_SUPPORT */
#endif /* APCLI_SUPPORT */
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

		case CMD_RTPRIV_IOCTL_APC_INIT:
			APCli_Init(pAd, pData);
			break;

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
						PRINT_MAC(pBssidStr)));
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
				HtPhyMode = pAd->MeshTab.wdev.HTPhyMode;
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
				HtPhyMode = pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.HTPhyMode;

			RtmpDrvMaxRateGet(pAd, (UINT8)HtPhyMode.field.MODE
							, (UINT8)HtPhyMode.field.ShortGI
							, (UINT8)HtPhyMode.field.BW
							, (UINT8)HtPhyMode.field.MCS
							, (UINT32 *)&pRate->BitRate);
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
INT Set_P2p_OpMode_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	UINT OpMode;
	POS_COOKIE pObj;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	OpMode = simple_strtol(arg, 0, 16);


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

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2p_OpMode_Proc::(OpMode = %d)\n", pObj->ioctl_if, OpMode));

	return TRUE;
}

INT Set_P2pCli_Enable_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
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


INT Set_P2pCli_Ssid_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	POS_COOKIE pObj;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	INT success = FALSE;
	/*UCHAR keyMaterial[40]; */
	UCHAR PskKey[100];

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
			NdisZeroMemory(PskKey, 100);
			NdisMoveMemory(PskKey, pAd->ApCfg.ApCliTab[ifIndex].PSK, pAd->ApCfg.ApCliTab[ifIndex].PSKLen);

#if 0
			if ((strlen((RTMP_STRING *) PskKey) >= 8) && (strlen((RTMP_STRING *) PskKey) < 64))
			{
	    		RtmpPasswordHash((char *)PskKey, (PUCHAR) pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, keyMaterial);
	    		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].PMK, keyMaterial, 32);
			}
			else if ((INT) strlen((RTMP_STRING *) PskKey) == 64)
			{
				AtoH((RTMP_STRING *) PskKey, pAd->ApCfg.ApCliTab[ifIndex].PMK, 32);
			}
#else
			RT_CfgSetWPAPSKKey(pAd, (RTMP_STRING *)PskKey,
									pAd->ApCfg.ApCliTab[ifIndex].PSKLen,
									(PUCHAR)pAd->ApCfg.ApCliTab[ifIndex].CfgSsid,
									pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen,
									pAd->ApCfg.ApCliTab[ifIndex].PMK);
#endif
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
INT	Set_P2pCli_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG       i;
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;

	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	if (rtstrcasecmp(arg, "WEPAUTO") == TRUE)
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if (rtstrcasecmp(arg, "SHARED") == TRUE)
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeShared;
	else if (rtstrcasecmp(arg, "WPAPSK") == TRUE)
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeWPAPSK;
	else if (rtstrcasecmp(arg, "WPA2PSK") == TRUE)
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeWPA2PSK;
	else
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeOpen;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (IS_ENTRY_APCLI(&pAd->MacTab.Content[i]))
		{
			pAd->MacTab.tr_entry[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}

    RTMPMakeRSNIE(pAd, pAd->ApCfg.ApCliTab[ifIndex].AuthMode, pAd->ApCfg.ApCliTab[ifIndex].WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));

	pAd->ApCfg.ApCliTab[ifIndex].DefaultKeyId  = 0;

	if(pAd->ApCfg.ApCliTab[ifIndex].AuthMode >= Ndis802_11AuthModeWPA)
		pAd->ApCfg.ApCliTab[ifIndex].DefaultKeyId = 1;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_AuthMode_Proc::(AuthMode=%d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].AuthMode));
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
INT	Set_P2pCli_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->WepStatus = Ndis802_11WEPDisabled;
	if (rtstrcasecmp(arg, "WEP") == TRUE)
    {
		if (pApCliEntry->AuthMode < Ndis802_11AuthModeWPA)
			pApCliEntry->WepStatus = Ndis802_11WEPEnabled;
	}
	else if (rtstrcasecmp(arg, "TKIP") == TRUE)
	{
		if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA)
			pApCliEntry->WepStatus = Ndis802_11TKIPEnable;
    }
	else if (rtstrcasecmp(arg, "AES") == TRUE)
	{
		if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA)
			pApCliEntry->WepStatus = Ndis802_11AESEnable;
	}
	else
	{
		pApCliEntry->WepStatus = Ndis802_11WEPDisabled;
	}

	pApCliEntry->PairCipher     = pApCliEntry->WepStatus;
	pApCliEntry->GroupCipher    = pApCliEntry->WepStatus;
	pApCliEntry->bMixCipher		= FALSE;

	if (pApCliEntry->WepStatus >= Ndis802_11TKIPEnable)
		pApCliEntry->DefaultKeyId = 1;

	RTMPMakeRSNIE(pAd, pApCliEntry->AuthMode, pApCliEntry->WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));
	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_EncrypType_Proc::(EncrypType=%d)\n", ifIndex, pApCliEntry->WepStatus));

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
INT	Set_P2pCli_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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
		pApCliEntry->DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  /* Invalid argument  */

	DBGPRINT(RT_DEBUG_TRACE, ("IF(p2p%d) Set_P2pCli_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", ifIndex, pApCliEntry->DefaultKeyId));

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
INT	Set_P2pCli_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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
INT	Set_P2pCli_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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
INT	Set_P2pCli_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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
INT	Set_P2pCli_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
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

INT Set_P2pCli_TxMode_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->DesiredTransmitSetting.field.FixedTxMode = RT_CfgSetFixedTxPhyMode(arg);
	DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_TxMode_Proc = %d\n", ifIndex,
									pApCliEntry->DesiredTransmitSetting.field.FixedTxMode));

	return TRUE;
}

INT Set_P2pCli_TxMcs_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	POS_COOKIE 		pObj;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT	pApCliEntry = NULL;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_P2P)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->DesiredTransmitSetting.field.MCS =
			RT_CfgSetTxMCSProc(arg, &pApCliEntry->bAutoTxRateSwitch);

	if (pApCliEntry->DesiredTransmitSetting.field.MCS == MCS_AUTO)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_TxMcs_Proc = AUTO\n", ifIndex));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(p2p%d) Set_P2pCli_TxMcs_Proc = %d\n", ifIndex,
									pApCliEntry->DesiredTransmitSetting.field.MCS));
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
#endif /* P2P_SUPPORT */

#ifdef RT6352
INT Set_MacWatchDog_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN bEnableWD;

	bEnableWD = simple_strtol(arg, 0, 10);

	pAd->bEnableMacWD = bEnableWD;

	DBGPRINT(RT_DEBUG_TRACE, ("EnableMacWD = %d \n", pAd->bEnableMacWD));

	return TRUE;
}


#if 0 /* Not Used */
INT Set_DumpRF_Proc(
	IN RTMP_ADAPTER	*pAd,
	IN RTMP_STRING *arg)
{
	int i;
	UCHAR Value;

	for (i = 0; i < pAd->chipCap.MaxNumOfRfId; i++)
	{
		RT635xReadRFRegister(pAd, RF_BANK0, i, &Value);
		printk("%d %03d %02X\n", RF_BANK0, i, Value);
	}

	for (i = 0; i < pAd->chipCap.MaxNumOfRfId; i++)
	{
		RT635xReadRFRegister(pAd, RF_BANK4, i, &Value);
		printk("%d %03d %02X\n", RF_BANK4, i, Value);
	}

	for (i = 0; i < pAd->chipCap.MaxNumOfRfId; i++)
	{
		RT635xReadRFRegister(pAd, RF_BANK5, i, &Value);
		printk("%d %03d %02X\n", RF_BANK5, i, Value);
	}

	for (i = 0; i < pAd->chipCap.MaxNumOfRfId; i++)
	{
		RT635xReadRFRegister(pAd, RF_BANK6, i, &Value);
		printk("%d %03d %02X\n", RF_BANK6, i, Value);
	}

	for (i = 0; i < pAd->chipCap.MaxNumOfRfId; i++)
	{
		RT635xReadRFRegister(pAd, RF_BANK7, i, &Value);
		printk("%d %03d %02X\n", RF_BANK7, i, Value);
	}

	return TRUE;
}
#endif
INT Set_DumpBBP_Proc(
	IN RTMP_ADAPTER	*pAd,
	IN RTMP_STRING *arg)
{
	int i;
	UCHAR BBPValue = 0;

	for (i = 0; i < pAd->chipCap.MaxNumOfBbpId; i++)
	{
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, i, &BBPValue);
		DBGPRINT(RT_DEBUG_OFF, ("%03d = %02X\n", i, BBPValue));
	}

	for (i = 0; i < pAd->chipCap.MaxNumOfBbpId; i++)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R158, i);
		DBGPRINT(RT_DEBUG_OFF, ("%03d = %02X\n", BBP_R158, i));

		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R159, &BBPValue);
		DBGPRINT(RT_DEBUG_OFF, ("%03d = %02X\n", BBP_R159, BBPValue));
	}

	for (i = 0; i < pAd->chipCap.MaxNumOfBbpId; i++)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R195, i);
		DBGPRINT(RT_DEBUG_OFF, ("%03d = %02X\n", BBP_R195, i));

		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R196, &BBPValue);
		DBGPRINT(RT_DEBUG_OFF, ("%03d = %02X\n", BBP_R196, BBPValue));
	}

	return TRUE;
}

INT Set_KickOutHwNull_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN bEnableWD;

	bEnableWD = simple_strtol(arg, 0, 10);

	RtmpKickOutHwNullFrame(pAd, TRUE, TRUE);

	DBGPRINT(RT_DEBUG_ERROR, ("KickOutHwNull = %d \n", bEnableWD));

	return TRUE;
}
#endif /* RT6352 */

#ifdef DYNAMIC_VGA_SUPPORT
#if 0 /* Not Used */
INT Set_DyncVga_FalseCCATh_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	ULONG th;

	th = simple_strtol(arg, 0, 10);

	if (th > 65535)
		th = 65535;

	pAd->CommonCfg.lna_vga_ctl.nFalseCCATh = th;

	DBGPRINT(RT_DEBUG_OFF, ("%s: set falseCCA threshold %lu for microwave oven application!!\n", __FUNCTION__, th));

	return TRUE;
}

INT Set_DyncVga_LowFalseCCATh_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	ULONG th;

	th = simple_strtol(arg, 0, 10);

	if (th > 65535)
		th = 65535;

	pAd->CommonCfg.lna_vga_ctl.nLowFalseCCATh = th;

	DBGPRINT(RT_DEBUG_OFF, ("%s: set low falseCCA threshold %lu for microwave oven application!!\n", __FUNCTION__, th));

	return TRUE;
}
#endif /* Not Used */
INT Set_DyncVgaEnable_Proc(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *arg)
{
	UINT Enable;

	Enable = simple_strtol(arg, 0, 10);

	pAd->CommonCfg.lna_vga_ctl.bDyncVgaEnable = (Enable > 0) ? TRUE : FALSE;

	if (pAd->CommonCfg.lna_vga_ctl.bDyncVgaEnable == TRUE)	{
		dynamic_vga_enable(pAd);
	} else {
		dynamic_vga_disable(pAd);
	}

	DBGPRINT(RT_DEBUG_TRACE,
				("Set_DyncVgaEnable_Proc::(enable = %d)\n",
				pAd->CommonCfg.lna_vga_ctl.bDyncVgaEnable));

	return TRUE;
}


INT set_false_cca_hi_th(PRTMP_ADAPTER pAd, RTMP_STRING *arg)
{
	INT32 val = simple_strtol(arg, 0, 10);

	pAd->CommonCfg.lna_vga_ctl.nFalseCCATh = (val <= 0) ? 800 : val;

	DBGPRINT(RT_DEBUG_OFF, ("%s::(false cca high threshould = %d)\n",
		__FUNCTION__, pAd->CommonCfg.lna_vga_ctl.nFalseCCATh));

	return TRUE;
}


INT set_false_cca_low_th(PRTMP_ADAPTER pAd, RTMP_STRING *arg)
{
	INT32 val = simple_strtol(arg, 0, 10);

	pAd->CommonCfg.lna_vga_ctl.nLowFalseCCATh = (val <= 0) ? 10 : val;

	DBGPRINT(RT_DEBUG_OFF, ("%s::(false cca low threshould = %d)\n",
		__FUNCTION__, pAd->CommonCfg.lna_vga_ctl.nLowFalseCCATh));

	return TRUE;
}
#endif /* DYNAMIC_VGA_SUPPORT */

#ifdef CONFIG_HOTSPOT
static INT Set_AP_HS_IE(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 EID,
	IN RTMP_STRING *IE,
	IN UINT32 IELen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;
	PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[apidx].HotSpotCtrl;

	switch (EID)
	{
		case IE_INTERWORKING:
			os_alloc_mem(NULL, &pHSCtrl->InterWorkingIE, IELen);
			NdisMoveMemory(pHSCtrl->InterWorkingIE, IE, IELen);
			pHSCtrl->InterWorkingIELen = IELen;
			pHSCtrl->AccessNetWorkType  = (*(IE + 2)) & 0x0F;
			if (IELen > 3)
			{
				pHSCtrl->IsHessid = TRUE;
				if (IELen == 7)
					NdisMoveMemory(pHSCtrl->Hessid, IE + 3, MAC_ADDR_LEN);
				else
					NdisMoveMemory(pHSCtrl->Hessid, IE + 5 , MAC_ADDR_LEN);
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Set Interworking IE\n"));
			break;
		case IE_ADVERTISEMENT_PROTO:
			os_alloc_mem(NULL, &pHSCtrl->AdvertisementProtoIE, IELen);
			NdisMoveMemory(pHSCtrl->AdvertisementProtoIE, IE, IELen);
			pHSCtrl->AdvertisementProtoIELen = IELen;
			DBGPRINT(RT_DEBUG_TRACE, ("Set Advertisement Protocol IE\n"));
			break;
		case IE_QOS_MAP_SET:
			{
				int tmp = 0;
				char *pos = (char *)(IE+2);
				os_alloc_mem(NULL, &pHSCtrl->QosMapSetIE, IELen);
				NdisMoveMemory(pHSCtrl->QosMapSetIE, IE, IELen);
				pHSCtrl->QosMapSetIELen = IELen;
				for(tmp = 0;tmp<(IELen-16-2)/2;tmp++)
				{
					pHSCtrl->DscpException[tmp] = *pos & 0xff;
					pHSCtrl->DscpException[tmp] |= (*(pos+1) & 0xff) << 8;
					pos += 2;
				}
				for(tmp = 0;tmp<8;tmp++)
				{
					pHSCtrl->DscpRange[tmp] = *pos & 0xff;
					pHSCtrl->DscpRange[tmp] |= (*(pos+1) & 0xff) << 8;
					pos += 2;
				}
				DBGPRINT(RT_DEBUG_TRACE, ("Set Qos MAP Set IE\n"));
			}
			break;
		case IE_ROAMING_CONSORTIUM:
			os_alloc_mem(NULL, &pHSCtrl->RoamingConsortiumIE, IELen);
			NdisMoveMemory(pHSCtrl->RoamingConsortiumIE, IE, IELen);
			pHSCtrl->RoamingConsortiumIELen = IELen;
			DBGPRINT(RT_DEBUG_TRACE, ("Set Roaming Consortium IE\n"));
			break;
		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Unknown IE(EID = %d)\n", __FUNCTION__, EID));
			break;
	}

	return TRUE;
}
#endif

static INT Set_AP_VENDOR_SPECIFIC_IE(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 OUIType,
	IN RTMP_STRING *IE,
	IN UINT32 IELen)
{
#ifdef CONFIG_HOTSPOT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;

	PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[apidx].HotSpotCtrl;
#endif

	switch (OUIType) {
#ifdef CONFIG_HOTSPOT
		case OUI_P2P:
			os_alloc_mem(NULL, &pHSCtrl->P2PIE, IELen);
			NdisMoveMemory(pHSCtrl->P2PIE, IE, IELen);
			pHSCtrl->P2PIELen = IELen;
			DBGPRINT(RT_DEBUG_TRACE, ("Set P2P IE\n"));
			break;
		case OUI_HS2_INDICATION:
			os_alloc_mem(NULL, &pHSCtrl->HSIndicationIE, IELen);
			NdisMoveMemory(pHSCtrl->HSIndicationIE, IE, IELen);
			pHSCtrl->HSIndicationIELen = IELen;
			DBGPRINT(RT_DEBUG_TRACE, ("Set HS2.0 Indication IE\n"));
			break;
#endif
		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Unknown OUIType = %d\n", __FUNCTION__, OUIType));
		break;

	}

	return TRUE;
}

#ifdef CONFIG_DOT11V_WNM
static INT Set_AP_WNM_IE(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 EID,
	IN RTMP_STRING *IE,
	IN UINT32 IELen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;
	PWNM_CTRL pWNMCtrl =  &pAd->ApCfg.MBSSID[apidx].WNMCtrl;

	switch (EID)
	{
		case IE_TIME_ADVERTISEMENT:
			os_alloc_mem(NULL, &pWNMCtrl->TimeadvertisementIE, IELen);
			NdisMoveMemory(pWNMCtrl->TimeadvertisementIE, IE, IELen);
			pWNMCtrl->TimeadvertisementIELen = IELen;
			DBGPRINT(RT_DEBUG_TRACE, ("Set Time Advertisement IE\n"));
			break;
		case IE_TIME_ZONE:
			os_alloc_mem(NULL, &pWNMCtrl->TimezoneIE, IELen);
			NdisMoveMemory(pWNMCtrl->TimezoneIE, IE, IELen);
			pWNMCtrl->TimezoneIELen = IELen;
			DBGPRINT(RT_DEBUG_TRACE, ("Set Time Zone IE\n"));
			break;
		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Unknown IE(EID = %d)\n", __FUNCTION__, EID));
			break;
	}

	return TRUE;
}
#endif

INT Set_AP_IE(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *IE,
	IN UINT32 IELen)
{
	UINT8 EID;
	UINT8 OUIType;

	EID = *IE;

	switch (EID)
	{
#ifdef CONFIG_HOTSPOT
		case IE_INTERWORKING:
		case IE_ADVERTISEMENT_PROTO:
		case IE_QOS_MAP_SET:
		case IE_ROAMING_CONSORTIUM:
			Set_AP_HS_IE(pAd, EID, IE, IELen);
			break;
#endif

#ifdef CONFIG_DOT11V_WNM
		case IE_TIME_ADVERTISEMENT:
		case IE_TIME_ZONE:
			Set_AP_WNM_IE(pAd, EID, IE, IELen);
			break;
#endif
		case IE_VENDOR_SPECIFIC:
			OUIType = *(IE + 5);
			Set_AP_VENDOR_SPECIFIC_IE(pAd, OUIType, IE, IELen);
			break;
		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Unknown IE(EID = %d)\n", __FUNCTION__, EID));
			break;
	}

	return TRUE;
}

#ifdef CONFIG_HOTSPOT
INT Send_ANQP_Rsp(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *PeerMACAddr,
	IN RTMP_STRING *ANQPRsp,
	IN UINT32 ANQPRspLen)
{
	UCHAR *Buf;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;
	PHOTSPOT_CTRL pHSCtrl = &pAd->ApCfg.MBSSID[apidx].HotSpotCtrl;
	PGAS_CTRL pGASCtrl = &pAd->ApCfg.MBSSID[apidx].GASCtrl;
	GAS_EVENT_DATA *Event;
	GAS_PEER_ENTRY *GASPeerEntry;
	GAS_QUERY_RSP_FRAGMENT *GASQueryRspFrag, *Tmp;
	UINT32 Len = 0, i, QueryRspOffset = 0;
	BOOLEAN Cancelled;
	BOOLEAN IsFound = FALSE;
	unsigned long irqFlags=0;

	RTMP_SEM_LOCK(&pGASCtrl->GASPeerListLock);
	/* Cancel PostReply timer after receiving daemon response */
	DlListForEach(GASPeerEntry, &pGASCtrl->GASPeerList, GAS_PEER_ENTRY, List)
	{
		if (MAC_ADDR_EQUAL(GASPeerEntry->PeerMACAddr, PeerMACAddr))
		{
			if (GASPeerEntry->PostReplyTimerRunning)
			{
				RTMPCancelTimer(&GASPeerEntry->PostReplyTimer, &Cancelled);
				GASPeerEntry->PostReplyTimerRunning = FALSE;
			}

			break;
		}
	}

	RTMP_SEM_UNLOCK(&pGASCtrl->GASPeerListLock);

	os_alloc_mem(NULL, (UCHAR **)&Buf, sizeof(*Event) + ANQPRspLen);

	if (!Buf)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
		
		//RTMP_SEM_EVENT_WAIT(&pGASCtrl->GASPeerListLock, Ret);
		RTMP_INT_LOCK(&pAd->irq_lock, irqFlags);
		DlListForEach(GASPeerEntry, &pGASCtrl->GASPeerList, GAS_PEER_ENTRY, List)
		{
			if (MAC_ADDR_EQUAL(GASPeerEntry->PeerMACAddr, PeerMACAddr))
			{
				IsFound = TRUE;
				DBGPRINT(RT_DEBUG_OFF, ("!!GAS RSP DialogToken!! = %x\n",
						GASPeerEntry->DialogToken));
				break;
			}
		}
		//RTMP_SEM_EVENT_UP(&pGASCtrl->GASPeerListLock);
		RTMP_INT_UNLOCK(&pAd->irq_lock, irqFlags);
		
		if (IsFound) {
			//RTMP_SEM_EVENT_WAIT(&pGASCtrl->GASPeerListLock, Ret);
			RTMP_INT_LOCK(&pAd->irq_lock, irqFlags);
			DlListDel(&GASPeerEntry->List);
			DlListInit(&GASPeerEntry->GASQueryRspFragList);
			//RTMP_SEM_EVENT_UP(&pGASCtrl->GASPeerListLock);
			RTMP_INT_UNLOCK(&pAd->irq_lock, irqFlags);
			if (GASPeerEntry->PostReplyTimerRunning)
			{
				RTMPCancelTimer(&GASPeerEntry->PostReplyTimer, &Cancelled);
				GASPeerEntry->PostReplyTimerRunning = FALSE;
			}
			RTMPReleaseTimer(&GASPeerEntry->PostReplyTimer, &Cancelled);

			if (GASPeerEntry->GASRspBufferingTimerRunning)
			{
				RTMPCancelTimer(&GASPeerEntry->GASRspBufferingTimer, &Cancelled);
				GASPeerEntry->GASRspBufferingTimerRunning = FALSE;
			}
			RTMPReleaseTimer(&GASPeerEntry->GASRspBufferingTimer, &Cancelled);
			os_free_mem(NULL, GASPeerEntry);
		}
		goto error0;
	}

	NdisZeroMemory(Buf, sizeof(*Event) + ANQPRspLen);

	Event = (GAS_EVENT_DATA *)Buf;

	Event->ControlIndex = apidx;
	Len += 1;
	NdisMoveMemory(Event->PeerMACAddr, PeerMACAddr, MAC_ADDR_LEN);
	Len += MAC_ADDR_LEN;

	if (ANQPRspLen <= pHSCtrl->MMPDUSize)
	{
		Event->EventType = GAS_RSP;
		Len += 2;

		RTMP_SEM_LOCK(&pGASCtrl->GASPeerListLock);
		DlListForEach(GASPeerEntry, &pGASCtrl->GASPeerList, GAS_PEER_ENTRY, List)
		{
			if (MAC_ADDR_EQUAL(GASPeerEntry->PeerMACAddr, PeerMACAddr))
			{
				IsFound = TRUE;
				DBGPRINT(RT_DEBUG_OFF, ("GAS RSP DialogToken = %x\n", GASPeerEntry->DialogToken));
				Event->u.GAS_RSP_DATA.DialogToken = GASPeerEntry->DialogToken;
				Len += 1;
				
				break;
			}
		}
		RTMP_SEM_UNLOCK(&pGASCtrl->GASPeerListLock);

		if (!IsFound) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s Can not find peer address in GASPeerList\n", __FUNCTION__));
			goto error1;
		}

		DBGPRINT(RT_DEBUG_OFF, ("GAS RSP DialogToken = %x\n", GASPeerEntry->DialogToken));
		Event->u.GAS_RSP_DATA.StatusCode = 0;
		Len += 2;
		Event->u.GAS_RSP_DATA.GASComebackDelay = 0;
		Len += 2;
		Event->u.GAS_RSP_DATA.AdvertisementProID = ACCESS_NETWORK_QUERY_PROTOCOL;
		Len += 1;
		Event->u.GAS_RSP_DATA.QueryRspLen = ANQPRspLen;
		Len += 2;

		NdisMoveMemory(Event->u.GAS_RSP_DATA.QueryRsp, ANQPRsp, ANQPRspLen);
		Len += ANQPRspLen;

		GASPeerEntry->CurrentState = WAIT_GAS_RSP;

		SendGASRsp(pAd, (GAS_EVENT_DATA *)Buf);
	}
	else
	{
		Event->EventType = GAS_RSP_MORE;
		Len += 2;

		RTMP_SEM_LOCK(&pGASCtrl->GASPeerListLock);
		DlListForEach(GASPeerEntry, &pGASCtrl->GASPeerList, GAS_PEER_ENTRY, List)
		{
			if (MAC_ADDR_EQUAL(GASPeerEntry->PeerMACAddr, PeerMACAddr))
			{
				IsFound = TRUE;

				Event->u.GAS_RSP_MORE_DATA.DialogToken = GASPeerEntry->DialogToken;
				Len += 1;

				if ((ANQPRspLen % pHSCtrl->MMPDUSize) == 0)
					GASPeerEntry->GASRspFragNum = ANQPRspLen / pHSCtrl->MMPDUSize;
				else
					GASPeerEntry->GASRspFragNum = (ANQPRspLen / pHSCtrl->MMPDUSize) + 1;

				GASPeerEntry->CurrentGASFragNum = 0;

				for (i = 0; i < GASPeerEntry->GASRspFragNum; i++)
				{
					os_alloc_mem(NULL, (UCHAR **)&GASQueryRspFrag, sizeof(*GASQueryRspFrag));

					if (!GASQueryRspFrag)
					{
						DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
						RTMP_SEM_UNLOCK(&pGASCtrl->GASPeerListLock);
						goto error1;
					}
					GASPeerEntry->AllocResource++;

					NdisZeroMemory(GASQueryRspFrag, sizeof(*GASQueryRspFrag));

					GASQueryRspFrag->GASRspFragID = i;

					if (i < (GASPeerEntry->GASRspFragNum - 1))
						GASQueryRspFrag->FragQueryRspLen = pHSCtrl->MMPDUSize;
					else
						GASQueryRspFrag->FragQueryRspLen = ANQPRspLen - (pHSCtrl->MMPDUSize * i);

					os_alloc_mem(NULL, (UCHAR **)&GASQueryRspFrag->FragQueryRsp,
										GASQueryRspFrag->FragQueryRspLen);
					GASPeerEntry->AllocResource++;

					if (!GASQueryRspFrag->FragQueryRsp)
					{
						DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
						RTMP_SEM_UNLOCK(&pGASCtrl->GASPeerListLock);
						goto error2;
					}

					NdisMoveMemory(GASQueryRspFrag->FragQueryRsp, &ANQPRsp[QueryRspOffset],
									GASQueryRspFrag->FragQueryRspLen);

					QueryRspOffset += GASQueryRspFrag->FragQueryRspLen;

					DlListAddTail(&GASPeerEntry->GASQueryRspFragList,
								  &GASQueryRspFrag->List);
				}
				break;
			}
		}
		RTMP_SEM_UNLOCK(&pGASCtrl->GASPeerListLock);

		if (!IsFound) {
			DBGPRINT(RT_DEBUG_OFF, ("%s Can not find peer address[%02x:%02x:%02x:%02x:%02x:%02x] in GASPeerList (ComeRsp)\n", __FUNCTION__, PeerMACAddr[0],PeerMACAddr[1],PeerMACAddr[2],PeerMACAddr[3],PeerMACAddr[4],PeerMACAddr[5]));
			goto error1;
		}

		Event->u.GAS_RSP_MORE_DATA.StatusCode = 0;
		Len += 2;
		Event->u.GAS_RSP_MORE_DATA.GASComebackDelay = pGASCtrl->cb_delay ? \
													pGASCtrl->cb_delay : 3; //1000;
		Len += 2;
		Event->u.GAS_RSP_MORE_DATA.AdvertisementProID = ACCESS_NETWORK_QUERY_PROTOCOL;
		Len += 1;

		GASPeerEntry->CurrentState = WAIT_GAS_RSP;
		
		SendGASRsp(pAd, (GAS_EVENT_DATA *)Buf);
	}

	os_free_mem(NULL, Buf);

	return TRUE;

error2:
	RTMP_SEM_LOCK(&pGASCtrl->GASPeerListLock);
	DlListForEachSafe(GASQueryRspFrag, Tmp, &GASPeerEntry->GASQueryRspFragList,
											GAS_QUERY_RSP_FRAGMENT, List)
	{
		DlListDel(&GASQueryRspFrag->List);
		os_free_mem(NULL, GASQueryRspFrag);
	}

	DlListInit(&GASPeerEntry->GASQueryRspFragList);
	RTMP_SEM_UNLOCK(&pGASCtrl->GASPeerListLock);
error1:
	os_free_mem(NULL, Buf);
error0:
	return FALSE;
}

#endif

#ifdef CONFIG_DOT11V_WNM
INT Send_BTM_Req(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *PeerMACAddr,
	IN RTMP_STRING *BTMReq,
	IN UINT32 BTMReqLen)
{
	UCHAR *Buf;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	UCHAR APIndex = pObj->ioctl_if;
	PWNM_CTRL pWNMCtrl = &pAd->ApCfg.MBSSID[APIndex].WNMCtrl;
	BTM_EVENT_DATA *Event;
	BTM_PEER_ENTRY *BTMPeerEntry;
	UINT32 Len = 0;
	INT32 Ret;
	BOOLEAN IsFound = FALSE;

	DBGPRINT(RT_DEBUG_OFF, ("%s\n", __FUNCTION__));

	RTMP_SEM_EVENT_WAIT(&pWNMCtrl->BTMPeerListLock, Ret);
	DlListForEach(BTMPeerEntry, &pWNMCtrl->BTMPeerList, BTM_PEER_ENTRY, List)
	{
		if (MAC_ADDR_EQUAL(BTMPeerEntry->PeerMACAddr, PeerMACAddr))
		{
			IsFound = TRUE;
			break;
		}
	}
	RTMP_SEM_EVENT_UP(&pWNMCtrl->BTMPeerListLock);

	if (!IsFound) 
	{
		os_alloc_mem(NULL, (UCHAR **)&BTMPeerEntry, sizeof(*BTMPeerEntry));

		if (!BTMPeerEntry)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
			goto error0;
		}

		NdisZeroMemory(BTMPeerEntry, sizeof(*BTMPeerEntry));		

		BTMPeerEntry->CurrentState = WAIT_BTM_REQ;
		BTMPeerEntry->ControlIndex = APIndex;
		NdisMoveMemory(BTMPeerEntry->PeerMACAddr, PeerMACAddr, MAC_ADDR_LEN);
		BTMPeerEntry->DialogToken = 1;
		BTMPeerEntry->Priv = pAd;

		RTMPInitTimer(pAd, &BTMPeerEntry->WaitPeerBTMRspTimer,
				GET_TIMER_FUNCTION(WaitPeerBTMRspTimeout), BTMPeerEntry, FALSE);
		RTMP_SEM_EVENT_WAIT(&pWNMCtrl->BTMPeerListLock, Ret);
		DlListAddTail(&pWNMCtrl->BTMPeerList, &BTMPeerEntry->List);
		RTMP_SEM_EVENT_UP(&pWNMCtrl->BTMPeerListLock);
	}

	os_alloc_mem(NULL, (UCHAR **)&Buf, sizeof(*Event) + BTMReqLen);

	if (!Buf)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
		goto error1;
	}

	NdisZeroMemory(Buf, sizeof(*Event) + BTMReqLen);

	Event = (BTM_EVENT_DATA *)Buf;

	Event->ControlIndex = APIndex;
	Len += 1;

	NdisMoveMemory(Event->PeerMACAddr, PeerMACAddr, MAC_ADDR_LEN);
	Len += MAC_ADDR_LEN;
	
	Event->EventType = BTM_REQ;
	Len += 2;

	Event->u.BTM_REQ_DATA.DialogToken = BTMPeerEntry->DialogToken;
	Len += 1;

	Event->u.BTM_REQ_DATA.BTMReqLen = BTMReqLen;
	Len += 2;

	NdisMoveMemory(Event->u.BTM_REQ_DATA.BTMReq, BTMReq, BTMReqLen);
	Len += BTMReqLen;

	MlmeEnqueue(pAd, BTM_STATE_MACHINE, BTM_REQ, Len, Buf, 0);

	os_free_mem(NULL, Buf);

	return TRUE;
	 
error1:
	if (!IsFound)
		os_free_mem(NULL, BTMPeerEntry);
error0:
	return FALSE;
}

#ifdef CONFIG_HOTSPOT_R2
INT Send_WNM_Notify_Req(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *PeerMACAddr,
	IN RTMP_STRING *WNMNotifyReq,
	IN UINT32 WNMNotifyReqLen,
	IN UINT32 type)
{
	UCHAR *Buf;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	UCHAR APIndex = pObj->ioctl_if;
	PWNM_CTRL pWNMCtrl = &pAd->ApCfg.MBSSID[APIndex].WNMCtrl;
	WNM_NOTIFY_EVENT_DATA *Event;
	WNM_NOTIFY_PEER_ENTRY *WNMNotifyPeerEntry;
	UINT32 Len = 0;
	INT32 Ret;
	BOOLEAN IsFound = FALSE;

	DBGPRINT(RT_DEBUG_OFF, ("%s\n", __FUNCTION__));

	RTMP_SEM_EVENT_WAIT(&pWNMCtrl->WNMNotifyPeerListLock, Ret);
	DlListForEach(WNMNotifyPeerEntry, &pWNMCtrl->WNMNotifyPeerList, WNM_NOTIFY_PEER_ENTRY, List)
	{
		if (MAC_ADDR_EQUAL(WNMNotifyPeerEntry->PeerMACAddr, PeerMACAddr))
		{
			IsFound = TRUE;
			break;
		}
	}
	RTMP_SEM_EVENT_UP(&pWNMCtrl->WNMNotifyPeerListLock);

	DBGPRINT(RT_DEBUG_OFF, ("*%d\n", IsFound));
	if (!IsFound) 
	{
		os_alloc_mem(NULL, (UCHAR **)&WNMNotifyPeerEntry, sizeof(*WNMNotifyPeerEntry));

		if (!WNMNotifyPeerEntry)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
			goto error0;
		}

		NdisZeroMemory(WNMNotifyPeerEntry, sizeof(*WNMNotifyPeerEntry));		

		WNMNotifyPeerEntry->CurrentState = WNM_NOTIFY_REQ;
		WNMNotifyPeerEntry->ControlIndex = APIndex;
		NdisMoveMemory(WNMNotifyPeerEntry->PeerMACAddr, PeerMACAddr, MAC_ADDR_LEN);
		WNMNotifyPeerEntry->DialogToken = 1;
		WNMNotifyPeerEntry->Priv = pAd;

		RTMPInitTimer(pAd, &WNMNotifyPeerEntry->WaitPeerWNMNotifyRspTimer,
				GET_TIMER_FUNCTION(WaitPeerWNMNotifyRspTimeout), WNMNotifyPeerEntry, FALSE);
		RTMP_SEM_EVENT_WAIT(&pWNMCtrl->WNMNotifyPeerListLock, Ret);
		DlListAddTail(&pWNMCtrl->WNMNotifyPeerList, &WNMNotifyPeerEntry->List);
		RTMP_SEM_EVENT_UP(&pWNMCtrl->WNMNotifyPeerListLock);
	}

	os_alloc_mem(NULL, (UCHAR **)&Buf, sizeof(*Event) + WNMNotifyReqLen);

	if (!Buf)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Not available memory\n", __FUNCTION__));
		goto error1;
	}

	//RTMPusecDelay(200000); //200ms

	NdisZeroMemory(Buf, sizeof(*Event) + WNMNotifyReqLen);

	Event = (WNM_NOTIFY_EVENT_DATA *)Buf;

	Event->ControlIndex = APIndex;
	Len += 1;

	NdisMoveMemory(Event->PeerMACAddr, PeerMACAddr, MAC_ADDR_LEN);
	Len += MAC_ADDR_LEN;
	
	Event->EventType = type; //WNM_NOTIFY_REQ;
	Len += 2;

	Event->u.WNM_NOTIFY_REQ_DATA.DialogToken = WNMNotifyPeerEntry->DialogToken;
	Len += 1;

	Event->u.WNM_NOTIFY_REQ_DATA.WNMNotifyReqLen = WNMNotifyReqLen;
	Len += 2;

	NdisMoveMemory(Event->u.WNM_NOTIFY_REQ_DATA.WNMNotifyReq, WNMNotifyReq, WNMNotifyReqLen);
	Len += WNMNotifyReqLen;

	MlmeEnqueue(pAd, WNM_NOTIFY_STATE_MACHINE, WNM_NOTIFY_REQ, Len, Buf, 0);

	os_free_mem(NULL, Buf);

	return TRUE;
	 
error1:
	if (!IsFound)
		os_free_mem(NULL, WNMNotifyPeerEntry);
error0:
	return FALSE;
}

INT Send_QOSMAP_Configure(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *PeerMACAddr,
	IN RTMP_STRING *QosMapBuf,
	IN UINT32 	QosMapLen,
	IN UINT8	Apidx)
{
	UCHAR *Buf;
	MLME_QOS_ACTION_STRUCT QosMapConfig;
	QOSMAP_SET *pQOSMap = &QosMapConfig.QOSMap;
	
	NdisZeroMemory(&QosMapConfig, sizeof(MLME_QOS_ACTION_STRUCT));
	COPY_MAC_ADDR(QosMapConfig.Addr, PeerMACAddr);
	QosMapConfig.ActionField = ACTION_QOSMAP_CONFIG;
	QosMapConfig.apidx = Apidx;
	pQOSMap->DSCP_Field_Len = QosMapLen;
	NdisMoveMemory(pQOSMap->DSCP_Field, QosMapBuf, QosMapLen);
	MlmeEnqueue(pAd, ACTION_STATE_MACHINE, MT2_MLME_QOS_CATE, sizeof(MLME_QOS_ACTION_STRUCT), (PVOID)&QosMapConfig, 0);
	RTMP_MLME_HANDLER(pAd);
	
	return TRUE;
}
#endif /* CONFIG_HOTSPOT_R2 */
#endif /* CONFIG_DOT11V_WNM */
