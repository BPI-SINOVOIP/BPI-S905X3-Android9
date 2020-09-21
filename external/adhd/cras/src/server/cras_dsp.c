/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <syslog.h>
#include "dumper.h"
#include "cras_expr.h"
#include "cras_dsp_ini.h"
#include "cras_dsp_pipeline.h"
#include "dsp_util.h"
#include "utlist.h"

/* We have a dsp_context for each pipeline. The context records the
 * parameters used to create a pipeline, so the pipeline can be
 * (re-)loaded later. The pipeline is (re-)loaded in the following
 * cases:
 *
 * (1) The client asks to (re-)load it with cras_load_pipeline().
 * (2) The client asks to reload the ini with cras_reload_ini().
 *
 * The pipeline is (re-)loaded asynchronously in an internal thread,
 * so the client needs to use cras_dsp_get_pipeline() and
 * cras_dsp_put_pipeline() to safely access the pipeline.
 */
struct cras_dsp_context {
	pthread_mutex_t mutex;
	struct pipeline *pipeline;

	struct cras_expr_env env;
	int sample_rate;
	const char *purpose;
	struct cras_dsp_context *prev, *next;
};

static struct dumper *syslog_dumper;
static const char *ini_filename;
static struct ini *ini;
static struct cras_dsp_context *context_list;

static void initialize_environment(struct cras_expr_env *env)
{
	cras_expr_env_install_builtins(env);
	cras_expr_env_set_variable_boolean(env, "disable_eq", 0);
	cras_expr_env_set_variable_boolean(env, "disable_drc", 0);
	cras_expr_env_set_variable_string(env, "dsp_name", "");
	cras_expr_env_set_variable_boolean(env, "swap_lr_disabled", 1);
}

static struct pipeline *prepare_pipeline(struct cras_dsp_context *ctx)
{
	struct pipeline *pipeline;
	const char *purpose = ctx->purpose;

	if (!ini)
		return NULL;

	pipeline = cras_dsp_pipeline_create(ini, &ctx->env, purpose);

	if (pipeline) {
		syslog(LOG_DEBUG, "pipeline created");
	} else {
		syslog(LOG_DEBUG, "cannot create pipeline");
		goto bail;
	}

	if (cras_dsp_pipeline_load(pipeline) != 0) {
		syslog(LOG_ERR, "cannot load pipeline");
		goto bail;
	}

	if (cras_dsp_pipeline_instantiate(pipeline, ctx->sample_rate) != 0) {
		syslog(LOG_ERR, "cannot instantiate pipeline");
		goto bail;
	}

	if (cras_dsp_pipeline_get_sample_rate(pipeline) != ctx->sample_rate) {
		syslog(LOG_ERR, "pipeline sample rate mismatch (%d vs %d)",
		       cras_dsp_pipeline_get_sample_rate(pipeline),
		       ctx->sample_rate);
		goto bail;
	}

	return pipeline;

bail:
	if (pipeline)
		cras_dsp_pipeline_free(pipeline);
	return NULL;
}

static void cmd_load_pipeline(struct cras_dsp_context *ctx)
{
	struct pipeline *pipeline, *old_pipeline;

	pipeline = prepare_pipeline(ctx);

	/* This locking is short to avoild blocking audio thread. */
	pthread_mutex_lock(&ctx->mutex);
	old_pipeline = ctx->pipeline;
	ctx->pipeline = pipeline;
	pthread_mutex_unlock(&ctx->mutex);

	if (old_pipeline)
		cras_dsp_pipeline_free(old_pipeline);
}

static void cmd_reload_ini()
{
	struct ini *old_ini = ini;
	struct cras_dsp_context *ctx;

	ini = cras_dsp_ini_create(ini_filename);
	if (!ini)
		syslog(LOG_ERR, "cannot create dsp ini");

	DL_FOREACH(context_list, ctx) {
		cmd_load_pipeline(ctx);
	}

	if (old_ini)
		cras_dsp_ini_free(old_ini);
}

/* Exported functions */

void cras_dsp_init(const char *filename)
{
	dsp_enable_flush_denormal_to_zero();
	ini_filename = strdup(filename);
	syslog_dumper = syslog_dumper_create(LOG_ERR);
	cmd_reload_ini();
}

void cras_dsp_stop()
{
	syslog_dumper_free(syslog_dumper);
	free((char *)ini_filename);
	if (ini) {
		cras_dsp_ini_free(ini);
		ini = NULL;
	}
}

struct cras_dsp_context *cras_dsp_context_new(int sample_rate,
					      const char *purpose)
{
	struct cras_dsp_context *ctx = calloc(1, sizeof(*ctx));

	pthread_mutex_init(&ctx->mutex, NULL);
	initialize_environment(&ctx->env);
	ctx->sample_rate = sample_rate;
	ctx->purpose = strdup(purpose);

	DL_APPEND(context_list, ctx);
	return ctx;
}

void cras_dsp_context_free(struct cras_dsp_context *ctx)
{
	DL_DELETE(context_list, ctx);

	pthread_mutex_destroy(&ctx->mutex);
	if (ctx->pipeline) {
		cras_dsp_pipeline_free(ctx->pipeline);
		ctx->pipeline = NULL;
	}
	cras_expr_env_free(&ctx->env);
	free((char *)ctx->purpose);
	free(ctx);
}

void cras_dsp_set_variable_string(struct cras_dsp_context *ctx, const char *key,
			   const char *value)
{
	cras_expr_env_set_variable_string(&ctx->env, key, value);
}

void cras_dsp_set_variable_boolean(struct cras_dsp_context *ctx,
				   const char *key,
				   char value)
{
	cras_expr_env_set_variable_boolean(&ctx->env, key, value);
}

void cras_dsp_load_pipeline(struct cras_dsp_context *ctx)
{
	cmd_load_pipeline(ctx);
}

struct pipeline *cras_dsp_get_pipeline(struct cras_dsp_context *ctx)
{
	pthread_mutex_lock(&ctx->mutex);
	if (!ctx->pipeline) {
		pthread_mutex_unlock(&ctx->mutex);
		return NULL;
	}
	return ctx->pipeline;
}

void cras_dsp_put_pipeline(struct cras_dsp_context *ctx)
{
	pthread_mutex_unlock(&ctx->mutex);
}

void cras_dsp_reload_ini()
{
	cmd_reload_ini();
}

void cras_dsp_dump_info()
{
	struct pipeline *pipeline;
	struct cras_dsp_context *ctx;

	if (ini)
		cras_dsp_ini_dump(syslog_dumper, ini);
	DL_FOREACH(context_list, ctx) {
		cras_expr_env_dump(syslog_dumper, &ctx->env);
		pipeline = ctx->pipeline;
		if (pipeline)
			cras_dsp_pipeline_dump(syslog_dumper, pipeline);
	}
}

unsigned int cras_dsp_num_output_channels(const struct cras_dsp_context *ctx)
{
	return cras_dsp_pipeline_get_num_output_channels(ctx->pipeline);
}

unsigned int cras_dsp_num_input_channels(const struct cras_dsp_context *ctx)
{
	return cras_dsp_pipeline_get_num_input_channels(ctx->pipeline);
}
