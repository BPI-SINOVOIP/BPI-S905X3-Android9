/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#include <ese/bit_spec.h>
#include <gtest/gtest.h>

using ::testing::Test;

struct TestSpec {
  struct {
    struct bit_spec bit[8];
  } single;
  struct {
    struct bit_spec high;
    struct bit_spec mid;
    struct bit_spec low;
  } adjacent;
  struct {
    struct bit_spec all8;
    struct bit_spec upper6;
    struct bit_spec lower6;
  } overlap;
};

const struct TestSpec kTestSpec = {
  .single = {
    .bit = {
      { .value = 1, .shift = 0 },
      { .value = 1, .shift = 1 },
      { .value = 1, .shift = 2 },
      { .value = 1, .shift = 3 },
      { .value = 1, .shift = 4 },
      { .value = 1, .shift = 5 },
      { .value = 1, .shift = 6 },
      { .value = 1, .shift = 7 },
    },
  },
  .adjacent = {
    .high = { .value = 0x3, .shift = 6 },
    .mid = { .value = 0xf, .shift = 2 },
    .low = { .value = 0x3, .shift = 0 },
  },
  .overlap = {
    .all8 = { .value = 0xff, .shift = 0 },
    .upper6 = { .value = 0x3f, .shift = 2 },
    .lower6 = { .value = 0x3f, .shift = 0 },
  },
};

class BitSpecTest : public virtual Test {
 public:
  BitSpecTest() {
  }
  virtual ~BitSpecTest() { }
  virtual void SetUp() { }
  virtual void TearDown() { }
};

TEST_F(BitSpecTest, single_bits_assign_accrue) {
  uint8_t byte = 0;
  uint8_t expected_byte = 0;
  // Accrue bits.
  for (int bit = 0; bit < 8; ++bit) {
    expected_byte |= (1 << bit);
    bs_assign(&byte, kTestSpec.single.bit[bit], 1);
    EXPECT_EQ(expected_byte, byte);
    EXPECT_EQ(1, bs_get(kTestSpec.single.bit[bit], expected_byte));
  }
}

TEST_F(BitSpecTest, single_bits_assign1_individual) {
  // One bit at a time.
  for (int bit = 0; bit < 8; ++bit) {
    uint8_t expected_byte = (1 << bit);
    uint8_t byte = bs_set(0, kTestSpec.single.bit[bit], 1);
    EXPECT_EQ(expected_byte, byte);
    EXPECT_EQ(1, bs_get(kTestSpec.single.bit[bit], expected_byte));
  }
}

TEST_F(BitSpecTest, single_bits_assign0_individual) {
  // One bit at a time.
  for (int bit = 0; bit < 8; ++bit) {
    uint8_t expected_byte = 0xff ^ (1 << bit);
    uint8_t byte = bs_set(0xff, kTestSpec.single.bit[bit], 0);
    EXPECT_EQ(expected_byte, byte);
    EXPECT_EQ(0, bs_get(kTestSpec.single.bit[bit], expected_byte));
  }
}

TEST_F(BitSpecTest, single_bits_clear_individual) {
  // One bit at a time.
  for (int bit = 0; bit < 8; ++bit) {
    uint8_t byte = 0xff;
    uint8_t expected_byte = 0xff ^ (1 << bit);
    byte &= bs_clear(kTestSpec.single.bit[bit]);
    EXPECT_EQ(expected_byte, byte);
    EXPECT_EQ(0, bs_get(kTestSpec.single.bit[bit], expected_byte));
  }
}

TEST_F(BitSpecTest, adjacent_bit_assign) {
  uint8_t byte = 0;
  EXPECT_EQ(0, bs_get(kTestSpec.adjacent.high, byte));
  EXPECT_EQ(0, bs_get(kTestSpec.adjacent.mid, byte));
  EXPECT_EQ(0, bs_get(kTestSpec.adjacent.low, byte));
  byte = 0xff;
  EXPECT_EQ(0x3, bs_get(kTestSpec.adjacent.high, byte));
  EXPECT_EQ(0xf, bs_get(kTestSpec.adjacent.mid, byte));
  EXPECT_EQ(0x3, bs_get(kTestSpec.adjacent.low, byte));
  for (int i = 0; i < 0xf; ++i) {
    bs_assign(&byte, kTestSpec.adjacent.mid, i);
    EXPECT_EQ(0x3, bs_get(kTestSpec.adjacent.high, byte));
    EXPECT_EQ(i, bs_get(kTestSpec.adjacent.mid, byte));
    EXPECT_EQ(0x3, bs_get(kTestSpec.adjacent.low, byte));
  }
  byte = 0xff;
  for (int i = 0; i < 0x3; ++i) {
    bs_assign(&byte, kTestSpec.adjacent.low, i);
    bs_assign(&byte, kTestSpec.adjacent.high, i);
    EXPECT_EQ(i, bs_get(kTestSpec.adjacent.high, byte));
    EXPECT_EQ(0xf, bs_get(kTestSpec.adjacent.mid, byte));
    EXPECT_EQ(i, bs_get(kTestSpec.adjacent.low, byte));
  }
}

TEST_F(BitSpecTest, overlap_bit_assign) {
  uint8_t byte = 0;
  EXPECT_EQ(0, bs_get(kTestSpec.overlap.upper6, byte));
  EXPECT_EQ(0, bs_get(kTestSpec.overlap.lower6, byte));
  EXPECT_EQ(0, bs_get(kTestSpec.overlap.all8, byte));
  byte = 0xff;
  EXPECT_EQ(0x3f, bs_get(kTestSpec.overlap.upper6, byte));
  EXPECT_EQ(0x3f, bs_get(kTestSpec.overlap.lower6, byte));
  EXPECT_EQ(0xff, bs_get(kTestSpec.overlap.all8, byte));

  byte = 0;
  for (int i = 0; i < 0x3f; ++i) {
    bs_assign(&byte, kTestSpec.overlap.lower6, i);
    EXPECT_EQ((i & (0x3f << 2)) >> 2, bs_get(kTestSpec.overlap.upper6, byte));
    EXPECT_EQ(i, bs_get(kTestSpec.overlap.lower6, byte));
    EXPECT_EQ(i, bs_get(kTestSpec.overlap.all8, byte));
  }
  byte = 0;
  for (int i = 0; i < 0x3f; ++i) {
    bs_assign(&byte, kTestSpec.overlap.upper6, i);
    EXPECT_EQ(i, bs_get(kTestSpec.overlap.upper6, byte));
    EXPECT_EQ((i << 2) & 0x3f, bs_get(kTestSpec.overlap.lower6, byte));
    EXPECT_EQ(i << 2, bs_get(kTestSpec.overlap.all8, byte));
  }
  byte = 0;
  for (int i = 0; i < 0xff; ++i) {
    bs_assign(&byte, kTestSpec.overlap.all8, i);
    EXPECT_EQ(i >> 2, bs_get(kTestSpec.overlap.upper6, byte));
    EXPECT_EQ(i & 0x3f, bs_get(kTestSpec.overlap.lower6, byte));
    EXPECT_EQ(i, bs_get(kTestSpec.overlap.all8, byte));
  }
}
