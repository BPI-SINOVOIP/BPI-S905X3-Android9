// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/patch_reader.h"

#include <string.h>

#include <limits>

#include "bsdiff/brotli_decompressor.h"
#include "bsdiff/bspatch.h"
#include "bsdiff/bz2_decompressor.h"
#include "bsdiff/constants.h"
#include "bsdiff/logging.h"
#include "bsdiff/utils.h"

namespace bsdiff {

bool BsdiffPatchReader::Init(const uint8_t* patch_data, size_t patch_size) {
  //   File format:
  //   0       8    magic header
  //   8       8    X
  //   16      8    Y
  //   24      8    new_file_size
  //   32      X    compressed control block
  //   32+X    Y    compressed diff block
  //   32+X+Y  ???  compressed extra block
  // with control block a set of triples (x,y,z) meaning "add x bytes
  // from oldfile to x bytes from the diff block; copy y bytes from the
  // extra block; seek forwards in oldfile by z bytes".

  if (patch_size < 32) {
    LOG(ERROR) << "Too small to be a bspatch.";
    return false;
  }
  // Check for appropriate magic.
  std::vector<CompressorType> compression_type;
  if (memcmp(patch_data, kLegacyMagicHeader, 8) == 0) {
    // The magic header is "BSDIFF40" for legacy format.
    compression_type = {CompressorType::kBZ2, CompressorType::kBZ2,
                        CompressorType::kBZ2};
  } else if (memcmp(patch_data, kBSDF2MagicHeader, 5) == 0) {
    // The magic header for BSDF2 format:
    // 0 5 BSDF2
    // 5 1 compressed type for control stream
    // 6 1 compressed type for diff stream
    // 7 1 compressed type for extra stream
    for (size_t i = 5; i < 8; i++) {
      uint8_t type = patch_data[i];
      switch (type) {
        case static_cast<uint8_t>(CompressorType::kBZ2):
          compression_type.push_back(CompressorType::kBZ2);
          break;
        case static_cast<uint8_t>(CompressorType::kBrotli):
          compression_type.push_back(CompressorType::kBrotli);
          break;
        default:
          LOG(ERROR) << "Unsupported compression type: "
                     << static_cast<int>(type);
          return false;
      }
    }
  } else {
    LOG(ERROR) << "Not a bsdiff patch.";
    return false;
  }

  // Read lengths from header.
  int64_t ctrl_len = ParseInt64(patch_data + 8);
  int64_t diff_len = ParseInt64(patch_data + 16);
  int64_t signed_newsize = ParseInt64(patch_data + 24);
  if ((ctrl_len < 0) || (diff_len < 0) || (signed_newsize < 0) ||
      (32 + ctrl_len + diff_len > static_cast<int64_t>(patch_size))) {
    LOG(ERROR) << "Corrupt patch.  ctrl_len: " << ctrl_len
               << ", data_len: " << diff_len
               << ", new_file_size: " << signed_newsize
               << ", patch_size: " << patch_size;
    return false;
  }
  new_file_size_ = signed_newsize;

  ctrl_stream_ = CreateDecompressor(compression_type[0]);
  diff_stream_ = CreateDecompressor(compression_type[1]);
  extra_stream_ = CreateDecompressor(compression_type[2]);
  if (!(ctrl_stream_ && diff_stream_ && extra_stream_)) {
    LOG(ERROR) << "uninitialized decompressor stream";
    return false;
  }

  size_t offset = 32;
  if (!ctrl_stream_->SetInputData(const_cast<uint8_t*>(patch_data) + offset,
                                  ctrl_len)) {
    LOG(ERROR) << "Failed to init ctrl stream, ctrl_len: " << ctrl_len;
    return false;
  }

  offset += ctrl_len;
  if (!diff_stream_->SetInputData(const_cast<uint8_t*>(patch_data) + offset,
                                  diff_len)) {
    LOG(ERROR) << "Failed to init ctrl stream, diff_len: " << diff_len;
    return false;
  }

  offset += diff_len;
  if (!extra_stream_->SetInputData(const_cast<uint8_t*>(patch_data) + offset,
                                   patch_size - offset)) {
    LOG(ERROR) << "Failed to init extra stream, extra_offset: " << offset
               << ", patch_size: " << patch_size;
    return false;
  }
  return true;
}

bool BsdiffPatchReader::ParseControlEntry(ControlEntry* control_entry) {
  if (!control_entry)
    return false;

  uint8_t buf[8];
  if (!ctrl_stream_->Read(buf, 8))
    return false;
  int64_t diff_size = ParseInt64(buf);

  if (!ctrl_stream_->Read(buf, 8))
    return false;
  int64_t extra_size = ParseInt64(buf);

  // Sanity check.
  if (diff_size < 0 || extra_size < 0) {
    LOG(ERROR) << "Corrupt patch; diff_size: " << diff_size
               << ", extra_size: " << extra_size;
    return false;
  }

  control_entry->diff_size = diff_size;
  control_entry->extra_size = extra_size;

  if (!ctrl_stream_->Read(buf, 8))
    return false;
  control_entry->offset_increment = ParseInt64(buf);

  return true;
}

bool BsdiffPatchReader::ReadDiffStream(uint8_t* buf, size_t size) {
  return diff_stream_->Read(buf, size);
}

bool BsdiffPatchReader::ReadExtraStream(uint8_t* buf, size_t size) {
  return extra_stream_->Read(buf, size);
}

bool BsdiffPatchReader::Finish() {
  if (!ctrl_stream_->Close()) {
    LOG(ERROR) << "Failed to close the control stream";
    return false;
  }

  if (!diff_stream_->Close()) {
    LOG(ERROR) << "Failed to close the diff stream";
    return false;
  }

  if (!extra_stream_->Close()) {
    LOG(ERROR) << "Failed to close the extra stream";
    return false;
  }
  return true;
}

}  // namespace bsdiff
