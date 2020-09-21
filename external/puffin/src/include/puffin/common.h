// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_INCLUDE_PUFFIN_COMMON_H_
#define SRC_INCLUDE_PUFFIN_COMMON_H_

#include <functional>
#include <memory>
#include <vector>

#ifdef USE_BRILLO
#include "base/macros.h"
#include "brillo/brillo_export.h"
#define PUFFIN_EXPORT BRILLO_EXPORT

#else  // USE_BRILLO

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete
#endif  // DISALLOW_COPY_AND_ASSIGN

#ifndef PUFFIN_EXPORT
#define PUFFIN_EXPORT __attribute__((__visibility__("default")))
#endif  // PUFFIN_EXPORT

#endif  // USE_BRILLO

namespace puffin {

using Buffer = std::vector<uint8_t>;
using UniqueBufferPtr = std::unique_ptr<Buffer>;
using SharedBufferPtr = std::shared_ptr<Buffer>;

// This class is similar to the protobuf generated for |ProtoByteExtent|. We
// defined an extra class so the users of puffin do not have to include
// puffin.pb.h and deal with its use.
struct PUFFIN_EXPORT ByteExtent {
  ByteExtent(uint64_t offset, uint64_t length)
      : offset(offset), length(length) {}

  bool operator==(const ByteExtent& other) const {
    return this->length == other.length && this->offset == other.offset;
  }

  uint64_t offset;
  uint64_t length;
};

struct PUFFIN_EXPORT BitExtent {
  BitExtent(uint64_t offset, uint64_t length)
      : offset(offset), length(length) {}

  bool operator==(const BitExtent& other) const {
    return this->length == other.length && this->offset == other.offset;
  }

  uint64_t offset;
  uint64_t length;
};

}  // namespace puffin

#endif  // SRC_INCLUDE_PUFFIN_COMMON_H_
