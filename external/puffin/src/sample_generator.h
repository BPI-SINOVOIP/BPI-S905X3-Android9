// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_SAMPLE_GENERATOR_H_
#define SRC_SAMPLE_GENERATOR_H_

#include <string>
#include <vector>

#include "puffin/src/include/puffin/puffer.h"

namespace puffin {
namespace sample_generator {

void PrintArray(const std::string& name, const Buffer& array);

// Creates and prints a sample for for adding to the list of unit tests for
// puffer/huffer.
//
// Example:
//   Buffer orig = {1, 2, 3, 4, 5};
//   Puffer puffer;
//   sample_generator::PrintSample(
//     &puffer, Z_DEFAULT_COMPRESSION, Z_FIXED, orig);
bool PrintSample(Puffer* puffer,
                 int compression,
                 int strategy,
                 const Buffer& original);

}  // namespace sample_generator
}  // namespace puffin

#endif  // SRC_SAMPLE_GENERATOR_H_
