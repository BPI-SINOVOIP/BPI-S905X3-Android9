// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_SET_ERRORS_H_
#define SRC_SET_ERRORS_H_

#ifdef USE_BRILLO
#include "base/logging.h"
#else
#include "glog/logging.h"
#endif

namespace puffin {

#define TEST_AND_RETURN_FALSE(_x)   \
  do {                              \
    if (!(_x)) {                    \
      LOG(ERROR) << #_x " failed."; \
      return false;                 \
    }                               \
  } while (0)

#define TEST_AND_RETURN_VALUE(_x, _v) \
  do {                                \
    if (!(_x)) {                      \
      LOG(ERROR) << #_x " failed.";   \
      return (_v);                    \
    }                                 \
  } while (0)

#define TEST_AND_RETURN_FALSE_SET_ERROR(_x, _error) \
  do {                                              \
    if (!(_x)) {                                    \
      (*error) = (_error);                          \
      LOG(ERROR) << #_x " failed.";                 \
      return false;                                 \
    }                                               \
  } while (0)

}  // namespace puffin

#endif  // SRC_SET_ERRORS_H_
