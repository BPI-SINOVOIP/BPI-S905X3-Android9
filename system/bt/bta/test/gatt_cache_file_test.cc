/******************************************************************************
 *
 *  Copyright 2018 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include "bta/include/bta_gatt_api.h"

using bluetooth::Uuid;

/* This test makes sure that v3 cache element is properly encoded into file*/
TEST(GattCacheTest, nv_attr_to_binary_test) {
  tBTA_GATTC_NV_ATTR attr{
      .uuid = Uuid::FromString("1800"),
      .s_handle = 0x0001,
      .e_handle = 0xFFFF,
      .attr_type = 0x01,
      .id = 0x02,
      .prop = 0x03,
      .is_primary = false,
      .incl_srvc_handle = 0x4543,
  };

  constexpr size_t len = sizeof(tBTA_GATTC_NV_ATTR);
  uint8_t binary_form[len] = {0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x10,
                              0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B,
                              0x34, 0xFB, 0x01, 0x00, 0xFF, 0xFF, 0x01,
                              0x02, 0x03, 0x00, 0x43, 0x45};

  // USEFUL for debugging:
  // LOG(ERROR) << " " << base::HexEncode(binary_form, len);
  EXPECT_EQ(memcmp(binary_form, &attr, len), 0);
}
