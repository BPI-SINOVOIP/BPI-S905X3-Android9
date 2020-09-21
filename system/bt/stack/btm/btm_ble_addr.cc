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
 *  This file contains functions for BLE address management.
 *
 ******************************************************************************/

#include <base/bind.h>
#include <string.h>

#include "bt_types.h"
#include "btm_int.h"
#include "btu.h"
#include "device/include/controller.h"
#include "gap_api.h"
#include "hcimsgs.h"

#include "btm_ble_int.h"
#include "smp_api.h"

/*******************************************************************************
 *
 * Function         btm_gen_resolve_paddr_cmpl
 *
 * Description      This is callback functioin when resolvable private address
 *                  generation is complete.
 *
 * Returns          void
 *
 ******************************************************************************/
static void btm_gen_resolve_paddr_cmpl(tSMP_ENC* p) {
  tBTM_LE_RANDOM_CB* p_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
  BTM_TRACE_EVENT("btm_gen_resolve_paddr_cmpl");

  if (p) {
    /* set hash to be LSB of rpAddress */
    p_cb->private_addr.address[5] = p->param_buf[0];
    p_cb->private_addr.address[4] = p->param_buf[1];
    p_cb->private_addr.address[3] = p->param_buf[2];
    /* set it to controller */
    btm_ble_set_random_address(p_cb->private_addr);

    p_cb->own_addr_type = BLE_ADDR_RANDOM;

    /* start a periodical timer to refresh random addr */
    period_ms_t interval_ms = BTM_BLE_PRIVATE_ADDR_INT_MS;
#if (BTM_BLE_CONFORMANCE_TESTING == TRUE)
    interval_ms = btm_cb.ble_ctr_cb.rpa_tout * 1000;
#endif
    alarm_set_on_mloop(p_cb->refresh_raddr_timer, interval_ms,
                       btm_ble_refresh_raddr_timer_timeout, NULL);
  } else {
    /* random address set failure */
    BTM_TRACE_DEBUG("set random address failed");
  }
}
/*******************************************************************************
 *
 * Function         btm_gen_resolve_paddr_low
 *
 * Description      This function is called when random address has generate the
 *                  random number base for low 3 byte bd address.
 *
 * Returns          void
 *
 ******************************************************************************/
void btm_gen_resolve_paddr_low(BT_OCTET8 rand) {
  tBTM_LE_RANDOM_CB* p_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
  tSMP_ENC output;

  BTM_TRACE_EVENT("btm_gen_resolve_paddr_low");
  rand[2] &= (~BLE_RESOLVE_ADDR_MASK);
  rand[2] |= BLE_RESOLVE_ADDR_MSB;

  p_cb->private_addr.address[2] = rand[0];
  p_cb->private_addr.address[1] = rand[1];
  p_cb->private_addr.address[0] = rand[2];

  /* encrypt with ur IRK */
  if (!SMP_Encrypt(btm_cb.devcb.id_keys.irk, BT_OCTET16_LEN, rand, 3,
                   &output)) {
    btm_gen_resolve_paddr_cmpl(NULL);
  } else {
    btm_gen_resolve_paddr_cmpl(&output);
  }
}
/*******************************************************************************
 *
 * Function         btm_gen_resolvable_private_addr
 *
 * Description      This function generate a resolvable private address.
 *
 * Returns          void
 *
 ******************************************************************************/
void btm_gen_resolvable_private_addr(base::Callback<void(BT_OCTET8)> cb) {
  BTM_TRACE_EVENT("%s", __func__);
  /* generate 3B rand as BD LSB, SRK with it, get BD MSB */
  btsnd_hcic_ble_rand(std::move(cb));
}
/*******************************************************************************
 *
 * Function         btm_gen_non_resolve_paddr_cmpl
 *
 * Description      This is the callback function when non-resolvable private
 *                  function is generated and write to controller.
 *
 * Returns          void
 *
 ******************************************************************************/
static void btm_gen_non_resolve_paddr_cmpl(BT_OCTET8 rand) {
  tBTM_LE_RANDOM_CB* p_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
  tBTM_BLE_ADDR_CBACK* p_cback = p_cb->p_generate_cback;
  void* p_data = p_cb->p;
  uint8_t* pp;
  RawAddress static_random;

  BTM_TRACE_EVENT("btm_gen_non_resolve_paddr_cmpl");

  p_cb->p_generate_cback = NULL;
  pp = rand;
  STREAM_TO_BDADDR(static_random, pp);
  /* mask off the 2 MSB */
  static_random.address[0] &= BLE_STATIC_PRIVATE_MSB_MASK;

  /* report complete */
  if (p_cback) (*p_cback)(static_random, p_data);
}
/*******************************************************************************
 *
 * Function         btm_gen_non_resolvable_private_addr
 *
 * Description      This function generate a non-resolvable private address.
 *
 *
 * Returns          void
 *
 ******************************************************************************/
void btm_gen_non_resolvable_private_addr(tBTM_BLE_ADDR_CBACK* p_cback,
                                         void* p) {
  tBTM_LE_RANDOM_CB* p_mgnt_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;

  BTM_TRACE_EVENT("btm_gen_non_resolvable_private_addr");

  if (p_mgnt_cb->p_generate_cback != NULL) return;

  p_mgnt_cb->p_generate_cback = p_cback;
  p_mgnt_cb->p = p;
  btsnd_hcic_ble_rand(base::Bind(&btm_gen_non_resolve_paddr_cmpl));
}

/*******************************************************************************
 *  Utility functions for Random address resolving
 ******************************************************************************/
/*******************************************************************************
 *
 * Function         btm_ble_proc_resolve_x
 *
 * Description      This function compares the X with random address 3 MSO bytes
 *                  to find a match.
 *
 * Returns          true on match, false otherwise
 *
 ******************************************************************************/
static bool btm_ble_proc_resolve_x(const tSMP_ENC& encrypt_output,
                                   const RawAddress& random_bda) {
  BTM_TRACE_EVENT("btm_ble_proc_resolve_x");

  /* compare the hash with 3 LSB of bd address */
  uint8_t comp[3];
  comp[0] = random_bda.address[5];
  comp[1] = random_bda.address[4];
  comp[2] = random_bda.address[3];

  if (!memcmp(encrypt_output.param_buf, comp, 3)) {
    BTM_TRACE_EVENT("match is found");
    return true;
  }

  return false;
}

/*******************************************************************************
 *
 * Function         btm_ble_init_pseudo_addr
 *
 * Description      This function is used to initialize pseudo address.
 *                  If pseudo address is not available, use dummy address
 *
 * Returns          true is updated; false otherwise.
 *
 ******************************************************************************/
bool btm_ble_init_pseudo_addr(tBTM_SEC_DEV_REC* p_dev_rec,
                              const RawAddress& new_pseudo_addr) {
  if (p_dev_rec->ble.pseudo_addr.IsEmpty()) {
    p_dev_rec->ble.pseudo_addr = new_pseudo_addr;
    return true;
  }

  return false;
}

/*******************************************************************************
 *
 * Function         btm_ble_addr_resolvable
 *
 * Description      This function checks if a RPA is resolvable by the device
 *                  key.
 *
 * Returns          true is resolvable; false otherwise.
 *
 ******************************************************************************/
bool btm_ble_addr_resolvable(const RawAddress& rpa,
                             tBTM_SEC_DEV_REC* p_dev_rec) {
  bool rt = false;

  if (!BTM_BLE_IS_RESOLVE_BDA(rpa)) return rt;

  uint8_t rand[3];
  tSMP_ENC output;
  if ((p_dev_rec->device_type & BT_DEVICE_TYPE_BLE) &&
      (p_dev_rec->ble.key_type & BTM_LE_KEY_PID)) {
    BTM_TRACE_DEBUG("%s try to resolve", __func__);
    /* use the 3 MSB of bd address as prand */
    rand[0] = rpa.address[2];
    rand[1] = rpa.address[1];
    rand[2] = rpa.address[0];

    /* generate X = E irk(R0, R1, R2) and R is random address 3 LSO */
    SMP_Encrypt(p_dev_rec->ble.keys.irk, BT_OCTET16_LEN, &rand[0], 3, &output);

    rand[0] = rpa.address[5];
    rand[1] = rpa.address[4];
    rand[2] = rpa.address[3];

    if (!memcmp(output.param_buf, &rand[0], 3)) {
      btm_ble_init_pseudo_addr(p_dev_rec, rpa);
      rt = true;
    }
  }
  return rt;
}

/*******************************************************************************
 *
 * Function         btm_ble_match_random_bda
 *
 * Description      This function match the random address to the appointed
 *                  device record, starting from calculating IRK. If the record
 *                  index exceeds the maximum record number, matching failed and
 *                  send a callback.
 *
 * Returns          None.
 *
 ******************************************************************************/
static bool btm_ble_match_random_bda(void* data, void* context) {
  RawAddress* random_bda = (RawAddress*)context;
  /* use the 3 MSB of bd address as prand */

  uint8_t rand[3];
  rand[0] = random_bda->address[2];
  rand[1] = random_bda->address[1];
  rand[2] = random_bda->address[0];

  BTM_TRACE_EVENT("%s next iteration", __func__);

  tSMP_ENC output;
  tBTM_SEC_DEV_REC* p_dev_rec = static_cast<tBTM_SEC_DEV_REC*>(data);

  BTM_TRACE_DEBUG("sec_flags = %02x device_type = %d", p_dev_rec->sec_flags,
                  p_dev_rec->device_type);

  if (!(p_dev_rec->device_type & BT_DEVICE_TYPE_BLE) ||
      !(p_dev_rec->ble.key_type & BTM_LE_KEY_PID))
    return true;

  /* generate X = E irk(R0, R1, R2) and R is random address 3 LSO */
  SMP_Encrypt(p_dev_rec->ble.keys.irk, BT_OCTET16_LEN, &rand[0], 3, &output);
  // if it was match, finish iteration, otherwise continue
  return !btm_ble_proc_resolve_x(output, *random_bda);
}

/*******************************************************************************
 *
 * Function         btm_ble_resolve_random_addr
 *
 * Description      This function is called to resolve a random address.
 *
 * Returns          pointer to the security record of the device whom a random
 *                  address is matched to.
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_ble_resolve_random_addr(const RawAddress& random_bda) {
  BTM_TRACE_EVENT("%s", __func__);

  /* start to resolve random address */
  /* check for next security record */

  list_node_t* n = list_foreach(btm_cb.sec_dev_rec, btm_ble_match_random_bda,
                                (void*)&random_bda);
  tBTM_SEC_DEV_REC* p_dev_rec = nullptr;
  if (n != nullptr) p_dev_rec = static_cast<tBTM_SEC_DEV_REC*>(list_node(n));

  BTM_TRACE_EVENT("%s:  %sresolved", __func__,
                  (p_dev_rec == nullptr ? "not " : ""));
  return p_dev_rec;
}

/*******************************************************************************
 *  address mapping between pseudo address and real connection address
 ******************************************************************************/
/*******************************************************************************
 *
 * Function         btm_find_dev_by_identity_addr
 *
 * Description      find the security record whose LE static address is matching
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_find_dev_by_identity_addr(const RawAddress& bd_addr,
                                                uint8_t addr_type) {
#if (BLE_PRIVACY_SPT == TRUE)
  list_node_t* end = list_end(btm_cb.sec_dev_rec);
  for (list_node_t* node = list_begin(btm_cb.sec_dev_rec); node != end;
       node = list_next(node)) {
    tBTM_SEC_DEV_REC* p_dev_rec =
        static_cast<tBTM_SEC_DEV_REC*>(list_node(node));
    if (p_dev_rec->ble.static_addr == bd_addr) {
      if ((p_dev_rec->ble.static_addr_type & (~BLE_ADDR_TYPE_ID_BIT)) !=
          (addr_type & (~BLE_ADDR_TYPE_ID_BIT)))
        BTM_TRACE_WARNING(
            "%s find pseudo->random match with diff addr type: %d vs %d",
            __func__, p_dev_rec->ble.static_addr_type, addr_type);

      /* found the match */
      return p_dev_rec;
    }
  }
#endif

  return NULL;
}

/*******************************************************************************
 *
 * Function         btm_identity_addr_to_random_pseudo
 *
 * Description      This function map a static BD address to a pseudo random
 *                  address in security database.
 *
 ******************************************************************************/
bool btm_identity_addr_to_random_pseudo(RawAddress* bd_addr,
                                        uint8_t* p_addr_type, bool refresh) {
#if (BLE_PRIVACY_SPT == TRUE)
  tBTM_SEC_DEV_REC* p_dev_rec =
      btm_find_dev_by_identity_addr(*bd_addr, *p_addr_type);

  BTM_TRACE_EVENT("%s", __func__);
  /* evt reported on static address, map static address to random pseudo */
  if (p_dev_rec != NULL) {
    /* if RPA offloading is supported, or 4.2 controller, do RPA refresh */
    if (refresh &&
        controller_get_interface()->get_ble_resolving_list_max_size() != 0)
      btm_ble_read_resolving_list_entry(p_dev_rec);

    /* assign the original address to be the current report address */
    if (!btm_ble_init_pseudo_addr(p_dev_rec, *bd_addr))
      *bd_addr = p_dev_rec->ble.pseudo_addr;

    *p_addr_type = p_dev_rec->ble.ble_addr_type;
    return true;
  }
#endif
  return false;
}

/*******************************************************************************
 *
 * Function         btm_random_pseudo_to_identity_addr
 *
 * Description      This function map a random pseudo address to a public
 *                  address. random_pseudo is input and output parameter
 *
 ******************************************************************************/
bool btm_random_pseudo_to_identity_addr(RawAddress* random_pseudo,
                                        uint8_t* p_static_addr_type) {
#if (BLE_PRIVACY_SPT == TRUE)
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(*random_pseudo);

  if (p_dev_rec != NULL) {
    if (p_dev_rec->ble.in_controller_list & BTM_RESOLVING_LIST_BIT) {
      *p_static_addr_type = p_dev_rec->ble.static_addr_type;
      *random_pseudo = p_dev_rec->ble.static_addr;
      if (controller_get_interface()->supports_ble_privacy())
        *p_static_addr_type |= BLE_ADDR_TYPE_ID_BIT;
      return true;
    }
  }
#endif
  return false;
}

/*******************************************************************************
 *
 * Function         btm_ble_refresh_peer_resolvable_private_addr
 *
 * Description      This function refresh the currently used resolvable remote
 *                  private address into security database and set active
 *                  connection address.
 *
 ******************************************************************************/
void btm_ble_refresh_peer_resolvable_private_addr(const RawAddress& pseudo_bda,
                                                  const RawAddress& rpa,
                                                  uint8_t rra_type) {
#if (BLE_PRIVACY_SPT == TRUE)
  uint8_t rra_dummy = false;
  if (rpa.IsEmpty()) rra_dummy = true;

  /* update security record here, in adv event or connection complete process */
  tBTM_SEC_DEV_REC* p_sec_rec = btm_find_dev(pseudo_bda);
  if (p_sec_rec != NULL) {
    p_sec_rec->ble.cur_rand_addr = rpa;

    /* unknown, if dummy address, set to static */
    if (rra_type == BTM_BLE_ADDR_PSEUDO)
      p_sec_rec->ble.active_addr_type =
          rra_dummy ? BTM_BLE_ADDR_STATIC : BTM_BLE_ADDR_RRA;
    else
      p_sec_rec->ble.active_addr_type = rra_type;
  } else {
    BTM_TRACE_ERROR("No matching known device in record");
    return;
  }

  BTM_TRACE_DEBUG("%s: active_addr_type: %d ", __func__,
                  p_sec_rec->ble.active_addr_type);

  /* connection refresh remote address */
  tACL_CONN* p_acl = btm_bda_to_acl(p_sec_rec->bd_addr, BT_TRANSPORT_LE);
  if (p_acl == NULL)
    p_acl = btm_bda_to_acl(p_sec_rec->ble.pseudo_addr, BT_TRANSPORT_LE);

  if (p_acl != NULL) {
    if (rra_type == BTM_BLE_ADDR_PSEUDO) {
      /* use static address, resolvable_private_addr is empty */
      if (rra_dummy) {
        p_acl->active_remote_addr_type = p_sec_rec->ble.static_addr_type;
        p_acl->active_remote_addr = p_sec_rec->ble.static_addr;
      } else {
        p_acl->active_remote_addr_type = BLE_ADDR_RANDOM;
        p_acl->active_remote_addr = rpa;
      }
    } else {
      p_acl->active_remote_addr_type = rra_type;
      p_acl->active_remote_addr = rpa;
    }

    BTM_TRACE_DEBUG("p_acl->active_remote_addr_type: %d ",
                    p_acl->active_remote_addr_type);
    VLOG(1) << __func__ << " conn_addr: " << p_acl->active_remote_addr;
  }
#endif
}

/*******************************************************************************
 *
 * Function         btm_ble_refresh_local_resolvable_private_addr
 *
 * Description      This function refresh the currently used resolvable private
 *                  address for the active link to the remote device
 *
 ******************************************************************************/
void btm_ble_refresh_local_resolvable_private_addr(
    const RawAddress& pseudo_addr, const RawAddress& local_rpa) {
#if (BLE_PRIVACY_SPT == TRUE)
  tACL_CONN* p = btm_bda_to_acl(pseudo_addr, BT_TRANSPORT_LE);

  if (p != NULL) {
    if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE) {
      p->conn_addr_type = BLE_ADDR_RANDOM;
      if (!local_rpa.IsEmpty())
        p->conn_addr = local_rpa;
      else
        p->conn_addr = btm_cb.ble_ctr_cb.addr_mgnt_cb.private_addr;
    } else {
      p->conn_addr_type = BLE_ADDR_PUBLIC;
      p->conn_addr = *controller_get_interface()->get_address();
    }
  }
#endif
}
