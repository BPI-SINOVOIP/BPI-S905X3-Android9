/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "hash.h"

namespace android {
namespace os {
namespace statsd {

namespace {
// Lower-level versions of Get... that read directly from a character buffer
// without any bounds checking.
inline uint32_t DecodeFixed32(const char *ptr) {
  return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0]))) |
          (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8) |
          (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16) |
          (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
}

inline uint64_t DecodeFixed64(const char* ptr) {
    uint64_t lo = DecodeFixed32(ptr);
    uint64_t hi = DecodeFixed32(ptr + 4);
    return (hi << 32) | lo;
}

// 0xff is in case char is signed.
static inline uint32_t ByteAs32(char c) { return static_cast<uint32_t>(c) & 0xff; }
static inline uint64_t ByteAs64(char c) { return static_cast<uint64_t>(c) & 0xff; }

}  // namespace

uint32_t Hash32(const char *data, size_t n, uint32_t seed) {
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  // Initialize the hash to a 'random' value
  uint32_t h = static_cast<uint32_t>(seed ^ n);

  // Mix 4 bytes at a time into the hash
  while (n >= 4) {
    uint32_t k = DecodeFixed32(data);
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
    data += 4;
    n -= 4;
  }

  // Handle the last few bytes of the input array
  switch (n) {
    case 3:
      h ^= ByteAs32(data[2]) << 16;
    case 2:
      h ^= ByteAs32(data[1]) << 8;
    case 1:
      h ^= ByteAs32(data[0]);
      h *= m;
  }

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

uint64_t Hash64(const char* data, size_t n, uint64_t seed) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (n * m);

  while (n >= 8) {
    uint64_t k = DecodeFixed64(data);
    data += 8;
    n -= 8;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  switch (n) {
    case 7:
      h ^= ByteAs64(data[6]) << 48;
    case 6:
      h ^= ByteAs64(data[5]) << 40;
    case 5:
      h ^= ByteAs64(data[4]) << 32;
    case 4:
      h ^= ByteAs64(data[3]) << 24;
    case 3:
      h ^= ByteAs64(data[2]) << 16;
    case 2:
      h ^= ByteAs64(data[1]) << 8;
    case 1:
      h ^= ByteAs64(data[0]);
      h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}
}  // namespace statsd
}  // namespace os
}  // namespace android
