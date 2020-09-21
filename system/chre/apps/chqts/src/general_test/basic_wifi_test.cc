/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <general_test/basic_wifi_test.h>

#include <chre.h>
#include <shared/send_message.h>

/*
 * Test to check expected functionality of the CHRE WiFi APIs.
 * TODO: Currently the test only exists to verify that expected APIs are
 *       implemented and doesn't fail. Make the test more comprehensive by
 *       validating callback results, etc.
 */
namespace general_test {

namespace {

void testConfigureScanMonitorAsync() {
  if (!chreWifiConfigureScanMonitorAsync(
          true /* enable */, nullptr /* cookie */)) {
    nanoapp_testing::sendFatalFailureToHost(
        "Failed to request to enable scan monitor.");
  }
  if (!chreWifiConfigureScanMonitorAsync(
          false /* enable */, nullptr /* cookie */)) {
    nanoapp_testing::sendFatalFailureToHost(
        "Failed to request to disable scan monitor.");
  }
}

void testRequestScanAsync() {
  if (!chreWifiRequestScanAsyncDefault(nullptr /* cookie */)) {
    nanoapp_testing::sendFatalFailureToHost(
        "Failed to request for on-demand wifi scan.");
  }
}

} // anonymous namespace

BasicWifiTest::BasicWifiTest()
    : Test(CHRE_API_VERSION_1_1) {
}

void BasicWifiTest::setUp(
    uint32_t messageSize, const void * /* message */) {
  if (messageSize != 0) {
    nanoapp_testing::sendFatalFailureToHost(
        "Expected 0 byte message, got more bytes:", &messageSize);
  } else {
    uint32_t capabilities = chreWifiGetCapabilities();

    if (capabilities & CHRE_WIFI_CAPABILITIES_SCAN_MONITORING) {
      testConfigureScanMonitorAsync();
    }
    if (capabilities & CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN) {
      testRequestScanAsync();
    }
    // TODO: Add test for chreWifiRequestRangingAsync

    nanoapp_testing::sendSuccessToHost();
  }
}

void BasicWifiTest::handleEvent(uint32_t /* senderInstanceId */,
                                uint16_t eventType,
                                const void * /* eventData */) {
  if (eventType != CHRE_EVENT_WIFI_ASYNC_RESULT
      && eventType != CHRE_EVENT_WIFI_SCAN_RESULT) {
    unexpectedEvent(eventType);
  }
}

} // namespace general_test
