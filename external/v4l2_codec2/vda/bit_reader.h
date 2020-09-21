// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 43ddd7a

#ifndef BIT_READER_H_
#define BIT_READER_H_

#include <stdint.h>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "bit_reader_core.h"

namespace media {

class BitReader : private BitReaderCore::ByteStreamProvider {
 public:
  // Initialize the reader to start reading at |data|, |size| being size
  // of |data| in bytes.
  BitReader(const uint8_t* data, int size);
  ~BitReader() override;

  template<typename T> bool ReadBits(int num_bits, T* out) {
    return bit_reader_core_.ReadBits(num_bits, out);
  }

  bool ReadFlag(bool* flag) {
    return bit_reader_core_.ReadFlag(flag);
  }

  // Read |num_bits| of binary data into |str|. |num_bits| must be a positive
  // multiple of 8. This is not efficient for extracting large strings.
  // If false is returned, |str| may not be valid.
  bool ReadString(int num_bits, std::string* str);

  bool SkipBits(int num_bits) {
    return bit_reader_core_.SkipBits(num_bits);
  }

  int bits_available() const {
    return initial_size_ * 8 - bits_read();
  }

  int bits_read() const {
    return bit_reader_core_.bits_read();
  }

 private:
  // BitReaderCore::ByteStreamProvider implementation.
  int GetBytes(int max_n, const uint8_t** out) override;

  // Total number of bytes that was initially passed to BitReader.
  const int initial_size_;

  // Pointer to the next unread byte in the stream.
  const uint8_t* data_;

  // Bytes left in the stream.
  int bytes_left_;

  BitReaderCore bit_reader_core_;

  DISALLOW_COPY_AND_ASSIGN(BitReader);
};

}  // namespace media

#endif  // BIT_READER_H_
