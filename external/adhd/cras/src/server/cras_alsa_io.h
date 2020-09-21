/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_ALSA_IO_H_
#define CRAS_ALSA_IO_H_

#include <alsa/asoundlib.h>

#include "cras_card_config.h"
#include "cras_types.h"

struct cras_alsa_mixer;
struct cras_ionode;
struct cras_use_case_mgr;
struct ucm_section;

/* Initializes an alsa iodev.
 * Args:
 *    card_index - 0 based index, value of "XX" in "hw:XX,YY".
 *    card_name - The name of the card.
 *    device_index - 0 based index, value of "YY" in "hw:XX,YY".
 *    dev_name - The name of the device.
 *    dev_id - The id string of the device.
 *    card_type - the type of the card this iodev belongs.
 *    is_first - if this is the first iodev on the card.
 *    mixer - The mixer for the alsa device.
 *    config - Card config for this alsa device.
 *    ucm - CRAS use case manager if available.
 *    hctl - high-level control manager if available.
 *    direction - input or output.
 *    usb_vid - vendor ID of USB device.
 *    usb_pid - product ID of USB device.
 *    usb_serial_number - serial number of USB device.
 * Returns:
 *    A pointer to the newly created iodev if successful, NULL otherwise.
 */
struct cras_iodev *alsa_iodev_create(size_t card_index,
				     const char *card_name,
				     size_t device_index,
				     const char *dev_name,
				     const char *dev_id,
				     enum CRAS_ALSA_CARD_TYPE card_type,
				     int is_first,
				     struct cras_alsa_mixer *mixer,
				     const struct cras_card_config *config,
				     struct cras_use_case_mgr *ucm,
				     snd_hctl_t *hctl,
				     enum CRAS_STREAM_DIRECTION direction,
				     size_t usb_vid,
				     size_t usb_pid,
				     char *usb_serial_number);

/* Complete initializeation of this iodev with the legacy method.
 * Add IO nodes and find jacks for this iodev with magic sauce, then choose
 * the current active node.
 * Args:
 *    iodev - ALSA io device associated with the IO nodes.
 *    section - UCM section information if available (or NULL).
 * Returns:
 *    0 for success, negative error code on error.
 */
int alsa_iodev_legacy_complete_init(struct cras_iodev *iodev);

/* Add IO nodes and jacks for this iodev using UCM data.
 * Args:
 *    iodev - ALSA io device associated with the given section.
 *    section - UCM section information.
 * Returns:
 *    0 for success, negative error code on error.
 */
int alsa_iodev_ucm_add_nodes_and_jacks(struct cras_iodev *iodev,
				       struct ucm_section *section);

/* Complete initialization of this iodev with fully-spec UCM data.
 * After all UCM devices associated with the same iodev have been processed
 * this is called to finish iodev setup.
 * Args:
 *    iodev - ALSA io device.
 */
void alsa_iodev_ucm_complete_init(struct cras_iodev *iodev);

/* Destroys an alsa_iodev created with alsa_iodev_create. */
void alsa_iodev_destroy(struct cras_iodev *iodev);

/* Returns the ALSA device index for the given ALSA iodev. */
unsigned alsa_iodev_index(struct cras_iodev *iodev);

/* Returns whether this IODEV has ALSA hctl jacks. */
int alsa_iodev_has_hctl_jacks(struct cras_iodev *iodev);

#endif /* CRAS_ALSA_IO_H_ */
