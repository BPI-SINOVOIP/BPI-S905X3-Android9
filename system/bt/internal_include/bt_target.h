/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
 *  Copyright 1999-2016 Broadcom Corporation
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

#ifndef BT_TARGET_H
#define BT_TARGET_H

#ifndef BUILDCFG
#define BUILDCFG
#endif

#if !defined(HAS_BDROID_BUILDCFG) && !defined(HAS_NO_BDROID_BUILDCFG)
#error \
    "An Android.mk file did not include bdroid_CFLAGS and possibly not bdroid_C_INCLUDES"
#endif

#ifdef HAS_BDROID_BUILDCFG
#include "bdroid_buildcfg.h"
#endif

#include "bt_types.h" /* This must be defined AFTER buildcfg.h */

//------------------Added from bdroid_buildcfg.h---------------------
#ifndef L2CAP_EXTFEA_SUPPORTED_MASK
#define L2CAP_EXTFEA_SUPPORTED_MASK                                            \
  (L2CAP_EXTFEA_ENH_RETRANS | L2CAP_EXTFEA_STREAM_MODE | L2CAP_EXTFEA_NO_CRC | \
   L2CAP_EXTFEA_FIXED_CHNLS)
#endif

#ifndef BTUI_OPS_FORMATS
#define BTUI_OPS_FORMATS (BTA_OP_VCARD21_MASK | BTA_OP_ANY_MASK)
#endif

#ifndef BTA_RFC_MTU_SIZE
#define BTA_RFC_MTU_SIZE \
  (L2CAP_MTU_SIZE - L2CAP_MIN_OFFSET - RFCOMM_DATA_OVERHEAD)
#endif

#ifndef BTA_PAN_INCLUDED
#define BTA_PAN_INCLUDED TRUE
#endif

#ifndef BTA_HD_INCLUDED
#define BTA_HD_INCLUDED TRUE
#endif

#ifndef BTA_HH_INCLUDED
#define BTA_HH_INCLUDED TRUE
#endif

#ifndef BTA_HH_ROLE
#define BTA_HH_ROLE BTA_MASTER_ROLE_PREF
#endif

#ifndef BTA_HH_LE_INCLUDED
#define BTA_HH_LE_INCLUDED TRUE
#endif

#ifndef BTA_AR_INCLUDED
#define BTA_AR_INCLUDED TRUE
#endif

#ifndef BTA_AV_SINK_INCLUDED
#define BTA_AV_SINK_INCLUDED FALSE
#endif

#ifndef BTA_DISABLE_DELAY
#define BTA_DISABLE_DELAY 200 /* in milliseconds */
#endif

#ifndef AVDT_VERSION
#define AVDT_VERSION 0x0103
#endif

#ifndef BTA_AG_AT_MAX_LEN
#define BTA_AG_AT_MAX_LEN 512
#endif

#ifndef BTA_AG_SCO_PKT_TYPES
#define BTA_AG_SCO_PKT_TYPES                                     \
  (BTM_SCO_LINK_ONLY_MASK | ESCO_PKT_TYPES_MASK_EV3 |            \
   ESCO_PKT_TYPES_MASK_NO_3_EV3 | ESCO_PKT_TYPES_MASK_NO_2_EV5 | \
   ESCO_PKT_TYPES_MASK_NO_3_EV5)
#endif

#ifndef BTA_AV_RET_TOUT
#define BTA_AV_RET_TOUT 15
#endif

/* TRUE to use SCMS-T content protection */
#ifndef BTA_AV_CO_CP_SCMS_T
#define BTA_AV_CO_CP_SCMS_T FALSE
#endif

#ifndef BTA_DM_SDP_DB_SIZE
#define BTA_DM_SDP_DB_SIZE 8000
#endif

#ifndef HL_INCLUDED
#define HL_INCLUDED TRUE
#endif

#ifndef AG_VOICE_SETTINGS
#define AG_VOICE_SETTINGS HCI_DEFAULT_VOICE_SETTINGS
#endif

#ifndef BTIF_DM_OOB_TEST
#define BTIF_DM_OOB_TEST TRUE
#endif

// How long to wait before activating sniff mode after entering the
// idle state for server FT/RFCOMM, OPS connections
#ifndef BTA_FTS_OPS_IDLE_TO_SNIFF_DELAY_MS
#define BTA_FTS_OPS_IDLE_TO_SNIFF_DELAY_MS 7000
#endif

// How long to wait before activating sniff mode after entering the
// idle state for client FT/RFCOMM connections
#ifndef BTA_FTC_IDLE_TO_SNIFF_DELAY_MS
#define BTA_FTC_IDLE_TO_SNIFF_DELAY_MS 5000
#endif

//------------------End added from bdroid_buildcfg.h---------------------

/******************************************************************************
 *
 * Buffer sizes
 *
 *****************************************************************************/

#ifndef BT_DEFAULT_BUFFER_SIZE
#define BT_DEFAULT_BUFFER_SIZE (4096 + 16)
#endif

#ifndef BT_SMALL_BUFFER_SIZE
#define BT_SMALL_BUFFER_SIZE 660
#endif

/* Receives HCI events from the lower-layer. */
#ifndef HCI_CMD_BUF_SIZE
#define HCI_CMD_BUF_SIZE BT_SMALL_BUFFER_SIZE
#endif

/* Sends SDP data packets. */
#ifndef SDP_DATA_BUF_SIZE
#define SDP_DATA_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* Sends RFCOMM command packets. */
#ifndef RFCOMM_CMD_BUF_SIZE
#define RFCOMM_CMD_BUF_SIZE BT_SMALL_BUFFER_SIZE
#endif

/* Sends RFCOMM data packets. */
#ifndef RFCOMM_DATA_BUF_SIZE
#define RFCOMM_DATA_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* Sends L2CAP packets to the peer and HCI messages to the controller. */
#ifndef L2CAP_CMD_BUF_SIZE
#define L2CAP_CMD_BUF_SIZE BT_SMALL_BUFFER_SIZE
#endif

#ifndef L2CAP_USER_TX_BUF_SIZE
#define L2CAP_USER_TX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

#ifndef L2CAP_USER_RX_BUF_SIZE
#define L2CAP_USER_RX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* Sends L2CAP segmented packets in ERTM mode */
#ifndef L2CAP_FCR_TX_BUF_SIZE
#define L2CAP_FCR_TX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* Receives L2CAP segmented packets in ERTM mode */
#ifndef L2CAP_FCR_RX_BUF_SIZE
#define L2CAP_FCR_RX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

#ifndef L2CAP_FCR_ERTM_BUF_SIZE
#define L2CAP_FCR_ERTM_BUF_SIZE (10240 + 24)
#endif

/* Number of ACL buffers to assign to LE */
/*
 * TODO: Do we need this?
 * It was used when the HCI buffers were shared with BR/EDR.
 */
#ifndef L2C_DEF_NUM_BLE_BUF_SHARED
#define L2C_DEF_NUM_BLE_BUF_SHARED 1
#endif

/* Used by BTM when it sends HCI commands to the controller. */
#ifndef BTM_CMD_BUF_SIZE
#define BTM_CMD_BUF_SIZE BT_SMALL_BUFFER_SIZE
#endif

#ifndef OBX_LRG_DATA_BUF_SIZE
#define OBX_LRG_DATA_BUF_SIZE (8080 + 26)
#endif

/* Used to send data to L2CAP. */
#ifndef GAP_DATA_BUF_SIZE
#define GAP_DATA_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* BNEP data and protocol messages. */
#ifndef BNEP_BUF_SIZE
#define BNEP_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* AVDTP buffer size for protocol messages */
#ifndef AVDT_CMD_BUF_SIZE
#define AVDT_CMD_BUF_SIZE BT_SMALL_BUFFER_SIZE
#endif

#ifndef PAN_BUF_SIZE
#define PAN_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* Maximum number of buffers to allocate for PAN */
#ifndef PAN_BUF_MAX
#define PAN_BUF_MAX 100
#endif

/* AVCTP buffer size for protocol messages */
#ifndef AVCT_CMD_BUF_SIZE
#define AVCT_CMD_BUF_SIZE 288
#endif

/* AVRCP buffer size for protocol messages */
#ifndef AVRC_CMD_BUF_SIZE
#define AVRC_CMD_BUF_SIZE 288
#endif

/* AVRCP Metadata buffer size for protocol messages */
#ifndef AVRC_META_CMD_BUF_SIZE
#define AVRC_META_CMD_BUF_SIZE BT_SMALL_BUFFER_SIZE
#endif

#ifndef BTA_HL_LRG_DATA_BUF_SIZE
#define BTA_HL_LRG_DATA_BUF_SIZE (10240 + 24)
#endif

/* GATT Data sending buffer size */
#ifndef GATT_DATA_BUF_SIZE
#define GATT_DATA_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/******************************************************************************
 *
 * BTM
 *
 *****************************************************************************/

/* Cancel Inquiry on incoming SSP */
#ifndef BTM_NO_SSP_ON_INQUIRY
#define BTM_NO_SSP_ON_INQUIRY FALSE
#endif

/* Includes SCO if TRUE */
#ifndef BTM_SCO_INCLUDED
#define BTM_SCO_INCLUDED TRUE /* TRUE includes SCO code */
#endif

/*  This is used to work around a controller bug that doesn't like Disconnect
 *  issued while there is a role switch in progress
*/
#ifndef BTM_DISC_DURING_RS
#define BTM_DISC_DURING_RS TRUE
#endif

/**************************
 * Initial SCO TX credit
 ************************/
/* max TX SCO data packet size */
#ifndef BTM_SCO_DATA_SIZE_MAX
#define BTM_SCO_DATA_SIZE_MAX 240
#endif

/* The size in bytes of the BTM inquiry database. */
#ifndef BTM_INQ_DB_SIZE
#define BTM_INQ_DB_SIZE 40
#endif

/* The default scan mode */
#ifndef BTM_DEFAULT_SCAN_TYPE
#define BTM_DEFAULT_SCAN_TYPE BTM_SCAN_TYPE_INTERLACED
#endif

/* Should connections to unknown devices be allowed when not discoverable? */
#ifndef BTM_ALLOW_CONN_IF_NONDISCOVER
#define BTM_ALLOW_CONN_IF_NONDISCOVER TRUE
#endif

/* Sets the Page_Scan_Window:  the length of time that the device is performing
 * a page scan. */
#ifndef BTM_DEFAULT_CONN_WINDOW
#define BTM_DEFAULT_CONN_WINDOW 0x0012
#endif

/* Sets the Page_Scan_Activity:  the interval between the start of two
 * consecutive page scans. */
#ifndef BTM_DEFAULT_CONN_INTERVAL
#define BTM_DEFAULT_CONN_INTERVAL 0x0800
#endif

/* When automatic inquiry scan is enabled, this sets the inquiry scan window. */
#ifndef BTM_DEFAULT_DISC_WINDOW
#define BTM_DEFAULT_DISC_WINDOW 0x0012
#endif

/* When automatic inquiry scan is enabled, this sets the inquiry scan interval.
 */
#ifndef BTM_DEFAULT_DISC_INTERVAL
#define BTM_DEFAULT_DISC_INTERVAL 0x0800
#endif

/* Default class of device
* {SERVICE_CLASS, MAJOR_CLASS, MINOR_CLASS}
*
* SERVICE_CLASS:0x5A (Bit17 -Networking,Bit19 - Capturing,Bit20 -Object
* Transfer,Bit22 -Telephony)
* MAJOR_CLASS:0x02 - PHONE
* MINOR_CLASS:0x0C - SMART_PHONE
*
*/
#ifndef BTA_DM_COD
#define BTA_DM_COD \
  { 0x5A, 0x02, 0x0C }
#endif

/* The number of SCO links. */
#ifndef BTM_MAX_SCO_LINKS
#define BTM_MAX_SCO_LINKS 3
#endif

/* The number of security records for peer devices. */
#ifndef BTM_SEC_MAX_DEVICE_RECORDS
#define BTM_SEC_MAX_DEVICE_RECORDS 100
#endif

/* The number of security records for services. */
#ifndef BTM_SEC_MAX_SERVICE_RECORDS
#define BTM_SEC_MAX_SERVICE_RECORDS 32
#endif

/* If True, force a retrieval of remote device name for each bond in case it's
 * changed */
#ifndef BTM_SEC_FORCE_RNR_FOR_DBOND
#define BTM_SEC_FORCE_RNR_FOR_DBOND FALSE
#endif

/* Maximum device name length used in btm database. */
#ifndef BTM_MAX_REM_BD_NAME_LEN
#define BTM_MAX_REM_BD_NAME_LEN 248
#endif

/* Maximum local device name length stored btm database */
#ifndef BTM_MAX_LOC_BD_NAME_LEN
#define BTM_MAX_LOC_BD_NAME_LEN 248
#endif

/* Fixed Default String. When this is defined as null string, the device's
 * product model name is used as the default local name.
 */
#ifndef BTM_DEF_LOCAL_NAME
#define BTM_DEF_LOCAL_NAME ""
#endif

/* Maximum service name stored with security authorization (0 if not needed) */
#ifndef BTM_SEC_SERVICE_NAME_LEN
#define BTM_SEC_SERVICE_NAME_LEN BT_MAX_SERVICE_NAME_LEN
#endif

/* Maximum length of the service name. */
#ifndef BT_MAX_SERVICE_NAME_LEN
#define BT_MAX_SERVICE_NAME_LEN 21
#endif

/* The maximum number of clients that can register with the power manager. */
#ifndef BTM_MAX_PM_RECORDS
#define BTM_MAX_PM_RECORDS 2
#endif

/* This is set to show debug trace messages for the power manager. */
#ifndef BTM_PM_DEBUG
#define BTM_PM_DEBUG FALSE
#endif

/* This is set to TRUE if link is to be unparked due to BTM_CreateSCO API. */
#ifndef BTM_SCO_WAKE_PARKED_LINK
#define BTM_SCO_WAKE_PARKED_LINK TRUE
#endif

/* If the user does not respond to security process requests within this many
 * seconds,
 * a negative response would be sent automatically.
 * 30 is LMP response timeout value */
#ifndef BTM_SEC_TIMEOUT_VALUE
#define BTM_SEC_TIMEOUT_VALUE 35
#endif

/* Maximum number of callbacks that can be registered using
 * BTM_RegisterForVSEvents */
#ifndef BTM_MAX_VSE_CALLBACKS
#define BTM_MAX_VSE_CALLBACKS 3
#endif

/******************************************
 *    Lisbon Features
 ******************************************/
/* This is set to TRUE if the FEC is required for EIR packet. */
#ifndef BTM_EIR_DEFAULT_FEC_REQUIRED
#define BTM_EIR_DEFAULT_FEC_REQUIRED TRUE
#endif

/* The IO capability of the local device (for Simple Pairing) */
#ifndef BTM_LOCAL_IO_CAPS
#define BTM_LOCAL_IO_CAPS BTM_IO_CAP_IO
#endif

#ifndef BTM_LOCAL_IO_CAPS_BLE
#define BTM_LOCAL_IO_CAPS_BLE BTM_IO_CAP_KBDISP
#endif

/* The default MITM Protection Requirement (for Simple Pairing)
 * Possible values are BTM_AUTH_SP_YES or BTM_AUTH_SP_NO */
#ifndef BTM_DEFAULT_AUTH_REQ
#define BTM_DEFAULT_AUTH_REQ BTM_AUTH_SP_NO
#endif

/* The default MITM Protection Requirement for dedicated bonding using Simple
 * Pairing
 * Possible values are BTM_AUTH_AP_YES or BTM_AUTH_AP_NO */
#ifndef BTM_DEFAULT_DD_AUTH_REQ
#define BTM_DEFAULT_DD_AUTH_REQ BTM_AUTH_AP_YES
#endif

/* TRUE to include Sniff Subrating */
#ifndef BTM_SSR_INCLUDED
#define BTM_SSR_INCLUDED TRUE
#endif

/*************************
 * End of Lisbon Features
 *************************/

/* 4.1/4.2 secure connections feature */
#ifndef SC_MODE_INCLUDED
#define SC_MODE_INCLUDED TRUE
#endif

/* Used for conformance testing ONLY */
#ifndef BTM_BLE_CONFORMANCE_TESTING
#define BTM_BLE_CONFORMANCE_TESTING FALSE
#endif

/******************************************************************************
 *
 * L2CAP
 *
 *****************************************************************************/

/* The maximum number of simultaneous links that L2CAP can support. */
#ifndef MAX_ACL_CONNECTIONS
#define MAX_L2CAP_LINKS 13
#else
#define MAX_L2CAP_LINKS MAX_ACL_CONNECTIONS
#endif

/* The maximum number of simultaneous channels that L2CAP can support. */
#ifndef MAX_L2CAP_CHANNELS
#define MAX_L2CAP_CHANNELS 32
#endif

/* The maximum number of simultaneous applications that can register with L2CAP.
 */
#ifndef MAX_L2CAP_CLIENTS
#define MAX_L2CAP_CLIENTS 15
#endif

/* The number of seconds of link inactivity before a link is disconnected. */
#ifndef L2CAP_LINK_INACTIVITY_TOUT
#define L2CAP_LINK_INACTIVITY_TOUT 4
#endif

/* The number of seconds of link inactivity after bonding before a link is
 * disconnected. */
#ifndef L2CAP_BONDING_TIMEOUT
#define L2CAP_BONDING_TIMEOUT 3
#endif

/* The time from the HCI connection complete to disconnect if no channel is
 * established. */
#ifndef L2CAP_LINK_STARTUP_TOUT
#define L2CAP_LINK_STARTUP_TOUT 60
#endif

/* The L2CAP MTU; must be in accord with the HCI ACL buffer size. */
#ifndef L2CAP_MTU_SIZE
#define L2CAP_MTU_SIZE 1691
#endif

/*
 * The L2CAP MPS over Bluetooth; must be in accord with the FCR tx buffer size
 * and ACL down buffer size.
 */
#ifndef L2CAP_MPS_OVER_BR_EDR
#define L2CAP_MPS_OVER_BR_EDR 1010
#endif

/* If host flow control enabled, this is the number of buffers the controller
 * can have unacknowledged. */
#ifndef L2CAP_HOST_FC_ACL_BUFS
#define L2CAP_HOST_FC_ACL_BUFS 20
#endif

/* This is set to enable L2CAP to  take the ACL link out of park mode when ACL
 * data is to be sent. */
#ifndef L2CAP_WAKE_PARKED_LINK
#define L2CAP_WAKE_PARKED_LINK TRUE
#endif

/* Whether link wants to be the master or the slave. */
#ifndef L2CAP_DESIRED_LINK_ROLE
#define L2CAP_DESIRED_LINK_ROLE HCI_ROLE_SLAVE
#endif

/* Include Non-Flushable Packet Boundary Flag feature of Lisbon */
#ifndef L2CAP_NON_FLUSHABLE_PB_INCLUDED
#define L2CAP_NON_FLUSHABLE_PB_INCLUDED TRUE
#endif

/* Minimum number of ACL credit for high priority link */
#ifndef L2CAP_HIGH_PRI_MIN_XMIT_QUOTA
#define L2CAP_HIGH_PRI_MIN_XMIT_QUOTA 5
#endif

/* used for monitoring HCI ACL credit management */
#ifndef L2CAP_HCI_FLOW_CONTROL_DEBUG
#define L2CAP_HCI_FLOW_CONTROL_DEBUG TRUE
#endif

/* Used for features using fixed channels; set to zero if no fixed channels
 * supported (BLE, etc.) */
/* Excluding L2CAP signaling channel and UCD */
#ifndef L2CAP_NUM_FIXED_CHNLS
#define L2CAP_NUM_FIXED_CHNLS 32
#endif

/* First fixed channel supported */
#ifndef L2CAP_FIRST_FIXED_CHNL
#define L2CAP_FIRST_FIXED_CHNL 4
#endif

#ifndef L2CAP_LAST_FIXED_CHNL
#define L2CAP_LAST_FIXED_CHNL \
  (L2CAP_FIRST_FIXED_CHNL + L2CAP_NUM_FIXED_CHNLS - 1)
#endif

/* Round Robin service channels in link */
#ifndef L2CAP_ROUND_ROBIN_CHANNEL_SERVICE
#define L2CAP_ROUND_ROBIN_CHANNEL_SERVICE TRUE
#endif

/* used for monitoring eL2CAP data flow */
#ifndef L2CAP_ERTM_STATS
#define L2CAP_ERTM_STATS FALSE
#endif

/* Used for conformance testing ONLY:  When TRUE lets scriptwrapper overwrite
 * info response */
#ifndef L2CAP_CONFORMANCE_TESTING
#define L2CAP_CONFORMANCE_TESTING FALSE
#endif

/*
 * Max bytes per connection to buffer locally before dropping the
 * connection if local client does not receive it  - default is 1MB
 */
#ifndef L2CAP_MAX_RX_BUFFER
#define L2CAP_MAX_RX_BUFFER 0x100000
#endif

/******************************************************************************
 *
 * BLE
 *
 *****************************************************************************/

#ifndef LOCAL_BLE_CONTROLLER_ID
#define LOCAL_BLE_CONTROLLER_ID 1
#endif

/*
 * Toggles support for general LE privacy features such as remote address
 * resolution, local address rotation etc.
 */
#ifndef BLE_PRIVACY_SPT
#define BLE_PRIVACY_SPT TRUE
#endif

/*
 * Enables or disables support for local privacy (ex. address rotation)
 */
#ifndef BLE_LOCAL_PRIVACY_ENABLED
#define BLE_LOCAL_PRIVACY_ENABLED TRUE
#endif

/*
 * Toggles support for vendor specific extensions such as RPA offloading,
 * feature discovery, multi-adv etc.
 */
#ifndef BLE_VND_INCLUDED
#define BLE_VND_INCLUDED FALSE
#endif

/* The maximum number of simultaneous applications that can register with LE
 * L2CAP. */
#ifndef BLE_MAX_L2CAP_CLIENTS
#define BLE_MAX_L2CAP_CLIENTS 15
#endif

/******************************************************************************
 *
 * ATT/GATT Protocol/Profile Settings
 *
 *****************************************************************************/
#ifndef BLE_LLT_INCLUDED
#define BLE_LLT_INCLUDED TRUE
#endif

#ifndef BLE_DELAY_REQUEST_ENC
/* This flag is to work around IPHONE problem, We need to wait for iPhone ready
   before send encryption request to iPhone */
#define BLE_DELAY_REQUEST_ENC FALSE
#endif

#ifndef GATT_MAX_SR_PROFILES
#define GATT_MAX_SR_PROFILES 32 /* max is 32 */
#endif

#ifndef GATT_MAX_APPS
#define GATT_MAX_APPS 32 /* note: 2 apps used internally GATT and GAP */
#endif

#ifndef GATT_MAX_PHY_CHANNEL
#define GATT_MAX_PHY_CHANNEL 7
#endif

/* Used for conformance testing ONLY */
#ifndef GATT_CONFORMANCE_TESTING
#define GATT_CONFORMANCE_TESTING FALSE
#endif

/******************************************************************************
 *
 * SMP
 *
 *****************************************************************************/
#ifndef SMP_DEBUG
#define SMP_DEBUG FALSE
#endif

#ifndef SMP_DEFAULT_AUTH_REQ
#define SMP_DEFAULT_AUTH_REQ SMP_AUTH_NB_ENC_ONLY
#endif

#ifndef SMP_MAX_ENC_KEY_SIZE
#define SMP_MAX_ENC_KEY_SIZE 16
#endif

/* minimum link timeout after SMP pairing is done, leave room for key exchange
   and racing condition for the following service connection.
   Prefer greater than 0 second, and no less than default inactivity link idle
   timer(L2CAP_LINK_INACTIVITY_TOUT) in l2cap) */
#ifndef SMP_LINK_TOUT_MIN
#if (L2CAP_LINK_INACTIVITY_TOUT > 0)
#define SMP_LINK_TOUT_MIN L2CAP_LINK_INACTIVITY_TOUT
#else
#define SMP_LINK_TOUT_MIN 2
#endif
#endif
/******************************************************************************
 *
 * SDP
 *
 *****************************************************************************/

/* This is set to enable SDP server functionality. */
#ifndef SDP_SERVER_ENABLED
#define SDP_SERVER_ENABLED TRUE
#endif

/* The maximum number of SDP records the server can support. */
#ifndef SDP_MAX_RECORDS
#define SDP_MAX_RECORDS 30
#endif

/* The maximum number of attributes in each record. */
#ifndef SDP_MAX_REC_ATTR
#define SDP_MAX_REC_ATTR 25
#endif

#ifndef SDP_MAX_PAD_LEN
#define SDP_MAX_PAD_LEN 600
#endif

/* The maximum length, in bytes, of an attribute. */
#ifndef SDP_MAX_ATTR_LEN
#define SDP_MAX_ATTR_LEN 400
#endif

/* The maximum number of attribute filters supported by SDP databases. */
#ifndef SDP_MAX_ATTR_FILTERS
#define SDP_MAX_ATTR_FILTERS 15
#endif

/* The maximum number of UUID filters supported by SDP databases. */
#ifndef SDP_MAX_UUID_FILTERS
#define SDP_MAX_UUID_FILTERS 3
#endif

/* The maximum number of record handles retrieved in a search. */
#ifndef SDP_MAX_DISC_SERVER_RECS
#define SDP_MAX_DISC_SERVER_RECS 21
#endif

/* The size of a scratchpad buffer, in bytes, for storing the response to an
 * attribute request. */
#ifndef SDP_MAX_LIST_BYTE_COUNT
#define SDP_MAX_LIST_BYTE_COUNT 4096
#endif

/* The maximum number of parameters in an SDP protocol element. */
#ifndef SDP_MAX_PROTOCOL_PARAMS
#define SDP_MAX_PROTOCOL_PARAMS 2
#endif

/* The maximum number of simultaneous client and server connections. */
#ifndef SDP_MAX_CONNECTIONS
#define SDP_MAX_CONNECTIONS 4
#endif

/* The MTU size for the L2CAP configuration. */
#ifndef SDP_MTU_SIZE
#define SDP_MTU_SIZE 1024
#endif

/* The flush timeout for the L2CAP configuration. */
#ifndef SDP_FLUSH_TO
#define SDP_FLUSH_TO 0xFFFF
#endif

/* The name for security authorization. */
#ifndef SDP_SERVICE_NAME
#define SDP_SERVICE_NAME "Service Discovery"
#endif

/* The security level for BTM. */
#ifndef SDP_SECURITY_LEVEL
#define SDP_SECURITY_LEVEL BTM_SEC_NONE
#endif

/******************************************************************************
 *
 * RFCOMM
 *
 *****************************************************************************/

/* The maximum number of ports supported. */
#ifndef MAX_RFC_PORTS
#define MAX_RFC_PORTS 30
#endif

/* The maximum simultaneous links to different devices. */
#ifndef MAX_ACL_CONNECTIONS
#define MAX_BD_CONNECTIONS 7
#else
#define MAX_BD_CONNECTIONS MAX_ACL_CONNECTIONS
#endif

/* The port receive queue low watermark level, in bytes. */
#ifndef PORT_RX_LOW_WM
#define PORT_RX_LOW_WM (BTA_RFC_MTU_SIZE * PORT_RX_BUF_LOW_WM)
#endif

/* The port receive queue high watermark level, in bytes. */
#ifndef PORT_RX_HIGH_WM
#define PORT_RX_HIGH_WM (BTA_RFC_MTU_SIZE * PORT_RX_BUF_HIGH_WM)
#endif

/* The port receive queue critical watermark level, in bytes. */
#ifndef PORT_RX_CRITICAL_WM
#define PORT_RX_CRITICAL_WM (BTA_RFC_MTU_SIZE * PORT_RX_BUF_CRITICAL_WM)
#endif

/* The port receive queue low watermark level, in number of buffers. */
#ifndef PORT_RX_BUF_LOW_WM
#define PORT_RX_BUF_LOW_WM 4
#endif

/* The port receive queue high watermark level, in number of buffers. */
#ifndef PORT_RX_BUF_HIGH_WM
#define PORT_RX_BUF_HIGH_WM 10
#endif

/* The port receive queue critical watermark level, in number of buffers. */
#ifndef PORT_RX_BUF_CRITICAL_WM
#define PORT_RX_BUF_CRITICAL_WM 15
#endif

/* The port transmit queue high watermark level, in bytes. */
#ifndef PORT_TX_HIGH_WM
#define PORT_TX_HIGH_WM (BTA_RFC_MTU_SIZE * PORT_TX_BUF_HIGH_WM)
#endif

/* The port transmit queue critical watermark level, in bytes. */
#ifndef PORT_TX_CRITICAL_WM
#define PORT_TX_CRITICAL_WM (BTA_RFC_MTU_SIZE * PORT_TX_BUF_CRITICAL_WM)
#endif

/* The port transmit queue high watermark level, in number of buffers. */
#ifndef PORT_TX_BUF_HIGH_WM
#define PORT_TX_BUF_HIGH_WM 10
#endif

/* The port transmit queue high watermark level, in number of buffers. */
#ifndef PORT_TX_BUF_CRITICAL_WM
#define PORT_TX_BUF_CRITICAL_WM 15
#endif

/* The RFCOMM multiplexer preferred flow control mechanism. */
#ifndef PORT_FC_DEFAULT
#define PORT_FC_DEFAULT PORT_FC_CREDIT
#endif

/******************************************************************************
 *
 * OBEX
 *
 *****************************************************************************/

/*
 * Buffer size to reassemble the SDU.
 * It will allow buffers to be used that are larger than the L2CAP_MAX_MTU.
 */
#ifndef OBX_USER_RX_BUF_SIZE
#define OBX_USER_RX_BUF_SIZE OBX_LRG_DATA_BUF_SIZE
#endif

/*
 * Buffer size to hold the SDU.
 * It will allow buffers to be used that are larger than the L2CAP_MAX_MTU.
 */
#ifndef OBX_USER_TX_BUF_SIZE
#define OBX_USER_TX_BUF_SIZE OBX_LRG_DATA_BUF_SIZE
#endif

/* Buffer size used to hold MPS segments during SDU reassembly. */
#ifndef OBX_FCR_RX_BUF_SIZE
#define OBX_FCR_RX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/*
 * Buffer size used to hold MPS segments used in (re)transmissions.
 * The size of each buffer must be able to hold the maximum MPS segment size
 * passed in L2CA_SetFCROptions plus BT_HDR (8) + HCI preamble (4) +
 * L2CAP_MIN_OFFSET (11 - as of BT 2.1 + EDR Spec).
 */
#ifndef OBX_FCR_TX_BUF_SIZE
#define OBX_FCR_TX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/*
 * Size of the transmission window when using enhanced retransmission mode.
 * Not used in basic and streaming modes. Range: 1 - 63
 */
#ifndef OBX_FCR_OPT_TX_WINDOW_SIZE_BR_EDR
#define OBX_FCR_OPT_TX_WINDOW_SIZE_BR_EDR 20
#endif

/*
 * Number of transmission attempts for a single I-Frame before taking
 * Down the connection. Used In ERTM mode only. Value is Ignored in basic and
 * Streaming modes.
 * Range: 0, 1-0xFF
 * 0 - infinite retransmissions
 * 1 - single transmission
 */
#ifndef OBX_FCR_OPT_MAX_TX_B4_DISCNT
#define OBX_FCR_OPT_MAX_TX_B4_DISCNT 20
#endif

/*
 * Retransmission Timeout
 * Range: Minimum 2000 (2 secs) on BR/EDR when supporting PBF.
 */
#ifndef OBX_FCR_OPT_RETX_TOUT
#define OBX_FCR_OPT_RETX_TOUT 2000
#endif

/*
 * Monitor Timeout
 * Range: Minimum 12000 (12 secs) on BR/EDR when supporting PBF.
 */
#ifndef OBX_FCR_OPT_MONITOR_TOUT
#define OBX_FCR_OPT_MONITOR_TOUT 12000
#endif

/*
 * Maximum PDU payload size.
 * Suggestion: The maximum amount of data that will fit into a 3-DH5 packet.
 * Range: 2 octets
 */
#ifndef OBX_FCR_OPT_MAX_PDU_SIZE
#define OBX_FCR_OPT_MAX_PDU_SIZE L2CAP_MPS_OVER_BR_EDR
#endif

/******************************************************************************
 *
 * BNEP
 *
 *****************************************************************************/

#ifndef BNEP_INCLUDED
#define BNEP_INCLUDED TRUE
#endif

/* BNEP status API call is used mainly to get the L2CAP handle */
#ifndef BNEP_SUPPORTS_STATUS_API
#define BNEP_SUPPORTS_STATUS_API TRUE
#endif

/*
 * When BNEP connection changes roles after the connection is established
 * we will do an authentication check again on the new role
*/
#ifndef BNEP_DO_AUTH_FOR_ROLE_SWITCH
#define BNEP_DO_AUTH_FOR_ROLE_SWITCH TRUE
#endif

/* Maximum number of protocol filters supported. */
#ifndef BNEP_MAX_PROT_FILTERS
#define BNEP_MAX_PROT_FILTERS 5
#endif

/* Maximum number of multicast filters supported. */
#ifndef BNEP_MAX_MULTI_FILTERS
#define BNEP_MAX_MULTI_FILTERS 5
#endif

/* Minimum MTU size. */
#ifndef BNEP_MIN_MTU_SIZE
#define BNEP_MIN_MTU_SIZE L2CAP_MTU_SIZE
#endif

/* Preferred MTU size. */
#ifndef BNEP_MTU_SIZE
#define BNEP_MTU_SIZE BNEP_MIN_MTU_SIZE
#endif

/* Maximum number of buffers allowed in transmit data queue. */
#ifndef BNEP_MAX_XMITQ_DEPTH
#define BNEP_MAX_XMITQ_DEPTH 20
#endif

/* Maximum number BNEP of connections supported. */
#ifndef BNEP_MAX_CONNECTIONS
#define BNEP_MAX_CONNECTIONS 7
#endif

/******************************************************************************
 *
 * AVDTP
 *
 *****************************************************************************/

/* Number of simultaneous links to different peer devices. */
#ifndef AVDT_NUM_LINKS
#define AVDT_NUM_LINKS 6
#endif

/* Number of simultaneous stream endpoints. */
#ifndef AVDT_NUM_SEPS
#define AVDT_NUM_SEPS 6
#endif

/* Number of transport channels setup by AVDT for all media streams */
#ifndef AVDT_NUM_TC_TBL
#define AVDT_NUM_TC_TBL (AVDT_NUM_SEPS + AVDT_NUM_LINKS)
#endif

/* Maximum size in bytes of the content protection information element. */
#ifndef AVDT_PROTECT_SIZE
#define AVDT_PROTECT_SIZE 90
#endif

/******************************************************************************
 *
 * PAN
 *
 *****************************************************************************/

#ifndef PAN_INCLUDED
#define PAN_INCLUDED TRUE
#endif

#ifndef PAN_NAP_DISABLED
#define PAN_NAP_DISABLED FALSE
#endif

#ifndef PANU_DISABLED
#define PANU_DISABLED FALSE
#endif

/* This will enable the PANU role */
#ifndef PAN_SUPPORTS_ROLE_PANU
#define PAN_SUPPORTS_ROLE_PANU TRUE
#endif

/* This will enable the GN role */
#ifndef PAN_SUPPORTS_ROLE_GN
#define PAN_SUPPORTS_ROLE_GN TRUE
#endif

/* This will enable the NAP role */
#ifndef PAN_SUPPORTS_ROLE_NAP
#define PAN_SUPPORTS_ROLE_NAP TRUE
#endif

/* This is just for debugging purposes */
#ifndef PAN_SUPPORTS_DEBUG_DUMP
#define PAN_SUPPORTS_DEBUG_DUMP TRUE
#endif

/* Maximum number of PAN connections allowed */
#ifndef MAX_PAN_CONNS
#define MAX_PAN_CONNS 7
#endif

/* Default service name for NAP role */
#ifndef PAN_NAP_DEFAULT_SERVICE_NAME
#define PAN_NAP_DEFAULT_SERVICE_NAME "Network Access Point Service"
#endif

/* Default service name for GN role */
#ifndef PAN_GN_DEFAULT_SERVICE_NAME
#define PAN_GN_DEFAULT_SERVICE_NAME "Group Network Service"
#endif

/* Default service name for PANU role */
#ifndef PAN_PANU_DEFAULT_SERVICE_NAME
#define PAN_PANU_DEFAULT_SERVICE_NAME "PAN User Service"
#endif

/* Default description for NAP role service */
#ifndef PAN_NAP_DEFAULT_DESCRIPTION
#define PAN_NAP_DEFAULT_DESCRIPTION "NAP"
#endif

/* Default description for GN role service */
#ifndef PAN_GN_DEFAULT_DESCRIPTION
#define PAN_GN_DEFAULT_DESCRIPTION "GN"
#endif

/* Default description for PANU role service */
#ifndef PAN_PANU_DEFAULT_DESCRIPTION
#define PAN_PANU_DEFAULT_DESCRIPTION "PANU"
#endif

/* Default Security level for PANU role. */
#ifndef PAN_PANU_SECURITY_LEVEL
#define PAN_PANU_SECURITY_LEVEL 0
#endif

/* Default Security level for GN role. */
#ifndef PAN_GN_SECURITY_LEVEL
#define PAN_GN_SECURITY_LEVEL 0
#endif

/* Default Security level for NAP role. */
#ifndef PAN_NAP_SECURITY_LEVEL
#define PAN_NAP_SECURITY_LEVEL 0
#endif

/******************************************************************************
 *
 * GAP
 *
 *****************************************************************************/

#ifndef GAP_INCLUDED
#define GAP_INCLUDED TRUE
#endif

/* The maximum number of simultaneous GAP L2CAP connections. */
#ifndef GAP_MAX_CONNECTIONS
#define GAP_MAX_CONNECTIONS 30
#endif

/* keep the raw data received from SDP server in database. */
#ifndef SDP_RAW_DATA_INCLUDED
#define SDP_RAW_DATA_INCLUDED TRUE
#endif

/* Inquiry duration in 1.28 second units. */
#ifndef SDP_DEBUG
#define SDP_DEBUG TRUE
#endif

/******************************************************************************
 *
 * HID
 *
 *****************************************************************************/

/* HID Device Role Included */
#ifndef HID_DEV_INCLUDED
#define HID_DEV_INCLUDED TRUE
#endif

#ifndef HID_CONTROL_BUF_SIZE
#define HID_CONTROL_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

#ifndef HID_INTERRUPT_BUF_SIZE
#define HID_INTERRUPT_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

#ifndef HID_DEV_MTU_SIZE
#define HID_DEV_MTU_SIZE 64
#endif

#ifndef HID_DEV_FLUSH_TO
#define HID_DEV_FLUSH_TO 0xffff
#endif

/*************************************************************************
 * Definitions for Both HID-Host & Device
*/
#ifndef HID_MAX_SVC_NAME_LEN
#define HID_MAX_SVC_NAME_LEN 32
#endif

#ifndef HID_MAX_SVC_DESCR_LEN
#define HID_MAX_SVC_DESCR_LEN 32
#endif

#ifndef HID_MAX_PROV_NAME_LEN
#define HID_MAX_PROV_NAME_LEN 32
#endif

/*************************************************************************
 * Definitions for HID-Host
*/
#ifndef HID_HOST_INCLUDED
#define HID_HOST_INCLUDED TRUE
#endif

#ifndef HID_HOST_MAX_DEVICES
#define HID_HOST_MAX_DEVICES 7
#endif

#ifndef HID_HOST_MTU
#define HID_HOST_MTU 640
#endif

#ifndef HID_HOST_FLUSH_TO
#define HID_HOST_FLUSH_TO 0xffff
#endif

#ifndef HID_HOST_MAX_CONN_RETRY
#define HID_HOST_MAX_CONN_RETRY 1
#endif

#ifndef HID_HOST_REPAGE_WIN
#define HID_HOST_REPAGE_WIN 2
#endif

/******************************************************************************
 *
 * AVCTP
 *
 *****************************************************************************/

/* Number of simultaneous ACL links to different peer devices. */
#ifndef AVCT_NUM_LINKS
#define AVCT_NUM_LINKS 6
#endif

/* Number of simultaneous AVCTP connections. */
#ifndef AVCT_NUM_CONN
#define AVCT_NUM_CONN 14  // 2 * MaxDevices + 2
#endif

/******************************************************************************
 *
 * AVRCP
 *
 *****************************************************************************/

#ifndef AVRC_ADV_CTRL_INCLUDED
#define AVRC_ADV_CTRL_INCLUDED TRUE
#endif

#ifndef DUMP_PCM_DATA
#define DUMP_PCM_DATA FALSE
#endif

/******************************************************************************
 *
 * MCAP
 *
 *****************************************************************************/
#ifndef MCA_INCLUDED
#define MCA_INCLUDED FALSE
#endif

/* The MTU size for the L2CAP configuration on control channel. 48 is the
 * minimal */
#ifndef MCA_CTRL_MTU
#define MCA_CTRL_MTU 60
#endif

/* The maximum number of registered MCAP instances. */
#ifndef MCA_NUM_REGS
#define MCA_NUM_REGS 12
#endif

/* The maximum number of control channels (to difference devices) per registered
 * MCAP instances. */
#ifndef MCA_NUM_LINKS
#define MCA_NUM_LINKS 3
#endif

/* The maximum number of MDEP (including HDP echo) per registered MCAP
 * instances. */
#ifndef MCA_NUM_DEPS
#define MCA_NUM_DEPS 13
#endif

/* The maximum number of MDL link per control channel. */
#ifndef MCA_NUM_MDLS
#define MCA_NUM_MDLS 4
#endif

/* Buffer size to reassemble the SDU. */
#ifndef MCA_USER_RX_BUF_SIZE
#define MCA_USER_RX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* Buffer size to hold the SDU. */
#ifndef MCA_USER_TX_BUF_SIZE
#define MCA_USER_TX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/*
 * Buffer size used to hold MPS segments during SDU reassembly
 */
#ifndef MCA_FCR_RX_BUF_SIZE
#define MCA_FCR_RX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/*
 * Default buffer size used to hold MPS segments used in (re)transmissions.
 * The size of each buffer must be able to hold the maximum MPS segment size
 * passed in tL2CAP_FCR_OPTIONS plus BT_HDR (8) + HCI preamble (4) +
 * L2CAP_MIN_OFFSET (11 - as of BT 2.1 + EDR Spec).
 */
#ifndef MCA_FCR_TX_BUF_SIZE
#define MCA_FCR_TX_BUF_SIZE BT_DEFAULT_BUFFER_SIZE
#endif

/* MCAP control channel FCR Option:
Size of the transmission window when using enhanced retransmission mode.
1 is defined by HDP specification for control channel.
*/
#ifndef MCA_FCR_OPT_TX_WINDOW_SIZE
#define MCA_FCR_OPT_TX_WINDOW_SIZE 1
#endif

/* MCAP control channel FCR Option:
Number of transmission attempts for a single I-Frame before taking
Down the connection. Used In ERTM mode only. Value is Ignored in basic and
Streaming modes.
Range: 0, 1-0xFF
0 - infinite retransmissions
1 - single transmission
*/
#ifndef MCA_FCR_OPT_MAX_TX_B4_DISCNT
#define MCA_FCR_OPT_MAX_TX_B4_DISCNT 20
#endif

/* MCAP control channel FCR Option: Retransmission Timeout
The AVRCP specification set a value in the range of 300 - 2000 ms
Timeout (in msecs) to detect Lost I-Frames. Only used in Enhanced retransmission
mode.
Range: Minimum 2000 (2 secs) when supporting PBF.
 */
#ifndef MCA_FCR_OPT_RETX_TOUT
#define MCA_FCR_OPT_RETX_TOUT 2000
#endif

/* MCAP control channel FCR Option: Monitor Timeout
The AVRCP specification set a value in the range of 300 - 2000 ms
Timeout (in msecs) to detect Lost S-Frames. Only used in Enhanced retransmission
mode.
Range: Minimum 12000 (12 secs) when supporting PBF.
*/
#ifndef MCA_FCR_OPT_MONITOR_TOUT
#define MCA_FCR_OPT_MONITOR_TOUT 12000
#endif

/* MCAP control channel FCR Option: Maximum PDU payload size.
The maximum number of payload octets that the local device can receive in a
single PDU.
*/
#ifndef MCA_FCR_OPT_MPS_SIZE
#define MCA_FCR_OPT_MPS_SIZE 1000
#endif

/******************************************************************************
 *
 * BTA
 *
 *****************************************************************************/
/* BTA EIR canned UUID list (default is dynamic) */
#ifndef BTA_EIR_CANNED_UUID_LIST
#define BTA_EIR_CANNED_UUID_LIST FALSE
#endif

/* Number of supported customer UUID in EIR */
#ifndef BTA_EIR_SERVER_NUM_CUSTOM_UUID
#define BTA_EIR_SERVER_NUM_CUSTOM_UUID 8
#endif

/* CHLD override */
#ifndef BTA_AG_CHLD_VAL_ECC
#define BTA_AG_CHLD_VAL_ECC "(0,1,1x,2,2x,3)"
#endif

#ifndef BTA_AG_CHLD_VAL
#define BTA_AG_CHLD_VAL "(0,1,2,3)"
#endif

/* Set the CIND to match HFP 1.5 */
#ifndef BTA_AG_CIND_INFO
#define BTA_AG_CIND_INFO                                                       \
  "(\"call\",(0,1)),(\"callsetup\",(0-3)),(\"service\",(0-1)),(\"signal\",(0-" \
  "5)),(\"roam\",(0,1)),(\"battchg\",(0-5)),(\"callheld\",(0-2))"
#endif

#ifndef BTA_DM_AVOID_A2DP_ROLESWITCH_ON_INQUIRY
#define BTA_DM_AVOID_A2DP_ROLESWITCH_ON_INQUIRY TRUE
#endif

/******************************************************************************
 *
 * Tracing:  Include trace header file here.
 *
 *****************************************************************************/

/* Enable/disable BTSnoop memory logging */
#ifndef BTSNOOP_MEM
#define BTSNOOP_MEM TRUE
#endif

#include "bt_trace.h"

#endif /* BT_TARGET_H */
