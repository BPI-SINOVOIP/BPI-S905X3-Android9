/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#ifndef BTM_INT_TYPES_H
#define BTM_INT_TYPES_H

#include "btm_api_types.h"
#include "btm_ble_api_types.h"
#include "btm_ble_int_types.h"
#include "hcidefs.h"
#include "osi/include/alarm.h"
#include "osi/include/list.h"
#include "rfcdefs.h"

typedef char tBTM_LOC_BD_NAME[BTM_MAX_LOC_BD_NAME_LEN + 1];

#define BTM_ACL_IS_CONNECTED(bda) \
  (btm_bda_to_acl(bda, BT_TRANSPORT_BR_EDR) != NULL)

/* Definitions for Server Channel Number (SCN) management
*/
#define BTM_MAX_SCN PORT_MAX_RFC_PORTS

/* Define masks for supported and exception 2.0 ACL packet types
*/
#define BTM_ACL_SUPPORTED_PKTS_MASK                                           \
  (HCI_PKT_TYPES_MASK_DM1 | HCI_PKT_TYPES_MASK_DH1 | HCI_PKT_TYPES_MASK_DM3 | \
   HCI_PKT_TYPES_MASK_DH3 | HCI_PKT_TYPES_MASK_DM5 | HCI_PKT_TYPES_MASK_DH5)

#define BTM_ACL_EXCEPTION_PKTS_MASK                            \
  (HCI_PKT_TYPES_MASK_NO_2_DH1 | HCI_PKT_TYPES_MASK_NO_3_DH1 | \
   HCI_PKT_TYPES_MASK_NO_2_DH3 | HCI_PKT_TYPES_MASK_NO_3_DH3 | \
   HCI_PKT_TYPES_MASK_NO_2_DH5 | HCI_PKT_TYPES_MASK_NO_3_DH5)

#define BTM_EPR_AVAILABLE(p)                                            \
  ((HCI_ATOMIC_ENCRYPT_SUPPORTED((p)->peer_lmp_feature_pages[0]) &&     \
    HCI_ATOMIC_ENCRYPT_SUPPORTED(                                       \
        controller_get_interface()->get_features_classic(0)->as_array)) \
       ? true                                                           \
       : false)

#define BTM_IS_BRCM_CONTROLLER()                                 \
  (controller_get_interface()->get_bt_version()->manufacturer == \
   LMP_COMPID_BROADCOM)

/* Define the ACL Management control structure
*/
typedef struct {
  uint16_t hci_handle;
  uint16_t pkt_types_mask;
  uint16_t clock_offset;
  RawAddress remote_addr;
  DEV_CLASS remote_dc;
  BD_NAME remote_name;

  uint16_t manufacturer;
  uint16_t lmp_subversion;
  uint16_t link_super_tout;
  BD_FEATURES
  peer_lmp_feature_pages[HCI_EXT_FEATURES_PAGE_MAX + 1]; /* Peer LMP Extended
                                                            features mask table
                                                            for the device */
  uint8_t num_read_pages;
  uint8_t lmp_version;

  bool in_use;
  uint8_t link_role;
  bool link_up_issued; /* True if busy_level link up has been issued */

#define BTM_ACL_SWKEY_STATE_IDLE 0
#define BTM_ACL_SWKEY_STATE_MODE_CHANGE 1
#define BTM_ACL_SWKEY_STATE_ENCRYPTION_OFF 2
#define BTM_ACL_SWKEY_STATE_SWITCHING 3
#define BTM_ACL_SWKEY_STATE_ENCRYPTION_ON 4
#define BTM_ACL_SWKEY_STATE_IN_PROGRESS 5
  uint8_t switch_role_state;

#define BTM_MAX_SW_ROLE_FAILED_ATTEMPTS 3
  uint8_t switch_role_failed_attempts;

#define BTM_ACL_ENCRYPT_STATE_IDLE 0
#define BTM_ACL_ENCRYPT_STATE_ENCRYPT_OFF 1 /* encryption turning off */
#define BTM_ACL_ENCRYPT_STATE_TEMP_FUNC \
  2 /* temporarily off for change link key or role switch */
#define BTM_ACL_ENCRYPT_STATE_ENCRYPT_ON 3 /* encryption turning on */
  uint8_t encrypt_state;                   /* overall BTM encryption state */

  tBT_TRANSPORT transport;
  RawAddress conn_addr;   /* local device address used for this connection */
  uint8_t conn_addr_type; /* local device address type for this connection */
  RawAddress active_remote_addr;   /* remote address used on this connection */
  uint8_t active_remote_addr_type; /* local device address type for this
                                      connection */
  BD_FEATURES peer_le_features; /* Peer LE Used features mask for the device */

} tACL_CONN;

/* Define the Device Management control structure
*/
typedef struct {
  tBTM_DEV_STATUS_CB* p_dev_status_cb; /* Device status change callback */
  tBTM_VS_EVT_CB* p_vend_spec_cb
      [BTM_MAX_VSE_CALLBACKS]; /* Register for vendor specific events  */

  tBTM_CMPL_CB*
      p_stored_link_key_cmpl_cb; /* Read/Write/Delete stored link key    */

  alarm_t* read_local_name_timer; /* Read local name timer */
  tBTM_CMPL_CB* p_rln_cmpl_cb;    /* Callback function to be called when  */
                                  /* read local name function complete    */

  alarm_t* read_rssi_timer;       /* Read RSSI timer */
  tBTM_CMPL_CB* p_rssi_cmpl_cb;   /* Callback function to be called when  */
                                  /* read RSSI function completes */

  alarm_t* read_failed_contact_counter_timer; /* Read Failed Contact Counter */
                                              /* timer */
  tBTM_CMPL_CB* p_failed_contact_counter_cmpl_cb; /* Callback function to be */
  /* called when read Failed Contact Counter function completes */

  alarm_t*
      read_automatic_flush_timeout_timer; /* Read Automatic Flush Timeout */
                                          /* timer */
  tBTM_CMPL_CB* p_automatic_flush_timeout_cmpl_cb; /* Callback function to be */
  /* called when read Automatic Flush Timeout function completes */

  alarm_t* read_link_quality_timer;
  tBTM_CMPL_CB* p_link_qual_cmpl_cb; /* Callback function to be called when  */
                                     /* read link quality function completes */

  alarm_t* read_inq_tx_power_timer;
  tBTM_CMPL_CB*
      p_inq_tx_power_cmpl_cb; /* Callback function to be called when  */
                              /* read inq tx power function completes  */

  alarm_t* qos_setup_timer;          /* QoS setup timer */
  tBTM_CMPL_CB* p_qos_setup_cmpl_cb; /* Callback function to be called when  */
                                     /* qos setup function completes         */

  tBTM_ROLE_SWITCH_CMPL switch_role_ref_data;
  tBTM_CMPL_CB* p_switch_role_cb; /* Callback function to be called when  */
                                  /* requested switch role is completed   */

  alarm_t* read_tx_power_timer;     /* Read tx power timer */
  tBTM_CMPL_CB* p_tx_power_cmpl_cb; /* Callback function to be called       */

  DEV_CLASS dev_class; /* Local device class                   */

  tBTM_CMPL_CB*
      p_le_test_cmd_cmpl_cb; /* Callback function to be called when
                             LE test mode command has been sent successfully */

  RawAddress read_tx_pwr_addr; /* read TX power target address     */

#define BTM_LE_SUPPORT_STATE_SIZE 8
  uint8_t le_supported_states[BTM_LE_SUPPORT_STATE_SIZE];

  tBTM_BLE_LOCAL_ID_KEYS id_keys;      /* local BLE ID keys */
  BT_OCTET16 ble_encryption_key_value; /* BLE encryption key */

#if (BTM_BLE_CONFORMANCE_TESTING == TRUE)
  bool no_disc_if_pair_fail;
  bool enable_test_mac_val;
  BT_OCTET8 test_mac;
  bool enable_test_local_sign_cntr;
  uint32_t test_local_sign_cntr;
#endif

  tBTM_IO_CAP loc_io_caps;      /* IO capability of the local device */
  tBTM_AUTH_REQ loc_auth_req;   /* the auth_req flag  */
  bool secure_connections_only; /* Rejects service level 0 connections if */
                                /* itself or peer device doesn't support */
                                /* secure connections */
} tBTM_DEVCB;

/* Define the structures and constants used for inquiry
*/

/* Definitions of limits for inquiries */
#define BTM_PER_INQ_MIN_MAX_PERIOD HCI_PER_INQ_MIN_MAX_PERIOD
#define BTM_PER_INQ_MAX_MAX_PERIOD HCI_PER_INQ_MAX_MAX_PERIOD
#define BTM_PER_INQ_MIN_MIN_PERIOD HCI_PER_INQ_MIN_MIN_PERIOD
#define BTM_PER_INQ_MAX_MIN_PERIOD HCI_PER_INQ_MAX_MIN_PERIOD
#define BTM_MAX_INQUIRY_LENGTH HCI_MAX_INQUIRY_LENGTH
#define BTM_MIN_INQUIRY_LEN 0x01

#define BTM_MIN_INQ_TX_POWER (-70)
#define BTM_MAX_INQ_TX_POWER 20

typedef struct {
  uint32_t inq_count; /* Used for determining if a response has already been */
  /* received for the current inquiry operation. (We do not   */
  /* want to flood the caller with multiple responses from    */
  /* the same device.                                         */
  RawAddress bd_addr;
} tINQ_BDADDR;

typedef struct {
  uint32_t time_of_resp;
  uint32_t
      inq_count; /* "timestamps" the entry with a particular inquiry count   */
                 /* Used for determining if a response has already been      */
                 /* received for the current inquiry operation. (We do not   */
                 /* want to flood the caller with multiple responses from    */
                 /* the same device.                                         */
  tBTM_INQ_INFO inq_info;
  bool in_use;
  bool scan_rsp;
} tINQ_DB_ENT;

enum { INQ_NONE, INQ_LE_OBSERVE, INQ_GENERAL };
typedef uint8_t tBTM_INQ_TYPE;

typedef struct {
  tBTM_CMPL_CB* p_remname_cmpl_cb;

#define BTM_EXT_RMT_NAME_TIMEOUT_MS (40 * 1000) /* 40 seconds */

  alarm_t* remote_name_timer;

  uint16_t discoverable_mode;
  uint16_t connectable_mode;
  uint16_t page_scan_window;
  uint16_t page_scan_period;
  uint16_t inq_scan_window;
  uint16_t inq_scan_period;
  uint16_t inq_scan_type;
  uint16_t page_scan_type; /* current page scan type */
  tBTM_INQ_TYPE scan_type;

  RawAddress remname_bda; /* Name of bd addr for active remote name request */
#define BTM_RMT_NAME_INACTIVE 0
#define BTM_RMT_NAME_EXT 0x1 /* Initiated through API */
#define BTM_RMT_NAME_SEC 0x2 /* Initiated internally by security manager */
#define BTM_RMT_NAME_INQ 0x4 /* Remote name initiated internally by inquiry */
  bool remname_active; /* State of a remote name request by external API */

  tBTM_CMPL_CB* p_inq_cmpl_cb;
  tBTM_INQ_RESULTS_CB* p_inq_results_cb;
  tBTM_CMPL_CB*
      p_inq_ble_cmpl_cb; /*completion callback exclusively for LE Observe*/
  tBTM_INQ_RESULTS_CB*
      p_inq_ble_results_cb; /*results callback exclusively for LE observe*/
  tBTM_CMPL_CB* p_inqfilter_cmpl_cb; /* Called (if not NULL) after inquiry
                                        filter completed */
  uint32_t inq_counter; /* Counter incremented each time an inquiry completes */
  /* Used for determining whether or not duplicate devices */
  /* have responded to the same inquiry */
  tINQ_BDADDR* p_bd_db;    /* Pointer to memory that holds bdaddrs */
  uint16_t num_bd_entries; /* Number of entries in database */
  uint16_t max_bd_entries; /* Maximum number of entries that can be stored */
  tINQ_DB_ENT inq_db[BTM_INQ_DB_SIZE];
  tBTM_INQ_PARMS inqparms; /* Contains the parameters for the current inquiry */
  tBTM_INQUIRY_CMPL
      inq_cmpl_info; /* Status and number of responses from the last inquiry */

  uint16_t per_min_delay; /* Current periodic minimum delay */
  uint16_t per_max_delay; /* Current periodic maximum delay */
  bool inqfilt_active;
  uint8_t pending_filt_complete_event; /* to take care of
                                          btm_event_filter_complete
                                          corresponding to */
  /* inquiry that has been cancelled*/
  uint8_t inqfilt_type; /* Contains the inquiry filter type (BD ADDR, COD, or
                           Clear) */

#define BTM_INQ_INACTIVE_STATE 0
#define BTM_INQ_CLR_FILT_STATE \
  1 /* Currently clearing the inquiry filter preceeding the inquiry request */
    /* (bypassed if filtering is not used)                                  */
#define BTM_INQ_SET_FILT_STATE \
  2 /* Sets the new filter (or turns off filtering) in this state */
#define BTM_INQ_ACTIVE_STATE \
  3 /* Actual inquiry or periodic inquiry is in progress */
#define BTM_INQ_REMNAME_STATE 4 /* Remote name requests are active  */

  uint8_t state;      /* Current state that the inquiry process is in */
  uint8_t inq_active; /* Bit Mask indicating type of inquiry is active */
  bool no_inc_ssp;    /* true, to stop inquiry on incoming SSP */
} tBTM_INQUIRY_VAR_ST;

/* The MSB of the clock offset field indicates whether the offset is valid. */
#define BTM_CLOCK_OFFSET_VALID 0x8000

/* Define the structures needed by security management
*/

#define BTM_SEC_INVALID_HANDLE 0xFFFF

typedef uint8_t* BTM_BD_NAME_PTR; /* Pointer to Device name */

/* Security callback is called by this unit when security
 *   procedures are completed.  Parameters are
 *              BD Address of remote
 *              Result of the operation
*/
typedef tBTM_SEC_CBACK tBTM_SEC_CALLBACK;

typedef void(tBTM_SCO_IND_CBACK)(uint16_t sco_inx);

/* MACROs to convert from SCO packet types mask to ESCO and back */
#define BTM_SCO_PKT_TYPE_MASK \
  (HCI_PKT_TYPES_MASK_HV1 | HCI_PKT_TYPES_MASK_HV2 | HCI_PKT_TYPES_MASK_HV3)

/* Mask defining only the SCO types of an esco packet type */
#define BTM_ESCO_PKT_TYPE_MASK \
  (ESCO_PKT_TYPES_MASK_HV1 | ESCO_PKT_TYPES_MASK_HV2 | ESCO_PKT_TYPES_MASK_HV3)

#define BTM_SCO_2_ESCO(scotype) \
  ((uint16_t)(((scotype)&BTM_SCO_PKT_TYPE_MASK) >> 5))
#define BTM_ESCO_2_SCO(escotype) \
  ((uint16_t)(((escotype)&BTM_ESCO_PKT_TYPE_MASK) << 5))

/* Define masks for supported and exception 2.0 SCO packet types
*/
#define BTM_SCO_SUPPORTED_PKTS_MASK                    \
  (ESCO_PKT_TYPES_MASK_HV1 | ESCO_PKT_TYPES_MASK_HV2 | \
   ESCO_PKT_TYPES_MASK_HV3 | ESCO_PKT_TYPES_MASK_EV3 | \
   ESCO_PKT_TYPES_MASK_EV4 | ESCO_PKT_TYPES_MASK_EV5)

#define BTM_SCO_EXCEPTION_PKTS_MASK                              \
  (ESCO_PKT_TYPES_MASK_NO_2_EV3 | ESCO_PKT_TYPES_MASK_NO_3_EV3 | \
   ESCO_PKT_TYPES_MASK_NO_2_EV5 | ESCO_PKT_TYPES_MASK_NO_3_EV5)

#define BTM_SCO_ROUTE_UNKNOWN 0xff

/* Define the structure that contains (e)SCO data */
typedef struct {
  tBTM_ESCO_CBACK* p_esco_cback; /* Callback for eSCO events     */
  enh_esco_params_t setup;
  tBTM_ESCO_DATA data; /* Connection complete information */
  uint8_t hci_status;
} tBTM_ESCO_INFO;

/* Define the structure used for SCO Management
*/
typedef struct {
  tBTM_ESCO_INFO esco; /* Current settings             */
  tBTM_SCO_CB* p_conn_cb; /* Callback for when connected  */
  tBTM_SCO_CB* p_disc_cb; /* Callback for when disconnect */
  uint16_t state;         /* The state of the SCO link    */
  uint16_t hci_handle;    /* HCI Handle                   */
  bool is_orig;           /* true if the originator       */
  bool rem_bd_known;      /* true if remote BD addr known */

} tSCO_CONN;

/* SCO Management control block */
typedef struct {
  tBTM_SCO_IND_CBACK* app_sco_ind_cb;
  tSCO_CONN sco_db[BTM_MAX_SCO_LINKS];
  enh_esco_params_t def_esco_parms;
  uint16_t sco_disc_reason;
  bool esco_supported;        /* true if 1.2 cntlr AND supports eSCO links */
  esco_data_path_t sco_route; /* HCI, PCM, or TEST */
} tSCO_CB;

#if (BTM_SCO_INCLUDED == TRUE)
extern void btm_set_sco_ind_cback(tBTM_SCO_IND_CBACK* sco_ind_cb);
extern void btm_accept_sco_link(uint16_t sco_inx, enh_esco_params_t* p_setup,
                                tBTM_SCO_CB* p_conn_cb, tBTM_SCO_CB* p_disc_cb);
extern void btm_reject_sco_link(uint16_t sco_inx);
extern void btm_sco_chk_pend_rolechange(uint16_t hci_handle);
extern void btm_sco_disc_chk_pend_for_modechange(uint16_t hci_handle);

#else
#define btm_accept_sco_link(sco_inx, p_setup, p_conn_cb, p_disc_cb)
#define btm_reject_sco_link(sco_inx)
#define btm_set_sco_ind_cback(sco_ind_cb)
#define btm_sco_chk_pend_rolechange(hci_handle)
#endif /* BTM_SCO_INCLUDED */

/*
 * Define structure for Security Service Record.
 * A record exists for each service registered with the Security Manager
*/
#define BTM_SEC_OUT_FLAGS \
  (BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_OUT_ENCRYPT | BTM_SEC_OUT_AUTHORIZE)
#define BTM_SEC_IN_FLAGS \
  (BTM_SEC_IN_AUTHENTICATE | BTM_SEC_IN_ENCRYPT | BTM_SEC_IN_AUTHORIZE)

#define BTM_SEC_OUT_LEVEL4_FLAGS                                       \
  (BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_OUT_ENCRYPT | BTM_SEC_OUT_MITM | \
   BTM_SEC_MODE4_LEVEL4)

#define BTM_SEC_IN_LEVEL4_FLAGS                                     \
  (BTM_SEC_IN_AUTHENTICATE | BTM_SEC_IN_ENCRYPT | BTM_SEC_IN_MITM | \
   BTM_SEC_MODE4_LEVEL4)
typedef struct {
  uint32_t mx_proto_id;     /* Service runs over this multiplexer protocol */
  uint32_t orig_mx_chan_id; /* Channel on the multiplexer protocol    */
  uint32_t term_mx_chan_id; /* Channel on the multiplexer protocol    */
  uint16_t psm;             /* L2CAP PSM value */
  uint16_t security_flags;  /* Bitmap of required security features */
  uint8_t service_id;       /* Passed in authorization callback */
#if BTM_SEC_SERVICE_NAME_LEN > 0
  uint8_t orig_service_name[BTM_SEC_SERVICE_NAME_LEN + 1];
  uint8_t term_service_name[BTM_SEC_SERVICE_NAME_LEN + 1];
#endif
} tBTM_SEC_SERV_REC;

/* LE Security information of device in Slave Role */
typedef struct {
  BT_OCTET16 irk;   /* peer diverified identity root */
  BT_OCTET16 pltk;  /* peer long term key */
  BT_OCTET16 pcsrk; /* peer SRK peer device used to secured sign local data  */

  BT_OCTET16 lltk;  /* local long term key */
  BT_OCTET16 lcsrk; /* local SRK peer device used to secured sign local data  */

  BT_OCTET8 rand;        /* random vector for LTK generation */
  uint16_t ediv;         /* LTK diversifier of this slave device */
  uint16_t div;          /* local DIV  to generate local LTK=d1(ER,DIV,0) and
                            CSRK=d1(ER,DIV,1)  */
  uint8_t sec_level;     /* local pairing security level */
  uint8_t key_size;      /* key size of the LTK delivered to peer device */
  uint8_t srk_sec_level; /* security property of peer SRK for this device */
  uint8_t local_csrk_sec_level; /* security property of local CSRK for this
                                   device */

  uint32_t counter;       /* peer sign counter for verifying rcv signed cmd */
  uint32_t local_counter; /* local sign counter for sending signed write cmd*/
} tBTM_SEC_BLE_KEYS;

typedef struct {
  RawAddress pseudo_addr; /* LE pseudo address of the device if different from
                          device address  */
  tBLE_ADDR_TYPE ble_addr_type; /* LE device type: public or random address */
  tBLE_ADDR_TYPE static_addr_type; /* static address type */
  RawAddress static_addr;          /* static address */

#define BTM_WHITE_LIST_BIT 0x01
#define BTM_RESOLVING_LIST_BIT 0x02
  uint8_t in_controller_list; /* in controller resolving list or not */
  uint8_t resolving_list_index;
#if (BLE_PRIVACY_SPT == TRUE)
  RawAddress cur_rand_addr; /* current random address */

#define BTM_BLE_ADDR_PSEUDO 0 /* address index device record */
#define BTM_BLE_ADDR_RRA 1    /* cur_rand_addr */
#define BTM_BLE_ADDR_STATIC 2 /* static_addr  */
  uint8_t active_addr_type;
#endif

  tBTM_LE_KEY_TYPE key_type; /* bit mask of valid key types in record */
  tBTM_SEC_BLE_KEYS keys;    /* LE device security info in slave rode */
} tBTM_SEC_BLE;

/* Peering bond type */
enum { BOND_TYPE_UNKNOWN, BOND_TYPE_PERSISTENT, BOND_TYPE_TEMPORARY };
typedef uint8_t tBTM_BOND_TYPE;

/*
 * Define structure for Security Device Record.
 * A record exists for each device authenticated with this device
*/
typedef struct {
  tBTM_SEC_SERV_REC* p_cur_service;
  tBTM_SEC_CALLBACK* p_callback;
  void* p_ref_data;
  uint32_t timestamp; /* Timestamp of the last connection   */
  uint32_t trusted_mask[BTM_SEC_SERVICE_ARRAY_SIZE]; /* Bitwise OR of trusted
                                                        services     */
  uint16_t hci_handle;     /* Handle to connection when exists   */
  uint16_t clock_offset;   /* Latest known clock offset          */
  RawAddress bd_addr;      /* BD_ADDR of the device              */
  DEV_CLASS dev_class;     /* DEV_CLASS of the device            */
  LINK_KEY link_key;       /* Device link key                    */
  uint8_t pin_code_length; /* Length of the pin_code used for paring */

#define BTM_SEC_AUTHORIZED BTM_SEC_FLAG_AUTHORIZED       /* 0x01 */
#define BTM_SEC_AUTHENTICATED BTM_SEC_FLAG_AUTHENTICATED /* 0x02 */
#define BTM_SEC_ENCRYPTED BTM_SEC_FLAG_ENCRYPTED         /* 0x04 */
#define BTM_SEC_NAME_KNOWN 0x08
#define BTM_SEC_LINK_KEY_KNOWN BTM_SEC_FLAG_LKEY_KNOWN   /* 0x10 */
#define BTM_SEC_LINK_KEY_AUTHED BTM_SEC_FLAG_LKEY_AUTHED /* 0x20 */
#define BTM_SEC_ROLE_SWITCHED 0x40
#define BTM_SEC_IN_USE 0x80
/* LE link security flag */
#define BTM_SEC_LE_AUTHENTICATED \
  0x0200 /* LE link is encrypted after pairing with MITM */
#define BTM_SEC_LE_ENCRYPTED 0x0400  /* LE link is encrypted */
#define BTM_SEC_LE_NAME_KNOWN 0x0800 /* not used */
#define BTM_SEC_LE_LINK_KEY_KNOWN \
  0x1000 /* bonded with peer (peer LTK and/or SRK is saved) */
#define BTM_SEC_LE_LINK_KEY_AUTHED 0x2000 /* pairing is done with MITM */
#define BTM_SEC_16_DIGIT_PIN_AUTHED \
  0x4000 /* pairing is done with 16 digit pin */

  uint16_t sec_flags; /* Current device security state      */

  tBTM_BD_NAME sec_bd_name; /* User friendly name of the device. (may be
                               truncated to save space in dev_rec table) */
  BD_FEATURES feature_pages[HCI_EXT_FEATURES_PAGE_MAX +
                            1]; /* Features supported by the device */
  uint8_t num_read_pages;

#define BTM_SEC_STATE_IDLE 0
#define BTM_SEC_STATE_AUTHENTICATING 1
#define BTM_SEC_STATE_ENCRYPTING 2
#define BTM_SEC_STATE_GETTING_NAME 3
#define BTM_SEC_STATE_AUTHORIZING 4
#define BTM_SEC_STATE_SWITCHING_ROLE 5
#define BTM_SEC_STATE_DISCONNECTING 6 /* disconnecting BR/EDR */
#define BTM_SEC_STATE_DELAY_FOR_ENC \
  7 /* delay to check for encryption to work around */
    /* controller problems */
#define BTM_SEC_STATE_DISCONNECTING_BLE 8  /* disconnecting BLE */
#define BTM_SEC_STATE_DISCONNECTING_BOTH 9 /* disconnecting BR/EDR and BLE */

  uint8_t sec_state;  /* Operating state                    */
  bool is_originator; /* true if device is originating connection */
  bool role_master;           /* true if current mode is master     */
  uint16_t security_required; /* Security required for connection   */
  bool link_key_not_sent; /* link key notification has not been sent waiting for
                             name */
  uint8_t link_key_type;  /* Type of key used in pairing   */
  bool link_key_changed;  /* Changed link key during current connection */

#define BTM_MAX_PRE_SM4_LKEY_TYPE \
  BTM_LKEY_TYPE_REMOTE_UNIT /* the link key type used by legacy pairing */

#define BTM_SM4_UNKNOWN 0x00
#define BTM_SM4_KNOWN 0x10
#define BTM_SM4_TRUE 0x11
#define BTM_SM4_REQ_PEND 0x08 /* set this bit when getting remote features */
#define BTM_SM4_UPGRADE 0x04  /* set this bit when upgrading link key */
#define BTM_SM4_RETRY                                     \
  0x02 /* set this bit to retry on HCI_ERR_KEY_MISSING or \
          HCI_ERR_LMP_ERR_TRANS_COLLISION */
#define BTM_SM4_DD_ACP \
  0x20 /* set this bit to indicate peer initiated dedicated bonding */
#define BTM_SM4_CONN_PEND                                               \
  0x40 /* set this bit to indicate accepting acl conn; to be cleared on \
          btm_acl_created */
  uint8_t sm4;                /* BTM_SM4_TRUE, if the peer supports SM4 */
  tBTM_IO_CAP rmt_io_caps;    /* IO capability of the peer device */
  tBTM_AUTH_REQ rmt_auth_req; /* the auth_req flag as in the IO caps rsp evt */
  bool remote_supports_secure_connections;
  bool remote_features_needed; /* set to true if the local device is in */
  /* "Secure Connections Only" mode and it receives */
  /* HCI_IO_CAPABILITY_REQUEST_EVT from the peer before */
  /* it knows peer's support for Secure Connections */

  uint16_t ble_hci_handle; /* use in DUMO connection */
  uint8_t enc_key_size;    /* current link encryption key size */
  tBT_DEVICE_TYPE device_type;
  bool new_encryption_key_is_p256; /* Set to true when the newly generated LK
                                   ** is generated from P-256.
                                   ** Link encrypted with such LK can be used
                                   ** for SM over BR/EDR.
                                   */
  bool no_smp_on_br;        /* if set to true then SMP on BR/EDR doesn't */
                            /* work, i.e. link keys crosspairing */
                            /* SC BR/EDR->SC LE doesn't happen */
  tBTM_BOND_TYPE bond_type; /* peering bond type */

  tBTM_SEC_BLE ble;
  tBTM_LE_CONN_PRAMS conn_params;

#if (BTM_DISC_DURING_RS == TRUE)
#define BTM_SEC_RS_NOT_PENDING 0 /* Role Switch not in progress */
#define BTM_SEC_RS_PENDING 1     /* Role Switch in progress */
#define BTM_SEC_DISC_PENDING 2   /* Disconnect is pending */
  uint8_t rs_disc_pending;
#endif
#define BTM_SEC_NO_LAST_SERVICE_ID 0
  uint8_t last_author_service_id; /* ID of last serviced authorized: Reset after
                                     each l2cap connection */

} tBTM_SEC_DEV_REC;

#define BTM_SEC_IS_SM4(sm) ((bool)(BTM_SM4_TRUE == ((sm)&BTM_SM4_TRUE)))
#define BTM_SEC_IS_SM4_LEGACY(sm) ((bool)(BTM_SM4_KNOWN == ((sm)&BTM_SM4_TRUE)))
#define BTM_SEC_IS_SM4_UNKNOWN(sm) \
  ((bool)(BTM_SM4_UNKNOWN == ((sm)&BTM_SM4_TRUE)))

#define BTM_SEC_LE_MASK                              \
  (BTM_SEC_LE_AUTHENTICATED | BTM_SEC_LE_ENCRYPTED | \
   BTM_SEC_LE_LINK_KEY_KNOWN | BTM_SEC_LE_LINK_KEY_AUTHED)

/*
 * Define device configuration structure
*/
typedef struct {
  tBTM_LOC_BD_NAME bd_name;  /* local Bluetooth device name */
  bool pin_type;             /* true if PIN type is fixed */
  uint8_t pin_code_len;      /* Bonding information */
  PIN_CODE pin_code;         /* PIN CODE if pin type is fixed */
  bool connectable;          /* If true page scan should be enabled */
  uint8_t def_inq_scan_mode; /* ??? limited/general/none */
} tBTM_CFG;

enum {
  BTM_PM_ST_ACTIVE = BTM_PM_STS_ACTIVE,
  BTM_PM_ST_HOLD = BTM_PM_STS_HOLD,
  BTM_PM_ST_SNIFF = BTM_PM_STS_SNIFF,
  BTM_PM_ST_PARK = BTM_PM_STS_PARK,
  BTM_PM_ST_PENDING = BTM_PM_STS_PENDING,
  BTM_PM_ST_INVALID = 0xFF
};
typedef uint8_t tBTM_PM_STATE;

enum {
  BTM_PM_SET_MODE_EVT, /* Set power mode API is called. */
  BTM_PM_UPDATE_EVT,
  BTM_PM_RD_MODE_EVT /* Read power mode API is called. */
};
typedef uint8_t tBTM_PM_EVENT;

typedef struct {
  uint16_t event;
  uint16_t len;
  uint8_t link_ind;
} tBTM_PM_MSG_DATA;

typedef struct {
  uint8_t hci_status;
  uint8_t mode;
  uint16_t interval;
} tBTM_PM_MD_CHG_DATA;

typedef struct {
  uint8_t pm_id; /* the entity that calls SetPowerMode API */
  tBTM_PM_PWR_MD* p_pmd;
} tBTM_PM_SET_MD_DATA;

typedef struct {
  void* p_data;
  uint8_t link_ind;
} tBTM_PM_SM_DATA;

typedef struct {
  tBTM_PM_PWR_MD req_mode[BTM_MAX_PM_RECORDS + 1]; /* the desired mode and
                                                      parameters of the
                                                      connection*/
  tBTM_PM_PWR_MD
      set_mode; /* the mode and parameters sent down to the host controller. */
  uint16_t interval; /* the interval from last mode change event. */
#if (BTM_SSR_INCLUDED == TRUE)
  uint16_t max_lat;    /* stored SSR maximum latency */
  uint16_t min_rmt_to; /* stored SSR minimum remote timeout */
  uint16_t min_loc_to; /* stored SSR minimum local timeout */
#endif
  tBTM_PM_STATE state; /* contains the current mode of the connection */
  bool chg_ind;        /* a request change indication */
} tBTM_PM_MCB;

#define BTM_PM_REC_NOT_USED 0
typedef struct {
  tBTM_PM_STATUS_CBACK*
      cback;    /* to notify the registered party of mode change event */
  uint8_t mask; /* registered request mask. 0, if this entry is not used */
} tBTM_PM_RCB;

enum {
  BTM_BLI_ACL_UP_EVT,
  BTM_BLI_ACL_DOWN_EVT,
  BTM_BLI_PAGE_EVT,
  BTM_BLI_PAGE_DONE_EVT,
  BTM_BLI_INQ_EVT,
  BTM_BLI_INQ_CANCEL_EVT,
  BTM_BLI_INQ_DONE_EVT
};
typedef uint8_t tBTM_BLI_EVENT;

/* Pairing State */
enum {
  BTM_PAIR_STATE_IDLE, /* Idle                                         */
  BTM_PAIR_STATE_GET_REM_NAME, /* Getting the remote name (to check for SM4) */
  BTM_PAIR_STATE_WAIT_PIN_REQ, /* Started authentication, waiting for PIN req
                                  (PIN is pre-fetched) */
  BTM_PAIR_STATE_WAIT_LOCAL_PIN,       /* Waiting for local PIN code */
  BTM_PAIR_STATE_WAIT_NUMERIC_CONFIRM, /* Waiting user 'yes' to numeric
                                          confirmation   */
  BTM_PAIR_STATE_KEY_ENTRY, /* Key entry state (we are a keyboard)          */
  BTM_PAIR_STATE_WAIT_LOCAL_OOB_RSP, /* Waiting for local response to peer OOB
                                        data  */
  BTM_PAIR_STATE_WAIT_LOCAL_IOCAPS, /* Waiting for local IO capabilities and OOB
                                       data */
  BTM_PAIR_STATE_INCOMING_SSP, /* Incoming SSP (got peer IO caps when idle) */
  BTM_PAIR_STATE_WAIT_AUTH_COMPLETE, /* All done, waiting authentication
                                        cpmplete    */
  BTM_PAIR_STATE_WAIT_DISCONNECT     /* Waiting to disconnect the ACL */
};
typedef uint8_t tBTM_PAIRING_STATE;

#define BTM_PAIR_FLAGS_WE_STARTED_DD \
  0x01 /* We want to do dedicated bonding              */
#define BTM_PAIR_FLAGS_PEER_STARTED_DD \
  0x02 /* Peer initiated dedicated bonding             */
#define BTM_PAIR_FLAGS_DISC_WHEN_DONE 0x04 /* Disconnect when done     */
#define BTM_PAIR_FLAGS_PIN_REQD \
  0x08 /* set this bit when pin_callback is called     */
#define BTM_PAIR_FLAGS_PRE_FETCH_PIN \
  0x10 /* set this bit when pre-fetch pin     */
#define BTM_PAIR_FLAGS_REJECTED_CONNECT \
  0x20 /* set this bit when rejected incoming connection  */
#define BTM_PAIR_FLAGS_WE_CANCEL_DD \
  0x40 /* set this bit when cancelling a bonding procedure */
#define BTM_PAIR_FLAGS_LE_ACTIVE \
  0x80 /* use this bit when SMP pairing is active */

typedef struct {
  bool is_mux;
  RawAddress bd_addr;
  uint16_t psm;
  bool is_orig;
  tBTM_SEC_CALLBACK* p_callback;
  void* p_ref_data;
  uint32_t mx_proto_id;
  uint32_t mx_chan_id;
  tBT_TRANSPORT transport;
  tBTM_BLE_SEC_ACT sec_act;
} tBTM_SEC_QUEUE_ENTRY;

#define CONN_ORIENT_TERM false
#define CONN_ORIENT_ORIG true
typedef bool CONNECTION_TYPE;

/* Define a structure to hold all the BTM data
*/

#define BTM_STATE_BUFFER_SIZE 5 /* size of state buffer */

typedef struct {
  tBTM_CFG cfg; /* Device configuration */

  /****************************************************
  **      ACL Management
  ****************************************************/
  tACL_CONN acl_db[MAX_L2CAP_LINKS];
  uint8_t btm_scn[BTM_MAX_SCN]; /* current SCNs: true if SCN is in use */
  uint16_t btm_def_link_policy;
  uint16_t btm_def_link_super_tout;

  tBTM_BL_EVENT_MASK bl_evt_mask;
  tBTM_BL_CHANGE_CB* p_bl_changed_cb; /* Callback for when Busy Level changed */

  /****************************************************
  **      Power Management
  ****************************************************/
  tBTM_PM_MCB pm_mode_db[MAX_L2CAP_LINKS];       /* per ACL link */
  tBTM_PM_RCB pm_reg_db[BTM_MAX_PM_RECORDS + 1]; /* per application/module */
  uint8_t pm_pend_link; /* the index of acl_db, which has a pending PM cmd */
  uint8_t pm_pend_id;   /* the id pf the module, which has a pending PM cmd */

  /*****************************************************
  **      Device control
  *****************************************************/
  tBTM_DEVCB devcb;

  /*****************************************************
  **      BLE Device controllers
  *****************************************************/
  tBTM_BLE_CB ble_ctr_cb;

  uint16_t enc_handle;
  BT_OCTET8 enc_rand; /* received rand value from LTK request*/
  uint16_t ediv;      /* received ediv value from LTK request */
  uint8_t key_size;
  tBTM_BLE_VSC_CB cmn_ble_vsc_cb;

  /* Packet types supported by the local device */
  uint16_t btm_acl_pkt_types_supported;
  uint16_t btm_sco_pkt_types_supported;

  /*****************************************************
  **      Inquiry
  *****************************************************/
  tBTM_INQUIRY_VAR_ST btm_inq_vars;

/*****************************************************
**      SCO Management
*****************************************************/
#if (BTM_SCO_INCLUDED == TRUE)
  tSCO_CB sco_cb;
#endif

  /*****************************************************
  **      Security Management
  *****************************************************/
  tBTM_APPL_INFO api;

#define BTM_SEC_MAX_RMT_NAME_CALLBACKS 2
  tBTM_RMT_NAME_CALLBACK* p_rmt_name_callback[BTM_SEC_MAX_RMT_NAME_CALLBACKS];

  tBTM_SEC_DEV_REC* p_collided_dev_rec;
  alarm_t* sec_collision_timer;
  uint32_t collision_start_time;
  uint32_t max_collision_delay;
  uint32_t dev_rec_count; /* Counter used for device record timestamp */
  uint8_t security_mode;
  bool pairing_disabled;
  bool connect_only_paired;
  bool security_mode_changed; /* mode changed during bonding */
  bool pin_type_changed;      /* pin type changed during bonding */
  bool sec_req_pending;       /*   true if a request is pending */

  uint8_t pin_code_len;             /* for legacy devices */
  PIN_CODE pin_code;                /* for legacy devices */
  tBTM_PAIRING_STATE pairing_state; /* The current pairing state    */
  uint8_t pairing_flags;            /* The current pairing flags    */
  RawAddress pairing_bda;           /* The device currently pairing */
  alarm_t* pairing_timer;           /* Timer for pairing process    */
  uint16_t disc_handle;             /* for legacy devices */
  uint8_t disc_reason;              /* for legacy devices */
  tBTM_SEC_SERV_REC sec_serv_rec[BTM_SEC_MAX_SERVICE_RECORDS];
  list_t* sec_dev_rec; /* list of tBTM_SEC_DEV_REC */
  tBTM_SEC_SERV_REC* p_out_serv;
  tBTM_MKEY_CALLBACK* mkey_cback;

  RawAddress connecting_bda;
  DEV_CLASS connecting_dc;

  uint8_t acl_disc_reason;
  uint8_t trace_level;
  uint8_t busy_level; /* the current busy level */
  bool is_paging;     /* true, if paging is in progess */
  bool is_inquiry;    /* true, if inquiry is in progess */
  fixed_queue_t* page_queue;
  bool paging;
  bool discing;
  fixed_queue_t* sec_pending_q; /* pending sequrity requests in
                                   tBTM_SEC_QUEUE_ENTRY format */

  char state_temp_buffer[BTM_STATE_BUFFER_SIZE];
} tBTM_CB;

/* security action for L2CAP COC channels */
#define BTM_SEC_OK 1
#define BTM_SEC_ENCRYPT 2         /* encrypt the link with current key */
#define BTM_SEC_ENCRYPT_NO_MITM 3 /* unauthenticated encryption or better */
#define BTM_SEC_ENCRYPT_MITM 4    /* authenticated encryption */
#define BTM_SEC_ENC_PENDING 5     /* wait for link encryption pending */

typedef uint8_t tBTM_SEC_ACTION;

#endif  // BTM_INT_TYPES_H
