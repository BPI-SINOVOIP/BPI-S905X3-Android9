// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <linux/input.h>
#include <map>
#include <poll.h>
#include <stdio.h>
#include <sys/param.h>
#include <gtest/gtest.h>
#include <string>
#include <syslog.h>
#include <vector>

extern "C" {
#include "cras_alsa_jack.h"
#include "cras_alsa_ucm_section.h"
#include "cras_gpio_jack.h"
#include "cras_tm.h"
#include "cras_types.h"
#include "cras_util.h"
}

namespace {

#define BITS_PER_BYTE		(8)
#define BITS_PER_LONG		(sizeof(long) * BITS_PER_BYTE)
#define NBITS(x)		((((x) - 1) / BITS_PER_LONG) + 1)
#define OFF(x)			((x) % BITS_PER_LONG)
#define BIT(x)			(1UL << OFF(x))
#define LONG(x)			((x) / BITS_PER_LONG)
#define IS_BIT_SET(bit, array)	!!((array[LONG(bit)]) & (1UL << OFF(bit)))

static int fake_jack_cb_plugged;
static void *fake_jack_cb_data;
static size_t fake_jack_cb_called;
unsigned int snd_hctl_elem_get_device_return_val;
unsigned int snd_hctl_elem_get_device_called;
static size_t snd_hctl_first_elem_called;
static snd_hctl_elem_t *snd_hctl_first_elem_return_val;
static size_t snd_hctl_elem_next_called;
std::deque<snd_hctl_elem_t *> snd_hctl_elem_next_ret_vals;
std::deque<snd_hctl_elem_t *> snd_hctl_elem_next_ret_vals_poped;
static size_t snd_hctl_elem_get_name_called;
static size_t snd_hctl_elem_set_callback_called;
static snd_hctl_elem_t *snd_hctl_elem_set_callback_obj;
static snd_hctl_elem_callback_t snd_hctl_elem_set_callback_value;
static size_t snd_hctl_find_elem_called;
static std::vector<snd_hctl_elem_t *> snd_hctl_find_elem_return_vals;
static std::map<std::string, size_t> snd_ctl_elem_id_set_name_map;
static size_t cras_system_add_select_fd_called;
static std::vector<int> cras_system_add_select_fd_values;
static size_t cras_system_rm_select_fd_called;
static std::vector<int> cras_system_rm_select_fd_values;
static size_t snd_hctl_elem_set_callback_private_called;
static void *snd_hctl_elem_set_callback_private_value;
static size_t snd_hctl_elem_get_hctl_called;
static snd_hctl_t *snd_hctl_elem_get_hctl_return_value;
static size_t snd_ctl_elem_value_get_boolean_called;
static int snd_ctl_elem_value_get_boolean_return_value;
static void *fake_jack_cb_arg;
static struct cras_alsa_mixer *fake_mixer;
static size_t cras_alsa_mixer_get_output_matching_name_called;
static size_t cras_alsa_mixer_get_input_matching_name_called;
static size_t cras_alsa_mixer_get_control_for_section_called;
static struct mixer_control *
    cras_alsa_mixer_get_output_matching_name_return_value;
static struct mixer_control *
    cras_alsa_mixer_get_input_matching_name_return_value;
static struct mixer_control *
    cras_alsa_mixer_get_control_for_section_return_value;
static size_t gpio_switch_list_for_each_called;
static std::vector<std::string> gpio_switch_list_for_each_dev_paths;
static std::vector<std::string> gpio_switch_list_for_each_dev_names;
static size_t gpio_switch_open_called;
static size_t gpio_switch_eviocgsw_called;
static size_t gpio_switch_eviocgbit_called;
static unsigned ucm_get_dev_for_jack_called;
static unsigned ucm_get_cap_control_called;
static char *ucm_get_cap_control_value;
static bool ucm_get_dev_for_jack_return;
static int ucm_set_enabled_value;
static unsigned long eviocbit_ret[NBITS(SW_CNT)];
static int gpio_switch_eviocgbit_fd;
static const char *edid_file_ret;
static size_t ucm_get_dsp_name_called;
static unsigned ucm_get_override_type_name_called;
static char *ucm_get_device_name_for_dev_value;
static snd_hctl_t *fake_hctl = (snd_hctl_t *)2;

static void ResetStubData() {
  gpio_switch_list_for_each_called = 0;
  gpio_switch_list_for_each_dev_paths.clear();
  gpio_switch_list_for_each_dev_paths.push_back("/dev/input/event3");
  gpio_switch_list_for_each_dev_paths.push_back("/dev/input/event2");
  gpio_switch_list_for_each_dev_names.clear();
  gpio_switch_open_called = 0;
  gpio_switch_eviocgsw_called = 0;
  gpio_switch_eviocgbit_called = 0;
  snd_hctl_elem_get_device_return_val = 0;
  snd_hctl_elem_get_device_called = 0;
  snd_hctl_first_elem_called = 0;
  snd_hctl_first_elem_return_val = reinterpret_cast<snd_hctl_elem_t *>(0x87);
  snd_hctl_elem_next_called = 0;
  snd_hctl_elem_next_ret_vals.clear();
  snd_hctl_elem_next_ret_vals_poped.clear();
  snd_hctl_elem_get_name_called = 0;
  snd_hctl_elem_set_callback_called = 0;
  snd_hctl_elem_set_callback_obj = NULL;
  snd_hctl_elem_set_callback_value = NULL;
  snd_hctl_find_elem_called = 0;
  snd_hctl_find_elem_return_vals.clear();
  snd_ctl_elem_id_set_name_map.clear();
  cras_system_add_select_fd_called = 0;
  cras_system_add_select_fd_values.clear();
  cras_system_rm_select_fd_called = 0;
  cras_system_rm_select_fd_values.clear();
  snd_hctl_elem_set_callback_private_called = 0;
  snd_hctl_elem_get_hctl_called = 0;
  snd_ctl_elem_value_get_boolean_called = 0;
  fake_jack_cb_called = 0;
  fake_jack_cb_plugged = 0;
  fake_jack_cb_arg = reinterpret_cast<void *>(0x987);
  fake_mixer = reinterpret_cast<struct cras_alsa_mixer *>(0x789);
  cras_alsa_mixer_get_output_matching_name_called = 0;
  cras_alsa_mixer_get_input_matching_name_called = 0;
  cras_alsa_mixer_get_control_for_section_called = 0;
  cras_alsa_mixer_get_output_matching_name_return_value =
      reinterpret_cast<struct mixer_control *>(0x456);
  cras_alsa_mixer_get_input_matching_name_return_value = NULL;
  cras_alsa_mixer_get_control_for_section_return_value =
      reinterpret_cast<struct mixer_control *>(0x456);
  ucm_get_dev_for_jack_called = 0;
  ucm_get_cap_control_called = 0;
  ucm_get_cap_control_value = NULL;
  ucm_get_dev_for_jack_return = false;
  edid_file_ret = NULL;
  ucm_get_dsp_name_called = 0;
  ucm_get_override_type_name_called = 0;
  ucm_get_device_name_for_dev_value = NULL;

  memset(eviocbit_ret, 0, sizeof(eviocbit_ret));
}

static void fake_jack_cb(const struct cras_alsa_jack *jack,
                         int plugged,
                         void *data)
{
  fake_jack_cb_called++;
  fake_jack_cb_plugged = plugged;
  fake_jack_cb_data = data;

  // Check that jack enable callback is called if there is a ucm device.
  ucm_set_enabled_value = !plugged;
  cras_alsa_jack_enable_ucm(jack, plugged);
  EXPECT_EQ(ucm_get_dev_for_jack_return ? plugged : !plugged,
            ucm_set_enabled_value);
}

TEST(AlsaJacks, CreateNullHctl) {
  struct cras_alsa_jack_list *jack_list;
  ResetStubData();
  jack_list = cras_alsa_jack_list_create(0, "c1", 0, 1,
                                         fake_mixer,
                                         NULL, NULL,
                                         CRAS_STREAM_OUTPUT,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_EQ(0, gpio_switch_open_called);
  EXPECT_EQ(0, gpio_switch_eviocgsw_called);
  EXPECT_EQ(0, gpio_switch_eviocgbit_called);

  cras_alsa_jack_list_destroy(jack_list);
}

TEST(AlsaJacks, CreateNoElements) {
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  snd_hctl_first_elem_return_val = NULL;
  jack_list = cras_alsa_jack_list_create(0, "c1", 0, 1,
                                         fake_mixer,
                                         NULL, fake_hctl,
                                         CRAS_STREAM_OUTPUT,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_EQ(0, gpio_switch_open_called);
  EXPECT_EQ(0, gpio_switch_eviocgsw_called);
  EXPECT_EQ(0, gpio_switch_eviocgbit_called);
  EXPECT_EQ(1, snd_hctl_first_elem_called);
  EXPECT_EQ(0, snd_hctl_elem_next_called);

  cras_alsa_jack_list_destroy(jack_list);
}

static struct cras_alsa_jack_list *run_test_with_elem_list(
    CRAS_STREAM_DIRECTION direction,
    std::string *elems,
    unsigned int device_index,
    struct cras_use_case_mgr *ucm,
    size_t nelems,
    size_t nhdmi_jacks,
    size_t njacks) {
  struct cras_alsa_jack_list *jack_list;

  snd_hctl_first_elem_return_val =
      reinterpret_cast<snd_hctl_elem_t *>(&elems[0]);
  for (unsigned int i = 1; i < nelems; i++)
    snd_hctl_elem_next_ret_vals.push_front(
        reinterpret_cast<snd_hctl_elem_t *>(&elems[i]));

  jack_list = cras_alsa_jack_list_create(0,
                                         "card_name",
                                         device_index,
                                         1,
                                         fake_mixer,
                                         ucm, fake_hctl,
                                         direction,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  if (jack_list == NULL)
    return jack_list;
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  EXPECT_EQ(ucm ? njacks : 0, ucm_get_dev_for_jack_called);
  EXPECT_EQ(ucm ? njacks : 0, ucm_get_override_type_name_called);
  EXPECT_EQ(1 + nhdmi_jacks, snd_hctl_first_elem_called);
  EXPECT_EQ(njacks, snd_hctl_elem_set_callback_called);

  /* For some functions, the number of calls to them could
   * be larger then expected count if there is ELD control
   * in given elements. */
  EXPECT_GE(snd_hctl_elem_next_called, nelems + nhdmi_jacks);
  EXPECT_GE(snd_hctl_elem_get_name_called, nelems + njacks);

  if (direction == CRAS_STREAM_OUTPUT)
    EXPECT_EQ(njacks, cras_alsa_mixer_get_output_matching_name_called);
  if (direction == CRAS_STREAM_INPUT && ucm_get_dev_for_jack_return)
    EXPECT_EQ(njacks, ucm_get_cap_control_called);

  return jack_list;
}

static struct cras_alsa_jack_list *run_test_with_section(
    CRAS_STREAM_DIRECTION direction,
    std::string *elems,
    size_t nelems,
    unsigned int device_index,
    struct cras_use_case_mgr *ucm,
    struct ucm_section *ucm_section,
    int add_jack_rc,
    size_t njacks) {
  struct cras_alsa_jack_list *jack_list;
  struct cras_alsa_jack *jack;

  for (size_t i = 0; i < nelems; i++) {
    snd_ctl_elem_id_set_name_map[elems[i]] = i;
    snd_hctl_find_elem_return_vals.push_back(
        reinterpret_cast<snd_hctl_elem_t*>(&elems[i]));
  }

  jack_list = cras_alsa_jack_list_create(0,
                                         "card_name",
                                         device_index,
                                         1,
                                         fake_mixer,
                                         ucm, fake_hctl,
                                         direction,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  if (jack_list == NULL)
    return jack_list;
  EXPECT_EQ(add_jack_rc,
      cras_alsa_jack_list_add_jack_for_section(jack_list, ucm_section, &jack));
  if (add_jack_rc == 0) {
    EXPECT_EQ(njacks, ucm_get_dsp_name_called);
    EXPECT_NE(jack, reinterpret_cast<struct cras_alsa_jack *>(NULL));
  } else {
    EXPECT_EQ(jack, reinterpret_cast<struct cras_alsa_jack *>(NULL));
  }
  if (add_jack_rc != 0 || njacks != ucm_get_dsp_name_called) {
      cras_alsa_jack_list_destroy(jack_list);
      return NULL;
  }
  EXPECT_EQ(njacks, snd_hctl_elem_set_callback_called);
  EXPECT_EQ(njacks, cras_alsa_mixer_get_control_for_section_called);

  return jack_list;
}

TEST(AlsaJacks, ReportNull) {
  cras_alsa_jack_list_report(NULL);
}

TEST(AlsaJacks, CreateNoJacks) {
  static std::string elem_names[] = {
    "Mic Jack",
    "foo",
    "bar",
  };
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  jack_list = run_test_with_elem_list(CRAS_STREAM_OUTPUT,
                                      elem_names,
                                      0,
                                      NULL,
                                      ARRAY_SIZE(elem_names),
                                      0,
                                      0);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);

  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(0, cras_system_rm_select_fd_called);
}

TEST(AlsaJacks, CreateGPIOHp) {
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  gpio_switch_list_for_each_dev_names.push_back("some-other-device");
  gpio_switch_list_for_each_dev_names.push_back("c1 Headphone Jack");
  eviocbit_ret[LONG(SW_HEADPHONE_INSERT)] |= 1 << OFF(SW_HEADPHONE_INSERT);
  gpio_switch_eviocgbit_fd = 2;
  snd_hctl_first_elem_return_val = NULL;
  jack_list = cras_alsa_jack_list_create(0, "c1", 0, 1,
                                         fake_mixer,
                                         NULL, fake_hctl,
                                         CRAS_STREAM_OUTPUT,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_GT(gpio_switch_open_called, 1);
  EXPECT_EQ(1, gpio_switch_eviocgsw_called);
  EXPECT_GT(gpio_switch_eviocgbit_called, 1);
  EXPECT_EQ(1, cras_system_add_select_fd_called);
  EXPECT_EQ(1, cras_system_rm_select_fd_called);
}

TEST(AlsaJacks, CreateGPIOMic) {
  struct cras_alsa_jack_list *jack_list;
  ResetStubData();
  ucm_get_dev_for_jack_return = true;
  gpio_switch_list_for_each_dev_names.push_back("c1 Mic Jack");
  gpio_switch_list_for_each_dev_names.push_back("c1 Headphone Jack");
  eviocbit_ret[LONG(SW_MICROPHONE_INSERT)] |= 1 << OFF(SW_MICROPHONE_INSERT);
  gpio_switch_eviocgbit_fd = 3;
  snd_hctl_first_elem_return_val = NULL;
  ucm_get_cap_control_value = reinterpret_cast<char *>(0x1);

  // Freed in destroy.
  cras_alsa_mixer_get_input_matching_name_return_value =
      reinterpret_cast<struct mixer_control *>(malloc(1));

  jack_list = cras_alsa_jack_list_create(
      0,
      "c1",
      0,
      1,
      fake_mixer,
      reinterpret_cast<struct cras_use_case_mgr *>(0x55),
      fake_hctl,
      CRAS_STREAM_INPUT,
      fake_jack_cb,
      fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  EXPECT_EQ(ucm_get_cap_control_called, 1);
  EXPECT_EQ(cras_alsa_mixer_get_input_matching_name_called, 1);
  cras_alsa_jack_list_destroy(jack_list);
}

TEST(AlsaJacks, CreateGPIOHdmi) {
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  gpio_switch_list_for_each_dev_names.push_back("c1 HDMI Jack");
  gpio_switch_list_for_each_dev_names.push_back("c1 Mic Jack");
  eviocbit_ret[LONG(SW_LINEOUT_INSERT)] |= 1 << OFF(SW_LINEOUT_INSERT);
  gpio_switch_eviocgbit_fd = 3;
  snd_hctl_first_elem_return_val = NULL;
  jack_list = cras_alsa_jack_list_create(0, "c1", 0, 1,
                                         fake_mixer,
                                         NULL, fake_hctl,
                                         CRAS_STREAM_OUTPUT,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  EXPECT_EQ(1, gpio_switch_eviocgsw_called);

  fake_jack_cb_called = 0;
  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);

  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_GT(gpio_switch_open_called, 1);
  EXPECT_GT(gpio_switch_eviocgbit_called, 1);
  EXPECT_EQ(1, cras_system_add_select_fd_called);
  EXPECT_EQ(1, cras_system_rm_select_fd_called);
}

void run_gpio_jack_test(
    int device_index,
    int is_first_device,
    enum CRAS_STREAM_DIRECTION direction,
    int should_create_jack,
    const char* jack_name)
{
  struct cras_alsa_jack_list *jack_list;
  struct cras_use_case_mgr *ucm =
      reinterpret_cast<struct cras_use_case_mgr *>(0x55);

  gpio_switch_list_for_each_dev_names.push_back("some-other-device one");
  gpio_switch_eviocgbit_fd = 2;
  if (direction == CRAS_STREAM_OUTPUT) {
    eviocbit_ret[LONG(SW_HEADPHONE_INSERT)] |= 1 << OFF(SW_HEADPHONE_INSERT);
  } else {
    eviocbit_ret[LONG(SW_MICROPHONE_INSERT)] |= 1 << OFF(SW_MICROPHONE_INSERT);
  }
  gpio_switch_list_for_each_dev_names.push_back(jack_name);
  snd_hctl_first_elem_return_val = NULL;

  jack_list = cras_alsa_jack_list_create(0, "c1", device_index,
                                         is_first_device,
                                         fake_mixer,
                                         ucm, fake_hctl,
                                         direction,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));

  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(should_create_jack, fake_jack_cb_plugged);
  EXPECT_EQ(should_create_jack, fake_jack_cb_called);

  cras_alsa_jack_list_destroy(jack_list);
}

TEST(AlsaJacks, CreateGPIOHpUCMPlaybackPCMMatched) {
  int device_index = 1;
  int is_first_device = 0;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_OUTPUT;
  int should_create_jack = 1;

  ResetStubData();

  /* PlaybackPCM matched, so create jack even if this is not the first device.*/
  ucm_get_dev_for_jack_return = true;
  ucm_get_device_name_for_dev_value = strdup("hw:c1,1");

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, CreateGPIOHpUCMCapturePCMMatched) {
  int device_index = 1;
  int is_first_device = 0;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_INPUT;
  int should_create_jack = 1;

  ResetStubData();

  /* CapturePCM matched, so create jack even if this is not the first device.*/
  ucm_get_dev_for_jack_return = true;
  ucm_get_device_name_for_dev_value = strdup("hw:c1,1");

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Mic Jack");
}

TEST(AlsaJacks, CreateGPIOHpUCMPlaybackPCMNotMatched) {
  int device_index = 0;
  int is_first_device = 1;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_OUTPUT;
  int should_create_jack = 0;

  ResetStubData();

  /* PlaybackPCM not matched, do not create jack. */
  ucm_get_dev_for_jack_return = true;
  ucm_get_device_name_for_dev_value = strdup("hw:c1,2");

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, CreateGPIOHpUCMPlaybackPCMNotSpecifiedFirstDevice) {
  int device_index = 1;
  int is_first_device = 1;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_OUTPUT;
  int should_create_jack = 1;

  ResetStubData();

  /* PlaybackPCM not specified, create jack for the first device. */
  ucm_get_dev_for_jack_return = true;
  ucm_get_device_name_for_dev_value = NULL;

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, CreateGPIOHpUCMPlaybackPCMNotSpecifiedSecondDevice) {
  int device_index = 1;
  int is_first_device = 0;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_OUTPUT;
  int should_create_jack = 0;

  ResetStubData();

  /* PlaybackPCM not specified, do not create jack for the second device. */
  ucm_get_dev_for_jack_return = true;
  ucm_get_device_name_for_dev_value = NULL;

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, CreateGPIOHpNoUCMFirstDevice) {
  int device_index = 1;
  int is_first_device = 1;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_OUTPUT;
  int should_create_jack = 1;

  ResetStubData();

  /* No UCM for this jack, create jack for the first device. */
  ucm_get_dev_for_jack_return = false;
  ucm_get_device_name_for_dev_value = NULL;

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, CreateGPIOHpNoUCMSecondDevice) {
  int device_index = 1;
  int is_first_device = 0;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_OUTPUT;
  int should_create_jack = 0;

  ResetStubData();

  /* No UCM for this jack, dot not create jack for the second device. */
  ucm_get_dev_for_jack_return = false;
  ucm_get_device_name_for_dev_value = NULL;

  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, CreateGPIOMicNoUCMFirstDeviceMicJack) {
  int device_index = 1;
  int is_first_device = 1;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_INPUT;
  int should_create_jack = 1;

  ResetStubData();

  // No UCM for this jack, create jack for the first device.
  ucm_get_dev_for_jack_return = false;
  ucm_get_device_name_for_dev_value = NULL;

  // Mic Jack is a valid name for microphone jack.
  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Mic Jack");
}

TEST(AlsaJacks, CreateGPIOMicNoUCMFirstDeviceHeadsetJack) {
  int device_index = 1;
  int is_first_device = 1;
  enum CRAS_STREAM_DIRECTION direction = CRAS_STREAM_INPUT;
  int should_create_jack = 1;

  ResetStubData();

  // No UCM for this jack, create jack for the first device.
  ucm_get_dev_for_jack_return = false;
  ucm_get_device_name_for_dev_value = NULL;

  // Headset Jack is a valid name for microphone jack.
  run_gpio_jack_test(
      device_index, is_first_device, direction, should_create_jack,
      "c1 Headset Jack");
}

TEST(AlsaJacks, GPIOHdmiWithEdid) {
  cras_alsa_jack_list* jack_list;

  ResetStubData();
  ucm_get_dev_for_jack_return = 1;
  edid_file_ret = static_cast<char*>(calloc(1, 1));  // Freed in destroy.
  gpio_switch_list_for_each_dev_names.push_back("c1 HDMI Jack");
  eviocbit_ret[LONG(SW_LINEOUT_INSERT)] |= 1 << OFF(SW_LINEOUT_INSERT);
  gpio_switch_eviocgbit_fd = 3;
  snd_hctl_first_elem_return_val = NULL;
  jack_list = cras_alsa_jack_list_create(
      0,
      "c1",
      0,
      1,
      fake_mixer,
      reinterpret_cast<struct cras_use_case_mgr *>(0x55),
      fake_hctl,
      CRAS_STREAM_OUTPUT,
      fake_jack_cb,
      fake_jack_cb_arg);
  ASSERT_NE(static_cast<cras_alsa_jack_list*>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));
  EXPECT_EQ(1, gpio_switch_eviocgsw_called);

  // EDID shouldn't open, callback should be skipped until re-try.
  fake_jack_cb_called = 0;
  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(0, fake_jack_cb_called);

  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_GT(gpio_switch_open_called, 1);
  EXPECT_GT(gpio_switch_eviocgbit_called, 1);
  EXPECT_EQ(1, cras_system_add_select_fd_called);
  EXPECT_EQ(1, cras_system_rm_select_fd_called);
}

TEST(AlsaJacks, CreateGPIOHpNoNameMatch) {
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  gpio_switch_list_for_each_dev_names.push_back("some-other-device one");
  gpio_switch_list_for_each_dev_names.push_back("some-other-device two");
  snd_hctl_first_elem_return_val = NULL;
  jack_list = cras_alsa_jack_list_create(0, "c2", 0, 1,
                                         fake_mixer,
                                         NULL, fake_hctl,
                                         CRAS_STREAM_OUTPUT,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_find_jacks_by_name_matching(jack_list));

  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_EQ(0, gpio_switch_open_called);
  EXPECT_EQ(0, cras_system_add_select_fd_called);
  EXPECT_EQ(0, cras_system_rm_select_fd_called);
}

TEST(AlsaJacks, CreateOneHpJack) {
  std::string elem_names[] = {
    "asdf",
    "Headphone Jack, klasdjf",
    "Mic Jack",
  };
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  jack_list = run_test_with_elem_list(CRAS_STREAM_OUTPUT,
                                      elem_names,
                                      0,
                                      NULL,
                                      ARRAY_SIZE(elem_names),
                                      0,
                                      1);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  ASSERT_NE(reinterpret_cast<snd_hctl_elem_callback_t>(NULL),
            snd_hctl_elem_set_callback_value);
  EXPECT_EQ(1, snd_hctl_elem_set_callback_called);

  snd_hctl_elem_get_hctl_return_value = reinterpret_cast<snd_hctl_t *>(0x33);
  snd_hctl_elem_get_name_called = 0;
  snd_ctl_elem_value_get_boolean_return_value = 1;
  snd_hctl_elem_set_callback_value(
      reinterpret_cast<snd_hctl_elem_t *>(&elem_names[1]), 0);
  EXPECT_EQ(1, snd_hctl_elem_get_name_called);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);
  EXPECT_EQ(fake_jack_cb_arg, fake_jack_cb_data);
  EXPECT_EQ(reinterpret_cast<snd_hctl_elem_t *>(&elem_names[1]),
            snd_hctl_elem_set_callback_obj);

  fake_jack_cb_called = 0;
  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);

  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(2, snd_hctl_elem_set_callback_called);
  EXPECT_EQ(reinterpret_cast<snd_hctl_elem_callback_t>(NULL),
            snd_hctl_elem_set_callback_value);
}

TEST(AlsaJacks, CreateOneMicJack) {
  static std::string elem_names[] = {
    "asdf",
    "Headphone Jack",
    "HDMI/DP,pcm=5 Jack",
    "HDMI/DP,pcm=6 Jack",
    "Mic Jack",
  };
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  jack_list = run_test_with_elem_list(CRAS_STREAM_INPUT,
                                      elem_names,
                                      0,
                                      NULL,
                                      ARRAY_SIZE(elem_names),
                                      0,
                                      1);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  ASSERT_NE(reinterpret_cast<snd_hctl_elem_callback_t>(NULL),
            snd_hctl_elem_set_callback_value);
  EXPECT_EQ(1, snd_hctl_elem_set_callback_called);

  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(0, cras_system_rm_select_fd_called);
  EXPECT_EQ(2, snd_hctl_elem_set_callback_called);
  EXPECT_EQ(reinterpret_cast<snd_hctl_elem_callback_t>(NULL),
            snd_hctl_elem_set_callback_value);
}

TEST(AlsaJacks, CreateHDMIJacksWithELD) {
  std::string elem_names[] = {
    "asdf",
    "HDMI/DP,pcm=3 Jack",
    "ELD",
    "HDMI/DP,pcm=4 Jack"
  };
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  snd_hctl_elem_get_device_return_val = 3;

  jack_list = run_test_with_elem_list(
      CRAS_STREAM_OUTPUT,
      elem_names,
      3,
      NULL,
      ARRAY_SIZE(elem_names),
      1,
      1);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);

  /* Assert get device is called for the ELD control */
  EXPECT_EQ(1, snd_hctl_elem_get_device_called);
  cras_alsa_jack_list_destroy(jack_list);
}

TEST(AlsaJacks, CreateOneHpTwoHDMIJacks) {
  std::string elem_names[] = {
    "asdf",
    "Headphone Jack, klasdjf",
    "HDMI/DP,pcm=5 Jack",
    "HDMI/DP,pcm=6 Jack",
    "Mic Jack",
  };
  struct cras_alsa_jack_list *jack_list;

  ResetStubData();
  ucm_get_dev_for_jack_return = true;
  jack_list = run_test_with_elem_list(
      CRAS_STREAM_OUTPUT,
      elem_names,
      5,
      reinterpret_cast<struct cras_use_case_mgr *>(0x55),
      ARRAY_SIZE(elem_names),
      1,
      1);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);

  snd_hctl_elem_get_hctl_return_value = reinterpret_cast<snd_hctl_t *>(0x33);
  snd_hctl_elem_get_name_called = 0;
  snd_ctl_elem_value_get_boolean_return_value = 1;
  snd_hctl_elem_set_callback_value(
      reinterpret_cast<snd_hctl_elem_t *>(&elem_names[2]), 0);
  EXPECT_EQ(1, snd_hctl_elem_get_name_called);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);
  EXPECT_EQ(fake_jack_cb_arg, fake_jack_cb_data);
  EXPECT_EQ(reinterpret_cast<snd_hctl_elem_t *>(&elem_names[2]),
            snd_hctl_elem_set_callback_obj);

  fake_jack_cb_called = 0;
  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);

  cras_alsa_jack_list_destroy(jack_list);
}

TEST(AlsaJacks, CreateHCTLHeadphoneJackFromUCM) {
  std::string elem_names[] = {
    "HP/DP,pcm=5 Jack",
    "Headphone Jack",
  };
  struct cras_alsa_jack_list *jack_list;
  struct ucm_section *section;

  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                               "Headphone Jack", "hctl");

  ResetStubData();
  ucm_get_dev_for_jack_return = true;

  jack_list = run_test_with_section(
      CRAS_STREAM_OUTPUT,
      elem_names,
      ARRAY_SIZE(elem_names),
      5,
      reinterpret_cast<struct cras_use_case_mgr *>(0x55),
      section,
      0,
      1);
  ASSERT_NE(reinterpret_cast<struct cras_alsa_jack_list *>(NULL), jack_list);

  snd_hctl_elem_get_hctl_return_value = reinterpret_cast<snd_hctl_t *>(0x33);
  snd_ctl_elem_value_get_boolean_return_value = 1;
  snd_hctl_elem_set_callback_value(
      reinterpret_cast<snd_hctl_elem_t *>(&elem_names[1]), 0);
  EXPECT_EQ(1, snd_hctl_elem_get_name_called);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);
  EXPECT_EQ(fake_jack_cb_arg, fake_jack_cb_data);
  EXPECT_EQ(reinterpret_cast<snd_hctl_elem_t *>(&elem_names[1]),
            snd_hctl_elem_set_callback_obj);

  fake_jack_cb_called = 0;
  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);

  ucm_section_free_list(section);
  cras_alsa_jack_list_destroy(jack_list);
}

TEST(AlsaJacks, CreateGPIOHeadphoneJackFromUCM) {
  struct cras_alsa_jack_list *jack_list;
  struct cras_alsa_jack *jack;
  struct ucm_section *section;

  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                               "c1 Headphone Jack", "gpio");

  ResetStubData();
  gpio_switch_list_for_each_dev_names.push_back("some-other-device");
  gpio_switch_list_for_each_dev_names.push_back("c1 Headphone Jack");
  eviocbit_ret[LONG(SW_HEADPHONE_INSERT)] |= 1 << OFF(SW_HEADPHONE_INSERT);
  gpio_switch_eviocgbit_fd = 2;
  snd_hctl_first_elem_return_val = NULL;
  jack_list = cras_alsa_jack_list_create(0, "c1", 0, 1,
                                         fake_mixer,
                                         NULL, fake_hctl,
                                         CRAS_STREAM_OUTPUT,
                                         fake_jack_cb,
                                         fake_jack_cb_arg);
  ASSERT_NE(static_cast<struct cras_alsa_jack_list *>(NULL), jack_list);
  EXPECT_EQ(0, cras_alsa_jack_list_add_jack_for_section(
                   jack_list, section, &jack));
  EXPECT_EQ(1, gpio_switch_list_for_each_called);
  EXPECT_GT(gpio_switch_open_called, 1);
  EXPECT_EQ(1, gpio_switch_eviocgsw_called);
  EXPECT_GT(gpio_switch_eviocgbit_called, 1);
  EXPECT_EQ(1, cras_system_add_select_fd_called);
  EXPECT_EQ(1, cras_alsa_mixer_get_control_for_section_called);

  fake_jack_cb_called = 0;
  ucm_get_dev_for_jack_return = true;
  cras_alsa_jack_list_report(jack_list);
  EXPECT_EQ(1, fake_jack_cb_plugged);
  EXPECT_EQ(1, fake_jack_cb_called);
  EXPECT_EQ(fake_jack_cb_arg, fake_jack_cb_data);

  ucm_section_free_list(section);
  cras_alsa_jack_list_destroy(jack_list);
  EXPECT_EQ(1, cras_system_rm_select_fd_called);
}

TEST(AlsaJacks, BadJackTypeFromUCM) {
  std::string elem_names[] = {
    "HP/DP,pcm=5 Jack",
    "Headphone Jack",
  };
  struct cras_alsa_jack_list *jack_list;
  struct ucm_section *section;

  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                               "Headphone Jack", "badtype");

  ResetStubData();
  ucm_get_dev_for_jack_return = true;

  jack_list = run_test_with_section(
      CRAS_STREAM_OUTPUT,
      elem_names,
      ARRAY_SIZE(elem_names),
      5,
      reinterpret_cast<struct cras_use_case_mgr *>(0x55),
      section,
      -22,
      1);
  EXPECT_EQ(reinterpret_cast<struct cras_alsa_jack_list *>(NULL), jack_list);

  ucm_section_free_list(section);
}

TEST(AlsaJacks, NoJackTypeFromUCM) {
  std::string elem_names[] = {
    "HP/DP,pcm=5 Jack",
    "Headphone Jack",
  };
  struct cras_alsa_jack_list *jack_list;
  struct ucm_section *section;

  section = ucm_section_create("Headphone", 0, CRAS_STREAM_OUTPUT,
                               "Headphone Jack", NULL);

  ResetStubData();
  ucm_get_dev_for_jack_return = true;

  jack_list = run_test_with_section(
      CRAS_STREAM_OUTPUT,
      elem_names,
      ARRAY_SIZE(elem_names),
      5,
      reinterpret_cast<struct cras_use_case_mgr *>(0x55),
      section,
      -22,
      1);
  EXPECT_EQ(reinterpret_cast<struct cras_alsa_jack_list *>(NULL), jack_list);

  ucm_section_free_list(section);
}

/* Stubs */

extern "C" {

// From cras_system_state
int cras_system_add_select_fd(int fd,
			      void (*callback)(void *data),
			      void *callback_data)
{
  cras_system_add_select_fd_called++;
  cras_system_add_select_fd_values.push_back(fd);
  return 0;
}
void cras_system_rm_select_fd(int fd)
{
  cras_system_rm_select_fd_called++;
  cras_system_rm_select_fd_values.push_back(fd);
}

// From alsa-lib hcontrol.c
unsigned int snd_hctl_elem_get_device(const snd_hctl_elem_t *obj) {
  snd_hctl_elem_get_device_called = 1;
  return snd_hctl_elem_get_device_return_val;
}
snd_hctl_elem_t *snd_hctl_first_elem(snd_hctl_t *hctl) {
  snd_hctl_first_elem_called++;

  /* When first elem is called, restored the poped ret values */
  while (!snd_hctl_elem_next_ret_vals_poped.empty()) {
    snd_hctl_elem_t *tmp = snd_hctl_elem_next_ret_vals_poped.back();
    snd_hctl_elem_next_ret_vals_poped.pop_back();
    snd_hctl_elem_next_ret_vals.push_back(tmp);
  }
  return snd_hctl_first_elem_return_val;
}
snd_hctl_elem_t *snd_hctl_elem_next(snd_hctl_elem_t *elem) {
  snd_hctl_elem_next_called++;
  if (snd_hctl_elem_next_ret_vals.empty())
    return NULL;
  snd_hctl_elem_t *ret_elem = snd_hctl_elem_next_ret_vals.back();
  snd_hctl_elem_next_ret_vals.pop_back();
  snd_hctl_elem_next_ret_vals_poped.push_back(ret_elem);
  return ret_elem;
}
const char *snd_hctl_elem_get_name(const snd_hctl_elem_t *obj) {
  snd_hctl_elem_get_name_called++;
  const std::string *name = reinterpret_cast<const std::string *>(obj);
  return name->c_str();
}
snd_ctl_elem_iface_t snd_hctl_elem_get_interface(const snd_hctl_elem_t *obj) {
  return SND_CTL_ELEM_IFACE_CARD;
}
void snd_hctl_elem_set_callback(snd_hctl_elem_t *obj,
                                snd_hctl_elem_callback_t val) {
  snd_hctl_elem_set_callback_called++;
  snd_hctl_elem_set_callback_obj = obj;
  snd_hctl_elem_set_callback_value = val;
}
void snd_hctl_elem_set_callback_private(snd_hctl_elem_t *obj, void * val) {
  snd_hctl_elem_set_callback_private_called++;
  snd_hctl_elem_set_callback_private_value = val;
}
void *snd_hctl_elem_get_callback_private(const snd_hctl_elem_t *obj) {
  return snd_hctl_elem_set_callback_private_value;
}
snd_hctl_t *snd_hctl_elem_get_hctl(snd_hctl_elem_t *elem) {
  snd_hctl_elem_get_hctl_called++;
  return snd_hctl_elem_get_hctl_return_value;
}
int snd_hctl_elem_read(snd_hctl_elem_t *elem, snd_ctl_elem_value_t * value) {
  return 0;
}
snd_hctl_elem_t *snd_hctl_find_elem(snd_hctl_t *hctl,
                                    const snd_ctl_elem_id_t *id) {
  const size_t* index = reinterpret_cast<const size_t*>(id);
  snd_hctl_find_elem_called++;
  if (*index < snd_hctl_find_elem_return_vals.size())
    return snd_hctl_find_elem_return_vals[*index];
  return NULL;
}
void snd_ctl_elem_id_set_interface(snd_ctl_elem_id_t *obj,
                                   snd_ctl_elem_iface_t val) {
}
void snd_ctl_elem_id_set_device(snd_ctl_elem_id_t *obj, unsigned int val) {
}
void snd_ctl_elem_id_set_name(snd_ctl_elem_id_t *obj, const char *val) {
  size_t *obj_id = reinterpret_cast<size_t*>(obj);
  std::map<std::string, size_t>::iterator id_name_it =
      snd_ctl_elem_id_set_name_map.find(val);
  if (id_name_it != snd_ctl_elem_id_set_name_map.end())
    *obj_id = id_name_it->second;
  else
    *obj_id = INT_MAX;
}

// From alsa-lib control.c
int snd_ctl_elem_value_get_boolean(const snd_ctl_elem_value_t *obj,
                                   unsigned int idx) {
  snd_ctl_elem_value_get_boolean_called++;
  return snd_ctl_elem_value_get_boolean_return_value;
}

// From cras_alsa_mixer
struct mixer_control *cras_alsa_mixer_get_output_matching_name(
    const struct cras_alsa_mixer *cras_mixer,
    size_t device_index,
    const char * const name)
{
  cras_alsa_mixer_get_output_matching_name_called++;
  return cras_alsa_mixer_get_output_matching_name_return_value;
}

struct mixer_control *cras_alsa_mixer_get_input_matching_name(
    struct cras_alsa_mixer *cras_mixer,
    const char *control_name)
{
  cras_alsa_mixer_get_input_matching_name_called++;
  return cras_alsa_mixer_get_input_matching_name_return_value;
}

struct mixer_control *cras_alsa_mixer_get_control_for_section(
    struct cras_alsa_mixer *cras_mixer,
    struct ucm_section *section)
{
  cras_alsa_mixer_get_control_for_section_called++;
  return cras_alsa_mixer_get_control_for_section_return_value;
}

int gpio_switch_eviocgbit(int fd, void *buf, size_t n_bytes)
{
  unsigned char *p = (unsigned char *)buf;

  /* Returns >= 0 if 'sw' is supported, negative if not.
   *
   *  Set the bit corresponding to 'sw' in 'buf'.  'buf' must have
   *  been allocated by the caller to accommodate this.
   */
  if (fd  == gpio_switch_eviocgbit_fd)
    memcpy(p, eviocbit_ret, n_bytes);
  else
    memset(p, 0, n_bytes);

  gpio_switch_eviocgbit_called++;
  return 1;
}

int gpio_switch_eviocgsw(int fd, void *bits, size_t n_bytes)
{
  /* Bits set to '1' indicate a switch is enabled.
   * Bits set to '0' indicate a switch is disabled
   */
  gpio_switch_eviocgsw_called++;
  memset(bits, 0xff, n_bytes);
  return 1;
}

int gpio_switch_read(int fd, void *buf, size_t n_bytes)
{
  /* This function is only invoked when the 'switch has changed'
   * callback is invoked.  That code is not exercised by this
   * unittest.
   */
  assert(0);
  return 0;
}

int gpio_switch_open(const char *pathname)
{
  ++gpio_switch_open_called;
  if (strstr(pathname, "event2"))
	  return 2;
  if (strstr(pathname, "event3"))
	  return 3;
  return 0;
}

void gpio_switch_list_for_each(gpio_switch_list_callback callback, void *arg)
{
  size_t i = 0;

  ++gpio_switch_list_for_each_called;

  while (i < gpio_switch_list_for_each_dev_names.size() &&
         i < gpio_switch_list_for_each_dev_paths.size()) {
    callback(gpio_switch_list_for_each_dev_paths[i].c_str(),
             gpio_switch_list_for_each_dev_names[i].c_str(),
             arg);
    i++;
  }
}

int ucm_set_enabled(
    struct cras_use_case_mgr *mgr, const char *dev, int enable) {
  ucm_set_enabled_value = enable;
  return 0;
}

char *ucm_get_cap_control(struct cras_use_case_mgr *mgr, const char *ucm_dev) {
  ++ucm_get_cap_control_called;
  return ucm_get_cap_control_value;
}

char *ucm_get_dev_for_jack(struct cras_use_case_mgr *mgr, const char *jack,
                           CRAS_STREAM_DIRECTION direction) {
  ++ucm_get_dev_for_jack_called;
  if (ucm_get_dev_for_jack_return)
    return static_cast<char*>(malloc(1)); // Will be freed in jack_list_destroy.
  return NULL;
}

const char *ucm_get_dsp_name(struct cras_use_case_mgr *mgr, const char *ucm_dev,
                       int direction) {
  ++ucm_get_dsp_name_called;
  return NULL;
}

const char *ucm_get_edid_file_for_dev(struct cras_use_case_mgr *mgr,
				      const char *dev) {
  return edid_file_ret;
}

const char *ucm_get_override_type_name(struct cras_use_case_mgr *mgr,
                                       const char *ucm_dev)
{
  ++ucm_get_override_type_name_called;
  return NULL;
}

const char *ucm_get_device_name_for_dev(struct cras_use_case_mgr *mgr,
                                        const char *dev,
                                        enum CRAS_STREAM_DIRECTION direction)
{
  return ucm_get_device_name_for_dev_value;
}

cras_timer *cras_tm_create_timer(
    cras_tm *tm,
    unsigned int ms,
    void (*cb)(cras_timer *t, void *data),
    void *cb_data) {
  return reinterpret_cast<cras_timer*>(0x55);
}

void cras_tm_cancel_timer(cras_tm *tm, cras_timer *t) {
}

cras_tm *cras_system_state_get_tm() {
  return reinterpret_cast<cras_tm*>(0x66);
}

int edid_valid(const unsigned char *edid_data) {
  return 0;
}

int edid_lpcm_support(const unsigned char *edid_data, int ext) {
  return 0;
}

int edid_get_monitor_name(const unsigned char *edid_data,
                          char *buf,
                          unsigned int buf_size) {
  return 0;
}

// Overwrite this function so unittest can run without 2 seconds of wait
// in find_gpio_jacks.
int wait_for_dev_input_access() {
  return 0;
}

} /* extern "C" */

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  openlog(NULL, LOG_PERROR, LOG_USER);
  return RUN_ALL_TESTS();
}
