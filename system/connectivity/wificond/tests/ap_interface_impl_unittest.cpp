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

#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wifi_system_test/mock_hostapd_manager.h>
#include <wifi_system_test/mock_interface_tool.h>

#include "wificond/tests/mock_ap_interface_event_callback.h"
#include "wificond/tests/mock_netlink_manager.h"
#include "wificond/tests/mock_netlink_utils.h"

#include "wificond/ap_interface_impl.h"

using android::wifi_system::HostapdManager;
using android::wifi_system::MockHostapdManager;
using android::wifi_system::MockInterfaceTool;
using std::placeholders::_1;
using std::placeholders::_2;
using std::unique_ptr;
using std::vector;
using testing::NiceMock;
using testing::Invoke;
using testing::Return;
using testing::Sequence;
using testing::StrEq;
using testing::_;

namespace android {
namespace wificond {
namespace {

const char kTestInterfaceName[] = "testwifi0";
const uint32_t kTestInterfaceIndex = 42;
const uint8_t kFakeMacAddress[] = {0x45, 0x54, 0xad, 0x67, 0x98, 0xf6};

void CaptureStationEventHandler(
    OnStationEventHandler* out_handler,
    uint32_t interface_index,
    OnStationEventHandler handler) {
  *out_handler = handler;
}

void CaptureChannelSwitchEventHandler(
    OnChannelSwitchEventHandler* out_handler,
    uint32_t interface_index,
    OnChannelSwitchEventHandler handler) {
  *out_handler = handler;
}

class ApInterfaceImplTest : public ::testing::Test {
 protected:
  unique_ptr<NiceMock<MockInterfaceTool>> if_tool_{
      new NiceMock<MockInterfaceTool>};
  unique_ptr<NiceMock<MockHostapdManager>> hostapd_manager_{
      new NiceMock<MockHostapdManager>};
  unique_ptr<NiceMock<MockNetlinkManager>> netlink_manager_{
      new NiceMock<MockNetlinkManager>()};
  unique_ptr<NiceMock<MockNetlinkUtils>> netlink_utils_{
      new NiceMock<MockNetlinkUtils>(netlink_manager_.get())};

  unique_ptr<ApInterfaceImpl> ap_interface_;

  void SetUp() override {
    ap_interface_.reset(new ApInterfaceImpl(
        kTestInterfaceName,
        kTestInterfaceIndex,
        netlink_utils_.get(),
        if_tool_.get(),
        hostapd_manager_.get()));
  }
};  // class ApInterfaceImplTest

}  // namespace

TEST_F(ApInterfaceImplTest, ShouldReportStartFailure) {
  EXPECT_CALL(*hostapd_manager_, StartHostapd())
      .WillOnce(Return(false));
  EXPECT_FALSE(ap_interface_->StartHostapd());
}

TEST_F(ApInterfaceImplTest, ShouldReportStartSuccess) {
  EXPECT_CALL(*hostapd_manager_, StartHostapd())
      .WillOnce(Return(true));
  EXPECT_TRUE(ap_interface_->StartHostapd());
}

TEST_F(ApInterfaceImplTest, ShouldReportStopFailure) {
  EXPECT_CALL(*hostapd_manager_, StopHostapd())
      .WillOnce(Return(false));
  EXPECT_FALSE(ap_interface_->StopHostapd());
}

TEST_F(ApInterfaceImplTest, ShouldReportStopSuccess) {
  EXPECT_CALL(*hostapd_manager_, StopHostapd())
      .WillOnce(Return(true));
  EXPECT_CALL(*if_tool_, SetUpState(StrEq(kTestInterfaceName), false))
      .WillOnce(Return(true));
  EXPECT_CALL(*netlink_utils_, SetInterfaceMode(
      kTestInterfaceIndex,
      NetlinkUtils::STATION_MODE)).WillOnce(Return(true));
  EXPECT_TRUE(ap_interface_->StopHostapd());
  testing::Mock::VerifyAndClearExpectations(if_tool_.get());
}

TEST_F(ApInterfaceImplTest, CanGetNumberOfAssociatedStations) {
  OnStationEventHandler handler;
  EXPECT_CALL(*netlink_utils_,
      SubscribeStationEvent(kTestInterfaceIndex, _)).
          WillOnce(Invoke(bind(CaptureStationEventHandler, &handler, _1, _2)));

  ap_interface_.reset(new ApInterfaceImpl(
        kTestInterfaceName,
        kTestInterfaceIndex,
        netlink_utils_.get(),
        if_tool_.get(),
        hostapd_manager_.get()));

  vector<uint8_t> fake_mac_address(kFakeMacAddress,
                                   kFakeMacAddress + sizeof(kFakeMacAddress));
  EXPECT_EQ(0, ap_interface_->GetNumberOfAssociatedStations());
  handler(NEW_STATION, fake_mac_address);
  EXPECT_EQ(1, ap_interface_->GetNumberOfAssociatedStations());
  handler(NEW_STATION, fake_mac_address);
  EXPECT_EQ(2, ap_interface_->GetNumberOfAssociatedStations());
  handler(DEL_STATION, fake_mac_address);
  EXPECT_EQ(1, ap_interface_->GetNumberOfAssociatedStations());
}

TEST_F(ApInterfaceImplTest, CallbackIsCalledOnNumAssociatedStationsChanged) {
  OnStationEventHandler handler;
  EXPECT_CALL(*netlink_utils_, SubscribeStationEvent(kTestInterfaceIndex, _))
      .WillOnce(Invoke(bind(CaptureStationEventHandler, &handler, _1, _2)));
  ap_interface_.reset(new ApInterfaceImpl(
      kTestInterfaceName, kTestInterfaceIndex, netlink_utils_.get(),
      if_tool_.get(), hostapd_manager_.get()));

  EXPECT_CALL(*hostapd_manager_, StartHostapd()).WillOnce(Return(true));
  auto binder = ap_interface_->GetBinder();
  sp<MockApInterfaceEventCallback> callback(new MockApInterfaceEventCallback());
  bool out_success = false;
  EXPECT_TRUE(binder->startHostapd(callback, &out_success).isOk());
  EXPECT_TRUE(out_success);

  vector<uint8_t> fake_mac_address(kFakeMacAddress,
                                   kFakeMacAddress + sizeof(kFakeMacAddress));
  EXPECT_CALL(*callback, onNumAssociatedStationsChanged(1));
  handler(NEW_STATION, fake_mac_address);
  EXPECT_CALL(*callback, onNumAssociatedStationsChanged(2));
  handler(NEW_STATION, fake_mac_address);
  EXPECT_CALL(*callback, onNumAssociatedStationsChanged(1));
  handler(DEL_STATION, fake_mac_address);
}

TEST_F(ApInterfaceImplTest, CallbackIsCalledOnSoftApChannelSwitched) {
  OnChannelSwitchEventHandler handler;
  EXPECT_CALL(*netlink_utils_, SubscribeChannelSwitchEvent(kTestInterfaceIndex, _))
      .WillOnce(Invoke(bind(CaptureChannelSwitchEventHandler, &handler, _1, _2)));
  ap_interface_.reset(new ApInterfaceImpl(
      kTestInterfaceName, kTestInterfaceIndex, netlink_utils_.get(),
      if_tool_.get(), hostapd_manager_.get()));

  EXPECT_CALL(*hostapd_manager_, StartHostapd()).WillOnce(Return(true));
  auto binder = ap_interface_->GetBinder();
  sp<MockApInterfaceEventCallback> callback(new MockApInterfaceEventCallback());
  bool out_success = false;
  EXPECT_TRUE(binder->startHostapd(callback, &out_success).isOk());
  EXPECT_TRUE(out_success);

  const uint32_t kTestChannelFrequency = 2437;
  const ChannelBandwidth kTestChannelBandwidth = ChannelBandwidth::BW_20;
  EXPECT_CALL(*callback, onSoftApChannelSwitched(kTestChannelFrequency,
                                                 kTestChannelBandwidth));
  handler(kTestChannelFrequency, kTestChannelBandwidth);
}

}  // namespace wificond
}  // namespace android
