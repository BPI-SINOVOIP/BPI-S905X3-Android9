/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _CRAS_ALSA_MIXER_H
#define _CRAS_ALSA_MIXER_H

#include <alsa/asoundlib.h>
#include <iniparser.h>

#include "cras_types.h"

/* cras_alsa_mixer represents the alsa mixer interface for an alsa card.  It
 * houses the volume and mute controls as well as playback switches for
 * headphones and mic.
 */

struct mixer_control;
struct cras_alsa_mixer;
struct cras_volume_curve;
struct cras_card_config;
struct mixer_name;
struct ucm_section;

/* Creates a cras_alsa_mixer instance for the given alsa device.
 * Args:
 *    card_name - Name of the card to open a mixer for.  This is an alsa name of
 *      the form "hw:X" where X ranges from 0 to 31 inclusive.
 * Returns:
 *    A pointer to the newly created cras_alsa_mixer which must later be freed
 *    by calling cras_alsa_mixer_destroy. The control in the mixer is not added
 *    yet.
 */
struct cras_alsa_mixer *cras_alsa_mixer_create(const char *card_name);

/* Adds controls to a cras_alsa_mixer from the given UCM section.
 * Args:
 *    cmix - A pointer to cras_alsa_mixer.
 *    section - A UCM section.
 * Returns:
 *    0 on success. Negative error code otherwise.
 */
int cras_alsa_mixer_add_controls_in_section(
		struct cras_alsa_mixer *cmix,
		struct ucm_section *section);

/* Adds controls to a cras_alsa_mixer instance by name matching.
 * Args:
 *    cmix - A pointer to cras_alsa_mixer.
 *    extra_controls - A list array of extra mixer control names, always added.
 *    coupled_controls - A list of coupled mixer control names.
 * Returns:
 *    0 on success. Other error code if error happens.
 */
int cras_alsa_mixer_add_controls_by_name_matching(
		struct cras_alsa_mixer *cmix,
		struct mixer_name *extra_controls,
		struct mixer_name *coupled_controls);

/* Destroys a cras_alsa_mixer that was returned from cras_alsa_mixer_create.
 * Args:
 *    cras_mixer - The cras_alsa_mixer pointer returned from
 *        cras_alsa_mixer_create.
 */
void cras_alsa_mixer_destroy(struct cras_alsa_mixer *cras_mixer);

/* Returns if the mixer has any main volume control. */
int cras_alsa_mixer_has_main_volume(const struct cras_alsa_mixer *cras_mixer);

/* Returns if the mixer control supports volume adjust. */
int cras_alsa_mixer_has_volume(const struct mixer_control *mixer_control);

/* Sets the output volume for the device associated with this mixer.
 * Args:
 *    cras_mixer - The mixer to set the volume on.
 *    dBFS - The volume level as dB * 100.  dB is a normally a negative quantity
 *      specifying how much to attenuate.
 *    mixer_output - The mixer output to set if not all attenuation can be
 *      obtained from the main controls.  Can be null.
 */
void cras_alsa_mixer_set_dBFS(struct cras_alsa_mixer *cras_mixer,
			      long dBFS,
			      struct mixer_control *mixer_output);

/* Gets the volume range of the mixer in dB.
 * Args:
 *    cras_mixer - The mixer to get the volume range.
 */
long cras_alsa_mixer_get_dB_range(struct cras_alsa_mixer *cras_mixer);

/* Gets the volume range of the mixer output in dB.
 * Args:
 *    mixer_output - The mixer output to get the volume range.
 */
long cras_alsa_mixer_get_output_dB_range(
		struct mixer_control *mixer_output);

/* Sets the capture gain for the device associated with this mixer.
 * Args:
 *    cras_mixer - The mixer to set the volume on.
 *    dBFS - The capture gain level as dB * 100.  dB can be a positive or a
 *    negative quantity specifying how much gain or attenuation to apply.
 *    mixer_input - The specific mixer control for input node, can be null.
 */
void cras_alsa_mixer_set_capture_dBFS(struct cras_alsa_mixer *cras_mixer,
				      long dBFS,
				      struct mixer_control* mixer_input);

/* Gets the minimum allowed setting for capture gain.
 * Args:
 *    cmix - The mixer to set the capture gain on.
 *    mixer_input - The additional input mixer control, mainly specified
 *      in ucm config. Can be null.
 * Returns:
 *    The minimum allowed capture gain in dBFS * 100.
 */
long cras_alsa_mixer_get_minimum_capture_gain(
                struct cras_alsa_mixer *cmix,
		struct mixer_control *mixer_input);

/* Gets the maximum allowed setting for capture gain.
 * Args:
 *    cmix - The mixer to set the capture gain on.
 *    mixer_input - The additional input mixer control, mainly specified
 *      in ucm config. Can be null.
 * Returns:
 *    The maximum allowed capture gain in dBFS * 100.
 */
long cras_alsa_mixer_get_maximum_capture_gain(
		struct cras_alsa_mixer *cmix,
		struct mixer_control *mixer_input);

/* Sets the playback switch for the device.
 * Args:
 *    cras_mixer - Mixer to set the playback switch.
 *    muted - 1 if muted, 0 if not.
 *    mixer_output - The output specific mixer control to mute/unmute. Pass NULL
 *                   to skip it.
 */
void cras_alsa_mixer_set_mute(struct cras_alsa_mixer *cras_mixer,
			      int muted,
			      struct mixer_control *mixer_output);

/* Sets the capture switch for the device.
 * Args:
 *    cras_mixer - Mixer to set the volume in.
 *    muted - 1 if muted, 0 if not.
 *    mixer_input - The mixer input to mute if no master mute.
 */
void cras_alsa_mixer_set_capture_mute(struct cras_alsa_mixer *cras_mixer,
				      int muted,
				      struct mixer_control *mixer_input);

/* Invokes the provided callback once for each output (input).
 * The callback will be provided with a reference to the control
 * that can be queried to see what the control supports.
 * Args:
 *    cras_mixer - Mixer to set the volume in.
 *    cb - Function to call for each output (input).
 *    cb_arg - Argument to pass to cb.
 */
typedef void (*cras_alsa_mixer_control_callback)(
		struct mixer_control *control, void *arg);
void cras_alsa_mixer_list_outputs(struct cras_alsa_mixer *cras_mixer,
				  cras_alsa_mixer_control_callback cb,
				  void *cb_arg);

void cras_alsa_mixer_list_inputs(struct cras_alsa_mixer *cras_mixer,
				 cras_alsa_mixer_control_callback cb,
				 void *cb_arg);

/* Gets the name of a given control. */
const char *cras_alsa_mixer_get_control_name(
		const struct mixer_control *control);

/* Returns the mixer control matching the given direction and name.
 * Args:
 *    cras_mixer - Mixer to search for a control.
 *    dir - Control's direction (OUTPUT or INPUT).
 *    name - Name to search for.
 *    create_missing - When non-zero, attempt to create a new control with
 *		       the given name.
 * Returns:
 *    A pointer to the matching mixer control, or NULL if none found.
 */
struct mixer_control *cras_alsa_mixer_get_control_matching_name(
		struct cras_alsa_mixer *cras_mixer,
		enum CRAS_STREAM_DIRECTION dir, const char *name,
		int create_missing);

/* Returns the mixer control associated with the given section.
 * The control is the one that matches 'mixer_name', or if that is not defined
 * then it will be the control matching 'section->name', based on the
 * coupled mixer controls.
 * Args:
 *    cras_mixer - Mixer to search for a control.
 *    section - Associated UCM section.
 * Returns:
 *    A pointer to the associated mixer control, or NULL if none found.
 */
struct mixer_control *cras_alsa_mixer_get_control_for_section(
		struct cras_alsa_mixer *cras_mixer,
		const struct ucm_section *section);

/* Finds the output that matches the given string.  Used to match Jacks to Mixer
 * elements.
 * Args:
 *    cras_mixer - Mixer to search for a control.
 *    name - The name to match against the controls.
 * Returns:
 *    A pointer to the output with a mixer control that matches "name".
 */
struct mixer_control *cras_alsa_mixer_get_output_matching_name(
		struct cras_alsa_mixer *cras_mixer,
		const char *name);

/* Finds the mixer control for that matches the control name of input control
 * name specified in ucm config.
 * Args:
 *    cras_mixer - Mixer to search for a control.
 *    name - Name of the control to search for.
 * Returns:
 *    A pointer to the input with a mixer control that matches "name".
 */
struct mixer_control *cras_alsa_mixer_get_input_matching_name(
		struct cras_alsa_mixer *cras_mixer,
		const char *name);

/* Sets the given output active or inactive. */
int cras_alsa_mixer_set_output_active_state(
		struct mixer_control *output,
		int active);

#endif /* _CRAS_ALSA_MIXER_H */
