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
#pragma once

#include "mca_api.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

const tMCA_CHNL_CFG* get_test_channel_config();

class McapMdl {
 public:
  /**
   * An abstraction for the MCAP Data Link (MDL)
   * @param mcap_test_interface Underlining MCAP interface to Bluetooth stack
   * @param mcl_handle Parent MCL handle
   * @param mdep_handle Associated MDEP handle
   * @param mdl_id Desired MDL ID, application supported
   * @param dep_id Peer or self MDEP ID
   * @param cfg Configuration flags
   */
  McapMdl(btmcap_test_interface_t* mcap_test_interface, tMCA_CL mcl_handle,
          tMCA_DEP mdep_handle, uint16_t mdl_id, uint8_t dep_id, uint8_t cfg);
  /**
   * Update this MDL's context so that it can be reused for a new connection
   * This will close this MDL connection at the same time
   * @param mdep_handle Associated MDEP handle
   * @param dep_id Peer or self MDEP ID
   * @param cfg Configuration flags
   * @return True on success
   */
  bool UpdateContext(tMCA_DEP mdep_handle, uint8_t dep_id, uint8_t cfg);
  /**
   * Request to create this MDL to remote device through MCL
   * The create command won't initiate an L2CAP connection unless a non-null
   * config is given
   * @param data_psm Data channel L2CAP PSM
   * @return True on success
   */
  bool Create(uint16_t data_psm, bool should_connect);
  /**
   * Connect this MDL to remote by configuring the data channel
   * @return True on success
   */
  bool Connect();
  /**
   * Close this MDL connection
   * @return True on success
   */
  bool Close();
  /**
   * Request to reconnect connect this MDL to remote device through MCL
   * @param data_psm Data channel L2CAP PSM
   * @return True on success
   */
  bool Reconnect(uint16_t data_psm);
  /**
   * Respond to a reconnect request from peer
   * @return True on success
   */
  bool ReconnectResponse();
  /**
   * Respond to a connect request from peer
   * @return True on success
   */
  bool CreateResponse();
  bool IsConnected();
  int32_t GetResponseCode();
  void SetResponseCode(int32_t rsp_code);
  uint16_t GetId();
  void SetHandle(tMCA_DL mdl_handle);
  tMCA_DL GetHandle();
  void SetMtu(uint16_t mtu);
  uint16_t GetMtu();

 private:
  // Initialized at start up
  btmcap_test_interface_t* _mcap_test_interface;
  tMCA_CL _mcl_handle;
  tMCA_DEP _mdep_handle;
  uint16_t _mdl_id;
  uint8_t _dep_id;
  uint8_t _cfg;

  // Initialized later
  tMCA_DL _mdl_handle = 0;
  uint16_t _data_mtu = 0;
  int32_t _mdl_rsp_code = -1;
};

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
