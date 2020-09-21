// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_shm.h"
#include "cras_mix.h"
#include "cras_types.h"

}

namespace {

static const size_t kBufferFrames = 8192;
static const size_t kNumChannels = 2;
static const size_t kNumSamples = kBufferFrames * kNumChannels;


static inline int need_to_scale(float scaler) {
	return (scaler < 0.99 || scaler > 1.01);
}

class MixTestSuiteS16_LE : public testing::Test{
  protected:
    virtual void SetUp() {
      fmt_ = SND_PCM_FORMAT_S16_LE;
      mix_buffer_ = (int16_t *)malloc(kBufferFrames * 4);
      src_buffer_ = static_cast<int16_t *>(
          calloc(1, kBufferFrames * 4 + sizeof(cras_audio_shm_area)));

      for (size_t i = 0; i < kBufferFrames * 2; i++) {
        src_buffer_[i] = i;
        mix_buffer_[i] = -i;
      }

      compare_buffer_ = (int16_t *)malloc(kBufferFrames * 4);
    }

    virtual void TearDown() {
      free(mix_buffer_);
      free(compare_buffer_);
      free(src_buffer_);
    }

    void _SetupBuffer() {
      for (size_t i = 0; i < kBufferFrames; i++) {
        src_buffer_[i] = i + (INT16_MAX >> 2);
        mix_buffer_[i] = i + (INT16_MAX >> 2);
        compare_buffer_[i] = mix_buffer_[i];
      }
      for (size_t i = kBufferFrames; i < kBufferFrames * 2; i++) {
        src_buffer_[i] = i - (INT16_MAX >> 2);
        mix_buffer_[i] = i - (INT16_MAX >> 2);
        compare_buffer_[i] = mix_buffer_[i];
      }
    }

    void TestScaleStride(float scaler) {
      _SetupBuffer();
      for (size_t i = 0; i < kBufferFrames * 2; i += 2) {
        int32_t tmp;
        if (need_to_scale(scaler))
          tmp = mix_buffer_[i] + src_buffer_[i/2] * scaler;
        else
          tmp = mix_buffer_[i] + src_buffer_[i/2];
        if (tmp > INT16_MAX)
          tmp = INT16_MAX;
        else if (tmp < INT16_MIN)
          tmp = INT16_MIN;
        compare_buffer_[i] = tmp;
      }

      cras_mix_add_scale_stride(
          fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
          kBufferFrames, 4, 2, scaler);

      EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
    }

    void ScaleIncrement(float start_scaler, float increment) {
      float scaler = start_scaler;
      for (size_t i = 0; i < kBufferFrames * 2; i++) {
        if (scaler > 0.9999999) {
        } else if (scaler < 0.0000001) {
          compare_buffer_[i] = 0;
        } else {
          compare_buffer_[i] = mix_buffer_[i] * scaler;
        }
        if (i % 2 == 1)
          scaler += increment;
      }
    }

  int16_t *mix_buffer_;
  int16_t *src_buffer_;
  int16_t *compare_buffer_;
  snd_pcm_format_t fmt_;
};

TEST_F(MixTestSuiteS16_LE, MixFirst) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 1.0);
  EXPECT_EQ(0, memcmp(mix_buffer_, src_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, MixTwo) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 2;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, MixTwoClip) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  for (size_t i = 0; i < kBufferFrames * 2; i++)
    src_buffer_[i] = INT16_MAX;
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = INT16_MAX;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, MixFirstMuted) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 1, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, MixFirstZeroVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, MixFirstHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 0.5;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, MixTwoSecondHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] + (int16_t)(src_buffer_[i] * 0.5);
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames*4));
}

TEST_F(MixTestSuiteS16_LE, ScaleFullVolumeIncrement) {
  float increment = 0.01;
  int step = 2;
  float start_scaler = 0.999999999;

  _SetupBuffer();
  // Scale full volume with positive increment will not change buffer.
  memcpy(compare_buffer_, src_buffer_, kBufferFrames * 4);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleMinVolumeIncrement) {
  float increment = -0.01;
  int step = 2;
  float start_scaler = 0.000000001;

  _SetupBuffer();
  // Scale min volume with negative increment will change buffer to zeros.
  memset(compare_buffer_, 0, kBufferFrames * 4);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleVolumePositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.1;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);
  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleVolumeNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 0.8;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleVolumeStartFullNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 1.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleVolumeStartZeroPositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleFullVolume) {
  memcpy(compare_buffer_, src_buffer_, kBufferFrames * 4);
  cras_scale_buffer(fmt_, (uint8_t *)mix_buffer_, kNumSamples, 0.999999999);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleMinVolume) {
  memset(compare_buffer_, 0, kBufferFrames * 4);
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.0000000001);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS16_LE, ScaleHalfVolume) {
  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 0.5;
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.5);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * 4));
}



TEST_F(MixTestSuiteS16_LE, StrideCopy) {
  TestScaleStride(1.0);
  TestScaleStride(100);
  TestScaleStride(0.5);
}

class MixTestSuiteS24_LE : public testing::Test{
  protected:
    virtual void SetUp() {
      fmt_ = SND_PCM_FORMAT_S24_LE;
      fr_bytes_ = 4 * kNumChannels;
      mix_buffer_ = (int32_t *)malloc(kBufferFrames * fr_bytes_);
      src_buffer_ = static_cast<int32_t *>(
          calloc(1, kBufferFrames * fr_bytes_ + sizeof(cras_audio_shm_area)));

      for (size_t i = 0; i < kBufferFrames * 2; i++) {
        src_buffer_[i] = i;
        mix_buffer_[i] = -i;
      }

      compare_buffer_ = (int32_t *)malloc(kBufferFrames * fr_bytes_);
    }

    virtual void TearDown() {
      free(mix_buffer_);
      free(compare_buffer_);
      free(src_buffer_);
    }

    void _SetupBuffer() {
      for (size_t i = 0; i < kBufferFrames; i++) {
        src_buffer_[i] = i + (0x007fffff >> 2);
        mix_buffer_[i] = i + (0x007fffff  >> 2);
        compare_buffer_[i] = mix_buffer_[i];
      }
      for (size_t i = kBufferFrames; i < kBufferFrames * 2; i++) {
        src_buffer_[i] = i - (0x007fffff >> 2);
        mix_buffer_[i] = i - (0x007fffff >> 2);
        compare_buffer_[i] = mix_buffer_[i];
      }
    }

    void TestScaleStride(float scaler) {
      _SetupBuffer();
      for (size_t i = 0; i < kBufferFrames * 2; i += 2) {
        int32_t tmp;
        if (need_to_scale(scaler))
          tmp = mix_buffer_[i] + src_buffer_[i/2] * scaler;
        else
          tmp = mix_buffer_[i] + src_buffer_[i/2];
        if (tmp > 0x007fffff)
          tmp = 0x007fffff;
        else if (tmp < (int32_t)0xff800000)
          tmp = (int32_t)0xff800000;
        compare_buffer_[i] = tmp;
      }

      cras_mix_add_scale_stride(
          fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
          kBufferFrames, 8, 4, scaler);

      EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 8));
    }

    void ScaleIncrement(float start_scaler, float increment) {
      float scaler = start_scaler;
      for (size_t i = 0; i < kBufferFrames * 2; i++) {
        if (scaler > 0.9999999) {
        } else if (scaler < 0.0000001) {
          compare_buffer_[i] = 0;
        } else {
          compare_buffer_[i] = mix_buffer_[i] * scaler;
        }
        if (i % 2 == 1)
          scaler += increment;
      }
    }

  int32_t *mix_buffer_;
  int32_t *src_buffer_;
  int32_t *compare_buffer_;
  snd_pcm_format_t fmt_;
  unsigned int fr_bytes_;
};

TEST_F(MixTestSuiteS24_LE, MixFirst) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 1.0);
  EXPECT_EQ(0, memcmp(mix_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, MixTwo) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 2;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, MixTwoClip) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  for (size_t i = 0; i < kBufferFrames * 2; i++)
    src_buffer_[i] = 0x007fffff;
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0x007fffff;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, MixFirstMuted) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 1, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, MixFirstZeroVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, MixFirstHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 0.5;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, MixTwoSecondHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] + (int32_t)(src_buffer_[i] * 0.5);
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleFullVolumeIncrement) {
  float increment = 0.01;
  int step = 2;
  float start_scaler = 0.999999999;

  _SetupBuffer();
  // Scale full volume with positive increment will not change buffer.
  memcpy(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleMinVolumeIncrement) {
  float increment = -0.01;
  int step = 2;
  float start_scaler = 0.000000001;

  _SetupBuffer();
  // Scale min volume with negative increment will change buffer to zeros.
  memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleVolumePositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.1;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);
  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleVolumeNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 0.8;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleVolumeStartFullNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 1.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS24_LE, ScaleVolumeStartZeroPositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS24_LE, ScaleFullVolume) {
  memcpy(compare_buffer_, src_buffer_, kBufferFrames  * fr_bytes_);
  cras_scale_buffer(fmt_, (uint8_t *)mix_buffer_, kNumSamples, 0.999999999);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleMinVolume) {
  memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.0000000001);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, ScaleHalfVolume) {
  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 0.5;
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.5);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_LE, StrideCopy) {
  TestScaleStride(1.0);
  TestScaleStride(100);
  TestScaleStride(0.1);
}

class MixTestSuiteS32_LE : public testing::Test{
  protected:
    virtual void SetUp() {
      fmt_ = SND_PCM_FORMAT_S32_LE;
      fr_bytes_ = 4 * kNumChannels;
      mix_buffer_ = (int32_t *)malloc(kBufferFrames * fr_bytes_);
      src_buffer_ = static_cast<int32_t *>(
          calloc(1, kBufferFrames * fr_bytes_ + sizeof(cras_audio_shm_area)));

      for (size_t i = 0; i < kBufferFrames * 2; i++) {
        src_buffer_[i] = i;
        mix_buffer_[i] = -i;
      }

      compare_buffer_ = (int32_t *)malloc(kBufferFrames * fr_bytes_);
    }

    virtual void TearDown() {
      free(mix_buffer_);
      free(compare_buffer_);
      free(src_buffer_);
    }

    void _SetupBuffer() {
      for (size_t i = 0; i < kBufferFrames; i++) {
        src_buffer_[i] = i + (INT32_MAX >> 2);
        mix_buffer_[i] = i + (INT32_MAX >> 2);
        compare_buffer_[i] = mix_buffer_[i];
      }
      for (size_t i = kBufferFrames; i < kBufferFrames * 2; i++) {
        src_buffer_[i] = i - (INT32_MAX >> 2);
        mix_buffer_[i] = i - (INT32_MAX >> 2);
        compare_buffer_[i] = mix_buffer_[i];
      }
    }

    void TestScaleStride(float scaler) {
      _SetupBuffer();
      for (size_t i = 0; i < kBufferFrames * 2; i += 2) {
        int64_t tmp;
        if (need_to_scale(scaler))
          tmp = mix_buffer_[i] + src_buffer_[i/2] * scaler;
        else
          tmp = mix_buffer_[i] + src_buffer_[i/2];
        if (tmp > INT32_MAX)
          tmp = INT32_MAX;
        else if (tmp < INT32_MIN)
          tmp = INT32_MIN;
        compare_buffer_[i] = tmp;
      }

      cras_mix_add_scale_stride(
          fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
          kBufferFrames, 8, 4, scaler);

      EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 8));
    }

    void ScaleIncrement(float start_scaler, float increment) {
      float scaler = start_scaler;
      for (size_t i = 0; i < kBufferFrames * 2; i++) {
        if (scaler > 0.9999999) {
        } else if (scaler < 0.0000001) {
          compare_buffer_[i] = 0;
        } else {
          compare_buffer_[i] = mix_buffer_[i] * scaler;
        }
        if (i % 2 == 1)
          scaler += increment;
      }
    }

  int32_t *mix_buffer_;
  int32_t *src_buffer_;
  int32_t *compare_buffer_;
  snd_pcm_format_t fmt_;
  unsigned int fr_bytes_;
};

TEST_F(MixTestSuiteS32_LE, MixFirst) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 1.0);
  EXPECT_EQ(0, memcmp(mix_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, MixTwo) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 2;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, MixTwoClip) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  for (size_t i = 0; i < kBufferFrames * 2; i++)
    src_buffer_[i] = INT32_MAX;
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = INT32_MAX;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, MixFirstMuted) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 1, 1.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, MixFirstZeroVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.0);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = 0;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, MixFirstHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 0.5;
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, MixTwoSecondHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] + (int32_t)(src_buffer_[i] * 0.5);
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleFullVolumeIncrement) {
  float increment = 0.01;
  int step = 2;
  float start_scaler = 0.999999999;

  _SetupBuffer();
  // Scale full volume with positive increment will not change buffer.
  memcpy(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleMinVolumeIncrement) {
  float increment = -0.01;
  int step = 2;
  float start_scaler = 0.000000001;

  _SetupBuffer();
  // Scale min volume with negative increment will change buffer to zeros.
  memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleVolumePositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.1;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);
  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleVolumeNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 0.8;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleVolumeStartFullNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 1.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS32_LE, ScaleVolumeStartZeroPositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS32_LE, ScaleFullVolume) {
  memcpy(compare_buffer_, src_buffer_, kBufferFrames  * fr_bytes_);
  cras_scale_buffer(fmt_, (uint8_t *)mix_buffer_, kNumSamples, 0.999999999);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleMinVolume) {
  memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.0000000001);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, ScaleHalfVolume) {
  for (size_t i = 0; i < kBufferFrames * 2; i++)
    compare_buffer_[i] = src_buffer_[i] * 0.5;
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.5);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS32_LE, StrideCopy) {
  TestScaleStride(1.0);
  TestScaleStride(100);
  TestScaleStride(0.1);
}

class MixTestSuiteS24_3LE : public testing::Test{
  protected:
    virtual void SetUp() {
      fmt_ = SND_PCM_FORMAT_S24_3LE;
      fr_bytes_ = 3 * kNumChannels;
      mix_buffer_ = (uint8_t *)malloc(kBufferFrames * fr_bytes_);
      src_buffer_ = static_cast<uint8_t *>(
          calloc(1, kBufferFrames * fr_bytes_ + sizeof(cras_audio_shm_area)));

      for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
        memcpy(src_buffer_ + 3*i, &i, 3);
        int32_t tmp = -i * 256;
        memcpy(mix_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
      }

      compare_buffer_ = (uint8_t *)malloc(kBufferFrames * fr_bytes_);
    }

    virtual void TearDown() {
      free(mix_buffer_);
      free(compare_buffer_);
      free(src_buffer_);
    }

    void _SetupBuffer() {
      memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
      for (size_t i = 0; i < kBufferFrames; i++) {
        int32_t tmp = (i << 8) + (INT32_MAX >> 2);
        memcpy(src_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
        memcpy(mix_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
        memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
      }
      for (size_t i = kBufferFrames; i < kBufferFrames * 2; i++) {
        int32_t tmp = (i << 8) - (INT32_MAX >> 2);
        memcpy(src_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
        memcpy(mix_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
        memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
      }
    }

    void TestScaleStride(float scaler) {
      _SetupBuffer();
      for (size_t i = 0; i < kBufferFrames * kNumChannels; i += 2) {
        int64_t tmp;
        int32_t src_frame = 0;
        int32_t dst_frame = 0;
        memcpy((uint8_t *)&src_frame + 1, src_buffer_ + 3*i/2, 3);
        memcpy((uint8_t *)&dst_frame + 1, mix_buffer_ + 3*i, 3);
        if (need_to_scale(scaler))
          tmp = (int64_t)dst_frame + (int64_t)src_frame * scaler;
        else
          tmp = (int64_t)dst_frame + (int64_t)src_frame;
        if (tmp > INT32_MAX)
          tmp = INT32_MAX;
        else if (tmp < INT32_MIN)
          tmp = INT32_MIN;
        dst_frame = (int32_t)tmp;
        memcpy(compare_buffer_ + 3*i, (uint8_t *)&dst_frame + 1, 3);
      }

      cras_mix_add_scale_stride(
          fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
          kBufferFrames, 6, 3, scaler);

      EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 6));
    }

    void ScaleIncrement(float start_scaler, float increment) {
      float scaler = start_scaler;
      for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
        int32_t tmp = 0;
        memcpy((uint8_t *)&tmp + 1, src_buffer_ + 3*i, 3);

        if (scaler > 0.9999999) {
	} else if (scaler < 0.0000001) {
	  tmp = 0;
        } else {
          tmp *= scaler;
	}

        memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);

        if (i % 2 == 1)
          scaler += increment;
      }
    }

  uint8_t *mix_buffer_;
  uint8_t *src_buffer_;
  uint8_t *compare_buffer_;
  snd_pcm_format_t fmt_;
  unsigned int fr_bytes_;
};

TEST_F(MixTestSuiteS24_3LE, MixFirst) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  EXPECT_EQ(0, memcmp(mix_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, MixTwo) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
    int32_t tmp = 0;
    memcpy((uint8_t *)&tmp + 1, src_buffer_ + 3*i, 3);
    tmp *= 2;
    memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
  }
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, MixTwoClip) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
    int32_t tmp = INT32_MAX;
    memcpy(src_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
  }
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 1.0);

  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
    int32_t tmp = INT32_MAX;
    memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
  }
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, MixFirstMuted) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 1, 1.0);

  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++)
    memset(compare_buffer_ + 3*i, 0, 3);
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, MixFirstZeroVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.0);

  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++)
    memset(compare_buffer_ + 3*i, 0, 3);
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, MixFirstHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
                      kNumSamples, 0, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
    int32_t tmp = 0;
    memcpy((uint8_t *)&tmp + 1, src_buffer_ + 3*i, 3);
    tmp *= 0.5;
    memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
  }
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, MixTwoSecondHalfVolume) {
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 0, 0, 1.0);
  cras_mix_add(fmt_, (uint8_t *)mix_buffer_, (uint8_t *)src_buffer_,
               kNumSamples, 1, 0, 0.5);

  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
    int32_t tmp1 = 0, tmp2 = 0;
    memcpy((uint8_t *)&tmp1 + 1, src_buffer_ + 3*i, 3);
    memcpy((uint8_t *)&tmp2 + 1, src_buffer_ + 3*i, 3);
    tmp1 = tmp1 + (int32_t)(tmp2 * 0.5);
    memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp1 + 1, 3);
  }
  EXPECT_EQ(0, memcmp(mix_buffer_, compare_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleFullVolumeIncrement) {
  float increment = 0.01;
  int step = 2;
  float start_scaler = 0.999999999;

  _SetupBuffer();
  // Scale full volume with positive increment will not change buffer.
  memcpy(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleMinVolumeIncrement) {
  float increment = -0.01;
  int step = 2;
  float start_scaler = 0.000000001;

  _SetupBuffer();
  // Scale min volume with negative increment will change buffer to zeros.
  memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleVolumePositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.1;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);
  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleVolumeNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 0.8;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleVolumeStartFullNegativeIncrement) {
  float increment = -0.0001;
  int step = 2;
  float start_scaler = 1.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS24_3LE, ScaleVolumeStartZeroPositiveIncrement) {
  float increment = 0.0001;
  int step = 2;
  float start_scaler = 0.0;

  _SetupBuffer();
  ScaleIncrement(start_scaler, increment);

  cras_scale_buffer_increment(
      fmt_, (uint8_t *)mix_buffer_, kBufferFrames, start_scaler, increment, step);

  EXPECT_EQ(0, memcmp(compare_buffer_, mix_buffer_, kBufferFrames * 4));
}

TEST_F(MixTestSuiteS24_3LE, ScaleFullVolume) {
  memcpy(compare_buffer_, src_buffer_, kBufferFrames  * fr_bytes_);
  cras_scale_buffer(fmt_, (uint8_t *)mix_buffer_, kNumSamples, 0.999999999);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleMinVolume) {
  memset(compare_buffer_, 0, kBufferFrames * fr_bytes_);
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.0000000001);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, ScaleHalfVolume) {
  for (size_t i = 0; i < kBufferFrames * kNumChannels; i++) {
    int32_t tmp = 0;
    memcpy((uint8_t *)&tmp + 1, src_buffer_ + 3*i, 3);
    tmp *= 0.5;
    memcpy(compare_buffer_ + 3*i, (uint8_t *)&tmp + 1, 3);
  }
  cras_scale_buffer(fmt_, (uint8_t *)src_buffer_, kNumSamples, 0.5);

  EXPECT_EQ(0, memcmp(compare_buffer_, src_buffer_, kBufferFrames * fr_bytes_));
}

TEST_F(MixTestSuiteS24_3LE, StrideCopy) {
  TestScaleStride(1.0);
  TestScaleStride(100);
  TestScaleStride(0.1);
}

/* Stubs */
extern "C" {

}  // extern "C"

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
