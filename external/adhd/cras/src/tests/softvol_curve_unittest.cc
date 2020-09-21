// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "softvol_curve.h"
}

namespace {

static float ABS_ERROR = 0.0000001;

TEST(SoftvolCurveTest, ScalerDecibelConvert) {
  float scaler;
  scaler = convert_softvol_scaler_from_dB(-2000);
  EXPECT_NEAR(scaler, 0.1f, ABS_ERROR);
  scaler = convert_softvol_scaler_from_dB(-1000);
  EXPECT_NEAR(scaler, 0.3162277f, ABS_ERROR);
  scaler = convert_softvol_scaler_from_dB(-4000);
  EXPECT_NEAR(scaler, 0.01f, ABS_ERROR);
  scaler = convert_softvol_scaler_from_dB(-3500);
  EXPECT_NEAR(scaler, 0.0177828f, ABS_ERROR);
}

}  //  namespace

/* Stubs */
extern "C" {

}  // extern "C"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

