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
#include <dbus/mock_object_proxy.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tpm_manager/client/tpm_nvram_dbus_proxy.h"

using testing::_;
using testing::Invoke;
using testing::StrictMock;
using testing::WithArgs;

namespace tpm_manager {

class TpmNvramDBusProxyTest : public testing::Test {
 public:
  ~TpmNvramDBusProxyTest() override = default;
  void SetUp() override {
    mock_object_proxy_ = new StrictMock<dbus::MockObjectProxy>(
        nullptr, "", dbus::ObjectPath(""));
    proxy_.set_object_proxy(mock_object_proxy_.get());
  }

 protected:
  scoped_refptr<StrictMock<dbus::MockObjectProxy>> mock_object_proxy_;
  TpmNvramDBusProxy proxy_;
};

TEST_F(TpmNvramDBusProxyTest, DefineSpace) {
  uint32_t nvram_index = 5;
  size_t nvram_size = 32;
  auto fake_dbus_call = [nvram_index, nvram_size](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    DefineSpaceRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_index());
    EXPECT_EQ(nvram_index, request.index());
    EXPECT_TRUE(request.has_size());
    EXPECT_EQ(nvram_size, request.size());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    DefineSpaceReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const DefineSpaceReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  };
  DefineSpaceRequest request;
  request.set_index(nvram_index);
  request.set_size(nvram_size);
  proxy_.DefineSpace(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmNvramDBusProxyTest, DestroySpaceRequest) {
  uint32_t nvram_index = 5;
  auto fake_dbus_call = [nvram_index](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    DestroySpaceRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_index());
    EXPECT_EQ(nvram_index, request.index());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    DestroySpaceReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const DestroySpaceReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  };
  DestroySpaceRequest request;
  request.set_index(nvram_index);
  proxy_.DestroySpace(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}
TEST_F(TpmNvramDBusProxyTest, WriteSpace) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  auto fake_dbus_call = [nvram_index, nvram_data](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    WriteSpaceRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_index());
    EXPECT_EQ(nvram_index, request.index());
    EXPECT_TRUE(request.has_data());
    EXPECT_EQ(nvram_data, request.data());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    WriteSpaceReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const WriteSpaceReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  };
  WriteSpaceRequest request;
  request.set_index(nvram_index);
  request.set_data(nvram_data);
  proxy_.WriteSpace(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmNvramDBusProxyTest, ReadSpace) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  auto fake_dbus_call = [nvram_index, nvram_data](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    ReadSpaceRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_index());
    EXPECT_EQ(nvram_index, request.index());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    ReadSpaceReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    reply.set_data(nvram_data);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count, nvram_data](const ReadSpaceReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
    EXPECT_TRUE(reply.has_data());
    EXPECT_EQ(nvram_data, reply.data());
  };
  ReadSpaceRequest request;
  request.set_index(nvram_index);
  proxy_.ReadSpace(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmNvramDBusProxyTest, LockSpace) {
  uint32_t nvram_index = 5;
  auto fake_dbus_call = [nvram_index](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    LockSpaceRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_index());
    EXPECT_EQ(nvram_index, request.index());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    LockSpaceReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const LockSpaceReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  };
  LockSpaceRequest request;
  request.set_index(nvram_index);
  proxy_.LockSpace(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmNvramDBusProxyTest, ListSpaces) {
  constexpr uint32_t nvram_index_list[] = {3, 4, 5};
  auto fake_dbus_call = [nvram_index_list](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    ListSpacesRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    ListSpacesReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    for (auto index : nvram_index_list) {
      reply.add_index_list(index);
    }
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count,
                   nvram_index_list](const ListSpacesReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
    EXPECT_EQ(arraysize(nvram_index_list), reply.index_list_size());
    for (size_t i = 0; i < 3; i++) {
      EXPECT_EQ(nvram_index_list[i], reply.index_list(i));
    }
  };
  ListSpacesRequest request;
  proxy_.ListSpaces(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmNvramDBusProxyTest, GetSpaceInfo) {
  uint32_t nvram_index = 5;
  size_t nvram_size = 32;
  auto fake_dbus_call = [nvram_index, nvram_size](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    GetSpaceInfoRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_index());
    EXPECT_EQ(nvram_index, request.index());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    GetSpaceInfoReply reply;
    reply.set_result(NVRAM_RESULT_SUCCESS);
    reply.set_size(nvram_size);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));
  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count,
                   nvram_size](const GetSpaceInfoReply& reply) {
    callback_count++;
    EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
    EXPECT_TRUE(reply.has_size());
    EXPECT_EQ(nvram_size, reply.size());
  };
  GetSpaceInfoRequest request;
  request.set_index(nvram_index);
  proxy_.GetSpaceInfo(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

}  // namespace tpm_manager
