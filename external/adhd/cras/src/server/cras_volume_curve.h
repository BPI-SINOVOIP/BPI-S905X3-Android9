/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_VOLUME_CURVE_H_
#define CRAS_VOLUME_CURVE_H_

#define MAX_VOLUME 100
#define NUM_VOLUME_STEPS (MAX_VOLUME + 1) /* 0-100 inclusive. */

/* Holds the function that converts from a volume index to a dBFS value. */
struct cras_volume_curve {
	/* Function to convert from index to dBFS value.
	 * Args:
	 *    curve - A curve from cras_volume_curve_create_* functions.
	 *    volume - The volume level from 0 to 100.
	 * Returns:
	 *    The volume to apply in dB * 100.  This value will normally be
	 *    negative and is means dB down from full scale.
	 */
	long (*get_dBFS)(const struct cras_volume_curve *curve, size_t volume);
};

/* Creates a system-default volume curve. The default curve maps one volume step
 * to 1 dB down.
 * Returns null on error, or the new volume curve on success.
 */
struct cras_volume_curve *cras_volume_curve_create_default();

/* Creates a volume curve with a specified max volume and step.
 * Args:
 *    max_volume - Maximum volume allowed in dBFS.
 *    volume_step - Number of dB to change for one volume tick.
 */
struct cras_volume_curve *cras_volume_curve_create_simple_step(
		long max_volume,
		long volume_step);

/* Creates a volume curve with each step's dB value called out.
 * Args:
 *    dB_values - Each element specifies what the volume should be set to (in
 *      dB) for the volume at that index.
 * Returns:
 *    A volume curve pointer that should  be passed to
 *    cras_volume_curve_destroy() when it is no longer needed. If there is an
 *    error NULL will be returned.
 */
struct cras_volume_curve *cras_volume_curve_create_explicit(
		long dB_values[101]);

/* Destroys a curve created with cras_volume_curve_create_*.
 * Args:
 *    curve - The curve to destroy.
 */
void cras_volume_curve_destroy(struct cras_volume_curve *curve);

#endif /* CRAS_VOLUME_CURVE_H_ */
