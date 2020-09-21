/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util/BigBuffer.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "android-base/logging.h"

namespace aapt {

void* BigBuffer::NextBlockImpl(size_t size) {
  if (!blocks_.empty()) {
    Block& block = blocks_.back();
    if (block.block_size_ - block.size >= size) {
      void* out_buffer = block.buffer.get() + block.size;
      block.size += size;
      size_ += size;
      return out_buffer;
    }
  }

  const size_t actual_size = std::max(block_size_, size);

  Block block = {};

  // Zero-allocate the block's buffer.
  block.buffer = std::unique_ptr<uint8_t[]>(new uint8_t[actual_size]());
  CHECK(block.buffer);

  block.size = size;
  block.block_size_ = actual_size;

  blocks_.push_back(std::move(block));
  size_ += size;
  return blocks_.back().buffer.get();
}

void* BigBuffer::NextBlock(size_t* out_size) {
  if (!blocks_.empty()) {
    Block& block = blocks_.back();
    if (block.size != block.block_size_) {
      void* out_buffer = block.buffer.get() + block.size;
      size_t size = block.block_size_ - block.size;
      block.size = block.block_size_;
      size_ += size;
      *out_size = size;
      return out_buffer;
    }
  }

  // Zero-allocate the block's buffer.
  Block block = {};
  block.buffer = std::unique_ptr<uint8_t[]>(new uint8_t[block_size_]());
  CHECK(block.buffer);
  block.size = block_size_;
  block.block_size_ = block_size_;
  blocks_.push_back(std::move(block));
  size_ += block_size_;
  *out_size = block_size_;
  return blocks_.back().buffer.get();
}

std::string BigBuffer::to_string() const {
  std::string result;
  for (const Block& block : blocks_) {
    result.append(block.buffer.get(), block.buffer.get() + block.size);
  }
  return result;
}

}  // namespace aapt
