/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dlfcn.h>
#include <syslog.h>
#include <ladspa.h>

#include "cras_dsp_module.h"

#define PLUGIN_PATH_PREFIX "/usr/lib/ladspa"
#define PLUGIN_PATH_MAX 256

struct ladspa_data {
	void *dlopen_handle;  /* the handle returned by dlopen() */
	const LADSPA_Descriptor *descriptor;
	LADSPA_Handle *handle;	/* returned by instantiate() */
	int activated;
};

static void activate(struct dsp_module *module)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *desc = data->descriptor;
	data->activated = 1;
	if (!desc->activate)
		return;
	desc->activate(data->handle);
}

static void deactivate(struct dsp_module *module)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *desc = data->descriptor;
	data->activated = 0;
	if (!desc->deactivate)
		return;
	desc->deactivate(data->handle);
}

static int instantiate(struct dsp_module *module, unsigned long sample_rate)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *desc = data->descriptor;

	data->handle = desc->instantiate(desc, sample_rate);
	if (!data->handle) {
		syslog(LOG_ERR, "instantiate failed for %s, rate %ld",
		       desc->Label, sample_rate);
		return -1;
	}
	return 0;
}

static void deinstantiate(struct dsp_module *module)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *desc = data->descriptor;

	if (data->activated)
		deactivate(module);
	desc->cleanup(data->handle);
	data->handle = NULL;
}

static void connect_port(struct dsp_module *module, unsigned long port,
			     float *data_location)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *desc = data->descriptor;
	desc->connect_port(data->handle, port, data_location);
}

static int get_delay(struct dsp_module *module)
{
	return 0;
}

static void run(struct dsp_module *module, unsigned long sample_count)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *desc = data->descriptor;

	if (!data->activated)
		activate(module);
	desc->run(data->handle, sample_count);
}

static void free_module(struct dsp_module *module)
{
	struct ladspa_data *data = module->data;
	if (data->activated)
		deactivate(module);
	if (data->dlopen_handle) {
		dlclose(data->dlopen_handle);
		data->dlopen_handle = NULL;
	}
	free(module->data);
	free(module);
}

static int get_properties(struct dsp_module *module)
{
	struct ladspa_data *data = module->data;
	int properties = 0;
	if (LADSPA_IS_INPLACE_BROKEN(data->descriptor->Properties))
		properties |= MODULE_INPLACE_BROKEN;
	return properties;
}

static void dump(struct dsp_module *module, struct dumper *d)
{
	struct ladspa_data *data = module->data;
	const LADSPA_Descriptor *descriptor = data->descriptor;

	dumpf(d, "  LADSPA: dlopen=%p, desc=%p, handle=%p, activated=%d\n",
	      data->dlopen_handle, data->descriptor, data->handle,
	      data->activated);
	if (descriptor) {
		dumpf(d, "   Name=%s\n", descriptor->Name);
		dumpf(d, "   Maker=%s\n", descriptor->Maker);
	}
}

static int verify_plugin_descriptor(struct plugin *plugin,
				    const LADSPA_Descriptor *desc)
{
	int i;
	struct port *port;

	if (desc->PortCount != ARRAY_COUNT(&plugin->ports)) {
		syslog(LOG_ERR, "port count mismatch: %s", plugin->title);
		return -1;
	}

	FOR_ARRAY_ELEMENT(&plugin->ports, i, port) {
		LADSPA_PortDescriptor port_desc = desc->PortDescriptors[i];
		if ((port->direction == PORT_INPUT) !=
		    !!(port_desc & LADSPA_PORT_INPUT)) {
			syslog(LOG_ERR, "port direction mismatch: %s:%d!",
			       plugin->title, i);
			return -1;
		}

		if ((port->type == PORT_CONTROL) !=
		    !!(port_desc & LADSPA_PORT_CONTROL)) {
			syslog(LOG_ERR, "port type mismatch: %s:%d!",
			       plugin->title, i);
			return -1;
		}
	}

	return 0;
}

struct dsp_module *cras_dsp_module_load_ladspa(struct plugin *plugin)
{
	char path[PLUGIN_PATH_MAX];
	int index;
	LADSPA_Descriptor_Function desc_func;
	struct ladspa_data *data = calloc(1, sizeof(struct ladspa_data));
	struct dsp_module *module;

	snprintf(path, sizeof(path), "%s/%s", PLUGIN_PATH_PREFIX,
		 plugin->library);

	data->dlopen_handle = dlopen(path, RTLD_NOW);
	if (!data->dlopen_handle) {
		syslog(LOG_ERR, "cannot open plugin from %s: %s", path,
		       dlerror());
		goto bail;
	}

	desc_func = (LADSPA_Descriptor_Function)dlsym(data->dlopen_handle,
						      "ladspa_descriptor");

	if (!desc_func) {
		syslog(LOG_ERR, "cannot find descriptor function from %s: %s",
		       path, dlerror());
		goto bail;
	}

	for (index = 0; ; index++) {
		const LADSPA_Descriptor *desc = desc_func(index);
		if (desc == NULL) {
			syslog(LOG_ERR, "cannot find label %s from %s",
			       plugin->label, path);
			goto bail;
		}
		if (strcmp(desc->Label, plugin->label) == 0) {
			syslog(LOG_DEBUG, "plugin '%s' loaded from %s",
			       plugin->label, path);
			if (verify_plugin_descriptor(plugin, desc) != 0)
				goto bail;
			data->descriptor = desc;
			break;
		}
	}

	module = calloc(1, sizeof(struct dsp_module));
	module->data = data;
	module->instantiate = &instantiate;
	module->connect_port = &connect_port;
	module->get_delay = &get_delay;
	module->run = &run;
	module->deinstantiate = &deinstantiate;
	module->get_properties = &get_properties;
	module->free_module = &free_module;
	module->dump = &dump;
	return module;
bail:
	if (data->dlopen_handle)
		dlclose(data->dlopen_handle);
	free(data);
	return NULL;
}
