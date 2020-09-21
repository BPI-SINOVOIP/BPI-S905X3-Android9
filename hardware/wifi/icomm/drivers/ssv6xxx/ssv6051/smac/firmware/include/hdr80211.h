#ifndef _HDR80211_H_
#define _HDR80211_H_

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define STRUCT_PACKED
#endif


#define MAC_ADDR_LEN				6

#define FCS_LEN                     4
#define HDR80211_MGMT_LEN           24
#define IEEE80211_QOS_CTL_LEN       2
#define IEEE80211_HT_CTL_LEN        4

/**
 * IEEE 802.11 Address offset position. This defines the offset of Address1,
 * Address2, Address3 and Address4 if present.
 */
#define OFFSET_ADDR1                        4
#define OFFSET_ADDR2                        10
#define OFFSET_ADDR3                        16
#define OFFSET_ADDR4                        24


/* IEEE 802.11 Frame Control: */
#define M_FC_VER		            0x0003
#define M_FC_FTYPE		            0x000c
#define M_FC_STYPE		            0x00f0
#define M_FC_TODS		            0x0100
#define M_FC_FROMDS		            0x0200
#define M_FC_MOREFRAGS	            0x0400
#define M_FC_RETRY		            0x0800
#define M_FC_PWRMGMT	            0x1000
#define M_FC_MOREDATA		        0x2000
#define M_FC_PROTECTED	            0x4000
#define M_FC_ORDER		            0x8000

#define M_SC_FRAG		            0x000F
#define M_SC_SEQ		            0xFFF0

/* Frame Type: */
#define FT_MGMT		                0x0000
#define FT_CTRL		                0x0004
#define FT_DATA		                0x0008



/* Frame SubType: Management Frames */
#define FST_ASSOC_REQ	            0x0000
#define FST_ASSOC_RESP	            0x0010
#define FST_REASSOC_REQ	            0x0020
#define FST_REASSOC_RESP	        0x0030
#define FST_PROBE_REQ	            0x0040
#define FST_PROBE_RESP	            0x0050
#define FST_BEACON		            0x0080
#define FST_ATIM		            0x0090
#define FST_DISASSOC	            0x00A0
#define FST_AUTH		            0x00B0
#define FST_DEAUTH		            0x00C0
#define FST_ACTION		            0x00D0

/* Frame SubType: Control Frames */
#define FST_CONTROL_WRAPPER         0x0070
#define FST_BA_REQ	                0x0080
#define FST_BA	                    0x0090
#define FST_PSPOLL		            0x00A0
#define FST_RTS		                0x00B0
#define FST_CTS		                0x00C0
#define FST_ACK		                0x00D0
#define FST_CFEND		            0x00E0
#define FST_CFENDACK	            0x00F0

/* Frame SubType: Data Frames */
#define FST_DATA			        0x0000
#define FST_DATA_CFACK		        0x0010
#define FST_DATA_CFPOLL		        0x0020
#define FST_DATA_CFACKPOLL		    0x0030
#define FST_NULLFUNC		        0x0040
#define FST_CFACK			        0x0050
#define FST_CFPOLL			        0x0060
#define FST_CFACKPOLL		        0x0070
#define FST_QOS_DATA		        0x0080
#define FST_QOS_DATA_CFACK		    0x0090
#define FST_QOS_DATA_CFPOLL		    0x00A0
#define FST_QOS_DATA_CFACKPOLL	    0x00B0
#define FST_QOS_NULLFUNC		    0x00C0
#define FST_QOS_CFACK		        0x00D0
#define FST_QOS_CFPOLL		        0x00E0
#define FST_QOS_CFACKPOLL		    0x00F0


#define FC_TYPE(fc)           ((fc) & (M_FC_FTYPE|M_FC_STYPE))
#define FC_FTYPE(fc)          ((fc) & M_FC_FTYPE)
#define FC_STYPE(fc)          ((fc) & M_FC_STYPE)


#define IS_TODS_SET(fc)       ((fc) & M_FC_TODS)
#define IS_FROMDS_SET(fc)     ((fc) & M_FC_FROMDS)
#define IS_MOREFLAG_SET(fc)   ((fc) & M_FC_MOREFRAGS)
#define IS_RETRY_SET(fc)      ((fc) & M_FC_RETRY)
#define IS_PM_SET(fc)         ((fc) & M_FC_PWRMGMT)
#define IS_MOREDATA_SET(fc)   ((fc) & M_FC_MOREDATA)
#define IS_PROTECT_SET(fc)    ((fc) & M_FC_PROTECTED)
#define IS_ORDER_SET(fc)      ((fc) & M_FC_ORDER)
#define IS_4ADDR_FORMAT(fc)   IS_EQUAL(((fc)&(M_FC_TODS|M_FC_FROMDS)), (M_FC_TODS|M_FC_FROMDS))
#define IS_MGMT_FRAME(fc)     IS_EQUAL(FC_FTYPE(fc), FT_MGMT)
#define IS_CTRL_FRAME(fc)     IS_EQUAL(FC_FTYPE(fc), FT_CTRL)
#define IS_DATA_FRAME(fc)     IS_EQUAL(FC_FTYPE(fc), FT_DATA)
#define IS_QOS_DATA(fc)       IS_EQUAL(((fc)&(M_FC_FTYPE|FST_QOS_DATA)), (FT_DATA|FST_QOS_DATA))
#define IS_NULL_DATA(fc)      IS_EQUAL((fc)&(M_FC_FTYPE|FST_NULLFUNC), (FT_DATA|FST_NULLFUNC))
#define IS_QOS_NULL_DATA(fc)  IS_EQUAL((fc)&(M_FC_FTYPE|FST_NULLFUNC|FST_QOS_DATA), (FT_DATA|FST_NULLFUNC|FST_QOS_DATA)) 



#define IS_ASSOC_REQ(fc)      IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_ASSOC_REQ))
#define IS_ASSOC_RESP(fc)     IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_ASSOC_RESP))
#define IS_REASSOC_REQ(fc)    IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_REASSOC_REQ))
#define IS_REASSOC_RESP(fc)   IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_REASSOC_RESP))
#define IS_PROBE_REQ(fc)      IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_PROBE_REQ))
#define IS_PROBE_RESP(fc)     IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_PROBE_RESP))
#define IS_BEACON(fc)         IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_BEACON))
#define IS_ATIM(fc)           IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_ATIM))
#define IS_DISASSOC(fc)       IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_DISASSOC))
#define IS_AUTH(fc)           IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_AUTH))
#define IS_DEAUTH(fc)         IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_DEAUTH))
#define IS_ACTION(fc)         IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_ACTION))
#define IS_PSPOLL(fc)         IS_EQUAL(FC_TYPE(fc), (FT_CTRL|FST_PSPOLL))
#define IS_RTS(fc)            IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_RTS))
#define IS_CTS(fc)            IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_CTS))
#define IS_ACK(fc)            IS_EQUAL(FC_TYPE(fc), (FT_MGMT|FST_ACK))
#define IS_BAR(fc)            IS_EQUAL(FC_TYPE(fc), (FT_CTRL|FST_BA_REQ))
#define IS_BA(fc)             IS_EQUAL(FC_TYPE(fc), (FT_CTRL|FST_BA))

#define GET_FC_MOREFRAG(fc)	  (((fc) & 0x0400) >> 10)

typedef struct FC_Field_st {
    u16                 ver:2;
    u16                 type:2;
    u16                 subtype:4;
    u16                 toDS:1;
    u16                 fromDS:1;
    u16                 MoreFlag:1;
    u16                 Retry:1;
    u16                 PwrMgmt:1;
    u16                 MoreData:1;
    u16                 Protected:1;
    u16                 order:1;    

} FC_Field, *PFC_Field;

typedef struct QOS_Ctrl_st {
    u16                 tid:4;
    u16                 bit4:1;
    u16                 ack_policy:2;
    u16                 rsvd:1;
    u16                 bit8_15:8;

} QOSCtrl_Field, *PQOSCtrl_Field;

typedef struct HDR80211_Data_st {
	FC_Field            fc;
	u16                 dur;
	ETHER_ADDR          addr1;
	ETHER_ADDR          addr2;
	ETHER_ADDR          addr3;
	u16                 seq_ctrl;    
    
} HDR80211_Data, *PHDR80211_Data;

typedef struct HDR80211_Data_4addr_st {
	FC_Field            fc;
	u16                 dur;
	ETHER_ADDR          addr1;
	ETHER_ADDR          addr2;
	ETHER_ADDR          addr3;
	u16                 seq_ctrl;        
    ETHER_ADDR          addr4;

} HDR80211_Data_4addr, *PHDR80211_Data_4addr;

#define BASE_LEN(PTR2variable,PTR)			(PTR2variable - (u8 *) PTR)


typedef struct HDR80211_Mgmt_st {
    FC_Field            fc;
    u16                 dur;
    ETHER_ADDR          da;
    ETHER_ADDR          sa;
    ETHER_ADDR          bssid;
    le16                frg_num:4;
    le16                seq_ctrl:12;

    union {
        struct {
            u16         auth_algo;
            u16         trans_id;
            u16         status_code;
            /* possibly followed by Challenge text */
            u8          variable[0];    
        } auth;
        struct {
            u16         reason_code;
        } deauth;
        struct {
            u16         capab_info;
            u16         listen_interval;
            /* followed by SSID and Supported rates */
            u8          variable[0];
        } assoc_req;
        struct {
			u16         capab_info;
			u16         status_code;
			u16         aid;
			/* followed by Supported rates */
			u8 variable[0];
		} assoc_resp, reassoc_resp;
        struct {
			u16         capab_info;
			u16         listen_interval;
			u8          current_ap[6];
			/* followed by SSID and Supported rates */
			u8 variable[0];
		} reassoc_req;
        struct {
			u16         reason_code;
		} disassoc;
        struct {
			u64         timestamp;
			u16         beacon_int;
			u16         capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params, TIM */
			u8 variable[0];
		} beacon;
		struct { /*lint -save -e43 */
			/* only variable items: SSID, Supported rates */
			u8 variable[0];
		} probe_req; /*lint -restore */
		struct {
			u64         timestamp;
			u16         beacon_int;
			u16         capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params */
			u8 variable[0];
		} probe_resp;
        struct {
            u8          category;
            union {
                struct {
					u8  action_code;
					u8  dialog_token;
					u8  status_code;
					u8  variable[0];
				} wme_action;
				struct {
					u8  action_code;
					u8  element_id;
					u8  length;
//					struct ieee80211_channel_sw_ie sw_elem;
				} chan_switch;
                struct {
					u8  action_code;
					u8  dialog_token;
					u8  element_id;
					u8  length;
//					struct ieee80211_msrment_ie msr_elem;
				} measurement;
				struct{
					u8  action_code;
					u8  dialog_token;
					u16 capab;
					u16 timeout;
					u16 start_seq_num;
				} STRUCT_PACKED addba_req;
				struct{
					u8  action_code;
					u8  dialog_token;
					u16 status;
					u16 capab;
					u16 timeout;
				} STRUCT_PACKED addba_resp;
                struct {
					u8  action_code;
					u16 params;
					u16 reason_code;
				} STRUCT_PACKED delba;
				struct {
					u8  action_code;
					/* capab_info for open and confirm,
					 * reason for close
					 */
					u16 aux;
					/* Followed in plink_confirm by status
					 * code, AID and supported rates,
					 * and directly by supported rates in
					 * plink_open and plink_close
					 */
					u8  variable[0];
				} plink_action;
                struct{
					u8  action_code;
					u8  variable[0];
				} mesh_action;
				struct {
					u8  action;
//					u8  trans_id[WLAN_SA_QUERY_TR_ID_LEN];
				} sa_query;
				struct {
					u8  action;
					u8  smps_control;
				} ht_smps;
            } u;           
        } STRUCT_PACKED action;
    } u;
    
} STRUCT_PACKED HDR80211_Mgmt, *PHDR80211_Mgmt;

typedef struct HDR80211_Ctrl_st 
{
    FC_Field            fc;
    u16                 dur;
    ETHER_ADDR          ra;
    ETHER_ADDR          ta;

} HDR80211_Ctrl, *PHDR80211_Ctrl;


typedef struct HDR8023_Data_st {
	ETHER_ADDR			dest;
	ETHER_ADDR			src;
	u16					protocol;
} HDR8023_Data, *PHDR8023_Data;


#define HDR80211_MGMT(x)    /*lint -save -e740 */ (HDR80211_Mgmt *)((u8 *)(x)+((PKT_Info *)(x))->hdr_offset) /*lint -restore */
#define HDR80211_CTRL(x)
#define HDR80211_DATA(x)





/* Define WLAN Cipher Suite */
#define CIPHER_SUITE_NONE                   0
#define CIPHER_SUITE_WEP40                  1
#define CIPHER_SUITE_WEP104                 2
#define CIPHER_SUITE_TKIP                   3
#define CIPHER_SUITE_CCMP                   4



/**
 * WLAN Operation Mode Definition (for wlan_mode field):
 *
 * @ WLAN_STA: operate as STA (infrastructure) mode
 * @ WLAN_AP: operate as AP (infrastructure) mode
 * @ WLAN_IBSS: operate as IBSS (ad-hoc) mode
 * @ WLAN_WDS: Wireless Distribution System mode (Wireless Bridge)
 */
#define WLAN_STA                            0
#define WLAN_AP                             1
#define WLAN_IBSS                           2
#define WLAN_WDS                            3




/**
 * HT Mode Definition (for ht_mode field):
 * 
 * @ HT_NONE
 * @ HT_MF
 * @ HT_GF
 */
#define HT_NONE                         0
#define HT_MF                           1
#define HT_GF                           2


#if 0
static u8 *GET_QOS_CTRL(HDR80211_Data_4addr *hdr)
{
	if (hdr->fc.fromDS && hdr->fc.toDS)
		return (u8 *)hdr + 30;
	else
		return (u8 *)hdr + 24;
}
#endif

#endif /* _HDR80211_H_ */



