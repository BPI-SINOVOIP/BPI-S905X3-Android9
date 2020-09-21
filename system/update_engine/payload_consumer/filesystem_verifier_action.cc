//
// Copyright (C) 2012 The Android Open Source Project
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

#include "update_engine/payload_consumer/filesystem_verifier_action.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdlib>
#include <string>

#include <base/bind.h>
#include <brillo/data_encoding.h>
#include <brillo/streams/file_stream.h>

#include "update_engine/common/boot_control_interface.h"
#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/delta_performer.h"
#include "update_engine/payload_consumer/payload_constants.h"

using brillo::data_encoding::Base64Encode;
using std::string;

namespace chromeos_update_engine {

namespace {
const off_t kReadFileBufferSize = 128 * 1024;
}  // namespace

void FilesystemVerifierAction::PerformAction() {
  // Will tell the ActionProcessor we've failed if we return.
  ScopedActionCompleter abort_action_completer(processor_, this);

  if (!HasInputObject()) {
    LOG(ERROR) << "FilesystemVerifierAction missing input object.";
    return;
  }
  install_plan_ = GetInputObject();

  if (install_plan_.partitions.empty()) {
    LOG(INFO) << "No partitions to verify.";
    if (HasOutputPipe())
      SetOutputObject(install_plan_);
    abort_action_completer.set_code(ErrorCode::kSuccess);
    return;
  }

  StartPartitionHashing();
  abort_action_completer.set_should_complete(false);
}

void FilesystemVerifierAction::TerminateProcessing() {
  cancelled_ = true;
  Cleanup(ErrorCode::kSuccess);  // error code is ignored if canceled_ is true.
}

bool FilesystemVerifierAction::IsCleanupPending() const {
  return src_stream_ != nullptr;
}

void FilesystemVerifierAction::Cleanup(ErrorCode code) {
  src_stream_.reset();
  // This memory is not used anymore.
  buffer_.clear();

  if (cancelled_)
    return;
  if (code == ErrorCode::kSuccess && HasOutputPipe())
    SetOutputObject(install_plan_);
  processor_->ActionComplete(this, code);
}

void FilesystemVerifierAction::StartPartitionHashing() {
  if (partition_index_ == install_plan_.partitions.size()) {
    Cleanup(ErrorCode::kSuccess);
    return;
  }
  InstallPlan::Partition& partition =
      install_plan_.partitions[partition_index_];

  string part_path;
  switch (verifier_step_) {
    case VerifierStep::kVerifySourceHash:
      part_path = partition.source_path;
      remaining_size_ = partition.source_size;
      break;
    case VerifierStep::kVerifyTargetHash:
      part_path = partition.target_path;
      remaining_size_ = partition.target_size;
      break;
  }
  LOG(INFO) << "Hashing partition " << partition_index_ << " ("
            << partition.name << ") on device " << part_path;
  if (part_path.empty())
    return Cleanup(ErrorCode::kFilesystemVerifierError);

  brillo::ErrorPtr error;
  src_stream_ = brillo::FileStream::Open(
      base::FilePath(part_path),
      brillo::Stream::AccessMode::READ,
      brillo::FileStream::Disposition::OPEN_EXISTING,
      &error);

  if (!src_stream_) {
    LOG(ERROR) << "Unable to open " << part_path << " for reading";
    return Cleanup(ErrorCode::kFilesystemVerifierError);
  }

  buffer_.resize(kReadFileBufferSize);
  read_done_ = false;
  hasher_.reset(new HashCalculator());

  // Start the first read.
  ScheduleRead();
}

void FilesystemVerifierAction::ScheduleRead() {
  size_t bytes_to_read = std::min(static_cast<int64_t>(buffer_.size()),
                                  remaining_size_);
  if (!bytes_to_read) {
    OnReadDoneCallback(0);
    return;
  }

  bool read_async_ok = src_stream_->ReadAsync(
    buffer_.data(),
    bytes_to_read,
    base::Bind(&FilesystemVerifierAction::OnReadDoneCallback,
               base::Unretained(this)),
    base::Bind(&FilesystemVerifierAction::OnReadErrorCallback,
               base::Unretained(this)),
    nullptr);

  if (!read_async_ok) {
    LOG(ERROR) << "Unable to schedule an asynchronous read from the stream.";
    Cleanup(ErrorCode::kError);
  }
}

void FilesystemVerifierAction::OnReadDoneCallback(size_t bytes_read) {
  if (bytes_read == 0) {
    read_done_ = true;
  } else {
    remaining_size_ -= bytes_read;
    CHECK(!read_done_);
    if (!hasher_->Update(buffer_.data(), bytes_read)) {
      LOG(ERROR) << "Unable to update the hash.";
      Cleanup(ErrorCode::kError);
      return;
    }
  }

  // We either terminate the current partition or have more data to read.
  if (cancelled_)
    return Cleanup(ErrorCode::kError);

  if (read_done_ || remaining_size_ == 0) {
    if (remaining_size_ != 0) {
      LOG(ERROR) << "Failed to read the remaining " << remaining_size_
                 << " bytes from partition "
                 << install_plan_.partitions[partition_index_].name;
      return Cleanup(ErrorCode::kFilesystemVerifierError);
    }
    return FinishPartitionHashing();
  }
  ScheduleRead();
}

void FilesystemVerifierAction::OnReadErrorCallback(
      const brillo::Error* error) {
  // TODO(deymo): Transform the read-error into an specific ErrorCode.
  LOG(ERROR) << "Asynchronous read failed.";
  Cleanup(ErrorCode::kError);
}

void FilesystemVerifierAction::FinishPartitionHashing() {
  if (!hasher_->Finalize()) {
    LOG(ERROR) << "Unable to finalize the hash.";
    return Cleanup(ErrorCode::kError);
  }
  InstallPlan::Partition& partition =
      install_plan_.partitions[partition_index_];
  LOG(INFO) << "Hash of " << partition.name << ": "
            << Base64Encode(hasher_->raw_hash());

  switch (verifier_step_) {
    case VerifierStep::kVerifyTargetHash:
      if (partition.target_hash != hasher_->raw_hash()) {
        LOG(ERROR) << "New '" << partition.name
                   << "' partition verification failed.";
        if (partition.source_hash.empty()) {
          // No need to verify source if it is a full payload.
          return Cleanup(ErrorCode::kNewRootfsVerificationError);
        }
        // If we have not verified source partition yet, now that the target
        // partition does not match, and it's not a full payload, we need to
        // switch to kVerifySourceHash step to check if it's because the source
        // partition does not match either.
        verifier_step_ = VerifierStep::kVerifySourceHash;
      } else {
        partition_index_++;
      }
      break;
    case VerifierStep::kVerifySourceHash:
      if (partition.source_hash != hasher_->raw_hash()) {
        LOG(ERROR) << "Old '" << partition.name
                   << "' partition verification failed.";
        LOG(ERROR) << "This is a server-side error due to mismatched delta"
                   << " update image!";
        LOG(ERROR) << "The delta I've been given contains a " << partition.name
                   << " delta update that must be applied over a "
                   << partition.name << " with a specific checksum, but the "
                   << partition.name
                   << " we're starting with doesn't have that checksum! This"
                      " means that the delta I've been given doesn't match my"
                      " existing system. The "
                   << partition.name << " partition I have has hash: "
                   << Base64Encode(hasher_->raw_hash())
                   << " but the update expected me to have "
                   << Base64Encode(partition.source_hash) << " .";
        LOG(INFO) << "To get the checksum of the " << partition.name
                  << " partition run this command: dd if="
                  << partition.source_path
                  << " bs=1M count=" << partition.source_size
                  << " iflag=count_bytes 2>/dev/null | openssl dgst -sha256 "
                     "-binary | openssl base64";
        LOG(INFO) << "To get the checksum of partitions in a bin file, "
                  << "run: .../src/scripts/sha256_partitions.sh .../file.bin";
        return Cleanup(ErrorCode::kDownloadStateInitializationError);
      }
      // The action will skip kVerifySourceHash step if target partition hash
      // matches, if we are in this step, it means target hash does not match,
      // and now that the source partition hash matches, we should set the error
      // code to reflect the error in target partition.
      // We only need to verify the source partition which the target hash does
      // not match, the rest of the partitions don't matter.
      return Cleanup(ErrorCode::kNewRootfsVerificationError);
  }
  // Start hashing the next partition, if any.
  hasher_.reset();
  buffer_.clear();
  src_stream_->CloseBlocking(nullptr);
  StartPartitionHashing();
}

}  // namespace chromeos_update_engine
