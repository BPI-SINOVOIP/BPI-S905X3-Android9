/*
 * Copyright (C) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <utils/StrongPointer.h>

#include "android/net/wifi/IClientInterface.h"
#include "android/net/wifi/IWificond.h"
#include "wificond/tests/integration/process_utils.h"
#include "wificond/tests/shell_utils.h"

using android::net::wifi::IClientInterface;
using android::net::wifi::IWificond;
using android::sp;
using android::wificond::tests::integration::RunShellCommand;
using android::wificond::tests::integration::ScopedDevModeWificond;
using android::wificond::tests::integration::SupplicantIsDead;
using android::wificond::tests::integration::SupplicantIsRunning;
using android::wificond::tests::integration::WaitForTrue;
using android::wificond::tests::integration::WificondIsDead;

namespace android {
namespace wificond {
namespace {

constexpr int kTimeoutSeconds = 3;
}  // namespace

TEST(ServiceTest, ShouldTearDownSystemOnStartup) {
  // Simulate doing normal connectivity things by startup supplicant.
  ScopedDevModeWificond dev_mode;
  sp<IWificond> service = dev_mode.EnterDevModeOrDie();

  bool supplicant_started = false;
  EXPECT_TRUE(service->enableSupplicant(&supplicant_started).isOk());
  EXPECT_TRUE(supplicant_started);

  EXPECT_TRUE(WaitForTrue(SupplicantIsRunning, kTimeoutSeconds));

  // Kill wificond abruptly.  It should not clean up on the way out.
  RunShellCommand("stop wificond");
  EXPECT_TRUE(WaitForTrue(WificondIsDead, kTimeoutSeconds));

  // Supplicant should still be up.
  EXPECT_TRUE(SupplicantIsRunning());

  // Restart wificond, which should kill supplicant on startup.
  service = dev_mode.EnterDevModeOrDie();
  EXPECT_TRUE(WaitForTrue(SupplicantIsDead, kTimeoutSeconds));
}

TEST(ServiceTest, CanStartStopSupplicant) {
  ScopedDevModeWificond dev_mode;
  sp<IWificond> service = dev_mode.EnterDevModeOrDie();

  for (int iteration = 0; iteration < 4; iteration++) {
    bool supplicant_started = false;
    EXPECT_TRUE(service->enableSupplicant(&supplicant_started).isOk());
    EXPECT_TRUE(supplicant_started);

    EXPECT_TRUE(WaitForTrue(SupplicantIsRunning,
                            kTimeoutSeconds))
        << "Failed on iteration " << iteration;

    // We look for supplicant so quickly that we miss when it dies on startup
    sleep(1);
    EXPECT_TRUE(SupplicantIsRunning()) << "Failed on iteration " << iteration;

    bool supplicant_stopped = false;
    EXPECT_TRUE(
        service->disableSupplicant(&supplicant_stopped).isOk());
    EXPECT_TRUE(supplicant_stopped);

    EXPECT_TRUE(WaitForTrue(SupplicantIsDead, kTimeoutSeconds))
        << "Failed on iteration " << iteration;
  }
}

}  // namespace wificond
}  // namespace android
