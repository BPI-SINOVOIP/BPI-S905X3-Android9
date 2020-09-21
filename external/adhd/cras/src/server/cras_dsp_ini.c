/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include "cras_dsp_ini.h"

#define MAX_INI_KEY_LENGTH 64  /* names like "output_source:output_0" */
#define MAX_NR_PORT 128	/* the max number of ports for a plugin */
#define MAX_PORT_NAME_LENGTH 20 /* names like "output_32" */

/* Format of the ini file (See dsp.ini.sample for an example).

- Each section in the ini file specifies a plugin. The section name is
  just an identifier. The "library" and "label" attributes in a
  section must be defined. The "library" attribute is the name of the
  shared library from which this plugin will be loaded, or a special
  value "builtin" for built-in plugins. The "label" attribute specify
  which plugin inside the shared library should be loaded.

- Built-in plugins have an attribute "label" which has value "source"
  or "sink". It defines where the audio data flows into and flows out
  of the pipeline.  Built-in plugins also have a attribute "purpose"
  which has the value "playback" or "capture". It defines which
  pipeline these plugins belong to.

- Each plugin can have an optional "disable expression", which defines
  under which conditions the plugin is disabled.

- Each plugin have some ports which specify the parameters for the
  plugin or to specify connections to other plugins. The ports in each
  plugin are numbered from 0. Each port is either an input port or an
  output port, and each port is either an audio port or a control
  port. The connections between two ports are expressed by giving the
  same value to both ports. For audio ports, the value should be
  "{identifier}". For control ports, the value shoule be
  "<identifier>". For example, the following fragment

  [plugin1]
  ...
  output_4={audio_left}
  output_5={audio_right}

  [plugin2]
  ...
  input_0={audio_left}

  [plugin3]
  ...
  input_2={audio_right}

  specifies these connections:
  port 4 of plugin1 --> port 0 of plugin2
  port 5 of plugin1 --> port 2 of plugin3

*/

static const char *getstring(struct ini *ini, const char *sec_name,
			     const char *key)
{
	char full_key[MAX_INI_KEY_LENGTH];
	snprintf(full_key, sizeof(full_key), "%s:%s", sec_name, key);
	return iniparser_getstring(ini->dict, full_key, NULL);
}

static int lookup_flow(struct ini *ini, const char *name)
{
	int i;
	const struct flow *flow;

	FOR_ARRAY_ELEMENT(&ini->flows, i, flow) {
		if (strcmp(flow->name, name) == 0)
			return i;
	}

	return -1;
}

static int lookup_or_add_flow(struct ini *ini, const char *name)
{
	struct flow *flow;
	int i = lookup_flow(ini, name);
	if (i != -1)
		return i;
	i = ARRAY_COUNT(&ini->flows);
	flow = ARRAY_APPEND_ZERO(&ini->flows);
	flow->name = name;
	return i;
}

static int parse_ports(struct ini *ini, const char *sec_name,
		       struct plugin *plugin)
{
	char key[MAX_PORT_NAME_LENGTH];
	const char *str;
	int i;
	struct port *p;
	int direction;

	for (i = 0; i < MAX_NR_PORT; i++) {
		direction = PORT_INPUT;
		snprintf(key, sizeof(key), "input_%d", i);
		str = getstring(ini, sec_name, key);
		if (str == NULL)  {
			direction = PORT_OUTPUT;
			snprintf(key, sizeof(key), "output_%d", i);
			str = getstring(ini, sec_name, key);
			if (str == NULL)
				break; /* no more ports */
		}

		if (*str == '\0') {
			syslog(LOG_ERR, "empty value for %s:%s", sec_name, key);
			return -1;
		}

		if (str[0] == '<' || str[0] == '{') {
			p = ARRAY_APPEND_ZERO(&plugin->ports);
			p->type = (str[0] == '<') ? PORT_CONTROL : PORT_AUDIO;
			p->flow_id = lookup_or_add_flow(ini, str);
			p->init_value = 0;
		} else {
			char *endptr;
			float init_value = strtof(str, &endptr);
			if (endptr == str) {
				syslog(LOG_ERR, "cannot parse number from '%s'",
				       str);
			}
			p = ARRAY_APPEND_ZERO(&plugin->ports);
			p->type = PORT_CONTROL;
			p->flow_id = INVALID_FLOW_ID;
			p->init_value = init_value;
		}
		p->direction = direction;
	}

	return 0;
}

static int parse_plugin_section(struct ini *ini, const char *sec_name,
				struct plugin *p)
{
	p->title = sec_name;
	p->library = getstring(ini, sec_name, "library");
	p->label = getstring(ini, sec_name, "label");
	p->purpose = getstring(ini, sec_name, "purpose");
	p->disable_expr = cras_expr_expression_parse(
		getstring(ini, sec_name, "disable"));

	if (p->library == NULL || p->label == NULL) {
		syslog(LOG_ERR, "A plugin must have library and label: %s",
		       sec_name);
		return -1;
	}

	if (parse_ports(ini, sec_name, p) < 0) {
		syslog(LOG_ERR, "Failed to parse ports: %s", sec_name);
		return -1;
	}

	return 0;
}

static void fill_flow_info(struct ini *ini)
{
	int i, j;
	struct plugin *plugin;
	struct port *port;
	struct flow *flow;
	struct plugin **pplugin;
	int *pport;

	FOR_ARRAY_ELEMENT(&ini->plugins, i, plugin) {
		FOR_ARRAY_ELEMENT(&plugin->ports, j, port) {
			int flow_id = port->flow_id;
			if (flow_id == INVALID_FLOW_ID)
				continue;
			flow = ARRAY_ELEMENT(&ini->flows, flow_id);
			flow->type = port->type;
			if (port->direction == PORT_INPUT) {
				pplugin = &flow->to;
				pport = &flow->to_port;
			} else {
				pplugin = &flow->from;
				pport = &flow->from_port;
			}
			*pplugin = plugin;
			*pport = j;
		}
	}
}

/* Adds a port to a plugin with specified flow id and direction. */
static void add_audio_port(struct ini *ini,
			   struct plugin *plugin,
			   int flow_id,
			   enum port_direction port_direction)
{
	struct port *p;
	p = ARRAY_APPEND_ZERO(&plugin->ports);
	p->type = PORT_AUDIO;
	p->flow_id = flow_id;
	p->init_value = 0;
	p->direction = port_direction;
}

/* Fills fields for a swap_lr plugin.*/
static void fill_swap_lr_plugin(struct ini *ini,
				struct plugin *plugin,
				int input_flowid_0,
				int input_flowid_1,
				int output_flowid_0,
				int output_flowid_1)
{
	plugin->title = "swap_lr";
	plugin->library = "builtin";
	plugin->label = "swap_lr";
	plugin->purpose = "playback";
	plugin->disable_expr = cras_expr_expression_parse("swap_lr_disabled");

	add_audio_port(ini, plugin, input_flowid_0, PORT_INPUT);
	add_audio_port(ini, plugin, input_flowid_1, PORT_INPUT);
	add_audio_port(ini, plugin, output_flowid_0, PORT_OUTPUT);
	add_audio_port(ini, plugin, output_flowid_1, PORT_OUTPUT);
}

/* Adds a new flow with name. If there is already a flow with the name, returns
 * INVALID_FLOW_ID.
 */
static int add_new_flow(struct ini *ini, const char *name)
{
	struct flow *flow;
	int i = lookup_flow(ini, name);
	if (i != -1)
		return INVALID_FLOW_ID;
	i = ARRAY_COUNT(&ini->flows);
	flow = ARRAY_APPEND_ZERO(&ini->flows);
	flow->name = name;
	return i;
}

/* Finds the first playback sink plugin in ini. */
struct plugin *find_first_playback_sink_plugin(struct ini *ini)
{
	int i;
	struct plugin *plugin;

	FOR_ARRAY_ELEMENT(&ini->plugins, i, plugin) {
		if (strcmp(plugin->library, "builtin") != 0)
			continue;
		if (strcmp(plugin->label, "sink") != 0)
			continue;
		if (!plugin->purpose ||
		    strcmp(plugin->purpose, "playback") != 0)
			continue;
		return plugin;
	}

	return NULL;
}

/* Inserts a swap_lr plugin before sink. Handles the port change such that
 * the port originally connects to sink will connect to swap_lr.
 */
static int insert_swap_lr_plugin(struct ini *ini)
{
	struct plugin *swap_lr, *sink;
	int sink_input_flowid_0, sink_input_flowid_1;
	int swap_lr_output_flowid_0, swap_lr_output_flowid_1;

	/* Only add swap_lr plugin for two-channel playback dsp.
	 * TODO(cychiang): Handle multiple sinks if needed.
	 */
	sink = find_first_playback_sink_plugin(ini);
	if ((sink == NULL) || ARRAY_COUNT(&sink->ports) != 2)
		return 0;

	/* Gets the original flow ids of the sink input ports. */
	sink_input_flowid_0 = ARRAY_ELEMENT(&sink->ports, 0)->flow_id;
	sink_input_flowid_1 = ARRAY_ELEMENT(&sink->ports, 1)->flow_id;

	/* Create new flow ids for swap_lr output ports. */
	swap_lr_output_flowid_0 = add_new_flow(ini, "{swap_lr_out:0}");
	swap_lr_output_flowid_1 = add_new_flow(ini, "{swap_lr_out:1}");

	if (swap_lr_output_flowid_0 == INVALID_FLOW_ID ||
	    swap_lr_output_flowid_1 == INVALID_FLOW_ID) {
		syslog(LOG_ERR, "Can not create flow id for swap_lr_out");
		return -EINVAL;
	}

	/* Creates a swap_lr plugin and sets the input and output ports. */
	swap_lr = ARRAY_APPEND_ZERO(&ini->plugins);
	fill_swap_lr_plugin(ini,
			    swap_lr,
			    sink_input_flowid_0,
			    sink_input_flowid_1,
			    swap_lr_output_flowid_0,
			    swap_lr_output_flowid_1);

	/* Look up first sink again because ini->plugins could be realloc'ed */
	sink = find_first_playback_sink_plugin(ini);

	/* The flow ids of sink input ports should be changed to flow ids of
	 * {swap_lr_out:0}, {swap_lr_out:1}. */
	ARRAY_ELEMENT(&sink->ports, 0)->flow_id = swap_lr_output_flowid_0;
	ARRAY_ELEMENT(&sink->ports, 1)->flow_id = swap_lr_output_flowid_1;

	return 0;
}

struct ini *cras_dsp_ini_create(const char *ini_filename)
{
	struct ini *ini;
	dictionary *dict;
	int nsec, i;
	const char *sec_name;
	struct plugin *plugin;
	int rc;

	ini = calloc(1, sizeof(struct ini));
	if (!ini) {
		syslog(LOG_ERR, "no memory for ini struct");
		return NULL;
	}

	dict = iniparser_load((char *)ini_filename);
	if (!dict) {
		syslog(LOG_ERR, "no ini file %s", ini_filename);
		goto bail;
	}
	ini->dict = dict;

	/* Parse the plugin sections */
	nsec = iniparser_getnsec(dict);
	for (i = 0; i < nsec; i++) {
		sec_name = iniparser_getsecname(dict, i);
		plugin = ARRAY_APPEND_ZERO(&ini->plugins);
		if (parse_plugin_section(ini, sec_name, plugin) < 0)
			goto bail;
	}

	/* Insert a swap_lr plugin before sink. */
	rc = insert_swap_lr_plugin(ini);
	if (rc < 0) {
		syslog(LOG_ERR, "failed to insert swap_lr plugin");
		goto bail;
	}

	/* Fill flow info now because now the plugin array won't change */
	fill_flow_info(ini);

	return ini;
bail:
	cras_dsp_ini_free(ini);
	return NULL;
}

void cras_dsp_ini_free(struct ini *ini)
{
	struct plugin *p;
	int i;

	/* free plugins */
	FOR_ARRAY_ELEMENT(&ini->plugins, i, p) {
		cras_expr_expression_free(p->disable_expr);
		ARRAY_FREE(&p->ports);
	}
	ARRAY_FREE(&ini->plugins);
	ARRAY_FREE(&ini->flows);

	if (ini->dict) {
		iniparser_freedict(ini->dict);
		ini->dict = NULL;
	}

	free(ini);
}

static const char *port_direction_str(enum port_direction port_direction)
{
	switch (port_direction) {
	case PORT_INPUT: return "input";
	case PORT_OUTPUT: return "output";
	default: return "unknown";
	}
}

static const char *port_type_str(enum port_type port_type)
{
	switch (port_type) {
	case PORT_CONTROL: return "control";
	case PORT_AUDIO: return "audio";
	default: return "unknown";
	}
}

static const char *plugin_title(struct plugin *plugin)
{
	if (plugin == NULL)
		return "(null)";
	return plugin->title;
}

void cras_dsp_ini_dump(struct dumper *d, struct ini *ini)
{
	int i, j;
	struct plugin *plugin;
	struct port *port;
	const struct flow *flow;

	dumpf(d, "---- ini dump begin ---\n");
	dumpf(d, "ini->dict = %p\n", ini->dict);

	dumpf(d, "number of plugins = %d\n", ARRAY_COUNT(&ini->plugins));
	FOR_ARRAY_ELEMENT(&ini->plugins, i, plugin) {
		dumpf(d, "[plugin %d: %s]\n", i, plugin->title);
		dumpf(d, "library=%s\n", plugin->library);
		dumpf(d, "label=%s\n", plugin->label);
		dumpf(d, "purpose=%s\n", plugin->purpose);
		dumpf(d, "disable=%p\n", plugin->disable_expr);
		FOR_ARRAY_ELEMENT(&plugin->ports, j, port) {
			dumpf(d,
			      "  [%s port %d] type=%s, flow_id=%d, value=%g\n",
			      port_direction_str(port->direction), j,
			      port_type_str(port->type), port->flow_id,
			      port->init_value);
		}
	}

	dumpf(d, "number of flows = %d\n", ARRAY_COUNT(&ini->flows));
	FOR_ARRAY_ELEMENT(&ini->flows, i, flow) {
		dumpf(d, "  [flow %d] %s, %s, %s:%d -> %s:%d\n",
		      i, flow->name, port_type_str(flow->type),
		      plugin_title(flow->from), flow->from_port,
		      plugin_title(flow->to), flow->to_port);
	}

	dumpf(d, "---- ini dump end ----\n");
}
