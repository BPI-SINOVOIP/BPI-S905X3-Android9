/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include <general_test/wifi_capabilities_test.h>

#include <chre.h>

#include <shared/send_message.h>

namespace general_test {

WifiCapabilitiesTest::WifiCapabilitiesTest()
    : Test(CHRE_API_VERSION_1_1) {
}

void WifiCapabilitiesTest::setUp(uint32_t messageSize,
                                 const void * /* message */) {
  if (messageSize != 0) {
    nanoapp_testing::sendFatalFailureToHost(
        "Expected 0 byte message, got more bytes:", &messageSize);
  } else {
    uint32_t allCapabilities = CHRE_WIFI_CAPABILITIES_NONE;

    if (mApiVersion >= CHRE_API_VERSION_1_1) {
      allCapabilities |= CHRE_WIFI_CAPABILITIES_SCAN_MONITORING
          | CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN;
    }
    if (mApiVersion >= CHRE_API_VERSION_1_2) {
      allCapabilities |= CHRE_WIFI_CAPABILITIES_RADIO_CHAIN_PREF
          | CHRE_WIFI_CAPABILITIES_RTT_RANGING;
    }

    // Call the new API
    uint32_t capabilities = chreWifiGetCapabilities();

    // Clear out known capabilities, any remaining are unknown
    if ((capabilities & ~allCapabilities) != 0) {
      if (mApiVersion > CHRE_API_VERSION_1_2) {
        nanoapp_testing::sendFatalFailureToHost(
            "New version with unknown capabilities encountered:",
            &capabilities);
      } else {
        nanoapp_testing::sendFatalFailureToHost(
            "Received unexpected capabilities:", &capabilities);
      }
    } else {
      nanoapp_testing::sendSuccessToHost();
    }
  }
}

void WifiCapabilitiesTest::handleEvent(uint32_t /* senderInstanceId */,
                                       uint16_t eventType,
                                       const void * /* eventData */) {
  unexpectedEvent(eventType);
}

} // namespace general_test
