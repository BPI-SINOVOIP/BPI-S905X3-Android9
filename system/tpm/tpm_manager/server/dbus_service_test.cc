//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <string>

#include <brillo/bind_lambda.h>
#include <brillo/dbus/dbus_object_test_helpers.h>
#include <dbus/mock_bus.h>
#include <dbus/mock_exported_object.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tpm_manager/common/mock_tpm_nvram_interface.h"
#include "tpm_manager/common/mock_tpm_ownership_interface.h"
#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/common/tpm_nvram_dbus_interface.h"
#include "tpm_manager/common/tpm_ownership_dbus_interface.h"
#include "tpm_manager/server/dbus_service.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace tpm_manager {

class DBusServiceTest : public testing::Test {
 public:
  ~DBusServiceTest() override = default;
  void SetUp() override {
    dbus::Bus::Options options;
    mock_bus_ = new NiceMock<dbus::MockBus>(options);
    dbus::ObjectPath path(kTpmManagerServicePath);
    mock_exported_object_ =
        new NiceMock<dbus::MockExportedObject>(mock_bus_.get(), path);
    ON_CALL(*mock_bus_, GetExportedObject(path))
        .WillByDefault(Return(mock_exported_object_.get()));
    dbus_service_.reset(new DBusService(mock_bus_, &mock_nvram_service_,
                                        &mock_ownership_service_));
    scoped_refptr<brillo::dbus_utils::AsyncEventSequencer> sequencer(
        new brillo::dbus_utils::AsyncEventSequencer());
    dbus_service_->RegisterDBusObjectsAsync(sequencer.get());
  }

  template <typename RequestProtobufType, typename ReplyProtobufType>
  void ExecuteMethod(const std::string& method_name,
                     const RequestProtobufType& request,
                     ReplyProtobufType* reply,
                     const std::string& interface) {
    std::unique_ptr<dbus::MethodCall> call =
        CreateMethodCall(method_name, interface);
    dbus::MessageWriter writer(call.get());
    writer.AppendProtoAsArrayOfBytes(request);
    auto response = brillo::dbus_utils::testing::CallMethod(
        dbus_service_->dbus_object_, call.get());
    dbus::MessageReader reader(response.get());
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(reply));
  }

 protected:
  std::unique_ptr<dbus::MethodCall> CreateMethodCall(
      const std::string& method_name,
      const std::string& interface) {
    std::unique_ptr<dbus::MethodCall> call(
        new dbus::MethodCall(interface, method_name));
    call->SetSerial(1);
    return call;
  }

  scoped_refptr<dbus::MockBus> mock_bus_;
  scoped_refptr<dbus::MockExportedObject> mock_exported_object_;
  StrictMock<MockTpmNvramInterface> mock_nvram_service_;
  StrictMock<MockTpmOwnershipInterface> mock_ownership_service_;
  std::unique_ptr<DBusService> dbus_service_;
};

TEST_F(DBusServiceTest, CopyableCallback) {
  EXPECT_CALL(mock_ownership_service_, GetTpmStatus(_, _))
      .WillOnce(WithArgs<1>(Invoke(
          [](const TpmOwnershipInterface::GetTpmStatusCallback& callback) {
            // Copy the callback, then call the original.
            GetTpmStatusReply reply;
            base::Closure copy = base::Bind(callback, reply);
            callback.Run(reply);
          })));
  GetTpmStatusRequest request;
  GetTpmStatusReply reply;
  ExecuteMethod(kGetTpmStatus, request, &reply, kTpmOwnershipInterface);
}

TEST_F(DBusServiceTest, GetTpmStatus) {
  GetTpmStatusRequest request;
  EXPECT_CALL(mock_ownership_service_, GetTpmStatus(_, _))
      .WillOnce(Invoke(
          [](const GetTpmStatusRequest& request,
             const TpmOwnershipInterface::GetTpmStatusCallback& callback) {
            GetTpmStatusReply reply;
            reply.set_status(STATUS_SUCCESS);
            reply.set_enabled(true);
            reply.set_owned(true);
            reply.set_dictionary_attack_counter(3);
            reply.set_dictionary_attack_threshold(4);
            reply.set_dictionary_attack_lockout_in_effect(true);
            reply.set_dictionary_attack_lockout_seconds_remaining(5);
            callback.Run(reply);
          }));
  GetTpmStatusReply reply;
  ExecuteMethod(kGetTpmStatus, request, &reply, kTpmOwnershipInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.enabled());
  EXPECT_TRUE(reply.owned());
  EXPECT_EQ(3, reply.dictionary_attack_counter());
  EXPECT_EQ(4, reply.dictionary_attack_threshold());
  EXPECT_TRUE(reply.dictionary_attack_lockout_in_effect());
  EXPECT_EQ(5, reply.dictionary_attack_lockout_seconds_remaining());
}

TEST_F(DBusServiceTest, TakeOwnership) {
  EXPECT_CALL(mock_ownership_service_, TakeOwnership(_, _))
      .WillOnce(Invoke(
          [](const TakeOwnershipRequest& request,
             const TpmOwnershipInterface::TakeOwnershipCallback& callback) {
            TakeOwnershipReply reply;
            reply.set_status(STATUS_SUCCESS);
            callback.Run(reply);
          }));
  TakeOwnershipRequest request;
  TakeOwnershipReply reply;
  ExecuteMethod(kTakeOwnership, request, &reply, kTpmOwnershipInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, RemoveOwnerDependency) {
  std::string owner_dependency("owner_dependency");
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(owner_dependency);
  EXPECT_CALL(mock_ownership_service_, RemoveOwnerDependency(_, _))
      .WillOnce(Invoke([&owner_dependency](
          const RemoveOwnerDependencyRequest& request,
          const TpmOwnershipInterface::RemoveOwnerDependencyCallback&
              callback) {
        EXPECT_TRUE(request.has_owner_dependency());
        EXPECT_EQ(owner_dependency, request.owner_dependency());
        RemoveOwnerDependencyReply reply;
        reply.set_status(STATUS_SUCCESS);
        callback.Run(reply);
      }));
  RemoveOwnerDependencyReply reply;
  ExecuteMethod(kRemoveOwnerDependency, request, &reply,
                kTpmOwnershipInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, DefineSpace) {
  uint32_t nvram_index = 5;
  size_t nvram_length = 32;
  DefineSpaceRequest request;
  request.set_index(nvram_index);
  request.set_size(nvram_length);
  EXPECT_CALL(mock_nvram_service_, DefineSpace(_, _))
      .WillOnce(Invoke([nvram_index, nvram_length](
          const DefineSpaceRequest& request,
          const TpmNvramInterface::DefineSpaceCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        EXPECT_TRUE(request.has_size());
        EXPECT_EQ(nvram_length, request.size());
        DefineSpaceReply reply;
        reply.set_result(NVRAM_RESULT_SUCCESS);
        callback.Run(reply);
      }));
  DefineSpaceReply reply;
  ExecuteMethod(kDefineSpace, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(DBusServiceTest, DestroySpace) {
  uint32_t nvram_index = 5;
  DestroySpaceRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, DestroySpace(_, _))
      .WillOnce(Invoke([nvram_index](
          const DestroySpaceRequest& request,
          const TpmNvramInterface::DestroySpaceCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        DestroySpaceReply reply;
        reply.set_result(NVRAM_RESULT_SUCCESS);
        callback.Run(reply);
      }));
  DestroySpaceReply reply;
  ExecuteMethod(kDestroySpace, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(DBusServiceTest, WriteSpace) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  WriteSpaceRequest request;
  request.set_index(nvram_index);
  request.set_data(nvram_data);
  EXPECT_CALL(mock_nvram_service_, WriteSpace(_, _))
      .WillOnce(Invoke([nvram_index, nvram_data](
          const WriteSpaceRequest& request,
          const TpmNvramInterface::WriteSpaceCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        EXPECT_TRUE(request.has_data());
        EXPECT_EQ(nvram_data, request.data());
        WriteSpaceReply reply;
        reply.set_result(NVRAM_RESULT_SUCCESS);
        callback.Run(reply);
      }));
  WriteSpaceReply reply;
  ExecuteMethod(kWriteSpace, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(DBusServiceTest, ReadSpace) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  ReadSpaceRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, ReadSpace(_, _))
      .WillOnce(Invoke([nvram_index, nvram_data](
          const ReadSpaceRequest& request,
          const TpmNvramInterface::ReadSpaceCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        ReadSpaceReply reply;
        reply.set_result(NVRAM_RESULT_SUCCESS);
        reply.set_data(nvram_data);
        callback.Run(reply);
      }));
  ReadSpaceReply reply;
  ExecuteMethod(kReadSpace, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  EXPECT_TRUE(reply.has_data());
  EXPECT_EQ(nvram_data, reply.data());
}

TEST_F(DBusServiceTest, LockSpace) {
  uint32_t nvram_index = 5;
  LockSpaceRequest request;
  request.set_index(nvram_index);
  request.set_lock_read(true);
  request.set_lock_write(true);
  EXPECT_CALL(mock_nvram_service_, LockSpace(_, _))
      .WillOnce(Invoke(
          [nvram_index](const LockSpaceRequest& request,
                        const TpmNvramInterface::LockSpaceCallback& callback) {
            EXPECT_TRUE(request.has_index());
            EXPECT_EQ(nvram_index, request.index());
            EXPECT_TRUE(request.lock_read());
            EXPECT_TRUE(request.lock_write());
            LockSpaceReply reply;
            reply.set_result(NVRAM_RESULT_SUCCESS);
            callback.Run(reply);
          }));
  LockSpaceReply reply;
  ExecuteMethod(kLockSpace, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(DBusServiceTest, ListSpaces) {
  constexpr uint32_t nvram_index_list[] = {3, 4, 5};
  ListSpacesRequest request;
  EXPECT_CALL(mock_nvram_service_, ListSpaces(_, _))
      .WillOnce(Invoke([nvram_index_list](
          const ListSpacesRequest& request,
          const TpmNvramInterface::ListSpacesCallback& callback) {
        ListSpacesReply reply;
        reply.set_result(NVRAM_RESULT_SUCCESS);
        for (auto index : nvram_index_list) {
          reply.add_index_list(index);
        }
        callback.Run(reply);
      }));
  ListSpacesReply reply;
  ExecuteMethod(kListSpaces, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  EXPECT_EQ(arraysize(nvram_index_list), reply.index_list_size());
  for (size_t i = 0; i < 3; i++) {
    EXPECT_EQ(nvram_index_list[i], reply.index_list(i));
  }
}

TEST_F(DBusServiceTest, GetSpaceInfo) {
  uint32_t nvram_index = 5;
  size_t nvram_size = 32;
  GetSpaceInfoRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, GetSpaceInfo(_, _))
      .WillOnce(Invoke([nvram_index, nvram_size](
          const GetSpaceInfoRequest& request,
          const TpmNvramInterface::GetSpaceInfoCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        GetSpaceInfoReply reply;
        reply.set_result(NVRAM_RESULT_SUCCESS);
        reply.set_size(nvram_size);
        reply.set_is_read_locked(true);
        reply.set_is_write_locked(true);
        callback.Run(reply);
      }));
  GetSpaceInfoReply reply;
  ExecuteMethod(kGetSpaceInfo, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  EXPECT_TRUE(reply.has_size());
  EXPECT_EQ(nvram_size, reply.size());
  EXPECT_TRUE(reply.is_read_locked());
  EXPECT_TRUE(reply.is_write_locked());
}

}  // namespace tpm_manager
