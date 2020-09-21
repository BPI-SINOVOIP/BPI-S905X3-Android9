// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_COMPRESSOR_BUFFER_H_
#define _BSDIFF_COMPRESSOR_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

namespace bsdiff {

// An utility class to store the output from a compressor. It has a small
// temporary buffer that holds the compressed data every time the compressed
// stream has some output. That buffer is then appended to a list of vectors
// stored in memory. This approach is more efficient than a simple vector
// because it saves some resize / reallocation of memory. And the compressed
// data is copied once no matter how many times we fetch output from the
// compressor.
class CompressorBuffer {
 public:
  explicit CompressorBuffer(size_t buffer_size) {
    comp_buffer_.resize(buffer_size);
  }

  size_t buffer_size() { return comp_buffer_.size(); }
  uint8_t* buffer_data() { return comp_buffer_.data(); }

  // Concatenate the data in |comp_chunks_| and return the data in the form of
  // a single vector.
  const std::vector<uint8_t>& GetCompressedData();

  // Add the data in |comp_buffer_| to the end of |comp_chunks_|.
  void AddDataToChunks(size_t data_size);

 private:
  // A list of chunks of compressed data. The final compressed representation is
  // the concatenation of all the compressed data.
  std::vector<std::vector<uint8_t>> comp_chunks_;

  // A concatenated version of the |comp_chunks_|, used to store the compressed
  // memory after Finish() is called.
  std::vector<uint8_t> comp_data_;

  // A temporary compression buffer for multiple calls to Write().
  std::vector<uint8_t> comp_buffer_;
};

}  // namespace bsdiff

#endif  // _BSDIFF_COMPRESSOR_BUFFER_H_
