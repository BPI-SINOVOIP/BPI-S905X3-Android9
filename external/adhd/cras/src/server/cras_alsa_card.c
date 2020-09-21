/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* For asprintf */
#endif

#include <alsa/asoundlib.h>
#include <syslog.h>

#include "cras_alsa_card.h"
#include "cras_alsa_io.h"
#include "cras_alsa_mixer.h"
#include "cras_alsa_ucm.h"
#include "cras_device_blacklist.h"
#include "cras_card_config.h"
#include "cras_config.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_system_state.h"
#include "cras_types.h"
#include "cras_util.h"
#include "utlist.h"

#define MAX_ALSA_CARDS 32 /* Alsa limit on number of cards. */
#define MAX_ALSA_PCM_NAME_LENGTH 6 /* Alsa names "hw:XX" + 1 for null. */
#define MAX_INI_NAME_LENGTH 63 /* 63 chars + 1 for null where declared. */
#define MAX_COUPLED_OUTPUT_SIZE 4

struct iodev_list_node {
	struct cras_iodev *iodev;
	enum CRAS_STREAM_DIRECTION direction;
	struct iodev_list_node *prev, *next;
};

/* Keeps an fd that is registered with system state.  A list of fds must be
 * kept so that they can be removed when the card is destroyed. */
struct hctl_poll_fd {
	int fd;
	struct hctl_poll_fd *prev, *next;
};

/* Holds information about each sound card on the system.
 * name - of the form hw:XX,YY.
 * card_index - 0 based index, value of "XX" in the name.
 * iodevs - Input and output devices for this card.
 * mixer - Controls the mixer controls for this card.
 * ucm - CRAS use case manager if available.
 * hctl - ALSA high-level control interface.
 * hctl_poll_fds - List of fds registered with cras_system_state.
 * config - Config info for this card, can be NULL if none found.
 */
struct cras_alsa_card {
	char name[MAX_ALSA_PCM_NAME_LENGTH];
	size_t card_index;
	struct iodev_list_node *iodevs;
	struct cras_alsa_mixer *mixer;
	struct cras_use_case_mgr *ucm;
	snd_hctl_t *hctl;
	struct hctl_poll_fd *hctl_poll_fds;
	struct cras_card_config *config;
};

/* Creates an iodev for the given device.
 * Args:
 *    alsa_card - the alsa_card the device will be added to.
 *    info - Information about the card type and priority.
 *    card_name - The name of the card.
 *    dev_name - The name of the device.
 *    dev_id - The id string of the device.
 *    device_index - 0 based index, value of "YY" in "hw:XX,YY".
 *    direction - Input or output.
 * Returns:
 *    Pointer to the created iodev, or NULL on error.
 *    other negative error code otherwise.
 */
struct cras_iodev *create_iodev_for_device(
		struct cras_alsa_card *alsa_card,
		struct cras_alsa_card_info *info,
		const char *card_name,
		const char *dev_name,
		const char *dev_id,
		unsigned device_index,
		enum CRAS_STREAM_DIRECTION direction)
{
	struct iodev_list_node *new_dev;
	struct iodev_list_node *node;
	int first = 1;

	/* Find whether this is the first device in this direction, and
	 * avoid duplicate device indexes. */
	DL_FOREACH(alsa_card->iodevs, node) {
		if (node->direction != direction)
			continue;
		first = 0;
		if (alsa_iodev_index(node->iodev) == device_index) {
			syslog(LOG_DEBUG,
			       "Skipping duplicate device for %s:%s:%s [%u]",
			       card_name, dev_name, dev_id, device_index);
			return node->iodev;
		}
	}

	new_dev = calloc(1, sizeof(*new_dev));
	if (new_dev == NULL)
		return NULL;

	new_dev->direction = direction;
	new_dev->iodev = alsa_iodev_create(info->card_index,
					   card_name,
					   device_index,
					   dev_name,
					   dev_id,
					   info->card_type,
					   first,
					   alsa_card->mixer,
					   alsa_card->config,
					   alsa_card->ucm,
					   alsa_card->hctl,
					   direction,
					   info->usb_vendor_id,
					   info->usb_product_id,
					   info->usb_serial_number);
	if (new_dev->iodev == NULL) {
		syslog(LOG_ERR, "Couldn't create alsa_iodev for %u:%u\n",
		       info->card_index, device_index);
		free(new_dev);
		return NULL;
	}

	syslog(LOG_DEBUG, "New %s device %u:%d",
	       direction == CRAS_STREAM_OUTPUT ? "playback" : "capture",
	       info->card_index,
	       device_index);

	DL_APPEND(alsa_card->iodevs, new_dev);
	return new_dev->iodev;
}

/* Returns non-zero if this card has hctl jacks.
 */
static int card_has_hctl_jack(struct cras_alsa_card *alsa_card)
{
	struct iodev_list_node *node;

	/* Find the first device that has an hctl jack. */
	DL_FOREACH(alsa_card->iodevs, node) {
		if (alsa_iodev_has_hctl_jacks(node->iodev))
			return 1;
	}
	return 0;
}

/* Check if a device should be ignored for this card. Returns non-zero if the
 * device is in the blacklist and should be ignored.
 */
static int should_ignore_dev(struct cras_alsa_card_info *info,
			     struct cras_device_blacklist *blacklist,
			     size_t device_index)
{
	if (info->card_type == ALSA_CARD_TYPE_USB)
		return cras_device_blacklist_check(blacklist,
						   info->usb_vendor_id,
						   info->usb_product_id,
						   info->usb_desc_checksum,
						   device_index);
	return 0;
}

/* Filters an array of mixer control names. Keep a name if it is
 * specified in the ucm config. */
static struct mixer_name *filter_controls(struct cras_use_case_mgr *ucm,
					  struct mixer_name *controls)
{
	struct mixer_name *control;
	DL_FOREACH(controls, control) {
		char *dev = ucm_get_dev_for_mixer(ucm, control->name,
						  CRAS_STREAM_OUTPUT);
		if (!dev)
			DL_DELETE(controls, control);
	}
	return controls;
}

/* Handles notifications from alsa controls.  Called by main thread when a poll
 * fd provided by alsa signals there is an event available. */
static void alsa_control_event_pending(void *arg)
{
	struct cras_alsa_card *card;

	card = (struct cras_alsa_card *)arg;
	if (card == NULL) {
		syslog(LOG_ERR, "Invalid card from control event.");
		return;
	}

	/* handle_events will trigger the callback registered with each control
	 * that has changed. */
	snd_hctl_handle_events(card->hctl);
}

static int add_controls_and_iodevs_by_matching(
		struct cras_alsa_card_info *info,
		struct cras_device_blacklist *blacklist,
		struct cras_alsa_card *alsa_card,
		const char *card_name,
		snd_ctl_t *handle)
{
	struct mixer_name *coupled_controls = NULL;
	int dev_idx;
	snd_pcm_info_t *dev_info;
	struct mixer_name *extra_controls = NULL;
	int rc = 0;

	snd_pcm_info_alloca(&dev_info);

	if (alsa_card->ucm) {
		char *extra_main_volume;

		/* Filter the extra output mixer names */
		extra_controls =
			filter_controls(alsa_card->ucm,
				mixer_name_add(extra_controls, "IEC958",
					       CRAS_STREAM_OUTPUT,
					       MIXER_NAME_VOLUME));

		/* Get the extra main volume control. */
		extra_main_volume = ucm_get_flag(alsa_card->ucm,
						 "ExtraMainVolume");
		if (extra_main_volume) {
			extra_controls =
				mixer_name_add(extra_controls,
					       extra_main_volume,
					       CRAS_STREAM_OUTPUT,
					       MIXER_NAME_MAIN_VOLUME);
			free(extra_main_volume);
		}
		mixer_name_dump(extra_controls, "extra controls");

		/* Check if coupled controls has been specified for speaker. */
		coupled_controls = ucm_get_coupled_mixer_names(
					alsa_card->ucm, "Speaker");
		mixer_name_dump(coupled_controls, "coupled controls");
	}

	/* Add controls to mixer by name matching. */
	rc = cras_alsa_mixer_add_controls_by_name_matching(
			alsa_card->mixer,
			extra_controls,
			coupled_controls);
	if (rc) {
		syslog(LOG_ERR, "Fail adding controls to mixer for %s.",
		       alsa_card->name);
		goto error;
	}

	/* Go through every device. */
	dev_idx = -1;
	while (1) {
		rc = snd_ctl_pcm_next_device(handle, &dev_idx);
		if (rc < 0)
			goto error;
		if (dev_idx < 0)
			break;

		snd_pcm_info_set_device(dev_info, dev_idx);
		snd_pcm_info_set_subdevice(dev_info, 0);

		/* Check for playback devices. */
		snd_pcm_info_set_stream(
			dev_info, SND_PCM_STREAM_PLAYBACK);
		if (snd_ctl_pcm_info(handle, dev_info) == 0 &&
		    !should_ignore_dev(info, blacklist, dev_idx)) {
			struct cras_iodev *iodev =
				create_iodev_for_device(
					alsa_card,
					info,
					card_name,
					snd_pcm_info_get_name(dev_info),
					snd_pcm_info_get_id(dev_info),
					dev_idx,
					CRAS_STREAM_OUTPUT);
			if (iodev) {
				rc = alsa_iodev_legacy_complete_init(
					iodev);
				if (rc < 0)
					goto error;
			}
		}

		/* Check for capture devices. */
		snd_pcm_info_set_stream(
			dev_info, SND_PCM_STREAM_CAPTURE);
		if (snd_ctl_pcm_info(handle, dev_info) == 0) {
			struct cras_iodev *iodev =
				create_iodev_for_device(
					alsa_card,
					info,
					card_name,
					snd_pcm_info_get_name(dev_info),
					snd_pcm_info_get_id(dev_info),
					dev_idx,
					CRAS_STREAM_INPUT);
			if (iodev) {
				rc = alsa_iodev_legacy_complete_init(
					iodev);
				if (rc < 0)
					goto error;
			}
		}
	}
error:
	mixer_name_free(coupled_controls);
	mixer_name_free(extra_controls);
	return rc;
}

static int add_controls_and_iodevs_with_ucm(
		struct cras_alsa_card_info *info,
		struct cras_alsa_card *alsa_card,
		const char *card_name,
		snd_ctl_t *handle)
{
	snd_pcm_info_t *dev_info;
	struct iodev_list_node *node;
	int rc = 0;
	struct ucm_section *section;
	struct ucm_section *ucm_sections;

	snd_pcm_info_alloca(&dev_info);

	/* Get info on the devices specified in the UCM config. */
	ucm_sections = ucm_get_sections(alsa_card->ucm);
	if (!ucm_sections) {
		syslog(LOG_ERR,
		       "Could not retrieve any UCM SectionDevice"
		       " info for '%s'.", card_name);
		rc = -ENOENT;
		goto error;
	}

	/* Create all of the controls first. */
	DL_FOREACH(ucm_sections, section) {
		rc = cras_alsa_mixer_add_controls_in_section(
				alsa_card->mixer, section);
		if (rc) {
			syslog(LOG_ERR, "Failed adding controls to"
					" mixer for '%s:%s'",
					card_name,
					section->name);
			goto error;
		}
	}

	/* Create all of the devices. */
	DL_FOREACH(ucm_sections, section) {
		snd_pcm_info_set_device(dev_info, section->dev_idx);
		snd_pcm_info_set_subdevice(dev_info, 0);
		if (section->dir == CRAS_STREAM_OUTPUT)
			snd_pcm_info_set_stream(
				dev_info, SND_PCM_STREAM_PLAYBACK);
		else if (section->dir == CRAS_STREAM_INPUT)
			snd_pcm_info_set_stream(
				dev_info, SND_PCM_STREAM_CAPTURE);
		else {
			syslog(LOG_ERR, "Unexpected direction: %d",
			       section->dir);
			rc = -EINVAL;
			goto error;
		}

		if (snd_ctl_pcm_info(handle, dev_info)) {
			syslog(LOG_ERR,
			       "Could not get info for device: %s",
			       section->name);
			continue;
		}

		create_iodev_for_device(
			alsa_card, info, card_name,
			snd_pcm_info_get_name(dev_info),
			snd_pcm_info_get_id(dev_info),
			section->dev_idx, section->dir);
	}

	/* Setup jacks and controls for the devices. */
	DL_FOREACH(ucm_sections, section) {
		DL_FOREACH(alsa_card->iodevs, node) {
			if (node->direction == section->dir &&
			    alsa_iodev_index(node->iodev) ==
			    section->dev_idx)
				break;
		}
		if (node) {
			rc = alsa_iodev_ucm_add_nodes_and_jacks(
				node->iodev, section);
			if (rc < 0)
				goto error;
		}
	}

	DL_FOREACH(alsa_card->iodevs, node) {
		alsa_iodev_ucm_complete_init(node->iodev);
	}

error:
	ucm_section_free_list(ucm_sections);
	return rc;
}

/*
 * Exported Interface.
 */

struct cras_alsa_card *cras_alsa_card_create(
		struct cras_alsa_card_info *info,
		const char *device_config_dir,
		struct cras_device_blacklist *blacklist,
		const char *ucm_suffix)
{
	snd_ctl_t *handle = NULL;
	int rc, n;
	snd_ctl_card_info_t *card_info;
	const char *card_name;
	struct cras_alsa_card *alsa_card;

	if (info->card_index >= MAX_ALSA_CARDS) {
		syslog(LOG_ERR,
		       "Invalid alsa card index %u",
		       info->card_index);
		return NULL;
	}

	snd_ctl_card_info_alloca(&card_info);

	alsa_card = calloc(1, sizeof(*alsa_card));
	if (alsa_card == NULL)
		return NULL;
	alsa_card->card_index = info->card_index;

	snprintf(alsa_card->name,
		 MAX_ALSA_PCM_NAME_LENGTH,
		 "hw:%u",
		 info->card_index);

	rc = snd_ctl_open(&handle, alsa_card->name, 0);
	if (rc < 0) {
		syslog(LOG_ERR, "Fail opening control %s.", alsa_card->name);
		goto error_bail;
	}

	rc = snd_ctl_card_info(handle, card_info);
	if (rc < 0) {
		syslog(LOG_ERR, "Error getting card info.");
		goto error_bail;
	}

	card_name = snd_ctl_card_info_get_name(card_info);
	if (card_name == NULL) {
		syslog(LOG_ERR, "Error getting card name.");
		goto error_bail;
	}

	/* Read config file for this card if it exists. */
	alsa_card->config = cras_card_config_create(device_config_dir,
						    card_name);
	if (alsa_card->config == NULL)
		syslog(LOG_DEBUG, "No config file for %s", alsa_card->name);

	/* Create a use case manager if a configuration is available. */
	if (ucm_suffix) {
		char *ucm_name;
		if (asprintf(&ucm_name, "%s.%s", card_name, ucm_suffix) == -1) {
			syslog(LOG_ERR, "Error creating ucm name");
			goto error_bail;
		}
		alsa_card->ucm = ucm_create(ucm_name);
		syslog(LOG_INFO, "Card %s (%s) has UCM: %s",
		       alsa_card->name, ucm_name,
		       alsa_card->ucm ? "yes" : "no");
		free(ucm_name);
	} else {
		alsa_card->ucm = ucm_create(card_name);
		syslog(LOG_INFO, "Card %s (%s) has UCM: %s",
		       alsa_card->name, card_name,
		       alsa_card->ucm ? "yes" : "no");
	}

	rc = snd_hctl_open(&alsa_card->hctl,
			   alsa_card->name,
			   SND_CTL_NONBLOCK);
	if (rc < 0) {
		syslog(LOG_DEBUG,
		       "failed to get hctl for %s", alsa_card->name);
		alsa_card->hctl = NULL;
	} else {
		rc = snd_hctl_nonblock(alsa_card->hctl, 1);
		if (rc < 0) {
			syslog(LOG_ERR,
			    "failed to nonblock hctl for %s", alsa_card->name);
			goto error_bail;
		}

		rc = snd_hctl_load(alsa_card->hctl);
		if (rc < 0) {
			syslog(LOG_ERR,
			       "failed to load hctl for %s", alsa_card->name);
			goto error_bail;
		}
	}

	/* Create one mixer per card. */
	alsa_card->mixer = cras_alsa_mixer_create(alsa_card->name);

	if (alsa_card->mixer == NULL) {
		syslog(LOG_ERR, "Fail opening mixer for %s.", alsa_card->name);
		goto error_bail;
	}

	if (alsa_card->ucm && ucm_has_fully_specified_ucm_flag(alsa_card->ucm))
		rc = add_controls_and_iodevs_with_ucm(
				info, alsa_card, card_name, handle);
	else
		rc = add_controls_and_iodevs_by_matching(
				info, blacklist, alsa_card, card_name, handle);
	if (rc)
		goto error_bail;

	n = alsa_card->hctl ?
		snd_hctl_poll_descriptors_count(alsa_card->hctl) : 0;
	if (n != 0 && card_has_hctl_jack(alsa_card)) {
		struct hctl_poll_fd *registered_fd;
		struct pollfd *pollfds;
		int i;

		pollfds = malloc(n * sizeof(*pollfds));
		if (pollfds == NULL) {
			rc = -ENOMEM;
			goto error_bail;
		}

		n = snd_hctl_poll_descriptors(alsa_card->hctl, pollfds, n);
		for (i = 0; i < n; i++) {
			registered_fd = calloc(1, sizeof(*registered_fd));
			if (registered_fd == NULL) {
				free(pollfds);
				rc = -ENOMEM;
				goto error_bail;
			}
			registered_fd->fd = pollfds[i].fd;
			DL_APPEND(alsa_card->hctl_poll_fds, registered_fd);
			rc = cras_system_add_select_fd(
					registered_fd->fd,
					alsa_control_event_pending,
					alsa_card);
			if (rc < 0) {
				DL_DELETE(alsa_card->hctl_poll_fds,
					  registered_fd);
				free(pollfds);
				goto error_bail;
			}
		}
		free(pollfds);
	}

	snd_ctl_close(handle);
	return alsa_card;

error_bail:
	if (handle != NULL)
		snd_ctl_close(handle);
	cras_alsa_card_destroy(alsa_card);
	return NULL;
}

void cras_alsa_card_destroy(struct cras_alsa_card *alsa_card)
{
	struct iodev_list_node *curr;
	struct hctl_poll_fd *poll_fd;

	if (alsa_card == NULL)
		return;

	DL_FOREACH(alsa_card->iodevs, curr) {
		alsa_iodev_destroy(curr->iodev);
		DL_DELETE(alsa_card->iodevs, curr);
		free(curr);
	}
	DL_FOREACH(alsa_card->hctl_poll_fds, poll_fd) {
		cras_system_rm_select_fd(poll_fd->fd);
		DL_DELETE(alsa_card->hctl_poll_fds, poll_fd);
		free(poll_fd);
	}
	if (alsa_card->hctl)
		snd_hctl_close(alsa_card->hctl);
	if (alsa_card->ucm)
		ucm_destroy(alsa_card->ucm);
	if (alsa_card->mixer)
		cras_alsa_mixer_destroy(alsa_card->mixer);
	if (alsa_card->config)
		cras_card_config_destroy(alsa_card->config);
	free(alsa_card);
}

size_t cras_alsa_card_get_index(const struct cras_alsa_card *alsa_card)
{
	assert(alsa_card);
	return alsa_card->card_index;
}
