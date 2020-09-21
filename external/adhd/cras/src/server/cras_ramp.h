/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_RAMP_H_
#define CRAS_RAMP_H_

#include "cras_iodev.h"

struct cras_ramp;

/*
 * Infomation telling user how to do ramping.
 * action CRAS_RAMP_ACTION_NONE: No scale should be applied.
 * action CRAS_RAMP_ACTION_PARTIAL: scale sample by sample starting from scaler
 *                                  and increase increment for each sample.
 * action CRAS_RAMP_ACTION_INVALID: There is an error in cras_ramp.
 */
enum CRAS_RAMP_ACTION_TYPE {
	CRAS_RAMP_ACTION_NONE,
	CRAS_RAMP_ACTION_PARTIAL,
	CRAS_RAMP_ACTION_INVALID,
};

/*
 * Struct to hold current ramping action for user.
 * Members:
 *   type: See CRAS_RAMP_ACTION_TYPE.
 *   scaler: The initial scaler to be applied.
 *   increment: The scaler increment that should be added to scaler for every
 *              frame.
 */
struct cras_ramp_action {
	enum CRAS_RAMP_ACTION_TYPE type;
	float scaler;
	float increment;
};

typedef void (*cras_ramp_cb)(void *arg);

/* Creates a ramp. */
struct cras_ramp* cras_ramp_create();

/* Destroys a ramp. */
void cras_ramp_destroy(struct cras_ramp* ramp);

/* Starts ramping up from 0 to 1 or from 1 to 0 for duration_frames frames.
 * Args:
 *   ramp[in]: The ramp struct to start.
 *   is_up[in]: 1 to ramp up and 0 to ramp down.
 *   duration_frames[in]: Ramp duration in frames.
 *   cb[in]: The callback function to call after ramping is done. User can set
 *           cb to turn off speaker/headphone switch after ramping down
 *           is done.
 *   cb_data[in]: The data passed to callback function.
 * Returns:
 *   0 on success; negative error code on failure.
 */
int cras_ramp_start(struct cras_ramp *ramp, int is_up, int duration_frames,
		    cras_ramp_cb cb, void *cb_data);

/* Resets ramp and cancels current ramping. */
int cras_ramp_reset(struct cras_ramp *ramp);

/* Gets current ramp action. */
struct cras_ramp_action cras_ramp_get_current_action(
		const struct cras_ramp *ramp);

/* Updates number of samples that went through ramping. */
int cras_ramp_update_ramped_frames(
		struct cras_ramp *ramp, int num_frames);

#endif /* CRAS_RAMP_H_ */
