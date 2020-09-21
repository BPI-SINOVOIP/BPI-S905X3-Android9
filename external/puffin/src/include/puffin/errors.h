// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_INCLUDE_PUFFIN_ERRORS_H_
#define SRC_INCLUDE_PUFFIN_ERRORS_H_

namespace puffin {

// The types of the errors.
enum class Error {
  kSuccess = 0,
  kInvalidInput,
  kInsufficientInput,
  kInsufficientOutput,
  kStreamIO,
};

}  // namespace puffin

#endif  // SRC_INCLUDE_PUFFIN_ERRORS_H_
