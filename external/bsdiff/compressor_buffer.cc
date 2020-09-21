// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/compressor_buffer.h"

#include "bsdiff/logging.h"

namespace bsdiff {

const std::vector<uint8_t>& CompressorBuffer::GetCompressedData() {
  if (!comp_chunks_.empty()) {
    size_t chunks_size = 0;
    for (const auto& chunk : comp_chunks_)
      chunks_size += chunk.size();
    comp_data_.reserve(comp_data_.size() + chunks_size);
    for (const auto& chunk : comp_chunks_) {
      comp_data_.insert(comp_data_.end(), chunk.begin(), chunk.end());
    }
    comp_chunks_.clear();
  }
  return comp_data_;
}

void CompressorBuffer::AddDataToChunks(size_t data_size) {
  if (data_size > comp_buffer_.size()) {
    LOG(ERROR) << "data size: " << data_size
               << " is larger than buffer size: " << comp_buffer_.size();
    return;
  }
  comp_chunks_.emplace_back(comp_buffer_.data(),
                            comp_buffer_.data() + data_size);
}

}  // namespace bsdiff
