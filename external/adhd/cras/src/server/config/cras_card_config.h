/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_CARD_CONFIG_H_
#define CRAS_CARD_CONFIG_H_

struct cras_card_config;
struct cras_volume_curve;

/* Creates a configuration based on the config file specified.
 * Args:
 *    config_path - Path containing the config files.
 *    card_name - Name of the card to load a configuration for.
 * Returns:
 *    A pointer to the created config on success, NULL on failure.
 */
struct cras_card_config *cras_card_config_create(const char *config_path,
						 const char *card_name);

/* Destroys a configuration returned by cras_card_config_create().
 * Args:
 *    card_config - Card configuration returned by cras_card_config_create()
 */
void cras_card_config_destroy(struct cras_card_config *card_config);

/* Returns the apporpriate volume curve to use for the control given by name.
 * Args:
 *    card_config - Card configuration returned by cras_card_config_create()
 * Returns:
 *    The specialized curve for the control if there is one, otherwise NULL.
 */
struct cras_volume_curve *cras_card_config_get_volume_curve_for_control(
		const struct cras_card_config *card_config,
		const char *control_name);

#endif /* CRAS_CARD_CONFIG_H_ */
