#ifndef _SOFT_MAIN_H_
#define _SOFT_MAIN_H_


void soft_mac_task( void *args );
s32 soft_mac_init(void);




typedef enum soft_mac_timer_type_en
{
    SOFT_MAC_TIMER_REORDER_BUFFER,
		
} soft_mac_timer_type;



enum POWER_STATE{
    PWR_IDLE,
    PWR_DOZE,
    PWR_DEEP,
};

typedef struct {
    u32     aid;
    u16     beacon_int;
    u8      dtim_count;
    u8      dtim_period;
    u8      HCI_fcmd_data;
    u8      HCI_fcmd_mgmt;
    u8      HCI_fcmd_ctrl;
    u8      state;
}ps_doze_st;

/* Parsed Information Elements */
struct ieee802_11_elems {
	const u8 *ssid;
	const u8 *supp_rates;
	const u8 *fh_params;
	const u8 *ds_params;
	const u8 *cf_params;
	const u8 *tim;
	const u8 *ibss_params;
	const u8 *challenge;
	const u8 *erp_info;
	const u8 *ext_supp_rates;
	const u8 *wpa_ie;
	const u8 *rsn_ie;
	const u8 *wmm_info; /* WMM Information or Parameter Element */
	const u8 *wmm_parameter;
	const u8 *wmm_tspec;
	const u8 *wps_ie;
	const u8 *power_cap;
	const u8 *supp_channels;
	const u8 *mdie;
	const u8 *ftie;
	const u8 *timeout_int;
	const u8 *ht_capabilities;
	const u8 *ht_operation;
	const u8 *vendor_ht_cap;
	const u8 *p2p;
	const u8 *link_id;
	const u8 *interworking;

	u8 ssid_len;
	u8 supp_rates_len;
	u8 fh_params_len;
	u8 ds_params_len;
	u8 cf_params_len;
	u8 tim_len;
	u8 ibss_params_len;
	u8 challenge_len;
	u8 erp_info_len;
	u8 ext_supp_rates_len;
	u8 wpa_ie_len;
	u8 rsn_ie_len;
	u8 wmm_info_len; /* 7 = WMM Information; 24 = WMM Parameter */
	u8 wmm_parameter_len;
	u8 wmm_tspec_len;
	u8 wps_ie_len;
	u8 power_cap_len;
	u8 supp_channels_len;
	u8 mdie_len;
	u8 ftie_len;
	u8 timeout_int_len;
	u8 ht_capabilities_len;
	u8 ht_operation_len;
	u8 vendor_ht_cap_len;
	u8 p2p_len;
	u8 interworking_len;
};

struct ieee80211_tim_ie {
	u8 dtim_count;
	u8 dtim_period;
	u8 bitmap_ctrl;
	/* variable size: 1 - 251 bytes */
	u8 virtual_map[1];
} STRUCT_PACKED;

typedef enum { ParseOK = 0, ParseUnknown = 1, ParseFailed = -1 } ParseRes;

#endif /* _SOFT_MAIN_H_ */

