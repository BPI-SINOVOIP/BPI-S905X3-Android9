/*
 * Copyright (C) 2017, The Android Open Source Project
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

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wifi_system_test/mock_interface_tool.h>
#include <wifi_system_test/mock_supplicant_manager.h>

#include "android/net/wifi/IWifiScannerImpl.h"
#include "wificond/scanning/offload/offload_scan_utils.h"
#include "wificond/scanning/scanner_impl.h"
#include "wificond/tests/mock_client_interface_impl.h"
#include "wificond/tests/mock_netlink_manager.h"
#include "wificond/tests/mock_netlink_utils.h"
#include "wificond/tests/mock_offload_scan_callback_interface_impl.h"
#include "wificond/tests/mock_offload_scan_manager.h"
#include "wificond/tests/mock_offload_service_utils.h"
#include "wificond/tests/mock_scan_utils.h"
#include "wificond/tests/offload_test_utils.h"

using ::android::binder::Status;
using ::android::net::wifi::IWifiScannerImpl;
using ::android::wifi_system::MockInterfaceTool;
using ::com::android::server::wifi::wificond::SingleScanSettings;
using ::com::android::server::wifi::wificond::PnoSettings;
using ::com::android::server::wifi::wificond::NativeScanResult;
using android::hardware::wifi::offload::V1_0::ScanResult;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

using namespace std::placeholders;

namespace android {
namespace wificond {

namespace {

constexpr uint32_t kFakeInterfaceIndex = 12;
constexpr uint32_t kFakeScanIntervalMs = 10000;

// This is a helper function to mock the behavior of ScanUtils::Scan()
// when we expect a error code.
// |interface_index_ignored|, |request_random_mac_ignored|, |ssids_ignored|,
// |freqs_ignored|, |error_code| are mapped to existing parameters of ScanUtils::Scan().
// |mock_error_code| is a additional parameter used for specifying expected error code.
bool ReturnErrorCodeForScanRequest(
    int mock_error_code,
    uint32_t interface_index_ignored,
    bool request_random_mac_ignored,
    int scan_type,
    const std::vector<std::vector<uint8_t>>& ssids_ignored,
    const std::vector<uint32_t>& freqs_ignored,
    int* error_code) {
  *error_code = mock_error_code;
  // Returing false because this helper function is used for failure case.
  return false;
}

bool CaptureSchedScanIntervalSetting(
    uint32_t /* interface_index */,
    const SchedScanIntervalSetting&  interval_setting,
    int32_t /* rssi_threshold_2g */,
    int32_t /* rssi_threshold_5g */,
    bool /* request_random_mac */,
    bool /* request_low_power_scan */,
    const  std::vector<std::vector<uint8_t>>& /* scan_ssids */,
    const std::vector<std::vector<uint8_t>>& /* match_ssids */,
    const  std::vector<uint32_t>& /* freqs */,
    int* /* error_code */,
    SchedScanIntervalSetting* out_interval_setting) {
  *out_interval_setting = interval_setting;
  return true;
}

bool ReturnOffloadScanResults(
    std::vector<NativeScanResult>* native_scan_results_,
    const std::vector<ScanResult>& offload_scan_results) {
  return OffloadScanUtils::convertToNativeScanResults(offload_scan_results,
                                                      native_scan_results_);
}

bool ReturnNetlinkScanResults(
    uint32_t interface_index,
    std::vector<NativeScanResult>* native_scan_results_,
    const std::vector<ScanResult>& offload_scan_results) {
  return OffloadScanUtils::convertToNativeScanResults(offload_scan_results,
                                                      native_scan_results_);
}

}  // namespace

class ScannerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ON_CALL(*offload_service_utils_, GetOffloadScanManager(_, _))
        .WillByDefault(Return(offload_scan_manager_));
    ON_CALL(*offload_service_utils_, GetOffloadScanCallbackInterface(_))
        .WillByDefault(Return(offload_scan_callback_interface_));
    dummy_scan_results_ = OffloadTestUtils::createOffloadScanResults();
  }

  void TearDown() override { dummy_scan_results_.clear(); }

  unique_ptr<ScannerImpl> scanner_impl_;
  NiceMock<MockNetlinkManager> netlink_manager_;
  NiceMock<MockNetlinkUtils> netlink_utils_{&netlink_manager_};
  NiceMock<MockScanUtils> scan_utils_{&netlink_manager_};
  NiceMock<MockInterfaceTool> if_tool_;
  NiceMock<MockClientInterfaceImpl> client_interface_impl_{
      &if_tool_, &netlink_utils_, &scan_utils_};
  shared_ptr<NiceMock<MockOffloadServiceUtils>> offload_service_utils_{
      new NiceMock<MockOffloadServiceUtils>()};
  shared_ptr<NiceMock<MockOffloadScanCallbackInterfaceImpl>>
      offload_scan_callback_interface_{
          new NiceMock<MockOffloadScanCallbackInterfaceImpl>(
              scanner_impl_.get())};
  std::shared_ptr<NiceMock<MockOffloadScanManager>> offload_scan_manager_{
      new NiceMock<MockOffloadScanManager>(offload_service_utils_,
                                           offload_scan_callback_interface_)};
  ScanCapabilities scan_capabilities_;
  WiphyFeatures wiphy_features_;
  std::vector<ScanResult> dummy_scan_results_;
};

TEST_F(ScannerTest, TestSingleScan) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_DEFAULT, _, _, _)).
      WillOnce(Return(true));
  bool success = false;
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_TRUE(scanner_impl_->scan(SingleScanSettings(), &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanForLowSpanScan) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_LOW_SPAN, _, _, _)).
      WillOnce(Return(true));
  wiphy_features_.supports_low_span_oneshot_scan = true;
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  SingleScanSettings settings;
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_LOW_SPAN;
  bool success = false;
  EXPECT_TRUE(scanner_impl.scan(settings, &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanForLowPowerScan) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_LOW_POWER, _, _, _)).
      WillOnce(Return(true));
  wiphy_features_.supports_low_power_oneshot_scan = true;
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  SingleScanSettings settings;
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_LOW_POWER;
  bool success = false;
  EXPECT_TRUE(scanner_impl.scan(settings, &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanForHighAccuracyScan) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_HIGH_ACCURACY, _, _, _)).
      WillOnce(Return(true));
  wiphy_features_.supports_high_accuracy_oneshot_scan = true;
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  SingleScanSettings settings;
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_HIGH_ACCURACY;
  bool success = false;
  EXPECT_TRUE(scanner_impl.scan(settings, &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanForLowSpanScanWithNoWiphySupport) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_DEFAULT, _, _, _)).
      WillOnce(Return(true));
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  SingleScanSettings settings;
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_LOW_SPAN;
  bool success = false;
  EXPECT_TRUE(scanner_impl.scan(settings, &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanForLowPowerScanWithNoWiphySupport) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_DEFAULT, _, _, _)).
      WillOnce(Return(true));
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  SingleScanSettings settings;
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_LOW_POWER;
  bool success = false;
  EXPECT_TRUE(scanner_impl.scan(settings, &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanForHighAccuracyScanWithNoWiphySupport) {
  EXPECT_CALL(scan_utils_,
              Scan(_, _, IWifiScannerImpl::SCAN_TYPE_DEFAULT, _, _, _)).
      WillOnce(Return(true));
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  SingleScanSettings settings;
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_HIGH_ACCURACY;
  bool success = false;
  EXPECT_TRUE(scanner_impl.scan(settings, &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestSingleScanFailure) {
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(
      scan_utils_,
      Scan(_, _, _, _, _, _)).
          WillOnce(Invoke(bind(
              ReturnErrorCodeForScanRequest, EBUSY,
              _1, _2, _3, _4, _5, _6)));

  bool success = false;
  EXPECT_TRUE(scanner_impl_->scan(SingleScanSettings(), &success).isOk());
  EXPECT_FALSE(success);
}

TEST_F(ScannerTest, TestProcessAbortsOnScanReturningNoDeviceError) {
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  ON_CALL(
      scan_utils_,
      Scan(_, _, _, _, _, _)).
          WillByDefault(Invoke(bind(
              ReturnErrorCodeForScanRequest, ENODEV,
              _1, _2, _3, _4, _5, _6)));

  bool success_ignored;
  EXPECT_DEATH(scanner_impl_->scan(SingleScanSettings(), &success_ignored),
               "Driver is in a bad state*");
}

TEST_F(ScannerTest, TestAbortScan) {
  bool single_scan_success = false;
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(scan_utils_, Scan(_, _, _, _, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(
      scanner_impl_->scan(SingleScanSettings(), &single_scan_success).isOk());
  EXPECT_TRUE(single_scan_success);

  EXPECT_CALL(scan_utils_, AbortScan(_));
  EXPECT_TRUE(scanner_impl_->abortScan().isOk());
}

TEST_F(ScannerTest, TestAbortScanNotIssuedIfNoOngoingScan) {
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(scan_utils_, AbortScan(_)).Times(0);
  EXPECT_TRUE(scanner_impl_->abortScan().isOk());
}

TEST_F(ScannerTest, TestGetScanResults) {
  vector<NativeScanResult> scan_results;
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(scan_utils_, GetScanResult(_, _)).WillOnce(Return(true));
  EXPECT_TRUE(scanner_impl_->getScanResults(&scan_results).isOk());
}

TEST_F(ScannerTest, TestStartPnoScanViaNetlink) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(false));
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  EXPECT_CALL(
      scan_utils_,
      StartScheduledScan(_, _, _, _, _, false, _, _, _, _)).
          WillOnce(Return(true));
  EXPECT_TRUE(scanner_impl.startPnoScan(PnoSettings(), &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestStartPnoScanViaNetlinkWithLowPowerScanWiphySupport) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(false));
  wiphy_features_.supports_low_power_oneshot_scan = true;
  ScannerImpl scanner_impl(kFakeInterfaceIndex, scan_capabilities_,
                           wiphy_features_, &client_interface_impl_,
                           &scan_utils_, offload_service_utils_);
  EXPECT_CALL(
      scan_utils_,
      StartScheduledScan(_, _, _, _, _, true, _, _, _, _)).
          WillOnce(Return(true));
  EXPECT_TRUE(scanner_impl.startPnoScan(PnoSettings(), &success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestStopPnoScanViaNetlink) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(false));
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  // StopScheduledScan() will be called no matter if there is an ongoing
  // scheduled scan or not. This is for making the system more robust.
  EXPECT_CALL(scan_utils_, StopScheduledScan(_)).WillOnce(Return(true));
  EXPECT_TRUE(scanner_impl_->stopPnoScan(&success).isOk());
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestStartScanOverOffload) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, startScan(_, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, stopScan(_))
      .Times(1)
      .WillRepeatedly(Return(true));
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  scanner_impl_->startPnoScan(PnoSettings(), &success);
  EXPECT_TRUE(success);
  scanner_impl_->stopPnoScan(&success);
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestStartScanOverNetlinkFallback) {
  bool success = false;
  ON_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .WillByDefault(Return(true));
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(*offload_scan_manager_, startScan(_, _, _, _, _, _, _))
      .WillOnce(Return(false));
  EXPECT_CALL(*offload_scan_manager_, stopScan(_)).Times(0);
  EXPECT_CALL(scan_utils_, StartScheduledScan(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(Return(true));
  EXPECT_CALL(scan_utils_, StopScheduledScan(_)).WillOnce(Return(true));
  EXPECT_TRUE(scanner_impl_->startPnoScan(PnoSettings(), &success).isOk());
  EXPECT_TRUE(success == true);
  scanner_impl_->stopPnoScan(&success);
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestAsyncErrorOverOffload) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, startScan(_, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, stopScan(_))
      .Times(1)
      .WillRepeatedly(Return(true));
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(scan_utils_, StartScheduledScan(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(Return(true));
  EXPECT_CALL(scan_utils_, StopScheduledScan(_)).WillOnce(Return(true));
  scanner_impl_->startPnoScan(PnoSettings(), &success);
  EXPECT_TRUE(success);
  scanner_impl_->OnOffloadError(
      OffloadScanCallbackInterface::AsyncErrorReason::REMOTE_FAILURE);
  scanner_impl_->stopPnoScan(&success);
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestGetScanResultsFromOffload) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, startScan(_, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, getScanResults(_))
      .Times(1)
      .WillOnce(
          Invoke(bind(ReturnOffloadScanResults, _1, dummy_scan_results_)));
  EXPECT_CALL(*offload_scan_manager_, stopScan(_))
      .Times(1)
      .WillRepeatedly(Return(true));
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  scanner_impl_->startPnoScan(PnoSettings(), &success);
  EXPECT_TRUE(success);
  scanner_impl_->OnOffloadScanResult();
  std::vector<NativeScanResult> scan_results;
  EXPECT_TRUE(scanner_impl_->getPnoScanResults(&scan_results).isOk());
  EXPECT_FALSE(scan_results.empty());
  scanner_impl_->stopPnoScan(&success);
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestGetScanResultsWhenOffloadFails) {
  bool success = false;
  EXPECT_CALL(*offload_service_utils_, IsOffloadScanSupported())
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, startScan(_, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, stopScan(_))
      .Times(1)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*offload_scan_manager_, getScanResults(_)).Times(0);
  EXPECT_CALL(scan_utils_, GetScanResult(_, _))
      .Times(1)
      .WillOnce(
          Invoke(bind(ReturnNetlinkScanResults, _1, _2, dummy_scan_results_)));
  scanner_impl_.reset(new ScannerImpl(kFakeInterfaceIndex,
                                      scan_capabilities_, wiphy_features_,
                                      &client_interface_impl_,
                                      &scan_utils_, offload_service_utils_));
  EXPECT_CALL(scan_utils_, StartScheduledScan(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(Return(true));
  EXPECT_CALL(scan_utils_, StopScheduledScan(_)).WillOnce(Return(true));
  EXPECT_TRUE(scanner_impl_->startPnoScan(PnoSettings(), &success).isOk());
  EXPECT_TRUE(success);
  scanner_impl_->OnOffloadError(
      OffloadScanCallbackInterface::AsyncErrorReason::REMOTE_FAILURE);
  std::vector<NativeScanResult> scan_results;
  EXPECT_TRUE(scanner_impl_->getPnoScanResults(&scan_results).isOk());
  EXPECT_FALSE(scan_results.empty());
  scanner_impl_->stopPnoScan(&success);
  EXPECT_TRUE(success);
}

TEST_F(ScannerTest, TestGenerateScanPlansIfDeviceSupports) {
  ScanCapabilities scan_capabilities_scan_plan_supported(
      0 /* max_num_scan_ssids */,
      0 /* max_num_sched_scan_ssids */,
      0 /* max_match_sets */,
      // Parameters above are not related to this test.
      2 /* 1 plan for finite repeated scan and 1 plan for ininfite scan loop */,
      kFakeScanIntervalMs * PnoSettings::kSlowScanIntervalMultiplier / 1000,
      PnoSettings::kFastScanIterations);
  ScannerImpl scanner(
      kFakeInterfaceIndex,
      scan_capabilities_scan_plan_supported, wiphy_features_,
      &client_interface_impl_,
      &scan_utils_, offload_service_utils_);

  PnoSettings pno_settings;
  pno_settings.interval_ms_ = kFakeScanIntervalMs;

  SchedScanIntervalSetting interval_setting;
  EXPECT_CALL(
      scan_utils_,
      StartScheduledScan(_, _, _, _, _, _, _, _, _, _)).
              WillOnce(Invoke(bind(
                  CaptureSchedScanIntervalSetting,
                  _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, &interval_setting)));

  bool success_ignored = 0;
  EXPECT_TRUE(scanner.startPnoScan(pno_settings, &success_ignored).isOk());
  /* 1 plan for finite repeated scan */
  EXPECT_EQ(1U, interval_setting.plans.size());
  EXPECT_EQ(kFakeScanIntervalMs * PnoSettings::kSlowScanIntervalMultiplier,
            interval_setting.final_interval_ms);
}

TEST_F(ScannerTest, TestGenerateSingleIntervalIfDeviceDoesNotSupportScanPlan) {
  ScanCapabilities scan_capabilities_no_scan_plan_support(
      0 /* max_num_scan_ssids */,
      0 /* max_num_sched_scan_ssids */,
      0 /* max_match_sets */,
      // Parameters above are not related to this test.
      0 /* max_num_scan_plans */,
      0 /* max_scan_plan_interval */,
      0 /* max_scan_plan_iterations */);
  ScannerImpl scanner(
      kFakeInterfaceIndex,
      scan_capabilities_no_scan_plan_support, wiphy_features_,
      &client_interface_impl_,
      &scan_utils_, offload_service_utils_);
  PnoSettings pno_settings;
  pno_settings.interval_ms_ = kFakeScanIntervalMs;

  SchedScanIntervalSetting interval_setting;
  EXPECT_CALL(
      scan_utils_,
      StartScheduledScan(_, _, _, _, _, _, _, _, _, _)).
              WillOnce(Invoke(bind(
                  CaptureSchedScanIntervalSetting,
                  _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, &interval_setting)));

  bool success_ignored = 0;
  EXPECT_TRUE(scanner.startPnoScan(pno_settings, &success_ignored).isOk());

  EXPECT_EQ(0U, interval_setting.plans.size());
  EXPECT_EQ(kFakeScanIntervalMs, interval_setting.final_interval_ms);
}

}  // namespace wificond
}  // namespace android
