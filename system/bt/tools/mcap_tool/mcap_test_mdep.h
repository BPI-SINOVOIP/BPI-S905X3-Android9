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

class McapMdep {
 public:
  /**
   * A control abstraction for a MCAP Data End Point (MDEP)
   * @param mcap_test_interface Underlining MCAP interface to Bluetooth stack
   * @param mcap_handle Parent MCAP application handle
   * @param type 0 for Echo, 1 for Normal, nothing else
   * @param max_mdl Maximum number of MDL's allowed
   * @param data_callback Data channel callback function
   */
  McapMdep(btmcap_test_interface_t* mcap_test_interface,
           tMCA_HANDLE mcap_handle, uint8_t type, uint8_t max_mdl,
           tMCA_DATA_CBACK* data_callback);
  /**
   * Actually create the MDEP in the stack
   * @return True if success
   */
  bool Create();
  /**
   * Destroy the MDEP in the stack
   * @return True if success
   */
  bool Delete();
  // Simple methods that are self-explanatory
  tMCA_DEP GetHandle() const;
  bool IsRegistered();

 private:
  // Initialized during start up
  btmcap_test_interface_t* _mcap_test_interface;
  tMCA_HANDLE _mcap_handle;
  tMCA_CS _mca_cs;

  // Initialized later
  tMCA_DEP _mdep_handle = 0;
};

}  // namespace SYSTEM_BT_TOOLS_MCAP_TOOL
