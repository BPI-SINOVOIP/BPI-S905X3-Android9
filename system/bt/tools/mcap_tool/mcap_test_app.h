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

#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <base/logging.h>

#include "mca_api.h"

#include "mcap_test_mcl.h"
#include "mcap_test_mdep.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

extern const tMCA_CHNL_CFG MCAP_TEST_CHANNEL_CONFIG;

class McapTestApp {
 public:
  /**
   * McapTestApp is the root node for an MCAP application
   *
   * @param mcap_test_interface interface to MCAP APIs on Bluetooth stack
   *                            from get_test_interface()
   */
  McapTestApp(btmcap_test_interface_t* mcap_test_interface);
  btmcap_test_interface_t* GetInterface();
  /**
   * Register an application with the Bluetooth stack
   * @param ctrl_psm Control channel L2CAP PSM
   * @param data_psm Data channel L2CAP PSM
   * @param sec_mask Security Mask
   * @param callback Control channel callback
   * @return
   */
  bool Register(uint16_t ctrl_psm, uint16_t data_psm, uint16_t sec_mask,
                tMCA_CTRL_CBACK* callback);
  /**
   * De-register the current application
   */
  void Deregister();
  /**
   * Check if current application is registered
   * @return True if registered
   */
  bool Registered();
  /**
   * Create MCAP Communication Link
   * @param bd_addr Peer Bluetooth Address
   * @param ctrl_psm Control channel L2CAP PSM, should be the same as registered
   *                 value for most cases
   * @param sec_mask Security mask
   * @return True on success
   */
  bool ConnectMcl(const RawAddress& bd_addr, uint16_t ctrl_psm,
                  uint16_t sec_mask);
  /**
   * Create MCAP Data End Point
   * @param type 0 - MCA_TDEP_ECHO, 1 - MCA_TDEP_DATA
   * @param max_mdl Maximum number of data channels for this end point
   * @param data_callback Data callback
   * @return True on success
   */
  bool CreateMdep(uint8_t type, uint8_t max_mdl,
                  tMCA_DATA_CBACK* data_callback);
  // Simple methods that are self-explanatory
  uint8_t GetHandle();
  McapMcl* FindMclByPeerAddress(const RawAddress& bd_addr);
  McapMcl* FindMclByHandle(tMCA_CL mcl_handle);
  McapMdep* FindMdepByHandle(tMCA_DEP mdep_handle);
  void RemoveMclByHandle(tMCA_CL mcl_handle);
  bool IsRegistered();
  /**
   * Callback function for control channel, need to be called by an external
   * function registered during McapTestApp::Register()
   * @param handle MCAP application handle, should be the same as GetHandle()
   * @param mcl MCL handle, FindMclByHandle(mcl) should return non-null value
   * @param event Control event
   * @param p_data Control data
   */
  void ControlCallback(tMCA_HANDLE handle, tMCA_CL mcl, uint8_t event,
                       tMCA_CTRL* p_data);

 private:
  // Initialized during start up
  tMCA_REG _mca_reg;
  btmcap_test_interface_t* _mcap_test_interface = nullptr;
  std::vector<McapMcl> _mcl_list;
  std::vector<McapMdep> _mdep_list;

  // Initialized later
  tMCA_HANDLE _mcap_handle = 0;
};

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
