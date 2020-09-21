// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "puffin/src/sample_generator.h"

#include <malloc.h>
#include <zlib.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "puffin/src/bit_reader.h"
#include "puffin/src/puff_writer.h"
#include "puffin/src/set_errors.h"

namespace puffin {
namespace sample_generator {

using std::cerr;
using std::cout;
using std::endl;
using std::string;

bool CompressToDeflate(const Buffer& uncomp,
                      Buffer* comp,
                      int compression,
                      int strategy) {
  z_stream stream;
  stream.next_in = (z_const Bytef*)uncomp.data();
  stream.avail_in = static_cast<unsigned int>(uncomp.size());
  stream.next_out = comp->data();
  stream.avail_out = comp->size();

  stream.zalloc = nullptr;
  stream.zfree = nullptr;
  stream.opaque = nullptr;

  TEST_AND_RETURN_FALSE(
      Z_OK == deflateInit2(&stream, compression, Z_DEFLATED, -15, 8, strategy));
  // If there was not enough output available return error.
  TEST_AND_RETURN_FALSE(Z_STREAM_END == deflate(&stream, Z_FINISH));
  TEST_AND_RETURN_FALSE(Z_OK == deflateEnd(&stream));
  comp->resize(stream.total_out);
  return true;
}

// Print an array into hex-format to the output. This can be used to create
// static arrays for unit testing of the puffer/huffer.
void PrintArray(const string& name, const Buffer& array) {
  cout << "const Buffer " << name << " = {" << endl << " ";
  for (size_t idx = 0; idx < array.size(); idx++) {
    cout << " 0x" << std::hex << std::uppercase << std::setfill('0')
         << std::setw(2) << uint(array[idx]);
    if (idx == array.size() - 1) {
      cout << std::dec << "};" << endl;
      return;
    }
    cout << ",";
    if ((idx + 1) % 12 == 0) {
      cout << endl << " ";
    }
  }
}

bool PrintSample(Puffer* puffer,
                 int compression,
                 int strategy,
                 const Buffer& original) {
  PrintArray("original", original);

  Buffer comp(original.size() * 4 + 10);
  TEST_AND_RETURN_FALSE(
      CompressToDeflate(original, &comp, compression, strategy));
  PrintArray("compressed", comp);

  Buffer puff(original.size() * 3 + 10);
  puffin::Error error;

  BufferBitReader bit_reader(comp.data(), comp.size());
  BufferPuffWriter puff_writer(puff.data(), puff.size());
  TEST_AND_RETURN_FALSE(
      puffer->PuffDeflate(&bit_reader, &puff_writer, nullptr, &error));
  TEST_AND_RETURN_FALSE(comp.size() == bit_reader.Offset());

  puff.resize(puff_writer.Size());
  PrintArray("puffed", puff);
  return true;
}

}  // namespace sample_generator
}  // namespace puffin
