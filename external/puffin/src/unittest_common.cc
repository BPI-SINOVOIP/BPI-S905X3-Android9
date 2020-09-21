// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "puffin/src/unittest_common.h"

namespace puffin {

using std::string;

bool MakeTempFile(string* filename, int* fd) {
  char tmp_template[] = "/tmp/puffin-XXXXXX";
  int mkstemp_fd = mkstemp(tmp_template);
  TEST_AND_RETURN_FALSE(mkstemp_fd >= 0);
  if (filename) {
    *filename = tmp_template;
  }
  if (fd) {
    *fd = mkstemp_fd;
  } else {
    close(mkstemp_fd);
  }
  return true;
}

}  // namespace puffin
