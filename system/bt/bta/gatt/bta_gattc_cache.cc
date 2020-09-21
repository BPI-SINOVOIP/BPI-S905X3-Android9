/******************************************************************************
 *
 *  Copyright 2003-2012 Broadcom Corporation
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
 *  This file contains the GATT client discovery procedures and cache
 *  related functions.
 *
 ******************************************************************************/

#define LOG_TAG "bt_bta_gattc"

#include "bt_target.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bt_common.h"
#include "bta_gattc_int.h"
#include "bta_sys.h"
#include "btm_api.h"
#include "btm_ble_api.h"
#include "btm_int.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "sdp_api.h"
#include "sdpdefs.h"
#include "utl.h"

using bluetooth::Uuid;
using base::StringPrintf;

static void bta_gattc_cache_write(const RawAddress& server_bda,
                                  uint16_t num_attr, tBTA_GATTC_NV_ATTR* attr);
static void bta_gattc_char_dscpt_disc_cmpl(uint16_t conn_id,
                                           tBTA_GATTC_SERV* p_srvc_cb);
static tGATT_STATUS bta_gattc_sdp_service_disc(uint16_t conn_id,
                                               tBTA_GATTC_SERV* p_server_cb);
const tBTA_GATTC_DESCRIPTOR* bta_gattc_get_descriptor_srcb(
    tBTA_GATTC_SERV* p_srcb, uint16_t handle);
tBTA_GATTC_CHARACTERISTIC* bta_gattc_get_characteristic_srcb(
    tBTA_GATTC_SERV* p_srcb, uint16_t handle);

#define BTA_GATT_SDP_DB_SIZE 4096

#define GATT_CACHE_PREFIX "/data/misc/bluetooth/gatt_cache_"
#define GATT_CACHE_VERSION 4

static void bta_gattc_generate_cache_file_name(char* buffer, size_t buffer_len,
                                               const RawAddress& bda) {
  snprintf(buffer, buffer_len, "%s%02x%02x%02x%02x%02x%02x", GATT_CACHE_PREFIX,
           bda.address[0], bda.address[1], bda.address[2], bda.address[3],
           bda.address[4], bda.address[5]);
}

/*****************************************************************************
 *  Constants and data types
 ****************************************************************************/

typedef struct {
  tSDP_DISCOVERY_DB* p_sdp_db;
  uint16_t sdp_conn_id;
} tBTA_GATTC_CB_DATA;

#if (BTA_GATT_DEBUG == TRUE)
/* utility functions */

/* debug function to display the server cache */
static void display_db(const std::vector<tBTA_GATTC_SERVICE>& cache) {
  for (const tBTA_GATTC_SERVICE& service : cache) {
    LOG(ERROR) << "Service: s_handle=" << loghex(service.s_handle)
               << ", e_handle=" << loghex(service.e_handle)
               << ", inst=" << loghex(service.handle)
               << ", uuid=" << service.uuid;

    if (service.characteristics.empty()) {
      LOG(ERROR) << "\t No characteristics";
      continue;
    }

    for (const tBTA_GATTC_CHARACTERISTIC& c : service.characteristics) {
      LOG(ERROR) << "\t Characteristic value_handle=" << loghex(c.value_handle)
                 << ", uuid=" << c.uuid << ", prop=" << loghex(c.properties);

      if (c.descriptors.empty()) {
        LOG(ERROR) << "\t\t No descriptors";
        continue;
      }

      for (const tBTA_GATTC_DESCRIPTOR& d : c.descriptors) {
        LOG(ERROR) << "\t\t Descriptor handle=" << loghex(d.handle)
                   << ", uuid=" << d.uuid;
      }
    }
  }
}

/* debug function to display the server cache */
static void bta_gattc_display_cache_server(
    const std::vector<tBTA_GATTC_SERVICE>& cache) {
  LOG(ERROR) << "<================Start Server Cache =============>";
  display_db(cache);
  LOG(ERROR) << "<================End Server Cache =============>";
  LOG(ERROR) << " ";
}

/** debug function to display the exploration list */
static void bta_gattc_display_explore_record(
    const std::vector<tBTA_GATTC_SERVICE>& cache) {
  LOG(ERROR) << "<================Start Explore Queue =============>";
  display_db(cache);
  LOG(ERROR) << "<================ End Explore Queue =============>";
  LOG(ERROR) << " ";
}
#endif /* BTA_GATT_DEBUG == TRUE */

/*******************************************************************************
 *
 * Function         bta_gattc_init_cache
 *
 * Description      Initialize the database cache and discovery related
 *                  resources.
 *
 * Returns          status
 *
 ******************************************************************************/
tGATT_STATUS bta_gattc_init_cache(tBTA_GATTC_SERV* p_srvc_cb) {
  // clear reallocating
  std::vector<tBTA_GATTC_SERVICE>().swap(p_srvc_cb->srvc_cache);
  std::vector<tBTA_GATTC_SERVICE>().swap(p_srvc_cb->pending_discovery);
  return GATT_SUCCESS;
}

tBTA_GATTC_SERVICE* bta_gattc_find_matching_service(
    std::vector<tBTA_GATTC_SERVICE>& services, uint16_t handle) {
  for (tBTA_GATTC_SERVICE& service : services) {
    if (handle >= service.s_handle && handle <= service.e_handle)
      return &service;
  }

  return nullptr;
}

/** Add a service into GATT database */
static void add_service_to_gatt_db(std::vector<tBTA_GATTC_SERVICE>& gatt_db,
                                   uint16_t s_handle, uint16_t e_handle,
                                   const Uuid& uuid, bool is_primary) {
#if (BTA_GATT_DEBUG == TRUE)
  VLOG(1) << "Add a service into GATT DB";
#endif

  gatt_db.emplace_back(tBTA_GATTC_SERVICE{
      .s_handle = s_handle,
      .e_handle = e_handle,
      .is_primary = is_primary,
      .uuid = uuid,
      .handle = s_handle,
  });
}

/** Add a characteristic into GATT database */
static void add_characteristic_to_gatt_db(
    std::vector<tBTA_GATTC_SERVICE>& gatt_db, uint16_t attr_handle,
    uint16_t value_handle, const Uuid& uuid, uint8_t property) {
#if (BTA_GATT_DEBUG == TRUE)
  VLOG(1) << __func__
          << ": Add a characteristic into service. handle:" << +value_handle
          << " uuid:" << uuid << " property=0x" << std::hex << +property;
#endif

  tBTA_GATTC_SERVICE* service =
      bta_gattc_find_matching_service(gatt_db, attr_handle);
  if (!service) {
    LOG(ERROR) << "Illegal action to add char/descr/incl srvc for non-existing "
                  "service!";
    return;
  }

  /* TODO(jpawlowski): We should use attribute handle, not value handle to refer
     to characteristic.
     This is just a temporary workaround.
  */
  if (service->e_handle < value_handle) service->e_handle = value_handle;

  service->characteristics.emplace_back(
      tBTA_GATTC_CHARACTERISTIC{.declaration_handle = attr_handle,
                                .value_handle = value_handle,
                                .properties = property,
                                .uuid = uuid});
  return;
}

/* Add an descriptor into database cache buffer */
static void add_descriptor_to_gatt_db(std::vector<tBTA_GATTC_SERVICE>& gatt_db,
                                      uint16_t handle, const Uuid& uuid) {
#if (BTA_GATT_DEBUG == TRUE)
  VLOG(1) << __func__ << ": add descriptor, handle=" << loghex(handle)
          << ", uuid=" << uuid;
#endif

  tBTA_GATTC_SERVICE* service =
      bta_gattc_find_matching_service(gatt_db, handle);
  if (!service) {
    LOG(ERROR) << "Illegal action to add descriptor for non-existing service!";
    return;
  }

  if (service->characteristics.empty()) {
    LOG(ERROR) << __func__
               << ": Illegal action to add descriptor before adding a "
                  "characteristic!";
    return;
  }

  tBTA_GATTC_CHARACTERISTIC* char_node = &service->characteristics.front();
  for (auto it = service->characteristics.begin();
       it != service->characteristics.end(); it++) {
    if (it->value_handle > handle) break;
    char_node = &(*it);
  }

  char_node->descriptors.emplace_back(
      tBTA_GATTC_DESCRIPTOR{.handle = handle, .uuid = uuid});
}

/* Add an attribute into database cache buffer */
static void add_incl_srvc_to_gatt_db(std::vector<tBTA_GATTC_SERVICE>& gatt_db,
                                     uint16_t handle, const Uuid& uuid,
                                     uint16_t incl_srvc_s_handle) {
#if (BTA_GATT_DEBUG == TRUE)
  VLOG(1) << __func__ << ": add included service, handle=" << loghex(handle)
          << ", uuid=" << uuid;
#endif

  tBTA_GATTC_SERVICE* service =
      bta_gattc_find_matching_service(gatt_db, handle);
  if (!service) {
    LOG(ERROR) << "Illegal action to add incl srvc for non-existing service!";
    return;
  }

  tBTA_GATTC_SERVICE* included_service =
      bta_gattc_find_matching_service(gatt_db, incl_srvc_s_handle);
  if (!included_service) {
    LOG(ERROR) << __func__
               << ": Illegal action to add non-existing included service!";
    return;
  }

  service->included_svc.emplace_back(tBTA_GATTC_INCLUDED_SVC{
      .handle = handle,
      .uuid = uuid,
      .owning_service = service,
      .included_service = included_service,
  });
}

/** Start primary service discovery */
tGATT_STATUS bta_gattc_discover_pri_service(uint16_t conn_id,
                                            tBTA_GATTC_SERV* p_server_cb,
                                            uint8_t disc_type) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);
  if (!p_clcb) return GATT_ERROR;

  if (p_clcb->transport == BTA_TRANSPORT_LE) {
    tGATT_DISC_PARAM param{.s_handle = 0x0001, .e_handle = 0xFFFF};
    return GATTC_Discover(conn_id, disc_type, &param);
  }

  return bta_gattc_sdp_service_disc(conn_id, p_server_cb);
}

/** Start discovery for characteristic descriptor */
void bta_gattc_start_disc_char_dscp(uint16_t conn_id,
                                    tBTA_GATTC_SERV* p_srvc_cb) {
  VLOG(1) << "starting discover characteristics descriptor";
  auto& characteristic = p_srvc_cb->pending_char;

  uint16_t end_handle = 0xFFFF;
  // if there are more characteristics in the service
  if (std::next(p_srvc_cb->pending_char) !=
      p_srvc_cb->pending_service->characteristics.end()) {
    // end at beginning of next characteristic
    end_handle = std::next(p_srvc_cb->pending_char)->declaration_handle - 1;
  } else {
    // end at the end of current service
    end_handle = p_srvc_cb->pending_service->e_handle;
  }

  tGATT_DISC_PARAM param{
      .s_handle = (uint16_t)(characteristic->value_handle + 1),
      .e_handle = end_handle};
  if (GATTC_Discover(conn_id, GATT_DISC_CHAR_DSCPT, &param) != 0) {
    bta_gattc_char_dscpt_disc_cmpl(conn_id, p_srvc_cb);
  }
}

/** process the service discovery complete event */
static void bta_gattc_explore_srvc(uint16_t conn_id,
                                   tBTA_GATTC_SERV* p_srvc_cb) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);
  if (!p_clcb) {
    LOG(ERROR) << "unknown conn_id=" << +conn_id;
    return;
  }

  /* start expore a service if there is service not been explored */
  if (p_srvc_cb->pending_service != p_srvc_cb->pending_discovery.end()) {
    auto& service = *p_srvc_cb->pending_service;
    VLOG(1) << "Start service discovery";

    /* start discovering included services */
    tGATT_DISC_PARAM param = {.s_handle = service.s_handle,
                              .e_handle = service.e_handle};
    GATTC_Discover(conn_id, GATT_DISC_INC_SRVC, &param);
    return;
  }

  /* no service found at all, the end of server discovery*/
  LOG(INFO) << __func__ << ": no more services found";

  p_srvc_cb->srvc_cache.swap(p_srvc_cb->pending_discovery);
  std::vector<tBTA_GATTC_SERVICE>().swap(p_srvc_cb->pending_discovery);

#if (BTA_GATT_DEBUG == TRUE)
  bta_gattc_display_cache_server(p_srvc_cb->srvc_cache);
#endif
  /* save cache to NV */
  p_clcb->p_srcb->state = BTA_GATTC_SERV_SAVE;

  if (btm_sec_is_a_bonded_dev(p_srvc_cb->server_bda)) {
    bta_gattc_cache_save(p_clcb->p_srcb, p_clcb->bta_conn_id);
  }

  bta_gattc_reset_discover_st(p_clcb->p_srcb, GATT_SUCCESS);
}

/** process the char descriptor discovery complete event */
static void bta_gattc_char_dscpt_disc_cmpl(uint16_t conn_id,
                                           tBTA_GATTC_SERV* p_srvc_cb) {
  ++p_srvc_cb->pending_char;
  if (p_srvc_cb->pending_char !=
      p_srvc_cb->pending_service->characteristics.end()) {
    /* start discoverying next characteristic for char descriptor */
    bta_gattc_start_disc_char_dscp(conn_id, p_srvc_cb);
    return;
  }

  /* all characteristic has been explored, start with next service if any */
#if (BTA_GATT_DEBUG == TRUE)
  LOG(ERROR) << "all char has been explored";
#endif
  p_srvc_cb->pending_service++;
  bta_gattc_explore_srvc(conn_id, p_srvc_cb);
}

static bool bta_gattc_srvc_in_list(std::vector<tBTA_GATTC_SERVICE>& services,
                                   uint16_t s_handle, uint16_t e_handle, Uuid) {
  if (!GATT_HANDLE_IS_VALID(s_handle) || !GATT_HANDLE_IS_VALID(e_handle)) {
    LOG(ERROR) << "invalid included service s_handle=" << loghex(s_handle)
               << ", e_handle=" << loghex(e_handle);
    return true;
  }

  for (tBTA_GATTC_SERVICE& service : services) {
    if (service.s_handle == s_handle || service.e_handle == e_handle)
      return true;
  }

  return false;
}

/*******************************************************************************
 *
 * Function         bta_gattc_sdp_callback
 *
 * Description      Process the discovery result from sdp
 *
 * Returns          void
 *
 ******************************************************************************/
void bta_gattc_sdp_callback(uint16_t sdp_status, void* user_data) {
  tBTA_GATTC_CB_DATA* cb_data = (tBTA_GATTC_CB_DATA*)user_data;
  tBTA_GATTC_SERV* p_srvc_cb = bta_gattc_find_scb_by_cid(cb_data->sdp_conn_id);

  if (p_srvc_cb == nullptr) {
    LOG(ERROR) << "GATT service discovery is done on unknown connection";
  } else {
    bool no_pending_disc = p_srvc_cb->pending_discovery.empty();

    if ((sdp_status == SDP_SUCCESS) || (sdp_status == SDP_DB_FULL)) {
      tSDP_DISC_REC* p_sdp_rec = NULL;
      do {
        /* find a service record, report it */
        p_sdp_rec = SDP_FindServiceInDb(cb_data->p_sdp_db, 0, p_sdp_rec);
        if (p_sdp_rec) {
          Uuid service_uuid;
          if (SDP_FindServiceUUIDInRec(p_sdp_rec, &service_uuid)) {
            tSDP_PROTOCOL_ELEM pe;
            if (SDP_FindProtocolListElemInRec(p_sdp_rec, UUID_PROTOCOL_ATT,
                                              &pe)) {
              uint16_t start_handle = (uint16_t)pe.params[0];
              uint16_t end_handle = (uint16_t)pe.params[1];

#if (BTA_GATT_DEBUG == TRUE)
              VLOG(1) << "Found ATT service uuid=" << service_uuid
                      << ", s_handle=" << loghex(start_handle)
                      << ", e_handle=" << loghex(end_handle);
#endif

              if (GATT_HANDLE_IS_VALID(start_handle) &&
                  GATT_HANDLE_IS_VALID(end_handle) && p_srvc_cb != NULL) {
                /* discover services result, add services into a service list */
                add_service_to_gatt_db(p_srvc_cb->pending_discovery,
                                       start_handle, end_handle, service_uuid,
                                       true);
              } else {
                LOG(ERROR) << "invalid start_handle=" << loghex(start_handle)
                           << ", end_handle=" << loghex(end_handle);
              }
            }
          }
        }
      } while (p_sdp_rec);
    }

    if (no_pending_disc) {
      p_srvc_cb->pending_service = p_srvc_cb->pending_discovery.begin();
    }

    /* start discover primary service */
    bta_gattc_explore_srvc(cb_data->sdp_conn_id, p_srvc_cb);
  }

  /* both were allocated in bta_gattc_sdp_service_disc */
  osi_free(cb_data->p_sdp_db);
  osi_free(cb_data);
}
/*******************************************************************************
 *
 * Function         bta_gattc_sdp_service_disc
 *
 * Description      Start DSP Service Discovert
 *
 * Returns          void
 *
 ******************************************************************************/
static tGATT_STATUS bta_gattc_sdp_service_disc(uint16_t conn_id,
                                               tBTA_GATTC_SERV* p_server_cb) {
  uint16_t num_attrs = 2;
  uint16_t attr_list[2];

  /*
   * On success, cb_data will be freed inside bta_gattc_sdp_callback,
   * otherwise it will be freed within this function.
   */
  tBTA_GATTC_CB_DATA* cb_data =
      (tBTA_GATTC_CB_DATA*)osi_malloc(sizeof(tBTA_GATTC_CB_DATA));

  cb_data->p_sdp_db = (tSDP_DISCOVERY_DB*)osi_malloc(BTA_GATT_SDP_DB_SIZE);
  attr_list[0] = ATTR_ID_SERVICE_CLASS_ID_LIST;
  attr_list[1] = ATTR_ID_PROTOCOL_DESC_LIST;

  Uuid uuid = Uuid::From16Bit(UUID_PROTOCOL_ATT);
  SDP_InitDiscoveryDb(cb_data->p_sdp_db, BTA_GATT_SDP_DB_SIZE, 1, &uuid,
                      num_attrs, attr_list);

  if (!SDP_ServiceSearchAttributeRequest2(p_server_cb->server_bda,
                                          cb_data->p_sdp_db,
                                          &bta_gattc_sdp_callback, cb_data)) {
    osi_free(cb_data->p_sdp_db);
    osi_free(cb_data);
    return GATT_ERROR;
  }

  cb_data->sdp_conn_id = conn_id;
  return GATT_SUCCESS;
}

/** callback function to GATT client stack */
void bta_gattc_disc_res_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                              tGATT_DISC_RES* p_data) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);
  tBTA_GATTC_SERV* p_srvc_cb = bta_gattc_find_scb_by_cid(conn_id);

  if (!p_srvc_cb || !p_clcb || p_clcb->state != BTA_GATTC_DISCOVER_ST) return;

  switch (disc_type) {
    case GATT_DISC_SRVC_ALL:
    case GATT_DISC_SRVC_BY_UUID:
      /* discover services result, add services into a service list */
      add_service_to_gatt_db(p_srvc_cb->pending_discovery, p_data->handle,
                             p_data->value.group_value.e_handle,
                             p_data->value.group_value.service_type, true);
      break;

    case GATT_DISC_INC_SRVC:
      /* add included service into service list if it's secondary or it never
         showed up in the primary service search */
      if (!bta_gattc_srvc_in_list(p_srvc_cb->pending_discovery,
                                  p_data->value.incl_service.s_handle,
                                  p_data->value.incl_service.e_handle,
                                  p_data->value.incl_service.service_type)) {
        add_service_to_gatt_db(p_srvc_cb->pending_discovery,
                               p_data->value.incl_service.s_handle,
                               p_data->value.incl_service.e_handle,
                               p_data->value.incl_service.service_type, false);
      }

      /* add into database */
      add_incl_srvc_to_gatt_db(p_srvc_cb->pending_discovery, p_data->handle,
                               p_data->value.incl_service.service_type,
                               p_data->value.incl_service.s_handle);
      break;

    case GATT_DISC_CHAR:
      /* add char value into database */
      add_characteristic_to_gatt_db(p_srvc_cb->pending_discovery,
                                    p_data->handle,
                                    p_data->value.dclr_value.val_handle,
                                    p_data->value.dclr_value.char_uuid,
                                    p_data->value.dclr_value.char_prop);
      break;

    case GATT_DISC_CHAR_DSCPT:
      add_descriptor_to_gatt_db(p_srvc_cb->pending_discovery, p_data->handle,
                                p_data->type);
      break;
  }
}

void bta_gattc_disc_cmpl_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                               tGATT_STATUS status) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);

  if (p_clcb && (status != GATT_SUCCESS || p_clcb->status != GATT_SUCCESS)) {
    if (status == GATT_SUCCESS) p_clcb->status = status;
    bta_gattc_sm_execute(p_clcb, BTA_GATTC_DISCOVER_CMPL_EVT, NULL);
    return;
  }

  tBTA_GATTC_SERV* p_srvc_cb = bta_gattc_find_scb_by_cid(conn_id);
  if (!p_srvc_cb) return;

  switch (disc_type) {
    case GATT_DISC_SRVC_ALL:
    case GATT_DISC_SRVC_BY_UUID:
// definition of all services are discovered, now it's time to discover
// their content
#if (BTA_GATT_DEBUG == TRUE)
      bta_gattc_display_explore_record(p_srvc_cb->pending_discovery);
#endif
      p_srvc_cb->pending_service = p_srvc_cb->pending_discovery.begin();
      bta_gattc_explore_srvc(conn_id, p_srvc_cb);
      break;

    case GATT_DISC_INC_SRVC: {
      auto& service = *p_srvc_cb->pending_service;

      /* start discoverying characteristic */

      tGATT_DISC_PARAM param = {.s_handle = service.s_handle,
                                .e_handle = service.e_handle};
      GATTC_Discover(conn_id, GATT_DISC_CHAR, &param);
      break;
    }

    case GATT_DISC_CHAR: {
#if (BTA_GATT_DEBUG == TRUE)
      bta_gattc_display_explore_record(p_srvc_cb->pending_discovery);
#endif
      auto& service = *p_srvc_cb->pending_service;
      if (!service.characteristics.empty()) {
        /* discover descriptors */
        p_srvc_cb->pending_char = service.characteristics.begin();
        bta_gattc_start_disc_char_dscp(conn_id, p_srvc_cb);
        return;
      }
      /* start next service */
      ++p_srvc_cb->pending_service;
      bta_gattc_explore_srvc(conn_id, p_srvc_cb);
      break;
    }

    case GATT_DISC_CHAR_DSCPT:
      bta_gattc_char_dscpt_disc_cmpl(conn_id, p_srvc_cb);
      break;
  }
}

/** search local cache for matching service record */
void bta_gattc_search_service(tBTA_GATTC_CLCB* p_clcb, Uuid* p_uuid) {
  for (const tBTA_GATTC_SERVICE& service : p_clcb->p_srcb->srvc_cache) {
    if (p_uuid && *p_uuid != service.uuid) continue;

#if (BTA_GATT_DEBUG == TRUE)
    VLOG(1) << __func__ << "found service " << service.uuid
            << ", inst:" << +service.handle << " handle:" << +service.s_handle;
#endif
    if (!p_clcb->p_rcb->p_cback) continue;

    tBTA_GATTC cb_data;
    memset(&cb_data, 0, sizeof(tBTA_GATTC));
    cb_data.srvc_res.conn_id = p_clcb->bta_conn_id;
    cb_data.srvc_res.service_uuid.inst_id = service.handle;
    cb_data.srvc_res.service_uuid.uuid = service.uuid;

    (*p_clcb->p_rcb->p_cback)(BTA_GATTC_SEARCH_RES_EVT, &cb_data);
  }
}

std::vector<tBTA_GATTC_SERVICE>* bta_gattc_get_services_srcb(
    tBTA_GATTC_SERV* p_srcb) {
  if (!p_srcb || p_srcb->srvc_cache.empty()) return NULL;

  return &p_srcb->srvc_cache;
}

std::vector<tBTA_GATTC_SERVICE>* bta_gattc_get_services(uint16_t conn_id) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);

  if (p_clcb == NULL) return NULL;

  tBTA_GATTC_SERV* p_srcb = p_clcb->p_srcb;

  return bta_gattc_get_services_srcb(p_srcb);
}

tBTA_GATTC_SERVICE* bta_gattc_get_service_for_handle_srcb(
    tBTA_GATTC_SERV* p_srcb, uint16_t handle) {
  std::vector<tBTA_GATTC_SERVICE>* services =
      bta_gattc_get_services_srcb(p_srcb);
  if (services == NULL) return NULL;
  return bta_gattc_find_matching_service(*services, handle);
}

const tBTA_GATTC_SERVICE* bta_gattc_get_service_for_handle(uint16_t conn_id,
                                                           uint16_t handle) {
  std::vector<tBTA_GATTC_SERVICE>* services = bta_gattc_get_services(conn_id);
  if (services == NULL) return NULL;

  return bta_gattc_find_matching_service(*services, handle);
}

tBTA_GATTC_CHARACTERISTIC* bta_gattc_get_characteristic_srcb(
    tBTA_GATTC_SERV* p_srcb, uint16_t handle) {
  tBTA_GATTC_SERVICE* service =
      bta_gattc_get_service_for_handle_srcb(p_srcb, handle);

  if (!service) return NULL;

  for (tBTA_GATTC_CHARACTERISTIC& charac : service->characteristics) {
    if (handle == charac.value_handle) return &charac;
  }

  return NULL;
}

tBTA_GATTC_CHARACTERISTIC* bta_gattc_get_characteristic(uint16_t conn_id,
                                                        uint16_t handle) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);

  if (p_clcb == NULL) return NULL;

  tBTA_GATTC_SERV* p_srcb = p_clcb->p_srcb;
  return bta_gattc_get_characteristic_srcb(p_srcb, handle);
}

const tBTA_GATTC_DESCRIPTOR* bta_gattc_get_descriptor_srcb(
    tBTA_GATTC_SERV* p_srcb, uint16_t handle) {
  const tBTA_GATTC_SERVICE* service =
      bta_gattc_get_service_for_handle_srcb(p_srcb, handle);

  if (!service) {
    return NULL;
  }

  for (const tBTA_GATTC_CHARACTERISTIC& charac : service->characteristics) {
    for (const tBTA_GATTC_DESCRIPTOR& desc : charac.descriptors) {
      if (handle == desc.handle) return &desc;
    }
  }

  return NULL;
}

const tBTA_GATTC_DESCRIPTOR* bta_gattc_get_descriptor(uint16_t conn_id,
                                                      uint16_t handle) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);

  if (p_clcb == NULL) return NULL;

  tBTA_GATTC_SERV* p_srcb = p_clcb->p_srcb;
  return bta_gattc_get_descriptor_srcb(p_srcb, handle);
}

tBTA_GATTC_CHARACTERISTIC* bta_gattc_get_owning_characteristic_srcb(
    tBTA_GATTC_SERV* p_srcb, uint16_t handle) {
  tBTA_GATTC_SERVICE* service =
      bta_gattc_get_service_for_handle_srcb(p_srcb, handle);

  if (!service) return NULL;

  for (tBTA_GATTC_CHARACTERISTIC& charac : service->characteristics) {
    for (const tBTA_GATTC_DESCRIPTOR& desc : charac.descriptors) {
      if (handle == desc.handle) return &charac;
    }
  }

  return NULL;
}

const tBTA_GATTC_CHARACTERISTIC* bta_gattc_get_owning_characteristic(
    uint16_t conn_id, uint16_t handle) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);
  if (!p_clcb) return NULL;

  return bta_gattc_get_owning_characteristic_srcb(p_clcb->p_srcb, handle);
}

/*******************************************************************************
 *
 * Function         bta_gattc_fill_gatt_db_el
 *
 * Description      fill a btgatt_db_element_t value
 *
 * Returns          None.
 *
 ******************************************************************************/
void bta_gattc_fill_gatt_db_el(btgatt_db_element_t* p_attr,
                               bt_gatt_db_attribute_type_t type,
                               uint16_t att_handle, uint16_t s_handle,
                               uint16_t e_handle, uint16_t id, const Uuid& uuid,
                               uint8_t prop) {
  p_attr->type = type;
  p_attr->attribute_handle = att_handle;
  p_attr->start_handle = s_handle;
  p_attr->end_handle = e_handle;
  p_attr->id = id;
  p_attr->properties = prop;

  // Permissions are not discoverable using the attribute protocol.
  // Core 5.0, Part F, 3.2.5 Attribute Permissions
  p_attr->permissions = 0;
  p_attr->uuid = uuid;
}

/*******************************************************************************
 * Returns          number of elements inside db from start_handle to end_handle
 ******************************************************************************/
static size_t bta_gattc_get_db_size(
    const std::vector<tBTA_GATTC_SERVICE>& services, uint16_t start_handle,
    uint16_t end_handle) {
  if (services.empty()) return 0;

  size_t db_size = 0;

  for (const tBTA_GATTC_SERVICE& service : services) {
    if (service.s_handle < start_handle) continue;

    if (service.e_handle > end_handle) break;

    db_size++;

    for (const tBTA_GATTC_CHARACTERISTIC& charac : service.characteristics) {
      db_size++;

      db_size += charac.descriptors.size();
    }

    db_size += service.included_svc.size();
  }

  return db_size;
}

/*******************************************************************************
 *
 * Function         bta_gattc_get_gatt_db_impl
 *
 * Description      copy the server GATT database into db parameter.
 *
 * Parameters       p_srvc_cb: server.
 *                  db: output parameter which will contain GATT database copy.
 *                      Caller is responsible for freeing it.
 *                  count: output parameter which will contain number of
 *                  elements in database.
 *
 * Returns          None.
 *
 ******************************************************************************/
static void bta_gattc_get_gatt_db_impl(tBTA_GATTC_SERV* p_srvc_cb,
                                       uint16_t start_handle,
                                       uint16_t end_handle,
                                       btgatt_db_element_t** db, int* count) {
  VLOG(1) << __func__
          << StringPrintf(": start_handle 0x%04x, end_handle 0x%04x",
                          start_handle, end_handle);

  if (p_srvc_cb->srvc_cache.empty()) {
    *count = 0;
    *db = NULL;
    return;
  }

  size_t db_size =
      bta_gattc_get_db_size(p_srvc_cb->srvc_cache, start_handle, end_handle);

  void* buffer = osi_malloc(db_size * sizeof(btgatt_db_element_t));
  btgatt_db_element_t* curr_db_attr = (btgatt_db_element_t*)buffer;

  for (const tBTA_GATTC_SERVICE& service : p_srvc_cb->srvc_cache) {
    if (service.s_handle < start_handle) continue;

    if (service.e_handle > end_handle) break;

    bta_gattc_fill_gatt_db_el(curr_db_attr,
                              service.is_primary ? BTGATT_DB_PRIMARY_SERVICE
                                                 : BTGATT_DB_SECONDARY_SERVICE,
                              0 /* att_handle */, service.s_handle,
                              service.e_handle, service.s_handle, service.uuid,
                              0 /* prop */);
    curr_db_attr++;

    for (const tBTA_GATTC_CHARACTERISTIC& charac : service.characteristics) {
      bta_gattc_fill_gatt_db_el(curr_db_attr, BTGATT_DB_CHARACTERISTIC,
                                charac.value_handle, 0 /* s_handle */,
                                0 /* e_handle */, charac.value_handle,
                                charac.uuid, charac.properties);
      curr_db_attr++;

      for (const tBTA_GATTC_DESCRIPTOR& desc : charac.descriptors) {
        bta_gattc_fill_gatt_db_el(
            curr_db_attr, BTGATT_DB_DESCRIPTOR, desc.handle, 0 /* s_handle */,
            0 /* e_handle */, desc.handle, desc.uuid, 0 /* property */);
        curr_db_attr++;
      }
    }

    for (const tBTA_GATTC_INCLUDED_SVC& p_isvc : service.included_svc) {
      bta_gattc_fill_gatt_db_el(
          curr_db_attr, BTGATT_DB_INCLUDED_SERVICE, p_isvc.handle,
          p_isvc.included_service ? p_isvc.included_service->s_handle : 0,
          0 /* e_handle */, p_isvc.handle, p_isvc.uuid, 0 /* property */);
      curr_db_attr++;
    }
  }

  *db = (btgatt_db_element_t*)buffer;
  *count = db_size;
}

/*******************************************************************************
 *
 * Function         bta_gattc_get_gatt_db
 *
 * Description      copy the server GATT database into db parameter.
 *
 * Parameters       conn_id: connection ID which identify the server.
 *                  db: output parameter which will contain GATT database copy.
 *                      Caller is responsible for freeing it.
 *                  count: number of elements in database.
 *
 * Returns          None.
 *
 ******************************************************************************/
void bta_gattc_get_gatt_db(uint16_t conn_id, uint16_t start_handle,
                           uint16_t end_handle, btgatt_db_element_t** db,
                           int* count) {
  tBTA_GATTC_CLCB* p_clcb = bta_gattc_find_clcb_by_conn_id(conn_id);

  LOG_DEBUG(LOG_TAG, "%s", __func__);
  if (p_clcb == NULL) {
    LOG(ERROR) << "Unknown conn_id=" << loghex(conn_id);
    return;
  }

  if (p_clcb->state != BTA_GATTC_CONN_ST) {
    LOG(ERROR) << "server cache not available, CLCB state=" << +p_clcb->state;
    return;
  }

  if (!p_clcb->p_srcb ||
      !p_clcb->p_srcb->pending_discovery.empty() || /* no active discovery */
      p_clcb->p_srcb->srvc_cache.empty()) {
    LOG(ERROR) << "No server cache available";
    return;
  }

  bta_gattc_get_gatt_db_impl(p_clcb->p_srcb, start_handle, end_handle, db,
                             count);
}

/* rebuild server cache from NV cache */
void bta_gattc_rebuild_cache(tBTA_GATTC_SERV* p_srvc_cb, uint16_t num_attr,
                             tBTA_GATTC_NV_ATTR* p_attr) {
  /* first attribute loading, initialize buffer */
  LOG(INFO) << __func__ << " " << num_attr;

  // clear reallocating
  std::vector<tBTA_GATTC_SERVICE>().swap(p_srvc_cb->srvc_cache);

  while (num_attr > 0 && p_attr != NULL) {
    switch (p_attr->attr_type) {
      case BTA_GATTC_ATTR_TYPE_SRVC:
        add_service_to_gatt_db(p_srvc_cb->srvc_cache, p_attr->s_handle,
                               p_attr->e_handle, p_attr->uuid,
                               p_attr->is_primary);
        break;

      case BTA_GATTC_ATTR_TYPE_CHAR:
        add_characteristic_to_gatt_db(p_srvc_cb->srvc_cache, p_attr->s_handle,
                                      p_attr->s_handle, p_attr->uuid,
                                      p_attr->prop);
        break;

      case BTA_GATTC_ATTR_TYPE_CHAR_DESCR:
        add_descriptor_to_gatt_db(p_srvc_cb->srvc_cache, p_attr->s_handle,
                                  p_attr->uuid);
        break;
      case BTA_GATTC_ATTR_TYPE_INCL_SRVC:
        add_incl_srvc_to_gatt_db(p_srvc_cb->srvc_cache, p_attr->s_handle,
                                 p_attr->uuid, p_attr->incl_srvc_handle);
        break;
    }
    p_attr++;
    num_attr--;
  }
}

/*******************************************************************************
 *
 * Function         bta_gattc_fill_nv_attr
 *
 * Description      fill a NV attribute entry value
 *
 * Returns          None.
 *
 ******************************************************************************/
void bta_gattc_fill_nv_attr(tBTA_GATTC_NV_ATTR* p_attr, uint8_t type,
                            uint16_t s_handle, uint16_t e_handle, Uuid uuid,
                            uint8_t prop, uint16_t incl_srvc_handle,
                            bool is_primary) {
  p_attr->s_handle = s_handle;
  p_attr->e_handle = e_handle;
  p_attr->attr_type = type;
  p_attr->is_primary = is_primary;
  p_attr->id = 0;
  p_attr->prop = prop;
  p_attr->incl_srvc_handle = incl_srvc_handle;
  p_attr->uuid = uuid;
}

/*******************************************************************************
 *
 * Function         bta_gattc_cache_save
 *
 * Description      save the server cache into NV
 *
 * Returns          None.
 *
 ******************************************************************************/
void bta_gattc_cache_save(tBTA_GATTC_SERV* p_srvc_cb, uint16_t conn_id) {
  if (p_srvc_cb->srvc_cache.empty()) return;

  int i = 0;
  size_t db_size = bta_gattc_get_db_size(p_srvc_cb->srvc_cache, 0x0000, 0xFFFF);
  tBTA_GATTC_NV_ATTR* nv_attr =
      (tBTA_GATTC_NV_ATTR*)osi_malloc(db_size * sizeof(tBTA_GATTC_NV_ATTR));

  for (const tBTA_GATTC_SERVICE& service : p_srvc_cb->srvc_cache) {
    bta_gattc_fill_nv_attr(&nv_attr[i++], BTA_GATTC_ATTR_TYPE_SRVC,
                           service.s_handle, service.e_handle, service.uuid,
                           0 /* properties */, 0 /* incl_srvc_handle */,
                           service.is_primary);
  }

  for (const tBTA_GATTC_SERVICE& service : p_srvc_cb->srvc_cache) {
    for (const tBTA_GATTC_CHARACTERISTIC& charac : service.characteristics) {
      bta_gattc_fill_nv_attr(
          &nv_attr[i++], BTA_GATTC_ATTR_TYPE_CHAR, charac.value_handle, 0,
          charac.uuid, charac.properties, 0 /* incl_srvc_handle */, false);

      for (const tBTA_GATTC_DESCRIPTOR& desc : charac.descriptors) {
        bta_gattc_fill_nv_attr(&nv_attr[i++], BTA_GATTC_ATTR_TYPE_CHAR_DESCR,
                               desc.handle, 0, desc.uuid, 0 /* properties */,
                               0 /* incl_srvc_handle */, false);
      }
    }

    for (const tBTA_GATTC_INCLUDED_SVC& p_isvc : service.included_svc) {
      bta_gattc_fill_nv_attr(&nv_attr[i++], BTA_GATTC_ATTR_TYPE_INCL_SRVC,
                             p_isvc.handle, 0, p_isvc.uuid, 0 /* properties */,
                             p_isvc.included_service->s_handle, false);
    }
  }

  bta_gattc_cache_write(p_srvc_cb->server_bda, db_size, nv_attr);
  osi_free(nv_attr);
}

/*******************************************************************************
 *
 * Function         bta_gattc_cache_load
 *
 * Description      Load GATT cache from storage for server.
 *
 * Parameter        p_clcb: pointer to server clcb, that will
 *                          be filled from storage
 * Returns          true on success, false otherwise
 *
 ******************************************************************************/
bool bta_gattc_cache_load(tBTA_GATTC_CLCB* p_clcb) {
  char fname[255] = {0};
  bta_gattc_generate_cache_file_name(fname, sizeof(fname),
                                     p_clcb->p_srcb->server_bda);

  FILE* fd = fopen(fname, "rb");
  if (!fd) {
    LOG(ERROR) << __func__ << ": can't open GATT cache file " << fname
               << " for reading, error: " << strerror(errno);
    return false;
  }

  uint16_t cache_ver = 0;
  tBTA_GATTC_NV_ATTR* attr = NULL;
  bool success = false;
  uint16_t num_attr = 0;

  if (fread(&cache_ver, sizeof(uint16_t), 1, fd) != 1) {
    LOG(ERROR) << __func__ << ": can't read GATT cache version from: " << fname;
    goto done;
  }

  if (cache_ver != GATT_CACHE_VERSION) {
    LOG(ERROR) << __func__ << ": wrong GATT cache version: " << fname;
    goto done;
  }

  if (fread(&num_attr, sizeof(uint16_t), 1, fd) != 1) {
    LOG(ERROR) << __func__
               << ": can't read number of GATT attributes: " << fname;
    goto done;
  }

  attr = (tBTA_GATTC_NV_ATTR*)osi_malloc(sizeof(tBTA_GATTC_NV_ATTR) * num_attr);

  if (fread(attr, sizeof(tBTA_GATTC_NV_ATTR), num_attr, fd) != num_attr) {
    LOG(ERROR) << __func__ << "s: can't read GATT attributes: " << fname;
    goto done;
  }

  bta_gattc_rebuild_cache(p_clcb->p_srcb, num_attr, attr);

  success = true;

done:
  osi_free(attr);
  fclose(fd);
  return success;
}

/*******************************************************************************
 *
 * Function         bta_gattc_cache_write
 *
 * Description      This callout function is executed by GATT when a server
 *                  cache is available to save.
 *
 * Parameter        server_bda: server bd address of this cache belongs to
 *                  num_attr: number of attribute to be save.
 *                  attr: pointer to the list of attributes to save.
 * Returns
 *
 ******************************************************************************/
static void bta_gattc_cache_write(const RawAddress& server_bda,
                                  uint16_t num_attr, tBTA_GATTC_NV_ATTR* attr) {
  char fname[255] = {0};
  bta_gattc_generate_cache_file_name(fname, sizeof(fname), server_bda);

  FILE* fd = fopen(fname, "wb");
  if (!fd) {
    LOG(ERROR) << __func__
               << ": can't open GATT cache file for writing: " << fname;
    return;
  }

  uint16_t cache_ver = GATT_CACHE_VERSION;
  if (fwrite(&cache_ver, sizeof(uint16_t), 1, fd) != 1) {
    LOG(ERROR) << __func__ << ": can't write GATT cache version: " << fname;
    fclose(fd);
    return;
  }

  if (fwrite(&num_attr, sizeof(uint16_t), 1, fd) != 1) {
    LOG(ERROR) << __func__
               << ": can't write GATT cache attribute count: " << fname;
    fclose(fd);
    return;
  }

  if (fwrite(attr, sizeof(tBTA_GATTC_NV_ATTR), num_attr, fd) != num_attr) {
    LOG(ERROR) << __func__ << ": can't write GATT cache attributes: " << fname;
    fclose(fd);
    return;
  }

  fclose(fd);
}

/*******************************************************************************
 *
 * Function         bta_gattc_cache_reset
 *
 * Description      This callout function is executed by GATTC to reset cache in
 *                  application
 *
 * Parameter        server_bda: server bd address of this cache belongs to
 *
 * Returns          void.
 *
 ******************************************************************************/
void bta_gattc_cache_reset(const RawAddress& server_bda) {
  VLOG(1) << __func__;
  char fname[255] = {0};
  bta_gattc_generate_cache_file_name(fname, sizeof(fname), server_bda);
  unlink(fname);
}
