/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Handles finding and monitoring ALSA Jack controls.  These controls represent
 * external jacks and report back when the plugged state of teh hack changes.
 */

#ifndef CRAS_ALSA_JACK_H_
#define CRAS_ALSA_JACK_H_

#include "cras_types.h"
#include "cras_alsa_ucm.h"

struct cras_alsa_jack;
struct cras_alsa_jack_list;
struct cras_alsa_mixer;

/* Callback type for users of jack_list to define, it will be called when the
 * jack state changes.
 * Args:
 *    jack - The jack that has changed.
 *    plugged - non-zero if the jack is attached.
 *    data - User defined pointer passed to cras_alsa_jack_create.
 */
typedef void (jack_state_change_callback)(const struct cras_alsa_jack *jack,
					  int plugged,
					  void *data);

/* Creates a jack list. The jacks can be added later by name matching or
 * fully specified UCM.
 * Args:
 *    card_index - Index ALSA uses to refer to the card.  The X in "hw:X".
 *    card_name - The name of the card (used to find gpio jacks).
 *    device_index - Index ALSA uses to refer to the device.  The Y in "hw:X".
 *    is_first_device - whether this device is the first device on the card.
 *    mixer - The mixer associated with this card, used to find controls that
 *      correspond to jacks.  For instance "Headphone switch" for "Headphone
 *      Jack".
 *    ucm - CRAS use case manager if available.
 *    hctl - ALSA high-level control interface if available.
 *    direction - Input or output, look for mic or headphone jacks.
 *    cb - Function to call when a jack state changes.
 *    cb_data - Passed to the callback when called.
 * Returns:
 *    A pointer to a new jack list on success, NULL if there is a failure.
 */
struct cras_alsa_jack_list *cras_alsa_jack_list_create(
		unsigned int card_index,
		const char *card_name,
		unsigned int device_index,
		int is_first_device,
		struct cras_alsa_mixer *mixer,
		struct cras_use_case_mgr *ucm,
		snd_hctl_t *hctl,
		enum CRAS_STREAM_DIRECTION direction,
		jack_state_change_callback *cb,
		void *cb_data);

/* Finds jacks by name matching.
 * The list holds all the interesting ALSA jacks for this
 * device. These jacks will be for headphones, speakers, HDMI, etc.
 * Args:
 *   jack_list - A pointer to a jack list.
 * Returns:
 *   0 on success. Error code if there is a failure.
 */
int cras_alsa_jack_list_find_jacks_by_name_matching(
	struct cras_alsa_jack_list *jack_list);

/* Add the jack defined by the UCM section information.
 * Args:
 *   jack_list - A pointer to a jack list.
 *   ucm_section - UCM section data.
 *   result_jack - Resulting jack that was added.
 * Returns:
 *   0 on success. Error code if there is a failure.
 */
int cras_alsa_jack_list_add_jack_for_section(
	struct cras_alsa_jack_list *jack_list,
	struct ucm_section *ucm_section,
	struct cras_alsa_jack **result_jack);

/* Destroys a jack list created with cras_alsa_jack_list_create.
 * Args:
 *    jack_list - The list to destroy.
 */
void cras_alsa_jack_list_destroy(struct cras_alsa_jack_list *jack_list);

/* Returns non-zero if the jack list has hctl jacks.
 * Args:
 *    jack_list - The list check.
 */
int cras_alsa_jack_list_has_hctl_jacks(struct cras_alsa_jack_list *jack_list);

/* Gets the mixer output associated with the given jack.
 * Args:
 *    jack - The jack to query for a mixer output.
 * Returns:
 *    A pointer to the mixer output if it exists, otherwise NULL.
 */
struct mixer_control *cras_alsa_jack_get_mixer_output(
		const struct cras_alsa_jack *jack);

/* Gets the mixer input associated with given jack.
 * Args:
 *    jack - The jack to query for a mixer input.
 * Returns:
 *    A pointer to the mixer input if it exists, otherwise NULL.
 */
struct mixer_control *cras_alsa_jack_get_mixer_input(
		const struct cras_alsa_jack *jack);

/* Query all jacks in the list and report the state to the callback.
 * Args:
 *    jack_list - The jack list to query.
 */
void cras_alsa_jack_list_report(const struct cras_alsa_jack_list *jack_list);

/* Gets the name of a jack.
 * Args:
 *    jack_list - The jack list to query.
 */
const char *cras_alsa_jack_get_name(const struct cras_alsa_jack *jack);

/* Gets the ucm device of a jack.
 * Args:
 *    jack - The alsa jack.
 */
const char *cras_alsa_jack_get_ucm_device(const struct cras_alsa_jack *jack);


void cras_alsa_jack_update_monitor_name(const struct cras_alsa_jack *jack,
					char *name_buf,
					unsigned int buf_size);

/* Updates the node type according to override_type_name in jack.
 * Currently this method only supports updating the node type to
 * CRAS_NODE_TYPE_INTERNAL_SPEAKER when override_type_name is
 * "Internal Speaker". This is used in All-In-One device where
 * output is an HDMI device, but it should be internal speaker from
 * user point of view.
 * Args:
 *    jack - The jack to query node type.
 *    type - The node type to be overwritten.
 */
void cras_alsa_jack_update_node_type(const struct cras_alsa_jack *jack,
				     enum CRAS_NODE_TYPE *type);

/* Gets the dsp name of a jack.
 * Args:
 *    jack_list - The jack list to query.
 */
const char *cras_alsa_jack_get_dsp_name(const struct cras_alsa_jack *jack);

/* Enables the ucm device for this jack if any.
 * Args:
 *    jack - The jack to query for a mixer output.
 */
void cras_alsa_jack_enable_ucm(const struct cras_alsa_jack *jack, int enable);


/* Find out whether the specified card has a jack with the given name.
 * Args:
 *    card_index - Index ALSA uses to refer to the card.  The X in "hw:X".
 *    jack_name - The name of the jack (for example, "Speaker Phantom Jack").
 */
int cras_alsa_jack_exists(unsigned int card_index, const char *jack_name);

#endif /* CRAS_ALSA_JACK_H_ */
