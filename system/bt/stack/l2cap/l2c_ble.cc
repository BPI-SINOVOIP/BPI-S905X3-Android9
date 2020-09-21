/******************************************************************************
 *
 *  Copyright 2009-2012 Broadcom Corporation
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
 *  this file contains functions relating to BLE management.
 *
 ******************************************************************************/

#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <string.h>
#include "bt_target.h"
#include "bt_utils.h"
#include "bta_hearing_aid_api.h"
#include "btm_int.h"
#include "btu.h"
#include "device/include/controller.h"
#include "hcimsgs.h"
#include "l2c_int.h"
#include "l2cdefs.h"
#include "log/log.h"
#include "osi/include/osi.h"
#include "stack_config.h"

using base::StringPrintf;

static void l2cble_start_conn_update(tL2C_LCB* p_lcb);

/*******************************************************************************
 *
 *  Function        L2CA_CancelBleConnectReq
 *
 *  Description     Cancel a pending connection attempt to a BLE device.
 *
 *  Parameters:     BD Address of remote
 *
 *  Return value:   true if connection was cancelled
 *
 ******************************************************************************/
bool L2CA_CancelBleConnectReq(const RawAddress& rem_bda) {
  tL2C_LCB* p_lcb;

  /* There can be only one BLE connection request outstanding at a time */
  if (btm_ble_get_conn_st() == BLE_CONN_IDLE) {
    L2CAP_TRACE_WARNING("%s - no connection pending", __func__);
    return (false);
  }

  if (rem_bda != l2cb.ble_connecting_bda) {
    LOG(WARNING) << __func__
                 << " different BDA Connecting: " << l2cb.ble_connecting_bda
                 << " Cancel: " << rem_bda;

    btm_ble_dequeue_direct_conn_req(rem_bda);
    return (false);
  }

  btsnd_hcic_ble_create_conn_cancel();

  p_lcb = l2cu_find_lcb_by_bd_addr(rem_bda, BT_TRANSPORT_LE);
  /* Do not remove lcb if an LE link is already up as a peripheral */
  if (p_lcb != NULL &&
      !(p_lcb->link_role == HCI_ROLE_SLAVE &&
        btm_bda_to_acl(rem_bda, BT_TRANSPORT_LE) != NULL)) {
    p_lcb->disc_reason = L2CAP_CONN_CANCEL;
    l2cu_release_lcb(p_lcb);
  }
  /* update state to be cancel, wait for connection cancel complete */
  btm_ble_set_conn_st(BLE_CONN_CANCEL);

  return (true);
}

/*******************************************************************************
 *
 *  Function        L2CA_UpdateBleConnParams
 *
 *  Description     Update BLE connection parameters.
 *
 *  Parameters:     BD Address of remote
 *
 *  Return value:   true if update started
 *
 ******************************************************************************/
bool L2CA_UpdateBleConnParams(const RawAddress& rem_bda, uint16_t min_int,
                              uint16_t max_int, uint16_t latency,
                              uint16_t timeout, uint16_t min_ce_len,
                              uint16_t max_ce_len) {
  tL2C_LCB* p_lcb;
  tACL_CONN* p_acl_cb = btm_bda_to_acl(rem_bda, BT_TRANSPORT_LE);

  /* See if we have a link control block for the remote device */
  p_lcb = l2cu_find_lcb_by_bd_addr(rem_bda, BT_TRANSPORT_LE);

  /* If we don't have one, create one and accept the connection. */
  if (!p_lcb || !p_acl_cb) {
    LOG(WARNING) << __func__ << " - unknown BD_ADDR " << rem_bda;
    return (false);
  }

  if (p_lcb->transport != BT_TRANSPORT_LE) {
    LOG(WARNING) << __func__ << " - BD_ADDR " << rem_bda << " not LE";
    return (false);
  }

  VLOG(2) << __func__ << ": BD_ADDR=" << rem_bda << ", min_int=" << min_int
          << ", max_int=" << max_int << ", min_ce_len=" << min_ce_len
          << ", max_ce_len=" << max_ce_len;

  p_lcb->min_interval = min_int;
  p_lcb->max_interval = max_int;
  p_lcb->latency = latency;
  p_lcb->timeout = timeout;
  p_lcb->conn_update_mask |= L2C_BLE_NEW_CONN_PARAM;
  p_lcb->min_ce_len = min_ce_len;
  p_lcb->max_ce_len = max_ce_len;

  l2cble_start_conn_update(p_lcb);

  return (true);
}

bool L2CA_UpdateBleConnParams(const RawAddress& rem_bda, uint16_t min_int,
                              uint16_t max_int, uint16_t latency,
                              uint16_t timeout) {
  return L2CA_UpdateBleConnParams(rem_bda, min_int, max_int, latency, timeout,
                                  0, 0);
}

/*******************************************************************************
 *
 *  Function        L2CA_EnableUpdateBleConnParams
 *
 *  Description     Enable or disable update based on the request from the peer
 *
 *  Parameters:     BD Address of remote
 *
 *  Return value:   true if update started
 *
 ******************************************************************************/
bool L2CA_EnableUpdateBleConnParams(const RawAddress& rem_bda, bool enable) {
  if (stack_config_get_interface()->get_pts_conn_updates_disabled())
    return false;

  tL2C_LCB* p_lcb;

  /* See if we have a link control block for the remote device */
  p_lcb = l2cu_find_lcb_by_bd_addr(rem_bda, BT_TRANSPORT_LE);

  if (!p_lcb) {
    LOG(WARNING) << __func__ << " - unknown BD_ADDR " << rem_bda;
    return false;
  }

  VLOG(2) << __func__ << " - BD_ADDR " << rem_bda
          << StringPrintf(" enable %d current upd state 0x%02x", enable,
                          p_lcb->conn_update_mask);

  if (p_lcb->transport != BT_TRANSPORT_LE) {
    LOG(WARNING) << __func__ << " - BD_ADDR " << rem_bda
                 << " not LE, link role " << p_lcb->link_role;
    return false;
  }

  if (enable)
    p_lcb->conn_update_mask &= ~L2C_BLE_CONN_UPDATE_DISABLE;
  else
    p_lcb->conn_update_mask |= L2C_BLE_CONN_UPDATE_DISABLE;

  l2cble_start_conn_update(p_lcb);

  return (true);
}

/*******************************************************************************
 *
 * Function         L2CA_GetBleConnRole
 *
 * Description      This function returns the connection role.
 *
 * Returns          link role.
 *
 ******************************************************************************/
uint8_t L2CA_GetBleConnRole(const RawAddress& bd_addr) {
  uint8_t role = HCI_ROLE_UNKNOWN;

  tL2C_LCB* p_lcb;

  p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr, BT_TRANSPORT_LE);
  if (p_lcb != NULL) role = p_lcb->link_role;

  return role;
}
/*******************************************************************************
 *
 * Function         L2CA_GetDisconnectReason
 *
 * Description      This function returns the disconnect reason code.
 *
 * Returns          disconnect reason
 *
 ******************************************************************************/
uint16_t L2CA_GetDisconnectReason(const RawAddress& remote_bda,
                                  tBT_TRANSPORT transport) {
  tL2C_LCB* p_lcb;
  uint16_t reason = 0;

  p_lcb = l2cu_find_lcb_by_bd_addr(remote_bda, transport);
  if (p_lcb != NULL) reason = p_lcb->disc_reason;

  L2CAP_TRACE_DEBUG("L2CA_GetDisconnectReason=%d ", reason);

  return reason;
}

/*******************************************************************************
 *
 * Function l2cble_notify_le_connection
 *
 * Description This function notifiy the l2cap connection to the app layer
 *
 * Returns none
 *
 ******************************************************************************/
void l2cble_notify_le_connection(const RawAddress& bda) {
  tL2C_LCB* p_lcb = l2cu_find_lcb_by_bd_addr(bda, BT_TRANSPORT_LE);
  tACL_CONN* p_acl = btm_bda_to_acl(bda, BT_TRANSPORT_LE);
  tL2C_CCB* p_ccb;

  if (p_lcb != NULL && p_acl != NULL && p_lcb->link_state != LST_CONNECTED) {
    /* update link status */
    btm_establish_continue(p_acl);
    /* update l2cap link status and send callback */
    p_lcb->link_state = LST_CONNECTED;
    l2cu_process_fixed_chnl_resp(p_lcb);
  }

  if (p_lcb != NULL) {
    /* For all channels, send the event through their FSMs */
    for (p_ccb = p_lcb->ccb_queue.p_first_ccb; p_ccb;
         p_ccb = p_ccb->p_next_ccb) {
      if (p_ccb->chnl_state == CST_CLOSED)
        l2c_csm_execute(p_ccb, L2CEVT_LP_CONNECT_CFM, NULL);
    }
  }
}

/*******************************************************************************
 *
 * Function         l2cble_scanner_conn_comp
 *
 * Description      This function is called when an HCI Connection Complete
 *                  event is received while we are a scanner (so we are master).
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_scanner_conn_comp(uint16_t handle, const RawAddress& bda,
                              tBLE_ADDR_TYPE type, uint16_t conn_interval,
                              uint16_t conn_latency, uint16_t conn_timeout) {
  tL2C_LCB* p_lcb;
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_or_alloc_dev(bda);

  L2CAP_TRACE_DEBUG(
      "l2cble_scanner_conn_comp: HANDLE=%d addr_type=%d conn_interval=%d "
      "slave_latency=%d supervision_tout=%d",
      handle, type, conn_interval, conn_latency, conn_timeout);

  l2cb.is_ble_connecting = false;

  /* See if we have a link control block for the remote device */
  p_lcb = l2cu_find_lcb_by_bd_addr(bda, BT_TRANSPORT_LE);

  /* If we don't have one, create one. this is auto connection complete. */
  if (!p_lcb) {
    p_lcb = l2cu_allocate_lcb(bda, false, BT_TRANSPORT_LE);
    if (!p_lcb) {
      btm_sec_disconnect(handle, HCI_ERR_NO_CONNECTION);
      L2CAP_TRACE_ERROR("l2cble_scanner_conn_comp - failed to allocate LCB");
      btm_ble_set_conn_st(BLE_CONN_IDLE);
      return;
    } else {
      if (!l2cu_initialize_fixed_ccb(
              p_lcb, L2CAP_ATT_CID,
              &l2cb.fixed_reg[L2CAP_ATT_CID - L2CAP_FIRST_FIXED_CHNL]
                   .fixed_chnl_opts)) {
        btm_sec_disconnect(handle, HCI_ERR_NO_CONNECTION);
        L2CAP_TRACE_WARNING("l2cble_scanner_conn_comp - LCB but no CCB");
        btm_ble_set_conn_st(BLE_CONN_IDLE);
        return;
      }
    }
  } else if (p_lcb->link_state != LST_CONNECTING) {
    L2CAP_TRACE_ERROR("L2CAP got BLE scanner conn_comp in bad state: %d",
                      p_lcb->link_state);
    btm_ble_set_conn_st(BLE_CONN_IDLE);
    return;
  }
  alarm_cancel(p_lcb->l2c_lcb_timer);

  /* Save the handle */
  p_lcb->handle = handle;

  /* Connected OK. Change state to connected, we were scanning so we are master
   */
  p_lcb->link_role = HCI_ROLE_MASTER;
  p_lcb->transport = BT_TRANSPORT_LE;

  /* update link parameter, set slave link as non-spec default upon link up */
  p_lcb->min_interval = p_lcb->max_interval = conn_interval;
  p_lcb->timeout = conn_timeout;
  p_lcb->latency = conn_latency;
  p_lcb->conn_update_mask = L2C_BLE_NOT_DEFAULT_PARAM;

  /* Tell BTM Acl management about the link */
  btm_acl_created(bda, NULL, p_dev_rec->sec_bd_name, handle, p_lcb->link_role,
                  BT_TRANSPORT_LE);

  p_lcb->peer_chnl_mask[0] = L2CAP_FIXED_CHNL_ATT_BIT |
                             L2CAP_FIXED_CHNL_BLE_SIG_BIT |
                             L2CAP_FIXED_CHNL_SMP_BIT;

  btm_ble_set_conn_st(BLE_CONN_IDLE);

#if (BLE_PRIVACY_SPT == TRUE)
  btm_ble_disable_resolving_list(BTM_BLE_RL_INIT, true);
#endif
}

/*******************************************************************************
 *
 * Function         l2cble_advertiser_conn_comp
 *
 * Description      This function is called when an HCI Connection Complete
 *                  event is received while we are an advertiser (so we are
 *                  slave).
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_advertiser_conn_comp(uint16_t handle, const RawAddress& bda,
                                 UNUSED_ATTR tBLE_ADDR_TYPE type,
                                 UNUSED_ATTR uint16_t conn_interval,
                                 UNUSED_ATTR uint16_t conn_latency,
                                 UNUSED_ATTR uint16_t conn_timeout) {
  tL2C_LCB* p_lcb;
  tBTM_SEC_DEV_REC* p_dev_rec;

  /* See if we have a link control block for the remote device */
  p_lcb = l2cu_find_lcb_by_bd_addr(bda, BT_TRANSPORT_LE);

  /* If we don't have one, create one and accept the connection. */
  if (!p_lcb) {
    p_lcb = l2cu_allocate_lcb(bda, false, BT_TRANSPORT_LE);
    if (!p_lcb) {
      btm_sec_disconnect(handle, HCI_ERR_NO_CONNECTION);
      L2CAP_TRACE_ERROR("l2cble_advertiser_conn_comp - failed to allocate LCB");
      return;
    } else {
      if (!l2cu_initialize_fixed_ccb(
              p_lcb, L2CAP_ATT_CID,
              &l2cb.fixed_reg[L2CAP_ATT_CID - L2CAP_FIRST_FIXED_CHNL]
                   .fixed_chnl_opts)) {
        btm_sec_disconnect(handle, HCI_ERR_NO_CONNECTION);
        L2CAP_TRACE_WARNING("l2cble_scanner_conn_comp - LCB but no CCB");
        return;
      }
    }
  }

  /* Save the handle */
  p_lcb->handle = handle;

  /* Connected OK. Change state to connected, we were advertising, so we are
   * slave */
  p_lcb->link_role = HCI_ROLE_SLAVE;
  p_lcb->transport = BT_TRANSPORT_LE;

  /* update link parameter, set slave link as non-spec default upon link up */
  p_lcb->min_interval = p_lcb->max_interval = conn_interval;
  p_lcb->timeout = conn_timeout;
  p_lcb->latency = conn_latency;
  p_lcb->conn_update_mask = L2C_BLE_NOT_DEFAULT_PARAM;

  /* Tell BTM Acl management about the link */
  p_dev_rec = btm_find_or_alloc_dev(bda);

  btm_acl_created(bda, NULL, p_dev_rec->sec_bd_name, handle, p_lcb->link_role,
                  BT_TRANSPORT_LE);

#if (BLE_PRIVACY_SPT == TRUE)
  btm_ble_disable_resolving_list(BTM_BLE_RL_ADV, true);
#endif

  p_lcb->peer_chnl_mask[0] = L2CAP_FIXED_CHNL_ATT_BIT |
                             L2CAP_FIXED_CHNL_BLE_SIG_BIT |
                             L2CAP_FIXED_CHNL_SMP_BIT;

  if (!HCI_LE_SLAVE_INIT_FEAT_EXC_SUPPORTED(
          controller_get_interface()->get_features_ble()->as_array)) {
    p_lcb->link_state = LST_CONNECTED;
    l2cu_process_fixed_chnl_resp(p_lcb);
  }

  /* when adv and initiating are both active, cancel the direct connection */
  if (l2cb.is_ble_connecting && bda == l2cb.ble_connecting_bda) {
    L2CA_CancelBleConnectReq(bda);
  }
}

/*******************************************************************************
 *
 * Function         l2cble_conn_comp
 *
 * Description      This function is called when an HCI Connection Complete
 *                  event is received.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_conn_comp(uint16_t handle, uint8_t role, const RawAddress& bda,
                      tBLE_ADDR_TYPE type, uint16_t conn_interval,
                      uint16_t conn_latency, uint16_t conn_timeout) {
  btm_ble_update_link_topology_mask(role, true);

  if (role == HCI_ROLE_MASTER) {
    l2cble_scanner_conn_comp(handle, bda, type, conn_interval, conn_latency,
                             conn_timeout);
  } else {
    l2cble_advertiser_conn_comp(handle, bda, type, conn_interval, conn_latency,
                                conn_timeout);
  }
}

/*******************************************************************************
 *
 *  Function        l2cble_start_conn_update
 *
 *  Description     Start the BLE connection parameter update process based on
 *                  status.
 *
 *  Parameters:     lcb : l2cap link control block
 *
 *  Return value:   none
 *
 ******************************************************************************/
static void l2cble_start_conn_update(tL2C_LCB* p_lcb) {
  uint16_t min_conn_int, max_conn_int, slave_latency, supervision_tout;
  tACL_CONN* p_acl_cb = btm_bda_to_acl(p_lcb->remote_bd_addr, BT_TRANSPORT_LE);
  if (!p_acl_cb) {
    LOG(ERROR) << "No known connection ACL for " << p_lcb->remote_bd_addr;
    return;
  }

  // TODO(armansito): The return value of this call wasn't being used but the
  // logic of this function might be depending on its side effects. We should
  // verify if this call is needed at all and remove it otherwise.
  btm_find_or_alloc_dev(p_lcb->remote_bd_addr);

  if (p_lcb->conn_update_mask & L2C_BLE_UPDATE_PENDING) return;

  if (p_lcb->conn_update_mask & L2C_BLE_CONN_UPDATE_DISABLE) {
    /* application requests to disable parameters update.
       If parameters are already updated, lets set them
       up to what has been requested during connection establishement */
    if (p_lcb->conn_update_mask & L2C_BLE_NOT_DEFAULT_PARAM &&
        /* current connection interval is greater than default min */
        p_lcb->min_interval > BTM_BLE_CONN_INT_MIN) {
      /* use 7.5 ms as fast connection parameter, 0 slave latency */
      min_conn_int = max_conn_int = BTM_BLE_CONN_INT_MIN;

      L2CA_AdjustConnectionIntervals(&min_conn_int, &max_conn_int,
                                     BTM_BLE_CONN_INT_MIN);

      slave_latency = BTM_BLE_CONN_SLAVE_LATENCY_DEF;
      supervision_tout = BTM_BLE_CONN_TIMEOUT_DEF;

      /* if both side 4.1, or we are master device, send HCI command */
      if (p_lcb->link_role == HCI_ROLE_MASTER
#if (BLE_LLT_INCLUDED == TRUE)
          || (HCI_LE_CONN_PARAM_REQ_SUPPORTED(
                  controller_get_interface()->get_features_ble()->as_array) &&
              HCI_LE_CONN_PARAM_REQ_SUPPORTED(p_acl_cb->peer_le_features))
#endif
              ) {
        btsnd_hcic_ble_upd_ll_conn_params(p_lcb->handle, min_conn_int,
                                          max_conn_int, slave_latency,
                                          supervision_tout, 0, 0);
        p_lcb->conn_update_mask |= L2C_BLE_UPDATE_PENDING;
      } else {
        l2cu_send_peer_ble_par_req(p_lcb, min_conn_int, max_conn_int,
                                   slave_latency, supervision_tout);
      }
      p_lcb->conn_update_mask &= ~L2C_BLE_NOT_DEFAULT_PARAM;
      p_lcb->conn_update_mask |= L2C_BLE_NEW_CONN_PARAM;
    }
  } else {
    /* application allows to do update, if we were delaying one do it now */
    if (p_lcb->conn_update_mask & L2C_BLE_NEW_CONN_PARAM) {
      /* if both side 4.1, or we are master device, send HCI command */
      if (p_lcb->link_role == HCI_ROLE_MASTER
#if (BLE_LLT_INCLUDED == TRUE)
          || (HCI_LE_CONN_PARAM_REQ_SUPPORTED(
                  controller_get_interface()->get_features_ble()->as_array) &&
              HCI_LE_CONN_PARAM_REQ_SUPPORTED(p_acl_cb->peer_le_features))
#endif
              ) {
        btsnd_hcic_ble_upd_ll_conn_params(p_lcb->handle, p_lcb->min_interval,
                                          p_lcb->max_interval, p_lcb->latency,
                                          p_lcb->timeout, p_lcb->min_ce_len,
                                          p_lcb->max_ce_len);
        p_lcb->conn_update_mask |= L2C_BLE_UPDATE_PENDING;
      } else {
        l2cu_send_peer_ble_par_req(p_lcb, p_lcb->min_interval,
                                   p_lcb->max_interval, p_lcb->latency,
                                   p_lcb->timeout);
      }
      p_lcb->conn_update_mask &= ~L2C_BLE_NEW_CONN_PARAM;
      p_lcb->conn_update_mask |= L2C_BLE_NOT_DEFAULT_PARAM;
    }
  }
}

/*******************************************************************************
 *
 * Function         l2cble_process_conn_update_evt
 *
 * Description      This function enables the connection update request from
 *                  remote after a successful connection update response is
 *                  received.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_process_conn_update_evt(uint16_t handle, uint8_t status,
                                    uint16_t interval, uint16_t latency,
                                    uint16_t timeout) {
  L2CAP_TRACE_DEBUG("%s", __func__);

  /* See if we have a link control block for the remote device */
  tL2C_LCB* p_lcb = l2cu_find_lcb_by_handle(handle);
  if (!p_lcb) {
    L2CAP_TRACE_WARNING("%s: Invalid handle: %d", __func__, handle);
    return;
  }

  p_lcb->conn_update_mask &= ~L2C_BLE_UPDATE_PENDING;

  if (status != HCI_SUCCESS) {
    L2CAP_TRACE_WARNING("%s: Error status: %d", __func__, status);
  }

  l2cble_start_conn_update(p_lcb);

  L2CAP_TRACE_DEBUG("%s: conn_update_mask=%d", __func__,
                    p_lcb->conn_update_mask);
}

/*******************************************************************************
 *
 * Function         l2cble_process_sig_cmd
 *
 * Description      This function is called when a signalling packet is received
 *                  on the BLE signalling CID
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_process_sig_cmd(tL2C_LCB* p_lcb, uint8_t* p, uint16_t pkt_len) {
  uint8_t* p_pkt_end;
  uint8_t cmd_code, id;
  uint16_t cmd_len;
  uint16_t min_interval, max_interval, latency, timeout;
  tL2C_CONN_INFO con_info;
  uint16_t lcid = 0, rcid = 0, mtu = 0, mps = 0, initial_credit = 0;
  tL2C_CCB *p_ccb = NULL, *temp_p_ccb = NULL;
  tL2C_RCB* p_rcb;
  uint16_t credit;
  p_pkt_end = p + pkt_len;

  STREAM_TO_UINT8(cmd_code, p);
  STREAM_TO_UINT8(id, p);
  STREAM_TO_UINT16(cmd_len, p);

  /* Check command length does not exceed packet length */
  if ((p + cmd_len) > p_pkt_end) {
    L2CAP_TRACE_WARNING(
        "L2CAP - LE - format error, pkt_len: %d  cmd_len: %d  code: %d",
        pkt_len, cmd_len, cmd_code);
    return;
  }

  switch (cmd_code) {
    case L2CAP_CMD_REJECT:
      p += 2;
      break;

    case L2CAP_CMD_ECHO_REQ:
    case L2CAP_CMD_ECHO_RSP:
    case L2CAP_CMD_INFO_RSP:
    case L2CAP_CMD_INFO_REQ:
      l2cu_send_peer_cmd_reject(p_lcb, L2CAP_CMD_REJ_NOT_UNDERSTOOD, id, 0, 0);
      break;

    case L2CAP_CMD_BLE_UPDATE_REQ:
      STREAM_TO_UINT16(min_interval, p); /* 0x0006 - 0x0C80 */
      STREAM_TO_UINT16(max_interval, p); /* 0x0006 - 0x0C80 */
      STREAM_TO_UINT16(latency, p);      /* 0x0000 - 0x03E8 */
      STREAM_TO_UINT16(timeout, p);      /* 0x000A - 0x0C80 */
      /* If we are a master, the slave wants to update the parameters */
      if (p_lcb->link_role == HCI_ROLE_MASTER) {
        L2CA_AdjustConnectionIntervals(&min_interval, &max_interval,
                                       BTM_BLE_CONN_INT_MIN_LIMIT);

        if (min_interval < BTM_BLE_CONN_INT_MIN ||
            min_interval > BTM_BLE_CONN_INT_MAX ||
            max_interval < BTM_BLE_CONN_INT_MIN ||
            max_interval > BTM_BLE_CONN_INT_MAX ||
            latency > BTM_BLE_CONN_LATENCY_MAX ||
            /*(timeout >= max_interval && latency > (timeout * 10/(max_interval
               * 1.25) - 1)) ||*/
            timeout < BTM_BLE_CONN_SUP_TOUT_MIN ||
            timeout > BTM_BLE_CONN_SUP_TOUT_MAX ||
            max_interval < min_interval) {
          l2cu_send_peer_ble_par_rsp(p_lcb, L2CAP_CFG_UNACCEPTABLE_PARAMS, id);
        } else {
          l2cu_send_peer_ble_par_rsp(p_lcb, L2CAP_CFG_OK, id);

          p_lcb->min_interval = min_interval;
          p_lcb->max_interval = max_interval;
          p_lcb->latency = latency;
          p_lcb->timeout = timeout;
          p_lcb->conn_update_mask |= L2C_BLE_NEW_CONN_PARAM;

          l2cble_start_conn_update(p_lcb);
        }
      } else
        l2cu_send_peer_cmd_reject(p_lcb, L2CAP_CMD_REJ_NOT_UNDERSTOOD, id, 0,
                                  0);
      break;

    case L2CAP_CMD_BLE_UPDATE_RSP:
      p += 2;
      break;

    case L2CAP_CMD_BLE_CREDIT_BASED_CONN_REQ:
      STREAM_TO_UINT16(con_info.psm, p);
      STREAM_TO_UINT16(rcid, p);
      STREAM_TO_UINT16(mtu, p);
      STREAM_TO_UINT16(mps, p);
      STREAM_TO_UINT16(initial_credit, p);

      L2CAP_TRACE_DEBUG(
          "Recv L2CAP_CMD_BLE_CREDIT_BASED_CONN_REQ with "
          "mtu = %d, "
          "mps = %d, "
          "initial credit = %d",
          mtu, mps, initial_credit);

      p_ccb = l2cu_find_ccb_by_remote_cid(p_lcb, rcid);
      if (p_ccb) {
        L2CAP_TRACE_WARNING("L2CAP - rcvd conn req for duplicated cid: 0x%04x",
                            rcid);
        l2cu_reject_ble_connection(p_lcb, id,
                                   L2CAP_LE_SOURCE_CID_ALREADY_ALLOCATED);
        break;
      }

      p_rcb = l2cu_find_ble_rcb_by_psm(con_info.psm);
      if (p_rcb == NULL) {
        L2CAP_TRACE_WARNING("L2CAP - rcvd conn req for unknown PSM: 0x%04x",
                            con_info.psm);
        l2cu_reject_ble_connection(p_lcb, id, L2CAP_LE_NO_PSM);
        break;
      } else {
        if (!p_rcb->api.pL2CA_ConnectInd_Cb) {
          L2CAP_TRACE_WARNING(
              "L2CAP - rcvd conn req for outgoing-only connection PSM: %d",
              con_info.psm);
          l2cu_reject_ble_connection(p_lcb, id, L2CAP_CONN_NO_PSM);
          break;
        }
      }

      /* Allocate a ccb for this.*/
      p_ccb = l2cu_allocate_ccb(p_lcb, 0);
      if (p_ccb == NULL) {
        L2CAP_TRACE_ERROR("L2CAP - unable to allocate CCB");
        l2cu_reject_ble_connection(p_lcb, id, L2CAP_CONN_NO_RESOURCES);
        break;
      }

      /* validate the parameters */
      if (mtu < L2CAP_LE_MIN_MTU || mps < L2CAP_LE_MIN_MPS ||
          mps > L2CAP_LE_MAX_MPS) {
        L2CAP_TRACE_ERROR("L2CAP don't like the params");
        l2cu_reject_ble_connection(p_lcb, id, L2CAP_CONN_NO_RESOURCES);
        break;
      }

      p_ccb->remote_id = id;
      p_ccb->p_rcb = p_rcb;
      p_ccb->remote_cid = rcid;

      p_ccb->peer_conn_cfg.mtu = mtu;
      p_ccb->peer_conn_cfg.mps = mps;
      p_ccb->peer_conn_cfg.credits = initial_credit;

      p_ccb->tx_mps = mps;
      p_ccb->ble_sdu = NULL;
      p_ccb->ble_sdu_length = 0;
      p_ccb->is_first_seg = true;
      p_ccb->peer_cfg.fcr.mode = L2CAP_FCR_LE_COC_MODE;

      l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_REQ, &con_info);
      break;

    case L2CAP_CMD_BLE_CREDIT_BASED_CONN_RES:
      L2CAP_TRACE_DEBUG("Recv L2CAP_CMD_BLE_CREDIT_BASED_CONN_RES");
      /* For all channels, see whose identifier matches this id */
      for (temp_p_ccb = p_lcb->ccb_queue.p_first_ccb; temp_p_ccb;
           temp_p_ccb = temp_p_ccb->p_next_ccb) {
        if (temp_p_ccb->local_id == id) {
          p_ccb = temp_p_ccb;
          break;
        }
      }
      if (p_ccb) {
        L2CAP_TRACE_DEBUG("I remember the connection req");
        STREAM_TO_UINT16(p_ccb->remote_cid, p);
        STREAM_TO_UINT16(p_ccb->peer_conn_cfg.mtu, p);
        STREAM_TO_UINT16(p_ccb->peer_conn_cfg.mps, p);
        STREAM_TO_UINT16(p_ccb->peer_conn_cfg.credits, p);
        STREAM_TO_UINT16(con_info.l2cap_result, p);
        con_info.remote_cid = p_ccb->remote_cid;

        L2CAP_TRACE_DEBUG(
            "remote_cid = %d, "
            "mtu = %d, "
            "mps = %d, "
            "initial_credit = %d, "
            "con_info.l2cap_result = %d",
            p_ccb->remote_cid, p_ccb->peer_conn_cfg.mtu,
            p_ccb->peer_conn_cfg.mps, p_ccb->peer_conn_cfg.credits,
            con_info.l2cap_result);

        /* validate the parameters */
        if (p_ccb->peer_conn_cfg.mtu < L2CAP_LE_MIN_MTU ||
            p_ccb->peer_conn_cfg.mps < L2CAP_LE_MIN_MPS ||
            p_ccb->peer_conn_cfg.mps > L2CAP_LE_MAX_MPS) {
          L2CAP_TRACE_ERROR("L2CAP don't like the params");
          con_info.l2cap_result = L2CAP_LE_NO_RESOURCES;
          l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP_NEG, &con_info);
          break;
        }

        p_ccb->tx_mps = p_ccb->peer_conn_cfg.mps;
        p_ccb->ble_sdu = NULL;
        p_ccb->ble_sdu_length = 0;
        p_ccb->is_first_seg = true;
        p_ccb->peer_cfg.fcr.mode = L2CAP_FCR_LE_COC_MODE;

        if (con_info.l2cap_result == L2CAP_LE_CONN_OK)
          l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP, &con_info);
        else
          l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP_NEG, &con_info);
      } else {
        L2CAP_TRACE_DEBUG("I DO NOT remember the connection req");
        con_info.l2cap_result = L2CAP_LE_INVALID_SOURCE_CID;
        l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP_NEG, &con_info);
      }
      break;

    case L2CAP_CMD_BLE_FLOW_CTRL_CREDIT:
      STREAM_TO_UINT16(lcid, p);
      p_ccb = l2cu_find_ccb_by_remote_cid(p_lcb, lcid);
      if (p_ccb == NULL) {
        L2CAP_TRACE_DEBUG("%s Credit received for unknown channel id %d",
                          __func__, lcid);
        break;
      }

      STREAM_TO_UINT16(credit, p);
      l2c_csm_execute(p_ccb, L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT, &credit);
      L2CAP_TRACE_DEBUG("%s Credit received", __func__);
      break;

    case L2CAP_CMD_DISC_REQ:
      if (p + 4 > p_pkt_end) {
        android_errorWriteLog(0x534e4554, "74121659");
        return;
      }
      STREAM_TO_UINT16(lcid, p);
      STREAM_TO_UINT16(rcid, p);

      p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid);
      if (p_ccb != NULL) {
        if (p_ccb->remote_cid == rcid) {
          p_ccb->remote_id = id;
          l2c_csm_execute(p_ccb, L2CEVT_L2CAP_DISCONNECT_REQ, NULL);
        }
      } else
        l2cu_send_peer_disc_rsp(p_lcb, id, lcid, rcid);

      break;

    case L2CAP_CMD_DISC_RSP:
      STREAM_TO_UINT16(rcid, p);
      STREAM_TO_UINT16(lcid, p);

      p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid);
      if (p_ccb != NULL) {
        if ((p_ccb->remote_cid == rcid) && (p_ccb->local_id == id))
          l2c_csm_execute(p_ccb, L2CEVT_L2CAP_DISCONNECT_RSP, NULL);
      }
      break;

    default:
      L2CAP_TRACE_WARNING("L2CAP - LE - unknown cmd code: %d", cmd_code);
      l2cu_send_peer_cmd_reject(p_lcb, L2CAP_CMD_REJ_NOT_UNDERSTOOD, id, 0, 0);
      break;
  }
}

/*******************************************************************************
 *
 * Function         l2cble_init_direct_conn
 *
 * Description      This function is to initate a direct connection
 *
 * Returns          true connection initiated, false otherwise.
 *
 ******************************************************************************/
bool l2cble_init_direct_conn(tL2C_LCB* p_lcb) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_or_alloc_dev(p_lcb->remote_bd_addr);
  tBTM_BLE_CB* p_cb = &btm_cb.ble_ctr_cb;
  uint16_t scan_int;
  uint16_t scan_win;
  RawAddress peer_addr;
  uint8_t peer_addr_type = BLE_ADDR_PUBLIC;
  uint8_t own_addr_type = BLE_ADDR_PUBLIC;

  /* There can be only one BLE connection request outstanding at a time */
  if (p_dev_rec == NULL) {
    L2CAP_TRACE_WARNING("unknown device, can not initate connection");
    return (false);
  }

  scan_int = (p_cb->scan_int == BTM_BLE_SCAN_PARAM_UNDEF)
                 ? BTM_BLE_SCAN_FAST_INT
                 : p_cb->scan_int;
  scan_win = (p_cb->scan_win == BTM_BLE_SCAN_PARAM_UNDEF)
                 ? BTM_BLE_SCAN_FAST_WIN
                 : p_cb->scan_win;

  peer_addr_type = p_lcb->ble_addr_type;
  peer_addr = p_lcb->remote_bd_addr;

#if (BLE_PRIVACY_SPT == TRUE)
  own_addr_type =
      btm_cb.ble_ctr_cb.privacy_mode ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
  if (p_dev_rec->ble.in_controller_list & BTM_RESOLVING_LIST_BIT) {
    if (btm_cb.ble_ctr_cb.privacy_mode >= BTM_PRIVACY_1_2)
      own_addr_type |= BLE_ADDR_TYPE_ID_BIT;

    btm_ble_enable_resolving_list(BTM_BLE_RL_INIT);
    btm_random_pseudo_to_identity_addr(&peer_addr, &peer_addr_type);
  } else {
    btm_ble_disable_resolving_list(BTM_BLE_RL_INIT, true);

    // If we have a current RPA, use that instead.
    if (!p_dev_rec->ble.cur_rand_addr.IsEmpty()) {
      peer_addr = p_dev_rec->ble.cur_rand_addr;
    }
  }
#endif

  if (!btm_ble_topology_check(BTM_BLE_STATE_INIT)) {
    l2cu_release_lcb(p_lcb);
    L2CAP_TRACE_ERROR("initate direct connection fail, topology limitation");
    return false;
  }

  btm_send_hci_create_connection(
      scan_int,       /* uint16_t scan_int      */
      scan_win,       /* uint16_t scan_win      */
      false,          /* uint8_t white_list     */
      peer_addr_type, /* uint8_t addr_type_peer */
      peer_addr,      /* BD_ADDR bda_peer     */
      own_addr_type,  /* uint8_t addr_type_own  */
      (uint16_t)(
          (p_dev_rec->conn_params.min_conn_int != BTM_BLE_CONN_PARAM_UNDEF)
              ? p_dev_rec->conn_params.min_conn_int
              : BTM_BLE_CONN_INT_MIN_DEF), /* uint16_t conn_int_min  */
      (uint16_t)(
          (p_dev_rec->conn_params.max_conn_int != BTM_BLE_CONN_PARAM_UNDEF)
              ? p_dev_rec->conn_params.max_conn_int
              : BTM_BLE_CONN_INT_MAX_DEF), /* uint16_t conn_int_max  */
      (uint16_t)(
          (p_dev_rec->conn_params.slave_latency != BTM_BLE_CONN_PARAM_UNDEF)
              ? p_dev_rec->conn_params.slave_latency
              : BTM_BLE_CONN_SLAVE_LATENCY_DEF), /* uint16_t conn_latency  */
      (uint16_t)(
          (p_dev_rec->conn_params.supervision_tout != BTM_BLE_CONN_PARAM_UNDEF)
              ? p_dev_rec->conn_params.supervision_tout
              : BTM_BLE_CONN_TIMEOUT_DEF), /* conn_timeout */
      0,                                   /* uint16_t min_len       */
      0,                                   /* uint16_t max_len       */
      p_lcb->initiating_phys);

  p_lcb->link_state = LST_CONNECTING;
  l2cb.is_ble_connecting = true;
  l2cb.ble_connecting_bda = p_lcb->remote_bd_addr;
  alarm_set_on_mloop(p_lcb->l2c_lcb_timer, L2CAP_BLE_LINK_CONNECT_TIMEOUT_MS,
                     l2c_lcb_timer_timeout, p_lcb);
  btm_ble_set_conn_st(BLE_DIR_CONN);

  return (true);
}

/*******************************************************************************
 *
 * Function         l2cble_create_conn
 *
 * Description      This function initiates an acl connection via HCI
 *
 * Returns          true if successful, false if connection not started.
 *
 ******************************************************************************/
bool l2cble_create_conn(tL2C_LCB* p_lcb) {
  tBTM_BLE_CONN_ST conn_st = btm_ble_get_conn_st();
  bool rt = false;

  /* There can be only one BLE connection request outstanding at a time */
  if (conn_st == BLE_CONN_IDLE) {
    rt = l2cble_init_direct_conn(p_lcb);
  } else {
    L2CAP_TRACE_WARNING(
        "L2CAP - LE - cannot start new connection at conn st: %d", conn_st);

    btm_ble_enqueue_direct_conn_req(p_lcb);

    if (conn_st == BLE_BG_CONN) btm_ble_suspend_bg_conn();

    rt = true;
  }
  return rt;
}

/*******************************************************************************
 *
 * Function         l2c_link_processs_ble_num_bufs
 *
 * Description      This function is called when a "controller buffer size"
 *                  event is first received from the controller. It updates
 *                  the L2CAP values.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2c_link_processs_ble_num_bufs(uint16_t num_lm_ble_bufs) {
  if (num_lm_ble_bufs == 0) {
    num_lm_ble_bufs = L2C_DEF_NUM_BLE_BUF_SHARED;
    l2cb.num_lm_acl_bufs -= L2C_DEF_NUM_BLE_BUF_SHARED;
  }

  l2cb.num_lm_ble_bufs = l2cb.controller_le_xmit_window = num_lm_ble_bufs;
}

/*******************************************************************************
 *
 * Function         l2c_ble_link_adjust_allocation
 *
 * Description      This function is called when a link is created or removed
 *                  to calculate the amount of packets each link may send to
 *                  the HCI without an ack coming back.
 *
 *                  Currently, this is a simple allocation, dividing the
 *                  number of Controller Packets by the number of links. In
 *                  the future, QOS configuration should be examined.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2c_ble_link_adjust_allocation(void) {
  uint16_t qq, yy, qq_remainder;
  tL2C_LCB* p_lcb;
  uint16_t hi_quota, low_quota;
  uint16_t num_lowpri_links = 0;
  uint16_t num_hipri_links = 0;
  uint16_t controller_xmit_quota = l2cb.num_lm_ble_bufs;
  uint16_t high_pri_link_quota = L2CAP_HIGH_PRI_MIN_XMIT_QUOTA_A;

  /* If no links active, reset buffer quotas and controller buffers */
  if (l2cb.num_ble_links_active == 0) {
    l2cb.controller_le_xmit_window = l2cb.num_lm_ble_bufs;
    l2cb.ble_round_robin_quota = l2cb.ble_round_robin_unacked = 0;
    return;
  }

  /* First, count the links */
  for (yy = 0, p_lcb = &l2cb.lcb_pool[0]; yy < MAX_L2CAP_LINKS; yy++, p_lcb++) {
    if (p_lcb->in_use && p_lcb->transport == BT_TRANSPORT_LE) {
      if (p_lcb->acl_priority == L2CAP_PRIORITY_HIGH)
        num_hipri_links++;
      else
        num_lowpri_links++;
    }
  }

  /* now adjust high priority link quota */
  low_quota = num_lowpri_links ? 1 : 0;
  while ((num_hipri_links * high_pri_link_quota + low_quota) >
         controller_xmit_quota)
    high_pri_link_quota--;

  /* Work out the xmit quota and buffer quota high and low priorities */
  hi_quota = num_hipri_links * high_pri_link_quota;
  low_quota =
      (hi_quota < controller_xmit_quota) ? controller_xmit_quota - hi_quota : 1;

  /* Work out and save the HCI xmit quota for each low priority link */

  /* If each low priority link cannot have at least one buffer */
  if (num_lowpri_links > low_quota) {
    l2cb.ble_round_robin_quota = low_quota;
    qq = qq_remainder = 0;
  }
  /* If each low priority link can have at least one buffer */
  else if (num_lowpri_links > 0) {
    l2cb.ble_round_robin_quota = 0;
    l2cb.ble_round_robin_unacked = 0;
    qq = low_quota / num_lowpri_links;
    qq_remainder = low_quota % num_lowpri_links;
  }
  /* If no low priority link */
  else {
    l2cb.ble_round_robin_quota = 0;
    l2cb.ble_round_robin_unacked = 0;
    qq = qq_remainder = 0;
  }
  L2CAP_TRACE_EVENT(
      "l2c_ble_link_adjust_allocation  num_hipri: %u  num_lowpri: %u  "
      "low_quota: %u  round_robin_quota: %u  qq: %u",
      num_hipri_links, num_lowpri_links, low_quota, l2cb.ble_round_robin_quota,
      qq);

  /* Now, assign the quotas to each link */
  for (yy = 0, p_lcb = &l2cb.lcb_pool[0]; yy < MAX_L2CAP_LINKS; yy++, p_lcb++) {
    if (p_lcb->in_use && p_lcb->transport == BT_TRANSPORT_LE) {
      if (p_lcb->acl_priority == L2CAP_PRIORITY_HIGH) {
        p_lcb->link_xmit_quota = high_pri_link_quota;
      } else {
        /* Safety check in case we switched to round-robin with something
         * outstanding */
        /* if sent_not_acked is added into round_robin_unacked then don't add it
         * again */
        /* l2cap keeps updating sent_not_acked for exiting from round robin */
        if ((p_lcb->link_xmit_quota > 0) && (qq == 0))
          l2cb.ble_round_robin_unacked += p_lcb->sent_not_acked;

        p_lcb->link_xmit_quota = qq;
        if (qq_remainder > 0) {
          p_lcb->link_xmit_quota++;
          qq_remainder--;
        }
      }

      L2CAP_TRACE_EVENT(
          "l2c_ble_link_adjust_allocation LCB %d   Priority: %d  XmitQuota: %d",
          yy, p_lcb->acl_priority, p_lcb->link_xmit_quota);

      L2CAP_TRACE_EVENT("        SentNotAcked: %d  RRUnacked: %d",
                        p_lcb->sent_not_acked, l2cb.round_robin_unacked);

      /* There is a special case where we have readjusted the link quotas and */
      /* this link may have sent anything but some other link sent packets so */
      /* so we may need a timer to kick off this link's transmissions. */
      if ((p_lcb->link_state == LST_CONNECTED) &&
          (!list_is_empty(p_lcb->link_xmit_data_q)) &&
          (p_lcb->sent_not_acked < p_lcb->link_xmit_quota)) {
        alarm_set_on_mloop(p_lcb->l2c_lcb_timer,
                           L2CAP_LINK_FLOW_CONTROL_TIMEOUT_MS,
                           l2c_lcb_timer_timeout, p_lcb);
      }
    }
  }
}

#if (BLE_LLT_INCLUDED == TRUE)
/*******************************************************************************
 *
 * Function         l2cble_process_rc_param_request_evt
 *
 * Description      process LE Remote Connection Parameter Request Event.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_process_rc_param_request_evt(uint16_t handle, uint16_t int_min,
                                         uint16_t int_max, uint16_t latency,
                                         uint16_t timeout) {
  tL2C_LCB* p_lcb = l2cu_find_lcb_by_handle(handle);

  if (p_lcb != NULL) {
    p_lcb->min_interval = int_min;
    p_lcb->max_interval = int_max;
    p_lcb->latency = latency;
    p_lcb->timeout = timeout;

    /* if update is enabled, always accept connection parameter update */
    if ((p_lcb->conn_update_mask & L2C_BLE_CONN_UPDATE_DISABLE) == 0) {
      btsnd_hcic_ble_rc_param_req_reply(handle, int_min, int_max, latency,
                                        timeout, 0, 0);
    } else {
      L2CAP_TRACE_EVENT("L2CAP - LE - update currently disabled");
      p_lcb->conn_update_mask |= L2C_BLE_NEW_CONN_PARAM;
      btsnd_hcic_ble_rc_param_req_neg_reply(handle,
                                            HCI_ERR_UNACCEPT_CONN_INTERVAL);
    }

  } else {
    L2CAP_TRACE_WARNING("No link to update connection parameter")
  }
}
#endif

/*******************************************************************************
 *
 * Function         l2cble_update_data_length
 *
 * Description      This function update link tx data length if applicable
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_update_data_length(tL2C_LCB* p_lcb) {
  uint16_t tx_mtu = 0;
  uint16_t i = 0;

  L2CAP_TRACE_DEBUG("%s", __func__);

  /* See if we have a link control block for the connection */
  if (p_lcb == NULL) return;

  for (i = 0; i < L2CAP_NUM_FIXED_CHNLS; i++) {
    if (i + L2CAP_FIRST_FIXED_CHNL != L2CAP_BLE_SIGNALLING_CID) {
      if ((p_lcb->p_fixed_ccbs[i] != NULL) &&
          (tx_mtu < (p_lcb->p_fixed_ccbs[i]->tx_data_len + L2CAP_PKT_OVERHEAD)))
        tx_mtu = p_lcb->p_fixed_ccbs[i]->tx_data_len + L2CAP_PKT_OVERHEAD;
    }
  }

  if (tx_mtu > BTM_BLE_DATA_SIZE_MAX) tx_mtu = BTM_BLE_DATA_SIZE_MAX;

  /* update TX data length if changed */
  if (p_lcb->tx_data_len != tx_mtu)
    BTM_SetBleDataLength(p_lcb->remote_bd_addr, tx_mtu);
}

/*******************************************************************************
 *
 * Function         l2cble_process_data_length_change_evt
 *
 * Description      This function process the data length change event
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_process_data_length_change_event(uint16_t handle,
                                             uint16_t tx_data_len,
                                             uint16_t rx_data_len) {
  tL2C_LCB* p_lcb = l2cu_find_lcb_by_handle(handle);

  L2CAP_TRACE_DEBUG("%s TX data len = %d", __func__, tx_data_len);
  if (p_lcb == NULL) return;

  if (tx_data_len > 0) p_lcb->tx_data_len = tx_data_len;

  /* ignore rx_data len for now */
}

/*******************************************************************************
 *
 * Function         l2cble_set_fixed_channel_tx_data_length
 *
 * Description      This function update max fixed channel tx data length if
 *                  applicable
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_set_fixed_channel_tx_data_length(const RawAddress& remote_bda,
                                             uint16_t fix_cid,
                                             uint16_t tx_mtu) {
  tL2C_LCB* p_lcb = l2cu_find_lcb_by_bd_addr(remote_bda, BT_TRANSPORT_LE);
  uint16_t cid = fix_cid - L2CAP_FIRST_FIXED_CHNL;

  L2CAP_TRACE_DEBUG("%s TX MTU = %d", __func__, tx_mtu);

  if (!controller_get_interface()->supports_ble_packet_extension()) {
    L2CAP_TRACE_WARNING("%s, request not supported", __func__);
    return;
  }

  /* See if we have a link control block for the connection */
  if (p_lcb == NULL) return;

  if (p_lcb->p_fixed_ccbs[cid] != NULL) {
    if (tx_mtu > BTM_BLE_DATA_SIZE_MAX) tx_mtu = BTM_BLE_DATA_SIZE_MAX;

    p_lcb->p_fixed_ccbs[cid]->tx_data_len = tx_mtu;
  }

  l2cble_update_data_length(p_lcb);
}

/*******************************************************************************
 *
 * Function         l2cble_credit_based_conn_req
 *
 * Description      This function sends LE Credit Based Connection Request for
 *                  LE connection oriented channels.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_credit_based_conn_req(tL2C_CCB* p_ccb) {
  if (!p_ccb) return;

  if (p_ccb->p_lcb && p_ccb->p_lcb->transport != BT_TRANSPORT_LE) {
    L2CAP_TRACE_WARNING("LE link doesn't exist");
    return;
  }

  l2cu_send_peer_ble_credit_based_conn_req(p_ccb);
  return;
}

/*******************************************************************************
 *
 * Function         l2cble_credit_based_conn_res
 *
 * Description      This function sends LE Credit Based Connection Response for
 *                  LE connection oriented channels.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_credit_based_conn_res(tL2C_CCB* p_ccb, uint16_t result) {
  if (!p_ccb) return;

  if (p_ccb->p_lcb && p_ccb->p_lcb->transport != BT_TRANSPORT_LE) {
    L2CAP_TRACE_WARNING("LE link doesn't exist");
    return;
  }

  l2cu_send_peer_ble_credit_based_conn_res(p_ccb, result);
  return;
}

/*******************************************************************************
 *
 * Function         l2cble_send_flow_control_credit
 *
 * Description      This function sends flow control credits for
 *                  LE connection oriented channels.
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_send_flow_control_credit(tL2C_CCB* p_ccb, uint16_t credit_value) {
  if (!p_ccb) return;

  if (p_ccb->p_lcb && p_ccb->p_lcb->transport != BT_TRANSPORT_LE) {
    L2CAP_TRACE_WARNING("LE link doesn't exist");
    return;
  }

  l2cu_send_peer_ble_flow_control_credit(p_ccb, credit_value);
  return;
}

/*******************************************************************************
 *
 * Function         l2cble_send_peer_disc_req
 *
 * Description      This function sends disconnect request
 *                  to the peer LE device
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_send_peer_disc_req(tL2C_CCB* p_ccb) {
  L2CAP_TRACE_DEBUG("%s", __func__);
  if (!p_ccb) return;

  if (p_ccb->p_lcb && p_ccb->p_lcb->transport != BT_TRANSPORT_LE) {
    L2CAP_TRACE_WARNING("LE link doesn't exist");
    return;
  }

  l2cu_send_peer_ble_credit_based_disconn_req(p_ccb);
  return;
}

/*******************************************************************************
 *
 * Function         l2cble_sec_comp
 *
 * Description      This function is called when security procedure for an LE
 *                  COC link is done
 *
 * Returns          void
 *
 ******************************************************************************/
void l2cble_sec_comp(const RawAddress* bda, tBT_TRANSPORT transport,
                     void* p_ref_data, uint8_t status) {
  const RawAddress& p_bda = *bda;
  tL2C_LCB* p_lcb = l2cu_find_lcb_by_bd_addr(p_bda, BT_TRANSPORT_LE);
  tL2CAP_SEC_DATA* p_buf = NULL;
  uint8_t sec_flag;
  uint8_t sec_act;

  if (!p_lcb) {
    L2CAP_TRACE_WARNING("%s: security complete for unknown device. bda=%s",
                        __func__, bda->ToString().c_str());
    return;
  }

  sec_act = p_lcb->sec_act;
  p_lcb->sec_act = 0;

  if (!fixed_queue_is_empty(p_lcb->le_sec_pending_q)) {
    p_buf = (tL2CAP_SEC_DATA*)fixed_queue_dequeue(p_lcb->le_sec_pending_q);
    if (!p_buf) {
      L2CAP_TRACE_WARNING(
          "%s Security complete for request not initiated from L2CAP",
          __func__);
      return;
    }

    if (status != BTM_SUCCESS) {
      (*(p_buf->p_callback))(p_bda, BT_TRANSPORT_LE, p_buf->p_ref_data, status);
    } else {
      if (sec_act == BTM_SEC_ENCRYPT_MITM) {
        BTM_GetSecurityFlagsByTransport(p_bda, &sec_flag, transport);
        if (sec_flag & BTM_SEC_FLAG_LKEY_AUTHED)
          (*(p_buf->p_callback))(p_bda, BT_TRANSPORT_LE, p_buf->p_ref_data,
                                 status);
        else {
          L2CAP_TRACE_DEBUG("%s MITM Protection Not present", __func__);
          (*(p_buf->p_callback))(p_bda, BT_TRANSPORT_LE, p_buf->p_ref_data,
                                 BTM_FAILED_ON_SECURITY);
        }
      } else {
        L2CAP_TRACE_DEBUG("%s MITM Protection not required sec_act = %d",
                          __func__, p_lcb->sec_act);

        (*(p_buf->p_callback))(p_bda, BT_TRANSPORT_LE, p_buf->p_ref_data,
                               status);
      }
    }
  } else {
    L2CAP_TRACE_WARNING(
        "%s Security complete for request not initiated from L2CAP", __func__);
    return;
  }
  osi_free(p_buf);

  while (!fixed_queue_is_empty(p_lcb->le_sec_pending_q)) {
    p_buf = (tL2CAP_SEC_DATA*)fixed_queue_dequeue(p_lcb->le_sec_pending_q);

    if (status != BTM_SUCCESS)
      (*(p_buf->p_callback))(p_bda, BT_TRANSPORT_LE, p_buf->p_ref_data, status);
    else
      l2ble_sec_access_req(p_bda, p_buf->psm, p_buf->is_originator,
                           p_buf->p_callback, p_buf->p_ref_data);

    osi_free(p_buf);
  }
}

/*******************************************************************************
 *
 * Function         l2ble_sec_access_req
 *
 * Description      This function is called by LE COC link to meet the
 *                  security requirement for the link
 *
 * Returns          true - security procedures are started
 *                  false - failure
 *
 ******************************************************************************/
bool l2ble_sec_access_req(const RawAddress& bd_addr, uint16_t psm,
                          bool is_originator, tL2CAP_SEC_CBACK* p_callback,
                          void* p_ref_data) {
  L2CAP_TRACE_DEBUG("%s", __func__);
  bool status;
  tL2C_LCB* p_lcb = NULL;

  if (!p_callback) {
    L2CAP_TRACE_ERROR("%s No callback function", __func__);
    return false;
  }

  p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr, BT_TRANSPORT_LE);

  if (!p_lcb) {
    L2CAP_TRACE_ERROR("%s Security check for unknown device", __func__);
    p_callback(bd_addr, BT_TRANSPORT_LE, p_ref_data, BTM_UNKNOWN_ADDR);
    return false;
  }

  tL2CAP_SEC_DATA* p_buf =
      (tL2CAP_SEC_DATA*)osi_malloc((uint16_t)sizeof(tL2CAP_SEC_DATA));
  if (!p_buf) {
    p_callback(bd_addr, BT_TRANSPORT_LE, p_ref_data, BTM_NO_RESOURCES);
    return false;
  }

  p_buf->psm = psm;
  p_buf->is_originator = is_originator;
  p_buf->p_callback = p_callback;
  p_buf->p_ref_data = p_ref_data;
  fixed_queue_enqueue(p_lcb->le_sec_pending_q, p_buf);
  status = btm_ble_start_sec_check(bd_addr, psm, is_originator,
                                   &l2cble_sec_comp, p_ref_data);

  return status;
}

/* This function is called to adjust the connection intervals based on various
 * constraints. For example, when there is at least one Hearing Aid device
 * bonded, the minimum interval is raised. On return, min_interval and
 * max_interval are updated. */
void L2CA_AdjustConnectionIntervals(uint16_t* min_interval,
                                    uint16_t* max_interval,
                                    uint16_t floor_interval) {
  uint16_t phone_min_interval = floor_interval;

  if (HearingAid::GetDeviceCount() > 0) {
    // When there are bonded Hearing Aid devices, we will constrained this
    // minimum interval.
    phone_min_interval = BTM_BLE_CONN_INT_MIN_HEARINGAID;
    L2CAP_TRACE_DEBUG("%s: Have Hearing Aids. Min. interval is set to %d",
                      __func__, phone_min_interval);
  }

  if (*min_interval < phone_min_interval) {
    L2CAP_TRACE_DEBUG("%s: requested min_interval=%d too small. Set to %d",
                      __func__, *min_interval, phone_min_interval);
    *min_interval = phone_min_interval;
  }

  // While this could result in connection parameters that fall
  // outside fo the range requested, this will allow the connection
  // to remain established.
  // In other words, this is a workaround for certain peripherals.
  if (*max_interval < phone_min_interval) {
    L2CAP_TRACE_DEBUG("%s: requested max_interval=%d too small. Set to %d",
                      __func__, *max_interval, phone_min_interval);
    *max_interval = phone_min_interval;
  }
}
