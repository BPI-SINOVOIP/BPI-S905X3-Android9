// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <map>
#include <stdio.h>
#include <syslog.h>
#include <vector>

extern "C" {

#include "cras_iodev.h"
#include "cras_shm.h"
#include "cras_system_state.h"
#include "cras_types.h"
#include "cras_alsa_mixer.h"

//  Include C file to test static functions.
#include "cras_alsa_io.c"
}

#define BUFFER_SIZE 8192

//  Data for simulating functions stubbed below.
static int cras_alsa_open_called;
static int cras_iodev_append_stream_ret;
static int cras_alsa_get_avail_frames_ret;
static int cras_alsa_get_avail_frames_avail;
static int cras_alsa_start_called;
static uint8_t *cras_alsa_mmap_begin_buffer;
static size_t cras_alsa_mmap_begin_frames;
static size_t cras_alsa_fill_properties_called;
static size_t alsa_mixer_set_dBFS_called;
static int alsa_mixer_set_dBFS_value;
static const struct mixer_control *alsa_mixer_set_dBFS_output;
static size_t alsa_mixer_set_capture_dBFS_called;
static int alsa_mixer_set_capture_dBFS_value;
static const struct mixer_control *alsa_mixer_set_capture_dBFS_input;
static const struct mixer_control
    *cras_alsa_mixer_get_minimum_capture_gain_mixer_input;
static const struct mixer_control
    *cras_alsa_mixer_get_maximum_capture_gain_mixer_input;
static size_t cras_alsa_mixer_list_outputs_called;
static size_t cras_alsa_mixer_list_inputs_called;
static size_t cras_alsa_mixer_get_control_for_section_called;
static struct mixer_control *
    cras_alsa_mixer_get_control_for_section_return_value;
static size_t sys_get_volume_called;
static size_t sys_get_volume_return_value;
static size_t sys_get_capture_gain_called;
static long sys_get_capture_gain_return_value;
static size_t alsa_mixer_set_mute_called;
static int alsa_mixer_set_mute_value;
static size_t alsa_mixer_get_dB_range_called;
static long alsa_mixer_get_dB_range_value;
static size_t alsa_mixer_get_output_dB_range_called;
static long alsa_mixer_get_output_dB_range_value;
static const struct mixer_control *alsa_mixer_set_mute_output;
static size_t alsa_mixer_set_capture_mute_called;
static int alsa_mixer_set_capture_mute_value;
static const struct mixer_control *alsa_mixer_set_capture_mute_input;
static size_t sys_get_mute_called;
static int sys_get_mute_return_value;
static size_t sys_get_capture_mute_called;
static int sys_get_capture_mute_return_value;
static struct cras_alsa_mixer *fake_mixer = (struct cras_alsa_mixer *)1;
static struct cras_card_config *fake_config = (struct cras_card_config *)2;
static struct mixer_control **cras_alsa_mixer_list_outputs_outputs;
static size_t cras_alsa_mixer_list_outputs_outputs_length;
static struct mixer_control **cras_alsa_mixer_list_inputs_outputs;
static size_t cras_alsa_mixer_list_inputs_outputs_length;
static size_t cras_alsa_mixer_set_output_active_state_called;
static std::vector<struct mixer_control *>
    cras_alsa_mixer_set_output_active_state_outputs;
static std::vector<int> cras_alsa_mixer_set_output_active_state_values;
static cras_audio_format *fake_format;
static size_t sys_set_volume_limits_called;
static size_t sys_set_capture_gain_limits_called;
static size_t cras_alsa_mixer_get_minimum_capture_gain_called;
static size_t cras_alsa_mixer_get_maximum_capture_gain_called;
static struct mixer_control *cras_alsa_jack_get_mixer_output_ret;
static struct mixer_control *cras_alsa_jack_get_mixer_input_ret;
static size_t cras_alsa_mixer_get_output_volume_curve_called;
typedef std::map<const struct mixer_control*, std::string> ControlNameMap;
static ControlNameMap cras_alsa_mixer_get_control_name_values;
static size_t cras_alsa_mixer_get_control_name_called;
static size_t cras_alsa_jack_list_create_called;
static size_t cras_alsa_jack_list_find_jacks_by_name_matching_called;
static size_t cras_alsa_jack_list_add_jack_for_section_called;
static struct cras_alsa_jack *
    cras_alsa_jack_list_add_jack_for_section_result_jack;
static size_t cras_alsa_jack_list_destroy_called;
static int cras_alsa_jack_list_has_hctl_jacks_return_val;
static jack_state_change_callback *cras_alsa_jack_list_create_cb;
static void *cras_alsa_jack_list_create_cb_data;
static char test_card_name[] = "TestCard";
static char test_dev_name[] = "TestDev";
static char test_dev_id[] = "TestDevId";
static size_t cras_iodev_add_node_called;
static struct cras_ionode *cras_iodev_set_node_attr_ionode;
static size_t cras_iodev_set_node_attr_called;
static enum ionode_attr cras_iodev_set_node_attr_attr;
static int cras_iodev_set_node_attr_value;
static unsigned cras_alsa_jack_enable_ucm_called;
static unsigned ucm_set_enabled_called;
static size_t cras_iodev_update_dsp_called;
static const char *cras_iodev_update_dsp_name;
static size_t ucm_get_dsp_name_default_called;
static const char *ucm_get_dsp_name_default_value;
static size_t cras_alsa_jack_get_dsp_name_called;
static const char *cras_alsa_jack_get_dsp_name_value;
static size_t cras_iodev_free_resources_called;
static size_t cras_alsa_jack_update_node_type_called;
static int ucm_swap_mode_exists_ret_value;
static int ucm_enable_swap_mode_ret_value;
static size_t ucm_enable_swap_mode_called;
static int is_utf8_string_ret_value;
static char *cras_alsa_jack_update_monitor_fake_name = 0;
static int cras_alsa_jack_get_name_called;
static const char *cras_alsa_jack_get_name_ret_value = 0;
static char default_jack_name[] = "Something Jack";
static int auto_unplug_input_node_ret = 0;
static int auto_unplug_output_node_ret = 0;
static int ucm_get_max_software_gain_called;
static int ucm_get_max_software_gain_ret_value;
static long ucm_get_max_software_gain_value;
static long cras_system_set_capture_gain_limits_set_value[2];
static long cras_alsa_mixer_get_minimum_capture_gain_ret_value;
static long cras_alsa_mixer_get_maximum_capture_gain_ret_value;
static snd_pcm_state_t snd_pcm_state_ret;
static int cras_alsa_attempt_resume_called;
static snd_hctl_t *fake_hctl = (snd_hctl_t *)2;
static size_t ucm_get_dma_period_for_dev_called;
static unsigned int ucm_get_dma_period_for_dev_ret;
static int cras_card_config_get_volume_curve_for_control_called;
typedef std::map<std::string, struct cras_volume_curve *> VolCurveMap;
static VolCurveMap cras_card_config_get_volume_curve_vals;
static int cras_alsa_mmap_get_whole_buffer_called;
static int cras_iodev_fill_odev_zeros_called;
static unsigned int cras_iodev_fill_odev_zeros_frames;
static int cras_iodev_frames_queued_ret;
static int cras_iodev_buffer_avail_ret;
static int cras_alsa_resume_appl_ptr_called;
static int cras_alsa_resume_appl_ptr_ahead;
static int ucm_get_enable_htimestamp_flag_ret;
static const struct cras_volume_curve *fake_get_dBFS_volume_curve_val;
static int cras_iodev_dsp_set_swap_mode_for_node_called;
static std::map<std::string, long> ucm_get_default_node_gain_values;

void ResetStubData() {
  cras_alsa_open_called = 0;
  cras_iodev_append_stream_ret = 0;
  cras_alsa_get_avail_frames_ret = 0;
  cras_alsa_get_avail_frames_avail = 0;
  cras_alsa_start_called = 0;
  cras_alsa_fill_properties_called = 0;
  sys_get_volume_called = 0;
  sys_get_capture_gain_called = 0;
  alsa_mixer_set_dBFS_called = 0;
  alsa_mixer_set_capture_dBFS_called = 0;
  sys_get_mute_called = 0;
  sys_get_capture_mute_called = 0;
  alsa_mixer_set_mute_called = 0;
  alsa_mixer_get_dB_range_called = 0;
  alsa_mixer_get_output_dB_range_called = 0;
  alsa_mixer_set_capture_mute_called = 0;
  cras_alsa_mixer_get_control_for_section_called = 0;
  cras_alsa_mixer_get_control_for_section_return_value = NULL;
  cras_alsa_mixer_list_outputs_called = 0;
  cras_alsa_mixer_list_outputs_outputs_length = 0;
  cras_alsa_mixer_list_inputs_called = 0;
  cras_alsa_mixer_list_inputs_outputs_length = 0;
  cras_alsa_mixer_set_output_active_state_called = 0;
  cras_alsa_mixer_set_output_active_state_outputs.clear();
  cras_alsa_mixer_set_output_active_state_values.clear();
  sys_set_volume_limits_called = 0;
  sys_set_capture_gain_limits_called = 0;
  sys_get_capture_gain_return_value = 0;
  cras_alsa_mixer_get_minimum_capture_gain_called = 0;
  cras_alsa_mixer_get_maximum_capture_gain_called = 0;
  cras_alsa_mixer_get_output_volume_curve_called = 0;
  cras_alsa_jack_get_mixer_output_ret = NULL;
  cras_alsa_jack_get_mixer_input_ret = NULL;
  cras_alsa_mixer_get_control_name_values.clear();
  cras_alsa_mixer_get_control_name_called = 0;
  cras_alsa_jack_list_create_called = 0;
  cras_alsa_jack_list_find_jacks_by_name_matching_called = 0;
  cras_alsa_jack_list_add_jack_for_section_called = 0;
  cras_alsa_jack_list_add_jack_for_section_result_jack = NULL;
  cras_alsa_jack_list_destroy_called = 0;
  cras_alsa_jack_list_has_hctl_jacks_return_val = 1;
  cras_iodev_add_node_called = 0;
  cras_iodev_set_node_attr_called = 0;
  cras_alsa_jack_enable_ucm_called = 0;
  ucm_set_enabled_called = 0;
  cras_iodev_update_dsp_called = 0;
  cras_iodev_update_dsp_name = 0;
  ucm_get_dsp_name_default_called = 0;
  ucm_get_dsp_name_default_value = NULL;
  cras_alsa_jack_get_dsp_name_called = 0;
  cras_alsa_jack_get_dsp_name_value = NULL;
  cras_iodev_free_resources_called = 0;
  cras_alsa_jack_update_node_type_called = 0;
  ucm_swap_mode_exists_ret_value = 0;
  ucm_enable_swap_mode_ret_value = 0;
  ucm_enable_swap_mode_called = 0;
  is_utf8_string_ret_value = 1;
  cras_alsa_jack_get_name_called = 0;
  cras_alsa_jack_get_name_ret_value = default_jack_name;
  cras_alsa_jack_update_monitor_fake_name = 0;
  ucm_get_max_software_gain_called = 0;
  ucm_get_max_software_gain_ret_value = -1;
  ucm_get_max_software_gain_value = 0;
  cras_card_config_get_volume_curve_for_control_called = 0;
  cras_card_config_get_volume_curve_vals.clear();
  cras_system_set_capture_gain_limits_set_value[0] = -1;
  cras_system_set_capture_gain_limits_set_value[1] = -1;
  cras_alsa_mixer_get_minimum_capture_gain_ret_value = 0;
  cras_alsa_mixer_get_maximum_capture_gain_ret_value = 0;
  snd_pcm_state_ret = SND_PCM_STATE_RUNNING;
  cras_alsa_attempt_resume_called = 0;
  ucm_get_dma_period_for_dev_called = 0;
  ucm_get_dma_period_for_dev_ret = 0;
  cras_alsa_mmap_get_whole_buffer_called = 0;
  cras_iodev_fill_odev_zeros_called = 0;
  cras_iodev_fill_odev_zeros_frames = 0;
  cras_iodev_frames_queued_ret = 0;
  cras_iodev_buffer_avail_ret = 0;
  cras_alsa_resume_appl_ptr_called = 0;
  cras_alsa_resume_appl_ptr_ahead = 0;
  ucm_get_enable_htimestamp_flag_ret = 0;
  fake_get_dBFS_volume_curve_val = NULL;
  cras_iodev_dsp_set_swap_mode_for_node_called = 0;
  ucm_get_default_node_gain_values.clear();
}

static long fake_get_dBFS(const struct cras_volume_curve *curve, size_t volume)
{
  fake_get_dBFS_volume_curve_val = curve;
  return (volume - 100) * 100;
}
static cras_volume_curve default_curve = {
  .get_dBFS = fake_get_dBFS,
};

static struct cras_iodev *alsa_iodev_create_with_default_parameters(
    size_t card_index,
    const char *dev_id,
    enum CRAS_ALSA_CARD_TYPE card_type,
    int is_first,
    struct cras_alsa_mixer *mixer,
    struct cras_card_config *config,
    struct cras_use_case_mgr *ucm,
    enum CRAS_STREAM_DIRECTION direction) {
  return alsa_iodev_create(card_index, test_card_name, 0, test_dev_name,
                           dev_id, card_type, is_first,
                           mixer, config, ucm, fake_hctl,
                           direction, 0, 0, (char *)"123");
}

namespace {

TEST(AlsaIoInit, InitializeInvalidDirection) {
  struct alsa_io *aio;

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, NULL,
      CRAS_NUM_DIRECTIONS);
  ASSERT_EQ(aio, (void *)NULL);
}

TEST(AlsaIoInit, InitializePlayback) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, test_dev_id, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  /* Get volume curve twice for iodev, and default node. */
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_fill_properties_called);
  EXPECT_EQ(1, cras_alsa_mixer_list_outputs_called);
  EXPECT_EQ(0, strncmp(test_card_name,
                       aio->base.info.name,
		       strlen(test_card_name)));
  EXPECT_EQ(0, ucm_get_dsp_name_default_called);
  EXPECT_EQ(NULL, cras_iodev_update_dsp_name);
  ASSERT_NE(reinterpret_cast<const char *>(NULL), aio->dev_name);
  EXPECT_EQ(0, strcmp(test_dev_name, aio->dev_name));
  ASSERT_NE(reinterpret_cast<const char *>(NULL), aio->dev_id);
  EXPECT_EQ(0, strcmp(test_dev_id, aio->dev_id));

  alsa_iodev_destroy((struct cras_iodev *)aio);
  EXPECT_EQ(1, cras_iodev_free_resources_called);
}

TEST(AlsaIoInit, DefaultNodeInternalCard) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  ASSERT_STREQ("(default)", aio->base.active_node->name);
  ASSERT_EQ(1, aio->base.active_node->plugged);
  ASSERT_EQ((void *)no_stream, (void *)aio->base.no_stream);
  ASSERT_EQ((void *)output_should_wake, (void *)aio->base.output_should_wake);
  alsa_iodev_destroy((struct cras_iodev *)aio);

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  ASSERT_STREQ("Speaker", aio->base.active_node->name);
  ASSERT_EQ(1, aio->base.active_node->plugged);
  ASSERT_EQ((void *)no_stream, (void *)aio->base.no_stream);
  ASSERT_EQ((void *)output_should_wake, (void *)aio->base.output_should_wake);
  alsa_iodev_destroy((struct cras_iodev *)aio);

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, NULL,
      CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  /* No more call to get volume curve for input device. */
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  ASSERT_STREQ("(default)", aio->base.active_node->name);
  ASSERT_EQ(1, aio->base.active_node->plugged);
  ASSERT_EQ((void *)no_stream, (void *)aio->base.no_stream);
  ASSERT_EQ((void *)output_should_wake, (void *)aio->base.output_should_wake);
  alsa_iodev_destroy((struct cras_iodev *)aio);

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  ASSERT_STREQ("Internal Mic", aio->base.active_node->name);
  ASSERT_EQ(1, aio->base.active_node->plugged);
  ASSERT_EQ((void *)no_stream, (void *)aio->base.no_stream);
  ASSERT_EQ((void *)output_should_wake, (void *)aio->base.output_should_wake);
  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaIoInit, DefaultNodeUSBCard) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_USB, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  ASSERT_STREQ("(default)", aio->base.active_node->name);
  ASSERT_EQ(1, aio->base.active_node->plugged);
  EXPECT_EQ(1, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(1, cras_iodev_set_node_attr_value);
  alsa_iodev_destroy((struct cras_iodev *)aio);

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_USB, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  ASSERT_STREQ("(default)", aio->base.active_node->name);
  ASSERT_EQ(1, aio->base.active_node->plugged);
  EXPECT_EQ(2, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(1, cras_iodev_set_node_attr_value);
  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaIoInit, OpenPlayback) {
  struct cras_iodev *iodev;
  struct cras_audio_format format;
  struct alsa_io *aio;

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 0,
                                                    fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  aio = (struct alsa_io *)iodev;
  format.frame_rate = 48000;
  cras_iodev_set_format(iodev, &format);

  // Test that these flags are cleared after open_dev.
  aio->is_free_running = 1;
  aio->filled_zeros_for_draining = 512;
  iodev->open_dev(iodev);
  EXPECT_EQ(1, cras_alsa_open_called);
  EXPECT_EQ(1, sys_set_volume_limits_called);
  EXPECT_EQ(1, alsa_mixer_set_dBFS_called);
  EXPECT_EQ(0, cras_alsa_start_called);
  EXPECT_EQ(0, cras_iodev_set_node_attr_called);
  EXPECT_EQ(0, aio->is_free_running);
  EXPECT_EQ(0, aio->filled_zeros_for_draining);
  EXPECT_EQ(SEVERE_UNDERRUN_MS * format.frame_rate / 1000,
            aio->severe_underrun_frames);

  alsa_iodev_destroy(iodev);
  free(fake_format);
}

TEST(AlsaIoInit, UsbCardAutoPlug) {
  struct cras_iodev *iodev;

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(0, cras_iodev_set_node_attr_called);
  alsa_iodev_destroy(iodev);

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL, ALSA_CARD_TYPE_USB,
                                                    0, fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(0, cras_iodev_set_node_attr_called);
  alsa_iodev_destroy(iodev);

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL, ALSA_CARD_TYPE_USB,
                                                    1, fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  // Should assume USB devs are plugged when they appear.
  EXPECT_EQ(1, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(1, cras_iodev_set_node_attr_value);
  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, UsbCardUseSoftwareVolume) {
  struct cras_iodev *iodev;

  alsa_mixer_get_dB_range_value = 1000;
  alsa_mixer_get_output_dB_range_value = 1000;
  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL, ALSA_CARD_TYPE_USB,
                                                    1, fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(1, alsa_mixer_get_dB_range_called);
  EXPECT_EQ(1, alsa_mixer_get_output_dB_range_called);
  EXPECT_EQ(1, iodev->active_node->software_volume_needed);
  alsa_iodev_destroy(iodev);

  alsa_mixer_get_dB_range_value = 3000;
  alsa_mixer_get_output_dB_range_value = 2000;
  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL, ALSA_CARD_TYPE_USB,
                                                    1, fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(1, alsa_mixer_get_dB_range_called);
  EXPECT_EQ(1, alsa_mixer_get_output_dB_range_called);
  EXPECT_EQ(0, iodev->active_node->software_volume_needed);
  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, UseSoftwareGain) {
  struct cras_iodev *iodev;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;

  /* Meet the requirements of using software gain. */
  ResetStubData();
  ucm_get_max_software_gain_ret_value = 0;
  ucm_get_max_software_gain_value = 2000;
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(1, iodev->active_node->software_volume_needed);
  EXPECT_EQ(2000, iodev->active_node->max_software_gain);
  ASSERT_EQ(1, sys_set_capture_gain_limits_called);
  /* The gain range is [DEFAULT_MIN_CAPTURE_GAIN, maximum softare gain]. */
  ASSERT_EQ(cras_system_set_capture_gain_limits_set_value[0],
      DEFAULT_MIN_CAPTURE_GAIN);
  ASSERT_EQ(cras_system_set_capture_gain_limits_set_value[1], 2000);

  /* MaxSoftwareGain is not specified in UCM */
  ResetStubData();
  ucm_get_max_software_gain_ret_value = 1;
  ucm_get_max_software_gain_value = 1;
  cras_alsa_mixer_get_minimum_capture_gain_ret_value = -500;
  cras_alsa_mixer_get_maximum_capture_gain_ret_value = 500;
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(0, iodev->active_node->software_volume_needed);
  EXPECT_EQ(0, iodev->active_node->max_software_gain);
  ASSERT_EQ(1, sys_set_capture_gain_limits_called);
  /* The gain range is reported by controls. */
  ASSERT_EQ(cras_system_set_capture_gain_limits_set_value[0], -500);
  ASSERT_EQ(cras_system_set_capture_gain_limits_set_value[1], 500);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, SoftwareGainWithDefaultNodeGain) {
  struct cras_iodev *iodev;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  long system_gain = 500;
  long default_node_gain = -1000;

  ResetStubData();

  // Use software gain.
  ucm_get_max_software_gain_ret_value = 0;
  ucm_get_max_software_gain_value = 2000;

  // Set default node gain to -1000 dBm.
  ucm_get_default_node_gain_values["Internal Mic"] = default_node_gain;

  // Assume this is the first device so it gets internal mic node name.
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));

  // Gain on node is 300 dBm.
  iodev->active_node->capture_gain = default_node_gain;

  // cras_iodev will call cras_iodev_adjust_active_node_gain to get gain for
  // software gain.
  ASSERT_EQ(system_gain + default_node_gain,
            cras_iodev_adjust_active_node_gain(iodev, system_gain));

  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, RouteBasedOnJackCallback) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_NE(aio, (void *)NULL);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_fill_properties_called);
  EXPECT_EQ(1, cras_alsa_mixer_list_outputs_called);
  EXPECT_EQ(1, cras_alsa_jack_list_create_called);
  EXPECT_EQ(1, cras_alsa_jack_list_find_jacks_by_name_matching_called);
  EXPECT_EQ(0, cras_alsa_jack_list_add_jack_for_section_called);

  cras_alsa_jack_list_create_cb(NULL, 1, cras_alsa_jack_list_create_cb_data);
  EXPECT_EQ(1, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(1, cras_iodev_set_node_attr_value);
  cras_alsa_jack_list_create_cb(NULL, 0, cras_alsa_jack_list_create_cb_data);
  EXPECT_EQ(2, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(0, cras_iodev_set_node_attr_value);

  alsa_iodev_destroy((struct cras_iodev *)aio);
  EXPECT_EQ(1, cras_alsa_jack_list_destroy_called);
}

TEST(AlsaIoInit, RouteBasedOnInputJackCallback) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, NULL,
      CRAS_STREAM_INPUT);
  ASSERT_NE(aio, (void *)NULL);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));

  EXPECT_EQ(SND_PCM_STREAM_CAPTURE, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_fill_properties_called);
  EXPECT_EQ(1, cras_alsa_jack_list_create_called);
  EXPECT_EQ(1, cras_alsa_jack_list_find_jacks_by_name_matching_called);
  EXPECT_EQ(0, cras_alsa_jack_list_add_jack_for_section_called);

  cras_alsa_jack_list_create_cb(NULL, 1, cras_alsa_jack_list_create_cb_data);
  EXPECT_EQ(1, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(1, cras_iodev_set_node_attr_value);
  cras_alsa_jack_list_create_cb(NULL, 0, cras_alsa_jack_list_create_cb_data);
  EXPECT_EQ(2, cras_iodev_set_node_attr_called);
  EXPECT_EQ(IONODE_ATTR_PLUGGED, cras_iodev_set_node_attr_attr);
  EXPECT_EQ(0, cras_iodev_set_node_attr_value);

  alsa_iodev_destroy((struct cras_iodev *)aio);
  EXPECT_EQ(1, cras_alsa_jack_list_destroy_called);
}

TEST(AlsaIoInit, InitializeCapture) {
  struct alsa_io *aio;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_INPUT);
  ASSERT_NE(aio, (void *)NULL);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));

  EXPECT_EQ(SND_PCM_STREAM_CAPTURE, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_fill_properties_called);
  EXPECT_EQ(1, cras_alsa_mixer_list_inputs_called);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaIoInit, OpenCapture) {
  struct cras_iodev *iodev;
  struct cras_audio_format format;
  struct alsa_io *aio;

  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 0,
                                                    fake_mixer, fake_config,
                                                    NULL, CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));

  aio = (struct alsa_io *)iodev;
  format.frame_rate = 48000;
  cras_iodev_set_format(iodev, &format);

  ResetStubData();
  iodev->open_dev(iodev);
  EXPECT_EQ(1, cras_alsa_open_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_minimum_capture_gain_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_maximum_capture_gain_called);
  EXPECT_EQ(1, sys_set_capture_gain_limits_called);
  EXPECT_EQ(1, sys_get_capture_gain_called);
  EXPECT_EQ(1, alsa_mixer_set_capture_dBFS_called);
  EXPECT_EQ(1, sys_get_capture_mute_called);
  EXPECT_EQ(1, alsa_mixer_set_capture_mute_called);
  EXPECT_EQ(1, cras_alsa_start_called);
  EXPECT_EQ(SEVERE_UNDERRUN_MS * format.frame_rate / 1000,
            aio->severe_underrun_frames);

  alsa_iodev_destroy(iodev);
  free(fake_format);
}

TEST(AlsaIoInit, OpenCaptureSetCaptureGainWithDefaultNodeGain) {
  struct cras_iodev *iodev;
  struct cras_audio_format format;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  long system_gain = 2000;
  long default_node_gain = -1000;

  ResetStubData();
  // Set default node gain to -1000 dBm.
  ucm_get_default_node_gain_values["Internal Mic"] = default_node_gain;

  // Assume this is the first device so it gets internal mic node name.
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));

  cras_iodev_set_format(iodev, &format);

  // Check the default node gain is the same as what specified in UCM.
  EXPECT_EQ(default_node_gain, iodev->active_node->capture_gain);
  // System gain is set to 2000 dBm.
  sys_get_capture_gain_return_value = system_gain;

  iodev->open_dev(iodev);
  iodev->close_dev(iodev);

  // Hardware gain is set to 2000 - 1000 dBm.
  EXPECT_EQ(system_gain + default_node_gain, alsa_mixer_set_capture_dBFS_value);

  alsa_iodev_destroy(iodev);
  free(fake_format);
}

TEST(AlsaIoInit, OpenCaptureSetCaptureGainWithSoftwareGain) {
  struct cras_iodev *iodev;
  struct cras_audio_format format;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;

  /* Meet the requirements of using software gain. */
  ResetStubData();
  ucm_get_max_software_gain_ret_value = 0;
  ucm_get_max_software_gain_value = 2000;

  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 0,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));

  cras_iodev_set_format(iodev, &format);

  /* System gain is set to 1000dBm */
  sys_get_capture_gain_return_value = 1000;

  iodev->open_dev(iodev);
  iodev->close_dev(iodev);

  /* Hardware gain is set to 0dB when software gain is used. */
  EXPECT_EQ(0, alsa_mixer_set_capture_dBFS_value);

  /* Test the case where software gain is not needed. */
  iodev->active_node->software_volume_needed = 0;
  iodev->open_dev(iodev);
  iodev->close_dev(iodev);

  /* Hardware gain is set to 1000dBm as got from system capture gain.*/
  EXPECT_EQ(1000, alsa_mixer_set_capture_dBFS_value);

  alsa_iodev_destroy(iodev);
  free(fake_format);
}

TEST(AlsaIoInit, UpdateActiveNode) {
  struct cras_iodev *iodev;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 0,
                                                    fake_mixer, fake_config,
                                                    NULL,
                                                    CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);

  iodev->update_active_node(iodev, 0, 1);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, StartDevice) {
  struct cras_iodev *iodev;
  int rc;

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 0,
                                                    NULL, fake_config, NULL,
                                                    CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);

  // Return right away if it is already running.
  snd_pcm_state_ret = SND_PCM_STATE_RUNNING;
  rc = iodev->start(iodev);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, cras_alsa_start_called);

  // Otherwise, start the device.
  snd_pcm_state_ret = SND_PCM_STATE_SETUP;
  rc = iodev->start(iodev);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_alsa_start_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, ResumeDevice) {
  struct cras_iodev *iodev;
  int rc;

  ResetStubData();
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 0,
                                                    NULL, fake_config, NULL,
                                                    CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init(iodev));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);

  // Attempt to resume if the device is suspended.
  snd_pcm_state_ret = SND_PCM_STATE_SUSPENDED;
  rc = iodev->start(iodev);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_alsa_attempt_resume_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaIoInit, DspNameDefault) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;

  ResetStubData();
  ucm_get_dsp_name_default_value = "hello";
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(1, ucm_get_dsp_name_default_called);
  EXPECT_EQ(1, cras_alsa_jack_get_dsp_name_called);
  EXPECT_STREQ("hello", cras_iodev_update_dsp_name);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaIoInit, DspNameJackOverride) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  const struct cras_alsa_jack *jack = (struct cras_alsa_jack*)4;

  ResetStubData();
  ucm_get_dsp_name_default_value = "default_dsp";
  cras_alsa_jack_get_dsp_name_value = "override_dsp";
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(1, ucm_get_dsp_name_default_called);
  EXPECT_EQ(1, cras_alsa_jack_get_dsp_name_called);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_STREQ("default_dsp", cras_iodev_update_dsp_name);

  // Add the jack node.
  cras_alsa_jack_list_create_cb(jack, 1, cras_alsa_jack_list_create_cb_data);
  EXPECT_EQ(1, ucm_get_dsp_name_default_called);

  // Mark the jack node as active.
  alsa_iodev_set_active_node(&aio->base, aio->base.nodes->next, 1);
  EXPECT_EQ(2, cras_alsa_jack_get_dsp_name_called);
  EXPECT_EQ(2, cras_iodev_update_dsp_called);
  EXPECT_STREQ("override_dsp", cras_iodev_update_dsp_name);

  // Mark the default node as active.
  alsa_iodev_set_active_node(&aio->base, aio->base.nodes, 1);
  EXPECT_EQ(1, ucm_get_dsp_name_default_called);
  EXPECT_EQ(3, cras_alsa_jack_get_dsp_name_called);
  EXPECT_EQ(3, cras_iodev_update_dsp_called);
  EXPECT_STREQ("default_dsp", cras_iodev_update_dsp_name);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaIoInit, NodeTypeOverride) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  const struct cras_alsa_jack *jack = (struct cras_alsa_jack*)4;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  // Add the jack node.
  cras_alsa_jack_list_create_cb(jack, 1, cras_alsa_jack_list_create_cb_data);
  // Verify that cras_alsa_jack_update_node_type is called when an output device
  // is created.
  EXPECT_EQ(1, cras_alsa_jack_update_node_type_called);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaIoInit, SwapMode) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct cras_ionode * const fake_node = (cras_ionode *)4;
  ResetStubData();
  // Stub replies that swap mode does not exist.
  ucm_swap_mode_exists_ret_value = 0;

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));

  aio->base.set_swap_mode_for_node((cras_iodev*)aio, fake_node, 1);
  /* Swap mode is implemented by dsp. */
  EXPECT_EQ(1, cras_iodev_dsp_set_swap_mode_for_node_called);

  // Stub replies that swap mode exists.
  ucm_swap_mode_exists_ret_value = 1;

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  // Enable swap mode.
  aio->base.set_swap_mode_for_node((cras_iodev*)aio, fake_node, 1);

  // Verify that ucm_enable_swap_mode is called when callback to enable
  // swap mode is called.
  EXPECT_EQ(1, ucm_enable_swap_mode_called);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

// Test that system settins aren't touched if no streams active.
TEST(AlsaOutputNode, SystemSettingsWhenInactive) {
  int rc;
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct mixer_control *outputs[2];

  ResetStubData();
  outputs[0] = reinterpret_cast<struct mixer_control *>(3);
  outputs[1] = reinterpret_cast<struct mixer_control *>(4);
  cras_alsa_mixer_list_outputs_outputs = outputs;
  cras_alsa_mixer_list_outputs_outputs_length = ARRAY_SIZE(outputs);
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  /* Two mixer controls calls get volume curve. */
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(1, cras_alsa_mixer_list_outputs_called);

  ResetStubData();
  rc = alsa_iodev_set_active_node((struct cras_iodev *)aio,
                                  aio->base.nodes->next, 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, alsa_mixer_set_mute_called);
  EXPECT_EQ(0, alsa_mixer_set_dBFS_called);
  ASSERT_EQ(2, cras_alsa_mixer_set_output_active_state_called);
  EXPECT_EQ(outputs[0], cras_alsa_mixer_set_output_active_state_outputs[0]);
  EXPECT_EQ(0, cras_alsa_mixer_set_output_active_state_values[0]);
  EXPECT_EQ(outputs[1], cras_alsa_mixer_set_output_active_state_outputs[1]);
  EXPECT_EQ(1, cras_alsa_mixer_set_output_active_state_values[1]);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  // No jack is defined, and UCM is not used.
  EXPECT_EQ(0, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(0, ucm_set_enabled_called);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

//  Test handling of different amounts of outputs.
TEST(AlsaOutputNode, TwoOutputs) {
  int rc;
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct mixer_control *outputs[2];

  ResetStubData();
  outputs[0] = reinterpret_cast<struct mixer_control *>(3);
  outputs[1] = reinterpret_cast<struct mixer_control *>(4);
  cras_alsa_mixer_list_outputs_outputs = outputs;
  cras_alsa_mixer_list_outputs_outputs_length = ARRAY_SIZE(outputs);
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, NULL,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(1, cras_alsa_mixer_list_outputs_called);

  aio->handle = (snd_pcm_t *)0x24;

  ResetStubData();
  rc = alsa_iodev_set_active_node((struct cras_iodev *)aio,
                                  aio->base.nodes->next, 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(2, alsa_mixer_set_mute_called);
  EXPECT_EQ(outputs[1], alsa_mixer_set_mute_output);
  EXPECT_EQ(1, alsa_mixer_set_dBFS_called);
  EXPECT_EQ(outputs[1], alsa_mixer_set_dBFS_output);
  ASSERT_EQ(2, cras_alsa_mixer_set_output_active_state_called);
  EXPECT_EQ(outputs[0], cras_alsa_mixer_set_output_active_state_outputs[0]);
  EXPECT_EQ(0, cras_alsa_mixer_set_output_active_state_values[0]);
  EXPECT_EQ(outputs[1], cras_alsa_mixer_set_output_active_state_outputs[1]);
  EXPECT_EQ(1, cras_alsa_mixer_set_output_active_state_values[1]);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  // No jacks defined, and UCM is not used.
  EXPECT_EQ(0, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(0, ucm_set_enabled_called);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaOutputNode, TwoJacksHeadphoneLineout) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer *)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr *)3;
  struct cras_iodev *iodev;
  struct mixer_control *output;
  struct ucm_section *section;

  ResetStubData();
  output = reinterpret_cast<struct mixer_control *>(3);
  cras_alsa_mixer_get_control_name_values[output] = "Headphone";

  // Create the iodev
  iodev = alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);
  EXPECT_EQ(1, cras_card_config_get_volume_curve_for_control_called);

  // First node 'Headphone'
  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                               "fake-jack", "gpio");
  ucm_section_set_mixer_name(section, "Headphone");
  cras_alsa_jack_list_add_jack_for_section_result_jack =
      reinterpret_cast<struct cras_alsa_jack *>(10);
  cras_alsa_mixer_get_control_for_section_return_value = output;
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  ucm_section_free_list(section);

  // Second node 'Line Out'
  section = ucm_section_create("Line Out", 0, CRAS_STREAM_OUTPUT,
                               "fake-jack", "gpio");
  ucm_section_set_mixer_name(section, "Headphone");
  cras_alsa_jack_list_add_jack_for_section_result_jack =
      reinterpret_cast<struct cras_alsa_jack *>(20);
  cras_alsa_mixer_get_control_for_section_return_value = output;
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  EXPECT_EQ(7, cras_card_config_get_volume_curve_for_control_called);
  ucm_section_free_list(section);

  // Both nodes are associated with the same mixer output. Different jack plug
  // report should trigger different node attribute change.
  cras_alsa_jack_get_mixer_output_ret = output;
  jack_output_plug_event(reinterpret_cast<struct cras_alsa_jack *>(10), 0, aio);
  EXPECT_STREQ(cras_iodev_set_node_attr_ionode->name, "Headphone");

  jack_output_plug_event(reinterpret_cast<struct cras_alsa_jack *>(20), 0, aio);
  EXPECT_STREQ(cras_iodev_set_node_attr_ionode->name, "Line Out");

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, OutputsFromUCM) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct cras_iodev *iodev;
  static const char *jack_name = "TestCard - Headset Jack";
  struct mixer_control *outputs[2];
  int rc;
  struct ucm_section *section;

  ResetStubData();
  outputs[0] = reinterpret_cast<struct mixer_control *>(3);
  outputs[1] = reinterpret_cast<struct mixer_control *>(4);
  cras_alsa_mixer_list_outputs_outputs = outputs;
  cras_alsa_mixer_list_outputs_outputs_length = ARRAY_SIZE(outputs);
  cras_alsa_mixer_get_control_name_values[outputs[0]] = INTERNAL_SPEAKER;
  cras_alsa_mixer_get_control_name_values[outputs[1]] = "Headphone";
  ucm_get_dma_period_for_dev_ret = 1000;

  // Create the IO device.
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_OUTPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);
  EXPECT_EQ(1, cras_card_config_get_volume_curve_for_control_called);

  // First node.
  section = ucm_section_create(INTERNAL_SPEAKER, 0, CRAS_STREAM_OUTPUT,
                               NULL, NULL);
  ucm_section_set_mixer_name(section, INTERNAL_SPEAKER);
  cras_alsa_jack_list_add_jack_for_section_result_jack =
      reinterpret_cast<struct cras_alsa_jack *>(1);
  cras_alsa_mixer_get_control_for_section_return_value = outputs[0];
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  ucm_section_free_list(section);
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);

  // Add a second node (will use the same iodev).
  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                               jack_name, "hctl");
  ucm_section_add_coupled(section, "HP-L", MIXER_NAME_VOLUME);
  ucm_section_add_coupled(section, "HP-R", MIXER_NAME_VOLUME);
  cras_alsa_jack_list_add_jack_for_section_result_jack = NULL;
  cras_alsa_mixer_get_control_for_section_return_value = outputs[1];
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  ucm_section_free_list(section);
  /* New nodes creation calls get volume curve once, NULL jack doesn't make
   * more calls. */
  EXPECT_EQ(5, cras_card_config_get_volume_curve_for_control_called);

  // Jack plug of an unkonwn device should do nothing.
  cras_alsa_jack_get_mixer_output_ret = NULL;
  cras_alsa_jack_get_name_ret_value = "Some other jack";
  jack_output_plug_event(reinterpret_cast<struct cras_alsa_jack *>(4), 0, aio);
  EXPECT_EQ(0, cras_iodev_set_node_attr_called);

  // Complete initialization, and make first node active.
  alsa_iodev_ucm_complete_init(iodev);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(2, cras_alsa_jack_list_add_jack_for_section_called);
  EXPECT_EQ(2, cras_alsa_mixer_get_control_for_section_called);
  EXPECT_EQ(1, ucm_get_dma_period_for_dev_called);
  EXPECT_EQ(ucm_get_dma_period_for_dev_ret, aio->dma_period_set_microsecs);

  aio->handle = (snd_pcm_t *)0x24;

  ResetStubData();
  rc = alsa_iodev_set_active_node(iodev, aio->base.nodes->next, 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(2, alsa_mixer_set_mute_called);
  EXPECT_EQ(outputs[1], alsa_mixer_set_mute_output);
  EXPECT_EQ(1, alsa_mixer_set_dBFS_called);
  EXPECT_EQ(outputs[1], alsa_mixer_set_dBFS_output);
  ASSERT_EQ(2, cras_alsa_mixer_set_output_active_state_called);
  EXPECT_EQ(outputs[0], cras_alsa_mixer_set_output_active_state_outputs[0]);
  EXPECT_EQ(0, cras_alsa_mixer_set_output_active_state_values[0]);
  EXPECT_EQ(outputs[1], cras_alsa_mixer_set_output_active_state_outputs[1]);
  EXPECT_EQ(1, cras_alsa_mixer_set_output_active_state_values[1]);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_EQ(1, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(1, ucm_set_enabled_called);

  // Simulate jack plug event.
  cras_alsa_jack_get_mixer_output_ret = outputs[1];
  cras_alsa_jack_get_name_ret_value = jack_name;
  jack_output_plug_event(reinterpret_cast<struct cras_alsa_jack *>(4), 0, aio);
  EXPECT_EQ(1, cras_iodev_set_node_attr_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, OutputNoControlsUCM) {
  struct alsa_io *aio;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct cras_iodev *iodev;
  struct ucm_section *section;

  ResetStubData();

  // Create the IO device.
  iodev = alsa_iodev_create_with_default_parameters(1, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_OUTPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);
  EXPECT_EQ(1, cras_card_config_get_volume_curve_for_control_called);

  // Node without controls or jacks.
  section = ucm_section_create(INTERNAL_SPEAKER, 1, CRAS_STREAM_OUTPUT,
                               NULL, NULL);
  // Device index doesn't match.
  EXPECT_EQ(-22, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  section->dev_idx = 0;
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_control_for_section_called);
  EXPECT_EQ(1, cras_iodev_add_node_called);
  ucm_section_free_list(section);

  // Complete initialization, and make first node active.
  alsa_iodev_ucm_complete_init(iodev);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_mixer_get_control_name_called);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_EQ(0, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(1, ucm_set_enabled_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, OutputFromJackUCM) {
  struct alsa_io *aio;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct cras_iodev *iodev;
  static const char *jack_name = "TestCard - Headset Jack";
  struct ucm_section *section;

  ResetStubData();

  // Create the IO device.
  iodev = alsa_iodev_create_with_default_parameters(1, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_OUTPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);
  EXPECT_EQ(1, cras_card_config_get_volume_curve_for_control_called);

  // Node without controls or jacks.
  cras_alsa_jack_list_add_jack_for_section_result_jack =
    reinterpret_cast<struct cras_alsa_jack *>(1);
  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
      jack_name, "hctl");
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  EXPECT_EQ(4, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_control_for_section_called);
  EXPECT_EQ(1, cras_iodev_add_node_called);
  EXPECT_EQ(1, cras_alsa_jack_list_add_jack_for_section_called);
  ucm_section_free_list(section);

  // Complete initialization, and make first node active.
  alsa_iodev_ucm_complete_init(iodev);
  EXPECT_EQ(SND_PCM_STREAM_PLAYBACK, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_mixer_get_control_name_called);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_EQ(1, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(0, ucm_set_enabled_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, InputsFromUCM) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct mixer_control *inputs[2];
  struct cras_iodev *iodev;
  static const char *jack_name = "TestCard - Headset Jack";
  int rc;
  struct ucm_section *section;

  ResetStubData();
  inputs[0] = reinterpret_cast<struct mixer_control *>(3);
  inputs[1] = reinterpret_cast<struct mixer_control *>(4);
  cras_alsa_mixer_list_inputs_outputs = inputs;
  cras_alsa_mixer_list_inputs_outputs_length = ARRAY_SIZE(inputs);
  cras_alsa_mixer_get_control_name_values[inputs[0]] = "Internal Mic";
  cras_alsa_mixer_get_control_name_values[inputs[1]] = "Mic";

  // Create the IO device.
  iodev = alsa_iodev_create_with_default_parameters(0, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);

  // First node.
  cras_alsa_mixer_get_control_for_section_return_value = inputs[0];
  ucm_get_max_software_gain_ret_value = -1;
  section = ucm_section_create(INTERNAL_MICROPHONE, 0, CRAS_STREAM_INPUT,
                               NULL, NULL);
  ucm_section_add_coupled(section, "MIC-L", MIXER_NAME_VOLUME);
  ucm_section_add_coupled(section, "MIC-R", MIXER_NAME_VOLUME);
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  ucm_section_free_list(section);

  // Add a second node (will use the same iodev).
  cras_alsa_mixer_get_control_name_called = 0;
  ucm_get_max_software_gain_ret_value = 0;
  ucm_get_max_software_gain_value = 2000;
  cras_alsa_jack_list_add_jack_for_section_result_jack =
      reinterpret_cast<struct cras_alsa_jack *>(1);
  cras_alsa_mixer_get_control_for_section_return_value = inputs[1];
  section = ucm_section_create("Mic", 0, CRAS_STREAM_INPUT, jack_name, "hctl");
  ucm_section_set_mixer_name(section, "Mic");
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  ucm_section_free_list(section);

  // Jack plug of an unkonwn device should do nothing.
  cras_alsa_jack_get_mixer_input_ret = NULL;
  cras_alsa_jack_get_name_ret_value = "Some other jack";
  jack_input_plug_event(reinterpret_cast<struct cras_alsa_jack *>(4), 0, aio);
  EXPECT_EQ(0, cras_iodev_set_node_attr_called);

  // Simulate jack plug event.
  cras_alsa_jack_get_mixer_input_ret = inputs[1];
  cras_alsa_jack_get_name_ret_value = jack_name;
  jack_input_plug_event(reinterpret_cast<struct cras_alsa_jack *>(4), 0, aio);
  EXPECT_EQ(1, cras_iodev_set_node_attr_called);

  // Complete initialization, and make first node active.
  alsa_iodev_ucm_complete_init(iodev);
  EXPECT_EQ(SND_PCM_STREAM_CAPTURE, aio->alsa_stream);
  EXPECT_EQ(2, cras_alsa_jack_list_add_jack_for_section_called);
  EXPECT_EQ(2, cras_alsa_mixer_get_control_for_section_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_control_name_called);
  EXPECT_EQ(1, sys_set_capture_gain_limits_called);
  EXPECT_EQ(2, cras_iodev_add_node_called);
  EXPECT_EQ(2, ucm_get_dma_period_for_dev_called);
  EXPECT_EQ(0, aio->dma_period_set_microsecs);

  aio->handle = (snd_pcm_t *)0x24;

  ResetStubData();
  rc = alsa_iodev_set_active_node(iodev, aio->base.nodes->next, 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, alsa_mixer_set_capture_dBFS_called);
  EXPECT_EQ(inputs[1], alsa_mixer_set_capture_dBFS_input);
  EXPECT_EQ(0, alsa_mixer_set_capture_dBFS_value);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_EQ(1, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(1, ucm_set_enabled_called);
  EXPECT_EQ(1, sys_set_capture_gain_limits_called);
  EXPECT_EQ(1, alsa_mixer_set_capture_mute_called);
  EXPECT_EQ(1, iodev->active_node->software_volume_needed);
  EXPECT_EQ(2000, iodev->active_node->max_software_gain);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, InputNoControlsUCM) {
  struct alsa_io *aio;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct cras_iodev *iodev;
  struct ucm_section *section;

  ResetStubData();

  // Create the IO device.
  iodev = alsa_iodev_create_with_default_parameters(1, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);

  // Node without controls or jacks.
  section = ucm_section_create(INTERNAL_MICROPHONE, 1, CRAS_STREAM_INPUT,
                               NULL, NULL);
  // Device index doesn't match.
  EXPECT_EQ(-22, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  section->dev_idx = 0;
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  EXPECT_EQ(1, cras_alsa_jack_list_add_jack_for_section_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_control_for_section_called);
  EXPECT_EQ(0, cras_alsa_mixer_get_control_name_called);
  EXPECT_EQ(1, cras_iodev_add_node_called);
  ucm_section_free_list(section);

  // Complete initialization, and make first node active.
  alsa_iodev_ucm_complete_init(iodev);
  EXPECT_EQ(SND_PCM_STREAM_CAPTURE, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_mixer_get_control_name_called);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_EQ(0, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(1, ucm_set_enabled_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, InputFromJackUCM) {
  struct alsa_io *aio;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct cras_iodev *iodev;
  static const char *jack_name = "TestCard - Headset Jack";
  struct ucm_section *section;

  ResetStubData();

  // Create the IO device.
  iodev = alsa_iodev_create_with_default_parameters(1, NULL,
                                                    ALSA_CARD_TYPE_INTERNAL, 1,
                                                    fake_mixer, fake_config,
                                                    fake_ucm,
                                                    CRAS_STREAM_INPUT);
  ASSERT_NE(iodev, (void *)NULL);
  aio = reinterpret_cast<struct alsa_io *>(iodev);

  // Node without controls or jacks.
  cras_alsa_jack_list_add_jack_for_section_result_jack =
      reinterpret_cast<struct cras_alsa_jack *>(1);
  section = ucm_section_create("Mic", 0, CRAS_STREAM_INPUT, jack_name, "hctl");
  ASSERT_EQ(0, alsa_iodev_ucm_add_nodes_and_jacks(iodev, section));
  EXPECT_EQ(1, cras_alsa_mixer_get_control_for_section_called);
  EXPECT_EQ(1, cras_iodev_add_node_called);
  EXPECT_EQ(1, cras_alsa_jack_list_add_jack_for_section_called);
  ucm_section_free_list(section);

  // Complete initialization, and make first node active.
  alsa_iodev_ucm_complete_init(iodev);
  EXPECT_EQ(SND_PCM_STREAM_CAPTURE, aio->alsa_stream);
  EXPECT_EQ(0, cras_alsa_mixer_get_control_name_called);
  EXPECT_EQ(1, cras_iodev_update_dsp_called);
  EXPECT_EQ(1, cras_alsa_jack_enable_ucm_called);
  EXPECT_EQ(0, ucm_set_enabled_called);

  alsa_iodev_destroy(iodev);
}

TEST(AlsaOutputNode, AutoUnplugOutputNode) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct mixer_control *outputs[2];
  const struct cras_alsa_jack *jack = (struct cras_alsa_jack*)4;

  ResetStubData();
  outputs[0] = reinterpret_cast<struct mixer_control *>(5);
  outputs[1] = reinterpret_cast<struct mixer_control *>(6);

  cras_alsa_mixer_list_outputs_outputs = outputs;
  cras_alsa_mixer_list_outputs_outputs_length = ARRAY_SIZE(outputs);

  cras_alsa_mixer_get_control_name_values[outputs[0]] = INTERNAL_SPEAKER;
  cras_alsa_mixer_get_control_name_values[outputs[1]] = "Headphone";
  auto_unplug_output_node_ret = 1;

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);

  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(3, cras_card_config_get_volume_curve_for_control_called);
  EXPECT_EQ(1, cras_alsa_mixer_list_outputs_called);
  EXPECT_EQ(2, cras_alsa_mixer_get_control_name_called);

  // Assert that the the internal speaker is plugged and other nodes aren't.
  ASSERT_NE(aio->base.nodes, (void *)NULL);
  EXPECT_EQ(aio->base.nodes->plugged, 1);
  ASSERT_NE(aio->base.nodes->next, (void *)NULL);
  EXPECT_EQ(aio->base.nodes->next->plugged, 0);

  // Plug headphone jack
  cras_alsa_jack_get_name_ret_value = "Headphone Jack";
  is_utf8_string_ret_value = 1;
  cras_alsa_jack_get_mixer_output_ret = outputs[1];
  cras_alsa_jack_list_create_cb(jack, 1, cras_alsa_jack_list_create_cb_data);

  // Assert internal speaker is auto unplugged
  EXPECT_EQ(aio->base.nodes->plugged, 0);
  EXPECT_EQ(aio->base.nodes->next->plugged, 1);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaOutputNode, AutoUnplugInputNode) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  struct mixer_control *inputs[2];
  const struct cras_alsa_jack *jack = (struct cras_alsa_jack*)4;

  ResetStubData();
  inputs[0] = reinterpret_cast<struct mixer_control *>(5);
  inputs[1] = reinterpret_cast<struct mixer_control *>(6);

  cras_alsa_mixer_list_inputs_outputs = inputs;
  cras_alsa_mixer_list_inputs_outputs_length = ARRAY_SIZE(inputs);

  cras_alsa_mixer_get_control_name_values[inputs[0]] = INTERNAL_MICROPHONE;
  cras_alsa_mixer_get_control_name_values[inputs[1]] = "Mic";
  auto_unplug_input_node_ret = 1;

  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 1, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_INPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));
  EXPECT_EQ(1, cras_alsa_mixer_list_inputs_called);
  EXPECT_EQ(2, cras_alsa_mixer_get_control_name_called);

  // Assert that the the internal speaker is plugged and other nodes aren't.
  ASSERT_NE(aio->base.nodes, (void *)NULL);
  EXPECT_EQ(aio->base.nodes->plugged, 1);
  ASSERT_NE(aio->base.nodes->next, (void *)NULL);
  EXPECT_EQ(aio->base.nodes->next->plugged, 0);

  // Plug headphone jack
  cras_alsa_jack_get_name_ret_value = "Mic Jack";
  is_utf8_string_ret_value = 1;
  cras_alsa_jack_get_mixer_input_ret = inputs[1];
  cras_alsa_jack_list_create_cb(jack, 1, cras_alsa_jack_list_create_cb_data);

  // Assert internal speaker is auto unplugged
  EXPECT_EQ(aio->base.nodes->plugged, 0);
  EXPECT_EQ(aio->base.nodes->next->plugged, 1);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

TEST(AlsaInitNode, SetNodeInitialState) {
  struct cras_ionode node;
  struct cras_iodev dev;

  memset(&dev, 0, sizeof(dev));
  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Unknown");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(0, node.plugged_time.tv_sec);
  ASSERT_EQ(CRAS_NODE_TYPE_UNKNOWN, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Speaker");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(1, node.plugged);
  ASSERT_GT(node.plugged_time.tv_sec, 0);
  ASSERT_EQ(CRAS_NODE_TYPE_INTERNAL_SPEAKER, node.type);
  ASSERT_EQ(NODE_POSITION_INTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Internal Mic");
  dev.direction = CRAS_STREAM_INPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(1, node.plugged);
  ASSERT_GT(node.plugged_time.tv_sec, 0);
  ASSERT_EQ(CRAS_NODE_TYPE_MIC, node.type);
  ASSERT_EQ(NODE_POSITION_INTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "HDMI");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(0, node.plugged_time.tv_sec);
  ASSERT_EQ(CRAS_NODE_TYPE_HDMI, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "IEC958");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_HDMI, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "HDMI Jack");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_HDMI, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Something HDMI Jack");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_HDMI, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Headphone");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_HEADPHONE, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Headphone Jack");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_HEADPHONE, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Mic");
  dev.direction = CRAS_STREAM_INPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_MIC, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Front Mic");
  dev.direction = CRAS_STREAM_INPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(1, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_MIC, node.type);
  ASSERT_EQ(NODE_POSITION_FRONT, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Rear Mic");
  dev.direction = CRAS_STREAM_INPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(1, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_MIC, node.type);
  ASSERT_EQ(NODE_POSITION_REAR, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Mic Jack");
  dev.direction = CRAS_STREAM_INPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_MIC, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Unknown");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_USB);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_USB, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  dev.direction = CRAS_STREAM_INPUT;
  strcpy(node.name, "DAISY-I2S Mic Jack");
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(0, node.plugged);
  ASSERT_EQ(CRAS_NODE_TYPE_MIC, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Speaker");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_USB);
  ASSERT_EQ(1, node.plugged);
  ASSERT_GT(node.plugged_time.tv_sec, 0);
  ASSERT_EQ(CRAS_NODE_TYPE_USB, node.type);
  ASSERT_EQ(NODE_POSITION_EXTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Haptic");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(1, node.plugged);
  ASSERT_GT(node.plugged_time.tv_sec, 0);
  ASSERT_EQ(CRAS_NODE_TYPE_HAPTIC, node.type);
  ASSERT_EQ(NODE_POSITION_INTERNAL, node.position);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Rumbler");
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(1, node.plugged);
  ASSERT_GT(node.plugged_time.tv_sec, 0);
  ASSERT_EQ(CRAS_NODE_TYPE_HAPTIC, node.type);
  ASSERT_EQ(NODE_POSITION_INTERNAL, node.position);
}

TEST(AlsaInitNode, SetNodeInitialStateDropInvalidUTF8NodeName) {
  struct cras_ionode node;
  struct cras_iodev dev;

  memset(&dev, 0, sizeof(dev));
  memset(&node, 0, sizeof(node));
  node.dev = &dev;

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Something USB");
  //0xfe can not appear in a valid UTF-8 string.
  node.name[0] = 0xfe;
  is_utf8_string_ret_value = 0;
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_USB);
  ASSERT_EQ(CRAS_NODE_TYPE_USB, node.type);
  ASSERT_STREQ("USB", node.name);

  memset(&node, 0, sizeof(node));
  node.dev = &dev;
  strcpy(node.name, "Something HDMI Jack");
  //0xfe can not appear in a valid UTF-8 string.
  node.name[0] = 0xfe;
  is_utf8_string_ret_value = 0;
  dev.direction = CRAS_STREAM_OUTPUT;
  set_node_initial_state(&node, ALSA_CARD_TYPE_INTERNAL);
  ASSERT_EQ(CRAS_NODE_TYPE_HDMI, node.type);
  ASSERT_STREQ("HDMI", node.name);
}

TEST(AlsaIoInit, HDMIJackUpdateInvalidUTF8MonitorName) {
  struct alsa_io *aio;
  struct cras_alsa_mixer * const fake_mixer = (struct cras_alsa_mixer*)2;
  struct cras_use_case_mgr * const fake_ucm = (struct cras_use_case_mgr*)3;
  const struct cras_alsa_jack *jack = (struct cras_alsa_jack*)4;

  ResetStubData();
  aio = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
      0, NULL, ALSA_CARD_TYPE_INTERNAL, 0, fake_mixer, fake_config, fake_ucm,
      CRAS_STREAM_OUTPUT);
  ASSERT_EQ(0, alsa_iodev_legacy_complete_init((struct cras_iodev *)aio));

  // Prepare the stub data such that the jack will be identified as an
  // HDMI jack, and thus the callback creates an HDMI node.
  cras_alsa_jack_get_name_ret_value = "HDMI Jack";
  // Set the jack name updated from monitor to be an invalid UTF8 string.
  cras_alsa_jack_update_monitor_fake_name = strdup("Something");
  cras_alsa_jack_update_monitor_fake_name[0] = 0xfe;
  is_utf8_string_ret_value = 0;

  // Add the jack node.
  cras_alsa_jack_list_create_cb(jack, 1, cras_alsa_jack_list_create_cb_data);

  EXPECT_EQ(2, cras_alsa_jack_get_name_called);
  ASSERT_EQ(CRAS_NODE_TYPE_HDMI, aio->base.nodes->next->type);
  // The node name should be "HDMI".
  ASSERT_STREQ("HDMI", aio->base.nodes->next->name);

  alsa_iodev_destroy((struct cras_iodev *)aio);
}

//  Test thread add/rm stream, open_alsa, and iodev config.
class AlsaVolumeMuteSuite : public testing::Test {
  protected:
    virtual void SetUp() {
      ResetStubData();
      output_control_ = reinterpret_cast<struct mixer_control *>(10);
      cras_alsa_mixer_list_outputs_outputs = &output_control_;
      cras_alsa_mixer_list_outputs_outputs_length = 1;
      cras_alsa_mixer_get_control_name_values[output_control_] = "Speaker";
      cras_alsa_mixer_list_outputs_outputs_length = 1;
      aio_output_ = (struct alsa_io *)alsa_iodev_create_with_default_parameters(
          0, NULL,
          ALSA_CARD_TYPE_INTERNAL, 1,
          fake_mixer, fake_config, NULL,
          CRAS_STREAM_OUTPUT);
      alsa_iodev_legacy_complete_init((struct cras_iodev *)aio_output_);
      EXPECT_EQ(2, cras_card_config_get_volume_curve_for_control_called);

      struct cras_ionode *node;
      int count = 0;
      DL_FOREACH(aio_output_->base.nodes, node) {
        printf("node %d \n", count);
      }
      aio_output_->base.direction = CRAS_STREAM_OUTPUT;
      fmt_.frame_rate = 44100;
      fmt_.num_channels = 2;
      fmt_.format = SND_PCM_FORMAT_S16_LE;
      aio_output_->base.format = &fmt_;
      cras_alsa_get_avail_frames_ret = -1;
    }

    virtual void TearDown() {
      alsa_iodev_destroy((struct cras_iodev *)aio_output_);
      cras_alsa_get_avail_frames_ret = 0;
    }

  struct mixer_control *output_control_;
  struct alsa_io *aio_output_;
  struct cras_audio_format fmt_;
};

TEST_F(AlsaVolumeMuteSuite, GetDefaultVolumeCurve) {
  int rc;
  struct cras_audio_format *fmt;

  fmt = (struct cras_audio_format *)malloc(sizeof(*fmt));
  memcpy(fmt, &fmt_, sizeof(fmt_));
  aio_output_->base.format = fmt;
  aio_output_->handle = (snd_pcm_t *)0x24;

  rc = aio_output_->base.open_dev(&aio_output_->base);
  ASSERT_EQ(0, rc);
  EXPECT_EQ(&default_curve, fake_get_dBFS_volume_curve_val);

  aio_output_->base.set_volume(&aio_output_->base);
  EXPECT_EQ(&default_curve, fake_get_dBFS_volume_curve_val);
}

TEST_F(AlsaVolumeMuteSuite, GetVolumeCurveFromNode)
{
  int rc;
  struct cras_audio_format *fmt;
  struct cras_alsa_jack *jack = (struct cras_alsa_jack*)4;
  struct cras_ionode *node;
  struct cras_volume_curve hp_curve = {
    .get_dBFS = fake_get_dBFS,
  };

  fmt = (struct cras_audio_format *)malloc(sizeof(*fmt));
  memcpy(fmt, &fmt_, sizeof(fmt_));
  aio_output_->base.format = fmt;
  aio_output_->handle = (snd_pcm_t *)0x24;

  // Headphone jack plugged and has its own volume curve.
  cras_alsa_jack_get_mixer_output_ret = NULL;
  cras_alsa_jack_get_name_ret_value = "Headphone";
  cras_card_config_get_volume_curve_vals["Headphone"] = &hp_curve;
  cras_alsa_jack_list_create_cb(jack, 1, cras_alsa_jack_list_create_cb_data);
  EXPECT_EQ(1, cras_alsa_jack_update_node_type_called);
  EXPECT_EQ(3, cras_card_config_get_volume_curve_for_control_called);

  // Switch to node 'Headphone'.
  node = aio_output_->base.nodes->next;
  aio_output_->base.active_node = node;

  rc = aio_output_->base.open_dev(&aio_output_->base);
  ASSERT_EQ(0, rc);
  EXPECT_EQ(&hp_curve, fake_get_dBFS_volume_curve_val);

  aio_output_->base.set_volume(&aio_output_->base);
  EXPECT_EQ(&hp_curve, fake_get_dBFS_volume_curve_val);
}

TEST_F(AlsaVolumeMuteSuite, SetVolume) {
  int rc;
  struct cras_audio_format *fmt;
  const size_t fake_system_volume = 55;
  const size_t fake_system_volume_dB = (fake_system_volume - 100) * 100;

  fmt = (struct cras_audio_format *)malloc(sizeof(*fmt));
  memcpy(fmt, &fmt_, sizeof(fmt_));
  aio_output_->base.format = fmt;
  aio_output_->handle = (snd_pcm_t *)0x24;

  aio_output_->num_underruns = 3; //  Something non-zero.
  sys_get_volume_return_value = fake_system_volume;
  rc = aio_output_->base.open_dev(&aio_output_->base);
  ASSERT_EQ(0, rc);
  EXPECT_EQ(1, alsa_mixer_set_dBFS_called);
  EXPECT_EQ(fake_system_volume_dB, alsa_mixer_set_dBFS_value);

  alsa_mixer_set_dBFS_called = 0;
  alsa_mixer_set_dBFS_value = 0;
  sys_get_volume_return_value = 50;
  sys_get_volume_called = 0;
  aio_output_->base.set_volume(&aio_output_->base);
  EXPECT_EQ(1, sys_get_volume_called);
  EXPECT_EQ(1, alsa_mixer_set_dBFS_called);
  EXPECT_EQ(-5000, alsa_mixer_set_dBFS_value);
  EXPECT_EQ(output_control_, alsa_mixer_set_dBFS_output);

  alsa_mixer_set_dBFS_called = 0;
  alsa_mixer_set_dBFS_value = 0;
  sys_get_volume_return_value = 0;
  sys_get_volume_called = 0;
  aio_output_->base.set_volume(&aio_output_->base);
  EXPECT_EQ(1, sys_get_volume_called);
  EXPECT_EQ(1, alsa_mixer_set_dBFS_called);
  EXPECT_EQ(-10000, alsa_mixer_set_dBFS_value);

  sys_get_volume_return_value = 80;
  aio_output_->base.active_node->volume = 90;
  aio_output_->base.set_volume(&aio_output_->base);
  EXPECT_EQ(-3000, alsa_mixer_set_dBFS_value);

  // close the dev.
  rc = aio_output_->base.close_dev(&aio_output_->base);
  EXPECT_EQ(0, rc);
  EXPECT_EQ((void *)NULL, aio_output_->handle);

  free(fmt);
}

TEST_F(AlsaVolumeMuteSuite, SetMute) {
  int muted;

  aio_output_->handle = (snd_pcm_t *)0x24;

  // Test mute.
  ResetStubData();
  muted = 1;

  sys_get_mute_return_value = muted;

  aio_output_->base.set_mute(&aio_output_->base);

  EXPECT_EQ(1, sys_get_mute_called);
  EXPECT_EQ(1, alsa_mixer_set_mute_called);
  EXPECT_EQ(muted, alsa_mixer_set_mute_value);
  EXPECT_EQ(output_control_, alsa_mixer_set_mute_output);

  // Test unmute.
  ResetStubData();
  muted = 0;

  sys_get_mute_return_value = muted;

  aio_output_->base.set_mute(&aio_output_->base);

  EXPECT_EQ(1, sys_get_mute_called);
  EXPECT_EQ(1, alsa_mixer_set_mute_called);
  EXPECT_EQ(muted, alsa_mixer_set_mute_value);
  EXPECT_EQ(output_control_, alsa_mixer_set_mute_output);
}

//  Test free run.
class AlsaFreeRunTestSuite: public testing::Test {
  protected:
    virtual void SetUp() {
      ResetStubData();
      memset(&aio, 0, sizeof(aio));
      fmt_.format = SND_PCM_FORMAT_S16_LE;
      fmt_.frame_rate = 48000;
      fmt_.num_channels = 2;
      aio.base.format = &fmt_;
      aio.base.buffer_size = BUFFER_SIZE;
      aio.base.min_cb_level = 240;
    }

    virtual void TearDown() {
    }

  struct alsa_io aio;
  struct cras_audio_format fmt_;
};

TEST_F(AlsaFreeRunTestSuite, FillWholeBufferWithZeros) {
  int rc;
  int16_t *zeros;

  cras_alsa_mmap_begin_buffer = (uint8_t *)calloc(
      BUFFER_SIZE * 2 * 2,
      sizeof(*cras_alsa_mmap_begin_buffer));
  memset(cras_alsa_mmap_begin_buffer, 0xff,
         sizeof(*cras_alsa_mmap_begin_buffer));

  rc = fill_whole_buffer_with_zeros(&aio.base);

  EXPECT_EQ(0, rc);
  zeros = (int16_t *)calloc(BUFFER_SIZE * 2, sizeof(*zeros));
  EXPECT_EQ(0, memcmp(zeros, cras_alsa_mmap_begin_buffer, BUFFER_SIZE * 2 * 2));

  free(zeros);
  free(cras_alsa_mmap_begin_buffer);
}

TEST_F(AlsaFreeRunTestSuite, EnterFreeRunAlreadyFreeRunning) {
  int rc;

  // Device is in free run state, no need to fill zeros or fill whole buffer.
  aio.is_free_running = 1;

  rc = no_stream(&aio.base, 1);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, cras_alsa_mmap_get_whole_buffer_called);
  EXPECT_EQ(0, cras_iodev_fill_odev_zeros_called);
  EXPECT_EQ(0, cras_iodev_fill_odev_zeros_frames);
}

TEST_F(AlsaFreeRunTestSuite, EnterFreeRunNotDrainedYetNeedToFillZeros) {
  int rc;

  // Device is not in free run state. There are still valid samples to play.
  // The number of valid samples is less than min_cb_level * 2.
  // Need to fill zeros targeting min_cb_level * 2 = 480.
  // The number of zeros to be filled is 480 - 200 = 280.
  cras_iodev_frames_queued_ret = 200;
  cras_iodev_buffer_avail_ret = BUFFER_SIZE - cras_iodev_frames_queued_ret;

  rc = no_stream(&aio.base, 1);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, cras_alsa_mmap_get_whole_buffer_called);
  EXPECT_EQ(1, cras_iodev_fill_odev_zeros_called);
  EXPECT_EQ(280, cras_iodev_fill_odev_zeros_frames);
  EXPECT_EQ(280, aio.filled_zeros_for_draining);
  EXPECT_EQ(0, aio.is_free_running);
}

TEST_F(AlsaFreeRunTestSuite, EnterFreeRunNotDrainedYetNoNeedToFillZeros) {
  int rc;

  // Device is not in free run state. There are still valid samples to play.
  // The number of valid samples is more than min_cb_level * 2.
  // No need to fill zeros.
  cras_iodev_frames_queued_ret = 500;
  cras_iodev_buffer_avail_ret = BUFFER_SIZE - cras_iodev_frames_queued_ret;

  rc = no_stream(&aio.base, 1);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, cras_alsa_mmap_get_whole_buffer_called);
  EXPECT_EQ(0, cras_iodev_fill_odev_zeros_called);
  EXPECT_EQ(0, aio.is_free_running);
}

TEST_F(AlsaFreeRunTestSuite, EnterFreeRunDrained) {
  int rc;

  // Device is not in free run state. There are still valid samples to play.
  // The number of valid samples is less than filled zeros.
  // Should enter free run state and fill whole buffer with zeros.
  cras_iodev_frames_queued_ret = 40;
  cras_iodev_buffer_avail_ret = BUFFER_SIZE - cras_iodev_frames_queued_ret;
  aio.filled_zeros_for_draining = 100;

  rc = no_stream(&aio.base, 1);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_alsa_mmap_get_whole_buffer_called);
  EXPECT_EQ(0, cras_iodev_fill_odev_zeros_called);
  EXPECT_EQ(1, aio.is_free_running);
}

TEST_F(AlsaFreeRunTestSuite, EnterFreeRunNoSamples) {
  int rc;

  // Device is not in free run state. There is no sample to play.
  // Should enter free run state and fill whole buffer with zeros.
  cras_iodev_frames_queued_ret = 0;
  cras_iodev_buffer_avail_ret = BUFFER_SIZE - cras_iodev_frames_queued_ret;

  rc = no_stream(&aio.base, 1);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_alsa_mmap_get_whole_buffer_called);
  EXPECT_EQ(0, cras_iodev_fill_odev_zeros_called);
  EXPECT_EQ(1, aio.is_free_running);
}

TEST_F(AlsaFreeRunTestSuite, OutputShouldWake) {

  aio.is_free_running = 1;

  EXPECT_EQ(0, output_should_wake(&aio.base));

  aio.is_free_running = 0;
  aio.base.state = CRAS_IODEV_STATE_NO_STREAM_RUN;
  EXPECT_EQ(1, output_should_wake(&aio.base));

  aio.base.state = CRAS_IODEV_STATE_NORMAL_RUN;
  EXPECT_EQ(1, output_should_wake(&aio.base));

  aio.base.state = CRAS_IODEV_STATE_OPEN;
  EXPECT_EQ(0, output_should_wake(&aio.base));
}

TEST_F(AlsaFreeRunTestSuite, LeaveFreeRunNotInFreeRun) {
  int rc;

  rc = no_stream(&aio.base, 0);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, cras_alsa_resume_appl_ptr_called);
}

TEST_F(AlsaFreeRunTestSuite, LeaveFreeRunInFreeRun) {
  int rc;

  aio.is_free_running = 1;
  aio.filled_zeros_for_draining = 100;
  aio.base.min_buffer_level = 512;

  rc = no_stream(&aio.base, 0);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_alsa_resume_appl_ptr_called);
  EXPECT_EQ(aio.base.min_buffer_level + aio.base.min_cb_level,
            cras_alsa_resume_appl_ptr_ahead);
  EXPECT_EQ(0, aio.is_free_running);
  EXPECT_EQ(0, aio.filled_zeros_for_draining);
}

// Reuse AlsaFreeRunTestSuite for output underrun handling because they are
// similar.
TEST_F(AlsaFreeRunTestSuite, OutputUnderrun) {
  int rc;
  int16_t *zeros;

  cras_alsa_mmap_begin_buffer = (uint8_t *)calloc(
      BUFFER_SIZE * 2 * 2,
      sizeof(*cras_alsa_mmap_begin_buffer));
  memset(cras_alsa_mmap_begin_buffer, 0xff,
         sizeof(*cras_alsa_mmap_begin_buffer));

  // Ask alsa_io to handle output underrun.
  rc = alsa_output_underrun(&aio.base);
  EXPECT_EQ(0, rc);

  // mmap buffer should be filled with zeros.
  zeros = (int16_t *)calloc(BUFFER_SIZE * 2, sizeof(*zeros));
  EXPECT_EQ(0, memcmp(zeros, cras_alsa_mmap_begin_buffer, BUFFER_SIZE * 2 * 2));

  // appl_ptr should be moved to min_buffer_level + min_cb_level ahead of
  // hw_ptr.
  EXPECT_EQ(1, cras_alsa_resume_appl_ptr_called);
  EXPECT_EQ(aio.base.min_buffer_level + aio.base.min_cb_level,
            cras_alsa_resume_appl_ptr_ahead);

  free(zeros);
  free(cras_alsa_mmap_begin_buffer);
}


}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  openlog(NULL, LOG_PERROR, LOG_USER);
  return RUN_ALL_TESTS();
}

//  Stubs

extern "C" {

//  From iodev.
int cras_iodev_list_add_output(struct cras_iodev *output)
{
  return 0;
}
int cras_iodev_list_rm_output(struct cras_iodev *dev)
{
  return 0;
}

int cras_iodev_list_add_input(struct cras_iodev *input)
{
  return 0;
}
int cras_iodev_list_rm_input(struct cras_iodev *dev)
{
  return 0;
}

char *cras_iodev_list_get_hotword_models(cras_node_id_t node_id)
{
	return NULL;
}

int cras_iodev_list_set_hotword_model(cras_node_id_t node_id,
				      const char *model_name)
{
	return 0;
}

struct audio_thread *cras_iodev_list_get_audio_thread()
{
  return NULL;
}

//  From alsa helper.
int cras_alsa_set_channel_map(snd_pcm_t *handle,
			      struct cras_audio_format *fmt)
{
  return 0;
}
int cras_alsa_get_channel_map(snd_pcm_t *handle,
			      struct cras_audio_format *fmt)
{
  return 0;
}
int cras_alsa_pcm_open(snd_pcm_t **handle, const char *dev,
		       snd_pcm_stream_t stream)
{
  *handle = (snd_pcm_t *)0x24;
  cras_alsa_open_called++;
  return 0;
}
int cras_alsa_pcm_close(snd_pcm_t *handle)
{
  return 0;
}
int cras_alsa_pcm_start(snd_pcm_t *handle)
{
  cras_alsa_start_called++;
  return 0;
}
int cras_alsa_pcm_drain(snd_pcm_t *handle)
{
  return 0;
}
int cras_alsa_fill_properties(const char *dev,
			      snd_pcm_stream_t stream,
			      size_t **rates,
			      size_t **channel_counts,
			      snd_pcm_format_t **formats)
{
  *rates = (size_t *)malloc(sizeof(**rates) * 3);
  (*rates)[0] = 44100;
  (*rates)[1] = 48000;
  (*rates)[2] = 0;
  *channel_counts = (size_t *)malloc(sizeof(**channel_counts) * 2);
  (*channel_counts)[0] = 2;
  (*channel_counts)[1] = 0;
  *formats = (snd_pcm_format_t *)malloc(sizeof(**formats) * 2);
  (*formats)[0] = SND_PCM_FORMAT_S16_LE;
  (*formats)[1] = (snd_pcm_format_t)0;

  cras_alsa_fill_properties_called++;
  return 0;
}
int cras_alsa_set_hwparams(snd_pcm_t *handle, struct cras_audio_format *format,
			   snd_pcm_uframes_t *buffer_size, int period_wakeup,
			   unsigned int dma_period_time)
{
  return 0;
}
int cras_alsa_set_swparams(snd_pcm_t *handle, int *enable_htimestamp)
{
  return 0;
}
int cras_alsa_get_avail_frames(snd_pcm_t *handle, snd_pcm_uframes_t buf_size,
                               snd_pcm_uframes_t severe_underrun_frames,
                               const char* dev_name,
                               snd_pcm_uframes_t *used,
                               struct timespec *tstamp,
                               unsigned int *num_underruns)
{
  *used = cras_alsa_get_avail_frames_avail;
  clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
  return cras_alsa_get_avail_frames_ret;
}
int cras_alsa_get_delay_frames(snd_pcm_t *handle, snd_pcm_uframes_t buf_size,
			       snd_pcm_sframes_t *delay)
{
  *delay = 0;
  return 0;
}
int cras_alsa_mmap_begin(snd_pcm_t *handle, unsigned int format_bytes,
			 uint8_t **dst, snd_pcm_uframes_t *offset,
			 snd_pcm_uframes_t *frames, unsigned int *underruns)
{
  *dst = cras_alsa_mmap_begin_buffer;
  *frames = cras_alsa_mmap_begin_frames;
  return 0;
}
int cras_alsa_mmap_commit(snd_pcm_t *handle, snd_pcm_uframes_t offset,
			  snd_pcm_uframes_t frames, unsigned int *underruns)
{
  return 0;
}
int cras_alsa_attempt_resume(snd_pcm_t *handle)
{
  cras_alsa_attempt_resume_called++;
  return 0;
}

//  ALSA stubs.
int snd_pcm_format_physical_width(snd_pcm_format_t format)
{
  return 16;
}

snd_pcm_state_t snd_pcm_state(snd_pcm_t *handle)
{
  return snd_pcm_state_ret;
}

const char *snd_strerror(int errnum)
{
  return "Alsa Error in UT";
}

struct mixer_control *cras_alsa_mixer_get_control_for_section(
		struct cras_alsa_mixer *cras_mixer,
		const struct ucm_section *section)
{
  cras_alsa_mixer_get_control_for_section_called++;
  return cras_alsa_mixer_get_control_for_section_return_value;
}

const char *cras_alsa_mixer_get_control_name(
		const struct mixer_control *control)
{
  ControlNameMap::iterator it;
  cras_alsa_mixer_get_control_name_called++;
  it = cras_alsa_mixer_get_control_name_values.find(control);
  if (it == cras_alsa_mixer_get_control_name_values.end())
    return "";
  return it->second.c_str();
}

//  From system_state.
size_t cras_system_get_volume()
{
  sys_get_volume_called++;
  return sys_get_volume_return_value;
}

long cras_system_get_capture_gain()
{
  sys_get_capture_gain_called++;
  return sys_get_capture_gain_return_value;
}

int cras_system_get_mute()
{
  sys_get_mute_called++;
  return sys_get_mute_return_value;
}

int cras_system_get_capture_mute()
{
  sys_get_capture_mute_called++;
  return sys_get_capture_mute_return_value;
}

void cras_system_set_volume_limits(long min, long max)
{
  sys_set_volume_limits_called++;
}

void cras_system_set_capture_gain_limits(long min, long max)
{
  cras_system_set_capture_gain_limits_set_value[0] = min;
  cras_system_set_capture_gain_limits_set_value[1] = max;
  sys_set_capture_gain_limits_called++;
}

//  From cras_alsa_mixer.
void cras_alsa_mixer_set_dBFS(struct cras_alsa_mixer *m,
			      long dB_level,
			      struct mixer_control *output)
{
  alsa_mixer_set_dBFS_called++;
  alsa_mixer_set_dBFS_value = dB_level;
  alsa_mixer_set_dBFS_output = output;
}

void cras_alsa_mixer_set_mute(struct cras_alsa_mixer *cras_mixer,
			      int muted,
			      struct mixer_control *mixer_output)
{
  alsa_mixer_set_mute_called++;
  alsa_mixer_set_mute_value = muted;
  alsa_mixer_set_mute_output = mixer_output;
}

long cras_alsa_mixer_get_dB_range(struct cras_alsa_mixer *cras_mixer)
{
  alsa_mixer_get_dB_range_called++;
  return alsa_mixer_get_dB_range_value;
}

long cras_alsa_mixer_get_output_dB_range(
    struct mixer_control *mixer_output)
{
  alsa_mixer_get_output_dB_range_called++;
  return alsa_mixer_get_output_dB_range_value;
}

void cras_alsa_mixer_set_capture_dBFS(struct cras_alsa_mixer *m, long dB_level,
		                      struct mixer_control *mixer_input)
{
  alsa_mixer_set_capture_dBFS_called++;
  alsa_mixer_set_capture_dBFS_value = dB_level;
  alsa_mixer_set_capture_dBFS_input = mixer_input;
}

void cras_alsa_mixer_set_capture_mute(struct cras_alsa_mixer *m, int mute,
				      struct mixer_control *mixer_input)
{
  alsa_mixer_set_capture_mute_called++;
  alsa_mixer_set_capture_mute_value = mute;
  alsa_mixer_set_capture_mute_input = mixer_input;
}

void cras_alsa_mixer_list_outputs(struct cras_alsa_mixer *cras_mixer,
				  cras_alsa_mixer_control_callback cb,
				  void *callback_arg)
{
  cras_alsa_mixer_list_outputs_called++;
  for (size_t i = 0; i < cras_alsa_mixer_list_outputs_outputs_length; i++) {
    cb(cras_alsa_mixer_list_outputs_outputs[i], callback_arg);
  }
}

void cras_alsa_mixer_list_inputs(struct cras_alsa_mixer *cras_mixer,
				  cras_alsa_mixer_control_callback cb,
				  void *callback_arg)
{
  cras_alsa_mixer_list_inputs_called++;
  for (size_t i = 0; i < cras_alsa_mixer_list_inputs_outputs_length; i++) {
    cb(cras_alsa_mixer_list_inputs_outputs[i], callback_arg);
  }
}

int cras_alsa_mixer_set_output_active_state(
		struct mixer_control *output,
		int active)
{
  cras_alsa_mixer_set_output_active_state_called++;
  cras_alsa_mixer_set_output_active_state_outputs.push_back(output);
  cras_alsa_mixer_set_output_active_state_values.push_back(active);
  return 0;
}

void cras_volume_curve_destroy(struct cras_volume_curve *curve)
{
}

long cras_alsa_mixer_get_minimum_capture_gain(struct cras_alsa_mixer *cmix,
		struct mixer_control *mixer_input)
{
	cras_alsa_mixer_get_minimum_capture_gain_called++;
	cras_alsa_mixer_get_minimum_capture_gain_mixer_input = mixer_input;
	return cras_alsa_mixer_get_minimum_capture_gain_ret_value;
}

long cras_alsa_mixer_get_maximum_capture_gain(struct cras_alsa_mixer *cmix,
		struct mixer_control *mixer_input)
{
	cras_alsa_mixer_get_maximum_capture_gain_called++;
	cras_alsa_mixer_get_maximum_capture_gain_mixer_input = mixer_input;
	return cras_alsa_mixer_get_maximum_capture_gain_ret_value;
}

int cras_alsa_mixer_has_main_volume(const struct cras_alsa_mixer *cras_mixer)
{
  return 1;
}

int cras_alsa_mixer_has_volume(const struct mixer_control *mixer_control)
{
  return 1;
}

// From cras_alsa_jack
struct cras_alsa_jack_list *cras_alsa_jack_list_create(
		unsigned int card_index,
		const char *card_name,
		unsigned int device_index,
		int check_gpio_jack,
		struct cras_alsa_mixer *mixer,
                struct cras_use_case_mgr *ucm,
		snd_hctl_t *hctl,
		enum CRAS_STREAM_DIRECTION direction,
		jack_state_change_callback *cb,
		void *cb_data)
{
  cras_alsa_jack_list_create_called++;
  cras_alsa_jack_list_create_cb = cb;
  cras_alsa_jack_list_create_cb_data = cb_data;
  return (struct cras_alsa_jack_list *)0xfee;
}

int cras_alsa_jack_list_find_jacks_by_name_matching(
	struct cras_alsa_jack_list *jack_list)
{
  cras_alsa_jack_list_find_jacks_by_name_matching_called++;
  return 0;
}

int cras_alsa_jack_list_add_jack_for_section(
	struct cras_alsa_jack_list *jack_list,
	struct ucm_section *ucm_section,
	struct cras_alsa_jack **result_jack)
{
  cras_alsa_jack_list_add_jack_for_section_called++;
  if (result_jack)
    *result_jack = cras_alsa_jack_list_add_jack_for_section_result_jack;
  return 0;
}

void cras_alsa_jack_list_destroy(struct cras_alsa_jack_list *jack_list)
{
  cras_alsa_jack_list_destroy_called++;
}

int cras_alsa_jack_list_has_hctl_jacks(struct cras_alsa_jack_list *jack_list)
{
  return cras_alsa_jack_list_has_hctl_jacks_return_val;
}

void cras_alsa_jack_list_report(const struct cras_alsa_jack_list *jack_list)
{
}

void cras_alsa_jack_enable_ucm(const struct cras_alsa_jack *jack, int enable) {
  cras_alsa_jack_enable_ucm_called++;
}

const char *cras_alsa_jack_get_name(const struct cras_alsa_jack *jack)
{
  cras_alsa_jack_get_name_called++;
  return cras_alsa_jack_get_name_ret_value;
}

const char *cras_alsa_jack_get_dsp_name(const struct cras_alsa_jack *jack)
{
  cras_alsa_jack_get_dsp_name_called++;
  return jack ? cras_alsa_jack_get_dsp_name_value : NULL;
}

const char *ucm_get_dsp_name_default(struct cras_use_case_mgr *mgr,
                                     int direction)
{
  ucm_get_dsp_name_default_called++;
  if (ucm_get_dsp_name_default_value)
    return strdup(ucm_get_dsp_name_default_value);
  else
    return NULL;
}

struct mixer_control *cras_alsa_jack_get_mixer_output(
    const struct cras_alsa_jack *jack)
{
  return cras_alsa_jack_get_mixer_output_ret;
}

struct mixer_control *cras_alsa_jack_get_mixer_input(
		const struct cras_alsa_jack *jack)
{
  return cras_alsa_jack_get_mixer_input_ret;
}

int ucm_set_enabled(
    struct cras_use_case_mgr *mgr, const char *dev, int enabled) {
  ucm_set_enabled_called++;
  return 0;
}

char *ucm_get_flag(struct cras_use_case_mgr *mgr, const char *flag_name) {
  char *ret = (char *)malloc(8);
  if ((!strcmp(flag_name, "AutoUnplugInputNode") &&
       auto_unplug_input_node_ret) ||
      (!strcmp(flag_name, "AutoUnplugOutputNode") &&
       auto_unplug_output_node_ret)) {
    snprintf(ret, 8, "%s", "1");
    return ret;
  }

  return NULL;
}

char *ucm_get_mic_positions(struct cras_use_case_mgr *mgr) {
  return NULL;
}

int ucm_swap_mode_exists(struct cras_use_case_mgr *mgr)
{
  return ucm_swap_mode_exists_ret_value;
}

int ucm_enable_swap_mode(struct cras_use_case_mgr *mgr, const char *node_name,
                         int enable)
{
  ucm_enable_swap_mode_called++;
  return ucm_enable_swap_mode_ret_value;
}

unsigned int ucm_get_min_buffer_level(struct cras_use_case_mgr *mgr)
{
  return 0;
}

unsigned int ucm_get_enable_htimestamp_flag(struct cras_use_case_mgr *mgr)
{
  return ucm_get_enable_htimestamp_flag_ret;
}

unsigned int ucm_get_disable_software_volume(struct cras_use_case_mgr *mgr)
{
  return 0;
}

int ucm_get_max_software_gain(struct cras_use_case_mgr *mgr, const char *dev,
    long *gain)
{
  ucm_get_max_software_gain_called++;
  *gain = ucm_get_max_software_gain_value;
  return ucm_get_max_software_gain_ret_value;
}

char *ucm_get_hotword_models(struct cras_use_case_mgr *mgr)
{
  return NULL;
}

int ucm_set_hotword_model(struct cras_use_case_mgr *mgr, const char *model)
{
  return 0;
}

unsigned int ucm_get_dma_period_for_dev(struct cras_use_case_mgr *mgr,
                                        const char *dev)
{
  ucm_get_dma_period_for_dev_called++;
  return ucm_get_dma_period_for_dev_ret;
}

int ucm_get_sample_rate_for_dev(struct cras_use_case_mgr *mgr, const char *dev,
				enum CRAS_STREAM_DIRECTION direction)
{
  return -EINVAL;
}

int ucm_get_capture_chmap_for_dev(struct cras_use_case_mgr *mgr,
          const char *dev,
          int8_t *channel_layout)
{
  return -EINVAL;
}

struct cras_volume_curve *cras_volume_curve_create_default()
{
  return &default_curve;
}

struct cras_volume_curve *cras_card_config_get_volume_curve_for_control(
    const struct cras_card_config *card_config,
    const char *control_name)
{
  VolCurveMap::iterator it;
  cras_card_config_get_volume_curve_for_control_called++;
  if (!control_name)
    return NULL;
  it = cras_card_config_get_volume_curve_vals.find(control_name);
  if (it == cras_card_config_get_volume_curve_vals.end())
    return NULL;
  return it->second;
}

void cras_iodev_free_format(struct cras_iodev *iodev)
{
}

int cras_iodev_set_format(struct cras_iodev *iodev,
			  const struct cras_audio_format *fmt)
{
  fake_format = (struct cras_audio_format *)calloc(1, sizeof(*fake_format));
  // Copy the content of format from fmt into format of iodev.
  memcpy(fake_format, fmt, sizeof(*fake_format));
  iodev->format = fake_format;
  return 0;
}

struct audio_thread *audio_thread_create() {
  return reinterpret_cast<audio_thread*>(0x323);
}

void audio_thread_destroy(audio_thread* thread) {
}



void cras_iodev_update_dsp(struct cras_iodev *iodev)
{
  cras_iodev_update_dsp_called++;
  cras_iodev_update_dsp_name = iodev->dsp_name;
}

int cras_iodev_set_node_attr(struct cras_ionode *ionode,
			     enum ionode_attr attr, int value)
{
  cras_iodev_set_node_attr_called++;
  cras_iodev_set_node_attr_ionode = ionode;
  cras_iodev_set_node_attr_attr = attr;
  cras_iodev_set_node_attr_value = value;
  if (ionode && (attr == IONODE_ATTR_PLUGGED))
    ionode->plugged = value;
  return 0;
}

void cras_iodev_add_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
  cras_iodev_add_node_called++;
  DL_APPEND(iodev->nodes, node);
}

void cras_iodev_rm_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
  DL_DELETE(iodev->nodes, node);
}

void cras_iodev_set_active_node(struct cras_iodev *iodev,
                                struct cras_ionode *node)
{
  iodev->active_node = node;
}

void cras_iodev_free_resources(struct cras_iodev *iodev)
{
  cras_iodev_free_resources_called++;
}

void cras_alsa_jack_update_monitor_name(const struct cras_alsa_jack *jack,
					char *name_buf,
					unsigned int buf_size)
{
  if (cras_alsa_jack_update_monitor_fake_name)
    strcpy(name_buf, cras_alsa_jack_update_monitor_fake_name);
}

void cras_alsa_jack_update_node_type(const struct cras_alsa_jack *jack,
				     enum CRAS_NODE_TYPE *type)
{
  cras_alsa_jack_update_node_type_called++;
}

const char *cras_alsa_jack_get_ucm_device(const struct cras_alsa_jack *jack)
{
  return NULL;
}

int ucm_get_default_node_gain(struct cras_use_case_mgr *mgr, const char *dev,
                              long *gain)
{
  if (ucm_get_default_node_gain_values.find(dev) ==
      ucm_get_default_node_gain_values.end())
    return 1;

  *gain = ucm_get_default_node_gain_values[dev];
  return 0;
}

void cras_iodev_init_audio_area(struct cras_iodev *iodev,
                                int num_channels) {
}

void cras_iodev_free_audio_area(struct cras_iodev *iodev) {
}

int cras_iodev_reset_rate_estimator(const struct cras_iodev *iodev)
{
  return 0;
}

int cras_iodev_frames_queued(struct cras_iodev *iodev, struct timespec *tstamp)
{
  clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
  return cras_iodev_frames_queued_ret;
}

int cras_iodev_buffer_avail(struct cras_iodev *iodev, unsigned hw_level)
{
  return cras_iodev_buffer_avail_ret;
}

int cras_iodev_fill_odev_zeros(struct cras_iodev *odev, unsigned int frames)
{
  cras_iodev_fill_odev_zeros_called++;
  cras_iodev_fill_odev_zeros_frames = frames;
  return 0;
}

void cras_audio_area_config_buf_pointers(struct cras_audio_area *area,
					 const struct cras_audio_format *fmt,
					 uint8_t *base_buffer)
{
}

void audio_thread_add_callback(int fd, thread_callback cb, void *data)
{
}

void audio_thread_rm_callback(int fd)
{
}

int audio_thread_rm_callback_sync(struct audio_thread *thread, int fd) {
  return 0;
}

int is_utf8_string(const char* string)
{
  return is_utf8_string_ret_value;
}

int cras_alsa_mmap_get_whole_buffer(snd_pcm_t *handle, uint8_t **dst,
				    unsigned int *underruns)
{
  snd_pcm_uframes_t offset, frames;

  cras_alsa_mmap_get_whole_buffer_called++;
  return cras_alsa_mmap_begin(handle, 0, dst, &offset, &frames, underruns);
}

int cras_alsa_resume_appl_ptr(snd_pcm_t *handle, snd_pcm_uframes_t ahead)
{
  cras_alsa_resume_appl_ptr_called++;
  cras_alsa_resume_appl_ptr_ahead = ahead;
  return 0;
}

int cras_iodev_default_no_stream_playback(struct cras_iodev *odev, int enable)
{
  return 0;
}

enum CRAS_IODEV_STATE cras_iodev_state(const struct cras_iodev *iodev)
{
  return iodev->state;
}

int cras_iodev_dsp_set_swap_mode_for_node(struct cras_iodev *iodev,
                                          struct cras_ionode *node,
                                          int enable)
{
  cras_iodev_dsp_set_swap_mode_for_node_called++;
  return 0;
}

struct cras_ramp* cras_ramp_create() {
  return (struct cras_ramp*)0x1;
}

}
