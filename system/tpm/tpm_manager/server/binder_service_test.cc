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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tpm_manager/client/tpm_nvram_binder_proxy.h"
#include "tpm_manager/client/tpm_ownership_binder_proxy.h"
#include "tpm_manager/common/mock_tpm_nvram_interface.h"
#include "tpm_manager/common/mock_tpm_ownership_interface.h"
#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/server/binder_service.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace tpm_manager {

// A test fixture to exercise both proxy and service layers. Tpm*BinderProxy
// classes get coverage here and do not need additional unit tests.
class BinderServiceTest : public testing::Test {
 public:
  ~BinderServiceTest() override = default;
  void SetUp() override {
    binder_service_.reset(
        new BinderService(&mock_nvram_service_, &mock_ownership_service_));
    binder_service_->InitForTesting();
    nvram_proxy_.reset(
        new TpmNvramBinderProxy(binder_service_->GetITpmNvram()));
    ownership_proxy_.reset(
        new TpmOwnershipBinderProxy(binder_service_->GetITpmOwnership()));
  }

  template <typename ResponseProtobufType>
  base::Callback<void(const ResponseProtobufType&)> GetCallback(
      ResponseProtobufType* proto) {
    return base::Bind(
        [](ResponseProtobufType* proto, const ResponseProtobufType& response) {
          *proto = response;
        },
        base::Unretained(proto));
  }

 protected:
  StrictMock<MockTpmNvramInterface> mock_nvram_service_;
  StrictMock<MockTpmOwnershipInterface> mock_ownership_service_;
  std::unique_ptr<BinderService> binder_service_;
  std::unique_ptr<TpmNvramBinderProxy> nvram_proxy_;
  std::unique_ptr<TpmOwnershipBinderProxy> ownership_proxy_;
};

TEST_F(BinderServiceTest, CopyableCallback) {
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
  ownership_proxy_->GetTpmStatus(request,
                                 GetCallback<GetTpmStatusReply>(&reply));
}

TEST_F(BinderServiceTest, GetTpmStatus) {
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
  ownership_proxy_->GetTpmStatus(request,
                                 GetCallback<GetTpmStatusReply>(&reply));
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.enabled());
  EXPECT_TRUE(reply.owned());
  EXPECT_EQ(3, reply.dictionary_attack_counter());
  EXPECT_EQ(4, reply.dictionary_attack_threshold());
  EXPECT_TRUE(reply.dictionary_attack_lockout_in_effect());
  EXPECT_EQ(5, reply.dictionary_attack_lockout_seconds_remaining());
}

TEST_F(BinderServiceTest, TakeOwnership) {
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
  ownership_proxy_->TakeOwnership(request,
                                  GetCallback<TakeOwnershipReply>(&reply));
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(BinderServiceTest, RemoveOwnerDependency) {
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
  ownership_proxy_->RemoveOwnerDependency(
      request, GetCallback<RemoveOwnerDependencyReply>(&reply));
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(BinderServiceTest, DefineSpace) {
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
  nvram_proxy_->DefineSpace(request, GetCallback<DefineSpaceReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(BinderServiceTest, DestroySpace) {
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
  nvram_proxy_->DestroySpace(request, GetCallback<DestroySpaceReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(BinderServiceTest, WriteSpace) {
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
  nvram_proxy_->WriteSpace(request, GetCallback<WriteSpaceReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(BinderServiceTest, ReadSpace) {
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
  nvram_proxy_->ReadSpace(request, GetCallback<ReadSpaceReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  EXPECT_TRUE(reply.has_data());
  EXPECT_EQ(nvram_data, reply.data());
}

TEST_F(BinderServiceTest, LockSpace) {
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
  nvram_proxy_->LockSpace(request, GetCallback<LockSpaceReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
}

TEST_F(BinderServiceTest, ListSpaces) {
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
  nvram_proxy_->ListSpaces(request, GetCallback<ListSpacesReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  EXPECT_EQ(arraysize(nvram_index_list), reply.index_list_size());
  for (size_t i = 0; i < 3; i++) {
    EXPECT_EQ(nvram_index_list[i], reply.index_list(i));
  }
}

TEST_F(BinderServiceTest, GetSpaceInfo) {
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
  nvram_proxy_->GetSpaceInfo(request, GetCallback<GetSpaceInfoReply>(&reply));
  EXPECT_EQ(NVRAM_RESULT_SUCCESS, reply.result());
  EXPECT_TRUE(reply.has_size());
  EXPECT_EQ(nvram_size, reply.size());
  EXPECT_TRUE(reply.is_read_locked());
  EXPECT_TRUE(reply.is_write_locked());
}

}  // namespace tpm_manager
