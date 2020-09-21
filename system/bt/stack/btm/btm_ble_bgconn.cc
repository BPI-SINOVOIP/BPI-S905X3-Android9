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

/******************************************************************************
 *
 *  This file contains functions for BLE whitelist operation.
 *
 ******************************************************************************/

#include <base/logging.h>
#include <string.h>
#include <unordered_map>

#include "bt_types.h"
#include "bt_utils.h"
#include "btm_int.h"
#include "btu.h"
#include "device/include/controller.h"
#include "hcimsgs.h"
#include "l2c_int.h"
#include "osi/include/allocator.h"
#include "osi/include/osi.h"

#ifndef BTM_BLE_SCAN_PARAM_TOUT
#define BTM_BLE_SCAN_PARAM_TOUT 50 /* 50 seconds */
#endif

static void btm_suspend_wl_activity(tBTM_BLE_WL_STATE wl_state);
static void btm_resume_wl_activity(tBTM_BLE_WL_STATE wl_state);

// Unfortunately (for now?) we have to maintain a copy of the device whitelist
// on the host to determine if a device is pending to be connected or not. This
// controls whether the host should keep trying to scan for whitelisted
// peripherals or not.
// TODO: Move all of this to controller/le/background_list or similar?
typedef struct background_connection_t {
  RawAddress address;
  uint8_t addr_type;

  bool in_controller_wl;
  uint8_t addr_type_in_wl;

  bool pending_removal;
} background_connection_t;

struct BgConnHash {
  bool operator()(const RawAddress& x) const {
    const uint8_t* a = x.address;
    return a[0] ^ (a[1] << 8) ^ (a[2] << 16) ^ (a[3] << 24) ^ a[4] ^
           (a[5] << 8);
  }
};

static std::unordered_map<RawAddress, background_connection_t, BgConnHash>
    background_connections;

static void background_connection_add(uint8_t addr_type,
                                      const RawAddress& address) {
  auto map_iter = background_connections.find(address);
  if (map_iter == background_connections.end()) {
    background_connections[address] =
        background_connection_t{address, addr_type, false, 0, false};
  } else {
    background_connection_t* connection = &map_iter->second;
    connection->addr_type = addr_type;
    connection->pending_removal = false;
  }
}

static void background_connection_remove(const RawAddress& address) {
  auto map_iter = background_connections.find(address);
  if (map_iter != background_connections.end()) {
    if (map_iter->second.in_controller_wl) {
      map_iter->second.pending_removal = true;
    } else {
      background_connections.erase(map_iter);
    }
  }
}

static void background_connections_clear() { background_connections.clear(); }

static bool background_connections_pending() {
  for (auto& map_el : background_connections) {
    background_connection_t* connection = &map_el.second;
    if (connection->pending_removal) continue;
    const bool connected =
        BTM_IsAclConnectionUp(connection->address, BT_TRANSPORT_LE);
    if (!connected) {
      return true;
    }
  }
  return false;
}

static int background_connections_count() {
  int count = 0;
  for (auto& map_el : background_connections) {
    if (!map_el.second.pending_removal) ++count;
  }
  return count;
}

/*******************************************************************************
 *
 * Function         btm_update_scanner_filter_policy
 *
 * Description      This function updates the filter policy of scanner
 ******************************************************************************/
void btm_update_scanner_filter_policy(tBTM_BLE_SFP scan_policy) {
  tBTM_BLE_INQ_CB* p_inq = &btm_cb.ble_ctr_cb.inq_var;

  uint32_t scan_interval =
      !p_inq->scan_interval ? BTM_BLE_GAP_DISC_SCAN_INT : p_inq->scan_interval;
  uint32_t scan_window =
      !p_inq->scan_window ? BTM_BLE_GAP_DISC_SCAN_WIN : p_inq->scan_window;

  BTM_TRACE_EVENT("%s", __func__);

  p_inq->sfp = scan_policy;
  p_inq->scan_type = p_inq->scan_type == BTM_BLE_SCAN_MODE_NONE
                         ? BTM_BLE_SCAN_MODE_ACTI
                         : p_inq->scan_type;

  btm_send_hci_set_scan_params(
      p_inq->scan_type, (uint16_t)scan_interval, (uint16_t)scan_window,
      btm_cb.ble_ctr_cb.addr_mgnt_cb.own_addr_type, scan_policy);
}

/*******************************************************************************
 *
 * Function         btm_ble_bgconn_cancel_if_disconnected
 *
 * Description      If a device has been disconnected, it must be re-added to
 *                  the white list. If needed, this function cancels a pending
 *                  initiate command in order to trigger restart of the initiate
 *                  command which in turn updates the white list.
 *
 * Parameters       bd_addr: updated device
 *
 ******************************************************************************/
void btm_ble_bgconn_cancel_if_disconnected(const RawAddress& bd_addr) {
  if (btm_cb.ble_ctr_cb.conn_state != BLE_BG_CONN) return;

  auto map_it = background_connections.find(bd_addr);
  if (map_it != background_connections.end()) {
    background_connection_t* connection = &map_it->second;
    if (!connection->in_controller_wl && !connection->pending_removal &&
        !BTM_IsAclConnectionUp(bd_addr, BT_TRANSPORT_LE)) {
      btm_ble_start_auto_conn(false);
    }
  }
}

/*******************************************************************************
 *
 * Function         btm_add_dev_to_controller
 *
 * Description      This function load the device into controller white list
 ******************************************************************************/
bool btm_add_dev_to_controller(bool to_add, const RawAddress& bd_addr) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);
  bool started = false;

  if (p_dev_rec != NULL && p_dev_rec->device_type & BT_DEVICE_TYPE_BLE) {
    if (to_add) {
      if (p_dev_rec->ble.ble_addr_type == BLE_ADDR_PUBLIC ||
          !BTM_BLE_IS_RESOLVE_BDA(bd_addr)) {
        background_connection_add(p_dev_rec->ble.ble_addr_type, bd_addr);
        started = true;
        p_dev_rec->ble.in_controller_list |= BTM_WHITE_LIST_BIT;
      } else if (p_dev_rec->ble.static_addr != bd_addr &&
                 !p_dev_rec->ble.static_addr.IsEmpty()) {
        background_connection_add(p_dev_rec->ble.static_addr_type,
                                  p_dev_rec->ble.static_addr);
        started = true;
        p_dev_rec->ble.in_controller_list |= BTM_WHITE_LIST_BIT;
      }
    } else {
      if (p_dev_rec->ble.ble_addr_type == BLE_ADDR_PUBLIC ||
          !BTM_BLE_IS_RESOLVE_BDA(bd_addr)) {
        background_connection_remove(bd_addr);
        started = true;
      }

      if (!p_dev_rec->ble.static_addr.IsEmpty() &&
          p_dev_rec->ble.static_addr != bd_addr) {
        background_connection_remove(p_dev_rec->ble.static_addr);
        started = true;
      }

      p_dev_rec->ble.in_controller_list &= ~BTM_WHITE_LIST_BIT;
    }
  } else {
    /* not a known device, i.e. attempt to connect to device never seen before
     */
    started = true;
    if (to_add)
      background_connection_add(BLE_ADDR_PUBLIC, bd_addr);
    else
      background_connection_remove(bd_addr);
  }

  return started;
}
/*******************************************************************************
 *
 * Function         btm_execute_wl_dev_operation
 *
 * Description      execute the pending whitelist device operation (loading or
 *                                                                  removing)
 ******************************************************************************/
bool btm_execute_wl_dev_operation(void) {
  // handle removals first to avoid filling up controller's white list
  for (auto map_it = background_connections.begin();
       map_it != background_connections.end();) {
    background_connection_t* connection = &map_it->second;
    if (connection->pending_removal) {
      btsnd_hcic_ble_remove_from_white_list(connection->addr_type_in_wl,
                                            connection->address);
      map_it = background_connections.erase(map_it);
    } else
      ++map_it;
  }
  for (auto& map_el : background_connections) {
    background_connection_t* connection = &map_el.second;
    const bool connected =
        BTM_IsAclConnectionUp(connection->address, BT_TRANSPORT_LE);
    if (!connection->in_controller_wl && !connected) {
      btsnd_hcic_ble_add_white_list(connection->addr_type, connection->address);
      connection->in_controller_wl = true;
      connection->addr_type_in_wl = connection->addr_type;
    } else if (connection->in_controller_wl && connected) {
      /* Bluetooth Core 4.2 as well as ESR08 disallows more than one
         connection between two LE addresses. Not all controllers handle this
         correctly, therefore we must make sure connected devices are not in
         the white list when bg connection attempt is active. */
      btsnd_hcic_ble_remove_from_white_list(connection->addr_type_in_wl,
                                            connection->address);
      connection->in_controller_wl = false;
    }
  }
  return true;
}

/*******************************************************************************
 *
 * Function         btm_update_dev_to_white_list
 *
 * Description      This function adds or removes a device into/from
 *                  the white list.
 *
 ******************************************************************************/
bool btm_update_dev_to_white_list(bool to_add, const RawAddress& bd_addr) {
  tBTM_BLE_CB* p_cb = &btm_cb.ble_ctr_cb;

  if (to_add &&
      background_connections_count() ==
          controller_get_interface()->get_ble_white_list_size()) {
    BTM_TRACE_ERROR("%s Whitelist full, unable to add device", __func__);
    return false;
  }

  btm_suspend_wl_activity(p_cb->wl_state);
  btm_add_dev_to_controller(to_add, bd_addr);
  btm_resume_wl_activity(p_cb->wl_state);
  return true;
}

/*******************************************************************************
 *
 * Function         btm_ble_clear_white_list
 *
 * Description      This function clears the white list.
 *
 ******************************************************************************/
void btm_ble_clear_white_list(void) {
  BTM_TRACE_EVENT("btm_ble_clear_white_list");
  btsnd_hcic_ble_clear_white_list();
  background_connections_clear();
}

/*******************************************************************************
 *
 * Function         btm_ble_clear_white_list_complete
 *
 * Description      Indicates white list cleared.
 *
 ******************************************************************************/
void btm_ble_clear_white_list_complete(uint8_t* p_data,
                                       UNUSED_ATTR uint16_t evt_len) {
  uint8_t status;

  STREAM_TO_UINT8(status, p_data);
  BTM_TRACE_EVENT("%s status=%d", __func__, status);
}

/*******************************************************************************
 *
 * Function         btm_ble_white_list_init
 *
 * Description      Initialize white list size
 *
 ******************************************************************************/
void btm_ble_white_list_init(uint8_t white_list_size) {
  BTM_TRACE_DEBUG("%s white_list_size = %d", __func__, white_list_size);
}

/*******************************************************************************
 *
 * Function         btm_ble_add_2_white_list_complete
 *
 * Description      White list element added
 *
 ******************************************************************************/
void btm_ble_add_2_white_list_complete(uint8_t status) {
  BTM_TRACE_EVENT("%s status=%d", __func__, status);
}

/*******************************************************************************
 *
 * Function         btm_ble_remove_from_white_list_complete
 *
 * Description      White list element removal complete
 *
 ******************************************************************************/
void btm_ble_remove_from_white_list_complete(uint8_t* p,
                                             UNUSED_ATTR uint16_t evt_len) {
  BTM_TRACE_EVENT("%s status=%d", __func__, *p);
}

void btm_ble_create_conn_cancel_complete(uint8_t* p) {
  uint8_t status;
  STREAM_TO_UINT8(status, p);

  if (status == HCI_ERR_COMMAND_DISALLOWED) {
    /* This is a sign that logic around keeping connection state is broken */
    LOG(ERROR)
        << "Attempt to cancel LE connection, when no connection is pending.";
    if (btm_ble_get_conn_st() == BLE_CONN_CANCEL) {
      btm_ble_set_conn_st(BLE_CONN_IDLE);
      btm_ble_update_mode_operation(HCI_ROLE_UNKNOWN, nullptr, status);
    }
  }
}

void btm_send_hci_create_connection(
    uint16_t scan_int, uint16_t scan_win, uint8_t init_filter_policy,
    uint8_t addr_type_peer, const RawAddress& bda_peer, uint8_t addr_type_own,
    uint16_t conn_int_min, uint16_t conn_int_max, uint16_t conn_latency,
    uint16_t conn_timeout, uint16_t min_ce_len, uint16_t max_ce_len,
    uint8_t initiating_phys) {
  if (controller_get_interface()->supports_ble_extended_advertising()) {
    EXT_CONN_PHY_CFG phy_cfg[3];  // maximum three phys

    int phy_cnt =
        std::bitset<std::numeric_limits<uint8_t>::digits>(initiating_phys)
            .count();

    LOG_ASSERT(phy_cnt < 3) << "More than three phys provided";
    // TODO(jpawlowski): tune parameters for different transports
    for (int i = 0; i < phy_cnt; i++) {
      phy_cfg[i].scan_int = scan_int;
      phy_cfg[i].scan_win = scan_win;
      phy_cfg[i].conn_int_min = conn_int_min;
      phy_cfg[i].conn_int_max = conn_int_max;
      phy_cfg[i].conn_latency = conn_latency;
      phy_cfg[i].sup_timeout = conn_timeout;
      phy_cfg[i].min_ce_len = min_ce_len;
      phy_cfg[i].max_ce_len = max_ce_len;
    }

    addr_type_peer &= ~BLE_ADDR_TYPE_ID_BIT;
    btsnd_hcic_ble_ext_create_conn(init_filter_policy, addr_type_own,
                                   addr_type_peer, bda_peer, initiating_phys,
                                   phy_cfg);
  } else {
    btsnd_hcic_ble_create_ll_conn(scan_int, scan_win, init_filter_policy,
                                  addr_type_peer, bda_peer, addr_type_own,
                                  conn_int_min, conn_int_max, conn_latency,
                                  conn_timeout, min_ce_len, max_ce_len);
  }
}

/*******************************************************************************
 *
 * Function         btm_ble_start_auto_conn
 *
 * Description      This function is to start/stop auto connection procedure.
 *
 * Parameters       start: true to start; false to stop.
 *
 * Returns          void
 *
 ******************************************************************************/
bool btm_ble_start_auto_conn(bool start) {
  tBTM_BLE_CB* p_cb = &btm_cb.ble_ctr_cb;
  bool exec = true;
  uint16_t scan_int;
  uint16_t scan_win;
  uint8_t own_addr_type = p_cb->addr_mgnt_cb.own_addr_type;
  uint8_t peer_addr_type = BLE_ADDR_PUBLIC;

  uint8_t phy = PHY_LE_1M;
  if (controller_get_interface()->supports_ble_2m_phy()) phy |= PHY_LE_2M;
  if (controller_get_interface()->supports_ble_coded_phy()) phy |= PHY_LE_CODED;

  BTM_TRACE_EVENT("%s start=%d", __func__, start);

  if (start) {
    if (p_cb->conn_state == BLE_CONN_IDLE && background_connections_pending() &&
        btm_ble_topology_check(BTM_BLE_STATE_INIT) && l2cu_can_allocate_lcb()) {
      p_cb->wl_state |= BTM_BLE_WL_INIT;

      btm_execute_wl_dev_operation();

#if (BLE_PRIVACY_SPT == TRUE)
      btm_ble_enable_resolving_list_for_platform(BTM_BLE_RL_INIT);
#endif
      scan_int = (p_cb->scan_int == BTM_BLE_SCAN_PARAM_UNDEF)
                     ? BTM_BLE_SCAN_SLOW_INT_1
                     : p_cb->scan_int;
      scan_win = (p_cb->scan_win == BTM_BLE_SCAN_PARAM_UNDEF)
                     ? BTM_BLE_SCAN_SLOW_WIN_1
                     : p_cb->scan_win;

#if (BLE_PRIVACY_SPT == TRUE)
      if (btm_cb.ble_ctr_cb.rl_state != BTM_BLE_RL_IDLE &&
          controller_get_interface()->supports_ble_privacy()) {
        own_addr_type |= BLE_ADDR_TYPE_ID_BIT;
        peer_addr_type |= BLE_ADDR_TYPE_ID_BIT;
      }
#endif

      btm_send_hci_create_connection(
          scan_int,                       /* uint16_t scan_int      */
          scan_win,                       /* uint16_t scan_win      */
          0x01,                           /* uint8_t white_list     */
          peer_addr_type,                 /* uint8_t addr_type_peer */
          RawAddress::kEmpty,             /* BD_ADDR bda_peer     */
          own_addr_type,                  /* uint8_t addr_type_own */
          BTM_BLE_CONN_INT_MIN_DEF,       /* uint16_t conn_int_min  */
          BTM_BLE_CONN_INT_MAX_DEF,       /* uint16_t conn_int_max  */
          BTM_BLE_CONN_SLAVE_LATENCY_DEF, /* uint16_t conn_latency  */
          BTM_BLE_CONN_TIMEOUT_DEF,       /* uint16_t conn_timeout  */
          0,                              /* uint16_t min_len       */
          0,                              /* uint16_t max_len       */
          phy);
      btm_ble_set_conn_st(BLE_BG_CONN);
    } else {
      exec = false;
    }
  } else {
    if (p_cb->conn_state == BLE_BG_CONN) {
      btsnd_hcic_ble_create_conn_cancel();
      btm_ble_set_conn_st(BLE_CONN_CANCEL);
      p_cb->wl_state &= ~BTM_BLE_WL_INIT;
    } else {
      BTM_TRACE_DEBUG("conn_st = %d, not in auto conn state, cannot stop",
                      p_cb->conn_state);
      exec = false;
    }
  }
  return exec;
}

/*******************************************************************************
 *
 * Function         btm_ble_suspend_bg_conn
 *
 * Description      This function is to suspend an active background connection
 *                  procedure.
 *
 * Parameters       none.
 *
 * Returns          none.
 *
 ******************************************************************************/
bool btm_ble_suspend_bg_conn(void) {
  BTM_TRACE_EVENT("%s", __func__);

  if (btm_cb.ble_ctr_cb.bg_conn_type == BTM_BLE_CONN_AUTO)
    return btm_ble_start_auto_conn(false);

  return false;
}
/*******************************************************************************
 *
 * Function         btm_suspend_wl_activity
 *
 * Description      This function is to suspend white list related activity
 *
 * Returns          none.
 *
 ******************************************************************************/
static void btm_suspend_wl_activity(tBTM_BLE_WL_STATE wl_state) {
  if (wl_state & BTM_BLE_WL_INIT) {
    btm_ble_start_auto_conn(false);
  }
}
/*******************************************************************************
 *
 * Function         btm_resume_wl_activity
 *
 * Description      This function is to resume white list related activity
 *
 * Returns          none.
 *
 ******************************************************************************/
static void btm_resume_wl_activity(tBTM_BLE_WL_STATE wl_state) {
  btm_ble_resume_bg_conn();
}
/*******************************************************************************
 *
 * Function         btm_ble_resume_bg_conn
 *
 * Description      This function is to resume a background auto connection
 *                  procedure.
 *
 * Parameters       none.
 *
 * Returns          none.
 *
 ******************************************************************************/
bool btm_ble_resume_bg_conn(void) {
  tBTM_BLE_CB* p_cb = &btm_cb.ble_ctr_cb;
  if (p_cb->bg_conn_type == BTM_BLE_CONN_AUTO) {
    return btm_ble_start_auto_conn(true);
  }

  return false;
}
/*******************************************************************************
 *
 * Function         btm_ble_get_conn_st
 *
 * Description      This function get BLE connection state
 *
 * Returns          connection state
 *
 ******************************************************************************/
tBTM_BLE_CONN_ST btm_ble_get_conn_st(void) {
  return btm_cb.ble_ctr_cb.conn_state;
}
/*******************************************************************************
 *
 * Function         btm_ble_set_conn_st
 *
 * Description      This function set BLE connection state
 *
 * Returns          None.
 *
 ******************************************************************************/
void btm_ble_set_conn_st(tBTM_BLE_CONN_ST new_st) {
  btm_cb.ble_ctr_cb.conn_state = new_st;

  if (new_st == BLE_BG_CONN || new_st == BLE_DIR_CONN)
    btm_ble_set_topology_mask(BTM_BLE_STATE_INIT_BIT);
  else
    btm_ble_clear_topology_mask(BTM_BLE_STATE_INIT_BIT);
}

/*******************************************************************************
 *
 * Function         btm_ble_enqueue_direct_conn_req
 *
 * Description      This function enqueue the direct connection request
 *
 * Returns          None.
 *
 ******************************************************************************/
void btm_ble_enqueue_direct_conn_req(void* p_param) {
  tBTM_BLE_CONN_REQ* p =
      (tBTM_BLE_CONN_REQ*)osi_malloc(sizeof(tBTM_BLE_CONN_REQ));

  p->p_param = p_param;

  fixed_queue_enqueue(btm_cb.ble_ctr_cb.conn_pending_q, p);
}
/*******************************************************************************
 *
 * Function         btm_ble_dequeue_direct_conn_req
 *
 * Description      This function dequeues the direct connection request
 *
 * Returns          None.
 *
 ******************************************************************************/
void btm_ble_dequeue_direct_conn_req(const RawAddress& rem_bda) {
  if (fixed_queue_is_empty(btm_cb.ble_ctr_cb.conn_pending_q)) return;

  list_t* list = fixed_queue_get_list(btm_cb.ble_ctr_cb.conn_pending_q);
  for (const list_node_t* node = list_begin(list); node != list_end(list);
       node = list_next(node)) {
    tBTM_BLE_CONN_REQ* p_req = (tBTM_BLE_CONN_REQ*)list_node(node);
    tL2C_LCB* p_lcb = (tL2C_LCB*)p_req->p_param;
    if ((p_lcb == NULL) || (!p_lcb->in_use)) {
      continue;
    }
    // If BD address matches
    if (rem_bda == p_lcb->remote_bd_addr) {
      fixed_queue_try_remove_from_queue(btm_cb.ble_ctr_cb.conn_pending_q,
                                        p_req);
      l2cu_release_lcb((tL2C_LCB*)p_req->p_param);
      osi_free((void*)p_req);
      break;
    }
  }
}
/*******************************************************************************
 *
 * Function         btm_send_pending_direct_conn
 *
 * Description      This function send the pending direct connection request in
 *                  queue
 *
 * Returns          true if started, false otherwise
 *
 ******************************************************************************/
bool btm_send_pending_direct_conn(void) {
  tBTM_BLE_CONN_REQ* p_req;
  bool rt = false;

  p_req = (tBTM_BLE_CONN_REQ*)fixed_queue_try_dequeue(
      btm_cb.ble_ctr_cb.conn_pending_q);
  if (p_req != NULL) {
    tL2C_LCB* p_lcb = (tL2C_LCB*)(p_req->p_param);
    /* Ignore entries that might have been released while queued. */
    if (p_lcb->in_use) rt = l2cble_init_direct_conn(p_lcb);
    osi_free(p_req);
  }

  return rt;
}
