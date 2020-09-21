#!/bin/bash -eux
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Builds fuzzers from within a container into /out/ directory.
# Expects /src/tpm2 to contain tpm2 checkout.

mkdir $WORK/tpm2
cd $SRC/tpm2
make V=1 obj=$WORK/tpm2 $WORK/tpm2/libtpm2.a

$CXX $CXXFLAGS $FUZZER_LDFLAGS -std=c++11 \
  $SRC/tpm2/fuzz/execute-command.cc -o $OUT/tpm2_execute_command_fuzzer \
  -I $SRC/tpm2 \
  $WORK/tpm2/libtpm2.a \
  /usr/lib/x86_64-linux-gnu/libcrypto.a /usr/lib/x86_64-linux-gnu/libssl.a \
  -lFuzzingEngine
