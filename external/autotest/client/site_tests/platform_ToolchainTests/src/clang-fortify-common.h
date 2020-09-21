// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

struct Failure {
  int line;
  const char *message;
  bool expected_death;
};

// Tests FORTIFY with -D_FORTIFY_SOURCE=1
std::vector<Failure> test_fortify_1();
// Tests FORTIFY with -D_FORTIFY_SOURCE=2
std::vector<Failure> test_fortify_2();
