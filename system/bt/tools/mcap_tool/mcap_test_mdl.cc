/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <base/logging.h>

#include "mca_defs.h"
#include "mcap_test_mdl.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

/* Test MCAP Channel Configurations */
const tMCA_CHNL_CFG MCAP_TEST_CHANNEL_CONFIG = {
    .fcr_opt =
        {
            L2CAP_FCR_ERTM_MODE,
            MCA_FCR_OPT_TX_WINDOW_SIZE, /* Tx window size */
            /* Maximum transmissions before disconnecting */
            MCA_FCR_OPT_MAX_TX_B4_DISCNT,
            MCA_FCR_OPT_RETX_TOUT,    /* retransmission timeout (2 secs) */
            MCA_FCR_OPT_MONITOR_TOUT, /* Monitor timeout (12 secs) */
            MCA_FCR_OPT_MPS_SIZE,     /* MPS segment size */
        },
    .user_rx_buf_size = BT_DEFAULT_BUFFER_SIZE,
    .user_tx_buf_size = BT_DEFAULT_BUFFER_SIZE,
    .fcr_rx_buf_size = BT_DEFAULT_BUFFER_SIZE,
    .fcr_tx_buf_size = BT_DEFAULT_BUFFER_SIZE,
    .fcs = MCA_FCS_NONE,
    .data_mtu = 572 /* L2CAP MTU of the MCAP data channel */
};

const tMCA_CHNL_CFG* get_test_channel_config() {
  return &MCAP_TEST_CHANNEL_CONFIG;
}

McapMdl::McapMdl(btmcap_test_interface_t* mcap_test_interface,
                 tMCA_CL mcl_handle, tMCA_DEP mdep_handle, uint16_t mdl_id,
                 uint8_t dep_id, uint8_t cfg) {
  _mcap_test_interface = mcap_test_interface;
  _mcl_handle = mcl_handle;
  _mdep_handle = mdep_handle;
  _mdl_id = mdl_id;
  _dep_id = dep_id;
  _cfg = cfg;
}

bool McapMdl::UpdateContext(tMCA_DEP mdep_handle, uint8_t dep_id, uint8_t cfg) {
  if (!_mdl_handle) {
    LOG(ERROR) << "MDL handle not initialized";
  }
  _mdep_handle = mdep_handle;
  _dep_id = dep_id;
  _cfg = cfg;
  tMCA_RESULT ret = _mcap_test_interface->close_mdl_request(_mdl_handle);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  if (ret != MCA_SUCCESS) return false;
  SetHandle(0);
  SetResponseCode(-1);
  SetMtu(0);
  return true;
}

bool McapMdl::Create(uint16_t data_psm, bool should_connect) {
  tMCA_RESULT ret = _mcap_test_interface->create_mdl_request(
      _mcl_handle, _mdep_handle, data_psm, _mdl_id, _dep_id, _cfg,
      should_connect ? &MCAP_TEST_CHANNEL_CONFIG : nullptr);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMdl::Close() {
  if (!_mdl_handle) {
    LOG(ERROR) << "MDL handle not initialized";
    return false;
  }
  tMCA_RESULT ret = _mcap_test_interface->close_mdl_request(_mdl_handle);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMdl::Reconnect(uint16_t data_psm) {
  tMCA_RESULT ret = _mcap_test_interface->reconnect_mdl_request(
      _mcl_handle, _mdep_handle, data_psm, _mdl_id, &MCAP_TEST_CHANNEL_CONFIG);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMdl::ReconnectResponse() {
  tMCA_RESULT ret = _mcap_test_interface->reconnect_mdl_response(
      _mcl_handle, _mdep_handle, _mdl_id, MCA_RSP_SUCCESS,
      &MCAP_TEST_CHANNEL_CONFIG);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMdl::CreateResponse() {
  tMCA_RESULT ret = _mcap_test_interface->create_mdl_response(
      _mcl_handle, _dep_id, _mdl_id, _cfg, MCA_SUCCESS,
      &MCAP_TEST_CHANNEL_CONFIG);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMdl::IsConnected() { return _mdl_handle > 0; }

uint16_t McapMdl::GetId() { return _mdl_id; }

int32_t McapMdl::GetResponseCode() { return _mdl_rsp_code; }

void McapMdl::SetResponseCode(int32_t rsp_code) { _mdl_rsp_code = rsp_code; }

void McapMdl::SetHandle(tMCA_DL mdl_handle) { _mdl_handle = mdl_handle; }

tMCA_DL McapMdl::GetHandle() { return _mdl_handle; }

void McapMdl::SetMtu(uint16_t mtu) { _data_mtu = mtu; }

uint16_t McapMdl::GetMtu() { return _data_mtu; }

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
