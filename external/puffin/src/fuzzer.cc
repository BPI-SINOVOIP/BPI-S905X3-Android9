// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/logging.h"
#include "brillo/test_helpers.h"

#include "puffin/src/bit_reader.h"
#include "puffin/src/bit_writer.h"
#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/huffer.h"
#include "puffin/src/include/puffin/puffer.h"
#include "puffin/src/include/puffin/puffpatch.h"
#include "puffin/src/memory_stream.h"
#include "puffin/src/puff_reader.h"
#include "puffin/src/puff_writer.h"

using std::vector;

using puffin::BitExtent;
using puffin::Buffer;
using puffin::BufferBitReader;
using puffin::BufferBitWriter;
using puffin::BufferPuffReader;
using puffin::BufferPuffWriter;
using puffin::Error;
using puffin::Huffer;
using puffin::MemoryStream;
using puffin::Puffer;

namespace {
void FuzzPuff(const uint8_t* data, size_t size) {
  BufferBitReader bit_reader(data, size);
  Buffer puff_buffer(size * 2);
  BufferPuffWriter puff_writer(puff_buffer.data(), puff_buffer.size());
  vector<BitExtent> bit_extents;
  Error error;
  Puffer puffer;
  puffer.PuffDeflate(&bit_reader, &puff_writer, &bit_extents, &error);
}

void FuzzHuff(const uint8_t* data, size_t size) {
  BufferPuffReader puff_reader(data, size);
  Buffer deflate_buffer(size);
  BufferBitWriter bit_writer(deflate_buffer.data(), deflate_buffer.size());
  Error error;
  Huffer huffer;
  huffer.HuffDeflate(&puff_reader, &bit_writer, &error);
}

void FuzzPuffPatch(const uint8_t* data, size_t size) {
  const size_t kBufferSize = 1000;
  Buffer src_buffer(kBufferSize);
  Buffer dst_buffer(kBufferSize);
  auto src = MemoryStream::CreateForRead(src_buffer);
  auto dst = MemoryStream::CreateForWrite(&dst_buffer);
  puffin::PuffPatch(std::move(src), std::move(dst), data, size, kBufferSize);
}

struct Environment {
  Environment() {
    // To turn off the logging.
    logging::SetMinLogLevel(logging::LOG_FATAL);

    // To turn off logging for bsdiff library.
    std::cerr.setstate(std::ios_base::failbit);
  }
};
Environment* env = new Environment();

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FuzzPuff(data, size);
  FuzzHuff(data, size);
  FuzzPuffPatch(data, size);
  return 0;
}
