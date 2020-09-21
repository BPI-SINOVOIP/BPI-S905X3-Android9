/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <stdlib.h>

#include "cras_volume_curve.h"
#include "softvol_curve.h"

/* This is a ramp that increases 0.5dB per step, for a total range of 50dB. */
const float softvol_scalers[101] = {
	0.003162, /* volume 0 */
	0.003350,
	0.003548,
	0.003758,
	0.003981,
	0.004217,
	0.004467,
	0.004732,
	0.005012,
	0.005309,
	0.005623,
	0.005957,
	0.006310,
	0.006683,
	0.007079,
	0.007499,
	0.007943,
	0.008414,
	0.008913,
	0.009441,
	0.010000,
	0.010593,
	0.011220,
	0.011885,
	0.012589,
	0.013335,
	0.014125,
	0.014962,
	0.015849,
	0.016788,
	0.017783,
	0.018836,
	0.019953,
	0.021135,
	0.022387,
	0.023714,
	0.025119,
	0.026607,
	0.028184,
	0.029854,
	0.031623,
	0.033497,
	0.035481,
	0.037584,
	0.039811,
	0.042170,
	0.044668,
	0.047315,
	0.050119,
	0.053088,
	0.056234,
	0.059566,
	0.063096,
	0.066834,
	0.070795,
	0.074989,
	0.079433,
	0.084140,
	0.089125,
	0.094406,
	0.100000,
	0.105925,
	0.112202,
	0.118850,
	0.125893,
	0.133352,
	0.141254,
	0.149624,
	0.158489,
	0.167880,
	0.177828,
	0.188365,
	0.199526,
	0.211349,
	0.223872,
	0.237137,
	0.251189,
	0.266073,
	0.281838,
	0.298538,
	0.316228,
	0.334965,
	0.354813,
	0.375837,
	0.398107,
	0.421697,
	0.446684,
	0.473151,
	0.501187,
	0.530884,
	0.562341,
	0.595662,
	0.630957,
	0.668344,
	0.707946,
	0.749894,
	0.794328,
	0.841395,
	0.891251,
	0.944061,
	1.000000, /* volume 100 */
};

float *softvol_build_from_curve(const struct cras_volume_curve *curve)
{
	float *scalers;
	unsigned int volume;

	if (!curve)
		return NULL;

	scalers = (float *)malloc(NUM_VOLUME_STEPS * sizeof(float));
	if (!scalers)
		return NULL;

	/* When software volume is used, it is assumed all volume curve values
	 * are relative to 0 dBFS when converting to scale. If a positive dBFS
	 * value is specified in curve config, it will be treated as invalid
	 * and clip to 1.0 in scale.
	 */
	for (volume = 0; volume <= MAX_VOLUME; volume++) {
		scalers[volume] = convert_softvol_scaler_from_dB(
				curve->get_dBFS(curve, volume));
		if (scalers[volume] > 1.0)
			scalers[volume] = 1.0;
	}

	return scalers;
}
