/******************************************************************************
 *
 *  Copyright 2008-2012 Broadcom Corporation
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

/******************************************************************************
 *
 *  this file contains the main ATT functions
 *
 ******************************************************************************/

#include "bt_target.h"

#include "bt_common.h"
#include "bt_utils.h"
#include "btif_storage.h"
#include "btm_ble_int.h"
#include "btm_int.h"
#include "device/include/interop.h"
#include "gatt_int.h"
#include "l2c_api.h"
#include "osi/include/osi.h"

using base::StringPrintf;

/* Configuration flags. */
#define GATT_L2C_CFG_IND_DONE (1 << 0)
#define GATT_L2C_CFG_CFM_DONE (1 << 1)

/* minimum GATT MTU size over BR/EDR link
 */
#define GATT_MIN_BR_MTU_SIZE 48

/******************************************************************************/
/*            L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/******************************************************************************/
static void gatt_le_connect_cback(uint16_t chan, const RawAddress& bd_addr,
                                  bool connected, uint16_t reason,
                                  tBT_TRANSPORT transport);
static void gatt_le_data_ind(uint16_t chan, const RawAddress& bd_addr,
                             BT_HDR* p_buf);
static void gatt_le_cong_cback(const RawAddress& remote_bda, bool congest);

static void gatt_l2cif_connect_ind_cback(const RawAddress& bd_addr,
                                         uint16_t l2cap_cid, uint16_t psm,
                                         uint8_t l2cap_id);
static void gatt_l2cif_connect_cfm_cback(uint16_t l2cap_cid, uint16_t result);
static void gatt_l2cif_config_ind_cback(uint16_t l2cap_cid,
                                        tL2CAP_CFG_INFO* p_cfg);
static void gatt_l2cif_config_cfm_cback(uint16_t l2cap_cid,
                                        tL2CAP_CFG_INFO* p_cfg);
static void gatt_l2cif_disconnect_ind_cback(uint16_t l2cap_cid,
                                            bool ack_needed);
static void gatt_l2cif_disconnect_cfm_cback(uint16_t l2cap_cid,
                                            uint16_t result);
static void gatt_l2cif_data_ind_cback(uint16_t l2cap_cid, BT_HDR* p_msg);
static void gatt_send_conn_cback(tGATT_TCB* p_tcb);
static void gatt_l2cif_congest_cback(uint16_t cid, bool congested);

static const tL2CAP_APPL_INFO dyn_info = {gatt_l2cif_connect_ind_cback,
                                          gatt_l2cif_connect_cfm_cback,
                                          NULL,
                                          gatt_l2cif_config_ind_cback,
                                          gatt_l2cif_config_cfm_cback,
                                          gatt_l2cif_disconnect_ind_cback,
                                          gatt_l2cif_disconnect_cfm_cback,
                                          NULL,
                                          gatt_l2cif_data_ind_cback,
                                          gatt_l2cif_congest_cback,
                                          NULL,
                                          NULL /* tL2CA_CREDITS_RECEIVED_CB */};

tGATT_CB gatt_cb;

/*******************************************************************************
 *
 * Function         gatt_init
 *
 * Description      This function is enable the GATT profile on the device.
 *                  It clears out the control blocks, and registers with L2CAP.
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_init(void) {
  tL2CAP_FIXED_CHNL_REG fixed_reg;

  VLOG(1) << __func__;

  gatt_cb = tGATT_CB();
  memset(&fixed_reg, 0, sizeof(tL2CAP_FIXED_CHNL_REG));

  gatt_cb.def_mtu_size = GATT_DEF_BLE_MTU_SIZE;
  gatt_cb.sign_op_queue = fixed_queue_new(SIZE_MAX);
  gatt_cb.srv_chg_clt_q = fixed_queue_new(SIZE_MAX);
  /* First, register fixed L2CAP channel for ATT over BLE */
  fixed_reg.fixed_chnl_opts.mode = L2CAP_FCR_BASIC_MODE;
  fixed_reg.fixed_chnl_opts.max_transmit = 0xFF;
  fixed_reg.fixed_chnl_opts.rtrans_tout = 2000;
  fixed_reg.fixed_chnl_opts.mon_tout = 12000;
  fixed_reg.fixed_chnl_opts.mps = 670;
  fixed_reg.fixed_chnl_opts.tx_win_sz = 1;

  fixed_reg.pL2CA_FixedConn_Cb = gatt_le_connect_cback;
  fixed_reg.pL2CA_FixedData_Cb = gatt_le_data_ind;
  fixed_reg.pL2CA_FixedCong_Cb = gatt_le_cong_cback; /* congestion callback */
  fixed_reg.default_idle_tout = 0xffff; /* 0xffff default idle timeout */

  L2CA_RegisterFixedChannel(L2CAP_ATT_CID, &fixed_reg);

  /* Now, register with L2CAP for ATT PSM over BR/EDR */
  if (!L2CA_Register(BT_PSM_ATT, (tL2CAP_APPL_INFO*)&dyn_info)) {
    LOG(ERROR) << "ATT Dynamic Registration failed";
  }

  BTM_SetSecurityLevel(true, "", BTM_SEC_SERVICE_ATT, BTM_SEC_NONE, BT_PSM_ATT,
                       0, 0);
  BTM_SetSecurityLevel(false, "", BTM_SEC_SERVICE_ATT, BTM_SEC_NONE, BT_PSM_ATT,
                       0, 0);

  gatt_cb.hdl_cfg.gatt_start_hdl = GATT_GATT_START_HANDLE;
  gatt_cb.hdl_cfg.gap_start_hdl = GATT_GAP_START_HANDLE;
  gatt_cb.hdl_cfg.app_start_hdl = GATT_APP_START_HANDLE;

  gatt_cb.hdl_list_info = new std::list<tGATT_HDL_LIST_ELEM>();
  gatt_cb.srv_list_info = new std::list<tGATT_SRV_LIST_ELEM>();
  gatt_profile_db_init();
}

/*******************************************************************************
 *
 * Function         gatt_free
 *
 * Description      This function frees resources used by the GATT profile.
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_free(void) {
  int i;
  VLOG(1) << __func__;

  fixed_queue_free(gatt_cb.sign_op_queue, NULL);
  gatt_cb.sign_op_queue = NULL;
  fixed_queue_free(gatt_cb.srv_chg_clt_q, NULL);
  gatt_cb.srv_chg_clt_q = NULL;
  for (i = 0; i < GATT_MAX_PHY_CHANNEL; i++) {
    gatt_cb.tcb[i].pending_enc_clcb = std::queue<tGATT_CLCB*>();

    fixed_queue_free(gatt_cb.tcb[i].pending_ind_q, NULL);
    gatt_cb.tcb[i].pending_ind_q = NULL;

    alarm_free(gatt_cb.tcb[i].conf_timer);
    gatt_cb.tcb[i].conf_timer = NULL;

    alarm_free(gatt_cb.tcb[i].ind_ack_timer);
    gatt_cb.tcb[i].ind_ack_timer = NULL;

    fixed_queue_free(gatt_cb.tcb[i].sr_cmd.multi_rsp_q, NULL);
    gatt_cb.tcb[i].sr_cmd.multi_rsp_q = NULL;
  }

  gatt_cb.hdl_list_info->clear();
  gatt_cb.hdl_list_info = nullptr;
  gatt_cb.srv_list_info->clear();
  gatt_cb.srv_list_info = nullptr;
}

/*******************************************************************************
 *
 * Function         gatt_connect
 *
 * Description      This function is called to initiate a connection to a peer
 *                  device.
 *
 * Parameter        rem_bda: remote device address to connect to.
 *
 * Returns          true if connection is started, otherwise return false.
 *
 ******************************************************************************/
bool gatt_connect(const RawAddress& rem_bda, tGATT_TCB* p_tcb,
                  tBT_TRANSPORT transport, uint8_t initiating_phys) {
  bool gatt_ret = false;

  if (gatt_get_ch_state(p_tcb) != GATT_CH_OPEN)
    gatt_set_ch_state(p_tcb, GATT_CH_CONN);

  if (transport == BT_TRANSPORT_LE) {
    p_tcb->att_lcid = L2CAP_ATT_CID;
    gatt_ret = L2CA_ConnectFixedChnl(L2CAP_ATT_CID, rem_bda, initiating_phys);
  } else {
    p_tcb->att_lcid = L2CA_ConnectReq(BT_PSM_ATT, rem_bda);
    if (p_tcb->att_lcid != 0) gatt_ret = true;
  }

  return gatt_ret;
}

/*******************************************************************************
 *
 * Function         gatt_disconnect
 *
 * Description      This function is called to disconnect to an ATT device.
 *
 * Parameter        p_tcb: pointer to the TCB to disconnect.
 *
 * Returns          true: if connection found and to be disconnected; otherwise
 *                  return false.
 *
 ******************************************************************************/
bool gatt_disconnect(tGATT_TCB* p_tcb) {
  bool ret = false;
  tGATT_CH_STATE ch_state;

  VLOG(1) << __func__;

  if (p_tcb != NULL) {
    ret = true;
    ch_state = gatt_get_ch_state(p_tcb);
    if (ch_state != GATT_CH_CLOSING) {
      if (p_tcb->att_lcid == L2CAP_ATT_CID) {
        if (ch_state == GATT_CH_OPEN) {
          /* only LCB exist between remote device and local */
          ret = L2CA_RemoveFixedChnl(L2CAP_ATT_CID, p_tcb->peer_bda);
        } else {
          ret = L2CA_CancelBleConnectReq(p_tcb->peer_bda);
          if (!ret) gatt_set_ch_state(p_tcb, GATT_CH_CLOSE);
        }
        gatt_set_ch_state(p_tcb, GATT_CH_CLOSING);
      } else {
        if ((ch_state == GATT_CH_OPEN) || (ch_state == GATT_CH_CFG))
          ret = L2CA_DisconnectReq(p_tcb->att_lcid);
        else
          VLOG(1) << __func__ << " gatt_disconnect channel not opened";
      }
    } else {
      VLOG(1) << __func__ << " already in closing state";
    }
  }

  return ret;
}

/*******************************************************************************
 *
 * Function         gatt_update_app_hold_link_status
 *
 * Description      Update the application use link status
 *
 * Returns          true if any modifications are made or
 *                  when it already exists, false otherwise.
 *
 ******************************************************************************/
bool gatt_update_app_hold_link_status(tGATT_IF gatt_if, tGATT_TCB* p_tcb,
                                      bool is_add) {
  auto& holders = p_tcb->app_hold_link;

  VLOG(1) << __func__;
  if (is_add) {
    auto ret = holders.insert(gatt_if);
    if (ret.second) {
      VLOG(1) << "added gatt_if=" << +gatt_if;
    } else {
      VLOG(1) << "attempt to add already existing gatt_if=" << +gatt_if;
    }
    return true;
  }

  //! is_add
  if (!holders.erase(gatt_if)) {
    VLOG(1) << "attempt to remove nonexisting gatt_if=" << +gatt_if;
    return false;
  }

  VLOG(1) << "removed gatt_if=" << +gatt_if;
  return true;
}

/*******************************************************************************
 *
 * Function         gatt_update_app_use_link_flag
 *
 * Description      Update the application use link flag and optional to check
 *                  the acl link if the link is up then set the idle time out
 *                  accordingly
 *
 * Returns          void.
 *
 ******************************************************************************/
void gatt_update_app_use_link_flag(tGATT_IF gatt_if, tGATT_TCB* p_tcb,
                                   bool is_add, bool check_acl_link) {
  VLOG(1) << StringPrintf("%s: is_add=%d chk_link=%d", __func__, is_add,
                          check_acl_link);

  if (!p_tcb) return;

  // If we make no modification, i.e. kill app that was never connected to a
  // device, skip updating the device state.
  if (!gatt_update_app_hold_link_status(gatt_if, p_tcb, is_add)) return;

  if (!check_acl_link ||
      p_tcb->att_lcid !=
          L2CAP_ATT_CID || /* only update link idle timer for fixed channel */
      (BTM_GetHCIConnHandle(p_tcb->peer_bda, p_tcb->transport) ==
       GATT_INVALID_ACL_HANDLE)) {
    return;
  }

  if (is_add) {
    VLOG(1) << "disable link idle timer";
    /* acl link is connected disable the idle timeout */
    GATT_SetIdleTimeout(p_tcb->peer_bda, GATT_LINK_NO_IDLE_TIMEOUT,
                        p_tcb->transport);
  } else {
    if (p_tcb->app_hold_link.empty()) {
      /* acl link is connected but no application needs to use the link
         so set the timeout value to GATT_LINK_IDLE_TIMEOUT_WHEN_NO_APP seconds
         */
      VLOG(1) << " start link idle timer = "
              << GATT_LINK_IDLE_TIMEOUT_WHEN_NO_APP << " sec";
      GATT_SetIdleTimeout(p_tcb->peer_bda, GATT_LINK_IDLE_TIMEOUT_WHEN_NO_APP,
                          p_tcb->transport);
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_act_connect
 *
 * Description      GATT connection initiation.
 *
 * Returns          void.
 *
 ******************************************************************************/
bool gatt_act_connect(tGATT_REG* p_reg, const RawAddress& bd_addr,
                      tBT_TRANSPORT transport, bool opportunistic,
                      int8_t initiating_phys) {
  bool ret = false;
  tGATT_TCB* p_tcb;
  uint8_t st;

  p_tcb = gatt_find_tcb_by_addr(bd_addr, transport);
  if (p_tcb != NULL) {
    ret = true;
    st = gatt_get_ch_state(p_tcb);

    /* before link down, another app try to open a GATT connection */
    if (st == GATT_CH_OPEN && p_tcb->app_hold_link.empty() &&
        transport == BT_TRANSPORT_LE) {
      if (!gatt_connect(bd_addr, p_tcb, transport, initiating_phys))
        ret = false;
    } else if (st == GATT_CH_CLOSING) {
      /* need to complete the closing first */
      ret = false;
    }
  } else {
    p_tcb = gatt_allocate_tcb_by_bdaddr(bd_addr, transport);
    if (p_tcb != NULL) {
      if (!gatt_connect(bd_addr, p_tcb, transport, initiating_phys)) {
        LOG(ERROR) << "gatt_connect failed";
        fixed_queue_free(p_tcb->pending_ind_q, NULL);
        *p_tcb = tGATT_TCB();
      } else
        ret = true;
    } else {
      ret = 0;
      LOG(ERROR) << "Max TCB for gatt_if [ " << +p_reg->gatt_if << "] reached.";
    }
  }

  if (ret) {
    if (!opportunistic)
      gatt_update_app_use_link_flag(p_reg->gatt_if, p_tcb, true, false);
    else
      VLOG(1) << __func__
              << ": connection is opportunistic, not updating app usage";
  }

  return ret;
}

/*******************************************************************************
 *
 * Function         gatt_le_connect_cback
 *
 * Description      This callback function is called by L2CAP to indicate that
 *                  the ATT fixed channel for LE is
 *                      connected (conn = true)/disconnected (conn = false).
 *
 ******************************************************************************/
static void gatt_le_connect_cback(uint16_t chan, const RawAddress& bd_addr,
                                  bool connected, uint16_t reason,
                                  tBT_TRANSPORT transport) {
  tGATT_TCB* p_tcb = gatt_find_tcb_by_addr(bd_addr, transport);
  bool check_srv_chg = false;
  tGATTS_SRV_CHG* p_srv_chg_clt = NULL;

  /* ignore all fixed channel connect/disconnect on BR/EDR link for GATT */
  if (transport == BT_TRANSPORT_BR_EDR) return;

  VLOG(1) << "GATT   ATT protocol channel with BDA: " << bd_addr << " is "
          << ((connected) ? "connected" : "disconnected");

  p_srv_chg_clt = gatt_is_bda_in_the_srv_chg_clt_list(bd_addr);
  if (p_srv_chg_clt != NULL) {
    check_srv_chg = true;
  } else {
    if (btm_sec_is_a_bonded_dev(bd_addr))
      gatt_add_a_bonded_dev_for_srv_chg(bd_addr);
  }

  if (connected) {
    /* do we have a channel initiating a connection? */
    if (p_tcb) {
      /* we are initiating connection */
      if (gatt_get_ch_state(p_tcb) == GATT_CH_CONN) {
        /* send callback */
        gatt_set_ch_state(p_tcb, GATT_CH_OPEN);
        p_tcb->payload_size = GATT_DEF_BLE_MTU_SIZE;

        gatt_send_conn_cback(p_tcb);
      }
      if (check_srv_chg) gatt_chk_srv_chg(p_srv_chg_clt);
    }
    /* this is incoming connection or background connection callback */

    else {
      p_tcb = gatt_allocate_tcb_by_bdaddr(bd_addr, BT_TRANSPORT_LE);
      if (p_tcb != NULL) {
        p_tcb->att_lcid = L2CAP_ATT_CID;

        gatt_set_ch_state(p_tcb, GATT_CH_OPEN);

        p_tcb->payload_size = GATT_DEF_BLE_MTU_SIZE;

        gatt_send_conn_cback(p_tcb);
        if (check_srv_chg) {
          gatt_chk_srv_chg(p_srv_chg_clt);
        }
      } else {
        LOG(ERROR) << "CCB max out, no rsources";
      }
    }
  } else {
    gatt_cleanup_upon_disc(bd_addr, reason, transport);
    VLOG(1) << "ATT disconnected";
  }
}

/*******************************************************************************
 *
 * Function         gatt_channel_congestion
 *
 * Description      This function is called to process the congestion callback
 *                  from lcb
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_channel_congestion(tGATT_TCB* p_tcb, bool congested) {
  uint8_t i = 0;
  tGATT_REG* p_reg = NULL;
  uint16_t conn_id;

  /* if uncongested, check to see if there is any more pending data */
  if (p_tcb != NULL && !congested) {
    gatt_cl_send_next_cmd_inq(*p_tcb);
  }
  /* notifying all applications for the connection up event */
  for (i = 0, p_reg = gatt_cb.cl_rcb; i < GATT_MAX_APPS; i++, p_reg++) {
    if (p_reg->in_use) {
      if (p_reg->app_cb.p_congestion_cb) {
        conn_id = GATT_CREATE_CONN_ID(p_tcb->tcb_idx, p_reg->gatt_if);
        (*p_reg->app_cb.p_congestion_cb)(conn_id, congested);
      }
    }
  }
}

void gatt_notify_phy_updated(uint8_t status, uint16_t handle, uint8_t tx_phy,
                             uint8_t rx_phy) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);
  if (!p_dev_rec) {
    BTM_TRACE_WARNING("%s: No Device Found!", __func__);
    return;
  }

  tGATT_TCB* p_tcb =
      gatt_find_tcb_by_addr(p_dev_rec->ble.pseudo_addr, BT_TRANSPORT_LE);
  if (p_tcb == NULL) return;

  for (int i = 0; i < GATT_MAX_APPS; i++) {
    tGATT_REG* p_reg = &gatt_cb.cl_rcb[i];
    if (p_reg->in_use && p_reg->app_cb.p_phy_update_cb) {
      uint16_t conn_id = GATT_CREATE_CONN_ID(p_tcb->tcb_idx, p_reg->gatt_if);
      (*p_reg->app_cb.p_phy_update_cb)(p_reg->gatt_if, conn_id, tx_phy, rx_phy,
                                       status);
    }
  }
}

void gatt_notify_conn_update(uint16_t handle, uint16_t interval,
                             uint16_t latency, uint16_t timeout,
                             uint8_t status) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);
  if (!p_dev_rec) {
    return;
  }

  tGATT_TCB* p_tcb =
      gatt_find_tcb_by_addr(p_dev_rec->ble.pseudo_addr, BT_TRANSPORT_LE);
  if (p_tcb == NULL) return;

  for (int i = 0; i < GATT_MAX_APPS; i++) {
    tGATT_REG* p_reg = &gatt_cb.cl_rcb[i];
    if (p_reg->in_use && p_reg->app_cb.p_conn_update_cb) {
      uint16_t conn_id = GATT_CREATE_CONN_ID(p_tcb->tcb_idx, p_reg->gatt_if);
      (*p_reg->app_cb.p_conn_update_cb)(p_reg->gatt_if, conn_id, interval,
                                        latency, timeout, status);
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_le_cong_cback
 *
 * Description      This function is called when GATT fixed channel is congested
 *                  or uncongested.
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_le_cong_cback(const RawAddress& remote_bda, bool congested) {
  tGATT_TCB* p_tcb = gatt_find_tcb_by_addr(remote_bda, BT_TRANSPORT_LE);

  /* if uncongested, check to see if there is any more pending data */
  if (p_tcb != NULL) {
    gatt_channel_congestion(p_tcb, congested);
  }
}

/*******************************************************************************
 *
 * Function         gatt_le_data_ind
 *
 * Description      This function is called when data is received from L2CAP.
 *                  if we are the originator of the connection, we are the ATT
 *                  client, and the received message is queued up for the
 *                  client.
 *
 *                  If we are the destination of the connection, we are the ATT
 *                  server, so the message is passed to the server processing
 *                  function.
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_le_data_ind(uint16_t chan, const RawAddress& bd_addr,
                             BT_HDR* p_buf) {
  tGATT_TCB* p_tcb;

  /* Find CCB based on bd addr */
  if ((p_tcb = gatt_find_tcb_by_addr(bd_addr, BT_TRANSPORT_LE)) != NULL) {
    if (gatt_get_ch_state(p_tcb) < GATT_CH_OPEN) {
      LOG(WARNING) << "ATT - Ignored L2CAP data while in state: "
                   << +gatt_get_ch_state(p_tcb);
    } else
      gatt_data_process(*p_tcb, p_buf);
  }

  osi_free(p_buf);
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_connect_ind
 *
 * Description      This function handles an inbound connection indication
 *                  from L2CAP. This is the case where we are acting as a
 *                  server.
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_l2cif_connect_ind_cback(const RawAddress& bd_addr,
                                         uint16_t lcid,
                                         UNUSED_ATTR uint16_t psm, uint8_t id) {
  /* do we already have a control channel for this peer? */
  uint8_t result = L2CAP_CONN_OK;
  tL2CAP_CFG_INFO cfg;
  tGATT_TCB* p_tcb = gatt_find_tcb_by_addr(bd_addr, BT_TRANSPORT_BR_EDR);

  LOG(ERROR) << "Connection indication cid = " << +lcid;
  /* new connection ? */
  if (p_tcb == NULL) {
    /* allocate tcb */
    p_tcb = gatt_allocate_tcb_by_bdaddr(bd_addr, BT_TRANSPORT_BR_EDR);
    if (p_tcb == NULL) {
      /* no tcb available, reject L2CAP connection */
      result = L2CAP_CONN_NO_RESOURCES;
    } else
      p_tcb->att_lcid = lcid;

  } else /* existing connection , reject it */
  {
    result = L2CAP_CONN_NO_RESOURCES;
  }

  /* Send L2CAP connect rsp */
  L2CA_ConnectRsp(bd_addr, id, lcid, result, 0);

  /* if result ok, proceed with connection */
  if (result == L2CAP_CONN_OK) {
    /* transition to configuration state */
    gatt_set_ch_state(p_tcb, GATT_CH_CFG);

    /* Send L2CAP config req */
    memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
    cfg.mtu_present = true;
    cfg.mtu = GATT_MAX_MTU_SIZE;

    L2CA_ConfigReq(lcid, &cfg);
  }
}

/*******************************************************************************
 *
 * Function         gatt_l2c_connect_cfm_cback
 *
 * Description      This is the L2CAP connect confirm callback function.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_l2cif_connect_cfm_cback(uint16_t lcid, uint16_t result) {
  tGATT_TCB* p_tcb;
  tL2CAP_CFG_INFO cfg;

  /* look up clcb for this channel */
  p_tcb = gatt_find_tcb_by_cid(lcid);
  if (p_tcb != NULL) {
    VLOG(1) << __func__
            << StringPrintf(" result: %d ch_state: %d, lcid:0x%x", result,
                            gatt_get_ch_state(p_tcb), p_tcb->att_lcid);

    /* if in correct state */
    if (gatt_get_ch_state(p_tcb) == GATT_CH_CONN) {
      /* if result successful */
      if (result == L2CAP_CONN_OK) {
        /* set channel state */
        gatt_set_ch_state(p_tcb, GATT_CH_CFG);

        /* Send L2CAP config req */
        memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
        cfg.mtu_present = true;
        cfg.mtu = GATT_MAX_MTU_SIZE;
        L2CA_ConfigReq(lcid, &cfg);
      }
      /* else initiating connection failure */
      else {
        gatt_cleanup_upon_disc(p_tcb->peer_bda, result, GATT_TRANSPORT_BR_EDR);
      }
    } else /* wrong state, disconnect it */
    {
      if (result == L2CAP_CONN_OK) {
        /* just in case the peer also accepts our connection - Send L2CAP
         * disconnect req */
        L2CA_DisconnectReq(lcid);
      }
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_config_cfm_cback
 *
 * Description      This is the L2CAP config confirm callback function.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_l2cif_config_cfm_cback(uint16_t lcid, tL2CAP_CFG_INFO* p_cfg) {
  tGATT_TCB* p_tcb;
  tGATTS_SRV_CHG* p_srv_chg_clt = NULL;

  /* look up clcb for this channel */
  p_tcb = gatt_find_tcb_by_cid(lcid);
  if (p_tcb != NULL) {
    /* if in correct state */
    if (gatt_get_ch_state(p_tcb) == GATT_CH_CFG) {
      /* if result successful */
      if (p_cfg->result == L2CAP_CFG_OK) {
        /* update flags */
        p_tcb->ch_flags |= GATT_L2C_CFG_CFM_DONE;

        /* if configuration complete */
        if (p_tcb->ch_flags & GATT_L2C_CFG_IND_DONE) {
          gatt_set_ch_state(p_tcb, GATT_CH_OPEN);

          p_srv_chg_clt = gatt_is_bda_in_the_srv_chg_clt_list(p_tcb->peer_bda);
          if (p_srv_chg_clt != NULL) {
            gatt_chk_srv_chg(p_srv_chg_clt);
          } else {
            if (btm_sec_is_a_bonded_dev(p_tcb->peer_bda))
              gatt_add_a_bonded_dev_for_srv_chg(p_tcb->peer_bda);
          }

          /* send callback */
          gatt_send_conn_cback(p_tcb);
        }
      }
      /* else failure */
      else {
        /* Send L2CAP disconnect req */
        L2CA_DisconnectReq(lcid);
      }
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_config_ind_cback
 *
 * Description      This is the L2CAP config indication callback function.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_l2cif_config_ind_cback(uint16_t lcid, tL2CAP_CFG_INFO* p_cfg) {
  tGATT_TCB* p_tcb;
  tGATTS_SRV_CHG* p_srv_chg_clt = NULL;
  /* look up clcb for this channel */
  p_tcb = gatt_find_tcb_by_cid(lcid);
  if (p_tcb != NULL) {
    /* GATT uses the smaller of our MTU and peer's MTU  */
    if (p_cfg->mtu_present &&
        (p_cfg->mtu >= GATT_MIN_BR_MTU_SIZE && p_cfg->mtu < L2CAP_DEFAULT_MTU))
      p_tcb->payload_size = p_cfg->mtu;
    else
      p_tcb->payload_size = L2CAP_DEFAULT_MTU;

    /* send L2CAP configure response */
    memset(p_cfg, 0, sizeof(tL2CAP_CFG_INFO));
    p_cfg->result = L2CAP_CFG_OK;
    L2CA_ConfigRsp(lcid, p_cfg);

    /* if first config ind */
    if ((p_tcb->ch_flags & GATT_L2C_CFG_IND_DONE) == 0) {
      /* update flags */
      p_tcb->ch_flags |= GATT_L2C_CFG_IND_DONE;

      /* if configuration complete */
      if (p_tcb->ch_flags & GATT_L2C_CFG_CFM_DONE) {
        gatt_set_ch_state(p_tcb, GATT_CH_OPEN);
        p_srv_chg_clt = gatt_is_bda_in_the_srv_chg_clt_list(p_tcb->peer_bda);
        if (p_srv_chg_clt != NULL) {
          gatt_chk_srv_chg(p_srv_chg_clt);
        } else {
          if (btm_sec_is_a_bonded_dev(p_tcb->peer_bda))
            gatt_add_a_bonded_dev_for_srv_chg(p_tcb->peer_bda);
        }

        /* send callback */
        gatt_send_conn_cback(p_tcb);
      }
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_disconnect_ind_cback
 *
 * Description      This is the L2CAP disconnect indication callback function.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_l2cif_disconnect_ind_cback(uint16_t lcid, bool ack_needed) {
  tGATT_TCB* p_tcb;
  uint16_t reason;

  /* look up clcb for this channel */
  p_tcb = gatt_find_tcb_by_cid(lcid);
  if (p_tcb != NULL) {
    if (ack_needed) {
      /* send L2CAP disconnect response */
      L2CA_DisconnectRsp(lcid);
    }
    if (gatt_is_bda_in_the_srv_chg_clt_list(p_tcb->peer_bda) == NULL) {
      if (btm_sec_is_a_bonded_dev(p_tcb->peer_bda))
        gatt_add_a_bonded_dev_for_srv_chg(p_tcb->peer_bda);
    }
    /* if ACL link is still up, no reason is logged, l2cap is disconnect from
     * peer */
    reason = L2CA_GetDisconnectReason(p_tcb->peer_bda, p_tcb->transport);
    if (reason == 0) reason = GATT_CONN_TERMINATE_PEER_USER;

    /* send disconnect callback */
    gatt_cleanup_upon_disc(p_tcb->peer_bda, reason, GATT_TRANSPORT_BR_EDR);
  }
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_disconnect_cfm_cback
 *
 * Description      This is the L2CAP disconnect confirm callback function.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_l2cif_disconnect_cfm_cback(uint16_t lcid,
                                            UNUSED_ATTR uint16_t result) {
  tGATT_TCB* p_tcb;
  uint16_t reason;

  /* look up clcb for this channel */
  p_tcb = gatt_find_tcb_by_cid(lcid);
  if (p_tcb != NULL) {
    /* If the device is not in the service changed client list, add it... */
    if (gatt_is_bda_in_the_srv_chg_clt_list(p_tcb->peer_bda) == NULL) {
      if (btm_sec_is_a_bonded_dev(p_tcb->peer_bda))
        gatt_add_a_bonded_dev_for_srv_chg(p_tcb->peer_bda);
    }

    /* send disconnect callback */
    /* if ACL link is still up, no reason is logged, l2cap is disconnect from
     * peer */
    reason = L2CA_GetDisconnectReason(p_tcb->peer_bda, p_tcb->transport);
    if (reason == 0) reason = GATT_CONN_TERMINATE_LOCAL_HOST;

    gatt_cleanup_upon_disc(p_tcb->peer_bda, reason, GATT_TRANSPORT_BR_EDR);
  }
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_data_ind_cback
 *
 * Description      This is the L2CAP data indication callback function.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_l2cif_data_ind_cback(uint16_t lcid, BT_HDR* p_buf) {
  tGATT_TCB* p_tcb;

  /* look up clcb for this channel */
  if ((p_tcb = gatt_find_tcb_by_cid(lcid)) != NULL &&
      gatt_get_ch_state(p_tcb) == GATT_CH_OPEN) {
    /* process the data */
    gatt_data_process(*p_tcb, p_buf);
  }

  osi_free(p_buf);
}

/*******************************************************************************
 *
 * Function         gatt_l2cif_congest_cback
 *
 * Description      L2CAP congestion callback
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_l2cif_congest_cback(uint16_t lcid, bool congested) {
  tGATT_TCB* p_tcb = gatt_find_tcb_by_cid(lcid);

  if (p_tcb != NULL) {
    gatt_channel_congestion(p_tcb, congested);
  }
}

/*******************************************************************************
 *
 * Function         gatt_send_conn_cback
 *
 * Description      Callback used to notify layer above about a connection.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
static void gatt_send_conn_cback(tGATT_TCB* p_tcb) {
  uint8_t i;
  tGATT_REG* p_reg;
  uint16_t conn_id;

  tGATT_BG_CONN_DEV* p_bg_dev = gatt_find_bg_dev(p_tcb->peer_bda);

  /* notifying all applications for the connection up event */
  for (i = 0, p_reg = gatt_cb.cl_rcb; i < GATT_MAX_APPS; i++, p_reg++) {
    if (p_reg->in_use) {
      if (p_bg_dev && gatt_is_bg_dev_for_app(p_bg_dev, p_reg->gatt_if))
        gatt_update_app_use_link_flag(p_reg->gatt_if, p_tcb, true, true);

      if (p_reg->app_cb.p_conn_cb) {
        conn_id = GATT_CREATE_CONN_ID(p_tcb->tcb_idx, p_reg->gatt_if);
        (*p_reg->app_cb.p_conn_cb)(p_reg->gatt_if, p_tcb->peer_bda, conn_id,
                                   true, 0, p_tcb->transport);
      }
    }
  }

  if (!p_tcb->app_hold_link.empty() && p_tcb->att_lcid == L2CAP_ATT_CID) {
    /* disable idle timeout if one or more clients are holding the link disable
     * the idle timer */
    GATT_SetIdleTimeout(p_tcb->peer_bda, GATT_LINK_NO_IDLE_TIMEOUT,
                        p_tcb->transport);
  }
}

/*******************************************************************************
 *
 * Function         gatt_le_data_ind
 *
 * Description      This function is called when data is received from L2CAP.
 *                  if we are the originator of the connection, we are the ATT
 *                  client, and the received message is queued up for the
 *                  client.
 *
 *                  If we are the destination of the connection, we are the ATT
 *                  server, so the message is passed to the server processing
 *                  function.
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_data_process(tGATT_TCB& tcb, BT_HDR* p_buf) {
  uint8_t* p = (uint8_t*)(p_buf + 1) + p_buf->offset;
  uint8_t op_code, pseudo_op_code;

  if (p_buf->len <= 0) {
    LOG(ERROR) << "invalid data length, ignore";
    return;
  }

  uint16_t msg_len = p_buf->len - 1;
  STREAM_TO_UINT8(op_code, p);

  /* remove the two MSBs associated with sign write and write cmd */
  pseudo_op_code = op_code & (~GATT_WRITE_CMD_MASK);

  if (pseudo_op_code >= GATT_OP_CODE_MAX) {
    /* Note: PTS: GATT/SR/UNS/BI-01-C mandates error on unsupported ATT request.
     */
    LOG(ERROR) << __func__
               << ": ATT - Rcvd L2CAP data, unknown cmd: " << loghex(op_code);
    gatt_send_error_rsp(tcb, GATT_REQ_NOT_SUPPORTED, op_code, 0, false);
    return;
  }

  if (op_code == GATT_SIGN_CMD_WRITE) {
    gatt_verify_signature(tcb, p_buf);
  } else {
    /* message from client */
    if ((op_code % 2) == 0)
      gatt_server_handle_client_req(tcb, op_code, msg_len, p);
    else
      gatt_client_handle_server_rsp(tcb, op_code, msg_len, p);
  }
}

/*******************************************************************************
 *
 * Function         gatt_add_a_bonded_dev_for_srv_chg
 *
 * Description      Add a bonded dev to the service changed client list
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_add_a_bonded_dev_for_srv_chg(const RawAddress& bda) {
  tGATTS_SRV_CHG_REQ req;
  tGATTS_SRV_CHG srv_chg_clt;

  srv_chg_clt.bda = bda;
  srv_chg_clt.srv_changed = false;
  if (gatt_add_srv_chg_clt(&srv_chg_clt) != NULL) {
    req.srv_chg.bda = bda;
    req.srv_chg.srv_changed = false;
    if (gatt_cb.cb_info.p_srv_chg_callback)
      (*gatt_cb.cb_info.p_srv_chg_callback)(GATTS_SRV_CHG_CMD_ADD_CLIENT, &req,
                                            NULL);
  }
}

/*******************************************************************************
 *
 * Function         gatt_send_srv_chg_ind
 *
 * Description      This function is called to send a service chnaged indication
 *                  to the specified bd address
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_send_srv_chg_ind(const RawAddress& peer_bda) {
  uint8_t handle_range[GATT_SIZE_OF_SRV_CHG_HNDL_RANGE];
  uint8_t* p = handle_range;
  uint16_t conn_id;

  VLOG(1) << "gatt_send_srv_chg_ind";

  if (gatt_cb.handle_of_h_r) {
    conn_id = gatt_profile_find_conn_id_by_bd_addr(peer_bda);
    if (conn_id != GATT_INVALID_CONN_ID) {
      UINT16_TO_STREAM(p, 1);
      UINT16_TO_STREAM(p, 0xFFFF);
      GATTS_HandleValueIndication(conn_id, gatt_cb.handle_of_h_r,
                                  GATT_SIZE_OF_SRV_CHG_HNDL_RANGE,
                                  handle_range);
    } else {
      LOG(ERROR) << "Unable to find conn_id for " << peer_bda;
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_chk_srv_chg
 *
 * Description      Check sending service chnaged Indication is required or not
 *                  if required then send the Indication
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_chk_srv_chg(tGATTS_SRV_CHG* p_srv_chg_clt) {
  VLOG(1) << __func__ << " srv_changed=" << +p_srv_chg_clt->srv_changed;

  if (p_srv_chg_clt->srv_changed) {
    gatt_send_srv_chg_ind(p_srv_chg_clt->bda);
  }
}

/*******************************************************************************
 *
 * Function         gatt_init_srv_chg
 *
 * Description      This function is used to initialize the service changed
 *                  attribute value
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_init_srv_chg(void) {
  tGATTS_SRV_CHG_REQ req;
  tGATTS_SRV_CHG_RSP rsp;
  bool status;
  uint8_t num_clients, i;
  tGATTS_SRV_CHG srv_chg_clt;

  VLOG(1) << __func__;
  if (gatt_cb.cb_info.p_srv_chg_callback) {
    status = (*gatt_cb.cb_info.p_srv_chg_callback)(
        GATTS_SRV_CHG_CMD_READ_NUM_CLENTS, NULL, &rsp);

    if (status && rsp.num_clients) {
      VLOG(1) << "num_srv_chg_clt_clients=" << +rsp.num_clients;
      num_clients = rsp.num_clients;
      i = 1; /* use one based index */
      while ((i <= num_clients) && status) {
        req.client_read_index = i;
        status = (*gatt_cb.cb_info.p_srv_chg_callback)(
            GATTS_SRV_CHG_CMD_READ_CLENT, &req, &rsp);
        if (status) {
          memcpy(&srv_chg_clt, &rsp.srv_chg, sizeof(tGATTS_SRV_CHG));
          if (gatt_add_srv_chg_clt(&srv_chg_clt) == NULL) {
            LOG(ERROR) << "Unable to add a service change client";
            status = false;
          }
        }
        i++;
      }
    }
  } else {
    VLOG(1) << __func__ << " callback not registered yet";
  }
}

/*******************************************************************************
 *
 * Function         gatt_proc_srv_chg
 *
 * Description      This function is process the service changed request
 *
 * Returns          void
 *
 ******************************************************************************/
void gatt_proc_srv_chg(void) {
  uint8_t start_idx, found_idx;
  RawAddress bda;
  tGATT_TCB* p_tcb;
  tBT_TRANSPORT transport;

  VLOG(1) << __func__;

  if (gatt_cb.cb_info.p_srv_chg_callback && gatt_cb.handle_of_h_r) {
    gatt_set_srv_chg();
    start_idx = 0;
    while (
        gatt_find_the_connected_bda(start_idx, bda, &found_idx, &transport)) {
      p_tcb = &gatt_cb.tcb[found_idx];

      bool send_indication = true;

      if (gatt_is_srv_chg_ind_pending(p_tcb)) {
        send_indication = false;
        VLOG(1) << "discard srv chg - already has one in the queue";
      }

      // Some LE GATT clients don't respond to service changed indications.
      char remote_name[BTM_MAX_REM_BD_NAME_LEN] = "";
      if (send_indication &&
          btif_storage_get_stored_remote_name(bda, remote_name)) {
        if (interop_match_name(INTEROP_GATTC_NO_SERVICE_CHANGED_IND,
                               remote_name)) {
          VLOG(1) << "discard srv chg - interop matched " << remote_name;
          send_indication = false;
        }
      }

      if (send_indication) gatt_send_srv_chg_ind(bda);

      start_idx = ++found_idx;
    }
  }
}

/*******************************************************************************
 *
 * Function         gatt_set_ch_state
 *
 * Description      This function set the ch_state in tcb
 *
 * Returns          none
 *
 ******************************************************************************/
void gatt_set_ch_state(tGATT_TCB* p_tcb, tGATT_CH_STATE ch_state) {
  if (p_tcb) {
    VLOG(1) << __func__ << ": old=" << +p_tcb->ch_state << " new=" << ch_state;
    p_tcb->ch_state = ch_state;
  }
}

/*******************************************************************************
 *
 * Function         gatt_get_ch_state
 *
 * Description      This function get the ch_state in tcb
 *
 * Returns          none
 *
 ******************************************************************************/
tGATT_CH_STATE gatt_get_ch_state(tGATT_TCB* p_tcb) {
  tGATT_CH_STATE ch_state = GATT_CH_CLOSE;
  if (p_tcb) {
    VLOG(1) << "gatt_get_ch_state: ch_state=" << +p_tcb->ch_state;
    ch_state = p_tcb->ch_state;
  }
  return ch_state;
}
