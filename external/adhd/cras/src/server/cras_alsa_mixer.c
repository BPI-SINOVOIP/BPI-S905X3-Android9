/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <limits.h>
#include <stdio.h>
#include <syslog.h>

#include "cras_alsa_mixer.h"
#include "cras_alsa_mixer_name.h"
#include "cras_alsa_ucm.h"
#include "cras_util.h"
#include "utlist.h"

#define MIXER_CONTROL_VOLUME_DB_INVALID LONG_MAX

/* Represents an ALSA control element. Each device can have several of these,
 * each potentially having independent volume and mute controls.
 * elem - ALSA mixer element.
 * has_volume - non-zero indicates there is a volume control.
 * has_mute - non-zero indicates there is a mute switch.
 * max_volume_dB - the maximum volume for this control, or
 *                 MIXER_CONTROL_VOLUME_DB_INVALID.
 * min_volume_dB - the minimum volume for this control, or
 *                 MIXER_CONTROL_VOLUME_DB_INVALID.
 */
struct mixer_control_element {
	snd_mixer_elem_t *elem;
	int has_volume;
	int has_mute;
	long max_volume_dB;
	long min_volume_dB;
	struct mixer_control_element *prev, *next;
};

/* Represents an ALSA control element related to a specific input/output
 * node such as speakers or headphones. A device can have several of these,
 * each potentially having independent volume and mute controls.
 *
 * Each will have at least one mixer_control_element. For cases where there
 * are separate control elements for left/right channels (for example),
 * additional mixer_control_elements are added.
 *
 * For controls with volume it is assumed that all elements have the same
 * range.
 *
 * name - Name of the control (typicially this is the same as the name of the
 *        mixer_control_element when there is one, or the name of the UCM
 *        parent when there are multiple).
 * dir - Control direction, OUTPUT or INPUT only.
 * elements - The mixer_control_elements that are driven by this control.
 * has_volume - non-zero indicates there is a volume control.
 * has_mute - non-zero indicates there is a mute switch.
 * max_volume_dB - Maximum volume available in the volume control.
 * min_volume_dB - Minimum volume available in the volume control.
 */
struct mixer_control {
	const char *name;
	enum CRAS_STREAM_DIRECTION dir;
	struct mixer_control_element *elements;
	int has_volume;
	int has_mute;
	long max_volume_dB;
	long min_volume_dB;
	struct mixer_control *prev, *next;
};

/* Holds a reference to the opened mixer and the volume controls.
 * mixer - Pointer to the opened alsa mixer.
 * main_volume_controls - List of volume controls (normally 'Master' and 'PCM').
 * playback_switch - Switch used to mute the device.
 * main_capture_controls - List of capture gain controls (normally 'Capture').
 * capture_switch - Switch used to mute the capture stream.
 * max_volume_dB - Maximum volume available in main volume controls.  The dBFS
 *   value setting will be applied relative to this.
 * min_volume_dB - Minimum volume available in main volume controls.
 */
struct cras_alsa_mixer {
	snd_mixer_t *mixer;
	struct mixer_control *main_volume_controls;
	struct mixer_control *output_controls;
	snd_mixer_elem_t *playback_switch;
	struct mixer_control *main_capture_controls;
	struct mixer_control *input_controls;
	snd_mixer_elem_t *capture_switch;
	long max_volume_dB;
	long min_volume_dB;
};

/* Wrapper for snd_mixer_open and helpers.
 * Args:
 *    mixdev - Name of the device to open the mixer for.
 *    mixer - Pointer filled with the opened mixer on success, NULL on failure.
 */
static void alsa_mixer_open(const char *mixdev,
			    snd_mixer_t **mixer)
{
	int rc;

	*mixer = NULL;
	rc = snd_mixer_open(mixer, 0);
	if (rc < 0) {
		syslog(LOG_ERR, "snd_mixer_open: %d: %s", rc, strerror(-rc));
		return;
	}
	rc = snd_mixer_attach(*mixer, mixdev);
	if (rc < 0) {
		syslog(LOG_ERR, "snd_mixer_attach: %d: %s", rc, strerror(-rc));
		goto fail_after_open;
	}
	rc = snd_mixer_selem_register(*mixer, NULL, NULL);
	if (rc < 0) {
		syslog(LOG_ERR, "snd_mixer_selem_register: %d: %s", rc, strerror(-rc));
		goto fail_after_open;
	}
	rc = snd_mixer_load(*mixer);
	if (rc < 0) {
		syslog(LOG_ERR, "snd_mixer_load: %d: %s", rc, strerror(-rc));
		goto fail_after_open;
	}
	return;

fail_after_open:
	snd_mixer_close(*mixer);
	*mixer = NULL;
}

static struct mixer_control_element *mixer_control_element_create(
					snd_mixer_elem_t *elem,
					enum CRAS_STREAM_DIRECTION dir)
{
	struct mixer_control_element *c;
	long min, max;

	if (!elem)
		return NULL;

	c = (struct mixer_control_element *)calloc(1, sizeof(*c));
	if (!c) {
		syslog(LOG_ERR, "No memory for mixer_control_elem.");
		return NULL;
	}

	c->elem = elem;
	c->max_volume_dB = MIXER_CONTROL_VOLUME_DB_INVALID;
	c->min_volume_dB = MIXER_CONTROL_VOLUME_DB_INVALID;

	if (dir == CRAS_STREAM_OUTPUT) {
		c->has_mute = snd_mixer_selem_has_playback_switch(elem);

		if (snd_mixer_selem_has_playback_volume(elem) &&
		    snd_mixer_selem_get_playback_dB_range(
						elem, &min, &max) == 0) {
			c->max_volume_dB = max;
			c->min_volume_dB = min;
			c->has_volume = 1;
		}
	}
	else if (dir == CRAS_STREAM_INPUT) {
		c->has_mute = snd_mixer_selem_has_capture_switch(elem);

		if (snd_mixer_selem_has_capture_volume(elem) &&
		    snd_mixer_selem_get_capture_dB_range(
						elem, &min, &max) == 0) {
			c->max_volume_dB = max;
			c->min_volume_dB = min;
			c->has_volume = 1;
		}
	}

	return c;
}

static void mixer_control_destroy(struct mixer_control *control) {
	struct mixer_control_element *elem;

	if (!control)
		return;

	DL_FOREACH(control->elements, elem) {
		DL_DELETE(control->elements, elem);
		free(elem);
	}
	if (control->name)
		free((void *)control->name);
	free(control);
}

static void mixer_control_destroy_list(struct mixer_control *control_list)
{
	struct mixer_control *control;
	if (!control_list)
		return;
	DL_FOREACH(control_list, control) {
		DL_DELETE(control_list, control);
		mixer_control_destroy(control);
	}
}

static int mixer_control_add_element(struct mixer_control *control,
				     snd_mixer_elem_t *snd_elem)
{
	struct mixer_control_element *elem;

	if (!control)
		return -EINVAL;

	elem = mixer_control_element_create(snd_elem, control->dir);
	if (!elem)
		return -ENOMEM;

	DL_APPEND(control->elements, elem);

	if (elem->has_volume) {
		if (!control->has_volume)
			control->has_volume = 1;

		/* Assume that all elements have a common volume range, and
		 * that both min and max values are valid if one of the two
		 * is valid. */
		if (control->min_volume_dB ==
		    MIXER_CONTROL_VOLUME_DB_INVALID) {
			control->min_volume_dB = elem->min_volume_dB;
			control->max_volume_dB = elem->max_volume_dB;
		} else if (control->min_volume_dB != elem->min_volume_dB ||
			   control->max_volume_dB != elem->max_volume_dB) {
			syslog(LOG_WARNING,
			    "Element '%s' of control '%s' has different"
			    "volume range: [%ld:%ld] ctrl: [%ld:%ld]",
			    snd_mixer_selem_get_name(elem->elem),
			    control->name,
			    elem->min_volume_dB, elem->max_volume_dB,
			    control->min_volume_dB, control->max_volume_dB);
		}
	}

	if (elem->has_mute && !control->has_mute)
		control->has_mute = 1;
	return 0;
}

static int mixer_control_create(struct mixer_control **control,
				const char *name,
				snd_mixer_elem_t *elem,
				enum CRAS_STREAM_DIRECTION dir)
{
	struct mixer_control *c;
	int rc = 0;

	if (!control)
		return -EINVAL;

	c = (struct mixer_control *)calloc(1, sizeof(*c));
	if (!c) {
		syslog(LOG_ERR, "No memory for mixer_control: %s", name);
		rc = -ENOMEM;
		goto error;
	}

	c->dir = dir;
	c->min_volume_dB = MIXER_CONTROL_VOLUME_DB_INVALID;
	c->max_volume_dB = MIXER_CONTROL_VOLUME_DB_INVALID;

	if (!name && elem)
		name = snd_mixer_selem_get_name(elem);
	if (!name) {
		syslog(LOG_ERR, "Control does not have a name.");
		rc = -EINVAL;
		goto error;
	}

	c->name = strdup(name);
	if (!c->name) {
		syslog(LOG_ERR, "No memory for control's name: %s", name);
		rc = -ENOMEM;
		goto error;
	}

	if (elem && (rc = mixer_control_add_element(c, elem)))
		goto error;

	*control = c;
	return 0;

error:
	mixer_control_destroy(c);
	*control = NULL;
	return rc;
}

/* Creates a mixer_control by finding mixer element names in simple mixer
 * interface.
 * Args:
 *    control[out] - Storage for resulting pointer to mixer_control.
 *    cmix[in] - Parent alsa mixer.
 *    name[in] - Optional name of the control. Input NULL to take the name of
 *               the first element from mixer_names.
 *    mixer_names[in] - Names of the ASLA mixer control elements. Must not
 *                      be empty.
 *    dir[in] - Control direction: CRAS_STREAM_OUTPUT or CRAS_STREAM_INPUT.
 * Returns:
 *    Returns 0 for success, negative error code otherwise. *control is
 *    initialized to NULL on error, or has a valid pointer for success.
 */
static int mixer_control_create_by_name(
		struct mixer_control **control,
		struct cras_alsa_mixer *cmix,
		const char *name,
		struct mixer_name *mixer_names,
		enum CRAS_STREAM_DIRECTION dir)
{
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
	struct mixer_control *c;
	struct mixer_name *m_name;
	int rc;

	if (!control)
		return -EINVAL;
	*control = NULL;
	if (!mixer_names)
		return -EINVAL;
	if (!name) {
		/* Assume that we're using the first name in the list of mixer
		 * names. */
		name = mixer_names->name;
	}

	rc = mixer_control_create(&c, name, NULL, dir);
	if (rc)
		return rc;

	snd_mixer_selem_id_malloc(&sid);

	DL_FOREACH(mixer_names, m_name) {
		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, m_name->name);
		elem = snd_mixer_find_selem(cmix->mixer, sid);
		if (!elem) {
			mixer_control_destroy(c);
			snd_mixer_selem_id_free(sid);
			syslog(LOG_ERR, "Unable to find simple control %s, 0",
			       m_name->name);
			return -ENOENT;
		}
		rc = mixer_control_add_element(c, elem);
		if (rc) {
			mixer_control_destroy(c);
			snd_mixer_selem_id_free(sid);
			return rc;
		}
	}

	snd_mixer_selem_id_free(sid);
	*control = c;
	return 0;
}

static int mixer_control_set_dBFS(
		const struct mixer_control *control, long to_set)
{
	const struct mixer_control_element *elem = NULL;
	int rc = -EINVAL;
	if (!control)
		return rc;
	DL_FOREACH(control->elements, elem) {
		if(elem->has_volume) {
			if (control->dir == CRAS_STREAM_OUTPUT)
				rc = snd_mixer_selem_set_playback_dB_all(
						elem->elem, to_set, 1);
			else if (control->dir == CRAS_STREAM_INPUT)
				rc = snd_mixer_selem_set_capture_dB_all(
						elem->elem, to_set, 1);
			if (rc)
				break;
			syslog(LOG_DEBUG, "%s:%s volume set to %ld",
			       control->name,
			       snd_mixer_selem_get_name(elem->elem),
			       to_set);
		}
	}
	if (rc && elem) {
		syslog(LOG_ERR, "Failed to set volume of '%s:%s': %d",
		       control->name,
		       snd_mixer_selem_get_name(elem->elem), rc);
	}
	return rc;
}

static int mixer_control_get_dBFS(
		const struct mixer_control *control, long *to_get)
{
	const struct mixer_control_element *elem = NULL;
	int rc = -EINVAL;
	if (!control || !to_get)
		return -EINVAL;
	DL_FOREACH(control->elements, elem) {
		if (elem->has_volume) {
			if (control->dir == CRAS_STREAM_OUTPUT)
				rc = snd_mixer_selem_get_playback_dB(
						elem->elem,
						SND_MIXER_SCHN_FRONT_LEFT,
						to_get);
			else if (control->dir == CRAS_STREAM_INPUT)
				rc = snd_mixer_selem_get_capture_dB(
						elem->elem,
						SND_MIXER_SCHN_FRONT_LEFT,
						to_get);
			/* Assume all of the elements of this control have
			 * the same value. */
			break;
		}
	}
	if (rc && elem) {
		syslog(LOG_ERR, "Failed to get volume of '%s:%s': %d",
		       control->name,
		       snd_mixer_selem_get_name(elem->elem), rc);
	}
	return rc;
}

static int mixer_control_set_mute(
		const struct mixer_control *control, int muted)
{
	const struct mixer_control_element *elem = NULL;
	int rc;
	if (!control)
		return -EINVAL;
	DL_FOREACH(control->elements, elem) {
		if(elem->has_mute) {
			if (control->dir == CRAS_STREAM_OUTPUT)
				rc = snd_mixer_selem_set_playback_switch_all(
					elem->elem, !muted);
			else if (control->dir == CRAS_STREAM_INPUT)
				rc = snd_mixer_selem_set_capture_switch_all(
					elem->elem, !muted);
			if (rc)
				break;
		}
	}
	if (rc && elem) {
		syslog(LOG_ERR, "Failed to mute '%s:%s': %d",
		       control->name,
		       snd_mixer_selem_get_name(elem->elem), rc);
	}
	return rc;
}

/* Adds the main volume control to the list and grabs the first seen playback
 * switch to use for mute. */
static int add_main_volume_control(struct cras_alsa_mixer *cmix,
				   snd_mixer_elem_t *elem)
{
	if (snd_mixer_selem_has_playback_volume(elem)) {
		long range;
		struct mixer_control *c, *next;
		int rc = mixer_control_create(&c, NULL, elem, CRAS_STREAM_OUTPUT);
		if (rc)
			return rc;

		if (c->has_volume) {
			cmix->max_volume_dB += c->max_volume_dB;
			cmix->min_volume_dB += c->min_volume_dB;
		}

		range = c->max_volume_dB - c->min_volume_dB;
		DL_FOREACH(cmix->main_volume_controls, next) {
			if (range > next->max_volume_dB - next->min_volume_dB)
				break;
		}

		syslog(LOG_DEBUG, "Add main volume control %s\n", c->name);
		DL_INSERT(cmix->main_volume_controls, next, c);
	}

	/* If cmix doesn't yet have a playback switch and this is a playback
	 * switch, use it. */
	if (cmix->playback_switch == NULL &&
			snd_mixer_selem_has_playback_switch(elem)) {
		syslog(LOG_DEBUG, "Using '%s' as playback switch.",
		       snd_mixer_selem_get_name(elem));
		cmix->playback_switch = elem;
	}

	return 0;
}

/* Adds the main capture control to the list and grabs the first seen capture
 * switch to mute input. */
static int add_main_capture_control(struct cras_alsa_mixer *cmix,
				    snd_mixer_elem_t *elem)
{
	/* TODO(dgreid) handle index != 0, map to correct input. */
	if (snd_mixer_selem_get_index(elem) > 0)
		return 0;

	if (snd_mixer_selem_has_capture_volume(elem)) {
		struct mixer_control *c;
		int rc = mixer_control_create(&c, NULL, elem, CRAS_STREAM_INPUT);
		if (rc)
			return rc;

		syslog(LOG_DEBUG, "Add main capture control %s\n", c->name);
		DL_APPEND(cmix->main_capture_controls, c);
	}

	/* If cmix doesn't yet have a capture switch and this is a capture
	 * switch, use it. */
	if (cmix->capture_switch == NULL &&
	    snd_mixer_selem_has_capture_switch(elem)) {
		syslog(LOG_DEBUG, "Using '%s' as capture switch.",
		       snd_mixer_selem_get_name(elem));
		cmix->capture_switch = elem;
	}

	return 0;
}

/* Adds a control to the list. */
static int add_control_with_name(struct cras_alsa_mixer *cmix,
				 enum CRAS_STREAM_DIRECTION dir,
				 snd_mixer_elem_t *elem,
				 const char *name)
{
	int index; /* Index part of mixer simple element */
	struct mixer_control *c;
	int rc;

	index = snd_mixer_selem_get_index(elem);
	syslog(LOG_DEBUG, "Add %s control: %s,%d\n",
	       dir == CRAS_STREAM_OUTPUT ? "output" : "input",
	       name, index);

	rc = mixer_control_create(&c, name, elem, dir);
	if (rc)
		return rc;

	if (c->has_volume)
		syslog(LOG_DEBUG, "Control '%s' volume range: [%ld:%ld]",
		       c->name, c->min_volume_dB, c->max_volume_dB);

	if (dir == CRAS_STREAM_OUTPUT)
		DL_APPEND(cmix->output_controls, c);
	else if (dir == CRAS_STREAM_INPUT)
		DL_APPEND(cmix->input_controls, c);
	return 0;
}

static int add_control(struct cras_alsa_mixer *cmix,
		       enum CRAS_STREAM_DIRECTION dir,
		       snd_mixer_elem_t *elem)
{
	return add_control_with_name(cmix, dir, elem,
				     snd_mixer_selem_get_name(elem));
}

static void list_controls(struct mixer_control *control_list,
			  cras_alsa_mixer_control_callback cb,
			  void *cb_arg)
{
	struct mixer_control *control;

	DL_FOREACH(control_list, control)
		cb(control, cb_arg);
}

static struct mixer_control *get_control_matching_name(
		struct mixer_control *control_list,
		const char *name)
{
	struct mixer_control *c;

	DL_FOREACH(control_list, c) {
		if (strstr(name, c->name))
			return c;
	}
	return NULL;
}

/* Creates a mixer_control with multiple control elements. */
static int add_control_with_coupled_mixers(
				struct cras_alsa_mixer *cmix,
				enum CRAS_STREAM_DIRECTION dir,
				const char *name,
				struct mixer_name *coupled_controls)
{
	struct mixer_control *c;
	int rc;

	rc = mixer_control_create_by_name(
		 &c, cmix, name, coupled_controls, dir);
	if (rc)
		return rc;
	syslog(LOG_DEBUG, "Add %s control: %s\n",
	       dir == CRAS_STREAM_OUTPUT ? "output" : "input",
	       c->name);
	mixer_name_dump(coupled_controls, "  elements");

	if (c->has_volume)
		syslog(LOG_DEBUG, "Control '%s' volume range: [%ld:%ld]",
		       c->name, c->min_volume_dB, c->max_volume_dB);

	if (dir == CRAS_STREAM_OUTPUT)
		DL_APPEND(cmix->output_controls, c);
	else if (dir == CRAS_STREAM_INPUT)
		DL_APPEND(cmix->input_controls, c);
	return 0;
}

static int add_control_by_name(struct cras_alsa_mixer *cmix,
			       enum CRAS_STREAM_DIRECTION dir,
			       const char *name)
{
	struct mixer_control *c;
	struct mixer_name *m_name;
	int rc;

	m_name = mixer_name_add(NULL, name, dir, MIXER_NAME_VOLUME);
	if (!m_name)
		return -ENOMEM;

	rc = mixer_control_create_by_name(&c, cmix, name, m_name, dir);
	mixer_name_free(m_name);
	if (rc)
		return rc;
	syslog(LOG_DEBUG, "Add %s control: %s\n",
	       dir == CRAS_STREAM_OUTPUT ? "output" : "input",
	       c->name);

	if (c->has_volume)
		syslog(LOG_DEBUG, "Control '%s' volume range: [%ld:%ld]",
		       c->name, c->min_volume_dB, c->max_volume_dB);

	if (dir == CRAS_STREAM_OUTPUT)
		DL_APPEND(cmix->output_controls, c);
	else if (dir == CRAS_STREAM_INPUT)
		DL_APPEND(cmix->input_controls, c);
	return 0;
}

/*
 * Exported interface.
 */

struct cras_alsa_mixer *cras_alsa_mixer_create(const char *card_name)
{
	struct cras_alsa_mixer *cmix;

	cmix = (struct cras_alsa_mixer *)calloc(1, sizeof(*cmix));
	if (cmix == NULL)
		return NULL;

	syslog(LOG_DEBUG, "Add mixer for device %s", card_name);

	alsa_mixer_open(card_name, &cmix->mixer);

	return cmix;
}

int cras_alsa_mixer_add_controls_by_name_matching(
		struct cras_alsa_mixer *cmix,
		struct mixer_name *extra_controls,
		struct mixer_name *coupled_controls)
{
	/* Names of controls for main system volume. */
	static const char * const main_volume_names[] = {
		"Master",
		"Digital",
		"PCM",
	};
	/* Names of controls for individual outputs. */
	static const char * const output_names[] = {
		"Headphone",
		"Headset",
		"HDMI",
		"Speaker",
	};
	/* Names of controls for capture gain/attenuation and mute. */
	static const char * const main_capture_names[] = {
		"Capture",
		"Digital Capture",
	};
	/* Names of controls for individual inputs. */
	static const char * const input_names[] = {
		"Mic",
		"Microphone",
	};

	struct mixer_name *default_controls = NULL;
	snd_mixer_elem_t *elem;
	int extra_main_volume = 0;
	snd_mixer_elem_t *other_elem = NULL;
	long other_dB_range = 0;
	int rc = 0;

	/* Note that there is no mixer on some cards. This is acceptable. */
	if (cmix->mixer == NULL) {
		syslog(LOG_DEBUG, "Couldn't open mixer.");
		return 0;
	}

	default_controls = mixer_name_add_array(default_controls,
				output_names, ARRAY_SIZE(output_names),
				CRAS_STREAM_OUTPUT, MIXER_NAME_VOLUME);
	default_controls = mixer_name_add_array(default_controls,
				input_names, ARRAY_SIZE(input_names),
				CRAS_STREAM_INPUT, MIXER_NAME_VOLUME);
	default_controls =
		mixer_name_add_array(default_controls,
			main_volume_names, ARRAY_SIZE(main_volume_names),
			CRAS_STREAM_OUTPUT, MIXER_NAME_MAIN_VOLUME);
	default_controls =
		mixer_name_add_array(default_controls,
			main_capture_names, ARRAY_SIZE(main_capture_names),
			CRAS_STREAM_INPUT, MIXER_NAME_MAIN_VOLUME);
	extra_main_volume =
		mixer_name_find(extra_controls, NULL,
				CRAS_STREAM_OUTPUT,
				MIXER_NAME_MAIN_VOLUME) != NULL;

	/* Find volume and mute controls. */
	for(elem = snd_mixer_first_elem(cmix->mixer);
			elem != NULL; elem = snd_mixer_elem_next(elem)) {
		const char *name;
		struct mixer_name *control;
		int found = 0;

		name = snd_mixer_selem_get_name(elem);
		if (name == NULL)
			continue;

		/* Find a matching control. */
		control = mixer_name_find(default_controls, name,
					  CRAS_STREAM_OUTPUT,
					  MIXER_NAME_UNDEFINED);

		/* If our extra controls contain a main volume
		 * entry, and we found a main volume entry, then
		 * skip it. */
		if (extra_main_volume &&
		    control && control->type == MIXER_NAME_MAIN_VOLUME)
			control = NULL;

		/* If we didn't match any of the defaults, match
		 * the extras list. */
		if (!control)
			control = mixer_name_find(extra_controls, name,
					  CRAS_STREAM_OUTPUT,
					  MIXER_NAME_UNDEFINED);

		if (control) {
			int rc = -1;
			switch(control->type) {
			case MIXER_NAME_MAIN_VOLUME:
				rc = add_main_volume_control(cmix, elem);
				break;
			case MIXER_NAME_VOLUME:
				/* TODO(dgreid) - determine device index. */
				rc = add_control(
					cmix, CRAS_STREAM_OUTPUT, elem);
				break;
			case MIXER_NAME_UNDEFINED:
				rc = -EINVAL;
				break;
			}
			if (rc) {
				syslog(LOG_ERR,
				       "Failed to add mixer control '%s'"
				       " with type '%d'",
				       control->name, control->type);
				return rc;
			}
			found = 1;
		}

		/* Find a matching input control. */
		control = mixer_name_find(default_controls, name,
					  CRAS_STREAM_INPUT,
					  MIXER_NAME_UNDEFINED);

		/* If we didn't match any of the defaults, match
		   the extras list */
		if (!control)
			control = mixer_name_find(extra_controls, name,
					  CRAS_STREAM_INPUT,
					  MIXER_NAME_UNDEFINED);

		if (control) {
			int rc = -1;
			switch(control->type) {
			case MIXER_NAME_MAIN_VOLUME:
				rc = add_main_capture_control(cmix, elem);
				break;
			case MIXER_NAME_VOLUME:
				rc = add_control(
					cmix, CRAS_STREAM_INPUT, elem);
				break;
			case MIXER_NAME_UNDEFINED:
				rc = -EINVAL;
				break;
			}
			if (rc) {
				syslog(LOG_ERR,
				       "Failed to add mixer control '%s'"
				       " with type '%d'",
				       control->name, control->type);
				return rc;
			}
			found = 1;
		}

		if (!found && snd_mixer_selem_has_playback_volume(elem)) {
			/* Temporarily cache one elem whose name is not
			 * in the list above, but has a playback volume
			 * control and the largest volume range. */
			long min, max, range;
			if (snd_mixer_selem_get_playback_dB_range(elem,
								  &min,
								  &max) != 0)
				continue;

			range = max - min;
			if (other_dB_range < range) {
				other_dB_range = range;
				other_elem = elem;
			}
		}
	}

	/* Handle coupled output names for speaker */
	if (coupled_controls) {
		rc = add_control_with_coupled_mixers(
				cmix, CRAS_STREAM_OUTPUT,
				"Speaker", coupled_controls);
		if (rc) {
			syslog(LOG_ERR, "Could not add coupled output");
			return rc;
		}
	}

	/* If there is no volume control and output control found,
	 * use the volume control which has the largest volume range
	 * in the mixer as a main volume control. */
	if (!cmix->main_volume_controls && !cmix->output_controls &&
	    other_elem) {
		rc = add_main_volume_control(cmix, other_elem);
		if (rc) {
			syslog(LOG_ERR, "Could not add other volume control");
			return rc;
		}
	}

	return rc;
}

int cras_alsa_mixer_add_controls_in_section(
		struct cras_alsa_mixer *cmix,
		struct ucm_section *section)
{
	int rc;

	/* Note that there is no mixer on some cards. This is acceptable. */
	if (cmix->mixer == NULL) {
		syslog(LOG_DEBUG, "Couldn't open mixer.");
		return 0;
	}

	if (!section) {
		syslog(LOG_ERR, "No UCM SectionDevice specified.");
		return -EINVAL;
	}

	/* TODO(muirj) - Extra main volume controls when fully-specified. */

	if (section->mixer_name) {
		rc = add_control_by_name(
				cmix, section->dir, section->mixer_name);
		if (rc) {
			syslog(LOG_ERR, "Could not add mixer control '%s': %s",
			       section->mixer_name, strerror(-rc));
			return rc;
		}
	}

	if (section->coupled) {
		rc = add_control_with_coupled_mixers(
				cmix, section->dir,
				section->name, section->coupled);
		if (rc) {
			syslog(LOG_ERR, "Could not add coupled control: %s",
			       strerror(-rc));
			return rc;
		}
	}
	return 0;
}

void cras_alsa_mixer_destroy(struct cras_alsa_mixer *cras_mixer)
{
	assert(cras_mixer);

	mixer_control_destroy_list(cras_mixer->main_volume_controls);
	mixer_control_destroy_list(cras_mixer->main_capture_controls);
	mixer_control_destroy_list(cras_mixer->output_controls);
	mixer_control_destroy_list(cras_mixer->input_controls);
	if (cras_mixer->mixer)
		snd_mixer_close(cras_mixer->mixer);
	free(cras_mixer);
}

int cras_alsa_mixer_has_main_volume(
		const struct cras_alsa_mixer *cras_mixer)
{
	return !!cras_mixer->main_volume_controls;
}

int cras_alsa_mixer_has_volume(const struct mixer_control *mixer_control)
{
	return mixer_control && mixer_control->has_volume;
}

void cras_alsa_mixer_set_dBFS(struct cras_alsa_mixer *cras_mixer,
			      long dBFS,
			      struct mixer_control *mixer_output)
{
	struct mixer_control *c;
	long to_set;

	assert(cras_mixer);

	/* dBFS is normally < 0 to specify the attenuation from max. max is the
	 * combined max of the master controls and the current output.
	 */
	to_set = dBFS + cras_mixer->max_volume_dB;
	if (cras_alsa_mixer_has_volume(mixer_output))
		to_set += mixer_output->max_volume_dB;
	/* Go through all the controls, set the volume level for each,
	 * taking the value closest but greater than the desired volume.  If the
	 * entire volume can't be set on the current control, move on to the
	 * next one until we have the exact volume, or gotten as close as we
	 * can. Once all of the volume is set the rest of the controls should be
	 * set to 0dB. */
	DL_FOREACH(cras_mixer->main_volume_controls, c) {
		long actual_dB;

		if (!c->has_volume)
			continue;
		mixer_control_set_dBFS(c, to_set);
		mixer_control_get_dBFS(c, &actual_dB);
		to_set -= actual_dB;
	}
	/* Apply the rest to the output-specific control. */
	if (cras_alsa_mixer_has_volume(mixer_output))
		mixer_control_set_dBFS(mixer_output, to_set);
}

long cras_alsa_mixer_get_dB_range(struct cras_alsa_mixer *cras_mixer)
{
	if (!cras_mixer)
		return 0;
	return cras_mixer->max_volume_dB - cras_mixer->min_volume_dB;
}

long cras_alsa_mixer_get_output_dB_range(
		struct mixer_control *mixer_output)
{
	if (!cras_alsa_mixer_has_volume(mixer_output))
		return 0;

	return mixer_output->max_volume_dB - mixer_output->min_volume_dB;
}

void cras_alsa_mixer_set_capture_dBFS(struct cras_alsa_mixer *cras_mixer,
				      long dBFS,
				      struct mixer_control *mixer_input)
{
	struct mixer_control *c;
	long to_set;

	assert(cras_mixer);
	to_set = dBFS;
	/* Go through all the controls, set the gain for each, taking the value
	 * closest but greater than the desired gain.  If the entire gain can't
	 * be set on the current control, move on to the next one until we have
	 * the exact gain, or gotten as close as we can. Once all of the gain is
	 * set the rest of the controls should be set to 0dB. */
	DL_FOREACH(cras_mixer->main_capture_controls, c) {
		long actual_dB;

		if (!c->has_volume)
			continue;
		mixer_control_set_dBFS(c, to_set);
		mixer_control_get_dBFS(c, &actual_dB);
		to_set -= actual_dB;
	}

	/* Apply the reset to input specific control */
	if (cras_alsa_mixer_has_volume(mixer_input))
		mixer_control_set_dBFS(mixer_input, to_set);
}

long cras_alsa_mixer_get_minimum_capture_gain(
                struct cras_alsa_mixer *cmix,
		struct mixer_control *mixer_input)
{
	struct mixer_control *c;
	long total_min = 0;

	assert(cmix);
	DL_FOREACH(cmix->main_capture_controls, c)
		if (c->has_volume)
			total_min += c->min_volume_dB;
	if (mixer_input &&
	    mixer_input->has_volume)
		total_min += mixer_input->min_volume_dB;

	return total_min;
}

long cras_alsa_mixer_get_maximum_capture_gain(
		struct cras_alsa_mixer *cmix,
		struct mixer_control *mixer_input)
{
	struct mixer_control *c;
	long total_max = 0;

	assert(cmix);
	DL_FOREACH(cmix->main_capture_controls, c)
		if (c->has_volume)
			total_max += c->max_volume_dB;

	if (mixer_input &&
	    mixer_input->has_volume)
		total_max += mixer_input->max_volume_dB;

	return total_max;
}

void cras_alsa_mixer_set_mute(struct cras_alsa_mixer *cras_mixer,
			      int muted,
			      struct mixer_control *mixer_output)
{
	assert(cras_mixer);

	if (cras_mixer->playback_switch) {
		snd_mixer_selem_set_playback_switch_all(
				cras_mixer->playback_switch, !muted);
	}
	if (mixer_output && mixer_output->has_mute) {
		mixer_control_set_mute(mixer_output, muted);
	}
}

void cras_alsa_mixer_set_capture_mute(struct cras_alsa_mixer *cras_mixer,
				      int muted,
				      struct mixer_control *mixer_input)
{
	assert(cras_mixer);
	if (cras_mixer->capture_switch) {
		snd_mixer_selem_set_capture_switch_all(
				cras_mixer->capture_switch, !muted);
		return;
	}
	if (mixer_input && mixer_input->has_mute)
		mixer_control_set_mute(mixer_input, muted);
}

void cras_alsa_mixer_list_outputs(struct cras_alsa_mixer *cras_mixer,
				  cras_alsa_mixer_control_callback cb,
				  void *cb_arg)
{
	assert(cras_mixer);
	list_controls(cras_mixer->output_controls, cb, cb_arg);
}

void cras_alsa_mixer_list_inputs(struct cras_alsa_mixer *cras_mixer,
				 cras_alsa_mixer_control_callback cb,
				 void *cb_arg)
{
	assert(cras_mixer);
	list_controls(cras_mixer->input_controls, cb, cb_arg);
}

const char *cras_alsa_mixer_get_control_name(
		const struct mixer_control *control)
{
	if (!control)
		return NULL;
	return control->name;
}

struct mixer_control *cras_alsa_mixer_get_control_matching_name(
		struct cras_alsa_mixer *cras_mixer,
		enum CRAS_STREAM_DIRECTION dir, const char *name,
		int create_missing)
{
	struct mixer_control *c;

	assert(cras_mixer);
	if (!name)
		return NULL;

	if (dir == CRAS_STREAM_OUTPUT) {
		c = get_control_matching_name(
				cras_mixer->output_controls, name);
	} else if (dir == CRAS_STREAM_INPUT) {
		c = get_control_matching_name(
				cras_mixer->input_controls, name);
	} else {
		return NULL;
        }

	/* TODO: Allowing creation of a new control is a workaround: we
	 * should pass the input names in ucm config to
	 * cras_alsa_mixer_create. */
	if (!c && cras_mixer->mixer && create_missing) {
		int rc = add_control_by_name(cras_mixer, dir, name);
		if (rc)
			return NULL;
		c = cras_alsa_mixer_get_control_matching_name(
				cras_mixer, dir, name, 0);
	}
	return c;
}

struct mixer_control *cras_alsa_mixer_get_control_for_section(
		struct cras_alsa_mixer *cras_mixer,
		const struct ucm_section *section)
{
	assert(cras_mixer && section);
	if (section->mixer_name) {
		return cras_alsa_mixer_get_control_matching_name(
			   cras_mixer, section->dir, section->mixer_name, 0);
	} else if (section->coupled) {
		return cras_alsa_mixer_get_control_matching_name(
			   cras_mixer, section->dir, section->name, 0);
	}
	return NULL;
}

struct mixer_control *cras_alsa_mixer_get_output_matching_name(
		struct cras_alsa_mixer *cras_mixer,
		const char * const name)
{
	return cras_alsa_mixer_get_control_matching_name(
			cras_mixer, CRAS_STREAM_OUTPUT, name, 0);
}

struct mixer_control *cras_alsa_mixer_get_input_matching_name(
		struct cras_alsa_mixer *cras_mixer,
		const char *name)
{
	/* TODO: Allowing creation of a new control is a workaround: we
	 * should pass the input names in ucm config to
	 * cras_alsa_mixer_create. */
	return cras_alsa_mixer_get_control_matching_name(
			cras_mixer, CRAS_STREAM_INPUT, name, 1);
}

int cras_alsa_mixer_set_output_active_state(
		struct mixer_control *output,
		int active)
{
	assert(output);
	if (!output->has_mute)
		return -1;
	return mixer_control_set_mute(output, !active);
}
