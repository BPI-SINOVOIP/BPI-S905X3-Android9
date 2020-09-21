// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "cras_config.h"
#include "cras_dsp_module.h"
#include "cras_dsp_pipeline.h"

#define MAX_MODULES 10
#define MAX_MOCK_PORTS 30
#define FILENAME_TEMPLATE "DspIniTest.XXXXXX"

static void fill_test_data(int16_t *data, size_t size)
{
  for (size_t i = 0; i < size; i++)
    data[i] = i;
}

static void verify_processed_data(int16_t *data, size_t size, int times)
{
  /* Each time the audio data flow through the mock plugin, the data
   * will be multiplied by 2 in module->run() below, so if there are n
   * plugins, the data will be multiplied by (1 << n). */
  int multiples = (1 << times);
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(i * multiples, data[i]);
    if ((int16_t)i * multiples != data[i])
      return;
  }
}

struct data {
  const char *title;
  int nr_ports;
  port_direction port_dir[MAX_MOCK_PORTS];
  int nr_in_audio;
  int nr_in_control;
  int nr_out_audio;
  int nr_out_control;
  int in_audio[MAX_MOCK_PORTS];
  int in_control[MAX_MOCK_PORTS];
  int out_audio[MAX_MOCK_PORTS];
  int out_control[MAX_MOCK_PORTS];
  int properties;

  int instantiate_called;
  int sample_rate;

  int connect_port_called[MAX_MOCK_PORTS];
  float *data_location[MAX_MOCK_PORTS];

  int run_called;
  float input[MAX_MOCK_PORTS];
  float output[MAX_MOCK_PORTS];

  int sample_count;

  int get_delay_called;
  int deinstantiate_called;
  int free_module_called;
  int get_properties_called;
};

static int instantiate(struct dsp_module *module, unsigned long sample_rate)
{
  struct data *data = (struct data *)module->data;
  data->instantiate_called++;
  data->sample_rate = sample_rate;
  return 0;
}

static void connect_port(struct dsp_module *module, unsigned long port,
                         float *data_location)
{
  struct data *data = (struct data *)module->data;
  data->connect_port_called[port]++;
  data->data_location[port] = data_location;
}

static int get_delay(struct dsp_module *module)
{
  struct data *data = (struct data *)module->data;
  data->get_delay_called++;

  /* If the module title is "mN", then use N as the delay. */
  int delay = 0;
  sscanf(data->title, "m%d", &delay);
  return delay;
}

static void run(struct dsp_module *module, unsigned long sample_count)
{
  struct data *data =  (struct data *)module->data;
  data->run_called++;
  data->sample_count = sample_count;

  for (int i = 0; i < data->nr_ports; i++) {
    if (data->port_dir[i] == PORT_INPUT)
      data->input[i] = *data->data_location[i];
  }

  /* copy the control port data */
  for (int i = 0; i < std::min(data->nr_in_control, data->nr_out_control); i++) {
    int from = data->in_control[i];
    int to = data->out_control[i];
    data->data_location[to][0] = data->data_location[from][0];
  }

  /* multiply the audio port data by 2 */
  for (int i = 0; i < std::min(data->nr_in_audio, data->nr_out_audio); i++) {
    int from = data->in_audio[i];
    int to = data->out_audio[i];
    for (unsigned int j = 0; j < sample_count; j++)
      data->data_location[to][j] = data->data_location[from][j] * 2;
  }
}

static void deinstantiate(struct dsp_module *module)
{
  struct data *data = (struct data *)module->data;
  data->deinstantiate_called++;
}

static void free_module(struct dsp_module *module)
{
  struct data *data = (struct data *)module->data;
  data->free_module_called++;
}

static void really_free_module(struct dsp_module *module)
{
    struct data *data = (struct data *)module->data;
    free(data);
    free(module);
}

static int get_properties(struct dsp_module *module)
{
  struct data *data = (struct data *)module->data;
  data->get_properties_called++;
  return data->properties;
}
static void dump(struct dsp_module *module, struct dumper *d) {}

static struct dsp_module *create_mock_module(struct plugin *plugin)
{
  struct data *data;
  struct dsp_module *module;

  data =  (struct data *)calloc(1, sizeof(struct data));
  data->title = plugin->title;
  data->nr_ports = ARRAY_COUNT(&plugin->ports);
  for (int i = 0; i < data->nr_ports; i++) {
    struct port *port = ARRAY_ELEMENT(&plugin->ports, i);
    data->port_dir[i] = port->direction;

    if (port->direction == PORT_INPUT) {
      if (port->type == PORT_AUDIO)
        data->in_audio[data->nr_in_audio++] = i;
      else
        data->in_control[data->nr_in_control++] = i;
    } else {
      if (port->type == PORT_AUDIO)
        data->out_audio[data->nr_out_audio++] = i;
      else
        data->out_control[data->nr_out_control++] = i;
    }
  }
  if (strcmp(plugin->label, "inplace_broken") == 0) {
    data->properties = MODULE_INPLACE_BROKEN;
  } else {
    data->properties = 0;
  }

  module = (struct dsp_module*)calloc(1, sizeof(struct dsp_module));
  module->data = data;
  module->instantiate = &instantiate;
  module->connect_port = &connect_port;
  module->get_delay = &get_delay;
  module->run = &run;
  module->deinstantiate = &deinstantiate;
  module->free_module = &free_module;
  module->get_properties = &get_properties;
  module->dump = &dump;
  return module;
}

static struct dsp_module *modules[MAX_MODULES];
static int num_modules;
static struct dsp_module *find_module(const char *name)
{
  for (int i = 0; i < num_modules; i++) {
    struct data *data = (struct data *)modules[i]->data;
    if (strcmp(name, data->title) == 0)
      return modules[i];
  }
  return NULL;
}

extern "C" {
struct dsp_module *cras_dsp_module_load_ladspa(struct plugin *plugin)
{
  return NULL;
}
struct dsp_module *cras_dsp_module_load_builtin(struct plugin *plugin)
{
  struct dsp_module *module = create_mock_module(plugin);
  modules[num_modules++] = module;
  return module;
}
}

namespace {

class DspPipelineTestSuite : public testing::Test {
 protected:
  virtual void SetUp() {
    num_modules = 0;
    strcpy(filename,  FILENAME_TEMPLATE);
    int fd = mkstemp(filename);
    fp = fdopen(fd, "w");
  }

  virtual void TearDown() {
    CloseFile();
    unlink(filename);
  }

  virtual void CloseFile() {
    if (fp) {
      fclose(fp);
      fp = NULL;
    }
  }

  char filename[sizeof(FILENAME_TEMPLATE) + 1];
  FILE *fp;
};

TEST_F(DspPipelineTestSuite, Simple) {
  const char *content =
      "[M1]\n"
      "library=builtin\n"
      "label=source\n"
      "purpose=capture\n"
      "output_0={audio}\n"
      "output_1=<control>\n"
      "input_2=3.0\n"
      "[M2]\n"
      "library=builtin\n"
      "label=sink\n"
      "purpose=capture\n"
      "input_0=<control>\n"
      "input_1={audio}\n"
      "\n";
  fprintf(fp, "%s", content);
  CloseFile();

  struct cras_expr_env env = CRAS_EXPR_ENV_INIT;
  struct ini *ini = cras_dsp_ini_create(filename);
  ASSERT_TRUE(ini);
  struct pipeline *p = cras_dsp_pipeline_create(ini, &env, "capture");
  ASSERT_TRUE(p);
  ASSERT_EQ(0, cras_dsp_pipeline_load(p));

  ASSERT_EQ(2, num_modules);
  struct dsp_module *m1 = find_module("m1");
  struct dsp_module *m2 = find_module("m2");
  ASSERT_TRUE(m1);
  ASSERT_TRUE(m2);

  ASSERT_EQ(1, cras_dsp_pipeline_get_num_input_channels(p));
  ASSERT_EQ(0, cras_dsp_pipeline_instantiate(p, 48000));

  struct data *d1 = (struct data *)m1->data;
  struct data *d2 = (struct data *)m2->data;

  /* check m1 */
  ASSERT_STREQ("m1", d1->title);
  ASSERT_EQ(3, d1->nr_ports);
  ASSERT_EQ(PORT_OUTPUT, d1->port_dir[0]);
  ASSERT_EQ(PORT_OUTPUT, d1->port_dir[1]);
  ASSERT_EQ(PORT_INPUT, d1->port_dir[2]);
  ASSERT_EQ(1, d1->instantiate_called);
  ASSERT_EQ(1, d1->get_delay_called);
  ASSERT_EQ(48000, d1->sample_rate);
  ASSERT_EQ(1, d1->connect_port_called[0]);
  ASSERT_EQ(1, d1->connect_port_called[1]);
  ASSERT_EQ(1, d1->connect_port_called[2]);
  ASSERT_TRUE(d1->data_location[0]);
  ASSERT_TRUE(d1->data_location[1]);
  ASSERT_TRUE(d1->data_location[2]);
  ASSERT_EQ(0, d1->run_called);
  ASSERT_EQ(0, d1->deinstantiate_called);
  ASSERT_EQ(0, d1->free_module_called);
  ASSERT_EQ(1, d1->get_properties_called);

  /* check m2 */
  ASSERT_STREQ("m2", d2->title);
  ASSERT_EQ(2, d2->nr_ports);
  ASSERT_EQ(PORT_INPUT, d2->port_dir[0]);
  ASSERT_EQ(PORT_INPUT, d2->port_dir[1]);
  ASSERT_EQ(1, d2->instantiate_called);
  ASSERT_EQ(1, d2->get_delay_called);
  ASSERT_EQ(48000, d2->sample_rate);
  ASSERT_EQ(1, d2->connect_port_called[0]);
  ASSERT_EQ(1, d2->connect_port_called[1]);
  ASSERT_TRUE(d2->data_location[0]);
  ASSERT_TRUE(d2->data_location[1]);
  ASSERT_EQ(0, d2->run_called);
  ASSERT_EQ(0, d2->deinstantiate_called);
  ASSERT_EQ(0, d2->free_module_called);
  ASSERT_EQ(1, d2->get_properties_called);

  /* check the buffer is shared */
  ASSERT_EQ(d1->data_location[0], d2->data_location[1]);
  ASSERT_EQ(d1->data_location[1], d2->data_location[0]);
  ASSERT_EQ(1, cras_dsp_pipeline_get_peak_audio_buffers(p));

  d1->data_location[0][0] = 100;
  cras_dsp_pipeline_run(p, DSP_BUFFER_SIZE);
  ASSERT_EQ(1, d1->run_called);
  ASSERT_EQ(1, d2->run_called);
  ASSERT_EQ(3, d1->input[2]);
  ASSERT_EQ(3, d2->input[0]);
  ASSERT_EQ(100, d2->input[1]);

  d1->data_location[0][0] = 1000;
  cras_dsp_pipeline_run(p, DSP_BUFFER_SIZE);
  ASSERT_EQ(2, d1->run_called);
  ASSERT_EQ(2, d2->run_called);
  ASSERT_EQ(3, d1->input[2]);
  ASSERT_EQ(3, d2->input[0]);
  ASSERT_EQ(1000, d2->input[1]);

  cras_dsp_pipeline_deinstantiate(p);
  ASSERT_EQ(1, d1->deinstantiate_called);
  ASSERT_EQ(1, d2->deinstantiate_called);

  cras_dsp_pipeline_free(p);
  ASSERT_EQ(1, d1->free_module_called);
  ASSERT_EQ(1, d2->free_module_called);

  cras_dsp_ini_free(ini);
  cras_expr_env_free(&env);

  really_free_module(m1);
  really_free_module(m2);
}

TEST_F(DspPipelineTestSuite, Complex) {
  /*
   *                   / --(b)-- 2 --(c)-- \
   *   0 ==(a0, a1)== 1                     4 ==(f0,f1)== 5
   *                   \ --(d)-- 3 --(e)-- /
   *
   *
   *                     --(g)-- 6 --(h)--
   */

  const char *content =
      "[M6]\n"
      "library=builtin\n"
      "label=foo\n"
      "input_0={g}\n"
      "output_1={h}\n"
      "[M5]\n"
      "library=builtin\n"
      "label=sink\n"
      "purpose=playback\n"
      "input_0={f0}\n"
      "input_1={f1}\n"
      "[M4]\n"
      "library=builtin\n"
      "label=foo\n"
      "disable=(equal? output_device \"HDMI\")\n"
      "input_0=3.14\n"
      "input_1={c}\n"
      "output_2={f0}\n"
      "input_3={e}\n"
      "output_4={f1}\n"
      "[M3]\n"
      "library=builtin\n"
      "label=foo\n"
      "input_0={d}\n"
      "output_1={e}\n"
      "[M2]\n"
      "library=builtin\n"
      "label=inplace_broken\n"
      "input_0={b}\n"
      "output_1={c}\n"
      "[M1]\n"
      "library=builtin\n"
      "label=foo\n"
      "disable=(equal? output_device \"USB\")\n"
      "input_0={a0}\n"
      "input_1={a1}\n"
      "output_2={b}\n"
      "output_3={d}\n"
      "[M0]\n"
      "library=builtin\n"
      "label=source\n"
      "purpose=playback\n"
      "output_0={a0}\n"
      "output_1={a1}\n";
  fprintf(fp, "%s", content);
  CloseFile();

  struct cras_expr_env env = CRAS_EXPR_ENV_INIT;
  cras_expr_env_install_builtins(&env);
  cras_expr_env_set_variable_string(&env, "output_device", "HDMI");
  cras_expr_env_set_variable_boolean(&env, "swap_lr_disabled", 1);

  struct ini *ini = cras_dsp_ini_create(filename);
  ASSERT_TRUE(ini);
  struct pipeline *p = cras_dsp_pipeline_create(ini, &env, "playback");
  ASSERT_TRUE(p);
  ASSERT_EQ(0, cras_dsp_pipeline_load(p));

  ASSERT_EQ(5, num_modules);  /* one not connected, one disabled */
  struct dsp_module *m0 = find_module("m0");
  struct dsp_module *m1 = find_module("m1");
  struct dsp_module *m2 = find_module("m2");
  struct dsp_module *m3 = find_module("m3");
  struct dsp_module *m5 = find_module("m5");

  ASSERT_TRUE(m0);
  ASSERT_TRUE(m1);
  ASSERT_TRUE(m2);
  ASSERT_TRUE(m3);
  ASSERT_FALSE(find_module("m4"));
  ASSERT_TRUE(m5);
  ASSERT_FALSE(find_module("m6"));

  ASSERT_EQ(2, cras_dsp_pipeline_get_num_input_channels(p));
  ASSERT_EQ(0, cras_dsp_pipeline_instantiate(p, 48000));

  struct data *d0 = (struct data *)m0->data;
  struct data *d1 = (struct data *)m1->data;
  struct data *d2 = (struct data *)m2->data;
  struct data *d3 = (struct data *)m3->data;
  struct data *d5 = (struct data *)m5->data;

  /*
   *                   / --(b)-- 2 --(c)-- \
   *   0 ==(a0, a1)== 1                     4 ==(f0,f1)== 5
   *                   \ --(d)-- 3 --(e)-- /
   *
   *
   *                     --(g)-- 6 --(h)--
   */

  ASSERT_EQ(d0->data_location[0], d1->data_location[0]);
  ASSERT_EQ(d0->data_location[1], d1->data_location[1]);
  ASSERT_EQ(d1->data_location[2], d2->data_location[0]);
  ASSERT_EQ(d1->data_location[3], d3->data_location[0]);
  ASSERT_NE(d2->data_location[0], d2->data_location[1]); /* inplace-broken */
  ASSERT_EQ(d2->data_location[1], d5->data_location[0]); /* m4 is disabled */
  ASSERT_EQ(d3->data_location[1], d5->data_location[1]);

  /* need 3 buffers because m2 has inplace-broken flag */
  ASSERT_EQ(3, cras_dsp_pipeline_get_peak_audio_buffers(p));

  int16_t *samples = new int16_t[DSP_BUFFER_SIZE];
  fill_test_data(samples, DSP_BUFFER_SIZE);
  cras_dsp_pipeline_apply(p, (uint8_t*)samples, 100);
  /* the data flow through 2 plugins because m4 is disabled. */
  verify_processed_data(samples, 100, 2);
  delete[] samples;

  ASSERT_EQ(1, d1->run_called);
  ASSERT_EQ(1, d3->run_called);

  /* check m5 */
  ASSERT_EQ(1, d5->run_called);
  ASSERT_EQ(100, d5->sample_count);

  /* re-instantiate */
  ASSERT_EQ(1, d5->instantiate_called);
  ASSERT_EQ(1, d5->get_delay_called);
  ASSERT_EQ(1 + 3 + 5, cras_dsp_pipeline_get_delay(p));

  cras_dsp_pipeline_deinstantiate(p);
  cras_dsp_pipeline_instantiate(p, 44100);

  ASSERT_EQ(1, d5->deinstantiate_called);
  ASSERT_EQ(2, d5->instantiate_called);
  ASSERT_EQ(2, d5->get_delay_called);
  ASSERT_EQ(1 + 3 + 5, cras_dsp_pipeline_get_delay(p));
  ASSERT_EQ(0, d5->free_module_called);
  ASSERT_EQ(44100, d5->sample_rate);
  ASSERT_EQ(2, d5->connect_port_called[0]);
  ASSERT_EQ(2, d5->connect_port_called[1]);

  cras_dsp_pipeline_free(p);
  cras_dsp_ini_free(ini);
  cras_expr_env_free(&env);

  really_free_module(m0);
  really_free_module(m1);
  really_free_module(m2);
  really_free_module(m3);
  really_free_module(m5);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
