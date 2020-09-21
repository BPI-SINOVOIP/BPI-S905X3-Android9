// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <iniparser.h>
#include <stdio.h>
#include <syslog.h>
#include <map>

extern "C" {
#include "cras_alsa_ucm.h"
#include "cras_types.h"
#include "cras_util.h"
#include "utlist.h"
#include "cras_util.h"

//  Include C file to test static functions.
#include "cras_alsa_ucm.c"
}

namespace {

static int snd_use_case_mgr_open_return;
static snd_use_case_mgr_t *snd_use_case_mgr_open_mgr_ptr;
static unsigned snd_use_case_mgr_open_called;
static unsigned snd_use_case_mgr_close_called;
static unsigned snd_use_case_get_called;
static std::vector<std::string> snd_use_case_get_id;
static int snd_use_case_set_return;
static std::map<std::string, std::string> snd_use_case_get_value;
static unsigned snd_use_case_set_called;
static std::vector<std::pair<std::string, std::string> > snd_use_case_set_param;
static std::map<std::string, const char **> fake_list;
static std::map<std::string, unsigned> fake_list_size;
static unsigned snd_use_case_free_list_called;
static std::vector<std::string> list_devices_callback_names;
static std::vector<void*> list_devices_callback_args;
static struct cras_use_case_mgr cras_ucm_mgr;
static const char *avail_verbs[] = { "HiFi", "Comment for Verb1" };

static void ResetStubData() {
  snd_use_case_mgr_open_called = 0;
  snd_use_case_mgr_open_return = 0;
  snd_use_case_mgr_close_called = 0;
  snd_use_case_set_return = 0;
  snd_use_case_get_called = 0;
  snd_use_case_set_called = 0;
  snd_use_case_set_param.clear();
  snd_use_case_free_list_called = 0;
  snd_use_case_get_id.clear();
  snd_use_case_get_value.clear();
  fake_list.clear();
  fake_list_size.clear();
  fake_list["_verbs"] = avail_verbs;
  fake_list_size["_verbs"] = 2;
  list_devices_callback_names.clear();
  list_devices_callback_args.clear();
  snd_use_case_mgr_open_mgr_ptr = reinterpret_cast<snd_use_case_mgr_t*>(0x55);
  cras_ucm_mgr.use_case = CRAS_STREAM_TYPE_DEFAULT;
}

static void list_devices_callback(const char* section_name, void *arg) {
  list_devices_callback_names.push_back(std::string(section_name));
  list_devices_callback_args.push_back(arg);
}

static void SetSectionDeviceData() {
  static const char *sections[] = { "Speaker", "Comment for Dev1",
                                    "IntMic", "Comment for Dev2",
                                    "Headphone", "Comment for Dev3",
                                    "ExtMic", "Comment for Dev4",
                                    "HDMI", "Comment for Dev5"};
  fake_list["_devices/HiFi"] = sections;
  fake_list_size["_devices/HiFi"] = 10;
  std::string id_1 = "=PlaybackPCM/Speaker/HiFi";
  std::string id_2 = "=CapturePCM/IntMic/HiFi";
  std::string id_3 = "=PlaybackPCM/Headphone/HiFi";
  std::string id_4 = "=CapturePCM/ExtMic/HiFi";
  std::string id_5 = "=PlaybackPCM/HDMI/HiFi";
  std::string value_1 = "test_card:0";
  std::string value_2 = "test_card:0";
  std::string value_3 = "test_card:0";
  std::string value_4 = "test_card:0";
  std::string value_5 = "test_card:1";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  snd_use_case_get_value[id_3] = value_3;
  snd_use_case_get_value[id_4] = value_4;
  snd_use_case_get_value[id_5] = value_5;
}

TEST(AlsaUcm, CreateFailInvalidCard) {
  ResetStubData();
  EXPECT_EQ(NULL, ucm_create(NULL));
  EXPECT_EQ(0, snd_use_case_mgr_open_called);
}

TEST(AlsaUcm, CreateFailCardNotFound) {
  ResetStubData();
  snd_use_case_mgr_open_return = -1;
  EXPECT_EQ(NULL, ucm_create("foo"));
  EXPECT_EQ(1, snd_use_case_mgr_open_called);
}

TEST(AlsaUcm, CreateFailNoHiFi) {
  ResetStubData();
  snd_use_case_set_return = -1;
  EXPECT_EQ(NULL, ucm_create("foo"));
  EXPECT_EQ(1, snd_use_case_mgr_open_called);
  EXPECT_EQ(1, snd_use_case_set_called);
  EXPECT_EQ(1, snd_use_case_mgr_close_called);
}

TEST(AlsaUcm, CreateSuccess) {
  struct cras_use_case_mgr *mgr;

  ResetStubData();

  mgr = ucm_create("foo");
  EXPECT_NE(static_cast<snd_use_case_mgr_t*>(NULL), mgr->mgr);
  EXPECT_EQ(1, snd_use_case_mgr_open_called);
  EXPECT_EQ(1, snd_use_case_set_called);
  EXPECT_EQ(0, snd_use_case_mgr_close_called);

  ucm_destroy(mgr);
  EXPECT_EQ(1, snd_use_case_mgr_close_called);
}

TEST(AlsaUcm, CheckEnabledEmptyList) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;

  ResetStubData();
  fake_list["_enadevs"] = NULL;
  fake_list_size["_enadevs"] = 0;

  EXPECT_EQ(0, ucm_set_enabled(mgr, "Dev1", 0));
  EXPECT_EQ(0, snd_use_case_set_called);

  EXPECT_EQ(0, ucm_set_enabled(mgr, "Dev1", 1));
  EXPECT_EQ(1, snd_use_case_set_called);

  EXPECT_EQ(0, snd_use_case_free_list_called);
}

TEST(AlsaUcm, CheckEnabledAlready) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *enabled[] = { "Dev2", "Dev1" };

  ResetStubData();

  fake_list["_enadevs"] = enabled;
  fake_list_size["_enadevs"] = 2;

  EXPECT_EQ(0, ucm_set_enabled(mgr, "Dev1", 1));
  EXPECT_EQ(0, snd_use_case_set_called);

  EXPECT_EQ(0, ucm_set_enabled(mgr, "Dev1", 0));
  EXPECT_EQ(1, snd_use_case_set_called);

  EXPECT_EQ(2, snd_use_case_free_list_called);
}

TEST(AlsaUcm, GetEdidForDev) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  std::string id = "=EDIDFile/Dev1/HiFi";
  std::string value = "EdidFileName";
  const char *file_name;

  ResetStubData();

  snd_use_case_get_value[id] = value;

  file_name = ucm_get_edid_file_for_dev(mgr, "Dev1");
  ASSERT_TRUE(file_name);
  EXPECT_EQ(0, strcmp(file_name, value.c_str()));
  free((void*)file_name);

  ASSERT_EQ(1, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id);
}

TEST(AlsaUcm, GetCapControlForDev) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  char *cap_control;
  std::string id = "=CaptureControl/Dev1/HiFi";
  std::string value = "MIC";

  ResetStubData();

  snd_use_case_get_value[id] = value;

  cap_control = ucm_get_cap_control(mgr, "Dev1");
  ASSERT_TRUE(cap_control);
  EXPECT_EQ(0, strcmp(cap_control, value.c_str()));
  free(cap_control);

  ASSERT_EQ(1, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id);
}

TEST(AlsaUcm, GetOverrideType) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *override_type_name;
  std::string id = "=OverrideNodeType/Dev1/HiFi";
  std::string value = "HDMI";

  ResetStubData();

  snd_use_case_get_value[id] = value;

  override_type_name = ucm_get_override_type_name(mgr, "Dev1");
  ASSERT_TRUE(override_type_name);
  EXPECT_EQ(0, strcmp(override_type_name, value.c_str()));
  free((void*)override_type_name);

  ASSERT_EQ(1, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id);
}

TEST(AlsaUcm, GetSectionsForVar) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct section_name *section_names, *c;

  ResetStubData();

  const char *sections[] = { "Sec1", "Comment for Sec1",
                             "Sec2", "Comment for Sec2",
                             "Sec3", "Comment for Sec3"};
  fake_list["Identifier"] = sections;
  fake_list_size["Identifier"] = 6;
  std::string id_1 = "=Var/Sec1/HiFi";
  std::string id_2 = "=Var/Sec2/HiFi";
  std::string id_3 = "=Var/Sec3/HiFi";
  std::string value_1 = "Value1";
  std::string value_2 = "Value2";
  std::string value_3 = "Value2";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  snd_use_case_get_value[id_3] = value_3;

  section_names = ucm_get_sections_for_var(mgr, "Var", "Value2", "Identifier",
                                           CRAS_STREAM_OUTPUT);

  ASSERT_TRUE(section_names);
  EXPECT_EQ(0, strcmp(section_names->name, "Sec2"));
  EXPECT_EQ(0, strcmp(section_names->next->name, "Sec3"));

  ASSERT_EQ(3, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id_1);
  EXPECT_EQ(snd_use_case_get_id[1], id_2);
  EXPECT_EQ(snd_use_case_get_id[2], id_3);

  DL_FOREACH(section_names, c) {
    DL_DELETE(section_names, c);
    free((void*)c->name);
    free(c);
  }
}

TEST(AlsaUcm, GetDevForJack) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *dev_name;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=JackName/Dev1/HiFi";
  std::string id_2 = "=JackName/Dev2/HiFi";
  std::string value_1 = "Value1";
  std::string value_2 = "Value2";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  dev_name = ucm_get_dev_for_jack(mgr, value_2.c_str(), CRAS_STREAM_OUTPUT);
  ASSERT_TRUE(dev_name);
  EXPECT_EQ(0, strcmp(dev_name, "Dev2"));
  free((void*)dev_name);

  ASSERT_EQ(2, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id_1);
  EXPECT_EQ(snd_use_case_get_id[1], id_2);
}

TEST(AlsaUcm, GetDevForHeadphoneJack) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *dev_name;
  const char *devices[] = { "Mic", "Comment for Dev1", "Headphone",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=JackName/Mic/HiFi";
  std::string id_2 = "=JackName/Headphone/HiFi";
  std::string value = "JackValue";

  snd_use_case_get_value[id_1] = value;
  snd_use_case_get_value[id_2] = value;

  /* Looking for jack with matched value with output direction, Headphone will
   * be found even though Mic section has the matched value too. */
  dev_name = ucm_get_dev_for_jack(mgr, value.c_str(), CRAS_STREAM_OUTPUT);

  ASSERT_TRUE(dev_name);
  EXPECT_EQ(0, strcmp(dev_name, "Headphone"));
  free((void*)dev_name);
}

TEST(AlsaUcm, GetDevForMicJack) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *dev_name;
  const char *devices[] = { "Headphone", "Comment for Dev1", "Mic",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=JackName/Headphone/HiFi";
  std::string id_2 = "=JackName/Mic/HiFi";
  std::string value = "JackValue";

  snd_use_case_get_value[id_1] = value;
  snd_use_case_get_value[id_2] = value;

  /* Looking for jack with matched value with input direction, Mic will be found
   * even though Headphone section has the matched value too. */
  dev_name = ucm_get_dev_for_jack(mgr, value.c_str(), CRAS_STREAM_INPUT);

  ASSERT_TRUE(dev_name);
  EXPECT_EQ(0, strcmp(dev_name, "Mic"));
  free((void*)dev_name);
}

TEST(AlsaUcm, GetDevForMixer) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *dev_name_out, *dev_name_in;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=MixerName/Dev1/HiFi";
  std::string id_2 = "=MixerName/Dev2/HiFi";
  std::string value_1 = "Value1";
  std::string value_2 = "Value2";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  dev_name_out = ucm_get_dev_for_mixer(
      mgr, value_1.c_str(), CRAS_STREAM_OUTPUT);
  dev_name_in = ucm_get_dev_for_mixer(mgr, value_2.c_str(), CRAS_STREAM_INPUT);

  ASSERT_TRUE(dev_name_out);
  EXPECT_EQ(0, strcmp(dev_name_out, "Dev1"));
  free((void*)dev_name_out);

  ASSERT_TRUE(dev_name_in);
  EXPECT_EQ(0, strcmp(dev_name_in, "Dev2"));
  free((void*)dev_name_in);
}

TEST(AlsaUcm, GetDeviceNameForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *input_dev_name, *output_dev_name;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=CapturePCM/Dev1/HiFi";
  std::string id_2 = "=PlaybackPCM/Dev2/HiFi";
  std::string value_1 = "DeviceName1";
  std::string value_2 = "DeviceName2";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  input_dev_name = ucm_get_device_name_for_dev(mgr, "Dev1", CRAS_STREAM_INPUT);
  output_dev_name = ucm_get_device_name_for_dev(mgr, "Dev2", CRAS_STREAM_OUTPUT);
  ASSERT_TRUE(input_dev_name);
  ASSERT_TRUE(output_dev_name);
  EXPECT_EQ(0, strcmp(input_dev_name, value_1.c_str()));
  EXPECT_EQ(0, strcmp(output_dev_name, value_2.c_str()));

  ASSERT_EQ(2, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id_1);
  EXPECT_EQ(snd_use_case_get_id[1], id_2);
}

TEST(AlsaUcm, GetDeviceRateForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int input_dev_rate, output_dev_rate;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=CaptureRate/Dev1/HiFi";
  std::string id_2 = "=PlaybackRate/Dev2/HiFi";
  std::string value_1 = "44100";
  std::string value_2 = "48000";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  input_dev_rate = ucm_get_sample_rate_for_dev(mgr, "Dev1", CRAS_STREAM_INPUT);
  output_dev_rate = ucm_get_sample_rate_for_dev(mgr, "Dev2",
						CRAS_STREAM_OUTPUT);
  EXPECT_EQ(44100, input_dev_rate);
  EXPECT_EQ(48000, output_dev_rate);

  ASSERT_EQ(2, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id_1);
  EXPECT_EQ(snd_use_case_get_id[1], id_2);
}

TEST(AlsaUcm, GetCaptureChannelMapForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int8_t channel_layout[CRAS_CH_MAX];
  int rc;

  ResetStubData();

  std::string id_1 = "=CaptureChannelMap/Dev1/HiFi";
  std::string value_1 = "-1 -1 0 1 -1 -1 -1 -1 -1 -1 -1";

  snd_use_case_get_value[id_1] = value_1;
  rc = ucm_get_capture_chmap_for_dev(mgr, "Dev1", channel_layout);

  EXPECT_EQ(0, rc);

  ASSERT_EQ(1, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id_1);
  EXPECT_EQ(channel_layout[0], -1);
  EXPECT_EQ(channel_layout[1], -1);
  EXPECT_EQ(channel_layout[2], 0);
  EXPECT_EQ(channel_layout[3], 1);
  EXPECT_EQ(channel_layout[4], -1);
  EXPECT_EQ(channel_layout[5], -1);
  EXPECT_EQ(channel_layout[6], -1);
  EXPECT_EQ(channel_layout[7], -1);
  EXPECT_EQ(channel_layout[8], -1);
  EXPECT_EQ(channel_layout[9], -1);
  EXPECT_EQ(channel_layout[10], -1);
}

TEST(AlsaUcm, GetHotwordModels) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *models;
  const char *modifiers[] = { "Mod1",
                            "Comment1",
                            "Hotword Model en",
                            "Comment2",
                            "Hotword Model jp",
                            "Comment3",
                            "Mod2",
                            "Comment4",
                            "Hotword Model de",
                            "Comment5" };
  ResetStubData();

  fake_list["_modifiers/HiFi"] = modifiers;
  fake_list_size["_modifiers/HiFi"] = 10;

  models = ucm_get_hotword_models(mgr);
  ASSERT_TRUE(models);
  EXPECT_EQ(0, strcmp(models, "en,jp,de"));
}

TEST(AlsaUcm, SetHotwordModel) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *modifiers[] = { "Hotword Model en",
                              "Comment1",
                              "Hotword Model jp",
                              "Comment2",
                              "Hotword Model de",
                              "Comment3" };
  const char *enabled_mods[] = { "Hotword Model en" };
  ResetStubData();

  fake_list["_modifiers/HiFi"] = modifiers;
  fake_list_size["_modifiers/HiFi"] = 6;

  EXPECT_EQ(-EINVAL, ucm_set_hotword_model(mgr, "zh"));
  EXPECT_EQ(0, snd_use_case_set_called);

  fake_list["_enamods"] = enabled_mods;
  fake_list_size["_enamods"] = 1;
  ucm_set_hotword_model(mgr, "jp");

  EXPECT_EQ(2, snd_use_case_set_called);
  EXPECT_EQ(snd_use_case_set_param[0],
      std::make_pair(std::string("_dismod"), std::string("Hotword Model en")));
  EXPECT_EQ(snd_use_case_set_param[1],
      std::make_pair(std::string("_enamod"), std::string("Hotword Model jp")));
}

TEST(AlsaUcm, SwapModeExists) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int rc;
  const char *modifiers_1[] = { "Speaker Swap Mode",
                                "Comment for Speaker Swap Mode",
                                "Microphone Swap Mode",
                                "Comment for Microphone Swap Mode" };
  const char *modifiers_2[] = { "Speaker Some Mode",
                                "Comment for Speaker Some Mode",
                                "Microphone Some Mode",
                                "Comment for Microphone Some Mode" };

  ResetStubData();

  fake_list["_modifiers/HiFi"] = modifiers_1;
  fake_list_size["_modifiers/HiFi"] = 4;
  rc = ucm_swap_mode_exists(mgr);
  EXPECT_EQ(1, rc);

  fake_list["_modifiers/HiFi"] = modifiers_2;
  fake_list_size["_modifiers/HiFi"] = 4;
  rc = ucm_swap_mode_exists(mgr);
  EXPECT_EQ(0, rc);
}

TEST(AlsaUcm, EnableSwapMode) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int rc;
  const char *modifiers[] = { "Speaker Swap Mode",
                              "Comment for Speaker Swap Mode",
                              "Microphone Swap Mode",
                              "Comment for Microphone Swap Mode" };
  const char *modifiers_enabled[] = {"Speaker Swap Mode"};

  ResetStubData();

  fake_list["_modifiers/HiFi"] = modifiers;
  fake_list_size["_modifiers/HiFi"] = 4;

  fake_list["_enamods"] = modifiers_enabled;
  fake_list_size["_enamods"] = 1;

  snd_use_case_set_return = 0;

  rc = ucm_enable_swap_mode(mgr, "Headphone", 1);
  EXPECT_EQ(-EPERM, rc);
  EXPECT_EQ(0, snd_use_case_set_called);

  rc = ucm_enable_swap_mode(mgr, "Speaker", 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, snd_use_case_set_called);

  rc = ucm_enable_swap_mode(mgr, "Microphone", 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_use_case_set_called);
}

TEST(AlsaUcm, DisableSwapMode) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int rc;
  const char *modifiers[] = { "Speaker Swap Mode",
                              "Comment for Speaker Swap Mode",
                              "Microphone Swap Mode",
                              "Comment for Microphone Swap Mode" };
  const char *modifiers_enabled[] = {"Speaker Swap Mode"};

  ResetStubData();

  fake_list["_modifiers/HiFi"] = modifiers;
  fake_list_size["_modifiers/HiFi"] = 4;

  fake_list["_enamods"] = modifiers_enabled;
  fake_list_size["_enamods"] = 1;

  snd_use_case_set_return = 0;

  rc = ucm_enable_swap_mode(mgr, "Headphone", 0);
  EXPECT_EQ(-EPERM, rc);
  EXPECT_EQ(0, snd_use_case_set_called);

  rc = ucm_enable_swap_mode(mgr, "Microphone", 0);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, snd_use_case_set_called);

  rc = ucm_enable_swap_mode(mgr, "Speaker", 0);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, snd_use_case_set_called);

}

TEST(AlsaFlag, GetFlag) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  char *flag_value;

  std::string id = "=FlagName//HiFi";
  std::string value = "1";
  ResetStubData();

  snd_use_case_get_value[id] = value;

  flag_value = ucm_get_flag(mgr, "FlagName");
  ASSERT_TRUE(flag_value);
  EXPECT_EQ(0, strcmp(flag_value, value.c_str()));
  free(flag_value);

  ASSERT_EQ(1, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id);
}

TEST(AlsaUcm, ModifierEnabled) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int enabled;

  ResetStubData();

  const char *mods[] = { "Mod1", "Mod2" };
  fake_list["_enamods"] = mods;
  fake_list_size["_enamods"] = 2;

  enabled = modifier_enabled(mgr, "Mod1");
  EXPECT_EQ(1, enabled);
  enabled = modifier_enabled(mgr, "Mod2");
  EXPECT_EQ(1, enabled);
  enabled = modifier_enabled(mgr, "Mod3");
  EXPECT_EQ(0, enabled);
}

TEST(AlsaUcm, SetModifierEnabled) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;

  ResetStubData();

  ucm_set_modifier_enabled(mgr, "Mod1", 1);
  EXPECT_EQ(snd_use_case_set_param[0],
            std::make_pair(std::string("_enamod"), std::string("Mod1")));
  EXPECT_EQ(1, snd_use_case_set_called);
  ucm_set_modifier_enabled(mgr, "Mod1", 0);
  EXPECT_EQ(snd_use_case_set_param[1],
            std::make_pair(std::string("_dismod"), std::string("Mod1")));
  EXPECT_EQ(2, snd_use_case_set_called);
}

TEST(AlsaUcm, EndWithSuffix) {
  EXPECT_EQ(1, ucm_str_ends_with_suffix("Foo bar", "bar"));
  EXPECT_EQ(1, ucm_str_ends_with_suffix("bar", "bar"));
  EXPECT_EQ(0, ucm_str_ends_with_suffix("Foo car", "bar"));
}

TEST(AlsaUcm, SectionExistsWithName) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *sections[] = { "Sec1", "Comment for Sec1", "Sec2",
                             "Comment for Sec2" };

  ResetStubData();

  fake_list["Identifier"] = sections;
  fake_list_size["Identifier"] = 4;
  EXPECT_EQ(1, ucm_section_exists_with_name(mgr, "Sec1", "Identifier"));
  EXPECT_EQ(1, ucm_section_exists_with_name(mgr, "Sec2", "Identifier"));
  EXPECT_EQ(0, ucm_section_exists_with_name(mgr, "Sec3", "Identifier"));
}

TEST(AlsaUcm, SectionExistsWithSuffix) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;

  ResetStubData();

  const char *sections[] = { "Sec1 Suffix1", "Comment for Sec1",
                             "Sec2 Suffix2", "Comment for Sec2" };
  fake_list["Identifier"] = sections;
  fake_list_size["Identifier"] = 4;
  EXPECT_EQ(1, ucm_section_exists_with_suffix(mgr, "Suffix1", "Identifier"));
  EXPECT_EQ(1, ucm_section_exists_with_suffix(mgr, "Suffix2", "Identifier"));
  EXPECT_EQ(0, ucm_section_exists_with_suffix(mgr, "Suffix3", "Identifier"));
}

TEST(AlsaUcm, DisableSoftwareVolume) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  unsigned int disable_software_volume;
  std::string id = "=DisableSoftwareVolume//HiFi";
  std::string value = "1";

  ResetStubData();

  snd_use_case_get_value[id] = value;

  disable_software_volume = ucm_get_disable_software_volume(mgr);
  ASSERT_TRUE(disable_software_volume);

  ASSERT_EQ(1, snd_use_case_get_called);
  EXPECT_EQ(snd_use_case_get_id[0], id);
}

TEST(AlsaUcm, GetCoupledMixersForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct mixer_name *mixer_names_1, *mixer_names_2, *c;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=CoupledMixers/Dev1/HiFi";
  std::string value_1 = "Mixer Name1,Mixer Name2,Mixer Name3";
  std::string id_2 = "=CoupledMixers/Dev2/HiFi";
  std::string value_2 = "";
  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  mixer_names_1 = ucm_get_coupled_mixer_names(mgr, "Dev1");
  mixer_names_2 = ucm_get_coupled_mixer_names(mgr, "Dev2");

  ASSERT_TRUE(mixer_names_1);
  EXPECT_EQ(0, strcmp(mixer_names_1->name, "Mixer Name1"));
  EXPECT_EQ(0, strcmp(mixer_names_1->next->name, "Mixer Name2"));
  EXPECT_EQ(0, strcmp(mixer_names_1->next->next->name, "Mixer Name3"));
  EXPECT_EQ(NULL, mixer_names_1->next->next->next);

  EXPECT_EQ(NULL, mixer_names_2);

  DL_FOREACH(mixer_names_1, c) {
    DL_DELETE(mixer_names_1, c);
    free((void*)c->name);
    free(c);
  }
}

TEST(AlsaUcm, FreeMixerNames) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct mixer_name *mixer_names_1;
  const char *devices[] = { "Dev1", "Comment for Dev1"};

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 2;
  std::string id_1 = "=CoupledMixers/Dev1/HiFi";
  std::string value_1 = "Mixer Name1,Mixer Name2,Mixer Name3";
  snd_use_case_get_value[id_1] = value_1;
  mixer_names_1 = ucm_get_coupled_mixer_names(mgr, "Dev1");


  ASSERT_TRUE(mixer_names_1);
  EXPECT_EQ(0, strcmp(mixer_names_1->name, "Mixer Name1"));
  EXPECT_EQ(0, strcmp(mixer_names_1->next->name, "Mixer Name2"));
  EXPECT_EQ(0, strcmp(mixer_names_1->next->next->name, "Mixer Name3"));
  EXPECT_EQ(NULL, mixer_names_1->next->next->next);

  /* No way to actually check if memory is freed. */
  mixer_name_free(mixer_names_1);
}

TEST(AlsaUcm, MaxSoftwareGain) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  long max_software_gain;
  int ret;
  std::string id = "=MaxSoftwareGain/Internal Mic/HiFi";
  std::string value = "2000";

  ResetStubData();

  /* Value can be found in UCM. */
  snd_use_case_get_value[id] = value;

  ret = ucm_get_max_software_gain(mgr, "Internal Mic", &max_software_gain);

  EXPECT_EQ(0, ret);
  EXPECT_EQ(2000, max_software_gain);

  ResetStubData();

  /* Value can not be found in UCM. */
  ret = ucm_get_max_software_gain(mgr, "Internal Mic", &max_software_gain);

  ASSERT_TRUE(ret);
}

TEST(AlsaUcm, DefaultNodeGain) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  long default_node_gain;
  int ret;
  std::string id = "=DefaultNodeGain/Internal Mic/HiFi";
  std::string value = "-2000";

  ResetStubData();

  /* Value can be found in UCM. */
  snd_use_case_get_value[id] = value;

  ret = ucm_get_default_node_gain(mgr, "Internal Mic", &default_node_gain);

  EXPECT_EQ(0, ret);
  EXPECT_EQ(-2000, default_node_gain);

  ResetStubData();

  /* Value can not be found in UCM. */
  ret = ucm_get_default_node_gain(mgr, "Internal Mic", &default_node_gain);

  ASSERT_TRUE(ret);
}

TEST(AlsaUcm, UseFullySpecifiedUCMConfig) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int fully_specified_flag;

  std::string id = "=FullySpecifiedUCM//HiFi";
  ResetStubData();

  /* Flag is not set */
  fully_specified_flag = ucm_has_fully_specified_ucm_flag(mgr);
  ASSERT_FALSE(fully_specified_flag);

  /* Flag is set to "1". */
  snd_use_case_get_value[id] = std::string("1");
  fully_specified_flag = ucm_has_fully_specified_ucm_flag(mgr);
  ASSERT_TRUE(fully_specified_flag);

  /* Flag is set to "0". */
  snd_use_case_get_value[id] = std::string("0");
  fully_specified_flag = ucm_has_fully_specified_ucm_flag(mgr);
  ASSERT_FALSE(fully_specified_flag);
}

TEST(AlsaUcm, EnableHtimestampFlag) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  unsigned int enable_htimestamp_flag;

  std::string id = "=EnableHtimestamp//HiFi";
  ResetStubData();

  /* Flag is not set */
  enable_htimestamp_flag = ucm_get_enable_htimestamp_flag(mgr);
  ASSERT_FALSE(enable_htimestamp_flag);

  /* Flag is set to "1". */
  snd_use_case_get_value[id] = std::string("1");
  enable_htimestamp_flag = ucm_get_enable_htimestamp_flag(mgr);
  ASSERT_TRUE(enable_htimestamp_flag);

  /* Flag is set to "0". */
  snd_use_case_get_value[id] = std::string("0");
  enable_htimestamp_flag = ucm_get_enable_htimestamp_flag(mgr);
  ASSERT_FALSE(enable_htimestamp_flag);
}

TEST(AlsaUcm, GetMixerNameForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *mixer_name_1, *mixer_name_2;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=MixerName/Dev1/HiFi";
  std::string id_2 = "=MixerName/Dev2/HiFi";
  std::string value_1 = "MixerName1";
  std::string value_2 = "MixerName2";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  mixer_name_1 = ucm_get_mixer_name_for_dev(mgr, "Dev1");
  mixer_name_2 = ucm_get_mixer_name_for_dev(mgr, "Dev2");

  EXPECT_EQ(0, strcmp(mixer_name_1, value_1.c_str()));
  EXPECT_EQ(0, strcmp(mixer_name_2, value_2.c_str()));
}

TEST(AlsaUcm, GetMainVolumeMixerName) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct mixer_name *mixer_names_1, *mixer_names_2, *c;

  ResetStubData();

  std::string id = "=MainVolumeNames//HiFi";
  std::string value_1 = "Mixer Name1,Mixer Name2,Mixer Name3";

  snd_use_case_get_value[id] = value_1;
  mixer_names_1 = ucm_get_main_volume_names(mgr);

  ResetStubData();

  /* Can not find MainVolumeNames */
  mixer_names_2 = ucm_get_main_volume_names(mgr);

  ASSERT_TRUE(mixer_names_1);
  EXPECT_EQ(0, strcmp(mixer_names_1->name, "Mixer Name1"));
  EXPECT_EQ(0, strcmp(mixer_names_1->next->name, "Mixer Name2"));
  EXPECT_EQ(0, strcmp(mixer_names_1->next->next->name, "Mixer Name3"));
  EXPECT_EQ(NULL, mixer_names_1->next->next->next);

  DL_FOREACH(mixer_names_1, c) {
    DL_DELETE(mixer_names_1, c);
    free((void*)c->name);
    free(c);
  }

  EXPECT_EQ(NULL, mixer_names_2);
}

TEST(AlsaUcm, ListSectionsByDeviceNameOutput) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  void* callback_arg = reinterpret_cast<void*>(0x56);
  int listed = 0;

  ResetStubData();
  SetSectionDeviceData();

  listed = ucm_list_section_devices_by_device_name(
      mgr, CRAS_STREAM_OUTPUT, "test_card:0", list_devices_callback,
      callback_arg);

  EXPECT_EQ(2, listed);
  EXPECT_EQ(2, list_devices_callback_names.size());
  EXPECT_EQ(2, list_devices_callback_args.size());

  EXPECT_EQ(
      0, strcmp(list_devices_callback_names[0].c_str(), "Speaker"));
  EXPECT_EQ(callback_arg, list_devices_callback_args[0]);

  EXPECT_EQ(
      0, strcmp(list_devices_callback_names[1].c_str(), "Headphone"));
  EXPECT_EQ(callback_arg, list_devices_callback_args[1]);
}

TEST(AlsaUcm, ListSectionsByDeviceNameInput) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  void* callback_arg = reinterpret_cast<void*>(0x56);
  int listed = 0;

  ResetStubData();
  SetSectionDeviceData();

  listed = ucm_list_section_devices_by_device_name(
      mgr, CRAS_STREAM_INPUT, "test_card:0", list_devices_callback,
      callback_arg);

  EXPECT_EQ(2, listed);
  EXPECT_EQ(2, list_devices_callback_names.size());
  EXPECT_EQ(2, list_devices_callback_args.size());

  EXPECT_EQ(
      0, strcmp(list_devices_callback_names[0].c_str(), "IntMic"));
  EXPECT_EQ(callback_arg, list_devices_callback_args[0]);

  EXPECT_EQ(
      0, strcmp(list_devices_callback_names[1].c_str(), "ExtMic"));
  EXPECT_EQ(callback_arg, list_devices_callback_args[1]);
}

TEST(AlsaUcm, GetJackNameForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *jack_name_1, *jack_name_2;
  const char *devices[] = { "Dev1", "Comment for Dev1", "Dev2",
                            "Comment for Dev2" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 4;
  std::string id_1 = "=JackName/Dev1/HiFi";
  std::string value_1 = "JackName1";

  snd_use_case_get_value[id_1] = value_1;
  jack_name_1 = ucm_get_jack_name_for_dev(mgr, "Dev1");
  jack_name_2 = ucm_get_jack_name_for_dev(mgr, "Dev2");

  EXPECT_EQ(0, strcmp(jack_name_1, value_1.c_str()));
  EXPECT_EQ(NULL, jack_name_2);
}

TEST(AlsaUcm, GetJackTypeForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  const char *jack_type_1, *jack_type_2, *jack_type_3, *jack_type_4;
  const char *devices[] = { "Dev1", "Comment for Dev1",
                            "Dev2", "Comment for Dev2",
                            "Dev3", "Comment for Dev3",
                            "Dev4", "Comment for Dev4"};

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 8;
  std::string id_1 = "=JackType/Dev1/HiFi";
  std::string value_1 = "hctl";
  std::string id_2 = "=JackType/Dev2/HiFi";
  std::string value_2 = "gpio";
  std::string id_3 = "=JackType/Dev3/HiFi";
  std::string value_3 = "something";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;
  snd_use_case_get_value[id_3] = value_3;

  jack_type_1 = ucm_get_jack_type_for_dev(mgr, "Dev1");
  jack_type_2 = ucm_get_jack_type_for_dev(mgr, "Dev2");
  jack_type_3 = ucm_get_jack_type_for_dev(mgr, "Dev3");
  jack_type_4 = ucm_get_jack_type_for_dev(mgr, "Dev4");

  /* Only "hctl" and "gpio" are valid types. */
  EXPECT_EQ(0, strcmp(jack_type_1, value_1.c_str()));
  EXPECT_EQ(0, strcmp(jack_type_2, value_2.c_str()));
  EXPECT_EQ(NULL, jack_type_3);
  EXPECT_EQ(NULL, jack_type_4);
}

TEST(AlsaUcm, GetPeriodFramesForDevice) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  int dma_period_1, dma_period_2, dma_period_3;
  const char *devices[] = { "Dev1", "Comment for Dev1",
                            "Dev2", "Comment for Dev2",
                            "Dev3", "Comment for Dev3" };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = 6;
  std::string id_1 = "=DmaPeriodMicrosecs/Dev1/HiFi";
  std::string value_1 = "1000";
  std::string id_2 = "=DmaPeriodMicrosecs/Dev2/HiFi";
  std::string value_2 = "-10";

  snd_use_case_get_value[id_1] = value_1;
  snd_use_case_get_value[id_2] = value_2;

  dma_period_1 = ucm_get_dma_period_for_dev(mgr, "Dev1");
  dma_period_2 = ucm_get_dma_period_for_dev(mgr, "Dev2");
  dma_period_3 = ucm_get_dma_period_for_dev(mgr, "Dev3");

  /* Only "hctl" and "gpio" are valid types. */
  EXPECT_EQ(1000, dma_period_1);
  EXPECT_EQ(0, dma_period_2);
  EXPECT_EQ(0, dma_period_3);
}

TEST(AlsaUcm, UcmSection) {
  struct ucm_section *section_list = NULL;
  struct ucm_section *section;
  struct mixer_name *controls = NULL;
  struct mixer_name *m_name;
  int dev_idx = 0;
  size_t i;
  enum CRAS_STREAM_DIRECTION dir = CRAS_STREAM_OUTPUT;
  static const char *name = "Headphone";
  static const char *jack_name = "my-card-name Headset Jack";
  static const char *jack_type = "gpio";
  static const char *mixer_name = "Control1";
  static const char *coupled_names[] = {
    "Coupled1",
    "Coupled2"
  };

  section = ucm_section_create(NULL, 0, CRAS_STREAM_OUTPUT, NULL, NULL);
  EXPECT_EQ(reinterpret_cast<struct ucm_section*>(NULL), section);

  section = ucm_section_create(name, dev_idx, dir, jack_name, jack_type);
  EXPECT_NE(reinterpret_cast<struct ucm_section*>(NULL), section);
  EXPECT_NE(name, section->name);
  EXPECT_EQ(0, strcmp(name, section->name));
  EXPECT_EQ(dev_idx, section->dev_idx);
  EXPECT_EQ(dir, section->dir);
  EXPECT_NE(jack_name, section->jack_name);
  EXPECT_NE(jack_type, section->jack_type);
  EXPECT_EQ(section->prev, section);
  EXPECT_EQ(reinterpret_cast<const char *>(NULL), section->mixer_name);
  EXPECT_EQ(reinterpret_cast<struct mixer_name*>(NULL), section->coupled);

  EXPECT_EQ(-EINVAL, ucm_section_set_mixer_name(section, NULL));
  EXPECT_EQ(-EINVAL, ucm_section_set_mixer_name(NULL, mixer_name));
  EXPECT_EQ(0, ucm_section_set_mixer_name(section, mixer_name));

  EXPECT_NE(section->mixer_name, mixer_name);
  EXPECT_EQ(0, strcmp(section->mixer_name, mixer_name));

  EXPECT_EQ(-EINVAL, ucm_section_add_coupled(
                         section, NULL, MIXER_NAME_VOLUME));
  EXPECT_EQ(-EINVAL, ucm_section_add_coupled(
                         NULL, coupled_names[0], MIXER_NAME_VOLUME));
  EXPECT_EQ(0, ucm_section_add_coupled(
                         section, coupled_names[0], MIXER_NAME_VOLUME));

  EXPECT_EQ(-EINVAL, ucm_section_concat_coupled(section, NULL));
  EXPECT_EQ(-EINVAL, ucm_section_concat_coupled(
                         NULL, reinterpret_cast<struct mixer_name*>(0x1111)));

  controls = NULL;
  for (i = 1; i < ARRAY_SIZE(coupled_names); i++) {
    controls = mixer_name_add(controls, coupled_names[i],
                              CRAS_STREAM_OUTPUT, MIXER_NAME_VOLUME);
  }
  /* Add controls to the list of coupled controls for this section. */
  EXPECT_EQ(0, ucm_section_concat_coupled(section, controls));

  i = 0;
  DL_FOREACH(section->coupled, m_name) {
    EXPECT_NE(m_name->name, coupled_names[i]);
    EXPECT_EQ(0, strcmp(m_name->name, coupled_names[i]));
    i++;
  }
  EXPECT_EQ(i, ARRAY_SIZE(coupled_names));

  DL_APPEND(section_list, section);
  ucm_section_free_list(section_list);
}

TEST(AlsaUcm, GetSections) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct ucm_section* sections;
  struct ucm_section* section;
  struct mixer_name* m_name;
  int section_count = 0;
  int i = 0;
  const char *devices[] = { "Headphone", "The headphones jack.",
                            "Speaker", "The speakers.",
                            "Mic", "Microphone jack.",
                            "Internal Mic", "Internal Microphones",
                            "HDMI", "HDMI output" };
  const char* ids[] = {
    "=PlaybackPCM/Headphone/HiFi",
    "=JackName/Headphone/HiFi",
    "=JackType/Headphone/HiFi",
    "=JackSwitch/Headphone/HiFi",
    "=CoupledMixers/Headphone/HiFi",

    "=PlaybackPCM/Speaker/HiFi",
    "=CoupledMixers/Speaker/HiFi",

    "=CapturePCM/Mic/HiFi",
    "=JackName/Mic/HiFi",
    "=JackType/Mic/HiFi",
    "=JackSwitch/Mic/HiFi",
    "=MixerName/Mic/HiFi",

    "=CapturePCM/Internal Mic/HiFi",
    "=CoupledMixers/Internal Mic/HiFi",
    "=JackSwitch/Internal Mic/HiFi",

    "=PlaybackPCM/HDMI/HiFi",
    "=MixerName/HDMI/HiFi",

    NULL
  };
  const char* values[] = {
    "hw:my-sound-card,0",
    "my-sound-card Headset Jack",
    "gpio",
    "2",
    "HP-L,HP-R",

    "hw:my-sound-card,0",
    "SPK-L,SPK-R",

    "hw:my-sound-card,0",
    "my-sound-card Headset Jack",
    "gpio",
    "0",
    "CAPTURE",

    "hw:my-sound-card,0",
    "MIC-L,MIC-R",
    "-10",

    "hw:my-sound-card,2",
    "HDMI",
  };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = ARRAY_SIZE(devices);

  while (ids[i]) {
    snd_use_case_get_value[ids[i]] = values[i];
    i++;
  }

  sections = ucm_get_sections(mgr);
  ASSERT_NE(sections, (struct ucm_section*)NULL);
  DL_FOREACH(sections, section) {
    section_count++;
  }
  EXPECT_EQ(section_count, ARRAY_SIZE(devices) / 2);

  // Headphone
  section = sections;
  EXPECT_EQ(0, strcmp(section->name, "Headphone"));
  EXPECT_EQ(0, section->dev_idx);
  EXPECT_EQ(CRAS_STREAM_OUTPUT, section->dir);
  EXPECT_EQ(0, strcmp(section->jack_name, values[1]));
  EXPECT_EQ(0, strcmp(section->jack_type, values[2]));
  EXPECT_EQ(NULL, section->mixer_name);
  ASSERT_NE((struct mixer_name*)NULL, section->coupled);
  m_name = section->coupled;
  EXPECT_EQ(0, strcmp(m_name->name, "HP-L"));
  m_name = m_name->next;
  EXPECT_EQ(0, strcmp(m_name->name, "HP-R"));
  EXPECT_EQ(NULL, m_name->next);
  EXPECT_EQ(2, section->jack_switch);

  // Speaker
  section = section->next;
  EXPECT_EQ(0, strcmp(section->name, "Speaker"));
  EXPECT_EQ(0, section->dev_idx);
  EXPECT_EQ(CRAS_STREAM_OUTPUT, section->dir);
  EXPECT_EQ(NULL, section->jack_name);
  EXPECT_EQ(NULL, section->jack_type);
  EXPECT_EQ(-1, section->jack_switch);
  EXPECT_EQ(NULL, section->mixer_name);
  ASSERT_NE((struct mixer_name*)NULL, section->coupled);
  m_name = section->coupled;
  EXPECT_EQ(0, strcmp(m_name->name, "SPK-L"));
  m_name = m_name->next;
  EXPECT_EQ(0, strcmp(m_name->name, "SPK-R"));
  EXPECT_EQ(NULL, m_name->next);

  // Mic
  section = section->next;
  EXPECT_EQ(0, strcmp(section->name, "Mic"));
  EXPECT_EQ(0, section->dev_idx);
  EXPECT_EQ(CRAS_STREAM_INPUT, section->dir);
  EXPECT_EQ(0, strcmp(section->jack_name, values[1]));
  EXPECT_EQ(0, strcmp(section->jack_type, values[2]));
  EXPECT_EQ(0, section->jack_switch);
  ASSERT_NE((const char *)NULL, section->mixer_name);
  EXPECT_EQ(0, strcmp(section->mixer_name, "CAPTURE"));
  EXPECT_EQ(NULL, section->coupled);

  // Internal Mic
  section = section->next;
  EXPECT_EQ(0, strcmp(section->name, "Internal Mic"));
  EXPECT_EQ(0, section->dev_idx);
  EXPECT_EQ(CRAS_STREAM_INPUT, section->dir);
  EXPECT_EQ(NULL, section->jack_name);
  EXPECT_EQ(NULL, section->jack_type);
  EXPECT_EQ(-1, section->jack_switch);
  EXPECT_EQ(NULL, section->mixer_name);
  ASSERT_NE((struct mixer_name*)NULL, section->coupled);
  m_name = section->coupled;
  EXPECT_EQ(0, strcmp(m_name->name, "MIC-L"));
  m_name = m_name->next;
  EXPECT_EQ(0, strcmp(m_name->name, "MIC-R"));

  // HDMI
  section = section->next;
  EXPECT_EQ(0, strcmp(section->name, "HDMI"));
  EXPECT_EQ(2, section->dev_idx);
  EXPECT_EQ(CRAS_STREAM_OUTPUT, section->dir);
  EXPECT_EQ(NULL, section->jack_name);
  EXPECT_EQ(NULL, section->jack_type);
  EXPECT_EQ(-1, section->jack_switch);
  ASSERT_NE((const char *)NULL, section->mixer_name);
  EXPECT_EQ(0, strcmp(section->mixer_name, "HDMI"));

  EXPECT_EQ(NULL, section->next);
  ucm_section_free_list(sections);
}

TEST(AlsaUcm, GetSectionsMissingPCM) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct ucm_section* sections;
  int i = 0;
  const char *devices[] = { "Headphone", "The headphones jack." };
  const char* ids[] = {
    "=JackName/Headphone/HiFi",
    "=CoupledMixers/Headphone/HiFi",
    NULL
  };
  const char* values[] = {
    "my-sound-card Headset Jack",
    "HP-L,HP-R",
  };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = ARRAY_SIZE(devices);

  while (ids[i]) {
    snd_use_case_get_value[ids[i]] = values[i];
    i++;
  }

  sections = ucm_get_sections(mgr);
  EXPECT_EQ(NULL, sections);
}

TEST(AlsaUcm, GetSectionsBadPCM) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;
  struct ucm_section* sections;
  int i = 0;
  const char *devices[] = { "Headphone", "The headphones jack." };
  const char* ids[] = {
    "=PlaybackPCM/Headphone/HiFi",
    "=JackName/Headphone/HiFi",
    "=CoupledMixers/Headphone/HiFi",
    NULL
  };
  const char* values[] = {
    "hw:my-sound-card:0",
    "my-sound-card Headset Jack",
    "HP-L,HP-R",
  };

  ResetStubData();

  fake_list["_devices/HiFi"] = devices;
  fake_list_size["_devices/HiFi"] = ARRAY_SIZE(devices);

  while (ids[i]) {
    snd_use_case_get_value[ids[i]] = values[i];
    i++;
  }

  sections = ucm_get_sections(mgr);
  EXPECT_EQ(NULL, sections);
}

TEST(AlsaUcm, CheckUseCaseVerbs) {
  struct cras_use_case_mgr *mgr = &cras_ucm_mgr;

  /* Verifies the mapping between stream types and verbs are correct. */
  mgr->use_case = CRAS_STREAM_TYPE_DEFAULT;
  EXPECT_EQ(0, strcmp("HiFi", uc_verb(mgr)));
  mgr->use_case = CRAS_STREAM_TYPE_MULTIMEDIA;
  EXPECT_EQ(0, strcmp("Multimedia", uc_verb(mgr)));
  mgr->use_case = CRAS_STREAM_TYPE_VOICE_COMMUNICATION;
  EXPECT_EQ(0, strcmp("Voice Call", uc_verb(mgr)));
  mgr->use_case = CRAS_STREAM_TYPE_SPEECH_RECOGNITION;
  EXPECT_EQ(0, strcmp("Speech", uc_verb(mgr)));
  mgr->use_case = CRAS_STREAM_TYPE_PRO_AUDIO;
  EXPECT_EQ(0, strcmp("Pro Audio", uc_verb(mgr)));
}

TEST(AlsaUcm, GetAvailUseCases) {
  struct cras_use_case_mgr *mgr;
  const char *verbs[] = { "HiFi", "Comment for Verb1",
                          "Voice Call", "Comment for Verb2",
                          "Speech", "Comment for Verb3" };

  ResetStubData();

  fake_list["_verbs"] = verbs;
  fake_list_size["_verbs"] = 6;

  mgr = ucm_create("foo");
  EXPECT_EQ(0x0D, mgr->avail_use_cases);
  ucm_destroy(mgr);
}

TEST(AlsaUcm, SetUseCase) {
  struct cras_use_case_mgr *mgr;
  const char *verbs[] = { "HiFi", "Comment for Verb1",
                          "Voice Call", "Comment for Verb2",
                          "Speech", "Comment for Verb3" };
  int rc;

  ResetStubData();

  fake_list["_verbs"] = verbs;
  fake_list_size["_verbs"] = 6;

  mgr = ucm_create("foo");
  EXPECT_EQ(snd_use_case_set_param[0],
      std::make_pair(std::string("_verb"), std::string("HiFi")));

  rc = ucm_set_use_case(mgr, CRAS_STREAM_TYPE_VOICE_COMMUNICATION);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(mgr->use_case, CRAS_STREAM_TYPE_VOICE_COMMUNICATION);
  EXPECT_EQ(snd_use_case_set_param[1],
      std::make_pair(std::string("_verb"), std::string("Voice Call")));

  /* Request unavailable use case will fail. */
  rc = ucm_set_use_case(mgr, CRAS_STREAM_TYPE_PRO_AUDIO);
  EXPECT_EQ(-1, rc);
  /* cras_use_case_mgr's use case should not be changed. */
  EXPECT_EQ(mgr->use_case, CRAS_STREAM_TYPE_VOICE_COMMUNICATION);
  /* And snd_use_case_set not being called. */
  EXPECT_EQ(2, snd_use_case_set_param.size());

  ucm_destroy(mgr);
}

/* Stubs */

extern "C" {

int snd_use_case_mgr_open(snd_use_case_mgr_t** uc_mgr, const char* card_name) {
  snd_use_case_mgr_open_called++;
  *uc_mgr = snd_use_case_mgr_open_mgr_ptr;
  return snd_use_case_mgr_open_return;
}

int snd_use_case_mgr_close(snd_use_case_mgr_t *uc_mgr) {
  snd_use_case_mgr_close_called++;
  return 0;
}

int snd_use_case_get(snd_use_case_mgr_t* uc_mgr,
                     const char *identifier,
                     const char **value) {
  snd_use_case_get_called++;
  snd_use_case_get_id.push_back(std::string(identifier));
  if (snd_use_case_get_value.find(identifier) == snd_use_case_get_value.end()) {
    *value = NULL;
    return -1;
  }
  *value = strdup(snd_use_case_get_value[identifier].c_str());
  return 0;
}

int snd_use_case_set(snd_use_case_mgr_t* uc_mgr,
                     const char *identifier,
                     const char *value) {
  snd_use_case_set_called++;
  snd_use_case_set_param.push_back(
      std::make_pair(std::string(identifier), std::string(value)));
  return snd_use_case_set_return;
}

int snd_use_case_get_list(snd_use_case_mgr_t *uc_mgr,
                          const char *identifier,
                          const char **list[]) {
  *list = fake_list[identifier];
  return fake_list_size[identifier];
}

int snd_use_case_free_list(const char *list[], int items) {
  snd_use_case_free_list_called++;
  return 0;
}

} /* extern "C" */

}  //  namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  openlog(NULL, LOG_PERROR, LOG_USER);
  return RUN_ALL_TESTS();
}
