// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/enum_flags.h"

#include <gtest/gtest.h>

namespace brillo {

class EnumFlagsTest : public testing::Test {};

enum SomeFlagsEnum /* : int */ {
  FLAG_NONE = 0,
  FLAG_ONE = 1,
  FLAG_TWO = 2,
  FLAG_THREE = 4,
};

enum class SomeFlagsEnumClass /* : int */ {
  NONE = 0,
  ONE = 1,
  TWO = 2,
  THREE = 4,
};

enum SomeBigFlagsEnum : int64_t {
  BIG_FLAG_NONE = 0,
  BIG_FLAG_ONE = 1,
  BIG_FLAG_TWO = 2,
  BIG_FLAG_THREE = 4,
  BIG_FLAG_FOUR = 8,
};

DECLARE_FLAGS_ENUM(SomeFlagsEnum);
DECLARE_FLAGS_ENUM(SomeFlagsEnumClass);
DECLARE_FLAGS_ENUM(SomeBigFlagsEnum);

// These first tests show how these operators are meant to be used.

TEST_F(EnumFlagsTest, SampleUsage) {
  SomeFlagsEnum value = FLAG_NONE;

  // Set a flag.
  value |= FLAG_ONE;
  EXPECT_EQ(FLAG_ONE, value);

  // Set another
  value |= FLAG_THREE;
  EXPECT_EQ(FLAG_ONE | FLAG_THREE, value);

  // Clear a flag
  value &= ~FLAG_ONE;
  EXPECT_EQ(FLAG_THREE, value);

  // Toggle a flag
  value ^= FLAG_TWO;
  EXPECT_EQ(FLAG_THREE | FLAG_TWO, value);
}

TEST_F(EnumFlagsTest, SampleUsageOfMasks) {
  SomeFlagsEnum flags = FLAG_ONE | FLAG_THREE;

  EXPECT_TRUE(flags & FLAG_ONE);
  EXPECT_TRUE(flags & FLAG_THREE);
  EXPECT_FALSE(flags & FLAG_TWO);
  EXPECT_TRUE(flags & ~FLAG_TWO);
}

TEST_F(EnumFlagsTest, SampleUsageWithEnumClass) {
  SomeFlagsEnumClass value = SomeFlagsEnumClass::NONE;

  // Set a flag.
  value |= SomeFlagsEnumClass::ONE;
  EXPECT_EQ(SomeFlagsEnumClass::ONE, value);

  // Set another
  value |= SomeFlagsEnumClass::THREE;
  EXPECT_EQ(SomeFlagsEnumClass::ONE | SomeFlagsEnumClass::THREE, value);

  // Clear a flag
  value &= ~SomeFlagsEnumClass::ONE;
  EXPECT_EQ(SomeFlagsEnumClass::THREE, value);

  // Toggle a flag
  value ^= SomeFlagsEnumClass::TWO;
  EXPECT_EQ(SomeFlagsEnumClass::THREE | SomeFlagsEnumClass::TWO, value);
}

TEST_F(EnumFlagsTest, SampleUsageWithBigEnumType) {
  SomeBigFlagsEnum value = BIG_FLAG_NONE;

  // Set a flag.
  value |= BIG_FLAG_ONE;
  EXPECT_EQ(BIG_FLAG_ONE, value);

  // Set another
  value |= BIG_FLAG_THREE;
  EXPECT_EQ(FLAG_ONE | BIG_FLAG_THREE, value);

  // Clear a flag
  value &= ~BIG_FLAG_ONE;
  EXPECT_EQ(BIG_FLAG_THREE, value);

  // Toggle a flag
  value ^= BIG_FLAG_TWO;
  EXPECT_EQ(BIG_FLAG_THREE | BIG_FLAG_TWO, value);
}

// These following tests verify the binary behavior of the operators.  They do
// not demonstrate standard usage.

TEST_F(EnumFlagsTest, BinaryBehaviorOfAssignmentOperators) {
  SomeFlagsEnum value = FLAG_NONE;

  // Set a flag.
  value |= FLAG_ONE;
  EXPECT_EQ(1, value);

  // Set another
  value |= FLAG_THREE;
  EXPECT_EQ(5, value);

  // Clear a flag
  value &= ~FLAG_ONE;
  EXPECT_EQ(4, value);

  // Toggle a flag
  value ^= FLAG_TWO;
  EXPECT_EQ(6, value);
}

TEST_F(EnumFlagsTest, BinaryBehaviorOfAssignmentOperatorsWithEnumClass) {
  SomeFlagsEnumClass value = SomeFlagsEnumClass::NONE;

  // Set a flag.
  value |= SomeFlagsEnumClass::ONE;
  EXPECT_EQ(1, static_cast<int>(value));  //

  // Set another
  value |= SomeFlagsEnumClass::THREE;
  EXPECT_EQ(5, static_cast<int>(value));

  // Clear a flag
  value &= ~SomeFlagsEnumClass::ONE;
  EXPECT_EQ(4, static_cast<int>(value));

  // Toggle a flag
  value ^= SomeFlagsEnumClass::TWO;
  EXPECT_EQ(6, static_cast<int>(value));
}

TEST_F(EnumFlagsTest, BinaryBehaviorOfSimpleOperations) {
  // These values are set directly with a cast for clarity.
  const int all_bits_int = -1;
  const SomeFlagsEnum all_bits = static_cast<SomeFlagsEnum>(all_bits_int);
  const SomeFlagsEnum just_2_bits = static_cast<SomeFlagsEnum>(3);

  // Inverting a flag should result in all bits set in the base type but that
  // one.
  EXPECT_EQ(-2, ~FLAG_ONE);
  EXPECT_EQ(-3, ~FLAG_TWO);

  // OR'ing two flags should result in both being set.
  EXPECT_EQ(3, FLAG_ONE | FLAG_TWO);

  // AND'ing two flags should result in 0.
  EXPECT_EQ(FLAG_NONE, FLAG_ONE & FLAG_TWO);

  // AND'ing a mask with a flag should result in that flag.
  EXPECT_EQ(FLAG_ONE, all_bits & FLAG_ONE);

  // XOR'ing two flags should result in both being set.
  EXPECT_EQ(3, FLAG_ONE ^ FLAG_TWO);

  // XOR'ing a mask with a flag should toggle that flag in the mask.
  EXPECT_EQ(FLAG_ONE, FLAG_NONE ^ FLAG_ONE);
  EXPECT_EQ(FLAG_TWO, just_2_bits ^ FLAG_ONE);
}

TEST_F(EnumFlagsTest, BinaryBehaviorOfSimpleOperationsOnEnumClass) {
  // These values are set directly with a cast for clarity.
  const int all_bits_int = -1;
  const SomeFlagsEnumClass all_bits =
      static_cast<SomeFlagsEnumClass>(all_bits_int);
  const SomeFlagsEnumClass just_2_bits = static_cast<SomeFlagsEnumClass>(3);

  // Inverting a flag should result in all bits set in the base type but that
  // one.
  EXPECT_EQ(-2, static_cast<int>(~SomeFlagsEnumClass::ONE));
  EXPECT_EQ(-3, static_cast<int>(~SomeFlagsEnumClass::TWO));

  // OR'ing two flags should result in both being set.
  EXPECT_EQ(
      3, static_cast<int>(SomeFlagsEnumClass::ONE | SomeFlagsEnumClass::TWO));

  // AND'ing two flags should result in 0.
  EXPECT_EQ(SomeFlagsEnumClass::NONE,
            SomeFlagsEnumClass::ONE & SomeFlagsEnumClass::TWO);

  // AND'ing a mask with a flag should result in that flag.
  EXPECT_EQ(SomeFlagsEnumClass::ONE, all_bits & SomeFlagsEnumClass::ONE);

  // XOR'ing two flags should result in both being set.
  EXPECT_EQ(
      3, static_cast<int>(SomeFlagsEnumClass::ONE ^ SomeFlagsEnumClass::TWO));

  // XOR'ing a mask with a flag should toggle that flag in the mask.
  EXPECT_EQ(SomeFlagsEnumClass::ONE,
            SomeFlagsEnumClass::NONE ^ SomeFlagsEnumClass::ONE);
  EXPECT_EQ(SomeFlagsEnumClass::TWO, just_2_bits ^ SomeFlagsEnumClass::ONE);
}

TEST_F(EnumFlagsTest, BinaryBehaviorOfSimpleOperationsWithBaseType) {
  // These values are set directly with a cast for clarity.
  const int64_t all_bits_int = -1;
  const SomeBigFlagsEnum all_bits = static_cast<SomeBigFlagsEnum>(all_bits_int);
  const SomeBigFlagsEnum just_2_bits = static_cast<SomeBigFlagsEnum>(3);

  // Inverting a flag should result in all bits set in the base type but that
  // one.
  EXPECT_EQ(all_bits ^ BIG_FLAG_ONE, ~BIG_FLAG_ONE);

  // OR'ing two flags should result in both being set.
  EXPECT_EQ(3, BIG_FLAG_ONE | BIG_FLAG_TWO);

  // AND'ing two flags should result in 0.
  EXPECT_EQ(BIG_FLAG_NONE, BIG_FLAG_ONE & BIG_FLAG_TWO);

  // AND'ing a mask with a flag should result in that flag.
  EXPECT_EQ(BIG_FLAG_ONE, all_bits & BIG_FLAG_ONE);

  // XOR'ing two flags should result in both being set.
  EXPECT_EQ(3, BIG_FLAG_ONE ^ BIG_FLAG_TWO);

  // XOR'ing a mask with a flag should toggle that flag in the mask.
  EXPECT_EQ(BIG_FLAG_ONE, BIG_FLAG_NONE ^ BIG_FLAG_ONE);
  EXPECT_EQ(BIG_FLAG_TWO, just_2_bits ^ BIG_FLAG_ONE);
}

}  // namespace brillo
