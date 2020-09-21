/******************************************************************************
 *
 *  Copyright 2014 The Android Open Source Project
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
 *  This is the implementation of the API for MCE subsystem
 *
 ******************************************************************************/

#include <string.h>

#include "bt_common.h"
#include "bt_types.h"
#include "bta_api.h"
#include "bta_mce_api.h"
#include "bta_mce_int.h"
#include "bta_sys.h"
#include "port_api.h"
#include "sdp_api.h"

/*****************************************************************************
 *  Constants
 ****************************************************************************/

static const tBTA_SYS_REG bta_mce_reg = {bta_mce_sm_execute, NULL};

/*******************************************************************************
 *
 * Function         BTA_MceEnable
 *
 * Description      Enable the MCE I/F service. When the enable
 *                  operation is complete the callback function will be
 *                  called with a BTA_MCE_ENABLE_EVT. This function must
 *                  be called before other functions in the MCE API are
 *                  called.
 *
 * Returns          BTA_MCE_SUCCESS if successful.
 *                  BTA_MCE_FAIL if internal failure.
 *
 ******************************************************************************/
tBTA_MCE_STATUS BTA_MceEnable(tBTA_MCE_DM_CBACK* p_cback) {
  tBTA_MCE_STATUS status = BTA_MCE_FAILURE;

  APPL_TRACE_API("%", __func__);

  if (p_cback && !bta_sys_is_register(BTA_ID_MCE)) {
    memset(&bta_mce_cb, 0, sizeof(tBTA_MCE_CB));

    /* register with BTA system manager */
    bta_sys_register(BTA_ID_MCE, &bta_mce_reg);

    if (p_cback) {
      tBTA_MCE_API_ENABLE* p_buf =
          (tBTA_MCE_API_ENABLE*)osi_malloc(sizeof(tBTA_MCE_API_ENABLE));
      p_buf->hdr.event = BTA_MCE_API_ENABLE_EVT;
      p_buf->p_cback = p_cback;
      bta_sys_sendmsg(p_buf);
      status = BTA_MCE_SUCCESS;
    }
  }

  return status;
}

/*******************************************************************************
 *
 * Function         BTA_MceGetRemoteMasInstances
 *
 * Description      This function performs service discovery for the MAS service
 *                  by the given peer device. When the operation is completed
 *                  the tBTA_MCE_DM_CBACK callback function will be  called with
 *                  a BTA_MCE_MAS_DISCOVERY_COMP_EVT.
 *
 * Returns          BTA_MCE_SUCCESS, if the request is being processed.
 *                  BTA_MCE_FAILURE, otherwise.
 *
 ******************************************************************************/
tBTA_MCE_STATUS BTA_MceGetRemoteMasInstances(const RawAddress& bd_addr) {
  tBTA_MCE_API_GET_REMOTE_MAS_INSTANCES* p_msg =
      (tBTA_MCE_API_GET_REMOTE_MAS_INSTANCES*)osi_malloc(
          sizeof(tBTA_MCE_API_GET_REMOTE_MAS_INSTANCES));

  APPL_TRACE_API("%s", __func__);

  p_msg->hdr.event = BTA_MCE_API_GET_REMOTE_MAS_INSTANCES_EVT;
  p_msg->bd_addr = bd_addr;

  bta_sys_sendmsg(p_msg);

  return BTA_MCE_SUCCESS;
}
