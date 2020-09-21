// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "cras_dsp.h"
#include "cras_dsp_module.h"

#define FILENAME_TEMPLATE "DspTest.XXXXXX"

namespace {

extern "C" {
struct dsp_module *cras_dsp_module_load_ladspa(struct plugin *plugin)
{
  return NULL;
}
}

class DspTestSuite : public testing::Test {
 protected:
  virtual void SetUp() {
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

TEST_F(DspTestSuite, Simple) {
  const char *content =
      "[M1]\n"
      "library=builtin\n"
      "label=source\n"
      "purpose=capture\n"
      "output_0={audio}\n"
      "disable=(not (equal? variable \"foo\"))\n"
      "[M2]\n"
      "library=builtin\n"
      "label=sink\n"
      "purpose=capture\n"
      "input_0={audio}\n"
      "\n";
  fprintf(fp, "%s", content);
  CloseFile();

  cras_dsp_init(filename);
  struct cras_dsp_context *ctx1, *ctx3, *ctx4;
  ctx1 = cras_dsp_context_new(44100, "playback");  /* wrong purpose */
  ctx3 = cras_dsp_context_new(44100, "capture");
  ctx4 = cras_dsp_context_new(44100, "capture");

  cras_dsp_set_variable_string(ctx1, "variable", "foo");
  cras_dsp_set_variable_string(ctx3, "variable", "bar");  /* wrong value */
  cras_dsp_set_variable_string(ctx4, "variable", "foo");

  cras_dsp_load_pipeline(ctx1);
  cras_dsp_load_pipeline(ctx3);
  cras_dsp_load_pipeline(ctx4);

  /* only ctx4 should load the pipeline successfully */
  ASSERT_EQ(NULL, cras_dsp_get_pipeline(ctx1));
  ASSERT_EQ(NULL, cras_dsp_get_pipeline(ctx3));

  struct pipeline *pipeline = cras_dsp_get_pipeline(ctx4);
  ASSERT_TRUE(pipeline);
  cras_dsp_put_pipeline(ctx4);

  /* change the variable to a wrong value, and we should fail to reload. */
  cras_dsp_set_variable_string(ctx4, "variable", "bar");
  cras_dsp_load_pipeline(ctx4);
  ASSERT_EQ(NULL, cras_dsp_get_pipeline(ctx4));

  /* change the variable back, and we should reload successfully. */
  cras_dsp_set_variable_string(ctx4, "variable", "foo");
  cras_dsp_reload_ini();
  ASSERT_TRUE(cras_dsp_get_pipeline(ctx4));

  cras_dsp_context_free(ctx1);
  cras_dsp_context_free(ctx3);
  cras_dsp_context_free(ctx4);
  cras_dsp_stop();
}

static int empty_instantiate(struct dsp_module *module,
                             unsigned long sample_rate)
{
  return 0;
}

static void empty_connect_port(struct dsp_module *module, unsigned long port,
                               float *data_location) {}

static int empty_get_delay(struct dsp_module *module)
{
	return 0;
}

static void empty_run(struct dsp_module *module, unsigned long sample_count) {}

static void empty_deinstantiate(struct dsp_module *module) {}

static void empty_free_module(struct dsp_module *module)
{
  free(module);
}

static int empty_get_properties(struct dsp_module *module) { return 0; }

static void empty_dump(struct dsp_module *module, struct dumper *d)
{
  dumpf(d, "built-in module\n");
}

static void empty_init_module(struct dsp_module *module)
{
  module->instantiate = &empty_instantiate;
  module->connect_port = &empty_connect_port;
  module->get_delay = &empty_get_delay;
  module->run = &empty_run;
  module->deinstantiate = &empty_deinstantiate;
  module->free_module = &empty_free_module;
  module->get_properties = &empty_get_properties;
  module->dump = &empty_dump;
}

}  //  namespace

extern "C"
{
struct dsp_module *cras_dsp_module_load_builtin(struct plugin *plugin)
{
  struct dsp_module *module;
  module = (struct dsp_module *)calloc(1, sizeof(struct dsp_module));
  empty_init_module(module);
  return module;
}
} // extern "C"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
