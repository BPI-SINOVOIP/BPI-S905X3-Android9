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
#include "mcap_test_app.h"
#include "mca_defs.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

#define CASE_RETURN_STR(const) \
  case const:                  \
    return #const;

static const char* dump_mcap_events(const uint8_t event) {
  switch (event) {
    CASE_RETURN_STR(MCA_ERROR_RSP_EVT)
    CASE_RETURN_STR(MCA_CREATE_IND_EVT)
    CASE_RETURN_STR(MCA_CREATE_CFM_EVT)
    CASE_RETURN_STR(MCA_RECONNECT_IND_EVT)
    CASE_RETURN_STR(MCA_RECONNECT_CFM_EVT)
    CASE_RETURN_STR(MCA_ABORT_IND_EVT)
    CASE_RETURN_STR(MCA_ABORT_CFM_EVT)
    CASE_RETURN_STR(MCA_DELETE_IND_EVT)
    CASE_RETURN_STR(MCA_DELETE_CFM_EVT)
    CASE_RETURN_STR(MCA_SYNC_CAP_IND_EVT)
    CASE_RETURN_STR(MCA_SYNC_CAP_CFM_EVT)
    CASE_RETURN_STR(MCA_SYNC_SET_IND_EVT)
    CASE_RETURN_STR(MCA_SYNC_SET_CFM_EVT)
    CASE_RETURN_STR(MCA_SYNC_INFO_IND_EVT)
    CASE_RETURN_STR(MCA_CONNECT_IND_EVT)
    CASE_RETURN_STR(MCA_DISCONNECT_IND_EVT)
    CASE_RETURN_STR(MCA_OPEN_IND_EVT)
    CASE_RETURN_STR(MCA_OPEN_CFM_EVT)
    CASE_RETURN_STR(MCA_CLOSE_IND_EVT)
    CASE_RETURN_STR(MCA_CLOSE_CFM_EVT)
    CASE_RETURN_STR(MCA_CONG_CHG_EVT)
    CASE_RETURN_STR(MCA_RSP_TOUT_IND_EVT)
    default:
      return "Unknown event";
  }
}

static void print_mcap_event(const tMCA_DISCONNECT_IND* mcap_disconnect_ind) {
  printf("%s: peer_bd_addr=%s,l2cap_disconnect_reason=0x%04x\n", __func__,
         mcap_disconnect_ind->bd_addr.ToString().c_str(),
         mcap_disconnect_ind->reason);
}

static void print_mcap_event(const tMCA_CONNECT_IND* mcap_connect_ind) {
  printf("%s: peer_bd_addr=%s, peer_mtu=%d \n", __func__,
         mcap_connect_ind->bd_addr.ToString().c_str(), mcap_connect_ind->mtu);
}

static void print_mcap_event(const tMCA_RSP_EVT* mcap_rsp) {
  printf("%s: response, mdl_id=%d, op_code=0x%02x, rsp_code=0x%02x\n", __func__,
         mcap_rsp->mdl_id, mcap_rsp->op_code, mcap_rsp->rsp_code);
}

static void print_mcap_event(const tMCA_EVT_HDR* mcap_evt_hdr) {
  printf("%s: event, mdl_id=%d, op_code=0x%02x\n", __func__,
         mcap_evt_hdr->mdl_id, mcap_evt_hdr->op_code);
}

static void print_mcap_event(const tMCA_CREATE_IND* mcap_create_ind) {
  printf("%s: mdl_id=%d, op_code=0x%02x, dep_id=%d, cfg=0x%02x\n", __func__,
         mcap_create_ind->mdl_id, mcap_create_ind->op_code,
         mcap_create_ind->dep_id, mcap_create_ind->cfg);
}

static void print_mcap_event(const tMCA_CREATE_CFM* mcap_create_cfm) {
  printf("%s: mdl_id=%d, op_code=0x%02x, rsp_code=%d, cfg=0x%02x\n", __func__,
         mcap_create_cfm->mdl_id, mcap_create_cfm->op_code,
         mcap_create_cfm->rsp_code, mcap_create_cfm->cfg);
}

static void print_mcap_event(const tMCA_DL_OPEN* mcap_dl_open) {
  printf("%s: mdl_id=%d, mdl_handle=%d, mtu=%d\n", __func__,
         mcap_dl_open->mdl_id, mcap_dl_open->mdl, mcap_dl_open->mtu);
}

static void print_mcap_event(const tMCA_DL_CLOSE* mcap_dl_close) {
  printf("%s: mdl_id=%d, mdl_handle=%d, l2cap_disconnect_reason=0x%04x\n",
         __func__, mcap_dl_close->mdl_id, mcap_dl_close->mdl,
         mcap_dl_close->reason);
}

static void print_mcap_event(const tMCA_CONG_CHG* mcap_congestion_change) {
  printf("%s: mdl_id=%d, mdl_handle=%d, congested=%d\n", __func__,
         mcap_congestion_change->mdl_id, mcap_congestion_change->mdl,
         mcap_congestion_change->cong);
}

McapTestApp::McapTestApp(btmcap_test_interface_t* mcap_test_interface)
    : _mcl_list(), _mdep_list() {
  _mcap_test_interface = mcap_test_interface;
}

btmcap_test_interface_t* McapTestApp::GetInterface() {
  return _mcap_test_interface;
}

bool McapTestApp::Register(uint16_t ctrl_psm, uint16_t data_psm,
                           uint16_t sec_mask, tMCA_CTRL_CBACK* callback) {
  if (!callback) {
    LOG(ERROR) << "callback is null";
    return false;
  }
  _mca_reg.rsp_tout = 5000;
  _mca_reg.ctrl_psm = ctrl_psm;
  _mca_reg.data_psm = data_psm;
  _mca_reg.sec_mask = sec_mask;
  _mcap_handle =
      _mcap_test_interface->register_application(&_mca_reg, callback);
  return _mcap_handle > 0;
}

void McapTestApp::Deregister() {
  _mcap_test_interface->deregister_application(_mcap_handle);
  _mcap_handle = 0;
}

bool McapTestApp::Registered() { return _mcap_handle > 0; }

bool McapTestApp::ConnectMcl(const RawAddress& bd_addr, uint16_t ctrl_psm,
                             uint16_t sec_mask) {
  if (!Registered()) {
    LOG(ERROR) << "Application not registered";
    return false;
  }
  McapMcl* mcap_mcl = FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(INFO) << "MCL does not exist, creating new MCL";
    _mcl_list.push_back(McapMcl(_mcap_test_interface, _mcap_handle, bd_addr));
    mcap_mcl = &_mcl_list[_mcl_list.size() - 1];
  }
  if (mcap_mcl->GetHandle() != 0) {
    LOG(ERROR) << "MCL is still active, cannot make another connection";
    return false;
  }
  return mcap_mcl->Connect(ctrl_psm, sec_mask);
}

bool McapTestApp::CreateMdep(uint8_t type, uint8_t max_mdl,
                             tMCA_DATA_CBACK* data_callback) {
  if (!data_callback) {
    LOG(ERROR) << "Data callback is null";
    return false;
  }
  _mdep_list.push_back(McapMdep(_mcap_test_interface, _mcap_handle, type,
                                max_mdl, data_callback));
  return _mdep_list[_mdep_list.size() - 1].Create();
}

uint8_t McapTestApp::GetHandle() { return _mcap_handle; }

McapMcl* McapTestApp::FindMclByPeerAddress(const RawAddress& bd_addr) {
  for (McapMcl& mcl : _mcl_list) {
    if (mcl.GetPeerAddress() == bd_addr) {
      return &mcl;
    }
  }
  return nullptr;
}

McapMcl* McapTestApp::FindMclByHandle(tMCA_CL mcl_handle) {
  for (McapMcl& mcl : _mcl_list) {
    if (mcl.GetHandle() == mcl_handle) {
      return &mcl;
    }
  }
  return nullptr;
}

McapMdep* McapTestApp::FindMdepByHandle(tMCA_DEP mdep_handle) {
  for (McapMdep& mdep : _mdep_list) {
    if (mdep.GetHandle() == mdep_handle) {
      return &mdep;
    }
  }
  return nullptr;
}

void McapTestApp::RemoveMclByHandle(tMCA_CL mcl_handle) {
  LOG(INFO) << "Removing MCL handle " << (int)mcl_handle;
  for (std::vector<McapMcl>::iterator it = _mcl_list.begin();
       it != _mcl_list.end(); ++it) {
    if (it->GetHandle() == mcl_handle) {
      _mcl_list.erase(it);
      LOG(INFO) << "Removed MCL handle " << (int)mcl_handle;
      return;
    }
  }
}

bool McapTestApp::IsRegistered() { return _mcap_handle > 0; }

void McapTestApp::ControlCallback(tMCA_HANDLE handle, tMCA_CL mcl,
                                  uint8_t event, tMCA_CTRL* p_data) {
  McapMcl* mcap_mcl = FindMclByHandle(mcl);
  McapMdl* mcap_mdl = nullptr;
  printf("%s: mcap_handle=%d, mcl_handle=%d, event=%s (0x%02x)\n", __func__,
         handle, mcl, dump_mcap_events(event), event);
  if (_mcap_handle != handle) {
    LOG(ERROR) << "MCAP handle mismatch, self=" << _mcap_handle
               << ", other=" << handle;
    return;
  }
  switch (event) {
    case MCA_ERROR_RSP_EVT:
      print_mcap_event(&p_data->rsp);
      break;

    case MCA_CREATE_CFM_EVT:
      // Called when MCA_CreateMdl succeeded step 1 when response is received
      print_mcap_event(&p_data->create_cfm);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL " << (int)mcl << " not connected";
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->create_cfm.mdl_id);
      if (!mcap_mdl) {
        LOG(ERROR) << "MDL not found for id " << p_data->create_cfm.mdl_id;
        break;
      }
      if (mcap_mdl->GetResponseCode() >= 0) {
        LOG(ERROR) << "MDL already got response " << mcap_mdl->GetResponseCode()
                   << " for id " << p_data->create_cfm.mdl_id;
        break;
      }
      mcap_mdl->SetResponseCode(p_data->create_cfm.rsp_code);
      break;

    case MCA_CREATE_IND_EVT: {
      // Should be replied with MCA_CreateMdlRsp
      print_mcap_event(&p_data->create_ind);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL " << (int)mcl << " not connected";
        break;
      }
      McapMdep* mcap_mdep = FindMdepByHandle(p_data->create_ind.dep_id);
      if (!mcap_mdep) {
        LOG(ERROR) << "MDEP ID " << (int)p_data->create_ind.dep_id
                   << " does not exist";
        _mcap_test_interface->create_mdl_response(
            mcl, p_data->create_ind.dep_id, p_data->create_ind.mdl_id, 0,
            MCA_RSP_BAD_MDEP, get_test_channel_config());
        break;
      }
      bool ret = mcap_mcl->CreateMdlResponse(
          mcap_mdep->GetHandle(), p_data->create_ind.mdl_id,
          p_data->create_ind.dep_id, p_data->create_ind.cfg);
      LOG(INFO) << (ret ? "SUCCESS" : "FAIL");
      if (!ret) {
        _mcap_test_interface->create_mdl_response(
            mcl, p_data->create_ind.dep_id, p_data->create_ind.mdl_id, 0,
            MCA_RSP_NO_RESOURCE, get_test_channel_config());
      }
      break;
    }
    case MCA_RECONNECT_IND_EVT: {
      // Called when remote device asks to reconnect
      // reply with MCA_ReconnectMdlRsp
      print_mcap_event(&p_data->reconnect_ind);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL " << (int)mcl << " not connected";
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->reconnect_ind.mdl_id);
      if (mcap_mdl && !mcap_mdl->IsConnected()) {
        LOG(INFO) << "Creating reconnect response for MDL "
                  << (int)p_data->reconnect_ind.mdl_id;
        mcap_mdl->ReconnectResponse();
        break;
      }
      LOG_IF(WARNING, mcap_mdl && mcap_mdl->IsConnected())
          << "MDL ID " << (int)p_data->reconnect_ind.mdl_id
          << " is already connected";
      LOG_IF(WARNING, !mcap_mdl) << "No MDL for mdl_id "
                                 << p_data->reconnect_ind.mdl_id;
      tMCA_DEP mdep_handle = 0;
      if (_mdep_list.size() > 0) {
        mdep_handle = _mdep_list[0].GetHandle();
      } else {
        LOG(ERROR) << "Cannot find any available MDEP";
      }
      tMCA_RESULT ret = _mcap_test_interface->reconnect_mdl_response(
          mcl, mdep_handle, p_data->reconnect_ind.mdl_id, MCA_RSP_BAD_MDL,
          get_test_channel_config());
      LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << ret;
      break;
    }
    case MCA_RECONNECT_CFM_EVT:
      // Called when MCA_ReconnectMdl step 1, receives a response
      print_mcap_event(&p_data->reconnect_cfm);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL not connected for mcl_handle " << (int)mcl;
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->reconnect_cfm.mdl_id);
      if (!mcap_mdl) {
        LOG(ERROR) << "MDL not found for id " << p_data->reconnect_cfm.mdl_id;
        break;
      }
      if (mcap_mdl->GetResponseCode() >= 0) {
        LOG(ERROR) << "MDL already got response " << mcap_mdl->GetResponseCode()
                   << " for id " << p_data->reconnect_cfm.mdl_id;
        break;
      }
      mcap_mdl->SetResponseCode(p_data->reconnect_cfm.rsp_code);
      break;

    case MCA_ABORT_IND_EVT:
      print_mcap_event(&p_data->abort_ind);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL not connected for mcl_handle " << (int)mcl;
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->abort_ind.mdl_id);
      if (!mcap_mdl) {
        LOG(ERROR) << "MDL not found for id " << (int)p_data->abort_ind.mdl_id;
        break;
      }
      if (mcap_mdl->IsConnected()) {
        LOG(ERROR) << "MDL is already connected for id "
                   << (int)p_data->abort_ind.mdl_id;
      }
      mcap_mcl->RemoveMdl(p_data->abort_ind.mdl_id);
      break;

    case MCA_ABORT_CFM_EVT:
      // Called when MCA_Abort succeeded
      print_mcap_event(&p_data->abort_cfm);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL " << (int)mcl << " not connected";
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->abort_cfm.mdl_id);
      if (!mcap_mdl) {
        LOG(ERROR) << "MDL not found for id " << (int)p_data->abort_cfm.mdl_id;
        break;
      }
      if (mcap_mdl->IsConnected()) {
        LOG(ERROR) << "MDL is already connected for id "
                   << (int)p_data->abort_cfm.mdl_id;
      }
      mcap_mcl->RemoveMdl(p_data->abort_cfm.mdl_id);
      break;

    case MCA_DELETE_IND_EVT:
      print_mcap_event(&p_data->delete_ind);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL " << (int)mcl << " not connected";
        break;
      }
      if (p_data->delete_ind.mdl_id == MCA_ALL_MDL_ID) {
        mcap_mcl->RemoveAllMdl();
      } else {
        mcap_mcl->RemoveMdl(p_data->delete_ind.mdl_id);
      }
      break;

    case MCA_DELETE_CFM_EVT:
      // Called when MCA_Delete succeeded
      print_mcap_event(&p_data->delete_cfm);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL " << (int)mcl << " not connected";
        break;
      }
      if (p_data->delete_cfm.rsp_code) {
        LOG(ERROR) << "No success response " << (int)p_data->delete_cfm.rsp_code
                   << " when deleting MDL_ID "
                   << (int)p_data->delete_cfm.mdl_id;
        break;
      }
      if (p_data->delete_cfm.mdl_id == MCA_ALL_MDL_ID) {
        mcap_mcl->RemoveAllMdl();
      } else {
        mcap_mcl->RemoveMdl(p_data->delete_cfm.mdl_id);
      }
      break;

    case MCA_CONNECT_IND_EVT: {
      // Called when MCA_ConnectReq succeeded
      print_mcap_event(&p_data->connect_ind);
      LOG(INFO) << "Received MCL handle " << (int)mcl;
      RawAddress bd_addr = p_data->connect_ind.bd_addr;
      mcap_mcl = FindMclByPeerAddress(bd_addr);
      if (!mcap_mcl) {
        LOG(INFO) << "Creating new MCL for ID " << (int)mcl;
        _mcl_list.push_back(
            McapMcl(_mcap_test_interface, _mcap_handle, bd_addr));
        mcap_mcl = &_mcl_list[_mcl_list.size() - 1];
      }
      if (mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL is already connected for handle " << (int)mcl;
        break;
      }
      mcap_mcl->SetHandle(mcl);
      mcap_mcl->SetMtu(p_data->connect_ind.mtu);
      break;
    }
    case MCA_DISCONNECT_IND_EVT: {
      // Called when MCA_ConnectReq failed or MCA_DisconnectReq succeeded
      print_mcap_event(&p_data->disconnect_ind);
      RawAddress bd_addr = p_data->disconnect_ind.bd_addr;
      mcap_mcl = FindMclByPeerAddress(bd_addr);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for BD addr " << bd_addr;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(WARNING) << "MCL for " << bd_addr << " is already disconnected";
      }
      mcap_mcl->SetHandle(0);
      mcap_mcl->SetMtu(0);
      mcap_mcl->ResetAllMdl();
      break;
    }
    case MCA_OPEN_IND_EVT:
    // Called when MCA_CreateMdlRsp succeeded step 2, data channel is open
    // Called when MCA_ReconnectMdlRsp succeeded step 2, data channel is open
    case MCA_OPEN_CFM_EVT:
      // Called when MCA_CreateMdl succeeded step 2, data channel is open
      // Called when MCA_ReconnectMdl succeeded step 2, data channel is open
      // Called when MCA_DataChnlCfg succeeded
      print_mcap_event(&p_data->open_ind);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL not connected for mcl_handle " << (int)mcl;
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->open_ind.mdl_id);
      if (mcap_mdl) {
        if (mcap_mdl->IsConnected()) {
          LOG(ERROR) << "MDL is already connected for mcl_handle "
                     << (int)p_data->open_ind.mdl_id;
          break;
        }
        mcap_mdl->SetMtu(p_data->open_ind.mtu);
        mcap_mdl->SetHandle(p_data->open_ind.mdl);
      } else {
        LOG(ERROR) << "No MDL for mdl_id " << (int)p_data->reconnect_ind.mdl_id;
      }
      break;

    case MCA_CLOSE_IND_EVT:
    case MCA_CLOSE_CFM_EVT:
      // Called when MCA_CloseReq is successful
      print_mcap_event(&p_data->close_cfm);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL not connected for mcl_handle " << (int)mcl;
        break;
      }
      mcap_mdl = mcap_mcl->FindMdlById(p_data->close_cfm.mdl_id);
      if (mcap_mdl) {
        mcap_mdl->SetMtu(0);
        mcap_mdl->SetHandle(0);
      } else {
        LOG(WARNING) << "No MDL for mdl_id " << (int)p_data->close_cfm.mdl_id;
      }
      break;

    case MCA_CONG_CHG_EVT:
      print_mcap_event(&p_data->cong_chg);
      if (!mcap_mcl) {
        LOG(ERROR) << "No MCL for mcl_handle " << (int)mcl;
        break;
      }
      if (!mcap_mcl->IsConnected()) {
        LOG(ERROR) << "MCL not connected for mcl_handle " << (int)mcl;
        break;
      }
      break;

    case MCA_RSP_TOUT_IND_EVT:
    case MCA_SYNC_CAP_IND_EVT:
    case MCA_SYNC_CAP_CFM_EVT:
    case MCA_SYNC_SET_IND_EVT:
    case MCA_SYNC_SET_CFM_EVT:
    case MCA_SYNC_INFO_IND_EVT:
      print_mcap_event(&p_data->hdr);
      break;
  }
}

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
