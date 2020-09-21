// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_card_config.h"
#include "cras_types.h"
}

namespace {

static unsigned int cras_volume_curve_create_default_called;
static struct cras_volume_curve* cras_volume_curve_create_default_return;
static unsigned int cras_volume_curve_create_simple_step_called;
static long cras_volume_curve_create_simple_step_max_volume;
static long cras_volume_curve_create_simple_step_volume_step;
static struct cras_volume_curve* cras_volume_curve_create_simple_step_return;
static unsigned int cras_volume_curve_create_explicit_called;
static long cras_explicit_curve[101];
static struct cras_volume_curve* cras_volume_curve_create_explicit_return;

static const char CONFIG_PATH[] = CRAS_UT_TMPDIR;

void CreateConfigFile(const char* name, const char* config_text) {
  FILE* f;
  char card_path[128];

  snprintf(card_path, sizeof(card_path), "%s/%s", CONFIG_PATH, name);
  f = fopen(card_path, "w");
  if (f == NULL)
    return;

  fprintf(f, "%s", config_text);

  fclose(f);
}

class CardConfigTestSuite : public testing::Test{
  protected:
    virtual void SetUp() {
      cras_volume_curve_create_default_called = 0;
      cras_volume_curve_create_default_return =
          reinterpret_cast<struct cras_volume_curve*>(0x55);
      cras_volume_curve_create_simple_step_return =
          reinterpret_cast<struct cras_volume_curve*>(0x56);
      cras_volume_curve_create_explicit_return =
          reinterpret_cast<struct cras_volume_curve*>(0x57);
      cras_volume_curve_create_simple_step_called = 0;
      cras_volume_curve_create_simple_step_return = NULL;
      cras_volume_curve_create_explicit_called = 0;
      cras_volume_curve_create_explicit_return = NULL;
    }
};

// Test that no config is returned if the file doesn't exist.
TEST_F(CardConfigTestSuite, NoConfigFound) {
  struct cras_card_config* config;

  config = cras_card_config_create(CONFIG_PATH, "no_effing_way_this_exists");
  EXPECT_EQ(NULL, config);
}

// Test an empty config file, should return a null volume curve.
TEST_F(CardConfigTestSuite, EmptyConfigFileReturnsNullVolumeCurve) {
  static const char empty_config_text[] = "";
  static const char empty_config_name[] = "EmptyConfigCard";
  struct cras_card_config* config;
  struct cras_volume_curve* curve;

  CreateConfigFile(empty_config_name, empty_config_text);

  config = cras_card_config_create(CONFIG_PATH, empty_config_name);
  EXPECT_NE(static_cast<struct cras_card_config*>(NULL), config);

  curve = cras_card_config_get_volume_curve_for_control(config, "asdf");
  EXPECT_EQ(0, cras_volume_curve_create_default_called);
  EXPECT_EQ(static_cast<struct cras_volume_curve*>(NULL), curve);

  cras_card_config_destroy(config);
}

// Getting a curve from a null config should always return null volume curve.
TEST_F(CardConfigTestSuite, NullConfigGivesDefaultVolumeCurve) {
  struct cras_volume_curve* curve;

  curve = cras_card_config_get_volume_curve_for_control(NULL, "asdf");
  EXPECT_EQ(0, cras_volume_curve_create_default_called);
  EXPECT_EQ(static_cast<struct cras_volume_curve*>(NULL), curve);
}

// Test getting a curve from a simple_step configuration.
TEST_F(CardConfigTestSuite, SimpleStepConfig) {
  static const char simple_config_name[] = "simple";
  static const char simple_config_text[] =
    "[Card1]\n"
    "volume_curve = simple_step\n"
    "volume_step = 75\n"
    "max_volume = -600\n";
  struct cras_card_config* config;
  struct cras_volume_curve* curve;

  CreateConfigFile(simple_config_name, simple_config_text);

  config = cras_card_config_create(CONFIG_PATH, simple_config_name);
  EXPECT_NE(static_cast<struct cras_card_config*>(NULL), config);

  // Unknown config should return default curve.
  curve = cras_card_config_get_volume_curve_for_control(NULL, "asdf");
  EXPECT_EQ(0, cras_volume_curve_create_default_called);
  EXPECT_EQ(static_cast<struct cras_volume_curve*>(NULL), curve);

  // Test a config that specifies simple_step.
  curve = cras_card_config_get_volume_curve_for_control(config, "Card1");
  EXPECT_EQ(cras_volume_curve_create_simple_step_return, curve);
  EXPECT_EQ(0, cras_volume_curve_create_default_called);
  EXPECT_EQ(1, cras_volume_curve_create_simple_step_called);
  EXPECT_EQ(-600, cras_volume_curve_create_simple_step_max_volume);
  EXPECT_EQ(75, cras_volume_curve_create_simple_step_volume_step);

  cras_card_config_destroy(config);
}

// Test getting a curve from an explicit configuration.
TEST_F(CardConfigTestSuite, ExplicitCurveConfig) {
  static const char explicit_config_name[] = "explicit";
  static const char explicit_config_text[] =
    "[Card1]\n"
    "volume_curve = explicit\n"
    "dB_at_0 = -9950\n"
    "dB_at_1 = -9850\n"
    "dB_at_2 = -9750\n"
    "dB_at_3 = -9650\n"
    "dB_at_4 = -9550\n"
    "dB_at_5 = -9450\n"
    "dB_at_6 = -9350\n"
    "dB_at_7 = -9250\n"
    "dB_at_8 = -9150\n"
    "dB_at_9 = -9050\n"
    "dB_at_10 = -8950\n"
    "dB_at_11 = -8850\n"
    "dB_at_12 = -8750\n"
    "dB_at_13 = -8650\n"
    "dB_at_14 = -8550\n"
    "dB_at_15 = -8450\n"
    "dB_at_16 = -8350\n"
    "dB_at_17 = -8250\n"
    "dB_at_18 = -8150\n"
    "dB_at_19 = -8050\n"
    "dB_at_20 = -7950\n"
    "dB_at_21 = -7850\n"
    "dB_at_22 = -7750\n"
    "dB_at_23 = -7650\n"
    "dB_at_24 = -7550\n"
    "dB_at_25 = -7450\n"
    "dB_at_26 = -7350\n"
    "dB_at_27 = -7250\n"
    "dB_at_28 = -7150\n"
    "dB_at_29 = -7050\n"
    "dB_at_30 = -6950\n"
    "dB_at_31 = -6850\n"
    "dB_at_32 = -6750\n"
    "dB_at_33 = -6650\n"
    "dB_at_34 = -6550\n"
    "dB_at_35 = -6450\n"
    "dB_at_36 = -6350\n"
    "dB_at_37 = -6250\n"
    "dB_at_38 = -6150\n"
    "dB_at_39 = -6050\n"
    "dB_at_40 = -5950\n"
    "dB_at_41 = -5850\n"
    "dB_at_42 = -5750\n"
    "dB_at_43 = -5650\n"
    "dB_at_44 = -5550\n"
    "dB_at_45 = -5450\n"
    "dB_at_46 = -5350\n"
    "dB_at_47 = -5250\n"
    "dB_at_48 = -5150\n"
    "dB_at_49 = -5050\n"
    "dB_at_50 = -4950\n"
    "dB_at_51 = -4850\n"
    "dB_at_52 = -4750\n"
    "dB_at_53 = -4650\n"
    "dB_at_54 = -4550\n"
    "dB_at_55 = -4450\n"
    "dB_at_56 = -4350\n"
    "dB_at_57 = -4250\n"
    "dB_at_58 = -4150\n"
    "dB_at_59 = -4050\n"
    "dB_at_60 = -3950\n"
    "dB_at_61 = -3850\n"
    "dB_at_62 = -3750\n"
    "dB_at_63 = -3650\n"
    "dB_at_64 = -3550\n"
    "dB_at_65 = -3450\n"
    "dB_at_66 = -3350\n"
    "dB_at_67 = -3250\n"
    "dB_at_68 = -3150\n"
    "dB_at_69 = -3050\n"
    "dB_at_70 = -2950\n"
    "dB_at_71 = -2850\n"
    "dB_at_72 = -2750\n"
    "dB_at_73 = -2650\n"
    "dB_at_74 = -2550\n"
    "dB_at_75 = -2450\n"
    "dB_at_76 = -2350\n"
    "dB_at_77 = -2250\n"
    "dB_at_78 = -2150\n"
    "dB_at_79 = -2050\n"
    "dB_at_80 = -1950\n"
    "dB_at_81 = -1850\n"
    "dB_at_82 = -1750\n"
    "dB_at_83 = -1650\n"
    "dB_at_84 = -1550\n"
    "dB_at_85 = -1450\n"
    "dB_at_86 = -1350\n"
    "dB_at_87 = -1250\n"
    "dB_at_88 = -1150\n"
    "dB_at_89 = -1050\n"
    "dB_at_90 = -950\n"
    "dB_at_91 = -850\n"
    "dB_at_92 = -750\n"
    "dB_at_93 = -650\n"
    "dB_at_94 = -550\n"
    "dB_at_95 = -450\n"
    "dB_at_96 = -350\n"
    "dB_at_97 = -250\n"
    "dB_at_98 = -150\n"
    "dB_at_99 = -50\n"
    "dB_at_100 = 50\n";
  struct cras_card_config* config;
  struct cras_volume_curve* curve;

  CreateConfigFile(explicit_config_name, explicit_config_text);

  config = cras_card_config_create(CONFIG_PATH, explicit_config_name);
  EXPECT_NE(static_cast<struct cras_card_config*>(NULL), config);

  // Test a explicit curve config.
  curve = cras_card_config_get_volume_curve_for_control(config, "Card1");
  EXPECT_EQ(0, cras_volume_curve_create_default_called);
  EXPECT_EQ(0, cras_volume_curve_create_simple_step_called);
  EXPECT_EQ(1, cras_volume_curve_create_explicit_called);
  EXPECT_EQ(cras_volume_curve_create_explicit_return, curve);
  for (unsigned int i = 0; i < 101; i++) {
    EXPECT_EQ((static_cast<long>(i) - 100) * 100 + 50, cras_explicit_curve[i]);
  }

  cras_card_config_destroy(config);
}

// Stubs.
extern "C" {

struct cras_volume_curve* cras_volume_curve_create_default() {
  cras_volume_curve_create_default_called++;
  return cras_volume_curve_create_default_return;
}

struct cras_volume_curve *cras_volume_curve_create_simple_step(
    long max_volume,
    long volume_step) {
  cras_volume_curve_create_simple_step_called++;
  cras_volume_curve_create_simple_step_max_volume = max_volume;
  cras_volume_curve_create_simple_step_volume_step = volume_step;
  return cras_volume_curve_create_simple_step_return;
}

struct cras_volume_curve *cras_volume_curve_create_explicit(
    long dB_vals[101]) {
  cras_volume_curve_create_explicit_called++;
  memcpy(cras_explicit_curve, dB_vals, sizeof(cras_explicit_curve));
  return cras_volume_curve_create_explicit_return;
}

}

}  //  namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
