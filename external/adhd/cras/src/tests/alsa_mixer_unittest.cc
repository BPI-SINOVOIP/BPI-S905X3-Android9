// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <syslog.h>
#include <vector>

extern "C" {
#include "cras_alsa_mixer.h"
#include "cras_alsa_mixer_name.h"
#include "cras_types.h"
#include "cras_util.h"
#include "cras_volume_curve.h"
#include "utlist.h"

//  Include C file to test static functions and use the definition of some
//  structure.
#include "cras_alsa_mixer.c"
}

namespace {

static size_t snd_mixer_open_called;
static int snd_mixer_open_return_value;
static size_t snd_mixer_close_called;
static size_t snd_mixer_attach_called;
static int snd_mixer_attach_return_value;
const char *snd_mixer_attach_mixdev;
static size_t snd_mixer_selem_register_called;
static int snd_mixer_selem_register_return_value;
static size_t snd_mixer_load_called;
static int snd_mixer_load_return_value;
static size_t snd_mixer_first_elem_called;
static snd_mixer_elem_t *snd_mixer_first_elem_return_value;
static int snd_mixer_elem_next_called;
static snd_mixer_elem_t **snd_mixer_elem_next_return_values;
static int snd_mixer_elem_next_return_values_index;
static int snd_mixer_elem_next_return_values_length;
static int snd_mixer_selem_set_playback_dB_all_called;
static long *snd_mixer_selem_set_playback_dB_all_values;
static int snd_mixer_selem_set_playback_dB_all_values_length;
static int snd_mixer_selem_set_playback_switch_all_called;
static int snd_mixer_selem_set_playback_switch_all_value;
static int snd_mixer_selem_has_playback_volume_called;
static int *snd_mixer_selem_has_playback_volume_return_values;
static int snd_mixer_selem_has_playback_volume_return_values_length;
static int snd_mixer_selem_has_playback_switch_called;
static int *snd_mixer_selem_has_playback_switch_return_values;
static int snd_mixer_selem_has_playback_switch_return_values_length;
static int snd_mixer_selem_set_capture_dB_all_called;
static long *snd_mixer_selem_set_capture_dB_all_values;
static int snd_mixer_selem_set_capture_dB_all_values_length;
static int snd_mixer_selem_set_capture_switch_all_called;
static int snd_mixer_selem_set_capture_switch_all_value;
static int snd_mixer_selem_has_capture_volume_called;
static int *snd_mixer_selem_has_capture_volume_return_values;
static int snd_mixer_selem_has_capture_volume_return_values_length;
static int snd_mixer_selem_has_capture_switch_called;
static int *snd_mixer_selem_has_capture_switch_return_values;
static int snd_mixer_selem_has_capture_switch_return_values_length;
static int snd_mixer_selem_get_name_called;
static const char **snd_mixer_selem_get_name_return_values;
static int snd_mixer_selem_get_name_return_values_length;
static int snd_mixer_selem_get_playback_dB_called;
static long *snd_mixer_selem_get_playback_dB_return_values;
static int snd_mixer_selem_get_playback_dB_return_values_length;
static int snd_mixer_selem_get_capture_dB_called;
static long *snd_mixer_selem_get_capture_dB_return_values;
static int snd_mixer_selem_get_capture_dB_return_values_length;
static size_t cras_volume_curve_destroy_called;
static size_t snd_mixer_selem_get_playback_dB_range_called;
static size_t snd_mixer_selem_get_playback_dB_range_values_length;
static const long *snd_mixer_selem_get_playback_dB_range_min_values;
static const long *snd_mixer_selem_get_playback_dB_range_max_values;
static size_t snd_mixer_selem_get_capture_dB_range_called;
static size_t snd_mixer_selem_get_capture_dB_range_values_length;
static const long *snd_mixer_selem_get_capture_dB_range_min_values;
static const long *snd_mixer_selem_get_capture_dB_range_max_values;
static size_t iniparser_getstring_return_index;
static size_t iniparser_getstring_return_length;
static char **iniparser_getstring_returns;
static size_t snd_mixer_find_selem_called;
static std::map<std::string, snd_mixer_elem_t*> snd_mixer_find_elem_map;
static std::string snd_mixer_find_elem_id_name;

static void ResetStubData() {
  iniparser_getstring_return_index = 0;
  iniparser_getstring_return_length = 0;
  snd_mixer_open_called = 0;
  snd_mixer_open_return_value = 0;
  snd_mixer_close_called = 0;
  snd_mixer_attach_called = 0;
  snd_mixer_attach_return_value = 0;
  snd_mixer_attach_mixdev = static_cast<const char *>(NULL);
  snd_mixer_selem_register_called = 0;
  snd_mixer_selem_register_return_value = 0;
  snd_mixer_load_called = 0;
  snd_mixer_load_return_value = 0;
  snd_mixer_first_elem_called = 0;
  snd_mixer_first_elem_return_value = static_cast<snd_mixer_elem_t *>(NULL);
  snd_mixer_elem_next_called = 0;
  snd_mixer_elem_next_return_values = static_cast<snd_mixer_elem_t **>(NULL);
  snd_mixer_elem_next_return_values_index = 0;
  snd_mixer_elem_next_return_values_length = 0;
  snd_mixer_selem_set_playback_dB_all_called = 0;
  snd_mixer_selem_set_playback_dB_all_values = static_cast<long *>(NULL);
  snd_mixer_selem_set_playback_dB_all_values_length = 0;
  snd_mixer_selem_set_playback_switch_all_called = 0;
  snd_mixer_selem_has_playback_volume_called = 0;
  snd_mixer_selem_has_playback_volume_return_values = static_cast<int *>(NULL);
  snd_mixer_selem_has_playback_volume_return_values_length = 0;
  snd_mixer_selem_has_playback_switch_called = 0;
  snd_mixer_selem_has_playback_switch_return_values = static_cast<int *>(NULL);
  snd_mixer_selem_has_playback_switch_return_values_length = 0;
  snd_mixer_selem_set_capture_dB_all_called = 0;
  snd_mixer_selem_set_capture_dB_all_values = static_cast<long *>(NULL);
  snd_mixer_selem_set_capture_dB_all_values_length = 0;
  snd_mixer_selem_set_capture_switch_all_called = 0;
  snd_mixer_selem_has_capture_volume_called = 0;
  snd_mixer_selem_has_capture_volume_return_values = static_cast<int *>(NULL);
  snd_mixer_selem_has_capture_volume_return_values_length = 0;
  snd_mixer_selem_has_capture_switch_called = 0;
  snd_mixer_selem_has_capture_switch_return_values = static_cast<int *>(NULL);
  snd_mixer_selem_has_capture_switch_return_values_length = 0;
  snd_mixer_selem_get_name_called = 0;
  snd_mixer_selem_get_name_return_values = static_cast<const char **>(NULL);
  snd_mixer_selem_get_name_return_values_length = 0;
  snd_mixer_selem_get_playback_dB_called = 0;
  snd_mixer_selem_get_playback_dB_return_values = static_cast<long *>(NULL);
  snd_mixer_selem_get_playback_dB_return_values_length = 0;
  snd_mixer_selem_get_capture_dB_called = 0;
  snd_mixer_selem_get_capture_dB_return_values = static_cast<long *>(NULL);
  snd_mixer_selem_get_capture_dB_return_values_length = 0;
  cras_volume_curve_destroy_called = 0;
  snd_mixer_selem_get_playback_dB_range_called = 0;
  snd_mixer_selem_get_playback_dB_range_values_length = 0;
  snd_mixer_selem_get_playback_dB_range_min_values = static_cast<long *>(NULL);
  snd_mixer_selem_get_playback_dB_range_max_values = static_cast<long *>(NULL);
  snd_mixer_selem_get_capture_dB_range_called = 0;
  snd_mixer_selem_get_capture_dB_range_values_length = 0;
  snd_mixer_selem_get_capture_dB_range_min_values = static_cast<long *>(NULL);
  snd_mixer_selem_get_capture_dB_range_max_values = static_cast<long *>(NULL);
  snd_mixer_find_selem_called = 0;
  snd_mixer_find_elem_map.clear();
  snd_mixer_find_elem_id_name.clear();
}

struct cras_alsa_mixer *create_mixer_and_add_controls_by_name_matching(
    const char *card_name,
    struct mixer_name *extra_controls,
    struct mixer_name *coupled_controls) {
  struct cras_alsa_mixer *cmix = cras_alsa_mixer_create(card_name);
  cras_alsa_mixer_add_controls_by_name_matching(
      cmix, extra_controls, coupled_controls);
  return cmix;
}

TEST(AlsaMixer, CreateFailOpen) {
  struct cras_alsa_mixer *c;

  ResetStubData();
  snd_mixer_open_return_value = -1;
  c = cras_alsa_mixer_create("hw:0");
  EXPECT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
}

TEST(AlsaMixer, CreateFailAttach) {
  struct cras_alsa_mixer *c;

  ResetStubData();
  snd_mixer_attach_return_value = -1;
  c = cras_alsa_mixer_create("hw:0");
  EXPECT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_close_called);
}

TEST(AlsaMixer, CreateFailSelemRegister) {
  struct cras_alsa_mixer *c;

  ResetStubData();
  snd_mixer_selem_register_return_value = -1;
  c = cras_alsa_mixer_create("hw:0");
  EXPECT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_close_called);
}

TEST(AlsaMixer, CreateFailLoad) {
  struct cras_alsa_mixer *c;

  ResetStubData();
  snd_mixer_load_return_value = -1;
  c = cras_alsa_mixer_create("hw:0");
  EXPECT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(1, snd_mixer_close_called);
}

TEST(AlsaMixer, CreateNoElements) {
  struct cras_alsa_mixer *c;

  ResetStubData();
  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, NULL);
  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);

  /* set mute shouldn't call anything. */
  cras_alsa_mixer_set_mute(c, 0, NULL);
  EXPECT_EQ(0, snd_mixer_selem_set_playback_switch_all_called);
  /* set volume shouldn't call anything. */
  cras_alsa_mixer_set_dBFS(c, 0, NULL);
  EXPECT_EQ(0, snd_mixer_selem_set_playback_dB_all_called);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
}

TEST(AlsaMixer, CreateOneUnknownElementWithoutVolume) {
  struct cras_alsa_mixer *c;
  int element_playback_volume[] = {
    0,
  };
  int element_playback_switches[] = {
    1,
  };
  const char *element_names[] = {
    "Unknown",
  };
  struct mixer_control *mixer_output;
  int rc;

  ResetStubData();
  snd_mixer_first_elem_return_value = reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, NULL);
  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_get_name_called);
  EXPECT_EQ(0, snd_mixer_selem_get_playback_dB_range_called);

  /* set mute shouldn't call anything. */
  cras_alsa_mixer_set_mute(c, 0, NULL);
  EXPECT_EQ(0, snd_mixer_selem_set_playback_switch_all_called);

  ResetStubData();
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  rc = mixer_control_create(&mixer_output, NULL,
                            reinterpret_cast<snd_mixer_elem_t *>(1),
                            CRAS_STREAM_OUTPUT);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_mixer_selem_get_name_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_switch_called);
  EXPECT_EQ(1, snd_mixer_selem_get_playback_dB_range_called);

  /* if passed a mixer output then it should mute that. */
  cras_alsa_mixer_set_mute(c, 0, mixer_output);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_switch_all_called);
  /* set volume shouldn't call anything. */
  cras_alsa_mixer_set_dBFS(c, 0, NULL);
  EXPECT_EQ(0, snd_mixer_selem_set_playback_dB_all_called);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_control_destroy(mixer_output);
}

TEST(AlsaMixer, CreateOneUnknownElementWithVolume) {
  struct cras_alsa_mixer *c;
  static const long min_volumes[] = {-500};
  static const long max_volumes[] = {40};
  int element_playback_volume[] = {
    1,
    0,
  };
  int element_playback_switches[] = {
    0,
    1,
  };
  const char *element_names[] = {
    "Unknown",
    "Playback",
  };
  struct mixer_control *mixer_output;
  int rc;

  ResetStubData();
  snd_mixer_first_elem_return_value = reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);
  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, NULL);
  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);
  EXPECT_EQ(3, snd_mixer_selem_has_playback_volume_called);
  EXPECT_EQ(2, snd_mixer_selem_get_playback_dB_range_called);
  EXPECT_EQ(3, snd_mixer_selem_get_name_called);

  /* Should use "Playback" since it has playback switch. */
  cras_alsa_mixer_set_mute(c, 0, NULL);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_switch_all_called);

  ResetStubData();
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  rc = mixer_control_create(&mixer_output, NULL,
                            reinterpret_cast<snd_mixer_elem_t *>(2),
                            CRAS_STREAM_OUTPUT);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_mixer_selem_get_name_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_switch_called);
  EXPECT_EQ(0, snd_mixer_selem_get_playback_dB_range_called);

  /*
   * If passed a mixer output then it should mute both "Playback" and that
   * mixer_output.
   */
  cras_alsa_mixer_set_mute(c, 0, mixer_output);
  EXPECT_EQ(2, snd_mixer_selem_set_playback_switch_all_called);
  cras_alsa_mixer_set_dBFS(c, 0, NULL);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_dB_all_called);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_control_destroy(mixer_output);
}

TEST(AlsaMixer, CreateOneMasterElement) {
  struct cras_alsa_mixer *c;
  int element_playback_volume[] = {
    1,
    1,
  };
  int element_playback_switches[] = {
    1,
    1,
  };
  const char *element_names[] = {
    "Master",
    "Playback"
  };
  struct mixer_control *mixer_output;
  int rc;
  long set_dB_values[3];
  static const long min_volumes[] = {0, 0};
  static const long max_volumes[] = {950, 950};

  ResetStubData();
  snd_mixer_first_elem_return_value = reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, NULL);
  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);
  EXPECT_EQ(3, snd_mixer_selem_get_name_called);
  EXPECT_EQ(1, snd_mixer_elem_next_called);

  /* set mute should be called for Master. */
  cras_alsa_mixer_set_mute(c, 0, NULL);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_switch_all_called);
  /* set volume should be called for Master. */
  cras_alsa_mixer_set_dBFS(c, 0, NULL);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_dB_all_called);

  ResetStubData();
  snd_mixer_selem_set_playback_dB_all_values = set_dB_values;
  snd_mixer_selem_set_playback_dB_all_values_length =
      ARRAY_SIZE(set_dB_values);
  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  rc = mixer_control_create(&mixer_output, NULL,
                            reinterpret_cast<snd_mixer_elem_t *>(2),
                            CRAS_STREAM_OUTPUT);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_mixer_selem_get_name_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_switch_called);
  EXPECT_EQ(1, snd_mixer_selem_get_playback_dB_range_called);

  /* if passed a mixer output then it should set the volume for that too. */
  cras_alsa_mixer_set_dBFS(c, 0, mixer_output);
  EXPECT_EQ(2, snd_mixer_selem_set_playback_dB_all_called);
  EXPECT_EQ(950, set_dB_values[0]);
  EXPECT_EQ(950, set_dB_values[1]);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_control_destroy(mixer_output);
}

TEST(AlsaMixer, CreateTwoMainVolumeElements) {
  struct cras_alsa_mixer *c;
  snd_mixer_elem_t *elements[] = {
    reinterpret_cast<snd_mixer_elem_t *>(2),
  };
  int element_playback_volume[] = {
    1,
    1,
    1,
  };
  int element_playback_switches[] = {
    1,
    1,
    1,
  };
  const char *element_names[] = {
    "Master",
    "PCM",
    "Other",
  };
  struct mixer_control *mixer_output;
  int rc;
  static const long min_volumes[] = {-500, -1250, -500};
  static const long max_volumes[] = {40, 40, 0};
  long get_dB_returns[] = {0, 0, 0};
  long set_dB_values[3];

  ResetStubData();
  snd_mixer_first_elem_return_value = reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_elem_next_return_values = elements;
  snd_mixer_elem_next_return_values_length = ARRAY_SIZE(elements);
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  snd_mixer_selem_get_playback_dB_range_called = 0;
  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);
  snd_mixer_selem_set_playback_dB_all_values = set_dB_values;
  snd_mixer_selem_set_playback_dB_all_values_length =
      ARRAY_SIZE(set_dB_values);
  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, NULL);
  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(2, snd_mixer_selem_get_playback_dB_range_called);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);
  EXPECT_EQ(2, snd_mixer_elem_next_called);
  EXPECT_EQ(5, snd_mixer_selem_get_name_called);
  EXPECT_EQ(3, snd_mixer_selem_has_playback_switch_called);

  /* Set mute should be called for Master only. */
  cras_alsa_mixer_set_mute(c, 0, NULL);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_switch_all_called);

  /* Set volume should be called for Master and PCM. If Master doesn't set to
   * anything but zero then the entire volume should be passed to the PCM
   * control.*/

  /* Set volume should be called for Master and PCM. (without mixer_output) */
  snd_mixer_selem_get_playback_dB_return_values = get_dB_returns;
  snd_mixer_selem_get_playback_dB_return_values_length =
      ARRAY_SIZE(get_dB_returns);
  cras_alsa_mixer_set_dBFS(c, -50, NULL);
  EXPECT_EQ(2, snd_mixer_selem_set_playback_dB_all_called);
  EXPECT_EQ(2, snd_mixer_selem_get_playback_dB_called);
  /* volume should be set relative to max volume (40 + 40). */
  EXPECT_EQ(30, set_dB_values[0]);
  EXPECT_EQ(30, set_dB_values[1]);

  ResetStubData();
  snd_mixer_selem_set_playback_dB_all_values = set_dB_values;
  snd_mixer_selem_set_playback_dB_all_values_length = ARRAY_SIZE(set_dB_values);
  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);
  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  rc = mixer_control_create(&mixer_output, NULL,
                            reinterpret_cast<snd_mixer_elem_t *>(3),
                            CRAS_STREAM_OUTPUT);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_mixer_selem_get_name_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_has_playback_switch_called);
  EXPECT_EQ(1, snd_mixer_selem_get_playback_dB_range_called);

  /* Set volume should be called for Master, PCM, and the mixer_output passed
   * in. If Master doesn't set to anything but zero then the entire volume
   * should be passed to the PCM control.*/
  cras_alsa_mixer_set_dBFS(c, -50, mixer_output);
  EXPECT_EQ(3, snd_mixer_selem_set_playback_dB_all_called);
  EXPECT_EQ(2, snd_mixer_selem_get_playback_dB_called);
  EXPECT_EQ(30, set_dB_values[0]);
  EXPECT_EQ(30, set_dB_values[1]);
  EXPECT_EQ(30, set_dB_values[2]);
  /* Set volume should be called for Master and PCM. Since the controls were
   * sorted, Master should get the volume remaining after PCM is set, in this
   * case -50 - -24 = -26. */
  long get_dB_returns2[] = {
    -25,
    -24,
  };
  snd_mixer_selem_get_playback_dB_return_values = get_dB_returns2;
  snd_mixer_selem_get_playback_dB_return_values_length =
      ARRAY_SIZE(get_dB_returns2);
  snd_mixer_selem_set_playback_dB_all_called = 0;
  snd_mixer_selem_get_playback_dB_called = 0;
  mixer_output->has_volume = 0;
  mixer_output->min_volume_dB = MIXER_CONTROL_VOLUME_DB_INVALID;
  mixer_output->max_volume_dB = MIXER_CONTROL_VOLUME_DB_INVALID;
  cras_alsa_mixer_set_dBFS(c, -50, mixer_output);
  EXPECT_EQ(2, snd_mixer_selem_set_playback_dB_all_called);
  EXPECT_EQ(2, snd_mixer_selem_get_playback_dB_called);
  EXPECT_EQ(54, set_dB_values[0]); // Master
  EXPECT_EQ(30, set_dB_values[1]); // PCM

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_control_destroy(mixer_output);
}

TEST(AlsaMixer, CreateTwoMainCaptureElements) {
  struct cras_alsa_mixer *c;
  snd_mixer_elem_t *elements[] = {
    reinterpret_cast<snd_mixer_elem_t *>(2),
  };
  int element_capture_volume[] = {
    1,
    1,
    1,
  };
  int element_capture_switches[] = {
    1,
    1,
    1,
  };
  const char *element_names[] = {
    "Capture",
    "Digital Capture",
    "Mic",
  };
  struct mixer_control *mixer_input;
  int rc;

  ResetStubData();
  snd_mixer_first_elem_return_value = reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_elem_next_return_values = elements;
  snd_mixer_elem_next_return_values_length = ARRAY_SIZE(elements);
  snd_mixer_selem_has_capture_volume_return_values = element_capture_volume;
  snd_mixer_selem_has_capture_volume_return_values_length =
      ARRAY_SIZE(element_capture_volume);
  snd_mixer_selem_has_capture_switch_return_values = element_capture_switches;
  snd_mixer_selem_has_capture_switch_return_values_length =
      ARRAY_SIZE(element_capture_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, NULL);
  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);
  EXPECT_EQ(2, snd_mixer_elem_next_called);
  EXPECT_EQ(5, snd_mixer_selem_get_name_called);
  EXPECT_EQ(3, snd_mixer_selem_has_capture_switch_called);

  /* Set mute should be called for Master only. */
  cras_alsa_mixer_set_capture_mute(c, 0, NULL);
  EXPECT_EQ(1, snd_mixer_selem_set_capture_switch_all_called);
  /* Set volume should be called for Capture and Digital Capture. If Capture
   * doesn't set to anything but zero then the entire volume should be passed to
   * the Digital Capture control. */
  long get_dB_returns[] = {
    0,
    0,
  };
  long set_dB_values[2];
  snd_mixer_selem_get_capture_dB_return_values = get_dB_returns;
  snd_mixer_selem_get_capture_dB_return_values_length =
      ARRAY_SIZE(get_dB_returns);
  snd_mixer_selem_set_capture_dB_all_values = set_dB_values;
  snd_mixer_selem_set_capture_dB_all_values_length =
      ARRAY_SIZE(set_dB_values);
  cras_alsa_mixer_set_capture_dBFS(c, -10, NULL);
  EXPECT_EQ(2, snd_mixer_selem_set_capture_dB_all_called);
  EXPECT_EQ(2, snd_mixer_selem_get_capture_dB_called);
  EXPECT_EQ(-10, set_dB_values[0]);
  EXPECT_EQ(-10, set_dB_values[1]);
  /* Set volume should be called for Capture and Digital Capture. Capture should
   * get the gain remaining after Mic Boos is set, in this case 20 - 25 = -5. */
  long get_dB_returns2[] = {
    25,
    -5,
  };
  snd_mixer_selem_get_capture_dB_return_values = get_dB_returns2;
  snd_mixer_selem_get_capture_dB_return_values_length =
      ARRAY_SIZE(get_dB_returns2);
  snd_mixer_selem_set_capture_dB_all_values = set_dB_values;
  snd_mixer_selem_set_capture_dB_all_values_length =
      ARRAY_SIZE(set_dB_values);
  snd_mixer_selem_set_capture_dB_all_called = 0;
  snd_mixer_selem_get_capture_dB_called = 0;
  cras_alsa_mixer_set_capture_dBFS(c, 20, NULL);
  EXPECT_EQ(2, snd_mixer_selem_set_capture_dB_all_called);
  EXPECT_EQ(2, snd_mixer_selem_get_capture_dB_called);
  EXPECT_EQ(20, set_dB_values[0]);
  EXPECT_EQ(-5, set_dB_values[1]);

  /* Set volume to the two main controls plus additional specific input
   * volume control */

  long get_dB_returns3[] = {
    0,
    0,
    0,
  };
  long set_dB_values3[3];

  snd_mixer_selem_get_capture_dB_return_values = get_dB_returns3;
  snd_mixer_selem_get_capture_dB_return_values_length =
      ARRAY_SIZE(get_dB_returns3);
  snd_mixer_selem_get_capture_dB_called = 0;
  snd_mixer_selem_set_capture_dB_all_values = set_dB_values3;
  snd_mixer_selem_set_capture_dB_all_values_length =
      ARRAY_SIZE(set_dB_values3);
  snd_mixer_selem_set_capture_dB_all_called = 0;
  snd_mixer_selem_has_capture_volume_return_values = element_capture_volume;
  snd_mixer_selem_has_capture_volume_return_values_length =
      ARRAY_SIZE(element_capture_volume);
  snd_mixer_selem_has_capture_switch_return_values = element_capture_switches;
  snd_mixer_selem_has_capture_switch_return_values_length =
      ARRAY_SIZE(element_capture_switches);
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  snd_mixer_selem_get_name_called = 0;
  snd_mixer_selem_has_capture_volume_called = 0;
  snd_mixer_selem_has_capture_switch_called = 0;
  snd_mixer_selem_get_capture_dB_range_called = 0;
  rc = mixer_control_create(&mixer_input, NULL,
                            reinterpret_cast<snd_mixer_elem_t *>(3),
                            CRAS_STREAM_INPUT);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_mixer_selem_get_name_called);
  EXPECT_EQ(1, snd_mixer_selem_has_capture_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_has_capture_switch_called);
  EXPECT_EQ(1, snd_mixer_selem_get_capture_dB_range_called);
  EXPECT_EQ(1, mixer_input->has_volume);

  cras_alsa_mixer_set_capture_dBFS(c, 20, mixer_input);

  EXPECT_EQ(3, snd_mixer_selem_set_capture_dB_all_called);
  EXPECT_EQ(2, snd_mixer_selem_get_capture_dB_called);
  EXPECT_EQ(20, set_dB_values3[0]);
  EXPECT_EQ(20, set_dB_values3[1]);
  EXPECT_EQ(20, set_dB_values3[2]);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_control_destroy(mixer_input);
}

class AlsaMixerOutputs : public testing::Test {
 protected:
  virtual void SetUp() {
    output_called_values_.clear();
    output_callback_called_ = 0;
    static snd_mixer_elem_t *elements[] = {
      reinterpret_cast<snd_mixer_elem_t *>(2),  // PCM
      reinterpret_cast<snd_mixer_elem_t *>(3),  // Headphone
      reinterpret_cast<snd_mixer_elem_t *>(4),  // Speaker
      reinterpret_cast<snd_mixer_elem_t *>(5),  // HDMI
      reinterpret_cast<snd_mixer_elem_t *>(6),  // IEC958
      reinterpret_cast<snd_mixer_elem_t *>(7),  // Mic Boost
      reinterpret_cast<snd_mixer_elem_t *>(8),  // Capture
    };
    static int element_playback_volume[] = {
      1,
      1,
      1,
      0,
      0,
      1,
      1,
    };
    static int element_playback_switches[] = {
      1,
      1,
      1,
      0,
      1,
      1,
      1,
    };
    static int element_capture_volume[] = {0, 0, 0, 0, 0, 0,
      1,
      1,
    };
    static int element_capture_switches[] = {0, 0, 0, 0, 0, 0,
      1,
      1,
    };
    static const long min_volumes[] = {0, 0, 0, 0, 0, 0, 500, -1250};
    static const long max_volumes[] = {0, 0, 0, 0, 0, 0, 3000, 400};
    static const char *element_names[] = {
      "Master",
      "PCM",
      "Headphone",
      "Speaker",
      "HDMI",
      "IEC958",
      "Capture",
      "Digital Capture",
    };
    static const char *output_names_extra[] = {
      "IEC958"
    };
    static char *iniparser_returns[] = {
      NULL,
    };
    struct mixer_name *extra_controls =
        mixer_name_add_array(NULL, output_names_extra,
                             ARRAY_SIZE(output_names_extra),
                             CRAS_STREAM_OUTPUT,
                             MIXER_NAME_VOLUME);

    ResetStubData();
    snd_mixer_first_elem_return_value =
        reinterpret_cast<snd_mixer_elem_t *>(1);  // Master
    snd_mixer_elem_next_return_values = elements;
    snd_mixer_elem_next_return_values_length = ARRAY_SIZE(elements);
    snd_mixer_selem_has_playback_volume_return_values =
        element_playback_volume;
    snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
    snd_mixer_selem_has_playback_switch_return_values =
        element_playback_switches;
    snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
    snd_mixer_selem_has_capture_volume_return_values =
        element_capture_volume;
    snd_mixer_selem_has_capture_volume_return_values_length =
      ARRAY_SIZE(element_capture_volume);
    snd_mixer_selem_has_capture_switch_return_values =
        element_capture_switches;
    snd_mixer_selem_has_capture_switch_return_values_length =
      ARRAY_SIZE(element_capture_switches);
    snd_mixer_selem_get_name_return_values = element_names;
    snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
    snd_mixer_selem_get_capture_dB_range_called = 0;
    snd_mixer_selem_get_capture_dB_range_min_values = min_volumes;
    snd_mixer_selem_get_capture_dB_range_max_values = max_volumes;
    snd_mixer_selem_get_capture_dB_range_values_length =
        ARRAY_SIZE(min_volumes);
    iniparser_getstring_returns = iniparser_returns;
    iniparser_getstring_return_length = ARRAY_SIZE(iniparser_returns);
    cras_mixer_ = create_mixer_and_add_controls_by_name_matching(
        "hw:0", extra_controls, NULL);
    ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), cras_mixer_);
    EXPECT_EQ(1, snd_mixer_open_called);
    EXPECT_EQ(1, snd_mixer_attach_called);
    EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
    EXPECT_EQ(1, snd_mixer_selem_register_called);
    EXPECT_EQ(1, snd_mixer_load_called);
    EXPECT_EQ(0, snd_mixer_close_called);
    EXPECT_EQ(ARRAY_SIZE(elements) + 1, snd_mixer_elem_next_called);
    EXPECT_EQ(8, snd_mixer_selem_has_playback_volume_called);
    EXPECT_EQ(7, snd_mixer_selem_has_playback_switch_called);
    EXPECT_EQ(4, snd_mixer_selem_has_capture_volume_called);
    EXPECT_EQ(3, snd_mixer_selem_has_capture_switch_called);
    mixer_name_free(extra_controls);
  }

  virtual void TearDown() {
    cras_alsa_mixer_destroy(cras_mixer_);
    EXPECT_EQ(1, snd_mixer_close_called);
  }

  static void OutputCallback(struct mixer_control *out, void *arg) {
    output_callback_called_++;
    output_called_values_.push_back(out);
  }

  struct cras_alsa_mixer *cras_mixer_;
  static size_t output_callback_called_;
  static std::vector<struct mixer_control *> output_called_values_;
};

size_t AlsaMixerOutputs::output_callback_called_;
std::vector<struct mixer_control *>
    AlsaMixerOutputs::output_called_values_;

TEST_F(AlsaMixerOutputs, CheckFourOutputs) {
  cras_alsa_mixer_list_outputs(cras_mixer_,
                               AlsaMixerOutputs::OutputCallback,
                               reinterpret_cast<void*>(555));
  EXPECT_EQ(4, output_callback_called_);
}

TEST_F(AlsaMixerOutputs, CheckFindOutputByNameNoMatch) {
  struct mixer_control *out;

  out = cras_alsa_mixer_get_output_matching_name(cras_mixer_,
                                                 "AAAAA Jack");
  EXPECT_EQ(static_cast<struct mixer_control *>(NULL), out);
}

TEST_F(AlsaMixerOutputs, CheckFindOutputByName) {
  struct mixer_control *out;

  out = cras_alsa_mixer_get_output_matching_name(cras_mixer_,
                                                 "Headphone Jack");
  EXPECT_NE(static_cast<struct mixer_control *>(NULL), out);
}

TEST_F(AlsaMixerOutputs, CheckFindOutputHDMIByName) {
  struct mixer_control *out;

  out = cras_alsa_mixer_get_output_matching_name(cras_mixer_,
                                                 "HDMI Jack");
  EXPECT_NE(static_cast<struct mixer_control *>(NULL), out);
}

TEST_F(AlsaMixerOutputs, CheckFindInputNameWorkaround) {
  struct mixer_control *control;
  snd_mixer_elem_t *elements[] = {
    reinterpret_cast<snd_mixer_elem_t *>(1),  // Speaker
    reinterpret_cast<snd_mixer_elem_t *>(2),  // Headphone
    reinterpret_cast<snd_mixer_elem_t *>(3),  // MIC
  };
  const char *element_names[] = {
    "Speaker",
    "Headphone",
    "MIC",
  };
  size_t i;

  ResetStubData();
  for (i = 0; i < ARRAY_SIZE(elements); i++)
    snd_mixer_find_elem_map[element_names[i]] = elements[i];

  snd_mixer_selem_get_name_called = 0;
  snd_mixer_selem_get_name_return_values = element_names;
  snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
  control = cras_alsa_mixer_get_input_matching_name(cras_mixer_,
                                                    "MIC");
  EXPECT_NE(static_cast<struct mixer_control *>(NULL), control);
  /* This exercises the 'workaround' where the control is added if it was
   * previouly missing in cras_alsa_mixer_get_input_matching_name().
   * snd_mixer_find_selem is called once for the missing control. */
  EXPECT_EQ(1, snd_mixer_find_selem_called);
  EXPECT_EQ(1, snd_mixer_selem_has_capture_volume_called);
  EXPECT_EQ(1, snd_mixer_selem_has_capture_switch_called);
}

TEST_F(AlsaMixerOutputs, ActivateDeactivate) {
  int rc;

  cras_alsa_mixer_list_outputs(cras_mixer_,
                               AlsaMixerOutputs::OutputCallback,
                               reinterpret_cast<void*>(555));
  EXPECT_EQ(4, output_callback_called_);
  EXPECT_EQ(4, output_called_values_.size());

  rc = cras_alsa_mixer_set_output_active_state(output_called_values_[0], 0);
  ASSERT_EQ(0, rc);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_switch_all_called);
  cras_alsa_mixer_set_output_active_state(output_called_values_[0], 1);
  EXPECT_EQ(2, snd_mixer_selem_set_playback_switch_all_called);
}

TEST_F(AlsaMixerOutputs, MinMaxCaptureGain) {
  long min, max;
  min = cras_alsa_mixer_get_minimum_capture_gain(cras_mixer_,
		  NULL);
  EXPECT_EQ(-750, min);
  max = cras_alsa_mixer_get_maximum_capture_gain(cras_mixer_,
		  NULL);
  EXPECT_EQ(3400, max);
}

TEST_F(AlsaMixerOutputs, MinMaxCaptureGainWithActiveInput) {
  struct mixer_control *mixer_input;
  long min, max;

  mixer_input = (struct mixer_control *)calloc(1, sizeof(*mixer_input));
  mixer_input->min_volume_dB = 50;
  mixer_input->max_volume_dB = 60;
  mixer_input->has_volume = 1;
  min = cras_alsa_mixer_get_minimum_capture_gain(cras_mixer_, mixer_input);
  max = cras_alsa_mixer_get_maximum_capture_gain(cras_mixer_, mixer_input);
  EXPECT_EQ(-700, min);
  EXPECT_EQ(3460, max);

  free((void *)mixer_input);
}

TEST(AlsaMixer, CreateWithCoupledOutputControls) {
  struct cras_alsa_mixer *c;
  struct mixer_control *output_control;
  struct mixer_control_element *c1, *c2, *c3, *c4;

  static const long min_volumes[] = {-70, -70};
  static const long max_volumes[] = {30, 30};

  long set_dB_values[2];

  const char *coupled_output_names[] = {"Left Master",
                                        "Right Master",
                                        "Left Speaker",
                                        "Right Speaker"};
  struct mixer_name *coupled_controls =
      mixer_name_add_array(NULL, coupled_output_names,
                           ARRAY_SIZE(coupled_output_names),
                           CRAS_STREAM_OUTPUT,
                           MIXER_NAME_VOLUME);
  int element_playback_volume[] = {1, 1, 0, 0};
  int element_playback_switches[] = {0, 0, 1, 1};

  long target_dBFS = -30;
  long expected_dB_value = target_dBFS + max_volumes[0];

  ResetStubData();

  snd_mixer_find_elem_map[std::string("Left Master")] =
      reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_find_elem_map[std::string("Right Master")] =
      reinterpret_cast<snd_mixer_elem_t *>(2);
  snd_mixer_find_elem_map[std::string("Left Speaker")] =
      reinterpret_cast<snd_mixer_elem_t *>(3);
  snd_mixer_find_elem_map[std::string("Right Speaker")] =
      reinterpret_cast<snd_mixer_elem_t *>(4);

  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);

  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);

  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, coupled_controls);

  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);

  output_control = c->output_controls;
  EXPECT_EQ(NULL, output_control->next);
  c1 = output_control->elements;
  c2 = c1->next;
  c3 = c2->next;
  c4 = c3->next;
  EXPECT_EQ(c1->elem, reinterpret_cast<snd_mixer_elem_t *>(1));
  EXPECT_EQ(c2->elem, reinterpret_cast<snd_mixer_elem_t *>(2));
  EXPECT_EQ(c3->elem, reinterpret_cast<snd_mixer_elem_t *>(3));
  EXPECT_EQ(c4->elem, reinterpret_cast<snd_mixer_elem_t *>(4));
  EXPECT_EQ(c4->next, reinterpret_cast<mixer_control_element *>(NULL));
  EXPECT_EQ(c1->has_volume, 1);
  EXPECT_EQ(c1->has_mute, 0);
  EXPECT_EQ(c2->has_volume, 1);
  EXPECT_EQ(c2->has_mute, 0);
  EXPECT_EQ(c3->has_volume, 0);
  EXPECT_EQ(c3->has_mute, 1);
  EXPECT_EQ(c4->has_volume, 0);
  EXPECT_EQ(c4->has_mute, 1);
  EXPECT_EQ(output_control->max_volume_dB, max_volumes[0]);
  EXPECT_EQ(output_control->min_volume_dB, min_volumes[0]);

  snd_mixer_selem_set_playback_dB_all_values = set_dB_values;
  snd_mixer_selem_set_playback_dB_all_values_length =
      ARRAY_SIZE(set_dB_values);

  cras_alsa_mixer_set_dBFS(c, target_dBFS, output_control);

  /* Set volume should set playback dB on two of the coupled controls. */
  EXPECT_EQ(2, snd_mixer_selem_set_playback_dB_all_called);
  EXPECT_EQ(set_dB_values[0], expected_dB_value);
  EXPECT_EQ(set_dB_values[1], expected_dB_value);

  /* Mute should set playback switch on two of the coupled controls. */
  cras_alsa_mixer_set_mute(c, 1, output_control);
  EXPECT_EQ(2, snd_mixer_selem_set_playback_switch_all_called);
  EXPECT_EQ(0, snd_mixer_selem_set_playback_switch_all_value);

  /* Unmute should set playback switch on two of the coupled controls. */
  cras_alsa_mixer_set_mute(c, 0, output_control);
  EXPECT_EQ(4, snd_mixer_selem_set_playback_switch_all_called);
  EXPECT_EQ(1, snd_mixer_selem_set_playback_switch_all_value);

  EXPECT_EQ(max_volumes[0] - min_volumes[0],
            cras_alsa_mixer_get_output_dB_range(output_control));

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_name_free(coupled_controls);
}

TEST(AlsaMixer, CoupledOutputHasMuteNoVolume) {
  struct cras_alsa_mixer *c;
  struct mixer_control *output_control;
  struct mixer_control_element *c1, *c2, *c3, *c4;

  static const long min_volumes[] = {-70};
  static const long max_volumes[] = {30};

  const char *coupled_output_names[] = {"Left Master",
                                        "Right Master",
                                        "Left Speaker",
                                        "Right Speaker"};
  struct mixer_name *coupled_controls =
      mixer_name_add_array(NULL, coupled_output_names,
                           ARRAY_SIZE(coupled_output_names),
                           CRAS_STREAM_OUTPUT,
                           MIXER_NAME_VOLUME);
  int element_playback_volume[] = {0, 0, 0, 0};
  int element_playback_switches[] = {0, 0, 1, 1};

  ResetStubData();

  snd_mixer_find_elem_map[std::string("Left Master")] =
      reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_find_elem_map[std::string("Right Master")] =
      reinterpret_cast<snd_mixer_elem_t *>(2);
  snd_mixer_find_elem_map[std::string("Left Speaker")] =
      reinterpret_cast<snd_mixer_elem_t *>(3);
  snd_mixer_find_elem_map[std::string("Right Speaker")] =
      reinterpret_cast<snd_mixer_elem_t *>(4);

  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);

  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);

  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, coupled_controls);

  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);

  output_control = c->output_controls;
  EXPECT_EQ(NULL, output_control->next);
  c1 = output_control->elements;
  c2 = c1->next;
  c3 = c2->next;
  c4 = c3->next;
  EXPECT_EQ(c1->elem, reinterpret_cast<snd_mixer_elem_t *>(1));
  EXPECT_EQ(c2->elem, reinterpret_cast<snd_mixer_elem_t *>(2));
  EXPECT_EQ(c3->elem, reinterpret_cast<snd_mixer_elem_t *>(3));
  EXPECT_EQ(c4->elem, reinterpret_cast<snd_mixer_elem_t *>(4));
  EXPECT_EQ(c4->next, reinterpret_cast<mixer_control_element *>(NULL));
  EXPECT_EQ(c1->has_volume, 0);
  EXPECT_EQ(c1->has_mute, 0);
  EXPECT_EQ(c2->has_volume, 0);
  EXPECT_EQ(c2->has_mute, 0);
  EXPECT_EQ(c3->has_volume, 0);
  EXPECT_EQ(c3->has_mute, 1);
  EXPECT_EQ(c4->has_volume, 0);
  EXPECT_EQ(c4->has_mute, 1);

  EXPECT_EQ(0, cras_alsa_mixer_has_volume(output_control));
  EXPECT_EQ(1, output_control->has_mute);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_name_free(coupled_controls);
}

TEST(AlsaMixer, CoupledOutputHasVolumeNoMute) {
  struct cras_alsa_mixer *c;
  struct mixer_control *output_control;
  struct mixer_control_element *c1, *c2, *c3, *c4;

  static const long min_volumes[] = {-70, -70};
  static const long max_volumes[] = {30, 30};

  const char *coupled_output_names[] = {"Left Master",
                                        "Right Master",
                                        "Left Speaker",
                                        "Right Speaker"};
  struct mixer_name *coupled_controls =
      mixer_name_add_array(NULL, coupled_output_names,
                           ARRAY_SIZE(coupled_output_names),
                           CRAS_STREAM_OUTPUT,
                           MIXER_NAME_VOLUME);
  int element_playback_volume[] = {1, 1, 0, 0};
  int element_playback_switches[] = {0, 0, 0, 0};

  ResetStubData();

  snd_mixer_find_elem_map[std::string("Left Master")] =
      reinterpret_cast<snd_mixer_elem_t *>(1);
  snd_mixer_find_elem_map[std::string("Right Master")] =
      reinterpret_cast<snd_mixer_elem_t *>(2);
  snd_mixer_find_elem_map[std::string("Left Speaker")] =
      reinterpret_cast<snd_mixer_elem_t *>(3);
  snd_mixer_find_elem_map[std::string("Right Speaker")] =
      reinterpret_cast<snd_mixer_elem_t *>(4);

  snd_mixer_selem_has_playback_volume_return_values = element_playback_volume;
  snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
  snd_mixer_selem_has_playback_switch_return_values = element_playback_switches;
  snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);

  snd_mixer_selem_get_playback_dB_range_min_values = min_volumes;
  snd_mixer_selem_get_playback_dB_range_max_values = max_volumes;
  snd_mixer_selem_get_playback_dB_range_values_length = ARRAY_SIZE(min_volumes);

  c = create_mixer_and_add_controls_by_name_matching(
      "hw:0", NULL, coupled_controls);

  ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), c);
  EXPECT_EQ(1, snd_mixer_open_called);
  EXPECT_EQ(1, snd_mixer_attach_called);
  EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
  EXPECT_EQ(1, snd_mixer_selem_register_called);
  EXPECT_EQ(1, snd_mixer_load_called);
  EXPECT_EQ(0, snd_mixer_close_called);

  output_control = c->output_controls;
  EXPECT_EQ(NULL, output_control->next);
  c1 = output_control->elements;
  c2 = c1->next;
  c3 = c2->next;
  c4 = c3->next;
  EXPECT_EQ(c1->elem, reinterpret_cast<snd_mixer_elem_t *>(1));
  EXPECT_EQ(c2->elem, reinterpret_cast<snd_mixer_elem_t *>(2));
  EXPECT_EQ(c3->elem, reinterpret_cast<snd_mixer_elem_t *>(3));
  EXPECT_EQ(c4->elem, reinterpret_cast<snd_mixer_elem_t *>(4));
  EXPECT_EQ(c4->next, reinterpret_cast<mixer_control_element *>(NULL));
  EXPECT_EQ(c1->has_volume, 1);
  EXPECT_EQ(c1->has_mute, 0);
  EXPECT_EQ(c2->has_volume, 1);
  EXPECT_EQ(c2->has_mute, 0);
  EXPECT_EQ(c3->has_volume, 0);
  EXPECT_EQ(c3->has_mute, 0);
  EXPECT_EQ(c4->has_volume, 0);
  EXPECT_EQ(c4->has_mute, 0);

  EXPECT_EQ(1, cras_alsa_mixer_has_volume(output_control));
  EXPECT_EQ(0, output_control->has_mute);

  cras_alsa_mixer_destroy(c);
  EXPECT_EQ(1, snd_mixer_close_called);
  mixer_name_free(coupled_controls);
}

TEST(AlsaMixer, MixerName) {
  struct mixer_name *names;
  struct mixer_name *control;
  size_t mixer_name_count;
  static const char *element_names[] = {
    "Master",
    "PCM",
    "Headphone",
    "Speaker",
    "HDMI",
    "IEC958",
  };

  names = mixer_name_add_array(NULL, element_names,
                               ARRAY_SIZE(element_names),
                               CRAS_STREAM_OUTPUT, MIXER_NAME_VOLUME);
  names = mixer_name_add(names, "Playback",
                         CRAS_STREAM_OUTPUT, MIXER_NAME_VOLUME);
  names = mixer_name_add(names, "Main",
                         CRAS_STREAM_OUTPUT, MIXER_NAME_MAIN_VOLUME);
  names = mixer_name_add(names, "Mic",
                         CRAS_STREAM_INPUT, MIXER_NAME_VOLUME);
  names = mixer_name_add(names, "Capture",
                         CRAS_STREAM_INPUT, MIXER_NAME_MAIN_VOLUME);

  /* Number of items (test mixer_name_add(_array)). */
  mixer_name_count = 0;
  DL_FOREACH(names, control) {
    mixer_name_count++;
  }
  EXPECT_EQ(10, mixer_name_count);

  /* Item not in the list: mismatch direction. */
  control = mixer_name_find(names, "Main",
                            CRAS_STREAM_INPUT, MIXER_NAME_UNDEFINED);
  EXPECT_EQ(1, control == NULL);

  /* Item not in the list: mismatch type. */
  control = mixer_name_find(names, "Main",
                            CRAS_STREAM_OUTPUT, MIXER_NAME_VOLUME);
  EXPECT_EQ(1, control == NULL);

  /* Find by name and direction. */
  control = mixer_name_find(names, "Main",
                            CRAS_STREAM_OUTPUT, MIXER_NAME_UNDEFINED);
  EXPECT_EQ(0, strcmp("Main", control->name));

  /* Find by type and direction. */
  control = mixer_name_find(names, NULL,
                            CRAS_STREAM_INPUT, MIXER_NAME_VOLUME);
  EXPECT_EQ(0, strcmp("Mic", control->name));

  mixer_name_free(names);
}

class AlsaMixerFullySpeced : public testing::Test {
 protected:
  virtual void SetUp() {
    callback_values_.clear();
    callback_called_ = 0;
    static snd_mixer_elem_t *elements[] = {
      reinterpret_cast<snd_mixer_elem_t *>(1),  // HP-L
      reinterpret_cast<snd_mixer_elem_t *>(2),  // HP-R
      reinterpret_cast<snd_mixer_elem_t *>(3),  // SPK-L
      reinterpret_cast<snd_mixer_elem_t *>(4),  // SPK-R
      reinterpret_cast<snd_mixer_elem_t *>(5),  // HDMI
      reinterpret_cast<snd_mixer_elem_t *>(6),  // CAPTURE
      reinterpret_cast<snd_mixer_elem_t *>(7),  // MIC-L
      reinterpret_cast<snd_mixer_elem_t *>(8),  // MIC-R
      reinterpret_cast<snd_mixer_elem_t *>(0),  // Unknown
    };
    static int element_playback_volume[] = {
      1,
      1,
      1,
      1,
      1,
      0, 0, 0,
    };
    static int element_playback_switches[] = {
      0,
      0,
      0,
      0,
      1,
      0, 0, 0,
    };
    static int element_capture_volume[] = {0, 0, 0, 0, 0,
      0,
      1,
      1,
    };
    static int element_capture_switches[] = {0, 0, 0, 0, 0,
      1,
      0,
      0,
    };
    static const long min_volumes[] = {-84, -84, -84, -84, -84, 0, 0, 0};
    static const long max_volumes[] = {0, 0, 0, 0, 0, 0, 84, 84};
    static const char *element_names[] = {
      "HP-L",
      "HP-R",
      "SPK-L",
      "SPK-R",
      "HDMI",
      "CAPTURE",
      "MIC-L",
      "MIC-R",
      "Unknown"
    };
    struct ucm_section *sections = NULL;
    struct ucm_section *section;
    size_t i;

    ResetStubData();

    for (i = 0; i < ARRAY_SIZE(elements); i++)
       snd_mixer_find_elem_map[element_names[i]] = elements[i];

    section = ucm_section_create("NullElement", 0, CRAS_STREAM_OUTPUT,
                                 NULL, NULL);
    ucm_section_set_mixer_name(section, "Unknown");
    DL_APPEND(sections, section);
    section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                                 "my-sound-card Headset Jack", "gpio");
    ucm_section_add_coupled(section, "HP-L", MIXER_NAME_VOLUME);
    ucm_section_add_coupled(section, "HP-R", MIXER_NAME_VOLUME);
    DL_APPEND(sections, section);
    section = ucm_section_create("Speaker", 0, CRAS_STREAM_OUTPUT,
                                 NULL, NULL);
    ucm_section_add_coupled(section, "SPK-L", MIXER_NAME_VOLUME);
    ucm_section_add_coupled(section, "SPK-R", MIXER_NAME_VOLUME);
    DL_APPEND(sections, section);
    section = ucm_section_create("Mic", 0, CRAS_STREAM_INPUT,
                                 "my-sound-card Headset Jack", "gpio");
    ucm_section_set_mixer_name(section, "CAPTURE");
    DL_APPEND(sections, section);
    section = ucm_section_create("Internal Mic", 0, CRAS_STREAM_INPUT,
                                 NULL, NULL);
    ucm_section_add_coupled(section, "MIC-L", MIXER_NAME_VOLUME);
    ucm_section_add_coupled(section, "MIC-R", MIXER_NAME_VOLUME);
    DL_APPEND(sections, section);
    section = ucm_section_create("HDMI", 0, CRAS_STREAM_OUTPUT,
                                 NULL, NULL);
    ucm_section_set_mixer_name(section, "HDMI");
    DL_APPEND(sections, section);
    ASSERT_NE(sections, (struct ucm_section *)NULL);

    snd_mixer_selem_has_playback_volume_return_values =
        element_playback_volume;
    snd_mixer_selem_has_playback_volume_return_values_length =
      ARRAY_SIZE(element_playback_volume);
    snd_mixer_selem_has_playback_switch_return_values =
        element_playback_switches;
    snd_mixer_selem_has_playback_switch_return_values_length =
      ARRAY_SIZE(element_playback_switches);
    snd_mixer_selem_has_capture_volume_return_values =
        element_capture_volume;
    snd_mixer_selem_has_capture_volume_return_values_length =
      ARRAY_SIZE(element_capture_volume);
    snd_mixer_selem_has_capture_switch_return_values =
        element_capture_switches;
    snd_mixer_selem_has_capture_switch_return_values_length =
      ARRAY_SIZE(element_capture_switches);
    snd_mixer_selem_get_name_return_values = element_names;
    snd_mixer_selem_get_name_return_values_length = ARRAY_SIZE(element_names);
    snd_mixer_selem_get_capture_dB_range_min_values = min_volumes;
    snd_mixer_selem_get_capture_dB_range_max_values = max_volumes;
    snd_mixer_selem_get_capture_dB_range_values_length =
        ARRAY_SIZE(min_volumes);

    cras_mixer_ = cras_alsa_mixer_create("hw:0");
    ASSERT_NE(static_cast<struct cras_alsa_mixer *>(NULL), cras_mixer_);
    EXPECT_EQ(1, snd_mixer_open_called);
    EXPECT_EQ(1, snd_mixer_attach_called);
    EXPECT_EQ(0, strcmp(snd_mixer_attach_mixdev, "hw:0"));
    EXPECT_EQ(1, snd_mixer_selem_register_called);
    EXPECT_EQ(1, snd_mixer_load_called);
    EXPECT_EQ(0, snd_mixer_close_called);

    section = sections;
    EXPECT_EQ(-ENOENT,\
              cras_alsa_mixer_add_controls_in_section(cras_mixer_, section));
    ASSERT_NE((struct ucm_section *)NULL, section->next);
    section = section->next;
    EXPECT_EQ(0, cras_alsa_mixer_add_controls_in_section(cras_mixer_, section));
    ASSERT_NE((struct ucm_section *)NULL, section->next);
    section = section->next;
    EXPECT_EQ(0, cras_alsa_mixer_add_controls_in_section(cras_mixer_, section));
    ASSERT_NE((struct ucm_section *)NULL, section->next);
    section = section->next;
    EXPECT_EQ(0, cras_alsa_mixer_add_controls_in_section(cras_mixer_, section));
    ASSERT_NE((struct ucm_section *)NULL, section->next);
    section = section->next;
    EXPECT_EQ(0, cras_alsa_mixer_add_controls_in_section(cras_mixer_, section));
    ASSERT_NE((struct ucm_section *)NULL, section->next);
    section = section->next;
    EXPECT_EQ(0, cras_alsa_mixer_add_controls_in_section(cras_mixer_, section));
    EXPECT_EQ(section->next, (struct ucm_section*)NULL);

    EXPECT_EQ(9, snd_mixer_find_selem_called);
    EXPECT_EQ(5, snd_mixer_selem_has_playback_volume_called);
    EXPECT_EQ(5, snd_mixer_selem_has_playback_switch_called);
    EXPECT_EQ(3, snd_mixer_selem_has_capture_volume_called);
    EXPECT_EQ(3, snd_mixer_selem_has_capture_switch_called);
    EXPECT_EQ(5, snd_mixer_selem_get_playback_dB_range_called);
    EXPECT_EQ(2, snd_mixer_selem_get_capture_dB_range_called);

    sections_ = sections;
  }

  virtual void TearDown() {
    ucm_section_free_list(sections_);
    cras_alsa_mixer_destroy(cras_mixer_);
    EXPECT_EQ(1, snd_mixer_close_called);
  }

  static void Callback(struct mixer_control *control, void *arg) {
    callback_called_++;
    callback_values_.push_back(control);
  }

  struct cras_alsa_mixer *cras_mixer_;
  static size_t callback_called_;
  static std::vector<struct mixer_control *> callback_values_;
  struct ucm_section *sections_;
};

size_t AlsaMixerFullySpeced::callback_called_;
std::vector<struct mixer_control *> AlsaMixerFullySpeced::callback_values_;

TEST_F(AlsaMixerFullySpeced, CheckControlCounts) {
  cras_alsa_mixer_list_outputs(cras_mixer_,
                               AlsaMixerFullySpeced::Callback,
                               reinterpret_cast<void*>(555));
  EXPECT_EQ(3, callback_called_);
  callback_called_ = 0;
  cras_alsa_mixer_list_inputs(cras_mixer_,
                               AlsaMixerFullySpeced::Callback,
                               reinterpret_cast<void*>(555));
  EXPECT_EQ(2, callback_called_);
}

TEST_F(AlsaMixerFullySpeced, CheckFindOutputByNameNoMatch) {
  struct mixer_control *out;

  out = cras_alsa_mixer_get_output_matching_name(cras_mixer_,
                                                 "AAAAA Jack");
  EXPECT_EQ(static_cast<struct mixer_control *>(NULL), out);
}

TEST_F(AlsaMixerFullySpeced, CheckFindOutputByName) {
  struct mixer_control *out;

  out = cras_alsa_mixer_get_output_matching_name(cras_mixer_,
                                                 "Headphone Jack");
  EXPECT_NE(static_cast<struct mixer_control *>(NULL), out);
}

TEST_F(AlsaMixerFullySpeced, CheckFindControlForSection) {
  struct mixer_control *control;
  struct ucm_section *section = sections_;

  // Look for the control for the Headphone section.
  // We've already asserted that section != NULL above.
  // Matching the control created by CoupledMixers.
  section = section->next;
  control = cras_alsa_mixer_get_control_for_section(cras_mixer_, section);
  ASSERT_NE(static_cast<struct mixer_control *>(NULL), control);
  EXPECT_EQ(0, strcmp(control->name, "Headphone"));

  // Look for the control for the Mic section.
  // Matching the control created by MixerName.
  section = section->next->next;
  control = cras_alsa_mixer_get_control_for_section(cras_mixer_, section);
  ASSERT_NE(static_cast<struct mixer_control *>(NULL), control);
  EXPECT_EQ(0, strcmp(control->name, "CAPTURE"));
}

/* Stubs */

extern "C" {
int snd_mixer_open(snd_mixer_t **mixer, int mode) {
  snd_mixer_open_called++;
  *mixer = reinterpret_cast<snd_mixer_t *>(2);
  return snd_mixer_open_return_value;
}
int snd_mixer_attach(snd_mixer_t *mixer, const char *name) {
  snd_mixer_attach_called++;
  snd_mixer_attach_mixdev = name;
  return snd_mixer_attach_return_value;
}
int snd_mixer_selem_register(snd_mixer_t *mixer,
                             struct snd_mixer_selem_regopt *options,
                             snd_mixer_class_t **classp) {
  snd_mixer_selem_register_called++;
  return snd_mixer_selem_register_return_value;
}
int snd_mixer_load(snd_mixer_t *mixer) {
  snd_mixer_load_called++;
  return snd_mixer_load_return_value;
}
const char *snd_mixer_selem_get_name(snd_mixer_elem_t *elem) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_get_name_called++;
  if (index >= snd_mixer_selem_get_name_return_values_length)
    return static_cast<char *>(NULL);

  return snd_mixer_selem_get_name_return_values[index];
}
unsigned int snd_mixer_selem_get_index(snd_mixer_elem_t *elem) {
  return 0;
}
int snd_mixer_selem_has_playback_volume(snd_mixer_elem_t *elem) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_has_playback_volume_called++;
  if (index >= snd_mixer_selem_has_playback_volume_return_values_length)
    return -1;

  return snd_mixer_selem_has_playback_volume_return_values[index];
}
int snd_mixer_selem_has_playback_switch(snd_mixer_elem_t *elem) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_has_playback_switch_called++;
  if (index >= snd_mixer_selem_has_playback_switch_return_values_length)
    return -1;

  return snd_mixer_selem_has_playback_switch_return_values[index];
}
int snd_mixer_selem_has_capture_volume(snd_mixer_elem_t *elem) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_has_capture_volume_called++;
  if (index >= snd_mixer_selem_has_capture_volume_return_values_length)
    return -1;

  return snd_mixer_selem_has_capture_volume_return_values[index];
}
int snd_mixer_selem_has_capture_switch(snd_mixer_elem_t *elem) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_has_capture_switch_called++;
  if (index >= snd_mixer_selem_has_capture_switch_return_values_length)
    return -1;

  return snd_mixer_selem_has_capture_switch_return_values[index];
}
snd_mixer_elem_t *snd_mixer_first_elem(snd_mixer_t *mixer) {
  snd_mixer_first_elem_called++;
  return snd_mixer_first_elem_return_value;
}
snd_mixer_elem_t *snd_mixer_elem_next(snd_mixer_elem_t *elem) {
  snd_mixer_elem_next_called++;
  if (snd_mixer_elem_next_return_values_index >=
      snd_mixer_elem_next_return_values_length)
    return static_cast<snd_mixer_elem_t *>(NULL);

  return snd_mixer_elem_next_return_values[
      snd_mixer_elem_next_return_values_index++];
}
int snd_mixer_close(snd_mixer_t *mixer) {
  snd_mixer_close_called++;
  return 0;
}
int snd_mixer_selem_set_playback_dB_all(snd_mixer_elem_t *elem,
                                        long value,
                                        int dir) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_set_playback_dB_all_called++;
  if (index < snd_mixer_selem_set_playback_dB_all_values_length)
    snd_mixer_selem_set_playback_dB_all_values[index] = value;
  return 0;
}
int snd_mixer_selem_get_playback_dB(snd_mixer_elem_t *elem,
                                    snd_mixer_selem_channel_id_t channel,
                                    long *value) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_get_playback_dB_called++;
  if (index >= snd_mixer_selem_get_playback_dB_return_values_length)
    *value = 0;
  else
    *value = snd_mixer_selem_get_playback_dB_return_values[index];
  return 0;
}
int snd_mixer_selem_set_playback_switch_all(snd_mixer_elem_t *elem, int value) {
  snd_mixer_selem_set_playback_switch_all_called++;
  snd_mixer_selem_set_playback_switch_all_value = value;
  return 0;
}
int snd_mixer_selem_set_capture_dB_all(snd_mixer_elem_t *elem,
                                       long value,
                                       int dir) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_set_capture_dB_all_called++;
  if (index < snd_mixer_selem_set_capture_dB_all_values_length)
    snd_mixer_selem_set_capture_dB_all_values[index] = value;
  return 0;
}
int snd_mixer_selem_get_capture_dB(snd_mixer_elem_t *elem,
                                   snd_mixer_selem_channel_id_t channel,
                                   long *value) {
  int index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_get_capture_dB_called++;
  if (index >= snd_mixer_selem_get_capture_dB_return_values_length)
    *value = 0;
  else
    *value = snd_mixer_selem_get_capture_dB_return_values[index];
  return 0;
}
int snd_mixer_selem_set_capture_switch_all(snd_mixer_elem_t *elem, int value) {
  snd_mixer_selem_set_capture_switch_all_called++;
  snd_mixer_selem_set_capture_switch_all_value = value;
  return 0;
}
int snd_mixer_selem_get_capture_dB_range(snd_mixer_elem_t *elem, long *min,
                                         long *max) {
  size_t index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_get_capture_dB_range_called++;
  if (index >= snd_mixer_selem_get_capture_dB_range_values_length) {
    *min = 0;
    *max = 0;
  } else {
    *min = snd_mixer_selem_get_capture_dB_range_min_values[index];
    *max = snd_mixer_selem_get_capture_dB_range_max_values[index];
  }
  return 0;
}
int snd_mixer_selem_get_playback_dB_range(snd_mixer_elem_t *elem,
                                          long *min,
                                          long *max) {
  size_t index = reinterpret_cast<size_t>(elem) - 1;
  snd_mixer_selem_get_playback_dB_range_called++;
  if (index >= snd_mixer_selem_get_playback_dB_range_values_length) {
    *min = 0;
    *max = 0;
  } else {
    *min = snd_mixer_selem_get_playback_dB_range_min_values[index];
    *max = snd_mixer_selem_get_playback_dB_range_max_values[index];
  }
  return 0;
}

snd_mixer_elem_t *snd_mixer_find_selem(
    snd_mixer_t *mixer, const snd_mixer_selem_id_t *id) {
  std::string name(snd_mixer_selem_id_get_name(id));
  unsigned int index = snd_mixer_selem_id_get_index(id);
  snd_mixer_find_selem_called++;
  if (index != 0)
    return NULL;
  if (snd_mixer_find_elem_map.find(name) == snd_mixer_find_elem_map.end()) {
    return NULL;
  }
  return snd_mixer_find_elem_map[name];
}

//  From cras_volume_curve.
static long get_dBFS_default(const struct cras_volume_curve *curve,
			     size_t volume)
{
  return 100 * (volume - 100);
}

struct cras_volume_curve *cras_volume_curve_create_default()
{
  struct cras_volume_curve *curve;
  curve = (struct cras_volume_curve *)calloc(1, sizeof(*curve));
  if (curve)
    curve->get_dBFS = get_dBFS_default;
  return curve;
}

void cras_volume_curve_destroy(struct cras_volume_curve *curve)
{
  cras_volume_curve_destroy_called++;
  free(curve);
}

// From libiniparser.
struct cras_volume_curve *cras_card_config_get_volume_curve_for_control(
		const struct cras_card_config *card_config,
		const char *control_name)
{
  struct cras_volume_curve *curve;
  curve = (struct cras_volume_curve *)calloc(1, sizeof(*curve));
  if (curve != NULL)
    curve->get_dBFS = get_dBFS_default;
  return curve;
}

} /* extern "C" */

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  openlog(NULL, LOG_PERROR, LOG_USER);
  return RUN_ALL_TESTS();
}
