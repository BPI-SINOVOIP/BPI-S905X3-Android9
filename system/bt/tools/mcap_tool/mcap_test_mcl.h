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

#include <vector>

#include "mca_api.h"
#include "mcap_test_mdl.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

class McapMcl {
 public:
  /**
   * A controller for a MCAP Communication Link (MCL)
   * @param mcap_test_interface Underlining interface to Bluetooth stack
   * @param mcap_handle Parent application handle
   * @param peer_bd_addr Peer Bluetooth MAC address
   */
  McapMcl(btmcap_test_interface_t* mcap_test_interface, tMCA_HANDLE mcap_handle,
          const RawAddress& peer_bd_addr);
  /**
   * Connect this MCL's control channel
   * @param ctrl_psm Control channel L2CAP PSM
   * @param sec_mask Security mask
   * @return True on success
   */
  bool Connect(uint16_t ctrl_psm, uint16_t sec_mask);
  /**
   * Disconnect from control channel
   * @return
   */
  bool Disconnect();
  /**
   * Close this MCL connection
   * @return
   */
  bool Close();
  /**
   * Allocate an MCAP Data Link (MDL) object and save to this MCL object
   * @param mdep_handle MDEP handle for data, MDEP must be prior created
   * @param mdl_id Desired MDL ID, user supported
   * @param dep_id Peer MDEP ID
   * @param cfg Configuration flags
   * @return True on success
   */
  McapMdl* AllocateMdl(tMCA_DEP mdep_handle, uint16_t mdl_id, uint8_t dep_id,
                       uint8_t cfg);
  /**
   * Send CREATE_MDL message to peer device, will allocate an MDL if the MDL for
   * corresponding MDL ID was not allocated before
   * @param mdep_handle MDEP handle for data, MDEP must be prior created
   * @param data_psm Data channel L2CAP PSM
   * @param mdl_id Desired MDL ID, user supported
   * @param peer_dep_id Peer MDEP ID
   * @param cfg Configuration flags
   * @param should_connect whether we should connect L2CAP immediately
   * @return True on success
   */
  bool CreateMdl(tMCA_DEP mdep_handle, uint16_t data_psm, uint16_t mdl_id,
                 uint8_t peer_dep_id, uint8_t cfg, bool should_connect);
  /**
   * Configure data channel, unblock any pending MDL L2CAP connection requests
   * @return True on Success
   */
  bool DataChannelConfig();
  /**
   * Respond to CREATE_MDL message received from peer device, will allocate an
   * MDL if the MDL for corresponding MDL ID was not allocated before
   * @param mdep_handle MDEP handle for data, MDEP must be prior created
   * @param mdl_id Desired MDL ID, peer supported
   * @param my_dep_id My MDEP ID
   * @param cfg Configuration flags
   * @return True on success
   */
  bool CreateMdlResponse(tMCA_DEP mdep_handle, uint16_t mdl_id,
                         uint8_t my_dep_id, uint8_t cfg);
  /**
   * Send ABORT_MDL request, aborting all pending CREATE_MDL requests
   * @return
   */
  bool AbortMdl();
  /**
   * Send DELETE_MDL request to remote
   * @param mdl_id None zero value mdl_id, 0xFFFF for all remote MDLs
   * @return True on success
   */
  bool DeleteMdl(uint16_t mdl_id);
  // Simple methods that are self-explanatory
  RawAddress& GetPeerAddress();
  void SetHandle(tMCA_CL handle);
  tMCA_CL GetHandle() const;
  void SetMtu(uint16_t mtu);
  uint16_t GetMtu();
  McapMdl* FindMdlById(uint16_t mdl_id);
  McapMdl* FindMdlByHandle(tMCA_DL mdl_handle);
  void RemoveAllMdl();
  void RemoveMdl(uint16_t mdl_id);
  void ResetAllMdl();
  void ResetMdl(uint16_t mdl_id);
  bool IsConnected();
  int ConnectedMdlCount();
  bool HasAvailableMdl();

 private:
  // Initialized during start up
  btmcap_test_interface_t* _mcap_test_interface;
  tMCA_HANDLE _mcap_handle;
  RawAddress _peer_bd_addr;
  std::vector<McapMdl> _mdl_list;

  // Initialized later
  tMCA_CL _mcl_handle = 0;
  uint16_t _control_mtu = 0;
};

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
