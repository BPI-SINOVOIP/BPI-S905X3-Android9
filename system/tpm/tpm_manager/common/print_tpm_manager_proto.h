// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED.

#ifndef TPM_MANAGER_COMMON_PRINT_TPM_MANAGER_PROTO_H_
#define TPM_MANAGER_COMMON_PRINT_TPM_MANAGER_PROTO_H_

#include <string>

#include "tpm_manager/common/tpm_manager.pb.h"

namespace tpm_manager {

std::string GetProtoDebugStringWithIndent(TpmManagerStatus value,
                                          int indent_size);
std::string GetProtoDebugString(TpmManagerStatus value);
std::string GetProtoDebugStringWithIndent(NvramResult value, int indent_size);
std::string GetProtoDebugString(NvramResult value);
std::string GetProtoDebugStringWithIndent(NvramSpaceAttribute value,
                                          int indent_size);
std::string GetProtoDebugString(NvramSpaceAttribute value);
std::string GetProtoDebugStringWithIndent(NvramSpacePolicy value,
                                          int indent_size);
std::string GetProtoDebugString(NvramSpacePolicy value);
std::string GetProtoDebugStringWithIndent(const NvramPolicyRecord& value,
                                          int indent_size);
std::string GetProtoDebugString(const NvramPolicyRecord& value);
std::string GetProtoDebugStringWithIndent(const LocalData& value,
                                          int indent_size);
std::string GetProtoDebugString(const LocalData& value);
std::string GetProtoDebugStringWithIndent(const DefineSpaceRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const DefineSpaceRequest& value);
std::string GetProtoDebugStringWithIndent(const DefineSpaceReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const DefineSpaceReply& value);
std::string GetProtoDebugStringWithIndent(const DestroySpaceRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const DestroySpaceRequest& value);
std::string GetProtoDebugStringWithIndent(const DestroySpaceReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const DestroySpaceReply& value);
std::string GetProtoDebugStringWithIndent(const WriteSpaceRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const WriteSpaceRequest& value);
std::string GetProtoDebugStringWithIndent(const WriteSpaceReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const WriteSpaceReply& value);
std::string GetProtoDebugStringWithIndent(const ReadSpaceRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const ReadSpaceRequest& value);
std::string GetProtoDebugStringWithIndent(const ReadSpaceReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const ReadSpaceReply& value);
std::string GetProtoDebugStringWithIndent(const LockSpaceRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const LockSpaceRequest& value);
std::string GetProtoDebugStringWithIndent(const LockSpaceReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const LockSpaceReply& value);
std::string GetProtoDebugStringWithIndent(const ListSpacesRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const ListSpacesRequest& value);
std::string GetProtoDebugStringWithIndent(const ListSpacesReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const ListSpacesReply& value);
std::string GetProtoDebugStringWithIndent(const GetSpaceInfoRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetSpaceInfoRequest& value);
std::string GetProtoDebugStringWithIndent(const GetSpaceInfoReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetSpaceInfoReply& value);
std::string GetProtoDebugStringWithIndent(const GetTpmStatusRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetTpmStatusRequest& value);
std::string GetProtoDebugStringWithIndent(const GetTpmStatusReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetTpmStatusReply& value);
std::string GetProtoDebugStringWithIndent(const TakeOwnershipRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const TakeOwnershipRequest& value);
std::string GetProtoDebugStringWithIndent(const TakeOwnershipReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const TakeOwnershipReply& value);
std::string GetProtoDebugStringWithIndent(
    const RemoveOwnerDependencyRequest& value,
    int indent_size);
std::string GetProtoDebugString(const RemoveOwnerDependencyRequest& value);
std::string GetProtoDebugStringWithIndent(
    const RemoveOwnerDependencyReply& value,
    int indent_size);
std::string GetProtoDebugString(const RemoveOwnerDependencyReply& value);

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_PRINT_TPM_MANAGER_PROTO_H_
