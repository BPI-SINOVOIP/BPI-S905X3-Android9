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

#include "mcap_test_mdep.h"

namespace SYSTEM_BT_TOOLS_MCAP_TOOL {

McapMdep::McapMdep(btmcap_test_interface_t* mcap_test_interface,
                   tMCA_HANDLE mcap_handle, uint8_t type, uint8_t max_mdl,
                   tMCA_DATA_CBACK* data_callback) {
  _mcap_test_interface = mcap_test_interface;
  _mcap_handle = mcap_handle;
  _mca_cs.type = (0 == type) ? MCA_TDEP_ECHO : MCA_TDEP_DATA;
  _mca_cs.max_mdl = max_mdl;
  _mca_cs.p_data_cback = data_callback;
}

bool McapMdep::Create() {
  tMCA_RESULT ret =
      _mcap_test_interface->create_mdep(_mcap_handle, &_mdep_handle, &_mca_cs);
  LOG(INFO) << "mdep_handle=" << (int)_mdep_handle;
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

bool McapMdep::Delete() {
  tMCA_RESULT ret =
      _mcap_test_interface->create_mdep(_mcap_handle, &_mdep_handle, &_mca_cs);
  LOG_IF(INFO, ret != MCA_SUCCESS) << "ret=" << (int)ret;
  return ret == MCA_SUCCESS;
}

tMCA_DEP McapMdep::GetHandle() const { return _mdep_handle; }

bool McapMdep::IsRegistered() { return _mdep_handle > 0; }

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
