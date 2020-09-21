// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

extern "C" {
// Test static functions
#include "cras_audio_format.c"
}

namespace {

class ChannelConvMtxTestSuite : public testing::Test {
  protected:
    virtual void SetUp() {
      int i;
      in_fmt = cras_audio_format_create(SND_PCM_FORMAT_S16_LE, 44100, 6);
      out_fmt = cras_audio_format_create(SND_PCM_FORMAT_S16_LE, 44100, 6);
      for (i = 0; i < CRAS_CH_MAX; i++) {
        in_fmt->channel_layout[i] = -1;
        out_fmt->channel_layout[i] = -1;
      }
    }

    virtual void TearDown() {
      cras_audio_format_destroy(in_fmt);
      cras_audio_format_destroy(out_fmt);
      if (conv_mtx)
        cras_channel_conv_matrix_destroy(conv_mtx, 6);
    }

    struct cras_audio_format *in_fmt;
    struct cras_audio_format *out_fmt;
    float **conv_mtx;
};

TEST_F(ChannelConvMtxTestSuite, MatrixCreateSuccess) {
  in_fmt->channel_layout[0] = 5;
  in_fmt->channel_layout[1] = 4;
  in_fmt->channel_layout[2] = 3;
  in_fmt->channel_layout[3] = 2;
  in_fmt->channel_layout[4] = 1;
  in_fmt->channel_layout[5] = 0;

  out_fmt->channel_layout[0] = 0;
  out_fmt->channel_layout[1] = 1;
  out_fmt->channel_layout[2] = 2;
  out_fmt->channel_layout[3] = 3;
  out_fmt->channel_layout[4] = 4;
  out_fmt->channel_layout[5] = 5;

  conv_mtx = cras_channel_conv_matrix_create(in_fmt, out_fmt);
  ASSERT_NE(conv_mtx, (void *)NULL);
}

TEST_F(ChannelConvMtxTestSuite, MatrixCreateFail) {
  in_fmt->channel_layout[0] = 5;
  in_fmt->channel_layout[1] = 4;
  in_fmt->channel_layout[2] = 3;
  in_fmt->channel_layout[3] = 2;
  in_fmt->channel_layout[4] = 1;
  in_fmt->channel_layout[5] = 0;

  out_fmt->channel_layout[0] = 0;
  out_fmt->channel_layout[1] = 1;
  out_fmt->channel_layout[2] = 2;
  out_fmt->channel_layout[3] = 3;
  out_fmt->channel_layout[4] = 4;
  out_fmt->channel_layout[7] = 5;

  conv_mtx = cras_channel_conv_matrix_create(in_fmt, out_fmt);
  ASSERT_EQ(conv_mtx, (void *)NULL);
}

TEST_F(ChannelConvMtxTestSuite, SLSRToRRRL) {
  in_fmt->channel_layout[0] = 0;
  in_fmt->channel_layout[1] = 1;
  in_fmt->channel_layout[4] = 2;
  in_fmt->channel_layout[5] = 3;
  /* Input format uses SL and SR*/
  in_fmt->channel_layout[6] = 4;
  in_fmt->channel_layout[7] = 5;

  out_fmt->channel_layout[0] = 0;
  out_fmt->channel_layout[1] = 1;
  /* Output format uses RR and RR */
  out_fmt->channel_layout[2] = 4;
  out_fmt->channel_layout[3] = 5;
  out_fmt->channel_layout[4] = 2;
  out_fmt->channel_layout[5] = 3;

  conv_mtx = cras_channel_conv_matrix_create(in_fmt, out_fmt);
  ASSERT_NE(conv_mtx, (void *)NULL);
}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
