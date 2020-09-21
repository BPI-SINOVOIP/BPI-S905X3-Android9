/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <alsa/control_external.h>
#include <cras_client.h>

static const size_t MAX_IODEVS = 10; /* Max devices to print out. */
static const size_t MAX_IONODES = 20; /* Max ionodes to print out. */

/* Support basic input/output volume/mute only. */
enum CTL_CRAS_MIXER_CONTROLS {
	CTL_CRAS_MIXER_PLAYBACK_SWITCH,
	CTL_CRAS_MIXER_PLAYBACK_VOLUME,
	CTL_CRAS_MIXER_CAPTURE_SWITCH,
	CTL_CRAS_MIXER_CAPTURE_VOLUME,
	NUM_CTL_CRAS_MIXER_ELEMS
};

/* Hold info specific to each control. */
struct cras_mixer_control {
	const char *name;
	int type;
	unsigned int access;
	unsigned int count;
};

/* CRAS mixer elements. */
static const struct cras_mixer_control cras_elems[NUM_CTL_CRAS_MIXER_ELEMS] = {
	{"Master Playback Switch", SND_CTL_ELEM_TYPE_BOOLEAN,
		SND_CTL_EXT_ACCESS_READWRITE, 1},
	{"Master Playback Volume", SND_CTL_ELEM_TYPE_INTEGER,
		SND_CTL_EXT_ACCESS_READWRITE, 1},
	{"Capture Switch", SND_CTL_ELEM_TYPE_BOOLEAN,
		SND_CTL_EXT_ACCESS_READWRITE, 1},
	{"Capture Volume", SND_CTL_ELEM_TYPE_INTEGER,
		SND_CTL_EXT_ACCESS_READWRITE, 1},
};

/* Holds the client and ctl plugin pointers. */
struct ctl_cras {
	snd_ctl_ext_t ext_ctl;
	struct cras_client *client;
};

/* Frees resources when the plugin is closed. */
static void ctl_cras_close(snd_ctl_ext_t *ext_ctl)
{
	struct ctl_cras *cras = (struct ctl_cras *)ext_ctl->private_data;

	if (cras) {
		cras_client_stop(cras->client);
		cras_client_destroy(cras->client);
	}
	free(cras);
}

/* Lists available controls. */
static int ctl_cras_elem_list(snd_ctl_ext_t *ext_ctl, unsigned int offset,
			      snd_ctl_elem_id_t *id)
{
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	if (offset >= NUM_CTL_CRAS_MIXER_ELEMS)
		return -EINVAL;
	snd_ctl_elem_id_set_name(id, cras_elems[offset].name);
	return 0;
}

/* Returns the number of available controls. */
static int ctl_cras_elem_count(snd_ctl_ext_t *ext_ctl)
{
	return NUM_CTL_CRAS_MIXER_ELEMS;
}

/* Gets a control key from a search id. */
static snd_ctl_ext_key_t ctl_cras_find_elem(snd_ctl_ext_t *ext_ctl,
					    const snd_ctl_elem_id_t *id)
{
	const char *name;
	unsigned int numid;

	numid = snd_ctl_elem_id_get_numid(id);
	if (numid - 1 < NUM_CTL_CRAS_MIXER_ELEMS)
		return numid - 1;

	name = snd_ctl_elem_id_get_name(id);

	for (numid = 0; numid < NUM_CTL_CRAS_MIXER_ELEMS; numid++)
		if (strcmp(cras_elems[numid].name, name) == 0)
			return numid;

	return SND_CTL_EXT_KEY_NOT_FOUND;
}

/* Fills accessibility, type and count based on the specified control. */
static int ctl_cras_get_attribute(snd_ctl_ext_t *ext_ctl, snd_ctl_ext_key_t key,
				  int *type, unsigned int *acc,
				  unsigned int *count)
{
	if (key >= NUM_CTL_CRAS_MIXER_ELEMS)
		return -EINVAL;
	*type = cras_elems[key].type;
	*acc = cras_elems[key].access;
	*count = cras_elems[key].count;
	return 0;
}

/* Returns the range of the specified control.  The volume sliders always run
 * from 0 to 100 for CRAS. */
static int ctl_cras_get_integer_info(snd_ctl_ext_t *ext_ctl,
				     snd_ctl_ext_key_t key,
				     long *imin, long *imax, long *istep)
{
	*istep = 0;
	*imin = 0;
	*imax = 100;
	return 0;
}

static long capture_index_to_gain(struct cras_client *client, long index)
{
	long min;
	long max;
	long dB_step;

	min = cras_client_get_system_min_capture_gain(client);
	max = cras_client_get_system_max_capture_gain(client);
	if (min >= max)
		return min;

	dB_step = (max - min) / 100;

	if (index <= 0)
		return min;
	if (index >= 100)
		return max;
	return index * dB_step + min;
}

static long capture_gain_to_index(struct cras_client *client, long gain)
{
	long min;
	long max;
	long dB_step;

	min = cras_client_get_system_min_capture_gain(client);
	max = cras_client_get_system_max_capture_gain(client);
	if (min >= max)
		return 0;

	dB_step = (max - min) / 100;

	if (gain <= min)
		return 0;
	if (gain >= max)
		return 100;
	return (gain - min) / dB_step;
}

static int get_nodes(struct cras_client *client,
		     enum CRAS_STREAM_DIRECTION dir,
		     struct cras_ionode_info *nodes,
		     size_t num_nodes)
{
	struct cras_iodev_info devs[MAX_IODEVS];
	size_t num_devs;
	int rc;

	if (dir == CRAS_STREAM_OUTPUT)
		rc = cras_client_get_output_devices(client, devs, nodes,
						    &num_devs, &num_nodes);
	else
		rc = cras_client_get_input_devices(client, devs, nodes,
						   &num_devs, &num_nodes);
	if (rc < 0)
		return 0;
	return num_nodes;
}

/* Gets the value of the given control from CRAS and puts it in value. */
static int ctl_cras_read_integer(snd_ctl_ext_t *ext_ctl, snd_ctl_ext_key_t key,
				 long *value)
{
	struct ctl_cras *cras = (struct ctl_cras *)ext_ctl->private_data;
	struct cras_ionode_info nodes[MAX_IONODES];
	int num_nodes, i;

	switch (key) {
	case CTL_CRAS_MIXER_PLAYBACK_SWITCH:
		*value = !cras_client_get_user_muted(cras->client);
		break;
	case CTL_CRAS_MIXER_PLAYBACK_VOLUME:
		num_nodes = get_nodes(cras->client, CRAS_STREAM_OUTPUT,
				      nodes, MAX_IONODES);
		for (i = 0; i < num_nodes; i++) {
			if (!nodes[i].active)
				continue;
			*value = nodes[i].volume;
			break;
		}
		break;
	case CTL_CRAS_MIXER_CAPTURE_SWITCH:
		*value = !cras_client_get_system_capture_muted(cras->client);
		break;
	case CTL_CRAS_MIXER_CAPTURE_VOLUME:
		num_nodes = get_nodes(cras->client, CRAS_STREAM_INPUT,
				      nodes, MAX_IONODES);
		for (i = 0; i < num_nodes; i++) {
			if (!nodes[i].active)
				continue;
			*value = capture_gain_to_index(
					cras->client,
					nodes[i].capture_gain);
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* Writes the given values to CRAS. */
static int ctl_cras_write_integer(snd_ctl_ext_t *ext_ctl, snd_ctl_ext_key_t key,
				   long *value)
{
	struct ctl_cras *cras = (struct ctl_cras *)ext_ctl->private_data;
	struct cras_ionode_info nodes[MAX_IONODES];
	int num_nodes, i;
	long gain;

	switch (key) {
	case CTL_CRAS_MIXER_PLAYBACK_SWITCH:
		cras_client_set_user_mute(cras->client, !(*value));
		break;
	case CTL_CRAS_MIXER_PLAYBACK_VOLUME:
		num_nodes = get_nodes(cras->client, CRAS_STREAM_OUTPUT,
				      nodes, MAX_IONODES);
		for (i = 0; i < num_nodes; i++) {
			if (!nodes[i].active)
				continue;
			cras_client_set_node_volume(cras->client,
					cras_make_node_id(nodes[i].iodev_idx,
							  nodes[i].ionode_idx),
					*value);
		}
		break;
	case CTL_CRAS_MIXER_CAPTURE_SWITCH:
		cras_client_set_system_capture_mute(cras->client, !(*value));
		break;
	case CTL_CRAS_MIXER_CAPTURE_VOLUME:
		gain = capture_index_to_gain(cras->client, *value);
		num_nodes = get_nodes(cras->client, CRAS_STREAM_INPUT,
				      nodes, MAX_IONODES);
		for (i = 0; i < num_nodes; i++) {
			if (!nodes[i].active)
				continue;
			cras_client_set_node_capture_gain(cras->client,
					cras_make_node_id(nodes[i].iodev_idx,
							  nodes[i].ionode_idx),
					gain);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const snd_ctl_ext_callback_t ctl_cras_ext_callback = {
	.close = ctl_cras_close,
	.elem_count = ctl_cras_elem_count,
	.elem_list = ctl_cras_elem_list,
	.find_elem = ctl_cras_find_elem,
	.get_attribute = ctl_cras_get_attribute,
	.get_integer_info = ctl_cras_get_integer_info,
	.read_integer = ctl_cras_read_integer,
	.write_integer = ctl_cras_write_integer,
};

SND_CTL_PLUGIN_DEFINE_FUNC(cras)
{
	struct ctl_cras *cras;
	int rc;

	cras = malloc(sizeof(*cras));
	if (cras == NULL)
		return -ENOMEM;

	rc = cras_client_create(&cras->client);
	if (rc != 0 || cras->client == NULL) {
		fprintf(stderr, "Couldn't create CRAS client\n");
		free(cras);
		return rc;
	}

	rc = cras_client_connect(cras->client);
	if (rc < 0) {
		fprintf(stderr, "Couldn't connect to cras.\n");
		cras_client_destroy(cras->client);
		free(cras);
		return rc;
	}

	rc = cras_client_run_thread(cras->client);
	if (rc < 0) {
		fprintf(stderr, "Couldn't start client thread.\n");
		cras_client_stop(cras->client);
		cras_client_destroy(cras->client);
		free(cras);
		return rc;
	}

	rc = cras_client_connected_wait(cras->client);
	if (rc < 0) {
		fprintf(stderr, "CRAS client wouldn't connect.\n");
		cras_client_stop(cras->client);
		cras_client_destroy(cras->client);
		free(cras);
		return rc;
	}

	cras->ext_ctl.version = SND_CTL_EXT_VERSION;
	cras->ext_ctl.card_idx = 0;
	strncpy(cras->ext_ctl.id, "cras", sizeof(cras->ext_ctl.id) - 1);
	cras->ext_ctl.id[sizeof(cras->ext_ctl.id) - 1] = '\0';
	strncpy(cras->ext_ctl.driver, "CRAS plugin",
		sizeof(cras->ext_ctl.driver) - 1);
	cras->ext_ctl.driver[sizeof(cras->ext_ctl.driver) - 1] = '\0';
	strncpy(cras->ext_ctl.name, "CRAS", sizeof(cras->ext_ctl.name) - 1);
	cras->ext_ctl.name[sizeof(cras->ext_ctl.name) - 1] = '\0';
	strncpy(cras->ext_ctl.longname, "CRAS",
		sizeof(cras->ext_ctl.longname) - 1);
	cras->ext_ctl.longname[sizeof(cras->ext_ctl.longname) - 1] = '\0';
	strncpy(cras->ext_ctl.mixername, "CRAS",
		sizeof(cras->ext_ctl.mixername) - 1);
	cras->ext_ctl.mixername[sizeof(cras->ext_ctl.mixername) - 1] = '\0';
	cras->ext_ctl.poll_fd = -1;

	cras->ext_ctl.callback = &ctl_cras_ext_callback;
	cras->ext_ctl.private_data = cras;

	rc = snd_ctl_ext_create(&cras->ext_ctl, name, mode);
	if (rc < 0) {
		cras_client_stop(cras->client);
		cras_client_destroy(cras->client);
		free(cras);
		return rc;
	}

	*handlep = cras->ext_ctl.handle;
	return 0;
}

SND_CTL_PLUGIN_SYMBOL(cras);
