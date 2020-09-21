//
//  Copyright 2015 Google, Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at:
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include <base/macros.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "service/gatt_client.h"
#include "service/hal/fake_bluetooth_gatt_interface.h"

using ::testing::_;
using ::testing::Return;

namespace bluetooth {
namespace {

class MockGattHandler
    : public hal::FakeBluetoothGattInterface::TestClientHandler {
 public:
  MockGattHandler() = default;
  ~MockGattHandler() override = default;

  MOCK_METHOD1(RegisterClient, bt_status_t(const bluetooth::Uuid&));
  MOCK_METHOD1(UnregisterClient, bt_status_t(int));
  MOCK_METHOD1(Scan, bt_status_t(bool));
  MOCK_METHOD4(Connect, bt_status_t(int, const RawAddress&, bool, int));
  MOCK_METHOD3(Disconnect, bt_status_t(int, const RawAddress&, int));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockGattHandler);
};

class GattClientTest : public ::testing::Test {
 public:
  GattClientTest() = default;
  ~GattClientTest() override = default;

  void SetUp() override {
    // Only set |mock_handler_| if a previous test case hasn't set it.
    if (!mock_handler_) mock_handler_.reset(new MockGattHandler());

    fake_hal_gatt_iface_ = new hal::FakeBluetoothGattInterface(
        nullptr, nullptr,
        std::static_pointer_cast<
            hal::FakeBluetoothGattInterface::TestClientHandler>(mock_handler_),
        nullptr);
    hal::BluetoothGattInterface::InitializeForTesting(fake_hal_gatt_iface_);

    factory_.reset(new GattClientFactory());
  }

  void TearDown() override {
    factory_.reset();
    hal::BluetoothGattInterface::CleanUp();
  }

 protected:
  hal::FakeBluetoothGattInterface* fake_hal_gatt_iface_;
  std::shared_ptr<MockGattHandler> mock_handler_;
  std::unique_ptr<GattClientFactory> factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(GattClientTest);
};

TEST_F(GattClientTest, RegisterInstance) {
  EXPECT_CALL(*mock_handler_, RegisterClient(_))
      .Times(2)
      .WillOnce(Return(BT_STATUS_FAIL))
      .WillOnce(Return(BT_STATUS_SUCCESS));

  // These will be asynchronously populated with a result when the callback
  // executes.
  BLEStatus status = BLE_STATUS_SUCCESS;
  Uuid cb_uuid;
  std::unique_ptr<GattClient> client;
  int callback_count = 0;

  auto callback = [&](BLEStatus in_status, const Uuid& uuid,
                      std::unique_ptr<BluetoothInstance> in_client) {
    status = in_status;
    cb_uuid = uuid;
    client = std::unique_ptr<GattClient>(
        static_cast<GattClient*>(in_client.release()));
    callback_count++;
  };

  Uuid uuid0 = Uuid::GetRandom();

  // HAL returns failure.
  EXPECT_FALSE(factory_->RegisterInstance(uuid0, callback));
  EXPECT_EQ(0, callback_count);

  // HAL returns success.
  EXPECT_TRUE(factory_->RegisterInstance(uuid0, callback));
  EXPECT_EQ(0, callback_count);

  // Calling twice with the same Uuid should fail with no additional call into
  // the stack.
  EXPECT_FALSE(factory_->RegisterInstance(uuid0, callback));

  testing::Mock::VerifyAndClearExpectations(mock_handler_.get());

  // Call with a different Uuid while one is pending.
  Uuid uuid1 = Uuid::GetRandom();
  EXPECT_CALL(*mock_handler_, RegisterClient(_))
      .Times(1)
      .WillOnce(Return(BT_STATUS_SUCCESS));
  EXPECT_TRUE(factory_->RegisterInstance(uuid1, callback));

  // Trigger callback with an unknown Uuid. This should get ignored.
  Uuid uuid2 = Uuid::GetRandom();
  fake_hal_gatt_iface_->NotifyRegisterClientCallback(0, 0, uuid2);
  EXPECT_EQ(0, callback_count);

  // |uuid0| succeeds.
  int client_id0 = 2;  // Pick something that's not 0.
  fake_hal_gatt_iface_->NotifyRegisterClientCallback(BT_STATUS_SUCCESS,
                                                     client_id0, uuid0);

  EXPECT_EQ(1, callback_count);
  ASSERT_TRUE(client.get() != nullptr);  // Assert to terminate in case of error
  EXPECT_EQ(BLE_STATUS_SUCCESS, status);
  EXPECT_EQ(client_id0, client->GetInstanceId());
  EXPECT_EQ(uuid0, client->GetAppIdentifier());
  EXPECT_EQ(uuid0, cb_uuid);

  // The client should unregister itself when deleted.
  EXPECT_CALL(*mock_handler_, UnregisterClient(client_id0))
      .Times(1)
      .WillOnce(Return(BT_STATUS_SUCCESS));
  client.reset();
  testing::Mock::VerifyAndClearExpectations(mock_handler_.get());

  // |uuid1| fails.
  int client_id1 = 3;
  fake_hal_gatt_iface_->NotifyRegisterClientCallback(BT_STATUS_FAIL, client_id1,
                                                     uuid1);

  EXPECT_EQ(2, callback_count);
  ASSERT_TRUE(client.get() == nullptr);  // Assert to terminate in case of error
  EXPECT_EQ(BLE_STATUS_FAILURE, status);
  EXPECT_EQ(uuid1, cb_uuid);
}

}  // namespace
}  // namespace bluetooth
