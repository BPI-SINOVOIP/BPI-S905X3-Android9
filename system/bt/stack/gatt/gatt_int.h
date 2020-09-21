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

#ifndef GATT_INT_H
#define GATT_INT_H

#include "bt_target.h"

#include "btm_ble_api.h"
#include "btu.h"
#include "gatt_api.h"
#include "osi/include/fixed_queue.h"

#include <base/strings/stringprintf.h>
#include <string.h>
#include <list>
#include <unordered_set>
#include <vector>

#define GATT_CREATE_CONN_ID(tcb_idx, gatt_if) \
  ((uint16_t)((((uint8_t)(tcb_idx)) << 8) | ((uint8_t)(gatt_if))))
#define GATT_GET_TCB_IDX(conn_id) ((uint8_t)(((uint16_t)(conn_id)) >> 8))
#define GATT_GET_GATT_IF(conn_id) ((tGATT_IF)((uint8_t)(conn_id)))

#define GATT_TRANS_ID_MAX 0x0fffffff /* 4 MSB is reserved */

/* security action for GATT write and read request */
#define GATT_SEC_NONE 0
#define GATT_SEC_OK 1
#define GATT_SEC_SIGN_DATA 2       /* compute the signature for the write cmd */
#define GATT_SEC_ENCRYPT 3         /* encrypt the link with current key */
#define GATT_SEC_ENCRYPT_NO_MITM 4 /* unauthenticated encryption or better */
#define GATT_SEC_ENCRYPT_MITM 5    /* authenticated encryption */
#define GATT_SEC_ENC_PENDING 6     /* wait for link encryption pending */
typedef uint8_t tGATT_SEC_ACTION;

#define GATT_ATTR_OP_SPT_MTU (0x00000001 << 0)
#define GATT_ATTR_OP_SPT_FIND_INFO (0x00000001 << 1)
#define GATT_ATTR_OP_SPT_FIND_BY_TYPE (0x00000001 << 2)
#define GATT_ATTR_OP_SPT_READ_BY_TYPE (0x00000001 << 3)
#define GATT_ATTR_OP_SPT_READ (0x00000001 << 4)
#define GATT_ATTR_OP_SPT_MULT_READ (0x00000001 << 5)
#define GATT_ATTR_OP_SPT_READ_BLOB (0x00000001 << 6)
#define GATT_ATTR_OP_SPT_READ_BY_GRP_TYPE (0x00000001 << 7)
#define GATT_ATTR_OP_SPT_WRITE (0x00000001 << 8)
#define GATT_ATTR_OP_SPT_WRITE_CMD (0x00000001 << 9)
#define GATT_ATTR_OP_SPT_PREP_WRITE (0x00000001 << 10)
#define GATT_ATTR_OP_SPT_EXE_WRITE (0x00000001 << 11)
#define GATT_ATTR_OP_SPT_HDL_VALUE_CONF (0x00000001 << 12)
#define GATT_ATTR_OP_SP_SIGN_WRITE (0x00000001 << 13)

#define GATT_INDEX_INVALID 0xff

#define GATT_PENDING_REQ_NONE 0

#define GATT_WRITE_CMD_MASK 0xc0 /*0x1100-0000*/
#define GATT_AUTH_SIGN_MASK 0x80 /*0x1000-0000*/
#define GATT_AUTH_SIGN_LEN 12

#define GATT_HDR_SIZE 3 /* 1B opcode + 2B handle */

/* wait for ATT cmd response timeout value */
#define GATT_WAIT_FOR_RSP_TIMEOUT_MS (30 * 1000)
#define GATT_WAIT_FOR_DISC_RSP_TIMEOUT_MS (5 * 1000)
#define GATT_REQ_RETRY_LIMIT 2

/* characteristic descriptor type */
#define GATT_DESCR_EXT_DSCPTOR 1  /* Characteristic Extended Properties */
#define GATT_DESCR_USER_DSCPTOR 2 /* Characteristic User Description    */
#define GATT_DESCR_CLT_CONFIG 3   /* Client Characteristic Configuration */
#define GATT_DESCR_SVR_CONFIG 4   /* Server Characteristic Configuration */
#define GATT_DESCR_PRES_FORMAT 5  /* Characteristic Presentation Format */
#define GATT_DESCR_AGGR_FORMAT 6  /* Characteristic Aggregate Format */
#define GATT_DESCR_VALID_RANGE 7  /* Characteristic Valid Range */
#define GATT_DESCR_UNKNOWN 0xff

#define GATT_SEC_FLAG_LKEY_UNAUTHED BTM_SEC_FLAG_LKEY_KNOWN
#define GATT_SEC_FLAG_LKEY_AUTHED BTM_SEC_FLAG_LKEY_AUTHED
#define GATT_SEC_FLAG_ENCRYPTED BTM_SEC_FLAG_ENCRYPTED
typedef uint8_t tGATT_SEC_FLAG;

/* Find Information Response Type
*/
#define GATT_INFO_TYPE_PAIR_16 0x01
#define GATT_INFO_TYPE_PAIR_128 0x02

/*  GATT client FIND_TYPE_VALUE_Request data */
typedef struct {
  bluetooth::Uuid uuid; /* type of attribute to be found */
  uint16_t s_handle;  /* starting handle */
  uint16_t e_handle;  /* ending handle */
  uint16_t value_len; /* length of the attribute value */
  uint8_t
      value[GATT_MAX_MTU_SIZE]; /* pointer to the attribute value to be found */
} tGATT_FIND_TYPE_VALUE;

/* client request message to ATT protocol
*/
typedef union {
  tGATT_READ_BY_TYPE browse;             /* read by type request */
  tGATT_FIND_TYPE_VALUE find_type_value; /* find by type value */
  tGATT_READ_MULTI read_multi;           /* read multiple request */
  tGATT_READ_PARTIAL read_blob;          /* read blob */
  tGATT_VALUE attr_value;                /* write request */
                                         /* prepare write */
  /* write blob */
  uint16_t handle; /* read,  handle value confirmation */
  uint16_t mtu;
  tGATT_EXEC_FLAG exec_write; /* execute write */
} tGATT_CL_MSG;

/* error response strucutre */
typedef struct {
  uint16_t handle;
  uint8_t cmd_code;
  uint8_t reason;
} tGATT_ERROR;

/* server response message to ATT protocol
*/
typedef union {
  /* data type            member          event   */
  tGATT_VALUE attr_value; /* READ, HANDLE_VALUE_IND, PREPARE_WRITE */
                          /* READ_BLOB, READ_BY_TYPE */
  tGATT_ERROR error;      /* ERROR_RSP */
  uint16_t handle;        /* WRITE, WRITE_BLOB */
  uint16_t mtu;           /* exchange MTU request */
} tGATT_SR_MSG;

/* Characteristic declaration attribute value
*/
typedef struct {
  tGATT_CHAR_PROP property;
  uint16_t char_val_handle;
} tGATT_CHAR_DECL;

/* attribute value maintained in the server database
*/
typedef union {
  bluetooth::Uuid uuid;        /* service declaration */
  tGATT_CHAR_DECL char_decl;   /* characteristic declaration */
  tGATT_INCL_SRVC incl_handle; /* included service */
} tGATT_ATTR_VALUE;

/* Attribute UUID type
*/
#define GATT_ATTR_UUID_TYPE_16 0
#define GATT_ATTR_UUID_TYPE_128 1
#define GATT_ATTR_UUID_TYPE_32 2
typedef uint8_t tGATT_ATTR_UUID_TYPE;

/* 16 bits UUID Attribute in server database
*/
typedef struct {
  std::unique_ptr<tGATT_ATTR_VALUE> p_value;
  tGATT_PERM permission;
  uint16_t handle;
  bluetooth::Uuid uuid;
  bt_gatt_db_attribute_type_t gatt_type;
} tGATT_ATTR;

/* Service Database definition
*/
typedef struct {
  std::vector<tGATT_ATTR> attr_list; /* pointer to the attributes */
  uint16_t end_handle;       /* Last handle number           */
  uint16_t next_handle;      /* Next usable handle value     */
} tGATT_SVC_DB;

/* Data Structure used for GATT server */
/* An GATT registration record consists of a handle, and 1 or more attributes */
/* A service registration information record consists of beginning and ending */
/* attribute handle, service UUID and a set of GATT server callback.          */

typedef struct {
  bluetooth::Uuid app_uuid128;
  tGATT_CBACK app_cb;
  tGATT_IF gatt_if; /* one based */
  bool in_use;
  uint8_t listening; /* if adv for all has been enabled */
} tGATT_REG;

struct tGATT_CLCB;

/* command queue for each connection */
typedef struct {
  BT_HDR* p_cmd;
  tGATT_CLCB* p_clcb;
  uint8_t op_code;
  bool to_send;
} tGATT_CMD_Q;

#if GATT_MAX_SR_PROFILES <= 8
typedef uint8_t tGATT_APP_MASK;
#elif GATT_MAX_SR_PROFILES <= 16
typedef uint16_t tGATT_APP_MASK;
#elif GATT_MAX_SR_PROFILES <= 32
typedef uint32_t tGATT_APP_MASK;
#endif

/* command details for each connection */
typedef struct {
  BT_HDR* p_rsp_msg;
  uint32_t trans_id;
  tGATT_READ_MULTI multi_req;
  fixed_queue_t* multi_rsp_q;
  uint16_t handle;
  uint8_t op_code;
  uint8_t status;
  uint8_t cback_cnt[GATT_MAX_APPS];
} tGATT_SR_CMD;

#define GATT_CH_CLOSE 0
#define GATT_CH_CLOSING 1
#define GATT_CH_CONN 2
#define GATT_CH_CFG 3
#define GATT_CH_OPEN 4

typedef uint8_t tGATT_CH_STATE;

#define GATT_GATT_START_HANDLE 1
#define GATT_GAP_START_HANDLE 20
#define GATT_APP_START_HANDLE 40

typedef struct hdl_cfg {
  uint16_t gatt_start_hdl;
  uint16_t gap_start_hdl;
  uint16_t app_start_hdl;
} tGATT_HDL_CFG;

typedef struct hdl_list_elem {
  tGATTS_HNDL_RANGE asgn_range; /* assigned handle range */
  tGATT_SVC_DB svc_db;
} tGATT_HDL_LIST_ELEM;

/* Data Structure used for GATT server                                        */
/* A GATT registration record consists of a handle, and 1 or more attributes  */
/* A service registration information record consists of beginning and ending */
/* attribute handle, service UUID and a set of GATT server callback.          */
typedef struct {
  tGATT_SVC_DB* p_db;  /* pointer to the service database */
  bluetooth::Uuid app_uuid; /* application UUID */
  uint32_t sdp_handle; /* primamry service SDP handle */
  uint16_t type;       /* service type UUID, primary or secondary */
  uint16_t s_hdl;      /* service starting handle */
  uint16_t e_hdl;      /* service ending handle */
  tGATT_IF gatt_if;    /* this service is belong to which application */
  bool is_primary;
} tGATT_SRV_LIST_ELEM;

typedef struct {
  std::queue<tGATT_CLCB*> pending_enc_clcb; /* pending encryption channel q */
  tGATT_SEC_ACTION sec_act;
  RawAddress peer_bda;
  tBT_TRANSPORT transport;
  uint32_t trans_id;

  uint16_t att_lcid; /* L2CAP channel ID for ATT */
  uint16_t payload_size;

  tGATT_CH_STATE ch_state;
  uint8_t ch_flags;

  std::unordered_set<uint8_t> app_hold_link;

  /* server needs */
  /* server response data */
  tGATT_SR_CMD sr_cmd;
  uint16_t indicate_handle;
  fixed_queue_t* pending_ind_q;

  alarm_t* conf_timer; /* peer confirm to indication timer */

  uint8_t prep_cnt[GATT_MAX_APPS];
  uint8_t ind_count;

  std::queue<tGATT_CMD_Q> cl_cmd_q;
  alarm_t* ind_ack_timer; /* local app confirm to indication timer */

  bool in_use;
  uint8_t tcb_idx;
} tGATT_TCB;

/* logic channel */
typedef struct {
  uint16_t
      next_disc_start_hdl; /* starting handle for the next inc srvv discovery */
  tGATT_DISC_RES result;
  bool wait_for_read_rsp;
} tGATT_READ_INC_UUID128;
struct tGATT_CLCB {
  tGATT_TCB* p_tcb; /* associated TCB of this CLCB */
  tGATT_REG* p_reg; /* owner of this CLCB */
  uint8_t sccb_idx;
  uint8_t* p_attr_buf; /* attribute buffer for read multiple, prepare write */
  bluetooth::Uuid uuid;
  uint16_t conn_id; /* connection handle */
  uint16_t s_handle; /* starting handle of the active request */
  uint16_t e_handle; /* ending handle of the active request */
  uint16_t counter; /* used as offset, attribute length, num of prepare write */
  uint16_t start_offset;
  tGATT_AUTH_REQ auth_req; /* authentication requirement */
  uint8_t operation;       /* one logic channel can have one operation active */
  uint8_t op_subtype;      /* operation subtype */
  uint8_t status;          /* operation status */
  bool first_read_blob_after_read;
  tGATT_READ_INC_UUID128 read_uuid128;
  bool in_use;
  alarm_t* gatt_rsp_timer_ent; /* peer response timer */
  uint8_t retry_count;
};

typedef struct {
  uint16_t handle;
  uint16_t uuid;
  uint32_t service_change;
} tGATT_SVC_CHG;

typedef struct {
  std::unordered_set<tGATT_IF> gatt_if;
  RawAddress remote_bda;
} tGATT_BG_CONN_DEV;

#define GATT_SVC_CHANGED_CONNECTING 1     /* wait for connection */
#define GATT_SVC_CHANGED_SERVICE 2        /* GATT service discovery */
#define GATT_SVC_CHANGED_CHARACTERISTIC 3 /* service change char discovery */
#define GATT_SVC_CHANGED_DESCRIPTOR 4     /* service change CCC discoery */
#define GATT_SVC_CHANGED_CONFIGURE_CCCD 5 /* config CCC */

typedef struct {
  uint16_t conn_id;
  bool in_use;
  bool connected;
  RawAddress bda;
  tBT_TRANSPORT transport;

  /* GATT service change CCC related variables */
  uint8_t ccc_stage;
  uint8_t ccc_result;
  uint16_t s_handle;
  uint16_t e_handle;
} tGATT_PROFILE_CLCB;

typedef struct {
  tGATT_TCB tcb[GATT_MAX_PHY_CHANNEL];
  fixed_queue_t* sign_op_queue;

  uint16_t next_handle;     /* next available handle */
  uint16_t last_service_handle; /* handle of last service */
  tGATT_SVC_CHG gattp_attr; /* GATT profile attribute service change */
  tGATT_IF gatt_if;
  std::list<tGATT_HDL_LIST_ELEM>* hdl_list_info;
  std::list<tGATT_SRV_LIST_ELEM>* srv_list_info;

  fixed_queue_t* srv_chg_clt_q; /* service change clients queue */
  tGATT_REG cl_rcb[GATT_MAX_APPS];
  tGATT_CLCB clcb[GATT_CL_MAX_LCB]; /* connection link control block*/
  uint16_t def_mtu_size;

#if (GATT_CONFORMANCE_TESTING == TRUE)
  bool enable_err_rsp;
  uint8_t req_op_code;
  uint8_t err_status;
  uint16_t handle;
#endif

  tGATT_PROFILE_CLCB profile_clcb[GATT_MAX_APPS];
  uint16_t
      handle_of_h_r; /* Handle of the handles reused characteristic value */

  tGATT_APPL_INFO cb_info;

  tGATT_HDL_CFG hdl_cfg;
  std::list<tGATT_BG_CONN_DEV> bgconn_dev;
} tGATT_CB;

#define GATT_SIZE_OF_SRV_CHG_HNDL_RANGE 4

/* Global GATT data */
extern tGATT_CB gatt_cb;

#if (GATT_CONFORMANCE_TESTING == TRUE)
extern void gatt_set_err_rsp(bool enable, uint8_t req_op_code,
                             uint8_t err_status);
#endif

/* from gatt_main.cc */
extern bool gatt_disconnect(tGATT_TCB* p_tcb);
extern bool gatt_act_connect(tGATT_REG* p_reg, const RawAddress& bd_addr,
                             tBT_TRANSPORT transport, bool opportunistic,
                             int8_t initiating_phys);
extern bool gatt_connect(const RawAddress& rem_bda, tGATT_TCB* p_tcb,
                         tBT_TRANSPORT transport, uint8_t initiating_phys);
extern void gatt_data_process(tGATT_TCB& p_tcb, BT_HDR* p_buf);
extern void gatt_update_app_use_link_flag(tGATT_IF gatt_if, tGATT_TCB* p_tcb,
                                          bool is_add, bool check_acl_link);

extern void gatt_profile_db_init(void);
extern void gatt_set_ch_state(tGATT_TCB* p_tcb, tGATT_CH_STATE ch_state);
extern tGATT_CH_STATE gatt_get_ch_state(tGATT_TCB* p_tcb);
extern void gatt_init_srv_chg(void);
extern void gatt_proc_srv_chg(void);
extern void gatt_send_srv_chg_ind(const RawAddress& peer_bda);
extern void gatt_chk_srv_chg(tGATTS_SRV_CHG* p_srv_chg_clt);
extern void gatt_add_a_bonded_dev_for_srv_chg(const RawAddress& bda);

/* from gatt_attr.cc */
extern uint16_t gatt_profile_find_conn_id_by_bd_addr(const RawAddress& bda);

/* Functions provided by att_protocol.cc */
extern tGATT_STATUS attp_send_cl_msg(tGATT_TCB& tcb, tGATT_CLCB* p_clcb,
                                     uint8_t op_code, tGATT_CL_MSG* p_msg);
extern BT_HDR* attp_build_sr_msg(tGATT_TCB& tcb, uint8_t op_code,
                                 tGATT_SR_MSG* p_msg);
extern tGATT_STATUS attp_send_sr_msg(tGATT_TCB& tcb, BT_HDR* p_msg);
extern tGATT_STATUS attp_send_msg_to_l2cap(tGATT_TCB& tcb, BT_HDR* p_toL2CAP);

/* utility functions */
extern uint8_t* gatt_dbg_op_name(uint8_t op_code);
extern uint32_t gatt_add_sdp_record(const bluetooth::Uuid& uuid,
                                    uint16_t start_hdl, uint16_t end_hdl);
extern bool gatt_parse_uuid_from_cmd(bluetooth::Uuid* p_uuid, uint16_t len,
                                     uint8_t** p_data);
extern uint8_t gatt_build_uuid_to_stream_len(const bluetooth::Uuid& uuid);
extern uint8_t gatt_build_uuid_to_stream(uint8_t** p_dst,
                                         const bluetooth::Uuid& uuid);
extern void gatt_sr_get_sec_info(const RawAddress& rem_bda,
                                 tBT_TRANSPORT transport, uint8_t* p_sec_flag,
                                 uint8_t* p_key_size);
extern void gatt_start_rsp_timer(tGATT_CLCB* p_clcb);
extern void gatt_start_conf_timer(tGATT_TCB* p_tcb);
extern void gatt_rsp_timeout(void* data);
extern void gatt_indication_confirmation_timeout(void* data);
extern void gatt_ind_ack_timeout(void* data);
extern void gatt_start_ind_ack_timer(tGATT_TCB& tcb);
extern tGATT_STATUS gatt_send_error_rsp(tGATT_TCB& tcb, uint8_t err_code,
                                        uint8_t op_code, uint16_t handle,
                                        bool deq);

extern bool gatt_is_srv_chg_ind_pending(tGATT_TCB* p_tcb);
extern tGATTS_SRV_CHG* gatt_is_bda_in_the_srv_chg_clt_list(
    const RawAddress& bda);

extern bool gatt_find_the_connected_bda(uint8_t start_idx, RawAddress& bda,
                                        uint8_t* p_found_idx,
                                        tBT_TRANSPORT* p_transport);
extern void gatt_set_srv_chg(void);
extern void gatt_delete_dev_from_srv_chg_clt_list(const RawAddress& bd_addr);
extern tGATT_VALUE* gatt_add_pending_ind(tGATT_TCB* p_tcb, tGATT_VALUE* p_ind);
extern void gatt_free_srvc_db_buffer_app_id(const bluetooth::Uuid& app_id);
extern bool gatt_cl_send_next_cmd_inq(tGATT_TCB& tcb);

/* reserved handle list */
extern std::list<tGATT_HDL_LIST_ELEM>::iterator gatt_find_hdl_buffer_by_app_id(
    const bluetooth::Uuid& app_uuid128, bluetooth::Uuid* p_svc_uuid,
    uint16_t svc_inst);
extern tGATT_HDL_LIST_ELEM* gatt_find_hdl_buffer_by_handle(uint16_t handle);
extern tGATTS_SRV_CHG* gatt_add_srv_chg_clt(tGATTS_SRV_CHG* p_srv_chg);

/* for background connection */
extern bool gatt_update_auto_connect_dev(tGATT_IF gatt_if, bool add,
                                         const RawAddress& bd_addr);
extern bool gatt_is_bg_dev_for_app(tGATT_BG_CONN_DEV* p_dev, tGATT_IF gatt_if);
extern bool gatt_remove_bg_dev_for_app(tGATT_IF gatt_if,
                                       const RawAddress& bd_addr);
extern uint8_t gatt_clear_bg_dev_for_addr(const RawAddress& bd_addr);
extern tGATT_BG_CONN_DEV* gatt_find_bg_dev(const RawAddress& remote_bda);
extern void gatt_deregister_bgdev_list(tGATT_IF gatt_if);

/* server function */
extern std::list<tGATT_SRV_LIST_ELEM>::iterator gatt_sr_find_i_rcb_by_handle(
    uint16_t handle);
extern tGATT_STATUS gatt_sr_process_app_rsp(tGATT_TCB& tcb, tGATT_IF gatt_if,
                                            uint32_t trans_id, uint8_t op_code,
                                            tGATT_STATUS status,
                                            tGATTS_RSP* p_msg);
extern void gatt_server_handle_client_req(tGATT_TCB& p_tcb, uint8_t op_code,
                                          uint16_t len, uint8_t* p_data);
extern void gatt_sr_send_req_callback(uint16_t conn_id, uint32_t trans_id,
                                      uint8_t op_code, tGATTS_DATA* p_req_data);
extern uint32_t gatt_sr_enqueue_cmd(tGATT_TCB& tcb, uint8_t op_code,
                                    uint16_t handle);
extern bool gatt_cancel_open(tGATT_IF gatt_if, const RawAddress& bda);
extern void gatt_notify_phy_updated(uint8_t status, uint16_t handle,
                                    uint8_t tx_phy, uint8_t rx_phy);
/*   */

extern tGATT_REG* gatt_get_regcb(tGATT_IF gatt_if);
extern bool gatt_is_clcb_allocated(uint16_t conn_id);
extern tGATT_CLCB* gatt_clcb_alloc(uint16_t conn_id);
extern void gatt_clcb_dealloc(tGATT_CLCB* p_clcb);

extern void gatt_sr_copy_prep_cnt_to_cback_cnt(tGATT_TCB& p_tcb);
extern bool gatt_sr_is_cback_cnt_zero(tGATT_TCB& p_tcb);
extern bool gatt_sr_is_prep_cnt_zero(tGATT_TCB& p_tcb);
extern void gatt_sr_reset_cback_cnt(tGATT_TCB& p_tcb);
extern void gatt_sr_reset_prep_cnt(tGATT_TCB& tcb);
extern void gatt_sr_update_cback_cnt(tGATT_TCB& p_tcb, tGATT_IF gatt_if,
                                     bool is_inc, bool is_reset_first);
extern void gatt_sr_update_prep_cnt(tGATT_TCB& tcb, tGATT_IF gatt_if,
                                    bool is_inc, bool is_reset_first);

extern uint8_t gatt_num_clcb_by_bd_addr(const RawAddress& bda);
extern tGATT_TCB* gatt_find_tcb_by_cid(uint16_t lcid);
extern tGATT_TCB* gatt_allocate_tcb_by_bdaddr(const RawAddress& bda,
                                              tBT_TRANSPORT transport);
extern tGATT_TCB* gatt_get_tcb_by_idx(uint8_t tcb_idx);
extern tGATT_TCB* gatt_find_tcb_by_addr(const RawAddress& bda,
                                        tBT_TRANSPORT transport);
extern bool gatt_send_ble_burst_data(const RawAddress& remote_bda,
                                     BT_HDR* p_buf);

/* GATT client functions */
extern void gatt_dequeue_sr_cmd(tGATT_TCB& tcb);
extern uint8_t gatt_send_write_msg(tGATT_TCB& p_tcb, tGATT_CLCB* p_clcb,
                                   uint8_t op_code, uint16_t handle,
                                   uint16_t len, uint16_t offset,
                                   uint8_t* p_data);
extern void gatt_cleanup_upon_disc(const RawAddress& bda, uint16_t reason,
                                   tBT_TRANSPORT transport);
extern void gatt_end_operation(tGATT_CLCB* p_clcb, tGATT_STATUS status,
                               void* p_data);

extern void gatt_act_discovery(tGATT_CLCB* p_clcb);
extern void gatt_act_read(tGATT_CLCB* p_clcb, uint16_t offset);
extern void gatt_act_write(tGATT_CLCB* p_clcb, uint8_t sec_act);
extern tGATT_CLCB* gatt_cmd_dequeue(tGATT_TCB& tcb, uint8_t* p_opcode);
extern void gatt_cmd_enq(tGATT_TCB& tcb, tGATT_CLCB* p_clcb, bool to_send,
                         uint8_t op_code, BT_HDR* p_buf);
extern void gatt_client_handle_server_rsp(tGATT_TCB& tcb, uint8_t op_code,
                                          uint16_t len, uint8_t* p_data);
extern void gatt_send_queue_write_cancel(tGATT_TCB& tcb, tGATT_CLCB* p_clcb,
                                         tGATT_EXEC_FLAG flag);

/* gatt_auth.cc */
extern void gatt_security_check_start(tGATT_CLCB* p_clcb);
extern void gatt_verify_signature(tGATT_TCB& tcb, BT_HDR* p_buf);
extern tGATT_STATUS gatt_get_link_encrypt_status(tGATT_TCB& tcb);
extern tGATT_SEC_ACTION gatt_get_sec_act(tGATT_TCB* p_tcb);
extern void gatt_set_sec_act(tGATT_TCB* p_tcb, tGATT_SEC_ACTION sec_act);

/* gatt_db.cc */
extern void gatts_init_service_db(tGATT_SVC_DB& db,
                                  const bluetooth::Uuid& service, bool is_pri,
                                  uint16_t s_hdl, uint16_t num_handle);
extern uint16_t gatts_add_included_service(tGATT_SVC_DB& db, uint16_t s_handle,
                                           uint16_t e_handle,
                                           const bluetooth::Uuid& service);
extern uint16_t gatts_add_characteristic(tGATT_SVC_DB& db, tGATT_PERM perm,
                                         tGATT_CHAR_PROP property,
                                         const bluetooth::Uuid& char_uuid);
extern uint16_t gatts_add_char_descr(tGATT_SVC_DB& db, tGATT_PERM perm,
                                     const bluetooth::Uuid& dscp_uuid);
extern tGATT_STATUS gatts_db_read_attr_value_by_type(
    tGATT_TCB& tcb, tGATT_SVC_DB* p_db, uint8_t op_code, BT_HDR* p_rsp,
    uint16_t s_handle, uint16_t e_handle, const bluetooth::Uuid& type,
    uint16_t* p_len, tGATT_SEC_FLAG sec_flag, uint8_t key_size,
    uint32_t trans_id, uint16_t* p_cur_handle);
extern tGATT_STATUS gatts_read_attr_value_by_handle(
    tGATT_TCB& tcb, tGATT_SVC_DB* p_db, uint8_t op_code, uint16_t handle,
    uint16_t offset, uint8_t* p_value, uint16_t* p_len, uint16_t mtu,
    tGATT_SEC_FLAG sec_flag, uint8_t key_size, uint32_t trans_id);
extern tGATT_STATUS gatts_write_attr_perm_check(
    tGATT_SVC_DB* p_db, uint8_t op_code, uint16_t handle, uint16_t offset,
    uint8_t* p_data, uint16_t len, tGATT_SEC_FLAG sec_flag, uint8_t key_size);
extern tGATT_STATUS gatts_read_attr_perm_check(tGATT_SVC_DB* p_db, bool is_long,
                                               uint16_t handle,
                                               tGATT_SEC_FLAG sec_flag,
                                               uint8_t key_size);
extern bluetooth::Uuid* gatts_get_service_uuid(tGATT_SVC_DB* p_db);

#endif
