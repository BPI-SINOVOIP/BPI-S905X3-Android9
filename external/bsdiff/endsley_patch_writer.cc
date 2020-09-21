// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/endsley_patch_writer.h"

#include <string.h>

#include <algorithm>

#include "bsdiff/brotli_compressor.h"
#include "bsdiff/bz2_compressor.h"
#include "bsdiff/logging.h"

namespace {

constexpr uint8_t kEndsleyMagicHeader[] = "ENDSLEY/BSDIFF43";

void EncodeInt64(int64_t x, uint8_t* buf) {
  uint64_t y = x < 0 ? (1ULL << 63ULL) - x : x;
  for (int i = 0; i < 8; ++i) {
    buf[i] = y & 0xff;
    y /= 256;
  }
}

// The minimum size that we would consider flushing out.
constexpr size_t kMinimumFlushSize = 1024 * 1024;  // 1 MiB

}  // namespace

namespace bsdiff {

bool EndsleyPatchWriter::Init(size_t new_size) {
  switch (compressor_type_) {
    case CompressorType::kNoCompression:
      // The patch is uncompressed and it will need exactly:
      //   new_size + 24 * len(control_entries) + sizeof(header)
      // We don't know the length of the control entries yet, but we can reserve
      // enough space to hold at least |new_size|.
      patch_->clear();
      patch_->reserve(new_size);
      break;
    case CompressorType::kBrotli:
      compressor_.reset(new BrotliCompressor(quality_));
      if (!compressor_) {
        LOG(ERROR) << "Error creating brotli compressor.";
        return false;
      }
      break;
    case CompressorType::kBZ2:
      compressor_.reset(new BZ2Compressor());
      if (!compressor_) {
        LOG(ERROR) << "Error creating BZ2 compressor.";
        return false;
      }
      break;
  }

  // Header is the magic followed by the new length.
  uint8_t header[24];
  memcpy(header, kEndsleyMagicHeader, 16);
  EncodeInt64(new_size, header + 16);
  EmitBuffer(header, sizeof(header));
  return true;
}

bool EndsleyPatchWriter::WriteDiffStream(const uint8_t* data, size_t size) {
  if (!size)
    return true;
  // Speed-up the common case where the diff stream data is added right after
  // the control entry that refers to it.
  if (control_.empty() && pending_diff_ >= size) {
    pending_diff_ -= size;
    EmitBuffer(data, size);
    return true;
  }

  diff_data_.insert(diff_data_.end(), data, data + size);
  return true;
}

bool EndsleyPatchWriter::WriteExtraStream(const uint8_t* data, size_t size) {
  if (!size)
    return true;
  // Speed-up the common case where the extra stream data is added right after
  // the diff stream data and the control entry that refers to it. Note that
  // the diff data comes first so we need to make sure it is all out.
  if (control_.empty() && !pending_diff_ && pending_extra_ >= size) {
    pending_extra_ -= size;
    EmitBuffer(data, size);
    return true;
  }

  extra_data_.insert(extra_data_.end(), data, data + size);
  return true;
}

bool EndsleyPatchWriter::AddControlEntry(const ControlEntry& entry) {
  // Speed-up the common case where the control entry is added when there's
  // nothing else pending.
  if (control_.empty() && diff_data_.empty() && extra_data_.empty() &&
      !pending_diff_ && !pending_extra_) {
    pending_diff_ = entry.diff_size;
    pending_extra_ = entry.extra_size;
    EmitControlEntry(entry);
    return true;
  }

  control_.push_back(entry);
  pending_control_data_ += entry.diff_size + entry.extra_size;

  // Check whether it is worth Flushing the entries now that the we have more
  // control entries. We need control entries to write enough output data, and
  // we need that output data to be at least 50% of the available diff and extra
  // data. This last requirement is to reduce the overhead of removing the
  // flushed data.
  if (pending_control_data_ > kMinimumFlushSize &&
      (diff_data_.size() + extra_data_.size()) / 2 <= pending_control_data_) {
    Flush();
  }

  return true;
}

bool EndsleyPatchWriter::Close() {
  // Flush any pending data.
  Flush();

  if (pending_diff_ || pending_extra_ || !control_.empty()) {
    LOG(ERROR) << "Insufficient data sent to diff/extra streams";
    return false;
  }

  if (!diff_data_.empty() || !extra_data_.empty()) {
    LOG(ERROR) << "Pending data to diff/extra not flushed out on Close()";
    return false;
  }

  if (compressor_) {
    if (!compressor_->Finish())
      return false;
    *patch_ = compressor_->GetCompressedData();
  }

  return true;
}

void EndsleyPatchWriter::EmitControlEntry(const ControlEntry& entry) {
  // Generate the 24 byte control entry.
  uint8_t buf[24];
  EncodeInt64(entry.diff_size, buf);
  EncodeInt64(entry.extra_size, buf + 8);
  EncodeInt64(entry.offset_increment, buf + 16);
  EmitBuffer(buf, sizeof(buf));
}

void EndsleyPatchWriter::EmitBuffer(const uint8_t* data, size_t size) {
  if (compressor_) {
    compressor_->Write(data, size);
  } else {
    patch_->insert(patch_->end(), data, data + size);
  }
}

void EndsleyPatchWriter::Flush() {
  size_t used_diff = 0;
  size_t used_extra = 0;
  size_t used_control = 0;
  do {
    if (!pending_diff_ && !pending_extra_ && used_control < control_.size()) {
      // We can emit a control entry in these conditions.
      const ControlEntry& entry = control_[used_control];
      used_control++;

      pending_diff_ = entry.diff_size;
      pending_extra_ = entry.extra_size;
      pending_control_data_ -= entry.extra_size + entry.diff_size;
      EmitControlEntry(entry);
    }

    if (pending_diff_) {
      size_t diff_size = std::min(diff_data_.size() - used_diff, pending_diff_);
      EmitBuffer(diff_data_.data() + used_diff, diff_size);
      pending_diff_ -= diff_size;
      used_diff += diff_size;
    }

    if (!pending_diff_ && pending_extra_) {
      size_t extra_size =
          std::min(extra_data_.size() - used_extra, pending_extra_);
      EmitBuffer(extra_data_.data() + used_extra, extra_size);
      pending_extra_ -= extra_size;
      used_extra += extra_size;
    }
  } while (!pending_diff_ && !pending_extra_ && used_control < control_.size());

  if (used_diff)
    diff_data_.erase(diff_data_.begin(), diff_data_.begin() + used_diff);
  if (used_extra)
    extra_data_.erase(extra_data_.begin(), extra_data_.begin() + used_extra);
  if (used_control)
    control_.erase(control_.begin(), control_.begin() + used_control);
}

}  // namespace bsdiff
