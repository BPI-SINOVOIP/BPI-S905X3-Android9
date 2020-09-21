/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <alsa/use-case.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>

#include "cras_alsa_ucm.h"
#include "utlist.h"

static const char jack_var[] = "JackName";
static const char jack_type_var[] = "JackType";
static const char jack_switch_var[] = "JackSwitch";
static const char edid_var[] = "EDIDFile";
static const char cap_var[] = "CaptureControl";
static const char mic_positions[] = "MicPositions";
static const char override_type_name_var[] = "OverrideNodeType";
static const char output_dsp_name_var[] = "OutputDspName";
static const char input_dsp_name_var[] = "InputDspName";
static const char mixer_var[] = "MixerName";
static const char swap_mode_suffix[] = "Swap Mode";
static const char min_buffer_level_var[] = "MinBufferLevel";
static const char dma_period_var[] = "DmaPeriodMicrosecs";
static const char disable_software_volume[] = "DisableSoftwareVolume";
static const char playback_device_name_var[] = "PlaybackPCM";
static const char playback_device_rate_var[] = "PlaybackRate";
static const char capture_device_name_var[] = "CapturePCM";
static const char capture_device_rate_var[] = "CaptureRate";
static const char capture_channel_map_var[] = "CaptureChannelMap";
static const char coupled_mixers[] = "CoupledMixers";
/* Set this value in a SectionDevice to specify the maximum software gain in dBm
 * and enable software gain on this node. */
static const char max_software_gain[] = "MaxSoftwareGain";
/* Set this value in a SectionDevice to specify the default node gain in dBm. */
static const char default_node_gain[] = "DefaultNodeGain";
static const char hotword_model_prefix[] = "Hotword Model";
static const char fully_specified_ucm_var[] = "FullySpecifiedUCM";
static const char main_volume_names[] = "MainVolumeNames";
static const char enable_htimestamp_var[] = "EnableHtimestamp";

/* Use case verbs corresponding to CRAS_STREAM_TYPE. */
static const char *use_case_verbs[] = {
	"HiFi",
	"Multimedia",
	"Voice Call",
	"Speech",
	"Pro Audio",
};

/* Represents a list of section names found in UCM. */
struct section_name {
	const char* name;
	struct section_name  *prev, *next;
};

struct cras_use_case_mgr {
	snd_use_case_mgr_t *mgr;
	const char *name;
	unsigned int avail_use_cases;
	enum CRAS_STREAM_TYPE use_case;
};

static inline const char *uc_verb(struct cras_use_case_mgr *mgr)
{
	return use_case_verbs[mgr->use_case];
}

static int device_enabled(struct cras_use_case_mgr *mgr, const char *dev)
{
	const char **list;
	unsigned int i;
	int num_devs;
	int enabled = 0;

	num_devs = snd_use_case_get_list(mgr->mgr, "_enadevs", &list);
	if (num_devs <= 0)
		return 0;

	for (i = 0; i < (unsigned int)num_devs; i++)
		if (!strcmp(dev, list[i])) {
			enabled = 1;
			break;
		}

	snd_use_case_free_list(list, num_devs);
	return enabled;
}

static int modifier_enabled(struct cras_use_case_mgr *mgr, const char *mod)
{
	const char **list;
	unsigned int mod_idx;
	int num_mods;

	num_mods = snd_use_case_get_list(mgr->mgr, "_enamods", &list);
	if (num_mods <= 0)
		return 0;

	for (mod_idx = 0; mod_idx < (unsigned int)num_mods; mod_idx++)
		if (!strcmp(mod, list[mod_idx]))
			break;

	snd_use_case_free_list(list, num_mods);
	return (mod_idx < (unsigned int)num_mods);
}

static int get_var(struct cras_use_case_mgr *mgr, const char *var,
		   const char *dev, const char *verb, const char **value)
{
	char *id;
	int rc;
	size_t len = strlen(var) + strlen(dev) + strlen(verb) + 4;

	id = (char *)malloc(len);
	if (!id)
		return -ENOMEM;
	snprintf(id, len, "=%s/%s/%s", var, dev, verb);
	rc = snd_use_case_get(mgr->mgr, id, value);

	free((void *)id);
	return rc;
}

static int get_int(struct cras_use_case_mgr *mgr, const char *var,
		   const char *dev, const char *verb, int *value)
{
	const char *str_value;
	int rc;

	if (!value)
		return -EINVAL;
	rc = get_var(mgr, var, dev, verb, &str_value);
	if (rc != 0)
		return rc;
	*value = atoi(str_value);
	free((void *)str_value);
	return 0;
}

static int ucm_set_modifier_enabled(struct cras_use_case_mgr *mgr,
				    const char *mod, int enable)
{
	return snd_use_case_set(mgr->mgr, enable ? "_enamod" : "_dismod", mod);
}

static int ucm_str_ends_with_suffix(const char *str, const char *suffix)
{
	if (!str || !suffix)
		return 0;
	size_t len_str = strlen(str);
	size_t len_suffix = strlen(suffix);
	if (len_suffix > len_str)
		return 0;
	return strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

static int ucm_section_exists_with_name(struct cras_use_case_mgr *mgr,
		const char *name, const char *identifier)
{
	const char **list;
	unsigned int i;
	int num_entries;
	int exist = 0;

	num_entries = snd_use_case_get_list(mgr->mgr, identifier, &list);
	if (num_entries <= 0)
		return 0;

	for (i = 0; i < (unsigned int)num_entries; i+=2) {

		if (!list[i])
			continue;

		if (strcmp(list[i], name) == 0) {
			exist = 1;
			break;
		}
	}
	snd_use_case_free_list(list, num_entries);
	return exist;
}

static int ucm_section_exists_with_suffix(struct cras_use_case_mgr *mgr,
		const char *suffix, const char *identifier)
{
	const char **list;
	unsigned int i;
	int num_entries;
	int exist = 0;

	num_entries = snd_use_case_get_list(mgr->mgr, identifier, &list);
	if (num_entries <= 0)
		return 0;

	for (i = 0; i < (unsigned int)num_entries; i+=2) {

		if (!list[i])
			continue;

		if (ucm_str_ends_with_suffix(list[i], suffix)) {
			exist = 1;
			break;
		}
	}
	snd_use_case_free_list(list, num_entries);
	return exist;
}

static int ucm_mod_exists_with_suffix(struct cras_use_case_mgr *mgr,
				      const char *suffix)
{
	char *identifier;
	int rc;

	identifier = snd_use_case_identifier("_modifiers/%s", uc_verb(mgr));
	rc = ucm_section_exists_with_suffix(mgr, suffix, identifier);
	free(identifier);
	return rc;
}

static int ucm_mod_exists_with_name(struct cras_use_case_mgr *mgr,
				    const char *name)
{
	char *identifier;
	int rc;

	identifier = snd_use_case_identifier("_modifiers/%s", uc_verb(mgr));
	rc = ucm_section_exists_with_name(mgr, name, identifier);
	free(identifier);
	return rc;
}

/* Get a list of section names whose variable is the matched value. */
static struct section_name * ucm_get_sections_for_var(
		struct cras_use_case_mgr *mgr,
		const char *var, const char *value,
		const char *identifier,
		enum CRAS_STREAM_DIRECTION direction)
{
	const char **list;
	struct section_name *section_names = NULL, *s_name;
	unsigned int i;
	int num_entries;
	int rc;

	num_entries = snd_use_case_get_list(mgr->mgr, identifier, &list);
	if (num_entries <= 0)
		return NULL;

	/* snd_use_case_get_list fills list with pairs of device name and
	 * comment, so device names are in even-indexed elements. */
	for (i = 0; i < (unsigned int)num_entries; i+=2) {
		const char *this_value;

		if (!list[i])
			continue;

		rc = get_var(mgr, var, list[i], uc_verb(mgr), &this_value);
		if (rc)
			continue;

		if (!strcmp(value, this_value)) {
			s_name = (struct section_name *)malloc(
					sizeof(struct section_name));

			if (!s_name) {
				syslog(LOG_ERR, "Failed to allocate memory");
				free((void *)this_value);
				break;
			}

			s_name->name = strdup(list[i]);
			DL_APPEND(section_names, s_name);
		}
		free((void *)this_value);
	}

	snd_use_case_free_list(list, num_entries);
	return section_names;
}

static struct section_name *ucm_get_devices_for_var(
		struct cras_use_case_mgr *mgr,
		const char *var, const char *value,
		enum CRAS_STREAM_DIRECTION dir)
{
	char *identifier;
	struct section_name *section_names;

	identifier = snd_use_case_identifier("_devices/%s", uc_verb(mgr));
	section_names = ucm_get_sections_for_var(mgr, var, value, identifier,
						 dir);
	free(identifier);
	return section_names;
}

static const char *ucm_get_playback_device_name_for_dev(
		struct cras_use_case_mgr *mgr, const char *dev)
{
	const char *name = NULL;
	int rc;

	rc = get_var(mgr, playback_device_name_var, dev, uc_verb(mgr), &name);
	if (rc)
		return NULL;

	return name;
}

static const char *ucm_get_capture_device_name_for_dev(
		struct cras_use_case_mgr *mgr, const char *dev)
{
	const char *name = NULL;
	int rc;

	rc = get_var(mgr, capture_device_name_var, dev, uc_verb(mgr), &name);
	if (rc)
		return NULL;

	return name;
}

/* Get a list of mixer names specified in a UCM variable separated by ",".
 * E.g. "Left Playback,Right Playback".
 */
static struct mixer_name *ucm_get_mixer_names(struct cras_use_case_mgr *mgr,
				const char *dev, const char* var,
				enum CRAS_STREAM_DIRECTION dir,
				mixer_name_type type)
{
	const char *names_in_string = NULL;
	int rc;
	char *tokens, *name, *laststr;
	struct mixer_name *names = NULL;

	rc = get_var(mgr, var, dev, uc_verb(mgr), &names_in_string);
	if (rc)
		return NULL;

	tokens = strdup(names_in_string);
	name = strtok_r(tokens, ",", &laststr);
	while (name != NULL) {
		names = mixer_name_add(names, name, dir, type);
		name = strtok_r(NULL, ",", &laststr);
	}
	free((void*)names_in_string);
	free(tokens);
	return names;
}

/* Exported Interface */

struct cras_use_case_mgr *ucm_create(const char *name)
{
	struct cras_use_case_mgr *mgr;
	int rc;
	const char **list;
	int num_verbs, i, j;

	if (!name)
		return NULL;

	mgr = (struct cras_use_case_mgr *)malloc(sizeof(*mgr));
	if (!mgr)
		return NULL;

	rc = snd_use_case_mgr_open(&mgr->mgr, name);
	if (rc) {
		syslog(LOG_WARNING, "Can not open ucm for card %s, rc = %d",
		       name, rc);
		goto cleanup;
	}

	mgr->name = name;
	mgr->avail_use_cases = 0;
	num_verbs = snd_use_case_get_list(mgr->mgr, "_verbs", &list);
	for (i = 0; i < num_verbs; i += 2) {
		for (j = 0; j < CRAS_STREAM_NUM_TYPES; ++j) {
			if (strcmp(list[i], use_case_verbs[j]) == 0)
				break;
		}
		if (j < CRAS_STREAM_NUM_TYPES)
			mgr->avail_use_cases |= (1 << j);
	}
	if (num_verbs > 0)
		snd_use_case_free_list(list, num_verbs);

	rc = ucm_set_use_case(mgr, CRAS_STREAM_TYPE_DEFAULT);
	if (rc)
		goto cleanup_mgr;

	return mgr;

cleanup_mgr:
	snd_use_case_mgr_close(mgr->mgr);
cleanup:
	free(mgr);
	return NULL;
}

void ucm_destroy(struct cras_use_case_mgr *mgr)
{
	snd_use_case_mgr_close(mgr->mgr);
	free(mgr);
}

int ucm_set_use_case(struct cras_use_case_mgr *mgr,
		     enum CRAS_STREAM_TYPE use_case)
{
	int rc;

	if (mgr->avail_use_cases & (1 << use_case)) {
		mgr->use_case = use_case;
	} else {
		syslog(LOG_ERR, "Unavailable use case %d for card %s",
		       use_case, mgr->name);
		return -1;
	}

	rc = snd_use_case_set(mgr->mgr, "_verb", uc_verb(mgr));
	if (rc) {
		syslog(LOG_ERR, "Can not set verb %s for card %s, rc = %d",
		       uc_verb(mgr), mgr->name, rc);
		return rc;
	}

	return 0;
}

int ucm_swap_mode_exists(struct cras_use_case_mgr *mgr)
{
	return ucm_mod_exists_with_suffix(mgr, swap_mode_suffix);
}

int ucm_enable_swap_mode(struct cras_use_case_mgr *mgr, const char *node_name,
			 int enable)
{
	char *swap_mod = NULL;
	int rc;
	size_t len = strlen(node_name) + 1 + strlen(swap_mode_suffix) + 1;
	swap_mod = (char *)malloc(len);
	if (!swap_mod)
		return -ENOMEM;
	snprintf(swap_mod, len, "%s %s", node_name, swap_mode_suffix);
	if (!ucm_mod_exists_with_name(mgr, swap_mod)) {
		syslog(LOG_ERR, "Can not find swap mode modifier %s.", swap_mod);
		free((void *)swap_mod);
		return -EPERM;
	}
	if (modifier_enabled(mgr, swap_mod) == !!enable) {
		free((void *)swap_mod);
		return 0;
	}
	rc = ucm_set_modifier_enabled(mgr, swap_mod, enable);
	free((void *)swap_mod);
	return rc;
}

int ucm_set_enabled(struct cras_use_case_mgr *mgr, const char *dev, int enable)
{
	if (device_enabled(mgr, dev) == !!enable)
		return 0;
	syslog(LOG_DEBUG, "UCM %s %s", enable ? "enable" : "disable", dev);
	return snd_use_case_set(mgr->mgr, enable ? "_enadev" : "_disdev", dev);
}

char *ucm_get_flag(struct cras_use_case_mgr *mgr, const char *flag_name)
{
	char *flag_value = NULL;
	const char *value;
	int rc;

	/* Set device to empty string since flag is specified in verb section */
	rc = get_var(mgr, flag_name, "", uc_verb(mgr), &value);
	if (!rc) {
		flag_value = strdup(value);
		free((void *)value);
	}

	return flag_value;
}

char *ucm_get_cap_control(struct cras_use_case_mgr *mgr, const char *ucm_dev)
{
	char *control_name = NULL;
	const char *value;
	int rc;

	rc = get_var(mgr, cap_var, ucm_dev, uc_verb(mgr), &value);
	if (!rc) {
		control_name = strdup(value);
		free((void *)value);
	}

	return control_name;
}

char *ucm_get_mic_positions(struct cras_use_case_mgr *mgr)
{
	char *control_name = NULL;
	const char *value;
	int rc;

	rc = get_var(mgr, mic_positions, "", uc_verb(mgr), &value);
	if (!rc) {
		control_name = strdup(value);
		free((void *)value);
	}

	return control_name;
}

const char *ucm_get_override_type_name(struct cras_use_case_mgr *mgr,
				       const char *dev)
{
	const char *override_type_name;
	int rc;

	rc = get_var(mgr, override_type_name_var, dev, uc_verb(mgr),
		     &override_type_name);
	if (rc)
		return NULL;

	return override_type_name;
}

char *ucm_get_dev_for_jack(struct cras_use_case_mgr *mgr, const char *jack,
			   enum CRAS_STREAM_DIRECTION direction)
{
	struct section_name *section_names, *c;
	char *ret = NULL;

	section_names = ucm_get_devices_for_var(mgr, jack_var, jack, direction);

	DL_FOREACH(section_names, c) {
		if (!strcmp(c->name, "Mic")) {
			/* Skip mic section for output */
			if (direction == CRAS_STREAM_OUTPUT)
				continue;
		} else {
			/* Only check mic for input. */
			if (direction == CRAS_STREAM_INPUT)
				continue;
		}
		ret = strdup(c->name);
		break;
	}

	DL_FOREACH(section_names, c) {
		DL_DELETE(section_names, c);
		free((void*)c->name);
		free(c);
	}

	return ret;
}

char *ucm_get_dev_for_mixer(struct cras_use_case_mgr *mgr, const char *mixer,
			    enum CRAS_STREAM_DIRECTION dir)
{
	struct section_name *section_names, *c;
	char *ret = NULL;

	section_names = ucm_get_devices_for_var(mgr, mixer_var, mixer, dir);

	if (section_names)
		ret = strdup(section_names->name);

	DL_FOREACH(section_names, c) {
		DL_DELETE(section_names, c);
		free((void*)c->name);
		free(c);
	}

	return ret;
}

const char *ucm_get_edid_file_for_dev(struct cras_use_case_mgr *mgr,
				      const char *dev)
{
	const char *file_name;
	int rc;

	rc = get_var(mgr, edid_var, dev, uc_verb(mgr), &file_name);
	if (rc)
		return NULL;

	return file_name;
}

const char *ucm_get_dsp_name(struct cras_use_case_mgr *mgr, const char *ucm_dev,
			     int direction)
{
	const char *var = (direction == CRAS_STREAM_OUTPUT)
		? output_dsp_name_var
		: input_dsp_name_var;
	const char *dsp_name = NULL;
	int rc;

	rc = get_var(mgr, var, ucm_dev, uc_verb(mgr), &dsp_name);
	if (rc)
		return NULL;

	return dsp_name;
}

const char *ucm_get_dsp_name_default(struct cras_use_case_mgr *mgr,
				     int direction)
{
	return ucm_get_dsp_name(mgr, "", direction);
}

unsigned int ucm_get_min_buffer_level(struct cras_use_case_mgr *mgr)
{
	int value;
	int rc;

	rc = get_int(mgr, min_buffer_level_var, "", uc_verb(mgr), &value);
	if (rc)
		return 0;

	return value;
}

unsigned int ucm_get_disable_software_volume(struct cras_use_case_mgr *mgr)
{
	int value;
	int rc;

	rc = get_int(mgr, disable_software_volume, "", uc_verb(mgr), &value);
	if (rc)
		return 0;

	return value;
}

int ucm_get_max_software_gain(struct cras_use_case_mgr *mgr, const char *dev,
			      long *gain)
{
	int value;
	int rc;

	rc = get_int(mgr, max_software_gain, dev, uc_verb(mgr), &value);
	if (rc)
		return rc;
	*gain = value;
	return 0;
}

int ucm_get_default_node_gain(struct cras_use_case_mgr *mgr, const char *dev,
			      long *gain)
{
	int value;
	int rc;

	rc = get_int(mgr, default_node_gain, dev, uc_verb(mgr), &value);
	if (rc)
		return rc;
	*gain = value;
	return 0;
}

const char *ucm_get_device_name_for_dev(
	struct cras_use_case_mgr *mgr, const char *dev,
	enum CRAS_STREAM_DIRECTION direction)
{
	if (direction == CRAS_STREAM_OUTPUT)
		return ucm_get_playback_device_name_for_dev(mgr, dev);
	else if (direction == CRAS_STREAM_INPUT)
		return ucm_get_capture_device_name_for_dev(mgr, dev);
	return NULL;
}

int ucm_get_sample_rate_for_dev(struct cras_use_case_mgr *mgr, const char *dev,
				enum CRAS_STREAM_DIRECTION direction)
{
	int value;
	int rc;
	const char *var_name;

	if (direction == CRAS_STREAM_OUTPUT)
		var_name = playback_device_rate_var;
	else if (direction == CRAS_STREAM_INPUT)
		var_name = capture_device_rate_var;
	else
		return -EINVAL;

	rc = get_int(mgr, var_name, dev, uc_verb(mgr), &value);
	if (rc)
		return rc;

	return value;
}

int ucm_get_capture_chmap_for_dev(struct cras_use_case_mgr *mgr,
				  const char *dev,
				  int8_t *channel_layout)
{
	const char *var_str;
	char *tokens, *token;
	int i, rc;

	rc = get_var(mgr, capture_channel_map_var, dev, uc_verb(mgr), &var_str);
	if (rc)
		return rc;

	tokens = strdup(var_str);
	token = strtok(tokens, " ");
	for (i = 0; token && (i < CRAS_CH_MAX); i++) {
		channel_layout[i] = atoi(token);
		token = strtok(NULL, " ");
	}

	free((void *)tokens);
	free((void *)var_str);
	return (i == CRAS_CH_MAX) ? 0 : -EINVAL;
}

struct mixer_name *ucm_get_coupled_mixer_names(
		struct cras_use_case_mgr *mgr, const char *dev)
{
	return ucm_get_mixer_names(mgr, dev, coupled_mixers,
				   CRAS_STREAM_OUTPUT,
				   MIXER_NAME_VOLUME);
}

static int get_device_index_from_target(const char *target_device_name)
{
	/* Expects a string in the form: hw:card-name,<num> */
	const char *pos = target_device_name;
	if (!pos)
		return -1;
	while (*pos && *pos != ',')
		++pos;
	if (*pos == ',') {
		++pos;
		return atoi(pos);
	}
	return -1;
}

struct ucm_section *ucm_get_sections(struct cras_use_case_mgr *mgr)
{
	struct ucm_section *sections = NULL;
	struct ucm_section *dev_sec;
	const char **list;
	int num_devs;
	int i;
	char *identifier;

	/* Find the list of all mixers using the control names defined in
	 * the header definintion for this function.  */
	identifier = snd_use_case_identifier("_devices/%s", uc_verb(mgr));
	num_devs = snd_use_case_get_list(mgr->mgr, identifier, &list);
	free(identifier);

	/* snd_use_case_get_list fills list with pairs of device name and
	 * comment, so device names are in even-indexed elements. */
	for (i = 0; i < num_devs; i += 2) {
		enum CRAS_STREAM_DIRECTION dir = CRAS_STREAM_UNDEFINED;
		int dev_idx = -1;
		const char *dev_name = strdup(list[i]);
		const char *jack_name;
		const char *jack_type;
		const char *mixer_name;
		struct mixer_name *m_name;
		int rc;
		const char *target_device_name;

		if (!dev_name)
			continue;

		target_device_name =
			ucm_get_playback_device_name_for_dev(mgr, dev_name);
		if (target_device_name)
			dir = CRAS_STREAM_OUTPUT;
		else {
			target_device_name =
				ucm_get_capture_device_name_for_dev(
					mgr, dev_name);
			if (target_device_name)
				dir = CRAS_STREAM_INPUT;
		}
		if (target_device_name) {
			dev_idx = get_device_index_from_target(
					target_device_name);
			free((void *)target_device_name);
		}

		if (dir == CRAS_STREAM_UNDEFINED) {
			syslog(LOG_ERR,
			       "UCM configuration for device '%s' missing"
			       " PlaybackPCM or CapturePCM definition.",
			       dev_name);
			goto error_cleanup;
		}

		if (dev_idx == -1) {
			syslog(LOG_ERR,
			       "PlaybackPCM or CapturePCM for '%s' must be in"
			       " the form 'hw:<card>,<number>'", dev_name);
			goto error_cleanup;
		}

		jack_name = ucm_get_jack_name_for_dev(mgr, dev_name);
		jack_type = ucm_get_jack_type_for_dev(mgr, dev_name);
		mixer_name = ucm_get_mixer_name_for_dev(mgr, dev_name);

		dev_sec = ucm_section_create(dev_name, dev_idx, dir,
					     jack_name, jack_type);
		if (jack_name)
			free((void *)jack_name);
		if (jack_type)
			free((void *)jack_type);

		if (!dev_sec) {
			syslog(LOG_ERR, "Failed to allocate memory.");
			if (mixer_name)
				free((void *)mixer_name);
			goto error_cleanup;
		}

		dev_sec->jack_switch =
			ucm_get_jack_switch_for_dev(mgr, dev_name);

		if (mixer_name) {
			rc = ucm_section_set_mixer_name(dev_sec, mixer_name);
			free((void *)mixer_name);
			if (rc)
				goto error_cleanup;
		}

		m_name = ucm_get_mixer_names(mgr, dev_name, coupled_mixers,
					     dir, MIXER_NAME_VOLUME);
		ucm_section_concat_coupled(dev_sec, m_name);

		DL_APPEND(sections, dev_sec);
		ucm_section_dump(dev_sec);
	}

	if (num_devs > 0)
		snd_use_case_free_list(list, num_devs);
	return sections;

error_cleanup:
	if (num_devs > 0)
		snd_use_case_free_list(list, num_devs);
	ucm_section_free_list(sections);
	return NULL;
}

char *ucm_get_hotword_models(struct cras_use_case_mgr *mgr)
{
	const char **list;
	int i, num_entries;
	int models_len = 0;
	char *models = NULL;
	const char *tmp;
	char *identifier;

	identifier = snd_use_case_identifier("_modifiers/%s", uc_verb(mgr));
	num_entries = snd_use_case_get_list(mgr->mgr, identifier, &list);
	free(identifier);
	if (num_entries <= 0)
		return 0;
	models = (char *)malloc(num_entries * 8);
	for (i = 0; i < num_entries; i+=2) {
		if (!list[i])
			continue;
		if (0 == strncmp(list[i], hotword_model_prefix,
				 strlen(hotword_model_prefix))) {
			tmp = list[i] + strlen(hotword_model_prefix);
			while (isspace(*tmp))
				tmp++;
			strcpy(models + models_len, tmp);
			models_len += strlen(tmp);
			if (i + 2 >= num_entries)
				models[models_len] = '\0';
			else
				models[models_len++] = ',';
		}
	}
	snd_use_case_free_list(list, num_entries);

	return models;
}

int ucm_set_hotword_model(struct cras_use_case_mgr *mgr, const char *model)
{
	const char **list;
	int num_enmods, mod_idx;
	char *model_mod = NULL;
	size_t model_mod_size = strlen(model) + 1 +
				strlen(hotword_model_prefix) + 1;
	model_mod = (char *)malloc(model_mod_size);
	if (!model_mod)
		return -ENOMEM;
	snprintf(model_mod, model_mod_size,
		 "%s %s", hotword_model_prefix, model);
	if (!ucm_mod_exists_with_name(mgr, model_mod)) {
		free((void *)model_mod);
		return -EINVAL;
	}

	/* Disable all currently enabled horword model modifiers. */
	num_enmods = snd_use_case_get_list(mgr->mgr, "_enamods", &list);
	if (num_enmods <= 0)
		goto enable_mod;

	for (mod_idx = 0; mod_idx < num_enmods; mod_idx++) {
		if (!strncmp(list[mod_idx], hotword_model_prefix,
			     strlen(hotword_model_prefix)))
			ucm_set_modifier_enabled(mgr, list[mod_idx], 0);
	}
	snd_use_case_free_list(list, num_enmods);

enable_mod:
	ucm_set_modifier_enabled(mgr, model_mod, 1);

	return 0;
}

int ucm_has_fully_specified_ucm_flag(struct cras_use_case_mgr *mgr)
{
	char *flag;
	int ret = 0;
	flag = ucm_get_flag(mgr, fully_specified_ucm_var);
	if (!flag)
		return 0;
	ret = !strcmp(flag, "1");
	free(flag);
	return ret;
}

const char *ucm_get_mixer_name_for_dev(struct cras_use_case_mgr *mgr, const char *dev)
{
	const char *name = NULL;
	int rc;

	rc = get_var(mgr, mixer_var, dev, uc_verb(mgr), &name);
	if (rc)
		return NULL;

	return name;
}

struct mixer_name *ucm_get_main_volume_names(struct cras_use_case_mgr *mgr)
{
	return ucm_get_mixer_names(mgr, "", main_volume_names,
				   CRAS_STREAM_OUTPUT, MIXER_NAME_MAIN_VOLUME);
}

int ucm_list_section_devices_by_device_name(
		struct cras_use_case_mgr *mgr,
		enum CRAS_STREAM_DIRECTION direction,
		const char *device_name,
		ucm_list_section_devices_callback cb,
		void *cb_arg)
{
	int listed= 0;
	struct section_name *section_names, *c;
	const char* var;
	char *identifier;

	if (direction == CRAS_STREAM_OUTPUT)
		var = playback_device_name_var;
	else if (direction == CRAS_STREAM_INPUT)
		var = capture_device_name_var;
	else
		return 0;

	identifier = snd_use_case_identifier("_devices/%s", uc_verb(mgr));
	section_names = ucm_get_sections_for_var(
		mgr, var, device_name, identifier, direction);
	free(identifier);
	if (!section_names)
		return 0;

	DL_FOREACH(section_names, c) {
		cb(c->name, cb_arg);
		listed++;
	}

	DL_FOREACH(section_names, c) {
		DL_DELETE(section_names, c);
		free((void*)c->name);
		free(c);
	}
	return listed;
}

const char *ucm_get_jack_name_for_dev(struct cras_use_case_mgr *mgr,
				      const char *dev)
{
	const char *name = NULL;
	int rc;

	rc = get_var(mgr, jack_var, dev, uc_verb(mgr), &name);
	if (rc)
		return NULL;

	return name;
}

const char *ucm_get_jack_type_for_dev(struct cras_use_case_mgr *mgr,
				      const char *dev)
{
	const char *name = NULL;
	int rc;

	rc = get_var(mgr, jack_type_var, dev, uc_verb(mgr), &name);
	if (rc)
		return NULL;

	if (strcmp(name, "hctl") && strcmp(name, "gpio")) {
		syslog(LOG_ERR, "Unknown jack type: %s", name);
		return NULL;
	}
	return name;
}

int ucm_get_jack_switch_for_dev(struct cras_use_case_mgr *mgr, const char *dev)
{
	int value;

	int rc = get_int(mgr, jack_switch_var, dev, uc_verb(mgr), &value);
	if (rc || value < 0)
		return -1;
	return value;
}

unsigned int ucm_get_dma_period_for_dev(struct cras_use_case_mgr *mgr,
					const char *dev)
{
	int value;

	int rc = get_int(mgr, dma_period_var, dev, uc_verb(mgr), &value);
	if (rc || value < 0)
		return 0;
	return value;
}

unsigned int ucm_get_enable_htimestamp_flag(struct cras_use_case_mgr *mgr)
{
	char *flag;
	int ret = 0;
	flag = ucm_get_flag(mgr, enable_htimestamp_var);
	if (!flag)
		return 0;
	ret = !strcmp(flag, "1");
	free(flag);
	return ret;
}
