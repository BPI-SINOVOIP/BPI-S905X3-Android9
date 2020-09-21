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
#include <cstring>

#include <base/logging.h>

#include "mca_api.h"
#include "mca_defs.h"
#include "mcap_test_mcl.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

McapMcl::McapMcl(btmcap_test_interface_t* mcap_test_interface,
                 tMCA_HANDLE mcap_handle, const RawAddress& peer_bd_addr)
    : _mdl_list() {
  _mcap_handle = mcap_handle;
  _mcap_test_interface = mcap_test_interface;
  memcpy(_peer_bd_addr.address, peer_bd_addr.address,
         sizeof(_peer_bd_addr.address));
}

bool McapMcl::Connect(uint16_t ctrl_psm, uint16_t sec_mask) {
  tMCA_RESULT ret = _mcap_test_interface->connect_mcl(
      _mcap_handle, _peer_bd_addr, ctrl_psm, sec_mask);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMcl::Disconnect() {
  if (!IsConnected()) {
    LOG(ERROR) << "MCL is not connected";
    return false;
  }
  tMCA_RESULT ret = _mcap_test_interface->disconnect_mcl(_mcl_handle);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

McapMdl* McapMcl::AllocateMdl(tMCA_DEP mdep_handle, uint16_t mdl_id,
                              uint8_t dep_id, uint8_t cfg) {
  if (!IsConnected()) {
    LOG(ERROR) << "MCL is not connected";
    return nullptr;
  }
  if (FindMdlById(mdl_id) != nullptr) {
    LOG(ERROR) << "mdl_id=" << mdl_id << "already exists";
    return nullptr;
  }
  if (!HasAvailableMdl()) {
    LOG(ERROR) << "No more avaible MDL, currently " << _mdl_list.size();
    return nullptr;
  }
  _mdl_list.push_back(McapMdl(_mcap_test_interface, _mcl_handle, mdep_handle,
                              mdl_id, dep_id, cfg));
  return &_mdl_list[_mdl_list.size() - 1];
}

bool McapMcl::CreateMdl(tMCA_DEP mdep_handle, uint16_t data_psm,
                        uint16_t mdl_id, uint8_t peer_dep_id, uint8_t cfg,
                        bool should_connect) {
  if (!IsConnected()) {
    LOG(ERROR) << "MCL is not connected";
    return false;
  }
  McapMdl* mcap_mdl = FindMdlById(mdl_id);
  if (!mcap_mdl) {
    LOG(INFO) << "mdl_id=" << mdl_id << "does not exists, creating new one";
    mcap_mdl = AllocateMdl(mdep_handle, mdl_id, peer_dep_id, cfg);
    if (!mcap_mdl) {
      return false;
    }
  }
  if (mcap_mdl->IsConnected()) {
    LOG(ERROR) << "mdl_id=" << mdl_id << "is already connected with handle "
               << (int)mcap_mdl->GetHandle();
    return false;
  }
  return mcap_mdl->Create(data_psm, should_connect);
}

bool McapMcl::DataChannelConfig() {
  tMCA_RESULT ret = _mcap_test_interface->data_channel_config(
      _mcl_handle, get_test_channel_config());
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMcl::CreateMdlResponse(tMCA_DEP mdep_handle, uint16_t mdl_id,
                                uint8_t my_dep_id, uint8_t cfg) {
  if (!IsConnected()) {
    LOG(ERROR) << "MCL is not connected";
    return false;
  }
  McapMdl* mcap_mdl = FindMdlById(mdl_id);
  if (!mcap_mdl) {
    LOG(INFO) << "mdl_id=" << mdl_id << " does not exists, creating new one";
    mcap_mdl = AllocateMdl(mdep_handle, mdl_id, my_dep_id, cfg);
    if (!mcap_mdl) {
      LOG(ERROR) << "MDL cannot be created";
      return false;
    }
  }
  if (mcap_mdl->IsConnected()) {
    LOG(INFO) << "mdl_id=" << mdl_id << " is already connected with handle "
              << (int)mcap_mdl->GetHandle() << ", updating context";
    mcap_mdl->UpdateContext(mdep_handle, my_dep_id, cfg);
  }
  return mcap_mdl->CreateResponse();
}

bool McapMcl::AbortMdl() {
  tMCA_RESULT ret = _mcap_test_interface->abort_mdl(_mcl_handle);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMcl::DeleteMdl(uint16_t mdl_id) {
  tMCA_RESULT ret = _mcap_test_interface->delete_mdl(_mcl_handle, mdl_id);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

RawAddress& McapMcl::GetPeerAddress() { return _peer_bd_addr; }

void McapMcl::SetHandle(tMCA_CL handle) { _mcl_handle = handle; }

tMCA_CL McapMcl::GetHandle() const { return _mcl_handle; }

void McapMcl::SetMtu(uint16_t mtu) { _control_mtu = mtu; }

uint16_t McapMcl::GetMtu() { return _control_mtu; }

McapMdl* McapMcl::FindMdlById(uint16_t mdl_id) {
  for (McapMdl& mdl : _mdl_list) {
    if (mdl.GetId() == mdl_id) {
      return &mdl;
    }
  }
  return nullptr;
}

McapMdl* McapMcl::FindMdlByHandle(tMCA_DL mdl_handle) {
  for (McapMdl& mdl : _mdl_list) {
    if (mdl.GetHandle() == mdl_handle) {
      return &mdl;
    }
  }
  return nullptr;
}

void McapMcl::RemoveAllMdl() { _mdl_list.clear(); }

void McapMcl::RemoveMdl(uint16_t mdl_id) {
  LOG(INFO) << "Removing MDL id " << (int)mdl_id;
  for (std::vector<McapMdl>::iterator it = _mdl_list.begin();
       it != _mdl_list.end(); ++it) {
    if (it->GetId() == mdl_id) {
      _mdl_list.erase(it);
      LOG(INFO) << "Removed MDL id " << (int)mdl_id;
      return;
    }
  }
}

void McapMcl::ResetAllMdl() {
  for (McapMdl& mcap_mdl : _mdl_list) {
    mcap_mdl.SetHandle(0);
    mcap_mdl.SetMtu(0);
    mcap_mdl.SetResponseCode(-1);
  }
}

void McapMcl::ResetMdl(uint16_t mdl_id) {
  LOG(INFO) << "Closing MDL id " << (int)mdl_id;
  McapMdl* mcap_mdl = FindMdlById(mdl_id);
  if (!mcap_mdl) {
    LOG(ERROR) << "Cannot find MDL for id " << (int)mdl_id;
    return;
  }
  if (mcap_mdl->IsConnected()) {
    LOG(ERROR) << "MDL " << (int)mdl_id << " is still connected";
    return;
  }
  mcap_mdl->SetHandle(0);
  mcap_mdl->SetMtu(0);
  mcap_mdl->SetResponseCode(-1);
}

bool McapMcl::IsConnected() { return _mcl_handle > 0; }

int McapMcl::ConnectedMdlCount() {
  int count = 0;
  for (McapMdl& mcap_mdl : _mdl_list) {
    if (mcap_mdl.IsConnected()) {
      count++;
    }
  }
  return count;
}

bool McapMcl::HasAvailableMdl() { return ConnectedMdlCount() < MCA_NUM_MDLS; }

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
